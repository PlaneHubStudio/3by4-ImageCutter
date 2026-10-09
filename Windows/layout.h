#ifndef CROP_LAYOUT_H
#define CROP_LAYOUT_H
#include <stdint.h>
typedef struct { int count, unit, x, y, width, height; } CropLayout;
static CropLayout crop_layout(int w, int h) {
    CropLayout best = {0}; int64_t area = 0;
    for (int n = 2; n <= 3; ++n) {
        int u = w/(3*n); if (h/4 < u) u = h/4;
        int cw = 3*n*u, ch = 4*u;
        if ((int64_t)cw*ch > area) { area = (int64_t)cw*ch; best = (CropLayout){n,u,(w-cw)/2,(h-ch)/2,cw,ch}; }
    }
    return best;
}
#endif
