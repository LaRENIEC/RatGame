#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "Player.h"

#define MSG_START_GAME        (WM_USER + 1)
#define MSG_TOGGLE_PICKUPS    (WM_USER + 2)
#define MSG_SHOW_START_MENU   (WM_USER + 3)

#define ID_START_BTN          2001
#define ID_OPTIONS_BTN        2002
#define ID_EXIT_BTN           2003  

#define MSG_SHOW_LEVELS_PANEL (WM_USER + 4)
#define ID_LEVELS_LIST        4001
#define ID_LEVEL_PLAY_BTN     4002
#define ID_LEVEL_CANCEL_BTN   4003
// IDs para el Start panel
#define ID_SINGLEPLAYER_BTN   3001
#define ID_MULTIPLAYER_BTN    3002
#define ID_EXTRAS_BTN         3003

#define ID_START_BTN   2001
#define ID_OPTIONS_BTN 2002
#define ID_EXIT_BTN    2003
#define MSG_START_GAME (WM_USER + 1)
// IDs para botones nuevos (elige valores libres en tu proyecto)
#define ID_LEVEL_EDITOR_BTN   4005

// mensaje personalizado para mostrar el editor (elige un offset de WM_USER)
#define MSG_SHOW_LEVEL_EDITOR (WM_USER + 200)
struct WeaponPickup;

class GameUI {
public:
    GameUI();
    ~GameUI();

    void Init(HINSTANCE hInstance, HWND mainWnd);
    void Shutdown();

    // panels show/hide
    void ShowStartPanel();
    void HideStartPanel();
    void ShowLevelsPanel();
    void HideLevelsPanel();
    bool HasActivePanel() const;

    // render UI (draw onto backbuffer HDC)
    void Render(HDC hdc);
    void OnResize(int w, int h, float scale); // <-- sólo declarar aquí

    // HUD rendering (call from RenderFrame for HUD elements)
    void RenderHUD(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups);

    // input (returns true if consumed)
    bool OnMouseMove(int x, int y);
    bool OnLButtonDown(int x, int y);
    bool OnLButtonUp(int x, int y);

private:
    HINSTANCE m_hInst = NULL;
    HWND m_mainWnd = NULL;

    int m_w = 0, m_h = 0;
    float m_scale = 1.0f;

    enum class Panel { None = 0, Start, Levels, LevelEditor };
    Panel m_active = Panel::None;

    // panel geometry
    int m_panelW = 520, m_panelH = 360;
    int m_panelX = 0, m_panelY = 0;

    struct Button {
        RECT rc; std::wstring text; int id;
        bool hover = false, pressed = false;
    };
    std::vector<Button> m_buttons;
    std::wstring m_title;

    // helpers
    void SetupStartPanel();
    void SetupLevelsPanel();
    void ClearPanel();
};
