#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "core/ai.hpp"
#include "game/audio.hpp"
#include "game/controls.hpp"
#include "game/fx.hpp"
#include "game/ui.hpp"

struct Options {
  bool demo = false;             // start straight into a CPU match
  std::vector<int> fighters;     // --fighters: the demo's line-up (roster indices)
  bool training = false;         // with --screen css: training character select
  int stage = -1;
  int tab = 0;                   // with --screen settings: tab to open
  std::string screen;            // title | menu | css | stage | results | settings | poses | anim
  std::vector<std::pair<int, std::string>> shots;  // frame -> screenshot path
  int recordFrom = -1, recordTo = -1, recordEvery = 1;  // --record: a run of frames saved as DIR/f_NNNNN.png
  std::string recordDir;
  int quitAfter = -1;
  int pauseAt = -1;              // open the pause menu at this frame (screenshots)
  int padGlyphs = -1;            // --pad-glyphs: gamepad prompts of this PadFamily, -1 = automatic
  bool debug = false;
  bool teams = false;  // --teams: team battle (red against blue) for this run
  bool fullRes = false;          // --fullres: screenshots at the resolution of the frame (no downscale)
  int ultShow = -2;              // --ult SLOT: the demo's first fighter unleashes its ultimate (-1 bare-handed, 0/1 weapon)
  char skillShow = 0;            // --skill n|s|u|d: the demo's first fighter uses that special again and again
  int weaponShow = -1;           // --weapon SLOT: the demo's first fighter keeps that weapon in hand (its kit)
};

// What the menus say of a fighter, from its fiche (screen_gallery.cpp).
struct Fiche {
  std::string nick;   // the fiche's title: "OMBRE VIOLETTE"
  std::string art;    // its martial art, bare-handed
  std::string style;  // its style in a word: "Polyvalent"
  int build = 3;       // its build, 1 (small) to 5 (huge): « gabarit léger » = 2
};
Fiche fiche(const CharacterDef& d);
const char* gameVersion();  // "v11.2"...: the title screen's version, also in the settings footer

class MatchScreen {
 public:
  enum class Result { Running, Finished, Quit, Options };  // Options: the pause menu opens the settings
  // The pause menu's lines (match_pause.cpp)
  enum class PauseItem { Resume, Moves, Reset, Dummy, InfiniteUlt, ZeroPercent, Fight, Boxes, Weapon, Options, Quit };
  // attract: the title screen's demo fight (CPUs only): silent with an Audio never initialised, never paused, filmed
  // by a fixed camera on the title's terrace, over at its first K.O.
  MatchScreen(const MatchConfig& cfg, const std::array<int, 4>& devices, Audio& audio, bool debug, bool attract = false);
  Result update();
  void draw();
  void drawAttract();  // the title's backdrop, the fighters and their effects: no HUD; they fade in and out
  const Match& match() const { return m_; }
  bool training() const { return m_.cfg.training; }
  void forcePause() { openPause(-1); }
  // Showcase (screenshots): fighter 0 unleashes the ultimate of weapon `slot` (-1: bare-handed) on fighter 1.
  void showUltimate(int slot);
  void showWeapon(int slot) { weaponShow_ = slot; }
  void showSkill(char dir);
  // For the results: Clash Orbs broken and biggest blow (in %) of each fighter, the fight's length since "GO !"
  int orbsBroken(int id) const { return id >= 0 && id < 4 ? orbs_[id] : 0; }
  float bestHit(int id) const { return id >= 0 && id < 4 ? bestHit_[id] : 0.0f; }
  int fightFrames() const { return goFrame_ < 0 ? 0 : m_.frame - goFrame_; }

