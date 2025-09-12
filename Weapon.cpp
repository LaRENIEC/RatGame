#include "Weapon.h"
#include <cmath>
#include <algorithm>

// forward declare concrete types if needed
// (Vec2, Bullet, Player están definidos en Player.h; sólo se usan en tiempo de compilación en link)

void DecreaseCooldown(int& timer_ms, int dt_ms) {
    if (timer_ms > 0) timer_ms = (std::max)(0, timer_ms - dt_ms);
}

void Weapon::Update(int dt_ms) {
    DecreaseCooldown(m_timer_ms, dt_ms);
}

// Las siguientes funciones manipulan directamente campos públicos de Player.
// Asegúrate de que Player tenga 'vel' (Vec2), 'angle' (float) y 'stun_ms' (int) públicos.

void Weapon::ApplyRecoil(Player& owner, float magnitude) const {
    // Recoil empuja la velocidad en dirección opuesta al apuntado
    // magnitude es un multiplicador (por ejemplo m_cfg.recoil)
    float rx = -std::cos(owner.angle) * magnitude;
    float ry = -std::sin(owner.angle) * magnitude;
    owner.vel.x += rx;
    owner.vel.y += ry * 0.5f; // darle algo de componente vertical menor
}

void Weapon::ApplyPushback(Player& owner, float magnitude) const {
    // Pushback es un impulso hacia atrás (más fuerte que recoil)
    float px = -std::cos(owner.angle) * magnitude;
    float py = -std::sin(owner.angle) * magnitude;
    owner.vel.x += px;
    owner.vel.y += py * 0.6f;
}

void Weapon::ApplyStun(Player& owner, int stunMs) const {
    if (stunMs <= 0) return;
    // Si el stun actual es menor que el nuevo, lo reemplazamos (no acumulamos más)
    owner.stun_ms = std::max<int>(owner.stun_ms, stunMs);
}

// SpawnProjectiles: crea pellets según m_cfg.pellets y m_cfg.spread.
// Requiere que existan tipos Bullet y Vec2 con constructor (Vec2 pos, Vec2 vel, int life_ms).
bool Weapon::SpawnProjectiles(const Player& owner, std::vector<Bullet>& outBullets, float baseSpeed, int lifeMs) const {
    if (m_cfg.pellets <= 0) return false;

    float baseAngle = owner.angle;
    int dmg = m_cfg.damage;
    bool fromPlayer = true; // ahora los proyectiles creados por este owner se marcan así

    if (m_cfg.pellets == 1) {
        float vx = std::cos(baseAngle) * baseSpeed * m_cfg.bulletSpeedMul;
        float vy = std::sin(baseAngle) * baseSpeed * m_cfg.bulletSpeedMul;
        outBullets.emplace_back(owner.GetMuzzlePos(), Vec2(vx, vy), lifeMs, m_cfg.damage, true);
        return true;
    }

    for (int i = 0; i < m_cfg.pellets; ++i) {
        float t = (m_cfg.pellets == 1) ? 0.0f : (float)i / (m_cfg.pellets - 1);
        float ang = baseAngle + (t - 0.5f) * m_cfg.spread;
        float vx = std::cos(ang) * baseSpeed * m_cfg.bulletSpeedMul;
        float vy = std::sin(ang) * baseSpeed * m_cfg.bulletSpeedMul;
        outBullets.emplace_back(owner.GetMuzzlePos(), Vec2(vx, vy), lifeMs, m_cfg.damage, true);
    }
    return true;
}
