#pragma once
#include "Weapon.h"

class Pistol : public Weapon {
public:
    Pistol();
    ~Pistol() override = default;

    void Update(int dt_ms) override;
    bool TryFire(const Player& owner, std::vector<Bullet>& outBullets, int& ownerAmmo) override;
};
