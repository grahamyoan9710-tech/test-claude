#include "game/fonts.hpp"

#include <vector>

extern "C" {
extern const unsigned char font_title_ttf[];
extern const unsigned int font_title_ttf_size;
extern const unsigned char font_ui_ttf[];
extern const unsigned int font_ui_ttf_size;
extern const unsigned char font_damage_ttf[];
extern const unsigned int font_damage_ttf_size;
extern const unsigned char font_display_ttf[];
extern const unsigned int font_display_ttf_size;
}

namespace {
constexpr int kFonts = 5;
Font g_fonts[kFonts];
bool g_loaded[kFonts] = {};

std::vector<int> frenchCodepoints() {
  std::vector<int> cps;
  for (int c = 32; c < 127; ++c) cps.push_back(c);
  for (int c = 160; c < 256; ++c) cps.push_back(c);  // Latin-1: é è à ç ô « » ...
  for (int c : {0x152, 0x153, 0x2013, 0x2014, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x20AC, 0x2212}) cps.push_back(c);
  for (int c : {0x14C, 0x14D, 0x16A, 0x16B}) cps.push_back(c);  // Ō ō Ū ū (Tenshō, Bijūdama...)
  for (int c = 0x2190; c <= 0x2193; ++c) cps.push_back(c);      // ← ↑ → ↓ (key caps)
  return cps;
}

Font loadEmbedded(const unsigned char* data, unsigned int size, int baseSize) {
  std::vector<int> cps = frenchCodepoints();
  Font f = LoadFontFromMemory(".ttf", data, int(size), baseSize, cps.data(), int(cps.size()));
  if (f.texture.id == 0) return GetFontDefault();
  GenTextureMipmaps(&f.texture);  // smooth when drawn small
  SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
  return f;
}
}  // namespace

void loadFonts() {
  g_fonts[0] = loadEmbedded(font_ui_ttf, font_ui_ttf_size, 72);
  g_fonts[1] = loadEmbedded(font_title_ttf, font_title_ttf_size, 160);
  g_fonts[2] = loadEmbedded(font_damage_ttf, font_damage_ttf_size, 160);
  g_fonts[3] = loadEmbedded(font_display_ttf, font_display_ttf_size, 160);
  g_fonts[4] = loadEmbedded(font_ui_ttf, font_ui_ttf_size, 96);  // Inter, larger: the « Épure » texts
  for (int i = 0; i < kFonts; ++i) g_loaded[i] = g_fonts[i].texture.id != GetFontDefault().texture.id;
}

void unloadFonts() {
  for (int i = 0; i < kFonts; ++i) {
    if (g_loaded[i]) UnloadFont(g_fonts[i]);
    g_loaded[i] = false;
  }
}

const Font& gameFont(FontKind kind) {
  const int i = int(kind);
  if (!g_loaded[i] && g_fonts[i].texture.id == 0) g_fonts[i] = GetFontDefault();
  return g_fonts[i];
}
