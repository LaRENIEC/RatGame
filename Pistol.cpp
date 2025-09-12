#include "Pistol.h"
#include "Player.h"
#include <cmath>

Pistol::Pistol() {
    m_cfg.cooldown_ms = 150;
    m_cfg.ammoPerShot = 1;
    m_cfg.bulletSpeedMul = 1.0f;
    m_cfg.pellets = 1;
    m_cfg.spread = 0.0f;
    m_cfg.recoil = 80.0f;
    m_cfg.pushback = 20.0f;
    m_cfg.stun_ms = 0;
    m_cfg.damage = 10;

    // magazine / reload
    m_cfg.magazineSize = 12;  // p. ej. 12 balas por cargador
    m_cfg.reload_ms = 800;    // 800ms recarga
}

void Pistol::Update(int dt_ms) {
    Weapon::Update(dt_ms);
}

bool Pistol::TryFire(const Player& ownerConst, std::vector<Bullet>& outBullets, int& ownerAmmo) {
    Player& owner = const_cast<Player&>(ownerConst);

    if (m_timer_ms > 0) return false;
    if (ownerAmmo <= 0) return false; // ownerAmmo ahora es el cargador

    const int life_ms = 2000;
    SpawnProjectiles(owner, outBullets, BULLET_SPEED, life_ms);

    ApplyRecoil(owner, m_cfg.recoil);
    ApplyPushback(owner, m_cfg.pushback);
    ApplyStun(owner, m_cfg.stun_ms);

    ownerAmmo -= m_cfg.ammoPerShot;
    if (ownerAmmo < 0) ownerAmmo = 0;
    m_timer_ms = m_cfg.cooldown_ms;
    return true;
}
