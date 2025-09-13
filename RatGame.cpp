// RatGame.cpp - versión parcheada: reemplaza lógica de controles nativos
// por el nuevo GameUI dibujado (elimina ventanas "botón" blancas y child controls nativos).

#define NOMINMAX
#include <windows.h>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include <string>
#include <memory>
#include <cstdio>

#include "Player.h"
#include "Input.h"
#include "GameMovement.h"
#include "Menu.h"         // ahora usado solo conceptualmente; no crea child controls
#include "Pistol.h"
#include "Shotgun.h"
#include "Level.h"
#include "PlayerSprite.h"
#include "TextureManager.h"

#include "Entity.h"
#include "Enemy.h"
#include "Boss.h"
#include "MaterialTextures.h"
#include "GameUI.h"
#include "leveleditor.h" // si lo usas
#include "UIManager.h"
#include <windowsx.h>
#include "hud.h"
#include "health_hud.h"
#include "weapon_hud.h"
#include "pickups_hud.h"
#include "pause_hud.h"
#include "Camera.h"
// al principio de RatGame.cpp (junto a otros globals)
#include <gdiplus.h> // añade si no está

using namespace Gdiplus;

// NUEVO: puntero a la textura del sky cargada para el nivel actual
static Gdiplus::Bitmap* g_skyBmp = nullptr;
static GameUI g_gameUI;
static CompositeHUD g_hud;
static UIManager g_uiMgr;
// Tamaño ventana por defecto
static const int WINDOW_W = 900;
static const int WINDOW_H = 600;

HINSTANCE g_hInst = NULL;
HWND g_hWnd = NULL;

// Juego - objetos principales
Player g_player;
static Camera g_camera;

std::vector<Bullet> g_bullets;
InputState g_input;
Menu g_menu; // lo mantenemos como objeto ligero (no crea child windows ahora)
// Globals extra para UI/estado
bool g_inGame = false;        // false = menu visible / juego en pausa
bool g_showPickups = true;    // toggled by Options
TextureManager g_texMgr;
PlayerSprite g_playerSprite;
bool g_playerSpriteLoaded = false;

static bool g_gamePaused = false;     // si true -> no actualizar la física/IA
static bool g_isCrouching = false;    // estado simple de crouch (usado en colisión/visual)
// double buffer
HBITMAP g_backBmp = NULL;
HDC g_backDC = NULL;
int g_clientW = WINDOW_W, g_clientH = WINDOW_H;
// escala de UI basada en la resolución actual respecto a WINDOW_W/H
static float g_uiScale = 1.0f;

// tamaño actual del backbuffer (para recrearlo sólo cuando cambie)
static int g_backW = 0;
static int g_backH = 0;
// Entities (enemies, bosses, etc.)
std::vector<std::unique_ptr<Entity>> g_entities;

// Cambié el nombre del alias para evitar conflicto con clock_t de C
using highres_clock = std::chrono::high_resolution_clock;
auto g_prevTime = highres_clock::now();

bool g_running = true;

LevelManager g_levelManager; // incluye Level.h en RatGame.cpp

// NOTE: GameMovement.h declara extern std::vector<WeaponPickup> g_pickups;
extern std::vector<WeaponPickup> g_pickups;

// forward decl para nivel cargado (declared in Level.cpp / Level.h)
extern std::unique_ptr<Level> g_currentLevel;

// Declaraciones
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int RunGameLoop();
void RenderFrame(HDC hdc, const Vec2& camOffset);
void UpdateBullets(float dt, float groundY);

// RatGame.cpp (poner junto a otros helpers/globals)
static void LoadSkyForCurrentLevel() {
    g_skyBmp = nullptr;
    if (!g_currentLevel) return;

    std::string skyName = g_currentLevel->sky;
    if (skyName.empty()) {
        std::wstring def = L"assets/sky/sky_default.png";
        g_skyBmp = g_texMgr.Load(def);
        return;
    }

    std::wstring ws;
    ws.reserve(16 + skyName.size());
    ws = L"assets/sky/";
    ws.append(skyName.begin(), skyName.end()); // asume ASCII/simple names

    g_skyBmp = g_texMgr.Load(ws);

    if (!g_skyBmp) {
        std::wstring def = L"assets/sky/sky_default.png";
        g_skyBmp = g_texMgr.Load(def);
    }
}

// --- helpers para entitites spawned from level ----------
static void SpawnEntitiesFromCurrentLevel() {
    g_entities.clear();
    if (!g_currentLevel) return;

    const float TILE = TILE_SIZE;
    std::vector<LevelObject> objs;
    g_currentLevel->EnumerateObjects(objs);

    for (auto& obj : objs) {
        float px = obj.WorldX(TILE);
        float py = obj.WorldY(TILE);

        if (obj.tag == 'E') {
            auto e = std::make_unique<Enemy>();
            e->pos = Vec2(px, py);
            e->vel = Vec2(0, 0);
            e->patrolLeftX = px - 80.0f;
            e->patrolRightX = px + 80.0f;
            g_entities.push_back(std::move(e));
        }
        else if (obj.tag == 'B') {
            auto b = std::make_unique<Boss>();
            b->pos = Vec2(px, py - 10.0f);
            b->vel = Vec2(0, 0);
            g_entities.push_back(std::move(b));
        }
        else if (obj.tag == 'A' || obj.tag == 'H') {
            // pickups los añadimos ya en ApplyLevel, pero podrías spawnarlos aquí si lo prefieres
        }
    }
}

