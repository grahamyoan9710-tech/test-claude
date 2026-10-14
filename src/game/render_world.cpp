// Stage backgrounds, platforms, projectiles and the Clash Orb.
// Look: painterly skies, floating islands and ancient ruins carved with glowing teal runes.
// Static scenery is baked once per theme into textures (premultiplied alpha) and scrolled
// with parallax; only light things (particles, pulses, water) are drawn every frame.
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
void runeGlow(Vector2 c, float r, Color col, float a, float rot = 0.0f) {
  glow(c, r * 2.2f, col, 0.35f * a, 24);
  spiral(c, r, 1.6f, std::max(1.5f, r * 0.22f), withAlpha(col, a), rot);
}

// Stylised round tree.
void tree(Vector2 base, float s, Color trunk, Color dark, Color light) {
  DrawLineEx(base, {base.x + s * 0.1f, base.y - s * 1.1f}, s * 0.18f, trunk);
  for (int i = 0; i < 4; ++i)
    DrawCircleV({base.x + (i - 1.5f) * s * 0.32f, base.y - s * (1.15f + 0.25f * (i % 2))}, s * 0.42f, dark);
  for (int i = 0; i < 3; ++i)
    DrawCircleV({base.x + (i - 1.2f) * s * 0.3f, base.y - s * (1.3f + 0.22f * (i % 2))}, s * 0.3f, light);
}

// Zonai monolith: chamfered slab, lit left face, darker right face, carved glowing runes.
void monolith(Vector2 base, float w, float h, Color lit, Color dark, Color rune, float runeA) {
  const float x0 = base.x - w * 0.5f, x1 = base.x + w * 0.5f, top = base.y - h;
  gquad({x0, top + w * 0.3f}, {base.x, top}, {base.x, base.y}, {x0, base.y}, lit, lit, shade(lit, 0.75f),
        shade(lit, 0.75f));
  gquad({base.x, top}, {x1, top + w * 0.3f}, {x1, base.y}, {base.x, base.y}, dark, dark, shade(dark, 0.75f),
        shade(dark, 0.75f));
  if (runeA <= 0.0f) return;
  runeGlow({base.x, top + w * 0.75f}, w * 0.26f, rune, runeA);
  for (float y = top + w * 1.4f; y < base.y - w * 0.6f; y += w * 0.9f) {
    DrawRectangleRec({base.x - w * 0.16f, y, w * 0.32f, std::max(1.5f, w * 0.06f)}, withAlpha(rune, runeA * 0.7f));
    DrawCircleV({base.x, y + w * 0.4f}, std::max(1.2f, w * 0.06f), withAlpha(rune, runeA * 0.6f));
  }
}

// Floating sky island: grassy top, earthy underside tapering to a point, dangling roots,
// optional waterfall dissolving into mist and decorations (1 = trees, 2 = Zonai ruin).
void skyIsland(Vector2 c, float w, int seed, float fog, Color haze, bool fall, int deco) {
  auto F = [&](Color col, float a = 1.0f) { return withAlpha(mixColor(col, haze, fog), a); };
  constexpr int n = 12;
  const float depth = w * (0.55f + 0.3f * hash(seed));
  Vector2 top[n + 1], bot[n + 1];
  for (int i = 0; i <= n; ++i) {
    float u = float(i) / n;
    float x = c.x + (u - 0.5f) * w;
    top[i] = {x, c.y - std::sin(u * kPi) * w * 0.035f};
    float k = std::pow(std::sin(u * kPi), 0.6f);
    bot[i] = {x + (0.5f - u) * w * 0.2f * k, c.y + w * 0.04f + depth * k * (0.7f + 0.3f * hash(seed + i * 5))};
  }
  const float lw = std::max(1.0f, w * 0.012f);
  for (int i = 2; i < n - 1; ++i) {  // roots and vines
    Vector2 p[4] = {bot[i]};
    float len = depth * (0.25f + 0.35f * hash(seed + i * 11));
    for (int k = 1; k < 4; ++k)
      p[k] = {p[k - 1].x + (hash(seed + i * 7 + k) - 0.5f) * w * 0.06f, p[k - 1].y + len / 3.0f};
    stroke(p, 4, lw * 1.6f, lw * 0.6f, i % 3 ? F({98, 72, 58, 255}) : F({84, 140, 70, 255}));
  }
  if (fall) {  // waterfall from the right rim, fading into mist
    float x = lerpf(top[n - 2].x, top[n - 1].x, 0.5f), fw = std::max(3.0f, w * 0.045f);
    float y0 = top[n - 2].y + 2.0f, len = depth * 2.4f;
    Color water = F({236, 246, 255, 255});
    gquad({x - fw * 0.5f, y0}, {x + fw * 0.5f, y0}, {x + fw * 0.9f, y0 + len}, {x - fw * 0.9f, y0 + len}, water, water,
          withAlpha(water, 0.0f), withAlpha(water, 0.0f));
    DrawLineEx({x - fw * 0.15f, y0}, {x - fw * 0.15f, y0 + len * 0.6f}, std::max(1.0f, fw * 0.25f), F(WHITE, 0.7f));
    for (int k = 0; k < 5; ++k)
      glow({x + (hash(seed + k) - 0.5f) * fw * 3.0f, y0 + len * (0.55f + 0.1f * k)}, fw * (2.0f + k * 0.6f),
           F(WHITE), 0.35f);
  }
  for (int i = 0; i < n; ++i) {  // body: warm earth fading to cool rock at the tip
    float u0 = float(i) / n, u1 = float(i + 1) / n;
    Color e0 = F(shade({196, 150, 100, 255}, 1.08f - 0.3f * u0)), e1 = F(shade({196, 150, 100, 255}, 1.08f - 0.3f * u1));
    Color r0 = F(shade({110, 86, 88, 255}, 1.05f - 0.3f * u0)), r1 = F(shade({110, 86, 88, 255}, 1.05f - 0.3f * u1));
    gquad(top[i], top[i + 1], bot[i + 1], bot[i], e0, e1, r1, r0);
  }
  for (int s = 1; s <= 2; ++s)  // strata
    for (int i = 1; i < n - 1; ++i) {
      float t = 0.18f + 0.22f * s;
      Vector2 a{lerpf(top[i].x, bot[i].x, t), lerpf(top[i].y, bot[i].y, t)};
      Vector2 b{lerpf(top[i + 1].x, bot[i + 1].x, t), lerpf(top[i + 1].y, bot[i + 1].y, t)};
      DrawLineEx(a, b, lw * 1.2f, F({120, 84, 64, 255}, 0.55f));
    }
  if (deco & 2) monolith({c.x + w * 0.2f, top[8].y + 2}, w * 0.09f, w * 0.34f, F({176, 170, 150, 255}),
                         F({120, 118, 112, 255}), F({90, 255, 210, 255}), 1.0f - fog);
  if (deco & 1) {
    tree({c.x - w * 0.22f, top[3].y + 2}, w * 0.16f, F({96, 70, 54, 255}), F({62, 128, 70, 255}), F({116, 180, 84, 255}));
    tree({c.x - w * 0.05f, top[5].y + 2}, w * 0.11f, F({96, 70, 54, 255}), F({52, 116, 66, 255}), F({104, 170, 80, 255}));
  }
  for (int i = 0; i < n; ++i)  // grass cap with a lit rim and moss drips
    gquad({top[i].x, top[i].y - lw * 3}, {top[i + 1].x, top[i + 1].y - lw * 3}, {top[i + 1].x, top[i + 1].y + lw * 3},
          {top[i].x, top[i].y + lw * 3}, F({168, 222, 112, 255}), F({168, 222, 112, 255}), F({92, 164, 72, 255}),
          F({92, 164, 72, 255}));
  for (int i = 1; i < n; ++i) ellipse({top[i].x, top[i].y + lw * 3}, lw * 2.5f, lw * (2.0f + 3.0f * hash(seed + i)), F({92, 164, 72, 255}));
}

// Glowing lightroot: knotted roots gripping the ground and a luminous bulb on top.
void lightroot(Vector2 base, float s, Color rootCol, Color glowCol, Color bulb) {
  glow({base.x, base.y - s * 1.6f}, s * 2.6f, glowCol, 0.32f);
  for (int k = 0; k < 6; ++k) {
    float side = (k % 2 ? 1.0f : -1.0f) * (0.3f + 0.25f * k);
    Vector2 p[4] = {{base.x, base.y - s * 1.45f},
                    {base.x + side * s * 0.25f, base.y - s * 0.9f},
                    {base.x + side * s * 0.55f, base.y - s * 0.35f},
                    {base.x + side * s * 0.8f, base.y}};
    stroke(p, 4, s * 0.16f, s * 0.07f, rootCol);
  }
  glow({base.x, base.y - s * 1.65f}, s * 0.7f, bulb, 0.6f);
  ellipse({base.x, base.y - s * 1.62f}, s * 0.3f, s * 0.26f, bulb);
  tri({base.x - s * 0.18f, base.y - s * 1.75f}, {base.x + s * 0.18f, base.y - s * 1.75f}, {base.x, base.y - s * 2.05f}, bulb);
  ellipse({base.x - s * 0.08f, base.y - s * 1.7f}, s * 0.1f, s * 0.08f, WHITE);
}

// Teal crystal: two-tone prism with a bright edge and a halo.
void crystal(Vector2 base, float h, float w, float lean, Color col, float glowA) {
  Vector2 tip{base.x + lean * h, base.y - h};
  Vector2 l{base.x - w * 0.5f + lean * h * 0.15f, base.y - h * 0.18f}, r{base.x + w * 0.5f + lean * h * 0.15f, base.y - h * 0.18f};
  glow({lerpf(base.x, tip.x, 0.5f), base.y - h * 0.5f}, h * 0.7f, col, glowA, 24);
  gquad(base, l, tip, {lerpf(base.x, tip.x, 0.3f), base.y - h * 0.5f}, shade(col, 0.95f), shade(col, 0.95f),
        mixColor(col, WHITE, 0.6f), mixColor(col, WHITE, 0.3f));
  gquad(base, r, tip, {lerpf(base.x, tip.x, 0.3f), base.y - h * 0.5f}, shade(col, 0.5f), shade(col, 0.5f),
        mixColor(col, WHITE, 0.2f), shade(col, 0.7f));
  DrawLineEx(l, tip, std::max(1.0f, w * 0.06f), withAlpha(mixColor(col, WHITE, 0.7f), 0.9f));
}

// Gloom tendril: writhing dark-red stroke rising from the ground.
void tendril(Vector2 base, float len, float thick, int seed, Color core, Color rim) {
  Vector2 p[8];
  for (int i = 0; i < 8; ++i) {
    float t = i / 7.0f;
    p[i] = {base.x + std::sin(t * 5.0f + seed) * len * 0.12f * t, base.y - len * t};
  }
  stroke(p, 8, thick * 1.9f, thick * 0.6f, withAlpha(rim, 0.35f));
  stroke(p, 8, thick, thick * 0.25f, core);
}

Art g_art[kStageThemeCount];

// ---- Îles Célestes: bright day, sea of clouds, distant islands -------------------------------
constexpr Color kSkyHaze{206, 228, 243, 255}, kSea{214, 228, 246, 255}, kSeaDeep{190, 208, 236, 255};

void bakeSky(Art& a) {
  const Color haze = kSkyHaze;
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, {38, 100, 192, 255}}, {0.36f, {88, 156, 226, 255}}, {0.64f, {170, 210, 240, 255}},
                         {0.8f, haze}, {1.0f, {228, 238, 246, 255}}});
    const Vector2 sun{1070, 150};
    glow(sun, 700, 560, withAlpha({255, 246, 214, 255}, 0.55f), withAlpha({255, 246, 214, 255}, 0.0f));
    glow(sun, 190, {255, 252, 236, 255}, 0.75f);
    DrawCircleV(sun, 36, {255, 254, 246, 255});
    for (int i = 0; i < 7; ++i) {  // far cloud towers on the horizon, lost in the haze
      float x = 60 + i * 210 + hash(i * 3) * 80, y = 600 + hash(i * 5) * 40;
      cumulus({x, y}, 170 + hash(i) * 120, 110 + hash(i * 7) * 90, i * 31, mixColor(haze, {160, 190, 226, 255}, 0.4f),
              mixColor(haze, WHITE, 0.3f), withAlpha(WHITE, 0.35f), 0.5f);
    }
    skyIsland({200, 360}, 96, 3, 0.7f, haze, true, 1);
    skyIsland({540, 270}, 56, 8, 0.78f, haze, false, 0);
    skyIsland({880, 410}, 140, 5, 0.64f, haze, true, 2);
    skyIsland({1260, 320}, 80, 11, 0.74f, haze, true, 0);
    for (int i = 0; i < 8; ++i) {  // light shafts toward the lower left
      float ang = 1.95f + i * 0.13f + hash(i * 13) * 0.05f, sp = 0.02f + 0.03f * hash(i * 17);
      Vector2 p1{sun.x + std::cos(ang - sp) * 1700, sun.y + std::sin(ang - sp) * 1700};
      Vector2 p2{sun.x + std::cos(ang + sp) * 1700, sun.y + std::sin(ang + sp) * 1700};
      Color c{255, 250, 228, 255};
      gtri(sun, p1, p2, withAlpha(c, 0.13f), withAlpha(c, 0.0f), withAlpha(c, 0.0f));
    }
  });
  // Nearer islands with trees, ruins and waterfalls, veiled by a few wisps.
  a.near = bake(1800, 560, 1.25f, [&] {
    const Color blue{176, 204, 236, 255};
    skyIsland({390, 200}, 250, 21, 0.42f, blue, true, 3);
    skyIsland({1290, 140}, 190, 33, 0.5f, blue, true, 2);
    skyIsland({860, 90}, 56, 41, 0.6f, blue, false, 0);
    skyIsland({1640, 330}, 76, 47, 0.58f, blue, true, 1);
    for (int i = 0; i < 6; ++i)  // drifting rocks
      skyIsland({100 + i * 300 + hash(i * 3) * 100, 50 + hash(i * 9) * 360}, 16 + hash(i) * 14, 60 + i, 0.6f, blue, false, 0);
    const float wx[3] = {300, 980, 1420}, wy[3] = {430, 300, 470};
    for (int i = 0; i < 3; ++i)
      cumulus({wx[i], wy[i]}, 220, 70, 200 + i * 13, withAlpha({214, 230, 248, 255}, 0.7f), withAlpha({248, 251, 255, 255}, 0.75f),
              withAlpha(WHITE, 0.4f), 0.5f);
  });
  // Top of the sea of clouds (wraps, drifts); its solid body is a flat gradient drawn per frame.
  a.mid = bake(1600, 260, 2.0f, [&] {
    const Color sh{168, 194, 228, 255}, body{242, 247, 255, 255}, hi{255, 255, 250, 200};
    for (int i = 0; i < 10; ++i) {
      float x = i * 160 + hash(i * 3) * 40, y = 150 + hash(i * 5) * 30;
      wrapX(x, 230, 1600, [&](float xx) {
        cumulus({xx, y}, 260, 120, i * 17, mixColor(sh, haze, 0.5f), mixColor(body, haze, 0.4f), withAlpha(WHITE, 0.4f), 0.5f);
      });
    }
    vgrad(0, 210, 1600, 50, mixColor(body, sh, 0.3f), kSea);
    for (int i = 0; i < 8; ++i) {
      float x = i * 200 + hash(i * 11) * 60, y = 225 + hash(i * 7) * 20;
      wrapX(x, 260, 1600, [&](float xx) { cumulus({xx, y}, 300, 140, 100 + i * 23, sh, body, hi, 0.5f); });
    }
    vgrad(0, 228, 1600, 32, withAlpha(kSea, 0.0f), kSea);
  });
}

// ---- Sanctuaire: golden hour over a ruined Zonai temple ----------------------------------------
constexpr Color kDusk{238, 172, 146, 255}, kGround{96, 70, 72, 255};

