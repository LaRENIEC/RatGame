#include "Player.h"
#include <cmath>
#include <memory>
#include <gdiplus.h>
#include "gdiplusbitmap.h"
#include "gdiplusgraphics.h"
#pragma comment(lib, "gdiplus.lib") // si no lo pones en otro lado, ayuda para linkear
using std::make_unique;
#include "Level.h"
#include "GameUI.h"
// NO uses 'using namespace Gdiplus;' to avoid nombre ambiguos.
// Referenciamos tipos GDI+ con su namespace completo: Gdiplus::Bitmap, Gdiplus::Graphics
// --- añadir al inicio del fichero Player.cpp (o justo antes de las funciones) ---
static const float MUZZLE_Y = -20.0f; // offset vertical desde player.pos (pies) hasta el "ojo/muzzle"
static const float MUZZLE_FORWARD = 28.0f; // distancia desde el "muzzle origin" hacia delante
// forward-declare weapon concrete classes constructors are in Weapon.cpp
#include "Weapon.h" // solo aquí, en CPP, para poder crear Pistol/Shotgun
#include <wtypes.h>
#include "Pistol.h"
#include "Shotgun.h"
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

static GameUI g_gameUI;
Player::Player() {
    pos = Vec2(400.0f, 400.0f);
    vel = Vec2(0, 0);
    onGround = false;
    angle = 0.0f;
    weaponType = PISTOL;
    hasShotgun = false;
    pistolMag = 12;
    pistolReserve = 48;
    shotgunMag = 0;
    shotgunReserve = 0;
    isReloading = false;
    reload_ms_remaining = 0;
    bodyAngle = 0.0f;
    angularVelocity = 0.0f;
    ragdoll = false;
    ragdollDragging = false;
    damage_flash_ms = 0;
    EquipWeapon(weaponType);

    // ---- collider size (AABB) en función del TILE_SIZE global ----
    // Ajusta estas fracciones si quieres que el jugador sea más estrecho/alto.
    collisionHalfW = TILE_SIZE * 0.28f; // ~0.56 tiles ancho
    collisionHalfH = TILE_SIZE * 0.45f; // ~0.9 tiles alto (ajústalo)
}

Player::~Player() {}

// ------------------- Update general -------------------
void Player::Update(int dt_ms) {
    // dt_ms es milisegundos desde el frame anterior
    // actualizar timers
    if (stun_ms > 0) {
        stun_ms = std::max(0, stun_ms - dt_ms);
    }
    if (damage_flash_ms > 0) {
        damage_flash_ms = std::max(0, damage_flash_ms - dt_ms);
    }

    // actualizar recarga
    UpdateReload(dt_ms);

    // movimiento básico (si no estás aplicando física en otro sitio)
    // aquí solo integramos velocidad a la posición
    float dt = dt_ms * 0.001f;
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // aplicar fricción simple cuando en tierra
    if (onGround) {
        vel.x *= 0.95f;
    }
    else {
        // gravedad
        vel.y += 980.0f * dt; // pixels / s^2
    }

    // Ragdoll: integrar rotación corporal con amortiguación
    if (ragdoll) {
        // si todavía no tenemos bodyAngle, inicializarlo con angle
        // (esto se hace en OnDeath, pero por seguridad:)
        // integrar velocidad angular
        bodyAngle += angularVelocity * (float)dt_ms;
        // amortiguación (reduce velocidad angular over time)
        const float damping = 0.995f; // por ms (suave), ajustar al gusto
        angularVelocity *= powf(damping, dt_ms);
        // si la velocidad es muy pequeña, estabilizar
        if (fabsf(angularVelocity) < 1e-4f) angularVelocity = 0.0f;
    }
}

void Player::Reset() {
    pos = Vec2(400.0f, 400.0f);
    vel = Vec2(0, 0);
    onGround = false;
    angle = 0.0f;
    weaponType = PISTOL;
    hasShotgun = false;
    pistolMag = 12;
    pistolReserve = 48;
    shotgunMag = 0;
    shotgunReserve = 0;
    isReloading = false;
    reload_ms_remaining = 0;
    reloadWeapon = PISTOL;
    stun_ms = 0;
    damage_flash_ms = 0;
    weapon.reset();
    EquipWeapon(weaponType);

    // Recalcular collider también al resetear
    collisionHalfW = TILE_SIZE * 0.28f;
    collisionHalfH = TILE_SIZE * 0.45f;
}

