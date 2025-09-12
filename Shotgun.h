#pragma once
#include "Weapon.h"

class Shotgun : public Weapon {
public:
    Shotgun();
    ~Shotgun() override = default;

    void Update(int dt_ms) override;
    bool TryFire(const Player& owner, std::vector<Bullet>& outBullets, int& ownerAmmo) override;
};
