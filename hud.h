#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include "GameMovement.h" // <-- asegura que WeaponPickup esté definido de forma consistente

struct Player;

class HUD {
public:
    HUD() {}
    virtual ~HUD() {}

    virtual void Init(HWND mainWnd) { (void)mainWnd; }
    virtual void Shutdown() {}
    virtual void OnResize(int w, int h, float scale) { (void)w; (void)h; (void)scale; }

    // FIRMA EXACTA usada por el resto del proyecto
    virtual void Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups, bool paused) {
        (void)hdc; (void)player; (void)pickups; (void)showPickups; (void)paused;
    }

    virtual bool OnMouseMove(int x, int y) { (void)x; (void)y; return false; }
    virtual bool OnLButtonDown(int x, int y) { (void)x; (void)y; return false; }
    virtual bool OnLButtonUp(int x, int y) { (void)x; (void)y; return false; }
    virtual void SetPaused(bool p) { (void)p; }
};

// CompositeHUD: contenedor de componentes HUD
class CompositeHUD : public HUD {
public:
    CompositeHUD() {}
    ~CompositeHUD() override { Shutdown(); }

    void Init(HWND mainWnd) override;
    void Shutdown() override;
    void OnResize(int w, int h, float scale) override;
    void Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups, bool paused) override;
    bool OnMouseMove(int x, int y) override;
    bool OnLButtonDown(int x, int y) override;
    bool OnLButtonUp(int x, int y) override;
    void SetPaused(bool p) override;

    void AddChild(std::unique_ptr<HUD> child);

private:
    std::vector<std::unique_ptr<HUD>> m_children;
    HWND m_mainWnd = NULL;
    int m_w = 0, m_h = 0;
    float m_scale = 1.0f;
};
