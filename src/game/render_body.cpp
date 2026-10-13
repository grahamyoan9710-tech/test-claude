// Procedural cartoon bodies: a small 2-bone IK rig drawn with thick outlined strokes.
// Look: painterly cel shading (warm key light from the upper left, cool teal-tinted shadow with a
// soft terminator, thin rim light), warm umber outlines, earthy palettes.
// Each hero has its own build (proportions), outfit, hair and face, drawn layer by layer:
// back cloth -> back hair -> back leg -> back arm -> torso -> front leg -> skirt -> head -> hair ->
// weapon -> front arm -> energy.
#include "game/fighter_art.hpp"
#include "game/pixel.hpp"
#include "game/render.hpp"

namespace {
const Color kOutline = {56, 35, 28, 255};  // warm dark umber
const Color kShadowTint = {38, 100, 116, 255};
const Color kRimTint = {200, 244, 236, 255};
const Color kWarmLight = {255, 236, 200, 255};
const Color kGold = {226, 180, 76, 255};
const Color kInkFace = {44, 28, 26, 255};
const Color kUltraGlow = {110, 255, 150, 255};  // TotK green (Ultrahand, Zonai runes)
const Vec2 kLight{-0.55f, -0.835f};  // towards the key light (screen space)
constexpr int kMaxPoly = 12;

// Every shape is drawn in layers, pass by pass for a whole group of shapes, so that the
// limbs of one group read as a single shaded volume (no seams at the joints).
enum Pass { kLine, kRim, kShadow, kLit };

Color tone(Color c, Color tint, float dark, float mix) {
  Color k = mixColor(shade(c, dark), tint, mix);
  k.a = c.a;
  return k;
}
Color shadowTone(Color c) { return tone(c, kShadowTint, 0.66f, 0.22f); }
Color midTone(Color c) { return tone(c, kShadowTint, 0.86f, 0.07f); }
Color litTone(Color c) { return tone(c, kWarmLight, 1.0f, 0.12f); }
Color rimTone(Color c) { return tone(c, kRimTint, 1.0f, 0.3f); }
Color backTone(Color c) { return tone(c, kShadowTint, 0.8f, 0.08f); }
// Earthy grade of the roster palettes: a little less saturated, a little warmer.
Color earthy(Color3 c3) {
  Color c = C(c3);
  float l = 0.3f * c.r + 0.59f * c.g + 0.11f * c.b;
  auto ch = [&](float v, float warm) { return (unsigned char)clampf(lerpf(v, l, 0.16f) * warm, 0.0f, 255.0f); };
  return {ch(c.r, 1.0f), ch(c.g, 0.975f), ch(c.b, 0.92f), 255};
}

// Discs and ellipses tessellated after their size: software GL pays for every triangle.
int segmentsFor(float r) { return std::clamp(int(r * 1.5f) + 6, 8, 36); }
void disc(Vector2 c, float r, Color col) { DrawCircleSector(c, r, 0, 360, segmentsFor(r), col); }
void fillEllipse(Vector2 c, float rx, float ry, Color col) {
  const int n = segmentsFor(std::max(rx, ry));
  Vector2 prev{c.x + rx, c.y};
  for (int i = 1; i <= n; ++i) {
    float a = i * 2.0f * kPi / n;
    Vector2 p{c.x + std::cos(a) * rx, c.y + std::sin(a) * ry};
    DrawTriangle(c, p, prev, col);
    prev = p;
  }
}
void capsule(Vector2 a, Vector2 b, float th, Color k) {
  DrawLineEx(a, b, th, k);
  disc(a, th * 0.5f, k);
  disc(b, th * 0.5f, k);
}
void triangle(Vector2 a, Vector2 b, Vector2 c, Color k) {
  float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  if (cross > 0) std::swap(b, c);
  DrawTriangle(a, b, c, k);
}
void fanFill(const Vector2* q, int n, Color k) {
  for (int i = 1; i + 1 < n; ++i) triangle(q[0], q[i], q[i + 1], k);
}
// Outward mitred offset of a convex polygon (sharp tips are clamped).
void growPoly(const Vector2* q, int n, float d, Vector2* out) {
  float area = 0.0f;
  for (int i = 0; i < n; ++i) area += q[i].x * q[(i + 1) % n].y - q[(i + 1) % n].x * q[i].y;
  const float sg = area > 0.0f ? 1.0f : -1.0f;
  for (int i = 0; i < n; ++i) {
    Vec2 e0 = (W(q[i]) - W(q[(i + n - 1) % n])).norm(), e1 = (W(q[(i + 1) % n]) - W(q[i])).norm();
    Vec2 n0 = Vec2{e0.y, -e0.x} * sg, n1 = Vec2{e1.y, -e1.x} * sg;
    if (n0.len2() < 0.5f) n0 = n1;
    if (n1.len2() < 0.5f) n1 = n0;
    Vec2 m = (n0 + n1).norm();
    if (m.len2() < 0.5f) m = n0;
    float k = std::max(0.5f, dot(m, n0));
    out[i] = V(W(q[i]) + m * (d / k));
  }
}

struct Painter {
  int pass = kLine;
  float ow = 3.0f;
  float alpha = 1.0f;
  Color flashCol = WHITE;
  float flash = 0.0f;
  bool flat = false;   // translucent draw: one flat layer
  bool ghost = false;  // afterimage: flat fill only, no outline, no details
  float glow = 0.6f;   // rune glow
  bool pixel = false;  // drawn as a pixel-art sprite: one-pixel ink lines

  Color fill(Color c) const { return withAlpha(mixColor(c, flashCol, flash), alpha * c.a / 255.0f); }
  Color line() const { return withAlpha(pixel ? pix::kInk : kOutline, alpha); }
  bool shaded() const { return !flat && !ghost; }
  bool details() const { return pass == kLit && !ghost; }

  void seg(Vector2 a, Vector2 b, float th, Color c) const {
    if (pass == kLine) {
      if (!ghost) capsule(a, b, th + ow * 2, line());
      return;
    }
    if (!shaded()) {
      if (pass == kLit) capsule(a, b, th, fill(c));
      return;
    }
    // Light offsets go across the stroke (never out of it), towards the light.
    Vec2 d = (W(b) - W(a)).norm();
    Vec2 n = d.len2() > 0.5f ? Vec2{-d.y, d.x} : kLight;
    Vec2 o = n * dot(n, kLight);
    o = o * (1.0f / std::max(o.len(), 0.5f));
    auto at = [&](Vector2 p, float k) { return Vector2{p.x + o.x * th * k, p.y + o.y * th * k}; };
    const bool rim = th > 12.0f, soft = th > 10.0f;  // thin strokes: plain two tones
    if (pass == kRim) {
      if (rim) capsule(a, b, th, fill(rimTone(c)));
    } else if (pass == kShadow) {
      if (rim) capsule(at(a, 0.06f), at(b, 0.06f), th * 0.86f, fill(shadowTone(c)));
      else capsule(a, b, th, fill(shadowTone(c)));
    } else {
      if (soft) capsule(at(a, 0.12f), at(b, 0.12f), th * 0.72f, fill(midTone(c)));
      capsule(at(a, 0.19f), at(b, 0.19f), th * 0.6f, fill(litTone(c)));
    }
  }
  void ellipse(Vector2 c, float rx, float ry, Color k) const {
    if (pass == kLine) {
      if (!ghost) fillEllipse(c, rx + ow, ry + ow, line());
      return;
    }
    if (!shaded()) {
      if (pass == kLit) fillEllipse(c, rx, ry, fill(k));
      return;
    }
    auto at = [&](float s) { return Vector2{c.x + kLight.x * rx * s, c.y + kLight.y * ry * s}; };
    const bool rim = std::min(rx, ry) > 9.0f, soft = std::max(rx, ry) > 8.0f;
    if (pass == kRim) {
      if (rim) fillEllipse(c, rx, ry, fill(rimTone(k)));
    } else if (pass == kShadow) {
      if (rim) fillEllipse(at(0.07f), rx * 0.93f, ry * 0.93f, fill(shadowTone(k)));
      else fillEllipse(c, rx, ry, fill(shadowTone(k)));
    } else {
      if (soft) fillEllipse(at(0.17f), rx * 0.83f, ry * 0.83f, fill(midTone(k)));
      fillEllipse(at(0.3f), rx * 0.7f, ry * 0.7f, fill(litTone(k)));
    }
  }
  void circle(Vector2 c, float r, Color k) const {
    if (pass == kLine) {
      if (!ghost) disc(c, r + ow, line());
      return;
    }
    if (!shaded()) {
      if (pass == kLit) disc(c, r, fill(k));
      return;
    }
    auto at = [&](float s) { return Vector2{c.x + kLight.x * r * s, c.y + kLight.y * r * s}; };
    const bool rim = r > 9.0f, soft = r > 7.0f;
    if (pass == kRim) {
      if (rim) disc(c, r, fill(rimTone(k)));
    } else if (pass == kShadow) {
      disc(rim ? at(0.07f) : c, rim ? r * 0.93f : r, fill(shadowTone(k)));
    } else {
      if (soft) disc(at(0.17f), r * 0.83f, fill(midTone(k)));
      disc(at(0.3f), r * 0.7f, fill(litTone(k)));
    }
  }
  // Convex polygon: mitred outline; shadow, then a mid inset towards the light (and a lit core for `lit`).
  void poly(const Vector2* q, int n, Color k, bool lit = false) const {
    if (n < 3) return;
    if (pass == kLine) {
      if (ghost) return;
      Vector2 g[kMaxPoly];
      growPoly(q, n, ow, g);
      fanFill(g, n, line());
      return;
    }
    if (!shaded()) {
      if (pass == kLit) fanFill(q, n, fill(k));
      return;
    }
    if (pass == kShadow) {
      fanFill(q, n, fill(shadowTone(k)));
    } else if (pass == kLit) {
      Vec2 m{};
      for (int i = 0; i < n; ++i) m += W(q[i]);
      m = m / float(n);
      float ext = 0.0f;
      for (int i = 0; i < n; ++i) ext = std::max(ext, (W(q[i]) - m).len());
      Vector2 in[kMaxPoly];
      auto inset = [&](float k2, float shift) {
        for (int i = 0; i < n; ++i) in[i] = V(m + (W(q[i]) - m) * k2 + kLight * (ext * shift));
      };
      inset(0.9f, 0.05f);
      fanFill(in, n, fill(midTone(k)));
      if (lit) {
        inset(0.62f, 0.16f);
        fanFill(in, n, fill(litTone(k)));
      }
    }
  }
  void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color k, bool lit = false) const {
    Vector2 q[4] = {a, b, c, d};
    poly(q, 4, k, lit);
  }
  // Single triangle (spikes, ears, hair...): mid tone with a lit inset towards the light.
  void tri(Vector2 a, Vector2 b, Vector2 c, Color k, bool shadedTri = true) const {
    if (pass == kLine) {
      if (ghost) return;
      Vector2 q[3] = {a, b, c}, g[3];
      growPoly(q, 3, ow, g);
      triangle(g[0], g[1], g[2], line());
      return;
    }
    if (!shaded() || !shadedTri) {
      if (pass == kLit) triangle(a, b, c, fill(shadedTri ? k : midTone(k)));
      return;
    }
    if (pass == kShadow) {
      triangle(a, b, c, fill(shadowTone(k)));
    } else if (pass == kLit) {
      Vector2 m{(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3};
      float per = (W(b) - W(a)).len() + (W(c) - W(b)).len() + (W(a) - W(c)).len();
      float area = std::fabs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) * 0.5f;
      float rin = per > 0.01f ? 2.0f * area / per : 0.0f;
      Vec2 o = kLight * (rin * 0.16f);
      auto in = [&](Vector2 p) { return V(W(m) + (W(p) - W(m)) * 0.76f + o); };
      triangle(a, b, c, fill(midTone(k)));
      triangle(in(a), in(b), in(c), fill(litTone(k)));
    }
  }
  // Tapered limb from a (width wa) to b (width wb), wm at fraction u of the way, the bulge leaning to the +n side
  // of a->b by `bias` (-1..1): thighs, calves, biceps, flared trousers. Round joints; shaded like seg() (a cylinder
  // lit by the key light), so a body reads as volumes, not sticks.
  void taper(Vector2 a, Vector2 b, float wa, float wm, float wb, float u, float bias, Color c, bool capB = true) const {
    const Vec2 A = W(a), E = W(b);
    Vec2 d = E - A;
    const float len = d.len();
    if (len < 0.5f) {
      circle(a, std::max(wa, wb) * 0.5f, c);
      return;
    }
    d = d * (1.0f / len);
    const Vec2 n{-d.y, d.x};
    const float lin = lerpf(wa, wb, u) * 0.5f;
    const float hp = std::max(lin, wm * 0.5f * (1.0f + 0.45f * bias)), hn = std::max(lin, wm * 0.5f * (1.0f - 0.45f * bias));
    const Vec2 M = A + d * (len * u);
    auto shape = [&](float k, float grow, Vec2 off, Color col) {
      Vector2 q[6] = {V(A + off + n * (wa * 0.5f * k + grow)), V(M + off + n * (hp * k + grow)),
                      V(E + off + n * (wb * 0.5f * k + grow)), V(E + off - n * (wb * 0.5f * k + grow)),
                      V(M + off - n * (hn * k + grow)), V(A + off - n * (wa * 0.5f * k + grow))};
      fanFill(q, 6, col);
      disc(V(A + off), wa * 0.5f * k + grow, col);
      if (capB) disc(V(E + off), wb * 0.5f * k + grow, col);
    };
    if (pass == kLine) {
      if (!ghost) shape(1.0f, ow, {}, line());
      return;
    }
    if (!shaded()) {
      if (pass == kLit) shape(1.0f, 0.0f, {}, fill(c));
      return;
    }
    Vec2 o = n * dot(n, kLight);
    o = o * (1.0f / std::max(o.len(), 0.5f));
    const float th = std::max(wa, std::max(wm, wb));
    const bool rim = th > 12.0f, soft = th > 10.0f;
    if (pass == kRim) {
      if (rim) shape(1.0f, 0.0f, {}, fill(rimTone(c)));
    } else if (pass == kShadow) {
      if (rim) shape(0.86f, 0.0f, o * (th * 0.06f), fill(shadowTone(c)));
      else shape(1.0f, 0.0f, {}, fill(shadowTone(c)));
    } else {
      if (soft) shape(0.72f, 0.0f, o * (th * 0.12f), fill(midTone(c)));
      shape(0.58f, 0.0f, o * (th * 0.2f), fill(litTone(c)));
    }
  }
  // Decoration over an already drawn group: outline with the group, all fill layers at once.
  template <class F>
  void deco(F fn) {
    if (pass == kLine) {
      fn();
    } else if (details()) {
      for (int p : {kRim, kShadow, kLit}) {
        pass = p;
        fn();
      }
    }
  }
  void glowLine(Vector2 a, Vector2 b, float w, float k = 1.0f, Color c = kZonaiGlow) const {
    if (!details() || flat) return;
    float g = clampf(glow * k, 0.0f, 2.0f);
    DrawLineEx(a, b, w * 3.4f, withAlpha(fill(c), alpha * 0.22f * g));
    DrawLineEx(a, b, w, withAlpha(fill(mixColor(c, WHITE, 0.25f + 0.2f * g)), alpha * std::min(1.0f, 0.45f + 0.5f * g)));
  }
  void glowDot(Vector2 c, float r, float k = 1.0f, Color col = kZonaiGlow) const {
    if (!details() || flat) return;
    float g = clampf(glow * k, 0.0f, 2.0f);
    disc(c, r * 2.6f, withAlpha(fill(col), alpha * 0.2f * g));
    disc(c, r, withAlpha(fill(mixColor(col, WHITE, 0.35f)), alpha * std::min(1.0f, 0.5f + 0.5f * g)));
  }
};

Xf xform(const CharacterDef& d, const Pose& pose, Vec2 feet, int facing, float scale) {
  Xf X;
  X.facing = facing;
  X.scale = scale;
  // An illustration is never squashed nor stretched: it moves with its skeleton only.
  const float k = art::rigged(d.hero) ? 0.0f : 1.0f;
  X.squash = 1.0f + (pose.squash - 1.0f) * k;
  X.sx = 1.0f + (pose.sx - 1.0f) * k;
  X.rot = pose.rot * facing;
  X.center = feet + Vec2{pose.shift.x * facing * scale, -d.height * 0.5f * scale * X.squash + pose.shift.y * scale};
  return X;
}

Vec2 ik(Vec2 root, Vec2 target, float l1, float l2, float bend, Vec2& end) {
  Vec2 d = target - root;
  float dist = clampf(d.len(), 0.001f, l1 + l2 - 0.01f);
  float c = clampf((l1 * l1 + dist * dist - l2 * l2) / (2.0f * l1 * dist), -1.0f, 1.0f);
  float a = std::acos(c);
  float base = std::atan2(d.y, d.x);
  end = root + d.norm() * dist;
  return root + Vec2{std::cos(base + bend * a), std::sin(base + bend * a)} * l1;
}

Vec2 polar(float deg, float r) { return {std::cos(deg2rad(deg)) * r, std::sin(deg2rad(deg)) * r}; }

// Secondary motion: a chain hanging from `root`. Its direction mixes the rest direction, gravity
// (world down, whatever the body rotation) and the drag of the fighter's motion (pose.flow, the
// tip lagging with pose.flowTip); a travelling wave makes cloth flutter.
void chain(Vec2 root, Vec2 rest, float len, int n, const Pose& pose, float time, float gravity, float drag,
           float flutter, Vec2* out) {
  const Vec2 down{std::sin(pose.rot), std::cos(pose.rot)};
  rest = rest.norm();
  out[0] = root;
  for (int i = 1; i <= n; ++i) {
    float k = float(i) / n;
    Vec2 dir = rest + down * gravity + lerpv(pose.flow, pose.flowTip, k) * (drag * (0.5f + 0.7f * k));
    dir = rotate(dir.norm(), std::sin(time * 7.0f - i * 1.4f) * flutter * (0.4f + k));
    out[i] = out[i - 1] + dir * (len / n);
  }
}

// ---- Builds: proportions of each hero (fractions of the height) ---------------------------------
struct Build {
  float head, torso, hipsY, shX, hipX, ua, fa, thigh, shin, chest, waist, arm, fore, leg, shinTh, hand, foot;
};
Build buildFor(Hero h) {
  switch (h) {
    //                    head   torso  hipsY  shX    hipX   ua     fa     thigh  shin   chest  waist  arm    fore   leg    shinT  hand   foot
    case Hero::Asta:      return {0.158f, 0.25f, 0.12f, 0.045f, 0.04f, 0.15f, 0.15f, 0.195f, 0.195f, 0.25f, 0.20f, 0.085f, 0.078f, 0.088f, 0.08f, 0.058f, 0.062f};
    case Hero::Yuno:      return {0.138f, 0.27f, 0.10f, 0.035f, 0.033f, 0.158f, 0.158f, 0.205f, 0.205f, 0.2f, 0.16f, 0.07f, 0.064f, 0.082f, 0.074f, 0.045f, 0.052f};
    case Hero::Ichigo:    return {0.132f, 0.27f, 0.10f, 0.04f, 0.036f, 0.158f, 0.158f, 0.205f, 0.205f, 0.22f, 0.17f, 0.08f, 0.072f, 0.105f, 0.1f, 0.046f, 0.05f};
    case Hero::Byakuya:   return {0.126f, 0.275f, 0.09f, 0.038f, 0.034f, 0.16f, 0.16f, 0.21f, 0.21f, 0.2f, 0.155f, 0.074f, 0.068f, 0.1f, 0.096f, 0.043f, 0.048f};
    case Hero::Link:      return {0.15f, 0.26f, 0.11f, 0.038f, 0.035f, 0.152f, 0.152f, 0.2f, 0.2f, 0.21f, 0.165f, 0.074f, 0.068f, 0.086f, 0.078f, 0.048f, 0.056f};
    case Hero::Ganondorf: return {0.112f, 0.30f, 0.09f, 0.06f, 0.045f, 0.16f, 0.16f, 0.21f, 0.21f, 0.33f, 0.25f, 0.105f, 0.098f, 0.11f, 0.1f, 0.062f, 0.066f};
    case Hero::Kratos:    return {0.12f, 0.29f, 0.10f, 0.055f, 0.042f, 0.158f, 0.158f, 0.205f, 0.205f, 0.30f, 0.22f, 0.1f, 0.092f, 0.1f, 0.086f, 0.06f, 0.06f};
    case Hero::Gojo:      return {0.112f, 0.255f, 0.04f, 0.034f, 0.03f, 0.165f, 0.165f, 0.235f, 0.235f, 0.19f, 0.15f, 0.066f, 0.06f, 0.078f, 0.07f, 0.042f, 0.05f};
    case Hero::Sukuna:    return {0.135f, 0.27f, 0.10f, 0.04f, 0.036f, 0.158f, 0.158f, 0.205f, 0.205f, 0.22f, 0.17f, 0.082f, 0.074f, 0.09f, 0.082f, 0.048f, 0.054f};
    case Hero::Kenpachi:  return {0.108f, 0.30f, 0.09f, 0.058f, 0.045f, 0.16f, 0.16f, 0.21f, 0.21f, 0.31f, 0.24f, 0.1f, 0.092f, 0.11f, 0.1f, 0.058f, 0.062f};
    case Hero::Ulquiorra: return {0.128f, 0.27f, 0.09f, 0.036f, 0.032f, 0.16f, 0.16f, 0.21f, 0.21f, 0.19f, 0.155f, 0.068f, 0.062f, 0.086f, 0.08f, 0.042f, 0.048f};
    case Hero::Hunter:    return {0.13f, 0.27f, 0.11f, 0.05f, 0.04f, 0.155f, 0.155f, 0.2f, 0.2f, 0.26f, 0.2f, 0.09f, 0.085f, 0.095f, 0.09f, 0.056f, 0.064f};
    case Hero::Naruto:    return {0.16f, 0.25f, 0.12f, 0.04f, 0.036f, 0.15f, 0.15f, 0.195f, 0.195f, 0.21f, 0.17f, 0.076f, 0.07f, 0.088f, 0.08f, 0.05f, 0.056f};
    case Hero::Sasuke:    return {0.14f, 0.27f, 0.10f, 0.038f, 0.034f, 0.158f, 0.158f, 0.205f, 0.205f, 0.2f, 0.155f, 0.072f, 0.065f, 0.09f, 0.084f, 0.045f, 0.05f};
    case Hero::Kakashi:   return {0.13f, 0.27f, 0.10f, 0.04f, 0.035f, 0.16f, 0.16f, 0.21f, 0.21f, 0.22f, 0.17f, 0.075f, 0.068f, 0.088f, 0.08f, 0.047f, 0.052f};
    case Hero::Grimmjow:  return {0.13f, 0.27f, 0.10f, 0.044f, 0.038f, 0.158f, 0.158f, 0.205f, 0.205f, 0.23f, 0.17f, 0.082f, 0.074f, 0.1f, 0.094f, 0.05f, 0.054f};
    case Hero::Starrk:    return {0.12f, 0.275f, 0.08f, 0.04f, 0.035f, 0.16f, 0.16f, 0.215f, 0.215f, 0.21f, 0.165f, 0.074f, 0.068f, 0.098f, 0.092f, 0.046f, 0.05f};
    default:              return {0.14f, 0.27f, 0.10f, 0.035f, 0.035f, 0.155f, 0.155f, 0.205f, 0.205f, 0.2f, 0.17f, 0.07f, 0.065f, 0.075f, 0.07f, 0.05f, 0.055f};
  }
}

