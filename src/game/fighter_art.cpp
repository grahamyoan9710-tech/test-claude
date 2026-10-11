// Fighters drawn from an illustration (see fighter_art.hpp).
//
// Each part of the picture is a grid of small quads whose corners are skinned on the skeleton: a corner
// follows its own bone, and blends into the neighbouring bone around a joint (elbow, knee...), so the picture
// bends there instead of breaking. The bones keep the picture's proportions; their rotations come from the
// pose's joints (the same IK targets as the drawn fighters).
#include "game/fighter_art.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#include "rlgl.h"

extern "C" {
extern const unsigned char fighter_naruto_png[], fighter_naruto_rig[];
extern const unsigned int fighter_naruto_png_size, fighter_naruto_rig_size;
extern const unsigned char fighter_ganondorf_png[], fighter_ganondorf_rig[];
extern const unsigned int fighter_ganondorf_png_size, fighter_ganondorf_rig_size;
}

namespace art {
namespace {

struct Source {
  Hero hero;
  const unsigned char* png;
  const unsigned int* pngSize;
  const unsigned char* rig;
  const unsigned int* rigSize;
};
const Source kSources[] = {
    {Hero::Naruto, fighter_naruto_png, &fighter_naruto_png_size, fighter_naruto_rig, &fighter_naruto_rig_size},
    {Hero::Ganondorf, fighter_ganondorf_png, &fighter_ganondorf_png_size, fighter_ganondorf_rig,
     &fighter_ganondorf_rig_size},
};

enum Bone { kTorso, kHead, kUpF, kLoF, kUpB, kLoB, kThF, kShF, kFtF, kThB, kShB, kFtB, kBones };
enum Kind { kPartTorso, kPartHead, kPartArmF, kPartArmB, kPartLegF, kPartLegB, kPartWhole };
constexpr int kCell = 12;  // grid step on the picture (px)

struct Joints {
  Vec2 neck, head, hips, shF, elbowF, handF, shB, elbowB, handB;
  Vec2 hipF, kneeF, ankleF, footF, hipB, kneeB, ankleB, footB;
};
struct JointName {
  const char* name;
  Vec2 Joints::*at;
};
const JointName kJoints[] = {
    {"neck", &Joints::neck},     {"head", &Joints::head},     {"hips", &Joints::hips},
    {"shF", &Joints::shF},       {"elbowF", &Joints::elbowF}, {"handF", &Joints::handF},
    {"shB", &Joints::shB},       {"elbowB", &Joints::elbowB}, {"handB", &Joints::handB},
    {"hipF", &Joints::hipF},     {"kneeF", &Joints::kneeF},   {"ankleF", &Joints::ankleF},
    {"footF", &Joints::footF},   {"hipB", &Joints::hipB},     {"kneeB", &Joints::kneeB},
    {"ankleB", &Joints::ankleB}, {"footB", &Joints::footB},
};

struct Vert {
  Vec2 src;   // on the picture (px)
  Vec2 n;     // bind position in fighter heights: origin under the hips at mid height, feet at y = 0.5
  float u, v; // atlas
  uint8_t bone[4];
  float w[4];
};
struct Part {
  Kind kind = kPartTorso;
  int nx = 0, ny = 0;
  std::vector<Vert> verts;  // (nx + 1) * (ny + 1)
  std::vector<int> cells;   // the cells with something drawn in them
};
struct Limb {  // lengths in fighter heights, IK bend side
  float l1 = 0, l2 = 0, bend = 1;
};

struct Art {
  bool tried = false, ok = false, rigged = false;
  Texture2D tex{};
  float ground = 0, top = 0, unit = 1;  // unit: picture px per fighter height
  Joints px{}, n{};
  uint32_t have = 0;  // joints found (bit per kJoints entry)
  float blendNeck = 10, blendShoulder = 14, blendElbow = 14, blendHip = 18, blendKnee = 18, blendAnkle = 12;
  Limb armF, armB, legF, legB;
  float footPhiF = 0, footPhiB = 0;  // foot tilt of the picture's shins
  std::vector<Part> parts;           // in drawing order
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

float sstep(float e0, float e1, float x) {
  const float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}
float angleOf(Vec2 v) { return std::atan2(v.y, v.x); }

// Two-bone IK (the drawn fighters' solver, with a relative reach margin).
Vec2 ik(Vec2 root, Vec2 target, float l1, float l2, float bend, Vec2& end) {
  const Vec2 d = target - root;
  const float dist = clampf(d.len(), 1e-4f, (l1 + l2) * 0.9995f);
  const float c = clampf((l1 * l1 + dist * dist - l2 * l2) / (2.0f * l1 * dist), -1.0f, 1.0f);
  const float a = std::acos(c), base = std::atan2(d.y, d.x);
  end = root + (d.len2() > 1e-8f ? d.norm() : Vec2{1.0f, 0.0f}) * dist;
  return root + Vec2{std::cos(base + bend * a), std::sin(base + bend * a)} * l1;
}

// Foot tilt: flat on the ground while the shin is near vertical, following it when it swings far (kicks).
float footPhi(Vec2 shin) {
  const float phi = shin.len2() > 1e-8f ? std::atan2(shin.x, shin.y) : 0.0f;
  return clampf(phi, -1.7f, 1.7f) * smoothstep((std::fabs(phi) - 0.3f) / 0.6f);
}

Limb limbOf(Vec2 a, Vec2 b, Vec2 c) {
  Limb l;
  l.l1 = (b - a).len();
  l.l2 = (c - b).len();
  Vec2 e;
  const Vec2 p = ik(a, c, l.l1, l.l2, 1.0f, e), q = ik(a, c, l.l1, l.l2, -1.0f, e);
  l.bend = (p - b).len2() <= (q - b).len2() ? 1.0f : -1.0f;
  return l;
}

// ---- Loading ----------------------------------------------------------------------------------------
void weigh(const Art& A, Kind kind, Vert& v) {
  const Joints& j = A.px;
  const Vec2 p = v.src;
  float w[4] = {1, 0, 0, 0};
  uint8_t b[4] = {kTorso, kTorso, kTorso, kTorso};
  switch (kind) {
    case kPartHead: {
      const Vec2 u = (j.head - j.neck).norm();
      const float bn = A.blendNeck;
      const float near = 1.0f - sstep(bn * 3.0f, bn * 6.0f, (p - j.neck).len());
      const float wt = sstep(bn, -bn, dot(p - j.neck, u)) * near;
      b[0] = kHead, w[0] = 1.0f - wt;
      b[1] = kTorso, w[1] = wt;
      break;
    }
    case kPartArmF: case kPartArmB: {
      const bool f = kind == kPartArmF;
      const Vec2 sh = f ? j.shF : j.shB, el = f ? j.elbowF : j.elbowB, hd = f ? j.handF : j.handB;
      const Vec2 uu = (el - sh).norm(), ul = (hd - el).norm();
      Vec2 ne = (uu + ul).norm();
      if (ne.len2() < 0.5f) ne = uu;
      const float wt = sstep(A.blendShoulder, -A.blendShoulder, dot(p - sh, uu));
      const float wl = sstep(-A.blendElbow, A.blendElbow, dot(p - el, ne));
      b[0] = kTorso, w[0] = wt;
      b[1] = f ? kUpF : kUpB, w[1] = (1.0f - wt) * (1.0f - wl);
      b[2] = f ? kLoF : kLoB, w[2] = (1.0f - wt) * wl;
      break;
    }
    case kPartLegF: case kPartLegB: {
      const bool f = kind == kPartLegF;
      const Vec2 hp = f ? j.hipF : j.hipB, kn = f ? j.kneeF : j.kneeB, an = f ? j.ankleF : j.ankleB,
                 ft = f ? j.footF : j.footB;
      const Vec2 ut = (kn - hp).norm(), us = (an - kn).norm(), uf = (ft - an).norm();
      Vec2 nk = (ut + us).norm(), na = (us + uf).norm();
      if (nk.len2() < 0.5f) nk = ut;
      if (na.len2() < 0.5f) na = us;
      const float wt = sstep(A.blendHip, -A.blendHip, dot(p - hp, ut));
      const float wk = sstep(-A.blendKnee, A.blendKnee, dot(p - kn, nk));
      const float wa = sstep(-A.blendAnkle, A.blendAnkle, dot(p - an, na));
      b[0] = kTorso, w[0] = wt;
      b[1] = f ? kThF : kThB, w[1] = (1.0f - wt) * (1.0f - wk);
      b[2] = f ? kShF : kShB, w[2] = (1.0f - wt) * wk * (1.0f - wa);
      b[3] = f ? kFtF : kFtB, w[3] = (1.0f - wt) * wk * wa;
      break;
    }
    default: break;
  }
  for (int i = 0; i < 4; ++i) {
    v.bone[i] = b[i];
    v.w[i] = w[i];
  }
}

void load(Art& A, const Source& s) {
  const std::string text(reinterpret_cast<const char*>(s.rig), *s.rigSize);
  struct PartLine {
    std::string name;
    int ax, ay, w, h, ox, oy;
  };
  std::vector<PartLine> lines;
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find('\n', pos);
    if (end == std::string::npos) end = text.size();
    const std::string line = text.substr(pos, end - pos);
    pos = end + 1;
    char key[32] = {}, name[32] = {};
    float x = 0, y = 0;
    PartLine pl;
    if (std::sscanf(line.c_str(), "ground %f", &x) == 1) A.ground = x;
    else if (std::sscanf(line.c_str(), "top %f", &x) == 1) A.top = x;
    else if (std::sscanf(line.c_str(), "joint %31s %f %f", name, &x, &y) == 3) {
      for (size_t i = 0; i < std::size(kJoints); ++i)
        if (std::strcmp(kJoints[i].name, name) == 0) {
          A.px.*kJoints[i].at = {x, y};
          A.have |= 1u << i;
        }
    } else if (std::sscanf(line.c_str(), "blend %31s %f", name, &x) == 2) {
      const std::string k = name;
      if (k == "neck") A.blendNeck = x;
      if (k == "shoulder") A.blendShoulder = x;
      if (k == "elbow") A.blendElbow = x;
      if (k == "hip") A.blendHip = x;
      if (k == "knee") A.blendKnee = x;
      if (k == "ankle") A.blendAnkle = x;
    } else if (std::sscanf(line.c_str(), "part %31s %d %d %d %d %d %d", key, &pl.ax, &pl.ay, &pl.w, &pl.h, &pl.ox,
                           &pl.oy) == 7) {
      pl.name = key;
      lines.push_back(pl);
    }
  }
  const uint32_t all = (1u << std::size(kJoints)) - 1u;
  A.rigged = (A.have & all) == all;
  if (lines.empty() || A.ground <= A.top) return;
  A.unit = A.ground - A.top;
  if (!(A.have & 4u)) A.px.hips = {lines[0].ox + lines[0].w * 0.5f, A.ground};  // portrait only: centre line
  auto norm = [&](Vec2 p) { return Vec2{(p.x - A.px.hips.x) / A.unit, (p.y - A.ground) / A.unit + 0.5f}; };
  for (const JointName& jn : kJoints) A.n.*jn.at = norm(A.px.*jn.at);
  if (A.rigged) {
    const Joints& n = A.n;
    A.armF = limbOf(n.shF, n.elbowF, n.handF);
    A.armB = limbOf(n.shB, n.elbowB, n.handB);
    A.legF = limbOf(n.hipF, n.kneeF, n.ankleF);
    A.legB = limbOf(n.hipB, n.kneeB, n.ankleB);
    A.footPhiF = footPhi(n.footF - n.kneeF);
    A.footPhiB = footPhi(n.footB - n.kneeB);
  }

