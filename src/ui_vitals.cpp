// Top-screen HUD overlay drawn over the world just before it is presented:
//   top-left   HP bar (segmented, chip-damage trail), stamina sliver, Light level
//   top-right  Glimmer and healing cells
//   bottom     weapon tier chip, key chips (B/Y/X) that dim when unusable, XP strip
#include "ui.h"

static const int UV_W = 256, UV_H = 192;

void uiTopOverlay(u8* buf, int frame) {
    uiSetTarget(buf, UV_W, UV_H);
    const Player& p = g_pl;

    // ---- top-left vitals -------------------------------------------------------
    uiDither(0, 0, 112, 30, UP_BG0);

    int hpFrac = p.maxhp > 0 ? iclamp(p.hp * 256 / p.maxhp, 0, 256) : 0;
    static int sHpDisp = -1;
    if (sHpDisp < 0 || hpFrac >= sHpDisp) sHpDisp = hpFrac;
    else { sHpDisp -= 3; if (sHpDisp < hpFrac) sHpDisp = hpFrac; }

    bool low = hpFrac < 64 && p.state != PS_DEAD;
    u8 hpCol = (low && ((frame >> 3) & 1)) ? UP_RED : UP_HP;
    const int bx = 6, bw = 92;
    uiSegBar(bx, 6, bw, 5, hpFrac, sHpDisp, 4, hpCol, UP_BG0, UP_RED_DK, UP_BG0);
    uiFrame(bx - 1, 5, bw + 2, 7, UP_LINE);
    uiEndTicks(bx, 6, bw, 5, UP_LINE_HI);

    char b[32];
    snprintf(b, sizeof(b), "LIGHT %d", p.lvl);
    uiText(bx, 15, b, 1, 1, UP_TEXT);
    snprintf(b, sizeof(b), "%d/%d", p.hp, p.maxhp);
    uiTextR(bx + bw, 15, b, 1, 1, UP_TEXT_DIM);

    int stFrac = p.maxstam > 0 ? p.stam * 256 / p.maxstam : 0;
    bool stOk = p.stam >= 220;                       // enough for a dodge
    uiSegBar(bx, 22, bw, 2, stFrac, stFrac, 1, stOk ? UP_ARC : UP_ARC_DK, UP_BG0, UP_ARC_DK, UP_BG0);

    // ---- top-right currency ----------------------------------------------------
    uiDither(UV_W - 64, 0, 64, 22, UP_BG0);
    snprintf(b, sizeof(b), "%d", p.gold);
    uiTextR(UV_W - 7, 5, b, 1, 1, UP_TEXT);
    uiDiamond(UV_W - 7 - uiTextW(b, 1, 1) - 6, 7, 3, UP_GOLD, true);

    snprintf(b, sizeof(b), "x%d", p.potions);
    uiTextR(UV_W - 7, 14, b, 1, 1, p.potions > 0 ? UP_TEXT : UP_TEXT_DIM);
    uiIcon8(UV_W - 7 - uiTextW(b, 1, 1) - 10, 12, kIconCell, p.potions > 0 ? UP_GREEN : UP_LINE);

    // ---- bottom-left weapon chip -----------------------------------------------
    int cy = UV_H - 19;
    int wi = iclamp(p.weapon, 0, 6);
    u8 tier = (u8)(UP_T0 + wi);
    const char* wn = kWeaponName[wi];
    int ww = uiTextW(wn, 1, 1) + 10;
    uiChip(6, cy, ww, wn, tier, tier);

    // ---- bottom-right key chips ------------------------------------------------
    struct Key { const char* k; const u8* icon; bool usable; u8 col; };
    Key keys[3] = {
        { "B", kIconDodge, p.stam >= 220, UP_ARC },
        { "Y", kIconBurst, p.stam >= 300, UP_PURPLE_LT },
        { "X", kIconCell,  p.potions > 0 && p.hp < p.maxhp && p.potCd == 0, UP_GREEN },
    };
    int x = UV_W - 3 * 26 - 2;
    for (int i = 0; i < 3; i++, x += 26) {
        const Key& k = keys[i];
        uiIcon8(x, cy + 1, k.icon, k.usable ? k.col : UP_LINE);
        uiChip(x + 10, cy, 13, k.k, k.usable ? UP_LINE_HI : UP_LINE, k.usable ? UP_TEXT : UP_TEXT_DIM);
    }

    // ---- XP strip along the very bottom ----------------------------------------
    int need = xpNeeded(p.lvl);
    int xpFrac = need > 0 ? iclamp(p.xp * 256 / need, 0, 256) : 0;
    uiSegBar(0, UV_H - 3, UV_W, 3, xpFrac, xpFrac, 8, UP_PURPLE_LT, UP_BG0, UP_PURPLE_DK, UP_BG0);
}
