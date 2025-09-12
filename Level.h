#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <tuple>

// Material como enum no-scoped (mejor compatibilidad con viejos ajustes)
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

class Level {
public:
    Level() = default;
    virtual ~Level() = default;

    int width = 0;
    int height = 0;
    std::vector<std::string> tiles;      // raw chars: tiles[row][col]
    std::vector<Material> materialGrid;  // row-major: materialGrid[r*width + c]
    float spawnX = 0.0f;
    float spawnY = 0.0f;

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

    // Devuelve true si la posición mundo (x,y) cae en un tile sólido
    bool IsSolidAtWorld(float x, float y, float tileSize) const;

    // builder helpers
    void BuildMaterialGrid();
    static Material CharToMaterial(char ch);

    // --------------------
    // extensibilidad / hooks
    // --------------------
    // Permite que derivados cambien comportamiento al aplicarse el nivel.
    virtual void OnApply() { /* default: no-op */ }

    // EnumerateObjects: devuelve una lista de (char tag, tileRow, tileCol).
    // El caller (RatGame) creará entidades según el tag (E,B, pickups, etc).
    // Puedes overridear para generar enemigos procedurales, spawns dinámicos, etc.
    virtual void EnumerateObjects(std::vector<std::tuple<char, int, int>>& out) const;

    // helper utilitario para que implementaciones puedan añadir una fila de tiles
    void EnsureTilesSize();
};

// Manager que carga/guarda niveles desde archivos
class LevelManager {
public:
    LevelManager() {}
    // carga "levels/levelNN.map" por index; devuelve nullptr si no existe o parse error
    std::unique_ptr<Level> LoadLevel(int levelIndex);

    // carga un level a partir de una ruta concreta (path absoluto o relativa)
    std::unique_ptr<Level> LoadLevelFromFile(const std::string& path);

    // guarda level en archivo (crea carpeta si hace falta). devuelve true si OK
    bool SaveLevelToFile(const Level& lvl, const std::string& path);

    // helper: crea un Level a partir de un array de strings (utile para convertir ascii -> file)
    static std::unique_ptr<Level> CreateLevelFromAscii(const std::vector<std::string>& rows, float spawnX, float spawnY);

    // aplica nivel (mismo comportamiento que tenías antes)
    void ApplyLevel(const Level& lvl);
};

// global pointer al nivel cargado
extern std::unique_ptr<Level> g_currentLevel;

// consulta de propiedades por material
MaterialInfo GetMaterialInfo(Material m);
