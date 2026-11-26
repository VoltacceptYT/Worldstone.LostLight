// Player, enemies, projectiles, pickups, particles.
#include "game.h"
#include <stdarg.h>

Player g_pl;
Enemy g_en[MAX_ENEMIES];
Proj g_proj[MAX_PROJ];
Pickup g_pk[MAX_PICKUPS];
Particle g_part[MAX_PART];
int g_hitstop, g_shake;
bool g_bossDead;
char g_msg[3][33];

const char* const kWeaponName[7] = {"Common Edge",  "Uncommon Edge", "Rare Edge",   "Legendary Edge",
                                    "Solar Edge",   "Void Edge",     "Exotic: Lost Light"};

struct EType { int hp, dmg, xp; fx rad, speed; };
static constexpr EType kET[4] = {
    {20, 6, 6, FX(0.35), FX(0.075)},
    {34, 10, 10, FX(0.30), FX(0.042)},
    {22, 7, 9, FX(0.30), FX(0.040)},
    {320, 18, 150, FX(0.80), FX(0.032)},
};
static const char* const kEName[4] = {"Thrall", "Acolyte", "Vandal", "Hive Ogre"};
static constexpr fx PR = FX(0.28);       // player radius
static constexpr fx PSPEED = FX(0.060);

void msg(const char* fmt, ...) {
    memmove(g_msg[0], g_msg[1], 33);
    memmove(g_msg[1], g_msg[2], 33);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_msg[2], 33, fmt, ap);
    va_end(ap);
}

int xpNeeded(int lvl) { return 24 * lvl; }

// ------------------------------------------------------------------ helpers
static bool moveEntity(fx* px, fx* pz, fx dx, fx dz, fx r) {
    bool moved = false;
    fx nx = *px + dx;
    if (!circleBlocked(nx, *pz, r)) { *px = nx; moved = true; }
    fx nz = *pz + dz;
    if (!circleBlocked(*px, nz, r)) { *pz = nz; moved = true; }
    return moved;
}

static void burst(fx x, fx y, fx z, int n, u8 c, int spd) {
    for (int i = 0; i < MAX_PART && n > 0; i++) {
        Particle& p = g_part[i];
        if (p.life > 0) continue;
        p.life = rndRange(16, 32);
        p.x = x; p.y = y; p.z = z;
        p.vx = rndRange(-spd, spd); p.vz = rndRange(-spd, spd); p.vy = rndRange(spd / 2, spd * 2);
        p.c = c;
        n--;
    }
}

static void spawnPickup(int kind, fx x, fx z, int val) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup& p = g_pk[i];
        if (p.alive) continue;
        p.alive = true; p.kind = (u8)kind; p.x = x; p.z = z; p.val = val;
        p.life = (kind == PK_CHEST || kind == PK_CHEST_OPEN) ? 0x7FFFFFFF : 1500;
        return;
    }
}

static void spawnEnemy(int type, fx x, fx z, int lvl) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy& e = g_en[i];
        if (e.alive) continue;
        memset(&e, 0, sizeof(e));
        e.alive = true; e.type = (u8)type; e.lvl = lvl;
        e.maxhp = e.hp = kET[type].hp * (100 + 18 * (lvl - 1)) / 100;
        e.dmg = kET[type].dmg * (100 + 12 * (lvl - 1)) / 100;
        e.x = x; e.z = z; e.sgn = (rnd() & 1) ? 1 : -1;
        e.anim = (int)(rnd() % 50);
        return;
    }
}

static void fireProj(fx x, fx z, fx dx, fx dz, fx speed, int dmg, int kind, int life) {
    for (int i = 0; i < MAX_PROJ; i++) {
        Proj& p = g_proj[i];
        if (p.alive) continue;
        p.alive = true; p.kind = (u8)kind; p.x = x; p.z = z;
        p.vx = fxmul(dx, speed); p.vz = fxmul(dz, speed);
        p.dmg = dmg; p.life = life;
        return;
    }
}

