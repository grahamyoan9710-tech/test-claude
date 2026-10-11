#pragma once
// Small math toolkit shared by the simulation and the renderer.
// World convention: x grows to the right, y grows DOWN (like the screen).
#include <algorithm>
#include <cmath>
#include <cstdint>

constexpr float kPi = 3.14159265358979f;

inline float deg2rad(float d) { return d * kPi / 180.0f; }
inline float rad2deg(float r) { return r * 180.0f / kPi; }
inline float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float signf(float v) { return v > 0.0f ? 1.0f : (v < 0.0f ? -1.0f : 0.0f); }
inline float approach(float v, float target, float step) {
  if (v < target) return std::min(v + step, target);
  return std::max(v - step, target);
}
inline float smoothstep(float t) {
  t = clampf(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

struct Vec2 {
  float x = 0.0f, y = 0.0f;
  constexpr Vec2() = default;
  constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}
  Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
  Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
  Vec2 operator*(float s) const { return {x * s, y * s}; }
  Vec2 operator/(float s) const { return {x / s, y / s}; }
  Vec2 operator-() const { return {-x, -y}; }
  Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
  Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
  Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
  float len() const { return std::sqrt(x * x + y * y); }
  float len2() const { return x * x + y * y; }
  Vec2 norm() const {
    float l = len();
    return l > 1e-6f ? Vec2{x / l, y / l} : Vec2{0.0f, 0.0f};
  }
};

inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline Vec2 lerpv(Vec2 a, Vec2 b, float t) { return a + (b - a) * t; }
inline Vec2 rotate(Vec2 v, float rad) {
  float c = std::cos(rad), s = std::sin(rad);
  return {v.x * c - v.y * s, v.x * s + v.y * c};
}

struct Rect {
  float x = 0, y = 0, w = 0, h = 0;
  float left() const { return x; }
  float right() const { return x + w; }
  float top() const { return y; }
  float bottom() const { return y + h; }
  Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
  bool overlaps(const Rect& o) const {
    return x < o.x + o.w && o.x < x + w && y < o.y + o.h && o.y < y + h;
  }
  bool contains(Vec2 p) const { return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h; }
};

inline bool circleRect(Vec2 c, float r, const Rect& rc) {
  float nx = clampf(c.x, rc.left(), rc.right());
  float ny = clampf(c.y, rc.top(), rc.bottom());
  float dx = c.x - nx, dy = c.y - ny;
  return dx * dx + dy * dy <= r * r;
}

struct Color3 {
  uint8_t r = 255, g = 255, b = 255;
};

// Tiny deterministic RNG (xorshift32) so matches are reproducible.
struct Rng {
  uint32_t s = 0x9E3779B9u;
  explicit Rng(uint32_t seed = 0x9E3779B9u) : s(seed ? seed : 0x9E3779B9u) {}
  uint32_t next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
  }
  float uniform() { return (next() >> 8) * (1.0f / 16777216.0f); }
  float range(float a, float b) { return a + (b - a) * uniform(); }
  int irange(int a, int b) { return a + int(next() % uint32_t(b - a + 1)); }
  bool chance(float p) { return uniform() < p; }
};
