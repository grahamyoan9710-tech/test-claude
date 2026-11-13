// The VS screen, between the arena and the fight (« Prestige »): each side in its fighter's flat colour, his name
// scrolling tone on tone in it, the two meeting on one white blade; the fighters face to face in their own pixel art
// (x4), their names on plates of their deep colour like a broadcast lower third, VS slammed on a night plate; where
// and how along the bottom.
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
  const float t = time_, sec = screenFrames_ / 60.0f;
  const Match& m = match_->match();
  const Settings& st = settings();
  auto in = [&](float delay, float len, float x1 = 0.2f, float y1 = 0.9f, float x2 = 0.1f) {
    return ui::seg(sec, delay, delay + len, x1, y1, x2, 1.0f);
  };
  DrawRectangle(0, 0, kScreenW, kScreenH, ui::kNight);

  // The sides: in a team battle, red against blue; otherwise one on the left, the next on the right...
  std::vector<const Fighter*> side[2];
  int shown = 0;
  for (const Fighter& fi : m.fighters) side[m.cfg.teams ? (fi.team & 1) : shown++ % 2].push_back(&fi);

  // The shake as VS lands
  Vector2 sh{0, 0};
  if (sec >= 0.85f && sec < 1.3f) {
    static const float kt[] = {0.0f, 0.18f, 0.36f, 0.56f, 1.0f};
    static const Vector2 kv[] = {{-10, 5}, {8, -6}, {-5, 4}, {3, -2}, {0, 0}};
    const float u = (sec - 0.85f) / 0.45f;
    for (int k = 0; k + 1 < 5; ++k)
      if (u >= kt[k] && u < kt[k + 1]) {
        const float v = (u - kt[k]) / (kt[k + 1] - kt[k]);
        sh = {kv[k].x + (kv[k + 1].x - kv[k].x) * v, kv[k].y + (kv[k + 1].y - kv[k].y) * v};
      }
  }
  rlPushMatrix();
  rlTranslatef(sh.x, sh.y, 0.0f);

  // Each side's colour slides in, its name scrolling in it, outlined tone on tone
  for (int k = 0; k < 2; ++k) {
    const bool L = k == 0;
    const Fighter* lead = side[k].empty() ? nullptr : side[k][0];
    const ui::Flood fl = lead ? ui::flood(lead->def->hero) : ui::Flood{ui::kDim, ui::kNightUp};
    const float ox = (L ? -760.0f : 760.0f) * (1.0f - in(0.0f, 0.36f, 0.7f, 0.0f, 0.2f));
    if (L) {
      ui::lean(-400 + ox, 596 + ox, fl.fill);
      ui::lean(596 + ox, 713 + ox, fl.deep);
    } else {
      ui::lean(836 + ox, 1700 + ox, fl.fill);
      ui::lean(719 + ox, 836 + ox, fl.deep);
    }
    if (!lead) continue;
    const std::string word = upperFr(fiche(*lead->def).nick) + " ";
    const float ww = ui::bigWidth(word.c_str(), 300, ui::Face::Cond);
    ui::bigClip(L ? 713 + ox : 719 + ox, L);
    for (int row = 0; row < 2; ++row) {
      const bool back = (row == 0) != L;  // left: the top row runs left, the bottom one right; mirrored on the right
      const float len = row == 0 ? 30.0f : 40.0f, off = std::fmod(t / len, 1.0f) * 3.0f * ww;
      const float x0 = back ? -3.0f * ww + off : -off;
      for (int r = 0; r < 6; ++r)
        ui::bigHollow(word.c_str(), x0 + r * ww + ox, row == 0 ? 40.0f : 330.0f, 300, ui::Face::Cond, 2.0f,
                      withAlpha(ui::kNight, 0.18f));
    }
    ui::bigClipOff();
  }
  if (sec >= 0.3f) {  // the blade, growing from its middle
    const float g = in(0.3f, 0.3f), h = 360.0f * g;
    const Color c = withAlpha(ui::kSilver, g);
    ui::quad({713 - ui::kLean * (360 - h), 360 - h}, {719 - ui::kLean * (360 - h), 360 - h}, {719 - ui::kLean * (360 + h), 360 + h},
             {713 - ui::kLean * (360 + h), 360 + h}, c, c, c, c);
  }

  // The fighters, face to face in their own pixel art (x4); the second of a side smaller, behind
  for (int k = 0; k < 2; ++k) {
    const bool L = k == 0;
    const float a = in(0.32f, 0.55f), dx = (L ? -140.0f : 140.0f) * (1.0f - a);
    for (int i = std::min(1, int(side[k].size()) - 1); i >= 0; --i) {
      const CharacterDef& d = *side[k][i]->def;
      int n;
      float fw, fh, fps;
      if (!art::clipInfo(d, art::kSheetIdle, n, fw, fh, fps)) continue;
      const float sc = i == 0 ? 4.0f : 3.0f, w = fw * sc, out = i == 0 ? 0.0f : 170.0f;
      const float x = L ? 60.0f - out : 1244.0f - w + out;
      art::drawClipPx(d, art::kSheetIdle, int(std::floor((t + i * 0.37f) * fps)) % std::max(1, n), x + dx, 720.0f - fh * sc, sc, !L, a);
    }
  }

  // Who fights: player and number, then the name on a plate of his deep colour (its edge cut like the blade)
  for (int k = 0; k < 2; ++k) {
    if (side[k].empty()) continue;
    const bool R = k == 1;
    const Fighter& fr = *side[k][0];
    const ui::Flood fl = ui::flood(fr.def->hero);
    const std::vector<std::string> lines = ui::nameLines(upperFr(fiche(*fr.def).nick), 96, 360);
    const float nh = 12.0f + 0.84f * 96 * lines.size() + 6.0f, bottom = 646.6f, top = bottom - nh;
    float nw = 0.0f;
    for (const auto& l : lines) nw = std::max(nw, ui::bigWidth(l.c_str(), 96, ui::Face::Cond));
    const float pw = nw + 44.0f;  // padding 14 + 30
    const float na = in(R ? 0.65f : 0.55f, 0.45f), ny = 20.0f * (1.0f - na);
    const float px = R ? kScreenW - 40.0f - pw : 40.0f, py = top + ny;
    const Color plate = withAlpha(fl.deep, na);
    if (R) ui::quad({px + 24, py}, {px + pw, py}, {px + pw, py + nh}, {px, py + nh}, plate, plate, plate, plate);
    else ui::quad({px, py}, {px + pw, py}, {px + pw - 24, py + nh}, {px, py + nh}, plate, plate, plate, plate);
    for (size_t i = 0; i < lines.size(); ++i) {
      const float lw = ui::bigWidth(lines[i].c_str(), 96, ui::Face::Cond);
      ui::big(lines[i].c_str(), R ? px + pw - 14 - lw : px + 14, py + 12 + 0.84f * 96 * i, 96, ui::Face::Cond, withAlpha(WHITE, na));
    }
    // The label above: its player's tag (J1 red, a CPU grey) and the number, night under it
    const float la = in(R ? 0.6f : 0.5f, 0.4f, 0.2f, 0.8f, 0.2f), ly = top - 36.0f + 12.0f * (1.0f - la);
    std::string tags = fr.cpu ? "CPU" : TextFormat("J%d", fr.port + 1);
    const Fiche fi = fiche(*fr.def);
    const std::string lab = "N° " + fr.def->id.substr(1) + (fi.art.empty() ? std::string() : " · " + upperFr(fi.art));
    const float tw = 18.0f + ui::capsWidth(tags.c_str(), 11, 0.08f) + 0.88f, lw = ui::capsWidth(lab.c_str(), 13, 0.3f) + 3.9f;
    const float bw = tw + 12.0f + lw + 9.0f, bx = R ? kScreenW - 40.0f - bw : 40.0f;
    DrawRectangleRec({bx, ly, bw, 28}, withAlpha(ui::kNight, la));
    const float tx = R ? bx + bw - tw : bx, lx = R ? bx + 9.0f : bx + tw + 12.0f;
    DrawRectangleRec({tx, ly, tw, 28}, withAlpha(fr.cpu ? ui::kCpuTag : slotColor(fr), la));
    ui::caps(tags.c_str(), tx + 9, ly + 14, 11, 0.08f, withAlpha(ui::kNight, la));
    ui::caps(lab.c_str(), lx, ly + 14, 13, 0.3f, withAlpha(ui::kSilver, la));
  }

  // VS: a night plate slams onto the blade, the letters land a beat later
  if (sec >= 0.78f) {
    const float g = in(0.78f, 0.3f), w = 320.0f * g, x = 640.0f - w * 0.5f, c = 34.0f * g;
    ui::quad({x + c, 262}, {x + w, 262}, {x + w - c, 422}, {x, 422}, ui::kNight, ui::kNight, ui::kNight, ui::kNight);
  }
  if (sec >= 0.85f) {
    const float u = (sec - 0.85f) / 0.5f;
    const float sc = u < 0.3f ? 1.7f - 0.74f * ui::seg(u, 0.0f, 0.3f) : u < 0.55f ? 0.96f + 0.06f * ui::seg(u, 0.3f, 0.55f)
                     : 1.02f - 0.02f * ui::seg(u, 0.55f, 1.0f);
    const float al = u < 0.3f ? ui::seg(u, 0.0f, 0.3f) : 1.0f, vw = ui::bigWidth("VS", 132, ui::Face::Wide);
    ui::scaled(sc, 640, 276 + 66, [&] { ui::big("VS", 640 - vw * 0.5f, 276, 132, ui::Face::Wide, withAlpha(WHITE, al), 1.0f); });
  }
  rlPopMatrix();

  // The lower third: where, and how
  {
    const float k = in(1.05f, 0.45f), y = 664.0f + 56.0f * (1.0f - k);
    DrawRectangleRec({0, y, float(kScreenW), 56}, ui::kNight);
    const int si = std::clamp(m.cfg.stage, 0, int(stageList().size()) - 1);
    const std::string parts[3] = {upperFr(stageList()[si].name), TextFormat("%d VIE%s", st.stocks, st.stocks > 1 ? "S" : ""),
                                  m.cfg.teams ? "EN ÉQUIPES" : "CHACUN POUR SOI"};
    float total = 0.0f;
    for (const auto& p : parts) total += ui::capsWidth(p.c_str(), 13, 0.32f) + 4.16f;
    total += 2 * (26.0f + 6.0f + 26.0f);
    float x = (kScreenW - total) * 0.5f;
    for (int i = 0; i < 3; ++i) {
      x += ui::caps(parts[i].c_str(), x, y + 28, 13, 0.32f, ui::kSilver) + 4.16f;
      if (i < 2) {
        ui::diamond({x + 26 + 3, y + 28}, 4.2f, ui::kSilver);
        x += 26.0f + 6.0f + 26.0f;
      }
    }
  }
}
