// --check-hitboxes OUT.tsv: for every move drawn by a frame-by-frame clip (every fighter, bare-handed and with each
// weapon), on every frame its hitboxes are out, which clip frame the game shows and how well the hitboxes sit on
// the drawing: the share of each hitbox covered by drawn pixels (the fighter, its weapon, its effect) and, when it
// covers nothing, how far the nearest drawn pixel is. One line per hitbox and frame; scripts/check_hitboxes.py
// sums it up per blow.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "core/hitshapes.hpp"
#include "core/match.hpp"
#include "game/fighter_art.hpp"
#include "game/render.hpp"

namespace {
float segDist(Vec2 p, Vec2 a, Vec2 b) {  // distance from p to the segment ab
  const Vec2 ab = b - a;
  const float l2 = ab.x * ab.x + ab.y * ab.y;
  const float t = l2 > 1e-6f ? clampf(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / l2, 0.0f, 1.0f) : 0.0f;
  return (p - (a + ab * t)).len();
}
}  // namespace

int checkHitboxes(const std::string& outPath) {
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(320, 240, "check");  // the clips load as textures too
  FILE* out = std::fopen(outPath.c_str(), "w");
  if (!out) return 1;
  std::fprintf(out, "fighter\tslot\tmove\tframe\tduration\tclip_frame\tbox\tcoverage\tgap\tcx\tcy\tradius\n");
  const auto& r = roster();
  int rows = 0;
  for (int ci = 0; ci < int(r.size()); ++ci) {
    const CharacterDef& d = r[ci];
    if (!art::hasClips(d.hero)) continue;
    for (int slot = -1; slot <= 1; ++slot)
      for (int mv = 0; mv < MOVE_COUNT; ++mv) {
        if (slot >= 0 && !art::hasSheet(d.hero, (slot == 0 ? art::kSheetWeapon0 : art::kSheetWeapon1) + mv)) continue;
        MatchConfig cfg;
        cfg.items = false;
        cfg.weapons = false;
        cfg.slots[0] = {true, ci, true, 5};
        cfg.slots[1] = {true, ci == 0 ? 1 : 0, true, 5};
        Match m(cfg);
        Fighter& f = m.fighters[0];
        if (slot >= 0) m.equipWeapon(f, slot);
        const MoveDef& md = f.moveDef(mv);
        if (md.duration <= 0 || md.hitboxes.empty()) continue;
        const bool air = mv >= NAir && mv <= DAir;
        for (int fr = 0; fr < md.duration; ++fr) {
          f.facing = 1;
          f.state = FState::Attack;
          f.move = mv;
          f.moveFrame = fr;
          f.stateFrame = fr;
          f.grounded = !air;
          const auto shapes = hitShapes(md, fr, f.center(), 1, f.height(), f.bladeLength());
          if (shapes.empty()) continue;
          const Pose p = computePose(f, m, 0.0f);
          art::SheetMask mk;
          if (!art::sheetMask(d, p, mk)) continue;
          const float H = d.height, k = H / mk.pxPerH;  // world units per clip pixel
          auto toPx = [&](Vec2 w) {  // world -> clip pixel (the clip's anchor on the feet)
            const Vec2 l = w - f.center();
            return Vec2{mk.ax + l.x / k, mk.ay + (l.y - H * 0.5f) / k};
          };
          for (const HitShape& s : shapes) {
            const Vec2 a = toPx(s.a), b = toPx(s.b);
            const float rad = s.radius / k;
            int inside = 0, covered = 0;
            float gap = 1e9f;
            const int x0 = int(std::floor(std::min(a.x, b.x) - rad)), x1 = int(std::ceil(std::max(a.x, b.x) + rad));
            const int y0 = int(std::floor(std::min(a.y, b.y) - rad)), y1 = int(std::ceil(std::max(a.y, b.y) + rad));
            for (int y = y0; y <= y1; ++y)
              for (int x = x0; x <= x1; ++x) {
                if (segDist({float(x), float(y)}, a, b) > rad) continue;
                ++inside;
                if (x >= 0 && y >= 0 && x < mk.w && y < mk.h && mk.opaque[size_t(y) * mk.w + x]) ++covered;
              }
            if (covered == 0)  // how far the drawing is from it
              for (int y = 0; y < mk.h; y += 2)
                for (int x = 0; x < mk.w; x += 2)
                  if (mk.opaque[size_t(y) * mk.w + x])
                    gap = std::min(gap, segDist({float(x), float(y)}, a, b) - rad);
            const Vec2 c = (s.a + s.b) * 0.5f - f.center();
            std::fprintf(out, "%s\t%d\t%s\t%d\t%d\t%d\t%d\t%.3f\t%.1f\t%.1f\t%.1f\t%.1f\n", d.name.c_str(), slot,
                         art::sheetName(art::kSheetMove0 + mv), fr, md.duration, mk.frame, s.index,
                         inside ? float(covered) / inside : 0.0f, covered ? 0.0f : gap * k, c.x, c.y, s.radius);
            ++rows;
          }
        }
      }
  }
  std::fclose(out);
  CloseWindow();
  std::printf("%d rows -> %s\n", rows, outPath.c_str());
  return 0;
}

