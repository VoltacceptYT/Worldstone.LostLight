// Worldstone: Lost Light - unofficial fan-made Destiny 2 ARPG for the Nintendo DS (BlocksDS / libnds)
#include "game.h"

enum { ST_TITLE, ST_PLAY, ST_PAUSE, ST_DEAD, ST_WIN };

static void newGame(u32 seed) {
    while (!dungeonGenerate(seed)) seed += 7919;
    playerReset();
    worldPopulate();
    consoleClear();
}

int main() {
    gfxInit();
    hudInit();
    hudTitle();
    int state = ST_TITLE, frame = 0, endT = 0;

    while (true) {
        scanKeys();
        u32 down = keysDown(), held = keysHeld();
        frame++;

        switch (state) {
        case ST_TITLE:
            gfxRenderTitle(frame);
            if (down & KEY_START) {
                newGame((u32)frame * 2654435761u ^ 0xC0FFEEu);
                state = ST_PLAY; endT = 0;
            }
            break;

        case ST_PLAY:
            if (down & KEY_START) {
                state = ST_PAUSE;
                hudBanner("            PAUSED", "      START to resume");
            }
            if (g_hitstop > 0) g_hitstop--;
            else worldUpdate(down, held);
            gfxRenderWorld(false);
            hudDraw(frame);
            if (g_pl.state == PS_DEAD && ++endT > 70) {
                state = ST_DEAD; hudDraw(0);
                hudBanner("       GUARDIAN DOWN", "   START: return to title");
            } else if (g_bossDead && ++endT > 120) {
                state = ST_WIN; hudDraw(0);
                hudBanner("    HIVE OGRE DEFEATED!", "   START: return to title");
            }
            break;

        case ST_PAUSE:
            gfxRenderWorld(true);
            if (down & KEY_START) state = ST_PLAY;
            break;

        case ST_DEAD:
            gfxRenderDeath(frame);
            if (down & KEY_START) { hudTitle(); state = ST_TITLE; }
            break;

        default:   // win
            gfxRenderWorld(true);
            if (down & KEY_START) { hudTitle(); state = ST_TITLE; }
            break;
        }
    }
    return 0;
}
