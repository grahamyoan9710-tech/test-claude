// Stage backgrounds, platforms and projectiles.
#include "game/render.hpp"

namespace {
float hash(int i) {
  uint32_t x = uint32_t(i) * 2654435761u;
  x ^= x >> 13;
  x *= 0x5bd1e995u;
  x ^= x >> 15;
  return float(x & 0xFFFF) / 65535.0f;
}

void tri(Vector2 a, Vector2 b, Vector2 c, Color col) {
  float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  if (cross > 0) std::swap(b, c);
  DrawTriangle(a, b, c, col);
}
void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color col) {
  tri(a, b, c, col);
  tri(a, c, d, col);
}

// Silhouette of rolling mountains, scrolled by parallax.
void mountains(float baseY, float amp, float scroll, float freq, Color col, int seed) {
  const int step = 32;
  for (int x = -step; x < kScreenW + step; x += step) {
    auto h = [&](float px) {
      float u = (px + scroll) * freq;
      return baseY - amp * (0.55f * std::sin(u + seed) + 0.3f * std::sin(u * 2.3f + seed * 2) +
                            0.15f * std::sin(u * 5.1f + seed * 3));
    };
    float y0 = h(float(x)), y1 = h(float(x + step));
    quad({float(x), y0}, {float(x + step), y1}, {float(x + step), float(kScreenH)}, {float(x), float(kScreenH)}, col);
  }
}

void cloud(Vector2 c, float s, Color col) {
  DrawCircleV({c.x, c.y}, 30 * s, col);
  DrawCircleV({c.x + 34 * s, c.y + 6 * s}, 24 * s, col);
  DrawCircleV({c.x - 34 * s, c.y + 8 * s}, 22 * s, col);
  DrawCircleV({c.x + 12 * s, c.y - 16 * s}, 24 * s, col);
  DrawEllipse(int(c.x), int(c.y + 16 * s), 60 * s, 14 * s, col);
}

void star(Vector2 c, float rOut, float rIn, float rot, Color col) {
  Vector2 pts[10];
  for (int i = 0; i < 10; ++i) {
    float a = rot + i * kPi / 5.0f - kPi / 2.0f;
    float r = (i % 2 == 0) ? rOut : rIn;
    pts[i] = {c.x + std::cos(a) * r, c.y + std::sin(a) * r};
  }
  for (int i = 0; i < 10; ++i) tri(c, pts[i], pts[(i + 1) % 10], col);
}
}  // namespace