// --fit-impacts OUT.tsv: for every clip-drawn move, each blow (hitbox windows merged as the renderer does) against
// every frame of its clip: the share of the blow's hitboxes (over all its frames) that the frame's drawing covers.
// scripts/fit_impacts.py picks, blow by blow and in order, the frames that cover them best.
int fitImpacts(const std::string& outPath) {
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(320, 240, "fit");
  FILE* out = std::fopen(outPath.c_str(), "w");
  if (!out) return 1;
  std::fprintf(out, "fighter\tclip\tblow\tstart\tend\tduration\tcoverage_per_frame\n");
  const auto& r = roster();
  int rows = 0;
  for (int ci = 0; ci < int(r.size()); ++ci) {
    const CharacterDef& d = r[ci];
    if (!art::hasClips(d.hero)) continue;
    for (int slot = -1; slot <= 1; ++slot)
      for (int mv = 0; mv < MOVE_COUNT; ++mv) {
        const int sheet = (slot < 0 ? art::kSheetMove0 : slot == 0 ? art::kSheetWeapon0 : art::kSheetWeapon1) + mv;
        const int nf = art::sheetFrames(d, sheet);
        if (nf <= 0) continue;
        MatchConfig cfg;
        cfg.items = false;
        cfg.weapons = false;
        cfg.slots[0] = {true, ci, true, 5};
        cfg.slots[1] = {true, ci == 0 ? 1 : 0, true, 5};
        Match m(cfg);
        Fighter& f = m.fighters[0];
        if (slot >= 0) m.equipWeapon(f, slot);
        const MoveDef& md = f.moveDef(mv);
        if (md.duration <= 0 || md.hitboxes.empty()) continue;
        std::vector<std::pair<int, int>> w;  // the blows, as clipImpact (render_fighter.cpp) merges them
        for (const auto& h : md.hitboxes)
          if (h.end > h.start) w.push_back({h.start, h.end});
        std::sort(w.begin(), w.end());
        std::vector<std::pair<int, int>> blows;
        for (const auto& [a, b] : w) {
          if (!blows.empty() && a <= blows.back().second) blows.back().second = std::max(blows.back().second, b);
          else blows.push_back({a, b});
        }
        if (blows.size() > 4) blows.resize(4);
        const float H = d.height;
        std::vector<art::SheetMask> masks(nf);
        for (int j = 0; j < nf; ++j) art::sheetMaskAt(d, sheet, j, masks[j]);
        for (int bi = 0; bi < int(blows.size()); ++bi) {
          std::vector<double> cov(nf, 0.0);
          long total = 0;
          for (int fr = blows[bi].first; fr < std::min(blows[bi].second, md.duration); ++fr) {
            f.facing = 1;
            f.moveFrame = fr;
            for (const HitShape& s : hitShapes(md, fr, f.center(), 1, H, f.bladeLength()))
              for (int j = 0; j < nf; ++j) {
                const art::SheetMask& mk = masks[j];
                const float k = H / mk.pxPerH;
                auto toPx = [&](Vec2 wpt) {
                  const Vec2 l = wpt - f.center();
                  return Vec2{mk.ax + l.x / k, mk.ay + (l.y - H * 0.5f) / k};
                };
                const Vec2 a = toPx(s.a), b = toPx(s.b);
                const float rad = s.radius / k;
                int inside = 0, covered = 0;
                for (int y = int(std::floor(std::min(a.y, b.y) - rad)); y <= int(std::ceil(std::max(a.y, b.y) + rad)); ++y)
                  for (int x = int(std::floor(std::min(a.x, b.x) - rad)); x <= int(std::ceil(std::max(a.x, b.x) + rad)); ++x) {
                    if (segDist({float(x), float(y)}, a, b) > rad) continue;
                    ++inside;
                    if (x >= 0 && y >= 0 && x < mk.w && y < mk.h && mk.opaque[size_t(y) * mk.w + x]) ++covered;
                  }
                if (inside) cov[j] += double(covered) / inside;
                if (j == 0) ++total;
              }
          }
          std::fprintf(out, "%s\t%s\t%d\t%d\t%d\t%d\t", d.name.c_str(), art::sheetName(sheet), bi, blows[bi].first,
                       blows[bi].second, md.duration);
          for (int j = 0; j < nf; ++j) std::fprintf(out, "%s%.3f", j ? "," : "", total ? cov[j] / total : 0.0);
          std::fprintf(out, "\n");
          ++rows;
        }
      }
  }
  std::fclose(out);
  CloseWindow();
  std::printf("%d blows -> %s\n", rows, outPath.c_str());
  return 0;
}


