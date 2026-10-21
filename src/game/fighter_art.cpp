// The fighters' pictures (see fighter_art.hpp): PixelLab clips, still pictures and effects, all embedded at build
// time from assets/sprites (CMakeLists.txt) with their index sheets.txt.
#include "game/fighter_art.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "rlgl.h"

extern "C" {
struct ClashSheet {
  const char* name;  // "<fighter>_<anim>"
  const unsigned char* data;
  const unsigned int* size;
};
extern const ClashSheet clash_sheets[];  // ends with a null name
extern const unsigned char sheets_txt[];
extern const unsigned int sheets_txt_size;
}

namespace art {
bool embeddedSprite(const char* name, const unsigned char** data, unsigned* size) {
  for (const ClashSheet* cs = clash_sheets; cs->name; ++cs)
    if (std::strcmp(cs->name, name) == 0) {
      *data = cs->data;
      *size = *cs->size;
      return true;
    }
  return false;
}
const std::string& spriteIndex() {
  static const std::string text(reinterpret_cast<const char*>(sheets_txt), sheets_txt_size);
  return text;
}

namespace {
std::string heroKey(Hero h) {  // "p01": the prefix of the fighter's files
  const auto& r = roster();
  for (const auto& c : r)
    if (c.hero == h) return c.id;
  return "";
}

struct Art {
  struct Sheet {  // a generated animation: frames side by side
    bool ok = false, loop = false;
    Texture2D tex{};
    int frames = 0, impact = -1;  // impact: the frame where the (first) blow lands (-1: none)
    int impacts[4] = {-1, -1, -1, -1}, nImpacts = 0;  // every blow's frame, in order (sheets.txt "7,22")
    float fw = 0, fh = 0, ax = 0, ay = 0, pxPerH = 1;  // frame size, the feet (hips' x on the ground), scale
    float fps = 15.0f;                                 // looping clips
    float drop = 0;  // px: the first frame sits this much lower than the anchor says, easing to 0 by the last
  };
  Sheet sheets[kSheetCount];
  bool clips = false, clipsTried = false;  // at least one sheet; sheets loaded
};
std::array<Art, size_t(Hero::Count)> g_art;

Shader g_tint{};
int g_tintLoc = -1;
bool g_tintTried = false;
const char* kTintFrag = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 tint;
out vec4 finalColor;
void main() {
  vec4 t = texture(texture0, fragTexCoord);
  finalColor = vec4(mix(t.rgb, tint.rgb, tint.a), t.a) * fragColor * colDiffuse;
}
)";

// Tint of the fighter: hit flash, afterimage, transformation. False when nothing tints it.
bool tintOf(const Pose& pose, Color flashColor, float flash, Color& tc, float& amount) {
  tc = flashColor;
  amount = clampf(flash, 0.0f, 1.0f);
  if (pose.ghost) amount = std::max(amount, 0.65f);
  Color formCol{};
  float formK = 0.0f;
  switch (pose.form) {  // a transformation lights the fighter in its colour (its aura is drawn around it)
    case Form::Awakened: formCol = pose.formColor, formK = 0.22f; break;
    default: break;
  }
  if (formK > amount) {
    tc = mixColor(formCol, flashColor, formK > 0.0f ? amount / formK : 0.0f);
    amount = formK;
  }
  if (!g_tintTried) {
    g_tintTried = true;
    g_tint = LoadShaderFromMemory(nullptr, kTintFrag);
    g_tintLoc = g_tint.id ? GetShaderLocation(g_tint, "tint") : -1;
  }
  return amount > 0.01f && g_tint.id != 0 && g_tintLoc >= 0;
}
void beginTint(Color tc, float amount) {
  BeginShaderMode(g_tint);
  const float v[4] = {tc.r / 255.0f, tc.g / 255.0f, tc.b / 255.0f, amount};
  SetShaderValue(g_tint, g_tintLoc, v, SHADER_UNIFORM_VEC4);
}

void loadSheets(Art& A, const char* fighter) {
  const std::string text(reinterpret_cast<const char*>(sheets_txt), sheets_txt_size);
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find('\n', pos);
    if (end == std::string::npos) end = text.size();
    const std::string line = text.substr(pos, end - pos);
    pos = end + 1;
    char who[32] = {}, anim[32] = {};
    int frames = 0, loop = 0;
    char impact[32] = "-1";
    float fw = 0, fh = 0, ax = 0, ay = 0, pxh = 0, fps = 15.0f, drop = 0.0f;
    if (std::sscanf(line.c_str(), "sheet %31s %31s %d %f %f %f %f %f %d %31s %f %f", who, anim, &frames, &fw, &fh, &ax,
                    &ay, &pxh, &loop, impact, &fps, &drop) < 9 || std::strcmp(who, fighter) != 0 || frames <= 0 || pxh <= 0.0f)
      continue;
    int id = -1;
    for (int i = 0; i < kSheetCount; ++i)
      if (std::strcmp(anim, sheetName(i)) == 0) id = i;
    if (id < 0) continue;
    const std::string key = std::string(fighter) + "_" + anim;
    for (const ClashSheet* cs = clash_sheets; cs->name; ++cs) {
      if (key != cs->name) continue;
      Image img = LoadImageFromMemory(".png", cs->data, int(*cs->size));
      if (!img.data) break;
      Art::Sheet& S = A.sheets[id];
      S.tex = LoadTextureFromImage(img);
      UnloadImage(img);
      if (S.tex.id == 0) break;
      GenTextureMipmaps(&S.tex);
      SetTextureFilter(S.tex, TEXTURE_FILTER_TRILINEAR);
      // Drawn smaller than its pixels, a clip leans on the sharper mipmap: crisp, not mushy (still no shimmer).
      rlTextureParameters(S.tex.id, RL_TEXTURE_MIPMAP_BIAS_RATIO, -60);
      S.frames = frames;
      S.fw = fw, S.fh = fh, S.ax = ax, S.ay = ay, S.pxPerH = pxh;
      S.loop = loop != 0;
      S.nImpacts = 0;
      for (const char* c = impact; *c && S.nImpacts < 4;) {  // "7" or "7,22,30": increasing frames
        char* end = nullptr;
        const long v = std::strtol(c, &end, 10);
        if (end == c) break;
        if (v >= 0 && v < frames && (S.nImpacts == 0 || v > S.impacts[S.nImpacts - 1]))
          S.impacts[S.nImpacts++] = int(v);
        c = *end == ',' ? end + 1 : end;
        if (*end != ',') break;
      }
      S.impact = S.nImpacts > 0 ? S.impacts[0] : -1;
      S.fps = fps > 0.0f ? fps : 15.0f;
      S.drop = drop;
      S.ok = true;
      A.clips = true;
      break;
    }
  }
}

