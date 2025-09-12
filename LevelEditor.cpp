// ---------------- Implementation ----------------

#include "leveleditor.h"
#include <algorithm>
#include "GameUI.h"
#include "UIManager.h"
#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM

static GameUI g_gameUI;
static UIManager g_uiMgr;
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
LevelEditor::LevelEditor() {
    // palette...
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
    m_palette.push_back({ 'P', L"Player start" });

    m_currentTile = '#';

    // sensible defaults
    m_viewOffsetX = 20;
    m_viewOffsetY = 80;
    m_tileSize = 32;
    m_mouseX = m_mouseY = 0;
    m_panning = false;
    m_panLastX = m_panLastY = 0;
    m_hBtnFit = NULL;
}

LevelEditor::~LevelEditor() { Destroy(); }

void LevelEditor::Init(HINSTANCE hInst, HWND hParent) {
    m_hInst = hInst;
    m_hParent = hParent;
    RegisterClassIfNeeded();
}

void LevelEditor::Destroy() {
    DestroyEditorWindow();
    m_hParent = NULL;
    m_hInst = NULL;
}

void LevelEditor::Show() {
    if (!m_hWnd) CreateEditorWindow();
    if (m_hWnd) ShowWindow(m_hWnd, SW_SHOW);
}

void LevelEditor::Hide() {
    if (m_hWnd) ShowWindow(m_hWnd, SW_HIDE);
}

bool LevelEditor::HandleCommand(int id, WPARAM wParam, LPARAM lParam) {
    switch (id) {
    case ID_ED_BTN_NEW:
        NewEmptyLevel(40, 15);
        InvalidateRect(m_hWnd, NULL, TRUE);
        return true;
    case ID_ED_BTN_LOAD: {
        wchar_t buf[32]; GetWindowTextW(m_hLevelIndex, buf, 31);
        int slot = _wtoi(buf);
        if (slot <= 0) { MessageBoxW(m_hWnd, L"Enter a valid level number (1..99)", L"Error", MB_OK); return true; }
        LoadLevelFromSlot(slot);
        InvalidateRect(m_hWnd, NULL, TRUE);
        return true;
    }
    case ID_ED_BTN_SAVE: {
        wchar_t buf[32]; GetWindowTextW(m_hLevelIndex, buf, 31);
        int slot = _wtoi(buf);
        if (slot <= 0) { MessageBoxW(m_hWnd, L"Enter a valid level number (1..99)", L"Error", MB_OK); return true; }
        SaveLevelToSlot(slot);
        return true;
    }
    case ID_ED_BTN_FIT:
        FitToWindow();
        InvalidateRect(m_hWnd, NULL, FALSE);
        return true;
    case ID_ED_TILE_LIST:
        if (HIWORD(wParam) == LBN_SELCHANGE) {
            int sel = (int)SendMessageW(m_hTileList, LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < (int)m_palette.size()) {
                m_currentTile = m_palette[sel].first;
                if (m_hWnd) InvalidateRect(m_hWnd, NULL, FALSE);
            }
            return true;
        }
        return false;
    }
    return false;
}
void LevelEditor::FitToWindow() {
    if (!m_editLevel || !m_hWnd) return;
    RECT rc; GetClientRect(m_hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // leave area for controls on top + palette on right (simple heuristic)
    int usableW = w - 40;
    int usableH = h - 96;
    if (usableW <= 0 || usableH <= 0) return;

    int tilesW = m_editLevel->width;
    int tilesH = m_editLevel->height;
    int tileW = std::max(4, usableW / tilesW);
    int tileH = std::max(4, usableH / tilesH);
    m_tileSize = std::min(tileW, tileH);

    // center grid in client
    int totalPxW = tilesW * m_tileSize;
    int totalPxH = tilesH * m_tileSize;
    m_viewOffsetX = (w - totalPxW) / 2;
    m_viewOffsetY = (h - totalPxH) / 2 + 24;
}

void LevelEditor::OnMouseWheel(int x, int y, short delta) {
    if (!m_editLevel) return;

    float oldTile = (float)m_tileSize;
    // world coordinates (tile indices) under cursor
    float worldX = (float)(x - m_viewOffsetX) / oldTile;
    float worldY = (float)(y - m_viewOffsetY) / oldTile;

    float factor = (delta > 0) ? 1.12f : 0.88f;
    float newSizeF = oldTile * factor;
    int newTile = clampi(round_to_int(newSizeF), 6, 256);
    if (newTile == m_tileSize) return;
    m_tileSize = newTile;

    // keep same world tile under cursor
    m_viewOffsetX = (int)round_to_int((float)x - worldX * (float)m_tileSize);
    m_viewOffsetY = (int)round_to_int((float)y - worldY * (float)m_tileSize);

    InvalidateRect(m_hWnd, NULL, FALSE);
}

void LevelEditor::OnRButtonDown(int x, int y) {
    m_panning = true;
    m_panLastX = x;
    m_panLastY = y;
    SetCapture(m_hWnd);
}

void LevelEditor::OnRButtonUp(int x, int y) {
    m_panning = false;
    ReleaseCapture();
}

void LevelEditor::RegisterClassIfNeeded() {
    static bool reg = false;
    if (reg) return;
    reg = true;

    WNDCLASS wc; ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = EditorWndProc;
    wc.hInstance = m_hInst;
    wc.lpszClassName = L"LevelEditorWndClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClass(&wc);
}

bool LevelEditor::CreateEditorWindow() {
    if (!m_hParent) return false;
    RECT rc; GetClientRect(m_hParent, &rc);
    int w = 680, h = 480;
    int x = (rc.right - w) / 2;
    int y = (rc.bottom - h) / 2;

    m_hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"LevelEditorWndClass", L"Level Editor",
        WS_CHILD | WS_VISIBLE | WS_BORDER, x, y, w, h, m_hParent, NULL, m_hInst, this);
    if (!m_hWnd) return false;

    CreateControls();
    return true;
}

