#pragma once
#include "hud.h"
#include <vector>

struct Player;

class PickupsHUD : public HUD {
public:
    // Constructor obligatorio con referencia al vector global de pickups
    explicit PickupsHUD(const std::vector<WeaponPickup>& pickupsRef, bool show = true)
        : showPickups(show), pickups(pickupsRef) {
    }

    ~PickupsHUD() override {}

    void Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickupsParam,
        bool showPickupsParam, bool paused) override;

    void SetShow(bool s) { showPickups = s; }

private:
    bool showPickups;
    const std::vector<WeaponPickup>& pickups; // referencia inicializada en ctor
};