// The fighter's clips, loaded the first time they are needed.
const Art* clipsOf(Hero h) {
  if (size_t(h) >= g_art.size()) return nullptr;
  Art& A = g_art[size_t(h)];
  if (!A.clipsTried) {
    A.clipsTried = true;
    loadSheets(A, heroKey(h).c_str());
  }
  return &A;
}

// The frame of a generated animation the pose shows. A blow is timed on the move: the frames before the impact
// play while it winds up, the impact holds while the hitbox is out, the rest is the recovery.
int sheetFrame(const Art::Sheet& S, const Pose& pose) {
  if (S.loop && pose.sheetPhase >= 0.0f) return int(std::floor(pose.sheetPhase * S.frames)) % S.frames;
  if (S.loop) return int(std::floor(std::max(0.0f, pose.sheetT) * S.fps)) % S.frames;
  const float t = clampf(pose.sheetT, 0.0f, 1.0f);
  const int m = std::min(S.nImpacts, pose.sheetHits);
  if (m == 0) return std::clamp(int(t * S.frames), 0, S.frames - 1);
  // Piecewise: the frames before each blow spread over the time before it, the blow's frame held while it is out,
  // then the frames after the last blow spread over the rest of the move.
  int from = 0;      // first frame of the stretch being played
  float since = 0.0f;  // when that stretch started
  for (int i = 0; i < m; ++i) {
    const float a = pose.sheetHitA[i], b = std::max(a, pose.sheetHitB[i]);
    const int k = S.impacts[i];
    if (t < a) {
      const int n = k - from;  // frames from..k-1 over [since, a)
      if (n <= 0) return k;
      return std::clamp(from + int((t - since) / std::max(1e-3f, a - since) * n), from, k - 1);
    }
    if (t < b) return k;
    from = k + 1;
    since = b;
  }
  const int rest = S.frames - from;
  if (rest <= 0) return S.frames - 1;
  return std::min(S.frames - 1, from + int((t - since) / std::max(1e-3f, 1.0f - since) * rest));
}
}  // namespace

