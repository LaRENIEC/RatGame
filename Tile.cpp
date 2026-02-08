#include "Tile.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <string>

#include "Level.h"
#include "MaterialTextures.h"
#include "TextureManager.h"

extern TextureManager g_texMgr;

namespace {
constexpr float kTileMargin = 1.0f;
std::array<Material, 256> s_charToMaterial{};
bool s_charTableInit = false;

std::array<Gdiplus::Bitmap*, kMaterialCount> s_materialBitmaps{};
std::array<HBRUSH, kMaterialCount> s_materialBrushes{};
bool s_renderInit = false;

void InitCharTable() {
    if (s_charTableInit) return;
    s_charTableInit = true;
    s_charToMaterial.fill(M_UNKNOWN);
    s_charToMaterial[static_cast<unsigned char>('.')] = M_AIR;
    s_charToMaterial[static_cast<unsigned char>('#')] = M_DIRT;
    s_charToMaterial[static_cast<unsigned char>('G')] = M_GRASS;
    s_charToMaterial[static_cast<unsigned char>('I')] = M_ICE;
    s_charToMaterial[static_cast<unsigned char>('S')] = M_SAND;
    s_charToMaterial[static_cast<unsigned char>('V')] = M_GRAVEL;
    s_charToMaterial[static_cast<unsigned char>('W')] = M_WATER;
    s_charToMaterial[static_cast<unsigned char>('X')] = M_SPIKES;
    s_charToMaterial[static_cast<unsigned char>('A')] = M_AIR;
    s_charToMaterial[static_cast<unsigned char>('H')] = M_AIR;
    s_charToMaterial[static_cast<unsigned char>('P')] = M_AIR;
}
}

MaterialInfo GetMaterialInfo(Material m) {
    switch (m) {
    case M_GRASS:
        return { "Grass", true, 0.9f, 0x4CAF50, true, false, 40, 0 };
    case M_ICE:
        return { "Ice", true, 0.98f, 0xA5F2F3, true, false, 30, 0 };
    case M_SAND:
        return { "Sand", true, 0.6f, 0xD2B48C, true, false, 30, 0 };
    case M_DIRT:
        return { "Dirt", true, 0.85f, 0x7B5A2B, true, false, 50, 0 };
    case M_GRAVEL:
        return { "Gravel", true, 0.8f, 0x808080, true, false, 35, 0 };
    case M_WATER:
        return { "Water", false, 0.4f, 0x2196F3, false, false, 0, 0 };
    case M_SPIKES:
        return { "Spikes", true, 0.2f, 0xB71C1C, false, false, 0, 25 };
    case M_AIR:
        return { "Air", false, 1.0f, 0x14181A, false, false, 0, 0 };
    default:
        return { "Unknown", false, 1.0f, 0xFF00FF, false, false, 0, 0 };
    }
}

Material TileCharToMaterial(char ch) {
    InitCharTable();
    return s_charToMaterial[static_cast<unsigned char>(ch)];
}

bool IsTileCharSolid(char ch) {
    Material m = TileCharToMaterial(ch);
    return GetMaterialInfo(m).solid;
}

void InitTileRenderer(TextureManager& /*tm*/) {
    if (s_renderInit) return;
    s_renderInit = true;
    InitCharTable();

    for (int mi = 0; mi < kMaterialCount; ++mi) {
        Material m = static_cast<Material>(mi);
        s_materialBitmaps[mi] = GetMaterialTexture(m);
        MaterialInfo info = GetMaterialInfo(m);
        int rr = (info.color >> 16) & 0xFF;
        int gg = (info.color >> 8) & 0xFF;
        int bb = (info.color) & 0xFF;
        s_materialBrushes[mi] = CreateSolidBrush(RGB(rr, gg, bb));
    }
}

void ShutdownTileRenderer() {
    for (auto& brush : s_materialBrushes) {
        if (brush) {
            DeleteObject(brush);
            brush = nullptr;
        }
    }
    s_renderInit = false;
}

void RenderVisibleTiles(HDC hdc, const Level& level, float camX, float camY, int clientW, int clientH) {
    if (!s_renderInit) {
        return;
    }

    const float tileSize = TILE_SIZE;

    int minC = std::max(0, (int)std::floor(camX / tileSize));
    int maxC = std::min(level.width - 1, (int)std::floor((camX + clientW) / tileSize));
    int minR = std::max(0, (int)std::floor(camY / tileSize));
    int maxR = std::min(level.height - 1, (int)std::floor((camY + clientH) / tileSize));

    const int margin = static_cast<int>(kTileMargin);
    minC = std::max(0, minC - margin);
    minR = std::max(0, minR - margin);
    maxC = std::min(level.width - 1, maxC + margin);
    maxR = std::min(level.height - 1, maxR + margin);

    for (int r = minR; r <= maxR; ++r) {
        const std::string& row = level.tiles[r];
        int baseY = (int)(r * tileSize - camY);
        for (int c = minC; c <= maxC; ++c) {
            char ch = row[c];
            if (ch == '.') continue;
            Material mat = TileCharToMaterial(ch);
            int sx = (int)(c * tileSize - camX);
            int sy = baseY;

            Gdiplus::Bitmap* bmp = s_materialBitmaps[mat];
            if (bmp) {
                g_texMgr.Draw(bmp, hdc, sx, sy, (int)tileSize, (int)tileSize);
            } else {
                RECT tr = { sx, sy, sx + (int)tileSize, sy + (int)tileSize };
                FillRect(hdc, &tr, s_materialBrushes[mat]);
            }
        }
    }
}
