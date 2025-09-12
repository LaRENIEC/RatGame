#include "Level.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <windows.h> // GetFileAttributesA, CreateDirectoryA
#include <cstdio>

#include "Player.h"
#include "GameMovement.h" // para WeaponPickup
#include "level1.h"

// declara los globals que están definidos en RatGame.cpp
extern Player g_player;
extern std::vector<WeaponPickup> g_pickups;
extern std::vector<Bullet> g_bullets;

// pointer global al nivel cargado
std::unique_ptr<Level> g_currentLevel = nullptr;

// ----------------------- Material table -----------------------
MaterialInfo GetMaterialInfo(Material m) {
    switch (m) {
    case M_GRASS:   return { "Grass",  true,  0.85f, 0x4CAF50 };
    case M_SAND:    return { "Sand",   true,  0.65f, 0xD2B48C };
    case M_DIRT:    return { "Dirt",   true,  0.9f,  0x7B5A2B };
    case M_GRAVEL:  return { "Gravel", true,  0.8f,  0x808080 };
    case M_WATER:   return { "Water",  false, 0.4f,  0x2196F3 };
    case M_SPIKES:  return { "Spikes", true,  0.2f,  0xB71C1C };
    case M_AIR:     return { "Air",    false, 1.0f,  0x14181A };
    default:        return { "Unknown", false, 1.0f, 0xFF00FF };
    }
}

// ----------------------- Material mapping -----------------------
Material Level::CharToMaterial(char ch) {
    switch (ch) {
    case '.': return M_AIR;
    case '#': return M_DIRT;
    case 'G': return M_DIRT; // <- 'G' ahora usa DIRT (eliminamos comportamiento/plantilla "grass")
    case 'S': return M_SAND;
    case 'V': return M_GRAVEL;
    case 'W': return M_WATER;
    case 'X': return M_SPIKES;
    case 'A': return M_AIR; // 'A' seguirá siendo interpretado como pickup en ApplyLevel
    case 'H': return M_AIR; // H = shotgun pickup char in map files (tile is air)
    case 'P': return M_AIR;
    default:  return M_UNKNOWN;
    }
}

void Level::BuildMaterialGrid() {
    materialGrid.clear();
    if (width <= 0 || height <= 0) return;
    materialGrid.resize(width * height, M_AIR);
    if ((int)tiles.size() == height) {
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                char ch = (c < (int)tiles[r].size()) ? tiles[r][c] : '.';
                materialGrid[r * width + c] = CharToMaterial(ch);
            }
        }
    }
    else {
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                materialGrid[r * width + c] = (r == height - 1) ? M_DIRT : M_AIR;
            }
        }
    }
}

bool Level::IsSolidAtWorld(float x, float y, float tileSize) const {
    Material m = MaterialAtWorld(x, y, tileSize);
    MaterialInfo mi = GetMaterialInfo(m);
    return mi.solid;
}

