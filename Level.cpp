#include "Level.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include "camera.h"

#include "Player.h"
#include "GameMovement.h" // WeaponPickup
#include "level1.h"       // si tienes un level embebido
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif
// globals del juego (definidos en RatGame.cpp)
extern Player g_player;
static Camera g_camera;
extern std::vector<WeaponPickup> g_pickups;
extern std::vector<Bullet> g_bullets;

std::unique_ptr<Level> g_currentLevel = nullptr;

// ----------------------- Material mapping -----------------------
Material Level::CharToMaterial(char ch) {
    return ::CharToMaterial(ch);
}

void Level::EnsureTilesSize() {
    if (width <= 0 || height <= 0) return;
    if ((int)tiles.size() != height) tiles.assign(height, std::string(width, '.'));
    for (auto& row : tiles) {
        if ((int)row.size() < width) row += std::string(width - (int)row.size(), '.');
        else if ((int)row.size() > width) row.resize(width);
    }
}

void Level::BuildMaterialGrid() {
    materialGrid.clear();
    if (width <= 0 || height <= 0) return;
    EnsureTilesSize();
    materialGrid.resize(width * height, M_AIR);
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            char ch = tiles[r][c];
            materialGrid[r * width + c] = CharToMaterial(ch);
        }
    }
}

bool Level::IsSolidAtWorld(float x, float y, float tileSize) const {
    Material m = MaterialAtWorld(x, y, tileSize);
    MaterialInfo mi = GetMaterialInfo(m);
    return mi.solid;
}

// Level.cpp
void Level::EnumerateObjects(std::vector<LevelObject>& out) const {
    out.clear();
    if (width <= 0 || height <= 0) return;
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            char ch = TileCharAt(r, c);
            if (ch == '.') continue;
            // ignore terrain tiles:
            if (IsTerrainChar(ch)) continue;
            LevelObject obj;
            obj.tag = ch;
            obj.row = r;
            obj.col = c;
            out.push_back(obj);
        }
    }
}
// asegúrate de tener #include "Player.h" en este CPP (ya lo tienes según tu paste)
void Level::PlacePlayerAtSpawn(Player& p) {
    const float TILE = TILE_SIZE;

    // 1) Preferir tile 'P' si existe (centro del tile)
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            if (TileCharAt(r, c) == 'P') {
                p.pos = Vec2(c * TILE + TILE * 0.5f, r * TILE + TILE * 0.5f);
                p.vel = Vec2(0.0f, 0.0f);
                // si por alguna razón el punto queda dentro de tile sólido, intentar elevarlo
                int tries = 0;
                while (tries < 8 && IsSolidAtWorld(p.pos.x, p.pos.y, TILE)) {
                    p.pos.y -= TILE; // subir un tile
                    ++tries;
                }
                return;
            }
        }
    }

    // 2) Si no hay 'P', usar spawnX/spawnY si tienen sentido (fallback)
    if (spawnX >= 0.0f && spawnY >= 0.0f) {
        p.pos = Vec2(spawnX, spawnY);
        p.vel = Vec2(0.0f, 0.0f);
        return;
    }
    else {
        // default seguro: un par de tiles desde la izquierda y cerca del final
        p.pos = Vec2(2.0f * TILE + TILE * 0.5f, (std::max(1, height - 3)) * TILE + TILE * 0.5f);
        p.vel = Vec2(0.0f, 0.0f);
    }

    // 3) Si dicho punto está dentro de sólido, buscar el primer tile no sólido (escaneo simple)
    if (IsSolidAtWorld(p.pos.x, p.pos.y, TILE)) {
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                Material m = TileMaterial(r, c);
                if (!GetMaterialInfo(m).solid) {
                    p.pos = Vec2(c * TILE + TILE * 0.5f, r * TILE + TILE * 0.5f);
                    p.vel = Vec2(0.0f, 0.0f);
                    return;
                }
            }
        }
    }
}

// ----------------------- helpers filesystem (Win32 + fallback POSIX) -----------------------
static bool DirExistsA(const char* p) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(p);
    return (a != INVALID_FILE_ATTRIBUTES) && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(p, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
#endif
}
static bool FileExistsA(const char* p) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(p);
    return (a != INVALID_FILE_ATTRIBUTES) && !(a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(p, &st) != 0) return false;
    return S_ISREG(st.st_mode);
#endif
}

static void EnsureDirExists(const std::string& path) {
    if (path.empty()) return;
#ifdef _WIN32
    if (!DirExistsA(path.c_str())) CreateDirectoryA(path.c_str(), NULL);
#else
    if (!DirExistsA(path.c_str())) {
        // try mkdir -p behavior
        std::string cur;
        for (size_t i = 0; i < path.size(); ++i) {
            cur.push_back(path[i]);
            if (path[i] == '/' || i + 1 == path.size()) {
                mkdir(cur.c_str(), 0755);
            }
        }
    }
#endif
}

