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

