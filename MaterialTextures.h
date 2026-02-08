// MaterialTextures.h
#pragma once

#include "Tile.h"
#include "TextureManager.h"
#include <gdiplus.h>

// Inicializa (carga) las texturas de materiales usando el TextureManager.
// Llamar una vez después de g_texMgr.Init().
void InitMaterialTextures(TextureManager& tm);

// Devuelve el bitmap (puede ser nullptr si no existe).
Gdiplus::Bitmap* GetMaterialTexture(Material m);

// Devuelve el path asociado (usa para debugging si lo necesitas)
const wchar_t* GetMaterialTexturePath(Material m);
