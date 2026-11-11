// The VS screen, between the arena and the fight: who fights whom, where, a tip (« Ligne d'horizon »: the sky's blue
// against the earth's gold, split by the horizon, slanted).
#include <vector>

#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr int kVsFrames = 165;  // then the fight; ENTRÉE skips it
constexpr FontKind kT = FontKind::Title;
constexpr Color kSand{255, 226, 181, 255};  // light text on the gold side
// The slanted horizon: from (0, 470) up to (1280, 290).
float lineY(float x) { return 470.0f - 180.0f * x / kScreenW; }
Vector2 lerp2(Vector2 a, Vector2 b, float f) { return {a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f}; }

// A convex polygon around c filled with a radial gradient: inner at c, mid at 45 % of (rx, ry), outer at 100 %.
void radialPoly(Vector2 c, float rx, float ry, Color inner, Color mid, Color outer, const std::vector<Vector2>& p) {
  auto at = [&](Vector2 q) {
    const float d = std::hypot((q.x - c.x) / rx, (q.y - c.y) / ry);
    return d < 0.45f ? mixColor(inner, mid, d / 0.45f) : mixColor(mid, outer, std::min(1.0f, (d - 0.45f) / 0.55f));
  };
  constexpr int kRings = 6;
  for (size_t i = 0; i < p.size(); ++i) {
    const Vector2 a = p[i], b = p[(i + 1) % p.size()];
    const int steps = std::max(1, int(std::hypot(b.x - a.x, b.y - a.y) / 40.0f));
    for (int k = 0; k < steps; ++k) {
      const Vector2 u = lerp2(a, b, float(k) / steps), v = lerp2(a, b, float(k + 1) / steps);
      for (int ring = 0; ring < kRings; ++ring) {
        const float f0 = float(ring) / kRings, f1 = float(ring + 1) / kRings;
        const Vector2 u0 = lerp2(c, u, f0), v0 = lerp2(c, v, f0), u1 = lerp2(c, u, f1), v1 = lerp2(c, v, f1);
        ui::quad(u0, v0, v1, u1, at(u0), at(v0), at(v1), at(u1));
      }
    }
  }
}

// A band along the slanted horizon, `off` px under it, `thick` px thick, from x0 to x1, colours at fractions `stops`.
void band(float x0, float x1, float off, float thick, const float* stops, const Color* cols, int n) {
  for (int i = 0; i + 1 < n; ++i) {
    const float l = x0 + (x1 - x0) * stops[i], r = x0 + (x1 - x0) * stops[i + 1];
    const float y0 = off - thick * 0.5f, y1 = off + thick * 0.5f;
    ui::quad({l, lineY(l) + y0}, {r, lineY(r) + y0}, {r, lineY(r) + y1}, {l, lineY(l) + y1}, cols[i], cols[i + 1],
             cols[i + 1], cols[i]);
  }
}
}  // namespace

void App::updateVersus(const MenuNav& nav) {
  if (!match_ || screenFrames_ >= kVsFrames || (screenFrames_ > 24 && (nav.confirm || nav.start))) goTo(Screen::Match);
}

