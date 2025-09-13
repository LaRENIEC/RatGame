// pause_hud.h
#pragma once
#include "hud.h"
#include <vector>
#include <string>

class PauseHUD : public HUD {
public:
    PauseHUD();
    ~PauseHUD() override;

    void Init(HWND mainWnd) override;
    void OnResize(int w, int h, float scale) override;
    void Render(HDC hdc, const Player&, const std::vector<WeaponPickup>&, bool, bool paused) override;
    bool OnMouseMove(int x, int y) override;
    bool OnLButtonDown(int x, int y) override;
    bool OnLButtonUp(int x, int y) override;
    void SetPaused(bool p) override;

private:
    HWND m_mainWnd = NULL;
    RECT m_pauseBtnRect = { 0,0,0,0 };
    RECT m_panelRect = { 0,0,0,0 };
    struct Btn { RECT rc; std::wstring text; int id; bool pressed=false, hover=false; };
    std::vector<Btn> m_buttons;
    bool m_mouseDownOnPause = false;
    bool m_paused = false;
    float m_scale = 1.0f;

    void BuildPanel();
    bool PointInRect(const RECT& r, int x, int y) const { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }
};