 private:
  void updateCamera(bool snap);
  void stepFight();  // one frame of the fight: inputs, simulation, effects
  Camera2D camera();
  // The pause (match_pause.cpp): opened by `port` (-1: the game); its menu, and the move list of whoever paused.
  void openPause(int port);
  Result updatePause();
  void drawPause();
  void drawMoveList(const Fighter& f);
  std::vector<std::pair<PauseItem, std::string>> pauseItems() const;
  const Fighter& listFighter() const;  // whose moves the list shows
  int keysChoice(int k) const;         // the "TOUCHES" switch: 0 that player's device, 1 the other kind
  static const char* dummyName(const CpuBrain& b, int level);
  void resume();
  void trackCombos();
  void trackCallouts();
  void drawTrainingPanel();
  void drawCallouts();  // world space: the damage popping over the fighters, the Clash Orb's spark

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
  int pausedBy_ = -1;     // the port that paused
  int listTab_ = 0;       // the move list's tab: 0 bare hands, 1 weapon A, 2 weapon B
  int keysDevice_ = -1;   // whose keys the move list shows
  bool quitAsk_ = false;  // QUITTER pressed once: once more quits
  int pauseAnim_ = 0;     // frames since the pause opened (the menu slides in)
  int dropSlot_ = 0;  // training: which of the player's two weapons the pause menu drops
  std::array<bool, 4> padLost_{};      // the player's gamepad got unplugged (auto pause)
  std::array<InputState, 4> held_{};   // buttons held when the game resumed: ignored until released
  int endTimer_ = 0;
  float finishZoom_ = 1.0f;  // the camera's zoom as the fight ended (GAME pushes in from it)
  int cutT_ = -1;            // frames since an ultimate's cut-in began (-1: none)
  int shown_ = 0;            // frames this fight has been drawn
  bool debug_ = false;
  bool attract_ = false;
  int ultShow_ = -2;
  int weaponShow_ = -1;
  char skillShow_ = 0;
  bool ultFired_ = false;
  float time_ = 0.0f;
  // Training: combo counter per fighter
  std::vector<float> prevPercent_;
  std::vector<int> combo_;
  std::vector<float> comboStart_;
  std::vector<int> comboShow_;
  // Over the fighters: the damage each takes (summed while hits keep coming); the Clash Orb's spark flying to its winner
  std::vector<float> popValue_, seenPercent_;
  std::vector<int> popAge_;
  Vec2 starFrom_;
  int starTo_ = -1, starT_ = 0;
  std::array<int, 4> orbs_{};
  std::array<float, 4> bestHit_{};
  int goFrame_ = -1;
  int lastCombo_ = 0;
  float lastComboDamage_ = 0.0f;
  int bestCombo_ = 0;
};

enum class Screen { Title, MainMenu, CharSelect, StageSelect, Versus, Match, Results, Settings, Poses, Anim, Gallery };

struct SlotSetup {
  int type = 0;  // 0 = off, 1 = human, 2 = cpu
  int device = kKeyboard1;
  int character = 0;
  int level = 5;
  bool random = false;  // a fighter drawn at random when the match starts
  int team = -1;        // team battles: 0 red, 1 blue (-1: cards 1-2 red, 3-4 blue)
};

// A cursor on the character select, one per input device: each player moves their own.
struct CssCursor {
  bool on = false;
  int col = 0, row = 1;  // card and field (-1: the « Règles & mode » button, 0 its player chip, 1 its fighter)
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
  void updateTitle(const MenuNav& nav);  // « Appuie sur ENTRÉE », the demo fight behind
  void drawTitle();
  void updateMainMenu(const MenuNav& nav);
  void drawMainMenu();
  void activateMenu(int item, int device = -1);
  void claimP1(int device);
  int featuredStage() const;  // the main menu's arena of the moment
  // Gives `device` to card `col` (made human); a card that played it gets the old device or a free one.
  void assignDevice(int col, int device);
  int freeDevice(int col) const;
  // screens (menus.cpp)
  void updateCss(const MenuNav& nav);
  void drawCss();
  void updateStage(const MenuNav& nav);
  void drawStage();
  // the VS screen, between the arena and the fight (screen_versus.cpp)
  void updateVersus(const MenuNav& nav);
  void drawVersus();
  // by: the device that pressed (-1: mouse / unknown).
  void changeField(int col, int row, int delta, int by = -1);
  bool padJoins();
  int activeSlots() const;
  bool readyToFight() const;  // enough fighters (and, in a team battle, two teams)
  int slotTeam(int i) const { return slots_[i].team >= 0 ? (slots_[i].team & 1) : i / 2; }
  int ownedCard(int device) const;          // the card this device plays (-1: none)
  void layoutCss() const;                   // where each player stands on the character select
  bool teamBattle() const;                  // a team battle (the rule, or --teams)
  bool canPick(int col, int device) const;  // may this device change the fighter of that card
  void stepFighter(int col, int delta);     // next / previous fighter of a card ("?" = random included)
  // screens (screen_results.cpp)
  void updateResults(const MenuNav& nav);
  void drawResults();
  // the fighters' gallery (screen_gallery.cpp)
  void updateGallery(const MenuNav& nav);
  void drawGallery();
  // screens (screen_settings.cpp)
  void updateSettings(const MenuNav& nav);
  void drawSettings();
  void drawPoses();      // development sheet (--screen poses)
  void drawAnimSheet();  // development filmstrips (--screen anim)
  int settingsRowCount() const;
  void changeSetting(int row, int delta);

