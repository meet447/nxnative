// Unit tests for nvn_bc_astc_profile and nvn_astc_encode_rgba8.
#include "../nvn_astcenc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    return 1;
}

static int expect_profile(uint32_t fmt, int ok, uint32_t mtl_pf, int bw, int bh, int hdr) {
    nvn_astc_profile p;
    memset(&p, 0xcd, sizeof p);
    int r = nvn_bc_astc_profile(fmt, &p);
    if (r != ok)
        return fail("profile return mismatch");
    if (!ok) return 0;
    if (p.mtl_pf != mtl_pf || p.block_w != bw || p.block_h != bh || p.is_hdr != hdr)
        return fail("profile fields mismatch");
    return 0;
}

int main(void) {
    if (expect_profile(0x41, 1, 208, 6, 6, 0)) return 1;
    if (expect_profile(0x45, 1, 190, 6, 6, 0)) return 1;
    if (expect_profile(0x44, 1, 204, 4, 4, 0)) return 1;
    if (expect_profile(0x4d, 1, 204, 4, 4, 0)) return 1;
    if (expect_profile(0x4f, 1, 115, 4, 4, 1)) return 1;
    if (expect_profile(0x25, 0, 0, 0, 0, 0)) return 1;

    nvn_astc_profile prof;
    if (!nvn_bc_astc_profile(0x41, &prof)) return fail("0x41 profile");

    uint8_t rgba[8 * 8 * 4];
    memset(rgba, 0, sizeof rgba);
    for (int i = 0; i < 64; i++) {
        rgba[i * 4] = 200;
        rgba[i * 4 + 1] = 40;
        rgba[i * 4 + 2] = 80;
        rgba[i * 4 + 3] = 255;
    }
    unsigned bx = (8u + (unsigned)prof.block_w - 1u) / (unsigned)prof.block_w;
    unsigned by = (8u + (unsigned)prof.block_h - 1u) / (unsigned)prof.block_h;
    size_t pitch = bx * 16;
    uint8_t *blk = (uint8_t *)malloc(pitch * by);
    if (!blk) return fail("oom");
    memset(blk, 0, pitch * by);
    if (nvn_astc_encode_rgba8(&prof, rgba, 32, 8, 8, blk, pitch) != 0) {
        free(blk);
        return fail("8x8 6x6 encode failed");
    }
    int nz = 0;
    for (size_t i = 0, n = pitch * by; i < n; i++) nz += blk[i] != 0;
    free(blk);
    if (nz == 0) return fail("8x8 output all zero");

    nvn_astc_profile p4 = {.mtl_pf = 204, .block_w = 4, .block_h = 4, .is_hdr = 0};
    uint8_t rgba4[4 * 4 * 4];
    memset(rgba4, 0, sizeof rgba4);
    for (int i = 0; i < 16; i++) {
        rgba4[i * 4] = 200;
        rgba4[i * 4 + 1] = 40;
        rgba4[i * 4 + 2] = 80;
        rgba4[i * 4 + 3] = 255;
    }
    uint8_t blk4[16];
    memset(blk4, 0, sizeof blk4);
    if (nvn_astc_encode_rgba8(&p4, rgba4, 16, 4, 4, blk4, 16) != 0) return fail("4x4 encode failed");
    nz = 0;
    for (size_t i = 0; i < sizeof blk4; i++) nz += blk4[i] != 0;
    if (nz == 0) return fail("4x4 output all zero");

    puts("t_astcenc ok");
    return 0;
}