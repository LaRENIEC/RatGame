// Camera.cpp
#include "Camera.h"
#include <algorithm>
#include <cmath>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

void Camera::SnapTo(const Vec2& targetPos, int viewW, int viewH) {
    float desiredX = targetPos.x - (float)viewW * 0.5f;
    float desiredY = targetPos.y - (float)viewH * 0.5f;

    float minTopX = minX;
    float maxTopX = maxX - (float)viewW;
    if (maxTopX < minTopX) {
        float mid = (minX + maxTopX) * 0.5f; // si level < viewport
        minTopX = maxTopX = mid;
    }

    float minTopY = minY;
    float maxTopY = maxY - (float)viewH;
    if (maxTopY < minTopY) {
        float mid = (minY + maxTopY) * 0.5f;
        minTopY = maxTopY = mid;
    }

    if (desiredX < minTopX) desiredX = minTopX;
    if (desiredX > maxTopX) desiredX = maxTopX;
    if (desiredY < minTopY) desiredY = minTopY;
    if (desiredY > maxTopY) desiredY = maxTopY;

    m_tx = desiredX;
    m_ty = desiredY;
}

Vec2 Camera::GetOffset(int /*viewW*/, int /*viewH*/) const {
    return Vec2{ m_tx, m_ty };
}
