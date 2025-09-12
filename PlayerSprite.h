#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include "TextureManager.h"
#include "Player.h"

// PlayerSprite reducido: solo body + 2 ears + 2 feet.
// Dibuja ojos simples sobre el body (no usa cabeza).

class PlayerSprite {
public:
    PlayerSprite();
    ~PlayerSprite();

    // Inicializa con un TextureManager y ruta base (p.ej. L"assets/player/")
    bool Init(TextureManager* texMgr, const std::wstring& basePath = L"assets/player/");

    // Actualiza animación (dt en segundos)
    void Update(float dt, const Player& player);

    // Dibuja en HDC (usa posiciones del Player)
    void Draw(HDC hdc, const Player& player, const Vec2& camOffset = Vec2(0, 0));

    // Ajustes de escala: escala multiplicativa
    void SetScale(float s) { m_scale = s; }
    float GetScale() const { return m_scale; }

    // Ajuste por altura objetivo en pixeles (calc scale para que body tenga esa altura)
    // targetBodyPixelHeight: altura deseada en px para la textura de 'body' después de escalar.
    void SetTargetBodyHeight(float targetBodyPixelHeight);

    // setters rápidos para ajuste fino
    void SetBodyScale(float bs) { if (bs > 0.05f) m_bodyScale = bs; }
    float GetBodyScale() const { return m_bodyScale; }
    void SetEarScale(float es) { if (es > 0.05f) m_earScale = es; }
    float GetEarScale() const { return m_earScale; }
    void SetFootScale(float fs) { if (fs > 0.05f) m_footScale = fs; }
    float GetFootScale() const { return m_footScale; }

private:
    TextureManager* m_tex = nullptr;
    Gdiplus::Bitmap* m_body = nullptr;
    Gdiplus::Bitmap* m_leftEar = nullptr;
    Gdiplus::Bitmap* m_rightEar = nullptr;
    Gdiplus::Bitmap* m_leftFoot = nullptr;
    Gdiplus::Bitmap* m_rightFoot = nullptr;

    // anim state
    float m_time = 0.0f;
    float m_handAngle = 0.0f; // no usado pero reservado
    float m_footAngle = 0.0f;
    float m_earWiggle = 0.0f;
    float m_bob = 0.0f;

    // ojos / boca procedurales
    float m_eyeOpenLeft = 1.0f;
    float m_eyeOpenRight = 1.0f;
    float m_mouthOpen = 0.0f;
    float m_eyeRadiusBase = 4.0f;
    float m_blinkTimer = 0.0f;
    float m_blinkInterval = 3.0f;

    // escalas
    float m_scale = 1.0f;
    float m_bodyScale = 0.82f; // body un poco más pequeño que antes
    float m_earScale = 1.20f;  // orejas perceptiblemente más grandes y visibles
    float m_footScale = 0.45f; // pies pequeños

    // helpers
    // ahora acepta flipHorizontal para poder reutilizar una sola textura para la oreja derecha
    void DrawBitmapRotated(Gdiplus::Graphics& g, Gdiplus::Bitmap* bmp, float cx, float cy, float angleDeg, float scale = 1.0f, bool flipHorizontal = false);
};