// ---------------------------------------------------
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInst = hInstance;

    // Registrar clase simple
    WNDCLASS wc; ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"RatGameClass";
    RegisterClass(&wc);

    // Crear ventana principal
    int left = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_W) / 2;
    int top = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_H) / 2;
    // Crear ventana principal (Ajustamos el rect para que el CLIENT tenga WINDOW_W x WINDOW_H)
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN; // permitir redimensionar, maximizebox, etc.
    RECT wr = { 0, 0, WINDOW_W, WINDOW_H };
    AdjustWindowRect(&wr, style, FALSE);
    int winW = wr.right - wr.left;
    int winH = wr.bottom - wr.top;

    g_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"RatGame - Modular 2D",
        style,
        left, top, winW, winH,
        NULL, NULL, hInstance, NULL);
    if (!g_hWnd) return -1;

    // Mostrar y actualizar ventana
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // Asegurarnos de conocer el tamaño real del client y notificar UI
    RECT clientRc; GetClientRect(g_hWnd, &clientRc);
    g_clientW = clientRc.right - clientRc.left;
    g_clientH = clientRc.bottom - clientRc.top;
    g_gameUI.OnResize(g_clientW, g_clientH, 1.0f);
    g_hud.OnResize(g_clientW, g_clientH, 1.0f);
    // Si el editor está abierto, forzamos que ajuste su viewport
    g_uiMgr.FitEditorToWindow();

    if (!g_hWnd) return -1;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Inicializar GDI+
    ULONG_PTR gdiPlusToken;
    Gdiplus::GdiplusStartupInput gdiPSI;
    Gdiplus::GdiplusStartup(&gdiPlusToken, &gdiPSI, nullptr);

    // Inicializar recursos GDI+ y texturas
    g_texMgr.Init();
    g_playerSpriteLoaded = g_playerSprite.Init(&g_texMgr, L"assets/player/");
    if (g_playerSpriteLoaded) {
        g_playerSprite.SetTargetBodyHeight(60.0f);
        g_playerSprite.SetScale(g_playerSprite.GetScale() * 0.95f); // ajuste fino si quieres
    }

    // inicializar texturas/materials
    InitMaterialTextures(g_texMgr);

    // mostrar mensaje si faltan texturas (antes de iniciar el juego)
    g_texMgr.ReportMissingTextures(g_hWnd);
    // Inicializar GameUI (ya no queremos crear child BUTTONs nativos)
    g_gameUI.Init(hInstance, g_hWnd);
    g_gameUI.ShowStartPanel();

    // Inicializar UIManager (paneles adicionales, p.e. LevelEditor embebido)
    g_uiMgr.Init(hInstance, g_hWnd);

    g_hud.Init(g_hWnd);
    g_hud.AddChild(std::make_unique<HealthHUD>());
    g_hud.AddChild(std::make_unique<WeaponHUD>());
    g_hud.AddChild(std::make_unique<PickupsHUD>(g_pickups, true));
    g_hud.AddChild(std::make_unique<PauseHUD>());
    g_hud.OnResize(g_clientW, g_clientH, 1.0f);

    g_hud.Init(g_hWnd);
    g_gameUI.OnResize(g_clientW, g_clientH, 1.0f); // si usas escalado variable
    g_hud.OnResize(g_clientW, g_clientH, 1.0f);

    // No crear los controles nativos: el objeto Menu se mantiene sólo para render textual
    // si quieres, puedes seguir usando g_menu.Render(buf) para dibujar título, pero
    // no creamos child windows.

    // Inicializar player (o esperar al ApplyLevel)
// colocar al jugador en el spawn del nivel (si existe) o fallback al centro/ventana
    if (g_currentLevel) {
        // asegúrate de que materialGrid/tiles están construidos antes de colocar al jugador
        g_currentLevel->PlacePlayerAtSpawn(g_player);
    }
    else {
        // si no hay nivel, colocamos al jugador en el centro de la ventana
        g_player.pos = Vec2((float)g_clientW * 0.5f, (float)g_clientH * 0.5f);
    }
    g_player.vel = Vec2(0, 0); // reset velocidad

    // Lanzar loop principal (bloqueante)
    int res = RunGameLoop();

    // cleanup GDI
    if (g_backDC) { DeleteDC(g_backDC); g_backDC = NULL; }
    if (g_backBmp) { DeleteObject(g_backBmp); g_backBmp = NULL; }
    g_gameUI.Shutdown();
    Gdiplus::GdiplusShutdown(gdiPlusToken);
    return res;
}