void Player::EquipWeapon(WeaponType t) {
    if (t == SHOTGUN && !hasShotgun) {
        // si no tiene shotgun, no cambiar; opcional: dar feedback
        weaponType = PISTOL;
        weapon = make_unique<Pistol>();
        return;
    }

    weaponType = t;
    if (t == PISTOL) weapon = make_unique<Pistol>();
    else weapon = make_unique<Shotgun>();
}

// Player.cpp

void Player::UpdateAim(int mouseX, int mouseY, const Vec2& camOffset /*= Vec2(0,0)*/) {
    // Convertir coords de cliente/pantalla a mundo: world = screen + camOffset
    float worldMouseX = float(mouseX) + camOffset.x;
    float worldMouseY = float(mouseY) + camOffset.y;

    // Origen de apuntado: el "muzzle" visual (pos base + offset vertical)
    float muzzleY = pos.y + MUZZLE_Y;
    float muzzleX = pos.x;

    float dx = worldMouseX - muzzleX;
    float dy = worldMouseY - muzzleY;

    // atan2 con el vector desde el muzzle->mouse en coordenadas de mundo
    angle = atan2f(dy, dx);
}

Vec2 Player::GetMuzzlePos() const {
    Vec2 base(pos.x, pos.y + MUZZLE_Y);
    return Vec2(base.x + cosf(angle) * MUZZLE_FORWARD,
        base.y + sinf(angle) * MUZZLE_FORWARD);
}

// ------------------- Draw (usa bodyAngle si ragdoll) -------------------
void Player::Draw(HDC hdc, Gdiplus::Bitmap* /*playerBmp*/) {
    // si estamos en ragdoll usamos bodyAngle, sino angle
    float drawAngle = ragdoll ? bodyAngle : angle;

    float drawX = pos.x;
    float drawY = pos.y + MUZZLE_Y;
    float size = 26.0f;

    POINT pts[3];
    Vec2 tip = Vec2(cosf(drawAngle) * size, sinf(drawAngle) * size);
    Vec2 left = Vec2(cosf(drawAngle + 2.2f) * size * 0.8f, sinf(drawAngle + 2.2f) * size * 0.8f);
    Vec2 right = Vec2(cosf(drawAngle - 2.2f) * size * 0.8f, sinf(drawAngle - 2.2f) * size * 0.8f);
    pts[0].x = (LONG)(drawX + tip.x); pts[0].y = (LONG)(drawY + tip.y);
    pts[1].x = (LONG)(drawX + left.x); pts[1].y = (LONG)(drawY + left.y);
    pts[2].x = (LONG)(drawX + right.x); pts[2].y = (LONG)(drawY + right.y);

    // sombra
    HBRUSH sh = CreateSolidBrush(RGB(10, 10, 10));
    HPEN penNull = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ old = SelectObject(hdc, sh);
    HGDIOBJ oldPen = SelectObject(hdc, penNull);
    POINT shPts[3] = { {pts[0].x + 3, pts[0].y + 3}, {pts[1].x + 3, pts[1].y + 3}, {pts[2].x + 3, pts[2].y + 3} };
    Polygon(hdc, shPts, 3);
    SelectObject(hdc, old);
    SelectObject(hdc, oldPen);
    DeleteObject(sh);
    DeleteObject(penNull);

    // body
    HBRUSH body = CreateSolidBrush(RGB(200, 180, 130));
    HPEN outline = CreatePen(PS_SOLID, 2, RGB(40, 30, 20));
    old = SelectObject(hdc, body);
    oldPen = SelectObject(hdc, outline);
    Polygon(hdc, pts, 3);
    SelectObject(hdc, old);
    SelectObject(hdc, oldPen);
    DeleteObject(body);
    DeleteObject(outline);

    // ojo (si quieres parpadear/flash por daño puedes cambiar color)
    Vec2 eye = Vec2(drawX + cosf(drawAngle) * size * 0.45f, drawY + sinf(drawAngle) * size * 0.45f);
    HBRUSH eyeBrush = CreateSolidBrush(RGB(10, 10, 10));
    old = SelectObject(hdc, eyeBrush);
    Ellipse(hdc, (int)(eye.x - 3), (int)(eye.y - 3), (int)(eye.x + 3), (int)(eye.y + 3));
    SelectObject(hdc, old);
    DeleteObject(eyeBrush);
}
// ------------------- Ragdoll mouse handlers -------------------
void Player::OnRagdollMouseDown(int mouseX, int /*mouseY*/) {
    if (!ragdoll) return;
    ragdollDragging = true;
    ragdollLastMouseX = mouseX;
}

