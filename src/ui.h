// Worldstone: Lost Light - custom UI toolkit (Destiny 2 FUI/HUD inspired look: deep navy panels,
// thin light rules, "tag" section headers, purple title bands, tiered segmented health bars).
// Everything here draws into an 8-bit 256-wide framebuffer; the top and bottom screens each own one.
#pragma once
#include "game.h"

// UI palette lives above the sprite palettes (0..63 shared, 64+ player, 80+ shard, 96+ UI).
enum UiPal : u8 {
    UP_BG0 = 96, UP_BG1, UP_BG2,                 // panel backgrounds, darkest -> lightest
    UP_LINE, UP_LINE_HI,                         // thin rules, dim and bright
    UP_TEXT, UP_TEXT_DIM,
    UP_PURPLE_DK, UP_PURPLE, UP_PURPLE_LT,       // title band / legendary header
    UP_GOLD, UP_GOLD_DK, UP_ORANGE, UP_ORANGE_DK,
    UP_RED, UP_RED_DK, UP_RED_LT,
    UP_OLIVE, UP_OLIVE_DK,                       // miniboss tier
    UP_GREEN, UP_GREEN_DK, UP_LIME,              // action button / lime accent
    UP_ARC, UP_ARC_DK, UP_HP,
    UP_BLUE, UP_BLUE_DK,
    UP_T0, UP_T1, UP_T2, UP_T3, UP_T4, UP_T5, UP_T6,       // item tier colours (weapon index 0..6)
    UP_T0D, UP_T1D, UP_T2D, UP_T3D, UP_T4D, UP_T5D, UP_T6D, // darker tier colours (card headers)
    UP_COUNT
};
static_assert(UP_COUNT <= 256, "UI palette overflow");

// ------------------------------------------------------------------ core (ui_core.cpp)
void uiPaletteInit(u16* pal);                    // pass BG_PALETTE or BG_PALETTE_SUB
void uiSetTarget(u8* px, int w, int h);          // select the framebuffer all primitives draw into

void uiPx(int x, int y, u8 c);
void uiRect(int x, int y, int w, int h, u8 c);
void uiFrame(int x, int y, int w, int h, u8 c);
void uiDither(int x, int y, int w, int h, u8 c); // 50% checker, used as a see-through scrim
void uiDiamond(int cx, int cy, int r, u8 c, bool filled);
void uiTriangle(int cx, int cy, int r, bool up, u8 c, bool filled);
void uiRing(int cx, int cy, int r, u8 c);
void uiZigzag(int x, int y, int w, int h, int phase, u8 c);
void uiIcon8(int x, int y, const u8* rows, u8 c); // 8x8 monochrome, bit 7 = left column
void uiEndTicks(int x, int y, int w, int h, u8 c); // little tick marks at the ends of a bar
void uiSegBar(int x, int y, int w, int h, int frac256, int lag256, int segs,
              u8 fg, u8 bg, u8 lagc, u8 tick);

int  uiTextW(const char* s, int scale, int track);
void uiText(int x, int y, const char* s, int scale, int track, u8 c);
void uiTextC(int cx, int y, const char* s, int scale, int track, u8 c);
void uiTextR(int xr, int y, const char* s, int scale, int track, u8 c);

// "> TITLE ............ 04/10" section header. Occupies 11px; rule sits at y + 10.
void uiTagHeader(int x, int y, int w, const char* label, const char* right);
// Keycap chip with a single letter (or short label).
void uiChip(int x, int y, int w, const char* s, u8 border, u8 text);

// 8x8 icon bitmaps shared by several screens.
extern const u8 kIconSkull[8], kIconDodge[8], kIconBurst[8], kIconCell[8], kIconFlame[8];

// -------------------------------------------------- top screen overlay (ui_vitals.cpp)
void uiTopOverlay(u8* buf, int frame);           // call right before presenting the world

// ----------------------------------------------- enemy plates + boss bar (ui_nameplates.cpp)
void uiQueuePlate(int enemyIdx, int sx, int barTop); // gfx queues one per visible enemy
void uiNameplatesFlush(int frame);               // draws and clears the queue
void uiBossBar(int frame);                       // pinned boss health bar

// ------------------------------------------------------ title / death (ui_screens.cpp)
void uiTitleBackdrop(u8* buf, int frame);        // before blitting the player sprite
void uiTitleForeground(u8* buf, int frame);      // after
void uiDeathBackdrop(u8* buf, int frame);
void uiDeathForeground(u8* buf, int frame);
