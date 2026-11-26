// UI toolkit core: palette, clipped primitives, font renderer, tag headers, chips, segmented bars.
// Everything draws into whatever framebuffer uiSetTarget() last selected.
#include "ui.h"
#include "assets.h"

static u8* s_px = nullptr;
static int s_w = 256, s_h = 192;

// Same order as enum UiPal in ui.h (starting at UP_BG0 = 96). RGB15 components are 0..31.
static const u16 kUiCols[UP_COUNT - UP_BG0] = {
    RGB15(3, 4, 8),    RGB15(5, 7, 12),   RGB15(8, 10, 16),      // BG0..BG2
    RGB15(11, 13, 19), RGB15(23, 25, 30),                        // LINE, LINE_HI
    RGB15(29, 30, 31), RGB15(15, 17, 22),                        // TEXT, TEXT_DIM
    RGB15(8, 4, 13),   RGB15(15, 8, 21),  RGB15(23, 16, 29),     // PURPLE DK/MID/LT
    RGB15(31, 25, 6),  RGB15(19, 13, 2),  RGB15(31, 17, 4), RGB15(18, 9, 2),   // GOLD, GOLD_DK, ORANGE, ORANGE_DK
    RGB15(29, 6, 5),   RGB15(14, 2, 3),   RGB15(31, 14, 12),     // RED, RED_DK, RED_LT
    RGB15(20, 21, 4),  RGB15(11, 12, 2),                         // OLIVE, OLIVE_DK
    RGB15(5, 22, 9),   RGB15(2, 12, 5),   RGB15(18, 31, 8),      // GREEN, GREEN_DK, LIME
    RGB15(14, 26, 31), RGB15(4, 11, 17),  RGB15(23, 28, 31),     // ARC, ARC_DK, HP
    RGB15(8, 14, 28),  RGB15(3, 6, 15),                          // BLUE, BLUE_DK
    // item tiers: common, uncommon, rare, legendary, solar, void, exotic
    RGB15(20, 20, 21), RGB15(8, 23, 10),  RGB15(8, 16, 30), RGB15(19, 10, 27),
    RGB15(31, 17, 4),  RGB15(16, 8, 28),  RGB15(31, 26, 6),
    // darker tier colours (card headers)
    RGB15(8, 8, 9),    RGB15(3, 10, 4),   RGB15(3, 6, 13),  RGB15(8, 4, 12),
    RGB15(14, 7, 1),   RGB15(7, 3, 12),   RGB15(14, 11, 2),
};

const u8 kIconSkull[8] = { 0x3C, 0x7E, 0xDB, 0xFF, 0x7E, 0x3C, 0x3C, 0x24 };
const u8 kIconDodge[8] = { 0x00, 0x08, 0x0C, 0xFE, 0xFF, 0xFE, 0x0C, 0x08 };
const u8 kIconBurst[8] = { 0x92, 0x54, 0x38, 0xFE, 0x38, 0x54, 0x92, 0x00 };
const u8 kIconCell[8]  = { 0x3C, 0x42, 0x99, 0xBD, 0xBD, 0x99, 0x42, 0x3C };
const u8 kIconFlame[8] = { 0x08, 0x18, 0x1C, 0x3C, 0x7E, 0x7E, 0x7E, 0x3C };

void uiPaletteInit(u16* pal) {
    for (int i = 0; i < UP_COUNT - UP_BG0; i++) pal[UP_BG0 + i] = kUiCols[i];
}

void uiSetTarget(u8* px, int w, int h) { s_px = px; s_w = w; s_h = h; }

// ------------------------------------------------------------------ primitives
void uiPx(int x, int y, u8 c) {
    if ((unsigned)x < (unsigned)s_w && (unsigned)y < (unsigned)s_h) s_px[y * s_w + x] = c;
}

void uiRect(int x, int y, int w, int h, u8 c) {
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > s_w) x1 = s_w;
    if (y1 > s_h) y1 = s_h;
    if (x >= x1 || y >= y1) return;
    for (; y < y1; y++) memset(s_px + y * s_w + x, c, (size_t)(x1 - x));
}

void uiFrame(int x, int y, int w, int h, u8 c) {
    if (w < 2 || h < 2) return;
    uiRect(x, y, w, 1, c);
    uiRect(x, y + h - 1, w, 1, c);
    uiRect(x, y + 1, 1, h - 2, c);
    uiRect(x + w - 1, y + 1, 1, h - 2, c);
}

void uiDither(int x, int y, int w, int h, u8 c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (((x + i + y + j) & 1) == 0) uiPx(x + i, y + j, c);
}

void uiDiamond(int cx, int cy, int r, u8 c, bool filled) {
    for (int dy = -r; dy <= r; dy++) {
        int half = r - iabs(dy);
        if (filled) uiRect(cx - half, cy + dy, 2 * half + 1, 1, c);
        else { uiPx(cx - half, cy + dy, c); uiPx(cx + half, cy + dy, c); }
    }
}

void uiTriangle(int cx, int cy, int r, bool up, u8 c, bool filled) {
    for (int i = 0; i <= r; i++) {
        int y = up ? cy - r / 2 + i : cy + r / 2 - i;
        if (filled || i == r) uiRect(cx - i, y, 2 * i + 1, 1, c);
        else { uiPx(cx - i, y, c); uiPx(cx + i, y, c); }
    }
}