void drawBackground(const StageDef& stage, Vec2 cam, float zoom, float time) {
  (void)zoom;
  const float W = float(kScreenW), H = float(kScreenH);
  switch (stage.theme) {
    case StageTheme::Cosmic: {
      DrawRectangleGradientV(0, 0, kScreenW, kScreenH, {8, 4, 24, 255}, {44, 12, 70, 255});
      for (int i = 0; i < 5; ++i) {  // nebulae
        float x = std::fmod(hash(i * 7) * 1800.0f - cam.x * 0.04f + 2000.0f, 1800.0f) - 260.0f;
        float y = hash(i * 13) * H * 0.8f - cam.y * 0.03f;
        Color c = i % 2 ? Color{120, 40, 160, 255} : Color{40, 90, 180, 255};
        for (int k = 0; k < 10; ++k) DrawCircleV({x, y}, 240.0f - k * 22.0f, withAlpha(c, 0.025f + k * 0.006f));
      }
      for (int i = 0; i < 160; ++i) {  // stars
        float depth = 0.02f + hash(i * 3) * 0.08f;
        float x = std::fmod(hash(i) * 2600.0f - cam.x * depth + 5200.0f, 2600.0f) - 300.0f;
        float y = std::fmod(hash(i + 999) * 1400.0f - cam.y * depth + 2800.0f, 1400.0f) - 300.0f;
        float tw = 0.5f + 0.5f * std::sin(time * (1.0f + hash(i * 5) * 3.0f) + i);
        DrawCircleV({x, y}, 0.8f + hash(i * 11) * 1.8f, withAlpha(WHITE, 0.3f + 0.7f * tw));
      }
      Vector2 planet{W * 0.82f - cam.x * 0.02f, H * 0.78f - cam.y * 0.02f};
      DrawCircleV(planet, 170, {70, 40, 120, 255});
      DrawCircleV({planet.x - 30, planet.y - 30}, 150, {95, 60, 150, 255});
      DrawRing(planet, 220, 236, 200, 340, 48, withAlpha({200, 170, 255, 255}, 0.5f));
      break;
    }
    case StageTheme::Sunset: {
      DrawRectangleGradientV(0, 0, kScreenW, int(H * 0.62f), {60, 36, 110, 255}, {250, 140, 90, 255});
      DrawRectangle(0, int(H * 0.62f), kScreenW, kScreenH, {250, 140, 90, 255});
      Vector2 sun{W * 0.5f - cam.x * 0.03f, H * 0.6f - cam.y * 0.02f};
      for (int k = 0; k < 5; ++k) DrawCircleV(sun, 150.0f - k * 20.0f, withAlpha({255, 220, 140, 255}, 0.08f + k * 0.04f));
      DrawCircleV(sun, 70, {255, 236, 170, 255});
      mountains(H * 0.66f - cam.y * 0.05f, 70, cam.x * 0.08f, 0.006f, {150, 70, 110, 255}, 1);
      mountains(H * 0.76f - cam.y * 0.1f, 60, cam.x * 0.18f, 0.009f, {95, 45, 90, 255}, 4);
      mountains(H * 0.88f - cam.y * 0.18f, 40, cam.x * 0.3f, 0.013f, {55, 28, 62, 255}, 9);
      for (int i = 0; i < 4; ++i) {
        float x = std::fmod(hash(i * 17) * 1700.0f + time * 8.0f - cam.x * 0.06f + 3400.0f, 1700.0f) - 200.0f;
        float cy = 80.0f + i * 45.0f - cam.y * 0.04f;
        Color sky = mixColor({60, 36, 110, 255}, {250, 140, 90, 255}, cy / (H * 0.62f));
        cloud({x, cy}, 0.9f + hash(i) * 0.6f, mixColor(sky, {255, 200, 180, 255}, 0.5f));
      }
      break;
    }
    case StageTheme::Sky: {
      DrawRectangleGradientV(0, 0, kScreenW, kScreenH, {80, 150, 240, 255}, {200, 235, 255, 255});
      for (int i = 0; i < 3; ++i) {  // distant floating islands
        float x = std::fmod(hash(i * 29) * 1600.0f - cam.x * 0.07f + 3200.0f, 1600.0f) - 150.0f;
        float y = 200.0f + hash(i * 31) * 250.0f - cam.y * 0.05f + std::sin(time * 0.5f + i) * 6.0f;
        Color c{150, 185, 225, 255};
        DrawEllipse(int(x), int(y), 90, 18, c);
        tri({x - 80, y}, {x + 80, y}, {x, y + 90}, c);
      }
      for (int i = 0; i < 9; ++i) {
        float depth = 0.05f + hash(i * 3) * 0.2f;
        float x = std::fmod(hash(i * 19) * 2000.0f + time * (6.0f + 10.0f * depth) - cam.x * depth + 4000.0f, 2000.0f) - 250.0f;
        float y = 60.0f + hash(i * 23) * 500.0f - cam.y * depth;
        Color sky = mixColor({80, 150, 240, 255}, {200, 235, 255, 255}, y / H);
        cloud({x, y}, 0.7f + depth * 4.0f, mixColor(sky, WHITE, 0.55f + depth * 1.5f));
      }
      break;
    }
  }
}