// --fit-hitboxes OUT.inc: moves the round hitboxes of every clip-drawn attack onto what its clip draws, and writes
// them as core/roster_hitfit.inc (applied by roster()). For each blow, the frame the game shows while the hitbox is
// out is compared with the clip's first frame (the stance the move starts from): what the blow adds to the drawing
// (the limb, the weapon, the effect thrown out) is where it hits. Each hitbox goes to the far end of what is added in
// its own direction from the body, and grows or shrinks to cover it, within limits that keep the move's reach and
// balance: hitboxes of one blow pointing the same way move together; hitboxes on the body (spins) stay as written;
// a blade swing keeps its sweep but is turned to where the blade is drawn, its reach set to the drawn tip. A report (OUT.inc with .tsv) gives each hitbox's cover of the drawing before and after.
namespace {
struct Px {
  float x, y;  // world units from the fighter's centre, facing right
};

float coverOf(const art::SheetMask& mk, float H, Vec2 c, float r) {  // share of the circle on drawn pixels
  const float k = H / mk.pxPerH;
  const float cx = mk.ax + c.x / k, cy = mk.ay + (c.y - H * 0.5f) / k, rr = r / k;
  int in = 0, on = 0;
  for (int y = int(cy - rr); y <= int(cy + rr); ++y)
    for (int x = int(cx - rr); x <= int(cx + rr); ++x) {
      if ((x - cx) * (x - cx) + (y - cy) * (y - cy) > rr * rr) continue;
      ++in;
      if (x >= 0 && y >= 0 && x < mk.w && y < mk.h && mk.opaque[size_t(y) * mk.w + x]) ++on;
    }
  return in ? float(on) / in : 0.0f;
}

float coverSeg(const art::SheetMask& mk, float H, Vec2 a, Vec2 b, float r) {  // the same for a capsule a-b
  const float k = H / mk.pxPerH;
  auto px = [&](Vec2 w) { return Vec2{mk.ax + w.x / k, mk.ay + (w.y - H * 0.5f) / k}; };
  const Vec2 pa = px(a), pb = px(b);
  const float rr = r / k;
  int in = 0, on = 0;
  for (int y = int(std::min(pa.y, pb.y) - rr); y <= int(std::max(pa.y, pb.y) + rr); ++y)
    for (int x = int(std::min(pa.x, pb.x) - rr); x <= int(std::max(pa.x, pb.x) + rr); ++x) {
      if (segDist({float(x), float(y)}, pa, pb) > rr) continue;
      ++in;
      if (x >= 0 && y >= 0 && x < mk.w && y < mk.h && mk.opaque[size_t(y) * mk.w + x]) ++on;
    }
  return in ? float(on) / in : 0.0f;
}

// Pixels the blow adds: drawn in `hit` and not within `grow` px of anything drawn in `rest`, in world units.
std::vector<Px> added(const art::SheetMask& hit, const art::SheetMask& rest, float H, int grow) {
  std::vector<Px> out;
  const float k = H / hit.pxPerH, kr = H / rest.pxPerH;
  for (int y = 0; y < hit.h; ++y)
    for (int x = 0; x < hit.w; ++x) {
      if (!hit.opaque[size_t(y) * hit.w + x]) continue;
      const float wx = (x - hit.ax) * k, wy = (y - hit.ay) * k + H * 0.5f;
      const float rx = rest.ax + wx / kr, ry = rest.ay + (wy - H * 0.5f) / kr;  // the same spot in the stance
      bool near = false;
      for (int dy = -grow; dy <= grow && !near; ++dy)
        for (int dx = -grow; dx <= grow && !near; ++dx) {
          const int qx = int(rx) + dx, qy = int(ry) + dy;
          near = qx >= 0 && qy >= 0 && qx < rest.w && qy < rest.h && rest.opaque[size_t(qy) * rest.w + qx];
        }
      if (!near) out.push_back({wx, wy});
    }
  return out;
}
}  // namespace

