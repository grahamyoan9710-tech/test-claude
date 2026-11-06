// Stage backgrounds and platforms (the arenas: render_stages.cpp), projectiles and the Clash Orb.
#include <cstring>

#include "game/fighter_art.hpp"
#include "game/paint.hpp"
#include "game/pixel.hpp"
#include "game/render.hpp"
#include "game/stage_art.hpp"
#include "game/ui.hpp"

using namespace paint;

namespace {
// A colour along three stops: c0 at 0, c1 at m, c2 at 1.
Color ramp3(float t, Color c0, float m, Color c1, Color c2) {
  t = clampf(t, 0.0f, 1.0f);
  return t < m ? mixColor(c0, c1, t / m) : mixColor(c1, c2, (t - m) / (1.0f - m));
}

// One half of the Clash Orb, the sky (top) or the gold earth (bottom), shaded like the mockup's SVG: a radial gradient
// set in the half's bounding box (so elliptical), as a polar mesh of vertex colours.
void orbHalf(Vec2 c, float r, bool top) {
  const float gx = (top ? -0.24f : 0.2f) * r, gy = (top ? -0.7f : 0.75f) * r, rx = 1.8f * r, ry = 0.9f * r;
  const float sy = top ? -1.0f : 1.0f;
  auto at = [&](int i, int j) {
    constexpr int kA = 18, kR = 4;
    const float a = kPi * i / kA, q = r * j / kR;
    return Vec2{std::cos(a) * q, sy * std::sin(a) * q};
  };
  auto col = [&](Vec2 p) {
    const float d = std::sqrt((p.x - gx) * (p.x - gx) / (rx * rx) + (p.y - gy) * (p.y - gy) / (ry * ry));
    return top ? ramp3(d, {216, 236, 255, 255}, 0.35f, {95, 162, 255, 255}, {11, 30, 85, 255})
               : ramp3(d, {255, 226, 168, 255}, 0.4f, {255, 181, 71, 255}, {106, 50, 8, 255});
  };
  for (int i = 0; i < 18; ++i)
    for (int j = 0; j < 4; ++j) {
      const Vec2 p0 = at(i, j), p1 = at(i + 1, j), p2 = at(i + 1, j + 1), p3 = at(i, j + 1);
      vfx::quad(c + p0, c + p1, c + p2, c + p3, col(p0), col(p1), col(p2), col(p3));
    }
}
}  // namespace

void drawBackground(const StageDef& stage, Vec2 cam, float zoom, float time) {
  (void)zoom;
  const float dx = clampf(cam.x, -1200.0f, 1200.0f), dy = clampf(cam.y + 150.0f, -500.0f, 500.0f);
  const float skyX = -40.0f + clampf(-dx * 0.03f, -40.0f, 40.0f), skyY = -40.0f + clampf(-dy * 0.03f, -40.0f, 40.0f);
  stageart::drawBackground(stage.theme, {dx, dy}, skyX, skyY, time);
}

void drawStageForeground(const StageDef& stage, Vec2 cam, float time) {
  const Vec2 pc{clampf(cam.x, -1200.0f, 1200.0f), clampf(cam.y + 150.0f, -500.0f, 500.0f)};
  stageart::drawForeground(stage.theme, pc, time);
}

void drawPlatforms(const Match& m, float time) { drawPlatformList(m.platforms, m.stage().theme, time); }

void drawPlatformList(const std::vector<Platform>& platforms, StageTheme theme, float time) {
  for (const auto& p : platforms) stageart::drawPlatform(p, theme, time);
}

