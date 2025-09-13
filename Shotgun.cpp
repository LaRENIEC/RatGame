#include "Shotgun.h"
#include "Player.h"
#include <cmath>

Shotgun::Shotgun() {
    m_cfg.cooldown_ms = 700;
    m_cfg.ammoPerShot = 1;
    m_cfg.bulletSpeedMul = 0.85f;
    m_cfg.pellets = 6;
    m_cfg.spread = 0.45f;
    m_cfg.recoil = 220.0f;
    m_cfg.pushback = 140.0f;
    m_cfg.stun_ms = 50;
    m_cfg.damage = 6;

    m_cfg.magazineSize = 8;   // ejemplo: 2 cartuchos en el cargador
    m_cfg.reload_ms = 1100;   // 1.1s recarga
}

void Shotgun::Update(int dt_ms) {
    Weapon::Update(dt_ms);
}

bool Shotgun::TryFire(const Player& ownerConst, std::vector<Bullet>& outBullets, int& ownerAmmo) {
    Player& owner = const_cast<Player&>(ownerConst);

    if (m_timer_ms > 0) return false;
    if (ownerAmmo <= 0) return false;

    const int life_ms = 1200;
    SpawnProjectiles(owner, outBullets, BULLET_SPEED, life_ms);

    ApplyRecoil(owner, m_cfg.recoil);
    ApplyPushback(owner, m_cfg.pushback);
    ApplyStun(owner, m_cfg.stun_ms);

    ownerAmmo -= m_cfg.ammoPerShot;
    if (ownerAmmo < 0) ownerAmmo = 0;
    m_timer_ms = m_cfg.cooldown_ms;
    return true;
}
