// weapon_hud.cpp
#include "weapon_hud.h"
#include "Player.h"
#include "Pistol.h"
#include "Shotgun.h"
#include <string>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
WeaponHUD::WeaponHUD() {}
WeaponHUD::~WeaponHUD() {}

void WeaponHUD::Init(HWND mainWnd) { m_mainWnd = mainWnd; }

void WeaponHUD::OnResize(int w, int h, float scale) {
    (void)w; (void)h;
    m_scale = scale;
    m_x = (int)(12 * scale);
    m_y = (int)(36 * scale);
}

void WeaponHUD::Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>&, bool, bool) {
    const wchar_t* wname = (player.weaponType == PISTOL) ? L"Pistol" : L"Shotgun";
    int mag = (player.weaponType == PISTOL) ? player.pistolMag : player.shotgunMag;
    int res = (player.weaponType == PISTOL) ? player.pistolReserve : player.shotgunReserve;
    wchar_t buf[128];
    swprintf_s(buf, L"Weapon: %s   Mag: %d   Reserve: %d", wname, mag, res);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(230, 230, 230));
    RECT tr = { m_x, m_y, m_x + 800, m_y + 20 };
    DrawTextW(hdc, buf, -1, &tr, DT_LEFT | DT_SINGLELINE);

    // reload bar (if reloading)
    if (player.isReloading) {
        int total = (player.reloadWeapon == PISTOL) ? Pistol().GetReloadMs() : Shotgun().GetReloadMs();
        if (total <= 0) total = 1;
        int left = player.reload_ms_remaining;
        float pct = 1.0f - (float)left / (float)total;
        if (pct < 0) pct = 0; if (pct > 1) pct = 1;
        int bx = m_x, by = m_y + 20, bw = 120, bh = 8;
        RECT back = { bx, by, bx + bw, by + bh };
        HBRUSH bb = CreateSolidBrush(RGB(40, 40, 40)); FillRect(hdc, &back, bb); DeleteObject(bb);
        RECT fill = { bx, by, bx + (int)(bw * pct), by + bh };
        HBRUSH ff = CreateSolidBrush(RGB(80, 200, 80)); FillRect(hdc, &fill, ff); DeleteObject(ff);
    }
}
