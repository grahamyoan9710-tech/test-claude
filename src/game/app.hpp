#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "core/ai.hpp"
#include "game/audio.hpp"
#include "game/controls.hpp"
#include "game/fx.hpp"

struct Options {
  bool demo = false;             // start straight into a CPU match
  int stage = -1;
  std::string screen;            // title | css | stage | results
  std::vector<std::pair<int, std::string>> shots;  // frame -> screenshot path
  int quitAfter = -1;
  bool debug = false;
};

class MatchScreen {
 public:
  enum class Result { Running, Finished, Quit };
  MatchScreen(const MatchConfig& cfg, const std::array<int, 4>& devices, Audio& audio, bool debug);
  Result update();
  void draw();
  const Match& match() const { return m_; }

 private:
  void updateCamera(bool snap);
  Camera2D camera();

  Match m_;
  std::array<int, 4> devices_;
  std::vector<CpuBrain> brains_;
  Audio& audio_;
  Fx fx_;
  Vec2 camPos_{0, -150};
  float camZoom_ = 0.8f;
  Vec2 shake_;
  bool paused_ = false;
  int pauseSel_ = 0;
  int endTimer_ = 0;
  bool debug_ = false;
  float time_ = 0.0f;
};

enum class Screen { Title, CharSelect, StageSelect, Match, Results };

struct SlotSetup {
  int type = 0;  // 0 = off, 1 = human, 2 = cpu
  int device = kKeyboard1;
  int character = 0;
  int level = 5;
};

class App {
 public:
  explicit App(Options opt);
  void run();

 private:
  void goTo(Screen s);
  void startMatch();
  void finishMatch();
  void updateTitle(const MenuNav& nav);
  void drawTitle();
  void updateCss(const MenuNav& nav);
  void drawCss();
  void updateStage(const MenuNav& nav);
  void drawStage();
  void updateResults(const MenuNav& nav);
  void drawResults();
  void changeField(int col, int row, int delta);
  int activeSlots() const;

  Options opt_;
  Screen screen_ = Screen::Title;
  Audio audio_;
  float time_ = 0.0f;
  int frames_ = 0;
  int screenFrames_ = 0;
  bool quit_ = false;

  std::array<SlotSetup, 4> slots_;
  int stocks_ = 3;
  int stage_ = 0;
  int cssCol_ = 0, cssRow_ = 1;
  int titleSel_ = 0;
  std::unique_ptr<MatchScreen> match_;

  struct ResultRow {
    int charIndex, palette, port;
    bool cpu;
    int kos, falls;
    float damage;
  };
  std::vector<ResultRow> results_;
  int winnerChar_ = -1, winnerPalette_ = 0, winnerPort_ = 0;
  bool winnerCpu_ = false;
};