void bakeSanctuary(Art& a) {
  const Color haze = kDusk;
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, {46, 54, 116, 255}}, {0.26f, {106, 88, 150, 255}}, {0.46f, {214, 134, 124, 255}},
                         {0.6f, {250, 186, 128, 255}}, {0.72f, {255, 216, 156, 255}}, {1.0f, {255, 228, 182, 255}}});
    const Vector2 sun{680, 470};
    glow(sun, 760, 520, withAlpha({255, 196, 120, 255}, 0.5f), withAlpha({255, 196, 120, 255}, 0.0f));
    for (int i = 0; i < 16; ++i) {  // rays fanning up from the sun
      float ang = kPi + (i + 0.5f) / 16.0f * kPi, sp = 0.025f + 0.03f * hash(i * 7);
      Vector2 p1{sun.x + std::cos(ang - sp) * 900, sun.y + std::sin(ang - sp) * 900};
      Vector2 p2{sun.x + std::cos(ang + sp) * 900, sun.y + std::sin(ang + sp) * 900};
      Color c{255, 226, 170, 255};
      gtri(sun, p1, p2, withAlpha(c, 0.12f), withAlpha(c, 0.0f), withAlpha(c, 0.0f));
    }
    glow(sun, 250, {255, 232, 176, 255}, 0.65f);
    DrawCircleV(sun, 58, {255, 246, 222, 255});
    const Vector2 cl[6] = {{180, 110}, {760, 70}, {1180, 160}, {420, 250}, {1000, 300}, {120, 360}};
    for (int i = 0; i < 6; ++i) {  // evening clouds lit from below
      float t = clampf((cl[i].y - 60) / 330.0f, 0, 1);
      stratus(cl[i], 300 + hash(i) * 280, 26 + hash(i * 5) * 16, i * 19, mixColor({100, 80, 138, 255}, {204, 124, 128, 255}, t),
              mixColor({248, 166, 140, 255}, {255, 224, 170, 255}, t));
    }
    skyIsland({250, 230}, 80, 7, 0.62f, {200, 140, 150, 255}, true, 2);
    skyIsland({1130, 300}, 54, 13, 0.66f, {214, 150, 146, 255}, false, 0);
    range(1360, 530, 80, 3, {226, 160, 150, 255}, {250, 204, 166, 255}, 800, {255, 226, 180, 255}, false);
    rlPushMatrix();  // nearer ranges, hazier at their feet
    rlTranslatef(0, 440, 0);
    range(1360, 130, 110, 9, {192, 126, 136, 255}, {238, 176, 146, 255}, 360, {255, 210, 160, 255}, false);
    range(1360, 220, 90, 17, {144, 92, 118, 255}, {204, 136, 132, 255}, 360, {255, 196, 150, 255}, false);
    rlPopMatrix();
  });
  // The ruined temple: tiers, a ring gate framing the sun, broken pillars.
  a.near = bake(2000, 600, 1.25f, [&] {
    auto F = [&](Color c, float k = 0.35f) { return mixColor(c, haze, k); };
    const Color stone = F({150, 104, 116, 255}), dark = F({112, 74, 96, 255}), lit = F({236, 170, 134, 255});
    const Color rune{100, 255, 214, 255};
    for (float x = 300; x < 1700; x += 8) {  // mound
      float u = (x - 300) / 1400.0f, u1 = (x + 8 - 300) / 1400.0f;
      float y0 = 600 - 110 * std::pow(std::sin(u * kPi), 0.5f), y1 = 600 - 110 * std::pow(std::sin(u1 * kPi), 0.5f);
      gquad({x, y0}, {x + 8, y1}, {x + 8, 600}, {x, 600}, F({128, 84, 102, 255}), F({128, 84, 102, 255}), F({160, 110, 116, 255}, 0.5f),
            F({160, 110, 116, 255}, 0.5f));
    }
    struct Tier {
      float x0, x1, y0, y1;
    };
    const Tier tiers[3] = {{690, 1310, 470, 540}, {770, 1230, 420, 470}, {850, 1150, 380, 420}};
    for (const Tier& t : tiers) {
      vgrad(t.x0, t.y0, t.x1 - t.x0, t.y1 - t.y0, stone, dark);
      DrawRectangleRec({t.x0, t.y0, t.x1 - t.x0, 5}, lit);
      for (float x = t.x0 + 40; x < t.x1 - 20; x += 80) DrawRectangleRec({x, t.y0 + 12, 3, t.y1 - t.y0 - 18}, withAlpha(dark, 0.8f));
    }
    for (int k = -2; k <= 2; ++k) runeGlow({1000 + k * 110.0f, 505}, 12, rune, 0.55f);
    // ring gate on two posts
    DrawRectangleRec({895, 300, 34, 82}, stone);
    DrawRectangleRec({1071, 300, 34, 82}, dark);
    DrawRing({1000, 240}, 96, 132, 0, 360, 72, stone);
    DrawRing({1000, 240}, 96, 132, 200, 340, 48, mixColor(stone, lit, 0.35f));
    DrawRing({1000, 240}, 92, 98, 0, 360, 72, withAlpha(lit, 0.8f));
    DrawRing({1000, 240}, 112, 115, 0, 360, 72, withAlpha(rune, 0.45f));
    for (int k = 0; k < 8; ++k) {
      float ang = k * kPi / 4.0f;
      Vector2 p{1000 + std::cos(ang) * 114, 240 + std::sin(ang) * 114};
      DrawCircleV(p, 13, dark);
      runeGlow(p, 8, rune, 0.7f, ang);
    }
    // broken pillars and floating fragments
    const float px[4] = {540, 640, 1360, 1470}, ph[4] = {250, 150, 190, 270};
    for (int k = 0; k < 4; ++k) {
      float x = px[k], y = 560 - ph[k] + 10 * (k == 1 || k == 2);
      vgrad(x - 24, y, 48, 560 - y, stone, dark);
      DrawRectangleRec({x - 24, y, 10, 560 - y}, withAlpha(lit, 0.5f));
      quad({x - 24, y}, {x + 24, y}, {x + 24, y - 10 - 20 * hash(k)}, {x - 4, y - 24 * hash(k * 3)}, stone);
      DrawRectangleRec({x - 32, 540, 64, 20}, dark);
      runeGlow({x, y + 50}, 9, rune, 0.5f);
    }
    const Vector2 fr[3] = {{800, 150}, {1210, 110}, {1150, 330}};
    for (int k = 0; k < 3; ++k) {  // floating rubble
      float s = 14 + hash(k * 7) * 10;
      Vector2 p = fr[k];
      quad({p.x - s, p.y - s * 0.5f}, {p.x + s * 0.8f, p.y - s * 0.7f}, {p.x + s, p.y + s * 0.4f}, {p.x - s * 0.6f, p.y + s * 0.6f}, stone);
      quad({p.x - s, p.y - s * 0.5f}, {p.x + s * 0.8f, p.y - s * 0.7f}, {p.x + s * 0.7f, p.y - s * 0.4f}, {p.x - s * 0.8f, p.y - s * 0.2f}, lit);
      tri({p.x - s * 0.6f, p.y + s * 0.6f}, {p.x + s, p.y + s * 0.4f}, {p.x + s * 0.1f, p.y + s * 1.6f}, dark);
    }
  });
  // Near monoliths and grass framing the arena.
  a.extra = bake(1600, 520, 1.25f, [&] {
    const Color lit{150, 96, 102, 255}, dark{84, 52, 72, 255}, rune{110, 255, 220, 255};
    const float mx[3] = {240, 1360, 1530}, mh[3] = {360, 300, 190}, mw[3] = {76, 64, 50};
    for (int k = 0; k < 3; ++k)
      wrapX(mx[k], 60, 1600, [&](float x) {
        glow({x, 520 - mh[k] * 0.5f}, mh[k] * 0.7f, {255, 200, 150, 255}, 0.18f);
        monolith({x, 520}, mw[k], mh[k], lit, dark, rune, 0.9f);
      });
    vgrad(0, 506, 1600, 14, kGround, kGround);
    for (int i = 0; i < 70; ++i) {  // grass tufts
      float x = hash(i * 3) * 1600, h = 20 + hash(i * 7) * 40;
      wrapX(x, 20, 1600, [&](float xx) {
        for (int b = -2; b <= 2; ++b)
          tri({xx + b * 5.0f, 520}, {xx + b * 5.0f + 6, 520}, {xx + b * 9.0f, 520 - h * (1.0f - std::fabs(float(b)) * 0.2f)},
              b % 2 ? Color{92, 70, 70, 255} : Color{118, 90, 76, 255});
      });
    }
  });
}

// ---- Profondeurs: dark abyss, lightroots, teal crystals and gloom ------------------------------
constexpr Color kAbyss{18, 34, 48, 255};
constexpr Color kRockLow{30, 14, 30, 255};
constexpr float kRootX = 300.0f, kRootS = 70.0f;  // the great lightroot of the near layer

// Ground line of the near layer: rocky outcrops, low in the middle of the arena.
float depthsGround(float px) {
  float u = px / 1800.0f * 2.0f * kPi;
  return 480 - 90 * std::cos(u) - 40 * std::sin(u * 3.0f + 1.0f) - 30 * std::sin(u * 5.0f) - 14 * std::sin(u * 13.0f);
}

void bakeDepths(Art& a) {
  const Color haze = kAbyss, teal{90, 240, 210, 255}, bulb{226, 255, 180, 255}, lightG{150, 255, 170, 255};
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, {4, 6, 14, 255}}, {0.3f, {8, 16, 30, 255}}, {0.55f, {16, 40, 52, 255}},
                         {0.78f, {18, 26, 40, 255}}, {1.0f, {46, 10, 26, 255}}});
    glow({680, 420}, 640, 420, withAlpha({60, 170, 150, 255}, 0.3f), withAlpha({60, 170, 150, 255}, 0.0f));
    for (int k = 0; k < 2; ++k) {  // colossal cave pillars
      float x = k ? 1200.0f : 150.0f;
      for (float y = 0; y < 800; y += 10) {
        auto half = [](float py) {
          return 46 + 74 * std::pow(std::fabs(py / 800.0f - 0.45f) / 0.55f, 1.6f) + 6 * std::sin(py * 0.031f);
        };
        float wv = half(y), wv1 = half(y + 10);
        Color c = mixColor({10, 16, 28, 255}, haze, 0.25f + 0.25f * std::sin(y / 800.0f * kPi));
        gquad({x - wv, y}, {x + wv, y}, {x + wv1, y + 10}, {x - wv1, y + 10}, c, shade(c, 0.7f), shade(c, 0.7f), c);
      }
    }
    glow({680, 400}, 150, lightG, 0.22f);  // a great lightroot glowing somewhere far behind
    for (int i = 0; i < 6; ++i)  // faraway lightroot glows
      glow({60 + i * 250 + hash(i * 3) * 80, 500 + hash(i * 5) * 70}, 50 + hash(i) * 40, lightG, 0.25f);
    range(1360, 640, 70, 5, {12, 20, 32, 255}, {36, 12, 26, 255}, 800, {70, 170, 150, 255}, true);
    const float fm[3] = {380, 930, 1110};
    for (int k = 0; k < 3; ++k)  // far Zonai monoliths
      monolith({fm[k], 640 - 70 * ridge(fm[k], 1360, 5) + 10}, 22 + 6 * k, 90 + 30 * hash(k), {26, 38, 54, 255}, {16, 24, 38, 255}, teal, 0.4f);
    rlPushMatrix();  // nearer ridges dotted with lightroots
    rlTranslatef(0, 400, 0);
    range(1360, 170, 110, 21, {18, 30, 44, 255}, {30, 14, 30, 255}, 400, {80, 200, 180, 255}, true);
    for (int i = 0; i < 4; ++i) {
      float x = 120 + i * 330 + hash(i * 7) * 100, y = 170 - 110 * ridge(x, 1360, 21) + 6;
      lightroot({x, y}, 16, {14, 22, 30, 255}, lightG, bulb);
      for (int c = 0; c < 3; ++c) crystal({x + 40 + c * 14.0f, y + 6}, 20 + c * 9.0f, 8, 0.15f * (c - 1), teal, 0.2f);
    }
    rlPopMatrix();
    for (int i = 0; i < 42; ++i) {  // stalactites
      float x = i * 33 + hash(i * 3) * 20, len = 40 + hash(i * 7) * 170 * (0.5f + 0.5f * std::fabs(std::sin(i * 0.7f)));
      float hw = 12 + hash(i * 11) * 22;
      gtri({x - hw, -2}, {x + hw, -2}, {x + (hash(i) - 0.5f) * 10, len}, {6, 8, 16, 255}, {6, 8, 16, 255}, {16, 26, 38, 255});
    }
    for (int i = 0; i < 9; ++i) {  // roots hanging from the vault, glowing at the tips
      float x = 90 + i * 140 + hash(i * 23) * 80, len = 90 + hash(i * 29) * 200;
      Vector2 p[6];
      for (int k = 0; k < 6; ++k) p[k] = {x + std::sin(k * 1.3f + i) * 1.6f * k, len * k / 5.0f - 4};
      stroke(p, 6, 5.0f, 1.5f, {10, 16, 26, 255});
      glow(p[5], 9, teal, 0.5f);
      dot(p[5], 1.8f, {200, 255, 230, 255});
    }
  });
  // Gloom pool at the bottom: red glow and writhing tendrils.
  a.extra = bake(1600, 260, 1.0f, [&] {
    vgrad(0, 40, 1600, 220, withAlpha({90, 10, 30, 255}, 0.0f), {74, 8, 26, 255});
    for (int i = 0; i < 22; ++i) {
      float x = i * 73 + hash(i * 5) * 40, len = 70 + hash(i * 3) * 150;
      wrapX(x, 40, 1600, [&](float xx) { tendril({xx, 262}, len, 7 + hash(i) * 7, i, {26, 4, 12, 255}, {220, 30, 70, 255}); });
    }
    for (int i = 0; i < 12; ++i) {
      float x = hash(i * 13) * 1600, y = 200 + hash(i * 17) * 50;
      wrapX(x, 20, 1600, [&](float xx) {
        glow({xx, y}, 18, {255, 40, 80, 255}, 0.5f, 16);
        ellipse({xx, y}, 6, 3, {255, 120, 140, 255});
      });
    }
  });
  // Near: a great lightroot, crystal spires and a Zonai monolith.
  a.near = bake(1800, 660, 1.25f, [&] {  // 660: below the lowest point of depthsGround (~642)
    const Color rock{20, 28, 42, 255}, rockLow = kRockLow;
    auto ground = depthsGround;
    for (float x = 0; x < 1800; x += 8) {
      float y0 = ground(x), y1 = ground(x + 8);
      gquad({x, y0}, {x + 8, y1}, {x + 8, 660}, {x, 660}, rock, rock, rockLow, rockLow);
      DrawLineEx({x, y0}, {x + 8, y1}, 2.0f, withAlpha(teal, 0.35f));
    }
    lightroot({kRootX, ground(kRootX) + 10}, kRootS, {16, 26, 34, 255}, lightG, bulb);
    for (int c = 0; c < 6; ++c) {
      float x = 1330 + c * 34.0f + hash(c) * 10;
      crystal({x, ground(x) + 14}, 70 + hash(c * 5) * 150, 30 + hash(c * 7) * 16, (hash(c * 9) - 0.5f) * 0.4f, teal, 0.25f);
    }
    monolith({1660, ground(1660) + 12}, 70, 260, {44, 52, 70, 255}, {26, 30, 46, 255}, teal, 0.9f);
  });
}

const Art& themeArt(StageTheme theme) {
  ensureParticleSprite();
  Art& a = g_art[int(theme) % kStageThemeCount];
  if (!a.ready) {
    a.ready = true;
    switch (theme) {
      case StageTheme::Cosmic: bakeDepths(a); break;
      case StageTheme::Sunset: bakeSanctuary(a); break;
      case StageTheme::Sky: bakeSky(a); break;
      default: stageart::bake(theme, a); break;  // the anime worlds (render_stages.cpp)
    }
  }
  return a;
}


// ---- Platforms ---------------------------------------------------------------------------------
struct PlatLook {
  Color capHi, cap, capLow, faceTop, faceLow, under, tip, line, rune;
};
PlatLook platLook(StageTheme th) {
  switch (th) {
    case StageTheme::Cosmic:
      return {{150, 255, 220, 255}, {70, 84, 112, 255}, {40, 46, 70, 255}, {62, 70, 98, 255}, {40, 44, 68, 255},
              {36, 40, 62, 255}, {12, 14, 26, 255}, {28, 30, 50, 255}, {90, 240, 210, 255}};
    case StageTheme::Sunset:
      return {{250, 238, 204, 255}, {222, 204, 166, 255}, {150, 124, 100, 255}, {204, 180, 146, 255}, {160, 132, 114, 255},
              {170, 140, 122, 255}, {96, 72, 86, 255}, {140, 112, 96, 255}, {100, 250, 214, 255}};
    default:
      return {{178, 232, 118, 255}, {110, 188, 76, 255}, {74, 136, 60, 255}, {206, 162, 106, 255}, {162, 114, 78, 255},
              {150, 108, 78, 255}, {86, 68, 78, 255}, {130, 90, 64, 255}, {90, 250, 210, 255}};
  }
}

// Waterfall pouring off an island's rim, streaming and dissolving into mist.
void waterfall(float x, float y0, float len, float w, float time) {
  const Color water{226, 242, 255, 255};
  gquad({x - w * 0.5f, y0}, {x + w * 0.5f, y0}, {x + w * 0.8f, y0 + len}, {x - w * 0.8f, y0 + len}, withAlpha(water, 0.92f),
        withAlpha(water, 0.92f), withAlpha(water, 0.0f), withAlpha(water, 0.0f));
  for (int k = 0; k < 6; ++k) {  // streaks running down
    float t = wrapf(time * 0.7f + k / 6.0f, 1.0f);
    float y = y0 + t * len, sx = x + (hash(k * 3) - 0.5f) * w * 0.6f;
    DrawLineEx({sx, y}, {sx, y + len * 0.12f}, 2.5f, withAlpha(WHITE, 0.7f * (1.0f - t)));
  }
  for (int k = 0; k < 5; ++k) {  // mist
    float p = 0.5f + 0.5f * std::sin(time * 1.4f + k * 1.7f);
    glow({x + (k - 2) * w * 0.9f, y0 + len * (0.72f + 0.06f * (k % 3))}, w * (2.0f + 0.6f * p), WHITE, 0.22f, 20);
  }
}

