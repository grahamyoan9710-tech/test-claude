// Procedural cartoon bodies: a small 2-bone IK rig drawn with thick outlined strokes.
#include "game/render.hpp"

namespace {
const Color kOutline = {22, 18, 32, 255};

struct Painter {
  int pass = 0;  // 0 = outline, 1 = fill
  float ow = 3.0f;
  float alpha = 1.0f;
  Color flashCol = WHITE;
  float flash = 0.0f;

  Color fill(Color c) const { return withAlpha(mixColor(c, flashCol, flash), alpha * c.a / 255.0f); }
  Color line() const { return withAlpha(kOutline, alpha); }
  void seg(Vector2 a, Vector2 b, float th, Color c) const {
    if (pass == 0) {
      DrawLineEx(a, b, th + ow * 2, line());
      DrawCircleV(a, th * 0.5f + ow, line());
      DrawCircleV(b, th * 0.5f + ow, line());
    } else {
      Color k = fill(c);
      DrawLineEx(a, b, th, k);
      DrawCircleV(a, th * 0.5f, k);
      DrawCircleV(b, th * 0.5f, k);
    }
  }
  void circle(Vector2 c, float r, Color k) const { DrawCircleV(c, pass == 0 ? r + ow : r, pass == 0 ? line() : fill(k)); }
  void ellipse(Vector2 c, float rx, float ry, Color k) const {
    if (pass == 0) DrawEllipse(int(c.x), int(c.y), rx + ow, ry + ow, line());
    else DrawEllipse(int(c.x), int(c.y), rx, ry, fill(k));
  }
  void tri(Vector2 a, Vector2 b, Vector2 c, Color k) const {
    float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross > 0) std::swap(b, c);
    if (pass == 0) {
      Vector2 m{(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3};
      auto grow = [&](Vector2 p) {
        Vec2 d = W(p) - W(m);
        float l = d.len();
        return l > 0.01f ? V(W(p) + d * ((l + ow * 1.7f) / l) - d) : p;
      };
      DrawTriangle(grow(a), grow(b), grow(c), line());
    } else {
      DrawTriangle(a, b, c, fill(k));
    }
  }
};

struct Xf {
  Vec2 center;
  int facing;
  float scale, rot, squash;
  Vector2 operator()(Vec2 l) const {
    Vec2 v{l.x * facing * scale, l.y * scale * squash};
    return V(center + rotate(v, rot));
  }
};

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

void drawEyes(const Painter& P, const Xf& X, Vec2 head, float r, const Pose& pose, float k, Color iris) {
  if (P.pass == 0) return;
  Color dark = P.fill({25, 20, 35, 255});
  Vec2 eyes[2] = {head + Vec2{0.18f * r, -0.08f * r}, head + Vec2{0.62f * r, -0.06f * r}};
  for (int i = 0; i < 2; ++i) {
    Vector2 e = X(eyes[i]);
    float s = X.scale * k * (i == 0 ? 1.0f : 0.85f);
    if (pose.hurt) {
      DrawLineEx({e.x - 4 * s, e.y - 4 * s}, {e.x + 4 * s, e.y + 4 * s}, 2.5f * X.scale, dark);
      DrawLineEx({e.x - 4 * s, e.y + 4 * s}, {e.x + 4 * s, e.y - 4 * s}, 2.5f * X.scale, dark);
    } else if (pose.blink) {
      DrawLineEx({e.x - 4 * s, e.y}, {e.x + 4 * s, e.y}, 2.0f * X.scale, dark);
    } else {
      DrawEllipse(int(e.x), int(e.y), 0.17f * r * s, 0.27f * r * s, P.fill(WHITE));
      DrawEllipse(int(e.x + 0.06f * r * s * X.facing), int(e.y), 0.1f * r * s, 0.17f * r * s, P.fill(iris));
      DrawCircleV({e.x + 0.03f * r * s * X.facing, e.y - 0.08f * r * s}, 0.04f * r * s, P.fill(WHITE));
    }
  }
}

void drawPuff(Painter& P, const Xf& X, const CharacterDef& d, const Palette& pal, const Pose& pose, float time) {
  const float H = d.height;
  Color body = C(pal.primary), feet = C(pal.secondary);
  Vec2 bc{0.0f, 0.08f * H + pose.crouch * 0.06f * H};
  float R = 0.42f * H;
  auto stubby = [&](Vec2 h) {
    Vec2 rel = h - bc;
    float l = rel.len();
    float maxL = R + 0.12f * H;
    return l > maxL ? bc + rel * (maxL / l) : h;
  };
  auto group = [&](const auto& fn) {
    P.pass = 0;
    fn();
    P.pass = 1;
    fn();
  };
  group([&] { P.ellipse(X(pose.footB), 0.13f * H * X.scale, 0.08f * H * X.scale, shade(feet, 0.8f)); });
  group([&] { P.circle(X(stubby(pose.handB)), 0.1f * H * X.scale, shade(body, 0.85f)); });
  group([&] {
    P.circle(X(bc), R * X.scale, body);
    P.tri(X(bc + polar(-100, R * 0.95f)), X(bc + polar(-75, R * 0.95f)), X(bc + Vec2{-0.02f * H, -R - 0.12f * H}), body);
  });
  group([&] { P.ellipse(X(pose.footF), 0.13f * H * X.scale, 0.08f * H * X.scale, feet); });
  if (pose.weapon && d.weaponLength > 0) group([&] {
    Vec2 hand = stubby(pose.handF);
    Vec2 tip = hand + pose.weaponDir * d.weaponLength;
    Vec2 perp{-pose.weaponDir.y, pose.weaponDir.x};
    P.seg(X(hand), X(tip), 6 * X.scale, {130, 85, 50, 255});
    P.seg(X(tip - perp * 16), X(tip + perp * 16), 26 * X.scale, C(pal.secondary));
  });
  group([&] { P.circle(X(stubby(pose.handF)), 0.1f * H * X.scale, body); });
  P.pass = 1;
  {
    Color dark = P.fill({30, 20, 40, 255});
    Vec2 eyes[2] = {bc + Vec2{0.1f * H, -0.12f * H}, bc + Vec2{0.26f * H, -0.12f * H}};
    for (auto e : eyes) {
      Vector2 p = X(e);
      if (pose.hurt || pose.blink) {
        DrawLineEx({p.x - 7 * X.scale, p.y}, {p.x + 7 * X.scale, p.y}, 3 * X.scale, dark);
      } else {
        DrawEllipse(int(p.x), int(p.y), 0.05f * H * X.scale, 0.1f * H * X.scale, dark);
        DrawEllipse(int(p.x), int(p.y + 0.04f * H * X.scale), 0.035f * H * X.scale, 0.04f * H * X.scale,
                    P.fill(C(pal.secondary)));
        DrawCircleV({p.x, p.y - 0.04f * H * X.scale}, 0.022f * H * X.scale, P.fill(WHITE));
      }
    }
    Vector2 b1 = X(bc + Vec2{0.05f * H, 0.02f * H}), b2 = X(bc + Vec2{0.33f * H, 0.02f * H});
    DrawEllipse(int(b1.x), int(b1.y), 0.05f * H * X.scale, 0.025f * H * X.scale, P.fill({255, 110, 150, 160}));
    DrawEllipse(int(b2.x), int(b2.y), 0.04f * H * X.scale, 0.025f * H * X.scale, P.fill({255, 110, 150, 160}));
    Vector2 mo = X(bc + Vec2{0.18f * H, 0.04f * H});
    if (pose.hurt) DrawCircleV(mo, 0.03f * H * X.scale, dark);
    else DrawLineEx({mo.x - 3 * X.scale, mo.y}, {mo.x + 3 * X.scale, mo.y + 2 * X.scale}, 2 * X.scale, dark);
  }
  (void)time;
}
}  // namespace

