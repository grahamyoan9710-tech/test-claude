// Pixel-art characters (see pixel.hpp). The sprite is rendered on the fly into a screen-sized low-resolution
// layer: the current transform (camera, supersampling) is kept and simply scaled down, so a character
// lands in the layer exactly where it would have been drawn, then the shader pass and the nearest-neighbour
// blit put it back in the frame. Raylib cannot nest texture modes, so the framebuffers and matrices are
// switched by hand (rlgl) and restored afterwards.
#include "game/pixel.hpp"

#include "game/settings.hpp"
#include "game/ui.hpp"
#include "raymath.h"

namespace pix {
namespace {
constexpr int kW = kScreenW / kArtPixel, kH = kScreenH / kArtPixel;

const RenderTexture2D* g_frame = nullptr;
RenderTexture2D g_raw{}, g_out{};
Shader g_shader{};
int g_texelLoc = -1, g_inkLoc = -1, g_lightLoc = -1;
bool g_ready = false;
float g_artUnit = 0.0f;

// The sprite look, on the low-resolution layer (premultiplied colours):
// - a one-pixel ink outline around the body (the classic dark sprite outline),
// - flat colour bands: the soft gradients of the HD drawing are quantised,
// - a pixel of highlight on the edges facing the key light (upper left), a pixel of shade on the others.
const char* kFrag = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec2 texel;
uniform vec3 ink;
uniform vec2 light;
out vec4 finalColor;
vec4 at(vec2 o) { return texture(texture0, fragTexCoord + o * texel); }
void main() {
  vec4 c = at(vec2(0.0));
  float l = at(vec2(-1.0, 0.0)).a, r = at(vec2(1.0, 0.0)).a, u = at(vec2(0.0, 1.0)).a, d = at(vec2(0.0, -1.0)).a;
  float n = max(max(l, r), max(u, d));
  if (c.a < 0.5) {
    finalColor = n >= 0.5 ? vec4(ink, 1.0) : c;
    return;
  }
  vec3 rgb = c.rgb / c.a;
  rgb = floor(rgb * 20.0 + 0.5) / 20.0;
  finalColor = vec4(rgb, 1.0);
}
)";

bool ready() {
  if (g_ready) return g_raw.id != 0 && g_out.id != 0;
  g_ready = true;
  g_raw = LoadRenderTexture(kW, kH);
  g_out = LoadRenderTexture(kW, kH);
  SetTextureFilter(g_raw.texture, TEXTURE_FILTER_POINT);
  SetTextureFilter(g_out.texture, TEXTURE_FILTER_POINT);
  g_shader = LoadShaderFromMemory(nullptr, kFrag);
  g_texelLoc = GetShaderLocation(g_shader, "texel");
  g_inkLoc = GetShaderLocation(g_shader, "ink");
  g_lightLoc = GetShaderLocation(g_shader, "light");
  return g_raw.id != 0 && g_out.id != 0;
}

void premultipliedBlend() {
  rlSetBlendFactorsSeparate(RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM_SEPARATE);
}

// Sub-rectangle of the layer, as a source rectangle of a render texture (stored upside down).
Rectangle layerSrc(int x, int y, int w, int h) { return {float(x), float(kH - y - h), float(w), -float(h)}; }
}  // namespace

bool enabled() { return settings().pixelArt; }

float artUnit() { return g_artUnit; }

void setFrameTarget(const RenderTexture2D* target) { g_frame = target; }

