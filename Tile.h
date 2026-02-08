#pragma once

#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

class Level;
class TextureManager;
namespace Gdiplus { class Bitmap; }

// Material como enum no-scoped
enum Material {
    M_AIR = 0,
    M_GRASS,
    M_ICE,
    M_SAND,
    M_DIRT,
    M_GRAVEL,
    M_WATER,
    M_SPIKES,
    M_UNKNOWN
};

constexpr int kMaterialCount = M_UNKNOWN + 1;

struct MaterialInfo {
    const char* name;
    bool solid;        // bloque que colisiona
    float friction;    // 0..1 multiplicador para movimiento
    uint32_t color;    // 0xRRGGBB para render simple
    bool destructible; // puede romperse
    bool explosive;    // explota al romperse
    int maxHealth;     // vida base si es destruible
    int touchDamage;   // daño al tocar (spikes, etc)
};

MaterialInfo GetMaterialInfo(Material m);
Material TileCharToMaterial(char ch);
bool IsTileCharSolid(char ch);

void InitTileRenderer(TextureManager& tm);
void ShutdownTileRenderer();
void RenderVisibleTiles(HDC hdc, const Level& level, float camX, float camY, int clientW, int clientH);