void drawSolid(const Rect& r, StageTheme th, float time, int seed) {
  const PlatLook c = platLook(th);
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h, cx = L + w * 0.5f;
  const float inset = w * 0.05f, depth = h + std::min(170.0f, w * 0.32f);
  constexpr int kMaxN = 24;
  const int n = clampf(int(w / 50.0f), 6, kMaxN - 1);
  Vector2 bot[kMaxN + 1];
  for (int i = 0; i <= n; ++i) {
    float u = float(i) / n, k = std::pow(std::sin(u * kPi), 0.7f);
    float x = lerpf(L + inset, R - inset, u);
    bot[i] = {x + (0.5f - u) * w * 0.12f * k, T + h + (depth - h) * k * (0.78f + 0.22f * hash(seed + i * 13))};
  }
  // Hanging roots and vines (behind the rock).
  if (th != StageTheme::Cosmic)
    for (int i = 1; i < n; ++i) {
      if (hash(seed + i * 17) < 0.35f) continue;
      bool vine = hash(seed + i * 29) < 0.4f;
      float len = (depth - h) * (0.35f + 0.5f * hash(seed + i * 5));
      Vector2 p[5] = {bot[i]};
      for (int k = 1; k < 5; ++k) {
        float sway = std::sin(time * 1.2f + i + k * 0.6f) * k * 1.2f;
        p[k] = {p[k - 1].x + (hash(seed + i * 7 + k) - 0.5f) * 10.0f + sway, p[k - 1].y + len / 4.0f};
      }
      stroke(p, 5, vine ? 3.0f : 4.5f, 1.5f, vine ? Color{78, 140, 66, 255} : Color{96, 70, 56, 255});
      if (vine)
        for (int k = 1; k < 5; ++k) ellipse({p[k].x + (k % 2 ? 3.0f : -3.0f), p[k].y}, 3.5f, 2.2f, {104, 170, 76, 255});
    }
  if (th == StageTheme::Sky && w >= 500.0f) waterfall(R - inset - 6.0f, T + h * 0.5f, depth + 180.0f, 18.0f, time);

  // Underside.
  if (th == StageTheme::Sunset) {  // inverted ziggurat of carved blocks, then a rocky tip
    float y = T + h, steps = 3, sh = (depth - h) * 0.6f / steps;
    for (int s = 0; s < 3; ++s) {
      float iw = (w - inset * 2) * (0.86f - 0.2f * s);
      Color a = shade(c.under, 1.0f - 0.12f * s), b = shade(c.under, 0.86f - 0.12f * s);
      vgrad(cx - iw * 0.5f, y, iw, sh, a, b);
      DrawRectangleRec({cx - iw * 0.5f, y + sh - 3, iw, 3}, withAlpha(c.tip, 0.5f));
      for (float x = cx - iw * 0.5f + 50; x < cx + iw * 0.5f - 20; x += 70) DrawRectangleRec({x, y + 4, 2.5f, sh - 8}, withAlpha(c.line, 0.7f));
      y += sh;
    }
    float iw = (w - inset * 2) * 0.3f;
    for (int i = 0; i < 6; ++i) {
      float u0 = i / 6.0f, u1 = (i + 1) / 6.0f;
      float y0 = y + (depth - h) * 0.5f * std::sin(u0 * kPi), y1 = y + (depth - h) * 0.5f * std::sin(u1 * kPi);
      gquad({cx - iw * 0.5f + iw * u0, y}, {cx - iw * 0.5f + iw * u1, y}, {cx - iw * 0.5f + iw * u1, y1},
            {cx - iw * 0.5f + iw * u0, y0}, c.under, c.under, c.tip, c.tip);
    }
    runeGlow({cx, T + h + sh * 0.5f}, 9, c.rune, 0.55f + 0.25f * std::sin(time * 2.0f), time * 0.3f);
  } else {
    for (int i = 0; i < n; ++i) {
      float u0 = float(i) / n, u1 = float(i + 1) / n;
      float k0 = (bot[i].y - T - h) / std::max(1.0f, depth - h), k1 = (bot[i + 1].y - T - h) / std::max(1.0f, depth - h);
      Color a0 = shade(c.under, 1.1f - 0.3f * u0), a1 = shade(c.under, 1.1f - 0.3f * u1);
      gquad({lerpf(L + inset, R - inset, u0), T + h}, {lerpf(L + inset, R - inset, u1), T + h}, bot[i + 1], bot[i], a0, a1,
            mixColor(a1, c.tip, k1), mixColor(a0, c.tip, k0));
    }
    for (int s = 1; s <= 2; ++s)  // strata following the cone
      for (int i = 1; i < n - 1; ++i) {
        float t = 0.3f * s;
        Vector2 a{lerpf(lerpf(L + inset, R - inset, float(i) / n), bot[i].x, t), lerpf(T + h, bot[i].y, t)};
        Vector2 b{lerpf(lerpf(L + inset, R - inset, float(i + 1) / n), bot[i + 1].x, t), lerpf(T + h, bot[i + 1].y, t)};
        DrawLineEx(a, b, 2.0f, withAlpha(c.line, 0.5f));
      }
    if (th == StageTheme::Cosmic) {  // glowing crystals growing from the rock
      for (int i = 2; i < n - 1; i += 3) {
        Vector2 p = bot[i];
        p.y -= (p.y - T - h) * 0.35f;
        float g = 0.5f + 0.5f * std::sin(time * 1.6f + i);
        glow({p.x, p.y + 10}, 36, c.rune, 0.18f + 0.1f * g, 20);
        crystal({p.x, p.y - 4}, -(26 + 20 * hash(seed + i)), 12, 0.2f * (hash(i) - 0.5f), c.rune, 0.0f);
      }
    }
  }

  // Front face.
  gquad({L, T}, {R, T}, {R - inset, T + h}, {L + inset, T + h}, c.faceTop, c.faceTop, c.faceLow, c.faceLow);
  gquad({R - inset * 0.6f, T}, {R, T}, {R - inset, T + h}, {R - inset * 1.4f, T + h}, withAlpha(c.tip, 0.0f),
        withAlpha(c.tip, 0.35f), withAlpha(c.tip, 0.35f), withAlpha(c.tip, 0.0f));
  switch (th) {
    case StageTheme::Sky: {
      DrawLineEx({L + inset * 0.5f, T + h * 0.62f}, {R - inset * 0.5f, T + h * 0.62f}, 2.0f, withAlpha(c.line, 0.6f));
      for (int i = 0; i < int(w / 70.0f); ++i)  // pebbles
        ellipse({L + inset + hash(seed + i) * (w - inset * 2), T + h * (0.35f + 0.5f * hash(seed + i * 3))},
                4 + 4 * hash(i), 2.5f + 2 * hash(i * 5), withAlpha(c.faceTop, 0.9f));
      if (w >= 300.0f) {  // Zonai plaque set in the rock
        float pw = std::min(150.0f, w * 0.26f), ph = h * 0.62f, py = T + h * 0.3f;
        Rectangle pr{cx - pw * 0.5f, py, pw, ph};
        DrawRectangleRounded(ui::grow(pr, 4), 0.3f, 6, {150, 146, 128, 255});
        DrawRectangleRounded(pr, 0.3f, 6, {88, 90, 86, 255});
        float g = 0.65f + 0.35f * std::sin(time * 2.0f);
        runeGlow({cx, py + ph * 0.5f}, ph * 0.3f, c.rune, g);
        for (int s = -1; s <= 1; s += 2) {
          DrawRectangleRec({cx + s * pw * 0.3f - 6, py + ph * 0.3f, 12, 3}, withAlpha(c.rune, g));
          DrawRectangleRec({cx + s * pw * 0.3f - 6, py + ph * 0.62f, 12, 3}, withAlpha(c.rune, g));
        }
      }
      break;
    }
    case StageTheme::Sunset: {
      for (float x = L + 60; x < R - 30; x += 90) DrawLineEx({x, T + 12}, {x - 3, T + h}, 2.0f, withAlpha(c.line, 0.7f));
      const float fy = T + h * 0.3f, fh = h * 0.45f;  // carved frieze of hooked spirals
      DrawRectangleRec({L + inset * 0.4f, fy - 3, w - inset * 0.8f, 2}, withAlpha(c.line, 0.8f));
      DrawRectangleRec({L + inset * 0.6f, fy + fh + 2, w - inset * 1.2f, 2}, withAlpha(c.line, 0.8f));
      for (float x = L + 36; x < R - 30; x += 46) {
        Vector2 p[5] = {{x, fy + fh}, {x, fy + 2}, {x + 26, fy + 2}, {x + 26, fy + fh * 0.65f}, {x + 12, fy + fh * 0.65f}};
        stroke(p, 5, 3.0f, 3.0f, withAlpha(c.line, 0.85f));
      }
      float g = 0.6f + 0.4f * std::sin(time * 1.8f);
      DrawRectangleRounded({cx - 26, fy - 6, 52, fh + 12}, 0.3f, 6, c.capLow);
      runeGlow({cx, fy + fh * 0.5f}, fh * 0.42f, c.rune, g);
      break;
    }
    case StageTheme::Cosmic: {
      for (float x = L + 110; x < R - 50; x += 110) DrawLineEx({x, T + 12}, {x - 4, T + h}, 2.0f, withAlpha(c.line, 0.9f));
      const float ly = T + h * 0.55f;  // rune circuit with a travelling pulse
      DrawLineEx({L + inset, ly}, {R - inset, ly}, 2.0f, withAlpha(c.rune, 0.35f));
      float px = lerpf(L + inset, R - inset, wrapf(time * 0.25f, 1.0f));
      glow({px, ly}, 26, c.rune, 0.5f, 16);
      for (float x = L + 165; x < R - 100; x += 220) {
        DrawCircleV({x, ly}, h * 0.3f, c.capLow);
        runeGlow({x, ly}, h * 0.2f, c.rune, 0.6f + 0.4f * std::sin(time * 2.0f + x * 0.01f));
      }
      break;
    }
  }

  // Top cap: the readable walking surface.
  Rectangle cap{L - 5, T - 5, w + 10, 15};
  DrawRectangleRounded({cap.x, cap.y + 4, cap.width, cap.height}, 0.6f, 6, c.capLow);
  DrawRectangleRounded(cap, 0.6f, 6, c.cap);
  DrawRectangleRounded({cap.x + 2, cap.y, cap.width - 4, 5}, 1.0f, 6, c.capHi);
  switch (th) {
    case StageTheme::Sky:
      for (float x = L + 6; x < R - 6; x += 22) {  // moss drips and grass blades
        float d = 4 + 7 * hash(seed + int(x));
        ellipse({x, T + 9}, 6, d, c.cap);
        float sw = std::sin(time * 2.0f + x * 0.05f) * 2.0f;
        tri({x - 4, T - 3}, {x + 3, T - 3}, {x + sw, T - 10 - 4 * hash(int(x) * 3)}, c.capHi);
        tri({x + 6, T - 3}, {x + 12, T - 3}, {x + 10 + sw, T - 8}, c.cap);
      }
      for (int i = 0; i < int(w / 90.0f); ++i)
        dot({L + 20 + hash(seed + i * 7) * (w - 40), T - 5}, 2.5f, i % 2 ? Color{255, 250, 236, 255} : Color{255, 214, 90, 255});
      break;
    case StageTheme::Sunset:
      for (int i = 0; i < int(w / 70.0f); ++i) {  // moss patches spilling over the edge
        float x = L + 10 + hash(seed + i * 5) * (w - 20), mw = 14 + 22 * hash(seed + i * 9);
        ellipse({x, T - 3}, mw, 4, {124, 156, 80, 255});
        ellipse({x, T + 6}, mw * 0.6f, 5 + 6 * hash(i), {104, 136, 70, 255});
      }
      break;
    case StageTheme::Cosmic: {
      float g = 0.6f + 0.4f * std::sin(time * 1.5f);
      gquad({L, T - 18}, {R, T - 18}, {R, T - 4}, {L, T - 4}, withAlpha(c.capHi, 0.0f), withAlpha(c.capHi, 0.0f),
            withAlpha(c.capHi, 0.12f * g), withAlpha(c.capHi, 0.12f * g));
      DrawRectangleRounded({cap.x + 2, cap.y - 1, cap.width - 4, 4}, 1.0f, 6, c.capHi);
      for (int i = 0; i < int(w / 80.0f); ++i) {  // little glowing bulbs along the edge
        float x = L + 16 + hash(seed + i * 3) * (w - 32);
        float p = 0.6f + 0.4f * std::sin(time * 2.5f + i);
        glow({x, T - 7}, 14, {200, 255, 160, 255}, 0.35f * p, 16);
        DrawLineEx({x, T - 2}, {x, T - 6}, 2.0f, {60, 110, 90, 255});
        dot({x, T - 8}, 3.2f, {226, 255, 180, 255});
      }
      break;
    }
  }
}

void drawThin(const Rect& r, StageTheme th, float time, int seed) {
  const PlatLook c = platLook(th);
  const float L = r.x, R = r.right(), T = r.y, w = r.w, cx = L + w * 0.5f;
  switch (th) {
    case StageTheme::Sky: {  // floating Zonai slab with an energy core
      float p = 0.6f + 0.4f * std::sin(time * 4.0f);
      quad({L + 14, T + 10}, {R - 14, T + 10}, {R - 34, T + 26}, {L + 34, T + 26}, {128, 124, 112, 255});
      glow({cx, T + 28}, 30, c.rune, 0.35f * p, 20);
      for (int k = 0; k < 3; ++k) {  // thrust motes
        float t = wrapf(time * 1.5f + k / 3.0f, 1.0f);
        dot({cx + (hash(k) - 0.5f) * 14, T + 30 + t * 30}, 3.0f * (1 - t), withAlpha(c.rune, 0.8f * (1 - t)));
      }
      DrawCircleV({cx, T + 26}, 8, {70, 72, 70, 255});
      DrawCircleV({cx, T + 26}, 5, mixColor(c.rune, WHITE, 0.4f * p));
      DrawRectangleRounded({L, T - 2, w, 16}, 0.5f, 6, {170, 164, 144, 255});
      DrawRectangleRounded({L, T - 3, w, 12}, 0.5f, 6, {206, 200, 176, 255});
      DrawRectangleRounded({L + 3, T - 3, w - 6, 4}, 1.0f, 6, {240, 236, 214, 255});
      for (int s = -1; s <= 1; s += 2) {
        DrawRectangleRec({cx + s * w * 0.3f - 10, T + 5, 20, 2.5f}, withAlpha(c.rune, p));
        dot({cx + s * w * 0.15f, T + 6}, 2.2f, withAlpha(c.rune, p));
      }
      break;
    }
    case StageTheme::Sunset: {  // carved stone ledge with moss and vines
      for (int s = 0; s < 2; ++s) {
        float x = s ? R - 14 : L + 14;
        Vector2 p[4] = {{x, T + 10}};
        for (int k = 1; k < 4; ++k)
          p[k] = {p[k - 1].x + std::sin(time * 1.3f + k + s * 2 + seed) * 2.0f, p[k - 1].y + 10 + 6 * hash(seed + s * 5 + k)};
        stroke(p, 4, 3.0f, 1.5f, {78, 132, 66, 255});
        ellipse(p[3], 3.5f, 2.5f, {104, 160, 76, 255});
      }
      quad({L + 6, T + 8}, {R - 6, T + 8}, {R - 18, T + 22}, {L + 18, T + 22}, c.faceLow);
      DrawRectangleRounded({L, T - 2, w, 14}, 0.4f, 6, c.capLow);
      DrawRectangleRounded({L, T - 3, w, 11}, 0.4f, 6, c.cap);
      DrawRectangleRounded({L + 3, T - 3, w - 6, 4}, 1.0f, 6, c.capHi);
      for (float x = L + 24; x < R - 20; x += 36) DrawRectangleRec({x, T + 11, 2, 8}, withAlpha(c.line, 0.8f));
      ellipse({L + w * (0.2f + 0.5f * hash(seed)), T - 3}, 18, 3.5f, {124, 156, 80, 255});
      runeGlow({cx, T + 15}, 5, c.rune, 0.6f + 0.3f * std::sin(time * 2.0f));
      break;
    }
    case StageTheme::Cosmic: {  // crystal bridge
      Rectangle rr{L, T - 2, w, 14};
      glow({cx, T + 4}, w * 0.6f, 22, withAlpha(c.rune, 0.25f), withAlpha(c.rune, 0.0f));
      DrawRectangleRounded(rr, 0.6f, 6, withAlpha({60, 170, 160, 255}, 0.7f));
      DrawRectangleRounded({L + 2, T - 2, w - 4, 4}, 1.0f, 6, c.capHi);
      break;
    }
  }
}
}  // namespace

