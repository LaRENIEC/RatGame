#include "UIManager.h"
#include <gdiplus.h>
#include <algorithm>
#include <cstdio>
#include "MaterialTextures.h"
#include "TextureManager.h"
#include "GameUI.h"
#include <windows.h>
#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
    m_palette.push_back({ 'I', L"Ice" });
// Externs de tu proyecto (asegúrate que existan en otros CPPs)
extern TextureManager g_texMgr;
extern LevelManager g_levelManager;

using namespace Gdiplus;


UIManager::UIManager() {
    // palette default
    m_palette.push_back({ '.', L"Air" });
    m_palette.push_back({ '#', L"Dirt" });
    m_palette.push_back({ 'G', L"Grass" });
    m_palette.push_back({ 'S', L"Sand" });
    m_palette.push_back({ 'V', L"Gravel" });
    m_palette.push_back({ 'W', L"Water" });
    m_palette.push_back({ 'X', L"Spikes" });
    m_palette.push_back({ 'H', L"Shotgun pickup" });
    m_palette.push_back({ 'A', L"Pistol pickup" });
    m_palette.push_back({ 'E', L"Enemy spawn" });
    m_palette.push_back({ 'B', L"Boss spawn" });
    m_palette.push_back({ 'P', L"Player start" }); // <-- nueva entrada
    m_currentTile = '#';
}

UIManager::~UIManager() { Shutdown(); }

void UIManager::Init(HINSTANCE hInst, HWND hMainWnd) {
    m_hInst = hInst;
    m_hMainWnd = hMainWnd;
}

void UIManager::Shutdown() {
    CloseActivePanel();
    m_hInst = NULL;
    m_hMainWnd = NULL;
}

static float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

void UIManager::DrawRoundedRect(Graphics& g, const RectF& r, float radius, Brush* brush, const Pen* pen) {
    GraphicsPath path;
    float x = r.X, y = r.Y, w = r.Width, h = r.Height;
    float d = radius * 2.0f;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
    if (brush) g.FillPath(brush, &path);
    if (pen) g.DrawPath(const_cast<Pen*>(pen), &path);
}

void UIManager::ClearPanelButtons() {
    m_buttons.clear();
    m_title.clear();
}

// ------------------------------------------------------------------
// Setup panels
// ------------------------------------------------------------------
void UIManager::SetupStartPanel() {
    ClearPanelButtons();
    m_title = L"RAT GAME";

    int bw = 300, bh = 44;
    int bx = (m_panelW - bw) / 2;
    int by = 86;

    m_buttons.push_back({ {bx, by, bx + bw, by + bh}, L"Singleplayer",  ID_SINGLEPLAYER_BTN });
    m_buttons.push_back({ {bx, by + 56, bx + bw, by + 56 + bh}, L"Multiplayer", ID_MULTIPLAYER_BTN });
    m_buttons.push_back({ {bx, by + 112, bx + bw, by + 112 + bh}, L"Extras", ID_EXTRAS_BTN });
    m_buttons.push_back({ {bx, by + 168, bx + bw, by + 168 + bh}, L"Level Editor", ID_LEVEL_EDITOR_BTN });
    m_buttons.push_back({ {bx, by + 224, bx + bw, by + 224 + bh}, L"Exit", ID_EXIT_BTN });
}

void UIManager::SetupLevelsPanel() {
    ClearPanelButtons();
    m_title = L"Select Level";

    int bw = 120, bh = 40;
    int bx = m_panelW - bw - 28;
    int by = 96;
    m_buttons.push_back({ {bx, by, bx + bw, by + bh}, L"Play", ID_LEVEL_PLAY_BTN });
    m_buttons.push_back({ {bx, by + 56, bx + bw, by + 56 + bh}, L"Cancel", ID_LEVEL_CANCEL_BTN });
}

