#pragma once
#include <array>
#include <vector>

#include "game/audio.hpp"
#include "game/common.hpp"

// Visual + audio feedback driven by the simulation's events.
class Fx {
 public:
  explicit Fx(Audio* audio) : audio_(audio) {}
  void onEvents(const Match& m);
  void update(const Match& m);
  void drawWorld() const;
  void drawScreen(const Camera2D& cam) const;
  Vec2 shakeOffset();
  std::array<float, 4> hudShake{};

 private:
  enum class PK : uint8_t { Spark, Dot, Smoke, Ring, Star, Shard };
  struct Particle {
    Vec2 pos, vel;
    float life = 0, maxLife = 1;
    float size = 4, size1 = 0;
    float rot = 0, rotVel = 0, gravity = 0, drag = 0.92f;
    Color color = WHITE;
    PK kind = PK::Dot;
  };
  struct KoBeam {
    Vec2 pos, dir;
    Color color;
    int t = 0;
  };
  void add(PK kind, Vec2 pos, Vec2 vel, float life, float size, float size1, Color c, float gravity = 0.0f);
  void burst(Vec2 pos, int count, float speed, Color c, PK kind, float life, float size);
  void dust(Vec2 feet, int count, float spread);

  std::vector<Particle> parts_;
  std::vector<KoBeam> kos_;
  float shake_ = 0.0f;
  float flash_ = 0.0f;
  Color flashCol_ = WHITE;
  Rng rng_{4242};
  Audio* audio_;
};