// ---------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
    {
        RECT rc; GetClientRect(hWnd, &rc);
        g_clientW = rc.right - rc.left;
        g_clientH = rc.bottom - rc.top;
    }
    break;
    case WM_SIZE:
    {
        g_clientW = LOWORD(lParam);
        g_clientH = HIWORD(lParam);

        // recalcular escalado/layouts
        float scale = 1.0f; // o calcula uno si lo quieres dinámico
        g_gameUI.OnResize(g_clientW, g_clientH, scale);
        g_hud.OnResize(g_clientW, g_clientH, scale);

        // Si el panel del editor está activo, que recalcule su grid/tileSize y reubique
        if (g_uiMgr.HasActivePanel()) {
            g_uiMgr.FitEditorToWindow();
            g_uiMgr.InvalidatePanel(false);
        }

        // forzar repintado
        InvalidateRect(hWnd, NULL, FALSE);
    }
    break;
    case WM_DISPLAYCHANGE:
    {
        // redimensionar/centrar la ventana al 90% del nuevo display (evita que quede pequeña)
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        int newW = std::max(640, (int)std::round(screenW * 0.90)); // mínimo 640
        int newH = std::max(480, (int)std::round(screenH * 0.90)); // mínimo 480
        int left = (screenW - newW) / 2;
        int top = (screenH - newH) / 2;

        SetWindowPos(hWnd, NULL, left, top, newW, newH, SWP_NOZORDER | SWP_SHOWWINDOW);

        // actualizar client realmente disponible
        RECT rc; GetClientRect(hWnd, &rc);
        g_clientW = rc.right - rc.left;
        g_clientH = rc.bottom - rc.top;

        // notificar UIs para que se reajusten
        g_gameUI.OnResize(g_clientW, g_clientH, 1.0f);
        g_hud.OnResize(g_clientW, g_clientH, 1.0f);
        g_uiMgr.FitEditorToWindow();
        g_uiMgr.InvalidatePanel(false);

        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }


    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        // Los botones ahora provienen de GameUI (que postea WM_COMMAND con el id del botón)
        switch (id) {
        case ID_SINGLEPLAYER_BTN:
            // mostrar panel Levels con el GameUI
            g_gameUI.ShowLevelsPanel();
            break;
        case ID_LEVEL_PLAY_BTN:
        {
            int levelIndex = 1; // o el índice que quieras pasar desde la UI
            auto lvl = g_levelManager.LoadLevel(levelIndex);
            if (lvl) {
                g_levelManager.ApplyLevel(*lvl);
                SpawnEntitiesFromCurrentLevel();
                // cargar sky que el nivel pide (assets/sky/<nombre>)
                LoadSkyForCurrentLevel();
                // ocultar paneles del GameUI y empezar juego
                g_gameUI.HideLevelsPanel();
                g_inGame = true;
            }
            else {
                MessageBoxW(hWnd, L"Level not found", L"Info", MB_OK);
            }
        }
        break;
        case MSG_TOGGLE_PAUSE:
        {
            g_gamePaused = !g_gamePaused;
            g_hud.SetPaused(g_gamePaused);
            return 0;
        }
        case MSG_SHOW_START_MENU:
        {
            // volver al menú principal: ocultar paneles de juego, desactivar inGame y despausar
            g_gamePaused = false;
            g_hud.SetPaused(false);
            g_inGame = false;
            g_gameUI.ShowStartPanel();
            return 0;
        }

        case ID_LEVEL_CANCEL_BTN:
            g_gameUI.HideLevelsPanel();
            break;
        case ID_EXIT_BTN:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        case ID_MULTIPLAYER_BTN:
            MessageBoxW(hWnd, L"Multiplayer no implementado aún", L"Info", MB_OK);
            break;
        case ID_EXTRAS_BTN:
            MessageBoxW(hWnd, L"Extras no implementado aún", L"Info", MB_OK);
            break;
        case ID_LEVEL_EDITOR_BTN:
            // Abrir el editor **como panel modal** dibujado por UIManager (NO crear ventana nativa)
            g_gameUI.HideStartPanel();
            g_uiMgr.ShowLevelEditorPanel();
            break;

        default:
            // Otros ids...
            break;
        }
    }
    break;

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        // HUD puede consumir el click (ej. pause)
        if (g_hud.OnLButtonDown(x, y)) break;
        if (g_gameUI.HasActivePanel()) { g_gameUI.OnLButtonDown(x, y); break; }
        if (g_uiMgr.HasActivePanel()) { g_uiMgr.OnLButtonDown(x, y); break; }
        break;
    }
    case WM_LBUTTONUP: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        if (g_hud.OnLButtonUp(x, y)) break;
        if (g_gameUI.HasActivePanel()) { g_gameUI.OnLButtonUp(x, y); break; }
        if (g_uiMgr.HasActivePanel()) { g_uiMgr.OnLButtonUp(x, y); break; }
        break;
    }
    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        if (g_hud.OnMouseMove(x, y)) break;
        if (g_gameUI.HasActivePanel()) { g_gameUI.OnMouseMove(x, y); break; }
        if (g_uiMgr.HasActivePanel()) { g_uiMgr.OnMouseMove(x, y); break; }
        break;
    }

    case WM_ERASEBKGND:
        // evitamos el borrado del fondo por defecto para prevenir flicker
        return 1;
        // --- nuevos mensajes desde UI/Game ---
    case MSG_START_GAME:
    {
        // ocultar paneles y poner juego en marcha
        g_gameUI.HideStartPanel();
        g_gameUI.HideLevelsPanel();
        g_inGame = true;

        // reset básico del estado del jugador y mundo (iniciar la partida "limpia")
        g_player.Reset();

        // TILE/worldGroundY en scope local del case
        const float TILE = TILE_SIZE;
        float worldGroundY = (g_currentLevel) ? (g_currentLevel->height * TILE) : (float)g_clientH;
        // colocar al jugador en el spawn del nivel (usa Level::PlacePlayerAtSpawn para evitar inconsistencias)
        if (g_currentLevel) {
            g_currentLevel->PlacePlayerAtSpawn(g_player);
        }
        else {
            g_player.pos = Vec2((float)g_clientW * 0.5f, (float)g_clientH * 0.5f);
        }
        g_player.vel = Vec2(0, 0);

        // Snap camera to player so it doesn't lerp from 0,0
        g_camera.SnapTo(g_player.pos, g_clientW, g_clientH);

        g_bullets.clear();

        // repoblar pickups
        g_pickups.clear();
        g_pickups.push_back(WeaponPickup{ SHOTGUN, Vec2((float)g_clientW * 0.75f, worldGroundY - 30.0f), 6 });

        // clear entities y añadir enemigos de prueba cerca del jugador
        g_entities.clear();
        {
            auto e1 = std::make_unique<Enemy>();
            e1->pos = Vec2(g_player.pos.x + 220.0f, g_player.pos.y - 8.0f);
            e1->vel = Vec2(0, 0);
            e1->patrolLeftX = e1->pos.x - 60.0f;
            e1->patrolRightX = e1->pos.x + 60.0f;
            g_entities.push_back(std::move(e1));

            auto e2 = std::make_unique<Enemy>();
            e2->pos = Vec2(g_player.pos.x - 220.0f, g_player.pos.y - 8.0f);
            e2->vel = Vec2(0, 0);
            e2->patrolLeftX = e2->pos.x - 60.0f;
            e2->patrolRightX = e2->pos.x + 60.0f;
            g_entities.push_back(std::move(e2));
        }

        return 0;
    }

    case MSG_TOGGLE_PICKUPS:
        g_showPickups = !g_showPickups;
        {
            const wchar_t* msg = g_showPickups ? L"Pickups: ON" : L"Pickups: OFF";
            MessageBoxW(hWnd, msg, L"Options", MB_OK);
        }
        return 0;

    case MSG_SHOW_LEVELS_PANEL:
        g_gameUI.ShowLevelsPanel();
        return 0;

    case MSG_SHOW_LEVEL_EDITOR:
        g_gameUI.HideStartPanel();
        g_uiMgr.ShowLevelEditorPanel();
        return 0;

    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ---------------------------------------------------
