#pragma once
#include <vector>
#include <algorithm>
#include "player.h"

// Forward declarations
struct Vec2;
struct Bullet;
class Player;

// Velocidad base de las balas (constante de compilación)
constexpr float BULLET_SPEED = 900.0f;

// Utilidad: decrementa el timer de cooldown (implementado en Weapon.cpp)
void DecreaseCooldown(int& timer_ms, int dt_ms);

// Stats/ configuración que comparten armas concretas
struct WeaponConfig {
    int cooldown_ms = 200;
    int ammoPerShot = 1;
    float bulletSpeedMul = 1.0f;
    int pellets = 1;
    float spread = 0.0f;
    float recoil = 0.0f;
    float pushback = 0.0f;
    int stun_ms = 0;
    int damage = 1;

    // Nuevos:
    int magazineSize = 1;   // capacidad del cargador
    int reload_ms = 800;    // tiempo de recarga en ms
};

// Interfaz base Weapon con utilidades protegidas
class Weapon {
public:
    Weapon() = default;
    virtual ~Weapon() = default;

    virtual void Update(int dt_ms);

    virtual bool TryFire(const Player& owner, std::vector<Bullet>& outBullets, int& ownerAmmo) = 0;

    virtual int GetCooldownMs() const { return m_cfg.cooldown_ms; }
    virtual int GetMagazineSize() const { return m_cfg.magazineSize; }   // nuevo getter
    virtual int GetReloadMs() const { return m_cfg.reload_ms; }         // nuevo getter

protected:
    WeaponConfig m_cfg{};
    int m_timer_ms = 0; // countdown en ms

    void ApplyRecoil(Player& owner, float magnitude) const;
    void ApplyPushback(Player& owner, float magnitude) const;
    void ApplyStun(Player& owner, int stunMs) const;

    bool SpawnProjectiles(const Player& owner, std::vector<Bullet>& outBullets, float baseSpeed, int lifeMs) const;
};
