#pragma once

#include <cstdint>

enum Material {
    M_AIR = 0,
    M_GRASS,
    M_SAND,
    M_DIRT,
    M_GRAVEL,
    M_WATER,
    M_SPIKES,
    M_ICE,
    M_UNKNOWN
};

constexpr int MATERIAL_COUNT = M_UNKNOWN + 1;

class Tile {
public:
    const char* name;
    bool solid;        // bloque que colisiona
    float friction;    // 0..1 multiplicador para movimiento
    uint32_t color;    // 0xRRGGBB para render simple
    bool destructible; // el tile puede destruirse
    bool explosive;    // explota al recibir daño
    int damage;        // daño que causa al contacto (si aplica)
};

Tile GetTile(Material m);
Material CharToMaterial(char ch);
bool IsTerrainChar(char ch);
bool IsSolidChar(char ch);