const char* sheetName(int anim) {
  static const char* const kStates[kSheetMove0] = {
      "idle", "walk", "run", "crouch", "jumpsquat", "jump", "airjump", "fall", "land", "shield", "roll", "spotdodge",
      "airdodge", "hurt", "tumble", "knockdown", "getup", "helpless", "ledge", "climb", "hold", "grabbed", "dizzy",
      "victory", "dash"};
  static const char* const kMoves[MOVE_COUNT] = {
      "jab", "jab2", "jab3", "ftilt", "utilt", "dtilt", "dashattack", "fsmash", "usmash", "dsmash", "nair", "fair",
      "bair", "uair", "dair", "nspecial", "sspecial", "uspecial", "dspecial", "grab", "pummel", "fthrow", "bthrow",
      "uthrow", "dthrow", "ledgeattack", "getupattack", "extra1", "extra2", "extra3", "ultimate"};
  static std::array<std::string, kSheetCount> names = [] {
    std::array<std::string, kSheetCount> n;
    for (int i = 0; i < kSheetMove0; ++i) {
      n[i] = kStates[i];
      n[kSheetWState0 + i] = std::string("w0_") + kStates[i];
      n[kSheetWState1 + i] = std::string("w1_") + kStates[i];
    }
    for (int m = 0; m < MOVE_COUNT; ++m) {
      n[kSheetMove0 + m] = kMoves[m];
      n[kSheetWeapon0 + m] = std::string("w0_") + kMoves[m];
      n[kSheetWeapon1 + m] = std::string("w1_") + kMoves[m];
    }
    return n;
  }();
  return anim >= 0 && anim < kSheetCount ? names[anim].c_str() : "";
}

bool hasSheet(Hero h, int a) {
  const Art* A = clipsOf(h);
  return A && a >= 0 && a < kSheetCount && A->sheets[a].ok;
}

float sheetDrop(const CharacterDef& d, int a) {
  const Art* A = clipsOf(d.hero);
  if (!A || a < 0 || a >= kSheetCount || !A->sheets[a].ok) return 0.0f;
  return A->sheets[a].drop * d.height / A->sheets[a].pxPerH;
}

bool sheetMask(const CharacterDef& d, const Pose& pose, SheetMask& out) {
  if (pose.sheet < 0 || pose.sheet >= kSheetCount) return false;
  const Art* A = clipsOf(d.hero);
  if (!A || !A->sheets[pose.sheet].ok) return false;
  return sheetMaskAt(d, pose.sheet, sheetFrame(A->sheets[pose.sheet], pose), out);
}

int sheetFrames(const CharacterDef& d, int sheet) {
  const Art* A = clipsOf(d.hero);
  return A && sheet >= 0 && sheet < kSheetCount && A->sheets[sheet].ok ? A->sheets[sheet].frames : 0;
}

