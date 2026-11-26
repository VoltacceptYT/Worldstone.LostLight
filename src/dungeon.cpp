#include "game.h"

Dungeon g_dun;

bool tileSolid(int tx, int tz) {
    if ((unsigned)tx >= (unsigned)MAP_W || (unsigned)tz >= (unsigned)MAP_H) return true;
    return g_dun.t[tz][tx] != T_FLOOR;
}

bool circleBlocked(fx x, fx z, fx r) {
    return tileSolid((x - r) >> 12, (z - r) >> 12) || tileSolid((x + r) >> 12, (z - r) >> 12) ||
           tileSolid((x - r) >> 12, (z + r) >> 12) || tileSolid((x + r) >> 12, (z + r) >> 12);
}

static void carveCorridor(int ax, int ay, int bx, int by) {
    int x = ax, y = ay;
    while (x != bx) { g_dun.t[y][x] = T_FLOOR; g_dun.t[y + 1][x] = T_FLOOR; x += (bx > x) ? 1 : -1; }
    while (y != by) { g_dun.t[y][x] = T_FLOOR; g_dun.t[y][x + 1] = T_FLOOR; y += (by > y) ? 1 : -1; }
    g_dun.t[y][x] = g_dun.t[y + 1][x] = g_dun.t[y][x + 1] = g_dun.t[y + 1][x + 1] = T_FLOOR;
}

bool dungeonGenerate(u32 seed) {
    rngSeed(seed);
    memset(&g_dun, 0, sizeof(g_dun));
    for (int attempt = 0; attempt < 300 && g_dun.nroom < MAX_ROOMS; attempt++) {
        int w = rndRange(6, 10), h = rndRange(6, 10);
        int x = rndRange(2, MAP_W - w - 4), y = rndRange(2, MAP_H - h - 4);
        bool ok = true;
        for (int i = 0; i < g_dun.nroom; i++) {
            const Room& r = g_dun.room[i];
            if (x < r.x + r.w + 3 && x + w + 3 > r.x && y < r.y + r.h + 3 && y + h + 3 > r.y) ok = false;
        }
        if (!ok) continue;
        for (int yy = y; yy < y + h; yy++)
            for (int xx = x; xx < x + w; xx++) g_dun.t[yy][xx] = T_FLOOR;
        if (g_dun.nroom > 0) {
            const Room& p = g_dun.room[g_dun.nroom - 1];
            carveCorridor(p.x + p.w / 2, p.y + p.h / 2, x + w / 2, y + h / 2);
        }
        g_dun.room[g_dun.nroom++] = Room{x, y, w, h};
    }
    if (g_dun.nroom < 6) return false;
    // a few extra links so the level has loops
    for (int k = 0; k < 3; k++) {
        const Room& a = g_dun.room[rndRange(0, g_dun.nroom - 1)];
        const Room& b = g_dun.room[rndRange(0, g_dun.nroom - 1)];
        carveCorridor(a.x + a.w / 2, a.y + a.h / 2, b.x + b.w / 2, b.y + b.h / 2);
    }
    // pillars inside rooms (never in the first or last room)
    for (int i = 1; i < g_dun.nroom - 1; i++) {
        const Room& r = g_dun.room[i];
        for (int yy = r.y + 2; yy < r.y + r.h - 2; yy++)
            for (int xx = r.x + 2; xx < r.x + r.w - 2; xx++)
                if (g_dun.t[yy][xx] == T_FLOOR && (rnd() % 100) < 12) g_dun.t[yy][xx] = T_PILLAR;
    }
    return true;
}

void roomRandomPos(int room, fx* x, fx* z) {
    const Room& r = g_dun.room[room];
    for (int tries = 0; tries < 30; tries++) {
        int tx = rndRange(r.x + 1, r.x + r.w - 2), tz = rndRange(r.y + 1, r.y + r.h - 2);
        if (!tileSolid(tx, tz)) { *x = FXI(tx) + 2048; *z = FXI(tz) + 2048; return; }
    }
    *x = FXI(r.x + r.w / 2) + 2048; *z = FXI(r.y + r.h / 2) + 2048;
}

void dungeonMarkSeen(int tx, int tz, int r) {
    for (int y = tz - r; y <= tz + r; y++)
        for (int x = tx - r; x <= tx + r; x++)
            if ((unsigned)x < (unsigned)MAP_W && (unsigned)y < (unsigned)MAP_H) g_dun.seen[y][x] = 1;
}