void drawBackground(const StageDef& stage, Vec2 cam, float zoom, float time) {
  (void)zoom;
  const Art& a = themeArt(stage.theme);
  const float dx = clampf(cam.x, -1200.0f, 1200.0f), dy = clampf(cam.y + 150.0f, -500.0f, 500.0f);
  const Vec2 pc{dx, dy};
  const float skyX = -40.0f + clampf(-dx * 0.03f, -40.0f, 40.0f), skyY = -40.0f + clampf(-dy * 0.03f, -40.0f, 40.0f);
  switch (stage.theme) {
    case StageTheme::Sky: {
      const float sy = 410.0f - dy * 0.18f;  // the sea of clouds is opaque from sy + 210 down
      drawBackdrop(a.sky, skyX, skyY, sy + 212.0f);
      drawLayer(a.near, -260.0f - dx * 0.14f, 10.0f - dy * 0.14f, true);
      vgrad(0, sy + 259.0f, kScreenW, std::max(0.0f, kScreenH - sy - 259.0f), kSea, kSeaDeep);
      drawLayer(a.mid, -dx * 0.18f - time * 6.0f, sy, true);
      motes(26, 11, pc, time, 6.0f, -8.0f, {255, 252, 230, 255}, {200, 255, 236, 255}, 1.4f);
      leaves(8, 5, pc, time, {120, 190, 80, 255}, {200, 220, 110, 255});
      break;
    }
    case StageTheme::Sunset: {
      drawBackdrop(a.sky, skyX, skyY);
      drawLayer(a.near, -360.0f - dx * 0.12f, 150.0f - dy * 0.12f, false);
      const float gy = 280.0f - dy * 0.25f;
      drawLayer(a.extra, -160.0f - dx * 0.25f, gy, true);
      if (gy + 519.0f < kScreenH) DrawRectangleRec({0, gy + 519.0f, float(kScreenW), kScreenH - gy - 519.0f}, kGround);
      motes(30, 23, pc, time, 4.0f, -6.0f, {255, 220, 140, 255}, {255, 250, 210, 255}, 1.5f);
      leaves(10, 9, pc, time, {226, 150, 90, 255}, {250, 200, 120, 255});
      break;
    }
    case StageTheme::Cosmic: {
      drawBackdrop(a.sky, skyX, skyY);
      const float gy = 470.0f - dy * 0.1f;
      drawLayer(a.extra, -dx * 0.1f - time * 3.0f, gy, true);
      float pulse = 0.5f + 0.5f * std::sin(time * 1.1f);
      vgrad(0, gy + 120.0f, kScreenW, std::max(0.0f, kScreenH - gy - 120.0f), withAlpha({255, 30, 70, 255}, 0.0f),
            withAlpha({255, 30, 70, 255}, 0.12f + 0.1f * pulse));
      const float nx = -260.0f - dx * 0.2f, ny = 140.0f - dy * 0.2f;
      drawLayer(a.near, nx, ny, true);
      if (ny + 659.0f < kScreenH) DrawRectangleRec({0, ny + 659.0f, float(kScreenW), kScreenH - ny - 659.0f}, kRockLow);
      float bx = wrapPos(nx, kRootX, 1800.0f, 300.0f);
      glow({bx, ny + depthsGround(kRootX) + 10.0f - kRootS * 1.65f}, 160.0f + 30.0f * pulse, {170, 255, 190, 255}, 0.18f + 0.1f * pulse, 24);
      motes(40, 37, pc, time, 3.0f, -14.0f, {110, 255, 220, 255}, {220, 255, 150, 255}, 1.5f);
      break;
    }
    default: stageart::drawBackground(stage.theme, a, pc, skyX, skyY, time); break;
  }
}

void drawStageForeground(const StageDef& stage, Vec2 cam, float time) {
  const Vec2 pc{clampf(cam.x, -1200.0f, 1200.0f), clampf(cam.y + 150.0f, -500.0f, 500.0f)};
  stageart::drawForeground(stage.theme, pc, time);
}

void drawPlatforms(const Match& m, float time) { drawPlatformList(m.platforms, m.stage().theme, time); }