// ------------------------------------------------------------------- player
static void playerHurt(int dmg, fx fx_, fx fz_) {
    Player& p = g_pl;
    if (p.state == PS_DEAD || p.iframes > 0) return;
    dmg -= p.lvl / 2;
    if (dmg < 1) dmg = 1;
    p.hp -= dmg;
    p.iframes = 45;
    g_hitstop = 4; g_shake = 8;
    fx dx = p.x - fx_, dz = p.z - fz_;
    fx d = flen(dx, dz);
    if (d > 0) {
        p.kx = (fx)(((int64_t)dx << 12) / d) / 8;
        p.kz = (fx)(((int64_t)dz << 12) / d) / 8;
    }
    burst(p.x, FX(0.6), p.z, 8, P_BLOOD, 500);
    if (p.state == PS_ATTACK) p.state = PS_IDLE;
    if (p.hp <= 0) {
        p.hp = 0; p.state = PS_DEAD;
        msg("Your Ghost cannot revive you.");
    } else {
        msg("Ouch! -%d HP", dmg);
    }
}

static void gainXp(int n) {
    Player& p = g_pl;
    p.xp += n;
    while (p.xp >= xpNeeded(p.lvl)) {
        p.xp -= xpNeeded(p.lvl);
        p.lvl++; p.maxhp += 12; p.str += 1; p.hp = p.maxhp; p.stam = p.maxstam;
        msg("LIGHT LEVEL %d reached!", p.lvl);
        burst(p.x, FX(0.5), p.z, 24, P_GOLD_LT, 600);
    }
}

static void enemyKill(Enemy& e) {
    e.alive = false;
    const EType& T = kET[e.type];
    g_pl.kills++;
    gainXp(T.xp * e.lvl);
    msg("%s slain (+%d xp)", kEName[e.type], T.xp * e.lvl);
    burst(e.x, FX(0.4), e.z, 16, P_GREY, 550);
    if (e.type == E_BOSS) {
        g_bossDead = true;
        g_shake = 20;
        for (int i = 0; i < 6; i++) spawnPickup(PK_GOLD, e.x + rndRange(-6000, 6000), e.z + rndRange(-6000, 6000), 40);
        return;
    }
    int n = 1 + (int)(rnd() % 2);
    for (int i = 0; i < n; i++)
        spawnPickup(PK_GOLD, e.x + rndRange(-1500, 1500), e.z + rndRange(-1500, 1500), (2 + (int)(rnd() % 4)) * e.lvl);
    if (rnd() % 100 < 14) spawnPickup(PK_POTION, e.x, e.z, 1);
}

static void enemyHurt(Enemy& e, int dmg, fx dx, fx dz, fx knock, bool crit) {
    e.hp -= dmg;
    e.hurt = 10;
    g_hitstop = crit ? 5 : 3;
    g_shake = crit ? 6 : 3;
    fx d = flen(dx, dz);
    if (d > 0 && e.type != E_BOSS) {
        e.kx = fxmul((fx)(((int64_t)dx << 12) / d), knock);
        e.kz = fxmul((fx)(((int64_t)dz << 12) / d), knock);
    }
    burst(e.x, FX(0.5), e.z, 5, P_WHITE, 400);
    if (crit) msg("CRITICAL! %d dmg", dmg);
    if (e.hp <= 0) enemyKill(e);
}

static void meleeHit(fx reach, int dmg, fx knock, bool all) {
    Player& p = g_pl;
    int sn = fsin(p.face), cs = fcos(p.face);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy& e = g_en[i];
        if (!e.alive) continue;
        fx dx = e.x - p.x, dz = e.z - p.z;
        fx L = flen(dx, dz);
        if (L > reach + kET[e.type].rad) continue;
        if (!all && fxmul(dx, sn) + fxmul(dz, cs) < fxmul(L, FX(0.25))) continue;
        bool crit = (rnd() % 100) < 10;
        int d = dmg * rndRange(85, 115) / 100;
        if (crit) d *= 2;
        if (e.st == ES_IDLE) e.st = ES_CHASE;
        enemyHurt(e, d, dx, dz, knock, crit);
    }
}

static int baseDamage() { return 8 + g_pl.str * 2 + g_pl.weapon * 3; }

static void startAttack(int combo) {
    Player& p = g_pl;
    p.state = PS_ATTACK; p.combo = combo; p.t = 0; p.queued = false;
    p.stam -= 60; p.stamDelay = 30;
    // aim assist: snap towards the nearest enemy
    fx best = FX(2.4);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy& e = g_en[i];
        if (!e.alive) continue;
        fx dx = e.x - p.x, dz = e.z - p.z, d = flen(dx, dz);
        if (d < best) { best = d; p.face = angleOf(dx, dz); }
    }
}

