// 2D rendering on the main (top) screen: 8-bit software framebuffer on a Mode 5 bitmap.
#include "game.h"
#include "assets.h"

constexpr int SW = 256, SH = 192;
constexpr int TILE = 32, TSHIFT = 5;
constexpr int WSHIFT = 7;
constexpr int WIN_W = 296, WIN_H = 224;

typedef u32 __attribute__((may_alias)) u32a;

static u8 s_buf[SW * SH] __attribute__((aligned(32)));
static u8 s_win[WIN_W * WIN_H] __attribute__((aligned(32)));
enum { TT_VOID, TT_FLOOR0, TT_FLOOR1, TT_FLOOR0S, TT_FLOOR1S, TT_WALL, TT_WALLF, TT_PILLAR, TT_COUNT };
static u8 s_tex[TT_COUNT][TILE * TILE] __attribute__((aligned(32)));
static u8* s_vram;
static u8 s_ovr;
static int s_camX, s_camY;

static inline u8 CC(u8 c) { return s_ovr ? s_ovr : c; }
static inline int toSX(fx wx) { return (wx >> WSHIFT) - s_camX; }
static inline int toSY(fx wz) { return (wz >> WSHIFT) - s_camY; }

static u32 s_vfx = 0x2545F491u;
static int vfxRange(int lo, int hi) {
    s_vfx ^= s_vfx << 13; s_vfx ^= s_vfx >> 17; s_vfx ^= s_vfx << 5;
    return lo + (int)(s_vfx % (u32)(hi - lo + 1));
}

// ----------------------------------------------------------------- palette
static void pal(int i, int r, int g, int b) { BG_PALETTE[i] = RGB15(r, g, b); }

static void setPalette() {
    pal(P_CLEAR, 0, 0, 0);      pal(P_WHITE, 31, 31, 31);   pal(P_INK, 2, 2, 3);
    pal(P_FLOOR_A, 8, 8, 10);   pal(P_FLOOR_B, 7, 7, 9);    pal(P_FLOOR_CRACK, 4, 4, 6);  pal(P_FLOOR_SPECK, 10, 10, 12);
    pal(P_WALL_TOP, 12, 12, 15); pal(P_WALL_HI, 15, 15, 19); pal(P_WALL_FACE, 8, 8, 11);
    pal(P_WALL_FACE_DK, 5, 5, 8); pal(P_MORTAR, 3, 3, 5);
    pal(P_PIL_TOP, 18, 16, 13); pal(P_PIL_SIDE, 12, 10, 8); pal(P_PIL_DK, 7, 5, 4);
    pal(P_SHADOW, 1, 1, 2);
    pal(P_SLIME, 6, 20, 10);    pal(P_SLIME_LT, 12, 27, 16); pal(P_SLIME_DK, 3, 12, 7);   pal(P_EYE_YL, 31, 31, 12);
    pal(P_BONE, 27, 27, 23);    pal(P_BONE_DK, 16, 16, 14);  pal(P_BLADE, 22, 24, 28);
    pal(P_CLOAK, 22, 10, 3);    pal(P_CLOAK_DK, 12, 5, 2);   pal(P_ARROW, 25, 18, 8);
    pal(P_OGRE, 11, 15, 9);     pal(P_OGRE_DK, 6, 9, 6);     pal(P_OGRE_ARMOR, 15, 11, 7);
    pal(P_HORN, 28, 26, 20);    pal(P_EYE_RED, 31, 6, 3);
    pal(P_GOLD, 31, 25, 2);     pal(P_GOLD_DK, 22, 15, 1);   pal(P_GOLD_LT, 31, 29, 12);
    pal(P_POTION, 28, 3, 5);    pal(P_POTION_LT, 31, 14, 16); pal(P_GLASS, 26, 27, 28);
    pal(P_CHEST, 14, 8, 3);     pal(P_CHEST_DK, 8, 4, 2);    pal(P_CHEST_BAND, 22, 16, 4);
    pal(P_BAR_BG, 8, 0, 0);     pal(P_BAR_FG, 4, 28, 6);
    pal(P_RED, 31, 4, 2);       pal(P_RED_DK, 18, 2, 2);     pal(P_EMBER, 31, 14, 3);
    pal(P_BLOOD, 28, 4, 4);     pal(P_HEAL, 4, 28, 8);       pal(P_GREY, 20, 20, 20);
    pal(P_ARC, 18, 27, 31);     pal(P_TELE, 31, 16, 6);
    for (int i = 0; i < PLAYER_PAL_N; i++) BG_PALETTE[PLAYER_PAL_BASE + i] = kPlayerPal[i];
    for (int i = 0; i < GHOST_PAL_N; i++) BG_PALETTE[GHOST_PAL_BASE + i] = kGhostPal[i];
}