void drawPlatformList(const std::vector<Platform>& platforms, StageTheme theme, float time) {
  int seed = 0;
  for (const auto& p : platforms) {
    seed += 101;
    if (stageart::drawPlatform(p, theme, time, seed)) continue;
    if (p.solid) drawSolid(p.rect, theme, time, seed);
    else drawThin(p.rect, theme, time, seed);
  }
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

// Crescent whose leading edge passes through `apex`.
void crescentAt(Vec2 apex, Vec2 d, float R, float thick, float span, Color body, Color edge) {
  vfx::crescent(apex - d * R, d, R, thick, span, body, edge);
}

void shotBlackSlash(const Shot& s, float time) {
  const float R = s.r * 2.0f;
  for (int i = 3; i >= 1; --i)  // dark after-images
    crescentAt(s.c - s.d * (i * s.r * 0.55f), s.d, R, R * 0.5f, 1.2f, Ap({20, 18, 26, 255}, 0.0f),
               Ap({20, 18, 26, 255}, 0.3f * s.k / i));
  crescentAt(s.c + s.d * 4.0f, s.d, R * 1.04f, R * 0.6f, 1.26f, Ap({210, 210, 225, 255}, 0.2f * s.k), Ap({240, 240, 250, 255}, 0.95f * s.k));
  crescentAt(s.c, s.d, R, R * 0.52f, 1.2f, Ap({8, 6, 12, 255}, 0.97f * s.k), Ap({28, 24, 34, 255}, s.k));
  const float base = std::atan2(s.d.y, s.d.x);
  const uint32_t fs = s.seed + uint32_t(time * 30.0f) * 131u;
  for (int i = 0; i < 3; ++i) {  // white crackle running across the black blade
    const float a0 = base + (i - 1) * 0.6f, a1 = a0 + (hash01(fs + i) - 0.5f) * 0.6f;
    const Vec2 o = s.c - s.d * R;
    vfx::bolt(o + Vec2{std::cos(a0), std::sin(a0)} * (R * 0.62f), o + Vec2{std::cos(a1), std::sin(a1)} * (R * 1.08f),
              R * 0.06f, 4, fs + i * 17u, std::max(1.5f, R * 0.035f), Ap({245, 245, 255, 255}, 0.9f * s.k));
  }
  for (int i = 0; i < 2; ++i) {  // black lightning leaping off the edge
    const float a = base + (hash01(fs + 40 + i) - 0.5f) * 2.2f;
    const Vec2 p0 = s.c - s.d * (R * 0.2f) + Vec2{std::cos(a), std::sin(a)} * (R * 0.7f);
    vfx::bolt(p0, p0 + Vec2{std::cos(a), std::sin(a)} * (R * 0.5f), R * 0.1f, 4, fs + 90 + i, std::max(2.0f, R * 0.05f),
              Ap({8, 6, 12, 255}, 0.9f * s.k), Ap({200, 200, 215, 255}, 0.6f * s.k));
  }
}

void shotWindBlade(const Shot& s) {
  const Color g{120, 232, 170, 255};
  const float R = s.r * 2.6f;
  vfx::additive(true);
  vfx::streak(s.c - s.d * (s.r * 6.0f), s.c, 0.0f, s.r * 1.4f, Ap(g, 0.0f), Ap(g, 0.45f * s.k));
  crescentAt(s.c, s.d, R, R * 0.3f, 1.05f, Ap(g, 0.15f * s.k), Ap({230, 255, 240, 255}, 0.95f * s.k));
  crescentAt(s.c - s.d * (s.r * 0.9f), s.d, R * 0.9f, R * 0.22f, 1.0f, Ap(g, 0.0f), Ap(g, 0.5f * s.k));
  vfx::additive(false);
}

void shotTornado(const Shot& s, float time) {
  const float bottom = s.c.y + s.r * 1.3f, top = s.c.y - s.r * 2.6f;
  const Color g{170, 250, 210, 255};
  vfx::additive(true);
  vfx::quad({s.c.x - s.r * 0.3f, bottom}, {s.c.x + s.r * 0.3f, bottom}, {s.c.x + s.r * 1.5f, top}, {s.c.x - s.r * 1.5f, top},
            Ap(g, 0.25f * s.k), Ap(g, 0.25f * s.k), Ap(g, 0.05f * s.k), Ap(g, 0.05f * s.k));
  for (int i = 0; i <= 8; ++i) {  // spinning rings, wider towards the top
    const float u = i / 8.0f;
    const float y = lerpf(bottom, top, u);
    const float rx = s.r * (0.3f + 1.25f * u), ry = rx * 0.24f;
    const float x = s.c.x + std::sin(time * 7.0f + i * 0.9f) * s.r * 0.22f * (0.3f + u);
    const float spin = time * 14.0f + i;
    Vec2 prev{};
    for (int k = 0; k <= 14; ++k) {
      const float a = spin + k * 2.0f * kPi / 14.0f;
      const Vec2 q{x + std::cos(a) * rx, y + std::sin(a) * ry};
      const float front = std::sin(a) > 0.0f ? 1.0f : 0.4f;
      if (k > 0) vfx::wedge(prev, q, 2.5f, 2.5f, Ap(WHITE, 0.55f * front * s.k), Ap(g, 0.55f * front * s.k));
      prev = q;
    }
  }
  vfx::additive(false);
  for (int i = 0; i < 6; ++i) {  // debris whirled around
    const float a = time * 9.0f + i * 1.05f;
    const float u = hash01(i * 7 + 1);
    const Vec2 q{s.c.x + std::cos(a) * s.r * (0.5f + u), lerpf(bottom, top, u) + std::sin(a) * s.r * 0.2f};
    DrawPoly(V(q), 4, 3.0f, a * 60.0f, Ap(i % 2 ? Color{90, 200, 110, 255} : Color{150, 130, 100, 255}, s.k));
  }
}

void shotGetsuga(const Shot& s, bool dark, float time) {
  const float R = s.r * 2.3f, th = R * 0.5f;
  if (dark) {  // black Getsuga with a red outline
    vfx::additive(true);
    for (int i = 3; i >= 1; --i)
      crescentAt(s.c - s.d * (i * s.r * 0.6f), s.d, R, th, 1.3f, Ap({200, 20, 40, 255}, 0.0f), Ap({220, 20, 40, 255}, 0.3f * s.k / i));
    crescentAt(s.c + s.d * 5.0f, s.d, R * 1.06f, th * 1.2f, 1.36f, Ap({255, 40, 60, 255}, 0.2f * s.k), Ap({255, 60, 70, 255}, s.k));
    vfx::additive(false);
    crescentAt(s.c, s.d, R, th, 1.3f, Ap({6, 2, 6, 255}, 0.97f * s.k), Ap({20, 4, 10, 255}, s.k));
    return;
  }
  const Color b{90, 170, 255, 255};
  vfx::additive(true);
  vfx::glow(s.c - s.d * (R * 0.3f), R * 1.1f, Ap(b, 0.25f * s.k), Ap(b, 0.0f));
  for (int i = 3; i >= 1; --i)
    crescentAt(s.c - s.d * (i * s.r * 0.6f), s.d, R, th, 1.3f, Ap(b, 0.0f), Ap(b, 0.35f * s.k / i));
  crescentAt(s.c, s.d, R, th, 1.3f, Ap(b, 0.2f * s.k), Ap({200, 230, 255, 255}, s.k));
  crescentAt(s.c, s.d, R * 0.98f, th * 0.35f, 1.22f, Ap(WHITE, 0.4f * s.k), Ap(WHITE, s.k));
  for (int i = 0; i < 5; ++i) {  // energy streaming off the tips
    const float a = std::atan2(s.d.y, s.d.x) + (i - 2) * 0.55f;
    const Vec2 p0 = s.c - s.d * R + Vec2{std::cos(a), std::sin(a)} * (R * 0.95f);
    const float l = R * (0.5f + 0.4f * std::sin(time * 30.0f + i));
    vfx::streak(p0 - s.d * l, p0, 0.0f, 4.0f, Ap(b, 0.0f), Ap({220, 240, 255, 255}, 0.7f * s.k));
  }
  vfx::additive(false);
}

// Getsuga Jujisho: two black crescents edged with red, crossing in an X, flying as one.
void shotGetsugaCross(const Shot& s, float time) {
  const float R = s.r * 2.1f, th = R * 0.46f;
  for (float tilt : {-0.62f, 0.62f}) {
    const Vec2 d = rotate(s.d, tilt);
    vfx::additive(true);
    for (int i = 3; i >= 1; --i)
      crescentAt(s.c - s.d * (i * s.r * 0.55f), d, R, th, 1.25f, Ap({200, 20, 40, 255}, 0.0f), Ap({230, 20, 40, 255}, 0.28f * s.k / i));
    crescentAt(s.c + s.d * 5.0f, d, R * 1.06f, th * 1.2f, 1.32f, Ap({255, 40, 60, 255}, 0.2f * s.k), Ap({255, 70, 80, 255}, s.k));
    vfx::additive(false);
    crescentAt(s.c, d, R, th, 1.25f, Ap({6, 2, 6, 255}, 0.97f * s.k), Ap({24, 4, 10, 255}, s.k));
  }
  vfx::additive(true);  // where the two blades cross: a hot red core
  vfx::glow(s.c, s.r * 0.9f, Ap({255, 60, 70, 255}, 0.7f * s.k), Ap({255, 20, 40, 255}, 0.0f));
  vfx::glow(s.c, s.r * 0.35f, Ap(WHITE, 0.8f * s.k), Ap({255, 140, 140, 255}, 0.0f));
  for (int i = 0; i < 4; ++i) {  // red sparks torn off the edges
    const float a = std::atan2(s.d.y, s.d.x) + kPi + (hash01(s.seed + i) - 0.5f) * 1.6f;
    const Vec2 p0 = s.c + Vec2{std::cos(a), std::sin(a)} * (s.r * (0.6f + 0.6f * std::fmod(time * 3.0f + i * 0.25f, 1.0f)));
    vfx::streak(p0, p0 - s.d * (s.r * 0.8f), 3.0f, 0.0f, Ap({255, 80, 90, 255}, 0.7f * s.k), Ap({255, 80, 90, 255}, 0.0f));
  }
  vfx::additive(false);
}

// Desgarron: five claws of cyan reiatsu raking forward, the middle ones leading.
void shotDesgarron(const Shot& s) {
  vfx::additive(true);
  vfx::glow(s.c, s.r * 1.4f, Ap({60, 214, 240, 255}, 0.22f * s.k), Ap({60, 214, 240, 255}, 0.0f));
  for (int i = 0; i < 5; ++i) {
    const float o = (i - 2) * s.r * 0.34f, lag = std::fabs(i - 2.0f) * s.r * 0.14f;
    const Vec2 tip = s.c + s.n * o + s.d * (s.r * 0.85f - lag);
    const Vec2 mid = tip - s.d * (s.r * 0.55f) + s.n * (o * 0.08f), tail = tip - s.d * (s.r * 2.0f) + s.n * (o * 0.3f);
    vfx::streak(tail, mid, 0.0f, s.r * 0.17f, Ap({60, 214, 240, 255}, 0.0f), Ap({120, 240, 255, 255}, 0.9f * s.k));
    vfx::streak(tip, mid, 0.0f, s.r * 0.17f, Ap({200, 250, 255, 255}, s.k), Ap({120, 240, 255, 255}, 0.9f * s.k));
    vfx::streak(tail + s.d * (s.r * 0.8f), tip, 0.0f, s.r * 0.05f, Ap(WHITE, 0.0f), Ap(WHITE, s.k));
  }
  vfx::additive(false);
}

// Garra de la Pantera: a bone blade flying point first, a cyan wake behind it.
void shotPanteraDart(const Shot& s) {
  vfx::additive(true);
  vfx::streak(s.c - s.d * (s.r * 3.6f), s.c, 0.0f, s.r * 0.9f, Ap({60, 214, 240, 255}, 0.0f), Ap({120, 240, 255, 255}, 0.7f * s.k));
  vfx::additive(false);
  vfx::wedge(s.c - s.d * (s.r * 1.5f), s.c + s.d * (s.r * 1.5f), s.r * 0.6f, 0.0f, Ap({220, 222, 214, 255}, s.k),
             Ap(WHITE, s.k));
  vfx::wedge(s.c - s.d * (s.r * 1.2f), s.c + s.d * (s.r * 1.0f), s.r * 0.22f, 0.0f, Ap({150, 230, 245, 255}, s.k),
             Ap(WHITE, s.k));
}

// A wolf of Los Lobos: a spirit of pale blue cero running on the ground, or leaping on its prey from above.
void shotCeroWolf(const Match& m, const Projectile& p, const Shot& s, float time) {
  const bool strike = p.vel.len2() < 0.5f;
  float dir = s.d.x >= 0.0f ? 1.0f : -1.0f;
  if (strike && p.owner >= 0 && p.owner < int(m.fighters.size())) dir = m.fighters[p.owner].pos.x <= p.pos.x ? 1.0f : -1.0f;
  const float sz = s.r * (strike ? 1.3f : 1.6f);
  Vec2 c = s.c;
  float tilt = 0.0f;
  if (strike) {  // the leap: from up behind, jaws first
    const float u = clampf((p.age - p.armDelay) / 7.0f, 0.0f, 1.0f);
    c = s.c + Vec2{-dir * sz * 1.6f * (1.0f - u), -sz * 2.2f * (1.0f - u)};
    tilt = 0.55f * dir * (1.0f - u * 0.5f);
  }
  auto at = [&](float x, float y) { return c + rotate(Vec2{x * dir, y} * sz, tilt); };
  const Color fur = Ap({140, 190, 255, 255}, 0.75f * s.k), core = Ap({235, 244, 255, 255}, s.k);
  vfx::additive(true);
  vfx::glow(c, sz * 1.9f, Ap({110, 170, 255, 255}, 0.28f * s.k), Ap({110, 170, 255, 255}, 0.0f));
  const float ph = time * 18.0f + float(s.seed % 7);
  for (int i = 0; i < 4; ++i) {
    const float hx = i < 2 ? 0.45f : -0.45f, a = ph + (i % 2) * kPi + (i < 2 ? 0.0f : 0.8f);
    const Vec2 paw = strike ? at(hx + 0.3f, 0.5f) : at(hx + std::sin(a) * 0.36f, 0.62f - std::max(0.0f, std::cos(a)) * 0.16f);
    vfx::wedge(at(hx, 0.08f), paw, sz * 0.17f, sz * 0.06f, fur, fur);
  }
  vfx::wedge(at(-0.7f, 0.0f), at(0.6f, -0.05f), sz * 0.52f, sz * 0.46f, fur, fur);
  vfx::wedge(at(-0.7f, -0.05f), at(-1.4f, -0.32f + 0.1f * std::sin(ph * 0.5f)), sz * 0.22f, sz * 0.04f, fur, fur);
  vfx::glow(at(0.85f, -0.3f), sz * 0.33f, fur, fur, 14);
  vfx::tri(at(1.0f, -0.42f), at(1.0f, -0.16f), at(1.58f, strike ? -0.1f : -0.26f), fur, fur, fur);
  vfx::tri(at(0.66f, -0.48f), at(0.86f, -0.56f), at(0.7f, -0.92f), fur, fur, fur);
  vfx::tri(at(0.9f, -0.54f), at(1.04f, -0.48f), at(0.98f, -0.88f), fur, fur, fur);
  vfx::glow(at(0.98f, -0.34f), sz * 0.09f, core, core, 8);
  vfx::additive(false);
}

// Cero Metralleta: one of a thousand small blue ceros.
void shotStarrkCero(const Shot& s) {
  vfx::additive(true);
  vfx::streak(s.c - s.d * (s.r * 3.2f), s.c, 0.0f, s.r * 1.2f, Ap({90, 150, 255, 255}, 0.0f), Ap({130, 185, 255, 255}, 0.6f * s.k));
  vfx::glow(s.c, s.r * 1.9f, Ap({110, 170, 255, 255}, 0.55f * s.k), Ap({110, 170, 255, 255}, 0.0f));
  vfx::glow(s.c, s.r * 0.7f, Ap(WHITE, s.k), Ap({200, 225, 255, 255}, 0.0f));
  vfx::additive(false);
}

void shotCero(const Shot& s, float time) {
  vfx::glow(s.c, s.r * 1.9f, Ap({20, 0, 6, 255}, 0.7f * s.k), Ap({20, 0, 6, 255}, 0.0f));
  vfx::additive(true);
  vfx::burst(s.c, 10, s.r * 0.5f, s.r * 2.2f, 0.3f, time * 8.0f, Ap({255, 60, 70, 255}, 0.8f * s.k), Ap({255, 30, 40, 255}, 0.0f), s.seed);
  vfx::glow(s.c, s.r * 1.2f, Ap({255, 40, 60, 255}, 0.9f * s.k), Ap({200, 10, 30, 255}, 0.0f));
  vfx::glow(s.c, s.r * 0.6f, Ap(WHITE, s.k), Ap({255, 180, 180, 255}, 0.0f));
  vfx::additive(false);
}

void petalShape(Vec2 p, Vec2 d, float len, float w, Color col) {
  const Vec2 n{-d.y, d.x};
  vfx::quad(p - d * len, p + n * w, p + d * len, p - n * w, col, col, withAlpha(WHITE, col.a / 255.0f), col);
}

void shotPetals(const Shot& s, float time) {
  vfx::additive(true);
  vfx::glow(s.c, s.r * 1.5f, Ap({255, 140, 190, 255}, 0.28f * s.k), Ap({255, 140, 190, 255}, 0.0f));
  vfx::additive(false);
  for (int i = 0; i < 28; ++i) {  // a swarm of blade petals swirling as it drifts
    const float sp = 2.0f + 2.5f * hash01(i * 5 + 1);
    const float a = hash01(i * 5 + 2) * 2.0f * kPi + time * sp;
    const float rad = s.r * (0.25f + 0.85f * std::sqrt(hash01(i * 5 + 3)));
    const Vec2 p = s.c + Vec2{std::cos(a) * rad, std::sin(a) * rad * 0.8f} - s.d * (hash01(i * 5 + 4) * s.r * 0.9f);
    const Vec2 tang = Vec2{-std::sin(a), std::cos(a) * 0.8f}.norm();
    const Color col = i % 4 == 0 ? Color{255, 238, 246, 255} : i % 4 == 1 ? Color{232, 92, 146, 255} : Color{255, 158, 196, 255};
    petalShape(p, tang, 5.0f + 3.0f * hash01(i), 2.0f + 1.5f * hash01(i + 9), Ap(col, s.k));
  }
}

void shotArrow(const Shot& s, bool light, float time) {
  const Vec2 tip = s.c + s.d * (s.r * 1.3f), tail = s.c - s.d * (s.r * 5.0f);
  if (light) {  // arrow of light: a blazing line with a long trail
    vfx::additive(true);
    vfx::streak(s.c - s.d * (s.r * 16.0f), tip, 0.0f, s.r * 1.5f, Ap({255, 200, 90, 255}, 0.0f), Ap({255, 220, 120, 255}, 0.6f * s.k));
    vfx::streak(tail, tip, 2.0f, 6.0f, Ap(WHITE, 0.7f * s.k), Ap(WHITE, s.k));
    vfx::glow(tip, s.r * 1.6f, Ap({255, 240, 190, 255}, 0.8f * s.k), Ap({255, 210, 110, 255}, 0.0f));
    vfx::burst(tip, 6, s.r * 0.2f, s.r * 2.4f, 0.25f, time * 6.0f, Ap(WHITE, 0.8f * s.k), Ap({255, 220, 140, 255}, 0.0f), s.seed);
    vfx::additive(false);
    return;
  }
  vfx::additive(true);
  vfx::streak(s.c - s.d * (s.r * 12.0f), tail, 0.0f, 3.0f, Ap(WHITE, 0.0f), Ap(WHITE, 0.3f * s.k));
  vfx::additive(false);
  vfx::wedge(tail, tip - s.d * 4.0f, 3.0f, 3.0f, Ap({120, 84, 50, 255}, s.k), Ap({140, 100, 60, 255}, s.k));  // shaft
  vfx::tri(tip + s.d * 6.0f, tip - s.d * 4.0f + s.n * 4.5f, tip - s.d * 4.0f - s.n * 4.5f, Ap({220, 226, 236, 255}, s.k),
           Ap({150, 156, 170, 255}, s.k), Ap({150, 156, 170, 255}, s.k));  // head
  for (int side = -1; side <= 1; side += 2)  // fletching
    vfx::tri(tail, tail + s.d * 9.0f, tail - s.d * 2.0f + s.n * (side * 6.0f), Ap({240, 236, 226, 255}, s.k),
             Ap({240, 236, 226, 255}, s.k), Ap({200, 60, 50, 255}, s.k));
}

void shotBomb(const Projectile& p, const Shot& s, float time) {
  const float spin = p.age * 0.25f, r = s.r * 1.15f;
  const float urgency = 1.0f - p.life / float(std::max(1, p.maxLife));
  const float blink = 0.5f + 0.5f * std::sin(p.age * (0.35f + urgency * 1.4f));
  vfx::additive(true);
  vfx::glow(s.c, r * 2.2f, Ap(kZonaiGlow, (0.15f + 0.35f * blink) * s.k), Ap(kZonaiGlow, 0.0f));
  vfx::additive(false);
  DrawCircleV(V(s.c), r + 2.0f, Ap({30, 34, 36, 255}, s.k));
  vfx::glow(s.c, r, Ap({244, 238, 220, 255}, s.k), Ap({196, 188, 168, 255}, s.k), 20);  // shell
  DrawRing(V(s.c), r * 0.62f, r * 0.74f, 0, 360, 24, Ap(mixColor(kZonaiGlow, {30, 120, 110, 255}, 1.0f - blink), s.k));
  DrawPoly(V(s.c), 6, r * 0.42f, spin * 57.3f, Ap(mixColor({40, 140, 120, 255}, kZonaiGlow, blink), s.k));
  DrawPoly(V(s.c), 6, r * 0.2f, spin * 57.3f + 30.0f, Ap(mixColor(kZonaiGlow, WHITE, blink), s.k));
  const Vec2 fd{std::cos(spin - 1.2f), std::sin(spin - 1.2f)};  // fuse with a spark at its tip
  const Vec2 f0 = s.c + fd * (r * 0.9f), f1 = s.c + fd * (r * 1.3f);
  DrawLineEx(V(f0), V(f1), 3.0f, Ap({60, 50, 40, 255}, s.k));
  vfx::additive(true);
  vfx::glow(f1, 7.0f + 2.0f * std::sin(time * 50.0f), Ap({255, 230, 140, 255}, s.k), Ap({255, 160, 60, 255}, 0.0f), 10);
  vfx::additive(false);
}

void shotSwordBeam(const Shot& s, float time) {
  const float len = s.r * 3.2f, w = s.r * 0.95f;
  const Vec2 tip = s.c + s.d * (len * 0.55f), hilt = s.c - s.d * (len * 0.45f);
  vfx::additive(true);
  vfx::streak(s.c - s.d * (len * 2.6f), hilt, 0.0f, w * 1.4f, Ap({255, 214, 110, 255}, 0.0f), Ap({255, 214, 110, 255}, 0.4f * s.k));
  vfx::glow(s.c, len * 0.8f, Ap({255, 220, 130, 255}, 0.3f * s.k), Ap({255, 220, 130, 255}, 0.0f));
  const Color body = Ap({255, 236, 170, 255}, 0.9f * s.k), core = Ap(WHITE, s.k);
  vfx::tri(tip, s.c + s.n * (w * 0.5f), s.c - s.n * (w * 0.5f), core, body, body);
  vfx::tri(hilt, s.c + s.n * (w * 0.5f), s.c - s.n * (w * 0.5f), Ap({255, 200, 90, 255}, 0.3f * s.k), body, body);
  vfx::wedge(hilt, tip, 2.0f, 2.0f, core, core);
  vfx::burst(tip, 4, 2.0f, s.r * (1.4f + 0.3f * std::sin(time * 40.0f)), 0.3f, kPi * 0.25f, core, Ap(WHITE, 0.0f));
  vfx::additive(false);
}

void shotDarkOrb(const Shot& s, float time) {
  const Color red{220, 30, 60, 255};
  vfx::glow(s.c, s.r * 2.1f, Ap({24, 0, 8, 255}, 0.65f * s.k), Ap({24, 0, 8, 255}, 0.0f));  // smoke halo
  vfx::additive(true);
  vfx::glow(s.c, s.r * 1.5f, Ap(red, 0.55f * s.k), Ap(red, 0.0f));
  for (int i = 0; i < 3; ++i) {  // tendrils of malice winding around
    Vec2 prev{};
    for (int k = 0; k <= 10; ++k) {
      const float a = time * 4.0f + i * 2.1f + k * 0.28f;
      const float rr = s.r * (1.35f - k * 0.04f);
      const Vec2 q = s.c + Vec2{std::cos(a), std::sin(a)} * rr;
      if (k > 0) vfx::wedge(prev, q, 1.0f + k * 0.4f, 1.0f + (k + 1) * 0.4f, Ap(red, 0.0f), Ap({255, 90, 110, 255}, 0.7f * s.k));
      prev = q;
    }
  }
  vfx::additive(false);
  vfx::glow(s.c, s.r, Ap({6, 0, 4, 255}, 0.97f * s.k), Ap({40, 0, 12, 255}, 0.97f * s.k), 24);
  vfx::softRing(s.c, s.r * 0.98f, s.r * 0.25f, Ap({255, 60, 80, 255}, 0.8f * s.k));
  vfx::additive(true);
  const uint32_t fs = s.seed + uint32_t(time * 20.0f) * 37u;
  for (int i = 0; i < 3; ++i) {  // crackling
    const float a = hash01(fs + i) * 2.0f * kPi;
    const Vec2 d{std::cos(a), std::sin(a)};
    vfx::bolt(s.c + d * (s.r * 0.6f), s.c + d * (s.r * 1.6f), s.r * 0.15f, 4, fs + i * 7u, 2.0f, Ap({255, 80, 100, 255}, 0.9f * s.k));
  }
  vfx::glow(s.c, s.r * 0.35f, Ap({255, 120, 120, 255}, 0.7f * s.k), Ap(red, 0.0f));  // smouldering heart
  vfx::additive(false);
}

void shotGloomSpear(const Shot& s) {
  const float len = s.r * 5.5f;
  const Vec2 tip = s.c + s.d * (len * 0.45f), tail = s.c - s.d * (len * 0.55f);
  vfx::additive(true);
  vfx::streak(tail - s.d * len, tip, 0.0f, s.r * 1.4f, Ap({220, 20, 50, 255}, 0.0f), Ap({255, 50, 80, 255}, 0.5f * s.k));
  vfx::additive(false);
  vfx::wedge(tail, tip - s.d * (s.r * 1.2f), s.r * 0.35f, s.r * 0.45f, Ap({24, 4, 10, 255}, s.k), Ap({40, 6, 14, 255}, s.k));
  vfx::tri(tip, tip - s.d * (s.r * 1.6f) + s.n * (s.r * 0.6f), tip - s.d * (s.r * 1.6f) - s.n * (s.r * 0.6f), Ap({255, 70, 90, 255}, s.k),
           Ap({30, 4, 10, 255}, s.k), Ap({30, 4, 10, 255}, s.k));
  vfx::additive(true);
  vfx::wedge(tail, tip, 1.5f, 2.5f, Ap({255, 60, 80, 255}, 0.2f * s.k), Ap({255, 120, 130, 255}, 0.8f * s.k));
  vfx::additive(false);
}

void shotAxe(const Projectile& p, const Shot& s) {
  const float dirSign = p.returning ? -1.0f : 1.0f;
  const float spin = p.age * 0.55f * dirSign;
  const Color frost{150, 225, 255, 255};
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.0f, Ap(frost, 0.3f * s.k), Ap(frost, 0.0f));
  for (int g = 1; g <= 3; ++g) {  // frost arcs left by the spinning blade
    const float a = spin - g * 0.45f * dirSign;
    const Vec2 u{std::cos(a), std::sin(a)};
    vfx::wedge(s.c, s.c + u * (s.r * 1.6f), 2.0f, s.r * 0.9f, Ap(frost, 0.0f), Ap(frost, 0.32f * s.k / g));
  }
  vfx::additive(false);
  const Vec2 u{std::cos(spin), std::sin(spin)}, n{-u.y, u.x};
  const Vec2 head = s.c + u * (s.r * 0.9f), butt = s.c - u * (s.r * 1.2f);
  vfx::wedge(butt, head, 5.0f, 5.0f, Ap({92, 64, 44, 255}, s.k), Ap({120, 86, 58, 255}, s.k));  // haft
  DrawCircleV(V(butt), 3.5f, Ap({200, 210, 220, 255}, s.k));
  const Vec2 b0 = head - u * (s.r * 0.3f), b1 = head + u * (s.r * 0.35f);  // the blade
  vfx::quad(b0, b1, b1 + n * (s.r * 1.15f) + u * (s.r * 0.25f), b0 + n * (s.r * 1.05f) - u * (s.r * 0.3f),
            Ap({90, 110, 130, 255}, s.k), Ap({110, 130, 150, 255}, s.k), Ap({230, 245, 255, 255}, s.k), Ap({210, 230, 250, 255}, s.k));
  DrawLineEx(V(b1 + n * (s.r * 1.15f) + u * (s.r * 0.25f)), V(b0 + n * (s.r * 1.05f) - u * (s.r * 0.3f)), 2.5f, Ap(WHITE, s.k));
  vfx::additive(true);
  vfx::glow(head + n * (s.r * 0.5f), s.r * 0.35f, Ap({120, 200, 255, 255}, 0.8f * s.k), Ap(frost, 0.0f), 10);  // rune
  vfx::additive(false);
}

void shotChainBlade(const Match& m, const Projectile& p, const Shot& s, float time) {
  if (p.owner >= 0 && p.owner < int(m.fighters.size())) {  // the chain back to the hand
    const Vec2 h = m.fighters[p.owner].center();
    const Vec2 dd = s.c - h;
    const int links = std::clamp(int(dd.len() / 12.0f), 1, 60);
    for (int i = 0; i < links; ++i) {
      const float t = (i + 0.5f) / links;
      const Vec2 q = h + dd * t + Vec2{0.0f, std::sin(t * kPi) * 14.0f};
      DrawCircleV(V(q), 3.2f, Ap({70, 60, 56, 255}, s.k));
      DrawCircleV(V(q), 1.6f, Ap({255, 140, 50, 255}, s.k));
    }
  }
  vfx::additive(true);
  vfx::glow(s.c, s.r * 1.8f, Ap({255, 110, 30, 255}, 0.5f * s.k), Ap({255, 80, 20, 255}, 0.0f));
  vfx::additive(false);
  const float spin = time * 20.0f;
  const Vec2 u{std::cos(spin), std::sin(spin)};
  vfx::wedge(s.c - u * s.r, s.c + u * (s.r * 1.4f), s.r * 0.5f, 1.0f, Ap({200, 60, 30, 255}, s.k), Ap({255, 220, 160, 255}, s.k));
}

