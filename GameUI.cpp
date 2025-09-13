// GameUI.cpp - renderiza paneles directamente sobre el HDC (backbuffer), sin child windows.
#include "GameUI.h"
#include <gdiplus.h>
#include <algorithm>
#include <windowsx.h>
#include "Weapon.h"
#include "GameMovement.h"

using namespace Gdiplus;
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif
static inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}
static void DrawRoundedRect(Graphics& g, const RectF& r, REAL radius, Brush* brush, const Pen* pen = nullptr) {
    GraphicsPath path;
    REAL x = r.X, y = r.Y, w = r.Width, h = r.Height, d = radius * 2.0f;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
    if (brush) g.FillPath(brush, &path);
    if (pen) g.DrawPath(const_cast<Pen*>(pen), &path);
}

GameUI::GameUI() {}
GameUI::~GameUI() { Shutdown(); }
void GameUI::Init(HINSTANCE hInstance, HWND mainWnd) { m_hInst = hInstance; m_mainWnd = mainWnd; }
void GameUI::Shutdown() { ClearPanel(); m_hInst = NULL; m_mainWnd = NULL; }

void GameUI::SetupStartPanel() {
    m_buttons.clear();
    m_title.clear();

    // Tamaños relativos
    int pad = (int)std::lround(14.0f * m_scale);
    int btnW = (int)std::lround(160.0f * m_scale);
    int btnH = (int)std::lround(48.0f * m_scale);
    int spacing = (int)std::lround(12.0f * m_scale);

    // Título y layout
    m_title = L"RAT GAME";
    int cx = m_panelX + m_panelW / 2;

    // calcular posiciones Y relativas dentro del panel
    int titleY = m_panelY + (int)std::lround(28.0f * m_scale);
    int firstBtnY = m_panelY + (int)std::lround(110.0f * m_scale);

    // helper para crear botón centrado
    auto addBtnCentered = [&](const std::wstring& text, int id, int idx) {
        int bx = cx - btnW / 2;
        int by = firstBtnY + idx * (btnH + spacing);
        Button b;
        b.rc = { bx, by, bx + btnW, by + btnH };
        b.text = text;
        b.id = id;
        b.hover = b.pressed = false;
        m_buttons.push_back(std::move(b));
        };

    addBtnCentered(L"Start", ID_START_BTN, 0);
    addBtnCentered(L"Levels", ID_SINGLEPLAYER_BTN, 1);
    addBtnCentered(L"Editor", ID_LEVEL_EDITOR_BTN, 2);
    addBtnCentered(L"Exit", ID_EXIT_BTN, 3);
}

void GameUI::SetupLevelsPanel() {
    m_buttons.clear();
    m_title = L"Choose Level";

    int pad = (int)std::lround(12.0f * m_scale);
    int btnW = (int)std::lround(120.0f * m_scale);
    int btnH = (int)std::lround(40.0f * m_scale);
    int spacing = (int)std::lround(8.0f * m_scale);

    int cx = m_panelX + m_panelW / 2;
    int startY = m_panelY + (int)std::lround(100.0f * m_scale);

    // Un par de botones: Play / Cancel
    Button play;
    play.rc = { cx - btnW - spacing / 2, startY, cx - spacing / 2, startY + btnH };
    play.text = L"Play"; play.id = ID_LEVEL_PLAY_BTN;
    m_buttons.push_back(play);

    Button cancel;
    cancel.rc = { cx + spacing / 2, startY, cx + btnW + spacing / 2, startY + btnH };
    cancel.text = L"Cancel"; cancel.id = ID_LEVEL_CANCEL_BTN;
    m_buttons.push_back(cancel);
}


void GameUI::ClearPanel() {
    m_buttons.clear();
    m_title.clear();
    m_active = Panel::None;
}

// API
void GameUI::ShowStartPanel() {
    m_active = Panel::Start;
    // si OnResize ya seteo m_scale/pos, construye la geometría
    SetupStartPanel();
}

void GameUI::ShowLevelsPanel() {
    m_active = Panel::Levels;
    SetupLevelsPanel();
}
void GameUI::HideStartPanel() { ClearPanel(); }
void GameUI::HideLevelsPanel() { ClearPanel(); }
bool GameUI::HasActivePanel() const { return m_active != Panel::None; }

