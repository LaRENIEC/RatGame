#pragma once
#include <windows.h>
#include "UIManager.h"

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

extern UIManager g_uiManager;
extern void ShowLevelEditor(); // si mantienes el editor con ventana o wrapper
class Menu {
public:
    Menu();
    ~Menu();

    void Create(HINSTANCE hInstance, HWND parent);
    void Destroy();
    bool HandleCommand(int id);
    void Show(bool show);
    void Render(HDC hdc);

private:
    HWND hParent;
    HWND hBtnStart, hBtnOptions, hBtnLevelEditor, hBtnExit;
    HFONT hTitleFont, hBtnFont;
    bool visible;
};