// ---- Cuts: how flesh and clothes fill out the skeleton (multiples of the build's thicknesses) -------
struct Cut {
  float thighA = 1.2f, thighM = 1.16f, thighB = 0.86f;               // hip, middle, knee
  float shinA = 0.92f, shinM = 1.08f, shinB = 0.64f, shinU = 0.32f;  // knee, calf, ankle
  float calf = 0.8f;                                                 // calf bulging at the back
  float upperA = 1.16f, upperM = 1.1f, upperB = 0.84f, bicep = 0.5f; // shoulder, middle, elbow
  float foreA = 0.94f, foreM = 1.08f, foreB = 0.72f;                 // elbow, forearm, wrist
  float chest = 1.0f, waist = 1.0f, hips = 1.0f;                     // torso
};
Cut cutFor(Hero h, Form form = Form::None) {
  Cut c;
  auto hakama = [&] {  // wide trousers flaring down to the ankles
    c.thighA = 1.26f; c.thighM = 1.24f; c.thighB = 1.08f;
    c.shinA = 1.12f; c.shinM = 1.22f; c.shinB = 1.46f; c.shinU = 0.78f; c.calf = 0.0f;
  };
  auto kimono = [&] {  // wide sleeves
    c.upperA = 1.2f; c.upperM = 1.18f; c.upperB = 1.1f; c.bicep = 0.0f;
    c.foreA = 1.1f; c.foreM = 1.16f; c.foreB = 1.18f;
  };
  auto muscle = [&] {  // bare, powerful arms
    c.upperA = 1.28f; c.upperM = 1.26f; c.upperB = 0.86f; c.bicep = 0.7f;
    c.foreA = 0.98f; c.foreM = 1.2f; c.foreB = 0.72f;
  };
  auto sleeve = [&] {  // jacket sleeves
    c.upperA = 1.16f; c.upperM = 1.12f; c.upperB = 1.0f; c.bicep = 0.2f;
    c.foreA = 1.02f; c.foreM = 1.06f; c.foreB = 0.86f;
  };
  switch (h) {
    case Hero::Asta: muscle(); c.chest = 1.04f; break;
    case Hero::Yuno: sleeve(); c.thighM = 1.2f; c.shinB = 0.78f; break;
    case Hero::Ichigo: hakama(); kimono(); break;
    case Hero::Byakuya: hakama(); kimono(); c.waist = 0.95f; break;
    case Hero::Link: sleeve(); c.shinB = 0.86f; break;
    case Hero::Ganondorf: muscle(); c.chest = 1.06f; c.shinB = 0.92f; break;
    case Hero::Kratos: muscle(); c.chest = 1.08f; c.waist = 0.96f; c.shinB = 0.84f; break;
    case Hero::Gojo: sleeve(); c.thighM = 1.2f; c.shinM = 1.12f; c.shinB = 0.86f; c.calf = 0.3f; break;
    case Hero::Sukuna: hakama(); kimono(); break;
    case Hero::Kenpachi: hakama(); muscle(); c.chest = 1.04f; break;
    case Hero::Ulquiorra: hakama(); sleeve(); c.shinB = 1.3f; break;
    case Hero::Hunter: muscle(); c.upperA = 1.34f; c.shinB = 0.95f; c.chest = 1.06f; break;
    case Hero::Naruto: sleeve(); c.thighM = 1.22f; c.shinB = 0.74f; break;
    case Hero::Sasuke: hakama(); sleeve(); c.shinB = 1.22f; break;
    case Hero::Kakashi: sleeve(); c.thighM = 1.2f; c.shinB = 0.74f; break;
    case Hero::Grimmjow:
      if (form == Form::Pantera) {  // the panther: lean armoured limbs
        muscle();
        c.shinB = 0.7f;
      } else {
        hakama();
        sleeve();
        c.chest = 1.04f;
      }
      break;
    case Hero::Starrk: hakama(); sleeve(); c.shinB = 1.3f; break;
    default: break;
  }
  return c;
}

}  // namespace

Rig buildRig(const CharacterDef& d, const Pose& pose) {
  Rig r;
  const float H = d.height;
  const Build b = buildFor(d.hero);
  r.hr = b.head * H;
  r.tilt = pose.lean * 0.55f;
  const float drop = pose.crouch * 0.14f * H;
  r.hips = {0.0f, b.hipsY * H + drop};
  r.neck = r.hips + rotate({0.0f, -b.torso * H}, pose.lean);
  r.chest = lerpv(r.hips, r.neck, 0.62f);
  r.head = r.neck + rotate({0.01f * H, -r.hr * 1.02f}, pose.lean * 1.15f);
  // Twist: the shoulders turn (one comes forward, the other goes back), the hips follow a little.
  const float tw = clampf(pose.twist, -1.2f, 1.2f);
  r.shF = r.neck + rotate({(b.shX - 0.045f * tw) * H, 0.045f * H}, pose.lean);
  r.shB = r.neck + rotate({(-b.shX + 0.065f * tw) * H, 0.045f * H}, pose.lean);
  r.hipF = r.hips + Vec2{(b.hipX - 0.012f * tw) * H, 0.0f};
  r.hipB = r.hips + Vec2{(-b.hipX + 0.018f * tw) * H, 0.0f};
  r.elbowF = ik(r.shF, pose.handF, b.ua * H, b.fa * H, 1.0f, r.handF);
  r.elbowB = ik(r.shB, pose.handB, b.ua * H, b.fa * H, 1.0f, r.handB);
  r.kneeF = ik(r.hipF, pose.footF, b.thigh * H, b.shin * H, -1.0f, r.footF);
  r.kneeB = ik(r.hipB, pose.footB, b.thigh * H, b.shin * H, -1.0f, r.footB);
  return r;
}

namespace {
Rig solveRig(const CharacterDef& d, const Pose& pose) {
  const Rig built = buildRig(d, pose);
  Rig r;
  if (art::solve(d, pose, built, r)) return r;  // drawn from a picture: the same motion on its proportions
  return built;
}

// ---- Looks: colours of a hero in one palette -------------------------------------------------------
struct Look {
  Color skin, hair, eyes;
  Color top, arm, fore, foreB, hand, handB, thigh, shin, foot;
  Color cloth, trim, accent, belt, dark;
};
Look lookFor(const CharacterDef& d, int palette) {
  const Palette& p = d.palettes[palette & 1];
  const Color pri = earthy(p.primary), sec = earthy(p.secondary), acc = earthy(p.accent), skin = earthy(p.skin);
  Look L;
  L.skin = skin;
  L.hair = earthy(p.hair);
  L.eyes = C(p.eyes);
  L.top = pri;
  L.arm = L.fore = L.foreB = pri;
  L.hand = L.handB = skin;
  L.thigh = L.shin = sec;
  L.foot = {70, 50, 40, 255};
  L.cloth = pri;
  L.trim = kGold;
  L.accent = acc;
  L.belt = {104, 70, 46, 255};
  L.dark = {34, 32, 38, 255};
  switch (d.hero) {
    case Hero::Asta:
      L.top = shade(sec, 0.62f);
      L.arm = L.fore = L.foreB = skin;
      L.thigh = L.shin = sec;
      L.foot = {36, 32, 34, 255};
      L.cloth = pri;
      L.trim = acc;
      L.belt = {70, 54, 44, 255};
      break;
    case Hero::Yuno:
      L.top = L.arm = L.fore = L.foreB = sec;
      L.thigh = L.shin = mixColor(sec, {30, 30, 36, 255}, 0.35f);
      L.foot = {44, 38, 40, 255};
      L.cloth = pri;
      L.trim = acc;
      L.belt = acc;
      break;
    case Hero::Ichigo:
      L.thigh = L.shin = pri;
      L.foot = sec;
      L.cloth = sec;  // inner collar, obi
      break;
    case Hero::Byakuya:
      L.top = pri;  // haori
      L.arm = L.fore = L.foreB = sec;
      L.hand = L.handB = {240, 238, 236, 255};
      L.thigh = L.shin = sec;
      L.foot = {236, 234, 230, 255};
      L.cloth = pri;
      L.trim = mixColor(pri, {200, 214, 232, 255}, 0.55f);  // silver scarf
      break;
    case Hero::Link:
      L.fore = {52, 64, 56, 255};  // Ultrahand
      L.hand = {64, 78, 66, 255};
      L.handB = {112, 78, 52, 255};
      L.thigh = L.shin = {128, 108, 84, 255};
      L.foot = {98, 66, 42, 255};
      L.belt = sec;
      L.trim = acc;
      break;
    case Hero::Ganondorf:
      L.arm = L.fore = L.foreB = pri;
      L.hand = L.handB = shade(pri, 1.25f);
      L.thigh = L.shin = shade(pri, 0.9f);
      L.foot = shade(pri, 0.8f);
      L.cloth = sec;
      L.trim = acc;
      L.belt = shade(pri, 0.8f);
      break;
    case Hero::Kratos:
      L.top = L.arm = skin;
      L.fore = L.foreB = sec;  // bracers
      L.thigh = skin;
      L.shin = sec;
      L.foot = shade(sec, 0.8f);
      L.cloth = pri;  // skirt
      L.trim = {200, 160, 84, 255};
      L.belt = {92, 62, 44, 255};
      break;
    case Hero::Gojo:
      L.thigh = L.shin = sec;
      L.foot = {26, 26, 32, 255};
      break;
    case Hero::Sukuna:
      L.thigh = L.shin = shade(pri, 0.9f);
      L.foot = {48, 40, 44, 255};
      L.cloth = sec;  // scarf, sash
      break;
    case Hero::Kenpachi:
      L.top = L.arm = sec;  // black kosode, sleeves rolled up
      L.fore = L.foreB = skin;
      L.thigh = L.shin = sec;
      L.foot = {228, 224, 216, 255};  // white tabi
      L.cloth = pri;                  // tattered captain's haori
      L.trim = acc;                   // the bells in his hair
      L.belt = {226, 222, 214, 255};
      break;
    case Hero::Ulquiorra:
      L.top = L.arm = L.fore = L.foreB = pri;
      L.thigh = L.shin = pri;
      L.foot = {40, 40, 46, 255};
      L.cloth = pri;  // coat tails
      L.belt = sec;   // black sash
      L.trim = acc;   // green
      break;
    case Hero::Hunter:
      L.top = L.arm = pri;  // wyvern plates
      L.fore = L.foreB = shade(pri, 0.85f);
      L.hand = L.handB = {72, 56, 46, 255};
      L.thigh = sec;
      L.shin = pri;
      L.foot = shade(pri, 0.7f);
      L.cloth = sec;
      L.trim = acc;
      L.belt = {92, 62, 44, 255};
      break;
    case Hero::Naruto:
      L.top = pri;  // jacket, black on the shoulders
      L.arm = sec;
      L.fore = L.foreB = pri;
      L.thigh = L.shin = pri;
      L.foot = {44, 44, 56, 255};
      L.cloth = sec;
      L.trim = {196, 200, 210, 255};  // forehead protector
      L.belt = sec;
      break;
    case Hero::Sasuke:
      L.top = L.arm = L.fore = L.foreB = pri;
      L.thigh = L.shin = sec;
      L.foot = {40, 38, 46, 255};
      L.cloth = sec;  // waist cloth
      L.belt = acc;   // purple rope
      L.trim = acc;
      break;
    case Hero::Grimmjow:  // the jacket hangs open: skin underneath
      L.top = skin;
      L.arm = L.fore = L.foreB = pri;
      L.thigh = L.shin = pri;
      L.foot = {44, 44, 50, 255};
      L.cloth = pri;   // jacket
      L.belt = sec;    // black sash
      L.trim = acc;    // teal
      break;
    case Hero::Starrk:
      L.top = L.arm = L.fore = L.foreB = pri;
      L.thigh = L.shin = pri;
      L.foot = {44, 44, 50, 255};
      L.cloth = pri;   // coat tails
      L.belt = sec;    // black sash
      L.trim = acc;
      break;
    case Hero::Kakashi:
      L.top = pri;  // flak jacket
      L.arm = L.fore = L.foreB = sec;
      L.hand = L.handB = {36, 36, 42, 255};  // gloves
      L.thigh = L.shin = sec;
      L.foot = {40, 44, 60, 255};
      L.cloth = sec;
      L.trim = acc;  // metal plates
      L.belt = shade(pri, 0.8f);
      break;
    default: break;
  }
  return L;
}

// Transformations change the colours of the body.
void formLook(Look& L, Form form) {
  switch (form) {
    case Form::SpartanRage:  // red-hot skin
      L.skin = mixColor(L.skin, {255, 120, 60, 255}, 0.45f);
      L.top = L.thigh = mixColor(L.top, {255, 120, 60, 255}, 0.3f);
      L.hand = L.handB = mixColor(L.hand, {255, 150, 70, 255}, 0.5f);
      break;
    case Form::DemonUnion:  // the devil's black arms
      L.fore = L.foreB = L.hand = L.handB = {22, 20, 26, 255};
      break;
    case Form::VastoLorde:  // white hollow body
      L.skin = L.hand = L.handB = {240, 238, 232, 255};
      L.top = L.arm = L.fore = L.foreB = L.thigh = L.shin = {236, 234, 228, 255};
      L.foot = {230, 228, 222, 255};
      L.hair = {240, 128, 40, 255};
      L.accent = {200, 30, 36, 255};
      break;
    case Form::RedDemon:
      L.skin = L.hand = L.handB = L.fore = L.foreB = {196, 46, 40, 255};
      break;
    case Form::Segunda:  // black body, pale face
      L.top = L.arm = L.fore = L.foreB = L.thigh = L.shin = {24, 26, 24, 255};
      L.hand = L.handB = {30, 34, 30, 255};
      L.cloth = {24, 26, 24, 255};
      L.foot = {14, 16, 14, 255};
      break;
    case Form::KuramaAvatar:  // golden chakra cloak
      L.top = L.arm = L.fore = L.foreB = L.thigh = L.shin = {255, 190, 60, 255};
      L.cloth = {230, 140, 30, 255};
      L.foot = {255, 170, 40, 255};
      break;
    case Form::Pantera:  // white armoured body, claws
      L.top = L.arm = L.fore = L.foreB = L.thigh = L.shin = {238, 238, 232, 255};
      L.hand = L.handB = L.foot = {232, 232, 226, 255};
      L.cloth = {238, 238, 232, 255};
      break;
    case Form::LosLobos:  // the fur-lined coat over the uniform
      L.cloth = {240, 238, 232, 255};
      L.trim = {176, 172, 168, 255};  // fur
      L.belt = {120, 84, 56, 255};    // the bandolier
      break;
    default: break;
  }
}

// ---- Faces --------------------------------------------------------------------------------------------
struct EyeStyle {
  Color iris{70, 52, 44, 255};
  float w = 0.16f, h = 0.25f;  // white of the eye (head radii)
  float lid = 0.0f;            // upper lid lowered (calm, narrow eyes)
  float brow = 0.0f;           // + = frowning (inner end lower)
  float browTh = 0.09f;
  bool glow = false;           // glowing iris
  bool dark = false;           // black sclera (hollow)
  bool lashes = false;
};

struct Body {
  Painter& P;
  const Xf& X;
  const CharacterDef& d;
  const Pose& pose;
  const Rig& R;
  const Build& B;
  const Look& L;
  float H, s, time;
  Hero hero;
  int pal;
  bool bust = false;

  WeaponVisual power() const { return pose.held; }
  bool has(WeaponVisual v) const { return pose.held == v; }
  template <class F>
  void group(F fn) {
    for (int p = kLine; p <= kLit; ++p) {
      if ((p == kRim || p == kShadow) && !P.shaded()) continue;
      P.pass = p;
      fn();
    }
  }
  float t(float k) const { return k * H * s; }  // stroke thickness from a fraction of H
  // Head frame: (x, y) in head radii, rotated with the head tilt.
  Vec2 hp(float x, float y) const { return R.head + rotate(Vec2{x, y} * R.hr, R.tilt); }
  Vec2 hp(Vec2 v) const { return hp(v.x, v.y); }
  Vec2 down() const { return {std::sin(pose.rot), std::cos(pose.rot)}; }
  Vec2 rl(Vec2 v) const { return rotate(v * H, pose.lean); }  // torso-relative offset (fractions of H)

  // ---- generic pieces -------------------------------------------------------------------------
  // Which side of a->b (+1: its +n side, see Painter::taper) faces the way the fighter looks, on screen.
  float frontSide(Vec2 a, Vec2 b) const {
    Vec2 d = W(X(b)) - W(X(a));
    if (d.len2() < 1e-4f) return 1.0f;
    d = d.norm();
    const Vec2 n{-d.y, d.x}, fwd = W(X(R.hips + Vec2{1.0f, 0.0f})) - W(X(R.hips));
    return dot(n, fwd) >= 0.0f ? 1.0f : -1.0f;
  }
  // Thigh and shin with flesh (or trousers) on them: quadriceps in front, the calf behind, or a flared hakama.
  void legShape(Vec2 hip, Vec2 knee, Vec2 ft, Color thigh, Color shin) {
    const Cut c = cutFor(hero, pose.form);
    const float l = B.leg, sh = B.shinTh;
    P.taper(X(hip), X(knee), t(l * c.thighA), t(l * c.thighM), t(l * c.thighB), 0.38f, 0.45f * frontSide(hip, knee), thigh);
    const bool flare = c.shinB > 1.0f;  // trousers: a flat hem just above the foot
    P.taper(X(knee), X(flare ? lerpv(knee, ft, 0.9f) : ft), t(sh * c.shinA), t(sh * c.shinM), t(sh * c.shinB), c.shinU,
            -c.calf * frontSide(knee, ft), shin, !flare);
  }
  // Upper arm (shoulder, biceps) and forearm.
  void armShape(Vec2 sh, Vec2 el, Vec2 hd, Color upper, Color lower) {
    const Cut c = cutFor(hero, pose.form);
    P.taper(X(sh), X(el), t(B.arm * c.upperA), t(B.arm * c.upperM), t(B.arm * c.upperB), 0.4f, c.bicep * frontSide(sh, el), upper);
    P.taper(X(el), X(hd), t(B.fore * c.foreA), t(B.fore * c.foreM), t(B.fore * c.foreB), 0.3f, 0.0f, lower);
  }
  // A fist along the forearm: knuckles forward, a little longer than wide.
  void fist(Vec2 el, Vec2 hd, Color c) {
    Vec2 d = hd - el;
    d = d.len2() > 1e-4f ? d.norm() : Vec2{1.0f, 0.0f};
    const Vec2 n{-d.y, d.x}, o = hd + d * (B.hand * H * 0.3f);
    const float r = B.hand * H;
    Vector2 q[8];
    for (int i = 0; i < 8; ++i) {
      const float a = i * kPi * 0.25f + kPi * 0.125f;
      q[i] = X(o + d * (std::cos(a) * r * 1.08f) + n * (std::sin(a) * r * 0.9f));
    }
    P.poly(q, 8, c, true);
  }
  // A boot: heel, sole and toe cap. The foot stays flat on the ground, and follows the shin when it swings up.
  void foot(Vec2 knee, Vec2 f, Color c, float k = 1.0f) {
    const Vec2 sd = f - knee;
    float phi = sd.len2() > 1e-4f ? std::atan2(sd.x, sd.y) : 0.0f;
    phi = clampf(phi, -1.7f, 1.7f) * smoothstep((std::fabs(phi) - 0.3f) / 0.6f);
    const float r = B.foot * H * k;
    const Vec2 pts[6] = {{-0.75f, -0.9f}, {-0.95f, 0.15f}, {1.55f, 0.2f}, {1.75f, -0.2f}, {0.9f, -0.65f}, {0.2f, -1.0f}};
    Vector2 q[6];
    for (int i = 0; i < 6; ++i) q[i] = X(f + rotate(pts[i] * r, -phi));
    P.poly(q, 6, c, true);
  }
  // Wide kimono sleeve hanging under the forearm.
  void sleeveFlap(Vec2 elbow, Vec2 hand, float depth, Color c) {
    Vec2 dn = down() * (depth * H);
    Vec2 a = lerpv(elbow, hand, 0.1f), b = lerpv(elbow, hand, 0.78f);
    P.quad(X(a), X(b), X(b + dn * 0.85f + (hand - elbow) * 0.08f), X(a + dn * 0.45f), c);
  }
  // Panel of cloth between two hanging chains.
  void panel(const Vec2* a, const Vec2* b, int n, Color c, bool lit = false) {
    for (int i = 0; i < n; ++i) P.quad(X(a[i]), X(a[i + 1]), X(b[i + 1]), X(b[i]), c, lit);
  }
  void ribbon(const Vec2* q, int n, float th0, float th1, Color c) {
    for (int i = 0; i < n; ++i) P.seg(X(q[i]), X(q[i + 1]), t(lerpf(th0, th1, n > 1 ? float(i) / (n - 1) : 0.0f)), c);
  }
  // Skirt / coat tails / robe from the waist: the hem corners follow the legs.
  void skirt(float topW, float len, float flare, Color c, float frontLift = 0.0f) {
    const Vec2 w0 = R.hips + rl({-topW, -0.03f}), w1 = R.hips + rl({topW, -0.03f});
    auto hem = [&](Vec2 hip, Vec2 knee, Vec2 foot, float side) {
      Vec2 legDir = (lerpv(knee, foot, 0.3f) - hip).norm();
      Vec2 p = hip + legDir * (len * H);
      return p + Vec2{side * flare * H, 0.0f};
    };
    Vec2 hb = hem(R.hipB, R.kneeB, R.footB, -1.0f), hf = hem(R.hipF, R.kneeF, R.footF, 1.0f);
    hf.y -= frontLift * H;
    Vec2 mid = lerpv(hb, hf, 0.5f) + down() * (0.02f * H);
    Vector2 q[5] = {X(w0), X(w1), X(hf), X(mid), X(hb)};
    P.poly(q, 5, c);
  }
  void hemLine(float topW, float len, float flare, Color c, float frontLift = 0.0f) {
    (void)topW;
    auto hem = [&](Vec2 hip, Vec2 knee, Vec2 foot, float side) {
      Vec2 legDir = (lerpv(knee, foot, 0.3f) - hip).norm();
      return hip + legDir * (len * H) + Vec2{side * flare * H, 0.0f};
    };
    Vec2 hb = hem(R.hipB, R.kneeB, R.footB, -1.0f), hf = hem(R.hipF, R.kneeF, R.footF, 1.0f);
    hf.y -= frontLift * H;
    Vec2 mid = lerpv(hb, hf, 0.5f) + down() * (0.02f * H);
    Vec2 up = rl({0.0f, -0.025f});
    DrawLineEx(X(hb + up), X(mid + up), t(0.022f), P.fill(c));
    DrawLineEx(X(mid + up), X(hf + up), t(0.022f), P.fill(c));
  }