// --- Render panel(s) onto HDC (backbuffer) ---
void GameUI::Render(HDC hdc) {
    if (!HasActivePanel()) return;
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeHighQuality);

    RECT rc; GetClientRect(m_mainWnd, &rc);
    int W = rc.right - rc.left, H = rc.bottom - rc.top;

    // dim background
    SolidBrush dim(Color(160, 0, 0, 0));
    g.FillRectangle(&dim, 0.0f, 0.0f, (REAL)W, (REAL)H);

    // card + shadow
    RectF shadow((REAL)(m_panelX + 8), (REAL)(m_panelY + 10), (REAL)m_panelW, (REAL)m_panelH);
    SolidBrush shb(Color(120, 0, 0, 0));
    DrawRoundedRect(g, shadow, 12.0f, &shb, nullptr);

    RectF card((REAL)m_panelX, (REAL)m_panelY, (REAL)m_panelW - 8.0f, (REAL)m_panelH - 8.0f);
    LinearGradientBrush grad(RectF((REAL)m_panelX, (REAL)m_panelY, card.Width, card.Height),
        Color(255, 36, 42, 54), Color(255, 60, 68, 86), LinearGradientModeVertical);
    Pen border(Color(200, 30, 30, 30), 1.0f);
    DrawRoundedRect(g, card, 12.0f, &grad, &border);

    // title
    FontFamily ff(L"Segoe UI");
    Font titleFont(&ff, 28.0f, FontStyleBold, UnitPixel);
    SolidBrush titleBrush(Color(240, 240, 240, 240));
    PointF titlePos((REAL)m_panelX + 22.0f, (REAL)m_panelY + 14.0f);
    g.DrawString(m_title.c_str(), -1, &titleFont, titlePos, &titleBrush);

    // Buttons
    for (auto& b : m_buttons) {
        // use the exact same rect coords that input code uses (no +6 offset)
        RectF br(
            (REAL)(m_panelX + b.rc.left),
            (REAL)(m_panelY + b.rc.top),
            (REAL)(b.rc.right - b.rc.left),
            (REAL)(b.rc.bottom - b.rc.top)
        );

        Color base = b.pressed ? Color(255, 60, 160, 110)
            : (b.hover ? Color(255, 90, 160, 120) : Color(255, 78, 110, 150));
        LinearGradientBrush bgrad(br, base, Color(255, 40, 60, 90), LinearGradientModeVertical);
        Pen bpen(Color(200, 14, 20, 30), 1.0f);
        DrawRoundedRect(g, br, 8.0f, &bgrad, &bpen);

        FontFamily ff(L"Segoe UI");
        Font btnFont(&ff, 13.0f, FontStyleBold, UnitPixel);
        SolidBrush txt(Color(245, 245, 245, 245));
        StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
        PointF center(br.X + br.Width / 2.0f, br.Y + br.Height / 2.0f - 1.0f);
        g.DrawString(b.text.c_str(), -1, &btnFont, center, &sf, &txt);
    }

    // optional: levels fake list if m_active == Levels (same visual as before)
    if (m_active == Panel::Levels) {
        RectF listRect((REAL)(m_panelX + 20), (REAL)(m_panelY + 64), 260.0f, (REAL)(m_panelH - 100));
        SolidBrush listBg(Color(255, 22, 26, 36));
        Pen listBorder(Color(180, 30, 30, 30), 1.0f);
        DrawRoundedRect(g, listRect, 6.0f, &listBg, &listBorder);
        FontFamily ff2(L"Segoe UI");
        Font itemFont(&ff2, 12.0f, FontStyleRegular, UnitPixel);
        SolidBrush itemText(Color(220, 220, 220, 220));
        for (int i = 0; i < 12; ++i) {
            float iy = listRect.Y + 8.0f + i * 26.0f;
            PointF itPos(listRect.X + 8.0f, iy);
            std::wstring label = L"Level " + std::to_wstring(i + 1);
            g.DrawString(label.c_str(), -1, &itemFont, itPos, &itemText);
        }
    }
}

