#pragma once
#include <windows.h>
#include <vector>
#include <string>

struct Player;
struct WeaponPickup;

#define MSG_TOGGLE_PAUSE (WM_USER + 50)

class HUD {
public:
    HUD() {}
    ~HUD() {}

    void Init(HWND mainWnd);
    void Shutdown();

    // Render onto the backbuffer HDC (same contract as GameUI::RenderHUD)
    void Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups, bool paused);

    // resize (optional scale arg if you use it)
    void OnResize(int w, int h, float scale = 1.0f);

    // input for HUD (returns true if consumed)
    bool OnMouseMove(int x, int y);
    bool OnLButtonDown(int x, int y);
    bool OnLButtonUp(int x, int y);

    // toggle whether pause-menu is active (the game loop also toggles paused global)
    void SetPaused(bool p) { m_paused = p; }

private:
    HWND m_mainWnd = NULL;
    int m_w = 0, m_h = 0;
    float m_scale = 1.0f;

    // pause button geometry (screen coords)
    RECT m_pauseBtnRect = { 8, 8, 8 + 36, 8 + 24 };

    // pause-panel buttons (resume/options/toggle pickups/quit->menu)
    RECT m_panelRect = { 0,0,0,0 };
    struct Btn { RECT rc; std::wstring text; int id; bool pressed = false, hover = false; };
    std::vector<Btn> m_panelButtons;

    bool m_paused = false;
    bool m_mouseDownOnPause = false;

    void BuildPausePanel();
    bool PointInRect(const RECT& r, int x, int y) const { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }
};