bool sheetMaskAt(const CharacterDef& d, int sheet, int frame, SheetMask& out) {
  if (sheet < 0 || sheet >= kSheetCount) return false;
  const Art* A = clipsOf(d.hero);
  if (!A || !A->sheets[sheet].ok) return false;
  const Art::Sheet& S = A->sheets[sheet];
  static std::map<std::string, Image> decoded;  // the strips, decoded once on the CPU
  const std::string key = heroKey(d.hero) + "_" + sheetName(sheet);
  auto it = decoded.find(key);
  if (it == decoded.end()) {
    const unsigned char* data = nullptr;
    unsigned size = 0;
    if (!embeddedSprite(key.c_str(), &data, &size)) return false;
    Image img = LoadImageFromMemory(".png", data, int(size));
    if (!img.data) return false;
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    it = decoded.emplace(key, img).first;
  }
  const Image& img = it->second;
  out.frame = std::clamp(frame, 0, S.frames - 1);
  out.w = int(S.fw);
  out.h = int(S.fh);
  out.ax = S.ax;
  out.ay = S.ay;
  out.pxPerH = S.pxPerH;
  out.opaque.assign(size_t(out.w) * out.h, 0);
  const auto* px = static_cast<const unsigned char*>(img.data);
  for (int y = 0; y < out.h && y < img.height; ++y)
    for (int x = 0; x < out.w; ++x) {
      const int sx = out.frame * out.w + x;
      if (sx < img.width) out.opaque[size_t(y) * out.w + x] = px[(size_t(y) * img.width + sx) * 4 + 3] > 0;
    }
  return true;
}

bool hasClips(Hero h) {
  const Art* A = clipsOf(h);
  return A && A->clips;
}

bool drawSheet(const CharacterDef& d, const Pose& pose, const Xf& X, float alpha, Color flashColor, float flash) {
  if (pose.sheet < 0 || pose.sheet >= kSheetCount || alpha <= 0.0f) return false;
  const Art* A = clipsOf(d.hero);
  if (!A || !A->sheets[pose.sheet].ok) return false;
  const Art::Sheet& S = A->sheets[pose.sheet];
  const int i = sheetFrame(S, pose);
  // Frame pixels to the fighter's frame: the anchor on the feet (y = +H/2), S.pxPerH pixels per fighter height.
  const float H = d.height, k = H / S.pxPerH, flip = pose.sheetFlip ? -1.0f : 1.0f;
  auto P = [&](float x, float y) { return X(Vec2{(x - S.ax) * k * flip, (y - S.ay) * k + 0.5f * H}); };
  const Vector2 q[4] = {P(0, 0), P(S.fw, 0), P(S.fw, S.fh), P(0, S.fh)};
  const float tw = float(S.tex.width), th = float(S.tex.height);
  const float u0 = i * S.fw / tw, u1 = (i + 1) * S.fw / tw;
  const float uv[4][2] = {{u0, 0.0f}, {u1, 0.0f}, {u1, S.fh / th}, {u0, S.fh / th}};
  Color tc;
  float amount = 0.0f;
  const bool shaded = tintOf(pose, flashColor, flash, tc, amount);
  if (shaded) beginTint(tc, amount);
  rlSetTexture(S.tex.id);
  rlBegin(RL_QUADS);
  const unsigned char a = (unsigned char)(255.0f * clampf(alpha, 0.0f, 1.0f));
  for (int c = 0; c < 4; ++c) {
    rlColor4ub(255, 255, 255, a);
    rlTexCoord2f(uv[c][0], uv[c][1]);
    rlVertex2f(q[c].x, q[c].y);
  }
  rlEnd();
  rlSetTexture(0);
  if (shaded) EndShaderMode();
  return true;
}

namespace {
struct Picture {
  Texture2D tex{};
  bool ok = false;
};
const Picture* pictureOf(const char* name) {
  static std::map<std::string, Picture> cache;
  auto it = cache.find(name);
  if (it != cache.end()) return it->second.ok ? &it->second : nullptr;
  Picture& P = cache[name];
  const unsigned char* data = nullptr;
  unsigned size = 0;
  if (embeddedSprite(name, &data, &size)) {
    Image img = LoadImageFromMemory(".png", data, int(size));
    if (img.data) {
      P.tex = LoadTextureFromImage(img);
      UnloadImage(img);
      if (P.tex.id != 0) {
        GenTextureMipmaps(&P.tex);
        SetTextureFilter(P.tex, TEXTURE_FILTER_TRILINEAR);
        P.ok = true;
      }
    }
  }
  return P.ok ? &P : nullptr;
}
}  // namespace