void uiRing(int cx, int cy, int r, u8 c) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        uiPx(cx + x, cy + y, c); uiPx(cx - x, cy + y, c);
        uiPx(cx + x, cy - y, c); uiPx(cx - x, cy - y, c);
        uiPx(cx + y, cy + x, c); uiPx(cx - y, cy + x, c);
        uiPx(cx + y, cy - x, c); uiPx(cx - y, cy - x, c);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

// Sawtooth band filling up from the bottom of the (x,y,w,h) box; phase scrolls it sideways.
void uiZigzag(int x, int y, int w, int h, int phase, u8 c) {
    int period = h * 2;
    if (period < 2) return;
    for (int i = 0; i < w; i++) {
        int t = (i + phase) % period;
        if (t < 0) t += period;
        int ht = t < h ? t : period - 1 - t;     // triangle wave 0..h-1
        uiRect(x + i, y + h - 1 - ht, 1, ht + 1, c);
    }
}

void uiIcon8(int x, int y, const u8* rows, u8 c) {
    for (int j = 0; j < 8; j++)
        for (int i = 0; i < 8; i++)
            if (rows[j] & (0x80 >> i)) uiPx(x + i, y + j, c);
}

void uiEndTicks(int x, int y, int w, int h, u8 c) {
    uiRect(x - 3, y - 2, 2, 1, c);       // little marks floating past each end, like the reference bars
    uiRect(x + w + 1, y - 2, 2, 1, c);
    uiRect(x - 3, y + h + 1, 2, 1, c);
    uiRect(x + w + 1, y + h + 1, 2, 1, c);
}

// Bar with a "chip damage" trail (lag) behind the live fill and segment dividers.
void uiSegBar(int x, int y, int w, int h, int frac256, int lag256, int segs,
              u8 fg, u8 bg, u8 lagc, u8 tick) {
    frac256 = iclamp(frac256, 0, 256);
    lag256 = iclamp(lag256, frac256, 256);
    uiRect(x, y, w, h, bg);
    uiRect(x, y, w * lag256 / 256, h, lagc);
    uiRect(x, y, w * frac256 / 256, h, fg);
    for (int i = 1; i < segs; i++) uiRect(x + w * i / segs, y, 1, h, tick);
}

// ------------------------------------------------------------------------ text
static char upc(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; }
static int glyphIndex(char c) {
    c = upc(c);
    if (c < 32 || c > 126) c = '?';
    return c - 32;
}
static int kern(char a, char b) {       // matches gfx.cpp's title kerning
    a = upc(a); b = upc(b);
    if (a == 'D' && b == 'E') return 1;
    if (a == 'S' && b == 'T') return -2;
    return 0;
}

int uiTextW(const char* s, int scale, int track) {
    int w = 0;
    for (int i = 0; s[i]; i++) {
        w += kFontW[glyphIndex(s[i])] * scale;
        if (s[i + 1]) w += (track + kern(s[i], s[i + 1])) * scale;
    }
    return w;
}

void uiText(int x, int y, const char* s, int scale, int track, u8 c) {
    for (int i = 0; s[i]; i++) {
        int gi = glyphIndex(s[i]), w = kFontW[gi];
        for (int r = 0; r < 5; r++) {
            u8 bits = kFontRows[gi][r];
            for (int k = 0; k < w; k++)
                if (bits & (0x10 >> k)) uiRect(x + k * scale, y + r * scale, scale, scale, c);
        }
        x += w * scale;
        if (s[i + 1]) x += (track + kern(s[i], s[i + 1])) * scale;
    }
}

void uiTextC(int cx, int y, const char* s, int scale, int track, u8 c) {
    uiText(cx - uiTextW(s, scale, track) / 2, y, s, scale, track, c);
}
void uiTextR(int xr, int y, const char* s, int scale, int track, u8 c) {
    uiText(xr - uiTextW(s, scale, track), y, s, scale, track, c);
}

// ------------------------------------------------------------- composite widgets
// "> TITLE ............ 04/10"  (11px tall, rule at y + 10)
void uiTagHeader(int x, int y, int w, const char* label, const char* right) {
    uiPx(x, y + 2, UP_LINE_HI); uiPx(x + 1, y + 3, UP_LINE_HI); uiPx(x + 2, y + 4, UP_LINE_HI);
    uiPx(x + 1, y + 5, UP_LINE_HI); uiPx(x, y + 6, UP_LINE_HI);
    uiText(x + 6, y + 2, label, 1, 1, UP_TEXT);
    if (right && right[0]) uiTextR(x + w - 1, y + 2, right, 1, 1, UP_TEXT_DIM);
    uiRect(x, y + 10, w, 1, UP_LINE);
    uiRect(x, y + 10, uiTextW(label, 1, 1) + 9, 1, UP_LINE_HI);   // bright underline beneath the label
}

// Keycap chip: 11px tall, label centred.
void uiChip(int x, int y, int w, const char* s, u8 border, u8 text) {
    uiRect(x + 1, y + 1, w - 2, 9, UP_BG1);
    uiFrame(x, y, w, 11, border);
    uiTextC(x + w / 2, y + 3, s, 1, 1, text);
}
