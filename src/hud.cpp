// Bottom screen: text console HUD, message log and a fog-of-war minimap.
#include "game.h"
#include "assets.h"

void hudInit() {
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    PrintConsole* con = consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, false);
    static ConsoleFont font;
    font.gfx = (u16*)kConsoleFont;
    font.pal = (u16*)kConsolePal;
    font.numChars = 95;
    font.numColors = 16;
    font.bpp = 4;
    font.asciiOffset = 32;
    font.convertSingleColor = false;
    consoleSetFont(con, &font);
    consoleClear();
}

void hudTitle() {
    consoleClear();
    iprintf("\x1b[1;0H    W O R L D S T O N E\n");
    iprintf("         LOST LIGHT\n\n");
    iprintf("  UNOFFICIAL FAN-MADE GAME\n");
    iprintf("  Not affiliated with Bungie.\n");
    iprintf("  Destiny 2 (c) Bungie, Inc.\n\n");
    iprintf(" D-Pad .... move\n");
    iprintf(" A ........ sword combo\n");
    iprintf(" B ........ dodge\n");
    iprintf(" Y ........ arc burst\n");
    iprintf(" X ........ healing cell\n");
    iprintf(" START .... pause\n\n");
    iprintf("   Press START, Guardian");
}

static void bar(char* out, int v, int mx, int w) {
    int n = mx > 0 ? v * w / mx : 0;
    n = iclamp(n, 0, w);
    for (int i = 0; i < w; i++) out[i] = i < n ? '#' : '-';
    out[w] = 0;
}

void hudBanner(const char* l1, const char* l2) {
    iprintf("\x1b[11;0H%-32s", l1);
    iprintf("\x1b[13;0H%-32s", l2);
}

void hudDraw(int frame) {
    const Player& p = g_pl;
    char b[24];
    if (frame % 3 == 0) {
        int room = 0;
        iprintf("\x1b[0;0H WORLDSTONE  Light %-2d Kills %-3d", p.lvl, p.kills);
        bar(b, p.hp, p.maxhp, 14);
        iprintf("\x1b[1;0H HP [%s] %3d/%-3d ", b, p.hp, p.maxhp);
        bar(b, p.stam, p.maxstam, 14);
        iprintf("\x1b[2;0H ST [%s]         ", b);
        bar(b, p.xp, xpNeeded(p.lvl), 14);
        iprintf("\x1b[3;0H XP [%s] %3d/%-3d ", b, p.xp, xpNeeded(p.lvl));
        iprintf("\x1b[4;0H Glimmer %-5d Cells x%d ATK %-3d", p.gold, p.potions, 8 + p.str * 2 + p.weapon * 3);
        iprintf("\x1b[5;0H Wpn: %-26s", kWeaponName[p.weapon]);
        for (int i = 0; i < 3; i++) iprintf("\x1b[%d;0H %-31s", 6 + i, g_msg[i]);
        (void)room;
    }
    if (frame % 6 == 0) {
        int ptx = p.x >> 12, ptz = p.z >> 12;
        dungeonMarkSeen(ptx, ptz, 5);
        char row[33];
        for (int r = 0; r < 14; r++) {
            for (int c = 0; c < 32; c++) {
                int tx = ptx - 16 + c, tz = ptz - 7 + r;
                char ch = ' ';
                if ((unsigned)tx < (unsigned)MAP_W && (unsigned)tz < (unsigned)MAP_H) {
                    if (g_dun.seen[tz][tx]) {
                        if (g_dun.t[tz][tx] == T_FLOOR) ch = '.';
                        else if (g_dun.t[tz][tx] == T_PILLAR) ch = 'o';
                        else {
                            for (int yy = -1; yy <= 1; yy++)
                                for (int xx = -1; xx <= 1; xx++)
                                    if (!tileSolid(tx + xx, tz + yy)) ch = '#';
                        }
                    }
                }
                row[c] = ch;
            }
            row[32] = 0;
            auto put = [&](fx wx, fx wz, char ch) {
                int cx = (wx >> 12) - (ptx - 16), cz = (wz >> 12) - (ptz - 7);
                if (cz == r && cx >= 0 && cx < 32) row[cx] = ch;
            };
            for (int i = 0; i < MAX_PICKUPS; i++)
                if (g_pk[i].alive && g_dun.seen[g_pk[i].z >> 12][g_pk[i].x >> 12])
                    put(g_pk[i].x, g_pk[i].z, g_pk[i].kind == PK_CHEST ? 'C' : (g_pk[i].kind == PK_CHEST_OPEN ? '+' : '$'));
            for (int i = 0; i < MAX_ENEMIES; i++) {
                const Enemy& e = g_en[i];
                if (!e.alive) continue;
                if (e.type == E_BOSS) { if (g_dun.seen[e.z >> 12][e.x >> 12]) put(e.x, e.z, 'W'); }
                else if (iabs(e.x - p.x) < FXI(7) && iabs(e.z - p.z) < FXI(7)) put(e.x, e.z, 'x');
            }
            put(p.x, p.z, '@');
            iprintf("\x1b[%d;0H%s", 9 + r, row);
        }
        iprintf("\x1b[23;0H A:atk B:roll Y:spin X:potion");
    }
}