static void playerUpdate(u32 down, u32 held) {
    Player& p = g_pl;
    if (p.state == PS_DEAD) return;
    if (p.iframes > 0) p.iframes--;
    if (p.potCd > 0) p.potCd--;
    if (p.stamDelay > 0) p.stamDelay--;
    else if (p.stam < p.maxstam) p.stam = p.stam + 6 > p.maxstam ? p.maxstam : p.stam + 6;

    if (p.kx || p.kz) {
        moveEntity(&p.x, &p.z, p.kx, p.kz, PR);
        p.kx = fxmul(p.kx, FX(0.8)); p.kz = fxmul(p.kz, FX(0.8));
        if (iabs(p.kx) < 30) p.kx = 0;
        if (iabs(p.kz) < 30) p.kz = 0;
    }

    int ix = ((held & KEY_RIGHT) ? 1 : 0) - ((held & KEY_LEFT) ? 1 : 0);
    int iy = ((held & KEY_UP) ? 1 : 0) - ((held & KEY_DOWN) ? 1 : 0);
    fx mx = 0, mz = 0;
    if (ix || iy) {
        mx = FXI(ix);
        mz = FXI(-iy);
        if (ix && iy) { mx = fxmul(mx, FX(0.7071)); mz = fxmul(mz, FX(0.7071)); }
    }

    if ((down & KEY_X) && p.potions > 0 && p.hp < p.maxhp && p.potCd == 0) {
        p.potions--; p.potCd = 40;
        p.hp += p.maxhp * 4 / 10;
        if (p.hp > p.maxhp) p.hp = p.maxhp;
        msg("Healing cell used (%d left)", p.potions);
        burst(p.x, FX(0.5), p.z, 10, P_HEAL, 350);
    }

    auto startRoll = [&]() {
        p.state = PS_ROLL; p.t = 0; p.iframes = 16;
        p.stam -= 220; p.stamDelay = 45;
        fx rx = (mx || mz) ? mx : fsin(p.face), rz = (mx || mz) ? mz : fcos(p.face);
        p.rvx = fxmul(rx, FX(0.12)); p.rvz = fxmul(rz, FX(0.12));
        p.face = angleOf(rx, rz);
        burst(p.x, FX(0.1), p.z, 6, P_GREY, 300);
    };

    switch (p.state) {
    case PS_IDLE:
        p.moving = false;
        if ((down & KEY_B) && p.stam >= 220) startRoll();
        else if ((down & KEY_Y) && p.stam >= 300) { p.state = PS_SPIN; p.t = 0; p.stam -= 300; p.stamDelay = 60; }
        else if ((down & KEY_A) && p.stam >= 60) startAttack(0);
        else if (mx || mz) {
            moveEntity(&p.x, &p.z, fxmul(mx, PSPEED), fxmul(mz, PSPEED), PR);
            int diff = (s16)(angleOf(mx, mz) - p.face);
            p.face += iclamp(diff, -2600, 2600);
            p.walk += 2200;
            p.moving = true;
        }
        break;
    case PS_ATTACK: {
        p.t++;
        if ((down & KEY_A) && p.t >= 3) p.queued = true;
        if (p.t < 8) moveEntity(&p.x, &p.z, fxmul(fsin(p.face), FX(0.025)), fxmul(fcos(p.face), FX(0.025)), PR);
        if (p.t == 5) {
            int dmg = baseDamage();
            if (p.combo == 2) dmg = dmg * 16 / 10;
            meleeHit(FX(1.45), dmg, p.combo == 2 ? FX(0.30) : FX(0.18), false);
        }
        if ((down & KEY_B) && p.t >= 8 && p.stam >= 220) { startRoll(); break; }
        int len = (p.combo == 2) ? 26 : 17;
        if (p.t >= 9 && p.queued && p.combo < 2 && p.stam >= 60) startAttack(p.combo + 1);
        else if (p.t >= len) { p.state = PS_IDLE; p.combo = 0; }
        break;
    }
    case PS_ROLL:
        p.t++;
        moveEntity(&p.x, &p.z, p.rvx, p.rvz, PR);
        if (p.t >= 16) p.state = PS_IDLE;
        break;
    case PS_SPIN:
        p.t++;
        p.face += 2800;
        if (p.t == 6 || p.t == 13) meleeHit(FX(1.9), baseDamage() * 13 / 10, FX(0.28), true);
        if (p.t >= 20) p.state = PS_IDLE;
        break;
    default: break;
    }
}

