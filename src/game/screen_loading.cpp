// The launch screen (« Prestige »), as big games prepare their shaders before the title: every fighter's clips, every
// effect and every picture is loaded and every shader compiled while a bar fills, so that nothing is decoded or
// compiled in the middle of a fight or a transition (each effect used to be decoded the first time it played).
#include <cstdio>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/pixel.hpp"
#include "game/render.hpp"
#include "game/rtx.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kB = FontKind::Body;
constexpr double kBudget = 1.0 / 90.0;  // seconds of loading per frame: the bar keeps moving
constexpr int kOutroFrames = 30;         // the full bar holds, then the title
}  // namespace

void App::updateLoading() {
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
  const int n = art::preloadCount() + 1;
  const float done = float(std::min(loadNext_, n)) / float(n);
  const float out = loadDoneAt_ < 0 ? 0.0f : clampf((frames_ - loadDoneAt_ - 12) / float(kOutroFrames - 12), 0.0f, 1.0f);
  const float a = ui::seg(sec, 0.0f, 0.5f) * (1.0f - out);
  ClearBackground(ui::kNight);

  // The blade, far right, and the logo
  ui::lean(1180, 1184, withAlpha(ui::kSilver, 0.16f * a));
  ui::LogoIn in;
  in.word = a;
  in.lines = a;
  in.cut = ui::seg(sec, 0.0f, 0.6f);
  ui::prestigeLogo(640 - 600 * 0.42f, 150, 0.42f, t, in);
  if (out > 0.0f) DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kNight, out));

  // The bar: a thin track, the cyan fill and its bright head
  const float x0 = 340, x1 = 940, y = 470;
  const char* what = loadNext_ <= 1                          ? "SHADERS"
                     : loadNext_ > n - 1                     ? "PRÊT"
                     : art::preloadKind(loadNext_ - 1) == 0  ? "COMBATTANTS"
                     : art::preloadKind(loadNext_ - 1) == 1  ? "EFFETS"
                                                             : "ARÈNES ET MENUS";
  ui::caps("CHARGEMENT", x0, y - 26, 12, 0.3f, withAlpha(ui::kSilver, a));
  ui::caps(what, x0 + ui::capsWidth("CHARGEMENT", 12, 0.3f) + 18, y - 26, 11, 0.22f, withAlpha(ui::kBlue, a));
  const char* pct = TextFormat("%d %%", int(done * 100.0f + 0.5f));
  ui::exo(pct, x1 - ui::exoWidth(pct, 26), y - 14, 26, withAlpha(ui::kSilver, a));
  DrawRectangleRec({x0, y, x1 - x0, 3}, withAlpha(ui::kSilver, 0.12f * a));
  const float fx = x0 + (x1 - x0) * done;
  DrawRectangleRec({x0, y, fx - x0, 3}, withAlpha(ui::kBlue, a));
  DrawRectangleRec({fx - 2, y - 3, 4, 9}, withAlpha(ui::kSilver, a));
  const char* note = "Une seule fois au lancement : ensuite, plus rien ne se charge pendant les combats.";
  drawText(note, 640 - textWidth(note, 13, kB) * 0.5f, y + 22, 13, withAlpha(ui::kMist, a), kB);
}