// ------------------------------------------------------------- primitives
static void rawRect(int x, int y, int w, int h, u8 c) {
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > SW) x1 = SW;
    if (y1 > SH) y1 = SH;
    if (x >= x1 || y >= y1) return;
    for (; y < y1; y++) memset(s_buf + y * SW + x, c, (size_t)(x1 - x));
}
static inline void rect(int x, int y, int w, int h, u8 c) { rawRect(x, y, w, h, CC(c)); }

static void line(int x0, int y0, int x1, int y1, int th, u8 c) {
    c = CC(c);
    if (th < 1) th = 1;
    int dx = iabs(x1 - x0), dy = -iabs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy, h = th >> 1;
    for (;;) {
        rawRect(x0 - h, y0 - h, th, th, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void ellipse(int cx, int cy, int rx, int ry, u8 c, bool dither) {
    if (rx <= 0 || ry <= 0) return;
    c = CC(c);
    for (int j = -ry; j <= ry; j++) {
        int y = cy + j;
        if ((unsigned)y >= (unsigned)SH) continue;
        int w = rx * (int)sqrt32(ry * ry - j * j) / ry;
        if (!dither) {
            rawRect(cx - w, y, 2 * w + 1, 1, c);
        } else {
            for (int x = cx - w; x <= cx + w; x++)
                if (((x + y) & 1) == 0 && (unsigned)x < (unsigned)SW) s_buf[y * SW + x] = c;
        }
    }
}

static void shadow(int cx, int cy, int rx, int ry) { ellipse(cx, cy, rx, ry, P_SHADOW, true); }

static void blitStretch(const u8* src, int sw, int sh, int dx, int dy, int dw, int dh, bool flip, u8 flash) {
    if (dw <= 0 || dh <= 0 || dw > SW) return;
    u8 xmap[SW];
    for (int i = 0; i < dw; i++) {
        int s = i * sw / dw;
        xmap[i] = (u8)(flip ? sw - 1 - s : s);
    }
    int i0 = dx < 0 ? -dx : 0, i1 = dx + dw > SW ? SW - dx : dw;
    for (int j = 0; j < dh; j++) {
        int y = dy + j;
        if ((unsigned)y >= (unsigned)SH) continue;
        const u8* row = src + (j * sh / dh) * sw;
        u8* out = s_buf + y * SW;
        for (int i = i0; i < i1; i++) {
            u8 c = row[xmap[i]];
            if (c) out[dx + i] = flash ? flash : c;
        }
    }
}

// ------------------------------------------------------------------- font
static int glyphIndex(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    if (c < 32 || c > 126) c = '?';
    return c - 32;
}
static char upc(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; }

static int kern(char a, char b) {
    a = upc(a); b = upc(b);
    if (a == 'D' && b == 'E') return 1;
    if (a == 'S' && b == 'T') return -2;
    return 0;
}

static int textWidth(const char* s, int scale, int track) {
    int w = 0;
    for (int i = 0; s[i]; i++) {
        w += kFontW[glyphIndex(s[i])] * scale;
        if (s[i + 1]) w += (track + kern(s[i], s[i + 1])) * scale;
    }
    return w;
}

static void drawText(int x, int y, const char* s, int scale, int track, u8 c) {
    for (int i = 0; s[i]; i++) {
        int gi = glyphIndex(s[i]), w = kFontW[gi];
        for (int r = 0; r < 5; r++) {
            u8 bits = kFontRows[gi][r];
            for (int k = 0; k < w; k++)
                if (bits & (0x10 >> k)) rawRect(x + k * scale, y + r * scale, scale, scale, c);
        }
        x += w * scale;
        if (s[i + 1]) x += (track + kern(s[i], s[i + 1])) * scale;
    }
}

static void centerText(int cx, int y, const char* s, int scale, int track, u8 c) {
    drawText(cx - textWidth(s, scale, track) / 2, y, s, scale, track, c);
}

// --------------------------------------------------------------- tile art
static u32 hsh(u32 a, u32 b, u32 c) {
    u32 h = a * 374761393u + b * 668265263u + c * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static void texFloor(u8* t, int v) {
    u8 base = v ? P_FLOOR_B : P_FLOOR_A;
    for (int y = 0; y < TILE; y++)
        for (int x = 0; x < TILE; x++) {
            u8 c = base;
            u32 h = hsh((u32)x, (u32)y, (u32)v + 1);
            if (x == 0 || y == 0) c = P_FLOOR_CRACK;
            else if (h % 29 == 0) c = P_FLOOR_SPECK;
            else if (h % 41 == 1) c = P_FLOOR_CRACK;
            t[y * TILE + x] = c;
        }
    for (int i = 0; i < 10; i++) t[(6 + i) * TILE + 10 + v * 9 + i / 2] = P_FLOOR_CRACK;
}

static void texShade(u8* dst, const u8* src) {
    memcpy(dst, src, TILE * TILE);
    for (int y = 0; y < 6; y++)
        for (int x = 0; x < TILE; x++)
            if (y < 2 || ((x + y) & 1) == 0) dst[y * TILE + x] = P_FLOOR_CRACK;
}

static void texWall(u8* t, bool face) {
    int topH = face ? 20 : TILE;
    for (int y = 0; y < TILE; y++)
        for (int x = 0; x < TILE; x++) {
            u32 h = hsh((u32)x / 8, (u32)y / 6, face ? 7u : 3u);
            u8 c;
            if (y < topH) {
                int off = ((y >> 3) & 1) ? 8 : 0;
                bool mortar = (y & 7) == 7 || ((x + off) & 15) == 15;
                c = mortar ? P_MORTAR : ((h % 5 == 0 || y == 0) ? P_WALL_HI : P_WALL_TOP);
            } else {
                int fy = y - topH;
                int off = ((fy / 6) & 1) ? 8 : 0;
                bool mortar = (fy % 6) == 5 || ((x + off) & 15) == 15;
                c = (fy == 0) ? P_MORTAR : (mortar ? P_MORTAR : ((h % 4 == 0) ? P_WALL_FACE_DK : P_WALL_FACE));
                if (y == TILE - 1) c = P_WALL_FACE_DK;
            }
            t[y * TILE + x] = c;
        }
}

static void texPillar(u8* t) {
    memcpy(t, s_tex[TT_FLOOR0], TILE * TILE);
    for (int x = 5; x < 29; x++)
        if (x & 1) t[30 * TILE + x] = P_FLOOR_CRACK;
    for (int y = 2; y < 30; y++)
        for (int x = 4; x < 28; x++) {
            bool edge = (x == 4 || x == 27 || y == 2 || y == 29 || y == 20);
            u8 c;
            if (y < 20) c = hsh((u32)x, (u32)y, 9u) % 7 == 0 ? P_WALL_HI : P_PIL_TOP;
            else c = P_PIL_SIDE;
            if (edge) c = P_PIL_DK;
            t[y * TILE + x] = c;
        }
}

static void buildTiles() {
    memset(s_tex[TT_VOID], 0, TILE * TILE);
    texFloor(s_tex[TT_FLOOR0], 0);
    texFloor(s_tex[TT_FLOOR1], 1);
    texShade(s_tex[TT_FLOOR0S], s_tex[TT_FLOOR0]);
    texShade(s_tex[TT_FLOOR1S], s_tex[TT_FLOOR1]);
    texWall(s_tex[TT_WALL], false);
    texWall(s_tex[TT_WALLF], true);
    texPillar(s_tex[TT_PILLAR]);
}

static bool nearFloor(int tx, int tz) {
    for (int yy = -1; yy <= 1; yy++)
        for (int xx = -1; xx <= 1; xx++)
            if (!tileSolid(tx + xx, tz + yy)) return true;
    return false;
}

static int pickTile(int tx, int tz) {
    if ((unsigned)tx >= (unsigned)MAP_W || (unsigned)tz >= (unsigned)MAP_H) return TT_VOID;
    int t = g_dun.t[tz][tx];
    if (t == T_FLOOR) {
        bool shaded = tz > 0 && g_dun.t[tz - 1][tx] == T_WALL;
        return TT_FLOOR0 + ((tx + tz) & 1) + (shaded ? 2 : 0);
    }
    if (t == T_PILLAR) return TT_PILLAR;
    if (!nearFloor(tx, tz)) return TT_VOID;
    return tileSolid(tx, tz + 1) ? TT_WALL : TT_WALLF;
}

static void composeWindow(int tx0, int tz0) {
    for (int j = 0; j < WIN_H / TILE; j++)
        for (int i = 0; i < 9; i++) {
            const u32a* src = (const u32a*)s_tex[pickTile(tx0 + i, tz0 + j)];
            for (int r = 0; r < TILE; r++) {
                u32a* d = (u32a*)(s_win + (j * TILE + r) * WIN_W + i * TILE);
                const u32a* s = src + r * 8;
                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
                d[4] = s[4]; d[5] = s[5]; d[6] = s[6]; d[7] = s[7];
            }
        }
}

static void blitWindow(int ox, int oy) {
    int a = ox & 3, sh = a * 8;
    for (int y = 0; y < SH; y++) {
        const u32a* w = (const u32a*)(s_win + (oy + y) * WIN_W) + (ox >> 2);
        u32a* d = (u32a*)(s_buf + y * SW);
        if (a == 0) {
            for (int k = 0; k < SW / 4; k++) d[k] = w[k];
        } else {
            for (int k = 0; k < SW / 4; k++) d[k] = (w[k] >> sh) | (w[k + 1] << (32 - sh));
        }
    }
}

// ---------------------------------------------------------------- humanoid
struct Fig {
    int x, y;
    bool flip;
    int walk;
    bool moving;
    int s8;
    u8 body, head, leg, weapon, trim, eye;
    int arm;
    bool bow, horns;
};

static void drawFig(const Fig& g) {
    auto S = [&](int v) { return (v * g.s8) >> 3; };
    int d = g.flip ? -1 : 1;
    int sw = g.moving ? (fsin(g.walk) * S(5)) >> 12 : 0;
    int bob = g.moving ? (iabs(fsin(g.walk)) * S(2)) >> 12 : 0;
    int hipY = g.y - S(14);
    int th = S(5) < 2 ? 2 : S(5);

    line(g.x - S(3), hipY, g.x - S(3) - sw, g.y, th, g.leg);
    line(g.x + S(3), hipY, g.x + S(3) + sw, g.y, th, g.leg);

    int torsoTop = hipY - S(15) - bob;
    rect(g.x - S(8), torsoTop, S(16), S(15), g.body);
    rect(g.x - S(8), hipY - S(3) - bob, S(16), S(3), g.trim);

    int hl = g.x - S(5) + d * S(1), headTop = torsoTop - S(10);
    rect(hl, headTop, S(10), S(10), g.head);
    int ex = d > 0 ? hl + S(10) - S(4) : hl + S(1);
    rect(ex, headTop + S(3), S(3), S(2) < 2 ? 2 : S(2), g.eye);
    if (g.horns) {
        line(hl + S(1), headTop, hl - S(2), headTop - S(8), S(2), P_HORN);
        line(hl + S(9), headTop, hl + S(12), headTop - S(8), S(2), P_HORN);
    }

    int shY = torsoTop + S(3), xf = g.x + d * S(7), xb = g.x - d * S(7);
    int armTh = S(4) < 2 ? 2 : S(4);
    line(xb, shY, xb - d * S(2) - sw / 2, shY + S(11), armTh, g.body);

    int hx, hy, tx, ty;
    switch (g.arm) {
    case 1:  hx = xf - d * S(3);  hy = shY - S(9);  tx = hx - d * S(2);  ty = hy - S(15); break;
    case 2:  hx = xf + d * S(12); hy = shY + S(5);  tx = hx + d * S(10); ty = hy + S(9);  break;
    case 3:  hx = xf + d * S(3);  hy = shY - S(13); tx = hx;             ty = hy - S(18); break;
    default: hx = xf + d * S(3);  hy = shY + S(11); tx = hx + d * S(7);  ty = hy - S(11); break;
    }
    line(xf, shY, hx, hy, armTh, g.body);
    if (g.bow) {
        int bx = hx + d * S(2), by = hy;
        line(bx, by - S(12), bx + d * S(5), by, 2, g.weapon);
        line(bx + d * S(5), by, bx, by + S(12), 2, g.weapon);
        line(bx, by - S(12), bx, by + S(12), 1, P_WHITE);
        if (g.arm == 2) line(bx - d * S(4), by, bx + d * S(10), by, 1, P_ARROW);
    } else {
        int bt = S(2) < 2 ? 2 : S(2);
        line(hx, hy, tx, ty, bt, g.weapon);
    }
}

// -------------------------------------------------------------- the player
static void drawArc(int cx, int cy, int r, int a0, int a1, int th, u8 c) {
    int steps = (a1 - a0) / 700 + 1;
    for (int i = 0; i <= steps; i++) {
        int a = a0 + (a1 - a0) * i / steps;
        int x = cx + ((fsin(a) * r) >> 12), y = cy + ((fcos(a) * r) >> 12);
        rect(x - th / 2, y - th / 2, th, th, c);
    }
}

static void drawPlayer(int frame) {
    const Player& p = g_pl;
    static bool flip = false;
    static int deadT = 0;
    (void)frame;
    int sn = fsin(p.face);
    if (sn > 700) flip = false;
    else if (sn < -700) flip = true;

    int sx = toSX(p.x), sy = toSY(p.z);
    int dw = 64, dh = 64, lift = 0;
    bool f = flip;
    u8 flash = 0;

    if (p.state == PS_DEAD) {
        deadT++;
        dh = 64 - deadT * 2;
        if (dh < 10) dh = 10;
        dw = 64 + deadT / 2;
        flash = (deadT & 4) ? P_WHITE : 0;
    } else {
        deadT = 0;
        if (p.iframes > 0 && p.state != PS_ROLL && (p.iframes & 2)) return;
        if (p.state == PS_ROLL) { dw = 60; dh = 40; f = ((p.t / 3) & 1) ? !flip : flip; }
        else if (p.state == PS_ATTACK && p.t >= 5 && p.t < 11) { dw = 70; dh = 62; }
        else if (p.state == PS_SPIN) { dw = 58; dh = 62; f = (p.t & 2) != 0; }
        else if (p.moving) lift = (iabs(fsin(p.walk)) * 3) >> 12;
    }
    int cxs = f ? 30 : 33;
    blitStretch(kPlayerIdle, 64, 64, sx - cxs * dw / 64, sy - 54 * dh / 64 - lift, dw, dh, f, flash);
}

static void drawSlash() {
    const Player& p = g_pl;
    int cx = toSX(p.x), cy = toSY(p.z) - 20;
    if (p.state == PS_ATTACK && p.t >= 4 && p.t < 14) {
        int u = (p.t - 4) * 256 / 9;
        int sgn = (p.combo & 1) ? -1 : 1;
        int span = DEG(70), trail = DEG(60);
        int start = p.face - sgn * span;
        int cur = start + sgn * 2 * span * u / 256;
        int from = sgn > 0 ? (cur - trail > start ? cur - trail : start) : (cur + trail < start ? cur + trail : start);
        int a0 = from < cur ? from : cur, a1 = from < cur ? cur : from;
        int r = p.combo == 2 ? 44 : 38;
        drawArc(cx, cy, r, a0, a1, p.combo == 2 ? 4 : 3, P_ARC);
        drawArc(cx, cy, r - 3, a0, a1, 1, P_WHITE);
    } else if (p.state == PS_SPIN) {
        drawArc(cx, cy, 52, p.face - DEG(150), p.face, 4, P_ARC);
        drawArc(cx, cy, 48, p.face - DEG(110), p.face, 2, P_WHITE);
    }
}

// ------------------------------------------------------------------ enemies
static void hpBar(int cx, int top, int w, int frac256) {
    if (frac256 < 0) frac256 = 0;
    if (frac256 > 256) frac256 = 256;
    rawRect(cx - w / 2, top, w, 3, P_BAR_BG);
    rawRect(cx - w / 2, top, w * frac256 / 256, 3, P_BAR_FG);
}

static void drawEnemy(const Enemy& e) {
    int sx = toSX(e.x), sy = toSY(e.z);
    u8 flash = e.hurt > 0 ? (u8)P_WHITE : (u8)0;
    bool tele = (e.st == ES_WINDUP) && ((e.timer & 4) == 0);
    int barTop = sy - 30;

    switch (e.type) {
    case E_SLIME: {
        int s = fsin(e.anim * 1300);
        int hop = (e.st == ES_IDLE) ? 0 : (iabs(s) * 6) >> 12;
        int sq = (s * 2) >> 12;
        shadow(sx, sy, 11, 4);
        s_ovr = flash;
        rect(sx - 13, sy - 5, 4, 5, P_SLIME_DK);
        rect(sx + 9, sy - 5, 4, 5, P_SLIME_DK);
        ellipse(sx, sy - 9 - hop, 12 + sq, 9 - sq, P_SLIME, false);
        ellipse(sx - 3, sy - 13 - hop, 6, 3, P_SLIME_LT, false);
        rect(sx - 7, sy - 11 - hop, 4, 4, P_EYE_YL);
        rect(sx + 3, sy - 11 - hop, 4, 4, P_EYE_YL);
        rect(sx - 6, sy - 10 - hop, 2, 3, P_INK);
        rect(sx + 4, sy - 10 - hop, 2, 3, P_INK);
        s_ovr = 0;
        barTop = sy - 26 - hop;
        break;
    }
    case E_SKELETON:
    case E_ARCHER: {
        Fig g = {};
        g.x = sx; g.y = sy; g.flip = fsin(e.face) < 0; g.walk = e.anim * 1800;
        g.moving = e.st == ES_CHASE;
        bool skel = e.type == E_SKELETON;
        g.s8 = skel ? 8 : 7;
        if (skel) {
            g.body = P_BONE; g.head = P_BONE; g.leg = P_BONE_DK; g.weapon = P_BLADE; g.trim = P_BONE_DK; g.eye = P_EYE_RED;
            g.arm = e.st == ES_WINDUP ? 1 : (e.st == ES_STRIKE ? 2 : 0);
        } else {
            g.body = P_CLOAK; g.head = P_CLOAK_DK; g.leg = P_CLOAK_DK; g.weapon = P_ARROW; g.trim = P_GOLD_DK; g.eye = P_EYE_YL;
            g.bow = true;
            g.arm = e.st == ES_WINDUP ? 2 : 0;
        }
        shadow(sx, sy, (9 * g.s8) >> 3, 3);
        s_ovr = tele ? (u8)P_TELE : flash;
        drawFig(g);
        s_ovr = 0;
        barTop = sy - ((40 * g.s8) >> 3) - 8;
        break;
    }
    default: {
        Fig g = {};
        g.x = sx; g.y = sy; g.flip = fsin(e.face) < 0; g.walk = e.anim * 1200;
        g.moving = e.st == ES_CHASE || e.st == ES_STRIKE;
        g.s8 = 18;
        g.body = P_OGRE; g.head = P_OGRE; g.leg = P_OGRE_DK; g.weapon = P_HORN; g.trim = P_OGRE_ARMOR; g.eye = P_EYE_RED;
        g.horns = true;
        g.arm = (e.st == ES_WINDUP && e.aux == 0) ? 3 : (e.st == ES_RECOVER ? 2 : 0);
        shadow(sx, sy, 24, 7);
        s_ovr = tele ? (u8)P_TELE : flash;
        drawFig(g);
        s_ovr = 0;
        barTop = sy - ((40 * g.s8) >> 3) - 22;
        break;
    }
    }
    if (e.hp < e.maxhp || e.type == E_BOSS)
        hpBar(sx, barTop, e.type == E_BOSS ? 44 : 20, e.hp * 256 / e.maxhp);
}

static bool onScreen(int sx, int sy, int m) {
    return sx > -m && sx < SW + m && sy > -m && sy < SH + m + 40;
}

static void drawGroundFx() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy& e = g_en[i];
        if (!e.alive || e.type != E_BOSS || e.st != ES_WINDUP || e.aux != 0) continue;
        int sx = toSX(e.x), sy = toSY(e.z);
        if (!onScreen(sx, sy, 90)) continue;
        ellipse(sx, sy, 83, 83, (e.timer & 4) ? P_RED : P_RED_DK, true);
    }
}

static void drawPickups(int frame) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        const Pickup& k = g_pk[i];
        if (!k.alive) continue;
        int sx = toSX(k.x), sy = toSY(k.z);
        if (!onScreen(sx, sy, 24)) continue;
        if (k.kind == PK_CHEST || k.kind == PK_CHEST_OPEN) {
            shadow(sx, sy, 13, 4);
            rawRect(sx - 11, sy - 11, 22, 11, P_CHEST);
            rawRect(sx - 11, sy - 3, 22, 3, P_CHEST_DK);
            if (k.kind == PK_CHEST) {
                rawRect(sx - 12, sy - 17, 24, 7, P_CHEST_DK);
                rawRect(sx - 12, sy - 12, 24, 2, P_CHEST_BAND);
                rawRect(sx - 2, sy - 17, 4, 11, P_CHEST_BAND);
                rawRect(sx - 1, sy - 11, 2, 3, P_GOLD);
            } else {
                rawRect(sx - 12, sy - 25, 24, 6, P_CHEST_DK);
                rawRect(sx - 12, sy - 21, 24, 2, P_CHEST_BAND);
                rawRect(sx - 9, sy - 11, 18, 3, P_INK);
                rawRect(sx - 6, sy - 12, 12, 1, P_GOLD_LT);
            }
        } else {
            int bob = (fsin(frame * 400 + i * 900) * 3) >> 12;
            shadow(sx, sy, 5, 2);
            if (k.kind == PK_GOLD) {
                int w = 1 + ((iabs(fsin(frame * 700 + i * 500)) * 3) >> 12);
                rawRect(sx - w - 1, sy - 12 - bob, 2 * w + 2, 7, P_GOLD_DK);
                rawRect(sx - w, sy - 11 - bob, 2 * w, 5, P_GOLD);
                rawRect(sx - w, sy - 11 - bob, 1, 2, P_GOLD_LT);
            } else {
                rawRect(sx - 3, sy - 12 - bob, 6, 8, P_POTION);
                rawRect(sx - 2, sy - 16 - bob, 4, 4, P_GLASS);
                rawRect(sx - 2, sy - 11 - bob, 1, 4, P_POTION_LT);
            }
        }
    }
}