void shotBlueOrb(const Shot& s, float time) {
  const Color b{90, 160, 255, 255};
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.6f, Ap(b, 0.35f * s.k), Ap(b, 0.0f));
  for (int arm = 0; arm < 5; ++arm) {  // vortex arms spiralling inwards
    Vec2 prev{};
    for (int k = 0; k <= 10; ++k) {
      const float u = k / 10.0f;
      const float a = -time * 6.0f + arm * 2.0f * kPi / 5.0f + u * 3.0f;
      const Vec2 q = s.c + Vec2{std::cos(a), std::sin(a)} * (s.r * (2.8f - 2.4f * u));
      if (k > 0) vfx::wedge(prev, q, 1.0f + 4.0f * u, 1.0f + 4.0f * u, Ap(b, 0.6f * u * s.k), Ap({200, 230, 255, 255}, 0.7f * u * s.k));
      prev = q;
    }
  }
  for (int i = 0; i < 2; ++i) {  // space folding in
    const float ph = std::fmod(time * 2.0f + i * 0.5f, 1.0f);
    vfx::softRing(s.c, s.r * (2.6f - 2.0f * ph), 6.0f, Ap({170, 210, 255, 255}, 0.5f * ph * s.k));
  }
  vfx::glow(s.c, s.r * 0.9f, Ap({210, 235, 255, 255}, 0.95f * s.k), Ap(b, 0.0f));
  vfx::glow(s.c, s.r * 0.4f, Ap(WHITE, s.k), Ap(WHITE, 0.0f));
  vfx::additive(false);
}

void shotRedOrb(const Shot& s, float time) {
  const Color r{255, 50, 70, 255};
  vfx::additive(true);
  vfx::streak(s.c - s.d * (s.r * 5.0f), s.c, 0.0f, s.r * 1.6f, Ap(r, 0.0f), Ap(r, 0.5f * s.k));
  vfx::glow(s.c, s.r * 2.4f, Ap(r, 0.4f * s.k), Ap(r, 0.0f));
  vfx::burst(s.c, 8, s.r * 0.6f, s.r * 2.2f, 0.35f, time * -9.0f, Ap({255, 150, 160, 255}, 0.8f * s.k), Ap(r, 0.0f), s.seed);
  for (int i = 0; i < 2; ++i) {  // pushing space away
    const float ph = std::fmod(time * 3.0f + i * 0.5f, 1.0f);
    vfx::softRing(s.c, s.r * (0.8f + 1.8f * ph), 5.0f, Ap({255, 120, 130, 255}, 0.6f * (1.0f - ph) * s.k));
  }
  vfx::glow(s.c, s.r * 0.9f, Ap({255, 220, 220, 255}, s.k), Ap(r, 0.0f));
  vfx::additive(false);
}

void shotPurpleOrb(const Shot& s, float time) {
  const Color v{170, 80, 255, 255};
  // The erased trail: a band of nothingness behind it
  vfx::streak(s.c - s.d * (s.r * 3.4f), s.c, s.r * 1.0f, s.r * 1.9f, Ap({8, 0, 14, 255}, 0.0f), Ap({8, 0, 14, 255}, 0.55f * s.k));
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.0f, Ap(v, 0.4f * s.k), Ap(v, 0.0f));
  vfx::additive(false);
  vfx::glow(s.c, s.r * 0.95f, Ap({20, 0, 30, 255}, 0.97f * s.k), Ap({60, 10, 90, 255}, 0.97f * s.k), 36);  // devouring core
  vfx::additive(true);
  for (int i = 0; i < 2; ++i) {  // blue and red, still swirling into one another
    const Color col = i ? Color{255, 70, 100, 255} : Color{80, 150, 255, 255};
    const float a0 = time * 5.0f + i * kPi;
    Vec2 prev{};
    for (int k = 0; k <= 16; ++k) {
      const float a = a0 + k * kPi / 16.0f;
      const Vec2 q = s.c + Vec2{std::cos(a), std::sin(a)} * (s.r * 1.02f);
      if (k > 0) vfx::wedge(prev, q, s.r * 0.12f, s.r * 0.12f, Ap(col, 0.8f * s.k), Ap(col, 0.8f * s.k));
      prev = q;
    }
  }
  vfx::softRing(s.c, s.r, s.r * 0.35f, Ap({220, 160, 255, 255}, 0.9f * s.k));
  vfx::softRing(s.c, s.r * 0.98f, 4.0f, Ap(WHITE, s.k));
  const uint32_t fs = s.seed + uint32_t(time * 30.0f) * 53u;
  for (int i = 0; i < 6; ++i) {  // crackling energy
    const float a = hash01(fs + i * 3u) * 2.0f * kPi;
    const Vec2 d{std::cos(a), std::sin(a)};
    vfx::bolt(s.c + d * (s.r * 0.9f), s.c + d * (s.r * (1.4f + 0.5f * hash01(fs + i * 5u))), s.r * 0.12f, 5, fs + i * 11u,
              std::max(2.0f, s.r * 0.04f), Ap({200, 140, 255, 255}, 0.9f * s.k), Ap(WHITE, s.k));
  }
  for (int i = 0; i < 2; ++i) {  // space caving in around it
    const float ph = std::fmod(time * 2.5f + i * 0.5f, 1.0f);
    vfx::softRing(s.c, s.r * (2.2f - 1.0f * ph), s.r * 0.1f, Ap(v, 0.45f * ph * s.k));
  }
  vfx::additive(false);
}

void shotDismantle(const Projectile& p, const Shot& s) {
  // Invisible cuts: only thin slashes flashing in the air along its path give it away.
  vfx::additive(true);
  for (int k = 0; k < 6; ++k) {
    const uint32_t sd = s.seed * 31u + uint32_t(p.age - k) * 7919u;
    const Vec2 q = s.c - p.vel * (k * 1.3f) + s.n * ((hash01(sd + 1) - 0.5f) * s.r * 2.0f);
    const float a = hash01(sd + 2) * kPi, len = 22.0f + hash01(sd + 3) * 46.0f;
    const Vec2 d{std::cos(a), std::sin(a)};
    const float al = (1.0f - k / 6.0f) * s.k;
    vfx::wedge(q, q + d * len * 0.5f, 2.6f, 0.0f, Ap(WHITE, al), Ap(WHITE, al));
    vfx::wedge(q, q - d * len * 0.5f, 2.6f, 0.0f, Ap(WHITE, al), Ap(WHITE, al));
    vfx::streak(q - d * len * 0.5f, q + d * len * 0.5f, 6.0f, 6.0f, Ap({255, 60, 70, 255}, 0.3f * al), Ap({255, 60, 70, 255}, 0.3f * al));
  }
  vfx::additive(false);
}

// Fuga: Sukuna's arrow of fire (the PixelLab effect fx_fuga_arrow: a white-hot head, flames streaming back).
void shotFireArrow(const Shot& s, float time) {
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.6f, Ap({255, 110, 30, 255}, 0.45f * s.k), Ap({255, 80, 20, 255}, 0.0f));
  vfx::additive(false);
  if (art::drawFx("fuga_arrow", time, s.c, s.r * 9.0f / 110.0f, std::atan2(s.d.y, s.d.x), false, s.k)) return;
  vfx::additive(true);
  for (int i = 0; i < 6; ++i) {  // without the effect: flames licking back from the head
    const float l = s.r * (3.0f + 2.5f * hash01(s.seed + i) + std::sin(time * 30.0f + i * 1.7f));
    const Vec2 tail = s.c - s.d * l + s.n * (std::sin(time * 25.0f + i * 2.3f) * s.r * 0.55f);
    vfx::streak(s.c + s.d * (s.r * 0.4f), tail, s.r * 1.1f, 0.0f, Ap(i % 2 ? Color{255, 140, 40, 255} : Color{255, 70, 20, 255}, 0.7f * s.k),
                Ap({255, 40, 10, 255}, 0.0f));
  }
  vfx::glow(s.c + s.d * (s.r * 0.8f), s.r * 0.9f, Ap({255, 250, 220, 255}, s.k), Ap({255, 200, 120, 255}, 0.0f));
  vfx::additive(false);
}

void shotLightning(const Shot& s, float time) {
  const uint32_t fs = s.seed + uint32_t(time * 60.0f) * 101u;  // a new bolt every frame
  const Vec2 a = s.c - s.d * (s.r * 8.0f), b = s.c + s.d * s.r;
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.2f, Ap({150, 190, 255, 255}, 0.45f * s.k), Ap({150, 190, 255, 255}, 0.0f));
  vfx::bolt(a, b, s.r * 0.7f, 10, fs, 14.0f, Ap({120, 120, 255, 255}, 0.35f * s.k));
  vfx::bolt(a, b, s.r * 0.7f, 10, fs, 6.0f, Ap({190, 220, 255, 255}, 0.9f * s.k), Ap(WHITE, s.k));
  for (int i = 0; i < 3; ++i) {  // forks
    const float t = 0.2f + 0.6f * hash01(fs + i * 3u);
    const Vec2 p0 = lerpv(a, b, t) + s.n * ((hash01(fs + i * 5u) - 0.5f) * s.r);
    const float side = hash01(fs + i * 7u) < 0.5f ? -1.0f : 1.0f;
    const Vec2 p1 = p0 + s.n * (side * s.r * (1.2f + hash01(fs + i * 9u))) - s.d * (s.r * 0.8f);
    vfx::bolt(p0, p1, s.r * 0.3f, 4, fs + i * 13u, 3.0f, Ap({190, 220, 255, 255}, 0.8f * s.k), Ap(WHITE, 0.9f * s.k));
  }
  vfx::glow(b, s.r * 0.9f, Ap(WHITE, s.k), Ap({190, 220, 255, 255}, 0.0f));
  vfx::additive(false);
}

// ---- v5 shots --------------------------------------------------------------------------------------

// The warning before a strike lands (armDelay): a ring on the spot, a thin line from the sky.
void shotWarning(const Projectile& p, const Shot& s, float time, bool fromSky) {
  const float u = std::min(1.0f, float(p.age) / std::max(1, p.armDelay));
  const Color c = p.hit.fx == HitFx::BlackFlame ? Color{200, 40, 70, 255} : hitFxGlow(p.hit.fx);
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

void shotReiatsuWave(const Shot& s, float time) {
  const Color y{255, 226, 80, 255};
  const float R = s.r * 2.2f, th = R * 0.55f;
  vfx::additive(true);
  vfx::glow(s.c - s.d * (R * 0.3f), R * 1.2f, Ap(y, 0.25f * s.k), Ap(y, 0.0f));
  for (int i = 3; i >= 1; --i)
    crescentAt(s.c - s.d * (i * s.r * 0.5f), s.d, R, th, 1.35f, Ap(y, 0.0f), Ap(y, 0.3f * s.k / i));
  vfx::additive(false);
  crescentAt(s.c, s.d, R, th, 1.35f, Ap({40, 30, 10, 255}, 0.9f * s.k), Ap({60, 46, 12, 255}, s.k));
  vfx::additive(true);
  crescentAt(s.c + s.d * 3.0f, s.d, R * 1.03f, th * 0.4f, 1.3f, Ap(y, 0.5f * s.k), Ap({255, 250, 200, 255}, s.k));
  const uint32_t fs = s.seed + uint32_t(time * 24.0f) * 37u;
  for (int i = 0; i < 7; ++i) {  // wild spikes of reiatsu
    const float a = std::atan2(s.d.y, s.d.x) + (hash01(fs + i) - 0.5f) * 2.6f;
    const Vec2 o = s.c - s.d * R + Vec2{std::cos(a), std::sin(a)} * (R * 0.95f);
    vfx::wedge(o, o + Vec2{std::cos(a), std::sin(a)} * (R * (0.2f + 0.3f * hash01(fs + 9 + i))), 5.0f, 0.0f, Ap(y, 0.8f * s.k),
               Ap(WHITE, 0.6f * s.k));
  }
  vfx::additive(false);
}

void shotCeroOscuras(const Shot& s, float time) {
  const Color g{80, 240, 140, 255};
  vfx::streak(s.c - s.d * (s.r * 4.0f), s.c, 0.0f, s.r * 1.5f, Ap({8, 14, 10, 255}, 0.0f), Ap({8, 14, 10, 255}, 0.6f * s.k));
  vfx::glow(s.c, s.r * 1.4f, Ap({6, 10, 8, 255}, 0.95f * s.k), Ap({6, 10, 8, 255}, 0.0f));
  vfx::additive(true);
  vfx::burst(s.c, 8, s.r * 0.6f, s.r * 1.9f, 0.32f, time * 10.0f, Ap(g, 0.7f * s.k), Ap(g, 0.0f), s.seed);
  vfx::softRing(s.c, s.r * 0.9f, s.r * 0.3f, Ap(g, 0.9f * s.k));
  vfx::additive(false);
  vfx::glow(s.c, s.r * 0.55f, Ap({0, 0, 0, 255}, s.k), Ap({10, 30, 16, 255}, s.k));
}

void spear(const Shot& s, float len, float w, Color body, Color core, Color glow) {
  const Vec2 tip = s.c + s.d * (len * 0.45f), tail = s.c - s.d * (len * 0.55f);
  vfx::additive(true);
  vfx::streak(tail - s.d * (len * 0.6f), tail, 0.0f, w * 1.6f, Ap(glow, 0.0f), Ap(glow, 0.35f * s.k));
  vfx::streak(tail, tip, w * 2.4f, w * 2.4f, Ap(glow, 0.3f * s.k), Ap(glow, 0.3f * s.k));
  vfx::additive(false);
  vfx::wedge(tail, tip - s.d * (w * 2.0f), w * 0.7f, w * 0.7f, Ap(body, s.k), Ap(body, s.k));
  vfx::tri(tip, tip - s.d * (w * 3.4f) + s.n * (w * 1.4f), tip - s.d * (w * 3.4f) - s.n * (w * 1.4f), Ap(body, s.k), Ap(body, s.k),
           Ap(body, s.k));
  vfx::additive(true);
  vfx::wedge(tail, tip, w * 0.25f, w * 0.1f, Ap(core, s.k), Ap(core, s.k));
  vfx::glow(tip, w * 1.6f, Ap(core, 0.8f * s.k), Ap(glow, 0.0f));
  vfx::additive(false);
}

void shotLuzDeLaLuna(const Shot& s, float time) {  // a smaller javelin of the same teal light as the Lanza
  if (art::drawFx("lanza", time, s.c, s.r * 8.0f / 194.0f, std::atan2(s.d.y, s.d.x), false, s.k)) return;
  spear(s, s.r * 7.0f, s.r * 0.45f, {90, 240, 150, 255}, {230, 255, 236, 255}, {70, 255, 140, 255});
}

// Lanza del Relámpago: the PixelLab javelin (fx_lanza: a barbed head of teal light, a crackling shaft).
void shotLanza(const Shot& s, float time) {
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.2f, Ap({60, 230, 200, 255}, 0.4f * s.k), Ap({60, 230, 200, 255}, 0.0f));
  vfx::additive(false);
  if (art::drawFx("lanza", time, s.c, s.r * 10.0f / 194.0f, std::atan2(s.d.y, s.d.x), false, s.k)) return;
  spear(s, s.r * 6.0f, s.r * 0.55f, {60, 220, 120, 255}, {230, 255, 236, 255}, {70, 255, 140, 255});
  const uint32_t fs = s.seed + uint32_t(time * 40.0f) * 71u;
  vfx::additive(true);
  for (int i = 0; i < 4; ++i) {  // green lightning wrapping the javelin
    const Vec2 a = s.c - s.d * (s.r * (3.0f - i * 1.2f)), b = a + s.d * (s.r * 1.6f) + s.n * ((hash01(fs + i) - 0.5f) * s.r * 2.0f);
    vfx::bolt(a, b, s.r * 0.4f, 5, fs + i * 7u, 2.5f, Ap({120, 255, 170, 255}, 0.9f * s.k), Ap(WHITE, s.k));
  }
  vfx::additive(false);
}

