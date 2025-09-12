#pragma once
#include "Entity.h"
#include <vector>
#include <memory>

// Enemy simple: patrulla, detecta jugador y dispara.
class Enemy : public Entity {
public:
    Enemy();
    virtual ~Enemy();

    // override
    virtual void Update(float dt, const Player& player, std::vector<Bullet>& outBullets) override;
    virtual void Draw(HDC hdc) const override;
    virtual void OnDeath() override;

    // configuración pública para fácil ajuste:
    float patrolLeftX;
    float patrolRightX;
    float speed;          // velocidad de movimiento horizontal objetivo
    float aggroRange;     // distancia para perseguir
    float fireRange;      // distancia para disparar
    float fireCooldownMs; // ms entre disparos
    float fireTimerMs;    // contador interno

    // arma propia (si quieres cambiar comportamiento)
    int bulletsPerShot;
    float bulletSpeed;
    float bulletSpread; // radianes
    int bulletLifeMs;
    int damagePerBullet;
};