void sprite(Rect b, float alpha, const std::function<void(float)>& draw) {
  if (alpha <= 0.0f) return;
  if (!enabled() || !g_frame || !ready()) {
    draw(alpha);
    return;
  }
  rlDrawRenderBatchActive();
  const Matrix mv = rlGetMatrixModelview(), proj = rlGetMatrixProjection();
  // Bounds in layer pixels (the current transform gives frame pixels).
  const float k = 1.0f / (g_renderScale * kArtPixel);
  float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
  for (Vector2 p : {Vector2{b.x, b.y}, Vector2{b.right(), b.y}, Vector2{b.x, b.bottom()}, Vector2{b.right(), b.bottom()}}) {
    const Vector2 q = Vector2Transform(p, mv);
    x0 = std::min(x0, q.x * k);
    y0 = std::min(y0, q.y * k);
    x1 = std::max(x1, q.x * k);
    y1 = std::max(y1, q.y * k);
  }
  const int ix0 = std::max(0, int(std::floor(x0)) - 2), iy0 = std::max(0, int(std::floor(y0)) - 2);
  const int ix1 = std::min(kW, int(std::ceil(x1)) + 2), iy1 = std::min(kH, int(std::ceil(y1)) + 2);
  if (ix1 <= ix0 || iy1 <= iy0) return;  // off screen
  const int w = ix1 - ix0, h = iy1 - iy0;

  // 1. The character, in the low-resolution layer.
  rlEnableFramebuffer(g_raw.id);
  rlViewport(0, 0, kW, kH);
  rlSetMatrixProjection(MatrixOrtho(0, kW, kH, 0, 0, 1));
  rlSetMatrixModelview(MatrixMultiply(mv, MatrixScale(k, k, 1.0f)));
  rlClearColor(0, 0, 0, 0);
  rlClearScreenBuffers();
  ui::frameBlend();  // "over" for alpha too: the layer ends up premultiplied
  const float unitsToPx = std::sqrt(std::fabs(mv.m0 * mv.m5 - mv.m1 * mv.m4)) * k;
  g_artUnit = unitsToPx > 0.0f ? 1.0f / unitsToPx : 0.0f;
  draw(1.0f);
  g_artUnit = 0.0f;
  rlDrawRenderBatchActive();

  // 2. The sprite look.
  rlEnableFramebuffer(g_out.id);
  rlClearScreenBuffers();
  rlSetMatrixModelview(MatrixIdentity());
  premultipliedBlend();
  BeginShaderMode(g_shader);
  const float texel[2] = {1.0f / kW, 1.0f / kH}, light[2] = {-1.0f, -1.0f};
  const float ink[3] = {kInk.r / 255.0f, kInk.g / 255.0f, kInk.b / 255.0f};
  SetShaderValue(g_shader, g_texelLoc, texel, SHADER_UNIFORM_VEC2);
  SetShaderValue(g_shader, g_inkLoc, ink, SHADER_UNIFORM_VEC3);
  SetShaderValue(g_shader, g_lightLoc, light, SHADER_UNIFORM_VEC2);
  DrawTexturePro(g_raw.texture, layerSrc(ix0, iy0, w, h), {float(ix0), float(iy0), float(w), float(h)}, {0, 0}, 0.0f,
                 WHITE);
  EndShaderMode();
  rlDrawRenderBatchActive();

  // 3. Back into the frame, each art pixel a square of kArtPixel virtual pixels.
  rlEnableFramebuffer(g_frame->id);
  rlViewport(0, 0, g_frame->texture.width, g_frame->texture.height);
  rlSetMatrixProjection(proj);
  const float s = g_renderScale * kArtPixel;
  rlSetMatrixModelview(MatrixScale(s, s, 1.0f));
  const unsigned char a = (unsigned char)clampf(alpha * 255.0f, 0.0f, 255.0f);
  DrawTexturePro(g_out.texture, layerSrc(ix0, iy0, w, h), {float(ix0), float(iy0), float(w), float(h)}, {0, 0}, 0.0f,
                 {a, a, a, a});
  rlDrawRenderBatchActive();
  rlSetMatrixModelview(mv);
  ui::frameBlend();
}

void unload() {
  if (g_raw.id) UnloadRenderTexture(g_raw);
  if (g_out.id) UnloadRenderTexture(g_out);
  if (g_shader.id) UnloadShader(g_shader);
  g_raw = g_out = RenderTexture2D{};
  g_shader = Shader{};
  g_ready = false;
}
}  // namespace pix