void UIManager::SetupLevelEditorPanel() {
    ClearPanelButtons();
    m_title = L"Level Editor";

    // top buttons (Save, Load, New, Close)
    int bw = 92, bh = 36;
    int bx = m_panelW - bw - 20;
    int by = 16;
    // Save / Load / New / Close
    m_buttons.push_back({ {bx, by, bx + bw, by + bh}, L"Save", EDITOR_BTN_SAVE });
    m_buttons.push_back({ {bx - (bw + 8), by, bx - 8, by + bh}, L"Load", EDITOR_BTN_LOAD });
    m_buttons.push_back({ {bx - 2 * (bw + 8), by, bx - (bw + 8), by + bh}, L"New", EDITOR_BTN_NEW });
    m_buttons.push_back({ {m_panelW - 60, by, m_panelW - 20, by + bh}, L"Close", EDITOR_BTN_CLOSE });
}

// ------------------------------------------------------------------
// Open/Close panels (centered)
// ------------------------------------------------------------------
void UIManager::ShowStartPanel() {
    m_active = UIPanel::Start;
    RECT rc; GetClientRect(m_hMainWnd, &rc);
    int gw = rc.right - rc.left, gh = rc.bottom - rc.top;
    m_panelW = 520; m_panelH = 360;
    m_panelX = (gw - m_panelW) / 2;
    m_panelY = (gh - m_panelH) / 2;
    SetupStartPanel();
    InvalidatePanel(false);
}

void UIManager::ShowLevelsPanel() {
    m_active = UIPanel::Levels;
    RECT rc; GetClientRect(m_hMainWnd, &rc);
    int gw = rc.right - rc.left, gh = rc.bottom - rc.top;
    m_panelW = 560; m_panelH = 420;
    m_panelX = (gw - m_panelW) / 2;
    m_panelY = (gh - m_panelH) / 2;
    SetupLevelsPanel();
    InvalidatePanel(false);
}

void UIManager::ShowLevelEditorPanel() {
    if (!m_hMainWnd) return;
    m_active = UIPanel::LevelEditor;

    // Usar todo el client rect para que el editor ocupe toda la pantalla
    RECT rc; GetClientRect(m_hMainWnd, &rc);
    int gw = rc.right - rc.left;
    int gh = rc.bottom - rc.top;

    // asignar full-screen al panel
    m_panelX = 0;
    m_panelY = 0;
    m_panelW = gw;
    m_panelH = gh;

    SetupLevelEditorPanel(); // Los botones se posicionan en función de m_panelW ahora
    // ensure editor has at least an empty level to edit
    if (!m_editLevel) NewEmptyLevel(40, 15);
    InvalidatePanel(false);
}

void UIManager::CloseActivePanel() {
    if (m_active == UIPanel::None) return;
    m_active = UIPanel::None;
    ClearPanelButtons();
    InvalidatePanel(false);
}

bool UIManager::PointInsidePanel(int x, int y) const {
    return (x >= m_panelX && x < m_panelX + m_panelW && y >= m_panelY && y < m_panelY + m_panelH);
}

