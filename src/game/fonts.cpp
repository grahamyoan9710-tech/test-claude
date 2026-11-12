#include "game/fonts.hpp"

#include <algorithm>
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
constexpr int kBigBase = 128;  // px of the distance fields (raylib's: 4 px of padding, 64 per px: +-2 px of outline)
Font g_big[3];  // Title, Display, Damage
Shader g_sdf{};
int g_strokeLoc = -1, g_strokeColLoc = -1, g_hollowLoc = -1, g_clipLoc = -1, g_frameLoc = -1;
float g_clip[4] = {0, 0, 0, 0}, g_frame[2] = {720, 1};
const char* kSdfFrag = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform float stroke;
uniform vec4 strokeCol;
uniform float hollow;
uniform vec4 clip;   // w > 0: keep where x * clip.x + y * clip.y <= clip.z (screen units, y down)
uniform vec2 frame;  // the target's height in pixels, its pixels per screen unit
out vec4 finalColor;
void main() {
  if (clip.w > 0.5) {
    vec2 p = vec2(gl_FragCoord.x, frame.x - gl_FragCoord.y) / frame.y;
    if (dot(clip.xy, p) > clip.z) discard;
  }
  float d = texture(texture0, fragTexCoord).a - 0.5;
  float aa = max(length(vec2(dFdx(d), dFdy(d))), 1e-4);
  float outer = smoothstep(-aa, aa, d + stroke), inner = smoothstep(-aa, aa, d - stroke);
  if (hollow > 0.5) {
    finalColor = vec4(fragColor.rgb, fragColor.a * (outer - inner));
  } else if (stroke <= 0.0) {
    finalColor = vec4(fragColor.rgb, fragColor.a * smoothstep(-aa, aa, d));
  } else {
    finalColor = vec4(mix(strokeCol.rgb, fragColor.rgb, inner), mix(strokeCol.a, 1.0, inner) * outer * fragColor.a);
  }
}
)";

std::vector<int> frenchCodepoints() {
  std::vector<int> cps;
  for (int c = 32; c < 127; ++c) cps.push_back(c);
  for (int c = 160; c < 256; ++c) cps.push_back(c);  // Latin-1: é è à ç ô « » ...
  for (int c : {0x152, 0x153, 0x2013, 0x2014, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x20AC, 0x2212}) cps.push_back(c);
  for (int c : {0x14C, 0x14D, 0x16A, 0x16B}) cps.push_back(c);  // Ō ō Ū ū (Tenshō, Bijūdama...)
  for (int c = 0x2190; c <= 0x2193; ++c) cps.push_back(c);      // ← ↑ → ↓ (key caps)
  return cps;
}

// The big type is all capitals: their letters, accents, figures and signs.
std::vector<int> bigCodepoints() {
  std::vector<int> cps;
  for (int c = 32; c < 97; ++c) cps.push_back(c);
  for (int c : {0xAB, 0xB0, 0xB7, 0xBB, 0xC0, 0xC2, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCE, 0xCF, 0xD4, 0xD9, 0xDB, 0xDC, 0x152, 0x14C,
                0x16A, 0x2019, 0x2013, 0x2026})
    cps.push_back(c);
  return cps;
}

Font loadBig(const unsigned char* data, unsigned int size) {
  std::vector<int> cps = bigCodepoints();
  Font f{};
  f.baseSize = kBigBase;
  f.glyphCount = int(cps.size());
  f.glyphs = LoadFontData(data, int(size), kBigBase, cps.data(), f.glyphCount, FONT_SDF);
  if (!f.glyphs) return Font{};
  Image atlas = GenImageFontAtlas(f.glyphs, &f.recs, f.glyphCount, kBigBase, 0, 1);
  f.texture = LoadTextureFromImage(atlas);
  UnloadImage(atlas);
  SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
  return f;
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
  g_big[0] = loadBig(font_title_ttf, font_title_ttf_size);
  g_big[1] = loadBig(font_display_ttf, font_display_ttf_size);
  g_big[2] = loadBig(font_damage_ttf, font_damage_ttf_size);
  g_sdf = LoadShaderFromMemory(nullptr, kSdfFrag);
  g_strokeLoc = GetShaderLocation(g_sdf, "stroke");
  g_strokeColLoc = GetShaderLocation(g_sdf, "strokeCol");
  g_hollowLoc = GetShaderLocation(g_sdf, "hollow");
  g_clipLoc = GetShaderLocation(g_sdf, "clip");
  g_frameLoc = GetShaderLocation(g_sdf, "frame");
}

void unloadFonts() {
  for (int i = 0; i < kFonts; ++i) {
    if (g_loaded[i]) UnloadFont(g_fonts[i]);
    g_loaded[i] = false;
  }
  for (Font& f : g_big)
    if (f.texture.id != 0) UnloadFont(f);
  if (g_sdf.id != 0) UnloadShader(g_sdf);
}

const Font& bigFont(FontKind kind) {
  const Font& f = g_big[kind == FontKind::Display ? 1 : kind == FontKind::Damage ? 2 : 0];
  return f.texture.id != 0 ? f : gameFont(kind);
}

float bigFieldPerPx(float size) { return 64.0f / 255.0f * kBigBase / std::max(1.0f, size); }

void beginBig(float stroke, Color strokeCol, bool hollow) {
  if (g_sdf.id == 0) return;
  BeginShaderMode(g_sdf);
  const float sc[4] = {strokeCol.r / 255.0f, strokeCol.g / 255.0f, strokeCol.b / 255.0f, strokeCol.a / 255.0f};
  const float h = hollow ? 1.0f : 0.0f;
  SetShaderValue(g_sdf, g_strokeLoc, &stroke, SHADER_UNIFORM_FLOAT);
  SetShaderValue(g_sdf, g_strokeColLoc, sc, SHADER_UNIFORM_VEC4);
  SetShaderValue(g_sdf, g_hollowLoc, &h, SHADER_UNIFORM_FLOAT);
  SetShaderValue(g_sdf, g_clipLoc, g_clip, SHADER_UNIFORM_VEC4);
  SetShaderValue(g_sdf, g_frameLoc, g_frame, SHADER_UNIFORM_VEC2);
}

void setBigClip(bool on, float a, float b, float c, float targetH, float scale) {
  g_clip[0] = a, g_clip[1] = b, g_clip[2] = c, g_clip[3] = on ? 1.0f : 0.0f;
  g_frame[0] = targetH, g_frame[1] = scale;
}

void endBig() {
  if (g_sdf.id != 0) EndShaderMode();
}

const Font& gameFont(FontKind kind) {
  const int i = int(kind);
  if (!g_loaded[i] && g_fonts[i].texture.id == 0) g_fonts[i] = GetFontDefault();
  return g_fonts[i];
}
