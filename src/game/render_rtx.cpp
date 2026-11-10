// The real "RTX" (rtx.hpp): 2D ray traced light. The traced scene is small (kW x kH): what gives light (the effects,
// additive), what blocks it (the mask, at the frame's size so its edges are the sprites' own: alpha the solids, green
// the floors that catch the light). The shaders work in pixels of their target (gl_FragCoord), every target the
// frame's way up, so the same uv reads the same place in all of them.
#include "game/rtx.hpp"

#include <algorithm>
#include <cmath>

#include "game/fighter_art.hpp"
#include "game/pixel.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/stage_art.hpp"
#include "game/ui.hpp"
#include "raymath.h"

namespace rtx {
namespace {
constexpr int kW = kScreenW / 2, kH = kScreenH / 2;  // the traced scene: 2 virtual pixels per pixel

// Mask draws: the picture's coverage only (the colour mask picks the channel).
const char* kMaskFrag = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
out vec4 finalColor;
void main() { finalColor = vec4(1.0, 1.0, 1.0, texture(texture0, fragTexCoord).a * fragColor.a); }
)";

// The arena's own lights, read off its pictures: bright, saturated pixels (lanterns, a red moon, glowing crystals)
// give light too, as much as `lamps` (0 in daylight, where the sky would light everything).
#define LAMP_FN \
  "uniform sampler2D albedoT;\n" \
  "uniform float lamps;\n" \
  "vec3 lamp(vec2 uv) {\n" \
  "  vec3 c = texture(albedoT, uv).rgb;\n" \
  "  float m = max(c.r, max(c.g, c.b)), n = min(c.r, min(c.g, c.b));\n" \
  "  return c * lamps * smoothstep(0.72, 0.95, m) * smoothstep(0.35, 0.6, (m - n) / max(m, 1e-3));\n" \
  "}\n"

// Jump flooding: each pixel keeps the nearest pixel that is solid or gives light (its own position if it is one).
const char* kSeedFrag = "#version 330\n" LAMP_FN R"(
uniform sampler2D texture0;  // the mask (frame size)
uniform sampler2D emitT;
uniform vec2 size;
out vec4 finalColor;
void main() {
  vec2 uv = gl_FragCoord.xy / size;
  float s = texture(texture0, uv).a;
  vec3 e = texture(emitT, uv).rgb + lamp(uv) * (1.0 - s);
  bool seed = s > 0.5 || max(e.r, max(e.g, e.b)) > 0.15;
  finalColor = seed ? vec4(gl_FragCoord.xy, 1.0, 1.0) : vec4(0.0);
}
)";
const char* kJumpFrag = R"(#version 330
uniform sampler2D texture0;  // the previous pass
uniform vec2 size;
uniform float jump;
out vec4 finalColor;
void main() {
  vec2 p = gl_FragCoord.xy;
  vec4 best = vec4(0.0);
  float bd = 1e9;
  for (int y = -1; y <= 1; ++y)
    for (int x = -1; x <= 1; ++x) {
      vec2 q = p + vec2(x, y) * jump;
      if (q.x < 0.0 || q.y < 0.0 || q.x > size.x || q.y > size.y) continue;
      vec4 s = texture(texture0, q / size);
      float d = distance(s.xy, p);
      if (s.z > 0.5 && d < bd) bd = d, best = s;
    }
  finalColor = best;
}
)";

