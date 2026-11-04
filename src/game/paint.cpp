// Painting kit of the stage renderers (see paint.hpp).
#include "game/paint.hpp"

#include "game/ui.hpp"

namespace paint {
float hash(int i) {
  uint32_t x = uint32_t(i) * 2654435761u;
  x ^= x >> 13;
  x *= 0x5bd1e995u;
  x ^= x >> 15;
  return float(x & 0xFFFF) / 65535.0f;
}
float wrapf(float v, float m) {
  v = std::fmod(v, m);
  return v < 0.0f ? v + m : v;
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

// One colour per corner: smooth gradients for free.
void vtx(Vector2 p, Color c) {
  rlColor4ub(c.r, c.g, c.b, c.a);
  rlVertex2f(p.x, p.y);
}
void gtri(Vector2 a, Vector2 b, Vector2 c, Color ca, Color cb, Color cc) {
  rlBegin(RL_TRIANGLES);
  vtx(a, ca);
  vtx(b, cb);
  vtx(c, cc);
  rlEnd();
}
void gquad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color ca, Color cb, Color cc, Color cd) {
  rlBegin(RL_TRIANGLES);
  vtx(a, ca);
  vtx(b, cb);
  vtx(c, cc);
  vtx(a, ca);
  vtx(c, cc);
  vtx(d, cd);
  rlEnd();
}
void vgrad(float x, float y, float w, float h, Color top, Color bottom) {
  gquad({x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, top, top, bottom, bottom);
}
// Radial (elliptic) gradient; with in == out it is a plain ellipse.
// (seg = 0 picks a segment count from the size: software GL pays for every tiny triangle.)
void glow(Vector2 c, float rx, float ry, Color in, Color out, int seg) {
  if (seg <= 0) seg = std::clamp(int(std::max(rx, ry) * 0.6f), 8, 40);
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < seg; ++i) {
    float a0 = 2.0f * kPi * i / seg, a1 = 2.0f * kPi * (i + 1) / seg;
    vtx(c, in);
    vtx({c.x + std::cos(a0) * rx, c.y + std::sin(a0) * ry}, out);
    vtx({c.x + std::cos(a1) * rx, c.y + std::sin(a1) * ry}, out);
  }
  rlEnd();
}
void glow(Vector2 c, float r, Color col, float a, int seg) {
  glow(c, r, r, withAlpha(col, a), withAlpha(col, 0.0f), seg);
}
void ellipse(Vector2 c, float rx, float ry, Color col) { glow(c, rx, ry, col, col); }
void dot(Vector2 c, float r, Color col) { glow(c, r, r, col, col, 8); }
// Ellipse with a feathered rim of `fpx` pixels: soft, painterly edges that never alias.
void softEllipse(Vector2 c, float rx, float ry, Color col, float fpx, int seg) {
  const Color out = withAlpha(col, 0.0f);
  const float k = 1.0f - std::min(0.6f, fpx / std::max(1.0f, std::min(rx, ry)));
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < seg; ++i) {
    float a0 = 2.0f * kPi * i / seg, a1 = 2.0f * kPi * (i + 1) / seg;
    Vector2 o0{c.x + std::cos(a0) * rx, c.y + std::sin(a0) * ry}, o1{c.x + std::cos(a1) * rx, c.y + std::sin(a1) * ry};
    Vector2 i0{c.x + (o0.x - c.x) * k, c.y + (o0.y - c.y) * k}, i1{c.x + (o1.x - c.x) * k, c.y + (o1.y - c.y) * k};
    vtx(c, col);
    vtx(i0, col);
    vtx(i1, col);
    vtx(i0, col);
    vtx(o0, out);
    vtx(o1, out);
    vtx(i0, col);
    vtx(o1, out);
    vtx(i1, col);
  }
  rlEnd();
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

// Thick polyline with round joints.
void stroke(const Vector2* p, int n, float w0, float w1, Color col) {
  for (int i = 0; i + 1 < n; ++i) {
    float w = lerpf(w0, w1, float(i) / std::max(1, n - 2));
    DrawLineEx(p[i], p[i + 1], w, col);
    if (i + 2 < n && w > 3.0f) glow(p[i + 1], w * 0.5f, w * 0.5f, col, col, 8);
  }
}


void gradient(float w, float h, std::initializer_list<Stop> stops) {
  const Stop* prev = nullptr;
  for (const Stop& s : stops) {
    if (prev) vgrad(0, prev->t * h, w, (s.t - prev->t) * h, prev->c, s.c);
    prev = &s;
  }
}

// Cumulus from puffs: shadowed base, lit body, soft highlights toward the light (lx = -1..1).
void cumulus(Vector2 c, float w, float h, int seed, Color shadow, Color body, Color hi, float lx) {
  constexpr int kMax = 24;
  Vector2 p[kMax];
  float r[kMax];
  const int n = std::min(kMax, 5 + int(w / 45.0f));
  for (int i = 0; i < n; ++i) {
    float u = (i + 0.5f) / n;
    float bump = std::sin(u * kPi);
    r[i] = h * (0.26f + 0.44f * bump) * (0.8f + 0.4f * hash(seed + i * 7));
    p[i] = {c.x + (u - 0.5f) * (w - r[i]), c.y - bump * h * 0.3f + (hash(seed + i * 3) - 0.5f) * h * 0.2f};
  }
  softEllipse({c.x, c.y + h * 0.12f}, w * 0.5f, h * 0.22f, shadow, 4.0f);
  for (int i = 0; i < n; ++i) softEllipse({p[i].x - lx * r[i] * 0.06f, p[i].y + r[i] * 0.14f}, r[i], r[i], shadow, 3.0f);
  for (int i = 0; i < n; ++i) softEllipse({p[i].x + lx * r[i] * 0.16f, p[i].y - r[i] * 0.1f}, r[i] * 0.84f, r[i] * 0.84f, body, 4.0f);
  for (int i = 0; i < n; ++i)
    softEllipse({p[i].x + lx * r[i] * 0.34f, p[i].y - r[i] * 0.36f}, r[i] * 0.42f, r[i] * 0.42f, hi, r[i] * 0.2f);
}

// Long evening cloud lit from below: ragged dark body over a thin glowing rim, soft haze around.
void stratus(Vector2 c, float w, float h, int seed, Color body, Color rim) {
  const int n = 6 + int(w / 28.0f);
  glow(c, w * 0.62f, h * 1.1f, withAlpha(body, 0.3f), withAlpha(body, 0.0f));
  for (int pass = 0; pass < 2; ++pass)
    for (int i = 0; i < n; ++i) {
      float u = hash(seed + i * 13), k = std::sin(u * kPi);
      float rx = h * (0.5f + 1.2f * k) * (0.6f + 0.6f * hash(seed + i * 5));
      Vector2 p{c.x + (u - 0.5f) * w, c.y - k * h * 0.25f + (hash(seed + i) - 0.5f) * h * 0.5f};
      if (pass == 0) softEllipse({p.x, p.y + rx * 0.07f}, rx, rx * 0.3f, rim, 3.0f);
      else softEllipse({p.x - rx * 0.03f, p.y - rx * 0.03f}, rx * 0.97f, rx * 0.29f, body, 4.0f);
    }
}

// Ridge profile 0..1, tileable over `period` (integer harmonics), with sharp peaks.
float ridge(float x, float period, int seed) {
  float u = x / period * 2.0f * kPi, s = float(seed);
  return 0.5f * (1.0f - std::fabs(std::sin(u * 2.5f + s))) + 0.32f * (0.5f + 0.5f * std::sin(u * 3.0f + s * 1.7f)) +
         0.18f * (0.5f + 0.5f * std::sin(u * 11.0f + s * 2.3f));
}
// Mountain range with faceted light and a glowing rim on the faces turned to the light.
void range(float w, float base, float amp, int seed, Color top, Color low, float lowY, Color rim, bool lightRight) {
  const float step = 6.0f, fade = amp * 0.45f;
  for (float x = 0; x < w; x += step) {
    float y0 = base - amp * ridge(x, w, seed), y1 = base - amp * ridge(x + step, w, seed);
    gquad({x, y0}, {x + step, y1}, {x + step, lowY}, {x, lowY}, top, top, low, low);
    if (lightRight ? y1 > y0 : y1 < y0) {  // sunlit face: glow fading down from the crest
      gquad({x, y0}, {x + step, y1}, {x + step, y1 + fade}, {x, y0 + fade}, withAlpha(rim, 0.22f), withAlpha(rim, 0.22f),
            withAlpha(rim, 0.0f), withAlpha(rim, 0.0f));
      DrawLineEx({x, y0}, {x + step, y1}, 2.0f, withAlpha(rim, 0.55f));
    }
  }
}

bool g_preview = false;

// Software GL pays for every covered pixel, even transparent ones: read the layer back once
// and keep only the 32-px tiles that hold something (runs of tiles merged per row).
void findTiles(Layer& L) {
  Image img = LoadImageFromTexture(L.rt.texture);
  const auto* px = static_cast<const unsigned char*>(img.data);
  const float t = 32.0f;
  for (float ty = 0; ty < L.h; ty += t) {
    float run = -1;
    for (float tx = 0;; tx += t) {
      bool used = false;
      if (tx < L.w && px) {
        int x0 = int(tx * L.scale), x1 = std::min(img.width, int((tx + t) * L.scale) + 1);
        int y0 = std::max(0, img.height - int((ty + t) * L.scale) - 1), y1 = img.height - int(ty * L.scale);
        for (int y = y0; y < y1 && !used; ++y)
          for (int x = x0; x < x1; ++x)
            if (px[(y * img.width + x) * 4 + 3] > 1) {
              used = true;
              break;
            }
      }
      if (used && run < 0) run = tx;
      if (!used && run >= 0) {
        L.tiles.push_back({run, ty, std::min(tx, L.w) - run, std::min(t, L.h - ty)});
        run = -1;
      }
      if (tx >= L.w) break;
    }
  }
  UnloadImage(img);
}

BakeState bakeBegin(Layer& L, float w, float h, float scale) {
  BakeState s;
  L.w = w;
  L.h = h;
  L.scale = scale;
  rlDrawRenderBatchActive();
  s.fbo = rlGetActiveFramebuffer();
  s.fw = rlGetFramebufferWidth();
  s.fh = rlGetFramebufferHeight();
  s.view = rlGetMatrixModelview();
  L.rt = LoadRenderTexture(int(w * scale), int(h * scale));
  SetTextureFilter(L.rt.texture, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(L.rt.texture, TEXTURE_WRAP_CLAMP);
  BeginTextureMode(L.rt);
  ClearBackground(BLANK);
  rlScalef(scale, scale, 1.0f);
  ui::frameBlend();
  return s;
}

void bakeEnd(Layer& L, const BakeState& s, bool sparse) {
  if (s.fbo != 0) {
    RenderTexture2D outer{};
    outer.id = s.fbo;
    outer.texture.width = s.fw;
    outer.texture.height = s.fh;
    BeginTextureMode(outer);
  } else {
    EndTextureMode();
  }
  rlSetMatrixModelview(s.view);
  if (sparse) findTiles(L);
}

// Draws an opaque backdrop at (x, y), cut at `maxY` where something opaque covers the rest.
void drawBackdrop(const Layer& L, float x, float y, float maxY) {
  const float h = std::min(L.h, maxY - y), th = float(L.rt.texture.height);
  if (L.rt.id == 0 || h <= 0.0f) return;
  rlDrawRenderBatchActive();
  SetTextureFilter(L.rt.texture, g_preview ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
  rlDisableColorBlend();
  DrawTexturePro(L.rt.texture, {0, th - h * L.scale, float(L.rt.texture.width), -h * L.scale}, {x, y, L.w, h}, {0, 0}, 0.0f,
                 WHITE);
  rlDrawRenderBatchActive();
  rlEnableColorBlend();
}

// Draws a sparse layer at (x, y); a wrapping layer repeats horizontally across the screen.
void drawLayer(const Layer& L, float x, float y, bool wrap, float alpha) {
  if (L.rt.id == 0) return;
  const unsigned char a = (unsigned char)(clampf(alpha, 0.0f, 1.0f) * 255.0f);
  const float th = float(L.rt.texture.height);
  BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);  // (flushes: safe to change the filter)
  const bool crisp = !g_preview && std::fabs(L.scale - g_renderScale) < 0.01f;
  SetTextureFilter(L.rt.texture, crisp ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
  if (wrap) x = wrapf(x, L.w) - L.w;
  for (; x < kScreenW; x += L.w) {
    for (const Rectangle& t : L.tiles) {
      Rectangle d{x + t.x, y + t.y, t.width, t.height};
      if (d.x > kScreenW || d.x + d.width < 0 || d.y > kScreenH || d.y + d.height < 0) continue;
      Rectangle s{t.x * L.scale, th - (t.y + t.height) * L.scale, t.width * L.scale, -t.height * L.scale};
      DrawTexturePro(L.rt.texture, s, d, {0, 0}, 0.0f, {a, a, a, a});
    }
    if (!wrap) break;
  }
  ui::frameBlend();
}
// Screen x of a point of a wrapping layer drawn at x0 (first repeat after -margin).
float wrapPos(float x0, float bx, float w, float margin) { return wrapf(x0 + bx + margin, w) - margin; }

namespace {
Layer g_dot;
}  // namespace

void ensureParticleSprite() {
  if (g_dot.rt.id == 0)  // soft halo around a bright core
    g_dot = bake(32, 32, 2.0f, [] {
      glow({16, 16}, 16, 16, withAlpha(WHITE, 0.3f), withAlpha(WHITE, 0.0f), 32);
      glow({16, 16}, 4.6f, 4.6f, WHITE, withAlpha(WHITE, 0.85f), 16);
    }, false);
}

void motes(int count, int seed, Vec2 cam, float time, float vx, float vy, Color c0, Color c1, float size) {
  const Texture2D& tex = g_dot.rt.texture;
  if (g_dot.rt.id == 0) return;
  BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
  for (int i = 0; i < count; ++i) {
    float depth = 0.15f + 0.5f * hash(seed + i * 3);
    float x = wrapf(hash(seed + i) * 1500.0f + time * vx * (0.5f + depth) - cam.x * depth, 1500.0f) - 110.0f;
    float y = wrapf(hash(seed + i + 77) * 900.0f + time * vy * (0.5f + depth) - cam.y * depth, 900.0f) - 90.0f;
    x += std::sin(time * 0.7f + i) * 14.0f;
    float k = 0.55f + 0.45f * std::sin(time * (1.0f + hash(seed + i * 5) * 2.0f) + i);
    Color c = mixColor(c0, c1, hash(seed + i * 9));
    float r = size * (0.6f + depth) * 3.5f;
    Color t{(unsigned char)(c.r * k), (unsigned char)(c.g * k), (unsigned char)(c.b * k), (unsigned char)(255 * k)};
    DrawTexturePro(tex, {0, 0, float(tex.width), float(tex.height)}, {x - r, y - r, r * 2, r * 2}, {0, 0}, 0.0f, t);
  }
  ui::frameBlend();
}
void leaves(int count, int seed, Vec2 cam, float time, Color c0, Color c1) {
  for (int i = 0; i < count; ++i) {
    float depth = 0.25f + 0.5f * hash(seed + i * 3);
    float x = wrapf(hash(seed + i) * 1500.0f - time * 30.0f * (0.6f + depth) - cam.x * depth, 1500.0f) - 110.0f;
    float y = wrapf(hash(seed + i + 31) * 900.0f + time * 22.0f * (0.6f + depth) - cam.y * depth, 900.0f) - 90.0f;
    x += std::sin(time * 1.3f + i * 2) * 20.0f;
    float rot = time * (1.5f + hash(i) * 2.0f) + i;
    float s = 4.0f + 4.0f * depth;
    Vec2 d = rotate({s, 0}, rot), n = rotate({0, s * 0.4f * std::cos(time * 3.0f + i)}, rot);
    quad({x - d.x, y - d.y}, {x + n.x, y + n.y}, {x + d.x, y + d.y}, {x - n.x, y - n.y}, mixColor(c0, c1, hash(i * 7)));
  }
}
}  // namespace paint
