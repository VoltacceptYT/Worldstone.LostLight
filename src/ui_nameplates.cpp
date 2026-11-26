// Floating enemy health plates + the pinned boss bar, styled after the HUD reference:
// base combatants get a plain tick, elites a filled diamond, minibosses a triangle,
// and the lone ultra/boss gets a wide pinned bar with its own smoothed "chip damage" trail.
#include "ui.h"

static const int UV_W = 256, UV_H = 192;

struct QueuedPlate { int idx, sx, top; };
static QueuedPlate s_plates[20];
static int s_nPlates = 0;

static int s_dispHp[MAX_ENEMIES];
static bool s_dispInit[MAX_ENEMIES];

void uiQueuePlate(int enemyIdx, int sx, int barTop) {
    if (s_nPlates >= (int)(sizeof(s_plates) / sizeof(s_plates[0]))) return;
    s_plates[s_nPlates++] = { enemyIdx, sx, barTop };
}

void uiNameplatesFlush(int frame) {
    (void)frame;
    for (int i = 0; i < s_nPlates; i++) {
        const QueuedPlate& q = s_plates[i];
        if ((unsigned)q.idx >= (unsigned)MAX_ENEMIES) continue;
        const Enemy& e = g_en[q.idx];
        if (!e.alive || e.type == E_BOSS) continue;

        int frac = e.maxhp > 0 ? e.hp * 256 / e.maxhp : 0;
        int& disp = s_dispHp[q.idx];
        if (!s_dispInit[q.idx]) { disp = frac; s_dispInit[q.idx] = true; }
        else if (disp > frac) { disp -= 6; if (disp < frac) disp = frac; }
        else disp = frac;

        bool miniboss = e.maxhp >= 60 && e.type != E_SKELETON;
        bool elite = !miniboss && e.type == E_SKELETON;
        u8 fg = miniboss ? UP_OLIVE : (elite ? UP_ORANGE : UP_RED);
        u8 lagc = miniboss ? UP_OLIVE_DK : (elite ? UP_ORANGE_DK : UP_RED_DK);
        int segs = miniboss ? 3 : (elite ? 2 : 1);
        int w = miniboss ? 40 : (elite ? 32 : 22);

        int bx = q.sx - w / 2, by = q.top;
        if (miniboss) uiTriangle(q.sx, by - 8, 5, true, fg, false);
        else uiDiamond(q.sx, by - 6, 4, fg, elite);
        uiSegBar(bx, by, w, 3, frac, disp, segs, fg, UP_BG0, lagc, UP_LINE_HI);
    }
    s_nPlates = 0;
}

void uiBossBar(int frame) {
    int bossIdx = -1;
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (g_en[i].alive && g_en[i].type == E_BOSS) { bossIdx = i; break; }

    static int sDisp = -1;
    if (bossIdx < 0) { sDisp = -1; return; }

    const Enemy& e = g_en[bossIdx];
    int frac = e.maxhp > 0 ? e.hp * 256 / e.maxhp : 0;
    if (sDisp < 0) sDisp = frac;
    else if (sDisp > frac) { sDisp -= 3; if (sDisp < frac) sDisp = frac; }
    else sDisp = frac;

    int w = 160, h = 7, x = (UV_W - w) / 2, y = UV_H - 46;
    uiDiamond(UV_W / 2, y - 9, 7, UP_GOLD, false);
    uiDiamond(UV_W / 2, y - 9, 3, UP_GOLD_DK, true);
    uiSegBar(x, y, w, h, frac, sDisp, 8, UP_GOLD, UP_BG0, UP_GOLD_DK, UP_LINE_HI);
    uiFrame(x, y, w, h, UP_LINE);
    uiEndTicks(x, y, w, h, UP_LINE_HI);
    uiTextC(UV_W / 2, y + h + 3, "HIVE OGRE", 1, 1, UP_TEXT);
    (void)frame;
}