void drawCharacter(const CharacterDef& d, int palette, const Pose& pose, Vec2 feet, int facing, float scale,
                   float alpha, Color flashColor, float flash, float time) {
  const Palette& pal = d.palettes[palette & 1];
  const float H = d.height;
  Xf X;
  X.facing = facing;
  X.scale = scale;
  X.squash = pose.squash;
  X.rot = pose.rot * facing;
  X.center = feet + Vec2{pose.shift.x * facing * scale, -H * 0.5f * scale * pose.squash + pose.shift.y * scale};

  Painter P;
  P.ow = std::max(1.5f, 3.0f * scale * (H / 100.0f));
  P.alpha = alpha;
  P.flashCol = flashColor;
  P.flash = clampf(flash, 0.0f, 1.0f);

  if (d.style == BodyStyle::Puff) {
    drawPuff(P, X, d, pal, pose, time);
    return;
  }

  const Dims dm = dimsFor(d.style);
  const float hr = dm.headR * H;
  const float drop = pose.crouch * 0.14f * H;
  const Vec2 hips{0.0f, 0.10f * H + drop};
  const Vec2 neck = hips + rotate({0.0f, -0.27f * H}, pose.lean);
  const Vec2 head = neck + rotate({0.0f, -hr * 0.9f}, pose.lean * 1.3f);
  const Vec2 shF = neck + rotate({0.035f * H, 0.04f * H}, pose.lean);
  const Vec2 shB = neck + rotate({-0.035f * H, 0.04f * H}, pose.lean);
  const Vec2 hipF = hips + Vec2{0.035f * H, 0.0f}, hipB = hips + Vec2{-0.035f * H, 0.0f};
  const float ua = 0.155f * H, thigh = 0.205f * H;
  Vec2 handF, handB, footF, footB;
  const Vec2 elbowF = ik(shF, pose.handF, ua, ua, 1.0f, handF);
  const Vec2 elbowB = ik(shB, pose.handB, ua, ua, 1.0f, handB);
  const Vec2 kneeF = ik(hipF, pose.footF, thigh, thigh, -1.0f, footF);
  const Vec2 kneeB = ik(hipB, pose.footB, thigh, thigh, -1.0f, footB);
  const float s = scale;

  Color primary = C(pal.primary), secondary = C(pal.secondary), accent = C(pal.accent), skin = C(pal.skin);
  Color torsoC = primary, armC = primary, handC = skin, legC = secondary, footC{90, 55, 35, 255}, headC = skin;
  Color hair = shade(primary, 0.45f);
  switch (d.style) {
    case BodyStyle::Brawler: handC = WHITE; break;
    case BodyStyle::Speedster: armC = handC = legC = footC = headC = primary; break;
    case BodyStyle::Brute: armC = handC = legC = skin; footC = secondary; break;
    case BodyStyle::Knight: torsoC = secondary; legC = shade(primary, 0.6f); footC = secondary; break;
    case BodyStyle::Ninja: armC = handC = legC = footC = secondary; headC = primary; break;
    default: break;
  }
  const float sway = std::sin(time * 6.0f);

  auto group = [&](const auto& fn) {
    P.pass = 0;
    fn();
    P.pass = 1;
    fn();
  };
  group([&] {
    // --- Back accessories ---------------------------------------------------
    if (d.style == BodyStyle::Knight) {  // cape
      Vec2 c0 = neck + Vec2{-0.03f * H, 0.0f}, c1 = neck + Vec2{0.02f * H, 0.02f * H};
      Vec2 c2 = hips + Vec2{-0.26f * H - sway * 0.03f * H, 0.3f * H}, c3 = hips + Vec2{-0.06f * H, 0.34f * H};
      P.tri(X(c0), X(c2), X(c3), shade(primary, 0.75f));
      P.tri(X(c0), X(c1), X(c3), shade(primary, 0.75f));
    }
    if (d.style == BodyStyle::Speedster) {  // lightning tail
      Vec2 t0 = hips + Vec2{-0.1f * H, 0.0f};
      Vec2 t1 = t0 + Vec2{-0.14f * H, -0.12f * H + sway * 0.02f * H};
      Vec2 t2 = t1 + Vec2{0.06f * H, -0.1f * H};
      Vec2 t3 = t2 + Vec2{-0.18f * H, -0.2f * H + sway * 0.03f * H};
      P.seg(X(t0), X(t1), 0.06f * H * s, primary);
      P.seg(X(t1), X(t2), 0.07f * H * s, primary);
      P.seg(X(t2), X(t3), 0.1f * H * s, primary);
      P.tri(X(t3 + Vec2{0.02f * H, 0.04f * H}), X(t3 + Vec2{-0.08f * H, -0.08f * H}), X(t3 + Vec2{0.06f * H, -0.03f * H}), accent);
    }
    if (d.style == BodyStyle::Ninja) {  // scarf
      Vec2 a = neck + Vec2{-0.02f * H, 0.01f * H};
      Vec2 b = a + Vec2{-0.17f * H, 0.03f * H + sway * 0.04f * H};
      Vec2 c = b + Vec2{-0.17f * H, -0.02f * H - sway * 0.05f * H};
      P.seg(X(a), X(b), 0.06f * H * s, accent);
      P.seg(X(b), X(c), 0.05f * H * s, accent);
    }
    if (d.style == BodyStyle::Knight) {  // hair + ponytail behind the head
      P.circle(X(head + Vec2{-0.15f * hr, -0.2f * hr}), hr * 1.02f * s, hair);
      P.seg(X(head + Vec2{-0.7f * hr, -0.5f * hr}), X(head + Vec2{-1.9f * hr, 0.7f * hr + sway * 0.2f * hr}),
            0.08f * H * s, hair);
    }

  });
  group([&] {
    // --- Back limbs -----------------------------------------------------------
    P.seg(X(hipB), X(kneeB), dm.legTh * H * s, shade(legC, 0.78f));
    P.seg(X(kneeB), X(footB), dm.legTh * H * s, shade(legC, 0.78f));
    P.ellipse(X(footB + Vec2{0.02f * H, -0.01f * H}), dm.footR * 1.4f * H * s, dm.footR * H * s, shade(footC, 0.78f));
  });
  group([&] {
    P.seg(X(shB), X(elbowB), dm.armTh * H * s, shade(armC, 0.78f));
    P.seg(X(elbowB), X(handB), dm.armTh * H * s, shade(armC, 0.78f));
    P.circle(X(handB), dm.handR * H * s, shade(handC, 0.78f));

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
    if (P.pass == 1) {
      Vec2 mid = lerpv(hips, neck, 0.45f);
      if (d.style == BodyStyle::Brute)
        P.ellipse(X(mid + Vec2{0.06f * H, 0.02f * H}), 0.09f * H * s, 0.12f * H * s, accent);
      if (d.style == BodyStyle::Brawler || d.style == BodyStyle::Knight || d.style == BodyStyle::Ninja)
        P.seg(X(hips + Vec2{-0.09f * H, -0.02f * H}), X(hips + Vec2{0.09f * H, -0.02f * H}), 0.04f * H * s,
              d.style == BodyStyle::Brawler ? shade(secondary, 0.8f) : accent);
    }

  });
  group([&] {
    // --- Front leg --------------------------------------------------------------
    P.seg(X(hipF), X(kneeF), dm.legTh * H * s, legC);
    P.seg(X(kneeF), X(footF), dm.legTh * H * s, legC);
    P.ellipse(X(footF + Vec2{0.02f * H, -0.01f * H}), dm.footR * 1.4f * H * s, dm.footR * H * s, footC);

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
    if (d.style == BodyStyle::Brawler) {  // flame hair
      for (int i = 0; i < 4; ++i) {
        float a0 = -170.0f + i * 38.0f;
        Vec2 b0 = head + polar(a0, hr * 0.85f), b1 = head + polar(a0 + 45.0f, hr * 0.85f);
        Vec2 tip = head + polar(a0 + 10.0f, hr * (1.75f + 0.12f * std::sin(time * 9.0f + i))) + Vec2{-0.3f * hr, 0.0f};
        P.tri(X(b0), X(b1), X(tip), i % 2 ? accent : primary);
      }
    }
    if (d.style == BodyStyle::Brute) {  // horns
      P.tri(X(head + polar(-150, hr)), X(head + polar(-115, hr)), X(head + Vec2{-1.25f * hr, -1.55f * hr}), accent);
      P.tri(X(head + polar(-70, hr)), X(head + polar(-35, hr)), X(head + Vec2{1.05f * hr, -1.6f * hr}), accent);
    }
    if (P.pass == 1) {
      if (d.style == BodyStyle::Ninja)  // eye band of the hood
        P.ellipse(X(head + Vec2{0.42f * hr, -0.08f * hr}), 0.6f * hr * s, 0.3f * hr * s, skin);
      drawEyes(P, X, head, hr, pose, 1.0f, d.style == BodyStyle::Brute ? Color{200, 40, 30, 255} : Color{30, 30, 50, 255});
      if (d.style == BodyStyle::Speedster) P.circle(X(head + Vec2{0.55f * hr, 0.45f * hr}), 0.18f * hr * s, accent);
      if (d.style == BodyStyle::Knight) P.tri(X(head + polar(-110, hr * 0.9f)), X(head + polar(-60, hr * 0.9f)), X(head + polar(-80, hr * 1.4f)), accent);
      if (d.style == BodyStyle::Brute) {
        Vector2 m0 = X(head + Vec2{0.55f * hr, 0.5f * hr});
        DrawTriangle(m0, {m0.x + 3 * s * facing, m0.y - 6 * s}, {m0.x + 6 * s * facing, m0.y}, P.fill(WHITE));
      }
      if (d.style != BodyStyle::Ninja && pose.hurt) P.circle(X(head + Vec2{0.55f * hr, 0.5f * hr}), 0.12f * hr * s, {60, 20, 30, 255});
    }
  });
  group([&] {
    if (d.style == BodyStyle::Ninja) {  // headband tails
      Vec2 k0 = head + Vec2{-0.85f * hr, -0.3f * hr};
      P.seg(X(k0), X(k0 + Vec2{-0.5f * hr, 0.3f * hr + sway * 0.2f * hr}), 0.035f * H * s, accent);
      P.seg(X(k0), X(k0 + Vec2{-0.6f * hr, -0.05f * hr - sway * 0.15f * hr}), 0.03f * H * s, accent);
    }

  });
  group([&] {
    // --- Front arm + weapon ------------------------------------------------------
    if (pose.weapon && d.weaponLength > 0) {
      Vec2 dir = pose.weaponDir.norm();
      Vec2 perp{-dir.y, dir.x};
      Vec2 tip = handF + dir * d.weaponLength;
      if (d.style == BodyStyle::Knight) {
        P.seg(X(handF), X(tip), 0.035f * H * s, {225, 232, 245, 255});
        P.seg(X(handF - perp * 9.0f), X(handF + perp * 9.0f), 0.03f * H * s, accent);
        if (P.pass == 1) DrawLineEx(X(handF + dir * 8.0f), X(tip - dir * 4.0f), 1.5f * s, P.fill(WHITE));
      } else {
        P.seg(X(handF), X(tip), 0.03f * H * s, {70, 70, 85, 255});
        P.circle(X(handF - dir * 6.0f), 0.025f * H * s, accent);
      }
    }
    P.seg(X(shF), X(elbowF), dm.armTh * H * s, armC);
    P.seg(X(elbowF), X(handF), dm.armTh * H * s, armC);
    P.circle(X(handF), dm.handR * H * s, handC);
  });
}
