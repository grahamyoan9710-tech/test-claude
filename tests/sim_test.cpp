// Headless tests for the simulation: physics, combat, recovery and full CPU matches.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "core/ai.hpp"
#include "core/match.hpp"

static int g_failures = 0;
#define CHECK(cond, ...)                                     \
  do {                                                       \
    if (!(cond)) {                                           \
      std::printf("  FAIL %s:%d: %s -- ", __FILE__, __LINE__, #cond); \
      std::printf(__VA_ARGS__);                              \
      std::printf("\n");                                     \
      ++g_failures;                                          \
    }                                                        \
  } while (0)

static MatchConfig duel(int stage, int c0, int c1, bool cpu0, bool cpu1, int stocks = 3) {
  MatchConfig cfg;
  cfg.stage = stage;
  cfg.stocks = stocks;
  cfg.slots[0] = {true, c0, cpu0, 9};
  cfg.slots[1] = {true, c1, cpu1, 9};
  return cfg;
}

static void skipCountdown(Match& m) {
  std::array<InputState, 4> none{};
  while (!m.started()) m.step(none);
}

static bool sane(const Match& m) {
  for (const auto& f : m.fighters) {
    if (!std::isfinite(f.pos.x) || !std::isfinite(f.pos.y) || !std::isfinite(f.percent)) return false;
    if (std::fabs(f.pos.x) > 5000 || std::fabs(f.pos.y) > 5000) return false;
  }
  return true;
}

static void testKnockback() {
  std::printf("[knockback]\n");
  float a = computeKnockback(10, 10, 100, 100, 30);
  float b = computeKnockback(100, 10, 100, 100, 30);
  float c = computeKnockback(100, 10, 130, 100, 30);
  CHECK(b > a, "more percent -> more knockback (%f vs %f)", b, a);
  CHECK(c < b, "heavier -> less knockback (%f vs %f)", c, b);
}

static void testJabHits() {
  std::printf("[jab]\n");
  Match m(duel(0, 0, 0, false, false));
  skipCountdown(m);
  m.fighters[0].pos = {0, 0};
  m.fighters[0].facing = 1;
  m.fighters[1].pos = {45, 0};
  std::array<InputState, 4> in{};
  for (int i = 0; i < 20; ++i) m.step(in);
  in[0].attack = true;
  m.step(in);
  in[0].attack = false;
  for (int i = 0; i < 20; ++i) m.step(in);
  CHECK(m.fighters[1].percent > 0.0f, "victim percent %f", m.fighters[1].percent);
  CHECK(m.fighters[0].percent == 0.0f, "attacker untouched");
}

static void testSmashKills() {
  std::printf("[smash kill]\n");
  Match m(duel(0, 2, 0, false, false));
  skipCountdown(m);
  m.fighters[0].pos = {200, 0};
  m.fighters[0].facing = 1;
  m.fighters[1].pos = {270, 0};
  m.fighters[1].percent = 150;
  std::array<InputState, 4> in{};
  for (int i = 0; i < 10; ++i) m.step(in);
  in[0].cstick = {1, 0};
  m.step(in);
  in[0].cstick = {0, 0};
  int stocks = m.fighters[1].stocks;
  for (int i = 0; i < 300; ++i) m.step(in);
  CHECK(m.fighters[1].stocks < stocks || m.fighters[1].state == FState::Dead || m.fighters[1].state == FState::Respawn,
        "Titan fsmash should KO at 150%% (pos %.0f,%.0f)", m.fighters[1].pos.x, m.fighters[1].pos.y);
}

// Drives fighter 0 like a keyboard player (digital inputs).
struct Pad {
  Match& m;
  std::array<InputState, 4> in{};
  void run(int frames, InputState s) {
    for (int i = 0; i < frames; ++i) {
      in[0] = s;
      m.step(in);
    }
  }
  void idle(int frames) { run(frames, InputState{}); }
};

static InputState key(float x, float y, bool attack = false, bool jump = false, bool shield = false,
                      bool special = false, bool grab = false, bool smash = false) {
  InputState s;
  s.stick = {x, y};
  s.attack = attack;
  s.jump = jump;
  s.shield = shield;
  s.special = special;
  s.grab = grab;
  s.smash = smash;
  return s;
}

static void testKeyboardControls() {
  std::printf("[keyboard controls]\n");
  auto fresh = [](int stage = 0) {
    Match* m = new Match(duel(stage, 0, 2, false, false));
    skipCountdown(*m);
    m->fighters[0].pos = {-200, 0};
    m->fighters[1].pos = {300, 0};
    std::array<InputState, 4> none{};
    for (int i = 0; i < 30; ++i) m->step(none);
    return m;
  };
  auto moveAfter = [&](std::vector<std::pair<int, InputState>> script) {
    Match* m = fresh();
    Pad p{*m};
    for (auto& [n, s] : script) p.run(n, s);
    int mv = m->fighters[0].state == FState::Attack ? m->fighters[0].move : -1;
    delete m;
    return mv;
  };
  // Jumps
  {
    Match* m = fresh();
    Pad p{*m};
    float y0 = m->fighters[0].pos.y, minShort = y0, minFull = y0;
    p.run(2, key(0, 0, false, true));
    for (int i = 0; i < 60; ++i) {
      p.idle(1);
      minShort = std::min(minShort, m->fighters[0].pos.y);
    }
    p.run(40, key(0, 0, false, true));
    for (int i = 0; i < 40; ++i) {
      p.idle(1);
      minFull = std::min(minFull, m->fighters[0].pos.y);
    }
    for (int i = 0; i < 40; ++i) p.run(1, key(0, 0, false, true));
    CHECK(minFull < minShort - 40.0f, "full hop (%.0f) higher than short hop (%.0f)", y0 - minFull, y0 - minShort);
    delete m;
  }
  CHECK(moveAfter({{1, key(1, 0, true)}}) == FSmash, "right+attack together = forward smash");
  CHECK(moveAfter({{6, key(1, 0)}, {1, key(1, 0, true)}}) == FTilt, "hold right then attack = forward tilt");
  CHECK(moveAfter({{20, key(1, 0)}, {1, key(1, 0, true)}}) == DashAttack, "run then attack = dash attack");
  CHECK(moveAfter({{1, key(0, -1, true)}}) == USmash, "up+attack together = up smash");
  CHECK(moveAfter({{6, key(0, -1)}, {1, key(0, -1, true)}}) == UTilt, "hold up then attack = up tilt");
  CHECK(moveAfter({{1, key(0, 1, true)}}) == DSmash, "down+attack together = down smash");
  CHECK(moveAfter({{8, key(0, 1)}, {1, key(0, 1, true)}}) == DTilt, "crouch then attack = down tilt");
  CHECK(moveAfter({{1, key(0, 0, true)}}) == Jab, "attack = jab");
  CHECK(moveAfter({{1, key(0, 0, false, false, false, false, false, true)}}) == FSmash, "smash key = forward smash");
  CHECK(moveAfter({{1, key(0, -1, false, false, false, false, false, true)}}) == USmash, "smash key + up = up smash");
  CHECK(moveAfter({{1, key(0, -1, false, false, false, true)}}) == USpecial, "up+special = up special");
  CHECK(moveAfter({{1, key(1, 0, false, false, false, true)}}) == SSpecial, "side+special = side special");
  // Aerials (fighter faces right)
  auto aerial = [&](InputState s) {
    return moveAfter({{1, key(0, 0, false, true)}, {12, key(0, 0, false, true)}, {1, s}});
  };
  CHECK(aerial(key(0, 0, true)) == NAir, "neutral air");
  CHECK(aerial(key(1, 0, true)) == FAir, "forward air");
  CHECK(aerial(key(-1, 0, true)) == BAir, "back air");
  CHECK(aerial(key(0, 1, true)) == DAir, "down air");
  CHECK(aerial(key(0, -1, true)) == UAir, "up air");
  // Shield options
  {
    Match* m = fresh();
    Pad p{*m};
    p.run(5, key(0, 0, false, false, true));
    CHECK(m->fighters[0].state == FState::Shield, "shield held");
    p.run(1, key(-1, 0, false, false, true));
    CHECK(m->fighters[0].state == FState::Roll, "shield + direction = roll (state %d)", int(m->fighters[0].state));
    p.idle(40);
    p.run(5, key(0, 0, false, false, true));
    p.run(1, key(0, 1, false, false, true));
    CHECK(m->fighters[0].state == FState::SpotDodge, "shield + down = spot dodge");
    delete m;
  }
  // Grab and throw
  {
    Match* m = fresh();
    m->fighters[1].pos = {-150, 0};
    Pad p{*m};
    p.idle(5);
    p.run(1, key(0, 0, false, false, false, false, true));
    p.idle(10);
    CHECK(m->fighters[0].state == FState::Holding, "grab connects (state %d)", int(m->fighters[0].state));
    p.run(8, key(0, -1));
    p.idle(40);
    CHECK(m->fighters[1].percent > 0.0f, "up throw deals damage");
    delete m;
  }
  // Ledge: fall next to the ledge, grab it, climb
  {
    Match* m = fresh();
    Fighter& f = m->fighters[0];
    f.pos = {-470, -40};
    f.grounded = false;
    f.state = FState::Air;
    Pad p{*m};
    for (int i = 0; i < 60 && f.state != FState::LedgeHang; ++i) p.idle(1);
    CHECK(f.state == FState::LedgeHang, "ledge grabbed (state %d pos %.0f,%.0f)", int(f.state), f.pos.x, f.pos.y);
    p.idle(10);
    p.run(1, key(1, 0));
    p.idle(40);
    CHECK(f.grounded && f.groundPlat == 0 && f.pos.x > -440.0f, "climbed on stage (%.0f,%.0f)", f.pos.x, f.pos.y);
    delete m;
  }
  // Drop through a soft platform
  {
    Match* m = new Match(duel(1, 0, 2, false, false));
    skipCountdown(*m);
    Fighter& f = m->fighters[0];
    f.pos = {-210, -160};
    f.grounded = false;
    f.groundPlat = -1;
    f.state = FState::Air;
    Pad p{*m};
    p.idle(30);
    CHECK(f.grounded && f.groundPlat == 1, "standing on the soft platform");
    p.run(1, key(0, 1));
    p.idle(20);
    CHECK(f.pos.y > -100.0f, "dropped through (y=%.0f)", f.pos.y);
    delete m;
  }
}

static void testCombos() {
  std::printf("[combos and Smash Bros. controls]\n");
  auto fresh = [] {
    Match* m = new Match(duel(0, 8, 2, false, false));
    skipCountdown(*m);
    m->fighters[0].pos = {-200, 0};
    m->fighters[0].facing = 1;
    m->fighters[1].pos = {300, 0};
    std::array<InputState, 4> none{};
    for (int i = 0; i < 30; ++i) m->step(none);
    return m;
  };
  auto run = [&](std::vector<std::pair<int, InputState>> script, Fighter* out = nullptr) {
    Match* m = fresh();
    Pad p{*m};
    for (auto& [n, s] : script) p.run(n, s);
    if (out) *out = m->fighters[0];
    int mv = m->fighters[0].state == FState::Attack ? m->fighters[0].move : -1;
    delete m;
    return mv;
  };
  CHECK(run({{1, key(0, 0, true, false, false, true)}}) == FSmash, "attack + special together = forward smash");
  CHECK(run({{1, key(0, -1, true, false, false, true)}}) == USmash, "up + attack + special = up smash");
  CHECK(run({{1, key(0, 0, true)}, {1, key(0, 0, true, false, false, true)}}) == FSmash,
        "special a frame after the attack still makes the smash");
  Fighter f{};
  CHECK(run({{2, key(0, 1)}, {2, key(1, 1)}, {1, key(1, 0)}, {1, key(1, 0, true)}}, &f) == FSmash,
        "quarter circle forward + attack = forward smash");
  CHECK(f.charge > 0 && !f.charging, "... at full power without charging (charge %d)", f.charge);
  CHECK(run({{2, key(0, 1)}, {2, key(-1, 1)}, {1, key(-1, 0)}, {1, key(-1, 0, true)}}, &f) == FSmash && f.facing < 0,
        "the quarter circle the other way turns around");
  CHECK(run({{3, key(0, 1)}, {1, key(0, -1)}, {1, key(0, -1, true)}}, &f) == USmash && f.charge > 0,
        "down, up + attack = up smash at full power");
  CHECK(run({{3, key(0, 1)}, {3, key(1, 1)}, {14, key(1, 0)}, {1, key(1, 0, true)}}, &f) == FSmash && f.charge > 0,
        "a slow quarter circle (forward held 14 frames) still charges the smash");
  CHECK(run({{2, key(0, 1)}, {2, key(1, 1)}, {2, key(1, 0)}, {2, key(0, 0)}, {1, key(0, 0, true)}}, &f) == FSmash &&
            f.charge > 0,
        "the stick let go 2 frames before the button still charges the smash");
  CHECK(run({{8, key(1, 0)}, {1, key(1, 0, true)}}) == FTilt, "holding forward then attack stays a tilt");
  InputState up = key(0, -1);
  up.tapJump = true;
  run({{1, up}, {12, up}}, &f);
  CHECK(!f.grounded, "the stick flicked up jumps (tap jump)");
  run({{1, key(0, -1)}, {12, key(0, -1)}}, &f);
  CHECK(f.grounded, "no tap jump without the setting (keyboards)");
  InputState upTilt = key(0, -1, true);
  upTilt.tapJump = true;
  CHECK(run({{6, key(0, -0.4f)}, {1, upTilt}}) == UTilt, "tap jump still allows the up tilt");
  // Up jumps and attacks upward, as in Smash Bros.: the jump's crouch turns into the up attack.
  auto pad = [](float y, bool attack = false, bool special = false) {
    InputState s = key(0, y, attack, false, false, special);
    s.tapJump = true;
    return s;
  };
  auto keys = [&](float y, bool attack = false, bool special = false) {
    InputState s = pad(y, attack, special);
    s.digital = true;
    return s;
  };
  CHECK(run({{1, pad(-1)}, {1, pad(-1, true)}}) == USmash, "stick up then attack in the jump's crouch = up smash");
  CHECK(run({{1, pad(-1)}, {1, pad(-1, false, true)}}) == USpecial, "stick up then special = up special");
  CHECK(run({{1, keys(-1, true)}}) == USmash, "Up + attack together (keys) = up smash");
  CHECK(run({{1, keys(-1)}, {2, keys(-1)}, {1, keys(-1, true)}}) == UTilt, "Up then attack a little after (keys) = up tilt");
  CHECK(run({{1, keys(-1)}, {1, keys(-1, false, true)}}) == USpecial, "Up then special (keys) = up special");
  run({{1, keys(-1)}, {14, keys(-1)}}, &f);
  CHECK(!f.grounded, "Up alone (keys) jumps");
  run({{8, key(0, 0, false, false, true)}, {1, pad(-1)}, {12, pad(-1)}}, &f);
  CHECK(!f.grounded, "stick up out of the shield jumps");
}

// Jabs and tilts: the stick moves the fighter a little while it hits, without turning it or taking it off an edge.
static void testAttackStep() {
  std::printf("[moving while attacking]\n");
  // Presses attack once at x (stick neutral), then holds the stick at sx while the jab plays out.
  auto jab = [](float x, float sx, Fighter* out) {
    Match m(duel(0, 8, 2, false, false));
    skipCountdown(m);
    m.fighters[0].pos = {x, 0};
    m.fighters[0].facing = 1;
    m.fighters[1].pos = {x - 700.0f, 0};
    Pad p{m};
    p.idle(30);
    const float x0 = m.fighters[0].pos.x;
    p.run(1, key(0, 0, true));
    const int move = m.fighters[0].move;
    for (int i = 0; i < 40 && m.fighters[0].state == FState::Attack; ++i) p.run(1, key(sx, 0));
    *out = m.fighters[0];
    return std::pair<int, float>(move, out->pos.x - x0);
  };
  Fighter f{};
  auto [mv, still] = jab(-200.0f, 0.0f, &f);
  CHECK(mv == Jab && std::fabs(still) < 2.0f, "a jab without the stick stays in place (moved %.1f)", still);
  auto [mv2, fwd] = jab(-200.0f, 1.0f, &f);
  CHECK(mv2 == Jab && fwd > 8.0f && f.facing == 1, "the stick forward moves the jab forward (moved %.1f)", fwd);
  auto [mv3, back] = jab(-200.0f, -1.0f, &f);
  CHECK(back < -8.0f && f.facing == 1, "... and back, still facing forward (moved %.1f, facing %d)", back, f.facing);
  Match m(duel(0, 8, 2, false, false));
  float right = 0.0f;
  for (const auto& pl : m.platforms)
    if (pl.solid && pl.rect.top() == 0.0f) right = std::max(right, pl.rect.right());
  jab(right - 6.0f, 1.0f, &f);
  CHECK(f.grounded && f.pos.x <= right, "never off the edge (x %.1f, edge %.1f)", f.pos.x, right);
}

static void testTeams() {
  std::printf("[team battle 2v2]\n");
  MatchConfig cfg;
  cfg.stage = 0;
  cfg.stocks = 2;
  cfg.teams = true;
  cfg.slots[0] = {true, 8, false, 9, 0};
  cfg.slots[1] = {true, 2, true, 9, 0};
  cfg.slots[2] = {true, 9, true, 9, 1};
  cfg.slots[3] = {true, 0, true, 9, 1};
  {  // a teammate's jab does nothing
    Match m(cfg);
    skipCountdown(m);
    Fighter& a = m.fighters[0];
    Fighter& mate = m.fighters[1];
    a.pos = {0, 0};
    a.facing = 1;
    mate.pos = {40, 0};
    m.fighters[2].pos = {-400, 0};
    m.fighters[3].pos = {-500, 0};
    std::array<InputState, 4> in{};
    for (int i = 0; i < 20; ++i) m.step(in);
    const float before = mate.percent;
    Pad p{m};
    for (int i = 0; i < 6; ++i) {
      p.run(1, key(0, 0, true));
      p.idle(12);
    }
    CHECK(mate.percent == before, "no damage to a teammate (%.1f%% -> %.1f%%)", before, mate.percent);
    m.fighters[2].pos = {40, 0};
    m.fighters[2].vel = {};
    p.idle(10);
    const float foe = m.fighters[2].percent;
    for (int i = 0; i < 6; ++i) {
      p.run(1, key(0, 0, true));
      p.idle(12);
    }
    CHECK(m.fighters[2].percent > foe, "the other team takes the hits (%.1f%%)", m.fighters[2].percent);
  }
  {  // CPUs play it out: one team wins, and the ranking starts with it
    MatchConfig c = cfg;
    c.slots[0].cpu = true;
    Match m(c);
    std::vector<CpuBrain> brains;
    for (const auto& f : m.fighters) brains.emplace_back(9, 77u + 13u * uint32_t(f.id));
    std::array<InputState, 4> in{};
    for (int i = 0; i < 60 * 60 * 8 && !m.finished; ++i) {
      for (size_t k = 0; k < m.fighters.size(); ++k) in[m.fighters[k].port] = brains[k].think(m, int(k));
      m.step(in);
    }
    CHECK(m.finished, "the team battle ends");
    CHECK(m.winnerTeam == 0 || m.winnerTeam == 1, "a team wins (%d)", m.winnerTeam);
    int standing = 0;
    for (const auto& f : m.fighters)
      if (!f.eliminated) {
        ++standing;
        CHECK(f.team == m.winnerTeam, "only the winning team is left");
      }
    CHECK(standing >= 1, "someone is left");
    auto rk = m.ranking();
    CHECK(m.fighters[rk[0]].team == m.winnerTeam && m.fighters[rk[1]].team == m.winnerTeam,
          "the winning team is ranked first");
    CHECK(sane(m), "sane");
  }
}

static void testFinalSmash() {
  std::printf("[clash orb + final smash]\n");
  Match m(duel(0, 0, 2, false, false));
  skipCountdown(m);
  Fighter& f = m.fighters[0];
  Fighter& v = m.fighters[1];
  f.pos = {-100, 0};
  f.facing = 1;
  v.pos = {150, 0};
  v.percent = 60;
  m.orb = Orb{};
  m.orb.active = true;
  m.orb.hp = 40;
  m.orb.life = 10000;
  Pad p{m};
  for (int i = 0; i < 400 && !f.finalSmash; ++i) {
    m.orb.pos = {-55, -60};
    m.orb.vel = {0, 0};
    p.run(1, key(0, 0, i % 4 == 0));
  }
  CHECK(f.finalSmash && f.ult >= kUltMax, "breaking the orb fills the ultimate meter (orb hp %.0f, meter %.0f)",
        m.orb.hp, f.ult);
  p.idle(30);
  p.run(1, key(0, -1, false, false, false, true));
  CHECK(f.move != FinalSmash, "an up special does not waste the ultimate");
  p.idle(80);
  p.run(1, key(0, 0, false, false, false, true));
  CHECK(f.move != FinalSmash, "the neutral special with a full meter stays the special (only U unleashes the ultimate)");
  p.idle(80);
  InputState ultKey;
  ultKey.ultimate = true;
  p.run(1, ultKey);
  CHECK(f.state == FState::Attack && f.move == FinalSmash, "the ultimate button unleashes the final smash");
  CHECK(f.ult == 0.0f && !f.finalSmash, "the meter is spent (%.0f)", f.ult);
  CHECK(m.freezeTimer > 0, "cinematic freeze");
  int stocks = v.stocks;
  p.idle(300);
  CHECK(v.stocks < stocks, "final smash KOs a 60%% Titan (percent %.0f)", v.percent);
}

static void testUltMeter() {
  std::printf("[ultimate meter]\n");
  Match m(duel(0, 2, 0, false, false));
  m.cfg.items = false;
  skipCountdown(m);
  Fighter& f = m.fighters[0];
  Fighter& v = m.fighters[1];
  f.pos = {-40, 0};
  f.facing = 1;
  v.pos = {10, 0};
  Pad p{m};
  InputState ult;
  ult.ultimate = true;
  p.run(1, ult);
  CHECK(f.state != FState::Attack, "an empty meter cannot unleash the ultimate");
  p.idle(10);
  float before = v.percent;
  p.run(1, key(0, 0, true));
  p.idle(30);
  float dealt = v.percent - before;
  CHECK(dealt > 0.0f, "jab connects (%.1f%%)", dealt);
  CHECK(std::fabs(f.ult - dealt * kUltPerDamageDealt) < 0.01f, "attacker meter +%.1f for %.1f%% dealt", f.ult, dealt);
  CHECK(std::fabs(v.ult - dealt * kUltPerDamageTaken) < 0.01f, "victim meter +%.1f for %.1f%% taken", v.ult, dealt);
  int readyEvents = 0;
  for (int i = 0; i < 4000 && !f.finalSmash; ++i) {
    v.pos = {10, 0};
    v.vel = v.kbVel = {0, 0};
    v.percent = 0.0f;
    f.pos = {-40, 0};
    f.facing = 1;
    m.step({key(0, 0, i % 12 == 0), InputState{}, InputState{}, InputState{}});
    for (const auto& e : m.events) readyEvents += e.type == EvType::UltReady && e.a == f.id;
  }
  CHECK(f.finalSmash && f.ult == kUltMax, "hitting fills the meter (%.0f)", f.ult);
  CHECK(readyEvents == 1, "one UltReady event (%d)", readyEvents);
  p.idle(40);
  p.run(1, ult);
  CHECK(f.state == FState::Attack && f.move == FinalSmash, "full meter + ultimate button = final smash");
  float vUlt = v.ult;
  p.idle(200);
  CHECK(f.ult == 0.0f, "no meter gained from the final smash itself (%.1f)", f.ult);
  CHECK(v.ult > vUlt, "the victim still charges its meter (%.1f -> %.1f)", vUlt, v.ult);
}

static void testUltEdgeCases() {
  std::printf("[ultimate edge cases + respawns]\n");
  {
    Match m(duel(0, 0, 3, false, false));
    m.cfg.items = false;
    skipCountdown(m);
    Fighter& a = m.fighters[0];
    Fighter& b = m.fighters[1];
    a.ult = kUltMax;
    a.finalSmash = true;
    for (Fighter* f : {&a, &b}) {  // both fall out of the stage on the same frame
      f->pos = {0, 3000};
      f->grounded = false;
      f->groundPlat = -1;
    }
    Pad p{m};
    for (int i = 0; i < 200 && !(a.state == FState::Respawn && b.state == FState::Respawn); ++i) p.idle(1);
    p.idle(20);
    CHECK(a.state == FState::Respawn && b.state == FState::Respawn, "both respawning");
    CHECK(std::fabs(a.pos.x - b.pos.x) >= 100.0f, "side-by-side respawn platforms (x %.0f / %.0f)", a.pos.x, b.pos.x);
    CHECK(a.finalSmash && a.ult == kUltMax, "a KO keeps a full ultimate meter");
  }
  {
    Match m(duel(0, 0, 2, false, false));
    m.cfg.items = false;
    skipCountdown(m);
    Fighter& f = m.fighters[0];
    f.ult = kUltMax;
    f.finalSmash = true;
    InputState ult;
    ult.ultimate = true;
    Pad p{m};
    p.run(2, ult);
    CHECK(f.move == FinalSmash && f.intangible > 0, "the final smash user is intangible (%d)", f.intangible);
  }
}

static int charIndex(const char* name) {
  for (int i = 0; i < int(roster().size()); ++i)
    if (roster()[i].name == name) return i;
  return 0;
}

static void testWeapons() {
  std::printf("[weapons]\n");
  const int kratos = charIndex("KRATOS"), gojo = charIndex("GOJO");
  MatchConfig cfg = duel(0, kratos, gojo, false, false);
  cfg.items = false;
  cfg.weapons = false;  // no random drops: the test drops them itself
  Match m(cfg);
  skipCountdown(m);
  Fighter& f = m.fighters[0];
  Fighter& v = m.fighters[1];
  f.pos = {-200, 0};
  f.facing = 1;
  v.pos = {250, 0};
  Pad p{m};
  m.dropWeaponFor(1, 0);  // Gojo's weapon: Kratos must not be able to take it
  m.items.back().pos = {-150, -300};
  for (int i = 0; i < 120 && !m.items.empty() && !m.items.back().grounded; ++i) p.idle(1);
  CHECK(!m.items.empty() && m.items.back().grounded, "a dropped weapon lands on the stage");
  p.run(30, key(1, 0));
  CHECK(f.weapon < 0 && m.items.size() == 1, "another character's weapon cannot be picked up");
  m.items.clear();
  m.dropWeaponFor(0, 0);  // Leviathan axe
  m.items.back().pos = {f.pos.x + 60, -300};
  for (int i = 0; i < 120 && !m.items.empty() && !m.items.back().grounded; ++i) p.idle(1);
  p.run(20, key(1, 0));
  CHECK(f.weapon == 0 && m.items.empty(), "walking over its own weapon picks it up (weapon %d)", f.weapon);
  p.idle(20);
  f.facing = 1;
  p.run(1, key(0, 0, false, false, false, true));
  CHECK(f.state == FState::Attack && f.moveDef(f.move).name == "Lancer de Léviathan", "the axe replaces the neutral special");
  bool thrown = false, back = false;
  for (int i = 0; i < 200; ++i) {
    p.idle(1);
    if (!m.projectiles.empty()) thrown = true;
    if (thrown && m.projectiles.empty()) {
      back = true;
      break;
    }
  }
  CHECK(thrown && back, "the axe flies and comes back (thrown %d back %d)", thrown, back);
  f.weaponTimer = 2;
  p.idle(3);
  CHECK(f.weapon < 0, "a weapon vanishes after its time");
  // Reach: a held weapon makes the base moves hit harder
  float base = 0.0f, boosted = 0.0f;
  for (int pass = 0; pass < 2; ++pass) {
    Match m2(cfg);
    skipCountdown(m2);
    m2.fighters[0].pos = {-40, 0};
    m2.fighters[0].facing = 1;
    m2.fighters[1].pos = {20, 0};
    if (pass == 1) m2.equipWeapon(m2.fighters[0], 1);
    Pad p2{m2};
    p2.run(1, key(0, 0, true));
    p2.idle(25);
    (pass == 0 ? base : boosted) = m2.fighters[1].percent;
  }
  CHECK(boosted > base * 1.1f, "weapon damage bonus (%.1f vs %.1f)", boosted, base);
}

// Sasuke's Amaterasu: he stands still and cannot attack while the black flames burn, then it needs time to come back.
static void testAmaterasu() {
  std::printf("[amaterasu]\n");
  MatchConfig cfg = duel(0, charIndex("SASUKE"), charIndex("NARUTO"), false, false);
  cfg.items = false;
  cfg.weapons = false;
  for (int air = 0; air < 2; ++air) {
    Match m(cfg);
    skipCountdown(m);
    Fighter& f = m.fighters[0];
    Fighter& v = m.fighters[1];
    f.pos = {-150, air ? -260.0f : 0.0f};
    f.facing = 1;
    v.pos = {150, 0};
    m.equipWeapon(f, 1);  // the Mangekyo Sharingan
    Pad p{m};
    if (!air) p.idle(10);
    p.run(1, key(0, 0, false, false, false, true));
    CHECK(f.state == FState::Attack && f.moveDef(f.move).name == "Amaterasu", "air %d: the neutral special is Amaterasu",
          air);
    const MoveDef& ama = f.moveDef(NSpecial);
    const float x0 = f.pos.x;
    bool held = true, moved = false;
    for (int i = 1; i < ama.duration - 1; ++i) {
      p.run(1, key(1, 0, i % 2 == 0, false, false, i % 3 == 0));  // walks, attacks and casts again: in vain
      held = held && f.state == FState::Attack && f.move == NSpecial;
      moved = moved || std::fabs(f.pos.x - x0) > 0.5f;
    }
    CHECK(held, "air %d: nothing interrupts the technique (state %d, move %d)", air, int(f.state), f.move);
    CHECK(!moved, "air %d: he does not move while casting (x %.1f -> %.1f)", air, x0, f.pos.x);
    if (air) continue;
    CHECK(v.percent > 5.0f, "the black flames burn (%.1f%%)", v.percent);
    p.idle(30);
    p.run(1, key(0, 0, false, false, false, true));
    CHECK(f.state != FState::Attack, "it cannot be cast again right away (state %d, move %d)", int(f.state), f.move);
    p.idle(ama.cooldown - ama.duration - 30);
    p.run(1, key(0, 0, false, false, false, true));
    CHECK(f.state == FState::Attack && f.move == NSpecial, "it comes back after %d frames", ama.cooldown);
  }
}

// With a picked-up weapon the basic attack becomes the weapon's own strike (its clip: w0_jab / w1_jab), and it hits.
static void testWeaponStrikes() {
  std::printf("[weapon strikes]\n");
  const auto& r = roster();
  for (int c = 0; c < int(r.size()); ++c)
    for (int slot = 0; slot < 2; ++slot) {
      MatchConfig cfg = duel(0, c, c == 0 ? 1 : 0, false, false);
      cfg.items = false;
      cfg.weapons = false;
      Match m(cfg);
      skipCountdown(m);
      Fighter& f = m.fighters[0];
      f.pos = {-60, 0};
      f.facing = 1;
      m.fighters[1].pos = {20, 0};
      m.equipWeapon(f, slot);
      Pad p{m};
      p.run(1, key(0, 0, true));
      const std::string name = f.state == FState::Attack ? f.moveDef(f.move).name : "-";
      const bool strike = f.state == FState::Attack && f.move == Jab && f.moveDef(Jab).nextMove < 0 &&
                          f.moveDef(Jab).name != r[c].moves[Jab].name;
      bool hit = false;
      for (int i = 0; i < 60 && !hit; ++i) {
        p.idle(1);
        hit = m.fighters[1].percent > 0.0f;
      }
      CHECK(strike && hit, "%s weapon %d: the jab becomes the weapon's strike and hits (move \"%s\", hit %d)",
            r[c].name.c_str(), slot, name.c_str(), hit);
    }
}

// Every ultimate (bare-handed and with each weapon) hits, lets its victim go, and can KO at high percent.
static void testAllUltimates() {
  std::printf("[every ultimate: 3 per fighter]\n");
  const int n = int(roster().size());
  for (int c = 0; c < n; ++c) {
    for (int slot = -1; slot <= 1; ++slot) {
      for (int pass = 0; pass < 2; ++pass) {  // 0: at 30%, how much damage; 1: at 130%, does it KO
        MatchConfig cfg = duel(0, c, c == 0 ? 1 : 0, false, false);
        cfg.items = false;
        cfg.weapons = false;
        Match m(cfg);
        skipCountdown(m);
        Fighter& f = m.fighters[0];
        Fighter& v = m.fighters[1];
        f.pos = {-120, 0};
        f.facing = 1;
        v.pos = {40, 0};
        v.percent = pass == 0 ? 30.0f : 130.0f;
        if (slot >= 0) m.equipWeapon(f, slot);
        Pad p{m};
        p.idle(10);
        f.ult = kUltMax;
        f.finalSmash = true;
        InputState ult;
        ult.ultimate = true;
        p.run(1, ult);
        const std::string name = f.moveDef(FinalSmash).name;
        CHECK(f.move == FinalSmash, "%s/%d: ultimate starts", roster()[c].name.c_str(), slot);
        const float before = v.percent;
        int stocks = v.stocks, captured = 0, maxCaptured = 0;
        bool sawForm = false;
        float peak = before;
        for (int i = 0; i < 360; ++i) {
          p.idle(1);
          if (v.stocks == stocks) peak = std::max(peak, v.percent);
          captured = v.state == FState::Captured ? captured + 1 : 0;
          maxCaptured = std::max(maxCaptured, captured);
          sawForm = sawForm || f.form >= 0;
          if (!sane(m)) break;
        }
        CHECK(sane(m), "%s: sane", name.c_str());
        CHECK(v.state != FState::Captured, "%s: the victim is released (state %d)", name.c_str(), int(v.state));
        CHECK(maxCaptured < 140, "%s: held %d frames", name.c_str(), maxCaptured);
        if (pass == 0) {
          const float dealt = peak - before;
          CHECK(dealt >= 10.0f || v.stocks < stocks, "%s: deals damage (%.1f%%)", name.c_str(), dealt);
          std::printf("  %-10s %-32s %5.1f%%%s%s%s\n", roster()[c].name.c_str(), name.c_str(), dealt,
                      v.stocks < stocks ? "  KO at 30%" : "", maxCaptured ? "  (capture)" : "",
                      sawForm ? "  (transformation)" : "");
        } else {
          CHECK(v.stocks < stocks || sawForm, "%s: KOs at 130%% (%.0f%%, pos %.0f,%.0f)", name.c_str(), v.percent, v.pos.x,
                v.pos.y);
        }
      }
    }
  }
}

static std::string TextFormatShield(float shield) {
  char buf[64];
  std::snprintf(buf, sizeof buf, "shield left %2.0f%%", 100.0f * shield / kMaxShield);
  return buf;
}

// Ultimates can be blocked: a fighter holding its shield is never caught, takes no damage while the shield
// holds, and the shield pays for it (it is left low, or breaks under the heaviest ones).
static void testUltimatesBlocked() {
  std::printf("[ultimates against a shield]\n");
  const int n = int(roster().size());
  int broken = 0, total = 0;
  for (int c = 0; c < n; ++c)
    for (int slot = -1; slot <= 1; ++slot) {
      MatchConfig cfg = duel(0, c, c == 0 ? 1 : 0, false, false);
      cfg.items = false;
      cfg.weapons = false;
      Match m(cfg);
      skipCountdown(m);
      Fighter& f = m.fighters[0];
      Fighter& v = m.fighters[1];
      f.pos = {-120, 0};
      f.facing = 1;
      v.pos = {40, 0};
      v.percent = 30.0f;
      if (slot >= 0) m.equipWeapon(f, slot);
      std::array<InputState, 4> in{};
      in[1].shield = true;
      for (int i = 0; i < 10; ++i) m.step(in);
      f.ult = kUltMax;
      f.finalSmash = true;
      in[0].ultimate = true;
      m.step(in);
      in[0].ultimate = false;
      const std::string name = f.moveDef(FinalSmash).name;
      bool caught = false, broke = false;
      float hurtWhileShielded = 0.0f, last = v.percent;
      for (int i = 0; i < 360 && f.state == FState::Attack && f.move == FinalSmash; ++i) {
        m.step(in);
        caught = caught || (v.state == FState::Captured && !broke);
        broke = broke || v.state == FState::ShieldBreak;
        if (!broke) hurtWhileShielded += std::max(0.0f, v.percent - last);
        last = v.percent;
      }
      ++total;
      broken += broke;
      CHECK(!caught, "%s: a shield stops the catch", name.c_str());
      CHECK(hurtWhileShielded < 0.01f, "%s: no damage through the shield (%.1f%%)", name.c_str(), hurtWhileShielded);
      std::printf("  %-10s %-32s %s\n", roster()[c].name.c_str(), name.c_str(),
                  broke ? "breaks the shield" : TextFormatShield(v.shield).c_str());
    }
  std::printf("  %d/%d ultimates blocked without breaking the shield\n", total - broken, total);
}

// Level 10 CPUs hit twice as hard as level 9.
static void testLevel10() {
  std::printf("[cpu level 10]\n");
  float dealt[2] = {0, 0};
  for (int pass = 0; pass < 2; ++pass) {
    MatchConfig cfg = duel(0, 0, 0, true, false);
    cfg.slots[0].level = pass == 0 ? 9 : 10;
    cfg.items = false;
    Match m(cfg);
    skipCountdown(m);
    m.fighters[0].pos = {-40, 0};
    m.fighters[0].facing = 1;
    m.fighters[1].pos = {20, 0};
    std::array<InputState, 4> in{};
    in[0].attack = true;
    m.step(in);
    in[0].attack = false;
    for (int i = 0; i < 25; ++i) m.step(in);
    dealt[pass] = m.fighters[1].percent;
  }
  CHECK(dealt[0] > 0.0f && std::fabs(dealt[1] - 2.0f * dealt[0]) < 0.01f, "level 10 jab %.1f%% vs level 9 %.1f%%", dealt[1],
        dealt[0]);
}

static void testTraining() {
  std::printf("[training mode]\n");
  MatchConfig cfg = duel(0, 2, 0, false, true);
  cfg.training = true;
  cfg.items = false;
  Match m(cfg);
  skipCountdown(m);
  CpuBrain dummy(5, 3);
  dummy.setDummy(CpuBrain::kDummyStand);
  Fighter& v = m.fighters[1];
  v.percent = 200;
  m.fighters[0].pos = {200, 0};
  m.fighters[0].facing = 1;
  v.pos = {270, 0};
  std::array<InputState, 4> in{};
  for (int i = 0; i < 10; ++i) {
    in[1] = dummy.think(m, 1);
    m.step(in);
  }
  CHECK(in[1].stick.len() == 0.0f && !in[1].attack && !in[1].jump, "standing dummy does nothing");
  in[0].cstick = {1, 0};
  m.step(in);
  in[0].cstick = {0, 0};
  int falls = v.falls;
  for (int i = 0; i < 600; ++i) {
    in[1] = dummy.think(m, 1);
    m.step(in);
  }
  CHECK(v.falls > falls, "the dummy was KO'd");
  CHECK(v.stocks == 3 && !v.eliminated && !m.finished, "no stock lost in training (stocks %d)", v.stocks);
  v.percent = 80;
  m.resetPositions();
  CHECK(v.percent == 0.0f && m.fighters[0].percent == 0.0f, "reset clears damage");

  // The options of the training pause menu
  {
    Match t(cfg);
    skipCountdown(t);
    Fighter& me = t.fighters[0];
    Fighter& dm = t.fighters[1];
    std::array<InputState, 4> none{};
    t.trainInfiniteUlt = true;
    t.step(none);
    CHECK(me.finalSmash && me.ult >= kUltMax, "infinite ultimate: the meter is full");
    me.pos = {-100, 0};
    me.facing = 1;
    dm.pos = {100, 0};
    Pad p{t};
    p.idle(20);
    p.run(1, key(0, 0, false, false, false, true));
    CHECK(me.move == FinalSmash, "the ultimate goes");
    p.idle(600);
    CHECK(me.finalSmash, "... and the meter fills up again");

    t.trainInfiniteUlt = false;
    t.trainZeroPercent = true;
    t.resetPositions();
    me.pos = {0, 0};
    me.facing = 1;
    dm.pos = {40, 0};
    p.idle(30);
    const int fallsBefore = dm.falls;
    bool hit = false;
    for (int i = 0; i < 4; ++i) {
      p.run(1, key(0, 0, true));
      for (int j = 0; j < 14; ++j) {
        p.idle(1);
        for (const auto& e : t.events) hit = hit || (e.type == EvType::Hit && e.b == dm.id);
      }
    }
    CHECK(hit && dm.percent == 0.0f && dm.falls == fallsBefore, "hit, and it stays at 0 %% (%.1f%%)", dm.percent);
    t.trainZeroPercent = false;

    t.setTrainFight(true);
    CHECK(dm.stocks == Match::kTrainStocks && me.stocks == Match::kTrainStocks, "fight mode: 3 stocks each");
    bool gameSet = false;
    for (int k = 0; k < 3; ++k) {
      for (int i = 0; i < 1500 && dm.stocks > 2 - k; ++i) {
        if (dm.active() && dm.state != FState::Respawn) dm.pos = {0, 3000};  // under the blast zone
        t.step(none);
        for (const auto& e : t.events) gameSet = gameSet || e.type == EvType::GameSet;
      }
    }
    CHECK(dm.stocks == 0 && dm.eliminated && gameSet, "three falls end the fight (stocks %d)", dm.stocks);
    CHECK(!t.finished, "the training goes on");
    for (int i = 0; i < 200; ++i) t.step(none);
    CHECK(dm.stocks == Match::kTrainStocks && !dm.eliminated && me.stocks == Match::kTrainStocks,
          "then the fight starts over (stocks %d)", dm.stocks);
    t.setTrainFight(false);
    const int falls2 = dm.falls;
    for (int i = 0; i < 1500 && dm.falls == falls2; ++i) {
      if (dm.active() && dm.state != FState::Respawn) dm.pos = {0, 3000};
      t.step(none);
    }
    CHECK(dm.falls > falls2 && dm.stocks == Match::kTrainStocks, "fight mode off: lives are infinite again");
  }
}

static void testRecovery() {
  std::printf("[recovery]\n");
  const int nChars = int(roster().size());
  for (int c = 0; c < nChars; ++c) {
    for (int side = -1; side <= 1; side += 2) {
      Match m(duel(0, c, 0, true, false));
      skipCountdown(m);
      CpuBrain brain(9, 42 + c);
      Fighter& f = m.fighters[0];
      m.fighters[1].pos = {-side * 300.0f, 0};
      f.pos = {side * 700.0f, 60.0f};
      f.grounded = false;
      f.airJumps = 0;  // worst case: only the special can save us
      f.state = FState::Air;
      bool safe = false;
      for (int i = 0; i < 600 && f.stocks == 3; ++i) {
        std::array<InputState, 4> in{};
        in[0] = brain.think(m, 0);
        m.step(in);
        if (f.state == FState::LedgeHang || (f.grounded && f.groundPlat == 0)) safe = true;
      }
      CHECK(safe && f.stocks == 3, "%s failed to recover from x=%d (pos %.0f,%.0f state %d)",
            roster()[c].name.c_str(), side * 700, f.pos.x, f.pos.y, int(f.state));
    }
  }
}

// Every stage, every fighter: thrown off either side (and into the Valley of the End's gap), it gets back.
static void testRecoveryStages() {
  std::printf("[recovery on every stage]\n");
  const int nChars = int(roster().size());
  int tries = 0, saved = 0;
  for (int st = 1; st < int(stageList().size()); ++st) {
    const StageDef& sd = stageList()[st];
    float outer = 0.0f;
    bool gap = false;
    for (const Platform& p : sd.platforms)
      if (p.solid) {
        outer = std::max({outer, std::fabs(p.base.left()), std::fabs(p.base.right())});
        gap = gap || (p.base.left() > 0.0f);  // a solid entirely on the right: there is a hole in the middle
      }
    for (int c = 0; c < nChars; ++c)
      for (int from = -1; from <= (gap ? 2 : 1); from += (gap ? 1 : 2)) {
        if (from == 0) continue;
        Match m(duel(st, c, 0, true, false));
        skipCountdown(m);
        CpuBrain brain(9, 77 + c * 3 + st);
        Fighter& f = m.fighters[0];
        const bool inGap = from == 2;
        m.fighters[1].pos = {inGap ? 300.0f : -from * 260.0f, 0};
        m.fighters[1].grounded = true;
        f.pos = inGap ? Vec2{0.0f, 120.0f} : Vec2{from * (outer + 260.0f), 60.0f};
        f.grounded = false;
        f.airJumps = 0;  // worst case: only the special can save us
        f.state = FState::Air;
        bool safe = false;
        for (int i = 0; i < 600 && f.stocks == 3; ++i) {
          std::array<InputState, 4> in{};
          in[0] = brain.think(m, 0);
          m.step(in);
          if (f.state == FState::LedgeHang || (f.grounded && f.groundPlat >= 0 && m.platforms[f.groundPlat].solid)) safe = true;
        }
        ++tries;
        saved += safe && f.stocks == 3;
        CHECK(safe && f.stocks == 3, "%s failed to recover on %s from %s (pos %.0f,%.0f state %d)", roster()[c].name.c_str(),
              sd.name.c_str(), inGap ? "the gap" : from < 0 ? "the left" : "the right", f.pos.x, f.pos.y, int(f.state));
      }
  }
  std::printf("  %d/%d recoveries\n", saved, tries);
}

struct BattleResult {
  int frames = 0;
  int winner = -1;
  bool finished = false;
};

static BattleResult cpuBattle(int stage, std::vector<int> chars, uint32_t seed, int maxFrames,
                              std::vector<float>* koPercents = nullptr) {
  MatchConfig cfg;
  cfg.stage = stage;
  cfg.stocks = 3;
  cfg.seed = seed;
  for (size_t i = 0; i < chars.size(); ++i) cfg.slots[i] = {true, chars[i], true, 9};
  Match m(cfg);
  std::vector<CpuBrain> brains;
  for (size_t i = 0; i < chars.size(); ++i) brains.emplace_back(9 - int(i), seed + uint32_t(i) * 77u);
  BattleResult r;
  for (int i = 0; i < maxFrames && !m.finished; ++i) {
    std::array<InputState, 4> in{};
    for (size_t k = 0; k < m.fighters.size(); ++k) in[m.fighters[k].port] = brains[k].think(m, int(k));
    std::vector<float> before;
    for (const auto& f : m.fighters) before.push_back(f.percent);
    m.step(in);
    for (const auto& e : m.events)
      if (e.type == EvType::KO && koPercents) koPercents->push_back(before[e.a]);
    if (!sane(m)) {
      CHECK(false, "insane state at frame %d", m.frame);
      break;
    }
  }
  r.frames = m.frame;
  r.winner = m.winner;
  r.finished = m.finished;
  return r;
}

static void testBattles() {
  std::printf("[cpu battles]\n");
  const int nChars = int(roster().size());
  int finished = 0, total = 0;
  long long frames = 0;
  std::vector<float> koPct;
  std::vector<int> wins(nChars, 0);
  for (int stage = 0; stage < int(stageList().size()); ++stage) {
    for (int k = 0; k < 4; ++k) {
      std::vector<int> chars = {(k) % nChars, (k + 1) % nChars, (k + 2) % nChars, (k + 3) % nChars};
      if (k % 2 == 1) chars.resize(2);
      BattleResult r = cpuBattle(stage, chars, 1000 + stage * 10 + k, 60 * 60 * 8, &koPct);
      ++total;
      if (r.finished) {
        ++finished;
        if (r.winner >= 0) wins[chars[r.winner]]++;
      }
      frames += r.frames;
      std::printf("  stage %d, %zu fighters: %s in %.1fs\n", stage, chars.size(),
                  r.finished ? "finished" : "TIMEOUT", r.frames / 60.0f);
    }
  }
  CHECK(finished >= total - 2, "most CPU battles should end (%d/%d)", finished, total);
  float avg = 0;
  for (float p : koPct) avg += p;
  if (!koPct.empty()) avg /= float(koPct.size());
  std::printf("  KOs: %zu, average KO percent: %.0f%%, avg match %.1fs\n", koPct.size(), avg,
              frames / 60.0f / total);
  for (int c = 0; c < nChars; ++c) std::printf("  wins %-6s %d\n", roster()[c].name.c_str(), wins[c]);
}

static void testBalance() {
  std::printf("[balance: round robin duels on stage 0]\n");
  const int n = int(roster().size());
  std::vector<int> wins(n, 0), games(n, 0);
  std::vector<std::vector<float>> koAt(n);  // per fighter: the victim's percent at each of its KOs
  int timeouts = 0, earlyUlt = 0;
  for (int a = 0; a < n; ++a)
    for (int b = 0; b < n; ++b) {
      if (a == b) continue;
      for (uint32_t seed = 0; seed < 3; ++seed) {
        MatchConfig cfg = duel(0, a, b, true, true);
        cfg.seed = 50 + seed;
        Match m(cfg);
        CpuBrain ba(9, seed * 3 + 1), bb(9, seed * 3 + 2);
        int ultUntil[2] = {-1, -1};  // a KO up to this frame comes from that fighter's ultimate
        for (int i = 0; i < 60 * 60 * 6 && !m.finished; ++i) {
          std::array<InputState, 4> in{};
          in[0] = ba.think(m, 0);
          in[1] = bb.think(m, 1);
          const float pct[2] = {m.fighters[0].percent, m.fighters[1].percent};
          const int by[2] = {m.fighters[0].lastHitBy, m.fighters[1].lastHitBy};
          for (int k = 0; k < 2; ++k)
            if (m.fighters[k].state == FState::Attack && m.fighters[k].move == FinalSmash) ultUntil[k] = m.frame + 150;
          m.step(in);
          for (const auto& e : m.events)  // a KO by the opponent's blow (not a fall on its own)
            if (e.type == EvType::KO && e.a >= 0 && e.a < 2 && by[e.a] == 1 - e.a) {
              koAt[e.a == 0 ? b : a].push_back(pct[e.a]);
              earlyUlt += pct[e.a] < 70.0f && m.frame <= ultUntil[1 - e.a];
            }
        }
        games[a]++;
        games[b]++;
        if (!m.finished) ++timeouts;
        else if (m.winner == 0) wins[a]++;
        else if (m.winner == 1) wins[b]++;
      }
    }
  for (int c = 0; c < n; ++c)
    std::printf("  %-6s win rate %3.0f%%\n", roster()[c].name.c_str(), 100.0f * wins[c] / std::max(1, games[c]));
  CHECK(timeouts <= 3, "%d duels timed out", timeouts);
  // How early the KOs come (the "win rate" lines above are read by tools/pixellab/fitall.sh: keep them apart).
  int early = 0, all = 0;
  for (int c = 0; c < n; ++c) {
    const auto& k = koAt[c];
    if (k.empty()) continue;
    float sum = 0.0f, low = 1e9f;
    int under = 0;
    for (float p : k) {
      sum += p;
      low = std::min(low, p);
      under += p < 70.0f;
    }
    early += under;
    all += int(k.size());
    std::printf("  %-6s KOs at %3.0f%% on average, the earliest at %3.0f%%, %2.0f%% of them under 70%%\n",
                roster()[c].name.c_str(), sum / float(k.size()), low, 100.0f * float(under) / float(k.size()));
  }
  std::printf("  %d KOs, %.0f%% of them under 70%% (%d of those by an ultimate)\n", all,
              100.0f * float(early) / float(std::max(1, all)), earlyUlt);
}

static void testDeterminism() {
  std::printf("[determinism]\n");
  BattleResult a = cpuBattle(1, {0, 3, 4}, 777, 60 * 60 * 3);
  BattleResult b = cpuBattle(1, {0, 3, 4}, 777, 60 * 60 * 3);
  CHECK(a.frames == b.frames && a.winner == b.winner, "same seed, same result (%d/%d vs %d/%d)", a.frames,
        a.winner, b.frames, b.winner);
}

int main(int argc, char** argv) {
  std::string only = argc > 1 ? argv[1] : "";
  if (only.empty() || only == "kb") testKnockback();
  if (only.empty() || only == "jab") testJabHits();
  if (only.empty() || only == "smash") testSmashKills();
  if (only.empty() || only == "keys") testKeyboardControls();
  if (only.empty() || only == "combos") testCombos();
  if (only.empty() || only == "step") testAttackStep();
  if (only.empty() || only == "strikes") testWeaponStrikes();
  if (only.empty() || only == "teams") testTeams();
  if (only.empty() || only == "fs") testFinalSmash();
  if (only.empty() || only == "ult") testUltMeter();
  if (only.empty() || only == "ult") testUltEdgeCases();
  if (only.empty() || only == "ults") testAllUltimates();
  if (only.empty() || only == "ults" || only == "block") testUltimatesBlocked();
  if (only.empty() || only == "cpu10") testLevel10();
  if (only.empty() || only == "training") testTraining();
  if (only.empty() || only == "weapons") testWeapons();
  if (only.empty() || only == "weapons" || only == "amaterasu") testAmaterasu();
  if (only.empty() || only == "recovery") testRecovery();
  if (only.empty() || only == "recovery") testRecoveryStages();
  if (only.empty() || only == "battle") testBattles();
  if (only.empty() || only == "balance") testBalance();
  if (only.empty() || only == "det") testDeterminism();
  if (g_failures) {
    std::printf("%d check(s) failed\n", g_failures);
    return 1;
  }
  std::printf("all tests passed\n");
  return 0;
}
