// GameMovement.cpp  (reemplaza tu fichero existente con este contenido)
#include "GameMovement.h"
#include <cmath>
#include "weapon.h"
#include "Shotgun.h"

// (parámetros tuneables) -- aumenté GRAVITY y JUMP_VEL opcionalmente
static const float GRAVITY = 1700.0f;   // aumentado para salto más "pesado"
static const float MOVE_SPEED = 220.0f;
static const float SPRINT_MULT = 1.8f;
static const float JUMP_VEL = -700.0f;  // opcionalmente aumenté un poco para compensar

static const float GROUND_ACCEL = 4500.0f;
static const float GROUND_ACCEL_STARTBOOST = 8500.0f;
static const float AIR_ACCEL = 1200.0f;
static const float GROUND_FRICTION = 4000.0f;
static const float AIR_FRICTION = 600.0f;
static const float SMALL_SPEED_FOR_BOOST = 60.0f;

// Ajusté umbral de daño por caída porque con mayor gravedad las velocidades en impact son mayores
static const float FALL_DAMAGE_MIN_SPEED = 900.0f; // velocidad mínima (px/s) para empezar a recibir daño
static const float FALL_DAMAGE_SCALE = 0.03f;      // daño = (speed - min) * scale
static const int FALL_DAMAGE_MAX = 200;            // cap

static bool prevOnGroundStatic = true;

std::vector<WeaponPickup> g_pickups;