bool pictureSize(const char* name, float& w, float& h) {
  const Picture* P = pictureOf(name);
  if (!P) return false;
  w = float(P->tex.width);
  h = float(P->tex.height);
  return true;
}

bool drawPicture(const char* name, Rectangle src, Rectangle dst, float alpha, bool flipX, Color tint) {
  const Picture* P = pictureOf(name);
  if (!P || alpha <= 0.0f) return P != nullptr;
  if (flipX) src.width = -src.width;
  DrawTexturePro(P->tex, src, dst, {0, 0}, 0.0f, withAlpha(tint, alpha * tint.a / 255.0f));
  return true;
}

bool drawPicture3(const char* name, Rectangle dst, int cap) {
  const Picture* P = pictureOf(name);
  if (!P) return false;
  const NPatchInfo n{{0, 0, float(P->tex.width), float(P->tex.height)}, cap, 0, cap, 0, NPATCH_THREE_PATCH_HORIZONTAL};
  DrawTextureNPatch(P->tex, n, dst, {0, 0}, 0.0f, WHITE);
  return true;
}
}  // namespace art

namespace {
struct Fx {
  art::FxInfo info;
  Texture2D tex{};
  bool ok = false;
};
const Fx* fxOf(const char* name) {
  static std::map<std::string, Fx> cache;
  auto it = cache.find(name);
  if (it != cache.end()) return it->second.ok ? &it->second : nullptr;
  Fx& F = cache[name];
  const std::string& index = art::spriteIndex();
  size_t pos = 0;
  while (pos < index.size()) {
    size_t end = index.find('\n', pos);
    if (end == std::string::npos) end = index.size();
    const std::string line = index.substr(pos, end - pos);
    pos = end + 1;
    char key[64] = {};
    int frames = 0, loop = 0;
    float fw = 0, fh = 0, ax = 0, ay = 0, fps = 15.0f, hit = 0.0f;
    if (std::sscanf(line.c_str(), "fx %63s %d %f %f %f %f %f %d %f", key, &frames, &fw, &fh, &ax, &ay, &fps, &loop, &hit) < 6 ||
        std::strcmp(key, name) != 0 || frames <= 0)
      continue;
    const unsigned char* data = nullptr;
    unsigned size = 0;
    if (!art::embeddedSprite(("fx_" + std::string(name)).c_str(), &data, &size)) break;
    Image img = LoadImageFromMemory(".png", data, int(size));
    if (!img.data) break;
    F.tex = LoadTextureFromImage(img);
    UnloadImage(img);
    if (F.tex.id == 0) break;
    GenTextureMipmaps(&F.tex);
    SetTextureFilter(F.tex, TEXTURE_FILTER_TRILINEAR);
    F.info = {frames, fw, fh, ax, ay, fps > 0.0f ? fps : 15.0f, loop != 0, hit};
    F.ok = true;
    break;
  }
  return F.ok ? &F : nullptr;
}
}  // namespace

