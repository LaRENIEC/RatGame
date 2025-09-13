#include "pickups_hud.h"
#include <windows.h>
#include "Player.h"
#include "GameMovement.h" // en caso de que quieras usar definiciones

void PickupsHUD::Render(HDC hdc, const Player& /*player*/, const std::vector<WeaponPickup>& /*pickupsParam*/,
    bool /*showPickupsParam*/, bool /*paused*/)
{
    if (!showPickups) return;

    int i = 0;
    for (const auto& pk : pickups) {
        wchar_t buf[128];
        const wchar_t* name = (pk.wtype == SHOTGUN) ? L"Shotgun" : L"Pistol";
        swprintf_s(buf, L"Pickup %d: %s (%.0f, %.0f)", i, name, pk.pos.x, pk.pos.y);
        RECT tr = { 10, 120 + i * 18, 800, 120 + i * 18 + 16 };
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(220, 200, 160));
        DrawTextW(hdc, buf, -1, &tr, DT_LEFT | DT_SINGLELINE);
        ++i;
    }
}