struct Act { fx z; int idx; };

static void drawActors(int frame) {
    Act list[MAX_ENEMIES + 1];
    int n = 0;
    list[n++] = Act{g_pl.z, -1};
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy& e = g_en[i];
        if (!e.alive) continue;
        if (!onScreen(toSX(e.x), toSY(e.z), 110)) continue;
        list[n++] = Act{e.z, i};
    }
    for (int i = 1; i < n; i++) {
        Act a = list[i];
        int j = i - 1;
        while (j >= 0 && list[j].z > a.z) { list[j + 1] = list[j]; j--; }
        list[j + 1] = a;
    }
    for (int i = 0; i < n; i++) {
        if (list[i].idx < 0) drawPlayer(frame);
        else drawEnemy(g_en[list[i].idx]);
    }
}

static void drawProjAndParts() {
    for (int i = 0; i < MAX_PROJ; i++) {
        const Proj& q = g_proj[i];
        if (!q.alive) continue;
        int sx = toSX(q.x), sy = toSY(q.z) - 12;
        if (!onScreen(sx, sy, 16)) continue;
        if (q.kind == 0) {
            int a = angleOf(q.vx, q.vz);
            int dx = (fsin(a) * 9) >> 12, dy = (fcos(a) * 9) >> 12;
            line(sx, sy, sx - dx, sy - dy, 1, P_ARROW);
            rawRect(sx - 1, sy - 1, 2, 2, P_WHITE);
        } else {
            ellipse(sx, sy, 4, 4, P_EMBER, false);
            ellipse(sx, sy, 2, 2, P_TELE, false);
        }
    }
    for (int i = 0; i < MAX_PART; i++) {
        const Particle& q = g_part[i];
        if (q.life <= 0) continue;
        int sx = toSX(q.x), sy = toSY(q.z) - (q.y >> WSHIFT);
        int sz = q.life > 8 ? 2 : 1;
        rawRect(sx, sy, sz, sz, q.c);
    }
}

