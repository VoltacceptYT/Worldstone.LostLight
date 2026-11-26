// Generated data (see tools/gen_assets.py). Sprite pixels are palette indices, 0 = transparent.
#pragma once
#include <nds.h>

constexpr int PLAYER_PAL_BASE = 64, SHARD_PAL_BASE = 80;
constexpr int PLAYER_PAL_N = 13, SHARD_PAL_N = 11;

extern const u8 kFontW[95];                // glyph widths, ASCII 32..126
extern const u8 kFontRows[95][5];          // 5 rows per glyph, bit 4 = leftmost column
extern const u16 kConsoleFont[95 * 16];    // 4bpp 8x8 tiles for the bottom-screen console
extern const u16 kConsolePal[16];

extern const u8 kPlayerIdle[64 * 64];
extern const u16 kPlayerPal[PLAYER_PAL_N];
extern const u8 kShardDied[48 * 48];
extern const u16 kShardPal[SHARD_PAL_N];