// ------------------------------------------------------------------
// Render
// ------------------------------------------------------------------
void UIManager::Render(HDC hdc) {
    if (m_active == UIPanel::None) return;

    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    RECT rc; GetClientRect(m_hMainWnd, &rc);
    int W = rc.right - rc.left;
    int H = rc.bottom - rc.top;

    // full-screen dim
    SolidBrush dimBrush(Color(160, 0, 0, 0));
    g.FillRectangle(&dimBrush, 0.0f, 0.0f, (REAL)W, (REAL)H);

    // shadow and card
    RectF shadowR((REAL)(m_panelX + 8), (REAL)(m_panelY + 10), (REAL)m_panelW, (REAL)m_panelH);
    SolidBrush shadowBrush(Color(120, 0, 0, 0));
    DrawRoundedRect(g, shadowR, 12.0f, &shadowBrush, nullptr);

    RectF cardR((REAL)m_panelX, (REAL)m_panelY, (REAL)m_panelW - 8.0f, (REAL)m_panelH - 8.0f);
    LinearGradientBrush cardGrad(RectF((REAL)m_panelX, (REAL)m_panelY, cardR.Width, cardR.Height),
        Color(255, 36, 42, 54), Color(255, 60, 68, 86), LinearGradientModeVertical);
    Pen border(Color(200, 30, 30, 30), 1.0f);
    DrawRoundedRect(g, cardR, 12.0f, &cardGrad, &border);

    // Title
    FontFamily ff(L"Segoe UI");
    Font titleFont(&ff, 22.0f, FontStyleBold, UnitPixel);
    SolidBrush titleBrush(Color(240, 240, 240, 240));
    PointF titlePos((REAL)m_panelX + 18.0f, (REAL)m_panelY + 12.0f);
    g.DrawString(m_title.c_str(), -1, &titleFont, titlePos, &titleBrush);

    // Buttons (common)
    for (auto& b : m_buttons) {
        RectF br((REAL)(m_panelX + b.rc.left + 6), (REAL)(m_panelY + b.rc.top + 6), (REAL)(b.rc.right - b.rc.left), (REAL)(b.rc.bottom - b.rc.top));
        Color base = b.pressed ? Color(255, 60, 160, 110) : (b.hover ? Color(255, 90, 160, 120) : Color(255, 78, 110, 150));
        LinearGradientBrush bgrad(br, base, Color(255, 40, 60, 90), LinearGradientModeVertical);
        Pen bpen(Color(200, 14, 20, 30), 1.0f);
        DrawRoundedRect(g, br, 8.0f, &bgrad, &bpen);

        Font btnFont(&ff, 12.0f, FontStyleBold, UnitPixel);
        SolidBrush txt(Color(245, 245, 245, 245));
        StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
        PointF center(br.X + br.Width / 2.0f, br.Y + br.Height / 2.0f - 1.0f);
        g.DrawString(b.text.c_str(), -1, &btnFont, center, &sf, &txt);
    }

    // Panel-specific content
    if (m_active == UIPanel::Levels) {
        RectF listRect((REAL)(m_panelX + 20), (REAL)(m_panelY + 64), 260.0f, (REAL)(m_panelH - 100));
        SolidBrush listBg(Color(255, 22, 26, 36));
        Pen listBorder(Color(180, 30, 30, 30), 1.0f);
        DrawRoundedRect(g, listRect, 6.0f, &listBg, &listBorder);

        Font itemFont(&ff, 12.0f, FontStyleRegular, UnitPixel);
        SolidBrush itemText(Color(220, 220, 220, 220));
        for (int i = 0; i < 12; ++i) {
            float iy = listRect.Y + 8.0f + i * 26.0f;
            PointF itPos(listRect.X + 8.0f, iy);
            std::wstring label = L"Level " + std::to_wstring(i + 1);
            g.DrawString(label.c_str(), -1, &itemFont, itPos, &itemText);
        }
    }
    else if (m_active == UIPanel::LevelEditor) {
        // Paint level editor UI into the panel
        PaintLevelEditor(g, hdc);
    }
}

// ------------------------------------------------------------------
// Input handling: map screen coordinates to panel-local coords and process
// ------------------------------------------------------------------
bool UIManager::OnMouseMove(int x, int y) {
    if (m_active == UIPanel::None) return false;
    bool changed = false;
    if (!PointInsidePanel(x, y)) {
        for (auto& b : m_buttons) if (b.hover) { b.hover = false; changed = true; }
        if (changed) InvalidateRect(m_hMainWnd, NULL, FALSE);
        return true; // consumed (modal)
    }

    int lx = x - m_panelX, ly = y - m_panelY;
    for (auto& b : m_buttons) {
        bool inside = (lx >= b.rc.left && lx < b.rc.right && ly >= b.rc.top && ly < b.rc.bottom);
        if (inside != b.hover) { b.hover = inside; changed = true; }
    }

    // If editor active and mouse down, do painting drag
    if (m_active == UIPanel::LevelEditor) {
        if (m_editorMouseDown) {
            Editor_HandleDrag(lx, ly);
        }
        if (m_editorPanning) {
            int dx = x - m_panLastX;
            int dy = y - m_panLastY;
            m_viewOffsetX += dx;
            m_viewOffsetY += dy;
            m_panLastX = x; m_panLastY = y;
            InvalidatePanel(false);
        }
    }

    if (changed) InvalidateRect(m_hMainWnd, NULL, FALSE);
    return true;
}