  // ---- hair ---------------------------------------------------------------------------------------
  // Spike: base on a circle (radius r0) around the hair centre hc, tip at distance len (head radii).
  void spike(Vec2 hc, float ang, float halfW, float r0, float len, Color c, float flowK = 1.0f) {
    Vec2 b0 = hc + polar(ang - halfW, r0), b1 = hc + polar(ang + halfW, r0), tip = hc + polar(ang, len);
    Vec2 fl = pose.flowTip * (0.28f * R.hr * flowK * (len - r0));
    P.tri(X(hp(b0)), X(hp(b1)), X(hp(tip) + fl), c);
  }
  // Strand hanging over the face: from two points of the hairline to a tip.
  void strand(Vec2 a, Vec2 b, Vec2 tip, Color c) { P.tri(X(hp(a)), X(hp(b)), X(hp(tip) + pose.flowTip * (0.1f * R.hr)), c); }
  // Base mass of the hair: the top and the back of the skull (convex pieces, the face stays free).
  void cap(Color c, float vol = 1.0f, float front = -0.3f, float nape = 0.42f) {
    const Vec2 top[6] = {{0.99f, front}, {0.78f, -0.8f}, {0.26f, -1.1f}, {-0.36f, -1.08f}, {-0.88f, -0.74f}, {-1.08f, -0.14f}};
    Vector2 q[6];
    for (int i = 0; i < 6; ++i) q[i] = X(hp(top[i] * vol));
    P.poly(q, 6, c);
    const Vec2 back[4] = {{-1.08f, -0.16f}, {-0.94f, nape}, {-0.5f, nape - 0.1f}, {-0.2f, -0.2f}};
    for (int i = 0; i < 4; ++i) q[i] = X(hp(back[i] * vol));
    P.poly(q, 4, c);
  }