void shotBarrel(const Projectile& p, const Shot& s, float time) {
  const float r = s.r, rot = p.age * 0.18f * (p.vel.x >= 0 ? 1.0f : -1.0f);
  const Vec2 u{std::cos(rot), std::sin(rot)}, n{-u.y, u.x};
  auto P = [&](float x, float y) { return s.c + u * (x * r) + n * (y * r); };
  const Color wood{150, 96, 56, 255}, dark{90, 56, 34, 255}, red{200, 44, 36, 255};
  vfx::quad(P(-1.0f, -0.8f), P(1.0f, -0.8f), P(1.0f, 0.8f), P(-1.0f, 0.8f), Ap(dark, s.k), Ap(dark, s.k), Ap(dark, s.k), Ap(dark, s.k));
  vfx::quad(P(-0.9f, -0.72f), P(0.9f, -0.72f), P(0.9f, 0.72f), P(-0.9f, 0.72f), Ap(wood, s.k), Ap(wood, s.k), Ap(wood, s.k), Ap(wood, s.k));
  vfx::quad(P(-0.15f, -0.72f), P(0.15f, -0.72f), P(0.15f, 0.72f), P(-0.15f, 0.72f), Ap(red, s.k), Ap(red, s.k), Ap(red, s.k), Ap(red, s.k));
  const Vec2 fuse = P(0.0f, -0.8f) + n * (-r * 0.3f);
  DrawLineEx(V(P(0.0f, -0.8f)), V(fuse), 2.0f, Ap({60, 50, 40, 255}, s.k));
  vfx::additive(true);
  vfx::glow(fuse, r * (0.35f + 0.15f * std::sin(time * 40.0f)), Ap({255, 220, 120, 255}, s.k), Ap({255, 120, 40, 255}, 0.0f));
  vfx::additive(false);
}

void shotFlash(const Shot& s, Color c) {
  vfx::additive(true);
  vfx::glow(s.c, s.r * 3.0f, Ap(c, 0.8f), Ap(c, 0.0f));
  vfx::glow(s.c, s.r * 1.2f, Ap(WHITE, 0.9f), Ap(WHITE, 0.0f));
  vfx::additive(false);
}

void shotShell(const Shot& s, float time) {  // gunlance shelling: a cone of fire
  vfx::additive(true);
  for (int i = 0; i < 5; ++i) {
    const float l = s.r * (2.0f + 1.5f * hash01(s.seed + i)) * (1.0f + 0.2f * std::sin(time * 40.0f + i));
    const Vec2 e = s.c + s.d * l + s.n * ((i - 2) * s.r * 0.35f);
    vfx::streak(s.c - s.d * (s.r * 0.8f), e, s.r * 0.6f, s.r * 1.1f, Ap({255, 220, 120, 255}, 0.9f * s.k), Ap({255, 80, 20, 255}, 0.0f));
  }
  vfx::glow(s.c, s.r * 1.4f, Ap({255, 240, 200, 255}, s.k), Ap({255, 120, 40, 255}, 0.0f));
  vfx::additive(false);
}

void shotRasengan(const Shot& s, float time) {
  const Color b{90, 170, 255, 255};
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.0f, Ap(b, 0.35f * s.k), Ap(b, 0.0f));
  for (int arm = 0; arm < 4; ++arm) {
    Vec2 prev{};
    for (int k = 0; k <= 8; ++k) {
      const float u = k / 8.0f, a = time * 18.0f + arm * kPi * 0.5f + u * 2.5f;
      const Vec2 q = s.c + Vec2{std::cos(a), std::sin(a)} * (s.r * (0.3f + 0.75f * u));
      if (k > 0) vfx::wedge(prev, q, 3.0f * (1.0f - u) + 1.0f, 3.0f * (1.0f - u) + 1.0f, Ap({220, 240, 255, 255}, 0.8f * s.k), Ap(b, 0.6f * s.k));
      prev = q;
    }
  }
  vfx::glow(s.c, s.r * 0.5f, Ap(WHITE, s.k), Ap(b, 0.0f));
  vfx::additive(false);
}

void shotRasenshuriken(const Shot& s, float time) {
  shotRasengan(s, time);
  vfx::additive(true);
  const float rot = time * 22.0f;
  for (int i = 0; i < 4; ++i) {  // the four great blades of wind
    const float a = rot + i * kPi * 0.5f;
    const Vec2 d{std::cos(a), std::sin(a)}, n{-d.y, d.x};
    const Vec2 b0 = s.c + d * (s.r * 0.6f), tip = s.c + d * (s.r * 3.0f) + n * (s.r * 0.9f);
    vfx::tri(b0 + n * (s.r * 0.35f), b0 - n * (s.r * 0.35f), tip, Ap(WHITE, 0.85f * s.k), Ap({200, 230, 255, 255}, 0.7f * s.k),
             Ap(WHITE, 0.2f * s.k));
  }
  vfx::softRing(s.c, s.r * 2.6f, s.r * 0.25f, Ap({230, 245, 255, 255}, 0.55f * s.k));  // ring of micro-blades
  vfx::additive(false);
}

void shotBijudama(const Shot& s, float time) {
  const Color v{120, 60, 200, 255};
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.2f, Ap(v, 0.4f * s.k), Ap(v, 0.0f));
  vfx::additive(false);
  vfx::glow(s.c, s.r, Ap({10, 0, 16, 255}, 0.97f * s.k), Ap({40, 10, 60, 255}, 0.97f * s.k), 36);
  vfx::additive(true);
  for (int i = 0; i < 3; ++i) {
    const float ph = std::fmod(time * 2.0f + i / 3.0f, 1.0f);
    vfx::softRing(s.c, s.r * (0.4f + 0.7f * ph), 4.0f, Ap({200, 150, 255, 255}, 0.6f * (1.0f - ph) * s.k));
  }
  vfx::softRing(s.c, s.r, s.r * 0.2f, Ap({180, 120, 255, 255}, 0.8f * s.k));
  vfx::additive(false);
}

void shotGreatFireball(const Shot& s, float time) {
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.2f, Ap({255, 120, 30, 255}, 0.45f * s.k), Ap({255, 80, 20, 255}, 0.0f));
  for (int i = 0; i < 8; ++i) {  // rolling flames
    const float a = time * 6.0f + i * kPi / 4.0f;
    const Vec2 q = s.c + Vec2{std::cos(a), std::sin(a)} * (s.r * 0.55f);
    vfx::glow(q, s.r * (0.6f + 0.15f * std::sin(time * 20.0f + i)), Ap(i % 2 ? Color{255, 170, 50, 255} : Color{255, 90, 20, 255}, 0.8f * s.k),
              Ap({255, 60, 10, 255}, 0.0f));
  }
  vfx::streak(s.c - s.d * (s.r * 2.8f), s.c, 0.0f, s.r * 1.3f, Ap({255, 80, 20, 255}, 0.0f), Ap({255, 140, 40, 255}, 0.6f * s.k));
  vfx::glow(s.c, s.r * 0.7f, Ap({255, 250, 210, 255}, s.k), Ap({255, 200, 100, 255}, 0.0f));
  vfx::additive(false);
}

void shotChidori(const Shot& s, float time) {
  spear(s, s.r * 6.0f, s.r * 0.35f, {170, 210, 255, 255}, WHITE, {120, 170, 255, 255});
  const uint32_t fs = s.seed + uint32_t(time * 60.0f) * 31u;
  vfx::additive(true);
  for (int i = 0; i < 3; ++i)
    vfx::bolt(s.c - s.d * (s.r * 3.0f), s.c + s.d * (s.r * 2.6f), s.r * 0.8f, 7, fs + i * 9u, 2.0f, Ap({200, 230, 255, 255}, 0.8f * s.k));
  vfx::additive(false);
}

// Kirin: the thunder dragon falling from the storm on its target.
void shotKirin(const Shot& s, float time) {
  const Rect v = vfx::viewRect(40.0f);
  const Vec2 top{s.c.x + s.r * 1.2f, v.y - 40.0f};
  const uint32_t fs = s.seed + uint32_t(time * 60.0f) * 97u;
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.6f, Ap({150, 190, 255, 255}, 0.55f * s.k), Ap({150, 190, 255, 255}, 0.0f));
  for (int k = 0; k < 3; ++k) {  // the dragon's body: braided bolts
    vfx::bolt(top + Vec2{(k - 1) * s.r * 0.4f, 0}, s.c, s.r * 0.8f, 14, fs + k * 13u, 26.0f - k * 6.0f, Ap({120, 140, 255, 255}, 0.35f * s.k));
    vfx::bolt(top + Vec2{(k - 1) * s.r * 0.4f, 0}, s.c, s.r * 0.8f, 14, fs + k * 13u, 8.0f, Ap({200, 230, 255, 255}, 0.9f * s.k), Ap(WHITE, s.k));
  }
  // the head: jaws of lightning opening on the target
  const Vec2 h = s.c + Vec2{0.0f, -s.r * 0.8f};
  vfx::wedge(h + Vec2{-s.r * 1.2f, -s.r * 0.6f}, h + Vec2{s.r * 0.2f, s.r * 0.6f}, 14.0f, 2.0f, Ap({200, 230, 255, 255}, s.k), Ap(WHITE, s.k));
  vfx::wedge(h + Vec2{s.r * 1.2f, -s.r * 0.6f}, h + Vec2{-s.r * 0.2f, s.r * 0.6f}, 14.0f, 2.0f, Ap({200, 230, 255, 255}, s.k), Ap(WHITE, s.k));
  vfx::glow(s.c, s.r * 1.2f, Ap(WHITE, s.k), Ap({200, 230, 255, 255}, 0.0f));
  vfx::additive(false);
}

void shotIndraArrow(const Shot& s, float time) {
  const Color v{170, 90, 255, 255};
  vfx::additive(true);
  vfx::streak(s.c - s.d * (s.r * 9.0f), s.c, 0.0f, s.r * 1.6f, Ap(v, 0.0f), Ap(v, 0.55f * s.k));
  for (int i = 0; i < 6; ++i) {  // purple flames streaming back
    const Vec2 tail = s.c - s.d * (s.r * (3.0f + 3.0f * hash01(s.seed + i))) + s.n * (std::sin(time * 25.0f + i * 2.0f) * s.r * 0.8f);
    vfx::streak(s.c, tail, s.r * 0.9f, 0.0f, Ap({210, 150, 255, 255}, 0.7f * s.k), Ap(v, 0.0f));
  }
  vfx::additive(false);
  spear(s, s.r * 3.0f, s.r * 0.3f, {200, 140, 255, 255}, WHITE, v);
}

// Amaterasu: black flames with glowing sandy edges burning on the ground under the target (the PixelLab effect
// fx_amaterasu). They rise out of the ground during the warning, burn while it lasts and sink as it ends.
void shotBlackFlames(const Projectile& p, const Shot& s, float time) {
  const float grow = smoothstep(clampf((p.age + 1.0f) / std::max(8.0f, float(p.armDelay)), 0.0f, 1.0f));
  const float end = clampf(p.life / 16.0f, 0.0f, 1.0f);
  const float h = s.r * 2.7f * (0.25f + 0.75f * grow) * (0.4f + 0.6f * end);  // flame height, world units
  const Vec2 base = s.c + Vec2{0.0f, s.r};
  const float a = std::min(1.0f, 2.0f * grow) * std::min(1.0f, 2.0f * end);
  vfx::additive(true);
  vfx::glow(base, s.r * 1.6f, Ap({240, 150, 70, 255}, 0.35f * a), Ap({240, 150, 70, 255}, 0.0f));
  vfx::additive(false);
  if (art::drawFx("amaterasu", float(p.age) / 60.0f + hash01(s.seed) * 2.0f, base, h / 100.0f, 0.0f, (s.seed & 1u) != 0, a))
    return;
  for (int i = 0; i < 10; ++i) {  // without the effect: tongues of black fire
    const float x = (hash01(s.seed + i * 3u) - 0.5f) * s.r * 1.6f;
    const float th = h * 0.6f * (1.0f + 0.5f * hash01(s.seed + i * 3u + 1)) * (0.8f + 0.3f * std::sin(time * 14.0f + i * 1.7f));
    const Vec2 b = base + Vec2{x, 0.0f}, tip = b + Vec2{std::sin(time * 9.0f + i) * s.r * 0.25f, -th};
    vfx::wedge(b, tip, s.r * 0.4f, 1.0f, Ap({10, 4, 12, 255}, 0.92f * a), Ap({10, 4, 12, 255}, 0.4f * a));
  }
}

void shotKunai(const Shot& s) {
  const Vec2 tip = s.c + s.d * (s.r * 1.6f), back = s.c - s.d * (s.r * 1.2f);
  vfx::tri(tip, s.c + s.n * (s.r * 0.55f), s.c - s.n * (s.r * 0.55f), Ap({170, 176, 190, 255}, s.k), Ap({110, 116, 130, 255}, s.k),
           Ap({110, 116, 130, 255}, s.k));
  DrawLineEx(V(s.c), V(back), std::max(2.0f, s.r * 0.35f), Ap({40, 40, 46, 255}, s.k));
  DrawRing(V(back), s.r * 0.25f, s.r * 0.4f, 0, 360, 12, Ap({150, 156, 168, 255}, s.k));
}

void shotWaterDragon(const Projectile& p, const Shot& s, float time) {
  const Color w{70, 160, 240, 255}, foam{220, 240, 255, 255};
  Vec2 pts[10];
  for (int i = 0; i < 10; ++i) {
    const float u = i / 9.0f;
    pts[i] = s.c - s.d * (u * s.r * 5.0f) + s.n * (std::sin(time * 8.0f + u * 6.0f + p.age * 0.1f) * s.r * 0.7f * u);
  }
  for (int i = 9; i > 0; --i) {
    const float w0 = s.r * (1.0f - 0.08f * i), w1 = s.r * (1.0f - 0.08f * (i - 1));
    vfx::wedge(pts[i], pts[i - 1], w0 * 1.1f, w1 * 1.1f, Ap(shade(w, 0.7f), 0.9f * s.k), Ap(shade(w, 0.7f), 0.9f * s.k));
    vfx::wedge(pts[i], pts[i - 1], w0 * 0.8f, w1 * 0.8f, Ap(w, 0.95f * s.k), Ap(w, 0.95f * s.k));
  }
  // head: jaws and eyes
  const Vec2 h = pts[0];
  vfx::tri(h + s.n * (s.r * 0.8f), h - s.n * (s.r * 0.8f), h + s.d * (s.r * 1.6f), Ap(w, s.k), Ap(w, s.k), Ap(foam, s.k));
  DrawCircleV(V(h + s.d * (s.r * 0.3f) - s.n * (s.r * 0.4f)), std::max(1.5f, s.r * 0.12f), Ap(WHITE, s.k));
  vfx::additive(true);
  for (int i = 0; i < 6; ++i) vfx::glow(pts[i + 2] + s.n * (s.r * 0.6f), s.r * 0.25f, Ap(foam, 0.5f * s.k), Ap(foam, 0.0f));
  vfx::additive(false);
}

// Kamui: space itself twists into a point and sucks in what stands there (the frame drawn so far is swirled,
// pix::swirl), with the PixelLab rift turning on top (fx_kamui: a spiral collapsing into a black hole). It opens
// during the warning, keeps pulling inwards while it lasts and collapses into a dot as it ends.
void shotKamuiVortex(const Projectile& p, const Shot& s) {
  const float open = smoothstep(clampf((p.age + 1.0f) / std::max(8.0f, float(p.armDelay)), 0.0f, 1.0f));
  const float end = clampf(p.life / 14.0f, 0.0f, 1.0f);  // 1 until its last 14 frames
  const float R = s.r * 2.4f * open * (0.3f + 0.7f * end);
  pix::swirl(s.c, R * 1.25f, (2.5f + 0.12f * p.age) * open, 0.3f + 0.5f * open, 0.6f * open, std::min(1.0f, 2.0f * open));
  const art::FxInfo* fx = art::fxInfo("kamui");
  if (!fx || R < 1.0f) return;
  const int frame = end >= 1.0f ? (p.age / 3) % 4 : std::min(fx->frames - 1, 4 + int((1.0f - end) * (fx->frames - 4)));
  art::drawFxFrame("kamui", frame, s.c, R / (fx->fw * 0.46f), -p.age * 0.12f, false, 0.6f * std::min(1.0f, 2.0f * open));
}

void shuriken4(const Shot& s, float time, float R, Color body, Color edge) {
  const float rot = time * 18.0f;
  for (int i = 0; i < 4; ++i) {
    const float a = rot + i * kPi * 0.5f;
    const Vec2 d{std::cos(a), std::sin(a)}, n{-d.y, d.x};
    vfx::tri(s.c + n * (R * 0.32f), s.c - n * (R * 0.32f), s.c + d * R + n * (R * 0.25f), Ap(body, s.k), Ap(body, s.k), Ap(edge, s.k));
  }
  DrawCircleV(V(s.c), R * 0.22f, Ap(shade(body, 0.6f), s.k));
}

void shotKamuiShuriken(const Shot& s, float time) {
  vfx::additive(true);
  vfx::glow(s.c, s.r * 1.6f, Ap({90, 140, 255, 255}, 0.35f * s.k), Ap({90, 140, 255, 255}, 0.0f));
  vfx::additive(false);
  shuriken4(s, time, s.r * 1.3f, {60, 80, 140, 255}, {170, 210, 255, 255});
}

