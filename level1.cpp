#include "level1.h"
#include "Level.h"
#include <vector>
#include <string>
#include <memory>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
// Generador procedural del level 1: ancho grande, plataformas, enemigos y boss.
std::unique_ptr<Level> CreateBuiltInLevel1() {
    const int TILE = 32;
    const int WIDTH = 180;   // ancho en tiles -> incrementa el espacio horizontal
    const int HEIGHT = 20;   // alto en tiles
    const int groundRow = HEIGHT - 1;   // fila inferior: dirt
    const int grassRow = HEIGHT - 2;    // fila encima del dirt: grass (superficie)

    std::vector<std::string> rows;
    rows.assign(HEIGHT, std::string(WIDTH, '.'));

    // 1) suelo continuo: grass over dirt
    for (int c = 0; c < WIDTH; ++c) {
        rows[grassRow][c] = 'G'; // grass
        rows[groundRow][c] = '#'; // dirt
    }

    // 2) plataformas distribuidas (varios "islands")
    auto placePlatform = [&](int row, int x0, int len) {
        if (row < 0 || row >= HEIGHT) return;
        int x1 = std::min(WIDTH - 1, x0 + len - 1);
        for (int x = std::max(0, x0); x <= x1; ++x) {
            rows[row][x] = '#';
            // opcional: poner un poco de grass en los bordes de plataformas
            if (row > 0) rows[row - 1][x] = '.'; // aire encima
        }
        };

    // plataformas principales (sube hacia la izquierda->derecha con saltos)
    placePlatform(HEIGHT - 5, 5, 10);    // inicio cercano al spawn
    placePlatform(HEIGHT - 8, 25, 8);
    placePlatform(HEIGHT - 6, 45, 12);
    placePlatform(HEIGHT - 10, 70, 10);
    placePlatform(HEIGHT - 7, 95, 14);
    placePlatform(HEIGHT - 9, 125, 12);
    placePlatform(HEIGHT - 6, 150, 10);  // antes del tramo final
    placePlatform(HEIGHT - 5, 170, 6);   // pequeño step antes del boss area

    // 3) plataformas grandes/puentes y pilares
    // una pasarela baja con huecos
    for (int x = 35; x < 55; x += 2) {
        rows[HEIGHT - 4][x] = '#';
    }

    // picos/spikes puntuales (zona de riesgo)
    rows[groundRow - 1][30] = 'X';
    rows[groundRow - 1][31] = 'X';
    rows[groundRow - 1][32] = 'X';

    // 4) enemigos 'E' colocados en plataformas y en suelo
    auto placeEnemy = [&](int tileX, int tileY) {
        if (tileY < 0 || tileY >= HEIGHT || tileX < 0 || tileX >= WIDTH) return;
        rows[tileY][tileX] = 'E';
        };

    placeEnemy(8, HEIGHT - 6);    // sobre primera plataforma
    placeEnemy(27, HEIGHT - 9);
    placeEnemy(50, HEIGHT - 7);
    placeEnemy(74, HEIGHT - 11);
    placeEnemy(98, HEIGHT - 8);
    placeEnemy(129, HEIGHT - 10);
    placeEnemy(152, HEIGHT - 7);

    // algunos enemigos en el suelo (patrullando)
    placeEnemy(18, grassRow - 0);
    placeEnemy(60, grassRow - 0);
    placeEnemy(110, grassRow - 0);

    // 5) pickups: shotgun(H) y ammo(A)
    auto placePickup = [&](char ch, int tx, int ty) {
        if (ty < 0 || ty >= HEIGHT || tx < 0 || tx >= WIDTH) return;
        rows[ty][tx] = ch;
        };

    placePickup('A', 12, HEIGHT - 3);  // ammo en una plataforma baja
    placePickup('H', 140, HEIGHT - 7); // shotgun más avanzado

    // 6) boss 'B' cerca del extremo derecho, ligeramente arriba del suelo
    int bossX = WIDTH - 8;
    int bossY = HEIGHT - 3;
    if (bossX >= 0 && bossX < WIDTH) rows[bossY][bossX] = 'B';

    // 7) spawn point (en píxeles). Spawn cerca del inicio, sobre la primera plataforma o suelo.
    float spawnTileX = 6; // tile column index
    float spawnTileY = HEIGHT - 3; // tile row index (un tile encima del dirt)
    float spawnX = spawnTileX * TILE + TILE * 0.5f;
    float spawnY = spawnTileY * TILE + TILE * 0.5f;

    // Build the Level through the manager helper
    return LevelManager::CreateLevelFromAscii(rows, spawnX, spawnY);
}