  Image img = LoadImageFromMemory(".png", s.png, int(*s.pngSize));
  if (!img.data) return;
  ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  const unsigned char* px = static_cast<const unsigned char*>(img.data);
  const int AW = img.width, AH = img.height;
  auto alphaAt = [&](int x, int y) {
    if (x < 0 || y < 0 || x >= AW || y >= AH) return 0;
    return int(px[(size_t(y) * AW + x) * 4 + 3]);
  };
  for (const PartLine& pl : lines) {
    Part P;
    const std::string& nm = pl.name;
    P.kind = nm == "torso" ? kPartTorso : nm == "head" ? kPartHead : nm == "armF" ? kPartArmF
           : nm == "armB" ? kPartArmB : nm == "legF" ? kPartLegF : nm == "legB" ? kPartLegB : kPartWhole;
    if (!A.rigged) P.kind = kPartWhole;
    P.nx = (pl.w + kCell - 1) / kCell;
    P.ny = (pl.h + kCell - 1) / kCell;
    P.verts.resize(size_t(P.nx + 1) * (P.ny + 1));
    for (int jy = 0; jy <= P.ny; ++jy)
      for (int ix = 0; ix <= P.nx; ++ix) {
        Vert& v = P.verts[size_t(jy) * (P.nx + 1) + ix];
        const float sx = float(std::min(ix * kCell, pl.w)), sy = float(std::min(jy * kCell, pl.h));
        v.src = {pl.ox + sx, pl.oy + sy};
        v.n = norm(v.src);
        v.u = (pl.ax + sx) / AW;
        v.v = (pl.ay + sy) / AH;
        weigh(A, P.kind, v);
      }
    for (int jy = 0; jy < P.ny; ++jy)
      for (int ix = 0; ix < P.nx; ++ix) {
        const int x0 = pl.ax + ix * kCell - 1, y0 = pl.ay + jy * kCell - 1;
        const int x1 = pl.ax + std::min((ix + 1) * kCell, pl.w) + 1, y1 = pl.ay + std::min((jy + 1) * kCell, pl.h) + 1;
        bool any = false;
        for (int y = y0; y < y1 && !any; ++y)
          for (int x = x0; x < x1; ++x)
            if (alphaAt(x, y) > 0) {
              any = true;
              break;
            }
        if (any) P.cells.push_back(ix + jy * P.nx);
      }
    A.parts.push_back(std::move(P));
  }
  A.tex = LoadTextureFromImage(img);
  UnloadImage(img);
  if (A.tex.id == 0) return;
  GenTextureMipmaps(&A.tex);
  SetTextureFilter(A.tex, TEXTURE_FILTER_TRILINEAR);
  A.ok = true;
}

Art* get(Hero h) {
  if (size_t(h) >= g_art.size()) return nullptr;
  Art& A = g_art[size_t(h)];
  if (!A.tried) {
    A.tried = true;
    for (const Source& s : kSources)
      if (s.hero == h) load(A, s);
  }
  return A.ok ? &A : nullptr;
}

// ---- Drawing ----------------------------------------------------------------------------------------
struct Mask {
  enum { None, Box, Disc } kind = None;
  Vec2 c;               // centre
  float a = 0, b = 0;   // Box: half width a, from y = c.y - b to the bottom fade (c.y + 0.3a .. c.y + a); Disc: radius a
  float at(Vector2 p) const {
    if (kind == Box) {
      const float edge = a * 0.08f;
      return sstep(c.x - a, c.x - a + edge, p.x) * sstep(c.x + a, c.x + a - edge, p.x) *
             sstep(c.y - b, c.y - b + edge, p.y) * sstep(c.y + a, c.y + a * 0.3f, p.y);
    }
    if (kind == Disc) return sstep(a, a - 1.2f, (W(p) - c).len());
    return 1.0f;
  }
};

// Draws every part: `place` gives the screen position of a grid corner. `sub` splits the cells (fine masks).
template <class Place>
void drawParts(const Art& A, Place place, Color tint, const Mask& mask, int sub = 1) {
  static std::vector<Vector2> pts;
  static std::vector<float> ks;
  rlSetTexture(A.tex.id);
  rlBegin(RL_QUADS);
  for (const Part& P : A.parts) {
    pts.resize(P.verts.size());
    ks.resize(P.verts.size());
    for (size_t i = 0; i < P.verts.size(); ++i) {
      pts[i] = place(P.verts[i]);
      ks[i] = mask.at(pts[i]);
    }
    const int row = P.nx + 1;
    for (int c : P.cells) {
      const int ix = c % P.nx, jy = c / P.nx;
      const int i00 = jy * row + ix, i10 = i00 + 1, i01 = i00 + row, i11 = i01 + 1;
      const Vert *v00 = &P.verts[i00], *v10 = &P.verts[i10], *v01 = &P.verts[i01], *v11 = &P.verts[i11];
      if (mask.kind != Mask::None && ks[i00] <= 0.0f && ks[i10] <= 0.0f && ks[i01] <= 0.0f && ks[i11] <= 0.0f)
        continue;
      for (int sy = 0; sy < sub; ++sy)
        for (int sx = 0; sx < sub; ++sx) {
          const float fx[2] = {float(sx) / sub, float(sx + 1) / sub}, fy[2] = {float(sy) / sub, float(sy + 1) / sub};
          // corners in drawing order: top-left, bottom-left, bottom-right, top-right
          const int ox[4] = {0, 0, 1, 1}, oy[4] = {0, 1, 1, 0};
          for (int k = 0; k < 4; ++k) {
            const float a = fx[ox[k]], b = fy[oy[k]];
            const float w00 = (1 - a) * (1 - b), w10 = a * (1 - b), w01 = (1 - a) * b, w11 = a * b;
            const Vector2 p{pts[i00].x * w00 + pts[i10].x * w10 + pts[i01].x * w01 + pts[i11].x * w11,
                            pts[i00].y * w00 + pts[i10].y * w10 + pts[i01].y * w01 + pts[i11].y * w11};
            const float u = v00->u * w00 + v10->u * w10 + v01->u * w01 + v11->u * w11;
            const float v = v00->v * w00 + v10->v * w10 + v01->v * w01 + v11->v * w11;
            const float m = sub > 1 && mask.kind != Mask::None ? mask.at(p)
                                                               : ks[i00] * w00 + ks[i10] * w10 + ks[i01] * w01 + ks[i11] * w11;
            rlColor4ub(tint.r, tint.g, tint.b, (unsigned char)(tint.a * clampf(m, 0.0f, 1.0f)));
            rlTexCoord2f(u, v);
            rlVertex2f(p.x, p.y);
          }
        }
    }
  }
  rlEnd();
  rlSetTexture(0);
}

struct BoneXf {
  Vec2 bind, now;
  float c = 1, s = 0;
};

// The picture in its own pose, placed with a uniform scale: `anchor` (picture px) lands on `at`.
void drawStill(const Art& A, Vec2 anchor, Vec2 at, float k, int facing, float alpha, const Mask& mask, int sub) {
  drawParts(A, [&](const Vert& v) { return V(at + Vec2{(v.src.x - anchor.x) * k * facing, (v.src.y - anchor.y) * k}); },
            withAlpha(WHITE, alpha), mask, sub);
}
}  // namespace

bool rigged(Hero h) {
  const Art* A = get(h);
  return A && A->rigged;
}

bool portrait(Hero h) { return get(h) != nullptr; }

bool solve(const CharacterDef& d, const Pose& pose, Rig& r) {
  const Art* A = get(d.hero);
  if (!A || !A->rigged) return false;
  const float H = d.height;
  const Joints& n = A->n;
  auto L = [&](Vec2 v) { return v * H; };
  r.hips = {0.0f, n.hips.y * H + pose.crouch * 0.14f * H};
  r.neck = r.hips + rotate(L(n.neck - n.hips), pose.lean);
  r.chest = lerpv(r.hips, r.neck, 0.62f);
  r.head = r.neck + rotate(L(n.head - n.neck), pose.lean * 1.15f);
  r.hr = std::max(0.04f, n.head.y + 0.5f) * H;
  r.tilt = pose.lean * 0.55f;
  const float tw = clampf(pose.twist, -1.2f, 1.2f);
  r.shF = r.neck + rotate(L(n.shF - n.neck) + Vec2{-0.045f * tw * H, 0.0f}, pose.lean);
  r.shB = r.neck + rotate(L(n.shB - n.neck) + Vec2{0.065f * tw * H, 0.0f}, pose.lean);
  r.hipF = r.hips + L(n.hipF - n.hips) + Vec2{-0.012f * tw * H, 0.0f};
  r.hipB = r.hips + L(n.hipB - n.hips) + Vec2{0.018f * tw * H, 0.0f};
  r.elbowF = ik(r.shF, pose.handF, A->armF.l1 * H, A->armF.l2 * H, A->armF.bend, r.handF);
  r.elbowB = ik(r.shB, pose.handB, A->armB.l1 * H, A->armB.l2 * H, A->armB.bend, r.handB);
  auto leg = [&](const Limb& l, Vec2 hip, Vec2 target, Vec2 nAnkle, Vec2 nFoot, float phi0, Vec2& knee, Vec2& foot) {
    float th = 0.0f;
    Vec2 ankle;
    for (int it = 0; it < 2; ++it) {
      knee = ik(hip, target + rotate(L(nAnkle - nFoot), th), l.l1 * H, l.l2 * H, l.bend, ankle);
      th = -(footPhi(target - knee) - phi0);
    }
    foot = ankle + rotate(L(nFoot - nAnkle), th);
  };
  leg(A->legF, r.hipF, pose.footF, n.ankleF, n.footF, A->footPhiF, r.kneeF, r.footF);
  leg(A->legB, r.hipB, pose.footB, n.ankleB, n.footB, A->footPhiB, r.kneeB, r.footB);
  return true;
}

bool stance(const CharacterDef& d, Pose& p, float bob) {
  const Art* A = get(d.hero);
  if (!A || !A->rigged) return false;
  const float H = d.height;
  p.handF = A->n.handF * H + Vec2{0.0f, bob};
  p.handB = A->n.handB * H + Vec2{0.0f, bob};
  p.footF = A->n.footF * H;
  p.footB = A->n.footB * H;
  p.lean -= 0.06f;  // the picture stands as it was drawn (the rest pose leans 0.06 forward)
  return true;
}

void drawFighter(const CharacterDef& d, const Pose& pose, const Rig& r, const Xf& X, float alpha, Color flashColor,
                 float flash) {
  const Art* A = get(d.hero);
  if (!A || !A->rigged || alpha <= 0.0f) return;
  const float H = d.height;
  const Joints& n = A->n;
  BoneXf T[kBones];
  auto bone = [&](int b, Vec2 bind, Vec2 now, float th) { T[b] = {bind, now, std::cos(th), std::sin(th)}; };
  auto turn = [](Vec2 now, Vec2 bind) { return angleOf(now) - angleOf(bind); };
  bone(kTorso, n.hips, r.hips, pose.lean);
  bone(kHead, n.head, r.head, r.tilt);
  bone(kUpF, n.shF, r.shF, turn(r.elbowF - r.shF, n.elbowF - n.shF));
  bone(kLoF, n.elbowF, r.elbowF, turn(r.handF - r.elbowF, n.handF - n.elbowF));
  bone(kUpB, n.shB, r.shB, turn(r.elbowB - r.shB, n.elbowB - n.shB));
  bone(kLoB, n.elbowB, r.elbowB, turn(r.handB - r.elbowB, n.handB - n.elbowB));
  auto leg = [&](int th, int sh, int ft, Vec2 hip, Vec2 knee, Vec2 foot, Vec2 nHip, Vec2 nKnee, Vec2 nAnkle, Vec2 nFoot,
                 float phi0) {
    const float a = -(footPhi(foot - knee) - phi0);
    const Vec2 ankle = foot + rotate((nAnkle - nFoot) * H, a);
    bone(th, nHip, hip, turn(knee - hip, nKnee - nHip));
    bone(sh, nKnee, knee, turn(ankle - knee, nAnkle - nKnee));
    bone(ft, nAnkle, ankle, a);
  };
  leg(kThF, kShF, kFtF, r.hipF, r.kneeF, r.footF, n.hipF, n.kneeF, n.ankleF, n.footF, A->footPhiF);
  leg(kThB, kShB, kFtB, r.hipB, r.kneeB, r.footB, n.hipB, n.kneeB, n.ankleB, n.footB, A->footPhiB);

  // Tint: hit flash, afterimage, Kurama's golden chakra cloak.
  Color tc = flashColor;
  float amount = clampf(flash, 0.0f, 1.0f);
  if (pose.ghost) amount = std::max(amount, 0.65f);
  if (pose.form == Form::KuramaAvatar && amount < 0.35f) {
    tc = mixColor({255, 190, 60, 255}, flashColor, amount / 0.35f);
    amount = 0.35f;
  }
  if (!g_tintTried) {
    g_tintTried = true;
    g_tint = LoadShaderFromMemory(nullptr, kTintFrag);
    g_tintLoc = g_tint.id ? GetShaderLocation(g_tint, "tint") : -1;
  }
  const bool shaded = amount > 0.01f && g_tint.id != 0 && g_tintLoc >= 0;
  if (shaded) {
    BeginShaderMode(g_tint);
    const float v[4] = {tc.r / 255.0f, tc.g / 255.0f, tc.b / 255.0f, amount};
    SetShaderValue(g_tint, g_tintLoc, v, SHADER_UNIFORM_VEC4);
  }
  drawParts(*A, [&](const Vert& v) {
    Vec2 p{};
    for (int i = 0; i < 4; ++i) {
      if (v.w[i] <= 0.0f) continue;
      const BoneXf& b = T[v.bone[i]];
      const Vec2 dd = (v.n - b.bind) * H;
      p += Vec2{b.c * dd.x - b.s * dd.y + b.now.x, b.s * dd.x + b.c * dd.y + b.now.y} * v.w[i];
    }
    return X(p);
  }, withAlpha(WHITE, alpha), Mask{});
  if (shaded) EndShaderMode();
}

bool drawWhole(Hero h, Vec2 feet, float height, int facing, float alpha) {
  const Art* A = get(h);
  if (!A) return false;
  drawStill(*A, {A->px.hips.x, A->ground}, feet, height / A->unit, facing < 0 ? -1 : 1, alpha, Mask{}, 1);
  return true;
}

bool drawBust(Hero h, Vec2 center, float size, float alpha) {
  const Art* A = get(h);
  if (!A) return false;
  const float headR = std::max(8.0f, A->px.head.y - A->top);
  const float k = size * 0.2f / headR;
  Mask m;  // the frame of the drawn bust: the hair may rise a little above it, the chest fades out below
  m.kind = Mask::Box;
  m.c = center;
  m.a = size * 0.5f;
  m.b = size * 0.62f;
  drawStill(*A, A->px.head, center + Vec2{size * 0.02f, -size * 0.12f}, k, 1, alpha, m, 2);
  return true;
}

bool drawDisc(Hero h, Vec2 center, float radius, float alpha) {
  const Art* A = get(h);
  if (!A) return false;
  const float headR = std::max(8.0f, A->px.head.y - A->top);
  const float k = radius * 0.62f / headR;
  Mask m;
  m.kind = Mask::Disc;
  m.c = center;
  m.a = radius * 0.94f;
  drawStill(*A, A->px.head, center + Vec2{0.0f, radius * 0.1f}, k, 1, alpha, m, 4);
  return true;
}
}  // namespace art