void UpdateBullets(float dt, float groundY) {
    const float TILE = TILE_SIZE;
    for (int i = (int)g_bullets.size() - 1; i >= 0; --i) {
        Bullet& b = g_bullets[i];
        b.life_ms -= (int)(dt * 1000.0f);
        if (b.life_ms <= 0) { g_bullets.erase(g_bullets.begin() + i); continue; }

        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;

        // Si hay un nivel, comprobar colisión con tiles sólidos
        if (g_currentLevel) {
            if (g_currentLevel->IsSolidAtWorld(b.pos.x, b.pos.y, TILE)) {
                // impacto contra tile: borrar bala (podrías spawnear partículas/sonido aquí)
                g_bullets.erase(g_bullets.begin() + i);
                continue;
            }
        }

        // Colisión con el jugador (si la bala NO viene del propio jugador)
        if (!b.fromPlayer && g_player.IsAlive()) {
            float dx = b.pos.x - g_player.pos.x;
            float dy = b.pos.y - g_player.pos.y;
            float dist2 = dx * dx + dy * dy;
            const float HIT_RADIUS = 18.0f; // radio aproximado del jugador
            if (dist2 <= HIT_RADIUS * HIT_RADIUS) {
                int dmg = b.damage;
                if (dmg <= 0) dmg = 8;
                g_player.ApplyDamage(dmg, b.pos);
                g_bullets.erase(g_bullets.begin() + i);
                continue;
            }
        }

        // Colisión con entidades (si la bala viene del jugador)
        if (b.fromPlayer) {
            for (auto& entPtr : g_entities) {
                Entity* e = entPtr.get();
                if (!e || !e->IsAlive()) continue;
                float dx = b.pos.x - e->pos.x;
                float dy = b.pos.y - e->pos.y;
                float dist2 = dx * dx + dy * dy;
                float hr = e->radius + 3.0f;
                if (dist2 <= hr * hr) {
                    int dmg = b.damage;
                    if (dmg <= 0) dmg = 10;
                    e->ApplyDamage(dmg, b.pos);
                    g_bullets.erase(g_bullets.begin() + i);
                    goto NEXT_BULLET;
                }
            }
        }

    NEXT_BULLET:
        // quitar si cae muy abajo (fuera del mundo). groundY ahora suele venir del nivel o fallback.
        if (i < (int)g_bullets.size()) {
            if (g_bullets[i].pos.y > groundY + 400.0f) {
                g_bullets.erase(g_bullets.begin() + i);
                continue;
            }
        }
    }
}

