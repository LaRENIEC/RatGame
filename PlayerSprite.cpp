#include "PlayerSprite.h"
#include <cmath>
#include <algorithm>

using namespace Gdiplus;
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

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

    // --- nuevas texturas de expresión facial (opcionales) ---
    m_faceNeutral = m_tex->Load(basePath + L"ratface_neutral.png");
    m_faceBlink = m_tex->Load(basePath + L"ratface_blink.png");
    m_faceHurt = m_tex->Load(basePath + L"ratface_hurt.png");
    m_faceDead = m_tex->Load(basePath + L"ratface_dead.png");
    // si no existen, serán nullptr y se usará el fallback procedural.

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

    // --- animación de marcha ---
    float speed = std::sqrt(player.vel.x * player.vel.x + player.vel.y * player.vel.y);
    float walkFactor = (player.onGround && speed > 10.0f) ? std::min<float>(speed / 200.0f, 1.5f) : 0.0f;

    m_footAngle = std::sin(m_time * (6.0f + walkFactor * 6.0f)) * (18.0f + walkFactor * 24.0f);
    m_earWiggle = std::sin(m_time * 6.0f) * (6.0f + walkFactor * 6.0f);
    m_bob = std::sin(m_time * (8.0f + walkFactor * 6.0f)) * (2.0f + walkFactor * 6.0f);

    // Si hay daño activo, fuerza expresión "hurt"
    if (isDamaged) {
        // forzamos la "expresión" en Draw a elegir m_faceHurt
        m_eyeOpenLeft = m_eyeOpenRight = 1.0f + 2.0f * std::min<float>(1.0f, player.damage_flash_ms / 400.0f);
        m_mouthOpen = 1.0f;
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
        // --- calcular desplazamiento de la cara mezclando movimiento y apuntado ---
        // aproximación al half-width del body (igual que en Draw)
        float approxBodyHalf = 28.0f * m_scale * m_bodyScale;
        // desplazamiento máximo por apuntado (en px)
        const float FACE_SHIFT_MAX_AIM = approxBodyHalf * 0.45f;
        // desplazamiento por movimiento (como antes, pero reducido)
        const float MOVE_THRESHOLD = 20.0f;
        const float MAX_MOVE_REF = 220.0f;
        float moveTarget = 0.0f;
        if (std::fabs(player.vel.x) > MOVE_THRESHOLD) {
            float t = std::min(1.0f, std::fabs(player.vel.x) / MAX_MOVE_REF);
            float direction = (player.vel.x < 0.0f) ? -1.0f : 1.0f;
            moveTarget = direction * (approxBodyHalf * 0.25f) * t;
        }

        // target basado en apuntado: cos(angle) = +/-1 según izquierda/derecha
        float aimTarget = std::cos(player.angle) * FACE_SHIFT_MAX_AIM;

        // mezcla: preferimos que la cara siga al mouse, pero si te mueves mucho,
        // mantenemos algo del movimiento para que no salte raro.
        float moveWeight = std::min(1.0f, std::fabs(player.vel.x) / MAX_MOVE_REF);
        float aimWeight = 0.9f; // cuánto prioriza el aim sobre el movimiento
        float targetFace = aimTarget * aimWeight + moveTarget * (1.0f - aimWeight) * moveWeight;

        // suavizado exponencial, m_faceResponse en unidades por segundo (conserva tu variable)
        float alpha = 1.0f - std::exp(-m_faceResponse * dt);
        m_faceOffsetX += (targetFace - m_faceOffsetX) * alpha;


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

    // aplicar camOffset: las coordenadas del sprite son en mundo, restamos camOffset para pantalla
    float px = player.pos.x - camOffset.x;
    float py = player.pos.y - camOffset.y + m_bob; // slight bob

    float s = m_scale;
    float bodyS = s * m_bodyScale;

    // escala de orejas (grandes pero razonables)
    float earS = s * m_earScale * 2.1f;
    float footS = s * m_footScale;

    // ---- AJUSTES para bajar orejas y colocarlas detrás del body ----
    float bodyOffsetY = -12.0f;
    float earHorizontalRatio = 0.60f; // orejas relativamente hacia afuera
    float footHorizontalRatio = 0.28f;

    float earOverlap = 0.90f; // mayor overlap => orejas bajadas
    float footOverlap = 0.90f;

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

    // --------------------------
    // --- CALCULAR faceOffsetX ---
    // --------------------------
    // Solo mover la cara lateralmente cuando el jugador camina (por vel.x).
    const float MOVE_THRESHOLD = 20.0f;   // px/s -> cuando empezar a mover la cara
    const float MAX_MOVE_REF = 220.0f;    // referencia para normalizar velocidad
    float faceOffsetX = 0.0f;
    if (std::fabs(player.vel.x) > MOVE_THRESHOLD) {
        float t = std::min(1.0f, std::fabs(player.vel.x) / MAX_MOVE_REF); // 0..1
        float direction = (player.vel.x < 0.0f) ? -1.0f : 1.0f;
        // cuánto desplazar en px (fracción del half-width del body)
        float FACE_SHIFT_MAX = bodyHalfW * 0.25f; // ajusta este valor si quieres más/menos movimiento
        faceOffsetX = direction * FACE_SHIFT_MAX * t;
    }
    // Nota: si quieres también que mire con el mouse cuando está quieto, sustituye el bloque anterior
    // por una mezcla con std::cos(player.angle). Aquí hemos dejado solo movimiento por caminar.

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
        RectF innerR(px + earXOffset - rightEarHalfW * 0.6f,
            rightEarCY - rightEarHalfH * 0.6f + 2.0f * earS,
            (rightEarHalfW * 2.0f) * 0.6f,
            (rightEarHalfH * 2.0f) * 0.6f);
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

    // ---- CARA: preferir bitmap de expresión; fallback al dibujo procedural de ojos/boca ----
    // decidir qué cara mostrar
    Gdiplus::Bitmap* faceBmp = nullptr;
    if (!player.IsAlive()) {
        faceBmp = m_faceDead ? m_faceDead : nullptr;
    }
    else if (player.damage_flash_ms > 0) {
        faceBmp = m_faceHurt ? m_faceHurt : nullptr;
    }
    else if (m_blinkTimer < 0.12f) {
        faceBmp = m_faceBlink ? m_faceBlink : nullptr;
    }
    else {
        faceBmp = m_faceNeutral ? m_faceNeutral : nullptr;
    }

    if (faceBmp) {
        // Ajustes fáciles de tunear:
        const float FACE_WIDTH_RATIO = 0.45f;     // proporción del ancho del body que debe ocupar la cara (0..1)
        const float MIN_FACE_SCALE = 0.30f;       // escala mínima absoluta
        const float MAX_FACE_SCALE = 1.00f;       // escala máxima absoluta
        const float VERTICAL_SHIFT_RATIO = -0.05f; // positivo baja la cara; negativo la sube (relativo a bodyHalfH)

        // dimensiones del body en pantalla (ya calculadas más arriba)
        float bodyW = bodyHalfW * 2.0f;
        float bodyH = bodyHalfH * 2.0f;

        int fw_px = faceBmp->GetWidth();
        int fh_px = faceBmp->GetHeight();
        if (fw_px <= 0) fw_px = 1;
        if (fh_px <= 0) fh_px = 1;

        // calculamos escala deseada respecto al ancho del body
        float desiredFaceW = bodyW * FACE_WIDTH_RATIO;
        float faceScale = desiredFaceW / float(fw_px);

        // límites manuales (no usamos std::clamp)
        if (faceScale < MIN_FACE_SCALE) faceScale = MIN_FACE_SCALE;
        if (faceScale > MAX_FACE_SCALE) faceScale = MAX_FACE_SCALE;

        // colocamos la cara centrada horizontalmente en 'px' y en el centro vertical del body
        float faceCenterY = bodyCenterY + VERTICAL_SHIFT_RATIO * bodyHalfH;

        // Dibujar la cara desplazada por faceOffsetX (no flippamos)
        DrawBitmapRotated(g, faceBmp, px + faceOffsetX, faceCenterY, 0.0f, faceScale, false);
    }
    else {
        // Fallback procedural (ojos/boca). Aplicar faceOffsetX aquí también.
        float eyeOffsetY = -10.0f * s;
        float eyeOffsetX = 10.0f * s;
        float eyeR = m_eyeRadiusBase * s;

        SolidBrush whiteBrush(Color::MakeARGB(255, 255, 255, 255));
        SolidBrush blackBrush(Color::MakeARGB(255, 8, 8, 8));
        Pen linePen(Color::MakeARGB(200, 30, 30, 30), 2.0f);

        float exLx = px - eyeOffsetX + faceOffsetX;
        float exRx = px + eyeOffsetX + faceOffsetX;
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
            RectF mouthRect(px - mr + faceOffsetX, mouthY - mr, mr * 2.0f, mr * 2.0f);
            g.FillEllipse(&blackBrush, mouthRect);
        }
        else {
            float hpPct = (player.maxHealth > 0) ? float(player.health) / float(player.maxHealth) : 1.0f;
            if (hpPct < 0.3f) {
                g.DrawBezier(&linePen,
                    px - 8.0f * s + faceOffsetX, mouthY,
                    px - 4.0f * s + faceOffsetX, mouthY + 6.0f * s,
                    px + 4.0f * s + faceOffsetX, mouthY + 6.0f * s,
                    px + 8.0f * s + faceOffsetX, mouthY);
            }
            else {
                g.DrawLine(&linePen, px - 6.0f * s + faceOffsetX, mouthY, px + 6.0f * s + faceOffsetX, mouthY);
            }
        }
    }

    // overlay si damaged (mantengo, aunque faceBmp puede cubrirlo)
    if (player.damage_flash_ms > 0) {
        float alpha = 0.4f * (player.damage_flash_ms / 400.0f);
        alpha = std::min<float>(1.0f, alpha);
        int a = (int)(alpha * 255.0f);
        SolidBrush redfb(Color::MakeARGB(a, 180, 40, 40));
        RectF bodyRect(px - bodyHalfW, bodyCenterY - bodyHalfH, bodyHalfW * 2.0f, bodyHalfH * 2.0f);
        g.FillEllipse(&redfb, bodyRect);
    }
}
