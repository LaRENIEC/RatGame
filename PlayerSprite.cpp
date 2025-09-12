#include "PlayerSprite.h"
#include <cmath>
#include <algorithm>

using namespace Gdiplus;

PlayerSprite::PlayerSprite() {}
PlayerSprite::~PlayerSprite() {
    // TextureManager owns bitmaps (no borramos).
}

bool PlayerSprite::Init(TextureManager* texMgr, const std::wstring& basePath) {
    m_tex = texMgr;
    if (!m_tex) return false;

    m_body = m_tex->Load(basePath + L"ratbody.png");

    // Intentar dos orejas separadas; si no, reutilizar una y flippear.
    m_leftEar = m_tex->Load(basePath + L"ratear1.png");
    m_rightEar = m_tex->Load(basePath + L"ratear2.png");
    if (!m_leftEar) m_leftEar = m_tex->Load(basePath + L"ratear1.png");

    m_leftFoot = m_tex->Load(basePath + L"ratfeet1.png");
    m_rightFoot = m_tex->Load(basePath + L"ratfeet2.png");

    return (m_body != nullptr);
}

void PlayerSprite::SetTargetBodyHeight(float targetBodyPixelHeight) {
    if (!m_body || targetBodyPixelHeight <= 0.0f) return;
    int bodyH = m_body->GetHeight();
    if (bodyH <= 0) return;
    m_scale = targetBodyPixelHeight / float(bodyH);
}

void PlayerSprite::Update(float dt, const Player& player) {
    m_time += dt;

    // blink
    m_blinkTimer -= dt;
    if (m_blinkTimer <= 0.0f) {
        m_blinkTimer = m_blinkInterval * (0.6f + (std::rand() % 40) / 100.0f);
    }

    bool isDamaged = (player.damage_flash_ms > 0);

    // --- VOLVEMOS A LA ANIMACIÓN ORIGINAL ---
    float speed = std::sqrt(player.vel.x * player.vel.x + player.vel.y * player.vel.y);
    float walkFactor = (player.onGround && speed > 10.0f) ? std::min<float>(speed / 200.0f, 1.5f) : 0.0f;

    m_footAngle = std::sin(m_time * (6.0f + walkFactor * 6.0f)) * (18.0f + walkFactor * 24.0f);
    m_earWiggle = std::sin(m_time * 6.0f) * (6.0f + walkFactor * 6.0f);
    m_bob = std::sin(m_time * (8.0f + walkFactor * 6.0f)) * (2.0f + walkFactor * 6.0f);

    // ojos/mouth (igual que antes)
    if (isDamaged) {
        float t = player.damage_flash_ms / 400.0f;
        t = std::min<float>(1.0f, t);
        m_eyeOpenLeft = m_eyeOpenRight = 1.0f + 2.0f * t;
        m_mouthOpen = 1.0f * t;
    }
    else {
        float hpPct = (player.maxHealth > 0) ? float(player.health) / float(player.maxHealth) : 1.0f;
        if (hpPct < 0.3f) {
            m_eyeOpenLeft = m_eyeOpenRight = 0.25f;
            m_mouthOpen = 0.0f;
        }
        else if (!player.IsAlive()) {
            m_eyeOpenLeft = m_eyeOpenRight = 0.0f;
            m_mouthOpen = 0.0f;
        }
        else {
            m_eyeOpenLeft = m_eyeOpenRight = (m_blinkTimer < 0.12f) ? 0.0f : 1.0f;
            m_mouthOpen = 0.0f;
        }
    }
}

void PlayerSprite::DrawBitmapRotated(Gdiplus::Graphics& g, Gdiplus::Bitmap* bmp, float cx, float cy, float angleDeg, float scale, bool flipHorizontal) {
    if (!bmp) return;
    Gdiplus::Matrix old;
    g.GetTransform(&old);

    g.TranslateTransform(cx, cy);
    g.RotateTransform(angleDeg);
    float sx = flipHorizontal ? -scale : scale;
    g.ScaleTransform(sx, scale);

    int bw = bmp->GetWidth();
    int bh = bmp->GetHeight();
    RectF dest(-bw / 2.0f, -bh / 2.0f, (REAL)bw, (REAL)bh);
    g.DrawImage(bmp, dest);

    g.SetTransform(&old);
}