void shotLightPillar(const Shot& s, float time) {
  const Rect v = vfx::viewRect(40.0f);
  const Color g{255, 220, 120, 255};
  vfx::additive(true);
  vfx::quad({s.c.x - s.r * 0.9f, v.y}, {s.c.x + s.r * 0.9f, v.y}, {s.c.x + s.r, s.c.y + s.r}, {s.c.x - s.r, s.c.y + s.r}, Ap(g, 0.15f * s.k),
            Ap(g, 0.15f * s.k), Ap(g, 0.7f * s.k), Ap(g, 0.7f * s.k));
  vfx::quad({s.c.x - s.r * 0.3f, v.y}, {s.c.x + s.r * 0.3f, v.y}, {s.c.x + s.r * 0.4f, s.c.y + s.r}, {s.c.x - s.r * 0.4f, s.c.y + s.r},
            Ap(WHITE, 0.4f * s.k), Ap(WHITE, 0.4f * s.k), Ap(WHITE, s.k), Ap(WHITE, s.k));
  vfx::glow(s.c + Vec2{0, s.r * 0.6f}, s.r * 1.8f, Ap(g, 0.6f * s.k), Ap(g, 0.0f));
  vfx::burst(s.c, 10, s.r * 0.5f, s.r * 2.2f, 0.2f, time * 3.0f, Ap(WHITE, 0.6f * s.k), Ap(g, 0.0f), s.seed);
  vfx::additive(false);
}

void shotGloomPillar(const Shot& s, float time) {
  const Color r{200, 20, 50, 255}, ink{14, 2, 8, 255};
  const float h = s.r * 3.4f;
  const Vec2 base = s.c + Vec2{0, s.r * 1.2f};
  vfx::additive(true);
  vfx::glow(base, s.r * 1.6f, Ap(r, 0.5f * s.k), Ap(r, 0.0f));
  vfx::additive(false);
  for (int i = 0; i < 7; ++i) {  // a column of malice erupting
    const float x = (i - 3) * s.r * 0.22f + std::sin(time * 8.0f + i) * s.r * 0.08f;
    const float hh = h * (0.7f + 0.3f * hash01(s.seed + i)) * (0.85f + 0.15f * std::sin(time * 12.0f + i));
    vfx::wedge(base + Vec2{x, 0}, base + Vec2{x * 0.6f, -hh}, s.r * 0.45f, 2.0f, Ap(ink, 0.95f * s.k), Ap(ink, 0.4f * s.k));
    vfx::additive(true);
    vfx::wedge(base + Vec2{x, 0}, base + Vec2{x * 0.6f, -hh}, s.r * 0.15f, 1.0f, Ap(r, 0.8f * s.k), Ap(r, 0.0f));
    vfx::additive(false);
  }
}

void shotPetalSphere(const Shot& s, float time) {
  vfx::additive(true);
  vfx::glow(s.c, s.r * 1.3f, Ap({255, 140, 190, 255}, 0.3f * s.k), Ap({255, 140, 190, 255}, 0.0f));
  vfx::additive(false);
  for (int i = 0; i < 46; ++i) {  // a sphere of blades closing in, turning on several axes
    const float a = hash01(i * 7 + 1) * 2.0f * kPi + time * (2.0f + hash01(i * 7 + 2) * 2.0f);
    const float b = hash01(i * 7 + 3) * kPi;
    const float rr = s.r * (0.95f + 0.1f * std::sin(time * 6.0f + i));
    const Vec2 q = s.c + Vec2{std::cos(a) * std::sin(b), std::cos(b) * 0.9f + std::sin(a) * std::sin(b) * 0.2f} * rr;
    const Vec2 d = Vec2{-std::sin(a), std::cos(a) * 0.3f}.norm();
    const Color col = i % 3 == 0 ? Color{255, 236, 244, 255} : Color{255, 150, 196, 255};
    petalShape(q, d, 5.0f, 2.2f, Ap(col, 0.92f * s.k));
  }
}

void shotWindRain(const Shot& s) {
  const Color g{140, 240, 180, 255};
  const float R = s.r * 2.2f;
  vfx::additive(true);
  vfx::streak(s.c - s.d * (s.r * 5.0f), s.c, 0.0f, s.r * 1.0f, Ap(g, 0.0f), Ap(g, 0.4f * s.k));
  crescentAt(s.c, s.d, R, R * 0.28f, 1.0f, Ap(g, 0.15f * s.k), Ap({230, 255, 240, 255}, 0.95f * s.k));
  vfx::additive(false);
}

void shotZephyrSpear(const Shot& s, float time) {
  spear(s, s.r * 5.0f, s.r * 0.4f, {150, 240, 190, 255}, WHITE, {120, 255, 180, 255});
  vfx::additive(true);
  for (int k = 0; k < 3; ++k) {  // wind spiralling round the spear
    Vec2 prev{};
    for (int i = 0; i <= 12; ++i) {
      const float u = i / 12.0f;
      const Vec2 q = s.c - s.d * (s.r * (3.0f - 5.0f * u)) + s.n * (std::sin(u * 10.0f - time * 20.0f + k * 2.1f) * s.r * (0.5f + 0.4f * u));
      if (i > 0) vfx::wedge(prev, q, 2.0f, 3.0f, Ap({220, 255, 235, 255}, 0.6f * s.k), Ap({220, 255, 235, 255}, 0.6f * s.k));
      prev = q;
    }
  }
  vfx::additive(false);
}

void shotMaxBlue(const Shot& s, float time) {
  shotBlueOrb(s, time);
  vfx::additive(true);
  for (int i = 0; i < 3; ++i) {  // the stage itself drawn into it
    const float ph = std::fmod(time * 1.2f + i / 3.0f, 1.0f);
    vfx::softRing(s.c, s.r * (4.0f - 3.0f * ph), 8.0f, Ap({120, 180, 255, 255}, 0.5f * ph * s.k));
  }
  vfx::additive(false);
  for (int i = 0; i < 14; ++i) {  // debris spiralling in
    const float u = std::fmod(time * 0.8f + hash01(i * 3 + 1), 1.0f);
    const float a = hash01(i * 3 + 2) * 2.0f * kPi + u * 6.0f;
    const Vec2 q = s.c + Vec2{std::cos(a), std::sin(a)} * (s.r * 3.5f * (1.0f - u));
    DrawPoly(V(q), 4, 4.0f + 3.0f * hash01(i), a * 90.0f, Ap({90, 80, 80, 255}, s.k));
  }
}

// Kamutoke strike: a vertical bolt from the sky onto the spot.
void shotSkyBolt(const Shot& s, float time) {
  const Rect v = vfx::viewRect(40.0f);
  const uint32_t fs = s.seed + uint32_t(time * 60.0f) * 101u;
  const Vec2 a{s.c.x + (hash01(fs) - 0.5f) * s.r, v.y - 20.0f};
  vfx::additive(true);
  vfx::glow(s.c, s.r * 2.0f, Ap({150, 200, 255, 255}, 0.5f * s.k), Ap({150, 200, 255, 255}, 0.0f));
  vfx::bolt(a, s.c, s.r * 0.6f, 12, fs, 18.0f, Ap({120, 140, 255, 255}, 0.4f * s.k));
  vfx::bolt(a, s.c, s.r * 0.6f, 12, fs, 6.0f, Ap({200, 230, 255, 255}, 0.95f * s.k), Ap(WHITE, s.k));
  vfx::glow(s.c, s.r * 0.9f, Ap(WHITE, s.k), Ap({200, 230, 255, 255}, 0.0f));
  vfx::additive(false);
}
}  // namespace

// Projectiles drawn by a PixelLab effect (assets/sprites/fx_<name>.png), sized on their hitbox: the effect's hit
// radius (its fx line in sheets.txt) is the projectile's radius. Without the effect the procedural drawing stays.
enum class FxPlace : uint8_t {
  Travel,  // flies along its velocity (drawn to the right, turned and mirrored to its course); origin on the hitbox
  Area,    // stays upright, origin on the hitbox centre
  Sky,     // comes down from above onto the hitbox centre
  Ground,  // rises from the ground under the hitbox (origin on the column's foot)
};
struct ShotFx {
  ProjVisual v;
  const char* fx;
  FxPlace place;
};
constexpr ShotFx kShotFx[] = {
    {ProjVisual::BlackSlash, "black_divider", FxPlace::Travel},  {ProjVisual::WindRain, "wind_rain", FxPlace::Travel},
    {ProjVisual::ZephyrSpear, "zephyr_spear", FxPlace::Travel},  {ProjVisual::GiantGetsuga, "getsuga", FxPlace::Travel},
    {ProjVisual::GetsugaCross, "getsuga_cross", FxPlace::Travel}, {ProjVisual::PetalSphere, "petal_sphere", FxPlace::Area},
    {ProjVisual::SwordBeam, "sword_beam", FxPlace::Travel},      {ProjVisual::LightPillar, "light_pillar", FxPlace::Sky},
    {ProjVisual::GloomPillar, "gloom_pillar", FxPlace::Ground},  {ProjVisual::PurpleOrb, "hollow_purple", FxPlace::Travel},
    {ProjVisual::MaxBlue, "max_blue", FxPlace::Area},            {ProjVisual::ReiatsuWave, "reiatsu_wave", FxPlace::Travel},
    {ProjVisual::Rasenshuriken, "rasenshuriken", FxPlace::Travel}, {ProjVisual::Bijudama, "bijudama", FxPlace::Travel},
    {ProjVisual::KirinBolt, "kirin", FxPlace::Sky},              {ProjVisual::IndraArrow, "indra_arrow", FxPlace::Travel},
    {ProjVisual::KamuiShuriken, "susanoo_shuriken", FxPlace::Travel}, {ProjVisual::StarrkCero, "starrk_cero", FxPlace::Travel},
    {ProjVisual::CeroWolf, "cero_wolf", FxPlace::Travel},
};

static bool shotSprite(const Projectile& p, const Shot& s, float time) {
  const ShotFx* e = nullptr;
  for (const ShotFx& x : kShotFx)
    if (x.v == p.visual) e = &x;
  const bool skyBolt = p.visual == ProjVisual::Lightning && p.vel.len2() < 1.0f;  // Kamutoke: a bolt from the sky
  const char* name = skyBolt ? "sky_bolt" : e ? e->fx : nullptr;
  const art::FxInfo* fx = name ? art::fxInfo(name) : nullptr;
  if (!fx) return false;
  const FxPlace place = skyBolt ? FxPlace::Sky : e->place;
  const float size = s.r / (fx->hit > 0.0f ? fx->hit : 0.4f * std::min(fx->fw, fx->fh));
  const float t = time + hash01(s.seed) * fx->duration();  // copies side by side do not pulse together
  if (place == FxPlace::Travel) {
    const bool flip = s.d.x < 0.0f;  // mirrored, not upside down, when it flies to the left
    const float rot = flip ? std::atan2(-s.d.y, -s.d.x) : std::atan2(s.d.y, s.d.x);
    return art::drawFx(name, t, s.c, size, rot, flip, s.k);
  }
  const Vec2 at = place == FxPlace::Ground ? s.c + Vec2{0.0f, s.r * 1.2f} : s.c;
  return art::drawFx(name, t, at, size, 0.0f, (s.seed & 1u) != 0 && place == FxPlace::Area, s.k);
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
    if (p.visual == ProjVisual::KamuiVortex) {  // the rift opening is its own warning
      shotKamuiVortex(p, s);
      continue;
    }
    if (p.visual == ProjVisual::BlackFlames) {  // so are the flames rising out of the ground
      shotBlackFlames(p, s, time);
      continue;
    }
    if (!p.armed()) {  // the warning before the strike
      const bool sky = p.visual == ProjVisual::LightPillar || p.visual == ProjVisual::KirinBolt ||
                       p.visual == ProjVisual::Lightning || p.visual == ProjVisual::WindRain;
      shotWarning(p, s, time, sky);
      continue;
    }
    if (p.armDelay > 0) s.k = std::min(s.k, 1.0f);
    if (shotSprite(p, s, time)) continue;
    switch (p.visual) {
      case ProjVisual::BlackSlash: shotBlackSlash(s, time); continue;
      case ProjVisual::WindBlade: shotWindBlade(s); continue;
      case ProjVisual::Tornado: shotTornado(s, time); continue;
      case ProjVisual::Getsuga: shotGetsuga(s, p.hit.fx == HitFx::Dark, time); continue;
      case ProjVisual::Cero: shotCero(s, time); continue;
      case ProjVisual::Petals: shotPetals(s, time); continue;
      case ProjVisual::Arrow: shotArrow(s, false, time); continue;
      case ProjVisual::LightArrow: shotArrow(s, true, time); continue;
      case ProjVisual::Bomb: shotBomb(p, s, time); continue;
      case ProjVisual::SwordBeam: shotSwordBeam(s, time); continue;
      case ProjVisual::DarkOrb: shotDarkOrb(s, time); continue;
      case ProjVisual::GloomSpear: shotGloomSpear(s); continue;
      case ProjVisual::Axe: shotAxe(p, s); continue;
      case ProjVisual::ChainBlade: shotChainBlade(m, p, s, time); continue;
      case ProjVisual::BlueOrb: shotBlueOrb(s, time); continue;
      case ProjVisual::RedOrb: shotRedOrb(s, time); continue;
      case ProjVisual::PurpleOrb: shotPurpleOrb(s, time); continue;
      case ProjVisual::Dismantle: shotDismantle(p, s); continue;
      case ProjVisual::FireArrow: shotFireArrow(s, time); continue;
      case ProjVisual::Lightning:
        if (p.vel.len2() < 1.0f) shotSkyBolt(s, time);
        else shotLightning(s, time);
        continue;
      case ProjVisual::ReiatsuWave: shotReiatsuWave(s, time); continue;
      case ProjVisual::CeroOscuras: shotCeroOscuras(s, time); continue;
      case ProjVisual::LuzDeLaLuna: shotLuzDeLaLuna(s, time); continue;
      case ProjVisual::Lanza: shotLanza(s, time); continue;
      case ProjVisual::BarrelBomb: shotBarrel(p, s, time); continue;
      case ProjVisual::Phial: shotFlash(s, {255, 230, 90, 255}); continue;
      case ProjVisual::GunlanceShell: shotShell(s, time); continue;
      case ProjVisual::Rasengan: shotRasengan(s, time); continue;
      case ProjVisual::Rasenshuriken: shotRasenshuriken(s, time); continue;
      case ProjVisual::ShadowClone: {
        const Fighter& o = m.fighters[std::clamp(p.owner, 0, int(m.fighters.size()) - 1)];
        Pose pose = restPose(*o.def, time * 1.6f + float(i));
        const float H = o.def->height, ph = std::sin(p.age * 0.45f);
        pose.footF = {0.12f * H * ph, 0.5f * H};
        pose.footB = {-0.12f * H * ph, 0.5f * H};
        pose.handF = {0.2f * H, -0.05f * H};
        pose.handB = {-0.25f * H, 0.05f * H};
        pose.lean = 0.35f;
        const int facing = p.vel.x >= 0.0f ? 1 : -1;
        runClip(*o.def, pose, std::fmod(p.age * 0.06f, 1.0f));  // the clone runs like its owner
        drawCharacterSprite(*o.def, o.palette, pose, {p.pos.x, p.pos.y + H * 0.5f}, facing, 1.0f, std::min(1.0f, s.k) * 0.92f,
                            {255, 240, 200, 255}, 0.12f, time);
        if (p.age < 6) shotFlash(s, {240, 240, 240, 255});
        continue;
      }
      case ProjVisual::Bijudama: shotBijudama(s, time); continue;
      case ProjVisual::GreatFireball: shotGreatFireball(s, time); continue;
      case ProjVisual::ChidoriSpear: shotChidori(s, time); continue;
      case ProjVisual::KirinBolt: shotKirin(s, time); continue;
      case ProjVisual::IndraArrow: shotIndraArrow(s, time); continue;
      case ProjVisual::BlackFlames: shotBlackFlames(p, s, time); continue;
      case ProjVisual::Kunai: shotKunai(s); continue;
      case ProjVisual::WaterDragon: shotWaterDragon(p, s, time); continue;
      case ProjVisual::KamuiShuriken: shotKamuiShuriken(s, time); continue;
      case ProjVisual::GiantGetsuga: shotGetsuga(s, false, time); continue;
      case ProjVisual::GetsugaCross: shotGetsugaCross(s, time); continue;
      case ProjVisual::Desgarron: shotDesgarron(s); continue;
      case ProjVisual::PanteraDart: shotPanteraDart(s); continue;
      case ProjVisual::CeroWolf: shotCeroWolf(m, p, s, time); continue;
      case ProjVisual::StarrkCero: shotStarrkCero(s); continue;
      case ProjVisual::LightPillar: shotLightPillar(s, time); continue;
      case ProjVisual::GloomPillar: shotGloomPillar(s, time); continue;
      case ProjVisual::PetalSphere: shotPetalSphere(s, time); continue;
      case ProjVisual::WindRain: shotWindRain(s); continue;
      case ProjVisual::ZephyrSpear: shotZephyrSpear(s, time); continue;
      case ProjVisual::IceAxe: shotAxe(p, s); continue;
      case ProjVisual::MaxBlue: shotMaxBlue(s, time); continue;
      default: break;
    }
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
      default: break;
    }
  }
}

// The Clash Orb as an ancient Zonai device: carved stone shell, teal energy core, rune ring.
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
  themeArt(s.theme);  // bake outside of the clip and transform below
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
    themeArt(s.theme);  // (bakes its own layers first, outside of this one)
    L = bake(area.width, area.height, g_renderScale, [&] { drawStagePreview(s, {0, 0, area.width, area.height}, 2.0f); }, false);
  }
  drawBackdrop(L, area.x, area.y, area.y + area.height);
}
