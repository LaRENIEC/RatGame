#include "TextureManager.h"

using namespace Gdiplus;

TextureManager::TextureManager() : m_gdiplusToken(0) {}

TextureManager::~TextureManager() {
    for (auto& p : m_cache) {
        delete p.second;
    }
    m_cache.clear();

    if (m_gdiplusToken) {
        GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }
}

bool TextureManager::Init() {
    if (m_gdiplusToken) return true;
    GdiplusStartupInput gdiplusStartupInput;
    Status s = GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
    return s == Ok;
}

// reemplaza la implementación actual de Load(...) por esta
Gdiplus::Bitmap* TextureManager::Load(const std::wstring& path) {
    // si ya está cacheada, devolverla
    auto it = m_cache.find(path);
    if (it != m_cache.end()) return it->second;

    // 1) intento directo
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path.c_str());
    if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) {
        m_cache[path] = bmp;
        // si estaba marcada como missing y ahora existe, quitar del set
        m_missing.erase(path);
        return bmp;
    }
    delete bmp;

    // 2) si no fue absoluto, intentar con exe_dir + path (p. ej. assets\player\...)
    std::wstring full;
    if (!path.empty() && !(path.size() >= 2 && path[1] == L':') && path[0] != L'\\' && path[0] != L'/') {
        wchar_t modpath[MAX_PATH];
        DWORD n = GetModuleFileNameW(NULL, modpath, MAX_PATH);
        if (n > 0) {
            std::wstring exe(modpath);
            size_t pos = exe.find_last_of(L"\\/"); // buscar separador
            std::wstring dir = (pos == std::wstring::npos) ? std::wstring() : exe.substr(0, pos + 1);
            full = dir + path;
            Gdiplus::Bitmap* bmp2 = Gdiplus::Bitmap::FromFile(full.c_str());
            if (bmp2 && bmp2->GetLastStatus() == Gdiplus::Ok) {
                m_cache[path] = bmp2; // indexamos por la ruta original para que llamadas posteriores con la misma key la obtengan
                m_missing.erase(path);
                return bmp2;
            }
            delete bmp2;
        }
    }

    // Si llegamos aquí: la textura no existe físicamente, registrar como faltante
    std::wstring reportPath = !full.empty() ? full : path;
    if (!reportPath.empty()) m_missing.insert(reportPath);

    // 3) crear un fallback programático (no nullptr). Esto evita crashes / objetos nulos.
    const int FW = 64, FH = 64;
    Gdiplus::Bitmap* fb = new Gdiplus::Bitmap(FW, FH, PixelFormat32bppARGB);
    if (fb && fb->GetLastStatus() == Gdiplus::Ok) {
        Gdiplus::Graphics gr(fb);
        // fondo oscuro
        gr.Clear(Gdiplus::Color::MakeARGB(255, 60, 60, 60));
        // rectángulo central contrastado
        Gdiplus::SolidBrush br(Gdiplus::Color::MakeARGB(255, 200, 120, 60));
        gr.FillRectangle(&br, 8, 8, FW - 16, FH - 16);
        // cruz negra
        Gdiplus::Pen pen(Gdiplus::Color::MakeARGB(255, 20, 20, 20), 2.0f);
        gr.DrawLine(&pen, 8, 8, FW - 8, FH - 8);
        gr.DrawLine(&pen, FW - 8, 8, 8, FH - 8);

        // cachear usando la key original (path) para que calls con la misma key reciban
        m_cache[path] = fb;
        return fb;
    }

    // si por alguna razón no se pudo crear el fallback, devolver nullptr (muy improbable)
    delete fb;
    return nullptr;
}
bool TextureManager::ReportMissingTextures(HWND parent) {
    if (m_missing.empty()) return false;

    // construir mensaje (limitar longitud por seguridad)
    std::wstring msg = L"The following textures are missing:\r\n\r\n";
    int count = 0;
    const int MAX_LIST = 20;
    for (const auto& p : m_missing) {
        msg += p;
        msg += L"\r\n";
        if (++count >= MAX_LIST) {
            msg += L"... (more missing)\r\n";
            break;
        }
    }

    // mostrar MessageBox — advertencia antes de empezar
    MessageBoxW(parent, msg.c_str(), L"Missing Textures", MB_OK | MB_ICONWARNING);

    return true;
}

void TextureManager::Draw(Gdiplus::Bitmap* bmp, HDC hdc, int x, int y, int w, int h) {
    if (!bmp) return;
    Graphics g(hdc);
    int bw = bmp->GetWidth();
    int bh = bmp->GetHeight();
    if (w <= 0) w = bw;
    if (h <= 0) h = bh;
    g.DrawImage(bmp, x, y, w, h);
}

Gdiplus::Bitmap* TextureManager::Get(const std::wstring& key) {
    auto it = m_cache.find(key);
    if (it == m_cache.end()) return nullptr;
    return it->second;
}