// ---- Projectiles -------------------------------------------------------------------------
namespace {
using vfx::hash01;
inline Color Ap(Color c, float a) { return withAlpha(c, a); }

struct Shot {  // a projectile seen by the renderer
  Vec2 c, d, n;    // centre, direction of flight, normal
  float r, k;      // radius, visibility (spawn pop and vanish fade)
  float age;
  uint32_t seed;
};

void shotWarning(const Projectile& p, const Shot& s, float time, bool fromSky) {
  const float u = std::min(1.0f, float(p.age) / std::max(1, p.armDelay));
  const Color c = hitFxGlow(p.hit.fx);
  vfx::additive(true);
  const float pulse = 0.6f + 0.4f * std::sin(time * 30.0f);
  vfx::softRing(s.c, s.r * (1.6f - 0.6f * u), 5.0f, Ap(c, (0.3f + 0.5f * u) * pulse));
  vfx::glow(s.c, s.r * 0.6f * u, Ap(c, 0.35f * u), Ap(c, 0.0f));
  if (fromSky) {
    const Rect v = vfx::viewRect(40.0f);
    vfx::streak({s.c.x, v.y}, s.c, 2.0f + 6.0f * u, 2.0f + 10.0f * u, Ap(c, 0.05f), Ap(c, 0.35f * u * pulse));
  }
  vfx::additive(false);
}

// The placeholder of a projectile whose PixelLab effect is not imported yet: its shape, in its element's colours.
void shotPlaceholder(const Projectile& p, const Shot& s, float time) {
  const Color c = hitFxColor(p.hit.fx), g = hitFxGlow(p.hit.fx);
  vfx::additive(true);
  switch (p.visual) {
    case ProjVisual::Crescent: case ProjVisual::Wave:
      vfx::crescent(s.c - s.d * s.r, s.d, s.r * 1.6f, s.r * 0.7f, 1.1f, Ap(c, 0.85f * s.k), Ap(g, s.k));
      break;
    case ProjVisual::Bolt: case ProjVisual::Arrow: case ProjVisual::Spear:
      vfx::streak(s.c - s.d * (s.r * 3.0f), s.c + s.d * s.r, s.r * 0.25f, s.r * 0.7f, Ap(c, 0.0f), Ap(g, s.k));
      break;
    case ProjVisual::Star: case ProjVisual::Blade:
      vfx::burst(s.c, p.visual == ProjVisual::Star ? 5 : 4, s.r * 0.3f, s.r * 1.3f, 0.5f, time * 9.0f, Ap(g, s.k),
                 Ap(c, 0.0f), s.seed);
      break;
    case ProjVisual::Pillar: {
      const Vec2 foot = s.c + Vec2{0.0f, s.r * 1.2f};
      vfx::streak(foot, foot + Vec2{0.0f, -s.r * 5.0f}, s.r * 1.1f, s.r * 0.5f, Ap(g, s.k), Ap(c, 0.0f));
      break;
    }
    default:
      vfx::glow(s.c, s.r * 1.5f, Ap(c, 0.6f * s.k), Ap(c, 0.0f));
      vfx::glow(s.c, s.r * 0.8f, Ap(g, s.k), Ap(g, 0.0f));
      break;
  }
  vfx::additive(false);
}
}  // namespace

// A ParticleFX effect (pfx_*, art/particlefx): picked by the owner, so shown even with kVfx off.
static bool chosenFx(const char* fx) { return fx && std::strncmp(fx, "pfx_", 4) == 0; }

// A projectile drawn by its PixelLab effect (ProjectileDef::fx, an "fx" line of sheets.txt), sized on its hitbox:
// the effect's hit radius is the projectile's radius. Flying shapes turn with their course; a column rises from the
// ground; a still mine or orb stays upright.
static bool shotSprite(const Projectile& p, const Shot& s, float time, float px) {
  const art::FxInfo* fx = p.fx ? art::fxInfo(p.fx) : nullptr;
  if (!fx) return false;
  // (pixel art keeps its owner's pixel size: its hitbox is made to fit the drawing instead)
  const float size = fx->pixel ? px : s.r / (fx->hit > 0.0f ? fx->hit : 0.4f * std::min(fx->fw, fx->fh));
  // A flying shape cycles its frames (copies side by side do not pulse together); a column plays once as it rises;
  // a ParticleFX one-shot plays once over the flight.
  float t = art::fxLoopTime(p.fx, time + hash01(s.seed) * fx->duration());
  if (chosenFx(p.fx) && !fx->loop) t = s.age / 60.0f;
  if (p.visual == ProjVisual::Pillar)
    return art::drawFx(p.fx, s.age / 60.0f, s.c + Vec2{0.0f, s.r * 1.2f}, size, 0.0f, false, s.k);
  if (p.vel.len2() < 1.0f) return art::drawFx(p.fx, t, s.c, size, 0.0f, (s.seed & 1u) != 0, s.k);
  const bool flip = s.d.x < 0.0f;  // mirrored, not upside down, when it flies to the left
  const float rot = flip ? std::atan2(-s.d.y, -s.d.x) : std::atan2(s.d.y, s.d.x);
  return art::drawFx(p.fx, t, s.c, size, rot, flip, s.k);
}

