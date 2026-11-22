// BC → ASTC transcode for GPUs without BC (A16 fork): packed ASTC host textures instead of unpacked RGBA.
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t mtl_pf; // Metal pixel format enum value
    int block_w, block_h;
    int is_hdr;      // 1 for BC6H → keep RGBA16F, do not use ASTC LDR
} nvn_astc_profile;

// Returns 1 if nvn_fmt is BC and fills *out. BC6H sets is_hdr=1 and mtl_pf=RGBA16Float (115).
int nvn_bc_astc_profile(uint32_t nvn_fmt, nvn_astc_profile *out);

// Encode tightly packed RGBA8 (pitch = width*4) into ASTC blocks. dst_pitch = ((w+bw-1)/bw)*16 bytes per block-row.
// Returns 0 on success.
int nvn_astc_encode_rgba8(const nvn_astc_profile *prof, const uint8_t *src, size_t src_pitch,
                         uint32_t w, uint32_t h, uint8_t *dst, size_t dst_pitch);

#ifdef __cplusplus
}
#endif