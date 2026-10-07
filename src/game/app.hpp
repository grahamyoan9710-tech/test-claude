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
  bool training = false;         // with --screen css: training character select
  int stage = -1;
  int tab = 0;                   // with --screen settings: tab to open
  std::string screen;            // title | menu | css | stage | results | settings
  std::vector<std::pair<int, std::string>> shots;  // frame -> screenshot path
  int quitAfter = -1;
  int pauseAt = -1;              // open the pause menu at this frame (screenshots)
  bool debug = false;
};

class MatchScreen {
 public:
  enum class Result { Running, Finished, Quit };
  MatchScreen(const MatchConfig& cfg, const std::array<int, 4>& devices, Audio& audio, bool debug);
  Result update();
  void draw();
  const Match& match() const { return m_; }
  bool training() const { return m_.cfg.training; }
  void forcePause() { paused_ = true; }

 private:
  void updateCamera(bool snap);
  Camera2D camera();
  Result updatePause();
  void drawPause();
  void trackCombos();
  void drawTrainingPanel();
  std::vector<std::string> pauseItems() const;

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
  // Training: combo counter per fighter
  std::vector<float> prevPercent_;
  std::vector<int> combo_;
  std::vector<float> comboStart_;
  std::vector<int> comboShow_;
  int lastCombo_ = 0;
  float lastComboDamage_ = 0.0f;
  int bestCombo_ = 0;
};

enum class Screen { Title, MainMenu, CharSelect, StageSelect, Match, Results, Settings };

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
  // flow
  void goTo(Screen s, bool instant = false);
  void enterScreen(Screen s);
  void applyDisplaySettings();
  void startMatch();
  void finishMatch();
  void toast(const char* text);
  // screens (screen_title.cpp)
  void updateTitle(const MenuNav& nav);
  void drawTitle();
  void updateMainMenu(const MenuNav& nav);
  void drawMainMenu();
  void activateMenu(int item);
  // screens (menus.cpp)
  void updateCss(const MenuNav& nav);
  void drawCss();
  void updateStage(const MenuNav& nav);
  void drawStage();
  void changeField(int col, int row, int delta);
  int activeSlots() const;
  // screens (screen_results.cpp)
  void updateResults(const MenuNav& nav);
  void drawResults();
  // screens (screen_settings.cpp)
  void updateSettings(const MenuNav& nav);
  void drawSettings();
  int settingsRowCount() const;
  void changeSetting(int row, int delta);

  Options opt_;
  Screen screen_ = Screen::Title;
  Screen next_ = Screen::Title;
  int transition_ = -1;  // frames into a screen transition, -1 = none
  Audio audio_;
  RenderTexture2D target_{};
  float targetScale_ = 0.0f;
  float time_ = 0.0f;
  int frames_ = 0;
  int screenFrames_ = 0;
  bool quit_ = false;
  std::string toast_;
  int toastTimer_ = 0;

  // main menu
  int menuSel_ = 0;
  int menuRight_ = 1;  // last selected tile of the right column
  bool introDone_ = false;
  // fight setup
  bool training_ = false;
  std::array<SlotSetup, 4> slots_;
  int stage_ = 0;
  int cssCol_ = 0, cssRow_ = 1;
  int rulesCol_ = 0;
  std::unique_ptr<MatchScreen> match_;
  // settings
  int setTab_ = 0;
  int setRow_ = -1;   // -1 = tab bar
  int setCol_ = 0;    // keyboard column in the controls tab
  bool waitingKey_ = false;
  int dragSlider_ = -1;
  Screen settingsReturn_ = Screen::MainMenu;

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