void App::drawVersus() {
  if (!match_) return;
  const float t = time_;
  const Match& m = match_->match();
  const Settings& st = settings();

  // The sky's half, the earth's half, faint stripes drifting towards the line from both sides
  radialPoly({300, 300}, 700, 500, {36, 87, 184, 255}, {14, 42, 110, 255}, {5, 12, 38, 255},
             {{0, 0}, {float(kScreenW), 0}, {float(kScreenW), lineY(kScreenW)}, {0, lineY(0)}});
  radialPoly({1000, 520}, 700, 520, {201, 106, 30, 255}, {106, 50, 8, 255}, {30, 14, 6, 255},
             {{0, lineY(0)}, {float(kScreenW), lineY(kScreenW)}, {float(kScreenW), float(kScreenH)}, {0, float(kScreenH)}});
  const float drift = std::fmod(t * 5.2f, 26.0f);
  for (int k = 1; k < 30; ++k) {
    const float off = k * 26.0f - drift;
    DrawLineEx({0, lineY(0) - off}, {float(kScreenW), lineY(kScreenW) - off}, 2.0f, withAlpha(ui::kSky, 0.07f));
    DrawLineEx({0, lineY(0) + off}, {float(kScreenW), lineY(kScreenW) + off}, 2.0f, withAlpha(kSand, 0.07f));
  }

  // The sides: in a team battle, red against blue; otherwise one on the left, the next on the right...
  std::vector<const Fighter*> side[2];
  int shown = 0;
  for (const Fighter& f : m.fighters) side[m.cfg.teams ? (f.team & 1) : shown++ % 2].push_back(&f);
  auto ease = [&](int delay) {  // they slide in once the screen is uncovered
    const float u = std::clamp((screenFrames_ - delay) / 30.0f, 0.0f, 1.0f);
    return 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
  };
  for (int k = 0; k < 2; ++k) {
    const float e = ease(k == 0 ? 8 : 15);
    for (int i = int(side[k].size()) - 1; i >= 0; --i) {  // the second one of a side behind the first
      const Fighter& f = *side[k][i];
      const float h = i == 0 ? 470.0f : 330.0f;
      Vector2 feet = k == 0 ? Vector2{288, 541} : Vector2{1025, 712};
      if (side[k].size() > 1) feet = i > 0 ? (k == 0 ? Vector2{95, 480} : Vector2{1195, 640}) : Vector2{feet.x + (k == 0 ? 50.0f : -50.0f), feet.y};
      feet.x += (1.0f - e) * (k == 0 ? -60.0f : 60.0f);
      ui::glow({feet.x, feet.y - h * 0.5f}, h * 0.55f, withAlpha(slotColor(f), 0.4f));
      drawShowcase(*f.def, f.palette, restPose(*f.def, t + i), {feet.x, feet.y}, k == 0 ? 1 : -1, h, t + i);
    }
  }

  // The line between them, glowing; under it, the time left before the fight
  for (int s = -1; s <= 1; s += 2) {  // a soft halo on both sides of the line
    const Color in = withAlpha(WHITE, 0.22f), out = withAlpha(WHITE, 0.0f);
    for (int i = 0; i < 2; ++i) {
      const float l = -10.0f + 650.0f * i, r = l + 650.0f, y0 = 0.0f, y1 = 18.0f * s;
      ui::quad({l, lineY(l) + y0}, {r, lineY(r) + y0}, {r, lineY(r) + y1}, {l, lineY(l) + y1}, in, in, out, out);
    }
  }
  const float lineStops[5] = {0.0f, 0.15f, 0.5f, 0.85f, 1.0f};
  const Color lineCols[5] = {withAlpha(ui::kBlue, 0.0f), ui::kBlue, WHITE, ui::kGold, withAlpha(ui::kGold, 0.0f)};
  band(-10, kScreenW + 10, 0, 4, lineStops, lineCols, 5);
  const float p = std::clamp(screenFrames_ / (kVsFrames * 0.85f), 0.0f, 1.0f);
  if (p > 0.0f) {
    const Color barCols[3] = {withAlpha(WHITE, 0.0f), withAlpha(WHITE, 0.85f), withAlpha(WHITE, 0.0f)};
    const float barStops[3] = {0.0f, 0.92f, 1.0f};
    band(-10, -10 + (kScreenW + 20) * p, 11, 6, barStops, barCols, 3);
  }

  // "VS", its spark pulsing behind it
  const float pulse = 0.5f + 0.5f * std::sin(t * 2.0f * kPi / 1.6f);
  ui::eclat({640, 380}, 20.0f * (0.9f + 0.3f * pulse), 0.6f + 0.4f * pulse);
  const float in = ease(4), vs = 184.0f * (1.0f + 0.5f * (1.0f - in)), vw = textWidth("VS", vs, kT);
  const float vy = capsY(380, vs, kT);
  drawText("VS", 640 - vw * 0.5f, vy + 6, vs, withAlpha(ui::kInk, 0.85f * in), kT);
  if (in > 0.0f) ui::gradientText("VS", 640 - vw * 0.5f, vy, vs, {156, 203, 255, 255}, WHITE, {255, 200, 112, 255});

  // Who: the first fighter of each side, its player, number, title and style
  auto info = [&](const Fighter& f, float edge, float top, bool right) {
    const Fiche fi = fiche(*f.def);
    const Color c = slotColor(f), light = mixColor(c, WHITE, 0.45f), soft = right ? kSand : ui::kSoft;
    const char* who = f.cpu ? "CPU" : TextFormat("J%d", f.port + 1);
    const std::string sub = f.cpu ? (f.cpuLevel >= kMaxCpuLevel ? std::string("Niveau 10 x2") : TextFormat("Niveau %d", f.cpuLevel))
                                  : std::string(deviceName(slots_[f.port].device));
    auto at = [&](float w) { return right ? edge - w : edge; };  // left edge of something w wide
    const float cw = textWidth(who, 19, kT) + 20, sw = textWidth(sub.c_str(), 17);
    const float chipX = right ? edge - cw : edge, subX = right ? chipX - 10 - sw : edge + cw + 10;
    DrawRectangleRec({chipX, top, cw, 26}, c);
    drawText(who, chipX + 10, capsY(top + 13, 19, kT), 19, ui::kInk, kT);
    drawText(sub.c_str(), subX, capsY(top + 13, 17, FontKind::Ui), 17, right ? Color{255, 217, 160, 255} : ui::kSky);
    drawText(f.def->name.c_str(), at(textWidth(f.def->name.c_str(), 19, kT)), capsY(top + 46, 19, kT), 19, light, kT);
    // the title on two lines, at its first space
    const size_t sp = fi.nick.find(' ');
    const std::string l1 = fi.nick.substr(0, sp), l2 = sp == std::string::npos ? std::string() : fi.nick.substr(sp + 1);
    float ns = 63.0f;
    while (ns > 30.0f && std::max(textWidth(l1.c_str(), ns, kT), textWidth(l2.c_str(), ns, kT)) > 380.0f) ns -= 1.0f;
    float y = top + 62 + ns * 0.33f;
    for (const std::string& l : {l1, l2}) {
      if (l.empty()) continue;
      drawText(l.c_str(), at(textWidth(l.c_str(), ns, kT)), capsY(y, ns, kT), ns, ui::kCream, kT);
      y += ns * 0.9f;
    }
    const std::string meta = fi.style.empty() ? fi.art : fi.art + "  ·  " + fi.style;
    drawText(meta.c_str(), at(textWidth(meta.c_str(), 19)), capsY(y - ns * 0.25f + 2, 19, FontKind::Ui), 19, soft);
  };
  if (!side[0].empty()) info(*side[0][0], 520 - (1.0f - ease(8)) * 60.0f, 54, false);
  if (!side[1].empty()) info(*side[1][0], 830 + (1.0f - ease(15)) * 60.0f, 480, true);
  // The second fighter of a side, in a line: under the first one's (left), above it (right)
  for (int k = 0; k < 2; ++k) {
    if (side[k].size() < 2) continue;
    const Fighter& f = *side[k][1];
    const std::string who = f.cpu ? std::string("CPU") : TextFormat("J%d", f.port + 1);
    const std::string what = f.def->name + "  " + fiche(*f.def).nick;
    const float cw = textWidth(who.c_str(), 15, kT) + 14, ww = textWidth(what.c_str(), 17, kT);
    const float y = k == 0 ? 280.0f : 448.0f, x = k == 0 ? 520.0f : 830.0f - cw - 10 - ww;
    DrawRectangleRec({x, y - 10, cw, 20}, slotColor(f));
    drawText(who.c_str(), x + 7, capsY(y, 15, kT), 15, ui::kInk, kT);
    drawText(what.c_str(), x + cw + 10, capsY(y, 17, kT), 17, k == 0 ? ui::kSoft : kSand, kT);
  }

  // A tip, top right
  std::vector<const char*> tips;
  if (m.cfg.items) tips.push_back("Casse la Clash Orb quand elle passe : c'est la seule façon d'obtenir ton ultime.");
  if (m.cfg.weapons) {
    tips.push_back("Tes deux armes tombent sur l'arène : reste dessus, touche Arme maintenue 1 s, pour la prendre.");
    tips.push_back("Avec tes deux armes, la touche Arme passe de l'une à l'autre, sans délai.");
  }
  tips.push_back("Une compétence se recharge en 2,25 s ; un coup qui touche s'enchaîne tout de suite.");
  const std::string tip = tips[size_t(vsTip_) % tips.size()];
  const float tipW = 268, tipH = ui::drawWrapped(tip, 0, 0, tipW, 19, WHITE, false) + 58;
  const Rectangle tb{948, 32, 300, tipH};
  ui::card(tb, 1.0f, withAlpha({5, 12, 38, 255}, 0.7f), withAlpha({5, 12, 38, 255}, 0.7f));
  const float aw = textWidth("ASTUCE", 13, kT) + 16;
  DrawRectangleRec({tb.x + 16, tb.y + 14, aw, 20}, ui::kGold);
  drawText("ASTUCE", tb.x + 24, capsY(tb.y + 24, 13, kT), 13, ui::kInk, kT);
  ui::drawWrapped(tip, tb.x + 16, tb.y + 44, tipW, 19, ui::kCream);

  // Where: the arena, the rules; how to skip
  const int si = std::clamp(m.cfg.stage, 0, int(stageList().size()) - 1);
  const StageDef& stage = stageList()[si];
  const float bw = 360, blurbH = ui::drawWrapped(stage.blurb, 0, 0, bw, 17, WHITE, false);
  const float by = 700 - (58 + blurbH), tx = 186;
  const Rectangle thumb{40, std::min(by, 700.0f - 74.0f), 132, 74};
  DrawRectangleRec(ui::grow(thumb, 2), kSand);
  drawStageThumb(si, stage, thumb);
  const std::string rules = TextFormat("ARÈNE  ·  %d VIE%s%s%s", st.stocks, st.stocks > 1 ? "S" : "", m.cfg.items ? "  ·  ORBE" : "",
                                       m.cfg.weapons ? "  ·  ARMES" : "");
  drawText(rules.c_str(), tx, capsY(by + 8, 15, FontKind::Ui), 15, kSand);
  drawText(upperFr(stage.name).c_str(), tx, capsY(by + 32, 33, kT), 33, ui::kCream, kT);
  ui::drawWrapped(stage.blurb, tx, by + 50, bw, 17, kSand);
  const float hy = thumb.y - 34;
  const float kw = ui::prompt("ENTRÉE", ui::Btn::A, 40, hy - 11, 22);
  drawText("PASSER", 40 + kw + 10, capsY(hy, 14, kT), 14, withAlpha(kSand, 0.85f), kT);
}
