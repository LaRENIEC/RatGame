#include "Enemy.h"
#include "Player.h"
#include <cmath>
#include <windows.h>

Enemy::Enemy() {
    maxHealth = 40;
    health = maxHealth;
    radius = 14.0f;
    team = 1;
    stun_ms = 0;

    patrolLeftX = 100.0f;
    patrolRightX = 300.0f;
    speed = 60.0f;
    aggroRange = 400.0f;
    fireRange = 380.0f;
    fireCooldownMs = 900.0f;
    fireTimerMs = 0.0f;

    bulletsPerShot = 1;
    bulletSpeed = 700.0f;
    bulletSpread = 0.0f;
    bulletLifeMs = 1800;
    damagePerBullet = 8;
}

Enemy::~Enemy() {}

static float AngleTo(const Vec2& from, const Vec2& to) {
    return atan2f(to.y - from.y, to.x - from.x);
}

// Enemy.cpp - optimized Update
void Enemy::Update(float dt, const Player& player, std::vector<Bullet>& outBullets) {
    if (!alive) {
        vel.y += 1500.0f * dt;
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        ResolveTileCollisions(*this, dt);
        return;
    }

    // Resolve tile collisions early (cheap now)
    ResolveTileCollisions(*this, dt);

    if (fireTimerMs > 0.0f) fireTimerMs = std::max<float>(0.0f, fireTimerMs - dt * 1000.0f);
    if (stun_ms > 0) { stun_ms = std::max<int>(0, stun_ms - (int)(dt * 1000.0f)); }

    // distancia al player: usamos squared distances para evitar sqrt
    float dx = player.pos.x - pos.x;
    float dy = player.pos.y - pos.y;
    float dist2 = dx * dx + dy * dy;
    float aggroRange2 = aggroRange * aggroRange;
    float fireRange2 = fireRange * fireRange;

    bool inAggro = dist2 <= aggroRange2;
    bool inFire = dist2 <= fireRange2;

    if (stun_ms <= 0) {
        if (inAggro) {
            float dir = (player.pos.x > pos.x) ? 1.0f : -1.0f;
            vel.x = dir * speed;
        }
        else {
            // patrulla
            if (patrolRightX <= patrolLeftX) {
                vel.x = 0.0f;
            }
            else {
                if (pos.x < patrolLeftX) vel.x = speed;
                else if (pos.x > patrolRightX) vel.x = -speed;
                // invertimos si alcanzó los extremos
                if ((pos.x <= patrolLeftX && vel.x < 0.0f) || (pos.x >= patrolRightX && vel.x > 0.0f)) {
                    vel.x = -vel.x;
                }
            }
        }
    }
    else {
        vel.x *= 0.9f;
    }

    // física simple
    vel.y += 1500.0f * dt;
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // Disparo: reservar bullets antes si vamos a generar varios
    if (alive && stun_ms <= 0 && inFire && fireTimerMs <= 0.0f) {
        float baseAngle = AngleTo(pos, player.pos);

        // reservar para evitar reallocs (pequeño optim)
        int willSpawn = bulletsPerShot;
        outBullets.reserve(outBullets.size() + willSpawn);

        if (bulletsPerShot == 1) {
            float vx = cosf(baseAngle) * bulletSpeed;
            float vy = sinf(baseAngle) * bulletSpeed;
            outBullets.emplace_back(pos, Vec2(vx, vy), bulletLifeMs, damagePerBullet, false);
        }
        else {
            for (int i = 0; i < bulletsPerShot; ++i) {
                float t = (bulletsPerShot == 1) ? 0.0f : (float)i / (bulletsPerShot - 1);
                float ang = baseAngle + (t - 0.5f) * bulletSpread;
                float vx = cosf(ang) * bulletSpeed;
                float vy = sinf(ang) * bulletSpeed;
                outBullets.emplace_back(pos, Vec2(vx, vy), bulletLifeMs, damagePerBullet, false);
            }
        }

        // retroceso
        vel.x += -cosf(baseAngle) * 60.0f;
        vel.y += -sinf(baseAngle) * 20.0f;

        fireTimerMs = fireCooldownMs;
    }
}

// Enemy.cpp - optimized Draw
void Enemy::Draw(HDC hdc) const {
    if (!hdc) return;
    int x = (int)pos.x;
    int y = (int)pos.y;
    int r = (int)radius;

    // caches estáticos (creados una vez)
    static HBRUSH s_fill = NULL;
    static HPEN s_pen = NULL;
    if (!s_fill) s_fill = CreateSolidBrush(RGB(180, 40, 40));
    if (!s_pen) s_pen = CreatePen(PS_SOLID, 2, RGB(90, 20, 20));

    HGDIOBJ oldB = SelectObject(hdc, s_fill);
    HGDIOBJ oldP = SelectObject(hdc, s_pen);
    Ellipse(hdc, x - r, y - r, x + r, y + r);
    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);

    // health bar (también con brushes reusables)
    static HBRUSH s_back = NULL;
    static HBRUSH s_fillbar = NULL;
    if (!s_back) s_back = CreateSolidBrush(RGB(40, 40, 40));
    if (!s_fillbar) s_fillbar = CreateSolidBrush(RGB(200, 0, 0));
    int barW = r * 2;
    int bx = x - r;
    int by = y - r - 10;
    RECT back = { bx, by, bx + barW, by + 6 };
    FillRect(hdc, &back, s_back);
    float pct = (maxHealth > 0) ? (float)health / (float)maxHealth : 0.0f;
    int fillW = std::max<int>(0, static_cast<int>(barW * pct));
    RECT fillR = { bx, by, bx + fillW, by + 6 };
    FillRect(hdc, &fillR, s_fillbar);
}


void Enemy::OnDeath() {
    // cambiar color ya ocurre en Draw cuando !alive
    // pequeño impulso al morir
    vel.y -= 120.0f;
    // podrías spawnear partículas o sound here
}