bool UIManager::OnLButtonDown(int x, int y) {
    if (m_active == UIPanel::None) return false;
    int lx = x - m_panelX, ly = y - m_panelY;

    // check buttons first
    for (auto& b : m_buttons) {
        bool inside = (lx >= b.rc.left && lx < b.rc.right && ly >= b.rc.top && ly < b.rc.bottom);
        if (inside) { b.pressed = true; InvalidateRect(m_hMainWnd, NULL, FALSE); }
    }

    // if editor panel, handle editor specific areas (palette or grid)
    if (m_active == UIPanel::LevelEditor) {
        m_editorMouseDown = true;
        bool consumed = Editor_HandleClick(lx, ly);
        SetCapture(m_hMainWnd);
        if (consumed) return true;
    }

    SetCapture(m_hMainWnd);
    return true;
}

bool UIManager::OnLButtonUp(int x, int y) {
    if (m_active == UIPanel::None) return false;
    int lx = x - m_panelX, ly = y - m_panelY;

    for (auto& b : m_buttons) {
        if (b.pressed) {
            b.pressed = false;
            bool inside = (lx >= b.rc.left && lx < b.rc.right && ly >= b.rc.top && ly < b.rc.bottom);
            if (inside && m_hMainWnd) {
                // handle editor buttons inline (no WM_COMMAND)
                if (m_active == UIPanel::LevelEditor) {
                    switch (b.id) {
                    case EDITOR_BTN_SAVE:
                        SaveLevelToSlot(m_levelSlot);
                        break;
                    case EDITOR_BTN_LOAD:
                        LoadLevelFromSlot(m_levelSlot);
                        break;
                    case EDITOR_BTN_NEW:
                        NewEmptyLevel(40, 15);
                        break;
                    case EDITOR_BTN_CLOSE:
                        CloseActivePanel();
                        break;
                    default:
                        // ignore
                        break;
                    }
                }
                else {
                    // For other panels we post WM_COMMAND so main WndProc handles it
                    PostMessage(m_hMainWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(b.id, 0), 0);
                }
            }
            if (m_hMainWnd) InvalidateRect(m_hMainWnd, NULL, FALSE);
        }
    }

    if (m_active == UIPanel::LevelEditor) {
        m_editorMouseDown = false;
    }

    ReleaseCapture();
    return true;
}

// ---------------- LevelEditor helpers ----------------

void UIManager::NewEmptyLevel(int w, int h) {
    m_editLevel = std::make_unique<Level>();
    m_editLevel->width = w;
    m_editLevel->height = h;
    m_editLevel->tiles.assign(h, std::string(w, '.'));
    m_editLevel->spawnX = 100.0f;
    m_editLevel->spawnY = (h - 2) * 32.0f;
    m_editLevel->BuildMaterialGrid();
    InvalidatePanel(false);
}

void UIManager::LoadLevelFromSlot(int slot) {
    if (slot <= 0) return;
    auto L = g_levelManager.LoadLevel(slot);
    if (!L) {
        wchar_t buf[128]; swprintf_s(buf, L"Level %d not found", slot);
        MessageBoxW(m_hMainWnd, buf, L"Load", MB_OK);
        return;
    }
    m_editLevel = std::move(L);
    InvalidatePanel(false);
}

