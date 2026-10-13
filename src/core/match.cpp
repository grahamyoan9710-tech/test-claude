#include "core/match.hpp"

#include <algorithm>

Match::Match(const MatchConfig& c) : cfg(c), rng(c.seed) {
  const auto& stages = stageList();
  stageDef = &stages[std::clamp(cfg.stage, 0, int(stages.size()) - 1)];
  platforms = stageDef->platforms;
  const auto& chars = roster();
  std::vector<int> used(chars.size(), 0);  // fighters picked twice alternate their colours
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
    f.team = cfg.teams ? slot.team : f.id;
    f.inTeam = cfg.teams;
    f.stocks = cfg.stocks;
    Vec2 spawn = stageDef->spawns[fighters.size() % stageDef->spawns.size()];
    f.pos = spawn;
    f.facing = spawn.x > 0 ? -1 : 1;
    f.airJumps = f.def->airJumps;
    f.grounded = false;
    fighters.push_back(f);
  }
  orbTimer = rng.irange(60 * 10, 60 * 20);
  itemTimer = rng.irange(60 * 7, 60 * 11);
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
    for (int i = 19; i > 0; --i) f.dirHist[i] = f.dirHist[i - 1];
    {
      const int x = f.in.stick.x > 0.5f ? 1 : f.in.stick.x < -0.5f ? -1 : 0;
      const int y = f.in.stick.y > 0.5f ? 1 : f.in.stick.y < -0.5f ? -1 : 0;
      f.dirHist[0] = uint8_t(5 + x - 3 * y);  // numpad notation: 2 down, 6 right, 3 down-right...
    }
  }
  if (freezeTimer > 0) {  // final smash cinematic: only its user moves
    --freezeTimer;
    updateFighter(fighters[freezeOwner]);
  } else {
    for (auto& f : fighters) updateFighter(f);
    updateProjectiles();
    updateOrb();
    updateItems();
  }
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
  loseWeapon(f, false);
  if (f.form >= 0) endForm(f);
  f.captor = -1;
  if (f.lastHitBy >= 0 && f.lastHitBy != f.id && frame - f.lastHitFrame < 600)
    fighters[f.lastHitBy].kos++;
  f.falls++;
  if (!cfg.training) f.stocks--;
  if (freezeTimer > 0 && freezeOwner == f.id) freezeTimer = 0;  // the ultimate meter itself is kept
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
    f.finalSmash = false;  // nothing left to unleash it with
    f.ult = 0.0f;
  }
}

void Match::respawnFighter(Fighter& f) {
  // Fighters KO'd at the same time get side-by-side respawn platforms.
  static const float offsets[4] = {0.0f, -130.0f, 130.0f, -260.0f};
  for (float dx : offsets) {
    f.pos = stageDef->respawn + Vec2{dx, 0.0f};
    bool taken = false;
    for (const auto& o : fighters)
      taken = taken || (o.id != f.id && o.state == FState::Respawn && std::fabs(o.pos.x - f.pos.x) < 100.0f);
    if (!taken) break;
  }
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
  if (fighters.size() < 2 || cfg.training) return;
  int alive = 0, last = -1;  // teams still standing (a fighter is its own team out of team battles)
  for (const auto& f : fighters)
    if (!f.eliminated) {
      bool counted = false;
      for (const auto& g : fighters) counted = counted || (g.id < f.id && !g.eliminated && g.team == f.team);
      if (!counted) ++alive;
      last = f.id;
    }
  if (alive <= 1) {
    finished = true;
    winner = alive == 1 ? last : -1;
    winnerTeam = alive == 1 ? fighters[last].team : -1;
    finishFrame = frame;
    emit(EvType::GameSet, {}, winner);
  }
}

void Match::updateOrb() {
  if (!cfg.items || !started()) return;
  if (!orb.active) {
    if (--orbTimer > 0) return;
    for (const auto& f : fighters)
      if (f.state == FState::Attack && f.move == FinalSmash) {
        orbTimer = 300;  // not during a final smash
        return;
      }
    orb = Orb{};
    orb.active = true;
    orb.pos = {rng.range(-350.0f, 350.0f), -520.0f};
    orb.vel = {rng.range(-2.0f, 2.0f), 1.5f};
    orb.hp = 40.0f;
    orb.life = 60 * 18;
    emit(EvType::OrbSpawn, orb.pos);
    return;
  }
  if (orb.flash > 0) --orb.flash;
  if (--orb.life <= 0) {
    orb.active = false;
    orbTimer = rng.irange(60 * 15, 60 * 30);
    return;
  }
  orb.vel += Vec2{rng.range(-0.15f, 0.15f), rng.range(-0.15f, 0.15f)};
  float sp = orb.vel.len();
  if (sp > 3.2f) orb.vel = orb.vel * approach(1.0f, 3.2f / sp, 0.05f);
  if (sp < 1.2f) orb.vel = orb.vel.len() > 0.01f ? orb.vel * (1.2f / sp) : Vec2{1.2f, 0.0f};
  orb.pos += orb.vel;
  if (orb.pos.x < -550.0f || orb.pos.x > 550.0f) {
    orb.pos.x = clampf(orb.pos.x, -550.0f, 550.0f);
    orb.vel.x = -orb.vel.x;
  }
  if (orb.pos.y < -560.0f || orb.pos.y > -70.0f) {
    orb.pos.y = clampf(orb.pos.y, -560.0f, -70.0f);
    orb.vel.y = -orb.vel.y;
  }
}

