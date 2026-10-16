#pragma once
// Painting kit of the stage renderers (render_world.cpp, render_stages.cpp): smooth gradients made of a few
// big triangles, painterly clouds and mountain ranges, and scenery baked once into textures (premultiplied
// alpha) then scrolled with parallax. Software GL pays for every covered pixel and every tiny triangle:
// static scenery belongs in a baked layer, only light things (particles, pulses, water) are drawn per frame.
#include <initializer_list>

#include "game/common.hpp"

namespace paint {
float hash(int i);  // deterministic noise in [0, 1]
float wrapf(float v, float m);

void tri(Vector2 a, Vector2 b, Vector2 c, Color col);
void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color col);
// One colour per corner: smooth gradients for free.
void vtx(Vector2 p, Color c);
void gtri(Vector2 a, Vector2 b, Vector2 c, Color ca, Color cb, Color cc);
void gquad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color ca, Color cb, Color cc, Color cd);
void vgrad(float x, float y, float w, float h, Color top, Color bottom);
// Radial (elliptic) gradient; with in == out it is a plain ellipse (seg = 0: picked from the size).
void glow(Vector2 c, float rx, float ry, Color in, Color out, int seg = 0);
void glow(Vector2 c, float r, Color col, float a, int seg = 0);
void ellipse(Vector2 c, float rx, float ry, Color col);
void dot(Vector2 c, float r, Color col);
// Ellipse with a feathered rim of `fpx` pixels: soft, painterly edges that never alias.
void softEllipse(Vector2 c, float rx, float ry, Color col, float fpx, int seg = 28);
void star(Vector2 c, float rOut, float rIn, float rot, Color col);
// Thick polyline with round joints, its width going from w0 to w1.
void stroke(const Vector2* p, int n, float w0, float w1, Color col);

struct Stop {
  float t;
  Color c;
};
// Vertical gradient over a w x h band through the stops (t = 0..1 from the top).
void gradient(float w, float h, std::initializer_list<Stop> stops);

// Repeats an element across the seam of a band that tiles every `period` pixels.
template <class F>
void wrapX(float x, float halfW, float period, F fn) {
  fn(x);
  if (x - halfW < 0.0f) fn(x + period);
  if (x + halfW > period) fn(x - period);
}

// Cumulus from puffs: shadowed base, lit body, soft highlights toward the light (lx = -1..1).
void cumulus(Vector2 c, float w, float h, int seed, Color shadow, Color body, Color hi, float lx);
// Long evening cloud lit from below: ragged dark body over a thin glowing rim, soft haze around.
void stratus(Vector2 c, float w, float h, int seed, Color body, Color rim);
// Ridge profile 0..1, tileable over `period` (integer harmonics), with sharp peaks.
float ridge(float x, float period, int seed);
// Mountain range with faceted light and a glowing rim on the faces turned to the light.
void range(float w, float base, float amp, int seed, Color top, Color low, float lowY, Color rim, bool lightRight);

// ---- Baked layers -------------------------------------------------------------------------
extern bool g_preview;  // drawing a scaled-down stage preview (smooth sampling)

struct Layer {
  RenderTexture2D rt{};
  float w = 0, h = 0, scale = 1;
  std::vector<Rectangle> tiles;  // parts with content (virtual units): the rest is never drawn
};

// Renders into a new transparent texture (premultiplied alpha), even in the middle of a frame: the
// frame's own target, viewport and transform are put back by bakeEnd.
struct BakeState {
  unsigned int fbo = 0;
  int fw = 0, fh = 0;
  Matrix view{};
};
BakeState bakeBegin(Layer& L, float w, float h, float scale);
void bakeEnd(Layer& L, const BakeState& s, bool sparse);
template <class F>
Layer bake(float w, float h, float scale, F draw, bool sparse = true) {
  Layer L;
  const BakeState s = bakeBegin(L, w, h, scale);
  draw();
  bakeEnd(L, s, sparse);
  return L;
}
// A fully opaque backdrop, drawn without blending and point-sampled in matches (as cheap as a flat fill
// on software GL; the scenery in it is soft and hazy anyway).
template <class F>
Layer bakeOpaque(float w, float h, float scale, F draw) {
  return bake(w, h, scale, draw, false);
}

// Draws an opaque backdrop at (x, y), cut at `maxY` where something opaque covers the rest.
void drawBackdrop(const Layer& L, float x, float y, float maxY = kScreenH);
// Draws a sparse layer at (x, y); a wrapping layer repeats horizontally across the screen.
void drawLayer(const Layer& L, float x, float y, bool wrap, float alpha = 1.0f);
// Screen x of a point of a wrapping layer drawn at x0 (first repeat after -margin).
float wrapPos(float x0, float bx, float w, float margin);

// The baked art of one stage theme.
struct Art {
  bool ready = false;
  Layer sky, far, mid, near, extra;
};

// ---- Particles (screen space, parallax with the camera) -----------------------------------------
void ensureParticleSprite();
// Gentle motes and spores (vy < 0 rises) with a soft halo.
void motes(int count, int seed, Vec2 cam, float time, float vx, float vy, Color c0, Color c1, float size);
// Tumbling leaves drifting down and to the left.
void leaves(int count, int seed, Vec2 cam, float time, Color c0, Color c1);
}  // namespace paint
