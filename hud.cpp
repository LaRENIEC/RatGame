#include "hud.h"
#include <gdiplus.h>
#include <string>
#include "Player.h" // forward: tu Player.h
#include "MaterialTextures.h" // para dibujar pickups (opcional)
#include "Weapon.h"
#include "GameUI.h"
#include "CommonTypes.h"
#include "GameMovement.h"
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif
using namespace Gdiplus;

void HUD::Init(HWND mainWnd) {
    m_mainWnd = mainWnd;
    RECT rc; GetClientRect(mainWnd, &rc);
    m_w = rc.right - rc.left; m_h = rc.bottom - rc.top;
    m_scale = 1.0f;
    m_pauseBtnRect = { m_w - 44, 8, m_w - 8, 8 + 28 }; // top-right small button by default
    BuildPausePanel();
}

void HUD::Shutdown() {
    m_mainWnd = NULL;
    m_panelButtons.clear();
}

void HUD::OnResize(int w, int h, float scale) {
    m_w = w; m_h = h; m_scale = scale;
    m_pauseBtnRect = { m_w - 44, 8, m_w - 8, 8 + 28 };
    BuildPausePanel();
}

void HUD::BuildPausePanel() {
    // centered modal
    int pw = std::max(240, (int)(320 * m_scale));
    int ph = std::max(140, (int)(180 * m_scale));
    int px = (m_w - pw) / 2;
    int py = (m_h - ph) / 2;
    m_panelRect = { px, py, px + pw, py + ph };
    m_panelButtons.clear();

    int bw = std::max(120, (int)(120 * m_scale));
    int bh = std::max(28, (int)(30 * m_scale));
    int bx = px + (pw - bw) / 2;
    int by = py + 28;

    m_panelButtons.push_back({ {bx, by, bx + bw, by + bh}, L"Resume", 1 });
    m_panelButtons.push_back({ {bx, by + (int)(bh + 8), bx + bw, by + (int)(bh + 8) + bh}, L"Toggle Pickups", 2 });
    m_panelButtons.push_back({ {bx, by + 2 * (bh + 8), bx + bw, by + 2 * (bh + 8) + bh}, L"Quit to Menu", 3 });
}

void HUD::Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups, bool paused) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeHighQuality);

    // --- healthbar & weapon info (top-left) ---
    int hbW = 200, hbH = 18;
    int hx = 12, hy = 12;
    RectF hbBg((REAL)hx - 2.0f, (REAL)hy - 2.0f, (REAL)hbW + 4.0f, (REAL)hbH + 4.0f);
    SolidBrush bgBrush(Color(160, 20, 20, 20));
    Pen outline(Color(255, 30, 30, 30), 1.0f);
    // rounded background
    {
        // small helper path
        GraphicsPath path;
        RectF r(hbBg);
        REAL rad = 6.0f;
        path.AddArc(r.X, r.Y, rad * 2, rad * 2, 180, 90);
        path.AddArc(r.X + r.Width - rad * 2, r.Y, rad * 2, rad * 2, 270, 90);
        path.AddArc(r.X + r.Width - rad * 2, r.Y + r.Height - rad * 2, rad * 2, rad * 2, 0, 90);
        path.AddArc(r.X, r.Y + r.Height - rad * 2, rad * 2, rad * 2, 90, 90);
        path.CloseFigure();
        g.FillPath(&bgBrush, &path);
        g.DrawPath(&outline, &path);
    }

    float pct = 0.0f;
    if (player.maxHealth != 0) pct = (float)player.health / (float)player.maxHealth;
    pct = std::max(0.0f, std::min(1.0f, pct)); // atención al tipo: 0.0f y 1.0f son float
    RectF fillRect((REAL)hx, (REAL)hy, (REAL)(hbW * pct), (REAL)hbH);
    SolidBrush fg(Color(255, (int)(200 * (1.0f - pct)), (int)(40 + 160 * pct), 40));
    g.FillRectangle(&fg, fillRect);

    FontFamily ff(L"Segoe UI");
    Font font(&ff, 10.0f, FontStyleRegular, UnitPixel);
    SolidBrush textBrush(Color(230, 230, 230, 230));
    std::wstring hpText = std::to_wstring(player.health) + L" / " + std::to_wstring(player.maxHealth);
    PointF tp((REAL)hx + hbW / 2.0f, (REAL)hy + hbH / 2.0f);
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(hpText.c_str(), -1, &font, tp, &sf, &textBrush);

    // Weapon text
    std::wstring wname = (player.weaponType == PISTOL) ? L"Pistol" : L"Shotgun";
    std::wstring hud = L"Weapon: " + wname + L"   Mag: " + std::to_wstring((player.weaponType == PISTOL) ? player.pistolMag : player.shotgunMag)
        + L"   Reserve: " + std::to_wstring((player.weaponType == PISTOL) ? player.pistolReserve : player.shotgunReserve);
    PointF hudPos(12.0f, 12.0f + hbH + 6.0f);
    g.DrawString(hud.c_str(), -1, &font, hudPos, &textBrush);

    // --- pause button (top-right) ---
    RectF pbr((REAL)m_pauseBtnRect.left, (REAL)m_pauseBtnRect.top, (REAL)(m_pauseBtnRect.right - m_pauseBtnRect.left), (REAL)(m_pauseBtnRect.bottom - m_pauseBtnRect.top));
    SolidBrush pbBack(paused ? Color(255, 200, 80, 80) : Color(255, 80, 80, 80));
    g.FillRectangle(&pbBack, pbr);

    Font pbFont(&ff, 12.0f, FontStyleBold, UnitPixel);
    SolidBrush pbText(Color(240, 240, 240, 240));
    PointF pbTxt(pbr.X + pbr.Width / 2.0f, pbr.Y + pbr.Height / 2.0f);
    StringFormat center; center.SetAlignment(StringAlignmentCenter); center.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(paused ? L"PAUSED" : L"PAUSE", -1, &pbFont, pbTxt, &center, &pbText);

    // pickups drawing (world coords: keep the same approach as before)
    if (showPickups) {
        HBRUSH pickupBrush = CreateSolidBrush(RGB(180, 80, 20));
        HGDIOBJ oldBrush = SelectObject(hdc, pickupBrush);
        for (auto& pk : pickups) {
            RECT pr = { (int)(pk.pos.x - 10), (int)(pk.pos.y - 10), (int)(pk.pos.x + 10), (int)(pk.pos.y + 10) };
            FillRect(hdc, &pr, pickupBrush);
        }
        SelectObject(hdc, oldBrush);
        DeleteObject(pickupBrush);
    }

    // --- pause modal (if paused) ---
    if (paused) {
        // dim
        SolidBrush dim(Color(160, 0, 0, 0));
        g.FillRectangle(&dim, 0.0f, 0.0f, (REAL)m_w, (REAL)m_h);

        // panel background
        RectF panel((REAL)m_panelRect.left, (REAL)m_panelRect.top, (REAL)(m_panelRect.right - m_panelRect.left), (REAL)(m_panelRect.bottom - m_panelRect.top));
        SolidBrush panelBg(Color(255, 30, 30, 30));
        g.FillRectangle(&panelBg, panel);

        // title
        Font titleF(&ff, 18.0f, FontStyleBold, UnitPixel);
        SolidBrush titleBrush(Color(245, 245, 245, 245));
        PointF tpos((REAL)m_panelRect.left + 12.0f, (REAL)m_panelRect.top + 8.0f);
        g.DrawString(L"Paused", -1, &titleF, tpos, &titleBrush);

        // buttons
        for (auto& b : m_panelButtons) {
            RectF br((REAL)b.rc.left, (REAL)b.rc.top, (REAL)(b.rc.right - b.rc.left), (REAL)(b.rc.bottom - b.rc.top));
            SolidBrush btnBg(b.pressed ? Color(255, 80, 160, 110) : (b.hover ? Color(255, 110, 160, 130) : Color(255, 90, 120, 140)));
            g.FillRectangle(&btnBg, br);
            Pen btnOutline(Color(200, 30, 30, 30), 1.0f);   // lvalue válido
            g.DrawRectangle(&btnOutline, br);
            Font f2(&ff, 12.0f, FontStyleBold, UnitPixel);
            g.DrawString(b.text.c_str(), -1, &f2, PointF(br.X + br.Width / 2.0f, br.Y + br.Height / 2.0f), &center, &textBrush);
    
        }
    }
}