void UpdateMovement(Player& player, const InputState& input, float dt, int clientW, float groundY, std::vector<Bullet>& outBullets) {
    // Si el jugador está en modo ragdoll (murió), no procesamos controls de movimiento,
    // solo aplicamos física (gravedad + integrador) para que "caiga".
    if (!player.IsAlive() && player.ragdoll) {
        // física simple mientras está en ragdoll
        player.vel.y += GRAVITY * dt;
        player.pos.x += player.vel.x * dt;
        player.pos.y += player.vel.y * dt;

        // colisión simple con suelo (para que deje de caer)
        if (player.pos.y >= groundY) {
            player.pos.y = groundY;
            player.vel.y = 0.0f;
            player.onGround = true;
        }
        else {
            player.onGround = false;
        }

        // límites horizontal
        if (player.pos.x < 20.0f) {
            player.pos.x = 20.0f;
            if (player.vel.x < 0.0f) player.vel.x = 0.0f;
        }
        if (player.pos.x > clientW - 20.0f) {
            player.pos.x = clientW - 20.0f;
            if (player.vel.x > 0.0f) player.vel.x = 0.0f;
        }

        // actualizar timers/recarga por si hace falta (probablemente no relevante)
        if (player.weapon) player.weapon->Update((int)(dt * 1000.0f));
        player.UpdateReload((int)(dt * 1000.0f));

        // stun countdown (aunque muerto no importa mucho)
        if (player.stun_ms > 0) {
            player.stun_ms = player.stun_ms - (int)(dt * 1000.0f);
            if (player.stun_ms < 0) player.stun_ms = 0;
        }

        return;
    }

    // --- Normal: jugador vivo, procesamos entrada y física ---

    // Velocidad objetivo base
    float targetBaseSpeed = MOVE_SPEED;
    if (input.sprint) targetBaseSpeed *= SPRINT_MULT;

    float moveDir = 0.0f;
    if (input.left) moveDir -= 1.0f;
    if (input.right) moveDir += 1.0f;
    float desiredVelX = moveDir * targetBaseSpeed;

    // Si aturdido: no aplicamos inputs de movimiento (pero mantenemos apuntado)
    if (player.stun_ms > 0) {
        player.UpdateAim(input.mousePos.x, input.mousePos.y);
    }
    else {
        bool onGround = player.onGround;
        float baseAccel = onGround ? GROUND_ACCEL : AIR_ACCEL;
        float friction = onGround ? GROUND_FRICTION : AIR_FRICTION;

        // impulso inicial si arrancamos desde casi parado
        float absVelX = std::fabs(player.vel.x);
        float accel = baseAccel;
        if (onGround && moveDir != 0.0f && absVelX < SMALL_SPEED_FOR_BOOST) {
            float t = 1.0f - (absVelX / SMALL_SPEED_FOR_BOOST);
            if (t < 0.0f) t = 0.0f;
            accel = baseAccel + (GROUND_ACCEL_STARTBOOST - baseAccel) * t;
        }

        // mover la velocidad hacia desiredVelX con límite accel*dt
        float delta = desiredVelX - player.vel.x;
        float maxChange = accel * dt;
        float change = delta;
        if (change > maxChange) change = maxChange;
        else if (change < -maxChange) change = -maxChange;
        player.vel.x += change;

        // si no hay input aplicamos fricción para detener
        if (std::fabs(moveDir) < 1e-4f) {
            float vabs = std::fabs(player.vel.x);
            float dec = friction * dt;
            if (vabs <= dec) {
                player.vel.x = 0.0f;
            }
            else {
                player.vel.x = (player.vel.x > 0.0f) ? (player.vel.x - dec) : (player.vel.x + dec);
            }
        }

        // clamp sencillo por seguridad: no superar targetBaseSpeed * 1.1
        float hardMax = targetBaseSpeed * 1.1f;
        if (player.vel.x > hardMax) player.vel.x = hardMax;
        if (player.vel.x < -hardMax) player.vel.x = -hardMax;
    }

    // saltos (flanco)
    static bool prevJump = false;
    if (player.stun_ms == 0) {
        if (input.jump && !prevJump && player.onGround) {
            player.vel.y = JUMP_VEL;
            player.onGround = false;
        }
    }
    prevJump = input.jump;

    // aim
    player.UpdateAim(input.mousePos.x, input.mousePos.y);

    // disparo on-press
    static bool prevLMB = false;
    if (player.stun_ms == 0 && input.lmb && !prevLMB) {
        if (player.weapon && !player.isReloading) {
            int& ammoRef = (player.weaponType == PISTOL) ? player.pistolMag : player.shotgunMag;
            player.weapon->TryFire(player, outBullets, ammoRef);
        }
    }
    prevLMB = input.lmb;

    // recargar on-press
    static bool prevReload = false;
    if (player.stun_ms == 0 && input.reload && !prevReload) {
        player.StartReload();
    }
    prevReload = input.reload;

    // pickups (antes de aplicar física para que la posición actual sea la consistente)
    for (int i = (int)g_pickups.size() - 1; i >= 0; --i) {
        auto& pk = g_pickups[i];
        float dx = player.pos.x - pk.pos.x;
        float dy = player.pos.y - pk.pos.y;
        float dist2 = dx * dx + dy * dy;
        const float PICK_RADIUS = 28.0f;
        if (dist2 <= PICK_RADIUS * PICK_RADIUS) {
            if (pk.wtype == SHOTGUN) {
                int magSize = static_cast<int>(Shotgun().GetMagazineSize());
                player.hasShotgun = true;
                player.shotgunReserve += static_cast<int>(pk.ammoAmount);
                if (player.shotgunMag == 0) {
                    int spaceInMag = magSize - player.shotgunMag;
                    int toTake = pk.ammoAmount < spaceInMag ? pk.ammoAmount : spaceInMag;
                    player.shotgunMag = player.shotgunMag + toTake;
                    player.shotgunReserve = player.shotgunReserve - toTake;
                    if (player.shotgunReserve < 0) player.shotgunReserve = 0;
                }
            }
            g_pickups.erase(g_pickups.begin() + i);
        }
    }

    // cambiar arma
    if (player.stun_ms == 0) {
        if (input.key1) player.EquipWeapon(PISTOL);
        if (input.key2) player.EquipWeapon(SHOTGUN);
    }

    // -------------------------
    // integración física (una sola vez)
    // -------------------------
    // guardamos prevOnGround para detectar landing en este frame
    bool prevOnGround = prevOnGroundStatic;

    player.vel.y += GRAVITY * dt;
    player.pos.x += player.vel.x * dt;
    player.pos.y += player.vel.y * dt;

    // colisión suelo (detectar landing)
    if (player.pos.y >= groundY) {
        // si veníamos en el aire y ahora tocamos suelo -> aplicar daño por caída si hace falta
        if (!prevOnGround) {
            float impactSpeed = player.vel.y; // hacia abajo es positivo
            if (impactSpeed > FALL_DAMAGE_MIN_SPEED) {
                float raw = (impactSpeed - FALL_DAMAGE_MIN_SPEED) * FALL_DAMAGE_SCALE;
                int dmg = (int)(raw + 0.5f);
                if (dmg < 1) dmg = 1;
                if (dmg > FALL_DAMAGE_MAX) dmg = FALL_DAMAGE_MAX;
                // aplica daño: la fuente la ponemos justo debajo del jugador
                player.ApplyDamage(dmg, Vec2(player.pos.x, groundY));
            }
        }

        player.pos.y = groundY;
        player.vel.y = 0.0f;
        player.onGround = true;
    }
    else {
        player.onGround = false;
    }

    prevOnGroundStatic = player.onGround;

    // límites horizontal
    if (player.pos.x < 20.0f) {
        player.pos.x = 20.0f;
        if (player.vel.x < 0.0f) player.vel.x = 0.0f;
    }
    if (player.pos.x > clientW - 20.0f) {
        player.pos.x = clientW - 20.0f;
        if (player.vel.x > 0.0f) player.vel.x = 0.0f;
    }

    // timers y recarga
    if (player.weapon) player.weapon->Update((int)(dt * 1000.0f));
    int dt_ms = (int)(dt * 1000.0f);
    player.UpdateReload(dt_ms);

    // stun countdown
    if (player.stun_ms > 0) {
        player.stun_ms = player.stun_ms - dt_ms;
        if (player.stun_ms < 0) player.stun_ms = 0;
    }

    // damage flash countdown (para que la sprite sepa cuándo mostrar expresión de daño)
    if (player.damage_flash_ms > 0) {
        player.damage_flash_ms = player.damage_flash_ms - dt_ms;
        if (player.damage_flash_ms < 0) player.damage_flash_ms = 0;
    }
 }