void playerReset() {
    memset(&g_pl, 0, sizeof(g_pl));
    memset(g_msg, 0, sizeof(g_msg));
    g_bossDead = false; g_hitstop = g_shake = 0;
    Player& p = g_pl;
    p.maxhp = p.hp = 100; p.maxstam = p.stam = 1000;
    p.lvl = 1; p.str = 3; p.potions = 2;
    const Room& r = g_dun.room[0];
    p.x = FXI(r.x + r.w / 2) + 2048; p.z = FXI(r.y + r.h / 2) + 2048;
    p.face = 0;
    msg("Guardian, find the Lost Light.");
}

// ------------------------------------------------------------------ enemies
static void enemyAI(Enemy& e) {
    const EType& T = kET[e.type];
    Player& p = g_pl;
    fx dx = p.x - e.x, dz = p.z - e.z, d = flen(dx, dz), ux = 0, uz = 0;
    if (d > 0) {
        ux = (fx)(((int64_t)dx << 12) / d);
        uz = (fx)(((int64_t)dz << 12) / d);
    }
    if (e.hurt > 0) e.hurt--;
    if (e.cool > 0) e.cool--;
    if (e.kx || e.kz) {
        moveEntity(&e.x, &e.z, e.kx, e.kz, T.rad);
        e.kx = fxmul(e.kx, FX(0.78)); e.kz = fxmul(e.kz, FX(0.78));
        if (iabs(e.kx) < 30) e.kx = 0;
        if (iabs(e.kz) < 30) e.kz = 0;
    }
    if (e.hurt > 6 && e.type != E_BOSS) return;   // stagger
    e.anim++;
    if (p.state == PS_DEAD) return;

    switch (e.type) {
    case E_SLIME:
        if (e.st == ES_IDLE) { if (d < FX(6.5)) e.st = ES_CHASE; break; }
        if (e.anim % 50 < 26) {
            moveEntity(&e.x, &e.z, fxmul(ux, T.speed), fxmul(uz, T.speed), T.rad);
            e.face = angleOf(ux, uz);
        }
        if (d < T.rad + PR + FX(0.12) && e.cool == 0) { playerHurt(e.dmg, e.x, e.z); e.cool = 50; }
        break;

    case E_SKELETON:
        switch (e.st) {
        case ES_IDLE: if (d < FX(8)) e.st = ES_CHASE; break;
        case ES_CHASE:
            e.face = angleOf(ux, uz);
            if (d > FX(0.95)) moveEntity(&e.x, &e.z, fxmul(ux, T.speed), fxmul(uz, T.speed), T.rad);
            else if (e.cool == 0) { e.st = ES_WINDUP; e.timer = 22; }
            break;
        case ES_WINDUP:
            if (e.timer > 6) e.face = angleOf(ux, uz);
            if (--e.timer <= 0) {
                e.st = ES_STRIKE; e.timer = 6;
                if (d < FX(1.5)) playerHurt(e.dmg, e.x, e.z);
            }
            break;
        case ES_STRIKE: if (--e.timer <= 0) { e.st = ES_RECOVER; e.timer = 22; } break;
        default: if (--e.timer <= 0) { e.st = ES_CHASE; e.cool = 25; } break;
        }
        break;

    case E_ARCHER:
        switch (e.st) {
        case ES_IDLE: if (d < FX(7)) e.st = ES_CHASE; break;
        case ES_CHASE:
            e.face = angleOf(ux, uz);
            if (d < FX(2.5)) moveEntity(&e.x, &e.z, fxmul(-ux, T.speed), fxmul(-uz, T.speed), T.rad);
            else if (d > FX(4.5)) moveEntity(&e.x, &e.z, fxmul(ux, T.speed), fxmul(uz, T.speed), T.rad);
            else moveEntity(&e.x, &e.z, fxmul(-uz * e.sgn, T.speed / 2), fxmul(ux * e.sgn, T.speed / 2), T.rad);
            if (e.cool == 0 && d < FX(5.5)) { e.st = ES_WINDUP; e.timer = 26; }
            break;
        default:
            e.face = angleOf(ux, uz);
            if (--e.timer <= 0) {
                fireProj(e.x, e.z, ux, uz, FX(0.11), e.dmg, 0, 100);
                e.st = ES_CHASE; e.cool = 100 + (int)(rnd() % 40);
                if (rnd() & 1) e.sgn = -e.sgn;
            }
            break;
        }
        break;

    default: {   // boss
        bool rage = e.hp * 2 < e.maxhp;
        switch (e.st) {
        case ES_IDLE:
            if (d < FX(9)) { e.st = ES_CHASE; e.cool = 60; msg("The Hive Ogre awakens!"); }
            break;
        case ES_CHASE:
            e.face = angleOf(ux, uz);
            if (d > FX(1.8)) {
                fx sp = rage ? T.speed * 3 / 2 : T.speed;
                moveEntity(&e.x, &e.z, fxmul(ux, sp), fxmul(uz, sp), T.rad);
            }
            if (e.cool == 0) {
                int r = (int)(rnd() % 100);
                e.st = ES_WINDUP;
                if (d < FX(2.6) && r < 60) { e.aux = 0; e.timer = 42; }
                else if (r < 75) { e.aux = 1; e.timer = 28; }
                else { e.aux = 2; e.timer = 26; e.ax = ux; e.az = uz; }
            }
            break;
        case ES_WINDUP:
            if (e.aux != 2) e.face = angleOf(ux, uz);
            else e.face = angleOf(e.ax, e.az);
            if (--e.timer <= 0) {
                if (e.aux == 0) {
                    if (d < FX(2.6)) playerHurt(e.dmg, e.x, e.z);
                    burst(e.x, FX(0.1), e.z, 20, P_EMBER, 700);
                    g_shake = 12; e.st = ES_RECOVER; e.timer = 34;
                } else if (e.aux == 1) {
                    int n = rage ? 12 : 8;
                    for (int k = 0; k < n; k++) {
                        int a = e.anim * 300 + k * (32768 / n);
                        fireProj(e.x, e.z, fsin(a), fcos(a), FX(0.07), e.dmg * 2 / 3, 1, 140);
                    }
                    e.st = ES_RECOVER; e.timer = 36;
                } else { e.st = ES_STRIKE; e.timer = 22; e.aux2 = 0; }
            }
            break;
        case ES_STRIKE:
            moveEntity(&e.x, &e.z, fxmul(e.ax, FX(0.13)), fxmul(e.az, FX(0.13)), T.rad);
            if (!e.aux2 && d < FX(1.3)) { e.aux2 = 1; playerHurt(e.dmg, e.x, e.z); }
            if (--e.timer <= 0) { e.st = ES_RECOVER; e.timer = 36; }
            break;
        default:
            if (--e.timer <= 0) { e.st = ES_CHASE; e.cool = rage ? 70 : 110; }
            break;
        }
        break;
    }
    }
}

