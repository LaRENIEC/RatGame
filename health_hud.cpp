// health_hud.cpp
#include "health_hud.h"
#include "Player.h"
#include <string>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
HealthHUD::HealthHUD() {}
HealthHUD::~HealthHUD() {}

void HealthHUD::Init(HWND mainWnd) {
    m_mainWnd = mainWnd;
}

void HealthHUD::OnResize(int w, int h, float scale) {
    (void)h;
    m_scale = scale;
    m_w = std::max(100, (int)(200 * scale));
    m_h = std::max(10, (int)(18 * scale));
    m_x = 10;
    m_y = 10;
}

void HealthHUD::Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>&, bool, bool) {
    // background rect (small framed background)
    RECT bg = { m_x - 2, m_y - 2, m_x + m_w + 2, m_y + m_h + 2 };
    HBRUSH brBg = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &bg, brBg);
    DeleteObject(brBg);

    // fill by percentage
    float pct = 0.0f;
    if (player.maxHealth > 0) pct = (float)player.health / (float)player.maxHealth;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    RECT fill = { m_x, m_y, m_x + (int)(m_w * pct), m_y + m_h };
    HBRUSH brFill = CreateSolidBrush(RGB(200, 40, 40));
    FillRect(hdc, &fill, brFill);
    DeleteObject(brFill);

    // health text centered
    wchar_t buf[64];
    swprintf_s(buf, L"%d / %d", player.health, player.maxHealth);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(230, 230, 230));
    RECT tr = { m_x, m_y, m_x + m_w, m_y + m_h };
    DrawTextW(hdc, buf, -1, &tr, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
}