int fitHitboxes(const std::string& outPath) {
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(320, 240, "fit");
  FILE* out = std::fopen(outPath.c_str(), "w");
  const std::string reportPath = outPath.substr(0, outPath.rfind('.')) + ".tsv";
  FILE* rep = std::fopen(reportPath.c_str(), "w");
  if (!out || !rep) return 1;
  std::fprintf(out, "// Generated by `super_clash --fit-hitboxes src/core/roster_hitfit.inc`: do not edit by hand.\n");
  std::fprintf(out, "// fighter, weapon slot, move, hitbox, radius as written, fitted offset x y, fitted radius, swing a0 a1 reach\n");
  std::fprintf(rep, "fighter\tslot\tmove\tbox\tclip_frame\tx0\ty0\tr0\tx1\ty1\tr1\tcover0\tcover1\tnote\n");
  const auto& r = roster();
  const int fitted[] = {Jab,    Jab2,   Jab3,   FTilt,    UTilt,    DTilt,    DashAttack,  FSmash,     USmash,
                        DSmash, NAir,   FAir,   BAir,     UAir,     DAir,     NSpecial,    SSpecial,   USpecial,
                        DSpecial, LedgeAttack, GetupAttack};
  int moved = 0, total = 0;
  for (int ci = 0; ci < int(r.size()); ++ci) {
    const CharacterDef& d = r[ci];
    if (!art::hasClips(d.hero)) continue;
    for (int slot = -1; slot <= 1; ++slot)
      for (int mv : fitted) {
        const int sheet = (slot < 0 ? art::kSheetMove0 : slot == 0 ? art::kSheetWeapon0 : art::kSheetWeapon1) + mv;
        if (!art::hasSheet(d.hero, slot < 0 ? art::kSheetMove0 + mv : sheet)) continue;
        if (slot >= 0 && !art::hasSheet(d.hero, sheet)) continue;
        MatchConfig cfg;
        cfg.items = false;
        cfg.weapons = false;
        cfg.slots[0] = {true, ci, true, 5};
        cfg.slots[1] = {true, ci == 0 ? 1 : 0, true, 5};
        Match m(cfg);
        Fighter& f = m.fighters[0];
        if (slot >= 0) m.equipWeapon(f, slot);
        const MoveDef& md = f.moveDef(mv);
        if (md.duration <= 0 || md.hitboxes.empty()) continue;
        if (slot >= 0) {  // a weapon move that is the bare-handed one (not the weapon's own) is fitted bare-handed
          bool own = false;
          if (const WeaponDef* w = f.weaponDef())
            for (const auto& [id, wm] : w->moves) own |= id == mv;
          if (!own) continue;
        }
        art::SheetMask rest;
        if (!art::sheetMaskAt(d, sheet, 0, rest)) continue;
        const float H = d.height;
        const bool air = mv >= NAir && mv <= DAir;
        const int n = int(md.hitboxes.size());
        std::vector<int> done(n, 0);
        for (int i = 0; i < n; ++i) {
          const HitboxDef& h = md.hitboxes[i];
          if (done[i] || h.end <= h.start) continue;
          ++total;
          const char* nm = art::sheetName(sheet);
          auto report = [&](int box, int frame, Vec2 o, float r0, Vec2 o1, float r1, float c0, float c1, const char* note) {
            std::fprintf(rep, "%s\t%d\t%s\t%d\t%d\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.3f\t%.3f\t%s\n", d.name.c_str(), slot,
                         nm, box, frame, o.x, o.y, r0, o1.x, o1.y, r1, c0, c1, note);
          };
          // The frame shown in the middle of its window.
          f.facing = 1;
          f.state = FState::Attack;
          f.move = mv;
          const int mid = (h.start + h.end - 1) / 2;
          f.moveFrame = f.stateFrame = mid;
          f.grounded = !air;
          const Pose p = computePose(f, m, 0.0f);
          art::SheetMask hit;
          if (!art::sheetMask(d, p, hit)) continue;
          if (h.swing) {  // a blade swing: turned to where the blade is drawn, its reach to the drawn tip
            done[i] = 1;
            const float blade = f.bladeLength(), am = swingAngle(h, float(mid));
            const BladeSeg s0 = swingSegment(h, am, H, blade);
            const float c0 = coverSeg(hit, H, s0.a, s0.b, h.radius);
            const Vec2 sh{kShoulderX * H, kShoulderY * H};
            // The blade (already drawn in the stance, so not "added"): the drawing far from the shoulder within the
            // sector the swing sweeps (widened by 30 degrees each side).
            const float lo = std::min(h.a0, h.a1) - 30.0f - am, hi = std::max(h.a0, h.a1) + 30.0f - am;
            std::vector<std::pair<float, float>> pts;  // distance from the shoulder, angle from the mid-swing angle
            const float k = H / hit.pxPerH;
            for (int y = 0; y < hit.h; ++y)
              for (int x = 0; x < hit.w; ++x) {
                if (!hit.opaque[size_t(y) * hit.w + x]) continue;
                const Vec2 v = Vec2{(x - hit.ax) * k, (y - hit.ay) * k + H * 0.5f} - sh;
                const float l = v.len(), da = std::fmod(rad2deg(std::atan2(-v.y, v.x)) - am + 540.0f, 360.0f) - 180.0f;
                if (l > kGripDist * H + 0.3f * blade && da >= lo && da <= hi) pts.push_back({l, da});
              }
            const float pxArea = (H / hit.pxPerH) * (H / hit.pxPerH);
            if (pts.size() * pxArea < 0.5f * h.radius * blade * 0.3f) {
              report(i, hit.frame, h.offset, h.radius, h.offset, h.radius, c0, c0, "swing, no blade drawn there: kept");
              continue;
            }
            std::sort(pts.begin(), pts.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
            const size_t keep = std::max<size_t>(8, std::min(pts.size(), pts.size() * 15 / 100));
            float sx = 0, sy = 0;
            for (size_t k = 0; k < keep; ++k) {
              sx += std::cos(deg2rad(pts[k].second));
              sy += std::sin(deg2rad(pts[k].second));
            }
            const float delta = clampf(rad2deg(std::atan2(sy, sx)), -60.0f, 60.0f);
            const float tip = pts[std::min(pts.size() - 1, keep / 10)].first - kGripDist * H;  // grip to the drawn tip
            HitboxDef h1 = h;
            h1.a0 += delta;
            h1.a1 += delta;
            h1.reach1 = clampf(tip / std::max(1.0f, blade), 0.8f * h.reach1, 1.3f * h.reach1);
            const BladeSeg s1 = swingSegment(h1, swingAngle(h1, float(mid)), H, blade);
            const float c1 = coverSeg(hit, H, s1.a, s1.b, h.radius);
            const bool better = c1 > c0 + 0.02f;
            char note[96];
            std::snprintf(note, sizeof note, better ? "swing turned %+.0f deg, reach %.2f" : "swing: no better, kept", delta,
                          h1.reach1);
            report(i, hit.frame, h.offset, h.radius, h.offset, h.radius, c0, better ? c1 : c0, note);
            if (!better) continue;
            std::fprintf(out, "    {\"%s\", %d, %d, %d, %.2ff, 0, 0, %.1ff, %.1ff, %.1ff, %.3ff},  // %s swing\n", d.name.c_str(),
                         slot, mv, i, h.radius, h.radius, h1.a0, h1.a1, h1.reach1, nm);
            ++moved;
            continue;
          }
          const float len = h.offset.len();
          const float c0 = coverOf(hit, H, h.offset, h.radius);
          if (len < 0.16f * H) {
            report(i, hit.frame, h.offset, h.radius, h.offset, h.radius, c0, c0, "on the body: kept");
            continue;
          }
          const Vec2 u = h.offset * (1.0f / len);
          // The blow's hitboxes pointing the same way (within 30 degrees) move together, led by the furthest out.
          std::vector<int> team;
          int lead = i;
          for (int j = i; j < n; ++j) {
            const HitboxDef& o = md.hitboxes[j];
            if (o.swing || o.end <= o.start || o.start >= h.end || o.end <= h.start) continue;
            const float l = o.offset.len();
            if (l < 0.16f * H || dot(o.offset * (1.0f / l), u) < 0.866f) continue;
            team.push_back(j);
            if (l > md.hitboxes[lead].offset.len()) lead = j;
          }
          for (int j : team) done[j] = 1;
          const HitboxDef& L = md.hitboxes[lead];
          const Vec2 lu = L.offset * (1.0f / L.offset.len());
          // What the blow adds, in front of the body in the lead's direction.
          std::vector<std::pair<float, Px>> far;
          for (const Px& q : added(hit, rest, H, 2)) {
            const Vec2 v{q.x, q.y};
            const float t = dot(v, lu), l = v.len();
            if (t > 0.25f * L.offset.len() && l > 1e-3f && t / l > 0.34f) far.push_back({t, q});
          }
          const float pxArea = (H / hit.pxPerH) * (H / hit.pxPerH);
          if (far.size() * pxArea < 0.35f * L.radius * L.radius) {  // nothing drawn out there: keep it
            for (int j : team) {
              const HitboxDef& o = md.hitboxes[j];
              const float c = coverOf(hit, H, o.offset, o.radius);
              report(j, hit.frame, o.offset, o.radius, o.offset, o.radius, c, c, "nothing added there: kept");
            }
            continue;
          }
          std::sort(far.begin(), far.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
          const size_t keep = std::max<size_t>(8, far.size() * 35 / 100);
          Vec2 c{0, 0};
          for (size_t k = 0; k < keep; ++k) c += Vec2{far[k].second.x, far[k].second.y};
          c = c * (1.0f / float(keep));
          std::vector<float> dist;
          for (size_t k = 0; k < keep; ++k) dist.push_back((Vec2{far[k].second.x, far[k].second.y} - c).len());
          std::sort(dist.begin(), dist.end());
          float r1 = clampf(dist[dist.size() * 85 / 100] + 2.0f, 0.8f * L.radius, 1.35f * L.radius);
          Vec2 shift = c - L.offset;
          const float maxShift = 0.45f * H;
          if (shift.len() > maxShift) shift = shift * (maxShift / shift.len());
          for (int j : team) {
            const HitboxDef& o = md.hitboxes[j];
            const Vec2 o1 = o.offset + shift;
            const float rr = j == lead ? r1 : clampf(o.radius * r1 / L.radius, 0.8f * o.radius, 1.35f * o.radius);
            const float cb = coverOf(hit, H, o.offset, o.radius), ca = coverOf(hit, H, o1, rr);
            const bool better = ca > cb + 0.02f;
            report(j, hit.frame, o.offset, o.radius, better ? o1 : o.offset, better ? rr : o.radius, cb, better ? ca : cb,
                   better ? (j == lead ? "fitted" : "moved with the blow") : "no better: kept");
            if (!better) continue;
            std::fprintf(out, "    {\"%s\", %d, %d, %d, %.2ff, %.1ff, %.1ff, %.1ff, 0, 0, 0},  // %s\n", d.name.c_str(), slot, mv,
                         j, o.radius, o1.x, o1.y, rr, nm);
            ++moved;
          }
        }
      }
  }
  std::fclose(out);
  std::fclose(rep);
  CloseWindow();
  std::printf("%d of %d hitboxes fitted -> %s (report %s)\n", moved, total, outPath.c_str(), reportPath.c_str());
  return 0;
}