// The light at each pixel: 16 rays (turned by a noise that changes every frame) march the distance field. A glow (an
// effect, a lantern): the ray goes through it, gathering its light all along, so its core counts more than its halo.
// A solid: the ray stops, with the light that solid caught the frame before, in its colour (one bounce, more over the
// frames). Off screen or too far: nothing. A pixel inside a solid sees out only through the few pixels next to its
// edge: a fighter is lit on the side that faces the light, never through his body. Blended with the frame before
// (where it was on screen: the camera moves), which smooths the noise.
const char* kTraceFrag = "#version 330\n" LAMP_FN R"(
uniform sampler2D texture0;  // the distance field
uniform sampler2D emitT, maskT, prevT;
uniform vec2 size;
uniform vec4 reproj;  // where a pixel was the frame before: p * xy + zw
uniform vec3 gains;   // the light, the bounce, the noise's frame
out vec4 finalColor;
const int RAYS = 16, STEPS = 40;
const float SKIN = 2.0, FAR = 420.0, DENSITY = 0.04;  // the glows' light per pixel crossed
vec3 emission(vec2 uv) { return texture(emitT, uv).rgb + lamp(uv) * (1.0 - texture(maskT, uv).a); }
float solid(vec2 q) { return texture(maskT, q / size).a; }
float sdf(vec2 q) { vec4 s = texture(texture0, q / size); return s.z > 0.5 ? distance(s.xy, q) : 1e4; }
void main() {
  vec2 p = gl_FragCoord.xy;
  float n = fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))) + gains.z * 0.618034);
  bool inside = solid(p) > 0.5;
  vec3 sum = vec3(0.0);
  for (int i = 0; i < RAYS; ++i) {
    float a = (float(i) + n) * 6.2831853 / float(RAYS);
    vec2 d = vec2(cos(a), sin(a));
    float t = 0.0;
    if (inside) {
      while (t <= SKIN && solid(p + d * t) > 0.5) t += 1.0;
      if (t > SKIN) continue;
    }
    for (int k = 0; k < STEPS; ++k) {
      vec2 q = p + d * t;
      if (t > FAR || q.x < 0.0 || q.y < 0.0 || q.x > size.x || q.y > size.y) break;
      float dist = sdf(q);
      if (dist < 0.75) {
        if (solid(q + d * 0.5) > 0.5) {  // a solid: the light it caught, bounced
          vec2 h = (q + d * 0.5) / size;
          sum += texture(prevT, h).rgb * texture(albedoT, h).rgb * gains.y;
          break;
        }
        // An effect: a glow the light passes through, adding up all along (its core shines more than its halo), till
        // the ray leaves it or meets a solid behind.
        for (int j = 0; j < 24 && t <= FAR; ++j, t += 2.0) {
          vec2 h = p + d * t;
          if (h.x < 0.0 || h.y < 0.0 || h.x > size.x || h.y > size.y || solid(h) > 0.5) break;
          vec3 e = emission(h / size);
          if (max(e.r, max(e.g, e.b)) < 0.15) break;
          sum += e * (2.0 * DENSITY * gains.x);
        }
        continue;
      }
      t += max(dist, 1.0);
    }
  }
  vec3 L = sum / float(RAYS);
  vec2 was = p * reproj.xy + reproj.zw;
  vec3 before = was.x >= 0.0 && was.y >= 0.0 && was.x <= size.x && was.y <= size.y ? texture(prevT, was / size).rgb : L;
  finalColor = vec4(mix(L, before, 0.8), 1.0);
}
)";

// Adds the light to the frame (blend: dst * (1 + src), each surface brightened in its own colour, never tinted dirty).
// On the solids (the sprites, the platforms' bodies) one light per art pixel, in flat bands; the floors catch it all;
// the far backdrop a little; the effects themselves not at all (they would burn to white), unless over a sprite.
const char* kCompositeFrag = "#version 330\n" LAMP_FN R"(
uniform sampler2D texture0;  // the light
uniform sampler2D maskT, emitT;
uniform vec2 size;   // the frame
uniform vec4 gain;   // backdrop, floor, solid, art pixel (frame pixels)
out vec4 finalColor;
void main() {
  vec2 g = gl_FragCoord.xy;
  vec4 m = texture(maskT, g / size);
  float s = step(0.5, m.a);
  vec2 at = mix(g, (floor(g / gain.w) + 0.5) * gain.w, s);
  vec3 L = texture(texture0, at / size).rgb;
  // On the sprites: a rim of light, half of it white (a dark coat must not turn the light's colour), at most +60 %, in
  // three flat steps.
  vec3 R = mix(vec3(max(L.r, max(L.g, L.b))), L, 0.5) * gain.z;
  R = floor(min(R, vec3(0.6)) * 5.0 + 0.4) / 5.0;
  vec3 e = texture(emitT, g / size).rgb + lamp(g / size);
  float lum = max(e.r, max(e.g, e.b));
  vec3 B = min(L * mix(gain.x, gain.y, m.g), vec3(0.9)) * (1.0 - smoothstep(0.3, 1.2, lum));
  finalColor = vec4(mix(B, R * (1.0 - smoothstep(0.4, 1.0, lum)), s), 1.0);
}
)";