// ----------------------- parsing of .map files (tolerante) -----------------------
static bool ParseLevelFromStream(std::istream& ifs, Level& outLevel, std::string& err) {
    std::string raw;
    auto trim = [](std::string s)->std::string {
        size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) ++a;
        size_t b = s.size(); while (b > a && isspace((unsigned char)s[b - 1])) --b;
        return s.substr(a, b - a);
        };

    auto readNonComment = [&](std::string& out)->bool {
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

    if (!raw.empty() && raw[0] == '@') {
        outLevel = Level();
        std::vector<std::string> tileRows;
        int expectedWidth = -1, expectedHeight = -1;
        bool inTileRows = false;

        auto processDirectiveLine = [&](const std::string& line) {
            if (line.empty()) return;
            if (line[0] != '@') {
                if (inTileRows) tileRows.push_back(line);
                return;
            }
            std::istringstream iss(line);
            std::string dir; iss >> dir;
            if (dir == "@SIZE") {
                iss >> expectedWidth >> expectedHeight;
            }
            else if (dir == "@SPAWN") {
                iss >> outLevel.spawnX >> outLevel.spawnY;
            }
            else if (dir == "@SKY") {
                std::string s;
                if (iss >> s) {
                    outLevel.sky = s;
                }
            }
            else if (dir == "@TILE_ROWS") {
                inTileRows = true;
            }
            else if (dir == "@END_TILE_ROWS") {
                inTileRows = false;
            }
            else if (dir == "@PLATFORM") {
                int row = 0, col = 0, len = 0; iss >> row >> col >> len;
                tileRows.push_back(std::string("#PL ") + std::to_string(row) + " " + std::to_string(col) + " " + std::to_string(len));
            }
            else if (dir == "@ENTITY" || dir == "@E") {
                char tag = 'E'; int col = 0, row = 0; iss >> tag >> col >> row;
                tileRows.push_back(std::string("#OBJ ") + tag + " " + std::to_string(col) + " " + std::to_string(row));
            }
            else if (dir == "@PICKUP") {
                char tag = 'A'; int col = 0, row = 0; iss >> tag >> col >> row;
                tileRows.push_back(std::string("#OBJ ") + tag + " " + std::to_string(col) + " " + std::to_string(row));
            }
            
            };

        processDirectiveLine(raw);
        while (std::getline(ifs, raw)) {
            std::string t = trim(raw);
            if (t.empty()) continue;
            if (t.rfind("#", 0) == 0 || t.rfind("//", 0) == 0 || t.rfind(";", 0) == 0) continue;
            processDirectiveLine(t);
        }

        if (expectedWidth <= 0) {
            for (auto& r : tileRows) { if (!r.empty() && r[0] != '#') expectedWidth = std::max<int>(expectedWidth, (int)r.size()); }
            if (expectedWidth <= 0) expectedWidth = 30;
        }
        if (expectedHeight <= 0) {
            int cnt = 0; for (auto& r : tileRows) if (r.empty() || r[0] != '#') ++cnt;
            expectedHeight = (cnt > 0) ? cnt : 20;
        }

        // clamp a límites globales (evitar anchos/altos absurdos)
        Level::ClampDimensions(expectedWidth, expectedHeight);

        outLevel.width = expectedWidth;
        outLevel.height = expectedHeight;
        outLevel.tiles.assign(outLevel.height, std::string(outLevel.width, '.'));

        int pasteRow = 0;
        for (auto& r : tileRows) {
            if (r.empty()) continue;
            if (r.rfind("#PL ", 0) == 0) {
                std::istringstream iss(r.substr(4));
                int row, col, len; iss >> row >> col >> len;
                if (row >= 0 && row < outLevel.height) {
                    for (int x = col; x < col + len && x < outLevel.width; ++x) if (x >= 0) outLevel.tiles[row][x] = '#';
                }
            }
            else if (r.rfind("#OBJ ", 0) == 0) {
                std::istringstream iss(r.substr(5));
                char tag; int col, row; iss >> tag >> col >> row;
                if (row >= 0 && row < outLevel.height && col >= 0 && col < outLevel.width) outLevel.tiles[row][col] = tag;
            }
            else {
                if (pasteRow >= outLevel.height) pasteRow = outLevel.height - 1;
                std::string src = r;
                // Truncar si la fila excede el ancho permitido
                if ((int)src.size() > outLevel.width) src = src.substr(0, outLevel.width);
                if ((int)src.size() < outLevel.width) src += std::string(outLevel.width - (int)src.size(), '.');
                outLevel.tiles[pasteRow++] = src;
            }
        }
        const float TILE = TILE_SIZE;
        // Si spawn ha sido proporcionado en directo (probablemente como tile indices en @SPAWN),
        // convertimos a world coords siempre (centro del tile).
        if (outLevel.spawnX >= 0.0f && outLevel.spawnY >= 0.0f) {
            // asumimos spawnX/Y como TILE indices (o números pequeños): conviértelo a mundo
            outLevel.spawnX = outLevel.spawnX * TILE + TILE * 0.5f;
            outLevel.spawnY = outLevel.spawnY * TILE + TILE * 0.5f;
        }
        else {
            outLevel.spawnX = 2 * TILE + TILE * 0.5f;
            outLevel.spawnY = (outLevel.height - 3) * TILE + TILE * 0.5f;
        }

        outLevel.BuildMaterialGrid();
        return true;
    }
    else {
        // legacy
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
    std::ifstream ifs(path.c_str(), std::ios::in);
    if (!ifs) return nullptr;
    auto L = std::make_unique<Level>();
    std::string err;
    if (!ParseLevelFromStream(ifs, *L, err)) {
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
        if (levelIndex == 1) {
            auto built = CreateBuiltInLevel1();
            if (built) return built;
        }
        return nullptr;
    }
    return LoadLevelFromFile(path);
}

bool LevelManager::SaveLevelToFile(const Level& lvl, const std::string& path) {
    size_t found = path.find_last_of("/\\");
    std::string parent = (found == std::string::npos) ? std::string("levels") : path.substr(0, found);
    EnsureDirExists(parent);

    std::ofstream ofs(path.c_str(), std::ios::out | std::ios::trunc);
    if (!ofs) return false;

    // uso formato de directivas (legible / editable)
    ofs << "@SIZE " << lvl.width << " " << lvl.height << "\n";
    const float TILE = TILE_SIZE;
    int spawnTileX = (int)std::floor((lvl.spawnX) / TILE);
    int spawnTileY = (int)std::floor((lvl.spawnY) / TILE);
    ofs << "@SPAWN " << spawnTileX << " " << spawnTileY << "\n";
    ofs << "@TILE_ROWS\n";
    for (int r = 0; r < lvl.height; ++r) {
        std::string row = (r < (int)lvl.tiles.size()) ? lvl.tiles[r] : std::string(lvl.width, '.');
        if ((int)row.size() < lvl.width) row += std::string(lvl.width - (int)row.size(), '.');
        ofs << row.substr(0, lvl.width) << "\n";
    }
    if (!lvl.sky.empty()) {
        ofs << "@SKY " << lvl.sky << "\n";
    }
    ofs << "@TILE_ROWS\n";
    ofs << "@END_TILE_ROWS\n";
    ofs.close();
    return true;
}

std::unique_ptr<Level> LevelManager::CreateLevelFromAscii(const std::vector<std::string>& rows, float spawnX, float spawnY) {
    if (rows.empty()) return nullptr;
    int h = (int)rows.size();
    int w = (int)rows[0].size();

    Level::ClampDimensions(w, h);

    auto L = std::make_unique<Level>();
    L->width = w; L->height = h;
    L->tiles.assign(h, std::string(w, '.'));
    for (int r = 0; r < h && r < (int)rows.size(); ++r) {
        std::string row = rows[r];
        if ((int)row.size() > w) row = row.substr(0, w);
        if ((int)row.size() < w) row += std::string(w - (int)row.size(), '.');
        L->tiles[r] = row;
    }
    L->spawnX = spawnX;
    L->spawnY = spawnY;
    L->BuildMaterialGrid();
    return L;
}   

void LevelManager::ApplyLevel(const Level& lvl) {
    g_currentLevel = std::make_unique<Level>(lvl);

    const float TILE = TILE_SIZE;
            float px = c * TILE + TILE / 2.0f;
            float py = r * TILE + TILE / 2.0f;
    g_camera.minY = -TILE * 1.0f;
    g_camera.maxX = (float)lvl.width * TILE + TILE * 2.0f;
    g_camera.maxY = (float)lvl.height * TILE + TILE * 2.0f;

    // Reset player/bullets/pickups
    g_player.Reset();
    // colocar jugador usando el player-start del nivel (primero busca 'P', si no existe usa spawnX/spawnY)
    g_currentLevel = std::make_unique<Level>(lvl); // (ya haces esto más arriba; si no, mantenlo)
    g_currentLevel->OnApply(); // si tu OnApply hace algo importante
    g_currentLevel->PlacePlayerAtSpawn(g_player);

    g_bullets.clear();

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

    g_currentLevel->OnApply();
}

// list levels in "levels" directory (Windows FindFirstFileA or POSIX opendir)
std::vector<std::string> LevelManager::ListAvailableLevels() const {
    std::vector<std::string> out;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("levels\\*.map", &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) out.emplace_back(fd.cFileName);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir("levels");
    if (!d) return out;
    struct dirent* ent;
    while ((ent = readdir(d)) != nullptr) {
        if (ent->d_type == DT_REG) out.emplace_back(ent->d_name);
    }
    closedir(d);
#endif
    return out;
}
