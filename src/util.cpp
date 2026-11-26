#include "game.h"

static u32 s_rng = 0x1234567;
void rngSeed(u32 s) { s_rng = s ? s : 0xBEEF; }
u32 rnd() {
    u32 x = s_rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return s_rng = x;
}
int rndRange(int lo, int hi) { return lo + (int)(rnd() % (u32)(hi - lo + 1)); }

// atan for 0 <= y <= x, result 0..4096 (45 degrees)
static int atanOct(int y, int x) {
    int t = (int)(((int64_t)y << 12) / x);
    return t + ((((t * (4096 - t)) >> 12) * 1424) >> 12);
}

int angleOf(fx sx, fx cz) {
    int ax = iabs(sx), az = iabs(cz);
    if (!ax && !az) return 0;
    int a = (ax <= az) ? atanOct(ax, az) : 8192 - atanOct(az, ax);
    if (cz < 0) a = 16384 - a;
    if (sx < 0) a = -a;
    return a;
}

fx flen(fx dx, fx dz) {
    int a = dx >> 4, b = dz >> 4;
    return (fx)(sqrt32((u32)(a * a + b * b)) << 4);
}