void LevelEditor::DestroyEditorWindow() {
    DestroyControls();
    if (m_hWnd) { DestroyWindow(m_hWnd); m_hWnd = NULL; }
}

void LevelEditor::CreateControls() {
    if (!m_hWnd) return;
    RECT rc; GetClientRect(m_hWnd, &rc);
    int w = rc.right - rc.left;

    m_hBtnNew = CreateWindowW(L"BUTTON", L"New", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 10, 70, 28, m_hWnd, (HMENU)ID_ED_BTN_NEW, m_hInst, NULL);
    m_hBtnLoad = CreateWindowW(L"BUTTON", L"Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        90, 10, 70, 28, m_hWnd, (HMENU)ID_ED_BTN_LOAD, m_hInst, NULL);
    m_hBtnSave = CreateWindowW(L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        170, 10, 70, 28, m_hWnd, (HMENU)ID_ED_BTN_SAVE, m_hInst, NULL);
    m_hBtnFit = CreateWindowW(L"BUTTON", L"Fit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        250, 10, 70, 28, m_hWnd, (HMENU)ID_ED_BTN_FIT, m_hInst, NULL);

    CreateWindowW(L"STATIC", L"Slot:", WS_CHILD | WS_VISIBLE,
        330, 12, 34, 20, m_hWnd, NULL, m_hInst, NULL);
    m_hLevelIndex = CreateWindowW(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        370, 10, 48, 24, m_hWnd, (HMENU)ID_ED_LEVEL_INDEX, m_hInst, NULL);

    m_hTileList = CreateWindowW(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
        w - 300, 10, 280, 200, m_hWnd, (HMENU)ID_ED_TILE_LIST, m_hInst, NULL);

    for (auto& p : m_palette) {
        std::wstring label = p.second + L" (" + std::wstring(1, p.first) + L")";
        SendMessageW(m_hTileList, LB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    int def = 1;
    SendMessage(m_hTileList, LB_SETCURSEL, def, 0);
    m_currentTile = m_palette[def].first;
}

void LevelEditor::DestroyControls() {
    if (m_hBtnNew) { DestroyWindow(m_hBtnNew); m_hBtnNew = NULL; }
    if (m_hBtnLoad) { DestroyWindow(m_hBtnLoad); m_hBtnLoad = NULL; }
    if (m_hBtnSave) { DestroyWindow(m_hBtnSave); m_hBtnSave = NULL; }
    if (m_hTileList) { DestroyWindow(m_hTileList); m_hTileList = NULL; }
    if (m_hLevelIndex) { DestroyWindow(m_hLevelIndex); m_hLevelIndex = NULL; }
}

void LevelEditor::NewEmptyLevel(int w, int h) {
    m_editLevel = std::make_unique<Level>();
    m_editLevel->width = w;
    m_editLevel->height = h;
    m_editLevel->tiles.assign(h, std::string(w, '.'));
    m_editLevel->spawnX = 100.0f;
    m_editLevel->spawnY = (h - 2) * 32.0f;
    m_editLevel->BuildMaterialGrid();
}

void LevelEditor::LoadLevelFromSlot(int slot) {
    if (slot <= 0) return;
    auto L = g_levelManager.LoadLevel(slot);
    if (!L) {
        wchar_t buf[128]; swprintf_s(buf, L"Level %d not found", slot);
        MessageBoxW(m_hWnd, buf, L"Load", MB_OK);
        return;
    }
    m_editLevel = std::move(L);
}

void LevelEditor::SaveLevelToSlot(int slot) {
    if (!m_editLevel) { MessageBoxW(m_hWnd, L"No level to save. Create or load one first.", L"Save", MB_OK); return; }
    char path[260];
    sprintf_s(path, "levels\\level%02d.map", slot);
    bool ok = g_levelManager.SaveLevelToFile(*m_editLevel, path);
    if (ok) {
        wchar_t buf[128]; swprintf_s(buf, L"Saved level %d to %S", slot, path);
        MessageBoxW(m_hWnd, buf, L"Save", MB_OK);
    }
    else {
        MessageBoxW(m_hWnd, L"Failed to save level file.", L"Save", MB_OK | MB_ICONERROR);
    }
}

void LevelEditor::Paint(HDC hdc) {
    RECT rc; GetClientRect(m_hWnd, &rc);
    HBRUSH bg = CreateSolidBrush(RGB(200, 200, 200));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    // header
    SetTextColor(hdc, RGB(10, 10, 10));
    SetBkMode(hdc, TRANSPARENT);
    RECT hdr = { 8, 4, rc.right - 8, 24 };
    DrawTextW(hdc, L"Level Editor - Left click paint, right-drag pan, wheel zoom", -1, &hdr, DT_LEFT | DT_SINGLELINE);

    if (!m_editLevel) return;

    // update mouse pos (client coords)
    POINT pt; if (GetCursorPos(&pt)) { ScreenToClient(m_hWnd, &pt); m_mouseX = pt.x; m_mouseY = pt.y; }

    int sx = m_viewOffsetX, sy = m_viewOffsetY;
    int tilesW = m_editLevel->width, tilesH = m_editLevel->height;
    int gridLeft = sx, gridTop = sy;
    int gridRight = sx + tilesW * m_tileSize;
    int gridBottom = sy + tilesH * m_tileSize;

    // draw tiles (simple loop; uses g_texMgr just like before)
    for (int r = 0; r < tilesH; ++r) {
        for (int c = 0; c < tilesW; ++c) {
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
            // grid lines
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
            HPEN old = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, x, y, NULL); LineTo(hdc, x + m_tileSize, y);
            LineTo(hdc, x + m_tileSize, y + m_tileSize); LineTo(hdc, x, y + m_tileSize);
            LineTo(hdc, x, y);
            SelectObject(hdc, old); DeleteObject(pen);
        }
    }

    // spawn marker
    int spx = (int)m_editLevel->spawnX;
    int spy = (int)m_editLevel->spawnY;
    int tx = sx + (int)(spx / 32.0f) * m_tileSize + m_tileSize / 2;
    int ty = sy + (int)(spy / 32.0f) * m_tileSize + m_tileSize / 2;
    if (tx >= gridLeft && tx < gridRight && ty >= gridTop && ty < gridBottom) {
        HBRUSH b2 = CreateSolidBrush(RGB(255, 60, 60));
        Ellipse(hdc, tx - 6, ty - 6, tx + 6, ty + 6);
        DeleteObject(b2);
    }

    // reticle (crosshair) if inside grid
    if (m_mouseX >= gridLeft && m_mouseX < gridRight && m_mouseY >= gridTop && m_mouseY < gridBottom) {
        HPEN pen = CreatePen(PS_DOT, 1, RGB(220, 220, 220));
        HPEN old = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, m_mouseX, gridTop, NULL); LineTo(hdc, m_mouseX, gridBottom);
        MoveToEx(hdc, gridLeft, m_mouseY, NULL); LineTo(hdc, gridRight, m_mouseY);
        SelectObject(hdc, old); DeleteObject(pen);
    }
}

void LevelEditor::OnMouseDown(int x, int y) {
    if (!m_editLevel) return;
    int sx = m_viewOffsetX, sy = m_viewOffsetY;
    int gx = x - sx, gy = y - sy;
    if (gx < 0 || gy < 0) return;
    int c = gx / m_tileSize; int r = gy / m_tileSize;
    if (r >= 0 && r < m_editLevel->height && c >= 0 && c < m_editLevel->width) {
        if (m_currentTile == 'P') {
            m_editLevel->spawnX = c * (float)32 + 16.0f;
            m_editLevel->spawnY = r * (float)32 + 16.0f;
        }
        else {
            m_editLevel->tiles[r][c] = m_currentTile;
        }
        m_editLevel->BuildMaterialGrid();
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void LevelEditor::OnMouseMove(int x, int y, bool lbutton) {
    if (!lbutton) return;
    OnMouseDown(x, y);
}

LRESULT CALLBACK LevelEditor::EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LevelEditor* p = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        p = (LevelEditor*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)p);
        p->m_hWnd = hwnd;
    }
    else {
        p = (LevelEditor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (p) return p->WndProc(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT LevelEditor::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        Paint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        OnMouseDown(x, y);
        return 0;
    }
    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        bool lbutton = (wParam & MK_LBUTTON) != 0;
        bool rbutton = (wParam & MK_RBUTTON) != 0;
        if (m_panning && rbutton) {
            int dx = x - m_panLastX;
            int dy = y - m_panLastY;
            m_viewOffsetX += dx;
            m_viewOffsetY += dy;
            m_panLastX = x; m_panLastY = y;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else {
            OnMouseMove(x, y, lbutton);
        }
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        HandleCommand(id, wParam, lParam);
        return 0;
    }
    case WM_RBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        OnRButtonDown(x, y);
        return 0;
    }
    case WM_RBUTTONUP: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        OnRButtonUp(x, y);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        OnMouseWheel(x, y, delta);
        return 0;
    }
    case WM_DESTROY: {
        m_hWnd = NULL;
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ----------------- Global single instance -----------------
static LevelEditor* g_levelEditor = nullptr;

// Simple API for the rest of the game to use
void CreateLevelEditor(HINSTANCE hInst, HWND mainWnd) {
    if (!g_levelEditor) g_levelEditor = new LevelEditor();
    g_levelEditor->Init(hInst, mainWnd);
}
void DestroyLevelEditor() {
    if (g_levelEditor) { g_levelEditor->Destroy(); delete g_levelEditor; g_levelEditor = nullptr; }
}
void ShowLevelEditor() { if (g_levelEditor) g_levelEditor->Show(); }
void HideLevelEditor() { if (g_levelEditor) g_levelEditor->Hide(); }

// Optional: call this from your main WM_COMMAND when user clicks "Level Editor" button
bool HandleLevelEditorCommand(int id, WPARAM wParam, LPARAM lParam) {
    if (!g_levelEditor) return false;
    return g_levelEditor->HandleCommand(id, wParam, lParam);
}

// EOF
