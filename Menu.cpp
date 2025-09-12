#include "Menu.h"
#include <string>
#include "GameUI.h"
// Definición única del objeto global (una sola en todo el proyecto)
UIManager g_uiManager;
Menu::Menu() {
    hParent = NULL;
    hBtnStart = hBtnOptions = hBtnExit = hBtnLevelEditor = NULL;
    hTitleFont = hBtnFont = NULL;
    visible = true;
}

Menu::~Menu() { Destroy(); }

void Menu::Create(HINSTANCE hInstance, HWND parent) {
    hParent = parent;

    // Creamos fuentes únicamente para el render en backbuffer (si quieres seguir dibujando título)
    hTitleFont = CreateFontW(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hBtnFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // NO creamos controles Windows: la interactiva del menú la maneja ahora GameUI / UIManager.
}

void Menu::Destroy() {
    if (hTitleFont) { DeleteObject(hTitleFont); hTitleFont = NULL; }
    if (hBtnFont) { DeleteObject(hBtnFont); hBtnFont = NULL; }
    hParent = NULL;
}

bool Menu::HandleCommand(int id) {
    switch (id) {
    case ID_START_BTN:
        // abrir el panel de Start dentro de la UI in-game
        g_uiManager.ShowStartPanel();
        return true;
    case ID_OPTIONS_BTN:
        // togglear dibujado de pickups (sigue usando mensaje al main si tu juego lo espera)
        PostMessage(hParent, MSG_TOGGLE_PICKUPS, 0, 0);
        return true;
    case ID_EXIT_BTN:
        PostMessage(hParent, WM_CLOSE, 0, 0);
        return true;
    case ID_LEVEL_EDITOR_BTN:
        // abrir el editor: si migras a overlay, aquí llamarías a g_uiManager.ShowLevelEditorPanel()
        // si mantienes el LevelEditor como child window, usa ShowLevelEditor()
        ShowLevelEditor();
        return true;
    }
    return false;
}

void Menu::Show(bool show) {
    visible = show;
    if (hBtnStart) ShowWindow(hBtnStart, show ? SW_SHOW : SW_HIDE);
    if (hBtnOptions) ShowWindow(hBtnOptions, show ? SW_SHOW : SW_HIDE);
    if (hBtnExit) ShowWindow(hBtnExit, show ? SW_SHOW : SW_HIDE);
}

void Menu::Render(HDC hdc) {
    if (!visible) return;
    HFONT old = (HFONT)SelectObject(hdc, hTitleFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(230, 230, 230));
    RECT r = { 20, 10, 500, 80 };
    DrawTextW(hdc, L"RAT GAME", -1, &r, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    SelectObject(hdc, old);
}