// --- HUD rendering (you already had RenderHUD; minor reuse) ---
void GameUI::RenderHUD(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    RECT rc; GetClientRect(m_mainWnd, &rc);
    int clientW = rc.right - rc.left;

    int hbW = 200, hbH = 18;
    int hx = clientW - hbW - 16, hy = 12;
    RectF hbBg((REAL)hx - 2.0f, (REAL)hy - 2.0f, (REAL)hbW + 4.0f, (REAL)hbH + 4.0f);
    SolidBrush bgBrush(Color(160, 20, 20, 20));
    Pen outline(Color(255, 30, 30, 30), 1.0f);
    DrawRoundedRect(g, hbBg, 6.0f, &bgBrush, &outline);

    float pct = 0.0f;
    if (player.maxHealth != 0) {
        pct = (float)player.health / (float)player.maxHealth;
    }
    if (pct < 0.0f) pct = 0.0f;
    else if (pct > 1.0f) pct = 1.0f;
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

    // weapon text
    std::wstring wname = (player.weaponType == PISTOL) ? L"Pistol" : L"Shotgun";
    std::wstring hud = L"Weapon: " + wname + L"   Mag: " + std::to_wstring((player.weaponType == PISTOL) ? player.pistolMag : player.shotgunMag)
        + L"   Reserve: " + std::to_wstring((player.weaponType == PISTOL) ? player.pistolReserve : player.shotgunReserve);
    PointF hudPos(12.0f, 12.0f);
    g.DrawString(hud.c_str(), -1, &font, hudPos, &textBrush);

    // pickups simple (GDI)
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
}

// --- input handling (map screen -> panel-local coords)
static bool PointInsideRect(const RECT& r, int x, int y) {
    return (x >= r.left && x < r.right && y >= r.top && y < r.bottom);
}
bool GameUI::OnMouseMove(int x, int y) {
    if (!HasActivePanel()) return false;
    if (x < m_panelX || x >= m_panelX + m_panelW || y < m_panelY || y >= m_panelY + m_panelH) {
        bool changed = false;
        for (auto& b : m_buttons) if (b.hover) { b.hover = false; changed = true; }
        if (changed && m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
        return true;
    }
    int lx = x - m_panelX, ly = y - m_panelY;
    bool changed = false;
    for (auto& b : m_buttons) {
        bool inside = PointInsideRect(b.rc, lx, ly);
        if (inside != b.hover) { b.hover = inside; changed = true; }
    }
    if (changed && m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
    return true;
}
bool GameUI::OnLButtonDown(int x, int y) {
    if (!HasActivePanel()) return false;
    int lx = x - m_panelX, ly = y - m_panelY;
    for (auto& b : m_buttons) {
        if (PointInsideRect(b.rc, lx, ly)) { b.pressed = true; if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE); }
    }
    SetCapture(m_mainWnd);
    return true;
}
bool GameUI::OnLButtonUp(int x, int y) {
    if (!HasActivePanel()) return false;
    int lx = x - m_panelX, ly = y - m_panelY;
    for (auto& b : m_buttons) {
        if (b.pressed) {
            b.pressed = false;
            bool inside = PointInsideRect(b.rc, lx, ly);
            if (inside && m_mainWnd) {
                // post WM_COMMAND so your WndProc handles actions as before
                PostMessage(m_mainWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(b.id, 0), 0);
            }
            if (m_mainWnd) InvalidateRect(m_mainWnd, NULL, FALSE);
        }
    }
    ReleaseCapture();
    return true;
}
// OnResize implementation (single, consistent)
void GameUI::OnResize(int w, int h, float /*scaleParam*/) {
    m_w = w; m_h = h;

    float sx = (float)w / (float)BASE_W;
    float sy = (float)h / (float)BASE_H;
    float s = std::min(sx, sy);
    s = clampf(s, UI_MIN_SCALE, UI_MAX_SCALE);
    m_scale = s;

    // usar las constantes del header
    m_panelW = (int)std::lround(GAMEUI_BASE_PANEL_W * m_scale);
    m_panelH = (int)std::lround(GAMEUI_BASE_PANEL_H * m_scale);

    m_panelX = (m_w - m_panelW) / 2;
    m_panelY = (m_h - m_panelH) / 2;

    if (m_active == Panel::Start) SetupStartPanel();
    else if (m_active == Panel::Levels) SetupLevelsPanel();
    else SetupStartPanel();
}