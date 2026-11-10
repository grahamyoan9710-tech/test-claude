// The real "RTX" (rtx.hpp): 2D ray traced light. The traced scene is small (kW x kH): what gives light (the effects,
// additive), what blocks it (the mask, at the frame's size so its edges are the sprites' own). The shaders work in
// pixels of their target (gl_FragCoord), every target the frame's way up, so the same uv reads the same place in all
// of them.
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
// The mask's channels: a sprite (a fighter, an item), a floor that catches the light, an effect's own pixels (not its
// halo), what blocks the light.
enum : int { kSprite = 1, kFloor = 2, kCore = 4, kSolid = 8 };

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

// The light at each pixel: 32 rays (turned by a noise that changes every frame) march the distance field. A glow (an
// effect, a lantern): the ray goes through it, gathering its light all along, so its core counts more than its halo.
// A solid: the ray stops, with the light that solid caught the frame before, in its colour (one bounce, more over the
// frames). Off screen or too far: nothing. A pixel inside a solid sees out only through the few pixels next to its
// edge: a fighter is lit on the side that faces the light, never through his body. Grainy: kResolveFrag cleans it.
// Alpha: how much the arena's moon or sun lights it (its colour: keyCol).
const char* kTraceFrag = "#version 330\n" LAMP_FN R"(
uniform sampler2D texture0;  // the distance field
uniform sampler2D emitT, maskT, prevT;
uniform vec2 size;
uniform vec3 gains;   // the light, the bounce, the noise's frame
uniform vec4 key;     // the moon or the sun: where, how wide, how far it lights (0: none)
uniform vec3 keyCol;  // its colour times its strength
out vec4 finalColor;
const int RAYS = 32, STEPS = 64;
const float SKIN = 2.0, FAR = 640.0, DENSITY = 0.04;  // the glows' light per pixel crossed
const float SHADOW = 260.0;  // a shadow of the moon or the sun fades over this length behind what casts it
vec3 emission(vec2 uv) { return texture(emitT, uv).rgb + lamp(uv) * (1.0 - texture(maskT, uv).a); }
float solid(vec2 q) { return texture(maskT, q / size).a; }
float sdf(vec2 q) { vec4 s = texture(texture0, q / size); return s.z > 0.5 ? distance(s.xy, q) : 1e4; }
// The moon or the sun: one ray toward a point of its disk (another each frame: soft shadows). Lit unless a solid is in
// the way (the nearer, the darker its shadow), dimmer far from it.
float keyLight(vec2 p, bool inside, float n) {
  if (key.w <= 0.0) return 0.0;
  float a = 6.2831853 * fract(n * 7.0 + 0.3), r = key.z * sqrt(n);
  vec2 v = key.xy + r * vec2(cos(a), sin(a)) - p, d = v / max(length(v), 1e-3);
  float t = 0.0, lit = 1.0;
  if (inside) {
    while (t <= SKIN && solid(p + d * t) > 0.5) t += 1.0;
    if (t > SKIN) return 0.0;
  }
  for (int k = 0; k < STEPS && t < min(length(v), SHADOW); ++k) {
    vec2 q = p + d * t;
    if (q.x < 0.0 || q.y < 0.0 || q.x > size.x || q.y > size.y) break;
    float dist = sdf(q);
    if (dist < 0.75 && solid(q + d * 0.5) > 0.5) {
      lit = smoothstep(0.0, SHADOW, t);
      break;
    }
    t += max(dist, 1.0);
  }
  float f = distance(key.xy, p) / key.w;
  return lit / (1.0 + f * f);
}
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
          vec4 b = texture(prevT, h);
          sum += (b.rgb + keyCol * b.a) * texture(albedoT, h).rgb * gains.y;
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
  finalColor = vec4(sum / float(RAYS), keyLight(p, inside, n));
}
)";