void UIManager::SaveLevelToSlot(int slot) {
    if (!m_editLevel) { MessageBoxW(m_hMainWnd, L"No level to save. Create or load one first.", L"Save", MB_OK); return; }
    char path[260];
    sprintf_s(path, "levels\\level%02d.map", slot);
    bool ok = g_levelManager.SaveLevelToFile(*m_editLevel, path);
    if (ok) {
        wchar_t buf[128]; swprintf_s(buf, L"Saved level %d to %S", slot, path);
        MessageBoxW(m_hMainWnd, buf, L"Save", MB_OK);
    }
    else {
        MessageBoxW(m_hMainWnd, L"Failed to save level file.", L"Save", MB_OK | MB_ICONERROR);
    }
}

// Dibuja paleta (izquierda) y preview del nivel (derecha) con GDI+/GDI
void UIManager::PaintLevelEditor(Gdiplus::Graphics& g, HDC hdc) {
    if (!m_hMainWnd) return;
    if (!hdc) return;
    if (m_panelW <= 0 || m_panelH <= 0) return;
    if (!m_editLevel) return;

    // panel local origin
    float px = (float)m_panelX, py = (float)m_panelY;

    // Dynamic layout: paleta ocupa 26% del ancho (clamped), grid el resto.
    const float paletteDesired = 260.0f;
    float palW = clampf(paletteDesired, 160.0f, (float)m_panelW * 0.4f); // mínimo 160, máximo 40% ancho
    RectF paletteRect(px + 18.0f, py + 64.0f, palW, (REAL)(m_panelH - 100));
    SolidBrush listBg(Color(255, 22, 26, 36));
    Pen listBorder(Color(180, 30, 30, 30), 1.0f);
    DrawRoundedRect(g, paletteRect, 6.0f, &listBg, &listBorder);

    FontFamily ff(L"Segoe UI");
    Font itemFont(&ff, 12.0f, FontStyleRegular, UnitPixel);
    SolidBrush itemText(Color(220, 220, 220, 220));

    // draw each palette entry as small box + label
    const float entryH = 28.0f;
    const float gap = 6.0f;
    for (size_t i = 0; i < m_palette.size(); ++i) {
        float iy = paletteRect.Y + 8.0f + i * (entryH + gap);
        if (iy + entryH > paletteRect.GetBottom()) break; // no dibujar fuera
        RectF box(paletteRect.X + 8.0f, iy, 24.0f, 24.0f);
        Color boxColor = (m_palette[i].first == m_currentTile) ? Color(255, 100, 200, 100) : Color(255, 60, 120, 80);
        SolidBrush boxBrush(boxColor);
        Pen boxPen(Color(200, 30, 30, 30), 1.0f);
        DrawRoundedRect(g, box, 4.0f, &boxBrush, &boxPen);

        PointF labelPos(box.X + box.Width + 8.0f, box.Y + box.Height / 2.0f - 1.0f);
        g.DrawString(m_palette[i].second.c_str(), -1, &itemFont, labelPos, &itemText);
    }

    // slot text
    std::wstring slotText = L"Slot: " + std::to_wstring(m_levelSlot) + L"  (use New/Load/Save buttons)";
    Font smallFont(&ff, 11.0f, FontStyleRegular, UnitPixel);
    g.DrawString(slotText.c_str(), -1, &smallFont, PointF(paletteRect.X, paletteRect.Y - 28.0f), &itemText);

    // grid viewport computed from remaining width
    float gridX = paletteRect.GetRight() + 14.0f;
    float gridY = py + 64.0f;
    float gridW = (float)m_panelW - (gridX - px) - 24.0f;
    float gridH = (float)m_panelH - (gridY - py) - 24.0f;
    if (gridW < 32.0f || gridH < 32.0f) return;

    // background for grid
    SolidBrush gridBg(Color(255, 10, 12, 18));
    Pen gridBr(Color(180, 30, 30, 30), 1.0f);
    DrawRoundedRect(g, RectF(gridX, gridY, gridW, gridH), 6.0f, &gridBg, &gridBr);

    // compute tile drawing origin inside grid (leave margin)
    int sx = (int)gridX + 8;
    int sy = (int)gridY + 8;

    // clip drawing to grid area to avoid OOB
    int maxTilesX = std::max(1, (int)(gridW / m_tileSize));
    int maxTilesY = std::max(1, (int)(gridH / m_tileSize));
    int drawW = std::min(m_editLevel->width, maxTilesX);
    int drawH = std::min(m_editLevel->height, maxTilesY);

    for (int r = 0; r < drawH; ++r) {
        for (int c = 0; c < drawW; ++c) {
            int x = sx + c * m_tileSize;
            int y = sy + r * m_tileSize;
            char ch = m_editLevel->TileCharAt(r, c);
            Material mat = Level::CharToMaterial(ch);
            Gdiplus::Bitmap* bmp = GetMaterialTexture(mat);
            if (bmp) {
                g_texMgr.Draw(bmp, hdc, x, y, m_tileSize, m_tileSize);
            }
            else {
                MaterialInfo mi = GetMaterialInfo(mat);
                int rr = (mi.color >> 16) & 0xFF;
                int gg = (mi.color >> 8) & 0xFF;
                int bb = (mi.color) & 0xFF;
                HBRUSH b = CreateSolidBrush(RGB(rr, gg, bb));
                RECT rct = { x, y, x + m_tileSize, y + m_tileSize };
                FillRect(hdc, &rct, b);
                DeleteObject(b);
            }

            // grid lines (GDI)
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
            HPEN old = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, x, y, NULL); LineTo(hdc, x + m_tileSize, y);
            LineTo(hdc, x + m_tileSize, y + m_tileSize); LineTo(hdc, x, y + m_tileSize);
            LineTo(hdc, x, y);
            SelectObject(hdc, old); DeleteObject(pen);
        }
    }

    // spawn marker visual: clamp in-case spawn outside drawn area
    int spx = (int)m_editLevel->spawnX;
    int spy = (int)m_editLevel->spawnY;
    int tx = sx + (int)(spx / 32.0f) * m_tileSize + m_tileSize / 2;
    int ty = sy + (int)(spy / 32.0f) * m_tileSize + m_tileSize / 2;
    // check drawn bounds
    if (tx >= sx && tx < sx + drawW * m_tileSize && ty >= sy && ty < sy + drawH * m_tileSize) {
        HBRUSH b2 = CreateSolidBrush(RGB(255, 60, 60));
        Ellipse(hdc, tx - 6, ty - 6, tx + 6, ty + 6);
        DeleteObject(b2);
    }
}

