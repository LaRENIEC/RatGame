#pragma once
#include "Player.h"
#include "Input.h"
#include <vector>
#include <cmath>

// Actualiza la física y genera balas si corresponde.
// clientW, groundY y dt deben pasarse desde el loop principal.
void UpdateMovement(Player& player, const InputState& input, float dt, int clientW, float groundY, std::vector<Bullet>& outBullets);

enum PickupType { PICKUP_WEAPON, PICKUP_AMMO };

struct WeaponPickup {
    WeaponType wtype;
    Vec2 pos;
    int ammoAmount; // cuánta munición de reserva entrega
};
extern std::vector<WeaponPickup> g_pickups;