// The rays' light, cleaned. Its grain (the dust) is averaged over the pixels around (a 5x5 tent; never across a
// solid's edge, so a fighter's lit side and the shadow he casts stay sharp), then over the frames: the frame before
// (where it was on screen: the camera moves) is kept only within this frame's spread around that average, so what
// moved or went out leaves no trail (the ghost images).
const char* kResolveFrag = R"(#version 330
uniform sampler2D texture0;  // this frame's rays
uniform sampler2D maskT, prevT;
uniform vec2 size;
uniform vec4 reproj;  // where a pixel was the frame before: p * xy + zw
out vec4 finalColor;
void main() {
  vec2 p = gl_FragCoord.xy;
  float s = step(0.5, texture(maskT, p / size).a);
  vec4 sum = vec4(0.0), sq = vec4(0.0);
  float n = 0.0;
  for (int y = -2; y <= 2; ++y)
    for (int x = -2; x <= 2; ++x) {
      vec2 q = clamp(p + vec2(x, y), vec2(0.5), size - 0.5);
      float w = float((3 - abs(x)) * (3 - abs(y))) * (1.0 - abs(step(0.5, texture(maskT, q / size).a) - s));
      vec4 c = texture(texture0, q / size);
      sum += c * w;
      sq += c * c * w;
      n += w;
    }
  vec4 mean = sum / n, spread = sqrt(max(sq / n - mean * mean, 0.0));
  vec2 was = p * reproj.xy + reproj.zw;
  vec4 before = was.x >= 0.0 && was.y >= 0.0 && was.x <= size.x && was.y <= size.y ? texture(prevT, was / size) : mean;
  finalColor = mix(mean, clamp(before, mean - spread, mean + spread), 0.85);
}
)";

// The frame, lit as in a path traced picture: the arena darker and a little warmer where no light comes, more toward
// the screen's edges (its highlights, snow or a bright sky, keep their white), lit in the light's own colour where it
// comes, the light hanging in the air around what gives it; the moon or the sun lights it all but the shadows. The
// fighters keep their colours (never dirtied): a slight neutral shade and a rim of light, one per art pixel in flat
// bands, as on the platforms' bodies. The effects and the lanterns stay as drawn.
const char* kCompositeFrag = R"(#version 330
uniform sampler2D texture0;  // the light
uniform sampler2D maskT, frameT;
uniform vec2 size;   // the frame
uniform vec4 gain;   // backdrop, floor, solid, art pixel (frame pixels)
uniform vec4 shade;  // the unlit arena's colour; whether this target holds the fighters and the effects yet
uniform vec2 extra;  // the haze, the lanterns
uniform vec3 keyCol;  // the moon's or the sun's colour times its strength
out vec4 finalColor;
void main() {
  vec2 g = gl_FragCoord.xy, uv = g / size;
  vec4 m = texture(maskT, uv), c = texture(frameT, uv);
  float s = step(0.5, m.a), f = step(0.5, m.r) * shade.a;
  vec4 T = texture(texture0, mix(g, (floor(g / gain.w) + 0.5) * gain.w, s) / size);
  vec3 L = T.rgb, K = keyCol * T.a;
  float lum = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
  vec2 d = uv - 0.5;
  vec3 base = mix(shade.rgb, vec3(0.92, 0.9, 0.86), smoothstep(0.72, 1.0, lum)) * (1.0 - 0.3 * dot(d, d));
  vec3 k = min(L * mix(mix(gain.x, gain.y, m.g), gain.z, s) + K * mix(mix(0.3, 0.7, m.g), 1.2, s), vec3(1.6));
  k = mix(k, floor(k * 5.0 + 0.5) / 5.0, s);
  vec3 arena = c.rgb * (base + k) + (L * extra.x + K * 0.06) * (1.0 - s);
  arena = mix(vec3(dot(arena, vec3(0.2126, 0.7152, 0.0722))), arena, 1.15);  // richer colours, not greyed
  vec3 F = L * gain.z + K * 0.7;  // the light on a fighter, half its colour (whites stay white)
  vec3 R = floor(min(mix(vec3(max(F.r, max(F.g, F.b))), F, 0.5), vec3(0.8)) * 5.0 + 0.4) / 5.0;
  vec3 fighter = c.rgb * (0.9 + R);
  float mx = max(c.r, max(c.g, c.b)), mn = min(c.r, min(c.g, c.b));
  // A lantern's pale hot centre counts too (graded alone it turned into a brown blot); never a fighter's skin.
  float hot = max(smoothstep(0.35, 0.6, (mx - mn) / max(mx, 1e-3)), smoothstep(0.2, 0.3, c.r - c.b));
  float lamp = step(0.001, extra.y) * (1.0 - step(0.5, m.r)) * smoothstep(0.72, 0.95, mx) * hot;
  finalColor = vec4(mix(mix(arena, fighter, f), c.rgb, max(m.b * shade.a, lamp)), c.a);
}
)";

struct Pass {
  Shader sh{};
  int size = -1, a = -1, b = -1, c = -1, d = -1, e = -1, f = -1, g = -1, h = -1;
};
Pass g_mask, g_seed, g_jump, g_trace, g_resolve, g_comp;
bool g_tried = false, g_on = false;
// (g_copy: a target as it was before its lighting.)
RenderTexture2D g_maskT{}, g_copy{}, g_emit{}, g_albedo{}, g_jfa[2]{}, g_raw{}, g_gi[2]{};
int g_cur = 0;
struct View {
  float z = 0, tx = 0, ty = 0, ox = 0, oy = 0, H = 0;
} g_prev;

