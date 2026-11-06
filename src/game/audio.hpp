#pragma once
#include <string>

#include "raylib.h"

enum class Sfx : int {
  HitLight, HitMedium, HitHeavy, HitFire, HitElectric, HitSlash,
  Swing, Jump, AirJump, Land, ShieldHit, ShieldBreak, KO, Crowd, Projectile,
  Grab, Counter, Ledge, MenuMove, MenuSelect, MenuBack, Beep, Go, GameSet,
  UltReady, UltUnleash,  // ultimate meter full / SUPER CLASH
  // Elements and big effects
  WindGust, IceCrack, PetalSwish, HitHoly, HitGloom,
  BeamHum, BeamBlast, Explosion, ImpactBoom, CutIn, DomainDrone,
  WeaponPickup, WeaponBreak, Reflect,
  Count
};

// Sounds recorded in assets/audio (sfx_<name>.mp3, embedded at build time) replace the synthesized ones; the
// others are synthesized at start-up. The announcer's lines are voice_<key>.mp3, the themes music_<key>.mp3.
// The announcer's key for a fighter's name: lower case letters ("N° 01" -> "n_01").
std::string voiceKey(const std::string& name);

class Audio {
 public:
  bool init();
  void shutdown();
  void play(Sfx s, float volume = 1.0f, float pitch = 1.0f);
  // The announcer: "three", "go", "game", "winner", a fighter's name ("n_01")... A new line cuts the last one.
  void voice(const char* key, float volume = 1.0f);
  // 0 = silence, 1 = menu theme, 2 = battle theme; `world`: the theme of a series (music_<world>.mp3, e.g.
  // "battle"), the synthesized battle theme when there is none.
  void setMusic(int mode, const char* world = nullptr);
  void update();  // feeds the recorded theme (call once a frame)
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
  Sound lines_[64] = {};
  char lineKeys_[64][24] = {};
  int lineCount_ = 0;
  Sound* lastLine_ = nullptr;
  Music track_ = {};
  bool trackOn_ = false;
  char trackKey_[24] = {};
  float trackGain_ = 0.6f;
};