// ---------------------------------------------------
// Cambia la firma: agregar const Vec2& camOffset
void RenderFrame(HDC hdc, const Vec2& camOffset) {
    // Crear/actualizar backbuffer (igual que antes)
// RenderFrame - backbuffer creation / resize fix
    HDC hdcScreen = GetDC(g_hWnd);
    if (!g_backDC || g_backW != g_clientW || g_backH != g_clientH) {
        if (g_backDC) { DeleteDC(g_backDC); g_backDC = NULL; }
        if (g_backBmp) { DeleteObject(g_backBmp); g_backBmp = NULL; }
        g_backDC = CreateCompatibleDC(hdcScreen);
        g_backBmp = CreateCompatibleBitmap(hdcScreen, g_clientW, g_clientH);
        SelectObject(g_backDC, g_backBmp);
        // guardar tamaño actual
        g_backW = g_clientW;
        g_backH = g_clientH;
    }
    ReleaseDC(g_hWnd, hdcScreen);
    HDC buf = g_backDC;

    // Background (si hay sky, usar TextureManager para estirar el bmp; si no, fallback color sólido)
    if (g_skyBmp) {
        g_texMgr.Draw(g_skyBmp, buf, 0, 0, g_clientW, g_clientH);
    }
    else {
        HBRUSH bg = CreateSolidBrush(RGB(20, 28, 40));
        RECT rc = { 0,0,g_clientW,g_clientH };
        FillRect(buf, &rc, bg);
        DeleteObject(bg);
    }

    // Si hay un panel modal creado por GameUI, no dibujamos el mundo
    // Si hay un panel modal creado por GameUI o UIManager, dibujamos el panel (modal) y no el mundo
    if (g_gameUI.HasActivePanel() || g_uiMgr.HasActivePanel()) {
        if (g_gameUI.HasActivePanel()) {
            g_gameUI.Render(buf);
            g_hud.Render(buf, g_player, g_pickups, g_showPickups, g_gamePaused);

        }
        else {
            // usar UIManager para render del panel embebido
            g_uiMgr.Render(buf);
            // UIManager no tiene RenderHUD en tu implementación anterior;
            // dibujamos HUD con la función existente del GameUI (HUD es común).
            g_gameUI.RenderHUD(buf, g_player, g_pickups, g_showPickups);
        }
        BitBlt(hdc, 0, 0, g_clientW, g_clientH, buf, 0, 0, SRCCOPY);
        return;
    }

    // Convertir camOffset a floats
    float camX = camOffset.x;
    float camY = camOffset.y;

    if (g_inGame) {
        // TILE/worldGroundY coherentes con RunGameLoop
        const float TILE = TILE_SIZE;
        float worldGroundY = (g_currentLevel) ? (g_currentLevel->height * TILE) : (float)g_clientH;

        int winGroundY = (int)(worldGroundY - camY);

        // Ground (fondo verde)
    //   HBRUSH ground = CreateSolidBrush(RGB(50, 100, 50));
    //   RECT grect = { 0, winGroundY + 10, g_clientW, g_clientH };
    //   FillRect(buf, &grect, ground);
    //   DeleteObject(ground);

        // Tile rendering usando texturas (si hay nivel)
        if (g_currentLevel) {
            for (int r = 0; r < g_currentLevel->height; ++r) {
                for (int c = 0; c < g_currentLevel->width; ++c) {
                    char ch = g_currentLevel->TileCharAt(r, c);
                    if (ch == '.') continue;
                    Material m = Level::CharToMaterial(ch);

                    // obtener bitmap para el material
                    Gdiplus::Bitmap* bmp = GetMaterialTexture(m);
                    int sx = (int)(c * TILE - camX);
                    int sy = (int)(r * TILE - camY);

                    if (bmp) {
                        g_texMgr.Draw(bmp, buf, sx, sy, (int)TILE, (int)TILE);
                    }
                    else {
                        MaterialInfo mi = GetMaterialInfo(m);
                        uint32_t col = mi.color;
                        int rr = (col >> 16) & 0xFF;
                        int gg = (col >> 8) & 0xFF;
                        int bb = (col) & 0xFF;
                        HBRUSH b = CreateSolidBrush(RGB(rr, gg, bb));
                        RECT tr = { sx, sy, sx + (int)TILE, sy + (int)TILE };
                        FillRect(buf, &tr, b);
                        DeleteObject(b);
                    }
                }
            }
        }

        // Draw bullets (offset por la cámara)
        HBRUSH bcol = CreateSolidBrush(RGB(255, 220, 60));
        HGDIOBJ oldBrush = SelectObject(buf, bcol);
        for (auto& b : g_bullets) {
            int bx = (int)(b.pos.x - camX);
            int by = (int)(b.pos.y - camY);
            Ellipse(buf, bx - 3, by - 3, bx + 3, by + 3);
        }
        SelectObject(buf, oldBrush);
        DeleteObject(bcol);

        // Draw pickups (offset)
        if (g_showPickups) {
            HBRUSH pickupBrush = CreateSolidBrush(RGB(180, 80, 20));
            HGDIOBJ oldBrush2 = SelectObject(buf, pickupBrush);
            for (auto& pk : g_pickups) {
                RECT pr = {
                    (int)(pk.pos.x - 10 - camX),
                    (int)(pk.pos.y - 10 - camY),
                    (int)(pk.pos.x + 10 - camX),
                    (int)(pk.pos.y + 10 - camY)
                };
                FillRect(buf, &pr, pickupBrush);
            }
            SelectObject(buf, oldBrush2);
            DeleteObject(pickupBrush);
        }

        // ------------------- Dibujado de entidades / player (coordenadas mundo usando GDI) -------------------
        // Guardamos el origen actual del viewport GDI
        POINT prevOrg = { 0, 0 };
        GetViewportOrgEx(buf, &prevOrg);

        // Movemos el origen para que las llamadas GDI que usan coords mundo se transformen a coords de pantalla.
        SetViewportOrgEx(buf, -(int)camX, -(int)camY, NULL);

        // Dibujar entidades (usando su Draw(HDC) original que espera coordenadas mundo)
        for (auto& entPtr : g_entities) {
            if (entPtr) entPtr->Draw(buf); // firma Draw(HDC) permanece igual
        }

        // Dibujar fallback player (GDI) en coordenadas mundo; esto funciona porque el viewport está desplazado
        if (!g_playerSpriteLoaded) {
            g_player.Draw(buf, nullptr); // Draw usa pos en coordenadas mundo tal cual
        }

        // Restaurar viewport GDI original antes de dibujar cosas que usan GDI+ o dibujadas en pantalla
        SetViewportOrgEx(buf, prevOrg.x, prevOrg.y, NULL);

        // Dibujar sprite del jugador (GDI+) — el sprite debe recibir el camOffset y restarlo internamente
        if (g_playerSpriteLoaded) {
            // Nota: PlayerSprite::Draw debe tener la firma:
            //   void Draw(HDC hdc, const Player& player, const Vec2& camOffset);
            g_playerSprite.Draw(buf, g_player, Vec2(camX, camY));
        }
        // HUD: health bar and weapon info (top-left/right) - HUD se dibuja en pantalla (no restar cam)
        int healthBarW = (int)(160.0f * g_uiScale);
        int healthBarH = (int)(12.0f * g_uiScale);
        int hx = g_clientW - healthBarW - (int)(10.0f * g_uiScale);
        int hy = (int)(10.0f * g_uiScale);
        RECT hbBack = { hx, hy, hx + healthBarW, hy + healthBarH };
        HBRUSH backBrush = CreateSolidBrush(RGB(40, 40, 40));
        FillRect(buf, &hbBack, backBrush);
        DeleteObject(backBrush);

        float hpPct = (float)g_player.health / (float)g_player.maxHealth;
        if (hpPct < 0.0f) hpPct = 0.0f;
        else if (hpPct > 1.0f) hpPct = 1.0f;
        int fillW = (int)(healthBarW * hpPct);
        RECT hbFill = { hx, hy, hx + fillW, hy + healthBarH };
        HBRUSH fillBrush = CreateSolidBrush(RGB(200, 40, 40));
        FillRect(buf, &hbFill, fillBrush);
        DeleteObject(fillBrush);

        // texto de % / HP
        wchar_t hpText[64];
        swprintf_s(hpText, L"HP: %d/%d", g_player.health, g_player.maxHealth);
        SetTextColor(buf, RGB(240, 240, 240));
        SetBkMode(buf, TRANSPARENT);
        RECT trhp = { hx - 200, hy, hx + 20, hy + 20 };
        DrawTextW(buf, hpText, -1, &trhp, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

        // Weapon HUD top-left
        const wchar_t* wname = (g_player.weaponType == PISTOL) ? L"Pistol" : L"Shotgun";
        int mag = (g_player.weaponType == PISTOL) ? g_player.pistolMag : g_player.shotgunMag;
        int res = (g_player.weaponType == PISTOL) ? g_player.pistolReserve : g_player.shotgunReserve;
        wchar_t hud[128];
        swprintf_s(hud, L"Weapon: %s   Mag: %d   Reserve: %d", wname, mag, res);
        RECT rh = { (int)(10.0f * g_uiScale), (int)(10.0f * g_uiScale), g_clientW, (int)(40.0f * g_uiScale) };
        SetTextColor(buf, RGB(230, 230, 230));
        SetBkMode(buf, TRANSPARENT);
        DrawTextW(buf, hud, -1, &rh, DT_LEFT | DT_SINGLELINE);

        // barra de recarga, si aplica (también HUD, no offset)
        if (g_player.isReloading) {
            int total = (g_player.reloadWeapon == PISTOL) ? Pistol().GetReloadMs() : Shotgun().GetReloadMs();
            int left = g_player.reload_ms_remaining;
            float pct = 1.0f - (float)left / (float)total;
            int barW = 120;
            int barH = 8;
            int bx = 10, by = 40;
            RECT barBack = { bx, by, bx + barW, by + barH };
            HBRUSH back = CreateSolidBrush(RGB(40, 40, 40));
            FillRect(buf, &barBack, back);
            DeleteObject(back);
            HBRUSH fill = CreateSolidBrush(RGB(80, 200, 80));
            RECT barFill = { bx, by, bx + (int)(barW * pct), by + barH };
            FillRect(buf, &barFill, fill);
            DeleteObject(fill);
        }

        // On-death ragdoll handling (player)
        if (!g_player.IsAlive() && g_player.ragdoll) {
            if (g_player.onGround && std::fabs(g_player.vel.y) < 1.0f && std::fabs(g_player.vel.x) < 1.0f) {
                g_inGame = false;
                MessageBoxW(g_hWnd, L"You died!", L"Game Over", MB_OK);
                g_player.Reset();
                g_bullets.clear();
                g_entities.clear();
            }
        }
    }
    else {
        // Menu drawing (no camera offset)
        SetTextColor(buf, RGB(240, 240, 240));
        SetBkMode(buf, TRANSPARENT);
        HFONT hTitle = CreateFontW(
            (int)(60.0f * g_uiScale), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT oldF = (HFONT)SelectObject(buf, hTitle);
        RECT tr = { 0, 40, g_clientW, 140 };
        DrawTextW(buf, L"RAT GAME", -1, &tr, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        SelectObject(buf, oldF);
        DeleteObject(hTitle);

        RECT sub = { 0, 120, g_clientW, 180 };
        DrawTextW(buf, L"Press Start to begin", -1, &sub, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    // Blit final
    BitBlt(hdc, 0, 0, g_clientW, g_clientH, buf, 0, 0, SRCCOPY);
}


// ---------------------------------------------------
int RunGameLoop() {
    const float TILE = TILE_SIZE;
    const float NO_GROUND_Y = 1e9f; // sentinel: no hay suelo si no hay level
    const float GRAVITY = 900.0f; // px/s^2 (ajusta si quieres)
    g_prevTime = highres_clock::now();

    while (g_running) {
        // pump messages
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto now = highres_clock::now();
        std::chrono::duration<float> elapsed = now - g_prevTime;
        float dt = elapsed.count();
        if (dt < 1e-6f) dt = 1e-6f;
        if (dt > 0.1f) dt = 0.1f;

        // Update input siempre
        g_input.Update(g_hWnd);

        // manejo de pause por tecla ESC (edge detection)
        static bool prevPauseKey = false;
        if (g_input.pause && !prevPauseKey) {
            g_gamePaused = !g_gamePaused;
            g_hud.SetPaused(g_gamePaused);
        }
        prevPauseKey = g_input.pause;

        // actualizar estado de crouch simple (global)
        g_isCrouching = g_input.crouch;
        if (g_playerSpriteLoaded) {
            // ajustar la altura objetivo del sprite para visual feedback
            if (g_isCrouching) g_playerSprite.SetTargetBodyHeight(40.0f);
            else g_playerSprite.SetTargetBodyHeight(60.0f);
        }

        if (g_inGame && !g_gamePaused) {
            // Si hay nivel, worldGroundY = altura del nivel; si no, usamos NO_GROUND_Y.
            float worldGroundY = (g_currentLevel) ? (g_currentLevel->height * TILE) : NO_GROUND_Y;

            // Para culling de balas (cuando no hay level, usamos borde de ventana)
            float bulletCullY = (g_currentLevel) ? worldGroundY : (float)g_clientH;

            // store previous pos/vel for collision resolution
            Vec2 prevPos = g_player.pos;
            Vec2 prevVel = g_player.vel;

            // Player movement may spawn bullets into playerNewBullets
            std::vector<Bullet> playerNewBullets;

            // Llamamos UpdateMovement: si no hay level, worldGroundY = NO_GROUND_Y -> no suelo.
            // (asumimos que UpdateMovement aplica controles horizontales y gravedad usando el groundY comparado)
            UpdateMovement(g_player, g_input, dt, g_clientW, worldGroundY, playerNewBullets);

            // registrar balas del jugador
            for (auto& b : playerNewBullets) {
                b.fromPlayer = true;
                if (b.damage == 0) {
                    if (g_player.weaponType == PISTOL) b.damage = 10;
                    else if (g_player.weaponType == SHOTGUN) b.damage = 6;
                    else b.damage = 8;
                }
                g_bullets.push_back(b);
            }

            // Update entities (pueden generar balas propias)
            std::vector<Bullet> entNewBullets;
            // Simple entity-vs-entity separation (run after entity updates)
            for (size_t i = 0; i < g_entities.size(); ++i) {
                for (size_t j = i + 1; j < g_entities.size(); ++j) {
                    auto& A = g_entities[i];
                    auto& B = g_entities[j];
                    if (!A || !B) continue;
                    if (!A->IsAlive() || !B->IsAlive()) continue;
                    float dx = B->pos.x - A->pos.x;
                    float dy = B->pos.y - A->pos.y;
                    float dist2 = dx * dx + dy * dy;
                    float minDist = A->radius + B->radius;
                    if (dist2 < minDist * minDist - 1e-6f) {
                        float dist = std::sqrt(dist2);
                        if (dist < 1e-4f) {
                            // evitar división por cero: empujar arbitrariamente
                            dx = 1.0f; dy = 0.0f; dist = 1.0f;
                        }
                        float overlap = minDist - dist;
                        float nx = dx / dist, ny = dy / dist;
                        // separar proporcionalmente (si quieres masa usa ratio)
                        float invMassA = 1.0f, invMassB = 1.0f; // puedes adaptarlo a peso real
                        float total = invMassA + invMassB;
                        A->pos.x -= nx * (overlap * (invMassA / total));
                        A->pos.y -= ny * (overlap * (invMassA / total));
                        B->pos.x += nx * (overlap * (invMassB / total));
                        B->pos.y += ny * (overlap * (invMassB / total));
                        // ajustar velocidades ligeramente para no re-penetrar
                        A->vel.x *= 0.8f; A->vel.y *= 0.8f;
                        B->vel.x *= 0.8f; B->vel.y *= 0.8f;
                    }
                }
            }

            for (auto& b : entNewBullets) {
                b.fromPlayer = false;
                if (b.damage == 0) b.damage = 8;
                g_bullets.push_back(b);
            }

            // ---- Tile collisions (AABB) solo si hay level (conserva tu implementación previa)
            float pr = g_isCrouching ? 8.0f : 12.0f; // hit radius reducido al agacharse
            if (g_currentLevel) {
                // dt (segundos) lo calculas arriba; aquí lo pasamos
                g_player.ResolveTileCollisions(dt);
            }
            else {
                g_player.onGround = false;
            }

            float left = g_player.pos.x - pr;
            float right = g_player.pos.x + pr;
            float top = g_player.pos.y - pr;
            float bottom = g_player.pos.y + pr;

            int minC = std::max(0, (int)std::floor(left / TILE));
            int maxC = std::min(g_currentLevel->width - 1, (int)std::floor(right / TILE));
            int minR = std::max(0, (int)std::floor(top / TILE));
            int maxR = std::min(g_currentLevel->height - 1, (int)std::floor(bottom / TILE));

            for (int r = minR; r <= maxR; ++r) {
                for (int c = minC; c <= maxC; ++c) {
                    Material mat = g_currentLevel->TileMaterial(r, c);
                    MaterialInfo mi = GetMaterialInfo(mat);
                    if (!mi.solid) continue;

                    float tileL = c * TILE;
                    float tileR = (c + 1) * TILE;
                    float tileT = r * TILE;
                    float tileB = (r + 1) * TILE;

                    float prevLeft = prevPos.x - pr;
                    float prevRight = prevPos.x + pr;
                    float prevTop = prevPos.y - pr;
                    float prevBottom = prevPos.y + pr;

                    left = g_player.pos.x - pr;
                    right = g_player.pos.x + pr;
                    top = g_player.pos.y - pr;
                    bottom = g_player.pos.y + pr;

                    // vertical collision (desde arriba)
                    if (prevBottom <= tileT && bottom >= tileT) {
                        g_player.pos.y = tileT - pr - 0.01f;
                        g_player.vel.y = 0.0f;
                        g_player.onGround = true;
                        top = g_player.pos.y - pr; bottom = g_player.pos.y + pr;
                    }
                    // techo
                    else if (prevTop >= tileB && top <= tileB) {
                        g_player.pos.y = tileB + pr + 0.01f;
                        g_player.vel.y = 0.0f;
                        top = g_player.pos.y - pr; bottom = g_player.pos.y + pr;
                    }

                    // colisión horizontal derecha
                    if (prevRight <= tileL && right >= tileL) {
                        if (!(bottom <= tileT || top >= tileB)) {
                            g_player.pos.x = tileL - pr - 0.01f;
                            g_player.vel.x = 0.0f;
                            left = g_player.pos.x - pr; right = g_player.pos.x + pr;
                        }
                    }
                    // izquierda
                    else if (prevLeft >= tileR && left <= tileR) {
                        if (!(bottom <= tileT || top >= tileB)) {
                            g_player.pos.x = tileR + pr + 0.01f;
                            g_player.vel.x = 0.0f;
                            left = g_player.pos.x - pr; right = g_player.pos.x + pr;
                        }
                    }
                }
            }


            // ---- Fallback: si NO hay nivel, UpdateMovement se ejecutó con NO_GROUND_Y,
            //     pero queremos asegurarnos de que el jugador *caiga* y que muera si cae demasiado.
            // Nota: si UpdateMovement ya aplica gravedad, aquí solo hacemos la comprobación de muerte.
            float deathThreshold;
            if (g_currentLevel) deathThreshold = g_currentLevel->height * TILE + 200.0f;
            else deathThreshold = (float)g_clientH + 200.0f;

            if (g_player.pos.y > deathThreshold) {
                // morir por caída instantáneamente
                if (g_player.IsAlive()) {
                    // aplica daño suficiente para matar
                    g_player.ApplyDamage(g_player.health, Vec2(0, 0));
                    // opcional: mostrar mensaje y reset en el render loop cuando ragdoll termina
                }
            }

            // actualizar balas (para culling usamos bulletCullY)
            UpdateBullets(dt, bulletCullY);

            // actualizar anim de sprite
            if (g_playerSpriteLoaded) { g_playerSprite.Update(dt, g_player); }
        } // end if g_inGame

        // Render (siempre)
        g_camera.SnapTo(g_player.pos, g_clientW, g_clientH);
        Vec2 camOffset = g_camera.GetOffset(g_clientW, g_clientH);

        // Muerte por caída estilo Geometry Dash / Mario:
        // Si el jugador baja más allá del bottom visible + margin -> muerte instantánea.
        const float fallKillMargin = 160.0f; // px; ajusta a tu gusto
        float worldBottomVisible = camOffset.y + (float)g_clientH;

        // puedes usar el 'radius' del jugador si lo tienes; aquí uso ~12 como aproximado
        const float playerRadiusApprox = 12.0f;
        if (g_player.pos.y - playerRadiusApprox > worldBottomVisible + fallKillMargin) {
            if (g_player.IsAlive()) {
                g_player.ApplyDamage(g_player.health, Vec2(0, 0)); // mata al jugador inmediatamente
            }
        }

        HDC hdc = GetDC(g_hWnd);
        RenderFrame(hdc, camOffset);
        ReleaseDC(g_hWnd, hdc);

        // ------------------------------------------------
        // FRAME SCHEDULER (target FPS + short spin)
        // ------------------------------------------------
        // Ajusta este valor para usar más/menos CPU:
        // 60 -> menos CPU (más ahorro), 120 -> más fluido, mayor uso CPU.
        const double target_fps = 120.0; // <- ajusta aquí
        static const auto target_frame_duration = std::chrono::duration<double>(1.0 / target_fps);

        // Time point al final del frame actual
        auto frameEnd = highres_clock::now();
        auto frameElapsed = frameEnd - now; // cuánto tardó este frame hasta aquí

        if (frameElapsed < target_frame_duration) {
            auto remaining = target_frame_duration - frameElapsed;

            // Dormimos la mayor parte del tiempo restante (si es suficientemente grande)
            // dejando ~1ms para el spin final que corrige la imprecisión de Sleep.
            const auto sleep_margin = std::chrono::milliseconds(1);

            if (remaining > sleep_margin) {
                auto to_sleep = remaining - sleep_margin;
                // convertir a ms para sleep_for
                std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(to_sleep));
            }

            // Spin-wait corto para completar el tiempo restante (usa CPU pero es breve)
            while ((highres_clock::now() - now) < target_frame_duration) {
                // NO hacer trabajo pesado aquí; solo esperar activamente unos micro/milis.
                // Esto aumenta el uso de CPU pero reduce jitter de frametimes.
                // Voluntariamente vacío.
            }
        }
        else {
            // Frame tardó más que el target; no dormir.
            // Podrías computar un frame-drop o acumular estadística aquí.
        }

        // Preparar para el siguiente tick
        g_prevTime = now;

    } // end while
    return 0;
}