Pass load(const char* frag, std::initializer_list<const char*> names) {
  Pass p;
  p.sh = LoadShaderFromMemory(nullptr, frag);
  p.size = GetShaderLocation(p.sh, "size");
  int* out[] = {&p.a, &p.b, &p.c, &p.d, &p.e, &p.f, &p.g, &p.h};
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
    g_trace = load(kTraceFrag, {"emitT", "maskT", "albedoT", "prevT", "gains", "lamps", "key", "keyCol"});
    g_resolve = load(kResolveFrag, {"maskT", "prevT", "reproj"});
    g_comp = load(kCompositeFrag, {"maskT", "frameT", "gain", "shade", "extra", "keyCol"});
  }
  for (const Pass* p : {&g_mask, &g_seed, &g_jump, &g_trace, &g_resolve, &g_comp})
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
void blit(const RenderTexture2D& from, const RenderTexture2D& to) {
  rlDrawRenderBatchActive();
  rlBindFramebuffer(RL_READ_FRAMEBUFFER, from.id);
  rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, to.id);
  rlBlitFramebuffer(0, 0, from.texture.width, from.texture.height, 0, 0, to.texture.width, to.texture.height,
                    0x00004000);  // GL_COLOR_BUFFER_BIT
  rlEnableFramebuffer(pix::frameTarget()->id);
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

