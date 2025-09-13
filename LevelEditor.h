// LevelEditor.h + LevelEditor.cpp (single-file for convenience)
// Drop this file into your project and add LevelEditor.h to includes where needed.
// It implements a simple Win32 level editor that integrates with your LevelManager
// and TextureManager. UI is basic but functional: palette (listbox), grid viewport,
// New / Load / Save (slot index) and mouse painting. Uses your existing Level struct
// and LevelManager::SaveLevelToFile / LoadLevel.

#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <cstdio>
#include "GameUI.h"
#include "Level.h"
#include "TextureManager.h"
#include "MaterialTextures.h"

// Externals defined in your RatGame.cpp
extern TextureManager g_texMgr;
extern LevelManager g_levelManager;

// IDs for controls inside the editor window
enum {
    ID_ED_BTN_NEW = 4000,
    ID_ED_BTN_LOAD,
    ID_ED_BTN_SAVE,
    ID_ED_BTN_FIT,
    ID_ED_TILE_LIST,
    ID_ED_LEVEL_INDEX, // edit control for numeric slot
};

class LevelEditor : public GameUI {
public:
    LevelEditor();
    ~LevelEditor();

    // Initialize editor (pass HINSTANCE and main window handle so the editor can be child)
    void Init(HINSTANCE hInst, HWND hParent);
    void Destroy();

    // Show / hide
    void Show();
    void Hide();

    // Message handler for commands coming from main WndProc (e.g. menu)
    bool HandleCommand(int id, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE m_hInst = NULL;
    HWND m_hParent = NULL;
    HWND m_hWnd = NULL; // editor main child window
    HWND m_hBtnNew = NULL;
    HWND m_hBtnLoad = NULL;
    HWND m_hBtnSave = NULL;
    HWND m_hBtnFit = NULL;
    HWND m_hTileList = NULL;
    HWND m_hLevelIndex = NULL;

    std::unique_ptr<Level> m_editLevel; // currently edited level

    // palette data: pair<char, label>
    std::vector<std::pair<char, std::wstring>> m_palette;
    char m_currentTile = '#';

    // helpers
    void RegisterClassIfNeeded();
    bool CreateEditorWindow();
    void DestroyEditorWindow();

    void CreateControls();
    void DestroyControls();

    void NewEmptyLevel(int w, int h);
    void LoadLevelFromSlot(int slot);
    void SaveLevelToSlot(int slot);
    // add inside LevelEditor class (private section)
    int m_viewOffsetX = 0;
    int m_viewOffsetY = 0;
    int m_tileSize = 32;         // tamaño actual del tile en px (zoom)
    int m_mouseX = 0, m_mouseY = 0; // mouse local (panel) pos for reticle
    bool m_panning = false;
    int m_panLastX = 0, m_panLastY = 0;
    // espacio reservado para la UI a la derecha (LISTBOX etc.)
    static const int RIGHT_PANEL_W = 360;
    // altura ocupada por los controles superiores (botones, header)
    static const int TOP_BAR_H = 56;
    // margen interior para centrar/ajustar
    static const int SIDE_MARGIN = 12;
    // cursor de teclado dentro de la rejilla (fila/columna)
    int m_cursorRow = 0;
    int m_cursorCol = 0;

    // helpers para redimensionar controles cuando la ventana cambia de tamaño
    void ResizeControls();
    // mantener el cursor dentro del viewport (ajusta m_viewOffsetX/Y si hace falta)
    void EnsureCursorVisible();

    // handler de teclado (flechas / enter / space)
    void OnKeyDownHandler(WPARAM wParam, LPARAM lParam);

    void FitToWindow(); // ajusta m_tileSize + offsets para ver todo el mapa
    void OnMouseWheel(int x, int y, short delta);
    void OnRButtonDown(int x, int y);
    void OnRButtonUp(int x, int y);

    void Paint(HDC hdc);
    void OnMouseDown(int x, int y);
    void OnMouseMove(int x, int y, bool lbutton);

    static LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
extern void CreateLevelEditor(HINSTANCE hInst, HWND mainWnd);
extern void ShowLevelEditor();