void drawPlatforms(const Match& m, float time) {
  const StageTheme theme = m.stage().theme;
  for (const auto& p : m.platforms) {
    const Rect& r = p.rect;
    if (p.solid) {
      Color top, body, edge;
      switch (theme) {
        case StageTheme::Cosmic: top = {80, 92, 140, 255}; body = {46, 50, 86, 255}; edge = {90, 230, 255, 255}; break;
        case StageTheme::Sunset: top = {95, 170, 70, 255}; body = {120, 80, 55, 255}; edge = {60, 120, 50, 255}; break;
        default: top = {115, 205, 90, 255}; body = {150, 110, 72, 255}; edge = {80, 160, 70, 255}; break;
      }
      float depth = r.h + std::min(160.0f, r.w * 0.3f);
      Vector2 a{r.left(), r.top()}, b{r.right(), r.top()};
      Vector2 c{r.right() - r.w * 0.12f, r.top() + r.h}, d{r.left() + r.w * 0.12f, r.top() + r.h};
      Vector2 e{r.left() + r.w * 0.5f, r.top() + depth};
      quad(a, b, c, d, body);
      tri(d, c, e, shade(body, 0.8f));
      for (int i = 1; i < 4; ++i) {  // strata
        float y = r.top() + r.h * i / 4.0f;
        float inset = r.w * 0.12f * i / 4.0f;
        DrawLineEx({r.left() + inset, y}, {r.right() - inset, y}, 2, shade(body, 0.75f));
      }
      DrawRectangleRounded({r.x - 4, r.y - 4, r.w + 8, 16}, 0.5f, 6, top);
      if (theme == StageTheme::Cosmic) {
        float glow = 0.6f + 0.4f * std::sin(time * 2.0f);
        DrawLineEx({r.left(), r.top()}, {r.right(), r.top()}, 3, withAlpha(edge, glow));
        DrawLineEx(d, e, 2, withAlpha(edge, 0.5f * glow));
        DrawLineEx(c, e, 2, withAlpha(edge, 0.5f * glow));
        for (float x = r.left() + 40.0f; x < r.right() - 30.0f; x += 60.0f) {  // running lights
          float k = 0.5f + 0.5f * std::sin(time * 4.0f - x * 0.02f);
          DrawCircleV({x, r.top() + r.h * 0.55f}, 6, withAlpha(edge, 0.15f * k));
          DrawCircleV({x, r.top() + r.h * 0.55f}, 2.5f, withAlpha(edge, 0.4f + 0.6f * k));
        }
      } else {
        for (float x = r.left(); x < r.right(); x += 14.0f)  // grass tufts
          tri({x, r.top() + 2}, {x + 8, r.top() + 2}, {x + 4, r.top() - 5 - 3 * std::sin(x + time * 2)}, edge);
      }
    } else {
      Rectangle rr{r.x, r.y, r.w, r.h};
      switch (theme) {
        case StageTheme::Cosmic:
          DrawRectangleRounded(rr, 0.6f, 6, withAlpha({90, 200, 255, 255}, 0.45f));
          DrawRectangleRoundedLinesEx(rr, 0.6f, 6, 2, withAlpha({160, 240, 255, 255}, 0.9f));
          break;
        case StageTheme::Sunset:
          DrawRectangleRounded(rr, 0.4f, 6, {165, 110, 62, 255});
          DrawRectangleRoundedLinesEx(rr, 0.4f, 6, 2, {80, 45, 25, 255});
          for (float x = r.x + 30; x < r.right() - 10; x += 40) DrawLine(int(x), int(r.y + 2), int(x), int(r.bottom() - 2), {110, 70, 40, 255});
          break;
        default:
          for (float x = r.x + 10; x <= r.right() - 10; x += 22) DrawCircleV({x, r.y + 8}, 13, {245, 250, 255, 240});
          DrawRectangleRounded({r.x, r.y + 2, r.w, 16}, 0.8f, 6, {255, 255, 255, 250});
          break;
      }
    }
  }
}

void drawProjectiles(const Match& m, float time) {
  for (const auto& p : m.projectiles) {
    Vector2 c = V(p.pos);
    float r = p.radius;
    float fade = std::min(1.0f, p.life / 10.0f);
    switch (p.visual) {
      case ProjVisual::Fireball:
        DrawCircleV(c, r * 1.9f, withAlpha({255, 110, 30, 255}, 0.3f * fade));
        DrawCircleV(c, r * (1.0f + 0.1f * std::sin(time * 40.0f)), withAlpha({255, 130, 30, 255}, fade));
        DrawCircleV({c.x - p.vel.x * 0.3f, c.y - p.vel.y * 0.3f}, r * 0.55f, withAlpha({255, 235, 120, 255}, fade));
        break;
      case ProjVisual::Bolt: {
        Vector2 prev = {c.x - p.vel.x * 2.5f, c.y - p.vel.y * 2.5f};
        DrawCircleV(c, r * 1.6f, withAlpha({120, 220, 255, 255}, 0.3f * fade));
        for (int i = 1; i <= 4; ++i) {
          float k = i / 4.0f;
          Vector2 q{lerpf(prev.x, c.x, k), lerpf(prev.y, c.y, k) + (i < 4 ? (hash(m.frame * 7 + i) - 0.5f) * 16.0f : 0)};
          DrawLineEx(prev, q, 5, withAlpha({140, 230, 255, 255}, fade));
          DrawLineEx(prev, q, 2, withAlpha(WHITE, fade));
          prev = q;
        }
        DrawCircleV(c, r * 0.6f, withAlpha({255, 255, 200, 255}, fade));
        break;
      }
      case ProjVisual::Shuriken: {
        float rot = time * 900.0f;
        DrawPoly(c, 4, r * 1.3f, rot, withAlpha({60, 60, 75, 255}, fade));
        DrawPoly(c, 4, r * 1.3f, rot + 45.0f, withAlpha({90, 90, 110, 255}, fade));
        DrawCircleV(c, r * 0.3f, withAlpha({200, 200, 220, 255}, fade));
        break;
      }
      case ProjVisual::Star:
        DrawCircleV(c, r * 1.6f, withAlpha({255, 240, 140, 255}, 0.25f * fade));
        star(c, r * 1.25f, r * 0.55f, time * 6.0f, withAlpha({255, 170, 40, 255}, fade));
        star(c, r * 0.95f, r * 0.42f, time * 6.0f, withAlpha({255, 235, 90, 255}, fade));
        break;
    }
  }
}

