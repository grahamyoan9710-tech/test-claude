#pragma once
#include "raylib.h"

enum class Sfx : int {
  HitLight, HitMedium, HitHeavy, HitFire, HitElectric, HitSlash,
  Swing, Jump, AirJump, Land, ShieldHit, ShieldBreak, KO, Crowd, Projectile,
  Grab, Counter, Ledge, MenuMove, MenuSelect, MenuBack, Beep, Go, GameSet,
  UltReady, UltUnleash,  // ultimate meter full / SUPER CLASH
  Count
};

// Every sound is synthesized at start-up: the game ships without asset files.
class Audio {
 public:
  bool init();
  void shutdown();
  void play(Sfx s, float volume = 1.0f, float pitch = 1.0f);
  // 0 = silence, 1 = menu theme, 2 = battle theme
  void setMusic(int mode);
  void toggleMute();
  // Volumes in percent (0..100), applied immediately.
  void setVolumes(int master, int music, int sfx);
  bool muted() const { return muted_; }
  bool ready() const { return ready_; }
  // Writes every synthesized sound + 20 s of each music theme as .wav files (no audio device needed).
  static int exportAll(const char* dir);

 private:
  static constexpr int kVoices = 4;
  bool ready_ = false;
  bool muted_ = false;
  float sfxGain_ = 1.0f;
  Sound voices_[int(Sfx::Count)][kVoices] = {};
  int next_[int(Sfx::Count)] = {};
  AudioStream music_ = {};
  bool musicLoaded_ = false;
};