bool HUD::OnMouseMove(int x, int y) {
    bool changed = false;
    // hover pause button
    bool pbHover = PointInRect(m_pauseBtnRect, x, y);
    // we don't store hover state for pause button, but we can invalidate if needed
    // hover on panel buttons if paused
    if (m_paused) {
        for (auto& b : m_panelButtons) {
            bool h = PointInRect(b.rc, x, y);
            if (h != b.hover) { b.hover = h; changed = true; }
        }
    }
    if (changed && m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
    return true; // HUD consumes the input when active/hovering pause area (modal-like)
}

bool HUD::OnLButtonDown(int x, int y) {
    if (PointInRect(m_pauseBtnRect, x, y)) {
        m_mouseDownOnPause = true;
        if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
        return true;
    }
    if (m_paused) {
        for (auto& b : m_panelButtons) {
            if (PointInRect(b.rc, x, y)) { b.pressed = true; if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE); }
        }
        SetCapture(m_mainWnd);
        return true;
    }
    return false;
}

bool HUD::OnLButtonUp(int x, int y) {
    if (m_mouseDownOnPause && PointInRect(m_pauseBtnRect, x, y)) {
        // toggle paused via WM message (main WndProc toggles g_gamePaused)
        PostMessage(m_mainWnd, MSG_TOGGLE_PAUSE, 0, 0);
        m_mouseDownOnPause = false;
        if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
        return true;
    }
    m_mouseDownOnPause = false;

    if (m_paused) {
        for (auto& b : m_panelButtons) {
            if (b.pressed) {
                b.pressed = false;
                bool inside = PointInRect(b.rc, x, y);
                if (inside) {
                    // button actions:
                    switch (b.id) {
                    case 1: // Resume
                        PostMessage(m_mainWnd, MSG_TOGGLE_PAUSE, 0, 0);
                        break;
                    case 2: // Toggle Pickups
                        PostMessage(m_mainWnd, MSG_TOGGLE_PICKUPS, 0, 0);
                        break;
                    case 3: // Quit to menu
                        // show start panel and unpause (handled in WndProc by MSG_SHOW_START_MENU)
                        PostMessage(m_mainWnd, MSG_SHOW_START_MENU, 0, 0);
                        break;
                    default:
                        break;
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
