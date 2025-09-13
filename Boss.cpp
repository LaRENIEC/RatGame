#include "Boss.h"
#include "Player.h"
#include <cmath>
#include <windows.h>
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif
Boss::Boss() : phaseTimer(0.0f), phase(0), burstCooldownMs(2000.0f), burstTimerMs(0.0f), burstShotsRemaining(0) {
    maxHealth = 500;
    health = maxHealth;
    radius = 36.0f;
    speed = 90.0f;
    aggroRange = 1000.0f;
    fireRange = 900.0f;
    fireCooldownMs = 600.0f;
    fireTimerMs = 0.0f;

    bulletsPerShot = 6;
    bulletSpeed = 640.0f;
    bulletSpread = 0.6f; // abierto
    bulletLifeMs = 2100;
    damagePerBullet = 12;
}

Boss::~Boss() {}

static float AngleBetween(const Vec2& a, const Vec2& b) {
    return atan2f(b.y - a.y, b.x - a.x);
}

// Boss.cpp - optimized Update
void Boss::Update(float dt, const Player& player, std::vector<Bullet>& outBullets) {
    if (!alive) {
        vel.y += 2000.0f * dt;
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        ResolveTileCollisions(*this, dt);
        return;
    }

    phaseTimer += dt;
    if (phaseTimer > 8.0f) {
        phase = (phase + 1) % 3;
        phaseTimer = 0.0f;
    }

    // timers (ms)
    if (fireTimerMs > 0.0f) fireTimerMs = std::max<float>(0.0f, fireTimerMs - dt * 1000.0f);
    if (burstTimerMs > 0.0f) burstTimerMs = std::max<float>(0.0f, burstTimerMs - dt * 1000.0f);

    // movement: follow player X
    float targetX = player.pos.x;
    float dx = targetX - pos.x;
    vel.x = (fabsf(dx) > 8.0f) ? ((dx > 0.0f) ? speed : -speed) : 0.0f;

    // slight bob
    pos.y += sinf(phaseTimer * 2.0f) * 6.0f * dt;

    // integrate
    vel.y += 800.0f * dt;
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // angle to player (compute once)
    float toPlayerAng = AngleBetween(pos, player.pos);

    if (phase == 0) {
        // bursts: spawn multiple pellets per burst
        if (burstTimerMs <= 0.0f) {
            burstShotsRemaining = 3;
            burstTimerMs = 300.0f;
        }
        if (burstShotsRemaining > 0 && burstTimerMs <= 1.0f) {
            int pellets = 8;
            float spread = 1.2f;
            outBullets.reserve(outBullets.size() + pellets);
            for (int i = 0; i < pellets; ++i) {
                float t = (pellets == 1) ? 0.0f : (float)i / (pellets - 1);
                float ang = toPlayerAng + (t - 0.5f) * spread;
                float vx = cosf(ang) * (bulletSpeed * 0.9f);
                float vy = sinf(ang) * (bulletSpeed * 0.9f);
                outBullets.emplace_back(pos, Vec2(vx, vy), bulletLifeMs, damagePerBullet, false);
            }
            burstShotsRemaining--;
            burstTimerMs = 300.0f;
        }
    }
    else if (phase == 1) {
        if (fireTimerMs <= 0.0f) {
            float vx = cosf(toPlayerAng) * (bulletSpeed * 1.1f);
            float vy = sinf(toPlayerAng) * (bulletSpeed * 1.1f);
            outBullets.emplace_back(pos, Vec2(vx, vy), bulletLifeMs, damagePerBullet, false);
            fireTimerMs = 420.0f;
        }
    }
    else { // phase 2
        if (burstTimerMs <= 0.0f) {
            float dashStrength = 550.0f;
            vel.x += cosf(toPlayerAng) * dashStrength;
            vel.y += sinf(toPlayerAng) * dashStrength * 0.3f;
            burstShotsRemaining = 1;
            burstTimerMs = 1200.0f;
        }
        else if (burstShotsRemaining > 0 && burstTimerMs < 300.0f) {
            int pellets = 18;
            float spread = 2.2f;
            outBullets.reserve(outBullets.size() + pellets);
            for (int i = 0; i < pellets; ++i) {
                float t = (pellets == 1) ? 0.0f : (float)i / (pellets - 1);
                float ang = toPlayerAng + (t - 0.5f) * spread;
                float vx = cosf(ang) * (bulletSpeed * 0.75f);
                float vy = sinf(ang) * (bulletSpeed * 0.75f);
                outBullets.emplace_back(pos, Vec2(vx, vy), bulletLifeMs, damagePerBullet, false);
            }
            burstShotsRemaining = 0;
            burstTimerMs = 2200.0f;
            // no need to llamar ResolveTileCollisions aquí; ya lo hacemos en la siguiente frame/loop si hace falta
        }
    }
}

// Boss.cpp - optimized Draw
void Boss::Draw(HDC hdc) const {
    if (!hdc) return;
    int x = (int)pos.x;
    int y = (int)pos.y;
    int r = (int)radius;

    static HBRUSH s_fill = NULL;
    static HPEN s_pen = NULL;
    if (!s_fill) s_fill = CreateSolidBrush(RGB(120, 30, 140));
    if (!s_pen) s_pen = CreatePen(PS_SOLID, 3, RGB(60, 10, 90));

    HGDIOBJ oldB = SelectObject(hdc, s_fill);
    HGDIOBJ oldP = SelectObject(hdc, s_pen);
    Ellipse(hdc, x - r, y - r, x + r, y + r);
    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);

    // health bar
    static HBRUSH s_back = NULL;
    static HBRUSH s_fillbar = NULL;
    if (!s_back) s_back = CreateSolidBrush(RGB(40, 40, 40));
    if (!s_fillbar) s_fillbar = CreateSolidBrush(RGB(200, 80, 80));
    int barW = r * 2 + 40;
    int bx = x - (barW / 2);
    int by = y - r - 18;
    RECT back = { bx, by, bx + barW, by + 8 };
    FillRect(hdc, &back, s_back);
    float pct = (maxHealth > 0) ? (float)health / (float)maxHealth : 0.0f;
    int fillW = std::max(0, (int)(barW * pct));
    RECT fillR = { bx, by, bx + fillW, by + 8 };
    FillRect(hdc, &fillR, s_fillbar);
}

void Boss::OnDeath() {
    // gran explosion? por ahora simple impulso y set alive=false
    vel.y -= 300.0f;
    // podrías spawnear varios minions, partículas, etc.
}
