#pragma once
#include <array>
#include <string>
#include <vector>

#include "game/audio.hpp"
#include "game/common.hpp"

// Visual + audio feedback driven by the simulation's events.
class Fx {
 public:
  explicit Fx(Audio* audio) : audio_(audio) {}
  void onEvents(const Match& m);
  void update(const Match& m);
  void drawBack(const Match& m, float time) const;  // behind the fighters: ki auras, pillars of light
  void drawWorld() const;                            // in front of everything in the world: particles
  void drawScreen(const Camera2D& cam) const;        // screen space: KO beams, weapon names, flashes
  Vec2 shakeOffset();
  std::array<float, 4> hudShake{};

  // Anime "impact frames": a few stark black / white frames when a signature hit (or a KO) lands.
  struct Impact {
    int t = 0, len = 0;  // current frame, number of frames
    Vec2 pos, dir;       // impact point (world), launch direction
    unsigned mask = 0;   // fighters drawn as silhouettes (bit = fighter id)
    int attacker = -1;   // its projectiles / beam are part of the picture
    HitFx fx = HitFx::Normal;
    bool ko = false;
    uint32_t seed = 0;
  };
  const Impact* impact() const { return impact_.t >= 0 && impact_.t < impact_.len ? &impact_ : nullptr; }
  float zoomPunch() const { return punch_; }  // extra camera zoom (0 = none)
  Vec2 punchFocus() const { return punchPos_; }

 private:
  enum class PK : uint8_t {
    Spark, Dot, Smoke, Ring, Star, Shard, Hex,
    Mote, Flash, Burst, Ember, Wisp,  // additive (light)
    Streak,    // slash streak through pos, angle `rot` (deg): length size, width size1
    Petal,     // tumbling blade petal
    Leaf,      // green leaf
    Gust,      // arc of wind, radius size -> size1
    Ice,       // ice shard, falls
    Zap,       // jagged lightning, angle `rot`: length size, width size1, core color2
    Cut,       // slash mark hanging in the air, angle `rot`: length size, width size1
    Drop,      // droplet stretched by its speed
    Fireball,  // ball of an explosion: color (hot) -> orange -> color2 (smoke)
    Debris,    // chunk of rock / metal, falls
    Shock,     // flat shock wave along the ground, half width size -> size1
  };
  struct Particle {
    Vec2 pos, vel;
    float life = 0, maxLife = 1;
    float size = 4, size1 = 0;
    float rot = 0, rotVel = 0, gravity = 0, drag = 0.92f;
    Color color = WHITE, color2 = BLANK;
    uint32_t seed = 0;
    PK kind = PK::Dot;
  };
  struct KoBeam {
    Vec2 pos, dir;
    Color color;
    int t = 0;
  };
  struct Pillar {  // light from the sky guiding a weapon down
    int character = 0, slot = 0;
    Vec2 pos;
    int t = 0, landT = -1;
    Color color, glow;
  };
  struct FloatText {  // weapon name rising above whoever picked it up
    Vec2 pos;
    std::string text;
    Color color;
    int t = 0;
  };
  Particle& add(PK kind, Vec2 pos, Vec2 vel, float life, float size, float size1, Color c, float gravity = 0.0f,
                float drag = 0.92f);
  void burst(Vec2 pos, int count, float speed, Color c, PK kind, float life, float size);
  void dust(Vec2 feet, int count, float spread);
  void motes(Vec2 pos, int count, float radius, float speed, float rise, Color col = kZonaiGlow);
  void hitSpark(Vec2 pos, Vec2 dir, float kb, HitFx fx, bool big);
  void elementBurst(Vec2 pos, Vec2 dir, HitFx fx, float heavy);
  void explosion(Vec2 pos, float r, HitFx fx);
  void startImpact(const Match& m, Vec2 pos, Vec2 dir, int attacker, int victim, HitFx fx, int len, bool ko);
  void hitSound(HitFx fx, float kb, bool big);
  void projectileTrails(const Match& m);
  Vec2 rdir() { const float a = rng_.range(0, 2 * kPi); return {std::cos(a), std::sin(a)}; }

  std::vector<Particle> parts_;
  Particle scratch_;  // returned by add() when the pool is full
  std::vector<KoBeam> kos_;
  std::vector<Pillar> pillars_;
  std::vector<FloatText> texts_;
  struct Combo {  // hits chained on a fighter that never got out of hitstun in between
    int attacker = -1, hits = 0, lastHit = -1000, since = 0;
    bool live = false;
    float base = 0.0f, damage = 0.0f;  // percent before the first hit, damage dealt since
    Vec2 pos;
    Color color = WHITE;
  };
  std::array<Combo, 4> combos_{};
  std::array<int, 4> stunnedAt_{-10, -10, -10, -10};  // last frame each fighter was seen stunned
  std::array<float, 4> calmPercent_{};                // its percent the last time it was not
  std::array<int, 4> fsWave_{-1, -1, -1, -1};  // last final smash shock wave burst, per fighter
  std::array<int, 4> beamMark_{-1, -1, -1, -1};  // beam whose start was already heard, per fighter
  Impact impact_;
  int lastImpact_ = -1000;
  float punch_ = 0.0f;
  Vec2 punchPos_;
  float shake_ = 0.0f;
  float flash_ = 0.0f;
  Color flashCol_ = WHITE;
  Rng rng_{4242};
  Audio* audio_;
};
