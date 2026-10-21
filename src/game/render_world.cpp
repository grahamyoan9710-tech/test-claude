// Stage backgrounds and platforms (the arenas: render_stages.cpp), projectiles and the Clash Orb.
#include "game/fighter_art.hpp"
#include "game/paint.hpp"
#include "game/pixel.hpp"
#include "game/render.hpp"
#include "game/stage_art.hpp"
#include "game/ui.hpp"

using namespace paint;

namespace {

// Zonai spiral rune.
void spiral(Vector2 c, float r, float turns, float thick, Color col, float rot = 0.0f) {
  const int n = int(clampf(r * 0.8f, 8.0f, 20.0f) * turns);
  Vector2 prev = c;
  for (int i = 1; i <= n; ++i) {
    float t = float(i) / n, a = rot + t * turns * 2.0f * kPi;
    Vector2 p{c.x + std::cos(a) * r * t, c.y + std::sin(a) * r * t};
    DrawLineEx(prev, p, thick, col);
    prev = p;
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

// A projectile drawn by its PixelLab effect (ProjectileDef::fx, an "fx" line of sheets.txt), sized on its hitbox:
// the effect's hit radius is the projectile's radius. Flying shapes turn with their course; a column rises from the
// ground; a still mine or orb stays upright.
static bool shotSprite(const Projectile& p, const Shot& s, float time) {
  const art::FxInfo* fx = p.fx ? art::fxInfo(p.fx) : nullptr;
  if (!fx) return false;
  const float size = s.r / (fx->hit > 0.0f ? fx->hit : 0.4f * std::min(fx->fw, fx->fh));
  // A flying shape cycles its frames (copies side by side do not pulse together); a column plays once as it rises.
  const float t = art::fxLoopTime(p.fx, time + hash01(s.seed) * fx->duration());
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
    if (!p.armed()) {  // the warning before the strike
      shotWarning(p, s, time, p.visual == ProjVisual::Pillar && p.vel.y > 0.5f);
      continue;
    }
    if (shotSprite(p, s, time)) continue;
    shotPlaceholder(p, s, time);
  }
}

void drawOrb(const Match& m, float time) {
  const Orb& o = m.orb;
  if (!o.active) return;
  if (o.life < 120 && (o.life / 6) % 2 == 0) return;  // about to vanish: blink
  const Vector2 c = V(o.pos);
  const float r = o.radius, pulse = 0.5f + 0.5f * std::sin(time * 5.0f);
  const Color teal{100, 250, 220, 255}, core{225, 255, 245, 255};
  const Color stone{170, 164, 140, 255}, stoneDk{96, 96, 88, 255}, ink{30, 34, 36, 255};
  glow(c, r * 2.7f, teal, 0.22f + 0.12f * pulse);
  const float rot = time * 1.1f;  // rune ring
  DrawRing(c, r * 1.42f, r * 1.5f, 0, 360, 48, withAlpha(teal, 0.4f + 0.2f * pulse));
  for (int i = 0; i < 12; ++i) {
    float a = rot + i * kPi / 6.0f;
    Vec2 d = rotate({1, 0}, a), t = rotate({0, 1}, a);
    Vector2 p{c.x + d.x * r * 1.72f, c.y + d.y * r * 1.72f};
    if (i % 3 == 0) {
      DrawCircleV(p, r * 0.13f, withAlpha(teal, 0.9f));
      DrawCircleV(p, r * 0.06f, core);
    } else {
      DrawLineEx({p.x - t.x * r * 0.14f, p.y - t.y * r * 0.14f}, {p.x + t.x * r * 0.14f, p.y + t.y * r * 0.14f}, 3.0f,
                 withAlpha(teal, 0.85f));
    }
  }
  const float spin = -time * 0.6f * 57.3f;  // stone shell (octagon, lit from the top left)
  DrawPoly(c, 8, r + 3.5f, spin, ink);
  DrawPoly(c, 8, r, spin, stoneDk);
  DrawPoly({c.x - r * 0.08f, c.y - r * 0.08f}, 8, r * 0.88f, spin, stone);
  for (int i = 0; i < 8; ++i) {  // carved notches glowing at the corners
    float a = deg2rad(spin) + i * kPi / 4.0f;
    Vector2 p0{c.x + std::cos(a) * r * 0.9f, c.y + std::sin(a) * r * 0.9f};
    Vector2 p1{c.x + std::cos(a) * r * 0.68f, c.y + std::sin(a) * r * 0.68f};
    DrawLineEx(p0, p1, 3.0f, withAlpha(teal, 0.6f + 0.4f * pulse));
  }
  DrawCircleV(c, r * 0.62f, ink);
  glow(c, r * 0.58f, r * 0.58f, core, mixColor(teal, {20, 120, 110, 255}, 0.4f));
  spiral(c, r * 0.42f, 1.5f, 2.5f, withAlpha({20, 110, 100, 255}, 0.9f), time * 2.0f);
  DrawCircleV(c, r * 0.12f, WHITE);
  if (o.hp < 25.0f) {  // cracks leaking light
    Color k = mixColor(teal, WHITE, 0.5f);
    DrawLineEx({c.x - r * 0.95f, c.y - r * 0.2f}, {c.x - r * 0.5f, c.y + r * 0.1f}, 2.5f, k);
    DrawLineEx({c.x - r * 0.5f, c.y + r * 0.1f}, {c.x - r * 0.35f, c.y + r * 0.7f}, 2.5f, k);
  }
  if (o.hp < 12.0f) {
    Color k = mixColor(teal, WHITE, 0.5f);
    DrawLineEx({c.x + r * 0.9f, c.y - r * 0.4f}, {c.x + r * 0.55f, c.y - r * 0.05f}, 2.5f, k);
    DrawLineEx({c.x + r * 0.55f, c.y - r * 0.05f}, {c.x + r * 0.7f, c.y + r * 0.5f}, 2.5f, k);
  }
  if (o.flash > 0) DrawCircleV(c, r * 1.1f, withAlpha(WHITE, o.flash / 8.0f));
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

void drawStageThumb(int index, const StageDef& s, Rectangle area) {
  static std::vector<Layer> cache;
  if (index < 0) return;
  if (int(cache.size()) <= index) cache.resize(index + 1);
  Layer& L = cache[index];
  if (L.rt.id == 0 || L.w != area.width || L.h != area.height || std::fabs(L.scale - g_renderScale) > 0.01f) {
    if (L.rt.id != 0) UnloadRenderTexture(L.rt);
    L = bake(area.width, area.height, g_renderScale, [&] { drawStagePreview(s, {0, 0, area.width, area.height}, 2.0f); }, false);
  }
  drawBackdrop(L, area.x, area.y, area.y + area.height);
}
