#include "Entity.h"
#include "CommonTypes.h"
#include <cmath>
#include <algorithm>
#include "Level.h" // <-- nuevo

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
extern std::unique_ptr<Level> g_currentLevel; // declarado en Level.cpp

Entity::Entity()
    : pos(0.0f, 0.0f), vel(0.0f, 0.0f), health(50), maxHealth(50),
    radius(14.0f), alive(true), team(1), stun_ms(0) {
}

Entity::~Entity() {}

// -----------------------------------------------------------------------------
        return IsSolidChar(ch);
    Level* L = g_currentLevel.get();
    if (!L) return;

    // convertimos radius a half extents
    const float hw = ent.radius;
    const float hh = ent.radius;
    const float eps = 0.001f;

    auto getAABB = [&](float cx, float cy, float& l, float& t, float& r, float& b) {
        l = cx - hw; r = cx + hw;
        t = cy - hh; b = cy + hh;
        };

    // check rápido si un char de tile es sólido (evita GetMaterialInfo y llamadas costosas)
    auto isTileCharSolid = [](char ch) -> bool {
        // según tu mapping: '#' 'G' 'S' 'V' 'X' son sólidos; 'W' es agua (no sólido), '.' y letras de objetos no sólidos
        // Ajusta si tu mapa tiene otros chars sólidos.
        switch (ch) {
        case '#': case 'G': case 'S': case 'V': case 'X':
            return true;
        default:
            return false;
        }
        };

    float l, t, r, b;
    getAABB(ent.pos.x, ent.pos.y, l, t, r, b);

    int minC = std::max(0, (int)std::floor(l / TILE));
    int maxC = std::min(L->width - 1, (int)std::floor(r / TILE));
    int minR = std::max(0, (int)std::floor(t / TILE));
    int maxR = std::min(L->height - 1, (int)std::floor(b / TILE));

    // One-pass MTV resolution (suficiente para la mayoría de entidades; menos trabajo que múltiples passes)
    for (int rr = minR; rr <= maxR; ++rr) {
        for (int cc = minC; cc <= maxC; ++cc) {
            char tileCh = L->TileCharAt(rr, cc);
            if (!isTileCharSolid(tileCh)) continue;

            float tileL = cc * TILE, tileT = rr * TILE;
            float tileR = tileL + TILE, tileB = tileT + TILE;

            float ix0 = std::max(l, tileL);
            float iy0 = std::max(t, tileT);
            float ix1 = std::min(r, tileR);
            float iy1 = std::min(b, tileB);

            if (!(ix1 > ix0 + eps && iy1 > iy0 + eps)) continue;

            float overlapX = ix1 - ix0;
            float overlapY = iy1 - iy0;

            if (overlapX < overlapY) {
                // separar en X
                float tileCenterX = (tileL + tileR) * 0.5f;
                if (ent.pos.x < tileCenterX) {
                    ent.pos.x -= (overlapX + eps);
                    if (ent.vel.x > 0.0f) ent.vel.x = 0.0f;
                }
                else {
                    ent.pos.x += (overlapX + eps);
                    if (ent.vel.x < 0.0f) ent.vel.x = 0.0f;
                }
            }
            else {
                // separar en Y
                float tileCenterY = (tileT + tileB) * 0.5f;
                if (ent.pos.y < tileCenterY) {
                    ent.pos.y -= (overlapY + eps);
                    if (ent.vel.y > 0.0f) ent.vel.y = 0.0f;
                }
                else {
                    ent.pos.y += (overlapY + eps);
                    if (ent.vel.y < 0.0f) ent.vel.y = 0.0f;
                }
            }

            // recompute local aabb so subsequent tiles see the new position
            getAABB(ent.pos.x, ent.pos.y, l, t, r, b);
        }
    }
}

void Entity::Update(float /*dt*/, const Player& /*player*/, std::vector<Bullet>& /*outBullets*/) {}

void Entity::Draw(HDC hdc) const {
    if (!hdc) return;
    int x = (int)pos.x, y = (int)pos.y, r = (int)radius;
    HBRUSH fill = CreateSolidBrush(RGB(200, 80, 80));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(40, 10, 10));
    HGDIOBJ oldB = SelectObject(hdc, fill);
    HGDIOBJ oldP = SelectObject(hdc, pen);
    Ellipse(hdc, x - r, y - r, x + r, y + r);
    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);
    DeleteObject(fill);
    DeleteObject(pen);

    int barW = r * 2;
    int bx = x - r, by = y - r - 10;
    RECT back = { bx, by, bx + barW, by + 6 };
    HBRUSH backBrush = CreateSolidBrush(RGB(50, 50, 50));
    FillRect(hdc, &back, backBrush);
    DeleteObject(backBrush);

    float pct = (maxHealth > 0) ? (float)health / (float)maxHealth : 0.0f;
    int fillW = std::max<int>(0, static_cast<int>(barW * pct));
    RECT fillR = { bx, by, bx + fillW, by + 6 };
    HBRUSH fillBrush = CreateSolidBrush(RGB((int)((1.0f - pct) * 200), (int)(pct * 200), 40));
    FillRect(hdc, &fillR, fillBrush);
    DeleteObject(fillBrush);
}

void Entity::ApplyDamage(int amount, const Vec2& source) {
    if (!alive) return;
    if (amount <= 0) return;
    health -= amount;
    if (health <= 0) {
        health = 0;
        alive = false;
        OnDeath();
    }
    else {
        float dx = pos.x - source.x;
        float dy = pos.y - source.y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 1e-4f) {
            dx /= len; dy /= len;
            vel.x += dx * (float)amount * 4.0f;
            vel.y += dy * (float)amount * 2.0f;
        }
    }
}

void Entity::OnDeath() {}
