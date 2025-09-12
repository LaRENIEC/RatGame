#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <gdiplus.h>
#include <memory>
#include <cmath>       // <-- necesario para std::floor / std::round / std::lround

#include "Level.h" // para manipular niveles (TileCharAt, BuildMaterialGrid, etc.)

// pequeños helpers portables:
static inline int clampi(int v, int lo, int hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }
static inline int round_to_int(float f) { return (int)std::floor(f + 0.5f); }

enum class UIPanel {
    None = 0,
    Start,
    Levels,
    LevelEditor
};

struct UIPanelButton {
    RECT rc;                 // rect local al panel (x,y dentro del panel)
    std::wstring text;
    int id;                  // ID to post as WM_COMMAND when clicked (or used internally)
    bool hover = false;
    bool pressed = false;
};

class UIManager {
public:
    UIManager();
    ~UIManager();

    void Init(HINSTANCE hInst, HWND hMainWnd);   // llamar desde wWinMain
    void Shutdown();

    // Mostrar paneles (abren modal)
    void ShowStartPanel();
    void ShowLevelsPanel();
    void ShowLevelEditorPanel();
    void CloseActivePanel(); // cierra panel activo

    // Renderizar el panel activo (si lo hay) sobre el HDC que le pases
    void Render(HDC hdc);

    // Consultas
    bool HasActivePanel() const { return m_active != UIPanel::None; }
    UIPanel GetActivePanel() const { return m_active; }

    // Manejo de input: devolver true si consumió el mensaje (cuando hay panel activo)
    bool OnMouseMove(int x, int y);
    bool OnLButtonDown(int x, int y);
    bool OnLButtonUp(int x, int y);

    // editor-specific API
    void OnEditorMouseWheel(int x, int y, short delta);
    void OnEditorRButtonDown(int x, int y);
    void OnEditorRButtonUp(int x, int y);
    void FitEditorToWindow();

private:
    HINSTANCE m_hInst = NULL;
    HWND m_hMainWnd = NULL;

    UIPanel m_active = UIPanel::None;

    // panel frame parameters (todos los paneles centrados)
    int m_panelW = 680, m_panelH = 480; // agrandado para editar niveles
    int m_panelX = 0, m_panelY = 0; // calculados por Show*

    // per-panel content (buttons + title)
    std::vector<UIPanelButton> m_buttons;
    std::wstring m_title;

    // helpers
    void SetupStartPanel();
    void SetupLevelsPanel();
    void SetupLevelEditorPanel();
    void ClearPanelButtons();
    void InvalidatePanel(bool eraseBackground = false);

    // utilities drawing
    void DrawRoundedRect(Gdiplus::Graphics& g, const Gdiplus::RectF& r, float radius,
        Gdiplus::Brush* brush, const Gdiplus::Pen* pen);
    bool PointInsidePanel(int x, int y) const;

    // ----------------- LevelEditor embedded state -----------------
    std::unique_ptr<Level> m_editLevel;
    int m_tileSize = 32;               // visual scale for editor grid
    int m_viewOffsetX = 16;            // margin left inside panel
    int m_viewOffsetY = 64;            // margin top inside panel
    std::vector<std::pair<char, std::wstring>> m_palette;
    char m_currentTile = '#';
    int m_levelSlot = 1;               // slot number used for load/save
    bool m_editorMouseDown = false;    // dragging paint

    bool m_editorPanning = false;
    int m_panLastX = 0, m_panLastY = 0;
    int m_mouseX = 0, m_mouseY = 0;

    // IDs para botones integrados del editor
    static constexpr int EDITOR_BTN_SAVE = 5001;
    static constexpr int EDITOR_BTN_LOAD = 5002;
    static constexpr int EDITOR_BTN_NEW = 5003;
    static constexpr int EDITOR_BTN_CLOSE = 5004;

    // internal helper methods for editor
    void NewEmptyLevel(int w, int h);
    void LoadLevelFromSlot(int slot);
    void SaveLevelToSlot(int slot);
    void PaintLevelEditor(Gdiplus::Graphics& g, HDC hdc); // dibuja paleta + grid + botones
    bool Editor_HandleClick(int lx, int ly); // coords locales del panel (x,y dentro del panel) -> true si consumido
    void Editor_HandleDrag(int lx, int ly);
};