void drawProjectiles(const Match& m, float time) {
  for (size_t i = 0; i < m.projectiles.size(); ++i) {
    const Projectile& p = m.projectiles[i];
    Shot s;
    s.c = p.pos;
    s.d = p.vel.len2() > 0.01f ? p.vel.norm() : Vec2{1.0f, 0.0f};
    s.n = {-s.d.y, s.d.x};
    s.r = p.radius;
    s.k = std::min(1.0f, p.life / 8.0f) * std::min(1.0f, (p.age + 1) / 3.0f);
    s.age = float(p.age);
    s.seed = uint32_t(i) * 7919u + uint32_t(p.owner + 1) * 104729u;
    if (chosenFx(p.fx)) {  // its own warning: it fades in until it strikes
      if (!p.armed()) s.k *= 0.3f + 0.7f * s.age / float(std::max(1, p.armDelay));
      const bool own = p.owner >= 0 && p.owner < int(m.fighters.size());
      if (shotSprite(p, s, time, own ? art::pixelScale(*m.fighters[p.owner].def) : 1.0f)) continue;
    }
    if (!kVfx) {  // a plain white ring on the hitbox (fainter: the warning before the strike)
      DrawRing(V(s.c), std::max(1.0f, s.r - 2.5f), s.r, 0, 360, 32, withAlpha(WHITE, (p.armed() ? 0.75f : 0.3f) * s.k));
      continue;
    }
    if (!p.armed()) {  // the warning before the strike
      shotWarning(p, s, time, p.visual == ProjVisual::Pillar && p.vel.y > 0.5f);
      continue;
    }
    if (shotSprite(p, s, time, p.owner >= 0 && p.owner < int(m.fighters.size()) ? art::pixelScale(*m.fighters[p.owner].def) : 1.0f))
      continue;
    shotPlaceholder(p, s, time);
  }
}

void drawClashOrb(Vec2 c, float r, float time, int detail, float damage) {
  const float k = r / 62.0f;  // the mockup draws it 62 units wide
  if (detail >= 2) {  // in play: its halo and the dashed ring turning around it
    vfx::ring(c, r, r * 1.58f, withAlpha(WHITE, 0.26f), withAlpha(WHITE, 0.0f));
    const float spin = time * 2.0f * kPi / 9.0f, thick = std::max(1.5f, 3.0f * k);
    for (int i = 0; i < 36; ++i) {
      const float a = spin + i * 2.0f * kPi / 36.0f;
      DrawRing(V(c), 80.0f * k - thick * 0.5f, 80.0f * k + thick * 0.5f, rad2deg(a), rad2deg(a + 0.05f), 2,
               withAlpha(ui::kCream, 0.55f));
    }
  }
  orbHalf(c, r, true);
  orbHalf(c, r, false);
  if (detail >= 2) {
    const Vector2 g[4] = {{c.x - 44 * k, c.y - 30 * k}, {c.x - 36 * k, c.y - 46 * k}, {c.x - 20 * k, c.y - 50 * k},
                          {c.x - 30 * k, c.y - 36 * k}};
    ui::quad(g[0], g[1], g[2], g[3], withAlpha(WHITE, 0.5f), withAlpha(WHITE, 0.5f), withAlpha(WHITE, 0.5f),
             withAlpha(WHITE, 0.5f));  // the glint of the sky
    const float w = std::max(1.5f, 4.0f * k);  // cracks, as it loses its life
    auto crack = [&](std::initializer_list<Vec2> pts) {
      const Vec2* p = pts.begin();
      for (size_t i = 1; i < pts.size(); ++i) DrawLineEx(V(c + p[i - 1] * k), V(c + p[i] * k), w, WHITE);
    };
    if (damage >= 0.3f) crack({{-10, -60}, {-4, -30}, {-22, -14}});
    if (damage >= 0.65f) crack({{30, 26}, {14, 18}, {18, 50}});
  }
  const float outline = (detail == 1 ? 10.0f : detail >= 2 ? 6.0f : 8.0f) * k;
  DrawRing(V(c), r - outline * 0.5f, r + outline * 0.5f, 0.0f, 360.0f, 48, ui::kInk);
  if (detail != 1) {  // the horizon line, out of the sphere in play
    const float half = detail >= 2 ? 96.0f * k : r;
    if (detail >= 2) ui::horizonLine(c.x - half, c.x + half, c.y, 6.0f * k);
    else DrawRectangleRec({c.x - half, c.y - std::max(0.75f, k * 4.0f), half * 2.0f, std::max(1.5f, k * 8.0f)}, WHITE);
  }
  if (detail >= 1) {  // the logo's spark at its heart
    const float beat = detail >= 2 ? 1.0f + 0.12f * std::sin(time * 2.0f * kPi / 1.8f) : 1.0f;
    ui::spark(V(c), 40.0f * k * beat, 0.0f, WHITE, detail >= 2 ? 0.15f : 0.2f);
    if (detail >= 2) DrawCircleV(V(c), 6.0f * k, WHITE);
  }
}