struct Pass {
  Shader sh{};
  int size = -1, a = -1, b = -1, c = -1, d = -1, e = -1, f = -1, g = -1;
};
Pass g_mask, g_seed, g_jump, g_trace, g_comp;
bool g_tried = false, g_on = false;
RenderTexture2D g_maskT{}, g_emit{}, g_albedo{}, g_jfa[2]{}, g_gi[2]{};
int g_cur = 0;
struct View {
  float z = 0, tx = 0, ty = 0, ox = 0, oy = 0, H = 0;
} g_prev;

Pass load(const char* frag, std::initializer_list<const char*> names) {
  Pass p;
  p.sh = LoadShaderFromMemory(nullptr, frag);
  p.size = GetShaderLocation(p.sh, "size");
  int* out[] = {&p.a, &p.b, &p.c, &p.d, &p.e, &p.f, &p.g};
  int i = 0;
  for (const char* n : names) *out[i++] = GetShaderLocation(p.sh, n);
  return p;
}
bool ready() {
  if (!g_tried) {
    g_tried = true;
    g_mask = load(kMaskFrag, {});
    g_seed = load(kSeedFrag, {"emitT", "albedoT", "lamps"});
    g_jump = load(kJumpFrag, {"jump"});
    g_trace = load(kTraceFrag, {"emitT", "maskT", "albedoT", "prevT", "reproj", "gains", "lamps"});
    g_comp = load(kCompositeFrag, {"maskT", "emitT", "gain", "albedoT", "lamps"});
  }
  for (const Pass* p : {&g_mask, &g_seed, &g_jump, &g_trace, &g_comp})
    if (p->sh.id == 0 || p->sh.id == rlGetShaderIdDefault()) return false;
  return true;
}

