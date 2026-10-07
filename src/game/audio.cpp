#include "game/audio.hpp"

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <vector>

namespace {
constexpr int kRate = 22050;
constexpr float kTau = 6.2831853f;

struct Noise {
  unsigned s = 22222u;
  float operator()() {
    s = s * 1664525u + 1013904223u;
    return float((s >> 9) & 0x7FFF) / 16384.0f - 1.0f;
  }
};

Wave synth(float seconds, const std::function<float(float)>& fn) {
  int n = int(seconds * kRate);
  short* data = (short*)MemAlloc(n * sizeof(short));
  for (int i = 0; i < n; ++i) {
    float t = float(i) / kRate;
    float fade = std::min(1.0f, (seconds - t) * 80.0f);  // avoid clicks at the end
    float v = std::fmax(-1.0f, std::fmin(1.0f, fn(t) * fade));
    data[i] = short(v * 32000.0f);
  }
  Wave w;
  w.frameCount = (unsigned)n;
  w.sampleRate = kRate;
  w.sampleSize = 16;
  w.channels = 1;
  w.data = data;
  return w;
}

float sweep(float t, float f0, float f1, float dur) {  // phase of an exponential sweep
  float k = std::log(f1 / f0) / dur;
  return kTau * f0 * (std::exp(k * t) - 1.0f) / k;
}
float square(float phase) { return std::sin(phase) >= 0 ? 1.0f : -1.0f; }

Wave makeHit(float power, float pitch) {
  Noise nz;
  float lp = 0;
  return synth(0.12f + 0.35f * power, [=](float t) mutable {
    lp += (nz() - lp) * (0.5f - 0.3f * power);
    float body = std::sin(sweep(t, 190.0f * pitch, 45.0f, 0.4f)) * std::exp(-t * (14.0f - 6.0f * power));
    float crack = lp * std::exp(-t * (40.0f - 20.0f * power));
    return 0.7f * body + 0.6f * crack;
  });
}

Wave makeSfx(Sfx s) {
  Noise nz;
  float lp = 0, lp2 = 0;
  switch (s) {
    case Sfx::HitLight: return makeHit(0.1f, 1.3f);
    case Sfx::HitMedium: return makeHit(0.5f, 1.0f);
    case Sfx::HitHeavy: return makeHit(1.0f, 0.75f);
    case Sfx::HitFire:
      return synth(0.4f, [=](float t) mutable {
        float crackle = (nz() > 0.93f ? 1.0f : 0.0f) * std::exp(-t * 6.0f);
        lp += (nz() - lp) * 0.3f;
        return 0.7f * lp * std::exp(-t * 9.0f) + 0.6f * crackle +
               0.6f * std::sin(sweep(t, 150, 50, 0.4f)) * std::exp(-t * 10.0f);
      });
    case Sfx::HitElectric:
      return synth(0.35f, [=](float t) mutable {
        float buzz = square(kTau * 70.0f * t) * 0.4f + nz() * 0.5f * (std::sin(kTau * 30 * t) > 0 ? 1.0f : 0.2f);
        return buzz * std::exp(-t * 8.0f);
      });
    case Sfx::HitSlash:
      return synth(0.3f, [=](float t) mutable {
        float n = nz();
        float hp = n - lp;
        lp = n;
        return 0.6f * hp * std::exp(-t * 18.0f) + 0.35f * std::sin(kTau * 2300.0f * t) * std::exp(-t * 12.0f) +
               0.5f * std::sin(sweep(t, 160, 60, 0.3f)) * std::exp(-t * 16.0f);
      });
    case Sfx::Swing:
      return synth(0.2f, [=](float t) mutable {
        float k = 0.05f + 0.25f * std::sin(t / 0.2f * 3.14159f);
        lp += (nz() - lp) * k;
        return lp * 1.2f * std::sin(t / 0.2f * 3.14159f);
      });
    case Sfx::Jump:
      return synth(0.13f, [](float t) { return 0.3f * square(sweep(t, 260, 620, 0.13f)) * (1.0f - t / 0.13f); });
    case Sfx::AirJump:
      return synth(0.15f, [](float t) { return 0.3f * square(sweep(t, 420, 1100, 0.15f)) * (1.0f - t / 0.15f); });
    case Sfx::Land:
      return synth(0.14f, [=](float t) mutable {
        lp += (nz() - lp) * 0.15f;
        return (0.8f * std::sin(sweep(t, 110, 40, 0.14f)) + 0.6f * lp) * std::exp(-t * 25.0f);
      });
    case Sfx::ShieldHit:
      return synth(0.25f, [](float t) {
        return (0.4f * std::sin(kTau * 1300 * t) + 0.3f * std::sin(kTau * 1950 * t)) * std::exp(-t * 14.0f);
      });
    case Sfx::ShieldBreak:
      return synth(0.8f, [=](float t) mutable {
        float v = 0;
        for (int k = 0; k < 6; ++k) v += std::sin(kTau * (1500.0f + k * 437.0f) * t) * std::exp(-t * (4.0f + k));
        return 0.15f * v + 0.4f * nz() * std::exp(-t * 7.0f);
      });
    case Sfx::KO:
      return synth(1.4f, [=](float t) mutable {
        lp += (nz() - lp) * 0.08f;
        lp2 += (nz() - lp2) * 0.5f;
        return 1.6f * lp * std::exp(-t * 2.5f) + 0.5f * lp2 * std::exp(-t * 12.0f) +
               0.8f * std::sin(sweep(t, 90, 25, 1.4f)) * std::exp(-t * 3.0f);
      });
    case Sfx::Crowd:
      return synth(2.2f, [=](float t) mutable {
        lp += (nz() - lp) * 0.22f;
        lp2 += (lp - lp2) * 0.3f;
        float env = std::min(1.0f, t / 0.35f) * std::exp(-std::max(0.0f, t - 0.6f) * 1.6f);
        float wobble = 0.75f + 0.25f * std::sin(kTau * 5.5f * t + std::sin(kTau * 1.3f * t) * 3.0f);
        return 1.5f * (lp - lp2) * env * wobble;
      });
    case Sfx::Projectile:
      return synth(0.22f, [](float t) {
        return 0.35f * std::sin(sweep(t, 1100, 250, 0.22f) + 3.0f * std::sin(kTau * 40 * t)) * (1.0f - t / 0.22f);
      });
    case Sfx::Grab:
      return synth(0.1f, [](float t) { return 0.7f * std::sin(sweep(t, 220, 110, 0.1f)) * std::exp(-t * 30.0f); });
    case Sfx::Counter:
      return synth(0.45f, [](float t) {
        return (0.4f * std::sin(kTau * 1800 * t) + 0.3f * std::sin(kTau * 2710 * t)) * std::exp(-t * 7.0f);
      });
    case Sfx::Ledge:
      return synth(0.05f, [=](float t) mutable { return 0.5f * nz() * std::exp(-t * 90.0f); });
    case Sfx::MenuMove:
      return synth(0.05f, [](float t) { return 0.2f * square(kTau * 880 * t) * (1.0f - t / 0.05f); });
    case Sfx::MenuSelect:
      return synth(0.14f, [](float t) { return 0.25f * square(kTau * (t < 0.06f ? 660.0f : 990.0f) * t); });
    case Sfx::MenuBack:
      return synth(0.14f, [](float t) { return 0.25f * square(kTau * (t < 0.06f ? 440.0f : 300.0f) * t); });
    case Sfx::Beep:
      return synth(0.2f, [](float t) { return 0.4f * std::sin(kTau * 660 * t) * std::exp(-t * 6.0f); });
    case Sfx::Go:
      return synth(0.5f, [](float t) {
        return (0.4f * std::sin(kTau * 990 * t) + 0.2f * std::sin(kTau * 1980 * t)) * std::exp(-t * 4.0f);
      });
    case Sfx::GameSet:
      return synth(1.0f, [](float t) {
        const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
        int i = std::min(3, int(t / 0.12f));
        float v = 0;
        for (int k = 0; k <= i; ++k) v += std::sin(kTau * notes[k] * t);
        return 0.18f * v * std::exp(-std::max(0.0f, t - 0.45f) * 4.0f);
      });
    case Sfx::Count: break;
  }
  return synth(0.01f, [](float) { return 0.0f; });
}

// ----------------------------------------------------------------------------
// Procedural chiptune (runs on the audio thread)
// ----------------------------------------------------------------------------
std::atomic<int> g_musicMode{0};
std::atomic<bool> g_musicMuted{false};
std::atomic<float> g_musicGain{0.6f};

float noteFreq(int midi) { return 440.0f * std::pow(2.0f, (midi - 69) / 12.0f); }

void musicCallback(void* buffer, unsigned int frames) {
  static double t = 0.0;
  static float vol = 0.0f;
  static Noise nz;
  short* out = (short*)buffer;
  const int mode = g_musicMode.load();
  const float bpm = mode == 2 ? 150.0f : 104.0f;
  const double step = 60.0 / bpm / 4.0;  // 16th note
  // A minor progression: Am F C G   (battle)  /  Am F Dm E (menu)
  static const int prog[2][4][3] = {{{57, 60, 64}, {53, 57, 60}, {48, 52, 55}, {55, 59, 62}},
                                    {{57, 60, 64}, {53, 57, 60}, {50, 53, 57}, {52, 56, 59}}};
  static const int lead[32] = {76, -1, 74, 76, -1, 72, -1, 69, 72, -1, 74, -1, 76, -1, 79, -1,
                               77, -1, 76, 74, -1, 72, -1, 74, 71, -1, 72, -1, 74, -1, -1, -1};
  const int (*P)[3] = prog[mode == 2 ? 0 : 1];
  for (unsigned i = 0; i < frames; ++i) {
    float target = (mode == 0 || g_musicMuted.load()) ? 0.0f : 1.0f;
    vol += (target - vol) * 0.0005f;
    long s16 = long(t / step);
    double inStep = t - s16 * step;
    int bar = int((s16 / 16) % 4);
    int st = int(s16 % 16);
    float v = 0.0f;
    if (vol > 0.001f) {
      // bass
      static const bool bassHit[16] = {1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0};
      int lastHit = st;
      while (!bassHit[lastHit]) --lastHit;
      double since = inStep + (st - lastHit) * step;
      float bf = noteFreq(P[bar][0] - 24);
      float tri = 2.0f * std::fabs(2.0f * float(std::fmod(t * bf, 1.0)) - 1.0f) - 1.0f;
      v += 0.32f * tri * float(std::exp(-since * 5.0));
      // arpeggio
      int tone = P[bar][(st % 4 == 3) ? 1 : st % 3] + 12;
      float af = noteFreq(tone);
      float duty = float(std::fmod(t * af, 1.0)) < 0.25f ? 1.0f : -1.0f;
      v += (mode == 2 ? 0.08f : 0.06f) * duty * float(std::exp(-inStep * 18.0));
      if (mode == 2) {
        // lead melody on the second half of the loop
        int li = int((s16 / 2) % 64);
        if (li >= 32) {
          int n = lead[li - 32];
          int k = li - 32;
          while (n < 0 && k > 0) n = lead[--k];
          double since2 = t - double((s16 / 2) * 2 - (li - 32 - k) * 2) * step;
          if (n > 0) {
            float lf = noteFreq(n);
            float sq = float(std::fmod(t * lf, 1.0)) < 0.5f ? 1.0f : -1.0f;
            v += 0.07f * sq * float(std::exp(-since2 * 2.5));
          }
        }
        // drums
        if (st % 8 == 0) v += 0.45f * float(std::sin(kTau * 55.0 * inStep * (1.0 + 2.0 * std::exp(-inStep * 30.0)))) * float(std::exp(-inStep * 14.0));
        if (st % 8 == 4) v += 0.22f * nz() * float(std::exp(-inStep * 20.0));
        if (st % 2 == 0) v += 0.05f * nz() * float(std::exp(-inStep * 80.0));
      }
    }
    out[i] = short(std::fmax(-1.0f, std::fmin(1.0f, v * vol * 1.3f * g_musicGain.load())) * 30000.0f);
    t += 1.0 / kRate;
  }
}
}  // namespace

