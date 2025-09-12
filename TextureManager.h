#pragma once
#include <string>
#include <unordered_map>
#include <windows.h>
#include <gdiplus.h>
#include <set>

class TextureManager {
public:
    TextureManager();
    ~TextureManager();

    // Inicializar GDI+ (llamar desde wWinMain)
    bool Init();

    // Cargar imagen PNG (si ya está cargada, devuelve cached)
    Gdiplus::Bitmap* Load(const std::wstring& path);

    // Dibujar (si bitmap no existe, no hace nada)
    void Draw(Gdiplus::Bitmap* bmp, HDC hdc, int x, int y, int w = -1, int h = -1);

    // Convenience: buscar por key
    Gdiplus::Bitmap* Get(const std::wstring& key);

    bool ReportMissingTextures(HWND parent);

private:
    ULONG_PTR m_gdiplusToken;
    std::set<std::wstring> m_missing;
    std::unordered_map<std::wstring, Gdiplus::Bitmap*> m_cache;
};