// ------------------------------------------------------------------- world
void worldPopulate() {
    memset(g_en, 0, sizeof(g_en));
    memset(g_proj, 0, sizeof(g_proj));
    memset(g_pk, 0, sizeof(g_pk));
    memset(g_part, 0, sizeof(g_part));
    fx x, z;
    // the start room holds a chest and a potion
    roomRandomPos(0, &x, &z); spawnPickup(PK_CHEST, x, z, 0);
    roomRandomPos(0, &x, &z); spawnPickup(PK_POTION, x, z, 1);
    int last = g_dun.nroom - 1;
    for (int i = 1; i < last; i++) {
        int lvl = 1 + i * 2 / 3;
        int count = 2 + (int)(rnd() % 3) + i / 4;
        for (int k = 0; k < count; k++) {
            int r = (int)(rnd() % 100), type;
            if (i < 2) type = r < 70 ? E_SLIME : E_SKELETON;
            else type = r < 30 ? E_SLIME : (r < 70 ? E_SKELETON : E_ARCHER);
            roomRandomPos(i, &x, &z);
            spawnEnemy(type, x, z, lvl);
        }
        if (rnd() % 100 < 45) { roomRandomPos(i, &x, &z); spawnPickup(PK_CHEST, x, z, 0); }
    }
    const Room& b = g_dun.room[last];
    int lvl = 1 + last * 2 / 3;
    spawnEnemy(E_BOSS, FXI(b.x + b.w / 2) + 2048, FXI(b.y + b.h / 2) + 2048, lvl);
    for (int k = 0; k < 2; k++) { roomRandomPos(last, &x, &z); spawnEnemy(E_SKELETON, x, z, lvl); }
}

