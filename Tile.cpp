#include "Tile.h"

Tile GetTile(Material m) {
    switch (m) {
    case M_GRASS:   return { "Grass",  true,  0.85f, 0x4CAF50, true,  false, 0 };
    case M_SAND:    return { "Sand",   true,  0.65f, 0xD2B48C, true,  false, 0 };
    case M_DIRT:    return { "Dirt",   true,  0.9f,  0x7B5A2B, true,  false, 0 };
    case M_GRAVEL:  return { "Gravel", true,  0.8f,  0x808080, true,  false, 0 };
    case M_WATER:   return { "Water",  false, 0.4f,  0x2196F3, false, false, 0 };
    case M_SPIKES:  return { "Spikes", true,  0.2f,  0xB71C1C, false, false, 20 };
    case M_ICE:     return { "Ice",    true,  0.2f,  0xB3E5FC, true,  false, 0 };
    case M_AIR:     return { "Air",    false, 1.0f,  0x14181A, false, false, 0 };
    default:        return { "Unknown", false, 1.0f, 0xFF00FF, false, false, 0 };
    }
}

Material CharToMaterial(char ch) {
    switch (ch) {
    case '.': return M_AIR;
    case '#': return M_DIRT;
    case 'G': return M_GRASS;
    case 'S': return M_SAND;
    case 'V': return M_GRAVEL;
    case 'W': return M_WATER;
    case 'X': return M_SPIKES;
    case 'I': return M_ICE;
    case 'A': return M_AIR;
    case 'H': return M_AIR;
    case 'P': return M_AIR;
    default:  return M_UNKNOWN;
    }
}

bool IsTerrainChar(char ch) {
    switch (ch) {
    case '#':
    case 'G':
    case 'S':
    case 'V':
    case 'W':
    case 'X':
    case 'I':
        return true;
    default:
        return false;
    }
}

bool IsSolidChar(char ch) {
    static bool init = false;
    static bool solidTable[256] = {};
    if (!init) {
        for (int i = 0; i < 256; ++i) {
            Material m = CharToMaterial(static_cast<char>(i));
            solidTable[i] = GetTile(m).solid;
        }
        init = true;
    }
    return solidTable[static_cast<unsigned char>(ch)];
}