void Match::hitOrb(int attacker, float damage, Vec2 from, HitFx fx) {
  orb.hp -= damage;
  orb.flash = 8;
  Vec2 d = (orb.pos - from).norm();
  orb.vel += d * (2.0f + damage * 0.25f);
  if (orb.vel.len() > 9.0f) orb.vel = orb.vel.norm() * 9.0f;
  emit(EvType::OrbHit, orb.pos, attacker, -1, damage, d, fx);
  if (orb.hp > 0.0f) return;
  orb.active = false;
  orbTimer = rng.irange(60 * 25, 60 * 45);
  emit(EvType::OrbBreak, orb.pos, attacker);
  if (attacker >= 0) gainUlt(fighters[attacker], kUltMax);  // the orb fills the ultimate meter
}

void Match::gainUlt(Fighter& f, float amount) {
  if (f.eliminated || f.finalSmash || amount <= 0.0f || (f.state == FState::Attack && f.move == FinalSmash)) return;
  f.ult = std::min(kUltMax, f.ult + amount);
  if (f.ult < kUltMax) return;
  f.finalSmash = true;
  emit(EvType::UltReady, f.center(), f.id);
}

void Match::startFinalSmash(Fighter& f) {
  f.finalSmash = false;
  f.ult = 0.0f;
  startMove(f, FinalSmash);
  freezeTimer = 30;
  freezeOwner = f.id;
  const MoveDef& fs = f.moveDef(FinalSmash);
  HitFx fx = fs.hitboxes.empty() ? fs.projectile.hit.fx : fs.hitboxes[0].fx;
  emit(EvType::FinalSmash, f.center(), f.id, -1, 0.0f, {}, fx);
}

void Match::startForm(Fighter& f, int form) {
  if (form < 0 || form >= int(f.def->forms.size())) return;
  f.form = form;
  f.formTimer = f.def->forms[form].duration;
  f.formMove = f.formTimer > 0 ? -1 : f.move;
  emit(EvType::Transform, f.center(), f.id, form, 0.0f, {}, f.def->forms[form].fx);
}

void Match::endForm(Fighter& f) {
  if (f.form < 0) return;
  f.form = -1;
  f.formTimer = 0;
  f.formMove = -1;
  emit(EvType::FormEnd, f.center(), f.id);
}

void Match::resetPositions() {
  projectiles.clear();
  items.clear();
  for (auto& f : fighters) {
    if (f.eliminated) continue;
    if (f.grabPartner >= 0) releaseGrab(f, false);
    Vec2 spawn = stageDef->spawns[f.id % stageDef->spawns.size()];
    f.pos = spawn;
    f.vel = f.kbVel = {0, 0};
    f.facing = spawn.x > 0 ? -1 : 1;
    f.percent = 0.0f;
    f.shield = kMaxShield;
    f.hitlag = f.hitstun = 0;
    f.intangible = f.invincible = 0;
    f.grounded = false;
    f.groundPlat = -1;
    f.ledge = -1;
    f.airJumps = f.def->airJumps;
    f.fastFall = f.airDodgeUsed = f.sideSpecialUsed = false;
    f.finalSmash = false;
    f.ult = 0.0f;
    f.captor = -1;
    loseWeapon(f, false);
    if (f.form >= 0) endForm(f);
    setState(f, FState::Air);
  }
  freezeTimer = 0;
}

std::vector<int> Match::ranking() const {
  std::vector<int> ids;
  for (const auto& f : fighters) ids.push_back(f.id);
  std::stable_sort(ids.begin(), ids.end(), [&](int a, int b) {
    const Fighter& fa = fighters[a];
    const Fighter& fb = fighters[b];
    if (cfg.teams && winnerTeam >= 0 && (fa.team == winnerTeam) != (fb.team == winnerTeam))
      return fa.team == winnerTeam;  // the winning team first
    if (fa.eliminated != fb.eliminated) return !fa.eliminated;
    if (fa.eliminated) return fa.koFrame > fb.koFrame;
    if (fa.stocks != fb.stocks) return fa.stocks > fb.stocks;
    return fa.percent < fb.percent;
  });
  return ids;
}
