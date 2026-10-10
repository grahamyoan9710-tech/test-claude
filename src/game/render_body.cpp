// Procedural cartoon bodies: a small 2-bone IK rig drawn with thick outlined strokes.
// Look: painterly cel shading (warm key light from the upper left, cool teal-tinted shadow with a
// soft terminator, thin rim light), warm umber outlines, earthy palettes with gold trims and the
// glowing teal runes of an ancient technology.
#include "game/render.hpp"

namespace {
const Color kOutline = {56, 35, 28, 255};  // warm dark umber
const Color kShadowTint = {38, 100, 116, 255};
const Color kRimTint = {200, 244, 236, 255};
const Color kWarmLight = {255, 236, 200, 255};
const Color kGold = {226, 180, 76, 255};
const Vec2 kLight{-0.55f, -0.835f};  // towards the key light (screen space)

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

struct Painter {
  int pass = kLine;
  float ow = 3.0f;
  float alpha = 1.0f;
  Color flashCol = WHITE;
  float flash = 0.0f;
  bool flat = false;   // translucent draw: one flat layer
  bool ghost = false;  // afterimage: flat fill only, no outline, no details
  float glow = 0.6f;   // rune glow

  Color fill(Color c) const { return withAlpha(mixColor(c, flashCol, flash), alpha * c.a / 255.0f); }
  Color line() const { return withAlpha(kOutline, alpha); }
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
  // Single triangle (spikes, ears, hair...): shadow, then a lit inset towards the light.
  void tri(Vector2 a, Vector2 b, Vector2 c, Color k, bool shadedTri = true) const {
    Vector2 m{(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3};
    if (pass == kLine) {
      if (ghost) return;
      auto grow = [&](Vector2 p) {
        Vec2 d = W(p) - W(m);
        float l = d.len();
        return l > 0.01f ? V(W(p) + d * ((l + ow * 1.7f) / l) - d) : p;
      };
      triangle(grow(a), grow(b), grow(c), line());
      return;
    }
    if (!shaded() || !shadedTri) {
      if (pass == kLit) triangle(a, b, c, fill(shadedTri ? k : midTone(k)));
      return;
    }
    if (pass == kShadow) {
      triangle(a, b, c, fill(shadowTone(k)));
    } else if (pass == kLit) {
      float per = (W(b) - W(a)).len() + (W(c) - W(b)).len() + (W(a) - W(c)).len();
      float area = std::fabs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) * 0.5f;
      float rin = per > 0.01f ? 2.0f * area / per : 0.0f;
      Vec2 o = kLight * (rin * 0.16f);
      auto in = [&](Vector2 p) { return V(W(m) + (W(p) - W(m)) * 0.76f + o); };
      triangle(a, b, c, fill(midTone(k)));
      triangle(in(a), in(b), in(c), fill(litTone(k)));
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
  void glowLine(Vector2 a, Vector2 b, float w, float k = 1.0f) const {
    if (!details() || flat) return;
    float g = clampf(glow * k, 0.0f, 2.0f);
    DrawLineEx(a, b, w * 3.4f, withAlpha(fill(kZonaiGlow), alpha * 0.22f * g));
    DrawLineEx(a, b, w, withAlpha(fill(mixColor(kZonaiGlow, WHITE, 0.25f + 0.2f * g)),
                                  alpha * std::min(1.0f, 0.45f + 0.5f * g)));
  }
  void glowDot(Vector2 c, float r, float k = 1.0f) const {
    if (!details() || flat) return;
    float g = clampf(glow * k, 0.0f, 2.0f);
    disc(c, r * 2.6f, withAlpha(fill(kZonaiGlow), alpha * 0.2f * g));
    disc(c, r, withAlpha(fill(mixColor(kZonaiGlow, WHITE, 0.35f)), alpha * std::min(1.0f, 0.5f + 0.5f * g)));
  }
};

struct Xf {
  Vec2 center;
  int facing;
  float scale, rot, squash, sx;
  Vector2 operator()(Vec2 l) const {
    Vec2 v{l.x * facing * scale * sx, l.y * scale * squash};
    return V(center + rotate(v, rot));
  }
};
Xf xform(const CharacterDef& d, const Pose& pose, Vec2 feet, int facing, float scale) {
  Xf X;
  X.facing = facing;
  X.scale = scale;
  X.squash = pose.squash;
  X.sx = pose.sx;
  X.rot = pose.rot * facing;
  X.center = feet + Vec2{pose.shift.x * facing * scale, -d.height * 0.5f * scale * pose.squash + pose.shift.y * scale};
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
float wrapAngle(float a) {
  a = std::fmod(a + kPi, 2.0f * kPi);
  return (a < 0.0f ? a + 2.0f * kPi : a) - kPi;
}

struct Dims {
  float headR, torsoTh, armTh, legTh, handR, footR;
};
Dims dimsFor(BodyStyle s) {
  switch (s) {
    case BodyStyle::Speedster: return {0.17f, 0.24f, 0.065f, 0.07f, 0.045f, 0.055f};
    case BodyStyle::Brute: return {0.13f, 0.32f, 0.11f, 0.12f, 0.075f, 0.075f};
    case BodyStyle::Knight: return {0.14f, 0.19f, 0.065f, 0.075f, 0.045f, 0.055f};
    case BodyStyle::Ninja: return {0.14f, 0.18f, 0.065f, 0.07f, 0.045f, 0.05f};
    default: return {0.15f, 0.22f, 0.075f, 0.085f, 0.055f, 0.06f};
  }
}

struct Rig {
  float hr;
  Vec2 hips, neck, head, shF, shB, hipF, hipB;
  Vec2 elbowF, elbowB, kneeF, kneeB, handF, handB, footF, footB;
};
Rig solveRig(const CharacterDef& d, const Pose& pose) {
  const float H = d.height;
  const Dims dm = dimsFor(d.style);
  Rig r;
  r.hr = dm.headR * H;
  const float drop = pose.crouch * 0.14f * H;
  r.hips = {0.0f, 0.10f * H + drop};
  r.neck = r.hips + rotate({0.0f, -0.27f * H}, pose.lean);
  r.head = r.neck + rotate({0.0f, -r.hr * 0.9f}, pose.lean * 1.3f);
  r.shF = r.neck + rotate({0.035f * H, 0.04f * H}, pose.lean);
  r.shB = r.neck + rotate({-0.035f * H, 0.04f * H}, pose.lean);
  r.hipF = r.hips + Vec2{0.035f * H, 0.0f};
  r.hipB = r.hips + Vec2{-0.035f * H, 0.0f};
  const float ua = 0.155f * H, thigh = 0.205f * H;
  r.elbowF = ik(r.shF, pose.handF, ua, ua, 1.0f, r.handF);
  r.elbowB = ik(r.shB, pose.handB, ua, ua, 1.0f, r.handB);
  r.kneeF = ik(r.hipF, pose.footF, thigh, thigh, -1.0f, r.footF);
  r.kneeB = ik(r.hipB, pose.footB, thigh, thigh, -1.0f, r.footB);
  return r;
}

// Puffo's arms and legs are stubs: hand and foot targets stay close to the ball.
Vec2 puffCenter(const CharacterDef& d, const Pose& pose) { return {0.0f, 0.08f * d.height + pose.crouch * 0.06f * d.height}; }
Vec2 stubby(const CharacterDef& d, const Pose& pose, Vec2 h, float reach = 0.54f) {
  Vec2 bc = puffCenter(d, pose);
  Vec2 rel = h - bc;
  float l = rel.len();
  float maxL = reach * d.height;
  return l > maxL ? bc + rel * (maxL / l) : h;
}
Vec2 stubbyFoot(const CharacterDef& d, const Pose& pose, Vec2 f) { return stubby(d, pose, f, 0.5f); }

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

void drawEyes(const Painter& P, const Xf& X, Vec2 head, float r, const Pose& pose, Color iris) {
  if (!P.details()) return;
  Color dark = P.fill({44, 28, 26, 255});
  Vec2 eyes[2] = {head + Vec2{0.18f * r, -0.08f * r}, head + Vec2{0.62f * r, -0.06f * r}};
  for (int i = 0; i < 2; ++i) {
    Vector2 e = X(eyes[i]);
    float s = X.scale * (i == 0 ? 1.0f : 0.85f);
    if (pose.hurt) {
      DrawLineEx({e.x - 4 * s, e.y - 4 * s}, {e.x + 4 * s, e.y + 4 * s}, 2.5f * X.scale, dark);
      DrawLineEx({e.x - 4 * s, e.y + 4 * s}, {e.x + 4 * s, e.y - 4 * s}, 2.5f * X.scale, dark);
    } else if (pose.blink) {
      DrawLineEx({e.x - 4 * s, e.y + 1 * s}, {e.x, e.y + 2 * s}, 2.0f * X.scale, dark);
      DrawLineEx({e.x, e.y + 2 * s}, {e.x + 4 * s, e.y + 1 * s}, 2.0f * X.scale, dark);
    } else {
      fillEllipse(e, 0.17f * r * s, 0.27f * r * s, P.fill({252, 246, 232, 255}));
      Vector2 ic{e.x + 0.06f * r * s * X.facing, e.y + 0.02f * r * s};
      fillEllipse(ic, 0.11f * r * s, 0.18f * r * s, P.fill(iris));
      fillEllipse(ic, 0.06f * r * s, 0.1f * r * s, dark);
      disc({e.x + 0.02f * r * s * X.facing, e.y - 0.09f * r * s}, 0.045f * r * s, P.fill(WHITE));
    }
  }
}

void drawPuff(Painter& P, const Xf& X, const CharacterDef& d, const Palette& pal, const Pose& pose, float time) {
  const float H = d.height;
  Color body = earthy(pal.primary), feet = earthy(pal.secondary);
  const Vec2 bc = puffCenter(d, pose);
  const float R = 0.42f * H;
  const float s = X.scale;
  auto group = [&](const auto& fn) {
    for (int p = kLine; p <= kLit; ++p) {
      if ((p == kRim || p == kShadow) && !P.shaded()) continue;
      P.pass = p;
      fn();
    }
  };
  group([&] { P.ellipse(X(stubbyFoot(d, pose, pose.footB)), 0.13f * H * s, 0.08f * H * s, backTone(feet)); });
  group([&] { P.circle(X(stubby(d, pose, pose.handB)), 0.1f * H * s, backTone(body)); });
  group([&] {
    P.circle(X(bc), R * s, body);
    Vec2 tip = bc + Vec2{-0.02f * H, -R - 0.12f * H} + pose.flowTip * (0.07f * H) +
               Vec2{std::sin(time * 2.3f) * 0.012f * H, 0.0f};
    P.tri(X(bc + polar(-100, R * 0.95f)), X(bc + polar(-75, R * 0.95f)), X(tip), body);
  });
  group([&] { P.ellipse(X(stubbyFoot(d, pose, pose.footF)), 0.13f * H * s, 0.08f * H * s, feet); });
  if (pose.weapon && d.weaponLength > 0) group([&] {
    Vec2 hand = stubby(d, pose, pose.handF);
    Vec2 dir = pose.weaponDir.norm();
    Vec2 tip = hand + dir * d.weaponLength;
    Vec2 perp{-dir.y, dir.x};
    P.seg(X(hand), X(tip), 6 * s, {124, 84, 54, 255});
    P.seg(X(tip - perp * 16), X(tip + perp * 16), 26 * s, feet);
    P.deco([&] {
      P.seg(X(tip - perp * 9), X(tip - perp * 6), 27 * s, kGold);
      P.seg(X(tip + perp * 6), X(tip + perp * 9), 27 * s, kGold);
    });
    P.glowLine(X(tip - perp * 2 - dir * 7), X(tip - perp * 2 + dir * 7), 1.6f * s);
  });
  group([&] { P.circle(X(stubby(d, pose, pose.handF)), 0.1f * H * s, body); });
  if (P.ghost) return;
  P.pass = kLit;
  Color dark = P.fill({48, 26, 34, 255});
  Vec2 eyes[2] = {bc + Vec2{0.1f * H, -0.12f * H}, bc + Vec2{0.26f * H, -0.12f * H}};
  for (auto e : eyes) {
    Vector2 p = X(e);
    if (pose.hurt || pose.blink) {
      DrawLineEx({p.x - 7 * s, p.y}, {p.x + 7 * s, p.y}, 3 * s, dark);
    } else {
      fillEllipse(p, 0.05f * H * s, 0.1f * H * s, dark);
      fillEllipse({p.x, p.y + 0.04f * H * s}, 0.035f * H * s, 0.04f * H * s, P.fill(feet));
      disc({p.x, p.y - 0.04f * H * s}, 0.022f * H * s, P.fill(WHITE));
    }
  }
  Color blush = P.fill({240, 110, 130, 150});
  fillEllipse(X(bc + Vec2{0.05f * H, 0.02f * H}), 0.05f * H * s, 0.025f * H * s, blush);
  fillEllipse(X(bc + Vec2{0.33f * H, 0.02f * H}), 0.04f * H * s, 0.025f * H * s, blush);
  Vector2 mo = X(bc + Vec2{0.18f * H, 0.04f * H});
  if (pose.hurt) disc(mo, 0.03f * H * s, dark);
  else DrawLineEx({mo.x - 3 * s, mo.y}, {mo.x + 3 * s, mo.y + 2 * s}, 2 * s, dark);
}
}  // namespace

void drawCharacter(const CharacterDef& d, int palette, const Pose& pose, Vec2 feet, int facing, float scale,
                   float alpha, Color flashColor, float flash, float time) {
  const Palette& pal = d.palettes[palette & 1];
  const float H = d.height;
  const Xf X = xform(d, pose, feet, facing, scale);

  Painter P;
  P.ow = std::max(1.5f, 3.0f * scale * (H / 100.0f));
  P.alpha = alpha;
  P.flashCol = flashColor;
  P.flash = clampf(flash, 0.0f, 1.0f);
  P.flat = alpha < 0.97f;
  P.ghost = pose.ghost;
  P.glow = 0.6f + 0.22f * std::sin(time * 2.6f) + pose.glow;

  if (d.style == BodyStyle::Puff) {
    drawPuff(P, X, d, pal, pose, time);
    return;
  }

  const Dims dm = dimsFor(d.style);
  const Rig R = solveRig(d, pose);
  const float hr = R.hr;
  const Vec2 hips = R.hips, neck = R.neck, head = R.head;
  const float s = scale;

  Color primary = earthy(pal.primary), secondary = earthy(pal.secondary), accent = earthy(pal.accent),
        skin = earthy(pal.skin);
  Color torsoC = primary, armC = primary, handC = skin, legC = secondary, footC{100, 64, 42, 255}, headC = skin;
  Color hair = shade(primary, 0.45f);
  switch (d.style) {
    case BodyStyle::Brawler: handC = {244, 236, 218, 255}; break;  // ivory gloves
    case BodyStyle::Speedster: armC = handC = legC = footC = headC = primary; break;
    case BodyStyle::Brute: armC = handC = legC = skin; footC = secondary; break;
    case BodyStyle::Knight: torsoC = secondary; legC = shade(primary, 0.6f); footC = secondary; break;
    case BodyStyle::Ninja: armC = handC = legC = footC = secondary; headC = primary; break;
    default: break;
  }

  auto group = [&](const auto& fn) {
    for (int p = kLine; p <= kLit; ++p) {
      if ((p == kRim || p == kShadow) && !P.shaded()) continue;
      P.pass = p;
      fn();
    }
  };
  group([&] {
    // --- Back accessories (secondary motion) ----------------------------------
    if (d.style == BodyStyle::Knight) {  // cape
      Vec2 c0 = neck + Vec2{-0.03f * H, 0.0f}, c1 = neck + Vec2{0.02f * H, 0.02f * H};
      Vec2 q[3];
      chain(c0, {-0.75f, 1.0f}, 0.5f * H, 2, pose, time, 0.35f, 0.9f, 0.05f, q);
      Vec2 c3 = hips + Vec2{-0.06f * H, 0.34f * H} + pose.flow * (0.05f * H);
      Color cape = shade(primary, 0.78f);
      P.tri(X(c0), X(q[1]), X(c3), cape, false);
      P.tri(X(q[1]), X(q[2]), X(c3), cape, false);
      P.tri(X(c0), X(c1), X(c3), cape, false);
      if (P.details()) {
        DrawLineEx(X(q[2]), X(c3), 2.6f * s, P.fill(kGold));
        DrawLineEx(X(lerpv(c0, c1, 0.5f)), X(lerpv(q[2], c3, 0.4f)), 1.6f * s, P.fill(shadowTone(cape)));
      }
    }
    if (d.style == BodyStyle::Speedster) {  // lightning tail: stiff, swung by the drag, tip lagging
      auto swing = [&](Vec2 drag) {
        Vec2 rest{-0.52f, -0.84f};
        float a = std::atan2(rest.y + drag.y * 0.5f, rest.x + drag.x * 0.5f) - std::atan2(rest.y, rest.x);
        return clampf(wrapAngle(a), -0.7f, 0.7f) + std::sin(time * 3.0f) * 0.04f;
      };
      const float a0 = swing(pose.flow), a1 = swing(pose.flowTip);
      Vec2 t0 = hips + Vec2{-0.1f * H, 0.0f};
      Vec2 t1 = t0 + rotate(Vec2{-0.14f, -0.12f} * H, a0);
      Vec2 t2 = t1 + rotate(Vec2{0.06f, -0.1f} * H, lerpf(a0, a1, 0.5f));
      Vec2 t3 = t2 + rotate(Vec2{-0.18f, -0.2f} * H, a1);
      P.seg(X(t0), X(t1), 0.06f * H * s, primary);
      P.seg(X(t1), X(t2), 0.07f * H * s, primary);
      P.seg(X(t2), X(t3), 0.1f * H * s, primary);
      auto tipPt = [&](Vec2 o) { return X(t3 + rotate(o * H, a1)); };
      P.tri(tipPt({0.02f, 0.04f}), tipPt({-0.08f, -0.08f}), tipPt({0.06f, -0.03f}), accent);
      P.glowDot(tipPt({-0.01f, -0.02f}), 1.8f * s, 0.8f);
    }
    if (d.style == BodyStyle::Ninja) {  // scarf
      Vec2 q[4];
      chain(neck + Vec2{-0.02f * H, 0.01f * H}, {-1.0f, 0.1f}, 0.36f * H, 3, pose, time, 0.3f, 1.0f, 0.2f, q);
      for (int i = 0; i < 3; ++i) P.seg(X(q[i]), X(q[i + 1]), lerpf(0.062f, 0.042f, i / 2.0f) * H * s, accent);
    }
    if (d.style == BodyStyle::Knight) {  // hair + ponytail behind the head
      P.circle(X(head + Vec2{-0.15f * hr, -0.2f * hr}), hr * 1.02f * s, hair);
      Vec2 q[3];
      chain(head + Vec2{-0.7f * hr, -0.5f * hr}, {-0.75f, 1.0f}, 1.7f * hr, 2, pose, time, 0.5f, 0.8f, 0.08f, q);
      P.seg(X(q[0]), X(q[1]), 0.08f * H * s, hair);
      P.seg(X(q[1]), X(q[2]), 0.068f * H * s, hair);
    }
  });
  group([&] {
    // --- Back limbs -----------------------------------------------------------
    P.seg(X(R.hipB), X(R.kneeB), dm.legTh * H * s, backTone(legC));
    P.seg(X(R.kneeB), X(R.footB), dm.legTh * H * s, backTone(legC));
    P.ellipse(X(R.footB + Vec2{0.02f * H, -0.01f * H}), dm.footR * 1.4f * H * s, dm.footR * H * s, backTone(footC));
  });
  group([&] {
    P.seg(X(R.shB), X(R.elbowB), dm.armTh * H * s, backTone(armC));
    P.seg(X(R.elbowB), X(R.handB), dm.armTh * H * s, backTone(armC));
    if (d.style == BodyStyle::Brawler)
      P.deco([&] {
        P.seg(X(lerpv(R.elbowB, R.handB, 0.7f)), X(lerpv(R.elbowB, R.handB, 0.82f)), dm.armTh * 1.2f * H * s,
              backTone(kGold));
      });
    P.circle(X(R.handB), dm.handR * H * s, backTone(handC));
  });
  group([&] {
    // --- Torso ------------------------------------------------------------------
    P.seg(X(hips), X(neck + rotate({0.0f, 0.03f * H}, pose.lean)), dm.torsoTh * H * s, torsoC);
    if (d.style == BodyStyle::Brute) {
      for (int i = 0; i < 3; ++i) {  // back spikes
        Vec2 base = lerpv(neck, hips, 0.15f + i * 0.3f) + rotate({-0.15f * H, 0.0f}, pose.lean);
        P.tri(X(base + Vec2{0.0f, -0.05f * H}), X(base + Vec2{0.0f, 0.05f * H}), X(base + Vec2{-0.12f * H, -0.04f * H}), accent);
      }
    }
    const Vec2 mid = lerpv(hips, neck, 0.45f);
    if (d.style == BodyStyle::Brute) {  // belly plate with a glowing ancient glyph
      Vec2 pc = mid + rotate({0.06f * H, 0.02f * H}, pose.lean);
      P.deco([&] { P.ellipse(X(pc), 0.09f * H * s, 0.12f * H * s, accent); });
      if (P.details() && !P.flat) {
        Vector2 g = X(pc + Vec2{0.005f * H, 0.0f});
        float gr = 0.042f * H * s;
        float k = std::min(1.6f, P.glow);
        disc(g, gr * 2.2f, withAlpha(P.fill(kZonaiGlow), P.alpha * 0.18f * k));
        DrawRing(g, gr * 0.72f, gr, 0, 360, 20, withAlpha(P.fill(kZonaiGlow), P.alpha * std::min(1.0f, 0.5f + 0.4f * k)));
        for (int i = 0; i < 3; ++i) {
          float a = time * 0.4f + i * 2.094f;
          P.glowLine({g.x + std::cos(a) * gr * 1.1f, g.y + std::sin(a) * gr * 1.1f},
                     {g.x + std::cos(a) * gr * 1.9f, g.y + std::sin(a) * gr * 1.9f}, 1.2f * s);
        }
        P.glowDot(g, gr * 0.3f);
      }
    }
    if (d.style == BodyStyle::Brawler || d.style == BodyStyle::Knight || d.style == BodyStyle::Ninja) {  // belt
      Vec2 b0 = hips + Vec2{-0.09f * H, -0.02f * H}, b1 = hips + Vec2{0.09f * H, -0.02f * H};
      Color belt = d.style == BodyStyle::Brawler ? Color{110, 72, 46, 255} : accent;
      P.deco([&] { P.seg(X(b0), X(b1), 0.045f * H * s, belt); });
      if (d.style == BodyStyle::Brawler) P.deco([&] { P.circle(X(hips + Vec2{0.05f * H, -0.02f * H}), 0.03f * H * s, kGold); });
      if (d.style == BodyStyle::Knight) {
        const Vec2 c0 = neck + rotate({-0.07f * H, 0.035f * H}, pose.lean);
        const Vec2 c1 = neck + rotate({0.07f * H, 0.035f * H}, pose.lean);
        P.deco([&] { P.seg(X(c0), X(c1), 0.03f * H * s, kGold); });  // gold collar
        P.glowDot(X(hips + Vec2{0.04f * H, -0.02f * H}), 1.6f * s);
      }
      if (d.style == BodyStyle::Ninja) P.deco([&] { P.circle(X(hips + Vec2{0.05f * H, -0.02f * H}), 0.022f * H * s, kGold); });
    }
  });
  group([&] {
    // --- Front leg --------------------------------------------------------------
    P.seg(X(R.hipF), X(R.kneeF), dm.legTh * H * s, legC);
    P.seg(X(R.kneeF), X(R.footF), dm.legTh * H * s, legC);
    P.ellipse(X(R.footF + Vec2{0.02f * H, -0.01f * H}), dm.footR * 1.4f * H * s, dm.footR * H * s, footC);
  });
  group([&] {
    // --- Head -----------------------------------------------------------------
    if (d.style == BodyStyle::Speedster) {  // ears behind
      Vec2 e1a = head + polar(-125, hr * 0.8f), e1b = head + polar(-95, hr * 0.8f), e1t = head + Vec2{-0.55f * hr, -2.0f * hr};
      P.tri(X(e1a), X(e1b), X(e1t), primary);
      P.tri(X(lerpv(e1a, e1t, 0.65f)), X(lerpv(e1b, e1t, 0.65f)), X(e1t), secondary);
    }
    P.circle(X(head), hr * s, headC);
    if (d.style == BodyStyle::Speedster) {
      Vec2 e2a = head + polar(-85, hr * 0.8f), e2b = head + polar(-55, hr * 0.8f), e2t = head + Vec2{0.35f * hr, -1.95f * hr};
      P.tri(X(e2a), X(e2b), X(e2t), primary);
      P.tri(X(lerpv(e2a, e2t, 0.65f)), X(lerpv(e2b, e2t, 0.65f)), X(e2t), secondary);
    }
    if (d.style == BodyStyle::Brawler) {  // flame hair, licking backwards with the motion
      for (int i = 0; i < 4; ++i) {
        float a0 = -170.0f + i * 38.0f;
        Vec2 b0 = head + polar(a0, hr * 0.85f), b1 = head + polar(a0 + 45.0f, hr * 0.85f);
        Vec2 tip = head + polar(a0 + 10.0f, hr * (1.75f + 0.12f * std::sin(time * 9.0f + i))) + Vec2{-0.3f * hr, 0.0f} +
                   pose.flowTip * (0.35f * hr * (0.6f + 0.15f * i));
        P.tri(X(b0), X(b1), X(tip), i % 2 ? accent : primary);
      }
    }
    if (d.style == BodyStyle::Brute) {  // horns
      Color horn = {232, 220, 190, 255};
      P.tri(X(head + polar(-150, hr)), X(head + polar(-115, hr)), X(head + Vec2{-1.25f * hr, -1.55f * hr}), horn);
      P.tri(X(head + polar(-70, hr)), X(head + polar(-35, hr)), X(head + Vec2{1.05f * hr, -1.6f * hr}), horn);
    }
    if (P.details()) {
      if (d.style == BodyStyle::Ninja) {  // eye band of the hood + glyph on the brow
        fillEllipse(X(head + Vec2{0.42f * hr, -0.08f * hr}), 0.6f * hr * s, 0.3f * hr * s, P.fill(skin));
        Vector2 gc = X(head + Vec2{0.28f * hr, -0.62f * hr});
        float gs = 0.13f * hr * s;
        P.glowLine({gc.x - gs, gc.y}, {gc.x, gc.y - gs}, 1.1f * s);
        P.glowLine({gc.x, gc.y - gs}, {gc.x + gs, gc.y}, 1.1f * s);
        P.glowLine({gc.x + gs, gc.y}, {gc.x, gc.y + gs}, 1.1f * s);
        P.glowLine({gc.x, gc.y + gs}, {gc.x - gs, gc.y}, 1.1f * s);
      }
      drawEyes(P, X, head, hr, pose, d.style == BodyStyle::Brute ? Color{200, 50, 34, 255} : Color{70, 52, 44, 255});
      if (d.style == BodyStyle::Speedster) {  // electric cheeks
        Vector2 ck = X(head + Vec2{0.55f * hr, 0.45f * hr});
        disc(ck, 0.34f * hr * s, withAlpha(P.fill(accent), P.alpha * 0.22f));
        disc(ck, 0.18f * hr * s, P.fill(accent));
      }
      if (d.style == BodyStyle::Knight) {  // tiara with a teal gem
        triangle(X(head + polar(-110, hr * 0.9f)), X(head + polar(-60, hr * 0.9f)), X(head + polar(-80, hr * 1.42f)),
                 P.fill(kGold));
        P.glowDot(X(head + polar(-82, hr * 1.06f)), 1.3f * s);
      }
      if (d.style == BodyStyle::Brute) {
        Vector2 m0 = X(head + Vec2{0.55f * hr, 0.5f * hr});
        triangle(m0, {m0.x + 3 * s * facing, m0.y - 6 * s}, {m0.x + 6 * s * facing, m0.y}, P.fill({246, 238, 220, 255}));
      }
      if (d.style != BodyStyle::Ninja && pose.hurt) disc(X(head + Vec2{0.55f * hr, 0.5f * hr}), 0.12f * hr * s, P.fill({64, 24, 30, 255}));
    }
  });
  group([&] {
    if (d.style == BodyStyle::Ninja) {  // headband tails
      Vec2 k0 = head + Vec2{-0.85f * hr, -0.3f * hr};
      Vec2 q[3], r[3];
      chain(k0, {-0.8f, 0.55f}, 0.65f * hr, 2, pose, time + 0.7f, 0.35f, 0.9f, 0.22f, q);
      chain(k0, {-1.0f, -0.1f}, 0.75f * hr, 2, pose, time, 0.2f, 1.0f, 0.25f, r);
      P.seg(X(q[0]), X(q[1]), 0.035f * H * s, accent);
      P.seg(X(q[1]), X(q[2]), 0.03f * H * s, accent);
      P.seg(X(r[0]), X(r[1]), 0.032f * H * s, accent);
      P.seg(X(r[1]), X(r[2]), 0.027f * H * s, accent);
    }
  });
  group([&] {
    // --- Front arm + weapon ------------------------------------------------------
    if (pose.weapon && d.weaponLength > 0) {
      Vec2 dir = pose.weaponDir.norm();
      Vec2 perp{-dir.y, dir.x};
      Vec2 tip = R.handF + dir * d.weaponLength;
      if (d.style == BodyStyle::Knight) {
        P.seg(X(R.handF), X(tip), 0.035f * H * s, {214, 222, 230, 255});
        P.seg(X(R.handF - perp * 9.0f), X(R.handF + perp * 9.0f), 0.03f * H * s, kGold);
        if (P.details() && !P.flat) {
          DrawLineEx(X(R.handF + dir * 8.0f), X(tip - dir * 4.0f), 1.3f * s, P.fill({250, 252, 255, 255}));
          P.glowLine(X(R.handF + dir * 10.0f), X(R.handF + dir * (d.weaponLength * 0.55f)), 1.2f * s, 0.8f);
        }
        P.glowDot(X(R.handF - dir * 6.0f), 1.5f * s);
      } else {
        P.seg(X(R.handF), X(tip), 0.03f * H * s, {74, 70, 78, 255});
        P.circle(X(R.handF - dir * 6.0f), 0.025f * H * s, accent);
        P.glowLine(X(R.handF + dir * 5.0f), X(tip - dir * 2.0f), 1.0f * s, 0.7f);
      }
    }
    P.seg(X(R.shF), X(R.elbowF), dm.armTh * H * s, armC);
    P.seg(X(R.elbowF), X(R.handF), dm.armTh * H * s, armC);
    if (d.style == BodyStyle::Brawler) {  // gold cuff + ancient glowing veins
      P.deco([&] {
        P.seg(X(lerpv(R.elbowF, R.handF, 0.7f)), X(lerpv(R.elbowF, R.handF, 0.82f)), dm.armTh * 1.2f * H * s, kGold);
      });
      P.glowLine(X(lerpv(R.elbowF, R.handF, 0.12f)), X(lerpv(R.elbowF, R.handF, 0.6f)), 1.2f * s, 0.8f);
    }
    P.circle(X(R.handF), dm.handR * H * s, handC);
  });
}

Vec2 limbWorld(const CharacterDef& d, const Pose& pose, Vec2 feet, int facing, float scale, Limb limb, bool weaponTip) {
  const Xf X = xform(d, pose, feet, facing, scale);
  Vec2 l;
  if (d.style == BodyStyle::Puff) {
    switch (limb) {
      case Limb::ArmBack: l = stubby(d, pose, pose.handB); break;
      case Limb::LegFront: l = stubbyFoot(d, pose, pose.footF); break;
      case Limb::LegBack: l = stubbyFoot(d, pose, pose.footB); break;
      case Limb::Head: case Limb::Body: case Limb::None: l = puffCenter(d, pose); break;
      default: l = stubby(d, pose, pose.handF); break;
    }
    if (weaponTip) l = stubby(d, pose, pose.handF) + pose.weaponDir.norm() * d.weaponLength;
    return W(X(l));
  }
  const Rig R = solveRig(d, pose);
  switch (limb) {
    case Limb::ArmBack: l = R.handB; break;
    case Limb::LegFront: l = R.footF; break;
    case Limb::LegBack: l = R.footB; break;
    case Limb::Head: l = R.head; break;
    case Limb::Body: case Limb::None: l = lerpv(R.hips, R.neck, 0.5f); break;
    default: l = R.handF; break;
  }
  if (weaponTip) l = R.handF + pose.weaponDir.norm() * d.weaponLength;
  return W(X(l));
}
