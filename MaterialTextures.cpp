// MaterialTextures.cpp
#include "MaterialTextures.h"
#include <map>
#include <string>

using namespace Gdiplus;

// mapa estático de Material -> ruta relativa al ejecutable
static std::map<Material, std::wstring> s_matToPath = {
    { M_AIR, L"assets/materials/air.png" },
    // Eliminamos la textura específica de "grass": apuntamos grass al mismo recurso que dirt
    { M_GRASS, L"assets/materials/dirt.png" }, // <- grass usa la textura dirt para "quitar template grass"
    { M_SAND, L"assets/materials/sand.png" },
    { M_DIRT, L"assets/materials/dirt.png" },
    { M_GRAVEL, L"assets/materials/gravel.png" },
    { M_WATER, L"assets/materials/water.png" },
    { M_SPIKES, L"assets/materials/spikes.png" },
    { M_UNKNOWN, L"assets/materials/unknown.png" }
};

// mapa cache de Material -> Bitmap*
static std::map<Material, Bitmap*> s_loaded;

void InitMaterialTextures(TextureManager& tm) {
    s_loaded.clear();
    for (auto& kv : s_matToPath) {
        Material m = kv.first;
        const std::wstring& p = kv.second;
        Bitmap* bmp = tm.Load(p);
        // tm.Load siempre intenta crear un fallback si no encuentra la imagen,
        // por lo que bmp rara vez será nullptr. Igual comprobamos.
        s_loaded[m] = bmp;
    }
}

Gdiplus::Bitmap* GetMaterialTexture(Material m) {
    auto it = s_loaded.find(m);
    if (it == s_loaded.end()) return nullptr;
    return it->second;
}

const wchar_t* GetMaterialTexturePath(Material m) {
    auto it = s_matToPath.find(m);
    if (it == s_matToPath.end()) return L"";
    return it->second.c_str();
}
