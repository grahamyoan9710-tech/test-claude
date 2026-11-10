// The arenas' "RTX" look, with "Qualité graphique : Haute" only: the ParticleFX effects light the backdrop and the
// platforms (their light pools spread on the floors in depth), the fighters cast a shadow away from the effects, and
// the glossy floors mirror the sky, the fighters and the effects (screen-space reflections).
// The shaders work in framebuffer pixels (gl_FragCoord: origin at the bottom left, y up).
#include <algorithm>
#include <cmath>

#include "game/fighter_art.hpp"
#include "game/pixel.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/stage_art.hpp"
#include "game/ui.hpp"
#include "rlgl.h"

namespace light {
namespace {
constexpr int kMaxLights = 16;
constexpr float kReach = 200.0f;  // how far an effect's light falls past its bright pixels (world units)
constexpr float kDepth = 4.0f;    // depth of a floor per unit of its height on screen

// Light added to what is under it: drawn with the blend dst * (1 + src), so it brightens the surfaces in their own
// colour (a dark floor stays dark, the sky behind an effect barely moves) without washing them to white.
const char* kLitFrag = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 lightA[16];  // centre (framebuffer px), core radius, reach
uniform vec4 lightB[16];  // colour, strength
uniform int count;
uniform vec4 floorY;      // back edge, walk line, front edge (framebuffer y), depth per px of the floor
uniform vec2 gain;        // on the floor, elsewhere
out vec4 finalColor;
void main() {
  float a = texture(texture0, fragTexCoord).a * fragColor.a;
  vec2 p = gl_FragCoord.xy;
  bool onFloor = p.y <= floorY.x && p.y >= floorY.z;
  vec3 L = vec3(0.0);
  for (int i = 0; i < count; ++i) {
    vec2 d = p - lightA[i].xy;
    // A point of the floor is lit from the light's height above it, at its depth: the pool is an ellipse.
    float dist = onFloor ? length(vec3(d.x, max(lightA[i].y - floorY.y, 0.0), (floorY.y - p.y) * floorY.w)) : length(d);
    float f = clamp(1.0 - (dist - lightA[i].z) / lightA[i].w, 0.0, 1.0);
    L += lightB[i].rgb * (lightB[i].a * f * f);
  }
  finalColor = vec4(L * ((onFloor ? gain.x : gain.y) * a), a);
}
)";

// A glossy floor mirrors the scene: the backdrop (the frame before the fighters) about the floor's back edge, and
// over it, near the fighters' feet, the fighters and the effects about the walk line (where they stand), squashed
// more toward the front edge so what stands high still shows there. A little blurred, fading toward the front; rain
// ripples it. Added to the floor, squared: what is bright (an effect, a lantern, a white coat) shines in it, the dark
// barely shows. Drawn after the fighters, only where the floor still shows, with a screen blend: a bright floor
// takes less of it than a dark one, so it is never washed out.
const char* kMirrorFrag = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;  // the frame, all drawn
uniform sampler2D backdrop;  // the frame before the fighters
uniform vec4 cur;            // framebuffer px per world unit, camera target, framebuffer height
uniform vec2 offs;           // camera offset in framebuffer px
uniform vec4 floorW;         // back edge, walk line, front edge (world y), gloss
uniform vec2 size;
uniform vec2 rain;           // ripple size (world units), time
uniform vec4 ends;           // the floor's left and right ends at its back edge, then at its front edge (world x)
out vec4 finalColor;
vec2 px(float x, float y) { return vec2((x - cur.y) * cur.x + offs.x, cur.w - ((y - cur.z) * cur.x + offs.y)); }
vec3 tap(sampler2D t, vec2 s) { return texture(t, clamp(s, vec2(0.5), size - 0.5) / size).rgb; }
void main() {
  vec2 g = gl_FragCoord.xy;
  // Something drawn over the floor here (a foot, an effect...), not a shadow (the floor, only darker).
  vec3 here = tap(texture0, g), was = tap(backdrop, g);
  float dim = dot(here, was) / max(dot(was, was), 1e-4);
  float covered = max(smoothstep(0.03, 0.08, length(here - was * min(dim, 1.0))), smoothstep(1.03, 1.12, dim));
  vec2 w = vec2((g.x - offs.x) / cur.x + cur.y, (cur.w - g.y - offs.y) / cur.x + cur.z);
  float back = w.y - floorW.x, front = max(w.y - floorW.y, 0.0);
  float t = clamp(back / (floorW.z - floorW.x), 0.0, 1.0), u = clamp(front / (floorW.z - floorW.y), 0.0, 1.0);
  float x = w.x + rain.x * sin(w.y * 1.7 + rain.y * 5.0) * (0.3 + t);
  float ySky = floorW.x - back * (1.6 + 2.4 * t), yNear = floorW.y - front * (1.2 + 2.0 * u);
  vec3 sky = vec3(0.0), near = vec3(0.0);
  float hit = 0.0;
  for (int k = -1; k <= 1; ++k) {
    vec3 a = tap(backdrop, px(x + float(k) * (0.4 + 2.0 * t), ySky));
    vec2 q = px(x + float(k) * (0.3 + 2.0 * u), yNear);
    vec3 b = tap(texture0, q), o = tap(backdrop, q);
    sky += a * a;
    near += pow(b, vec3(1.5));
    hit += smoothstep(0.02, 0.08, length(b - o));  // a fighter or an effect stands there
  }
  sky *= pow(1.0 - t, 0.7) / 3.0;
  near *= (1.0 - 0.6 * u) / 3.0;
  vec3 c = mix(sky, near, (hit / 3.0) * step(0.0, w.y - floorW.y + 2.0));
  // Only on the floor: its ends (rails, roof ridges...) do not mirror.
  vec2 e = mix(ends.xy, ends.zw, t);
  float inside = smoothstep(e.x, e.x + 3.0, w.x) * smoothstep(e.y, e.y - 3.0, w.x);
  finalColor = vec4(c * (floorW.w * (1.0 - covered) * inside * fragColor.a), 1.0);
}
)";

