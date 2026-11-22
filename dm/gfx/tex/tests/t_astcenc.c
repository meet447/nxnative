// Unit test for nvn_astc_encode_rgba8 (solid-fill 4x4 ASTC block).
#include "../nvn_astcenc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    nvn_astc_profile prof = {.mtl_pf = 204, .block_w = 4, .block_h = 4, .is_hdr = 0};
    uint8_t rgba[4 * 4 * 4];
    memset(rgba, 0, sizeof rgba);
    for (int i = 0; i < 16; i++) {
        rgba[i * 4] = 200;
        rgba[i * 4 + 1] = 40;
        rgba[i * 4 + 2] = 80;
        rgba[i * 4 + 3] = 255;
    }
    uint8_t blk[16];
    memset(blk, 0, sizeof blk);
    if (nvn_astc_encode_rgba8(&prof, rgba, 16, 4, 4, blk, 16) != 0) {
        fputs("encode failed\n", stderr);
        return 1;
    }
    int nz = 0;
    for (size_t i = 0; i < sizeof blk; i++) nz += blk[i] != 0;
    if (nz == 0) {
        fputs("output all zero\n", stderr);
        return 1;
    }
    puts("t_astcenc ok");
    return 0;
}