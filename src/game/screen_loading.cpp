// The launch screen (« Prestige », the mockup validated on 13/11), as big games prepare their shaders before the
// title: every fighter's clips, every effect and every picture is loaded and every shader compiled while the word
// CHARGEMENT fills, so that nothing is decoded or compiled in the middle of a fight or a transition (each effect used
// to be decoded the first time it played). The logo top left, the night raised on the right with a tip.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/pixel.hpp"
#include "game/render.hpp"
#include "game/rtx.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kB = FontKind::Body;
constexpr double kBudget = 1.0 / 90.0;  // seconds of loading per frame: the word keeps filling
constexpr int kOutroFrames = 30;         // « PRÊT » holds, then the title
constexpr Color kRim{214, 226, 255, 255};
// Only what the game really does.
const char* kTips[] = {
    "Casse la Clash Orb : c'est la seule façon d'obtenir ton ultime.",
    "Les armes tombent sur l'arène : reste dessus et maintiens la touche Arme 1 s pour la prendre.",
    "Avec tes deux armes, la touche Arme passe de l'une à l'autre, sans attendre.",
    "Attaque et Spéciale ensemble : une attaque smash. Sur une manette, le stick droit aussi.",
    "Paramètres, Commandes : choisis ton clavier ou ta manette, puis change ses touches.",
    "Manette PlayStation vue comme une Xbox (Steam Input) ? Paramètres, Commandes : PS4 ou PS5.",
    "F11 : plein écran ou fenêtre, à tout moment.",
};
constexpr int kTipCount = int(sizeof(kTips) / sizeof(kTips[0]));
}  // namespace

void App::updateLoading() {
  if (loadTip_ < 0) loadTip_ = int(std::time(nullptr) % kTipCount);
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || padAnyPressed(kPadA)) loadTip_ = (loadTip_ + 1) % kTipCount;
  const int n = art::preloadCount() + 1;  // item 0: the shaders
  if (loadNext_ >= n) {
    if (loadDoneAt_ < 0) {
      loadDoneAt_ = frames_;
      std::printf("Chargement : %d éléments en %.2f s\n", n, loadSeconds_);
    }
    if (frames_ - loadDoneAt_ >= kOutroFrames) enterScreen(Screen::Title);
    return;
  }
  const double t0 = GetTime();
  do {
    if (loadNext_ == 0) {
      pix::warm();
      light::warm();
      rtx::warm();
    } else {
      art::preload(loadNext_ - 1);
    }
    ++loadNext_;
  } while (loadNext_ < n && GetTime() - t0 < kBudget);
  loadSeconds_ += GetTime() - t0;
}

