// Worldstone: Lost Light (unofficial fan game) - shared declarations
#pragma once
#include <nds.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------- fixed point
// World units: 1 tile = 1 unit. Positions are 20.12 fixed point (4096 = 1.0).
typedef int32_t fx;
#define FX(v)  ((fx)((v) * 4096.0 + ((v) >= 0 ? 0.5 : -0.5)))
#define FXI(n) ((fx)(n) * 4096)
#define DEG(d) ((int)((d) * 32768 / 360))
constexpr int ANG_PI = 16384;

static inline fx fxmul(fx a, fx b) { return (fx)(((int64_t)a * (int64_t)b) >> 12); }
static inline int fsin(int a) { return sinLerp((s16)a); }
static inline int fcos(int a) { return cosLerp((s16)a); }
static inline int iabs(int a) { return a < 0 ? -a : a; }
static inline int iclamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void rngSeed(u32 s);
u32 rnd();
int rndRange(int lo, int hi);          // inclusive
int angleOf(fx sx, fx cz);             // angle measured from +Z towards +X
fx flen(fx dx, fx dz);

// -------------------------------------------------------------------- dungeon
constexpr int MAP_W = 48, MAP_H = 48, MAX_ROOMS = 14;
enum { T_WALL = 0, T_FLOOR = 1, T_PILLAR = 2 };
struct Room { int x, y, w, h; };
struct Dungeon {
    u8 t[MAP_H][MAP_W];
    u8 seen[MAP_H][MAP_W];
    Room room[MAX_ROOMS];
    int nroom;
};
extern Dungeon g_dun;
bool dungeonGenerate(u32 seed);
bool tileSolid(int tx, int tz);
bool circleBlocked(fx x, fx z, fx r);
void roomRandomPos(int room, fx* x, fx* z);
void dungeonMarkSeen(int tx, int tz, int r);

// ------------------------------------------------------------------- entities
enum { PS_IDLE, PS_ATTACK, PS_ROLL, PS_SPIN, PS_DEAD };
struct Player {
    fx x, z;
    int face;
    int state, t, combo;
    bool queued, moving;
    int hp, maxhp, stam, maxstam;       // stamina is stored x10
    int lvl, xp, gold, potions, str, weapon, kills;
    int iframes, stamDelay, potCd, walk;
    fx rvx, rvz, kx, kz;
};

enum { E_SLIME, E_SKELETON, E_ARCHER, E_BOSS };
enum { ES_IDLE, ES_CHASE, ES_WINDUP, ES_STRIKE, ES_RECOVER };
struct Enemy {
    bool alive;
    u8 type, st;
    int lvl, hp, maxhp, dmg;
    fx x, z, kx, kz, ax, az;
    int face, timer, cool, hurt, anim, aux, aux2, sgn;
};

struct Proj { bool alive; u8 kind; fx x, z, vx, vz; int life, dmg; };
enum { PK_GOLD, PK_POTION, PK_CHEST, PK_CHEST_OPEN };
struct Pickup { bool alive; u8 kind; fx x, z; int life, val; };
struct Particle { int life; fx x, y, z, vx, vy, vz; u8 c; };

constexpr int MAX_ENEMIES = 48, MAX_PROJ = 24, MAX_PICKUPS = 40, MAX_PART = 48;
extern Player g_pl;
extern Enemy g_en[MAX_ENEMIES];
extern Proj g_proj[MAX_PROJ];
extern Pickup g_pk[MAX_PICKUPS];
extern Particle g_part[MAX_PART];
extern int g_hitstop, g_shake;
extern bool g_bossDead;
extern char g_msg[3][33];
extern const char* const kWeaponName[7];

void msg(const char* fmt, ...);
void playerReset();
void worldPopulate();
void worldUpdate(u32 down, u32 held);
int xpNeeded(int lvl);

// ------------------------------------------------------------------- graphics
// Palette indices of the 8-bit top-screen framebuffer (0..63 are shared, 64+ hold the sprites).
enum PalIdx : u8 {
    P_CLEAR = 0, P_WHITE, P_INK,
    P_FLOOR_A, P_FLOOR_B, P_FLOOR_CRACK, P_FLOOR_SPECK,
    P_WALL_TOP, P_WALL_HI, P_WALL_FACE, P_WALL_FACE_DK, P_MORTAR,
    P_PIL_TOP, P_PIL_SIDE, P_PIL_DK,
    P_SHADOW,
    P_SLIME, P_SLIME_LT, P_SLIME_DK, P_EYE_YL,
    P_BONE, P_BONE_DK, P_BLADE,
    P_CLOAK, P_CLOAK_DK, P_ARROW,
    P_OGRE, P_OGRE_DK, P_OGRE_ARMOR, P_HORN, P_EYE_RED,
    P_GOLD, P_GOLD_DK, P_GOLD_LT, P_POTION, P_POTION_LT, P_GLASS,
    P_CHEST, P_CHEST_DK, P_CHEST_BAND,
    P_BAR_BG, P_BAR_FG,
    P_RED, P_RED_DK, P_EMBER, P_BLOOD, P_HEAL, P_GREY, P_ARC, P_TELE,
    P_COUNT
};
static_assert(P_COUNT <= 64, "shared palette must stay below the sprite palettes");

void gfxInit();
void gfxRenderWorld(bool frozen);
void gfxRenderTitle(int frame);
void gfxRenderDeath(int frame);

// ------------------------------------------------------------------------ hud
void hudInit();
void hudTitle();
void hudDraw(int frame);
void hudBanner(const char* l1, const char* l2);
