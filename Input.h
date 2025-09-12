#pragma once
#include <windows.h>

// Representa el estado de entrada que usa el juego
struct InputState {
    bool left = false, right = false, up = false, down = false;
    bool sprint = false;
    bool jump = false;
    bool lmb = false;
    bool reload = false;     // <-- nuevo: recargar con 'R'
    bool crouch = false;
    bool pause = false;

    POINT mousePos{ 0,0 };
    bool key1 = false, key2 = false;

    // Actualiza el estado leyendo teclas y mouse; requiere HWND para coords de cliente
    void Update(HWND hwnd) {
        left = (GetAsyncKeyState('A') & 0x8000) != 0 || (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
        right = (GetAsyncKeyState('D') & 0x8000) != 0 || (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
        up = (GetAsyncKeyState('W') & 0x8000) != 0 || (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
        down = (GetAsyncKeyState('S') & 0x8000) != 0 || (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;

        // crouch: usar tecla crouch explícita (C) o down (S)
        crouch = ((GetAsyncKeyState('C') & 0x8000) != 0) || down;

        sprint = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        jump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        reload = (GetAsyncKeyState('R') & 0x8000) != 0;

        // pause key (edge handled by game loop)
        pause = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

        key1 = (GetAsyncKeyState('1') & 0x1) != 0;
        key2 = (GetAsyncKeyState('2') & 0x1) != 0;

        POINT p; GetCursorPos(&p); ScreenToClient(hwnd, &p);
        mousePos = p;
    }
};