void App::drawLoading() {
  const float t = time_, sec = screenFrames_ / 60.0f;
  const int n = art::preloadCount() + 1, loaded = std::min(loadNext_, n);
  const float p = float(loaded) / float(n);
  const float out = loadDoneAt_ < 0 ? 0.0f : clampf((frames_ - loadDoneAt_ - 12) / float(kOutroFrames - 12), 0.0f, 1.0f);
  auto up = [&](float delay) { return ui::seg(sec, delay, delay + 0.5f, 0.2f, 0.8f, 0.2f); };  // .ld-up: fades in, rising
  ClearBackground(ui::kNight);

  // The night raised on the right, cut at the 12° blade, « CLASH HORIZON » drifting hollow in it
  {
    const float dx = 480.0f * (1.0f - ui::seg(sec, 0.0f, 0.6f));
    ui::lean(930 + dx, 1500 + dx, ui::kNightUp);
    const char* echo = "CLASH HORIZON CLASH HORIZON CLASH HORIZON CLASH HORIZON ";
    const float half = ui::bigWidth(echo, 220, ui::Face::Cond) * 0.5f;  // a loop: half of it, two names
    ui::bigClip(930 + dx, false);
    const float x1 = -half * std::fmod(t / 40.0f, 1.0f), x2 = -half * (1.0f - std::fmod(t / 56.0f, 1.0f));
    ui::bigHollow(echo, x1 + dx, 40, 220, ui::Face::Cond, 1.5f, withAlpha(kRim, 0.07f));
    ui::bigHollow(echo, x2 + dx, 300, 220, ui::Face::Cond, 1.5f, withAlpha(kRim, 0.07f));
    ui::bigClipOff();
    ui::lean(926 + dx, 930 + dx, ui::kSilver);
  }

  // The logo, as on the main menu, top left
  {
    const float a = up(0.0f);
    ui::LogoIn in;
    in.word = a;
    in.lines = a;
    in.cut = ui::seg(sec, 0.0f, 0.6f);
    ui::prestigeLogo(-48, 24 + 14.0f * (1.0f - a), 0.42f, t, in);
  }

  // The tip, on the night raised
  {
    const float a = up(0.4f), y = 470 + 14.0f * (1.0f - a);
    ui::caps("ASTUCE", 868, y + 0.534f * 11, 11, 0.22f, withAlpha(ui::kBlue, a));
    float ty = y + 11.97f + 8;
    for (const std::string& line : ui::wrapText(kTips[std::max(loadTip_, 0) % kTipCount], 370, 15, kB)) {
      drawText(line.c_str(), 868, ty + (21.75f - 1.2109f * 15) * 0.5f, 15, withAlpha(ui::kSilver, a), kB);  // 1.45
      ty += 21.75f;
    }
    const float ky = ty + 8 + 6 + 12;  // the gap, the key row's margin, half the key
    const float kw = padPrompts() ? ui::padGlyph(ui::Btn::A, 868, ky - 11, 22, promptFamily())
                                  : ui::keyCap("ENTRÉE", 868, ky, 24, 10, 0.4f, a);
    drawText("Astuce suivante", 868 + kw + 8, capsY(ky, 12, kB), 12, withAlpha(ui::kMist, a), kB);
  }

  // The word fills as the game loads; the cyan head is where it is
  {
    const float a = up(0.15f), y = 438 + 14.0f * (1.0f - a);
    const float w = ui::bigWidth("CHARGEMENT", 120, ui::Face::Cond) * p;
    ui::bigHollow("CHARGEMENT", 52, y, 120, ui::Face::Cond, 1.5f, withAlpha(ui::kMist, 0.55f * a));
    if (w > 0.5f) {
      beginClip({52, y - 20, w, 140});
      ui::big("CHARGEMENT", 52, y, 120, ui::Face::Cond, withAlpha(WHITE, a));
      endClip();
    }
    if (p < 1.0f) {  // skewed 12°, 8 x 112, 6 px after the fill
      const float hx = 52 + w + 6, k = 56.0f * ui::kLean;
      ui::quad({hx + k, y - 6}, {hx + 8 + k, y - 6}, {hx + 8 - k, y + 106}, {hx - k, y + 106}, withAlpha(ui::kBlue, a),
               withAlpha(ui::kBlue, a), withAlpha(ui::kBlue, a), withAlpha(ui::kBlue, a));
    }
  }
  // What is loading, how many, how far; the bar under it
  {
    const float a = up(0.25f), dy = 14.0f * (1.0f - a);
    const char* step = loaded >= n                              ? "PRÊT"
                       : loaded <= 1                            ? "SHADERS"
                       : art::preloadKind(loaded - 1) == 0      ? "COMBATTANTS"
                       : art::preloadKind(loaded - 1) == 1      ? "EFFETS"
                                                                : "ARÈNES ET MENUS";
    const float base = 592 + dy;  // the line's baseline: the 40 px percentage's
    ui::caps(step, 56, base - 3.77f, 11, 0.24f, withAlpha(ui::kBlue, a));
    const char* count = TextFormat("%d / %d", loaded, n);
    ui::exo(count, 56 + ui::capsWidth(step, 11, 0.24f) + 2.64f + 12, base, 16, withAlpha(ui::kMist, a));
    const char* pct = TextFormat("%d", int(p * 100.0f + 0.5f));
    const float sw = ui::exoWidth(" %", 22);
    ui::exo(" %", 660 - sw, base, 22, withAlpha(ui::kSilver, a));
    ui::exo(pct, 660 - sw - ui::exoWidth(pct, 40), base, 40, withAlpha(ui::kSilver, a));
    const float by = 604 + dy, fx = 56 + 604 * p;
    DrawRectangleRec({56, by, 604, 3}, withAlpha(kRim, 0.14f * a));
    DrawRectangleRec({56, by, fx - 56, 3}, withAlpha(ui::kBlue, a));
    const float k = 8.5f * ui::kLean;
    ui::quad({fx - 2 + k, by - 7}, {fx + 2 + k, by - 7}, {fx + 2 - k, by + 10}, {fx - 2 - k, by + 10},
             withAlpha(ui::kSilver, a), withAlpha(ui::kSilver, a), withAlpha(ui::kSilver, a), withAlpha(ui::kSilver, a));
  }
  {
    const float a = up(0.35f);
    drawText("Une seule fois au lancement : ensuite, plus rien ne se charge pendant les combats.", 56,
             640 + 14.0f * (1.0f - a), 13, withAlpha(ui::kMist, a), kB);
  }
  if (out > 0.0f) DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kNight, out));
}
