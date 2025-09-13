// weapon_hud.h
#pragma once
#include "hud.h"

class WeaponHUD : public HUD {
public:
    WeaponHUD();
    ~WeaponHUD() override;

    void Init(HWND mainWnd) override;
    void OnResize(int w, int h, float scale) override;
    void Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups, bool paused) override;

private:
    int m_x = 12, m_y = 36;
    float m_scale = 1.0f;
    HWND m_mainWnd = NULL;
};