// ----------------------- filesystem helpers -----------------------
static bool DirExistsA(const char* p) {
    DWORD a = GetFileAttributesA(p);
    return (a != INVALID_FILE_ATTRIBUTES) && (a & FILE_ATTRIBUTE_DIRECTORY);
}
static bool FileExistsA(const char* p) {
    DWORD a = GetFileAttributesA(p);
    return (a != INVALID_FILE_ATTRIBUTES) && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

// ensure directory exists (creates if not)
static void EnsureDirExists(const std::string& path) {
    if (path.empty()) return;
    if (!DirExistsA(path.c_str())) {
        CreateDirectoryA(path.c_str(), NULL);
    }
}

// ----------------------- parsing of .map files -----------------------
// Format:
//  # comment lines start with '#'
//  first non-comment line: "<width> <height>"
//  second non-comment: "<spawnX> <spawnY>"   (pixels, floats)
//  next 'height' lines: each a string of 'width' characters for tiles
//  extra lines ignored
// ----------------------- parsing of .map files (extended/directive-aware) -----------------------
static bool ParseLevelFromStream(std::istream& ifs, Level& outLevel, std::string& err) {
    std::string raw;
    // Trim helper
    auto trim = [](std::string s)->std::string {
        size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) ++a;
        size_t b = s.size(); while (b > a && isspace((unsigned char)s[b - 1])) --b;
        return s.substr(a, b - a);
        };

    // read first non-empty non-comment line (comments: '#' or '//' or ';')
    auto readNonComment = [&](std::string& out) -> bool {
        while (std::getline(ifs, out)) {
            std::string t = trim(out);
            if (t.empty()) continue;
            if (t.rfind("#", 0) == 0 || t.rfind("//", 0) == 0 || t.rfind(";", 0) == 0) continue;
            out = t;
            return true;
        }
        return false;
        };

    if (!readNonComment(raw)) { err = "Missing level header"; return false; }

    // If first token starts with '@', we are in directive mode.
    if (!raw.empty() && raw[0] == '@') {
        // directive-mode
        // we'll collect tile rows if @TILE_ROWS ... @END_TILE_ROWS present
        outLevel = Level(); // reset
        std::vector<std::string> tileRows;
        int expectedWidth = -1, expectedHeight = -1;
        bool inTileRows = false;

        // process first line plus rest
        auto processLine = [&](const std::string& line) {
            if (line.empty()) return;
            if (line[0] != '@') {
                if (inTileRows) {
                    // append row (allow padding later)
                    tileRows.push_back(line);
                }
                return;
            }
            // tokenise directive
            std::istringstream iss(line);
            std::string dir; iss >> dir; // e.g., @SIZE
            if (dir == "@SIZE") {
                iss >> expectedWidth >> expectedHeight;
            }
            else if (dir == "@SPAWN") {
                // accept either pixel coords or tile coords. If two ints small (<1000) assume tiles.
                float sx = 0, sy = 0; iss >> sx >> sy;
                // mark spawn in pixels later once TILE known; store temporarily in spawnX/Y as tiles (negative flag)
                outLevel.spawnX = sx; outLevel.spawnY = sy;
            }
            else if (dir == "@TILE_ROWS") {
                inTileRows = true;
            }
            else if (dir == "@END_TILE_ROWS") {
                inTileRows = false;
            }
            else if (dir == "@PLATFORM") {
                // @PLATFORM row col len  => we will materialize into tileRows later
                int row = 0, col = 0, len = 0; iss >> row >> col >> len;
                // If we don't have tileRows yet, store as temporary row entries (we'll ensure tile size later)
                // For simplicity push a special encoded string like "#PL row col len" to tileRows vector for postprocessing
                tileRows.push_back(std::string("#PL ") + std::to_string(row) + " " + std::to_string(col) + " " + std::to_string(len));
            }
            else if (dir == "@ENTITY" || dir == "@E") {
                // @ENTITY <tagchar> <col> <row>
                char tag = 'E'; int col = 0, row = 0; iss >> tag >> col >> row;
                // encode special marker row
                tileRows.push_back(std::string("#OBJ ") + tag + " " + std::to_string(col) + " " + std::to_string(row));
            }
            else if (dir == "@PICKUP") {
                char tag = 'A'; int col = 0, row = 0; iss >> tag >> col >> row;
                tileRows.push_back(std::string("#OBJ ") + tag + " " + std::to_string(col) + " " + std::to_string(row));
            }
            // other directives can be added here...
            };

        // process the first directive line we read
        processLine(raw);

        // process rest of file
        while (std::getline(ifs, raw)) {
            std::string t = trim(raw);
            if (t.empty()) continue;
            if (t.rfind("#", 0) == 0 || t.rfind("//", 0) == 0 || t.rfind(";", 0) == 0) continue;
            processLine(t);
        }

        // If expected size not set, try to deduce from tileRows
        if (expectedWidth <= 0) {
            for (auto& r : tileRows) {
                if (!r.empty() && r[0] == '#') continue;
                expectedWidth = std::max<int>(expectedWidth, static_cast<int>(r.size()));

            }
            if (expectedWidth <= 0) expectedWidth = 120;
        }
        if (expectedHeight <= 0) {
            // count only actual tile lines (not special encoded)
            int cnt = 0;
            for (auto& r : tileRows) if (r.empty() || r[0] != '#') ++cnt;
            expectedHeight = (cnt > 0) ? cnt : 20;
        }

        // create rows filled with '.'
        outLevel.width = expectedWidth;
        outLevel.height = expectedHeight;
        outLevel.tiles.assign(outLevel.height, std::string(outLevel.width, '.'));

        // apply tileRows content and special markers
        for (auto& r : tileRows) {
            if (r.empty()) continue;
            if (r.rfind("#PL ", 0) == 0) {
                // platform directive path: parse and set '#' at given row/cols
                std::istringstream iss(r.substr(4));
                int row, col, len; iss >> row >> col >> len;
                if (row >= 0 && row < outLevel.height) {
                    for (int x = col; x < col + len && x < outLevel.width; ++x) {
                        if (x >= 0) outLevel.tiles[row][x] = '#';
                    }
                }
            }
            else if (r.rfind("#OBJ ", 0) == 0) {
                std::istringstream iss(r.substr(5));
                char tag; int col, row; iss >> tag >> col >> row;
                if (row >= 0 && row < outLevel.height && col >= 0 && col < outLevel.width) {
                    outLevel.tiles[row][col] = tag;
                }
            }
            else {
                // plain tile row: append into top rows (we'll put them top-down)
                // find first empty row in tiles that consists entirely of '.' to paste into
                int target = -1;
                for (int tr = 0; tr < outLevel.height; ++tr) {
                    bool allDot = true;
                    for (char ch : outLevel.tiles[tr]) { if (ch != '.') { allDot = false; break; } }
                    if (allDot) { target = tr; break; }
                }
                if (target == -1) target = 0;
                std::string src = r;
                if ((int)src.size() < outLevel.width) src += std::string(outLevel.width - (int)src.size(), '.');
                outLevel.tiles[target] = src.substr(0, outLevel.width);
            }
        }

        // If spawn specified in tiles coords, convert to pixels (we expect user gave tiles coords)
        const float TILE = 32.0f;
        if (outLevel.spawnX >= 0.0f && outLevel.spawnY >= 0.0f) {
            // If spawn values look small it's tile coords; if large likely pixels already.
            if (outLevel.spawnX < 1000 && outLevel.spawnY < 1000) {
                outLevel.spawnX = outLevel.spawnX * TILE + TILE * 0.5f;
                outLevel.spawnY = outLevel.spawnY * TILE + TILE * 0.5f;
            }
        }
        else {
            // fallback spawn near left-top area
            outLevel.spawnX = 2 * TILE + TILE * 0.5f;
            outLevel.spawnY = (outLevel.height - 3) * TILE + TILE * 0.5f;
        }

        outLevel.BuildMaterialGrid();
        return true;
    }
    else {
        // legacy format: first line "width height"
        {
            std::istringstream iss(raw);
            if (!(iss >> outLevel.width >> outLevel.height)) { err = "Failed to parse width/height"; return false; }
            if (outLevel.width <= 0 || outLevel.height <= 0) { err = "Invalid width/height"; return false; }
        }

        if (!readNonComment(raw)) { err = "Missing spawn line"; return false; }
        {
            std::istringstream iss(raw);
            if (!(iss >> outLevel.spawnX >> outLevel.spawnY)) { err = "Failed to parse spawnX/spawnY"; return false; }
        }

        // read exact 'height' rows (skip comments/empty lines in between)
        outLevel.tiles.clear();
        while ((int)outLevel.tiles.size() < outLevel.height && std::getline(ifs, raw)) {
            size_t i = 0; while (i < raw.size() && isspace((unsigned char)raw[i])) ++i;
            if (i == raw.size()) continue;
            if (raw[i] == '#') continue;
            if ((int)raw.size() < outLevel.width) raw += std::string(outLevel.width - (int)raw.size(), '.');
            outLevel.tiles.push_back(raw.substr(0, outLevel.width));
        }
        if ((int)outLevel.tiles.size() < outLevel.height) { err = "Not enough tile rows"; return false; }
        outLevel.BuildMaterialGrid();
        return true;
    }
}