void drawOrb(const Match& m, float time) {
  const Orb& o = m.orb;
  if (!o.active) return;
  if (o.life < 120 && (o.life / 6) % 2 == 0) return;  // about to vanish: blink
  Vector2 c = V(o.pos);
  float r = o.radius;
  float hue = std::fmod(time * 120.0f, 360.0f);
  for (int k = 0; k < 6; ++k)
    DrawCircleV(c, r * (2.4f - k * 0.2f), withAlpha(ColorFromHSV(std::fmod(hue + k * 40.0f, 360.0f), 0.7f, 1.0f), 0.08f));
  for (int i = 0; i < 8; ++i) {  // rotating rays
    float a = time * 1.5f + i * kPi / 4.0f;
    Vector2 tip{c.x + std::cos(a) * r * 2.6f, c.y + std::sin(a) * r * 2.6f};
    DrawLineEx(c, tip, 3, withAlpha(ColorFromHSV(std::fmod(hue + i * 45.0f, 360.0f), 0.6f, 1.0f), 0.35f));
  }
  DrawCircleV(c, r + 3, {30, 20, 40, 255});
  DrawCircleGradient(int(c.x), int(c.y), r, {255, 250, 220, 255}, ColorFromHSV(hue, 0.85f, 1.0f));
  float spin = time * 2.0f;
  for (int s = -1; s <= 1; s += 2) {  // the ball's two curved bands
    Vec2 off = rotate({s * r * 0.95f, 0.0f}, spin);
    DrawRing({c.x + off.x, c.y + off.y}, r * 0.78f, r * 0.9f, 0, 360, 32, withAlpha({40, 25, 60, 255}, 0.55f));
  }
  DrawCircleV(c, r * 0.45f, {255, 255, 255, 255});
  star(c, r * 0.42f, r * 0.18f, spin, ColorFromHSV(hue, 0.9f, 0.9f));
  if (o.hp < 25.0f) {  // cracks
    DrawLineEx({c.x - r * 0.6f, c.y - r * 0.2f}, {c.x - r * 0.1f, c.y + r * 0.1f}, 2, {40, 20, 50, 255});
    DrawLineEx({c.x - r * 0.1f, c.y + r * 0.1f}, {c.x + r * 0.2f, c.y + r * 0.6f}, 2, {40, 20, 50, 255});
  }
  if (o.hp < 12.0f) DrawLineEx({c.x + r * 0.6f, c.y - r * 0.5f}, {c.x + r * 0.1f, c.y - r * 0.05f}, 2, {40, 20, 50, 255});
  if (o.flash > 0) DrawCircleV(c, r, withAlpha(WHITE, o.flash / 8.0f));
}

void drawStagePreview(const StageDef& s, Rectangle area, float time) {
  Color top, bottom;
  switch (s.theme) {
    case StageTheme::Cosmic: top = {8, 4, 24, 255}; bottom = {44, 12, 70, 255}; break;
    case StageTheme::Sunset: top = {60, 36, 110, 255}; bottom = {250, 140, 90, 255}; break;
    default: top = {80, 150, 240, 255}; bottom = {200, 235, 255, 255}; break;
  }
  DrawRectangleGradientV(int(area.x), int(area.y), int(area.width), int(area.height), top, bottom);
  if (s.theme == StageTheme::Cosmic)
    for (int i = 0; i < 30; ++i)
      DrawCircleV({area.x + hash(i) * area.width, area.y + hash(i + 50) * area.height}, 1.2f,
                  withAlpha(WHITE, 0.5f + 0.5f * std::sin(time * 2 + i)));
  float sc = area.width / 1500.0f;
  Vec2 o{area.x + area.width * 0.5f, area.y + area.height * 0.62f};
  for (const auto& p : s.platforms) {
    Rect r = p.base;
    Rectangle rr{o.x + r.x * sc, o.y + r.y * sc, r.w * sc, std::max(3.0f, r.h * sc)};
    if (p.solid) {
      DrawRectangleRec(rr, s.theme == StageTheme::Cosmic ? Color{60, 66, 100, 255} : Color{130, 90, 60, 255});
      DrawRectangle(int(rr.x), int(rr.y), int(rr.width), 3, s.theme == StageTheme::Cosmic ? Color{90, 230, 255, 255} : Color{100, 190, 80, 255});
    } else {
      DrawRectangleRec(rr, {240, 240, 250, 220});
    }
  }
}
