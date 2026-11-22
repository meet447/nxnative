#include "nvn_astcenc.h"
#include "../../third_party/astcenc/astcenc.h"
#include <cstdlib>
#include <string.h>

enum { RGBA16Float = 115, ASTC_SRGB0 = 186, ASTC_LDR0 = 204 };

static int pf_srgb(uint32_t pf) { return pf >= ASTC_SRGB0 && pf < ASTC_LDR0; }

int nvn_bc_astc_profile(uint32_t nvn_fmt, nvn_astc_profile *out) {
    if (!out) return 0;
    out->block_w = 4;
    out->block_h = 4;
    out->is_hdr = 0;
    switch (nvn_fmt) {
    case 0x41: case 0x42:
        out->mtl_pf = ASTC_LDR0 + 4;
        out->block_w = out->block_h = 6;
        return 1;
    case 0x45: case 0x46:
        out->mtl_pf = ASTC_SRGB0 + 4;
        out->block_w = out->block_h = 6;
        return 1;
    case 0x43: case 0x44:
        out->mtl_pf = ASTC_LDR0;
        return 1;
    case 0x47: case 0x48:
        out->mtl_pf = ASTC_SRGB0;
        return 1;
    case 0x49: case 0x4a: case 0x4b: case 0x4c:
        out->mtl_pf = ASTC_LDR0;
        return 1;
    case 0x4d:
        out->mtl_pf = ASTC_LDR0;
        return 1;
    case 0x4e:
        out->mtl_pf = ASTC_SRGB0;
        return 1;
    case 0x4f: case 0x50:
        out->mtl_pf = RGBA16Float;
        out->is_hdr = 1;
        return 1;
    default:
        return 0;
    }
}

int nvn_astc_encode_rgba8(const nvn_astc_profile *prof, const uint8_t *src, size_t src_pitch,
                          uint32_t w, uint32_t h, uint8_t *dst, size_t dst_pitch) {
    if (!prof || !src || !dst || !w || !h) return -1;
    astcenc_profile apr = pf_srgb(prof->mtl_pf) ? ASTCENC_PRF_LDR_SRGB : ASTCENC_PRF_LDR;
    astcenc_config config;
    astcenc_error st = astcenc_config_init(apr, (unsigned)prof->block_w, (unsigned)prof->block_h, 1,
                                           ASTCENC_PRE_FAST, 0, &config);
    if (st != ASTCENC_SUCCESS) return -1;
    astcenc_context *ctx = nullptr;
    st = astcenc_context_alloc(&config, 1, &ctx);
    if (st != ASTCENC_SUCCESS) return -1;
    void *plane = (void *)src;
    void *slices[1] = { plane };
    astcenc_image image{};
    image.dim_x = w;
    image.dim_y = h;
    image.dim_z = 1;
    image.data_type = ASTCENC_TYPE_U8;
    image.data = slices;
    astcenc_swizzle swz{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
    unsigned bx = (w + (unsigned)prof->block_w - 1) / (unsigned)prof->block_w;
    unsigned by = (h + (unsigned)prof->block_h - 1) / (unsigned)prof->block_h;
    size_t rowsz = (size_t)bx * 16;
    size_t need = rowsz * by;
    if (dst_pitch < rowsz) {
        astcenc_context_free(ctx);
        return -1;
    }
    (void)src_pitch;
    uint8_t stack[4096];
    uint8_t *tight = need <= sizeof stack ? stack : (uint8_t *)malloc(need);
    if (!tight) {
        astcenc_context_free(ctx);
        return -1;
    }
    st = astcenc_compress_image(ctx, &image, &swz, tight, need, 0);
    astcenc_context_free(ctx);
    if (st != ASTCENC_SUCCESS) {
        if (tight != stack) free(tight);
        return -1;
    }
    for (unsigned y = 0; y < by; y++)
        memcpy(dst + (size_t)y * dst_pitch, tight + (size_t)y * rowsz, rowsz);
    if (tight != stack) free(tight);
    return 0;
}