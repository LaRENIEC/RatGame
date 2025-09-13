#pragma once

#include <memory>
#include <windows.h>   // necesaria para HDC en la firma Draw
#include "CommonTypes.h"
#include "entity.h"
// forward declarations
class Weapon;
namespace Gdiplus { class Bitmap; }
enum WeaponType { PISTOL = 0, SHOTGUN = 1 };

class Player : public Entity
{
public:
    Player();
    ~Player();                // <-- declarar, pero definir en .cpp (ver más abajo)

    void Reset();                // <-- nueva función para re-inicializar el jugador

    Vec2 pos;
    Vec2 vel;
    bool onGround;
    float angle; // radianes
    WeaponType weaponType;

    std::unique_ptr<Weapon> weapon;

    int pistolAmmo;
    int shotgunAmmo;

    int pistolMag = 0;       // balas actualmente en cargador
    int pistolReserve = 0;   // balas de reserva
    int shotgunMag = 0;
    int shotgunReserve = 0;

    bool hasShotgun = false; // si el jugador ha encontrado la escopeta

    // recarga
    bool isReloading = false;
    int reload_ms_remaining = 0; // tiempo restante de recarga en ms
    WeaponType reloadWeapon = PISTOL; // arma que se está recargando

    // nuevos: vida / muerte
    int health = 100;
    int maxHealth = 100;
    bool alive = true;

    // Player.h (añade estas líneas en la sección de campos miembro)
    int damage_flash_ms = 0;   // ms desde el último daño -> usado por la sprite para expresión
    // podrías ya tener 'int stun_ms;' y 'bool alive;' etc.

    // nuevo: tiempo de aturdimiento (ms)
    int stun_ms = 0;
    // colisión AABB (el jugador ahora usará un rectángulo grande)
    float collisionHalfW = 28.0f; // medio ancho en px (ajusta)
    float collisionHalfH = 40.0f; // medio alto en px (ajusta)

    // ragdoll / rotation
    bool ragdoll = false;
    float bodyAngle = 0.0f;        // ángulo visual del cuerpo (rad)
    float angularVelocity = 0.0f;  // velocidad angular (rad/ms)
    bool ragdollDragging = false;
    int ragdollLastMouseX = 0;
    // actualización general (llamar cada frame con dt_ms)
    void Update(int dt_ms);
    void ResolveTileCollisions(float dt); // dt en segundos
    // handlers para rotar cadáver por drag del ratón (llamar desde WndProc)
    void OnRagdollMouseDown(int mouseX, int mouseY);
    void OnRagdollMouseMove(int mouseX, int mouseY);
    void OnRagdollMouseUp(int mouseX, int mouseY);
    // métodos nuevos
    void StartReload();              // inicia una recarga si es posible
    void UpdateReload(int dt_ms);    // actualizar estado de recarga cada frame

    void EquipWeapon(WeaponType t);
    void UpdateAim(int mouseX, int mouseY, const Vec2& camOffset = Vec2(0, 0));

    // Aplicar daño al jugador (source optional para empujar)
    void ApplyDamage(int amount, const Vec2& source = Vec2(0, 0));

    bool IsAlive() const { return alive && health > 0; }

    // Bitmap* queda forward-declarado, OK en la firma
    void Draw(HDC hdc, Gdiplus::Bitmap* playerBmp);

    // helper para obtener el punto de salida del proyectil (ya existía GetMuzzlePos())
    Vec2 GetMuzzlePos() const;

    // opcional: puedes añadir un helper para aplicar impulso si prefieres encap.
    void ApplyImpulse(const Vec2& imp) { vel = vel + imp; }

    // nuevo: llamada al morir
    void OnDeath();
};