void PlayerSprite::Draw(HDC hdc, const Player& player, const Vec2& camOffset) {
    Gdiplus::Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeHighQuality);

    float px = player.pos.x;
    float py = player.pos.y + m_bob; // slight bob

    float s = m_scale;
    float bodyS = s * m_bodyScale;

    // escala de orejas (grandes pero razonables)
    float earS = s * m_earScale * 2.1f;

    float footS = s * m_footScale;

    // ---- AJUSTES para bajar orejas y colocarlas detrás del body ----
    float bodyOffsetY = -12.0f;
    float earHorizontalRatio = 0.60f; // orejas relativamente hacia afuera
    float footHorizontalRatio = 0.28f;

    // aumentamos overlap para que las orejas queden más bajas (se 'solapan' más con el top del body)
    float earOverlap = 0.90f; // mayor overlap => orejas bajadas
    float footOverlap = 0.90f;

    // ajuste vertical: positivo = bajar las orejas respecto al top del cuerpo
    float earVerticalAdjust = 12.0f * s; // bajar bastante

    // obtener dimensiones reales (si existen bitmaps)
    float bodyHalfW = 28.0f * bodyS;
    float bodyHalfH = 22.0f * bodyS;
    if (m_body) {
        bodyHalfW = (m_body->GetWidth() * 0.5f) * bodyS;
        bodyHalfH = (m_body->GetHeight() * 0.5f) * bodyS;
    }

    // dimensiones orejas (fallback)
    float leftEarHalfW = 40.0f * earS, leftEarHalfH = 40.0f * earS;
    float rightEarHalfW = 40.0f * earS, rightEarHalfH = 40.0f * earS;
    if (m_leftEar) {
        leftEarHalfW = (m_leftEar->GetWidth() * 0.5f) * earS;
        leftEarHalfH = (m_leftEar->GetHeight() * 0.5f) * earS;
    }
    if (m_rightEar) {
        rightEarHalfW = (m_rightEar->GetWidth() * 0.5f) * earS;
        rightEarHalfH = (m_rightEar->GetHeight() * 0.5f) * earS;
    }
    else if (m_leftEar) {
        rightEarHalfW = leftEarHalfW;
        rightEarHalfH = leftEarHalfH;
    }

    // dimensiones pies (fallback)
    float leftFootHalfW = 12.0f * footS, leftFootHalfH = 10.0f * footS;
    float rightFootHalfW = 12.0f * footS, rightFootHalfH = 10.0f * footS;
    if (m_leftFoot) {
        leftFootHalfW = (m_leftFoot->GetWidth() * 0.5f) * footS;
        leftFootHalfH = (m_leftFoot->GetHeight() * 0.5f) * footS;
    }
    if (m_rightFoot) {
        rightFootHalfW = (m_rightFoot->GetWidth() * 0.5f) * footS;
        rightFootHalfH = (m_rightFoot->GetHeight() * 0.5f) * footS;
    }
    else if (m_leftFoot) {
        rightFootHalfW = leftFootHalfW;
        rightFootHalfH = leftFootHalfH;
    }

    // centro Y del body
    float bodyCenterY = py + bodyOffsetY * bodyS;
    float bodyTopY = bodyCenterY - bodyHalfH;
    float bodyBottomY = bodyCenterY + bodyHalfH;

    // ---- PIES: dibujados ANTES para que el body tape la parte superior (pies detrás) ----
    float footXOffset = bodyHalfW * footHorizontalRatio;
    float leftFootCY = bodyBottomY + leftFootHalfH * (1.0f - footOverlap);
    float rightFootCY = bodyBottomY + rightFootHalfH * (1.0f - footOverlap);

    leftFootCY = std::fmin(leftFootCY, bodyBottomY + leftFootHalfH * 0.45f);
    rightFootCY = std::fmin(rightFootCY, bodyBottomY + rightFootHalfH * 0.45f);

    // elevación extra para pies (mantenerlos un poco levantados)
    leftFootCY -= 8.0f * s;
    rightFootCY -= 8.0f * s;

    if (m_leftFoot) DrawBitmapRotated(g, m_leftFoot, px - footXOffset, leftFootCY, m_footAngle * 0.9f, footS, false);
    if (m_rightFoot) DrawBitmapRotated(g, m_rightFoot, px + footXOffset, rightFootCY, -m_footAngle * 0.9f, footS, false);

    // ---- OREJAS: dibujadas ANTES del body para que queden DETRÁS del body.png ----
    float earXOffset = bodyHalfW * earHorizontalRatio;
    float earWiggleOffset = std::sin(m_time * 6.0f) * (m_earWiggle * 0.25f);

    float leftEarCY = bodyTopY - leftEarHalfH * (1.0f - earOverlap) + earWiggleOffset + earVerticalAdjust;
    float rightEarCY = bodyTopY - rightEarHalfH * (1.0f - earOverlap) + earWiggleOffset + earVerticalAdjust;

    if (m_leftEar) {
        DrawBitmapRotated(g, m_leftEar, px - earXOffset, leftEarCY, m_earWiggle * 0.6f, earS, false);
    }
    else {
        SolidBrush outerBrush(Color::MakeARGB(255, 255, 255, 255));
        SolidBrush innerBrush(Color::MakeARGB(255, 240, 200, 210));
        Pen outlinePen(Color::MakeARGB(255, 20, 20, 20), 2.0f);
        RectF outerL(px - earXOffset - leftEarHalfW, leftEarCY - leftEarHalfH, leftEarHalfW * 2.0f, leftEarHalfH * 2.0f);
        g.FillEllipse(&outerBrush, outerL);
        g.DrawEllipse(&outlinePen, outerL);
        RectF innerL(px - earXOffset - leftEarHalfW * 0.6f, leftEarCY - leftEarHalfH * 0.6f + 2.0f * earS, (leftEarHalfW * 2.0f) * 0.6f, (leftEarHalfH * 2.0f) * 0.6f);
        g.FillEllipse(&innerBrush, innerL);
    }

    if (m_rightEar) {
        DrawBitmapRotated(g, m_rightEar, px + earXOffset, rightEarCY, -m_earWiggle * 0.6f, earS, false);
    }
    else if (m_leftEar) {
        DrawBitmapRotated(g, m_leftEar, px + earXOffset, rightEarCY, -m_earWiggle * 0.6f, earS, true);
    }
    else {
        SolidBrush outerBrush2(Color::MakeARGB(255, 255, 255, 255));
        SolidBrush innerBrush2(Color::MakeARGB(255, 240, 200, 210));
        Pen outlinePen2(Color::MakeARGB(255, 20, 20, 20), 2.0f);
        RectF outerR(px + earXOffset - rightEarHalfW, rightEarCY - rightEarHalfH, rightEarHalfW * 2.0f, rightEarHalfH * 2.0f);
        g.FillEllipse(&outerBrush2, outerR);
        g.DrawEllipse(&outlinePen2, outerR);
        RectF innerR(px + earXOffset - rightEarHalfW * 0.6f, rightEarCY - rightEarHalfH * 0.6f + 2.0f * earS, (rightEarHalfW * 2.0f) * 0.6f, (rightEarHalfH * 2.0f) * 0.6f);
        g.FillEllipse(&innerBrush2, innerR);
    }

    // ---- CUERPO: dibujar DESPUÉS para que tape pies/orejas (pies y orejas detrás) ----
    if (m_body) {
        DrawBitmapRotated(g, m_body, px, bodyCenterY, 0.0f, bodyS, false);
    }
    else {
        SolidBrush fb(Color::MakeARGB(255, 200, 180, 130));
        g.FillEllipse(&fb, px - bodyHalfW, bodyCenterY - bodyHalfH, bodyHalfW * 2.0f, bodyHalfH * 2.0f);
    }

    // ---- OJOS / BOCA (dibujados POR ENCIMA del body) ----
    float eyeOffsetY = -10.0f * s;
    float eyeOffsetX = 10.0f * s;
    float eyeR = m_eyeRadiusBase * s;

    SolidBrush whiteBrush(Color::MakeARGB(255, 255, 255, 255));
    SolidBrush blackBrush(Color::MakeARGB(255, 8, 8, 8));
    Pen linePen(Color::MakeARGB(200, 30, 30, 30), 2.0f);

    float exLx = px - eyeOffsetX;
    float exRx = px + eyeOffsetX;
    float ey = py + eyeOffsetY;

    auto drawEye = [&](float cx, float cy, float open) {
        if (open <= 0.05f) {
            g.DrawLine(&linePen, cx - eyeR, cy, cx + eyeR, cy);
        }
        else if (open < 0.5f) {
            RectF rect(cx - eyeR, cy - eyeR * 0.4f, eyeR * 2.0f, eyeR * 0.8f);
            g.FillEllipse(&blackBrush, rect);
        }
        else {
            RectF rect(cx - eyeR, cy - eyeR, eyeR * 2.0f, eyeR * 2.0f);
            g.FillEllipse(&whiteBrush, rect);
            RectF pupil(cx - eyeR * 0.4f, cy - eyeR * 0.4f, eyeR * 0.8f, eyeR * 0.8f);
            g.FillEllipse(&blackBrush, pupil);
        }
        };

    if (!player.IsAlive()) {
        Pen xp(Color::MakeARGB(255, 30, 30, 30), 3.0f);
        g.DrawLine(&xp, exLx - 6.0f * s, ey - 6.0f * s, exLx + 6.0f * s, ey + 6.0f * s);
        g.DrawLine(&xp, exLx + 6.0f * s, ey - 6.0f * s, exLx - 6.0f * s, ey + 6.0f * s);
        g.DrawLine(&xp, exRx - 6.0f * s, ey - 6.0f * s, exRx + 6.0f * s, ey + 6.0f * s);
        g.DrawLine(&xp, exRx + 6.0f * s, ey - 6.0f * s, exRx - 6.0f * s, ey + 6.0f * s);
    }
    else {
        drawEye(exLx, ey, m_eyeOpenLeft);
        drawEye(exRx, ey, m_eyeOpenRight);
    }

    float mouthY = py + 6.0f * s;
    if (m_mouthOpen > 0.01f) {
        float mr = 4.0f * s + 6.0f * s * m_mouthOpen;
        RectF mouthRect(px - mr, mouthY - mr, mr * 2.0f, mr * 2.0f);
        g.FillEllipse(&blackBrush, mouthRect);
    }
    else {
        float hpPct = (player.maxHealth > 0) ? float(player.health) / float(player.maxHealth) : 1.0f;
        if (hpPct < 0.3f) {
            g.DrawBezier(&linePen,
                px - 8.0f * s, mouthY,
                px - 4.0f * s, mouthY + 6.0f * s,
                px + 4.0f * s, mouthY + 6.0f * s,
                px + 8.0f * s, mouthY);
        }
        else {
            g.DrawLine(&linePen, px - 6.0f * s, mouthY, px + 6.0f * s, mouthY);
        }
    }

    // overlay si damaged
    if (player.damage_flash_ms > 0) {
        float alpha = 0.4f * (player.damage_flash_ms / 400.0f);
        alpha = std::min<float>(1.0f, alpha);
        int a = (int)(alpha * 255.0f);
        SolidBrush redfb(Color::MakeARGB(a, 180, 40, 40));
        RectF bodyRect(px - bodyHalfW, bodyCenterY - bodyHalfH, bodyHalfW * 2.0f, bodyHalfH * 2.0f);
        g.FillEllipse(&redfb, bodyRect);
    }
}