void Player::OnRagdollMouseMove(int mouseX, int /*mouseY*/) {
    if (!ragdoll || !ragdollDragging) return;
    int dx = mouseX - ragdollLastMouseX;
    ragdollLastMouseX = mouseX;

    // convertir dx (px) en torque / cambio angular
    // ajusta la constante al gusto (0.001..0.03 suelen funcionar)
    const float torqueFactor = 0.0008f; // rad per px per ms-equivalente
    angularVelocity += dx * torqueFactor;

    // limitar velocidad angular
    const float maxAngVel = 0.08f;
    if (angularVelocity > maxAngVel) angularVelocity = maxAngVel;
    if (angularVelocity < -maxAngVel) angularVelocity = -maxAngVel;
}

void Player::OnRagdollMouseUp(int /*mouseX*/, int /*mouseY*/) {
    ragdollDragging = false;
}
void Player::StartReload() {
    if (isReloading) return;
    // decide qué arma recargar (si tiene esa arma)
    WeaponType wt = weaponType;
    // Si el jugador no tiene la weaponType seleccionada (ej shotgun), cambia a pistola
    if (wt == SHOTGUN && !hasShotgun) wt = PISTOL;

    // obtener reload time y magazine size desde weapon temporal
    int reloadMs = 800;
    int magSize = 1;
    if (wt == PISTOL) {
        Pistol tmp; // temporal para leer config
        reloadMs = tmp.GetReloadMs();
        magSize = tmp.GetMagazineSize();
    }
    else if (wt == SHOTGUN) {
        Shotgun tmp;
        reloadMs = tmp.GetReloadMs();
        magSize = tmp.GetMagazineSize();
    }

    // comprobar si ya está lleno o no hay reserva
    int& magRef = (wt == PISTOL) ? pistolMag : shotgunMag;
    int& resRef = (wt == PISTOL) ? pistolReserve : shotgunReserve;
    if (magRef >= magSize) return;
    if (resRef <= 0) return;

    // arrancar reload
    isReloading = true;
    reload_ms_remaining = reloadMs;
    reloadWeapon = wt;
}

void Player::UpdateReload(int dt_ms) {
    if (!isReloading) return;
    reload_ms_remaining = std::max<int>(0, reload_ms_remaining - dt_ms);
    if (reload_ms_remaining > 0) return;

    // transferir munición de reserva al cargador
    if (reloadWeapon == PISTOL) {
        Pistol tmp; int magSize = tmp.GetMagazineSize();
        int need = magSize - pistolMag;
        int take = std::min<int>(need, pistolReserve);
        pistolMag += take;
        pistolReserve -= take;
    }
    else if (reloadWeapon == SHOTGUN) {
        Shotgun tmp; int magSize = tmp.GetMagazineSize();
        int need = magSize - shotgunMag;
        int take = std::min<int>(need, shotgunReserve);
        shotgunMag += take;
        shotgunReserve -= take;
    }

    isReloading = false;
    reload_ms_remaining = 0;
}
// ---------- daño / muerte ----------
void Player::ApplyDamage(int amount, const Vec2& source) {
    if (!alive) return;
    if (amount <= 0) return;

    health -= amount;
    if (health < 0) health = 0;

    // start damage flash so sprite can show hurt expression briefly
    damage_flash_ms = 400; // dura 400ms (ajusta a gusto)

    int stunCandidate = amount * 60;
    if (stunCandidate < 80) stunCandidate = 80;
    if (stunCandidate > 1200) stunCandidate = 1200;
    if (stunCandidate > stun_ms) stun_ms = stunCandidate;

    if (!(source.x == 0.0f && source.y == 0.0f)) {
        float dx = pos.x - source.x;
        float dy = pos.y - source.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 1e-4f) {
            dx /= len; dy /= len;
            float push = 40.0f + amount * 6.0f;
            vel.x += dx * push;
            vel.y += dy * (push * 0.5f);
        }
    }

    if (health == 0) {
        alive = false;
        OnDeath();
    }
}

