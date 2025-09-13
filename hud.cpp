#include "hud.h"
#include "Player.h" // si necesitas la definición completa

void CompositeHUD::Init(HWND mainWnd) {
    m_mainWnd = mainWnd;
    for (auto& c : m_children) if (c) c->Init(mainWnd);
}

void CompositeHUD::Shutdown() {
    for (auto& c : m_children) if (c) c->Shutdown();
    m_children.clear();
}

void CompositeHUD::OnResize(int w, int h, float scale) {
    m_w = w; m_h = h; m_scale = scale;
    for (auto& c : m_children) if (c) c->OnResize(w, h, scale);
}

void CompositeHUD::Render(HDC hdc, const Player& player, const std::vector<WeaponPickup>& pickups, bool showPickups, bool paused) {
    // Delegar en hijos (orden de añadidos)
    for (auto& c : m_children) {
        if (c) c->Render(hdc, player, pickups, showPickups, paused);
    }
}

bool CompositeHUD::OnMouseMove(int x, int y) {
    for (auto& c : m_children) if (c && c->OnMouseMove(x, y)) return true;
    return false;
}

bool CompositeHUD::OnLButtonDown(int x, int y) {
    for (auto& c : m_children) if (c && c->OnLButtonDown(x, y)) return true;
    return false;
}

bool CompositeHUD::OnLButtonUp(int x, int y) {
    for (auto& c : m_children) if (c && c->OnLButtonUp(x, y)) return true;
    return false;
}

void CompositeHUD::SetPaused(bool p) {
    for (auto& c : m_children) if (c) c->SetPaused(p);
}

void CompositeHUD::AddChild(std::unique_ptr<HUD> child) {
    if (!child) return;
    if (m_mainWnd) child->Init(m_mainWnd);
    if (m_w && m_h) child->OnResize(m_w, m_h, m_scale);
    m_children.push_back(std::move(child));
}
