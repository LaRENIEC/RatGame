// Entity.h
#pragma once
#include <windows.h>
#include <memory>
#include <vector>
#include "CommonTypes.h" // <-- ahora Vec2 y Bullet están definidos

class Player; // forward

class Entity {
public:
    Entity();
    virtual ~Entity();

    virtual void Update(float dt, const Player& player, std::vector<Bullet>& outBullets);
    virtual void Draw(HDC hdc) const;
    virtual void ApplyDamage(int amount, const Vec2& source);
    virtual void OnDeath();
    virtual void ResolveTileCollisions(Entity& ent, float dt);

    bool IsAlive() const { return alive; }

    Vec2 pos;
    Vec2 vel;
    int health;
    int maxHealth;
    float radius;
    bool alive;
    int team;
    int stun_ms;
};