void drawOrb(const Match& m, float time) {
  const Orb& o = m.orb;
  if (!o.active) return;
  if (o.life < 120 && (o.life / 6) % 2 == 0) return;  // about to vanish: blink
  const float grow = smoothstep(float(60 * 18 - o.life) / 18.0f);  // it opens up as it appears
  const Vec2 c = o.pos + Vec2{0.0f, -std::sin(time * 2.0f * kPi / 3.0f) * o.radius * 0.15f};
  drawClashOrb(c, o.radius * grow, time, 2, 1.0f - o.hp / 40.0f);
  if (o.flash > 0) DrawCircleV(V(c), o.radius * 1.05f, withAlpha(WHITE, 0.8f * o.flash / 8.0f));
}

void drawStagePreview(const StageDef& s, Rectangle area, float time) {
  // The real stage, scaled down and cropped to the area.
  const float sc = std::max(area.width / kScreenW, area.height / kScreenH);
  beginClip(area);
  rlPushMatrix();
  rlTranslatef(area.x + (area.width - kScreenW * sc) * 0.5f, area.y + (area.height - kScreenH * sc) * 0.5f, 0.0f);
  rlScalef(sc, sc, 1.0f);
  g_preview = true;
  drawBackground(s, {std::sin(time * 0.3f) * 60.0f, -150.0f}, 1.0f, time);
  g_preview = false;
  rlTranslatef(kScreenW * 0.5f, kScreenH * 0.5f, 0.0f);
  rlScalef(0.78f, 0.78f, 1.0f);
  rlTranslatef(0.0f, 90.0f, 0.0f);
  std::vector<Platform> plats = s.platforms;
  updatePlatforms(plats, int(time * 60.0f));
  drawPlatformList(plats, s.theme, time);
  rlPopMatrix();
  endClip();
}

void drawStageThumb(int index, const StageDef& s, Rectangle area, float cut) {
  static std::vector<Layer> cache;
  if (index < 0) return;
  if (int(cache.size()) <= index) cache.resize(index + 1);
  Layer& L = cache[index];
  if (L.rt.id == 0 || L.w != area.width || L.h != area.height || std::fabs(L.scale - g_renderScale) > 0.01f) {
    if (L.rt.id != 0) UnloadRenderTexture(L.rt);
    L = bake(area.width, area.height, g_renderScale, [&] { drawStagePreview(s, {0, 0, area.width, area.height}, 2.0f); }, false);
  }
  if (cut <= 0.0f) {
    drawBackdrop(L, area.x, area.y, area.y + area.height);
    return;
  }
  // The texture on the six corners of the cut box (a fan from its centre); the texture is upside down.
  const float tw = float(L.rt.texture.width), th = float(L.rt.texture.height);
  const Vector2 p[6] = {{area.x + cut, area.y}, {area.x + area.width, area.y}, {area.x + area.width, area.y + area.height - cut},
                        {area.x + area.width - cut, area.y + area.height}, {area.x, area.y + area.height}, {area.x, area.y + cut}};
  const Vector2 c{area.x + area.width * 0.5f, area.y + area.height * 0.5f};
  auto vertex = [&](Vector2 q) {
    rlTexCoord2f((q.x - area.x) * L.scale / tw, 1.0f - (q.y - area.y) * L.scale / th);
    rlVertex2f(q.x, q.y);
  };
  rlDrawRenderBatchActive();
  SetTextureFilter(L.rt.texture, g_preview ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
  rlSetTexture(L.rt.texture.id);
  rlBegin(RL_QUADS);  // rlgl textures quads only: each triangle is a quad with its last corner twice
  rlColor4ub(255, 255, 255, 255);
  for (int i = 0; i < 6; ++i) {
    vertex(c);
    vertex(p[(i + 1) % 6]);
    vertex(p[i]);
    vertex(p[i]);
  }
  rlEnd();
  rlSetTexture(0);
}