bool Audio::init() {
  InitAudioDevice();
  if (!IsAudioDeviceReady()) return false;
  for (int i = 0; i < int(Sfx::Count); ++i) {
    Wave w = makeSfx(Sfx(i));
    voices_[i][0] = LoadSoundFromWave(w);
    for (int v = 1; v < kVoices; ++v) voices_[i][v] = LoadSoundAlias(voices_[i][0]);
    UnloadWave(w);
  }
  SetAudioStreamBufferSizeDefault(2048);
  music_ = LoadAudioStream(kRate, 16, 1);
  SetAudioStreamCallback(music_, musicCallback);
  PlayAudioStream(music_);
  musicLoaded_ = true;
  ready_ = true;
  return true;
}

void Audio::shutdown() {
  if (!ready_) return;
  if (musicLoaded_) UnloadAudioStream(music_);
  for (int i = 0; i < int(Sfx::Count); ++i) {
    for (int v = 1; v < kVoices; ++v) UnloadSoundAlias(voices_[i][v]);
    UnloadSound(voices_[i][0]);
  }
  CloseAudioDevice();
  ready_ = false;
}

void Audio::play(Sfx s, float volume, float pitch) {
  if (!ready_ || muted_) return;
  int i = int(s);
  Sound& snd = voices_[i][next_[i]];
  next_[i] = (next_[i] + 1) % kVoices;
  SetSoundVolume(snd, std::min(1.0f, volume * sfxGain_));
  SetSoundPitch(snd, pitch);
  PlaySound(snd);
}

