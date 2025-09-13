#pragma once
#include "Player.h" // para Vec2 (ya lo usas en el proyecto)

class Camera {
public:
    Camera() = default;

    // Actualiza la cámara para seguir `targetPos`.
    void SnapTo(const Vec2& targetPos, int viewW, int viewH);
    // viewW/viewH son las dimensiones en píxeles del viewport (ventana cliente).
 //   void Update(const Vec2& targetPos, float dt, int viewW, int viewH);

    // Devuelve el offset (top-left world coords) que se resta de las coordenadas mundo para obtener pantalla.
    Vec2 GetOffset(int viewW, int viewH) const;

    // Límites de mundo (en coordenadas mundo). El caller (p.ej. LevelManager::ApplyLevel) debe setearlos.
    float minX = -1e6f;
    float minY = -1e6f;
    float maxX = 1e6f;
    float maxY = 1e6f;

private:
    float m_tx = 0.0f; // top-left x
    float m_ty = 0.0f; // top-left y
};