  Options opt_;
  Screen screen_ = Screen::MainMenu;
  Screen next_ = Screen::MainMenu;
  int transition_ = -1;  // frames into a screen transition, -1 = none
  ui::Wipe wipe_ = ui::Wipe::Horizon;
  Audio audio_;
  RenderTexture2D target_{};
  RenderTexture2D snap_{};  // the last screen, frozen, for the « Coupe » transition
  float targetScale_ = 0.0f;
  float time_ = 0.0f;
  int frames_ = 0;
  int screenFrames_ = 0;
  bool quit_ = false;
  std::string toast_;
  int toastTimer_ = 0;
  float slowTime_ = 0.0f;       // seconds spent below 45 FPS (automatic quality fallback)
  bool autoQualityDone_ = false;

  // main menu
  int menuSel_ = 0;
  float markY_ = -1.0f;  // the main menu's choice when menuChanged_ was set
  int menuChanged_ = 0;  // screenFrames_ when the main menu's choice last changed
  bool introDone_ = false;
  int gallerySel_ = 0;
  bool stageBrowse_ = false;  // the arenas opened from the main menu: ENTRÉE trains there, ÉCHAP goes back
  // fight setup
  bool training_ = false;
  std::array<SlotSetup, 4> slots_;
  int stage_ = 0;
  int cssCol_ = 0, cssRow_ = 1;  // the card and field under the mouse
  bool rulesOpen_ = false;  // the « Règles & mode » sheet is open over the character select
  int rulesAnim_ = 0;       // frames since it opened (its slide in)
  int rulesSel_ = 0;        // its field under the keyboard / gamepad
  std::array<CssCursor, kDeviceCount> cssCur_{};
  bool stageRandom_ = false;  // the stage select's "?" card: an arena drawn at random
  int vsTip_ = 0;             // the VS screen's tip (drawn when it opens)
  bool teamsForced_ = false;  // --teams
  std::array<int, 4> joinedAt_{-1000, -1000, -1000, -1000};  // frame a gamepad joined each card (flash)
  std::array<int, 4> pickSeen_{-3, -3, -3, -3}, pickAt_{};  // each card's fighter as last drawn, the frame it changed
  std::array<bool, 4> padLeft_{};  // pads that left their card on this visit of the CSS: no auto re-join
  std::unique_ptr<MatchScreen> match_;
  // settings
  int setTab_ = 0;
  int setRow_ = -1;   // -1 = tab bar
  int setCol_ = 0;    // controls tab: keyboard 1, keyboard 2, pad button 1, pad button 2
  bool waitingKey_ = false;
  bool waitingPad_ = false;  // rebinding a gamepad button (Commandes tab)
  int soundTestAt_ = -1000;  // frame of the last sound test (the audio card's meters move)
  bool bindPadButton(int action, int slot, int button);
  int dragSlider_ = -1;
  Screen settingsReturn_ = Screen::MainMenu;

  struct ResultRow {
    int charIndex, palette, port;
    bool cpu;
    int kos, falls;
    float damage;
    int team = -1;  // team battles: the fighter's team
    int orbs = 0;           // Clash Orbs broken
    float bestHit = 0.0f;   // its biggest blow, in %
    int stocks = 0, weapon = -1, level = 0;  // lives left, weapon in hand at the end, CPU level
  };
  std::vector<ResultRow> results_;
  std::string resultStage_;                  // the fight's arena, lives and length
  int resultStocks_ = 3, resultSeconds_ = 0;
  int resSel_ = 0;                  // the results' buttons: rematch, fighters, arena, main menu
  std::array<bool, 4> resReady_{};  // players ready for the rematch (by port)
  int resGo_ = -1;                  // frames left before the rematch once everyone is ready (-1: waiting)
  int winnerChar_ = -1, winnerPalette_ = 0, winnerPort_ = 0;
  bool winnerCpu_ = false;
  int winnerTeam_ = -1;  // team battles: the team that won
};