Shader g_lit{}, g_mirror{};
bool g_tried = false;
int g_litA = -1, g_litB = -1, g_litCount = -1, g_litFloor = -1, g_litGain = -1;
int g_mCur = -1, g_mOffs = -1, g_mFloor = -1, g_mSize = -1, g_mRain = -1, g_mBack = -1, g_mEnds = -1;
RenderTexture2D g_now{}, g_back{};  // the frame all drawn, the frame before the fighters
bool g_hasBack = false;

bool ready() {
  if (!g_tried) {
    g_tried = true;
    g_lit = LoadShaderFromMemory(nullptr, kLitFrag);
    g_mirror = LoadShaderFromMemory(nullptr, kMirrorFrag);
    g_litA = GetShaderLocation(g_lit, "lightA");
    g_litB = GetShaderLocation(g_lit, "lightB");
    g_litCount = GetShaderLocation(g_lit, "count");
    g_litFloor = GetShaderLocation(g_lit, "floorY");
    g_litGain = GetShaderLocation(g_lit, "gain");
    g_mCur = GetShaderLocation(g_mirror, "cur");
    g_mOffs = GetShaderLocation(g_mirror, "offs");
    g_mFloor = GetShaderLocation(g_mirror, "floorW");
    g_mSize = GetShaderLocation(g_mirror, "size");
    g_mRain = GetShaderLocation(g_mirror, "rain");
    g_mEnds = GetShaderLocation(g_mirror, "ends");
    g_mBack = GetShaderLocation(g_mirror, "backdrop");
  }
  return g_lit.id != 0 && g_mirror.id != 0 && g_litA >= 0 && g_mCur >= 0;
}

// The camera as the frame draws it (beginWorld): framebuffer px per world unit, offset in framebuffer px.
struct View {
  float z, tx, ty, ox, oy, H;
  Vector2 gl(Vec2 w) const { return {(w.x - tx) * z + ox, H - ((w.y - ty) * z + oy)}; }
};
View viewOf(const Camera2D& cam, const RenderTexture2D& frame) {
  return {cam.zoom * g_renderScale, cam.target.x, cam.target.y, cam.offset.x * g_renderScale,
          cam.offset.y * g_renderScale, float(frame.texture.height)};
}

// Copies the frame into `to` (made at its size if needed: loading it unbinds the frame, bound again after).
void copyFrame(const RenderTexture2D& frame, RenderTexture2D& to) {
  rlDrawRenderBatchActive();
  if (to.id == 0 || to.texture.width != frame.texture.width || to.texture.height != frame.texture.height) {
    if (to.id != 0) UnloadRenderTexture(to);
    to = LoadRenderTexture(frame.texture.width, frame.texture.height);
    SetTextureFilter(to.texture, TEXTURE_FILTER_BILINEAR);
  }
  rlBindFramebuffer(RL_READ_FRAMEBUFFER, frame.id);
  rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, to.id);
  rlBlitFramebuffer(0, 0, frame.texture.width, frame.texture.height, 0, 0, frame.texture.width, frame.texture.height,
                    0x00004000);  // GL_COLOR_BUFFER_BIT
  rlEnableFramebuffer(frame.id);
}