// Draws into the mask's channels the coverage of what `draw` draws, or (erase) clears them there.
template <class F>
void maskDraw(int channels, bool erase, F draw) {
  into(g_maskT, [&] {
    rlColorMask(channels & kSprite, channels & kFloor, channels & kCore, channels & kSolid);
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
Texture2D mask() { return g_maskT.texture; }

void beginFrame() {
  g_on = false;
  const RenderTexture2D* fr = pix::frameTarget();
  if (!settings().rtx || !fr || !ready()) return;
  const int W = fr->texture.width, H = fr->texture.height;
  if (g_maskT.texture.width != W || g_maskT.texture.height != H) {
    for (auto* t : {&g_maskT, &g_copy}) {
      if (t->id) UnloadRenderTexture(*t);
      *t = LoadRenderTexture(W, H);
      SetTextureFilter(t->texture, TEXTURE_FILTER_POINT);
    }
    rlEnableFramebuffer(fr->id);
  }
  if (g_emit.id == 0) {
    g_emit = floatTarget(kW, kH, true);
    g_albedo = LoadRenderTexture(kW, kH);
    for (auto* t : {&g_jfa[0], &g_jfa[1], &g_raw}) *t = floatTarget(kW, kH, false);
    for (auto* t : {&g_gi[0], &g_gi[1]}) *t = floatTarget(kW, kH, true);
    rlEnableFramebuffer(fr->id);  // (making a target unbinds the frame)
    for (auto* t : {&g_gi[0], &g_gi[1]}) clear(*t);
  }
  if (!g_emit.id || !g_jfa[0].id || !g_jfa[1].id || !g_raw.id || !g_gi[0].id || !g_gi[1].id || !g_albedo.id ||
      !g_maskT.id || !g_copy.id)
    return;
  clear(g_maskT);
  clear(g_emit);
  g_on = true;
}

// (Not from inside a pixel-art layer: its framebuffer is not the frame's.)
void solid(const std::function<void()>& draw) {
  if (g_on && pix::artUnit() == 0.0f) maskDraw(kSprite | kSolid, false, draw);
}

void light(const std::function<void()>& draw, const std::function<void()>& core) {
  if (!g_on || pix::artUnit() != 0.0f) return;
  into(g_emit, [&] {
    blend(RL_SRC_ALPHA, RL_ONE);
    draw();
  });
  ui::frameBlend();
  maskDraw(kCore, false, core);
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
      maskDraw(floored ? kFloor | kSolid : kSolid, false, [&] { stageart::drawPlatform(p, theme, 0.0f); });
      if (!floored) continue;
      const float l = p.rect.left() - 40.0f, w = p.rect.w + 80.0f;
      maskDraw(kSolid, true, [&] { DrawRectangleRec({l, fl.front - 4000.0f, w, 4000.0f}, WHITE); });
      maskDraw(kFloor, true, [&] {
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

  blit(*fr, g_albedo);  // the colours, small: what the light bounces off

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

  // The rays, then cleaned. Where each traced pixel was the frame before: through the world, from this camera to the
  // last one. The moon or the sun: from the screen to the traced pixels (y up, as in the targets).
  const float S = W / kW, gains[3] = {3.0f, 0.6f, float(art::frameId() % 64)};
  float key[4] = {}, keyCol[3] = {};
  if (stageart::KeyLight kl; stageart::keyLight(m.stage().theme, kl)) {
    const float u = g_renderScale / S;
    const float k[4] = {kl.at.x * u, (H - kl.at.y * g_renderScale) / S, kl.radius * u, kl.reach * u};
    std::copy(k, k + 4, key);
    std::copy(kl.light, kl.light + 3, keyCol);
  }
  const RenderTexture2D &now = g_gi[g_cur ^ 1], &before = g_gi[g_cur];
  pass(g_raw, g_trace, g_jfa[j].texture, [&] {
    SetShaderValueTexture(g_trace.sh, g_trace.a, g_emit.texture);
    SetShaderValueTexture(g_trace.sh, g_trace.b, g_maskT.texture);
    SetShaderValueTexture(g_trace.sh, g_trace.c, g_albedo.texture);
    SetShaderValueTexture(g_trace.sh, g_trace.d, before.texture);
    SetShaderValue(g_trace.sh, g_trace.e, gains, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_trace.sh, g_trace.f, &lamps, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_trace.sh, g_trace.g, key, SHADER_UNIFORM_VEC4);
    SetShaderValue(g_trace.sh, g_trace.h, keyCol, SHADER_UNIFORM_VEC3);
  });
  const View v = viewOf(cam, *fr), p = g_prev.z > 0.0f ? g_prev : v;
  const float r = p.z / v.z;
  const float reproj[4] = {r, r, ((v.tx - p.tx) * p.z + p.ox - v.ox * r) / S,
                           (H - (H - v.oy) * r - (v.ty - p.ty) * p.z - p.oy) / S};
  g_prev = v;
  pass(now, g_resolve, g_raw.texture, [&] {
    SetShaderValueTexture(g_resolve.sh, g_resolve.a, g_maskT.texture);
    SetShaderValueTexture(g_resolve.sh, g_resolve.b, before.texture);
    SetShaderValue(g_resolve.sh, g_resolve.c, reproj, SHADER_UNIFORM_VEC4);
  });
  g_cur ^= 1;
  const float tone = stageart::lightGain(m.stage().theme);

  // Into the frame, and into its copy before the fighters and the effects that the floors mirror (lit the same way,
  // else the floor would read there as something standing on it, and hide the reflection).
  const Matrix mv = rlGetMatrixModelview();
  const float size[2] = {W, H}, gain[4] = {0.7f * tone, 3.0f * tone, 1.8f * tone, g_renderScale * pix::artPixel()};
  const float extra[2] = {0.35f * tone, lamps};
  for (const RenderTexture2D* t : {fr, light::beforeFighters()}) {
    if (!t || t->texture.width != fr->texture.width || t->texture.height != fr->texture.height) continue;
    blit(*t, g_copy);
    const float amb = key[3] > 0.0f ? 0.8f : 1.0f;  // under a moon or a sun, its shadows darker
    const float shade[4] = {0.7f * amb, 0.65f * amb, 0.6f * amb, t == fr ? 1.0f : 0.0f};
    into(*t, [&] {
      rlSetMatrixModelview(MatrixIdentity());
      BeginShaderMode(g_comp.sh);
      blend(RL_ONE, RL_ZERO);
      SetShaderValue(g_comp.sh, g_comp.size, size, SHADER_UNIFORM_VEC2);
      SetShaderValue(g_comp.sh, g_comp.c, gain, SHADER_UNIFORM_VEC4);
      SetShaderValue(g_comp.sh, g_comp.d, shade, SHADER_UNIFORM_VEC4);
      SetShaderValue(g_comp.sh, g_comp.e, extra, SHADER_UNIFORM_VEC2);
      SetShaderValue(g_comp.sh, g_comp.f, keyCol, SHADER_UNIFORM_VEC3);
      SetShaderValueTexture(g_comp.sh, g_comp.a, g_maskT.texture);
      SetShaderValueTexture(g_comp.sh, g_comp.b, g_copy.texture);
      DrawTexturePro(now.texture, {0, 0, float(kW), float(kH)}, {0, 0, W, H}, {0, 0}, 0.0f, WHITE);
      rlDrawRenderBatchActive();
      EndShaderMode();
    });
  }
  rlSetMatrixModelview(mv);
  ui::frameBlend();
}
}  // namespace rtx