// ----------------------- LevelManager implementation -----------------------
std::unique_ptr<Level> LevelManager::LoadLevelFromFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::in);
    if (!ifs) return nullptr;
    auto L = std::make_unique<Level>();
    std::string err;
    if (!ParseLevelFromStream(ifs, *L, err)) {
        // parse error: print to stderr for debug and return nullptr
        std::cerr << "Level parse error (" << path << "): " << err << "\n";
        return nullptr;
    }
    return L;
}

std::unique_ptr<Level> LevelManager::LoadLevel(int levelIndex) {
    char buf[256];
    sprintf_s(buf, "levels\\level%02d.map", levelIndex);
    std::string path = buf;

    EnsureDirExists("levels");

    if (!FileExistsA(path.c_str())) {
        // Si piden el level 1, devolver el built-in
        if (levelIndex == 1) {
            auto built = CreateBuiltInLevel1();
            if (built) return built;
        }
        return nullptr;
    }

    return LoadLevelFromFile(path);
}

bool LevelManager::SaveLevelToFile(const Level& lvl, const std::string& path) {
    // create parent dir if necessary (simple heuristic)
    size_t pos = path.find_first_of("/\\");
    (void)pos;
    EnsureDirExists("levels"); // minimal: ensure levels/ exists in typical usage

    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs) return false;
    // write header and tiles
    ofs << lvl.width << " " << lvl.height << "\n";
    ofs << lvl.spawnX << " " << lvl.spawnY << "\n";
    for (int r = 0; r < lvl.height; ++r) {
        std::string row;
        if (r < (int)lvl.tiles.size()) row = lvl.tiles[r];
        if ((int)row.size() < lvl.width) row += std::string(lvl.width - (int)row.size(), '.');
        ofs << row.substr(0, lvl.width) << "\n";
    }
    ofs.close();
    return true;
}

