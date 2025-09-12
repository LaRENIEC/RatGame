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

void Enemy::Update(float dt, const Player& player, std::vector<Bullet>& outBullets) {
    if (!alive) {
        vel.y += 1500.0f * dt;
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        ResolveTileCollisions(*this, dt); // evita que cuerpos muertos atraviesen tiles
        return;
    }

    // ---------- resolver colisiones con tiles ----------
    ResolveTileCollisions(*this, dt);
    // timers
    if (fireTimerMs > 0.0f) fireTimerMs = std::max<float>(0.0f, fireTimerMs - dt * 1000.0f);
    if (stun_ms > 0) { stun_ms = std::max<int>(0, stun_ms - (int)(dt * 1000.0f)); }

    // distancia al player
    float dx = player.pos.x - pos.x;
    float dy = player.pos.y - pos.y;
    float dist2 = dx * dx + dy * dy;
    float dist = sqrtf(dist2);

    bool inAggro = dist <= aggroRange;
    bool inFire = dist <= fireRange;

    // AI movement: si en rango de aggro perseguir, sino patrullar entre puntos
    if (stun_ms <= 0) {
        if (inAggro) {
            // acercarse horizontalmente con velocidad limitada
            float dir = (player.pos.x > pos.x) ? 1.0f : -1.0f;
            vel.x = dir * speed;
        }
        else {
            // patrulla: mover entre left/right
            if (patrolRightX <= patrolLeftX) {
                // sin patrol: quedarse en sitio
                vel.x = 0.0f;
            }
            else {
                if (pos.x < patrolLeftX) {
                    vel.x = speed;
                }
                else if (pos.x > patrolRightX) {
                    vel.x = -speed;
                } // si está entre, mantener velocidad
                // si llega a extremos invierte velocidad
                if ((pos.x <= patrolLeftX && vel.x < 0.0f) || (pos.x >= patrolRightX && vel.x > 0.0f)) {
                    vel.x = -vel.x;
                }
            }
        }
    }
    else {
        // aturdido: disminuir velocidad gradualmente
        vel.x *= 0.9f;
    }

    // aplicar física sencilla
    vel.y += 1500.0f * dt; // gravedad local (puedes usar tu GRAVITY global)
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // Disparo: cuando en rango y cooldown listo (y no aturdido)
    if (alive && stun_ms <= 0 && inFire && fireTimerMs <= 0.0f) {
        // dispara hacia el jugador
        float baseAngle = AngleTo(pos, player.pos);

        // spawn bullets (single or spread)
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

        // retroceso simple
        vel.x += -cosf(baseAngle) * 60.0f;
        vel.y += -sinf(baseAngle) * 20.0f;

        fireTimerMs = fireCooldownMs;
    }
}

void Enemy::Draw(HDC hdc) const {
    // dibujar con color distinto
    int x = (int)pos.x;
    int y = (int)pos.y;
    int r = (int)radius;

    HBRUSH fill = CreateSolidBrush(RGB(180, 40, 40));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(90, 20, 20));
    HGDIOBJ oldB = SelectObject(hdc, fill);
    HGDIOBJ oldP = SelectObject(hdc, pen);
    Ellipse(hdc, x - r, y - r, x + r, y + r);
    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);
    DeleteObject(fill);
    DeleteObject(pen);

    // health bar (llamamos al base Draw para la barra para mantener estilo)
    // podrías evitar doble-dibujo; llamo a la implementación base sólo para barra:
    // Usamos una versión simplificada de la barra encima:
    int barW = r * 2;
    int bx = x - r;
    int by = y - r - 10;
    float pct = (maxHealth > 0) ? (float)health / (float)maxHealth : 0.0f;
    RECT back = { bx, by, bx + barW, by + 6 };
    HBRUSH backBrush = CreateSolidBrush(RGB(40, 40, 40));
    FillRect(hdc, &back, backBrush);
    DeleteObject(backBrush);
    int fillW = max(0, (int)(barW * pct));
    RECT fillR = { bx, by, bx + fillW, by + 6 };
    HBRUSH fillBrush = CreateSolidBrush(RGB((int)((1.0f - pct) * 200), (int)(pct * 200), 40));
    FillRect(hdc, &fillR, fillBrush);
    DeleteObject(fillBrush);
}

void Enemy::OnDeath() {
    // cambiar color ya ocurre en Draw cuando !alive
    // pequeño impulso al morir
    vel.y -= 120.0f;
    // podrías spawnear partículas o sound here
}