  void backHair() {
    const float hr = R.hr;
    if (pose.form == Form::VastoLorde) {  // long hair down the back
      Vec2 a[4], b[4];
      chain(hp(-0.4f, -0.9f), {-0.4f, 1.0f}, 4.4f * hr, 3, pose, time, 0.7f, 0.9f, 0.05f, a);
      chain(hp(-1.0f, 0.1f), {-0.15f, 1.0f}, 3.8f * hr, 3, pose, time + 0.3f, 0.7f, 1.0f, 0.05f, b);
      group([&] { panel(a, b, 3, L.hair); });
      return;
    }
    switch (hero) {
      case Hero::Byakuya: {  // long straight black hair down the back
        Vec2 a[4], b[4];
        const float k = bust ? 0.55f : 1.0f;
        chain(hp(-0.5f, -0.9f), {-0.35f, 1.0f}, 3.3f * hr * k, 3, pose, time, 0.8f, 0.7f, 0.03f, a);
        chain(hp(-0.95f, 0.15f), {-0.1f, 1.0f}, 2.7f * hr * k, 3, pose, time + 0.3f, 0.8f, 0.8f, 0.03f, b);
        group([&] { panel(a, b, 3, L.hair); });
        break;
      }
      case Hero::Link: {  // short ponytail
        Vec2 q[3];
        chain(hp(-0.86f, -0.2f), {-0.85f, 0.75f}, 1.35f * hr, 2, pose, time, 0.45f, 0.9f, 0.08f, q);
        group([&] {
          P.seg(X(q[0]), X(q[1]), t(0.05f), L.hair);
          P.seg(X(q[1]), X(q[2]), t(0.035f), L.hair);
          P.deco([&] { P.circle(X(q[0] + (q[1] - q[0]) * 0.18f), t(0.02f), {120, 88, 58, 255}); });
        });
        break;
      }
      case Hero::Ulquiorra: {  // black hair down to the nape
        Vec2 a[3], b[3];
        chain(hp(-0.6f, -0.6f), {-0.25f, 1.0f}, 1.6f * hr, 2, pose, time, 0.8f, 0.5f, 0.02f, a);
        chain(hp(-1.0f, 0.0f), {-0.05f, 1.0f}, 1.1f * hr, 2, pose, time + 0.3f, 0.8f, 0.5f, 0.02f, b);
        group([&] { panel(a, b, 2, L.hair); });
        break;
      }
      case Hero::Sasuke: {  // the spikes sticking out at the back
        const Vec2 hc{-0.3f, -0.3f};
        group([&] {
          const float sp[4][3] = {{-170, 16, 1.75f}, {-150, 15, 1.95f}, {170, 16, 1.6f}, {150, 14, 1.35f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.8f, k[2], L.hair, 0.8f);
        });
        break;
      }
      case Hero::Starrk: {  // wavy brown hair down to the shoulders
        Vec2 a[4], b[4];
        const float k = bust ? 0.6f : 1.0f;
        chain(hp(-0.45f, -0.8f), {-0.35f, 1.0f}, 2.3f * hr * k, 3, pose, time, 0.7f, 0.8f, 0.06f, a);
        chain(hp(-1.0f, 0.1f), {-0.1f, 1.0f}, 1.9f * hr * k, 3, pose, time + 0.4f, 0.7f, 0.9f, 0.06f, b);
        group([&] { panel(a, b, 3, L.hair); });
        break;
      }
      case Hero::Grimmjow:
        if (pose.form == Form::Pantera) {  // released: the hair falls to the waist
          Vec2 a[4], b[4];
          const float k = bust ? 0.5f : 1.0f;
          chain(hp(-0.4f, -0.9f), {-0.4f, 1.0f}, 4.2f * hr * k, 3, pose, time, 0.7f, 0.9f, 0.05f, a);
          chain(hp(-1.0f, 0.1f), {-0.15f, 1.0f}, 3.7f * hr * k, 3, pose, time + 0.3f, 0.7f, 1.0f, 0.05f, b);
          group([&] { panel(a, b, 3, L.hair); });
        }
        break;
      case Hero::Ganondorf: {  // long red braid
        Vec2 q[4];
        chain(hp(-0.9f, -0.05f), {-0.45f, 1.0f}, 2.6f * hr * (bust ? 0.5f : 1.0f), 3, pose, time, 0.6f, 0.9f, 0.05f, q);
        group([&] {
          ribbon(q, 3, 0.042f, 0.028f, L.hair);
          P.deco([&] {
            P.circle(X(q[0]), t(0.026f), L.trim);
            for (int i = 1; i < 3; ++i) P.circle(X(q[i]), t(0.02f), shade(L.hair, 0.8f));
          });
        });
        break;
      }
      default: break;
    }
  }

  void frontHair() {
    const Color c = L.hair;
    switch (hero) {
      case Hero::Asta: {  // ash-blond, wild spikes up and back
        const Vec2 hc{-0.12f, -0.2f};
        group([&] {
          cap(c, 1.02f, -0.32f, 0.3f);
          const float sp[7][3] = {{-178, 15, 1.25f}, {-152, 14, 1.6f}, {-128, 13, 1.78f}, {-104, 12, 1.82f},
                                  {-82, 12, 1.62f}, {-60, 12, 1.36f}, {-38, 12, 1.12f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.82f, k[2], c);
          strand({0.95f, -0.38f}, {0.55f, -0.62f}, {0.92f, 0.02f}, c);
          strand({0.62f, -0.5f}, {0.22f, -0.7f}, {0.5f, -0.06f}, c);
        });
        // dark headband over the hair, knotted at the back with two tails
        Vec2 k0 = hp(-0.98f, -0.32f);
        Vec2 q[3], r[3];
        chain(k0, {-0.9f, 0.5f}, 0.75f * R.hr, 2, pose, time + 0.7f, 0.35f, 0.9f, 0.2f, q);
        chain(k0, {-1.0f, -0.1f}, 0.85f * R.hr, 2, pose, time, 0.2f, 1.0f, 0.24f, r);
        group([&] {
          P.seg(X(q[0]), X(q[1]), t(0.032f), L.dark);
          P.seg(X(q[1]), X(q[2]), t(0.026f), L.dark);
          P.seg(X(r[0]), X(r[1]), t(0.03f), L.dark);
          P.seg(X(r[1]), X(r[2]), t(0.024f), L.dark);
          P.seg(X(hp(-0.98f, -0.32f)), X(hp(-0.2f, -0.52f)), t(0.05f), L.dark);
          P.seg(X(hp(-0.2f, -0.52f)), X(hp(0.9f, -0.44f)), t(0.05f), L.dark);
        });
        break;
      }
      case Hero::Yuno: {  // black hair, soft spikes, fringe over the forehead
        const Vec2 hc{-0.1f, -0.15f};
        group([&] {
          cap(c, 1.04f, -0.26f, 0.5f);
          const float sp[6][3] = {{150, 18, 1.18f}, {176, 16, 1.2f}, {-158, 15, 1.3f}, {-132, 14, 1.36f}, {-106, 13, 1.24f},
                                  {-80, 12, 1.12f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.84f, k[2], c);
          strand({1.0f, -0.3f}, {0.7f, -0.62f}, {1.0f, 0.12f}, c);
          strand({0.74f, -0.5f}, {0.4f, -0.72f}, {0.62f, 0.14f}, c);
          strand({0.42f, -0.62f}, {0.08f, -0.7f}, {0.24f, 0.05f}, c);
          strand({0.1f, -0.4f}, {-0.22f, -0.44f}, {-0.02f, 0.62f}, c);
        });
        break;
      }
      case Hero::Ichigo: {  // orange, spikes all around
        const Vec2 hc{-0.08f, -0.18f};
        group([&] {
          cap(c, 1.04f, -0.32f, 0.38f);
          const float sp[9][3] = {{158, 16, 1.05f}, {-178, 15, 1.3f}, {-154, 14, 1.5f}, {-130, 13, 1.62f}, {-106, 12, 1.6f},
                                  {-84, 12, 1.5f}, {-62, 12, 1.34f}, {-40, 12, 1.16f}, {-18, 12, 1.0f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.84f, k[2], c, 0.6f);
          strand({1.0f, -0.36f}, {0.66f, -0.6f}, {0.98f, 0.06f}, c);
          strand({0.7f, -0.5f}, {0.36f, -0.7f}, {0.6f, -0.02f}, c);
          strand({0.38f, -0.6f}, {0.02f, -0.7f}, {0.2f, -0.1f}, c);
        });
        break;
      }
      case Hero::Byakuya: {  // smooth black hair, long locks framing the face, white kenseikan
        group([&] {
          cap(c, 1.04f, -0.3f, 0.55f);
          strand({0.92f, -0.42f}, {0.6f, -0.72f}, {0.62f, 0.3f}, c);  // the lock between the eyes
          strand({0.3f, -0.66f}, {-0.05f, -0.7f}, {0.14f, 1.25f}, c);  // side lock to the chin
          strand({-0.1f, -0.5f}, {-0.5f, -0.4f}, {-0.3f, 1.05f}, c);
        });
        const Color ken{246, 246, 250, 255};
        group([&] {
          P.seg(X(hp(-0.3f, -1.0f)), X(hp(-0.08f, -0.7f)), t(0.026f), ken);
          P.seg(X(hp(-0.6f, -0.88f)), X(hp(-0.36f, -0.58f)), t(0.026f), ken);
          P.seg(X(hp(-0.86f, -0.62f)), X(hp(-0.6f, -0.36f)), t(0.026f), ken);
        });
        break;
      }
      case Hero::Link: {  // dirty blond, parted bangs
        const Vec2 hc{-0.1f, -0.18f};
        group([&] {
          cap(c, 1.04f, -0.28f, 0.42f);
          const float sp[4][3] = {{-170, 16, 1.18f}, {-142, 15, 1.26f}, {-114, 14, 1.2f}, {-88, 13, 1.1f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.84f, k[2], c, 0.6f);
          strand({1.0f, -0.32f}, {0.62f, -0.68f}, {0.98f, 0.14f}, c);
          strand({0.56f, -0.66f}, {0.2f, -0.78f}, {0.36f, 0.02f}, c);
          strand({0.12f, -0.5f}, {-0.2f, -0.5f}, {0.0f, 0.7f}, c);
        });
        // pointed ear over the side hair
        group([&] {
          P.tri(X(hp(-0.32f, -0.02f)), X(hp(-0.08f, 0.3f)), X(hp(-1.05f, -0.62f)), L.skin);
          P.deco([&] { P.circle(X(hp(-0.2f, 0.28f)), t(0.012f), {90, 160, 220, 255}); });
        });
        break;
      }
      case Hero::Ganondorf: {  // red hair swept back like flames, gold circlet with a jewel
        const Vec2 hc{-0.2f, -0.2f};
        group([&] {
          cap(c, 1.04f, -0.42f, 0.42f);
          const float sp[5][3] = {{-178, 14, 1.75f}, {-160, 13, 1.85f}, {-142, 12, 1.7f}, {162, 14, 1.45f}, {-124, 12, 1.4f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.84f, k[2], c, 1.2f);
          P.deco([&] {
            P.seg(X(hp(-0.9f, -0.46f)), X(hp(0.0f, -0.62f)), t(0.022f), L.trim);
            P.seg(X(hp(0.0f, -0.62f)), X(hp(0.9f, -0.5f)), t(0.022f), L.trim);
          });
        });
        group([&] {  // the jewel
          Vec2 j = hp(0.72f, -0.58f);
          const float r = 0.2f * R.hr;
          P.tri(X(j + Vec2{0, -r * 1.3f}), X(j + Vec2{r, 0}), X(j + Vec2{0, r * 1.3f}), L.trim);
          P.tri(X(j + Vec2{0, -r * 1.3f}), X(j + Vec2{-r, 0}), X(j + Vec2{0, r * 1.3f}), L.trim);
          P.deco([&] { P.circle(X(j), r * 0.55f * s, {120, 220, 170, 255}); });
        });
        break;
      }
      case Hero::Kratos: {  // bald: no hair; the thick beard
        group([&] {
          Vector2 q[7] = {X(hp(-0.3f, 0.12f)), X(hp(0.3f, 0.36f)), X(hp(0.92f, 0.3f)), X(hp(1.0f, 0.7f)),
                          X(hp(0.62f, 1.38f)), X(hp(0.05f, 1.22f)), X(hp(-0.36f, 0.62f))};
          P.poly(q, 7, c);
          P.deco([&] {
            P.seg(X(hp(0.5f, 0.52f)), X(hp(0.98f, 0.5f)), t(0.02f), shade(c, 0.85f));
          });
        });
        break;
      }
      case Hero::Gojo: {  // snow-white hair spiking up (falls down without the blindfold)
        const bool eyes = has(WeaponVisual::SixEyes);
        const float up = eyes ? 0.72f : 1.0f;
        const Vec2 hc{-0.06f, -0.22f};
        group([&] {
          cap(c, 1.06f, -0.3f, 0.45f);
          const float sp[8][3] = {{-178, 14, 1.3f}, {-156, 13, 1.62f}, {-134, 12, 1.92f}, {-112, 12, 2.06f},
                                  {-90, 12, 2.0f}, {-68, 12, 1.8f}, {-48, 12, 1.5f}, {-28, 12, 1.16f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.84f, 0.84f + (k[2] - 0.84f) * up, c, 0.5f);
          if (eyes) {  // fringe over the forehead
            strand({1.0f, -0.36f}, {0.62f, -0.64f}, {0.95f, 0.02f}, c);
            strand({0.66f, -0.56f}, {0.3f, -0.72f}, {0.5f, -0.04f}, c);
            strand({0.32f, -0.64f}, {-0.04f, -0.7f}, {0.12f, 0.0f}, c);
          } else {
            strand({1.0f, -0.4f}, {0.66f, -0.64f}, {0.96f, -0.18f}, c);
            strand({0.68f, -0.56f}, {0.34f, -0.7f}, {0.56f, -0.2f}, c);
          }
        });
        break;
      }
      case Hero::Sukuna: {  // pink, short spikes
        const Vec2 hc{-0.1f, -0.2f};
        group([&] {
          cap(c, 1.02f, -0.36f, 0.3f);
          const float sp[7][3] = {{-176, 15, 1.2f}, {-152, 14, 1.42f}, {-128, 13, 1.52f}, {-104, 12, 1.5f}, {-82, 12, 1.36f},
                                  {-60, 12, 1.2f}, {-38, 12, 1.04f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.84f, k[2], c, 0.6f);
          strand({0.98f, -0.4f}, {0.62f, -0.62f}, {0.9f, -0.12f}, c);
          strand({0.6f, -0.56f}, {0.26f, -0.7f}, {0.42f, -0.18f}, c);
        });
        break;
      }
      case Hero::Kenpachi: {  // hard black spikes all round, a little bell at every tip
        const Vec2 hc{-0.08f, -0.2f};
        const float sp[11][3] = {{162, 12, 1.5f}, {-178, 11, 1.8f}, {-160, 10, 2.0f}, {-142, 10, 2.1f}, {-124, 10, 2.15f},
                                 {-106, 10, 2.1f}, {-88, 10, 2.0f}, {-70, 10, 1.9f}, {-52, 10, 1.7f}, {-34, 11, 1.45f},
                                 {-16, 11, 1.2f}};
        group([&] {
          cap(c, 1.02f, -0.36f, 0.3f);
          for (auto& k : sp) spike(hc, k[0], k[1], 0.82f, k[2], c, 0.4f);
          strand({0.98f, -0.4f}, {0.66f, -0.62f}, {0.95f, -0.08f}, c);
        });
        group([&] {
          for (auto& k : sp) P.circle(X(hp(hc + polar(k[0], k[2] + 0.12f))), t(0.016f), L.trim);
        });
        break;
      }
      case Hero::Ulquiorra: {  // straight black hair framing the face, the broken helmet with its horn
        group([&] {
          cap(c, 1.04f, -0.3f, 0.5f);
          strand({0.95f, -0.44f}, {0.6f, -0.72f}, {0.64f, 0.34f}, c);  // the lock between the eyes
          strand({0.36f, -0.66f}, {0.0f, -0.72f}, {0.2f, 1.0f}, c);
          strand({-0.1f, -0.5f}, {-0.5f, -0.42f}, {-0.3f, 0.95f}, c);
        });
        const Color bone{238, 238, 232, 255};
        group([&] {
          Vector2 q[5] = {X(hp(-0.95f, -0.2f)), X(hp(-0.75f, -0.85f)), X(hp(-0.1f, -1.1f)), X(hp(0.05f, -0.82f)),
                          X(hp(-0.55f, -0.3f))};
          P.poly(q, 5, bone);
          P.tri(X(hp(-0.4f, -0.95f)), X(hp(-0.08f, -1.02f)), X(hp(-0.55f, -1.9f) + pose.flowTip * (0.05f * R.hr)), bone);
        });
        break;
      }
      case Hero::Hunter: {  // wyvern-plate helmet: crest of horns, cheek guards, a fringe under the visor
        const Color plate = L.top, dark = shade(L.top, 0.7f);
        group([&] {
          strand({1.0f, -0.24f}, {0.66f, -0.46f}, {0.95f, 0.16f}, c);
          strand({0.66f, -0.4f}, {0.3f, -0.5f}, {0.5f, 0.04f}, c);
          cap(plate, 1.12f, -0.44f, 0.5f);
          const Vec2 hc{-0.2f, -0.3f};
          spike(hc, -150, 12, 0.9f, 1.9f, dark, 0.3f);
          spike(hc, -128, 11, 0.95f, 2.1f, dark, 0.3f);
          spike(hc, -172, 12, 0.9f, 1.6f, dark, 0.3f);
          P.quad(X(hp(-0.8f, -0.1f)), X(hp(-0.05f, 0.05f)), X(hp(0.05f, 0.75f)), X(hp(-0.6f, 0.7f)), plate);
          P.deco([&] {
            P.seg(X(hp(-0.2f, -0.6f)), X(hp(1.05f, -0.46f)), t(0.026f), L.trim);  // visor rim
            P.circle(X(hp(0.15f, -0.82f)), t(0.02f), L.trim);
          });
        });
        break;
      }
      case Hero::Naruto: {  // spiky blond hair, the forehead protector and its tails
        const Vec2 hc{-0.08f, -0.22f};
        Vec2 q[3], r[3];
        chain(hp(-0.98f, -0.38f), {-1.0f, 0.3f}, 1.0f * R.hr, 2, pose, time + 0.7f, 0.3f, 1.0f, 0.22f, q);
        chain(hp(-0.98f, -0.38f), {-0.9f, 0.7f}, 0.9f * R.hr, 2, pose, time, 0.4f, 0.9f, 0.2f, r);
        group([&] {
          P.seg(X(q[0]), X(q[1]), t(0.028f), L.dark);
          P.seg(X(q[1]), X(q[2]), t(0.022f), L.dark);
          P.seg(X(r[0]), X(r[1]), t(0.026f), L.dark);
          P.seg(X(r[1]), X(r[2]), t(0.02f), L.dark);
        });
        group([&] {
          cap(c, 1.04f, -0.34f, 0.36f);
          const float sp[10][3] = {{168, 15, 1.3f}, {-172, 14, 1.55f}, {-150, 13, 1.75f}, {-128, 12, 1.85f}, {-106, 12, 1.8f},
                                   {-84, 12, 1.7f}, {-62, 12, 1.5f}, {-40, 12, 1.3f}, {-18, 12, 1.12f}, {4, 12, 1.0f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.84f, k[2], c, 0.6f);
          strand({1.0f, -0.32f}, {0.7f, -0.56f}, {1.02f, 0.1f}, c);
          strand({0.7f, -0.46f}, {0.36f, -0.62f}, {0.58f, 0.04f}, c);
          P.seg(X(hp(-0.98f, -0.38f)), X(hp(0.0f, -0.6f)), t(0.05f), L.dark);  // headband
          P.seg(X(hp(0.0f, -0.6f)), X(hp(0.96f, -0.46f)), t(0.05f), L.dark);
          P.quad(X(hp(0.2f, -0.82f)), X(hp(0.98f, -0.68f)), X(hp(0.98f, -0.3f)), X(hp(0.2f, -0.4f)), L.trim, true);
        });
        if (P.details()) {  // the leaf engraved on the plate
          DrawLineEx(X(hp(0.48f, -0.62f)), X(hp(0.74f, -0.5f)), t(0.008f) + 0.5f, P.fill({70, 74, 84, 255}));
          DrawLineEx(X(hp(0.74f, -0.5f)), X(hp(0.6f, -0.42f)), t(0.008f) + 0.5f, P.fill({70, 74, 84, 255}));
        }
        break;
      }
      case Hero::Sasuke: {  // black hair, long bangs on both sides of the face
        group([&] {
          cap(c, 1.04f, -0.3f, 0.4f);
          strand({1.0f, -0.36f}, {0.62f, -0.66f}, {0.98f, 0.62f}, c);   // front bang down to the jaw
          strand({0.5f, -0.6f}, {0.18f, -0.7f}, {0.42f, 0.08f}, c);
          strand({0.1f, -0.52f}, {-0.26f, -0.5f}, {-0.08f, 0.9f}, c);   // side bang
        });
        break;
      }
      case Hero::Grimmjow: {  // light blue hair, hard spikes swept back, two locks over the forehead
        const Vec2 hc{-0.14f, -0.2f};
        const bool released = pose.form == Form::Pantera;
        group([&] {
          cap(c, 1.04f, -0.34f, 0.36f);
          const float sp[7][3] = {{164, 14, 1.4f}, {-178, 15, 1.62f}, {-160, 14, 1.8f}, {-142, 13, 1.82f},
                                  {-124, 12, 1.66f}, {-106, 12, 1.42f}, {-88, 12, 1.18f}};
          for (auto& k : sp) spike(hc, k[0], k[1], 0.84f, released ? 0.84f + (k[2] - 0.84f) * 0.55f : k[2], c, 0.7f);
          strand({0.98f, -0.36f}, {0.7f, -0.6f}, {0.96f, 0.12f}, c);
          strand({0.72f, -0.5f}, {0.42f, -0.66f}, {0.66f, 0.04f}, c);
        });
        break;
      }
      case Hero::Starrk: {  // wavy brown hair, parted, long locks framing the face
        const Vec2 hc{-0.2f, -0.22f};
        group([&] {
          cap(c, 1.06f, -0.3f, 0.5f);
          spike(hc, -172, 17, 0.86f, 1.32f, c, 0.6f);
          spike(hc, -150, 16, 0.86f, 1.36f, c, 0.6f);
          spike(hc, -126, 15, 0.86f, 1.26f, c, 0.6f);
          strand({0.95f, -0.4f}, {0.62f, -0.7f}, {0.86f, 0.28f}, c);   // the lock between the eyes
          strand({0.34f, -0.66f}, {-0.02f, -0.72f}, {0.18f, 1.05f}, c); // down to the jaw
          strand({-0.1f, -0.5f}, {-0.5f, -0.4f}, {-0.26f, 0.95f}, c);
        });
        break;
      }
      case Hero::Kakashi: {  // silver hair swept up and back, the headband tilted over one eye
        const Vec2 hc{-0.12f, -0.2f};
        group([&] {
          cap(c, 1.04f, -0.34f, 0.35f);
          const float sp[8][3] = {{-178, 14, 1.5f}, {-160, 13, 1.85f}, {-142, 12, 2.05f}, {-124, 12, 2.15f},
                                  {-106, 12, 2.05f}, {-88, 12, 1.8f}, {-70, 12, 1.5f}, {-52, 12, 1.2f}};
          for (auto& k : sp) spike(hc, k[0] - 12.0f, k[1], 0.84f, k[2], c, 0.6f);
          strand({1.0f, -0.3f}, {0.7f, -0.56f}, {1.0f, 0.06f}, c);
        });
        const bool open = has(WeaponVisual::SharinganEye) || has(WeaponVisual::DoubleMangekyo);
        group([&] {
          // the protector slants down over the far eye (pushed up when the Sharingan is out)
          const float dy = open ? -0.3f : 0.0f;
          P.seg(X(hp(-0.98f, -0.5f)), X(hp(0.1f, -0.42f + dy)), t(0.05f), L.dark);
          P.seg(X(hp(0.1f, -0.42f + dy)), X(hp(0.98f, -0.08f + dy)), t(0.05f), L.dark);
          P.quad(X(hp(0.34f, -0.62f + dy)), X(hp(1.02f, -0.36f + dy)), X(hp(1.0f, 0.12f + dy)), X(hp(0.3f, -0.14f + dy)), L.trim, true);
        });
        break;
      }
      default: break;
    }
  }

  // Horns of the transformations, over the hair.
  void horns() {
    const Color bone{240, 234, 222, 255};
    switch (pose.form) {
      case Form::VastoLorde:  // two long horns sweeping forward
        group([&] {
          for (int i = 0; i < 2; ++i) {
            const float x = i ? 0.55f : 0.1f;
            P.tri(X(hp(x - 0.18f, -0.7f)), X(hp(x + 0.16f, -0.78f)), X(hp(x + 1.3f, -2.0f)), bone);
          }
        });
        break;
      case Form::RedDemon:
        group([&] {
          P.tri(X(hp(0.1f, -0.85f)), X(hp(0.35f, -0.9f)), X(hp(0.3f, -1.6f)), {60, 20, 20, 255});
          P.tri(X(hp(0.55f, -0.8f)), X(hp(0.8f, -0.78f)), X(hp(0.9f, -1.45f)), {60, 20, 20, 255});
        });
        break;
      case Form::DemonUnion:  // one black horn
        group([&] { P.tri(X(hp(0.2f, -0.85f)), X(hp(0.5f, -0.88f)), X(hp(0.1f, -1.75f)), {20, 18, 24, 255}); });
        break;
      case Form::Pantera:  // the mask has become a crest with the ears of a panther
        group([&] {
          P.tri(X(hp(-0.62f, -0.78f)), X(hp(-0.2f, -0.96f)), X(hp(-0.6f, -1.75f)), bone);
          P.tri(X(hp(0.05f, -0.98f)), X(hp(0.48f, -0.86f)), X(hp(0.24f, -1.7f)), bone);
          P.deco([&] { P.tri(X(hp(-0.5f, -0.9f)), X(hp(-0.3f, -0.98f)), X(hp(-0.55f, -1.45f)), {120, 220, 240, 255}, false); });
        });
        break;
      case Form::Segunda:  // the helmet's horns grow long
        group([&] {
          P.tri(X(hp(-0.6f, -0.8f)), X(hp(-0.2f, -0.95f)), X(hp(-1.2f, -2.2f)), bone);
          P.tri(X(hp(0.2f, -0.9f)), X(hp(0.5f, -0.85f)), X(hp(0.6f, -2.0f)), bone);
        });
        break;
      default: break;
    }
  }

  // ---- face ---------------------------------------------------------------------------------------
  EyeStyle eyeStyle() const {
    EyeStyle e;
    e.iris = L.eyes;
    switch (hero) {
      case Hero::Asta: e.w = 0.17f; e.h = 0.28f; e.brow = 0.12f; e.browTh = 0.11f; break;
      case Hero::Yuno: e.w = 0.16f; e.h = 0.22f; e.lid = 0.25f; e.brow = 0.05f; break;
      case Hero::Ichigo: e.w = 0.16f; e.h = 0.22f; e.lid = 0.2f; e.brow = 0.3f; e.browTh = 0.1f;
        e.dark = pal == 1; break;
      case Hero::Byakuya: e.w = 0.16f; e.h = 0.18f; e.lid = 0.45f; e.brow = 0.0f; e.browTh = 0.07f; break;
      case Hero::Link: e.w = 0.16f; e.h = 0.26f; e.brow = 0.06f; break;
      case Hero::Ganondorf: e.w = 0.15f; e.h = 0.17f; e.lid = 0.3f; e.brow = 0.42f; e.browTh = 0.14f; e.glow = true; break;
      case Hero::Kratos: e.w = 0.15f; e.h = 0.17f; e.lid = 0.3f; e.brow = 0.45f; e.browTh = 0.15f; break;
      case Hero::Gojo: e.w = 0.17f; e.h = 0.25f; e.lashes = true; e.glow = true; e.iris = {110, 210, 255, 255}; break;
      case Hero::Sukuna: e.w = 0.16f; e.h = 0.2f; e.lid = 0.15f; e.brow = 0.35f; e.browTh = 0.1f;
        e.glow = has(WeaponVisual::CursedFinger); break;
      case Hero::Kenpachi: e.w = 0.14f; e.h = 0.18f; e.lid = 0.25f; e.brow = 0.45f; e.browTh = 0.13f; break;
      case Hero::Ulquiorra: e.w = 0.16f; e.h = 0.22f; e.lid = 0.35f; e.browTh = 0.05f; break;
      case Hero::Hunter: e.w = 0.16f; e.h = 0.24f; e.brow = 0.1f; break;
      case Hero::Naruto: e.w = 0.18f; e.h = 0.27f; e.brow = 0.12f;
        if (has(WeaponVisual::KuramaChakra)) { e.iris = {255, 120, 40, 255}; e.glow = true; }
        if (has(WeaponVisual::SageScroll)) e.iris = {240, 200, 60, 255};
        break;
      case Hero::Sasuke: e.w = 0.16f; e.h = 0.2f; e.lid = 0.3f; e.brow = 0.2f;
        if (has(WeaponVisual::Mangekyo) || has(WeaponVisual::SusanooBow)) { e.iris = {220, 20, 30, 255}; e.glow = true; }
        break;
      case Hero::Kakashi: e.w = 0.16f; e.h = 0.2f; e.lid = 0.55f; break;
      case Hero::Grimmjow: e.w = 0.16f; e.h = 0.2f; e.lid = 0.25f; e.brow = 0.42f; e.browTh = 0.1f; break;
      case Hero::Starrk: e.w = 0.15f; e.h = 0.2f; e.lid = 0.55f; e.brow = -0.15f; e.browTh = 0.08f; break;
      default: break;
    }
    switch (pose.form) {  // transformed eyes
      case Form::VastoLorde: e.dark = true; e.iris = {250, 210, 60, 255}; e.glow = true; break;
      case Form::HollowMask: e.dark = true; e.iris = {250, 210, 60, 255}; e.glow = true; e.lid = 0.1f; break;
      case Form::RedDemon: e.iris = {255, 220, 80, 255}; e.glow = true; break;
      case Form::Segunda: e.iris = {90, 255, 150, 255}; e.glow = true; break;
      case Form::KuramaAvatar: e.iris = {255, 120, 40, 255}; e.glow = true; break;
      case Form::SpartanRage: e.iris = {255, 230, 140, 255}; e.glow = true; break;
      case Form::Pantera: e.iris = {130, 240, 255, 255}; e.glow = true; e.lid = 0.15f; break;
      case Form::LosLobos: e.lid = 0.35f; break;
      default: break;
    }
    return e;
  }
  void eye(Vec2 e, float k, const EyeStyle& st, bool far) {
    const float r = R.hr * k;
    const Color dark = P.fill(kInkFace);
    auto S = [&](float v) { return v * r * s; };
    if (pose.hurt) {
      Vector2 c = X(e);
      DrawLineEx({c.x - S(0.18f), c.y - S(0.16f)}, {c.x + S(0.16f), c.y}, S(0.08f) + 1.0f, dark);
      DrawLineEx({c.x - S(0.18f), c.y + S(0.16f)}, {c.x + S(0.16f), c.y}, S(0.08f) + 1.0f, dark);
      return;
    }
    if (pose.blink) {
      DrawLineEx(X(e + Vec2{-st.w * r, 0.02f * r}), X(e + Vec2{st.w * r, 0.05f * r}), S(0.07f) + 0.8f, dark);
      return;
    }
    const float h = st.h * (1.0f - 0.35f * st.lid);
    const Vec2 c = e + Vec2{0.0f, 0.06f * st.lid * r};
    if (st.glow && P.alpha > 0.9f) disc(X(c), S(st.w * 2.2f), withAlpha(P.fill(st.iris), P.alpha * 0.25f));
    fillEllipse(X(c), S(st.w), S(h), P.fill(st.dark ? Color{20, 16, 18, 255} : Color{252, 248, 238, 255}));
    const Vec2 ic = c + Vec2{0.05f * r, 0.02f * r};
    fillEllipse(X(ic), S(st.w * 0.66f), S(h * 0.82f), P.fill(st.iris));
    if (!st.glow) fillEllipse(X(ic + Vec2{0.01f * r, 0.02f * r}), S(st.w * 0.34f), S(h * 0.44f), P.fill({24, 18, 18, 255}));
    else fillEllipse(X(ic), S(st.w * 0.3f), S(h * 0.4f), P.fill(mixColor(st.iris, WHITE, 0.75f)));
    disc(X(ic + Vec2{-0.04f * r, -0.08f * r}), S(0.05f), P.fill(WHITE));
    // upper lid line (anime lashes), thicker at the outer corner
    DrawLineEx(X(c + Vec2{-st.w * r * 1.05f, -h * r * 0.55f}), X(c + Vec2{st.w * r * 1.1f, -h * r * 0.78f}),
               S(0.085f) + 0.8f, dark);
    if (st.lashes) {
      Color lash = P.fill({250, 250, 255, 255});
      DrawLineEx(X(c + Vec2{st.w * r * 0.9f, -h * r * 0.8f}), X(c + Vec2{st.w * r * 1.5f, -h * r * 1.15f}), S(0.05f) + 0.6f, lash);
      DrawLineEx(X(c + Vec2{-st.w * r * 1.0f, -h * r * 0.5f}), X(c + Vec2{-st.w * r * 1.5f, -h * r * 0.8f}), S(0.05f) + 0.6f, lash);
    }
    // brow
    {  // brow: lower at the inner end (towards the nose) when frowning
      const float by = -h * r - 0.16f * r, dy = st.brow * 0.14f * r;
      Vec2 b0 = c + Vec2{-st.w * r * 1.1f, by + (far ? dy : -dy)};
      Vec2 b1 = c + Vec2{st.w * r * 1.15f, by + (far ? -dy : dy) - 0.02f * r};
      Color bc = hero == Hero::Gojo ? Color{236, 236, 244, 255} : shade(L.hair, 0.55f);
      if (hero == Hero::Kratos) bc = shade(L.hair, 0.8f);
      DrawLineEx(X(b0), X(b1), S(st.browTh) + 1.0f, P.fill(bc));
    }
  }

  // Ichigo's Visored mask: bone white, red stripes over one eye, the teeth, black eyes with yellow irises.
  void hollowMask(Vec2 eN, Vec2 eF, const EyeStyle& st) {
    const Color bone = P.fill({246, 244, 238, 255}), red = P.fill({196, 26, 34, 255}), ink = P.fill({16, 12, 14, 255});
    Vector2 q[7] = {X(hp(-0.35f, -0.62f)), X(hp(0.5f, -0.78f)), X(hp(1.08f, -0.42f)), X(hp(1.1f, 0.42f)),
                    X(hp(0.72f, 1.02f)), X(hp(0.12f, 1.06f)), X(hp(-0.42f, 0.5f))};
    Vector2 g[7];
    growPoly(q, 7, P.ow * 0.8f, g);
    fanFill(g, 7, P.line());
    fanFill(q, 7, bone);
    DrawLineEx(X(hp(0.32f, -0.7f)), X(hp(0.3f, 0.62f)), t(0.03f), red);  // the stripes
    DrawLineEx(X(hp(0.1f, -0.66f)), X(hp(0.06f, 0.5f)), t(0.022f), red);
    for (int i = 0; i < 2; ++i) {  // eye holes
      const Vec2 e = i ? eF : eN;
      const float k = i ? 0.82f : 1.0f;
      fillEllipse(X(e), st.w * R.hr * s * 1.15f * k, st.h * R.hr * s * 0.85f * k, ink);
      if (!pose.blink && !pose.hurt) {
        fillEllipse(X(e + Vec2{0.04f * R.hr, 0.0f}), st.w * R.hr * s * 0.45f * k, st.h * R.hr * s * 0.5f * k,
                    P.fill({250, 210, 60, 255}));
      }
    }
    const Vec2 m0 = hp(0.2f, 0.72f), m1 = hp(0.98f, 0.62f);  // the teeth
    DrawLineEx(X(m0), X(m1), t(0.012f) + 0.6f, ink);
    for (int i = 0; i < 5; ++i) {
      const Vec2 p = lerpv(m0, m1, (i + 0.5f) / 5.0f);
      DrawLineEx(X(p + Vec2{0.0f, -0.07f * R.hr}), X(p + Vec2{0.0f, 0.07f * R.hr}), t(0.008f) + 0.5f, ink);
    }
  }

  void face() {
    if (!P.details()) return;
    const float hr = R.hr;
    const EyeStyle st = eyeStyle();
    const Color dark = P.fill(kInkFace);
    const Vec2 eN = hp(0.16f, 0.08f), eF = hp(0.62f, 0.08f);
    // marks under the eyes and on the skin (painted before the eyes)
    if (hero == Hero::Kratos) {  // the red tattoo: over the scalp, around the eye, down the cheek
      const Color red = P.fill(L.accent);
      DrawLineEx(X(hp(0.55f, -0.78f)), X(hp(0.0f, -0.98f)), t(0.034f), red);
      DrawLineEx(X(hp(0.0f, -0.98f)), X(hp(-0.6f, -0.78f)), t(0.034f), red);
      DrawLineEx(X(hp(-0.6f, -0.78f)), X(hp(-0.92f, -0.3f)), t(0.034f), red);
      fillEllipse(X(eN + Vec2{0.0f, -0.02f * hr}), 0.3f * hr * s, 0.3f * hr * s, red);
      DrawLineEx(X(hp(0.12f, -0.5f)), X(hp(0.3f, -0.8f)), t(0.03f), red);
      DrawLineEx(X(hp(0.1f, 0.2f)), X(hp(0.02f, 0.62f)), t(0.026f), red);
    }
    if (hero == Hero::Ichigo && pal == 1) {  // fragment of the hollow mask, red stripes
      Vector2 q[5] = {X(hp(-0.15f, -0.52f)), X(hp(0.5f, -0.6f)), X(hp(0.55f, 0.42f)), X(hp(0.25f, 0.85f)), X(hp(-0.2f, 0.55f))};
      Vector2 g[5];
      growPoly(q, 5, P.ow * 0.8f, g);
      fanFill(g, 5, P.line());
      fanFill(q, 5, P.fill({246, 244, 238, 255}));
      DrawLineEx(X(hp(0.08f, -0.55f)), X(hp(0.02f, 0.62f)), t(0.022f), P.fill({200, 30, 36, 255}));
      DrawLineEx(X(hp(0.32f, -0.58f)), X(hp(0.3f, 0.48f)), t(0.018f), P.fill({200, 30, 36, 255}));
    }
    if (hero == Hero::Grimmjow) {  // teal markings drawn out from the corners of the eyes
      const Color teal = P.fill(pose.form == Form::Pantera ? Color{60, 220, 240, 255} : Color{40, 170, 160, 255});
      DrawLineEx(X(eN + Vec2{-0.1f * hr, 0.14f * hr}), X(eN + Vec2{-0.4f * hr, 0.34f * hr}), t(0.016f) + 0.6f, teal);
      DrawLineEx(X(eF + Vec2{0.1f * hr, 0.14f * hr}), X(eF + Vec2{0.3f * hr, 0.32f * hr}), t(0.014f) + 0.6f, teal);
    }
    if (hero == Hero::Starrk) {  // a goatee and a shadow of beard
      fillEllipse(X(hp(0.5f, 0.98f)), 0.18f * hr * s, 0.12f * hr * s, P.fill(shade(L.hair, 0.9f)));
      fillEllipse(X(hp(0.3f, 0.8f)), 0.32f * hr * s, 0.16f * hr * s, P.fill(withAlpha(shade(L.skin, 0.8f), 0.6f)));
    }
    if (hero == Hero::Naruto && has(WeaponVisual::SageScroll)) {  // sage mode: orange pigment round the eyes
      fillEllipse(X(eN + Vec2{0.0f, -0.04f * hr}), 0.26f * hr * s, 0.32f * hr * s, P.fill({236, 120, 40, 255}));
      fillEllipse(X(eF + Vec2{0.0f, -0.04f * hr}), 0.22f * hr * s, 0.28f * hr * s, P.fill({236, 120, 40, 255}));
    }
    if (hero == Hero::Kakashi) {  // the mask over the nose and the mouth
      Vector2 q[6] = {X(hp(-0.42f, 0.28f)), X(hp(0.4f, 0.24f)), X(hp(1.02f, 0.22f)), X(hp(0.9f, 0.86f)),
                      X(hp(0.36f, 1.12f)), X(hp(-0.36f, 0.8f))};
      fanFill(q, 6, P.fill(shade(L.cloth, 0.85f)));
      DrawLineEx(X(hp(1.0f, 0.24f)), X(hp(0.6f, 0.62f)), t(0.008f) + 0.5f, P.fill(shade(L.cloth, 0.6f)));
    }
    const bool sharingan = has(WeaponVisual::SharinganEye) || has(WeaponVisual::DoubleMangekyo);
    if (hero == Hero::Gojo && !has(WeaponVisual::SixEyes)) {  // the blindfold (drawn with the head): a fold
      DrawLineEx(X(hp(-0.7f, -0.14f)), X(hp(0.9f, -0.06f)), t(0.008f) + 0.5f, P.fill({70, 70, 86, 255}));
    } else if (hero == Hero::Kenpachi) {  // eyepatch on one eye, a scar across the other
      eye(eF, 0.82f, st, true);
      fillEllipse(X(eN), 0.24f * hr * s, 0.28f * hr * s, P.fill({20, 18, 22, 255}));
      DrawLineEx(X(hp(-0.95f, -0.42f)), X(eN), t(0.012f) + 0.6f, P.fill({20, 18, 22, 255}));
      DrawLineEx(X(hp(0.6f, -0.62f)), X(hp(0.76f, 0.62f)), t(0.014f) + 0.6f, P.fill({150, 60, 60, 255}));
    } else if (hero == Hero::Kakashi && !sharingan) {  // one lazy eye, the other under the protector
      eye(eN, 1.0f, st, false);
    } else {
      EyeStyle far = st;
      if (hero == Hero::Kakashi) {  // the Sharingan: red, glowing
        far.iris = {220, 20, 30, 255};
        far.glow = true;
        far.lid = 0.15f;
        if (has(WeaponVisual::DoubleMangekyo)) {
          EyeStyle both = far;
          eye(eN, 1.0f, both, false);
        } else {
          eye(eN, 1.0f, st, false);
        }
        eye(eF, 0.82f, far, true);
        DrawLineEx(X(hp(0.6f, -0.5f)), X(hp(0.66f, 0.3f)), t(0.012f) + 0.5f, P.fill({150, 70, 70, 255}));  // scar
      } else {
        eye(eN, 1.0f, st, false);
        eye(eF, 0.82f, st, true);
      }
    }
    if (pose.form == Form::HollowMask) hollowMask(eN, eF, st);
    if (hero == Hero::Grimmjow) {  // what is left of his mask: the right jaw bone over his cheek, teeth bared
      const Color bone = P.fill({246, 244, 236, 255}), ink = P.fill({30, 24, 26, 255});
      Vector2 q[6] = {X(hp(-0.52f, 0.3f)), X(hp(-0.22f, 0.4f)), X(hp(0.26f, 0.7f)), X(hp(0.22f, 0.88f)),
                      X(hp(-0.24f, 0.7f)), X(hp(-0.56f, 0.5f))};
      Vector2 g[6];
      growPoly(q, 6, P.ow * 0.8f, g);
      fanFill(g, 6, P.line());
      fanFill(q, 6, bone);
      for (int i = 0; i < 3; ++i) {
        const Vec2 a = lerpv(hp(-0.2f, 0.44f), hp(0.22f, 0.72f), (i + 0.5f) / 3.0f);
        DrawLineEx(X(a), X(a + Vec2{0.0f, 0.1f * hr}), t(0.008f) + 0.5f, ink);
      }
    }
    if (hero == Hero::Ulquiorra) {  // green tear marks down the cheeks
      for (int i = 0; i < 2; ++i) {
        const Vec2 e = i ? eF : eN;
        DrawLineEx(X(e + Vec2{0.0f, 0.2f * hr}), X(e + Vec2{0.03f * hr, 0.78f * hr}), t(0.018f) + 0.6f, P.fill({60, 170, 110, 255}));
      }
    }
    if (hero == Hero::Naruto && !pose.hurt) {  // whiskers
      for (int i = 0; i < 3; ++i) {
        const float y = 0.38f + i * 0.1f;
        DrawLineEx(X(hp(-0.18f, y)), X(hp(0.16f, y + 0.02f)), t(0.008f) + 0.5f, P.fill({70, 50, 40, 255}));
        DrawLineEx(X(hp(0.7f, y)), X(hp(0.92f, y - 0.02f)), t(0.007f) + 0.5f, P.fill({70, 50, 40, 255}));
      }
    }
    if (hero == Hero::Sukuna) {  // extra eyes under the eyes, marks on the cheeks and the brow
      const bool hot = has(WeaponVisual::CursedFinger);
      const Color mk = P.fill(hot ? Color{230, 30, 40, 255} : Color{26, 20, 24, 255});
      if (hot) {
        const Color gl = withAlpha(P.fill({255, 60, 60, 255}), P.alpha * 0.3f);
        DrawLineEx(X(hp(0.05f, 0.5f)), X(hp(0.75f, 0.42f)), t(0.05f), gl);
        DrawLineEx(X(hp(0.1f, -0.66f)), X(hp(0.5f, -0.6f)), t(0.05f), gl);
      }
      DrawLineEx(X(hp(-0.1f, 0.5f)), X(hp(0.36f, 0.48f)), t(0.02f), mk);
      DrawLineEx(X(hp(0.5f, 0.46f)), X(hp(0.82f, 0.44f)), t(0.016f), mk);
      DrawLineEx(X(hp(0.2f, -0.62f)), X(hp(0.5f, -0.58f)), t(0.016f), mk);
      DrawLineEx(X(hp(0.35f, -0.6f)), X(hp(0.4f, -0.4f)), t(0.014f), mk);
      if (!pose.hurt && !pose.blink) {
        for (int i = 0; i < 2; ++i) {
          Vec2 e = (i ? eF : eN) + Vec2{0.02f * hr, 0.3f * hr};
          float k = i ? 0.75f : 0.95f;
          fillEllipse(X(e), 0.12f * hr * k * s, 0.07f * hr * k * s, P.fill({250, 240, 232, 255}));
          fillEllipse(X(e + Vec2{0.02f * hr, 0}), 0.06f * hr * k * s, 0.06f * hr * k * s, P.fill(L.eyes));
          DrawLineEx(X(e + Vec2{-0.14f * hr * k, -0.05f * hr}), X(e + Vec2{0.14f * hr * k, -0.07f * hr}), t(0.01f) + 0.6f, dark);
        }
      }
    }
    // nose and mouth
    DrawLineEx(X(hp(0.9f, 0.28f)), X(hp(0.96f, 0.38f)), t(0.008f) + 0.6f, withAlpha(dark, P.alpha * 0.6f));
    const Vec2 mo = hp(0.56f, 0.66f);
    if (pose.hurt || pose.shout) {
      fillEllipse(X(mo), 0.16f * hr * s, 0.12f * hr * s, P.fill({70, 24, 30, 255}));
      DrawLineEx(X(mo + Vec2{-0.12f * hr, -0.07f * hr}), X(mo + Vec2{0.12f * hr, -0.07f * hr}), t(0.012f), P.fill(WHITE));
    } else if (hero == Hero::Asta) {  // big grin
      DrawLineEx(X(mo + Vec2{-0.2f * hr, -0.04f * hr}), X(mo + Vec2{0.16f * hr, -0.08f * hr}), t(0.03f), P.fill({248, 246, 240, 255}));
      DrawLineEx(X(mo + Vec2{-0.22f * hr, -0.02f * hr}), X(mo + Vec2{0.18f * hr, -0.04f * hr}), t(0.008f) + 0.6f, dark);
    } else if (hero == Hero::Sukuna) {  // wicked smile
      DrawLineEx(X(mo + Vec2{-0.18f * hr, -0.08f * hr}), X(mo + Vec2{0.06f * hr, 0.0f}), t(0.01f) + 0.6f, dark);
      DrawLineEx(X(mo + Vec2{0.06f * hr, 0.0f}), X(mo + Vec2{0.2f * hr, -0.1f * hr}), t(0.01f) + 0.6f, dark);
    } else if (hero == Hero::Kenpachi || hero == Hero::Naruto || hero == Hero::Grimmjow) {  // wide grin
      DrawLineEx(X(mo + Vec2{-0.22f * hr, -0.06f * hr}), X(mo + Vec2{0.2f * hr, -0.1f * hr}), t(0.03f), P.fill({248, 246, 240, 255}));
      DrawLineEx(X(mo + Vec2{-0.24f * hr, -0.04f * hr}), X(mo + Vec2{0.22f * hr, -0.06f * hr}), t(0.008f) + 0.6f, dark);
    } else if (hero != Hero::Kratos && hero != Hero::Kakashi) {
      DrawLineEx(X(mo + Vec2{-0.1f * hr, 0.0f}), X(mo + Vec2{0.1f * hr, -0.01f * hr}), t(0.009f) + 0.6f, dark);
    }
  }

  // Head: neck, skull, chin, ear, then the face.
  void head() {
    const float hr = R.hr;
    group([&] {
      if (hero != Hero::Gojo && !bust) P.seg(X(R.neck), X(lerpv(R.neck, R.head, 0.55f)), t(B.head * 0.62f), L.skin);
      if (hero == Hero::Ganondorf || hero == Hero::Kratos || hero == Hero::Kenpachi) {  // square jaw
        P.quad(X(hp(-0.5f, 0.3f)), X(hp(0.95f, 0.25f)), X(hp(0.82f, 0.95f)), X(hp(0.0f, 1.0f)), L.skin);
      } else {
        P.tri(X(hp(-0.4f, 0.45f)), X(hp(0.92f, 0.32f)), X(hp(0.5f, 1.12f)), L.skin);
      }
      P.circle(X(R.head), hr * s, L.skin);
      if (hero == Hero::Ganondorf) P.tri(X(hp(0.9f, -0.05f)), X(hp(1.22f, 0.36f)), X(hp(0.86f, 0.42f)), L.skin);  // nose
      if (hero != Hero::Link && hero != Hero::Byakuya)
        P.deco([&] { P.ellipse(X(hp(-0.22f, 0.12f)), 0.17f * hr * s, 0.24f * hr * s, L.skin); });
      if (hero == Hero::Gojo && !has(WeaponVisual::SixEyes))
        P.deco([&] { P.seg(X(hp(-0.98f, -0.12f)), X(hp(1.0f, -0.02f)), t(B.head * 0.42f), {30, 30, 38, 255}); });
      face();
    });
  }

  // ---- Starrk's companions -------------------------------------------------------------------------
  // A wolf of Los Lobos (pale blue spirit), running: `dir` = +1 forward (the way the fighter faces).
  void spiritWolf(Vec2 c, float sz, float dir, float phase) {
    const Color fur{150, 196, 255, 255}, pale{236, 244, 255, 255};
    const float w = sz * s;
    auto at = [&](float x, float y) { return c + Vec2{x * dir, y} * sz; };
    group([&] {
      for (int i = 0; i < 4; ++i) {  // legs, two by two out of phase
        const float ph = phase + (i % 2) * kPi + (i < 2 ? 0.0f : 0.8f);
        const float hx = i < 2 ? 0.5f : -0.5f;
        const Vec2 hip = at(hx, 0.08f), paw = at(hx + std::sin(ph) * 0.34f, 0.6f - std::max(0.0f, std::cos(ph)) * 0.16f);
        P.taper(X(hip), X(paw), w * 0.24f, w * 0.2f, w * 0.12f, 0.4f, 0.0f, fur);
      }
      P.taper(X(at(-0.72f, 0.0f)), X(at(0.62f, -0.06f)), w * 0.5f, w * 0.56f, w * 0.46f, 0.55f, 0.0f, fur);  // body
      P.taper(X(at(-0.72f, -0.06f)), X(at(-1.42f, -0.34f + 0.1f * std::sin(phase * 0.5f))), w * 0.2f, w * 0.26f, w * 0.08f,
              0.4f, 0.0f, fur);  // tail
      P.circle(X(at(0.86f, -0.3f)), w * 0.3f, fur);
      P.tri(X(at(1.02f, -0.42f)), X(at(1.02f, -0.18f)), X(at(1.5f, -0.24f)), fur);    // muzzle
      P.tri(X(at(0.66f, -0.5f)), X(at(0.86f, -0.58f)), X(at(0.7f, -0.9f)), fur);      // ears
      P.tri(X(at(0.9f, -0.56f)), X(at(1.04f, -0.5f)), X(at(0.98f, -0.86f)), fur);
      P.deco([&] { P.circle(X(at(0.98f, -0.34f)), w * 0.06f, pale); });
    });
  }
  // The pack running round him (power-up): the wolves behind him first, the ones in front last.
  void wolfPack(bool front) {
    if (!has(WeaponVisual::LobosPack) || P.ghost || bust) return;
    for (int i = 0; i < 3; ++i) {
      const float a = time * 2.2f + i * 2.0944f, depth = std::sin(a);
      if ((depth > 0.0f) != front) continue;
      const float sz = 0.14f * H * (0.86f + 0.14f * depth);
      spiritWolf({std::cos(a) * 0.44f * H, 0.38f * H + depth * 0.04f * H}, sz, -std::sin(a) >= 0.0f ? 1.0f : -1.0f,
                 time * 14.0f + i * 2.0f);
    }
  }
  // Lilynette floating at his side: a small girl, short blonde hair, the broken mask with its horn over one eye.
  void lilynette() {
    if (!has(WeaponVisual::Lilynette) || P.ghost || bust) return;
    const float k = 0.42f, u = H * k, w = u * s;
    const Vec2 c = R.neck + Vec2{-0.36f * H, 0.02f * H + std::sin(time * 2.4f) * 0.03f * H};
    auto at = [&](float x, float y) { return c + Vec2{x, y} * u; };
    const Color skin = L.skin, hair{238, 212, 112, 255}, cloth{242, 240, 234, 255}, bone{246, 244, 236, 255};
    const float sw = std::sin(time * 3.0f);
    group([&] {
      P.taper(X(at(-0.05f, 0.16f)), X(at(-0.12f + 0.04f * sw, 0.5f)), w * 0.11f, w * 0.12f, w * 0.08f, 0.4f, 0.0f, skin);
      P.taper(X(at(0.05f, 0.16f)), X(at(0.1f - 0.04f * sw, 0.48f)), w * 0.11f, w * 0.12f, w * 0.08f, 0.4f, 0.0f, skin);
      P.taper(X(at(0.0f, -0.12f)), X(at(0.0f, 0.22f)), w * 0.22f, w * 0.24f, w * 0.28f, 0.5f, 0.0f, cloth);
      P.taper(X(at(-0.07f, -0.08f)), X(at(-0.18f, 0.1f)), w * 0.08f, w * 0.08f, w * 0.07f, 0.5f, 0.0f, cloth);
      P.taper(X(at(0.07f, -0.08f)), X(at(0.2f, 0.06f + 0.03f * sw)), w * 0.08f, w * 0.08f, w * 0.07f, 0.5f, 0.0f, cloth);
      P.circle(X(at(0.0f, -0.31f)), w * 0.19f, hair);
      P.ellipse(X(at(0.08f, -0.26f)), w * 0.11f, w * 0.11f, skin);
      P.tri(X(at(-0.02f, -0.44f)), X(at(0.2f, -0.4f)), X(at(0.13f, -0.27f)), hair);  // the fringe
    });
    group([&] {  // the broken mask on the top of her head, its horn
      P.quad(X(at(-0.12f, -0.44f)), X(at(0.0f, -0.48f)), X(at(0.02f, -0.37f)), X(at(-0.12f, -0.34f)), bone);
      P.tri(X(at(-0.1f, -0.44f)), X(at(-0.01f, -0.47f)), X(at(-0.1f, -0.66f)), bone);
      P.deco([&] { P.circle(X(at(0.12f, -0.28f)), w * 0.03f, {255, 120, 180, 255}); });
    });
  }

  // ---- back cloth (capes, robes, scarves) ------------------------------------------------------
  void backCloth() {
    switch (hero) {
      case Hero::Asta: {  // short hooded Black Bulls cloak over the back shoulder
        Vec2 a[3], b[3];
        chain(R.neck + rl({-0.06f, 0.0f}), {-0.55f, 1.0f}, 0.34f * H, 2, pose, time, 0.6f, 1.0f, 0.06f, a);
        chain(R.neck + rl({0.0f, 0.04f}), {-0.2f, 1.0f}, 0.27f * H, 2, pose, time + 0.4f, 0.6f, 0.9f, 0.06f, b);
        group([&] {
          panel(a, b, 2, L.cloth);
          P.circle(X(R.neck + rl({-0.05f, -0.01f})), t(0.065f), L.cloth);  // hood
          P.deco([&] {
            Vec2 e = lerpv(a[1], b[1], 0.5f) + (a[0] - a[1]) * 0.15f;
            P.circle(X(e), t(0.03f), L.trim);
            P.tri(X(e + Vec2{-0.02f * H, -0.01f * H}), X(e + Vec2{-0.005f * H, -0.02f * H}), X(e + Vec2{-0.04f * H, -0.05f * H}), L.trim);
            P.tri(X(e + Vec2{0.02f * H, -0.01f * H}), X(e + Vec2{0.005f * H, -0.02f * H}), X(e + Vec2{0.035f * H, -0.05f * H}), L.trim);
          });
        });
        break;
      }
      case Hero::Yuno: {  // long Golden Dawn robe, gold trims
        Vec2 a[4], b[4];
        chain(R.neck + rl({-0.05f, 0.01f}), {-0.4f, 1.0f}, 0.6f * H, 3, pose, time, 0.7f, 1.1f, 0.05f, a);
        chain(R.neck + rl({0.03f, 0.04f}), {-0.08f, 1.0f}, 0.52f * H, 3, pose, time + 0.5f, 0.7f, 1.0f, 0.05f, b);
        group([&] {
          panel(a, b, 3, L.cloth);
          P.circle(X(R.neck + rl({-0.045f, 0.0f})), t(0.06f), L.cloth);  // hood
          if (P.details()) {
            for (int i = 0; i < 3; ++i) DrawLineEx(X(a[i]), X(a[i + 1]), t(0.018f), P.fill(L.trim));
            DrawLineEx(X(a[3]), X(b[3]), t(0.02f), P.fill(L.trim));
          }
        });
        break;
      }
      case Hero::Byakuya: {  // haori back panel to the calves + the long silver scarf
        Vec2 a[4], b[4], q[4];
        chain(R.neck + rl({-0.06f, 0.02f}), {-0.3f, 1.0f}, 0.66f * H, 3, pose, time, 0.8f, 1.0f, 0.03f, a);
        chain(R.neck + rl({0.02f, 0.05f}), {-0.02f, 1.0f}, 0.6f * H, 3, pose, time + 0.5f, 0.8f, 0.9f, 0.03f, b);
        chain(R.neck + rl({-0.04f, -0.02f}), {-0.8f, 1.0f}, 0.5f * H, 3, pose, time + 0.2f, 0.5f, 1.2f, 0.14f, q);
        group([&] {
          panel(a, b, 3, L.cloth);
          if (P.details()) DrawLineEx(X(a[3]), X(b[3]), t(0.02f), P.fill(shade(L.cloth, 0.75f)));
        });
        group([&] { ribbon(q, 3, 0.075f, 0.05f, L.trim); });  // wide and soft: cloth, not a blade
        break;
      }
      case Hero::Ganondorf: {  // big dark red cape
        Vec2 a[4], b[4];
        chain(R.neck + rl({-0.09f, 0.0f}), {-0.6f, 1.0f}, 0.68f * H, 3, pose, time, 0.85f, 1.0f, 0.03f, a);
        chain(R.neck + rl({0.03f, 0.03f}), {-0.1f, 1.0f}, 0.6f * H, 3, pose, time + 0.5f, 0.85f, 0.9f, 0.03f, b);
        group([&] {
          panel(a, b, 3, L.cloth);
          if (P.details()) {
            DrawLineEx(X(lerpv(a[1], b[1], 0.4f)), X(lerpv(a[3], b[3], 0.35f)), t(0.012f), P.fill(shadowTone(L.cloth)));
            DrawLineEx(X(a[3]), X(b[3]), t(0.016f), P.fill(L.trim));
          }
        });
        break;
      }
      case Hero::Kenpachi: {  // the captain's haori, sleeveless and torn to shreds at the hem
        Vec2 a[4], b[4];
        chain(R.neck + rl({-0.07f, 0.0f}), {-0.4f, 1.0f}, 0.62f * H, 3, pose, time, 0.8f, 1.0f, 0.04f, a);
        chain(R.neck + rl({0.03f, 0.04f}), {-0.08f, 1.0f}, 0.55f * H, 3, pose, time + 0.5f, 0.8f, 0.9f, 0.04f, b);
        group([&] {
          panel(a, b, 3, L.cloth);
          for (int i = 0; i < 4; ++i) {  // rags
            const Vec2 h0 = lerpv(a[3], b[3], i / 4.0f), h1 = lerpv(a[3], b[3], (i + 1) / 4.0f);
            P.tri(X(h0), X(h1), X(lerpv(h0, h1, 0.5f) + down() * ((0.05f + 0.03f * (i % 2)) * H)), L.cloth, false);
          }
          if (P.details()) DrawLineEx(X(a[0]), X(a[3]), t(0.01f), P.fill(shade(L.cloth, 0.8f)));
        });
        break;
      }
      case Hero::Ulquiorra: {  // the long tails of the Arrancar coat
        Vec2 a[4], b[4];
        chain(R.hips + rl({-0.06f, -0.04f}), {-0.3f, 1.0f}, 0.4f * H, 3, pose, time, 0.8f, 1.0f, 0.04f, a);
        chain(R.hips + rl({0.02f, -0.03f}), {-0.05f, 1.0f}, 0.36f * H, 3, pose, time + 0.5f, 0.8f, 0.9f, 0.04f, b);
        group([&] { panel(a, b, 3, L.cloth); });
        break;
      }
      case Hero::Starrk: {
        wolfPack(false);
        lilynette();
        if (pose.form == Form::LosLobos) {  // the long coat lined with fur, down to the calves
          Vec2 a[4], b[4];
          chain(R.neck + rl({-0.08f, 0.0f}), {-0.4f, 1.0f}, 0.66f * H, 3, pose, time, 0.8f, 1.1f, 0.04f, a);
          chain(R.neck + rl({0.03f, 0.04f}), {-0.08f, 1.0f}, 0.6f * H, 3, pose, time + 0.5f, 0.8f, 1.0f, 0.04f, b);
          group([&] {
            panel(a, b, 3, L.cloth);
            for (int i = 0; i <= 4; ++i) P.circle(X(lerpv(a[3], b[3], i / 4.0f)), t(0.03f), L.trim);  // fur hem
          });
        } else {  // the tails of the Arrancar coat
          Vec2 a[4], b[4];
          chain(R.hips + rl({-0.06f, -0.04f}), {-0.3f, 1.0f}, 0.36f * H, 3, pose, time, 0.8f, 1.0f, 0.04f, a);
          chain(R.hips + rl({0.02f, -0.03f}), {-0.05f, 1.0f}, 0.32f * H, 3, pose, time + 0.5f, 0.8f, 0.9f, 0.04f, b);
          group([&] { panel(a, b, 3, L.cloth); });
        }
        break;
      }
      case Hero::Grimmjow:
        if (pose.form == Form::Pantera) {  // the long tail of the panther
          Vec2 q[5];
          chain(R.hips + rl({-0.07f, 0.02f}), {-1.0f, 0.2f}, 0.62f * H, 4, pose, time, 0.3f, 1.3f, 0.12f, q);
          group([&] {
            ribbon(q, 4, 0.04f, 0.022f, L.cloth);
            P.deco([&] { P.circle(X(q[4]), t(0.022f), L.hair); });
          });
        }
        break;
      case Hero::Sasuke: {  // the big purple rope, knotted in a bow at the back
        Vec2 q[3], r[3];
        const Vec2 k0 = R.hips + rl({-0.08f, -0.03f});
        chain(k0, {-0.6f, 1.0f}, 0.2f * H, 2, pose, time, 0.6f, 1.0f, 0.1f, q);
        chain(k0, {-0.2f, 1.0f}, 0.24f * H, 2, pose, time + 0.6f, 0.6f, 0.9f, 0.1f, r);
        group([&] {
          ribbon(q, 2, 0.04f, 0.03f, L.belt);
          ribbon(r, 2, 0.04f, 0.03f, L.belt);
          P.ellipse(X(k0 + rl({-0.04f, -0.01f})), t(0.05f), t(0.035f), L.belt);
          P.ellipse(X(k0 + rl({0.0f, -0.05f})), t(0.04f), t(0.03f), L.belt);
        });
        break;
      }
      case Hero::Naruto: {  // the toad sage scroll strapped across the back
        if (!has(WeaponVisual::SageScroll)) break;
        const Vec2 c = R.chest + rl({-0.1f, 0.0f});
        group([&] {
          P.seg(X(c + rl({-0.1f, -0.14f})), X(c + rl({0.1f, 0.14f})), t(0.09f), {236, 220, 180, 255});
          P.deco([&] {
            P.circle(X(c + rl({-0.1f, -0.14f})), t(0.05f), {150, 60, 40, 255});
            P.circle(X(c + rl({0.1f, 0.14f})), t(0.05f), {150, 60, 40, 255});
          });
        });
        break;
      }
      case Hero::Sukuna: {  // black scarf ends
        Vec2 q[4], r[4];
        chain(R.neck + rl({-0.04f, 0.0f}), {-1.0f, 0.5f}, 0.36f * H, 3, pose, time, 0.4f, 1.0f, 0.16f, q);
        chain(R.neck + rl({-0.03f, 0.01f}), {-0.8f, 0.9f}, 0.3f * H, 3, pose, time + 0.9f, 0.5f, 0.9f, 0.16f, r);
        group([&] {
          ribbon(q, 3, 0.05f, 0.036f, L.cloth);
          ribbon(r, 3, 0.044f, 0.032f, L.cloth);
        });
        break;
      }
      default: break;
    }
  }

  // ---- limbs -------------------------------------------------------------------------------------
  void leg(bool front) {
    const Vec2 hip = front ? R.hipF : R.hipB, knee = front ? R.kneeF : R.kneeB, ft = front ? R.footF : R.footB;
    auto tn = [&](Color c) { return front ? c : backTone(c); };
    group([&] {
      foot(knee, ft, tn(L.foot), hero == Hero::Ganondorf || hero == Hero::Kratos ? 1.05f : 1.0f);
      legShape(hip, knee, ft, tn(L.thigh), tn(L.shin));
      const Vec2 boot0 = lerpv(knee, ft, 0.42f);
      switch (hero) {
        case Hero::Asta: case Hero::Yuno: case Hero::Gojo:
          P.deco([&] { P.seg(X(boot0), X(ft), t(B.shinTh * 1.08f), tn(L.foot)); });
          break;
        case Hero::Link:
          P.deco([&] {
            P.seg(X(lerpv(knee, ft, 0.3f)), X(ft), t(B.shinTh * 1.12f), tn(L.foot));
            P.seg(X(lerpv(knee, ft, 0.28f)), X(lerpv(knee, ft, 0.38f)), t(B.shinTh * 1.22f), tn({140, 100, 64, 255}));
          });
          break;
        case Hero::Ganondorf:
          P.deco([&] {
            P.seg(X(lerpv(knee, ft, 0.15f)), X(ft), t(B.shinTh * 1.1f), tn(shade(L.shin, 1.15f)));
            P.circle(X(knee), t(0.03f), tn(L.trim));
          });
          break;
        case Hero::Kratos:
          P.deco([&] {
            for (int i = 0; i < 3; ++i)
              P.seg(X(lerpv(knee, ft, 0.22f + i * 0.22f)), X(lerpv(knee, ft, 0.3f + i * 0.22f)), t(B.shinTh * 1.12f),
                    tn(shade(L.shin, 1.25f)));
          });
          break;
        case Hero::Ichigo: case Hero::Byakuya: case Hero::Kenpachi: case Hero::Ulquiorra: case Hero::Sasuke:
          // hakama flaring at the bottom
          P.deco([&] { P.seg(X(lerpv(knee, ft, 0.55f)), X(lerpv(knee, ft, 0.8f)), t(B.shinTh * 1.18f), tn(L.shin)); });
          break;
        case Hero::Grimmjow:
          if (pose.form == Form::Pantera) {  // armour plates and claws on the toes
            P.deco([&] {
              P.circle(X(knee), t(0.036f), tn(shade(L.shin, 1.05f)));
              P.seg(X(lerpv(knee, ft, 0.45f)), X(lerpv(knee, ft, 0.6f)), t(B.shinTh * 0.9f), tn(shade(L.shin, 0.9f)));
            });
          } else {
            P.deco([&] { P.seg(X(lerpv(knee, ft, 0.55f)), X(lerpv(knee, ft, 0.8f)), t(B.shinTh * 1.18f), tn(L.shin)); });
          }
          break;
        case Hero::Starrk:
          P.deco([&] { P.seg(X(lerpv(knee, ft, 0.55f)), X(lerpv(knee, ft, 0.8f)), t(B.shinTh * 1.18f), tn(L.shin)); });
          break;
        case Hero::Hunter:  // greaves and knee guards
          P.deco([&] {
            P.seg(X(lerpv(knee, ft, 0.12f)), X(lerpv(knee, ft, 0.85f)), t(B.shinTh * 1.16f), tn(shade(L.shin, 1.1f)));
            P.circle(X(knee), t(0.036f), tn(shade(L.shin, 1.2f)));
            P.seg(X(lerpv(hip, knee, 0.55f)), X(lerpv(hip, knee, 0.7f)), t(B.leg * 1.08f), tn(L.belt));
          });
          break;
        case Hero::Naruto: case Hero::Kakashi:  // bandaged shins, a kunai holster strapped to the thigh
          P.deco([&] {
            P.seg(X(lerpv(knee, ft, 0.62f)), X(lerpv(knee, ft, 0.88f)), t(B.shinTh * 1.1f), tn({226, 222, 214, 255}));
            if (front) {
              P.seg(X(lerpv(hip, knee, 0.5f)), X(lerpv(hip, knee, 0.56f)), t(B.leg * 1.26f), tn({226, 222, 214, 255}));
              P.seg(X(lerpv(hip, knee, 0.42f)), X(lerpv(hip, knee, 0.66f)), t(B.leg * 0.55f), tn({46, 48, 56, 255}));
            }
          });
          break;
        default: break;
      }
    });
  }

  void arm(bool front) {
    const Vec2 sh = front ? R.shF : R.shB, el = front ? R.elbowF : R.elbowB, hd = front ? R.handF : R.handB;
    auto tn = [&](Color c) { return front ? c : backTone(c); };
    group([&] {
      Color upper = L.arm, lower = front ? L.fore : L.foreB, hand = front ? L.hand : L.handB;
      if (hero == Hero::Link && !front) lower = L.arm;
      armShape(sh, el, hd, tn(upper), tn(lower));
      switch (hero) {
        case Hero::Asta:  // wrapped forearms
          P.deco([&] { P.seg(X(lerpv(el, hd, 0.35f)), X(lerpv(el, hd, 0.8f)), t(B.fore * 1.06f), tn({226, 218, 200, 255})); });
          if (P.details())
            for (int i = 0; i < 3; ++i)
              DrawLineEx(X(lerpv(el, hd, 0.42f + i * 0.13f)), X(lerpv(el, hd, 0.48f + i * 0.13f) + Vec2{0, 0.012f * H}),
                         t(0.006f) + 0.5f, P.fill({150, 140, 124, 255}));
          break;
        case Hero::Ichigo: case Hero::Byakuya: case Hero::Sukuna:
          sleeveFlap(el, hd, hero == Hero::Sukuna ? 0.11f : 0.09f, tn(upper));
          break;
        case Hero::Link:
          if (!front) P.deco([&] { P.seg(X(lerpv(el, hd, 0.55f)), X(lerpv(el, hd, 0.9f)), t(B.fore * 1.1f), tn({112, 78, 52, 255})); });
          break;
        case Hero::Ganondorf:  // gauntlets, pauldron
          P.deco([&] {
            P.seg(X(lerpv(el, hd, 0.3f)), X(lerpv(el, hd, 0.85f)), t(B.fore * 1.12f), tn(shade(L.fore, 1.2f)));
            P.seg(X(lerpv(el, hd, 0.28f)), X(lerpv(el, hd, 0.36f)), t(B.fore * 1.2f), tn(L.trim));
          });
          P.ellipse(X(sh + rl({0.0f, -0.01f})), t(0.075f), t(0.062f), tn(L.top));
          P.deco([&] { P.seg(X(sh + rl({-0.06f, 0.02f})), X(sh + rl({0.06f, 0.025f})), t(0.016f), tn(L.trim)); });
          break;
        case Hero::Kratos:  // bracers
          P.deco([&] {
            P.seg(X(lerpv(el, hd, 0.25f)), X(lerpv(el, hd, 0.82f)), t(B.fore * 1.1f), tn(L.fore));
            P.seg(X(lerpv(el, hd, 0.45f)), X(lerpv(el, hd, 0.55f)), t(B.fore * 1.16f), tn({160, 150, 140, 255}));
          });
          break;
        case Hero::Gojo:
          P.deco([&] { P.seg(X(lerpv(el, hd, 0.84f)), X(lerpv(el, hd, 0.92f)), t(B.fore * 1.15f), tn(shade(L.top, 0.8f))); });
          break;
        case Hero::Kenpachi:  // kosode sleeve rolled up over the elbow
          P.deco([&] { P.seg(X(lerpv(sh, el, 0.55f)), X(lerpv(el, hd, 0.08f)), t(B.arm * 1.12f), tn(L.arm)); });
          break;
        case Hero::Ulquiorra:  // white sleeve, black cuff
          P.deco([&] { P.seg(X(lerpv(el, hd, 0.78f)), X(lerpv(el, hd, 0.88f)), t(B.fore * 1.15f), tn(L.belt)); });
          break;
        case Hero::Hunter:  // pauldron and plated gauntlet
          P.deco([&] {
            P.seg(X(lerpv(el, hd, 0.25f)), X(lerpv(el, hd, 0.85f)), t(B.fore * 1.14f), tn(L.fore));
            P.seg(X(lerpv(el, hd, 0.25f)), X(lerpv(el, hd, 0.33f)), t(B.fore * 1.2f), tn(L.trim));
          });
          P.ellipse(X(sh + rl({0.0f, -0.012f})), t(0.07f), t(0.058f), tn(shade(L.top, 1.1f)));
          P.deco([&] { P.seg(X(sh + rl({-0.055f, 0.02f})), X(sh + rl({0.055f, 0.025f})), t(0.014f), tn(L.trim)); });
          break;
        case Hero::Naruto:  // black upper sleeve, a white cuff
          P.deco([&] { P.seg(X(lerpv(el, hd, 0.82f)), X(lerpv(el, hd, 0.9f)), t(B.fore * 1.12f), tn({236, 232, 224, 255})); });
          break;
        case Hero::Sasuke:  // arm guards
          P.deco([&] { P.seg(X(lerpv(el, hd, 0.5f)), X(lerpv(el, hd, 0.9f)), t(B.fore * 1.12f), tn(L.cloth)); });
          break;
        case Hero::Grimmjow:
          if (pose.form == Form::Pantera) {  // the blades of Garra de la Pantera along the forearm
            Vec2 d = (hd - el).norm(), n{-d.y, d.x};
            if (n.y > 0.0f) n = n * -1.0f;  // on top of the arm
            for (int i = 0; i < 3; ++i) {
              const Vec2 b0 = lerpv(el, hd, 0.25f + i * 0.2f) + n * (B.fore * 0.4f * H);
              P.tri(X(b0), X(b0 + d * (0.05f * H)), X(b0 - d * (0.05f * H) + n * (0.07f * H)), tn({236, 236, 230, 255}));
            }
          } else {  // the sleeve ends in a black cuff
            P.deco([&] { P.seg(X(lerpv(el, hd, 0.8f)), X(lerpv(el, hd, 0.9f)), t(B.fore * 1.15f), tn(L.belt)); });
          }
          break;
        case Hero::Starrk:
          if (pose.form == Form::LosLobos)  // fur cuffs
            P.deco([&] { P.seg(X(lerpv(el, hd, 0.74f)), X(lerpv(el, hd, 0.9f)), t(B.fore * 1.45f), tn(L.trim)); });
          else
            P.deco([&] { P.seg(X(lerpv(el, hd, 0.82f)), X(lerpv(el, hd, 0.9f)), t(B.fore * 1.12f), tn(L.belt)); });
          break;
        case Hero::Kakashi:  // the red swirl on the shoulder, metal plates on the gloves
          P.deco([&] { P.circle(X(sh + rl({0.0f, 0.03f})), t(0.022f), tn({200, 50, 44, 255})); });
          break;
        default: break;
      }
      fist(el, hd, tn(hand));
      if (hero == Hero::Grimmjow && pose.form == Form::Pantera) {  // claws
        const Vec2 d = (hd - el).norm(), n{-d.y, d.x};
        for (int i = -1; i <= 1; ++i) {
          const Vec2 b0 = hd + d * (B.hand * H * 1.1f) + n * (i * B.hand * H * 0.45f);
          P.tri(X(b0 - n * (0.008f * H)), X(b0 + n * (0.008f * H)), X(b0 + d * (0.045f * H) + n * (i * 0.01f * H)), tn({60, 60, 70, 255}));
        }
      }
      if (hero == Hero::Kakashi && P.details())
        disc(X(hd + rl({0.0f, -0.006f})), t(B.hand * 0.45f), P.fill(tn(L.trim)));
      if (hero == Hero::Sukuna && P.details()) {  // black bands round the wrists
        const bool hot = has(WeaponVisual::CursedFinger);
        for (int i = 0; i < 2; ++i)
          DrawLineEx(X(lerpv(el, hd, 0.8f + i * 0.08f) + Vec2{0, -0.025f * H}), X(lerpv(el, hd, 0.8f + i * 0.08f) + Vec2{0, 0.025f * H}),
                     t(0.012f), P.fill(hot ? Color{230, 40, 50, 255} : Color{26, 20, 24, 255}));
      }
      if (hero == Hero::Link && front && P.details() && !P.flat) {  // Ultrahand: glowing Zonai lines
        P.glowLine(X(lerpv(el, hd, 0.15f)), X(lerpv(el, hd, 0.55f)), 1.3f * s, 0.9f, kUltraGlow);
        P.glowLine(X(lerpv(el, hd, 0.55f) + Vec2{0, -0.012f * H}), X(lerpv(el, hd, 0.85f) + Vec2{0, 0.008f * H}), 1.3f * s, 0.9f,
                   kUltraGlow);
        P.glowDot(X(lerpv(el, hd, 0.55f)), 1.4f * s, 1.0f, kUltraGlow);
        P.glowDot(X(hd), 1.6f * s, 0.8f, kUltraGlow);
      }
    });
  }

  // ---- torso ---------------------------------------------------------------------------------------
  // Shoulders, chest, a narrower waist and the hips: one shaded volume made of two convex pieces.
  void torsoShape(Color c) {
    const Cut cu = cutFor(hero, pose.form);
    const float cw = B.chest * 0.5f * cu.chest, ww = B.waist * 0.5f * cu.waist;
    const float hw = std::max(B.waist * 0.52f * cu.hips, B.hipX + B.leg * 0.5f);
    const Vec2 waist = lerpv(R.chest, R.hips, 0.5f);
    const Vec2 up[8] = {R.neck + rl({-cw * 0.5f, -0.01f}), R.neck + rl({-cw * 0.98f, 0.045f}), R.chest + rl({-cw * 0.94f, 0.0f}),
                        waist + rl({-ww, 0.0f}),           waist + rl({ww * 1.05f, 0.0f}),      R.chest + rl({cw * 1.05f, -0.005f}),
                        R.neck + rl({cw, 0.045f}),         R.neck + rl({cw * 0.5f, -0.01f})};
    Vector2 q[8];
    for (int i = 0; i < 8; ++i) q[i] = X(up[i]);
    P.poly(q, 8, c, true);
    const Vec2 lo[4] = {waist + rl({-ww, 0.0f}), R.hips + rl({-hw, 0.045f}), R.hips + rl({hw, 0.045f}), waist + rl({ww * 1.05f, 0.0f})};
    for (int i = 0; i < 4; ++i) q[i] = X(lo[i]);
    P.poly(q, 4, c, true);
  }
  void torso() {
    group([&] {
      torsoShape(L.top);
      const Vec2 belt0 = R.hips + rl({-B.waist * 0.5f, -0.02f}), belt1 = R.hips + rl({B.waist * 0.5f, -0.02f});
      switch (hero) {
        case Hero::Asta:
          P.deco([&] { P.seg(X(belt0), X(belt1), t(0.04f), L.belt); });
          P.deco([&] { P.circle(X(R.neck + rl({0.035f, 0.035f})), t(0.022f), L.trim); });  // cloak clasp
          break;
        case Hero::Yuno:
          P.deco([&] {
            P.ellipse(X(R.neck + rl({-0.01f, 0.05f})), t(0.12f), t(0.075f), L.cloth);  // mantle
            P.seg(X(belt0), X(belt1), t(0.03f), L.trim);
            P.circle(X(R.neck + rl({0.05f, 0.04f})), t(0.022f), L.trim);
          });
          if (P.details())
            DrawLineEx(X(R.neck + rl({-0.12f, 0.1f})), X(R.neck + rl({0.1f, 0.1f})), t(0.014f), P.fill(L.trim));
          break;
        case Hero::Ichigo:
          P.deco([&] {
            P.seg(X(R.neck + rl({-0.03f, 0.01f})), X(R.neck + rl({0.06f, 0.035f})), t(0.03f), L.cloth);  // white collar
            P.seg(X(R.neck + rl({0.06f, 0.035f})), X(R.chest + rl({0.08f, 0.04f})), t(0.03f), L.cloth);
            P.seg(X(R.shB + rl({-0.02f, -0.01f})), X(R.hips + rl({0.08f, -0.05f})), t(0.03f), L.accent);  // red strap
            P.seg(X(belt0), X(belt1), t(0.04f), L.cloth);  // obi
          });
          break;
        case Hero::Byakuya:
          P.deco([&] {
            P.tri(X(R.neck + rl({0.0f, 0.02f})), X(R.neck + rl({0.09f, 0.03f})), X(R.chest + rl({0.07f, 0.06f})), L.arm);  // kimono V
            P.seg(X(R.neck + rl({0.04f, 0.025f})), X(R.chest + rl({0.07f, 0.03f})), t(0.016f), {240, 240, 244, 255});
            P.seg(X(belt0), X(belt1), t(0.035f), L.arm);
          });
          break;
        case Hero::Link:
          P.deco([&] {
            P.seg(X(R.shF + rl({0.01f, -0.01f})), X(R.hips + rl({-0.07f, -0.04f})), t(0.024f), L.belt);  // strap
            P.seg(X(belt0), X(belt1), t(0.042f), L.belt);
            P.circle(X(R.hips + rl({0.05f, -0.02f})), t(0.02f), L.trim);
          });
          break;
        case Hero::Ganondorf:
          P.deco([&] {
            P.ellipse(X(R.chest + rl({0.04f, -0.01f})), t(0.12f), t(0.1f), shade(L.top, 1.18f));  // chest plate
            P.seg(X(belt0), X(belt1), t(0.05f), L.trim);
            P.circle(X(R.hips + rl({0.06f, -0.02f})), t(0.028f), {200, 40, 56, 255});
          });
          if (P.details()) {
            DrawLineEx(X(R.chest + rl({-0.06f, -0.09f})), X(R.chest + rl({0.13f, -0.06f})), t(0.012f), P.fill(L.trim));
            DrawLineEx(X(R.chest + rl({0.13f, -0.06f})), X(R.chest + rl({0.12f, 0.06f})), t(0.012f), P.fill(L.trim));
          }
          break;
        case Hero::Kratos:
          if (P.details()) {  // muscles, the red stripe down the back, a leather strap
            const Color m = P.fill(shadowTone(L.top));
            DrawLineEx(X(R.chest + rl({0.02f, 0.03f})), X(R.chest + rl({0.14f, 0.05f})), t(0.01f), m);
            DrawLineEx(X(R.chest + rl({0.06f, 0.08f})), X(R.hips + rl({0.08f, -0.08f})), t(0.008f), m);
            DrawLineEx(X(R.neck + rl({-0.1f, 0.06f})), X(R.hips + rl({-0.08f, -0.04f})), t(0.04f), P.fill(L.accent));
          }
          P.deco([&] {
            P.seg(X(R.shB + rl({0.0f, -0.01f})), X(R.hips + rl({0.1f, -0.06f})), t(0.03f), L.belt);
            P.seg(X(belt0), X(belt1), t(0.05f), L.belt);
            P.circle(X(R.hips + rl({0.07f, -0.02f})), t(0.024f), L.trim);
          });
          break;
        case Hero::Gojo:
          P.deco([&] { P.seg(X(R.neck + rl({0.0f, -0.01f})), X(lerpv(R.neck, R.head, 0.55f)), t(B.chest * 0.5f), L.top); });  // collar
          if (P.details()) {
            DrawLineEx(X(R.neck + rl({0.07f, 0.04f})), X(R.hips + rl({0.06f, -0.02f})), t(0.008f), P.fill(shade(L.top, 0.7f)));
            disc(X(R.neck + rl({0.06f, 0.02f})), t(0.012f), P.fill({200, 180, 120, 255}));
          }
          break;
        case Hero::Kenpachi:  // open kosode, the white haori hanging open over it, obi
          P.deco([&] {
            P.tri(X(R.neck + rl({-0.02f, 0.02f})), X(R.neck + rl({0.13f, 0.03f})), X(R.chest + rl({0.06f, 0.1f})), L.skin);
            P.seg(X(belt0), X(belt1), t(0.04f), L.belt);
            P.seg(X(R.shB + rl({0.0f, -0.01f})), X(R.hips + rl({-0.1f, 0.06f})), t(0.06f), L.cloth);
            P.seg(X(R.shF + rl({0.0f, -0.01f})), X(R.chest + rl({0.13f, 0.02f})), t(0.045f), L.cloth);
          });
          if (P.details()) DrawLineEx(X(R.chest + rl({0.04f, 0.02f})), X(R.chest + rl({0.08f, 0.08f})), t(0.008f), P.fill(shadowTone(L.skin)));
          break;
        case Hero::Ulquiorra:  // high collar, the hollow hole at the throat, black sash
          P.deco([&] {
            P.seg(X(R.neck + rl({-0.02f, 0.0f})), X(lerpv(R.neck, R.head, 0.5f)), t(B.chest * 0.55f), L.top);
            P.seg(X(belt0), X(belt1), t(0.04f), L.belt);
          });
          if (P.details()) {
            disc(X(R.neck + rl({0.03f, 0.06f})), t(0.024f), P.fill({14, 12, 16, 255}));
            DrawLineEx(X(R.neck + rl({0.06f, 0.04f})), X(R.hips + rl({0.06f, -0.04f})), t(0.008f), P.fill(shade(L.top, 0.75f)));
          }
          break;
        case Hero::Hunter:  // chest plate, belt and pouches
          P.deco([&] {
            P.ellipse(X(R.chest + rl({0.03f, -0.01f})), t(0.1f), t(0.085f), shade(L.top, 1.15f));
            P.seg(X(belt0), X(belt1), t(0.045f), L.belt);
            P.circle(X(R.hips + rl({0.07f, -0.01f})), t(0.024f), shade(L.belt, 0.8f));
            P.circle(X(R.hips + rl({-0.02f, 0.0f})), t(0.02f), shade(L.belt, 0.8f));
          });
          if (P.details())
            DrawLineEx(X(R.chest + rl({-0.05f, -0.08f})), X(R.chest + rl({0.12f, -0.05f})), t(0.012f), P.fill(L.trim));
          break;
        case Hero::Naruto:  // black yoke and collar, zipper
          P.deco([&] {
            P.ellipse(X(R.neck + rl({0.0f, 0.04f})), t(0.11f), t(0.06f), L.cloth);
            P.seg(X(R.neck + rl({0.0f, -0.01f})), X(lerpv(R.neck, R.head, 0.35f)), t(B.chest * 0.45f), L.cloth);
          });
          if (P.details())
            DrawLineEx(X(R.neck + rl({0.07f, 0.06f})), X(R.hips + rl({0.07f, -0.02f})), t(0.01f), P.fill({240, 238, 232, 255}));
          break;
        case Hero::Sasuke:  // wide open collar, the purple rope round the waist
          P.deco([&] {
            P.tri(X(R.neck + rl({-0.02f, 0.02f})), X(R.neck + rl({0.11f, 0.03f})), X(R.chest + rl({0.07f, 0.09f})), L.skin);
            P.seg(X(R.neck + rl({-0.05f, -0.01f})), X(lerpv(R.neck, R.head, 0.3f)), t(B.chest * 0.4f), L.top);
            P.seg(X(belt0), X(belt1), t(0.06f), L.belt);
          });
          break;
        case Hero::Kakashi:  // flak jacket: scroll pockets, high collar
          P.deco([&] {
            P.seg(X(R.neck + rl({0.0f, -0.01f})), X(lerpv(R.neck, R.head, 0.45f)), t(B.chest * 0.5f), L.cloth);
            for (int i = 0; i < 3; ++i)
              P.seg(X(R.chest + rl({0.0f + i * 0.04f, 0.02f})), X(R.chest + rl({0.0f + i * 0.04f, 0.1f})), t(0.03f),
                    shade(L.top, 1.12f));
            P.seg(X(belt0), X(belt1), t(0.03f), L.belt);
          });
          break;
        case Hero::Sukuna:
          P.deco([&] {
            P.tri(X(R.neck + rl({0.02f, 0.02f})), X(R.neck + rl({0.1f, 0.03f})), X(R.chest + rl({0.07f, 0.04f})), L.skin);  // V
            P.seg(X(belt0), X(belt1), t(0.05f), L.cloth);  // sash
            P.seg(X(R.neck + rl({-0.06f, 0.0f})), X(R.neck + rl({0.06f, 0.02f})), t(0.05f), L.cloth);  // scarf
          });
          break;
        case Hero::Grimmjow:
          P.deco([&] { P.seg(X(belt0), X(belt1), t(0.05f), pose.form == Form::Pantera ? shade(L.top, 0.85f) : L.belt); });
          if (P.details()) {  // chest and abs (bare under the open jacket, or the plates of the armour), the hollow hole
            const Color m = P.fill(shadowTone(L.top));
            DrawLineEx(X(R.chest + rl({0.0f, -0.01f})), X(R.chest + rl({0.12f, 0.0f})), t(0.01f), m);
            for (int i = 0; i < 2; ++i)
              DrawLineEx(X(R.chest + rl({0.03f, 0.05f + i * 0.04f})), X(R.chest + rl({0.1f, 0.05f + i * 0.04f})), t(0.008f), m);
            fillEllipse(X(lerpv(R.chest, R.hips, 0.6f) + rl({0.05f, 0.0f})), t(0.026f), t(0.03f), P.fill({18, 14, 18, 255}));
          }
          break;
        case Hero::Starrk:
          P.deco([&] { P.seg(X(belt0), X(belt1), t(0.045f), L.belt); });
          if (P.details())  // the edge of the jacket
            DrawLineEx(X(R.neck + rl({0.07f, 0.04f})), X(R.hips + rl({0.07f, -0.02f})), t(0.009f), P.fill(shade(L.top, 0.72f)));
          break;
        default: break;
      }
    });
    if (hero == Hero::Grimmjow && pose.form != Form::Pantera) {  // the white jacket hanging open, collar turned up
      const Cut cu = cutFor(hero, pose.form);
      const float cw = B.chest * 0.5f * cu.chest, ww = B.waist * 0.5f * cu.waist;
      const Vec2 waist = lerpv(R.chest, R.hips, 0.5f);
      group([&] {
        P.quad(X(R.neck + rl({-0.02f, -0.01f})), X(R.neck + rl({-cw, 0.045f})), X(waist + rl({-ww * 1.04f, 0.03f})),
               X(waist + rl({-0.03f, 0.03f})), L.cloth, true);
        P.quad(X(R.neck + rl({cw * 0.55f, 0.01f})), X(R.neck + rl({cw * 1.02f, 0.045f})), X(waist + rl({ww * 1.08f, 0.03f})),
               X(waist + rl({ww * 0.82f, 0.03f})), L.cloth, true);
        P.seg(X(R.neck + rl({-0.05f, 0.0f})), X(lerpv(R.neck, R.head, 0.42f) + rl({-0.02f, 0.0f})), t(0.055f), L.cloth);
      });
    }
    if (hero == Hero::Starrk) {
      group([&] {  // the high collar folded down, the jaw of his old mask worn as a necklace
        P.seg(X(R.neck + rl({-0.05f, 0.0f})), X(lerpv(R.neck, R.head, 0.38f) + rl({-0.02f, 0.0f})), t(0.06f), L.top);
        if (pose.form == Form::LosLobos)
          for (int i = 0; i < 5; ++i)  // a thick ruff of fur
            P.circle(X(R.neck + rl({-0.1f + i * 0.05f, 0.02f + 0.012f * (i % 2)})), t(0.04f), L.trim);
      });
      group([&] {
        const Vec2 j0 = R.neck + rl({-0.02f, 0.05f}), j1 = R.neck + rl({0.1f, 0.06f});
        P.quad(X(j0 + rl({0.0f, -0.012f})), X(j1 + rl({0.0f, -0.012f})), X(j1 + rl({-0.01f, 0.03f})), X(j0 + rl({0.01f, 0.025f})),
               {246, 244, 236, 255});
        if (P.details())
          for (int i = 0; i < 4; ++i) {
            const Vec2 a = lerpv(j0, j1, (i + 0.5f) / 4.0f);
            DrawLineEx(X(a + rl({0.0f, -0.006f})), X(a + rl({0.0f, 0.016f})), t(0.006f) + 0.5f, P.fill({40, 34, 34, 255}));
          }
      });
      if (pose.form == Form::LosLobos)
        group([&] {  // the bandolier across his chest
          const Vec2 a = R.shF + rl({0.0f, -0.01f}), b = R.hips + rl({-0.09f, -0.02f});
          P.seg(X(a), X(b), t(0.03f), L.belt);
          P.deco([&] {
            for (int i = 1; i < 5; ++i) P.circle(X(lerpv(a, b, i / 5.0f)), t(0.014f), {226, 196, 110, 255});
          });
        });
    }
  }

  // Garments over the hips, after the legs.
  void skirt() {
    switch (hero) {
      case Hero::Link:
        group([&] {
          skirt(B.waist * 0.52f, 0.17f, 0.02f, L.top);
          if (P.details()) hemLine(B.waist * 0.52f, 0.17f, 0.02f, mixColor(L.top, WHITE, 0.35f));
        });
        break;
      case Hero::Kratos:
        group([&] {
          skirt(B.waist * 0.55f, 0.18f, 0.03f, L.cloth);
          if (P.details()) {
            hemLine(B.waist * 0.55f, 0.18f, 0.03f, L.trim);
            DrawLineEx(X(R.hips + rl({0.02f, 0.0f})), X(lerpv(R.hips, R.kneeF, 0.75f)), t(0.008f), P.fill(shadowTone(L.cloth)));
          }
        });
        break;
      case Hero::Gojo:
        group([&] { skirt(B.waist * 0.55f, 0.13f, 0.03f, L.top); });
        break;
      case Hero::Sukuna:
        group([&] {
          skirt(B.waist * 0.55f, 0.27f, 0.05f, L.top);
          P.deco([&] {
            Vec2 q[3];
            chain(R.hips + rl({0.05f, -0.02f}), {0.15f, 1.0f}, 0.16f * H, 2, pose, time, 0.4f, 0.8f, 0.1f, q);
            P.seg(X(q[0]), X(q[1]), t(0.03f), L.cloth);
            P.seg(X(q[1]), X(q[2]), t(0.026f), L.cloth);
          });
        });
        break;
      case Hero::Byakuya:
        group([&] { skirt(B.waist * 0.6f, 0.26f, 0.04f, L.cloth, 0.02f); });
        break;
      case Hero::Ganondorf:
        group([&] {
          skirt(B.waist * 0.55f, 0.17f, 0.04f, shade(L.top, 1.1f));
          if (P.details()) hemLine(B.waist * 0.55f, 0.17f, 0.04f, L.trim);
        });
        break;
      case Hero::Yuno:
        group([&] { skirt(B.waist * 0.5f, 0.1f, 0.02f, L.top); });
        break;
      case Hero::Hunter:  // tassets
        group([&] {
          skirt(B.waist * 0.55f, 0.15f, 0.03f, shade(L.top, 0.9f));
          if (P.details()) hemLine(B.waist * 0.55f, 0.15f, 0.03f, L.trim);
        });
        break;
      case Hero::Sasuke:  // waist cloth
        group([&] { skirt(B.waist * 0.5f, 0.2f, 0.03f, L.cloth); });
        break;
      case Hero::Ulquiorra:  // front of the coat
        group([&] { skirt(B.waist * 0.5f, 0.14f, 0.02f, L.top); });
        break;
      case Hero::Starrk:  // the front of his jacket
        group([&] { skirt(B.waist * 0.55f, 0.13f, 0.03f, L.top); });
        break;
      default: break;
    }
  }

  // ---- weapons and energy -------------------------------------------------------------------
  WeaponStyle wstyle() const {
    WeaponStyle st;
    st.side = X.facing;
    st.ghost = P.ghost;
    st.tint = P.flashCol;
    if (art::rigged(hero)) st.slim = 0.74f;  // a realistic build: the blades thinner, the length (the hitbox) kept
    return st;
  }
  void frontWeaponDraw() {
    const WeaponVisual v = frontWeapon(d, pose);
    if (v == WeaponVisual::None && pose.form == Form::LosLobos) {  // released: a pistol in each hand, aimed along the arm
      const float sc = s * H / 100.0f;
      WeaponStyle st = wstyle();
      st.single = true;
      for (int h = 0; h < 2; ++h) {
        const Vec2 el = h ? R.elbowB : R.elbowF, hd = h ? R.handB : R.handF;
        const Vec2 wd = (W(X(hd + (hd - el).norm() * 10.0f)) - W(X(hd))).norm();
        drawWeaponStyled(WeaponVisual::LobosPistol, W(X(hd)), wd, sc, P.alpha, time, st);
      }
      return;
    }
    if (v == WeaponVisual::None) return;
    const float len = frontWeaponLength(d, pose);
    const float sc = len / std::max(1.0f, weaponVisualLength(v)) * s;
    Vec2 dir = pose.weaponDir.norm();
    Vec2 wd = W(X(R.handF + dir * 10.0f)) - W(X(R.handF));
    WeaponStyle st = wstyle();
    st.single = true;
    if (v == pose.held) {
      st.glow = 1.0f;
      st.fx = pose.heldFx;
    }
    drawWeaponStyled(v, W(X(R.handF)), wd.norm(), sc, P.alpha, time, st);
    if (v == WeaponVisual::BladesOfChaos) {  // the second blade in the other hand
      Vec2 wb = W(X(R.handB + rotate(dir, 0.5f) * 10.0f)) - W(X(R.handB));
      drawWeaponStyled(v, W(X(R.handB)), wb.norm(), sc, P.alpha, time, st);
    }
  }
  // Shield / bow carried on the back arm (drawn over the torso when the arm is held forward).
  void backArmGear() {
    const Vec2 hb = R.handB, eb = R.elbowB;
    WeaponStyle st = wstyle();
    if (hero == Hero::Sukuna && pose.bow >= 0.0f) {  // Fuga: no bow, an arrow of fire held between his hands
      const Vec2 a = W(X(R.handF)), b = W(X(R.handB)), d = b - a;
      const float len = d.len(), sc = s * H / 100.0f;  // the arrow: about as long as he is tall
      if (len > 1.0f)
        art::drawFx("fuga_arrow", time, (a + b) * 0.5f + Vec2{(d.x >= 0.0f ? 30.0f : -30.0f) * sc, 0.0f}, 80.0f * sc / 110.0f,
                    d.x >= 0.0f ? 0.0f : kPi, false,
                    P.alpha * (0.4f + 0.6f * pose.bow));
      return;
    }
    if (pose.bow >= 0.0f || has(WeaponVisual::BowOfLight) || has(WeaponVisual::SusanooBow)) {
      Vec2 aim = pose.bow >= 0.0f ? (R.handB - R.handF).norm() : Vec2{1.0f, 0.15f}.norm();
      if (aim.len2() < 0.5f) aim = {1, 0};
      Vec2 wd = (W(X(hb + aim * 10.0f)) - W(X(hb))).norm();
      const float sc = s * H / 100.0f;
      st.pull = std::max(0.0f, pose.bow);
      if (has(WeaponVisual::BowOfLight)) {
        st.glow = 0.6f;
        st.fx = HitFx::Light;
      } else if (has(WeaponVisual::SusanooBow)) {
        st.variant = 3;
      } else {
        st.variant = hero == Hero::Sukuna ? 2 : 1;
      }
      drawWeaponStyled(WeaponVisual::BowOfLight, W(X(hb)) - wd * (20.0f * sc), wd, sc, P.alpha, time, st);
      return;
    }
    if (d.baseWeapon == WeaponVisual::ShieldAndSword) {
      Vec2 c = lerpv(eb, hb, 0.62f);
      drawShieldVisual(false, W(X(c)), 0.15f * H * s, 1.0f, {1, 0}, P.alpha, time, st);
    } else if (d.baseWeapon == WeaponVisual::GuardianShield) {
      Vec2 c = lerpv(eb, hb, 0.55f);
      Vec2 fold = W(X(eb)) - W(X(hb));
      drawShieldVisual(true, W(X(c)), 0.17f * H * s, pose.shieldOpen, fold, P.alpha, time, st);
    }
  }
  bool gearInFront() const {
    if (pose.bow >= 0.0f) return true;
    if (d.baseWeapon == WeaponVisual::GuardianShield) return pose.shieldOpen > 0.3f;
    return R.handB.x > 0.06f * H;
  }

  void energy() {
    if (P.ghost || P.flat) return;
    const float k = P.alpha;
    // Ganondorf: red-black malice curling up from the hands
    if (hero == Hero::Ganondorf) {
      for (int h = 0; h < 2; ++h) {
        const Vec2 c = h ? R.handB : R.handF;
        for (int i = 0; i < 2; ++i) {
          Vec2 prev = c;
          const float ph = time * 4.0f + i * 2.4f + h;
          for (int j = 1; j <= 5; ++j) {
            float u = j / 5.0f;
            Vec2 q = c + Vec2{std::sin(ph + u * 5.0f) * 0.03f * H * u + (i ? 0.015f : -0.015f) * H, -0.11f * H * u};
            DrawLineEx(X(prev), X(q), t(0.02f * (1.0f - u * 0.7f)), withAlpha({110, 8, 30, 255}, 0.4f * k * (1.0f - u)));
            prev = q;
          }
        }
      }
    }
    // Yuno: wind wisps circling the legs
    if (hero == Hero::Yuno) {
      for (int i = 0; i < 2; ++i) {
        const float ph = time * (2.4f + i * 0.7f) + i * 2.0f;
        const Vec2 c = R.hips + Vec2{0.0f, (0.18f + i * 0.12f) * H};
        Vec2 prev{};
        for (int j = 0; j <= 7; ++j) {
          float a = ph + j * 0.32f;
          Vec2 q = c + Vec2{std::cos(a) * 0.22f * H, std::sin(a) * 0.05f * H};
          if (j > 0) DrawLineEx(X(prev), X(q), t(0.008f) + 0.5f, withAlpha({200, 255, 220, 255}, 0.55f * k * (j / 7.0f)));
          prev = q;
        }
      }
    }
    // Spartan Rage: fists of fire
    if (pose.form == Form::SpartanRage) {
      for (int h = 0; h < 2; ++h) {
        const Vec2 c = h ? R.handB : R.handF;
        for (int i = 0; i < 3; ++i) {
          const float ph = std::fmod(time * 3.0f + i * 0.33f + h * 0.5f, 1.0f);
          disc(X(c + Vec2{std::sin(time * 9.0f + i) * 0.02f * H, -ph * 0.12f * H}), t(0.04f * (1.0f - ph)) + 0.5f,
               withAlpha(i % 2 ? Color{255, 200, 80, 255} : Color{255, 90, 20, 255}, 0.75f * k * (1.0f - ph)));
        }
      }
    }
    // Hands charged with energy (casts, signs, a raised hand)
    if (pose.charge > 0.01f) {
      const Color c = hitFxColor(pose.chargeFx), g = hitFxGlow(pose.chargeFx);
      const Vec2 at = pose.charge > 0.0f && (R.handF - R.handB).len() < 0.12f * H ? lerpv(R.handF, R.handB, 0.5f) : R.handF;
      const float r = (0.05f + 0.07f * pose.charge) * H * s * (1.0f + 0.08f * std::sin(time * 20.0f));
      disc(X(at), r * 2.0f, withAlpha(c, 0.18f * k * pose.charge));
      disc(X(at), r, withAlpha(g, 0.55f * k * pose.charge));
      disc(X(at), r * 0.45f, withAlpha(WHITE, 0.8f * k * pose.charge));
    }
    // Gojo: Hollow Purple above the front hand
    if (has(WeaponVisual::HollowPurple)) {
      const float sc = s * H / 100.0f * 0.8f;
      Vec2 c = W(X(R.handF + Vec2{0.0f, -0.12f * H}));
      drawWeaponStyled(WeaponVisual::HollowPurple, c - Vec2{15.0f * sc, 0}, {1, 0}, sc, k, time, wstyle());
    }
    // Six Eyes: cursed energy shimmering round the head
    if (has(WeaponVisual::SixEyes)) {
      const Vec2 c = R.head;
      for (int i = 0; i < 5; ++i) {
        float a = time * 1.5f + i * 1.256f;
        Vec2 q = c + Vec2{std::cos(a) * 1.5f * R.hr, std::sin(a) * 1.2f * R.hr};
        disc(X(q), t(0.008f) + 0.6f, withAlpha({160, 220, 255, 255}, 0.7f * k));
      }
    }
    // Ulquiorra's Segunda Etapa crystal over the hand
    if (has(WeaponVisual::SegundaCore)) {
      const float sc = s * H / 100.0f * 0.8f;
      Vec2 c = W(X(R.handF + Vec2{0.0f, -0.12f * H}));
      drawWeaponStyled(WeaponVisual::SegundaCore, c - Vec2{15.0f * sc, 0}, {1, 0}, sc, k, time, wstyle());
    }
    // Naruto with Kurama's chakra: a cloak of golden flames
    if (has(WeaponVisual::KuramaChakra)) {
      for (int i = 0; i < 7; ++i) {
        float ph = std::fmod(time * 1.6f + i * 0.143f, 1.0f);
        Vec2 q = R.hips + Vec2{(i - 3) * 0.05f * H + std::sin(time * 6.0f + i) * 0.02f * H, -ph * 0.7f * H};
        disc(X(q), t(0.04f * (1.0f - ph)) + 0.5f, withAlpha({255, 190, 50, 255}, 0.55f * (1.0f - ph) * k));
      }
    }
    // Grimmjow's claws of reiatsu, on both hands
    if (has(WeaponVisual::PanteraClaws)) {
      for (int h = 0; h < 2; ++h) {
        const Vec2 el = h ? R.elbowB : R.elbowF, hd = h ? R.handB : R.handF;
        const Vec2 d = (hd - el).norm(), n{-d.y, d.x};
        for (int i = 0; i < 4; ++i) {
          const Vec2 b0 = hd + d * (B.hand * H) + n * ((i - 1.5f) * B.hand * H * 0.42f);
          const Vec2 tip = b0 + d * (0.13f * H) + n * ((i - 1.5f) * 0.012f * H);
          DrawLineEx(X(b0), X(tip), t(0.024f), withAlpha({60, 214, 240, 255}, 0.4f * k));
          DrawLineEx(X(b0), X(tip), t(0.01f) + 0.6f, withAlpha({220, 252, 255, 255}, 0.95f * k));
        }
      }
    }
    // Sukuna with a finger: red aura flickering
    if (has(WeaponVisual::CursedFinger)) {
      for (int i = 0; i < 3; ++i) {
        float ph = std::fmod(time * 1.4f + i * 0.33f, 1.0f);
        Vec2 q = R.chest + Vec2{(i - 1) * 0.12f * H, -ph * 0.4f * H};
        disc(X(q), t(0.02f * (1.0f - ph)) + 0.5f, withAlpha({230, 40, 50, 255}, 0.6f * (1.0f - ph) * k));
      }
    }
  }

  // Yuno's grimoire floating by his shoulder.
  void grimoire() {
    if (hero != Hero::Yuno) return;
    const bool big = has(WeaponVisual::Grimoire);
    const float bob = std::sin(time * 2.2f) * 0.025f * H;
    const Vec2 c = R.neck + Vec2{big ? -0.24f * H : -0.22f * H, -0.12f * H + bob};
    const float sc = s * H / 100.0f * (big ? 0.95f : 0.62f);
    WeaponStyle st = wstyle();
    st.open = big ? 1.0f : 0.0f;
    Vec2 dir = rotate(Vec2{1, 0}, std::sin(time * 1.3f) * 0.15f - 0.1f);
    if (X.facing < 0) dir.x = -dir.x;
    drawWeaponStyled(WeaponVisual::Grimoire, W(X(c)) - dir * (15.0f * sc), dir, sc, P.alpha, time, st);
  }

  // Props: bomb / spear in the throwing hand, an arrow on the string.
  void props() {
    if (pose.prop == 1) {  // Zonai bomb
      Vec2 c = R.handF + Vec2{0.0f, -0.04f * H};
      group([&] {
        P.circle(X(c), t(0.05f), {70, 140, 160, 255});
        P.glowDot(X(c), 1.6f * s, 1.2f);
      });
    }
    if (pose.bow > 0.0f) {  // arrow on the string
      Vec2 a = R.handF, b = R.handB + (R.handB - R.handF).norm() * (0.08f * H);
      Color sh = hero == Hero::Sukuna ? Color{255, 150, 50, 255} : Color{150, 110, 70, 255};
      DrawLineEx(X(a), X(b), t(0.012f) + 0.6f, P.fill(sh));
      Vec2 dir = (b - a).norm(), n{-dir.y, dir.x};
      triangle(X(b + dir * (0.04f * H)), X(b + n * (0.018f * H)), X(b - n * (0.018f * H)),
               P.fill(hero == Hero::Sukuna ? Color{255, 220, 120, 255} : Color{200, 206, 214, 255}));
      if (hero == Hero::Sukuna) {
        for (int i = 0; i < 3; ++i)
          disc(X(lerpv(a, b, 0.4f + i * 0.25f) + Vec2{0, std::sin(time * 20 + i) * 0.01f * H}), t(0.02f),
               withAlpha({255, 140, 40, 255}, 0.4f * P.alpha));
      }
    }
  }

  void draw() {
    backCloth();
    backHair();
    grimoire();
    leg(false);
    arm(false);
    const bool gearFront = gearInFront();
    if (!gearFront) backArmGear();
    torso();
    leg(true);
    skirt();
    if (gearFront) backArmGear();
    head();
    frontHair();
    horns();
    props();
    frontWeaponDraw();
    arm(true);
    energy();
    if (hero == Hero::Starrk) wolfPack(true);
  }
};

// ---- Bust (portraits) ---------------------------------------------------------------------------------
void drawBust(Body& b, float R) {
  // R: portrait radius in local units. Shoulders cut along the bottom of the medallion.
  Painter& P = b.P;
  const Xf& X = b.X;
  const Look& L = b.L;
  const float H = b.H;
  Vector2 q[11];
  const Vec2 sh[6] = {{-0.86f, 0.5f}, {-0.6f, 0.3f}, {-0.24f, 0.2f}, {0.3f, 0.2f}, {0.64f, 0.3f}, {0.86f, 0.5f}};
  auto bustPoly = [&](Color c) {
    int n = 0;
    for (auto p : sh) q[n++] = X(p * R);
    for (int i = 1; i <= 4; ++i) {
      float a = deg2rad(36.0f + i * 108.0f / 5.0f);
      q[n++] = X(Vec2{std::cos(a), std::sin(a)} * R);
    }
    P.poly(q, n, c);
  };
  Color top = L.top;
  if (b.hero == Hero::Kratos) top = L.skin;
  b.group([&] {
    bustPoly(top);
    if (!P.details()) return;
    auto ln = [&](Vec2 a, Vec2 c, float w, Color k) { DrawLineEx(X(a * R), X(c * R), w * R * X.scale, P.fill(k)); };
    switch (b.hero) {
      case Hero::Asta:
        ln({-0.8f, 0.45f}, {-0.3f, 0.24f}, 0.22f, L.cloth);
        disc(X(Vec2{0.18f, 0.32f} * R), 0.07f * R * X.scale, P.fill(L.trim));
        break;
      case Hero::Yuno:
        ln({-0.82f, 0.48f}, {-0.2f, 0.24f}, 0.16f, L.cloth);
        ln({0.82f, 0.48f}, {0.25f, 0.24f}, 0.16f, L.cloth);
        ln({-0.7f, 0.56f}, {0.7f, 0.56f}, 0.05f, L.trim);
        break;
      case Hero::Ichigo:
        ln({-0.2f, 0.22f}, {0.12f, 0.6f}, 0.1f, L.cloth);
        ln({0.3f, 0.22f}, {0.12f, 0.6f}, 0.1f, L.cloth);
        ln({-0.6f, 0.3f}, {0.5f, 0.9f}, 0.09f, L.accent);
        break;
      case Hero::Byakuya:
        ln({-0.2f, 0.22f}, {0.05f, 0.65f}, 0.12f, L.arm);
        ln({0.3f, 0.22f}, {0.05f, 0.65f}, 0.12f, L.arm);
        ln({-0.5f, 0.24f}, {0.5f, 0.26f}, 0.16f, L.trim);
        break;
      case Hero::Link:
        ln({0.55f, 0.28f}, {-0.3f, 0.95f}, 0.08f, L.belt);
        ln({-0.3f, 0.22f}, {0.35f, 0.22f}, 0.07f, mixColor(L.top, WHITE, 0.3f));
        break;
      case Hero::Ganondorf:
        disc(X(Vec2{-0.62f, 0.4f} * R), 0.24f * R * X.scale, P.fill(shade(L.top, 1.2f)));
        disc(X(Vec2{0.62f, 0.4f} * R), 0.24f * R * X.scale, P.fill(shade(L.top, 1.2f)));
        ln({-0.82f, 0.5f}, {-0.42f, 0.32f}, 0.05f, L.trim);
        ln({0.42f, 0.32f}, {0.82f, 0.5f}, 0.05f, L.trim);
        ln({-0.25f, 0.6f}, {0.25f, 0.6f}, 0.04f, L.trim);
        break;
      case Hero::Kratos:
        ln({0.55f, 0.25f}, {-0.25f, 0.95f}, 0.1f, L.belt);
        ln({-0.7f, 0.36f}, {-0.6f, 0.9f}, 0.12f, L.accent);
        break;
      case Hero::Gojo:
        ln({-0.3f, 0.24f}, {0.3f, 0.24f}, 0.3f, L.top);
        disc(X(Vec2{0.18f, 0.3f} * R), 0.05f * R * X.scale, P.fill({200, 180, 120, 255}));
        break;
      case Hero::Sukuna:
        ln({-0.05f, 0.22f}, {0.12f, 0.55f}, 0.14f, L.skin);
        ln({-0.5f, 0.24f}, {0.5f, 0.26f}, 0.16f, L.cloth);
        break;
      case Hero::Kenpachi:
        ln({-0.1f, 0.22f}, {0.1f, 0.62f}, 0.22f, L.skin);
        ln({-0.85f, 0.5f}, {-0.3f, 0.26f}, 0.2f, L.cloth);
        ln({0.85f, 0.5f}, {0.3f, 0.26f}, 0.2f, L.cloth);
        break;
      case Hero::Ulquiorra:
        ln({-0.3f, 0.2f}, {0.3f, 0.2f}, 0.26f, L.top);
        disc(X(Vec2{0.0f, 0.42f} * R), 0.08f * R * X.scale, P.fill({14, 12, 16, 255}));
        break;
      case Hero::Hunter:
        disc(X(Vec2{-0.62f, 0.4f} * R), 0.22f * R * X.scale, P.fill(shade(L.top, 1.15f)));
        disc(X(Vec2{0.62f, 0.4f} * R), 0.22f * R * X.scale, P.fill(shade(L.top, 1.15f)));
        ln({-0.3f, 0.5f}, {0.3f, 0.5f}, 0.05f, L.trim);
        break;
      case Hero::Naruto:
        ln({-0.8f, 0.42f}, {0.8f, 0.42f}, 0.26f, L.cloth);
        ln({0.06f, 0.4f}, {0.06f, 0.95f}, 0.05f, {240, 238, 232, 255});
        break;
      case Hero::Sasuke:
        ln({-0.05f, 0.22f}, {0.1f, 0.6f}, 0.16f, L.skin);
        break;
      case Hero::Grimmjow:  // the open jacket, collar up
        ln({-0.86f, 0.5f}, {-0.3f, 0.24f}, 0.26f, L.cloth);
        ln({0.86f, 0.5f}, {0.32f, 0.26f}, 0.22f, L.cloth);
        ln({-0.36f, 0.22f}, {-0.3f, 0.02f}, 0.12f, L.cloth);
        break;
      case Hero::Starrk:  // high collar, the jaw bone necklace
        ln({-0.34f, 0.22f}, {0.34f, 0.22f}, 0.28f, L.top);
        ln({-0.2f, 0.42f}, {0.26f, 0.42f}, 0.09f, {246, 244, 236, 255});
        break;
      case Hero::Kakashi:
        ln({-0.3f, 0.2f}, {0.3f, 0.2f}, 0.3f, L.cloth);
        ln({-0.5f, 0.6f}, {-0.2f, 0.6f}, 0.1f, shade(L.top, 1.12f));
        ln({0.2f, 0.6f}, {0.5f, 0.6f}, 0.1f, shade(L.top, 1.12f));
        break;
      default: break;
    }
  });
  (void)H;
}
}  // namespace

void drawCharacter(const CharacterDef& d, int palette, const Pose& pose, Vec2 feet, int facing, float scale,
                   float alpha, Color flashColor, float flash, float time) {
  const float H = d.height;
  const Xf X = xform(d, pose, feet, facing, scale);

  Painter P;
  P.ow = std::max(1.5f, 3.0f * scale * std::min(1.15f, H / 100.0f));
  if (const float px = pix::artUnit(); px > 0.0f) {  // pixel art: inner lines one art pixel wide
    P.pixel = true;
    P.ow = px * 0.9f;
  }
  P.alpha = alpha;
  P.flashCol = flashColor;
  P.flash = clampf(flash, 0.0f, 1.0f);
  P.flat = alpha < 0.97f;
  P.ghost = pose.ghost;
  P.glow = 0.6f + 0.22f * std::sin(time * 2.6f) + pose.glow;

  const Build B = buildFor(d.hero);
  const Rig R = solveRig(d, pose);
  Look L = lookFor(d, palette);
  formLook(L, pose.form);
  Body b{P, X, d, pose, R, B, L, H, scale, time, d.hero, palette & 1};
  if (art::rigged(d.hero) && pose.sheet >= 0) {  // a generated animation frame: it draws the turns and lifts itself
    Pose flat = pose;
    flat.rot = 0.0f;
    flat.shift = {0.0f, 0.0f};
    if (art::drawSheet(d, pose, xform(d, flat, feet + pose.sheetOffset * scale, facing, scale), alpha, flashColor, flash)) {
      b.energy();
      return;
    }
  }
  if (art::rigged(d.hero)) {  // the picture, with what the fighter carries, casts and summons around it
    b.grimoire();  // Yuno's book, behind him
    if (d.hero == Hero::Starrk) {
      b.wolfPack(false);
      b.lilynette();
    }
    const bool gearFront = b.gearInFront();  // shield, bow
    if (!gearFront) b.backArmGear();
    art::drawFighter(d, pose, R, X, alpha, flashColor, flash, art::Layers::BelowWeapon);
    if (gearFront) b.backArmGear();
    if (pose.form != Form::None) b.horns();
    b.frontWeaponDraw();  // under the front arm: the fist holds the grip
    art::drawFighter(d, pose, R, X, alpha, flashColor, flash, art::Layers::WeaponArm);
    b.props();
    b.energy();
    if (d.hero == Hero::Starrk) b.wolfPack(true);
    return;
  }
  b.draw();
}

void drawCharacterSprite(const CharacterDef& d, int palette, const Pose& pose, Vec2 feet, int facing, float scale,
                         float alpha, Color flashColor, float flash, float time) {
  const float h = d.height * scale, rx = std::max(h * 1.7f, 200.0f * scale);
  const bool illustrated = art::rigged(d.hero);  // a detailed picture: finer pixels, more colour bands
  pix::sprite({feet.x - rx, feet.y - h * 2.3f, rx * 2.0f, h * 2.9f}, alpha, [&](float a) {
    drawCharacter(d, palette, pose, feet, facing, scale, a, flashColor, flash, time);
  }, illustrated ? pix::kFinePixel : 0.0f, illustrated ? 40.0f : 20.0f);
}

void drawShowcase(const CharacterDef& d, int palette, const Pose& pose, Vec2 feet, int facing, float height,
                  float time) {
  if (art::rigged(d.hero)) {  // the picture itself, crisp, a little bigger (it is detailed)
    Pose p = pose;
    if (p.sheet < 0 && art::hasSheet(d.hero, art::kSheetIdle)) {  // its generated guard
      p.sheet = art::kSheetIdle;
      p.sheetT = time;
    }
    drawCharacter(d, palette, p, feet, facing, height * 1.1f / d.height, 1.0f, WHITE, 0.0f, time);
  } else if (!art::drawWhole(d.hero, feet + Vec2{0.0f, -std::sin(time * 2.6f) * 0.6f}, height * 1.1f, facing, 1.0f)) {
    drawCharacterSprite(d, palette, pose, feet, facing, height / d.height, 1.0f, WHITE, 0.0f, time);
  }
}

void drawPortrait(const CharacterDef& d, int palette, Vec2 center, float size, float time) {
  if (art::drawBust(d.hero, center, size, 1.0f)) return;
  const float H = d.height;
  const Build B = buildFor(d.hero);
  const float hrLocal = B.head * H;
  const float rad = size * 0.5f;
  const float scale = rad * 0.4f / hrLocal;
  Pose pose = restPose(d, time);
  pose.lean = 0.0f;
  pose.flow = pose.flowTip = {};
  Rig R{};
  R.hr = hrLocal;
  R.tilt = 0.0f;
  R.head = {0.0f, 0.0f};
  R.neck = {0.0f, hrLocal * 1.0f};
  R.chest = R.neck + Vec2{0.0f, 0.1f * H};
  R.hips = R.neck + Vec2{0.0f, B.torso * H};
  R.shF = R.neck + Vec2{0.1f * H, 0.05f * H};
  R.shB = R.neck + Vec2{-0.1f * H, 0.05f * H};
  Xf X;
  X.facing = 1;
  X.scale = scale;
  X.rot = 0.0f;
  X.squash = X.sx = 1.0f;
  X.center = center + Vec2{0.04f * rad, -0.14f * rad};
  Painter P;
  P.ow = std::max(1.0f, size * 0.035f);
  P.glow = 0.7f;
  const Look L = lookFor(d, palette);
  Body b{P, X, d, pose, R, B, L, H, scale, time, d.hero, palette & 1};
  b.bust = true;
  // The bust is laid out around the medallion centre: shift the frame so the shoulders sit low.
  Xf XB = X;
  XB.center = center;
  Body bb{P, XB, d, pose, R, B, L, H, scale, time, d.hero, palette & 1};
  bb.bust = true;
  b.backHair();
  drawBust(bb, rad / scale);
  b.head();
  b.frontHair();
}

void drawPortraitIn(const CharacterDef& d, int palette, Vec2 center, float size, Rectangle box, float slant,
                    float time) {
  if (art::drawBustIn(d.hero, center, size, box, slant, 1.0f)) return;
  beginClip({box.x + slant * 0.5f, box.y, box.width - slant, box.height});  // drawn by hand: a straight cut
  drawPortrait(d, palette, center, size, time);
  endClip();
}

void drawPortraitDisc(const CharacterDef& d, int palette, Vec2 center, float radius, float time) {
  if (art::drawDisc(d.hero, center, radius, 1.0f)) return;
  drawPortrait(d, palette, center + Vec2{0.0f, -radius * 0.055f}, radius * 1.72f, time);
}

Vec2 limbWorld(const CharacterDef& d, const Pose& pose, Vec2 feet, int facing, float scale, Limb limb, bool weaponTip) {
  const Xf X = xform(d, pose, feet, facing, scale);
  const Rig R = solveRig(d, pose);
  Vec2 l;
  switch (limb) {
    case Limb::ArmBack: l = R.handB; break;
    case Limb::LegFront: l = R.footF; break;
    case Limb::LegBack: l = R.footB; break;
    case Limb::Head: l = R.head; break;
    case Limb::Body: case Limb::None: l = lerpv(R.hips, R.neck, 0.5f); break;
    default: l = R.handF; break;
  }
  if (weaponTip) l = R.handF + pose.weaponDir.norm() * frontWeaponLength(d, pose);
  return W(X(l));
}
