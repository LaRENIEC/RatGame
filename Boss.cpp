#include "Boss.h"
#include "Player.h"
#include <cmath>
#include <windows.h>

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

    // reduce timers
    if (fireTimerMs > 0.0f) fireTimerMs = std::max<float>(0.0f, fireTimerMs - dt * 1000.0f);
    if (burstTimerMs > 0.0f) burstTimerMs = std::max<float>(0.0f, burstTimerMs - dt * 1000.0f);

    // simple movement: flota sobre el jugador en x, mantiene cierta altitud sobre ground
    float targetX = player.pos.x;
    float dx = targetX - pos.x;
    vel.x = (fabsf(dx) > 8.0f) ? ((dx > 0.0f) ? speed : -speed) : 0.0f;

    // slight bob in Y
    pos.y += sinf(phaseTimer * 2.0f) * 6.0f * dt;

    // integrator
    vel.y += 800.0f * dt; // gravedad parcial
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // attack patterns by phase
    float toPlayerAng = AngleBetween(pos, player.pos);
    if (phase == 0) {
        // spread shotgun-style bursts occasionally
        if (burstTimerMs <= 0.0f) {
            // start a small burst of 3 shots
            burstShotsRemaining = 3;
            burstTimerMs = 300.0f; // time between shots in burst
        }
        if (burstShotsRemaining > 0 && burstTimerMs <= 1.0f) {
            // shoot shotgun-style (many pellets)
            int pellets = 8;
            float spread = 1.2f;
            for (int i = 0; i < pellets; ++i) {
                float t = (pellets == 1) ? 0.0f : (float)i / (pellets - 1);
                float ang = toPlayerAng + (t - 0.5f) * spread;
                float vx = cosf(ang) * (bulletSpeed * 0.9f);
                float vy = sinf(ang) * (bulletSpeed * 0.9f);
                outBullets.emplace_back(pos, Vec2(vx, vy), bulletLifeMs);
            }
            burstShotsRemaining--;
            burstTimerMs = 300.0f;
        }
    }
    else if (phase == 1) {
        // steady single shots faster
        if (fireTimerMs <= 0.0f) {
            float vx = cosf(toPlayerAng) * (bulletSpeed * 1.1f);
            float vy = sinf(toPlayerAng) * (bulletSpeed * 1.1f);
            outBullets.emplace_back(pos, Vec2(vx, vy), bulletLifeMs);
            fireTimerMs = 420.0f;
        }
    }
    else { // phase 2
        // charge every few seconds: quick dash towards player then high-damage shotgun
        if (burstTimerMs <= 0.0f) {
            // dash
            float dashStrength = 550.0f;
            vel.x += cosf(toPlayerAng) * dashStrength;
            vel.y += sinf(toPlayerAng) * dashStrength * 0.3f;
            // prepare big shotgun after dash
            burstShotsRemaining = 1;
            burstTimerMs = 1200.0f; // time until big shot
        }
        else if (burstShotsRemaining > 0 && burstTimerMs < 300.0f) {
            // big shotgun volley
            int pellets = 18;
            float spread = 2.2f;
            for (int i = 0; i < pellets; ++i) {
                float t = (pellets == 1) ? 0.0f : (float)i / (pellets - 1);
                float ang = toPlayerAng + (t - 0.5f) * spread;
                float vx = cosf(ang) * (bulletSpeed * 0.75f);
                float vy = sinf(ang) * (bulletSpeed * 0.75f);
                outBullets.emplace_back(pos, Vec2(vx, vy), bulletLifeMs);
            }
            burstShotsRemaining = 0;
            // longer cooldown
            burstTimerMs = 2200.0f;
            ResolveTileCollisions(*this, dt);
        }
    }
}

void Boss::Draw(HDC hdc) const {
    int x = (int)pos.x;
    int y = (int)pos.y;
    int r = (int)radius;

    // big purple boss
    HBRUSH fill = CreateSolidBrush(RGB(120, 30, 140));
    HPEN pen = CreatePen(PS_SOLID, 3, RGB(60, 10, 90));
    HGDIOBJ oldB = SelectObject(hdc, fill);
    HGDIOBJ oldP = SelectObject(hdc, pen);
    Ellipse(hdc, x - r, y - r, x + r, y + r);
    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);
    DeleteObject(fill);
    DeleteObject(pen);

    // health bar large
    int barW = r * 2 + 40;
    int bx = x - (barW / 2);
    int by = y - r - 18;
    RECT back = { bx, by, bx + barW, by + 8 };
    HBRUSH backBrush = CreateSolidBrush(RGB(40, 40, 40));
    FillRect(hdc, &back, backBrush);
    DeleteObject(backBrush);
    float pct = (maxHealth > 0) ? (float)health / (float)maxHealth : 0.0f;
    int fillW = max(0, (int)(barW * pct));
    RECT fillR = { bx, by, bx + fillW, by + 8 };
    HBRUSH fillBrush = CreateSolidBrush(RGB((int)((1.0f - pct) * 200), (int)(pct * 200), 80));
    FillRect(hdc, &fillR, fillBrush);
    DeleteObject(fillBrush);
}

void Boss::OnDeath() {
    // gran explosion? por ahora simple impulso y set alive=false
    vel.y -= 300.0f;
    // podrías spawnear varios minions, partículas, etc.
}
