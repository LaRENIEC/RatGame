// pause_hud.cpp
#include "pause_hud.h"
#include <windows.h>
#include "GameUI.h"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

PauseHUD::PauseHUD() {}
PauseHUD::~PauseHUD() {}

void PauseHUD::Init(HWND mainWnd) {
    m_mainWnd = mainWnd;
    BuildPanel();
}

void PauseHUD::OnResize(int w, int h, float scale) {
    m_scale = scale;
    // top-right pause button
    m_pauseBtnRect = { w - 44, 8, w - 8, 8 + 28 };
    BuildPanel();
}

void PauseHUD::BuildPanel() {
    int pw = std::max(240, (int)(320 * m_scale));
    int ph = std::max(140, (int)(180 * m_scale));
    int px = (int)((400) - pw / 2);
    int py = (int)((240) - ph / 2);
    m_panelRect = { px, py, px + pw, py + ph };
    m_buttons.clear();
    int bw = std::max(120, (int)(120 * m_scale));
    int bh = std::max(28, (int)(30 * m_scale));
    int bx = px + (pw - bw) / 2;
    int by = py + 28;
    m_buttons.push_back({ {bx, by, bx + bw, by + bh}, L"Resume", 1 });
    m_buttons.push_back({ {bx, by + (int)(bh + 8), bx + bw, by + (int)(bh + 8) + bh}, L"Toggle Pickups", 2 });
    m_buttons.push_back({ {bx, by + 2 * (bh + 8), bx + bw, by + 2 * (bh + 8) + bh}, L"Quit to Menu", 3 });
}

void PauseHUD::Render(HDC hdc, const Player&, const std::vector<WeaponPickup>&, bool, bool paused) {
    // draw small pause button
    HBRUSH btn = CreateSolidBrush(paused ? RGB(200, 80, 80) : RGB(80, 80, 80));
    FillRect(hdc, &m_pauseBtnRect, btn); DeleteObject(btn);
    SetTextColor(hdc, RGB(240, 240, 240)); SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, paused ? L"PAUSED" : L"PAUSE", -1, &m_pauseBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (paused) {
        // dim whole screen (simple dark overlay)
        RECT full = { 0,0,10000,10000 };
        HBRUSH dim = CreateSolidBrush(RGB(12, 12, 12));
        FillRect(hdc, &full, dim); DeleteObject(dim);

        // panel bg
        HBRUSH bg = CreateSolidBrush(RGB(40, 40, 40));
        FillRect(hdc, &m_panelRect, bg); DeleteObject(bg);

        // title
        RECT title = { m_panelRect.left + 12, m_panelRect.top + 8, m_panelRect.right, m_panelRect.top + 36 };
        SetTextColor(hdc, RGB(245, 245, 245));
        DrawTextW(hdc, L"Paused", -1, &title, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        // buttons
        for (auto& b : m_buttons) {
            HBRUSH bb = CreateSolidBrush(b.pressed ? RGB(80, 160, 110) : (b.hover ? RGB(110, 160, 130) : RGB(90, 120, 140)));
            FillRect(hdc, &b.rc, bb); DeleteObject(bb);
            SetTextColor(hdc, RGB(230, 230, 230));
            DrawTextW(hdc, b.text.c_str(), -1, &b.rc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        }
    }
}

bool PauseHUD::OnMouseMove(int x, int y) {
    bool changed = false;
    if (m_paused) {
        for (auto& b : m_buttons) {
            bool h = PointInRect(b.rc, x, y);
            if (h != b.hover) { b.hover = h; changed = true; }
        }
    }
    if (changed && m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
    return PointInRect(m_pauseBtnRect, x, y) || (m_paused && changed);
}

bool PauseHUD::OnLButtonDown(int x, int y) {
    if (PointInRect(m_pauseBtnRect, x, y)) {
        m_mouseDownOnPause = true;
        if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
        return true;
    }
    if (m_paused) {
        for (auto& b : m_buttons) if (PointInRect(b.rc, x, y)) { b.pressed = true; if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE); }
        SetCapture(m_mainWnd);
        return true;
    }
    return false;
}

bool PauseHUD::OnLButtonUp(int x, int y) {
    if (m_mouseDownOnPause && PointInRect(m_pauseBtnRect, x, y)) {
        PostMessage(m_mainWnd, MSG_TOGGLE_PAUSE, 0, 0);
        m_mouseDownOnPause = false;
        if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
        return true;
    }
    m_mouseDownOnPause = false;

    if (m_paused) {
        for (auto& b : m_buttons) {
            if (b.pressed) {
                b.pressed = false;
                bool inside = PointInRect(b.rc, x, y);
                if (inside) {
                    switch (b.id) {
                    case 1: PostMessage(m_mainWnd, MSG_TOGGLE_PAUSE, 0, 0); break; // Resume
                    case 2: PostMessage(m_mainWnd, MSG_TOGGLE_PICKUPS, 0, 0); break; // Toggle pickups
                    case 3: PostMessage(m_mainWnd, MSG_SHOW_START_MENU, 0, 0); break; // Quit to menu
                    }
                }
            }
        }
        ReleaseCapture();
        if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
        return true;
    }
    return false;
}

void PauseHUD::SetPaused(bool p) { m_paused = p; if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE); }