// A half-float target (positions, light that adds up past 1).
RenderTexture2D floatTarget(int w, int h, bool smooth) {
  RenderTexture2D t{};
  t.id = rlLoadFramebuffer();
  t.texture = {rlLoadTexture(nullptr, w, h, RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 1), w, h, 1,
               PIXELFORMAT_UNCOMPRESSED_R16G16B16A16};
  rlFramebufferAttach(t.id, t.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
  if (!rlFramebufferComplete(t.id) || t.texture.id == 0) {
    rlUnloadFramebuffer(t.id);
    return RenderTexture2D{};
  }
  SetTextureFilter(t.texture, smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
  return t;
}

// Draws into `t` (same projection: the frame's pixels land scaled on its size), then back to the frame.
template <class F>
void into(const RenderTexture2D& t, F draw) {
  rlDrawRenderBatchActive();
  rlEnableFramebuffer(t.id);
  rlViewport(0, 0, t.texture.width, t.texture.height);
  draw();
  rlDrawRenderBatchActive();
  const RenderTexture2D* fr = pix::frameTarget();
  rlEnableFramebuffer(fr->id);
  rlViewport(0, 0, fr->texture.width, fr->texture.height);
}
void clear(const RenderTexture2D& t) {
  into(t, [] {
    rlClearColor(0, 0, 0, 0);
    rlClearScreenBuffers();
  });
}
void blend(int src, int dst) {
  rlSetBlendFactors(src, dst, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM);
}

// A full-target pass of `p` into `to` (its own pixels), `src` as texture0; `set` gives the other uniforms.
template <class F>
void pass(const RenderTexture2D& to, const Pass& p, Texture2D src, F set) {
  const Matrix mv = rlGetMatrixModelview(), pr = rlGetMatrixProjection();
  const float w = float(to.texture.width), h = float(to.texture.height), size[2] = {w, h};
  into(to, [&] {
    rlSetMatrixModelview(MatrixIdentity());
    rlSetMatrixProjection(MatrixOrtho(0, w, h, 0, 0, 1));
    blend(RL_ONE, RL_ZERO);
    BeginShaderMode(p.sh);
    SetShaderValue(p.sh, p.size, size, SHADER_UNIFORM_VEC2);
    set();
    DrawTexturePro(src, {0, 0, float(src.width), float(src.height)}, {0, 0, w, h}, {0, 0}, 0.0f, WHITE);
    rlDrawRenderBatchActive();
    EndShaderMode();
  });
  rlSetMatrixModelview(mv);
  rlSetMatrixProjection(pr);
  ui::frameBlend();
}

// Draws into the mask's channels (r, g, b, a) the coverage of what `draw` draws, or (erase) clears them there.
template <class F>
void maskDraw(bool g, bool a, bool erase, F draw) {
  into(g_maskT, [&] {
    rlColorMask(false, g, false, a);
    if (erase) {
      blend(RL_ZERO, RL_ZERO);
    } else {
      BeginShaderMode(g_mask.sh);
      blend(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA);
    }
    draw();
    rlDrawRenderBatchActive();
    if (!erase) EndShaderMode();
    rlColorMask(true, true, true, true);
  });
  ui::frameBlend();
}

View viewOf(const Camera2D& cam, const RenderTexture2D& frame) {
  return {cam.zoom * g_renderScale, cam.target.x, cam.target.y, cam.offset.x * g_renderScale,
          cam.offset.y * g_renderScale, float(frame.texture.height)};
}
}  // namespace

bool on() { return g_on; }

void beginFrame() {
  g_on = false;
  const RenderTexture2D* fr = pix::frameTarget();
  if (!settings().rtx || !fr || !ready()) return;
  const int W = fr->texture.width, H = fr->texture.height;
  if (g_maskT.texture.width != W || g_maskT.texture.height != H) {
    if (g_maskT.id) UnloadRenderTexture(g_maskT);
    g_maskT = LoadRenderTexture(W, H);
    SetTextureFilter(g_maskT.texture, TEXTURE_FILTER_POINT);
  }
  if (g_emit.id == 0) {
    g_emit = floatTarget(kW, kH, true);
    g_albedo = LoadRenderTexture(kW, kH);
    for (auto* t : {&g_jfa[0], &g_jfa[1]}) *t = floatTarget(kW, kH, false);
    for (auto* t : {&g_gi[0], &g_gi[1]}) *t = floatTarget(kW, kH, true);
    rlEnableFramebuffer(fr->id);  // (making a target unbinds the frame)
    for (auto* t : {&g_gi[0], &g_gi[1]}) clear(*t);
  }
  if (!g_emit.id || !g_jfa[0].id || !g_jfa[1].id || !g_gi[0].id || !g_gi[1].id || !g_albedo.id || !g_maskT.id) return;
  clear(g_maskT);
  clear(g_emit);
  g_on = true;
}

// (Not from inside a pixel-art layer: its framebuffer is not the frame's.)
void solid(const std::function<void()>& draw) {
  if (g_on && pix::artUnit() == 0.0f) maskDraw(false, true, false, draw);
}

void light(const std::function<void()>& draw) {
  if (!g_on || pix::artUnit() != 0.0f) return;
  into(g_emit, [&] {
    blend(RL_SRC_ALPHA, RL_ONE);
    draw();
  });
  ui::frameBlend();
}

void platforms(const Match& m) {
  if (!g_on) return;
  const StageTheme theme = m.stage().theme;
  // Those with a floor first: what stands above its front edge is that floor seen in depth (it catches the light,
  // blocks none); below, the platform's body. The others (thin ones, small islands) are solid all over.
  for (int floored = 1; floored >= 0; --floored)
    for (const auto& p : m.platforms) {
      stageart::Floor fl{};
      if (stageart::floorOf(p, theme, fl) != bool(floored)) continue;
      maskDraw(floored, true, false, [&] { stageart::drawPlatform(p, theme, 0.0f); });
      if (!floored) continue;
      const float l = p.rect.left() - 40.0f, w = p.rect.w + 80.0f;
      maskDraw(false, true, true, [&] { DrawRectangleRec({l, fl.front - 4000.0f, w, 4000.0f}, WHITE); });
      maskDraw(true, false, true, [&] {
        DrawRectangleRec({l, fl.front, w, 4000.0f}, WHITE);
        DrawRectangleRec({l, fl.back - 4000.0f, w, 4000.0f}, WHITE);
      });
    }
}

void resolve(const Match& m, const Camera2D& cam) {
  if (!g_on) return;
  g_on = false;
  const RenderTexture2D* fr = pix::frameTarget();
  const float W = float(fr->texture.width), H = float(fr->texture.height);

  // The colours, small: what the light bounces off.
  rlDrawRenderBatchActive();
  rlBindFramebuffer(RL_READ_FRAMEBUFFER, fr->id);
  rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, g_albedo.id);
  rlBlitFramebuffer(0, 0, int(W), int(H), 0, 0, kW, kH, 0x00004000);  // GL_COLOR_BUFFER_BIT
  rlEnableFramebuffer(fr->id);

  // The distance field.
  const float lamps = stageart::lampGain(m.stage().theme);
  pass(g_jfa[0], g_seed, g_maskT.texture, [&] {
    SetShaderValueTexture(g_seed.sh, g_seed.a, g_emit.texture);
    SetShaderValueTexture(g_seed.sh, g_seed.b, g_albedo.texture);
    SetShaderValue(g_seed.sh, g_seed.c, &lamps, SHADER_UNIFORM_FLOAT);
  });
  int j = 0;
  for (int jump = 256; jump >= 1; jump /= 2, j ^= 1) {
    const float k = float(jump);
    pass(g_jfa[j ^ 1], g_jump, g_jfa[j].texture, [&] { SetShaderValue(g_jump.sh, g_jump.a, &k, SHADER_UNIFORM_FLOAT); });
  }

  // The rays. Where each traced pixel was the frame before: through the world, from this camera to the last one.
  const View v = viewOf(cam, *fr), p = g_prev.z > 0.0f ? g_prev : v;
  const float S = W / kW, r = p.z / v.z;
  const float reproj[4] = {r, r, ((v.tx - p.tx) * p.z + p.ox - v.ox * r) / S,
                           (H - (H - v.oy) * r - (v.ty - p.ty) * p.z - p.oy) / S};
  g_prev = v;
  const float tone = stageart::lightGain(m.stage().theme);
  const float gains[3] = {1.5f, 0.35f, float(art::frameId() % 64)};
  const RenderTexture2D &now = g_gi[g_cur ^ 1], &before = g_gi[g_cur];
  pass(now, g_trace, g_jfa[j].texture, [&] {
    SetShaderValueTexture(g_trace.sh, g_trace.a, g_emit.texture);
    SetShaderValueTexture(g_trace.sh, g_trace.b, g_maskT.texture);
    SetShaderValueTexture(g_trace.sh, g_trace.c, g_albedo.texture);
    SetShaderValueTexture(g_trace.sh, g_trace.d, before.texture);
    SetShaderValue(g_trace.sh, g_trace.e, reproj, SHADER_UNIFORM_VEC4);
    SetShaderValue(g_trace.sh, g_trace.f, gains, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_trace.sh, g_trace.g, &lamps, SHADER_UNIFORM_FLOAT);
  });
  g_cur ^= 1;

  // Into the frame, and into its copy before the fighters that the floors mirror (else the light would read there as
  // something standing on the floor, and hide the reflection).
  const Matrix mv = rlGetMatrixModelview();
  const float size[2] = {W, H}, gain[4] = {0.35f * tone, 1.6f * tone, 1.5f * tone, g_renderScale * pix::artPixel()};
  for (const RenderTexture2D* t : {fr, light::beforeFighters()}) {
    if (!t || t->texture.width != fr->texture.width || t->texture.height != fr->texture.height) continue;
    into(*t, [&] {
      rlSetMatrixModelview(MatrixIdentity());
      BeginShaderMode(g_comp.sh);
      blend(RL_DST_COLOR, RL_ONE);
      SetShaderValue(g_comp.sh, g_comp.size, size, SHADER_UNIFORM_VEC2);
      SetShaderValue(g_comp.sh, g_comp.c, gain, SHADER_UNIFORM_VEC4);
      SetShaderValue(g_comp.sh, g_comp.e, &lamps, SHADER_UNIFORM_FLOAT);
      SetShaderValueTexture(g_comp.sh, g_comp.a, g_maskT.texture);
      SetShaderValueTexture(g_comp.sh, g_comp.b, g_emit.texture);
      SetShaderValueTexture(g_comp.sh, g_comp.d, g_albedo.texture);
      DrawTexturePro(now.texture, {0, 0, float(kW), float(kH)}, {0, 0, W, H}, {0, 0}, 0.0f, WHITE);
      rlDrawRenderBatchActive();
      EndShaderMode();
    });
  }
  rlSetMatrixModelview(mv);
  ui::frameBlend();
}
}  // namespace rtx
