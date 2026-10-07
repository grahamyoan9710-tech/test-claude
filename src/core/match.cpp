#include "core/match.hpp"

#include <algorithm>

Match::Match(const MatchConfig& c) : cfg(c), rng(c.seed) {
  const auto& stages = stageList();
  stageDef = &stages[std::clamp(cfg.stage, 0, int(stages.size()) - 1)];
  platforms = stageDef->platforms;
  const auto& chars = roster();
  std::array<int, 16> used{};
  for (int port = 0; port < 4; ++port) {
    const auto& slot = cfg.slots[port];
    if (!slot.active) continue;
    Fighter f;
    f.id = int(fighters.size());
    f.port = port;
    f.charIndex = std::clamp(slot.character, 0, int(chars.size()) - 1);
    f.def = &chars[f.charIndex];
    f.palette = used[f.charIndex]++ % 2;  // mirror match: alternate colours
    f.cpu = slot.cpu;
    f.cpuLevel = slot.level;
    f.stocks = cfg.stocks;
    Vec2 spawn = stageDef->spawns[fighters.size() % stageDef->spawns.size()];
    f.pos = spawn;
    f.facing = spawn.x > 0 ? -1 : 1;
    f.airJumps = f.def->airJumps;
    f.grounded = false;
    fighters.push_back(f);
  }
}

void Match::emit(EvType t, Vec2 pos, int a, int b, float strength, Vec2 dir, HitFx fx) {
  GameEvent e;
  e.type = t;
  e.pos = pos;
  e.a = a;
  e.b = b;
  e.strength = strength;
  e.dir = dir;
  e.fx = fx;
  events.push_back(e);
}

void Match::step(const std::array<InputState, 4>& inputs) {
  events.clear();
  if (finished) {
    ++frame;
    return;
  }
  updatePlatforms(platforms, frame);

  if (countdown > 0) {
    --countdown;
    if (countdown == 180 || countdown == 120 || countdown == 60)
      emit(EvType::Countdown, {}, -1, -1, float(countdown / 60));
    if (countdown == 0) emit(EvType::Go, {});
  }

  for (auto& f : fighters) {
    f.prev = f.in;
    f.in = started() ? inputs[f.port] : InputState{};
    for (int i = 5; i > 0; --i) f.stickHist[i] = f.stickHist[i - 1];
    f.stickHist[0] = f.in.stick;
  }
  for (auto& f : fighters) updateFighter(f);
  updateProjectiles();
  resolveHits();
  checkBlastZones();
  checkEnd();
  ++frame;
}

bool Match::ledgeValid(int ledge) const {
  int p = ledge / 2;
  return p >= 0 && p < int(platforms.size()) && platforms[p].solid && platforms[p].ledges;
}

Vec2 Match::ledgePoint(int ledge) const {
  const Rect& r = platforms[ledge / 2].rect;
  return (ledge % 2 == 0) ? Vec2{r.left(), r.top()} : Vec2{r.right(), r.top()};
}

int Match::ledgeOwner(int ledge) const {
  for (const auto& f : fighters)
    if ((f.state == FState::LedgeHang || f.state == FState::LedgeClimb) && f.ledge == ledge) return f.id;
  return -1;
}

void Match::checkBlastZones() {
  const Rect& b = stageDef->blast;
  for (auto& f : fighters) {
    if (!f.active() || f.state == FState::Respawn) continue;
    bool out = f.pos.x < b.left() || f.pos.x > b.right() || f.pos.y > b.bottom();
    if (f.pos.y - f.height() < b.top()) {
      if (f.state == FState::Hitstun) {
        out = true;
      } else {
        // Without hitstun you just bonk on the ceiling of the blast zone.
        f.pos.y = b.top() + f.height();
        f.vel.y = std::max(f.vel.y, 0.0f);
      }
    }
    if (out) koFighter(f);
  }
}

void Match::koFighter(Fighter& f) {
  const Rect& b = stageDef->blast;
  Vec2 p{clampf(f.pos.x, b.left(), b.right()), clampf(f.center().y, b.top(), b.bottom())};
  Vec2 dir = (p - Vec2{0, -150}).norm();
  emit(EvType::KO, p, f.id, f.lastHitBy, f.lastKnockback, dir);
  if (f.grabPartner >= 0) releaseGrab(f, false);
  if (f.lastHitBy >= 0 && f.lastHitBy != f.id && frame - f.lastHitFrame < 600)
    fighters[f.lastHitBy].kos++;
  f.falls++;
  f.stocks--;
  f.state = FState::Dead;
  f.stateFrame = 0;
  f.respawnTimer = 100;
  f.vel = f.kbVel = {0, 0};
  f.hitstun = f.hitlag = 0;
  f.move = -1;
  f.ledge = -1;
  if (f.stocks <= 0) {
    f.eliminated = true;
    f.koFrame = frame;
  }
}

void Match::respawnFighter(Fighter& f) {
  f.pos = stageDef->respawn;
  f.vel = f.kbVel = {0, 0};
  f.percent = 0.0f;
  f.shield = kMaxShield;
  f.grounded = false;
  f.groundPlat = -1;
  f.airJumps = f.def->airJumps;
  f.fastFall = f.airDodgeUsed = f.sideSpecialUsed = false;
  f.lastHitBy = -1;
  f.facing = 1;
  setState(f, FState::Respawn);
  emit(EvType::Respawn, f.pos, f.id);
}

void Match::checkEnd() {
  if (fighters.size() < 2) return;
  int alive = 0, last = -1;
  for (const auto& f : fighters)
    if (!f.eliminated) {
      ++alive;
      last = f.id;
    }
  if (alive <= 1) {
    finished = true;
    winner = alive == 1 ? last : -1;
    finishFrame = frame;
    emit(EvType::GameSet, {}, winner);
  }
}

std::vector<int> Match::ranking() const {
  std::vector<int> ids;
  for (const auto& f : fighters) ids.push_back(f.id);
  std::stable_sort(ids.begin(), ids.end(), [&](int a, int b) {
    const Fighter& fa = fighters[a];
    const Fighter& fb = fighters[b];
    if (fa.eliminated != fb.eliminated) return !fa.eliminated;
    if (fa.eliminated) return fa.koFrame > fb.koFrame;
    if (fa.stocks != fb.stocks) return fa.stocks > fb.stocks;
    return fa.percent < fb.percent;
  });
  return ids;
}