std::unique_ptr<Level> LevelManager::CreateLevelFromAscii(const std::vector<std::string>& rows, float spawnX, float spawnY) {
    if (rows.empty()) return nullptr;
    int h = (int)rows.size();
    int w = (int)rows[0].size();
    auto L = std::make_unique<Level>();
    L->width = w; L->height = h;
    L->tiles = rows;
    L->spawnX = spawnX;
    L->spawnY = spawnY;
    L->BuildMaterialGrid();
    return L;
}

void LevelManager::ApplyLevel(const Level& lvl) {
    g_currentLevel = std::make_unique<Level>(lvl);

    // Ajusta bounds de cámara aquí (coinciden con tile size en render/logic)
    const float TILE = 32.0f;
    // damos márgenes adicionales para permitir avanzar y evitar tope brusco
    g_camera.minX = -TILE * 2.0f;
    g_camera.minY = -TILE * 1.0f;
    g_camera.maxX = (float)lvl.width * TILE + TILE * 2.0f;
    g_camera.maxY = (float)lvl.height * TILE + TILE * 2.0f;

    // Reset player/bullets/pickups (tu código ya lo hace)
    g_player.Reset();
    g_player.pos = Vec2(lvl.spawnX, lvl.spawnY);
    g_player.vel = Vec2(0, 0);
    g_bullets.clear();

    // pickups desde tiles (ya lo haces)
    g_pickups.clear();
    const float TILE_SIZE = 32.0f;
    for (int r = 0; r < lvl.height; ++r) {
        for (int c = 0; c < lvl.width; ++c) {
            char ch = lvl.TileCharAt(r, c);
            float px = c * TILE_SIZE + TILE_SIZE / 2.0f;
            float py = r * TILE_SIZE + TILE_SIZE / 2.0f;
            if (ch == 'H') g_pickups.push_back(WeaponPickup{ SHOTGUN, Vec2(px, py), 6 });
            else if (ch == 'A') g_pickups.push_back(WeaponPickup{ PISTOL, Vec2(px, py), 12 });
        }
    }
}
void Level::EnumerateObjects(std::vector<std::tuple<char, int, int>>& out) const {
    out.clear();
    if (width <= 0 || height <= 0) return;

    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            char ch = TileCharAt(r, c);
            if (ch == '.') continue; // aire -> no es objeto

            // Ignorar tiles de terreno (ground/grass/etc.)
            // Ajusta esta lista si añades nuevos tiles que NO quieras tratar como "objetos".
            if (ch == '#' || ch == 'G' || ch == 'S' || ch == 'V' || ch == 'W' || ch == 'X') continue;

            // cualquier otro carácter lo consideramos un "objeto" (E, B, A, H, etiquetas custom, ...)
            out.emplace_back(ch, c, r);
        }
    }
}