struct Lights {
  int n = 0;
  float a[kMaxLights * 4], b[kMaxLights * 4];
};
// The strongest lights of the frame before that can reach the screen, in framebuffer px.
Lights pick(const View& v, const RenderTexture2D& frame, float reach) {
  std::vector<art::Light> all = art::lastLights();
  std::sort(all.begin(), all.end(), [](const art::Light& p, const art::Light& q) { return p.k > q.k; });
  Lights out;
  const float W = float(frame.texture.width);
  for (const art::Light& L : all) {
    if (out.n == kMaxLights || L.k < 0.02f) break;
    const Vector2 c = v.gl(L.at);
    const float r = L.r * v.z, R = reach * v.z;
    if (c.x < -r - R || c.x > W + r + R || c.y < -r - R || c.y > v.H + r + R) continue;
    const float A[4] = {c.x, c.y, r, R}, B[4] = {L.c.r / 255.0f, L.c.g / 255.0f, L.c.b / 255.0f, L.k};
    std::copy(A, A + 4, out.a + out.n * 4);
    std::copy(B, B + 4, out.b + out.n * 4);
    ++out.n;
  }
  return out;
}

void beginLit(const Lights& L) {
  BeginShaderMode(g_lit);
  SetShaderValueV(g_lit, g_litA, L.a, SHADER_UNIFORM_VEC4, L.n);
  SetShaderValueV(g_lit, g_litB, L.b, SHADER_UNIFORM_VEC4, L.n);
  SetShaderValue(g_lit, g_litCount, &L.n, SHADER_UNIFORM_INT);
  rlSetBlendFactors(RL_DST_COLOR, RL_ONE, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM);
}
void endLit() {
  EndShaderMode();
  ui::frameBlend();
}

// The fighter's shadow on its floor, away from each effect that lights it: short under a light above it, long when
// the light is low beside it.
void shadows(const Match& m, const Camera2D& cam) {
  const auto& lights = art::lastLights();
  if (lights.empty()) return;
  const float gain = stageart::lightGain(m.stage().theme);  // the effects' shadows show more at night
  for (const auto& f : m.fighters) {
    if (!f.active() || f.state == FState::LedgeHang || f.state == FState::Respawn) continue;
    const Platform* on = nullptr;
    for (const auto& p : m.platforms)
      if (f.pos.x >= p.rect.left() - 4.0f && f.pos.x <= p.rect.right() + 4.0f && p.rect.top() >= f.pos.y - 4.0f &&
          (!on || p.rect.top() < on->rect.top()))
        on = &p;
    stageart::Floor fl{};
    if (!on || !stageart::floorOf(*on, m.stage().theme, fl)) continue;
    const float H = f.height(), lift = fl.walk - f.pos.y, near = 1.0f - clampf(lift / 300.0f, 0.0f, 1.0f);
    if (near <= 0.0f) continue;
    // Only on this floor: clipped to its band.
    const Vector2 a = GetWorldToScreen2D({on->rect.left(), fl.back}, cam), b = GetWorldToScreen2D({on->rect.right(), fl.front}, cam);
    rlDrawRenderBatchActive();
    beginClip({a.x, a.y, b.x - a.x, b.y - a.y});
    for (const art::Light& L : lights) {
      const Vec2 mid = f.center(), d = mid - L.at;
      const float k = gain * L.k * near * clampf(1.0f - (d.len() - L.r) / H, 0.0f, 1.0f);
      if (k < 0.05f) continue;
      const float above = std::max(fl.walk - L.at.y, 0.3f * H), len = H * clampf(std::fabs(d.x) / above, 0.25f, 1.8f);
      const float dir = d.x >= 0.0f ? 1.0f : -1.0f, rx = 0.5f * len + 0.3f * f.width(), ry = 0.45f * (fl.front - fl.walk);
      const Color c{16, 10, 24, (unsigned char)(255.0f * std::min(0.6f, 0.55f * k))};
      rlPushMatrix();
      rlTranslatef(f.pos.x + dir * 0.5f * len, fl.walk + 0.15f * (fl.front - fl.walk), 0.0f);
      rlScalef(1.0f, ry / rx, 1.0f);
      vfx::glow({0.0f, 0.0f}, rx, c, withAlpha(c, 0.0f));
      rlPopMatrix();
    }
    rlDrawRenderBatchActive();
    endClip();
  }
}
}  // namespace

void backdrop(const Match& m, const Camera2D& cam) {
  const RenderTexture2D* fr = pix::frameTarget();
  if (!settings().highQuality || !fr || !ready()) return;
  const View v = viewOf(cam, *fr);
  const Lights L = pick(v, *fr, 2.0f * kReach);
  if (L.n == 0) return;
  rlDrawRenderBatchActive();
  beginLit(L);
  const float none[4] = {-1.0f, -1.0f, -2.0f, 0.0f}, gain[2] = {0.0f, 0.2f * stageart::lightGain(m.stage().theme)};
  SetShaderValue(g_lit, g_litFloor, none, SHADER_UNIFORM_VEC4);
  SetShaderValue(g_lit, g_litGain, gain, SHADER_UNIFORM_VEC2);
  const Rect r = vfx::viewRect(20.0f);
  DrawRectangleRec({r.x, r.y, r.w, r.h}, WHITE);
  rlDrawRenderBatchActive();
  endLit();
}