static void openChest(Pickup& c) {
    c.kind = PK_CHEST_OPEN;
    burst(c.x, FX(0.4), c.z, 14, P_GOLD, 500);
    for (int i = 0; i < 3; i++)
        spawnPickup(PK_GOLD, c.x + rndRange(-2500, 2500), c.z + rndRange(-2500, 2500), 5 + (int)(rnd() % 8));
    if (rnd() % 100 < 55) spawnPickup(PK_POTION, c.x, c.z + 2000, 1);
    if (g_pl.weapon < 6 && rnd() % 100 < 45) {
        g_pl.weapon++;
        msg("Engram: %s!", kWeaponName[g_pl.weapon]);
    } else {
        msg("Engram decoded.");
    }
}

void worldUpdate(u32 down, u32 held) {
    playerUpdate(down, held);
    Player& p = g_pl;

    Enemy* act[MAX_ENEMIES];
    int na = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy& e = g_en[i];
        if (!e.alive) continue;
        if (flen(p.x - e.x, p.z - e.z) > FXI(16)) continue;
        enemyAI(e);
        if (e.alive) act[na++] = &e;
    }
    for (int i = 0; i < na; i++)          // soft separation
        for (int j = i + 1; j < na; j++) {
            fx dx = act[j]->x - act[i]->x, dz = act[j]->z - act[i]->z;
            fx lim = kET[act[i]->type].rad + kET[act[j]->type].rad;
            if (iabs(dx) > lim || iabs(dz) > lim) continue;
            fx d = flen(dx, dz);
            if (d >= lim || d == 0) continue;
            fx push = (lim - d) / 4;
            fx px = fxmul((fx)(((int64_t)dx << 12) / d), push), pz = fxmul((fx)(((int64_t)dz << 12) / d), push);
            moveEntity(&act[j]->x, &act[j]->z, px, pz, kET[act[j]->type].rad);
            moveEntity(&act[i]->x, &act[i]->z, -px, -pz, kET[act[i]->type].rad);
        }

    for (int i = 0; i < MAX_PROJ; i++) {
        Proj& q = g_proj[i];
        if (!q.alive) continue;
        q.x += q.vx; q.z += q.vz;
        if (--q.life <= 0 || tileSolid(q.x >> 12, q.z >> 12)) {
            q.alive = false;
            burst(q.x, FX(0.4), q.z, 3, P_ARROW, 250);
            continue;
        }
        if (flen(q.x - p.x, q.z - p.z) < FX(0.34)) {
            if (p.iframes == 0 && p.state != PS_DEAD) { playerHurt(q.dmg, q.x - q.vx * 4, q.z - q.vz * 4); q.alive = false; }
        }
    }

    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup& k = g_pk[i];
        if (!k.alive) continue;
        if (--k.life <= 0) { k.alive = false; continue; }
        fx dx = p.x - k.x, dz = p.z - k.z, d = flen(dx, dz);
        if (k.kind == PK_CHEST) { if (d < FX(0.75)) openChest(k); continue; }
        if (k.kind == PK_CHEST_OPEN || p.state == PS_DEAD) continue;
        if (d < FX(1.6)) { k.x += fxmul(dx, FX(0.12)); k.z += fxmul(dz, FX(0.12)); }
        if (d < FX(0.4)) {
            k.alive = false;
            if (k.kind == PK_GOLD) p.gold += k.val;
            else if (p.potions < 9) { p.potions++; msg("Healing cell +1"); }
            else k.alive = true;
        }
    }

    for (int i = 0; i < MAX_PART; i++) {
        Particle& q = g_part[i];
        if (q.life <= 0) continue;
        q.life--;
        q.x += q.vx; q.y += q.vy; q.z += q.vz;
        q.vy -= 110;
        if (q.y < 0) { q.y = 0; q.vy = -q.vy / 3; q.vx /= 2; q.vz /= 2; }
    }
}
