// CommonTypes.h
#pragma once

#include <cmath>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
};
// ---------------- Camera ----------------
struct Camera {
    Vec2 pos = Vec2(0.0f, 0.0f);    // centro de cámara en coordenadas de mundo
    float smoothing = 8.0f;         // mayor => camera más "pegada" al objetivo
    float minX = 0.0f, minY = 0.0f; // límites del mundo (top-left)
    float maxX = 2000.0f, maxY = 1200.0f; // bottom-right del mundo

    void Update(const Vec2& target, float dt, int viewW, int viewH) {
        float halfW = viewW * 0.5f;
        float halfH = viewH * 0.5f;

        // Bias vertical: 0.0 -> jugador en la parte superior, 0.5 -> centro, 1.0 -> parte inferior.
        // Valor sugerido: 0.35 -> jugador queda en ~65% desde arriba (más cerca del suelo).
        const float verticalBias = 0.35f;
        float targetScreenY = viewH * (1.0f - verticalBias);

        // Queremos que: player_screen_y = target.y - pos.y + halfH == targetScreenY
        // => desired.pos.y = target.y + halfH - targetScreenY
        Vec2 desired;
        desired.x = target.x;
        desired.y = target.y + halfH - targetScreenY;

        // Clamp dentro de los límites del mundo
        if (desired.x - halfW < minX) desired.x = minX + halfW;
        if (desired.y - halfH < minY) desired.y = minY + halfH;
        if (desired.x + halfW > maxX) desired.x = maxX - halfW;
        if (desired.y + halfH > maxY) desired.y = maxY - halfH;

        // Smooth follow (igual que antes)
        float t = 1.0f - std::exp(-smoothing * dt);
        pos.x += (desired.x - pos.x) * t;
        pos.y += (desired.y - pos.y) * t;
    }

    Vec2 GetOffset(int viewW, int viewH) const {
        return Vec2(pos.x - viewW * 0.5f, pos.y - viewH * 0.5f);
    }
};
// -----------------------------------------
static Camera g_camera;

struct Bullet {
    Vec2 pos;
    Vec2 vel;
    int life_ms;
    int damage;
    bool fromPlayer; // true si fue creada por el jugador

    Bullet() : pos(), vel(), life_ms(0), damage(0), fromPlayer(false) {}
    Bullet(const Vec2& p, const Vec2& v, int life, int dmg = 0, bool from = false)
        : pos(p), vel(v), life_ms(life), damage(dmg), fromPlayer(from) {
    }

};
