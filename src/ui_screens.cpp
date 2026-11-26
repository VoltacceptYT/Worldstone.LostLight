// Full-screen framing for the title and death screens. Backdrop goes down before the
// hero art is blitted; foreground adds the tag header / chip chrome on top of it.
#include "ui.h"

static const int SCR_W = 256, SCR_H = 192;

void uiTitleBackdrop(u8* buf, int frame) {
    uiSetTarget(buf, SCR_W, SCR_H);
    uiRect(0, 0, SCR_W, SCR_H, UP_BG0);
    for (int y = 24; y < SCR_H - 24; y += 24) uiRect(0, y, SCR_W, 1, UP_BG1);
    uiZigzag(0, SCR_H - 24, SCR_W, 24, frame / 2, UP_PURPLE_DK);
    uiRect(0, SCR_H - 3, SCR_W, 3, UP_GOLD_DK);
}

void uiTitleForeground(u8* buf, int frame) {
    uiSetTarget(buf, SCR_W, SCR_H);
    uiTagHeader(8, 6, SCR_W - 16, "TITLE", "FAN GAME");
    uiDiamond(20, 100, 9, UP_PURPLE_LT, false);
    uiDiamond(SCR_W - 20, 100, 9, UP_PURPLE_LT, false);
    int cw = 72, cx = SCR_W / 2 - cw / 2, cy = 168;
    bool blink = (frame >> 4) & 1;
    uiChip(cx, cy, cw, "START", blink ? UP_GOLD : UP_LINE, blink ? UP_GOLD : UP_TEXT_DIM);
}

void uiDeathBackdrop(u8* buf, int frame) {
    uiSetTarget(buf, SCR_W, SCR_H);
    uiRect(0, 0, SCR_W, SCR_H, UP_BG0);
    uiDither(0, 0, SCR_W, 7, UP_RED_DK);
    uiDither(0, SCR_H - 7, SCR_W, 7, UP_RED_DK);
    uiRect(0, SCR_H - 3, SCR_W, 3, UP_RED_DK);
    (void)frame;
}

void uiDeathForeground(u8* buf, int frame) {
    uiSetTarget(buf, SCR_W, SCR_H);
    uiTagHeader(8, 6, SCR_W - 16, "DEBRIEF", "KIA");
    (void)frame;
}