// Convierte coords locales del panel a acciones: palette click o paint en grid.
// retorna true si consumió el click
bool UIManager::Editor_HandleClick(int lx, int ly) {
    // compute dynamic palette and grid areas (same logic as in PaintLevelEditor)
    int palX = 18;
    float paletteDesired = 260.0f;
    float palW = clampf(paletteDesired, 160.0f, (float)m_panelW * 0.4f);
    int palY = 64;
    int palH = m_panelH - 100;

    if (lx >= palX && lx < (int)(palX + palW) && ly >= palY && ly < palY + palH) {
        float entryH = 28.0f; float gap = 6.0f;
        int idx = (int)((ly - palY - 8.0f) / (entryH + gap));
        if (idx >= 0 && idx < (int)m_palette.size()) {
            m_currentTile = m_palette[idx].first;
            InvalidatePanel(false);
        }
        return true; // clicked palette region
    }

    // grid area
    int gridX = (int)(palX + palW + 14.0f);
    int gridY = 64;
    int sx = gridX + 8, sy = gridY + 8;
    if (!m_editLevel) return false;
    int gridW = m_editLevel->width * m_tileSize;
    int gridH = m_editLevel->height * m_tileSize;

    // allow clicks only inside the visible area
    if (lx >= sx && lx < sx + gridW && ly >= sy && ly < sy + gridH) {
        int c = (lx - sx) / m_tileSize;
        int r = (ly - sy) / m_tileSize;
        if (r >= 0 && r < m_editLevel->height && c >= 0 && c < m_editLevel->width) {
            if (m_currentTile == 'P') {
                // colocar spawn en el centro del tile
                m_editLevel->spawnX = c * (float)m_tileSize + (float)m_tileSize * 0.5f;
                m_editLevel->spawnY = r * (float)m_tileSize + (float)m_tileSize * 0.5f;
            }
            else {
                m_editLevel->tiles[r][c] = m_currentTile;
            }
            m_editLevel->BuildMaterialGrid();
            InvalidateRect(m_hMainWnd, NULL, FALSE);
            return true;
        }
    }
    return false;
}

