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
  p.run(1, key(0, 0, false, false, false, true));
  CHECK(f.move != FinalSmash, "the special button does not waste the ultimate");
  p.idle(60);
  InputState ult;
  ult.ultimate = true;
  p.run(1, ult);
  CHECK(f.state == FState::Attack && f.move == FinalSmash, "the ultimate button triggers the final smash");
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
  for (int i = 0; i < 120 && !m.items.back().grounded; ++i) p.idle(1);
  CHECK(!m.items.empty() && m.items.back().grounded, "a dropped weapon lands on the stage");
  p.run(30, key(1, 0));
  CHECK(f.weapon < 0 && m.items.size() == 1, "another character's weapon cannot be picked up");
  m.items.clear();
  m.dropWeaponFor(0, 0);  // Leviathan axe
  m.items.back().pos = {f.pos.x + 60, -300};
  for (int i = 0; i < 120 && !m.items.back().grounded; ++i) p.idle(1);
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

// Every ultimate (bare-handed and with each weapon) hits, lets its victim go, and can KO at high percent.
static void testAllUltimates() {
  std::printf("[the 45 ultimates]\n");
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
  int timeouts = 0;
  for (int a = 0; a < n; ++a)
    for (int b = 0; b < n; ++b) {
      if (a == b) continue;
      for (uint32_t seed = 0; seed < 3; ++seed) {
        MatchConfig cfg = duel(0, a, b, true, true);
        cfg.seed = 50 + seed;
        Match m(cfg);
        CpuBrain ba(9, seed * 3 + 1), bb(9, seed * 3 + 2);
        for (int i = 0; i < 60 * 60 * 6 && !m.finished; ++i) {
          std::array<InputState, 4> in{};
          in[0] = ba.think(m, 0);
          in[1] = bb.think(m, 1);
          m.step(in);
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
  if (only.empty() || only == "fs") testFinalSmash();
  if (only.empty() || only == "ult") testUltMeter();
  if (only.empty() || only == "ult") testUltEdgeCases();
  if (only.empty() || only == "ults") testAllUltimates();
  if (only.empty() || only == "cpu10") testLevel10();
  if (only.empty() || only == "training") testTraining();
  if (only.empty() || only == "weapons") testWeapons();
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
