#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <tuple>
#include "player.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dirent.h>
#endif

// Material como enum no-scoped
enum Material {
    M_AIR = 0,
    M_GRASS,
    M_SAND,
    M_DIRT,
    M_GRAVEL,
    M_WATER,
    M_SPIKES,
    M_UNKNOWN
};

struct MaterialInfo {
    const char* name;
    bool solid;        // bloque que colisiona
    float friction;    // 0..1 multiplicador para movimiento
    uint32_t color;    // 0xRRGGBB para render simple
};
constexpr float TILE_SIZE = 48.0f; // <- aquí cambias el tamaño; antes era 32.0f
struct LevelObject {
    char tag = '.';
    int row = 0;
    int col = 0;

    // devuelve posición world centrada en el tile
    inline float WorldX(float tileSize = TILE_SIZE) const { return col * tileSize + tileSize * 0.5f; }
    inline float WorldY(float tileSize = TILE_SIZE) const { return row * tileSize + tileSize * 0.5f; }
};
class Level {
public:
    Level() = default;
    virtual ~Level() = default;
    // ----- límites globales para evitar mapas "infinitos" -----
// puedes ajustar a lo que quieras; valores recomendados
// ancho grande (mapas largos) y altura moderada.
    static constexpr int MAX_WIDTH = 2048;
    static constexpr int MAX_HEIGHT = 512;

    // Clamp helper para normalizar width/height antes de usarlos
    static inline void ClampDimensions(int& w, int& h) {
        if (w < 1) w = 1;
        if (h < 1) h = 1;
        if (w > MAX_WIDTH) w = MAX_WIDTH;
        if (h > MAX_HEIGHT) h = MAX_HEIGHT;
    }

    int width = 0;
    int height = 0;
    std::vector<std::string> tiles;      // raw chars: tiles[row][col]
    std::vector<Material> materialGrid;  // row-major: materialGrid[r*width + c]
    std::string sky; // si vacío, usar sky por defecto o color sólido
    float spawnX = -1.0f;
    float spawnY = -1.0f;

    inline bool InBounds(int r, int c) const {
        return r >= 0 && r < height && c >= 0 && c < width;
    }
    inline Material TileMaterial(int r, int c) const {
        if (!InBounds(r, c)) return M_AIR;
        return materialGrid[r * width + c];
    }
    inline char TileCharAt(int r, int c) const {
        if (!InBounds(r, c)) return '.';
        if ((int)tiles.size() != height) return '.';
        return tiles[r][c];
    }

    inline int WorldToTileX(float x, float tileSize) const { return (int)(x / tileSize); }
    inline int WorldToTileY(float y, float tileSize) const { return (int)(y / tileSize); }

    Material MaterialAtWorld(float x, float y, float tileSize) const {
        int c = WorldToTileX(x, tileSize);
        int r = WorldToTileY(y, tileSize);
        return TileMaterial(r, c);
    }

    bool IsSolidAtWorld(float x, float y, float tileSize) const;

    // builder helpers
    void BuildMaterialGrid();
    static Material CharToMaterial(char ch);

    void PlacePlayerAtSpawn(Player& p);
    // extensibilidad / hooks
    virtual void OnApply() { /* default: no-op */ }

    virtual void EnumerateObjects(std::vector<LevelObject>& out) const;

    void EnsureTilesSize();
};

// Manager que carga/guarda niveles desde archivos
class LevelManager {
public:
    LevelManager() {}
    std::unique_ptr<Level> LoadLevel(int levelIndex);
    std::unique_ptr<Level> LoadLevelFromFile(const std::string& path);
    bool SaveLevelToFile(const Level& lvl, const std::string& path);
    static std::unique_ptr<Level> CreateLevelFromAscii(const std::vector<std::string>& rows, float spawnX, float spawnY);
    void ApplyLevel(const Level& lvl);

    // lista nombres de ficheros dentro de "levels" (por ejemplo "level01.map")
    std::vector<std::string> ListAvailableLevels() const;
};

extern std::unique_ptr<Level> g_currentLevel;

MaterialInfo GetMaterialInfo(Material m);
