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
    case Sfx::UltReady:
      // Ancient mechanism waking up: a low swell, then rising glassy bells over a shimmer.
      return synth(1.5f, [=](float t) mutable {
        const float notes[5] = {587.33f, 880.0f, 1174.66f, 1479.98f, 1760.0f};  // D A D F# A
        float bells = 0.0f;
        for (int k = 0; k < 5; ++k) {
          float u = t - 0.12f - k * 0.075f;
          if (u < 0.0f) break;
          float env = std::exp(-u * 3.0f) * std::min(1.0f, u * 400.0f);
          bells += env * (std::sin(kTau * notes[k] * u) +
                          0.3f * std::sin(kTau * notes[k] * 2.76f * u) * std::exp(-u * 7.0f) +
                          0.15f * std::sin(kTau * notes[k] * 5.4f * u) * std::exp(-u * 14.0f));
        }
        float swell = std::sin(sweep(t, 70.0f, 150.0f, 1.5f)) * std::sin(std::min(1.0f, t / 0.5f) * 1.5708f) *
                      std::exp(-std::max(0.0f, t - 0.35f) * 3.5f);
        float n = nz();
        float hp = n - lp;
        lp = n;
        float shimmer = hp * (0.6f + 0.4f * std::sin(kTau * 9.0f * t)) * std::min(1.0f, t * 3.0f) * std::exp(-t * 2.2f);
        return 0.13f * bells + 0.45f * swell + 0.07f * shimmer;
      });
    case Sfx::UltUnleash:
      // SUPER CLASH: a rising charge (stacked fifths + noise riser), a deep impact, ringing bells.
      return synth(2.1f, [=](float t) mutable {
        float charge = 0.0f;
        if (t < 0.6f) {
          float env = std::min(1.0f, t / 0.15f) * (1.0f - std::max(0.0f, t - 0.5f) / 0.1f);
          float ph = sweep(t, 80.0f, 520.0f, 0.6f);
          charge = env * (std::sin(ph) + 0.5f * std::sin(ph * 1.5f) + 0.3f * std::sin(ph * 2.0f) +
                          0.2f * std::sin(ph * 3.0f)) * (0.7f + 0.3f * std::sin(kTau * (8.0f + 30.0f * t) * t));
        }
        lp += (nz() - lp) * (0.05f + 0.5f * std::min(1.0f, t / 0.6f));
        float riser = t < 0.6f ? lp * (t / 0.6f) : 0.0f;
        float u = t - 0.58f;
        float boom = 0.0f, bells = 0.0f;
        if (u > 0.0f) {
          lp2 += (nz() - lp2) * 0.12f;
          boom = std::sin(sweep(u, 130.0f, 32.0f, 1.0f)) * std::exp(-u * 3.2f) + 1.2f * lp2 * std::exp(-u * 5.0f);
          const float notes[3] = {587.33f, 880.0f, 1318.5f};
          for (int k = 0; k < 3; ++k)
            bells += std::sin(kTau * notes[k] * u) * std::exp(-u * (2.0f + k)) * std::min(1.0f, u * 300.0f);
        }
        return 0.2f * charge + 0.35f * riser + 0.75f * boom + 0.12f * bells;
      });
    case Sfx::WindGust:
      // Whoosh: band-passed noise whose band opens and closes with the gust.
      return synth(0.55f, [=](float t) mutable {
        const float env = std::pow(std::sin(3.14159f * t / 0.55f), 2.0f);
        const float k = 0.03f + 0.22f * env;
        lp += (nz() - lp) * k;
        lp2 += (lp - lp2) * k * 0.5f;
        return 2.6f * (lp - lp2) * env;
      });
    case Sfx::IceCrack:
      // Brittle crack: a sharp click, glassy partials and a few crackles.
      return synth(0.45f, [=](float t) mutable {
        const float click = nz() * std::exp(-t * 160.0f);
        float glass = 0.0f;
        const float fr[4] = {2380.0f, 3710.0f, 5120.0f, 6650.0f};
        for (int k = 0; k < 4; ++k) glass += std::sin(kTau * fr[k] * t) * std::exp(-t * (9.0f + k * 4.0f));
        const float crackle = (nz() > 0.96f ? nz() : 0.0f) * std::exp(-t * 7.0f);
        return 0.9f * click + 0.16f * glass + 0.5f * crackle;
      });
    case Sfx::PetalSwish:
      // Silky swish: high-passed noise sweeping by, with a faint metallic shimmer of a thousand blades.
      return synth(0.42f, [=](float t) mutable {
        const float env = std::sin(3.14159f * std::min(1.0f, t / 0.42f));
        const float n = nz();
        const float hp = n - lp;
        lp += (n - lp) * 0.35f;
        const float shimmer = std::sin(kTau * (3400.0f + 600.0f * std::sin(kTau * 7.0f * t)) * t) * std::exp(-t * 6.0f);
        return (0.7f * hp + 0.12f * shimmer) * env;
      });
    case Sfx::HitHoly:
      // Sacred light: a soft thump and a bright bell chord.
      return synth(0.7f, [=](float t) mutable {
        const float thump = std::sin(sweep(t, 220.0f, 70.0f, 0.2f)) * std::exp(-t * 18.0f);
        const float notes[3] = {1318.5f, 1975.5f, 2637.0f};
        float bell = 0.0f;
        for (int k = 0; k < 3; ++k) bell += std::sin(kTau * notes[k] * t) * std::exp(-t * (5.0f + 2.0f * k));
        lp += (nz() - lp) * 0.6f;
        return 0.6f * thump + 0.18f * bell + 0.25f * lp * std::exp(-t * 30.0f);
      });
    case Sfx::HitGloom:
      // Malice: a low, rough thud with a growling tail.
      return synth(0.6f, [=](float t) mutable {
        lp += (nz() - lp) * 0.07f;
        const float growl = std::sin(sweep(t, 140.0f, 38.0f, 0.6f) + 2.0f * lp) * std::exp(-t * 5.0f);
        lp2 += (nz() - lp2) * 0.4f;
        return 0.8f * growl + 0.5f * lp2 * std::exp(-t * 22.0f);
      });
    case Sfx::BeamHum:
      // Sustained energy hum (detuned saws, a little buzz).
      return synth(1.2f, [=](float t) mutable {
        const float env = std::min(1.0f, t / 0.08f) * std::min(1.0f, (1.2f - t) / 0.3f);
        auto saw = [](float ph) { return 2.0f * (ph - std::floor(ph)) - 1.0f; };
        const float v = saw(82.0f * t) + saw(82.7f * t) + 0.6f * saw(164.3f * t) + 0.3f * saw(246.0f * t);
        lp += (v - lp) * 0.18f;
        return 0.28f * lp * env * (0.85f + 0.15f * std::sin(kTau * 13.0f * t));
      });
    case Sfx::BeamBlast:
      // The beam fires: a roaring burst of noise over a falling tone.
      return synth(1.0f, [=](float t) mutable {
        lp += (nz() - lp) * (0.5f - 0.35f * std::min(1.0f, t / 0.6f));
        const float roar = lp * std::exp(-t * 3.2f) * std::min(1.0f, t / 0.015f);
        const float tone = std::sin(sweep(t, 520.0f, 70.0f, 1.0f)) * std::exp(-t * 4.0f);
        return 1.1f * roar + 0.45f * tone;
      });
    case Sfx::Explosion:
      // Deep boom, crunchy crackle, rumbling tail.
      return synth(1.3f, [=](float t) mutable {
        lp += (nz() - lp) * 0.06f;
        lp2 += (nz() - lp2) * 0.45f;
        const float boom = std::sin(sweep(t, 110.0f, 28.0f, 1.3f)) * std::exp(-t * 3.0f);
        const float crackle = (nz() > 0.9f ? nz() : 0.0f) * std::exp(-t * 6.0f);
        return 0.9f * boom + 2.0f * lp * std::exp(-t * 2.4f) + 0.45f * lp2 * std::exp(-t * 14.0f) + 0.3f * crackle;
      });
    case Sfx::ImpactBoom:
      // Impact frames: a dry crack and a huge sub thump.
      return synth(0.9f, [=](float t) mutable {
        lp2 += (nz() - lp2) * 0.7f;
        const float crack = lp2 * std::exp(-t * 60.0f);
        const float sub = std::sin(sweep(t, 75.0f, 26.0f, 0.9f)) * std::exp(-t * 4.0f) * std::min(1.0f, t / 0.004f);
        lp += (nz() - lp) * 0.05f;
        return 0.9f * crack + 1.1f * sub + 0.8f * lp * std::exp(-t * 5.0f);
      });
    case Sfx::CutIn:
      // Super cut-in: a metallic "shing" over a fast whoosh.
      return synth(0.55f, [=](float t) mutable {
        float shing = 0.0f;
        const float fr[3] = {3150.0f, 4470.0f, 6020.0f};
        for (int k = 0; k < 3; ++k) shing += std::sin(kTau * fr[k] * t) * std::exp(-t * (8.0f + k * 3.0f));
        const float env = std::sin(3.14159f * std::min(1.0f, t / 0.3f));
        lp += (nz() - lp) * (0.1f + 0.3f * env);
        return 0.22f * shing * std::min(1.0f, t / 0.003f) + 1.2f * lp * env * (t < 0.3f ? 1.0f : 0.0f);
      });
    case Sfx::DomainDrone:
      // Domain expansion: an ominous swelling drone (fifths beating slowly) and a breath of noise.
      return synth(2.6f, [=](float t) mutable {
        const float env = std::min(1.0f, t / 0.5f) * std::min(1.0f, (2.6f - t) / 1.0f);
        auto saw = [](float ph) { return 2.0f * (ph - std::floor(ph)) - 1.0f; };
        const float v = saw(55.0f * t) + saw(55.6f * t) + 0.7f * saw(82.4f * t) + 0.4f * saw(110.3f * t);
        lp += (v - lp) * 0.06f;
        lp2 += (nz() - lp2) * 0.02f;
        return env * (0.42f * lp + 1.2f * lp2 * (0.6f + 0.4f * std::sin(kTau * 0.7f * t)));
      });
    case Sfx::WeaponPickup:
      // Bright chime arpeggio (E6 G#6 B6 E7).
      return synth(0.8f, [](float t) {
        const float notes[4] = {1318.5f, 1661.2f, 1975.5f, 2637.0f};
        float v = 0.0f;
        for (int k = 0; k < 4; ++k) {
          const float u = t - k * 0.06f;
          if (u < 0.0f) break;
          v += (std::sin(kTau * notes[k] * u) + 0.25f * std::sin(kTau * notes[k] * 2.0f * u)) * std::exp(-u * 6.0f) *
               std::min(1.0f, u * 500.0f);
        }
        return 0.22f * v;
      });
    case Sfx::WeaponBreak:
      // Metal shattering: inharmonic partials and a burst of noise.
      return synth(0.8f, [=](float t) mutable {
        float v = 0.0f;
        for (int k = 0; k < 6; ++k) v += std::sin(kTau * (820.0f + k * 613.0f) * t) * std::exp(-t * (6.0f + 2.0f * k));
        lp += (nz() - lp) * 0.5f;
        return 0.17f * v + 0.6f * lp * std::exp(-t * 9.0f) + 0.4f * std::sin(sweep(t, 200.0f, 60.0f, 0.3f)) * std::exp(-t * 14.0f);
      });
    case Sfx::Reflect:
      // "Ping": a bright rising tone.
      return synth(0.4f, [](float t) {
        return (0.45f * std::sin(sweep(t, 1500.0f, 2600.0f, 0.12f)) + 0.2f * std::sin(kTau * 3900.0f * t)) *
               std::exp(-t * 9.0f);
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
                                "beep", "go", "game_set", "ult_ready", "ult_unleash",
                                "wind_gust", "ice_crack", "petal_swish", "hit_holy", "hit_gloom",
                                "beam_hum", "beam_blast", "explosion", "impact_boom", "cut_in", "domain_drone",
                                "weapon_pickup", "weapon_break", "reflect"};
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