void Audio::setMusic(int mode) { g_musicMode.store(mode); }

int Audio::exportAll(const char* dir) {
  static const char* names[] = {"hit_light", "hit_medium", "hit_heavy", "hit_fire", "hit_electric", "hit_slash",
                                "swing", "jump", "air_jump", "land", "shield_hit", "shield_break", "ko", "crowd",
                                "projectile", "grab", "counter", "ledge", "menu_move", "menu_select", "menu_back",
                                "beep", "go", "game_set"};
  static_assert(sizeof(names) / sizeof(names[0]) == int(Sfx::Count), "one name per sound");
  int written = 0;
  for (int i = 0; i < int(Sfx::Count); ++i) {
    Wave w = makeSfx(Sfx(i));
    written += ExportWave(w, TextFormat("%s/%s.wav", dir, names[i])) ? 1 : 0;
    UnloadWave(w);
  }
  for (int mode = 1; mode <= 2; ++mode) {
    g_musicMode.store(mode);
    const int frames = kRate * 20;
    short* data = (short*)MemAlloc(frames * sizeof(short));
    for (int off = 0; off < frames; off += 1024) musicCallback(data + off, std::min(1024, frames - off));
    Wave w{(unsigned)frames, kRate, 16, 1, data};
    written += ExportWave(w, TextFormat("%s/music_%s.wav", dir, mode == 1 ? "menu" : "battle")) ? 1 : 0;
    UnloadWave(w);
  }
  g_musicMode.store(0);
  return written;
}

void Audio::setVolumes(int master, int music, int sfx) {
  if (ready_) SetMasterVolume(master / 100.0f);
  g_musicGain.store(music / 100.0f);
  sfxGain_ = sfx / 100.0f;
}

void Audio::toggleMute() {
  muted_ = !muted_;
  g_musicMuted.store(muted_);
}