void Player::OnDeath() {
    ragdoll = true;
    bodyAngle = angle; // start from current aim orientation
    angularVelocity = 0.0f;
    vel.y -= 220.0f;

    // marca muerte y muestra pantalla
    alive = false;

    // abrir la death screen en el UI
  //  g_gameUI.SetupDeathPanel(); // o g_gameUI.ShowDeathPanel(); (ver cambios abajo)
}
void Player::ResolveTileCollisions(float dt) {
    if (!g_currentLevel) return;
    const float TILE = TILE_SIZE;
    Level* L = g_currentLevel.get();
    if (!L) return;

    const float hw = collisionHalfW;
    const float hh = collisionHalfH;

    const float eps = 0.001f;
    // stepHeight relativo al TILE (permitir subir pequeños escalones)
    const float stepHeight = std::min((float)TILE * 0.5f, 16.0f);

    auto getAABB = [&](float cx, float cy, float& l, float& t, float& r, float& b) {
        l = cx - hw; r = cx + hw;
        t = cy - hh; b = cy + hh;
        };

    float l, t, r, b;
    getAABB(pos.x, pos.y, l, t, r, b);

    // rango de tiles a chequear (clamp)
    int minC = std::max(0, (int)std::floor(l / TILE));
    int maxC = std::min(L->width - 1, (int)std::floor(r / TILE));
    int minR = std::max(0, (int)std::floor(t / TILE));
    int maxR = std::min(L->height - 1, (int)std::floor(b / TILE));

    auto tileAABB = [&](int tr, int tc, float& tl, float& tt, float& tr_, float& tb) {
        tl = tc * TILE; tt = tr * TILE;
        tr_ = tl + TILE; tb = tt + TILE;
        };

    bool anyGround = false;

    // Haremos hasta N passes para resolver penetraciones múltiples (normalmente 2-3 bastan)
    const int MAX_PASSES = 4;
    for (int pass = 0; pass < MAX_PASSES; ++pass) {
        getAABB(pos.x, pos.y, l, t, r, b);
        minC = std::max(0, (int)std::floor(l / TILE));
        maxC = std::min(L->width - 1, (int)std::floor(r / TILE));
        minR = std::max(0, (int)std::floor(t / TILE));
        maxR = std::min(L->height - 1, (int)std::floor(b / TILE));

        bool anyPenetration = false;

        for (int rr = minR; rr <= maxR; ++rr) {
            for (int cc = minC; cc <= maxC; ++cc) {
                Material m = L->TileMaterial(rr, cc);
                if (!GetMaterialInfo(m).solid) continue;

                float tl, tt, tr_, tb;
                tileAABB(rr, cc, tl, tt, tr_, tb);

                // Intersección AABB
                float ix0 = std::max(l, tl);
                float iy0 = std::max(t, tt);
                float ix1 = std::min(r, tr_);
                float iy1 = std::min(b, tb);

                if (ix1 > ix0 + eps && iy1 > iy0 + eps) {
                    anyPenetration = true;

                    float overlapX = ix1 - ix0;
                    float overlapY = iy1 - iy0;

                    // Resolver por mínima penetración (MTV)
                    if (overlapX < overlapY) {
                        // desplazar en X
                        float tileCenterX = (tl + tr_) * 0.5f;
                        if (pos.x < tileCenterX) {
                            // jugador a la izquierda -> empujar a la izquierda
                            pos.x -= (overlapX + eps);
                            if (vel.x > 0.0f) vel.x = 0.0f;
                        }
                        else {
                            pos.x += (overlapX + eps);
                            if (vel.x < 0.0f) vel.x = 0.0f;
                        }
                    }
                    else {
                        // desplazar en Y
                        float tileCenterY = (tt + tb) * 0.5f;
                        if (pos.y < tileCenterY) {
                            // jugador arriba del tile -> aterrizó encima
                            pos.y -= (overlapY + eps);
                            if (vel.y > 0.0f) vel.y = 0.0f;
                            anyGround = true;
                        }
                        else {
                            // jugador debajo -> choqué la cabeza
                            pos.y += (overlapY + eps);
                            if (vel.y < 0.0f) vel.y = 0.0f;
                        }
                    }
                    // recomputar AABB localmente (siguiente iteración de tiles usará nueva pos)
                    getAABB(pos.x, pos.y, l, t, r, b);
                }
            }
        } // tiles loop

        if (!anyPenetration) break; // ya no hay intersecciones
    } // passes

    // Si no estamos en terreno pero actualmente hay bloqueo lateral, intentar "step-up"
    if (!anyGround) {
        // comprobar tiles a los lados justo por fuera del AABB
        getAABB(pos.x, pos.y, l, t, r, b);
        float checkLeftX = l - 1.0f;
        float checkRightX = r + 1.0f;
        int cLeft = std::max(0, (int)std::floor(checkLeftX / TILE));
        int cRight = std::min(L->width - 1, (int)std::floor(checkRightX / TILE));
        int rowBelow = std::min(L->height - 1, (int)std::floor(b / TILE));

        // detectar si hay un bloque inmediatamente al lado que nos impidió avanzar
        bool blockedLeft = false, blockedRight = false;
        auto intersectsTile = [&](float ax0, float ay0, float ax1, float ay1, int tr, int tc)->bool {
            float tl, tt, tr_, tb; tileAABB(tr, tc, tl, tt, tr_, tb);
            if (ax1 <= tl + eps) return false;
            if (ax0 >= tr_ - eps) return false;
            if (ay1 <= tt + eps) return false;
            if (ay0 >= tb - eps) return false;
            return true;
            };

        getAABB(pos.x, pos.y, l, t, r, b);
        if (cLeft >= 0 && cLeft < L->width) {
            if (intersectsTile(l, t, r, b, rowBelow, cLeft)) blockedLeft = true;
        }
        if (cRight >= 0 && cRight < L->width) {
            if (intersectsTile(l, t, r, b, rowBelow, cRight)) blockedRight = true;
        }

        if (blockedLeft || blockedRight) {
            // intentar subir un poco y comprobar si queda libre
            float tryY = pos.y - stepHeight;
            float tl, tt, tr_, tb;
            getAABB(pos.x, tryY, tl, tt, tr_, tb);
            int minC2 = std::max(0, (int)std::floor(tl / TILE));
            int maxC2 = std::min(L->width - 1, (int)std::floor(tr_ / TILE));
            int minR2 = std::max(0, (int)std::floor(tt / TILE));
            int maxR2 = std::min(L->height - 1, (int)std::floor(tb / TILE));
            bool coll = false;
            for (int rr = minR2; rr <= maxR2 && !coll; ++rr) {
                for (int cc = minC2; cc <= maxC2; ++cc) {
                    if (!GetMaterialInfo(L->TileMaterial(rr, cc)).solid) continue;
                    if (intersectsTile(tl, tt, tr_, tb, rr, cc)) { coll = true; break; }
                }
            }
            if (!coll) {
                // subir
                pos.y = tryY;
                // después de subir, intentar "aterrizar" en bloque bajo
                getAABB(pos.x, pos.y, l, t, r, b);
                int rr = std::min(L->height - 1, (int)std::floor(b / TILE));
                for (int cc = std::max(0, (int)std::floor(l / TILE)); cc <= std::min(L->width - 1, (int)std::floor(r / TILE)); ++cc) {
                    if (!GetMaterialInfo(L->TileMaterial(rr, cc)).solid) continue;
                    float tileT = rr * TILE;
                    if (b > tileT && pos.y < tileT) {
                        pos.y = tileT - hh - eps;
                        vel.y = std::min(vel.y, 0.0f);
                        anyGround = true;
                    }
                }
            }
        }
    }

    onGround = anyGround;
}
