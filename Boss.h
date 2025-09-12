#pragma once
#include "Enemy.h"

// boss con patrones: ráfagas y carga
class Boss : public Enemy {
public:
    Boss();
    virtual ~Boss();

    virtual void Update(float dt, const Player& player, std::vector<Bullet>& outBullets) override;
    virtual void Draw(HDC hdc) const override;
    virtual void OnDeath() override;

private:
    float phaseTimer;   // para cambiar patrones
    int phase;          // 0,1,2
    float burstCooldownMs;
    float burstTimerMs;
    int burstShotsRemaining;
};
