// health_hud.h
#pragma once
#include "hud.h"

struct Player;

class HealthHUD : public HUD {
public:
    HealthHUD();
    ~HealthHUD() override;

    void Init(HWND mainWnd) override;
    void OnResize(int w, int h, float scale) override;
    void Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups, bool paused) override;

private:
    int m_x = 10, m_y = 10;
    int m_w = 200, m_h = 18;
    float m_scale = 1.0f;
    HWND m_mainWnd = NULL;
};