namespace art {
const FxInfo* fxInfo(const char* name) {
  const Fx* F = fxOf(name);
  return F ? &F->info : nullptr;
}

bool drawFxFrame(const char* name, int frame, Vec2 at, float size, float rot, bool flip, float alpha, Color tint) {
  const Fx* F = fxOf(name);
  if (!F || alpha <= 0.0f) return false;
  const FxInfo& I = F->info;
  const int i = std::clamp(frame, 0, I.frames - 1);
  const float c = std::cos(rot), s = std::sin(rot), fl = flip ? -1.0f : 1.0f;
  auto P = [&](float x, float y) {
    const float lx = (x - I.ax) * size * fl, ly = (y - I.ay) * size;
    return Vector2{at.x + lx * c - ly * s, at.y + lx * s + ly * c};
  };
  const Vector2 q[4] = {P(0, 0), P(I.fw, 0), P(I.fw, I.fh), P(0, I.fh)};
  const float tw = float(F->tex.width), th = float(F->tex.height);
  const float u0 = i * I.fw / tw, u1 = (i + 1) * I.fw / tw;
  const float uv[4][2] = {{u0, 0.0f}, {u1, 0.0f}, {u1, I.fh / th}, {u0, I.fh / th}};
  rlSetTexture(F->tex.id);
  rlBegin(RL_QUADS);
  const unsigned char a = (unsigned char)(tint.a * std::clamp(alpha, 0.0f, 1.0f));
  for (int k = 0; k < 4; ++k) {
    rlColor4ub(tint.r, tint.g, tint.b, a);
    rlTexCoord2f(uv[k][0], uv[k][1]);
    rlVertex2f(q[k].x, q[k].y);
  }
  rlEnd();
  rlSetTexture(0);
  return true;
}

bool drawFxStrip(const char* name, float t, Vec2 from, Vec2 dir, float len, float size, float alpha, float fadeIn) {
  const Fx* F = fxOf(name);
  if (!F || alpha <= 0.0f || len <= 0.0f || size <= 0.0f) return F != nullptr;
  const FxInfo& I = F->info;
  int frame = int(std::floor(std::max(0.0f, t) * I.fps));
  frame = I.loop ? frame % I.frames : std::min(frame, I.frames - 1);
  const Vec2 d = dir.len2() > 1e-6f ? dir.norm() : Vec2{1.0f, 0.0f}, n{-d.y, d.x};
  const float tile = I.fw * size, top = I.ay * size, bottom = (I.fh - I.ay) * size;
  const float tw = float(F->tex.width), th = float(F->tex.height);
  const float a = 255.0f * std::clamp(alpha, 0.0f, 1.0f);
  auto alphaAt = [&](float x) {  // the fade at the start, linear: the GPU interpolates it between the cuts
    return (unsigned char)(a * (fadeIn > 0.0f ? std::clamp(x / fadeIn, 0.0f, 1.0f) : 1.0f));
  };
  rlSetTexture(F->tex.id);
  rlBegin(RL_QUADS);
  for (float x = 0.0f; x < len; x += tile) {
    const float w = std::min(tile, len - x), u0 = frame * I.fw / tw;
    // The tile is cut where the fade ends so the ramp stays straight.
    const float cuts[3] = {x, fadeIn > x && fadeIn < x + w ? fadeIn : x + w, x + w};
    for (int c = 0; c < 2; ++c) {
      const float xa = cuts[c], xb = cuts[c + 1];
      if (xb <= xa) continue;
      const float ua = u0 + (xa - x) / tile * I.fw / tw, ub = u0 + (xb - x) / tile * I.fw / tw;
      const Vec2 p0 = from + d * xa, p1 = from + d * xb;
      const Vec2 q[4] = {p0 - n * top, p1 - n * top, p1 + n * bottom, p0 + n * bottom};
      const float uv[4][2] = {{ua, 0.0f}, {ub, 0.0f}, {ub, I.fh / th}, {ua, I.fh / th}};
      const unsigned char qa[4] = {alphaAt(xa), alphaAt(xb), alphaAt(xb), alphaAt(xa)};
      for (int k = 0; k < 4; ++k) {
        rlColor4ub(255, 255, 255, qa[k]);
        rlTexCoord2f(uv[k][0], uv[k][1]);
        rlVertex2f(q[k].x, q[k].y);
      }
    }
  }
  rlEnd();
  rlSetTexture(0);
  return true;
}

bool drawFx(const char* name, float t, Vec2 at, float size, float rot, bool flip, float alpha, Color tint) {
  const Fx* F = fxOf(name);
  if (!F) return false;
  const FxInfo& I = F->info;
  int frame = int(std::floor(std::max(0.0f, t) * I.fps));
  frame = I.loop ? frame % I.frames : std::min(frame, I.frames - 1);
  return drawFxFrame(name, frame, at, size, rot, flip, alpha, tint);
}
}  // namespace art