void UIManager::Editor_HandleDrag(int lx, int ly) {
    // painting while dragging - same logic as click
    Editor_HandleClick(lx, ly);
}
void UIManager::InvalidatePanel(bool eraseBackground) {
    if (!m_hMainWnd) return;
    RECT r = { m_panelX, m_panelY, m_panelX + m_panelW, m_panelY + m_panelH };
    InvalidateRect(m_hMainWnd, &r, eraseBackground ? TRUE : FALSE);
}
void UIManager::FitEditorToWindow() {
    if (!m_editLevel) return;
    // calcular area de grid tal y como PaintLevelEditor lo hace
    float px = (float)m_panelX, py = (float)m_panelY;
    const float paletteDesired = 260.0f;
    float palW = clampf(paletteDesired, 160.0f, (float)m_panelW * 0.4f);

    // compute grid box same as PaintLevelEditor:
    int gridLeft = (int)(px + palW + 14.0f) + 8;
    int gridTop = (int)(py + 64.0f) + 8;
    int gridW = m_panelW - (gridLeft - m_panelX) - 32;
    int gridH = m_panelH - (gridTop - m_panelY) - 24;
    if (gridW <= 0 || gridH <= 0) return;
    int tileW = std::max(4, gridW / m_editLevel->width);
    int tileH = std::max(4, gridH / m_editLevel->height);
    m_tileSize = std::min(tileW, tileH);
    int totalPxW = m_editLevel->width * m_tileSize;
    int totalPxH = m_editLevel->height * m_tileSize;
    m_viewOffsetX = gridLeft + (gridW - totalPxW) / 2;
    m_viewOffsetY = gridTop + (gridH - totalPxH) / 2;
    InvalidatePanel(false);
}


// OnEditorMouseWheel (reemplaza la anterior)
void UIManager::OnEditorMouseWheel(int x, int y, short delta) {
    if (m_active != UIPanel::LevelEditor || !m_editLevel) return;
    // convert screen->panel local coords
    int lx = x - m_panelX;
    int ly = y - m_panelY;

    float oldTile = (float)m_tileSize;
    float worldX = (float)(lx - m_viewOffsetX) / oldTile;
    float worldY = (float)(ly - m_viewOffsetY) / oldTile;

    float factor = (delta > 0) ? 1.12f : 0.88f;
    float newSizeF = oldTile * factor;
    int newTile = clampi(round_to_int(newSizeF), 4, 256);
    if (newTile == m_tileSize) return;
    m_tileSize = newTile;

    // keep same tile under cursor
    m_viewOffsetX = (int)round_to_int((float)lx - worldX * m_tileSize);
    m_viewOffsetY = (int)round_to_int((float)ly - worldY * m_tileSize);

    InvalidatePanel(false);
}

// R button pan handlers
void UIManager::OnEditorRButtonDown(int x, int y) {
    if (m_active != UIPanel::LevelEditor) return;
    m_editorPanning = true;
    m_panLastX = x; m_panLastY = y;
}
void UIManager::OnEditorRButtonUp(int x, int y) {
    m_editorPanning = false;
}
