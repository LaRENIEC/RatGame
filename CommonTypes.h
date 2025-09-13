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