// -------------------------------------------------------------------- frame
static void present() {
    swiWaitForVBlank();
    DC_FlushRange(s_buf, sizeof(s_buf));
    dmaCopy(s_buf, s_vram, SW * SH);
}

void gfxInit() {
    lcdMainOnTop();
    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    vramSetBankA(VRAM_A_MAIN_BG);
    int bg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    s_vram = (u8*)bgGetGfxPtr(bg);
    bgUpdate();
    setPalette();
    buildTiles();
    memset(s_buf, 0, sizeof(s_buf));
    memset(s_win, 0, sizeof(s_win));
}

void gfxRenderWorld(bool frozen) {
    static int frame = 0;
    int shx = 0, shy = 0;
    if (!frozen) {
        frame++;
        if (g_shake > 0) {
            int m = g_shake > 6 ? 2 : 1;
            shx = vfxRange(-m, m); shy = vfxRange(-m, m);
            g_shake--;
        }
    }
    s_camX = (g_pl.x >> WSHIFT) - SW / 2 + shx;
    s_camY = (g_pl.z >> WSHIFT) - SH / 2 + shy;
    composeWindow(s_camX >> TSHIFT, s_camY >> TSHIFT);
    blitWindow(s_camX & (TILE - 1), s_camY & (TILE - 1));
    drawGroundFx();
    drawPickups(frame);
    drawActors(frame);
    drawProjAndParts();
    drawSlash();
    present();
}

void gfxRenderTitle(int frame) {
    memset(s_buf, P_WHITE, sizeof(s_buf));
    centerText(SW / 2, 22, "WORLDSTONE", 3, 4, P_INK);
    centerText(SW / 2, 48, "LOST LIGHT", 2, 4, P_INK);
    int bob = (frame >> 5) & 1;
    blitStretch(kPlayerIdle, 64, 64, SW / 2 - 66, 70 + bob, 128, 128, false, 0);
    present();
}

void gfxRenderDeath(int frame) {
    memset(s_buf, P_WHITE, sizeof(s_buf));
    int bob = (fsin(frame * 500) * 3) >> 12;
    blitStretch(kGhostDied, 48, 48, SW / 2 - 69, 6 + bob, 144, 144, false, 0);
    centerText(SW / 2, 134, "GUARDIAN DOWN", 2, 4, P_INK);
    char b[40];
    snprintf(b, sizeof(b), "LIGHT %d   KILLS %d", g_pl.lvl, g_pl.kills);
    centerText(SW / 2, 160, b, 1, 4, P_INK);
    if ((frame >> 5) & 1) centerText(SW / 2, 178, "PRESS START", 1, 4, P_INK);
    present();
}
