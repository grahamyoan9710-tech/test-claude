// The VS screen, between the arena and the fight (« Épure », one of its bold moments): the screen split by a slanted
// line, each side in its fighter's colour with a screentone, its name huge and hollow drifting behind, two slabs, the
// manga portrait (or the pixel art); "VS" in a disc; who fights at the bottom; where, and a bar till the fight.
#include <vector>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr int kVsFrames = 165;  // then the fight; ENTRÉE skips it
}  // namespace

void App::updateVersus(const MenuNav& nav) {
  if (!match_ || screenFrames_ >= kVsFrames || (screenFrames_ > 24 && (nav.confirm || nav.start))) goTo(Screen::Match);
}

void App::drawVersus() {
  if (!match_) return;
  const float t = time_, f = float(screenFrames_);
  const Match& m = match_->match();
  const Settings& st = settings();
  constexpr FontKind D = FontKind::Display, B = FontKind::Body;
  auto ease = [&](float delay, float len) {
    const float u = clampf((f - delay) / len, 0.0f, 1.0f);
    return 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
  };

  // The sides: in a team battle, red against blue; otherwise one on the left, the next on the right...
  std::vector<const Fighter*> side[2];
  int shown = 0;
  for (const Fighter& fi : m.fighters) side[m.cfg.teams ? (fi.team & 1) : shown++ % 2].push_back(&fi);

  // The split: from (742, 0) down to (538, 720)
  const Vector2 top{742, 0}, bot{538, 720};
  auto leftOf = [&](Vector2 p) { return (bot.x - top.x) * (p.y - top.y) - (bot.y - top.y) * (p.x - top.x) > 0.0f; };
  auto clipDraw = [&](std::vector<Vector2> poly, bool left, Color col) {  // a convex polygon, cut by the split line
    std::vector<Vector2> out;
    for (size_t i = 0; i < poly.size(); ++i) {
      const Vector2 a = poly[i], b = poly[(i + 1) % poly.size()];
      const bool ia = leftOf(a) == left, ib = leftOf(b) == left;
      if (ia) out.push_back(a);
      if (ia != ib) {  // where a-b crosses the line
        const float d1 = (bot.x - top.x) * (a.y - top.y) - (bot.y - top.y) * (a.x - top.x);
        const float d2 = (bot.x - top.x) * (b.y - top.y) - (bot.y - top.y) * (b.x - top.x);
        const float u = d1 / (d1 - d2);
        out.push_back({a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u});
      }
    }
    for (size_t i = 1; i + 1 < out.size(); ++i) ui::quad(out[0], out[i], out[i + 1], out[i + 1], col, col, col, col);
  };
  for (int k = 0; k < 2; ++k) {
    const bool L = k == 0;
    const Fighter* lead = side[k].empty() ? nullptr : side[k][0];
    const Color cc = lead ? auraColor(lead->def->hero) : ui::kStone;
    const float in = ease(0, 45), ox = (L ? -90.0f : 90.0f) * (1.0f - in);
    // Its ground: the fighter's colour, deep, darkening to the dusk
    const Color c0 = mixColor({18, 10, 16, 255}, cc, 0.24f), c1 = mixColor({16, 10, 16, 255}, cc, 0.07f), c2 = ui::kDeep;
    if (L) ui::quad({0 + ox, 0}, {top.x + ox, 0}, {bot.x + ox, 720}, {0 + ox, 720}, c0, c1, c2, c1);
    else ui::quad({top.x + ox, 0}, {1280 + ox, 0}, {1280 + ox, 720}, {bot.x + ox, 720}, c1, c0, c1, c2);
    if (ox != 0.0f) DrawRectangleRec({L ? 0.0f : 1280.0f + ox, 0, std::fabs(ox), 720}, c0);
    // The screentone, fading away from its corner
    for (float y = 6; y < 420; y += 12)
      for (float x = 6; x < 1280; x += 12) {
        const float proj = (L ? x : 1280 - x) * 0.342f + y * 0.94f, a = 1.0f - proj / (0.55f * 1114.0f);
        if (a <= 0.0f || leftOf({x, y}) != L) continue;
        DrawRectangleRec({x - 1.3f + ox, y - 1.3f, 2.6f, 2.6f}, withAlpha(ui::kCream, 0.18f * a * in));
      }
    if (!lead) continue;
    // Its name, huge and faint, drifting
    const std::string name = upperFr(fiche(*lead->def).nick);
    const float gw = textWidth(name.c_str(), 200, D), drift = 60.0f - 120.0f * clampf(f / 480.0f, 0.0f, 1.0f);
    const float gx = (L ? -20.0f : 1300.0f - gw) + drift + ox;
    beginClip(L ? Rectangle{0, 0, top.x, 720} : Rectangle{bot.x, 0, 1280 - bot.x, 720});
    drawText(name.c_str(), gx, 24, 200, withAlpha(cc, 0.13f * in), D);
    endClip();
    // Two slabs growing from their outer end: the fighter's colour, a thin cream one
    const float g = ease(6, 36), ang = -15.8f * kPi / 180.0f, ca = std::cos(ang), sa = std::sin(ang);
    auto slab = [&](Vector2 origin, float len, float y0, float h, Color col, bool fromRight) {
      const float l = len * g;
      Vector2 p[4];
      const float xs[2] = {fromRight ? -l : 0.0f, fromRight ? 0.0f : l};
      const float ys[2] = {y0, y0 + h};
      for (int j = 0; j < 4; ++j) {
        const float lx = xs[j == 1 || j == 2], ly = ys[j >= 2];
        p[j] = {origin.x + ox + lx * ca - ly * sa, origin.y + lx * sa + ly * ca};
      }
      clipDraw({p[0], p[1], p[2], p[3]}, L, col);
    };
    if (L) {
      slab({-160, 465}, 1000, -65, 130, cc, false);
      slab({-160, 567}, 1000, -7, 14, withAlpha(ui::kCream, 0.85f), false);
    } else {
      slab({1440, 235}, 1000, -65, 130, cc, true);
      slab({1440, 147}, 1000, -7, 14, withAlpha(ui::kCream, 0.85f), true);
    }
    // The fighters: the second of a side small behind, the first big; the manga portrait when there is one
    for (int i = int(side[k].size()) - 1; i >= 0; --i) {
      const Fighter& fr = *side[k][i];
      const float dr = (L ? -24.0f : 24.0f) * (1.0f - ease(0, 360));
      float aw, ah;
      if (i == 0 && art::pictureSize((fr.def->id + "_art").c_str(), aw, ah)) {
        const float h = 700, w = h * aw / ah, x = L ? 70.0f : 1260.0f - w;
        art::drawPictureFaded((fr.def->id + "_art").c_str(), {x + dr + ox, 720 - h, w, h}, L ? 0.1f : -0.1f, 0.4f, in, !L, 0.06f);
      } else {
        const float h = i == 0 ? 540.0f : 320.0f;
        const float fx = (L ? (i == 0 ? 300.0f : 110.0f) : (i == 0 ? 1000.0f : 1180.0f)) + dr + ox;
        ui::glow({fx, 720 - h * 0.45f}, h * 0.5f, withAlpha(cc, 0.3f * in));
        drawShowcase(*fr.def, fr.palette, restPose(*fr.def, t + i), {fx, 712.0f}, L ? 1 : -1, h, t + i);
      }
    }
  }

  // A shade along the bottom, under the names
  DrawRectangleGradientV(0, 430, kScreenW, 290, withAlpha(ui::kDeep, 0.0f), withAlpha(ui::kDeep, 0.85f));

  // The split line, drawing from its middle; "VS" in its disc, popping in
  {
    const float g = ease(12, 36), mx = (top.x + bot.x) * 0.5f, my = 360.0f;
    const Vector2 a{mx + (top.x - mx) * 1.03f * g, my + (top.y - 16 - my) * g}, b{mx + (bot.x - mx) * 1.03f * g, my + (bot.y + 16 - my) * g};
    DrawLineEx(a, b, 14.0f, withAlpha(ui::kCream, 0.12f));
    DrawLineEx(a, b, 6.0f, withAlpha(ui::kCream, 0.25f));
    DrawLineEx(a, b, 2.0f, ui::kCream);
    const float u = clampf((f - 21) / 33.0f, 0.0f, 1.0f);
    if (u > 0.0f) {  // a back-out ease: it lands a little small, then settles
      const float c1 = 1.70158f * 1.3f, c3 = c1 + 1.0f, e = 1.0f + c3 * std::pow(u - 1.0f, 3.0f) + c1 * std::pow(u - 1.0f, 2.0f);
      const float sc = 1.8f - 0.8f * e, al = clampf(u * 2.0f, 0.0f, 1.0f);
      DrawCircleV({640, 360}, 66 * sc + 1, withAlpha(ui::kCream, 0.4f * al));
      DrawCircleV({640, 360}, 66 * sc, withAlpha(ui::kDeep, al));
      const float vs = 44 * sc;
      drawText("VS", 640 - textWidth("VS", vs, D) * 0.5f, capsY(360, vs, D), vs, withAlpha(ui::kCream, al), D);
    }
  }

  // Who fights: their player, number and martial art, name and style; the second of a side on a line above
  const float nIn = ease(27, 36);
  auto who = [&](const Fighter& fr, bool right, float base, bool small) {
    const Fiche fi = fiche(*fr.def);
    const float x = right ? 1216.0f : 64.0f, dy = 24.0f * (1.0f - nIn);
    const char* tag = fr.cpu ? TextFormat("CPU %d", fr.cpuLevel) : TextFormat("J%d", fr.port + 1);
    const std::string tagS = tag;
    const std::string lab = "N° " + fr.def->id.substr(1) + (fi.art.empty() ? std::string() : " · " + upperFr(fi.art));
    const float tw = ui::labelWidth(tagS.c_str(), 11, 0.04f) + 16, lw = ui::labelWidth(lab.c_str(), 11, 0.16f);
    const float ty = base + dy;
    const float tx = right ? x - tw : x, lx = right ? tx - 10 - lw : x + tw + 10;
    const Color pc = fr.cpu ? Color{163, 167, 175, 255} : slotColor(fr);
    ui::roundRect({tx, ty, tw, 22}, 6, withAlpha(pc, nIn), withAlpha(pc, nIn));
    ui::label(tagS.c_str(), tx + 8, ty + 11, 11, 0.04f, withAlpha(Color{10, 11, 14, 255}, nIn));
    ui::label(lab.c_str(), lx, ty + 11, 11, 0.16f, withAlpha(ui::kSoft, nIn));
    if (small) return;
    const std::string name = upperFr(fi.nick);
    float ns = 72.0f;
    while (ns > 30.0f && textWidth(name.c_str(), ns, D) > 520.0f) ns -= 1.0f;
    const float nw = textWidth(name.c_str(), ns, D);
    drawText(name.c_str(), right ? x - nw : x, capsY(ty + 62, ns, D), ns, withAlpha(ui::kCream, nIn), D);
    const float sw = textWidth(fi.style.c_str(), 15, B);
    drawText(fi.style.c_str(), right ? x - sw : x, capsY(ty + 108, 15, B), 15, withAlpha(ui::kSoft, nIn), B);
  };
  for (int k = 0; k < 2; ++k) {
    if (side[k].empty()) continue;
    who(*side[k][0], k == 1, 506, false);
    if (side[k].size() > 1) who(*side[k][1], k == 1, 470, true);
  }

  // Where and how, the bar till the fight; a tip at the top; how to skip
  const int si = std::clamp(m.cfg.stage, 0, int(stageList().size()) - 1);
  const std::string rules = upperFr(stageList()[si].name) + TextFormat(" · %d VIE%s · ", st.stocks, st.stocks > 1 ? "S" : "") +
                            (m.cfg.teams ? "EN ÉQUIPES" : "CHACUN POUR SOI");
  ui::label(rules.c_str(), (kScreenW - ui::labelWidth(rules.c_str(), 11, 0.24f)) * 0.5f, 652, 11, 0.24f, ui::kSoft);
  DrawRectangleRec({530, 676, 220, 2}, withAlpha(ui::kCream, 0.15f));
  const float load = clampf((f - 18) / (kVsFrames - 30.0f), 0.0f, 1.0f), le = load < 0.5f ? 2 * load * load : 1 - std::pow(-2 * load + 2, 2) / 2;
  DrawRectangleRec({530, 676, 220 * le, 2}, ui::kCream);
  std::vector<const char*> tips;
  if (m.cfg.items) tips.push_back("Casse la Clash Orb quand elle passe : c'est la seule façon d'obtenir ton ultime.");
  if (m.cfg.weapons) {
    tips.push_back("Tes deux armes tombent sur l'arène : reste dessus, touche Arme maintenue 1 s, pour la prendre.");
    tips.push_back("Avec tes deux armes, la touche Arme passe de l'une à l'autre, sans délai.");
  }
  tips.push_back("Une compétence se recharge en 2,25 s ; un coup qui touche s'enchaîne tout de suite.");
  const std::string tip = std::string("ASTUCE  ·  ") + tips[size_t(vsTip_) % tips.size()];
  const float tipA = ease(40, 30);
  drawText(tip.c_str(), (kScreenW - textWidth(tip.c_str(), 13, B)) * 0.5f, capsY(26, 13, B), 13, withAlpha(ui::kSoft, tipA), B);
  const float kw = ui::promptWidth("ENTRÉE", ui::Btn::A, 22), pw = textWidth("Passer", 12, B);
  ui::prompt("ENTRÉE", ui::Btn::A, kScreenW - 40 - pw - 8 - kw, 676, 22);
  drawText("Passer", kScreenW - 40 - pw, capsY(687, 12, B), 12, withAlpha(ui::kMuted, tipA), B);
}