void arena(const Match& m, const Camera2D& cam, float time) {
  const RenderTexture2D* fr = pix::frameTarget();
  if (!settings().highQuality || !fr || !ready()) return;
  const View v = viewOf(cam, *fr);
  const StageTheme theme = m.stage().theme;

  // 1. The effects light the platforms: each drawn again, its picture masking the light.
  const Lights L = pick(v, *fr, kReach);
  if (L.n > 0) {
    rlDrawRenderBatchActive();
    beginLit(L);
    const float g = stageart::lightGain(theme), gain[2] = {1.4f * g, 0.45f * g};
    SetShaderValue(g_lit, g_litGain, gain, SHADER_UNIFORM_VEC2);
    for (const auto& p : m.platforms) {
      stageart::Floor fl{};
      float y[4] = {-1.0f, -1.0f, -2.0f, 0.0f};
      if (stageart::floorOf(p, theme, fl)) {
        y[0] = v.gl({0.0f, fl.back}).y, y[1] = v.gl({0.0f, fl.walk}).y, y[2] = v.gl({0.0f, fl.front}).y;
        y[3] = kDepth;
      }
      SetShaderValue(g_lit, g_litFloor, y, SHADER_UNIFORM_VEC4);
      stageart::drawPlatform(p, theme, time);
      rlDrawRenderBatchActive();
    }
    endLit();
  }

  // 2. The fighters' shadows, away from the effects.
  shadows(m, cam);

  // 3. What the floors mirror behind the fighters, and what tells them from the floor.
  g_hasBack = false;
  for (const auto& p : m.platforms) {
    stageart::Floor fl{};
    if (stageart::floorOf(p, theme, fl) && fl.gloss > 0.0f) {
      copyFrame(*fr, g_back);
      g_hasBack = true;
      break;
    }
  }
}

void reflections(const Match& m, const Camera2D& cam, float time) {
  const RenderTexture2D* fr = pix::frameTarget();
  if (!settings().highQuality || !fr || !g_hasBack || !ready() || g_back.texture.width != fr->texture.width ||
      g_back.texture.height != fr->texture.height)
    return;
  g_hasBack = false;
  copyFrame(*fr, g_now);
  const View v = viewOf(cam, *fr);
  BeginShaderMode(g_mirror);
  rlSetBlendFactors(RL_ONE_MINUS_DST_COLOR, RL_ONE, RL_FUNC_ADD);  // screen
  BeginBlendMode(BLEND_CUSTOM);
  const float curU[4] = {v.z, v.tx, v.ty, v.H}, offs[2] = {v.ox, v.oy};
  const float size[2] = {float(fr->texture.width), float(fr->texture.height)};
  SetShaderValue(g_mirror, g_mCur, curU, SHADER_UNIFORM_VEC4);
  SetShaderValue(g_mirror, g_mOffs, offs, SHADER_UNIFORM_VEC2);
  SetShaderValue(g_mirror, g_mSize, size, SHADER_UNIFORM_VEC2);
  for (const auto& p : m.platforms) {
    stageart::Floor fl{};
    if (!stageart::floorOf(p, m.stage().theme, fl) || fl.gloss <= 0.0f) continue;
    const float f[4] = {fl.back, fl.walk, fl.front, fl.gloss}, rain[2] = {fl.rain ? 1.2f : 0.0f, time};
    SetShaderValue(g_mirror, g_mFloor, f, SHADER_UNIFORM_VEC4);
    SetShaderValue(g_mirror, g_mRain, rain, SHADER_UNIFORM_VEC2);
    const float ends[4] = {fl.leftBack, fl.rightBack, fl.leftFront, fl.rightFront};
    SetShaderValue(g_mirror, g_mEnds, ends, SHADER_UNIFORM_VEC4);
    SetShaderValueTexture(g_mirror, g_mBack, g_back.texture);  // (each draw of the batch forgets it)
    DrawTexturePro(g_now.texture, {0, 0, 1, 1}, {p.rect.left() + 2.0f, fl.back, p.rect.w - 4.0f, fl.front - fl.back},
                   {0, 0}, 0.0f, WHITE);
    rlDrawRenderBatchActive();
  }
  EndShaderMode();
  ui::frameBlend();
}
}  // namespace light
