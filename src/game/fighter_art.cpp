// Fighters made from an illustration (see fighter_art.hpp).
//
// The parts taken from the picture (head, torso, skirt, cape) are grids of small quads whose corners follow the
// skeleton: the head and the torso rigidly, a skirt between the hips and both legs, a cape with the torso and
// trailing behind. The limbs are not pieces of the picture: each segment is drawn for the pose as a volume — the
// widths measured on the picture, its pattern from a straightened strip of the picture, lit by the game in the
// picture's own tones — with the picture's fists and shoes at the ends. The bones keep the picture's proportions;
// their directions come from the same pose on the drawn build (solve()).
#include "game/fighter_art.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "game/pixel.hpp"
#include "rlgl.h"

// The pictures embedded at build time (CLASH_FIGHTER_ART in CMakeLists.txt).
#define FIGHTER_ART(name)                                                      \
  extern const unsigned char fighter_##name##_png[], fighter_##name##_rig[]; \
  extern const unsigned int fighter_##name##_png_size, fighter_##name##_rig_size;
extern "C" {
FIGHTER_ART(asta)
FIGHTER_ART(yuno)
FIGHTER_ART(ichigo)
FIGHTER_ART(byakuya)
FIGHTER_ART(kenpachi)
FIGHTER_ART(ulquiorra)
FIGHTER_ART(grimmjow)
FIGHTER_ART(starrk)
FIGHTER_ART(naruto)
FIGHTER_ART(sasuke)
FIGHTER_ART(kakashi)
FIGHTER_ART(gojo)
FIGHTER_ART(sukuna)
FIGHTER_ART(link)
FIGHTER_ART(ganondorf)
FIGHTER_ART(kratos)
FIGHTER_ART(hunter)
}
#undef FIGHTER_ART
// Generated animations (assets/sprites, embedded by CMakeLists.txt with their index sheets.txt).
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

struct Source {
  Hero hero;
  const char* name;
  const unsigned char* png;
  const unsigned int* pngSize;
  const unsigned char* rig;
  const unsigned int* rigSize;
};
#define SOURCE(hero, name)                                                                            \
  {Hero::hero, #name, fighter_##name##_png, &fighter_##name##_png_size, fighter_##name##_rig, \
   &fighter_##name##_rig_size}
const Source kSources[] = {
    SOURCE(Asta, asta),         SOURCE(Yuno, yuno),       SOURCE(Ichigo, ichigo), SOURCE(Byakuya, byakuya),
    SOURCE(Kenpachi, kenpachi), SOURCE(Ulquiorra, ulquiorra), SOURCE(Grimmjow, grimmjow), SOURCE(Starrk, starrk),
    SOURCE(Naruto, naruto),     SOURCE(Sasuke, sasuke),   SOURCE(Kakashi, kakashi), SOURCE(Gojo, gojo),
    SOURCE(Sukuna, sukuna),     SOURCE(Link, link),       SOURCE(Ganondorf, ganondorf), SOURCE(Kratos, kratos),
    SOURCE(Hunter, hunter),
};
#undef SOURCE

const char* heroKey(Hero h) {  // "naruto": the prefix of the fighter's files
  for (const Source& s : kSources)
    if (s.hero == h) return s.name;
  return "";
}

enum Bone { kTorso, kHead, kUpF, kLoF, kUpB, kLoB, kThF, kShF, kFtF, kThB, kShB, kFtB, kBones };
enum Kind { kPartTorso, kPartHead, kPartArmF, kPartArmB, kPartLegF, kPartLegB, kPartSkirt, kPartCape, kPartWhole };
constexpr int kInfluences = 8;  // bones a grid corner may follow
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
  uint8_t bone[kInfluences];
  float w[kInfluences];
  float sway = 0.0f;  // cloth: how far the corner trails behind the motion (fighter heights)
};
struct Part {
  Kind kind = kPartTorso;
  int nx = 0, ny = 0;
  std::vector<Vert> verts;  // (nx + 1) * (ny + 1)
  std::vector<int> cells;   // the cells with something drawn in them
};
struct Limb {  // bone lengths in fighter heights
  float l1 = 0, l2 = 0;
};

// The limbs are drawn as volumes (not pieces of the picture): widths and colours measured on the picture.
struct Ramp {  // the shadow, middle and light tones of a colour, as the picture paints them
  Color c[3] = {{90, 90, 90, 255}, {128, 128, 128, 255}, {170, 170, 170, 255}};
};
struct LimbSample {
  float t, w;  // along the chain (0..2: shoulder/hip -> elbow/knee -> hand/ankle), half width (fighter heights)
  Ramp r;
};
struct Shoe {
  float heel = -0.03f, toe = 0.06f, sole = 0.04f;  // relative to the ankle (fighter heights)
  Ramp r;
};
enum LimbId { kArmF, kArmB, kLegF, kLegB, kLimbs };
// A limb segment straightened in the atlas: x along the bone (the joint at x = ext, the next one at ext + len),
// y across it (the bone at y = half), the picture's shading taken out (the game lights it).
struct Strip {
  bool ok = false;
  float ox = 0, oy = 0, ext = 0, half = 0, len = 0;  // atlas px
};
// A rigid piece of the picture at the end of a limb (fist, shoe), turning about its joint.
struct Piece {
  bool ok = false;
  float ax = 0, ay = 0, w = 0, h = 0, x0 = 0, y0 = 0;  // atlas position, box in the picture
  Vec2 anchor;                                       // the joint it turns about (picture px)
};

struct Art {
  bool tried = false, ok = false, rigged = false;
  bool volumes = false;  // limbs measured: drawn as volumes
  std::vector<LimbSample> limb[kLimbs];
  float fistR[2] = {0.03f, 0.03f};  // F, B
  Ramp fistC[2];
  Shoe shoe[2];
  Strip strip[kLimbs][2];
  Piece fist[2], foot[2];  // F, B
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
  float atlasW = 1, atlasH = 1;
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
  return l;
}

// ---- Loading ----------------------------------------------------------------------------------------
// Inside a leg: how much a point follows the thigh, the shin and the foot (blended around the knee and ankle).
struct LegShare {
  float thigh, shin, foot;
};
LegShare legShare(const Art& A, bool f, Vec2 p) {
  const Joints& j = A.px;
  const Vec2 hp = f ? j.hipF : j.hipB, kn = f ? j.kneeF : j.kneeB, an = f ? j.ankleF : j.ankleB, ft = f ? j.footF : j.footB;
  const Vec2 ut = (kn - hp).norm(), us = (an - kn).norm(), uf = (ft - an).norm();
  Vec2 nk = (ut + us).norm(), na = (us + uf).norm();
  if (nk.len2() < 0.5f) nk = ut;
  if (na.len2() < 0.5f) na = us;
  const float wk = sstep(-A.blendKnee, A.blendKnee, dot(p - kn, nk));
  const float wa = sstep(-A.blendAnkle, A.blendAnkle, dot(p - an, na));
  return {1.0f - wk, wk * (1.0f - wa), wk * wa};
}

float segDist(Vec2 p, Vec2 a, Vec2 b) {
  const Vec2 d = b - a;
  const float t = clampf(dot(p - a, d) / std::max(1e-6f, d.len2()), 0.0f, 1.0f);
  return (p - (a + d * t)).len();
}

void weigh(const Art& A, Kind kind, Vert& v, float partTop, float partBottom) {
  const Joints& j = A.px;
  const Vec2 p = v.src;
  float w[kInfluences] = {1};
  uint8_t b[kInfluences] = {kTorso};
  int n = 1;
  auto add = [&](int bone, float wt) {
    if (wt <= 1e-4f || n >= kInfluences) return;
    b[n] = uint8_t(bone);
    w[n++] = wt;
  };
  switch (kind) {
    case kPartHead: {
      const Vec2 u = (j.head - j.neck).norm();
      const float bn = A.blendNeck;
      const float near = 1.0f - sstep(bn * 3.0f, bn * 6.0f, (p - j.neck).len());
      const float wt = sstep(bn, -bn, dot(p - j.neck, u)) * near;
      w[0] = wt;
      add(kHead, 1.0f - wt);
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
      w[0] = wt;
      add(f ? kUpF : kUpB, (1.0f - wt) * (1.0f - wl));
      add(f ? kLoF : kLoB, (1.0f - wt) * wl);
      break;
    }
    case kPartLegF: case kPartLegB: {
      const bool f = kind == kPartLegF;
      const Vec2 hp = f ? j.hipF : j.hipB, kn = f ? j.kneeF : j.kneeB;
      const float wt = sstep(A.blendHip, -A.blendHip, dot(p - hp, (kn - hp).norm()));
      const LegShare s = legShare(A, f, p);
      w[0] = wt;
      add(f ? kThF : kThB, (1.0f - wt) * s.thigh);
      add(f ? kShF : kShB, (1.0f - wt) * s.shin);
      add(f ? kFtF : kFtB, (1.0f - wt) * s.foot);
      break;
    }
    case kPartSkirt: {  // cloth from the waist: the hips hold it, the legs pull its lower part only half way —
      // a kick lifts the robe and the leg comes out from under it instead of stretching the cloth like a sheet
      const float hipY = (j.hipF.y + j.hipB.y) * 0.5f, thigh = (j.kneeF - j.hipF).len();
      const float wt = 0.5f + 0.5f * sstep(hipY + 0.45f * thigh, hipY - 0.05f * thigh, p.y);
      const float dF = std::min(segDist(p, j.hipF, j.kneeF), segDist(p, j.kneeF, j.ankleF));
      const float dB = std::min(segDist(p, j.hipB, j.kneeB), segDist(p, j.kneeB, j.ankleB));
      const float c = 0.06f * A.unit, iF = 1.0f / (dF * dF + c * c), iB = 1.0f / (dB * dB + c * c);
      const float sF = iF / (iF + iB);
      const LegShare lf = legShare(A, true, p), lb = legShare(A, false, p);
      w[0] = wt;
      add(kThF, (1.0f - wt) * sF * lf.thigh);
      add(kShF, (1.0f - wt) * sF * lf.shin);
      add(kFtF, (1.0f - wt) * sF * lf.foot);
      add(kThB, (1.0f - wt) * (1.0f - sF) * lb.thigh);
      add(kShB, (1.0f - wt) * (1.0f - sF) * lb.shin);
      add(kFtB, (1.0f - wt) * (1.0f - sF) * lb.foot);
      v.sway = 0.035f * clampf((p.y - hipY) / std::max(1.0f, A.ground - hipY), 0.0f, 1.0f);
      break;
    }
    case kPartCape:  // hangs from the shoulders with the torso, its hem trails behind
      v.sway = 0.08f * clampf((p.y - partTop) / std::max(1.0f, partBottom - partTop), 0.0f, 1.0f);
      break;
    default: break;
  }
  float sum = 0.0f;
  for (int i = 0; i < n; ++i) sum += w[i];
  for (int i = 0; i < kInfluences; ++i) {
    v.bone[i] = i < n ? b[i] : uint8_t(kTorso);
    v.w[i] = i < n && sum > 0.0f ? w[i] / sum : 0.0f;
  }
}

Color rgb(const int* c) {
  return {(unsigned char)std::clamp(c[0], 0, 255), (unsigned char)std::clamp(c[1], 0, 255),
          (unsigned char)std::clamp(c[2], 0, 255), 255};
}
Ramp ramp(const int* c) {
  Ramp r;
  for (int i = 0; i < 3; ++i) r.c[i] = rgb(c + 3 * i);
  return r;
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
    float x = 0, y = 0, fz[3] = {};
    int ci[9] = {};
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
    } else if (std::sscanf(line.c_str(), "limb %31s %f %f %d %d %d %d %d %d %d %d %d", name, &x, &y, &ci[0], &ci[1],
                           &ci[2], &ci[3], &ci[4], &ci[5], &ci[6], &ci[7], &ci[8]) == 12) {
      const std::string k = name;
      const int id = k == "armF" ? kArmF : k == "armB" ? kArmB : k == "legF" ? kLegF : k == "legB" ? kLegB : -1;
      if (id >= 0) A.limb[id].push_back({x, y, ramp(ci)});
    } else if (std::sscanf(line.c_str(), "fist %31s %f %d %d %d %d %d %d %d %d %d", name, &x, &ci[0], &ci[1], &ci[2],
                           &ci[3], &ci[4], &ci[5], &ci[6], &ci[7], &ci[8]) == 11) {
      const int f = name[0] == 'B' ? 1 : 0;
      A.fistR[f] = x;
      A.fistC[f] = ramp(ci);
    } else if (std::sscanf(line.c_str(), "shoe %31s %f %f %f %f %d %d %d %d %d %d %d %d %d", name, &x, &y, &fz[0],
                           &fz[1], &ci[0], &ci[1], &ci[2], &ci[3], &ci[4], &ci[5], &ci[6], &ci[7], &ci[8]) == 14) {
      Shoe& sh = A.shoe[name[0] == 'B' ? 1 : 0];
      sh.heel = x;  // picture px for now, made relative to the ankle once the joints are known
      sh.toe = y;
      sh.sole = fz[0];
      sh.r = ramp(ci);
    } else if (int k = 0; std::sscanf(line.c_str(), "strip %31s %d %f %f %f %f %f", name, &k, &x, &y, &fz[0], &fz[1],
                                      &fz[2]) == 7) {
      const std::string nm = name;
      const int id = nm == "armF" ? kArmF : nm == "armB" ? kArmB : nm == "legF" ? kLegF : nm == "legB" ? kLegB : -1;
      if (id >= 0 && (k == 0 || k == 1)) A.strip[id][k] = {true, x, y, fz[0], fz[1], fz[2]};
    } else if (float pv[8]; std::sscanf(line.c_str(), "piece %31s %f %f %f %f %f %f %f %f", name, &pv[0], &pv[1],
                                         &pv[2], &pv[3], &pv[4], &pv[5], &pv[6], &pv[7]) == 9) {
      const std::string nm = name;
      Piece pc{true, pv[0], pv[1], pv[2], pv[3], pv[4], pv[5], {pv[6], pv[7]}};
      if (nm == "fistF") A.fist[0] = pc;
      if (nm == "fistB") A.fist[1] = pc;
      if (nm == "shoeF") A.foot[0] = pc;
      if (nm == "shoeB") A.foot[1] = pc;
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
    A.volumes = true;
    for (int i = 0; i < kLimbs; ++i) {
      std::vector<LimbSample>& L = A.limb[i];
      if (L.size() < 4) A.volumes = false;
      // Flat bands of colour along the limb (pixel art): a sample close to the band's colour joins it.
      Ramp band = L.empty() ? Ramp{} : L[0].r;
      for (LimbSample& q : L) {
        q.w /= A.unit;
        const Color a = q.r.c[1], b = band.c[1];
        const float dr = float(a.r) - b.r, dg = float(a.g) - b.g, db = float(a.b) - b.b;
        if (dr * dr + dg * dg + db * db < 34.0f * 34.0f) q.r = band;
        else band = q.r;
      }
      // Widths: smoothed along the limb (a measure can catch a strap or a fold).
      for (size_t i = 1; i + 1 < L.size(); ++i) {
        const float m = std::max(std::min(L[i - 1].w, L[i + 1].w), std::min(L[i].w, std::max(L[i - 1].w, L[i + 1].w)));
        L[i].w = 0.5f * (L[i].w + m);
      }
    }
    for (int f = 0; f < 2; ++f) {
      A.fistR[f] /= A.unit;
      const Vec2 ankle = f ? A.px.ankleB : A.px.ankleF;
      Shoe& sh = A.shoe[f];
      const float sole = std::min(sh.sole, A.ground + 2.0f);
      sh.heel = (sh.heel - ankle.x) / A.unit;
      sh.toe = (sh.toe - ankle.x) / A.unit;
      sh.sole = std::max(0.02f, (sole - ankle.y) / A.unit);
    }
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
           : nm == "armB" ? kPartArmB : nm == "legF" ? kPartLegF : nm == "legB" ? kPartLegB
           : nm == "skirt" ? kPartSkirt : nm == "cape" ? kPartCape : kPartWhole;
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
        weigh(A, P.kind, v, float(pl.oy), float(pl.oy + pl.h));
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
  A.atlasW = float(AW);
  A.atlasH = float(AH);
  A.tex = LoadTextureFromImage(img);
  UnloadImage(img);
  if (A.tex.id == 0) return;
  GenTextureMipmaps(&A.tex);
  SetTextureFilter(A.tex, TEXTURE_FILTER_TRILINEAR);
  A.ok = true;
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

Art* get(Hero h) {
  if (size_t(h) >= g_art.size()) return nullptr;
  Art& A = g_art[size_t(h)];
  if (!A.tried) {
    A.tried = true;
    for (const Source& s : kSources)
      if (s.hero == h) {
        load(A, s);
      }
  }
  return A.ok ? &A : nullptr;
}

// Tint of the fighter: hit flash, afterimage, and the colour a transformation gives the body (the avatars around
// it — wings, Susanoo, the fox — are drawn apart). False when nothing tints it.
bool tintOf(const Pose& pose, Color flashColor, float flash, Color& tc, float& amount) {
  tc = flashColor;
  amount = clampf(flash, 0.0f, 1.0f);
  if (pose.ghost) amount = std::max(amount, 0.65f);
  Color formCol{};
  float formK = 0.0f;
  switch (pose.form) {
    case Form::KuramaAvatar: formCol = {255, 190, 60, 255}, formK = 0.35f; break;   // golden chakra cloak
    case Form::SpartanRage: formCol = {255, 110, 40, 255}, formK = 0.3f; break;     // red-hot
    case Form::DemonUnion: formCol = {22, 18, 26, 255}, formK = 0.4f; break;        // black demon markings
    case Form::VastoLorde: formCol = {240, 240, 234, 255}, formK = 0.55f; break;    // white hollow body
    case Form::RedDemon: formCol = {225, 40, 32, 255}, formK = 0.4f; break;         // red skin
    case Form::Segunda: formCol = {20, 26, 22, 255}, formK = 0.5f; break;           // black body
    case Form::Pantera: formCol = {214, 238, 255, 255}, formK = 0.35f; break;       // white armour
    case Form::LosLobos: formCol = {236, 232, 220, 255}, formK = 0.15f; break;      // fur-lined coat
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

// ---- Drawing ----------------------------------------------------------------------------------------
struct Mask {
  enum { None, Box, Disc, Slant } kind = None;
  Vec2 c;               // centre
  float a = 0, b = 0;   // Box: half width a, from y = c.y - b to the bottom fade (c.y + 0.3a .. c.y + a); Disc: radius a
  Rectangle r{};        // Slant: the box of a parallelogram leaning right by `a` (like ui::slanted), sharp edges
  float at(Vector2 p) const {
    if (kind == Box) {
      const float edge = a * 0.08f;
      return sstep(c.x - a, c.x - a + edge, p.x) * sstep(c.x + a, c.x + a - edge, p.x) *
             sstep(c.y - b, c.y - b + edge, p.y) * sstep(c.y + a, c.y + a * 0.3f, p.y);
    }
    if (kind == Disc) return sstep(a, a - 1.2f, (W(p) - c).len());
    if (kind == Slant) {
      const float fy = (p.y - r.y) / std::max(1.0f, r.height);
      const float l = r.x + a * (1.0f - fy), rt = r.x + r.width - a * fy;
      return sstep(l, l + 1.0f, p.x) * sstep(rt, rt - 1.0f, p.x) * sstep(r.y, r.y + 1.0f, p.y) *
             sstep(r.y + r.height, r.y + r.height - 1.0f, p.y);
    }
    return 1.0f;
  }
};

// Draws every part: `place` gives the screen position of a grid corner. `sub` splits the cells (fine masks).
template <class Place>
void drawParts(const Art& A, Place place, Color tint, const Mask& mask, int sub = 1, unsigned kinds = ~0u) {
  static std::vector<Vector2> pts;
  static std::vector<float> ks;
  rlSetTexture(A.tex.id);
  rlBegin(RL_QUADS);
  for (const Part& P : A.parts) {
    if (!(kinds & (1u << P.kind))) continue;
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
      // A cell inside the mask needs no split (the masks are convex: the whole cell is inside).
      const int n = mask.kind != Mask::None && ks[i00] >= 1.0f && ks[i10] >= 1.0f && ks[i01] >= 1.0f && ks[i11] >= 1.0f
                        ? 1 : sub;
      for (int sy = 0; sy < n; ++sy)
        for (int sx = 0; sx < n; ++sx) {
          const float fx[2] = {float(sx) / n, float(sx + 1) / n}, fy[2] = {float(sy) / n, float(sy + 1) / n};
          // corners in drawing order: top-left, bottom-left, bottom-right, top-right
          const int ox[4] = {0, 0, 1, 1}, oy[4] = {0, 1, 1, 0};
          for (int k = 0; k < 4; ++k) {
            const float a = fx[ox[k]], b = fy[oy[k]];
            const float w00 = (1 - a) * (1 - b), w10 = a * (1 - b), w01 = (1 - a) * b, w11 = a * b;
            const Vector2 p{pts[i00].x * w00 + pts[i10].x * w10 + pts[i01].x * w01 + pts[i11].x * w11,
                            pts[i00].y * w00 + pts[i10].y * w10 + pts[i01].y * w01 + pts[i11].y * w11};
            const float u = v00->u * w00 + v10->u * w10 + v01->u * w01 + v11->u * w11;
            const float v = v00->v * w00 + v10->v * w10 + v01->v * w01 + v11->v * w11;
            const float m = n > 1 && mask.kind != Mask::None ? mask.at(p)
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

namespace {
// ---- Limbs drawn as volumes ---------------------------------------------------------------------------
// Pixel-art cel shading under the key light (upper left and in front, like the pictures): every shape is split
// into a shadow, a middle and a lit tone by the way its surface faces the light on screen, frame after frame —
// a raised arm is lit like a raised arm, not like a turned piece of paper.
constexpr float kLx = -0.4f, kLy = -0.6f, kLz = 0.7f;
constexpr float kLitAbove = 0.78f, kShadowBelow = 0.36f;

Color toneOf(const Ramp& r, int k) { return r.c[std::clamp(k, 0, 2)]; }
// Tone of a surface whose normal is (nx, ny) on screen and nz towards the viewer.
int toneAt(float nx, float ny, float nz) {
  const float d = nx * kLx + ny * kLy + nz * kLz;
  return d > kLitAbove ? 2 : d < kShadowBelow ? 0 : 1;
}

struct Flat {  // flat-coloured triangles, tinted like the picture (hit flash, afterimage, transformation)
  Color tint = WHITE;
  float amount = 0.0f, alpha = 1.0f;
  Color fin(Color c) const {
    Color m = amount > 0.0f ? mixColor(c, tint, amount) : c;
    m.a = (unsigned char)(m.a * alpha);
    return m;
  }
  void tri(Vector2 a, Vector2 b, Vector2 c, Color k) const {
    const Color f = fin(k);
    rlColor4ub(f.r, f.g, f.b, f.a);
    rlVertex2f(a.x, a.y);
    rlVertex2f(b.x, b.y);
    rlVertex2f(c.x, c.y);
  }
  void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color k) const {
    tri(a, b, c, k);
    tri(a, c, d, k);
  }
  void disc(Vector2 c, float r, Color k, int n = 16) const {
    for (int i = 0; i < n; ++i) {
      const float a0 = 2.0f * kPi * i / n, a1 = 2.0f * kPi * (i + 1) / n;
      tri(c, {c.x + std::cos(a0) * r, c.y + std::sin(a0) * r}, {c.x + std::cos(a1) * r, c.y + std::sin(a1) * r}, k);
    }
  }
  // A cel-shaded ellipse (a ball): axes u, v on screen.
  void ball(Vector2 c, Vec2 u, Vec2 v, const Ramp& k, int n = 16) const {
    const float rings[3] = {0.0f, 0.62f, 1.0f};
    for (int ring = 0; ring < 2; ++ring)
      for (int i = 0; i < n; ++i) {
        const float a0 = 2.0f * kPi * i / n, a1 = 2.0f * kPi * (i + 1) / n, am = (a0 + a1) * 0.5f;
        const float r0 = rings[ring], r1 = rings[ring + 1], rm = (r0 + r1) * 0.5f;
        const Vec2 dir = (u * std::cos(am) + v * std::sin(am)).norm();
        const Color col = toneOf(k, toneAt(dir.x * rm, dir.y * rm, std::sqrt(std::max(0.0f, 1.0f - rm * rm))));
        auto at = [&](float r, float a) {
          const Vec2 p = u * (std::cos(a) * r) + v * (std::sin(a) * r);
          return Vector2{c.x + p.x, c.y + p.y};
        };
        if (ring == 0) tri(c, at(r1, a0), at(r1, a1), col);
        else quad(at(r0, a0), at(r1, a0), at(r1, a1), at(r0, a1), col);
      }
  }
};

// A limb segment from its strip of the picture: the silhouette of the measured widths, the picture's own pattern
// (bands, wraps, folds, a badge), lit by the game: each slice across the limb takes the shadow / middle / light
// tone the picture gives that colour, by the way it faces the light now. Round caps at both joints.
struct StripDraw {
  const Art& A;
  const Xf& X;
  float H, facing, unit;
  float u(float x) const { return x / A.atlasW; }
  float v(float y) const { return y / A.atlasH; }
  // Light multiplier of a slice (vertex colour): the picture's tone over its lit tone, per channel.
  static Color mul(const Ramp& r, int k) {
    const Color a = r.c[std::clamp(k, 0, 2)], b = r.c[2];
    auto ch = [](float x, float y) { return (unsigned char)clampf(y > 0.0f ? 255.0f * x / y : 255.0f, 0.0f, 255.0f); };
    return {ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b), 255};
  }
  void vert(Vector2 p, float sx, float sy, Color c, const Strip& S, float alpha) const {
    rlColor4ub(c.r, c.g, c.b, (unsigned char)(255.0f * alpha));
    rlTexCoord2f(u(S.ox + sx), v(S.oy + sy));
    rlVertex2f(p.x, p.y);
  }
  void segment(const Strip& S, const std::vector<LimbSample>& L, int sgm, Vec2 ja, Vec2 jb, float alpha) const {
    std::vector<const LimbSample*> q;
    for (const LimbSample& smp : L)
      if (int(smp.t) == sgm || (sgm == 1 && smp.t >= 2.0f)) q.push_back(&smp);
    if (q.empty() || !S.ok) return;
    const Vector2 Ap = X(ja), Bp = X(jb);
    Vec2 ds = W(Bp) - W(Ap);
    if (ds.len2() < 1e-6f) ds = {0.0f, 1.0f};
    ds = ds.norm();
    const Vec2 ns{-ds.y, ds.x};
    const int n = int(q.size());
    auto wpic = [&](int m) {  // half width at station m, picture px
      const float a = q[std::max(0, m - 1)]->w, b = q[std::min(n - 1, m)]->w;
      return (m == 0 ? b : m == n ? a : 0.5f * (a + b)) * unit;
    };
    const float toScreen = H / unit * X.scale;  // picture px -> screen
    constexpr int kCuts = 8;
    constexpr float kIn = 0.94f;  // stay inside the picture's own outline
    for (int m = 0; m < n; ++m) {
      const float t0 = float(m) / n, t1 = float(m + 1) / n, w0 = wpic(m), w1 = wpic(m + 1);
      const Vec2 p0 = lerpv(W(Ap), W(Bp), t0), p1 = lerpv(W(Ap), W(Bp), t1);
      const float x0 = S.ext + t0 * S.len, x1 = S.ext + t1 * S.len;
      for (int c = 0; c < kCuts; ++c) {
        const float a0 = -1.0f + 2.0f * c / kCuts, a1 = -1.0f + 2.0f * (c + 1) / kCuts, am = 0.5f * (a0 + a1);
        const Color k = mul(q[m]->r, toneAt(ns.x * am, ns.y * am, std::sqrt(std::max(0.0f, 1.0f - am * am))));
        auto P = [&](const Vec2& p, float w, float a) { return V(p + ns * (w * toScreen * a)); };
        const float y00 = S.half + facing * a0 * w0 * kIn, y01 = S.half + facing * a1 * w0 * kIn;
        const float y10 = S.half + facing * a0 * w1 * kIn, y11 = S.half + facing * a1 * w1 * kIn;
        vert(P(p0, w0, a0), x0, y00, k, S, alpha);
        vert(P(p1, w1, a0), x1, y10, k, S, alpha);
        vert(P(p1, w1, a1), x1, y11, k, S, alpha);
        vert(P(p0, w0, a1), x0, y01, k, S, alpha);
      }
    }
    // Caps: half discs past each joint, from the strip's own pixels there.
    for (int end = 0; end < 2; ++end) {
      const Vec2 c = end ? W(Bp) : W(Ap);
      const float w = wpic(end ? n : 0), xc = S.ext + (end ? S.len : 0.0f), dir = end ? 1.0f : -1.0f;
      const Ramp& rp = end ? q.back()->r : q.front()->r;
      constexpr int kFan = 8;
      for (int i = 0; i < kFan; ++i) {
        const float b0 = -kPi * 0.5f + kPi * i / kFan, b1 = -kPi * 0.5f + kPi * (i + 1) / kFan, bm = 0.5f * (b0 + b1);
        const Vec2 nm = ns * std::sin(bm) + ds * (dir * std::cos(bm));
        const Color k = mul(rp, toneAt(nm.x * 0.6f, nm.y * 0.6f, 0.8f));
        auto P = [&](float b) { return V(c + (ns * std::sin(b) + ds * (dir * std::cos(b))) * (w * toScreen)); };
        auto sx = [&](float b) { return xc + dir * std::cos(b) * w * kIn; };
        auto sy = [&](float b) { return S.half + facing * std::sin(b) * w * kIn; };
        vert(V(c), xc, S.half, k, S, alpha);  // a triangle as a quad (rlgl textures quads only)
        vert(P(b0), sx(b0), sy(b0), k, S, alpha);
        vert(P(b1), sx(b1), sy(b1), k, S, alpha);
        vert(P(b1), sx(b1), sy(b1), k, S, alpha);
      }
    }
  }
  // A rigid piece (fist, shoe): its box in the picture turned by `angle` about its joint, placed at `at`.
  void piece(const Piece& pc, Vec2 at, float angle, float alpha) const {
    if (!pc.ok) return;
    const float k = H / unit;
    Vec2 corner[4] = {{pc.x0, pc.y0}, {pc.x0 + pc.w, pc.y0}, {pc.x0 + pc.w, pc.y0 + pc.h}, {pc.x0, pc.y0 + pc.h}};
    const float uvx[4] = {pc.ax, pc.ax + pc.w, pc.ax + pc.w, pc.ax}, uvy[4] = {pc.ay, pc.ay, pc.ay + pc.h, pc.ay + pc.h};
    Vector2 q[4];
    for (int i = 0; i < 4; ++i) q[i] = X(at + rotate((corner[i] - pc.anchor) * k, angle));
    const unsigned char a = (unsigned char)(255.0f * alpha);
    for (int i = 0; i < 4; ++i) {
      rlColor4ub(255, 255, 255, a);
      rlTexCoord2f(uvx[i] / A.atlasW, uvy[i] / A.atlasH);
      rlVertex2f(q[i].x, q[i].y);
    }
  }
};

// One limb: two segments (shoulder/hip -> elbow/knee -> hand/ankle) with the measured widths and colour bands,
// round at the joints. `ink`: the outline pass (everything one art pixel wider, in the sprite's ink).
void limbVolume(const std::vector<LimbSample>& L, const Vec2 j[3], float H, const Xf& X, const Flat& F, float ow,
                bool ink) {
  const Color inkC = pix::kInk;
  for (int sgm = 0; sgm < 2; ++sgm) {
    std::vector<const LimbSample*> q;
    for (const LimbSample& smp : L)
      if (int(smp.t) == sgm || (sgm == 1 && smp.t >= 2.0f)) q.push_back(&smp);
    if (q.empty()) continue;
    const Vector2 A = X(j[sgm]), B = X(j[sgm + 1]);
    Vec2 ds = W(B) - W(A);
    if (ds.len2() < 1e-6f) ds = {0.0f, 1.0f};
    ds = ds.norm();
    const Vec2 ns{-ds.y, ds.x};
    const int n = int(q.size());
    auto width = [&](int m) {  // half width at station m (0..n), screen units
      const float a = q[std::max(0, m - 1)]->w, b = q[std::min(n - 1, m)]->w;
      return (m == 0 ? b : m == n ? a : 0.5f * (a + b)) * H * X.scale;
    };
    auto station = [&](int m) { return lerpv(W(A), W(B), float(m) / n); };
    for (int m = 0; m < n; ++m) {
      const Vec2 p0 = station(m), p1 = station(m + 1);
      const float w0 = width(m), w1 = width(m + 1);
      if (ink) {
        F.quad(V(p0 + ns * (w0 + ow)), V(p1 + ns * (w1 + ow)), V(p1 - ns * (w1 + ow)), V(p0 - ns * (w0 + ow)), inkC);
        continue;
      }
      constexpr int kCuts = 6;  // across the limb: a cylinder lit from the upper left
      for (int c = 0; c < kCuts; ++c) {
        const float u0 = -1.0f + 2.0f * c / kCuts, u1 = -1.0f + 2.0f * (c + 1) / kCuts, um = 0.5f * (u0 + u1);
        const Color col = toneOf(q[m]->r, toneAt(ns.x * um, ns.y * um, std::sqrt(std::max(0.0f, 1.0f - um * um))));
        F.quad(V(p0 + ns * (w0 * u0)), V(p1 + ns * (w1 * u0)), V(p1 + ns * (w1 * u1)), V(p0 + ns * (w0 * u1)), col);
      }
    }
    // Round joints: the start of the segment (shoulder / hip only for the first one) and its end.
    const float wA = width(0), wB = width(n);
    if (sgm == 1 || ink) {
      if (ink) F.disc(A, wA + ow, inkC);
      else F.ball(A, Vec2{wA, 0.0f}, Vec2{0.0f, wA}, q.front()->r);
    }
    if (ink) F.disc(B, wB + ow, inkC);
  }
  // The elbow / knee on top of both segments.
  if (!ink && !L.empty()) {
    const Vector2 K = X(j[1]);
    const LimbSample* best = &L[0];
    for (const LimbSample& smp : L)
      if (std::fabs(smp.t - 1.0f) < std::fabs(best->t - 1.0f)) best = &smp;
    const float wk = best->w * H * X.scale;
    F.ball(K, Vec2{wk, 0.0f}, Vec2{0.0f, wk}, best->r);
  }
}

// A fist at the end of the forearm, oriented along it.
void fistVolume(Vec2 elbow, Vec2 hand, float r, const Ramp& c, const Xf& X, const Flat& F, float ow, bool ink) {
  const Vector2 C = X(hand);
  Vec2 d = W(X(hand)) - W(X(elbow));
  d = d.len2() > 1e-6f ? d.norm() : Vec2{1.0f, 0.0f};
  const Vec2 n{-d.y, d.x};
  const float rs = r * X.scale;
  if (ink) {
    Ramp inkR;
    inkR.c[0] = inkR.c[1] = inkR.c[2] = pix::kInk;
    F.ball(C, d * (rs * 1.08f + ow), n * (rs * 0.92f + ow), inkR);
  }
  else F.ball(C, d * (rs * 1.08f), n * (rs * 0.92f), c);
}

// A shoe under the ankle, tilted with the foot.
void shoeVolume(const Shoe& sh, Vec2 ankle, float angle, float H, const Xf& X, const Flat& F, float ow, bool ink) {
  const float Hx = sh.heel * H, Tx = sh.toe * H, S = sh.sole * H, Lf = Tx - Hx;
  const Vec2 pts[6] = {{Hx, S}, {Tx, S}, {Tx, S * 0.5f}, {Hx + 0.62f * Lf, S * 0.12f}, {Hx + 0.36f * Lf, -S * 0.3f},
                       {Hx + 0.06f * Lf, -S * 0.3f}};
  Vector2 q[6];
  Vec2 c{};
  for (int i = 0; i < 6; ++i) {
    q[i] = X(ankle + rotate(pts[i], angle));
    c += W(q[i]) * (1.0f / 6.0f);
  }
  if (ink) {
    for (int i = 0; i < 6; ++i) {  // edges, one art pixel thick around the shoe
      const Vec2 a = W(q[i]), b = W(q[(i + 1) % 6]);
      Vec2 e = b - a;
      if (e.len2() < 1e-6f) continue;
      e = e.norm();
      const Vec2 nn{-e.y, e.x};
      F.quad(V(a + nn * ow), V(b + nn * ow), V(b - nn * ow), V(a - nn * ow), pix::kInk);
      F.disc(V(a), ow, pix::kInk, 8);
    }
    return;
  }
  for (int i = 1; i < 5; ++i) F.tri(q[0], q[i], q[i + 1], sh.r.c[1]);
  // The sole in shadow, the top of the toe catching the light.
  auto at = [&](Vec2 p) { return X(ankle + rotate(p, angle)); };
  F.quad(at({Hx, S}), at({Tx, S}), at({Tx, S * 0.78f}), at({Hx, S * 0.78f}), toneOf(sh.r, 0));
  F.quad(at({Hx + 0.62f * Lf, S * 0.16f}), at({Tx, S * 0.52f}), at({Tx - 0.12f * Lf, S * 0.62f}),
         at({Hx + 0.56f * Lf, S * 0.3f}), toneOf(sh.r, 2));
  (void)c;
}
}  // namespace

bool rigged(Hero h) {
  const Art* A = get(h);
  return A && A->rigged;
}

bool portrait(Hero h) { return get(h) != nullptr; }

namespace {
// An illustration bends less than a cartoon: its limbs fold over the picture (the cloth looks crushed) when a knee
// or an elbow closes too far. Crouches are shallower, knees and elbows keep a minimum opening.
constexpr float kCrouch = 0.4f;
constexpr float kMinLeg = 0.68f, kMinArm = 0.52f;  // hip-to-foot / shoulder-to-hand, fraction of the full limb

// Torso of a pose on the picture: hips (lowered by `drop` more), neck, head, shoulders and hip joints.
void torso(const Art& A, float H, const Pose& pose, float drop, Rig& r) {
  const Joints& n = A.n;
  auto L = [&](Vec2 v) { return v * H; };
  r.hips = {0.0f, n.hips.y * H + pose.crouch * kCrouch * 0.14f * H + drop};
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
}

// Leg length on the picture, hip to sole.
float legReach(const Limb& l, Vec2 nAnkle, Vec2 nFoot) { return l.l1 + l.l2 + (nAnkle - nFoot).len(); }
}  // namespace

bool solve(const CharacterDef& d, const Pose& pose, const Rig& P, Rig& r) {
  const Art* A = get(d.hero);
  if (!A || !A->rigged) return false;
  const float H = d.height, ground = 0.5f * H;
  const Joints& n = A->n;
  auto L = [&](Vec2 v) { return v * H; };

  // Feet: the drawn foot's direction from its hip and its extension, on the picture's longer legs. A foot on the
  // ground stays on the ground (its distance from the hip scaled the same way).
  struct LegGoal {
    Vec2 v;          // drawn hip -> foot, scaled to the picture's leg
    float g = 0.0f;  // 1: the foot is on the ground
  };
  auto legGoal = [&](const Limb& l, Vec2 nAnkle, Vec2 nFoot, Vec2 phip, Vec2 pkn, Vec2 pft) {
    LegGoal G;
    const float lp = (pkn - phip).len() + (pft - pkn).len(), la = legReach(l, nAnkle, nFoot) * H;
    const Vec2 v = pft - phip;
    const float e = lp > 1e-4f ? clampf(v.len() / lp, kMinLeg, 1.0f) : 1.0f;
    const Vec2 rel = (v.len2() > 1e-8f ? v.norm() : Vec2{0.0f, 1.0f}) * (e * la);
    G.g = smoothstep((pft.y - (ground - 0.06f * H)) / (0.05f * H));
    G.v = {lerpf(rel.x, v.x * (la / std::max(lp, 1e-4f)), G.g), rel.y};
    return G;
  };
  const LegGoal gF = legGoal(A->legF, n.ankleF, n.footF, P.hipF, P.kneeF, P.footF);
  const LegGoal gB = legGoal(A->legB, n.ankleB, n.footB, P.hipB, P.kneeB, P.footB);

  // The hips come down when a foot on the ground could not reach it (a wide stance, a long stride).
  torso(*A, H, pose, 0.0f, r);
  float drop = 0.0f;
  auto need = [&](const LegGoal& G, const Limb& l, Vec2 nAnkle, Vec2 nFoot, Vec2 hip) {
    if (G.g <= 0.0f) return;
    const float reach = legReach(l, nAnkle, nFoot) * H * 0.985f;
    const float dx = std::min(std::fabs(G.v.x), reach * 0.95f);
    const float lowest = ground - std::sqrt(reach * reach - dx * dx);  // highest the hip may be
    drop = std::max(drop, (lowest - hip.y) * G.g);
  };
  need(gF, A->legF, n.ankleF, n.footF, r.hipF);
  need(gB, A->legB, n.ankleB, n.footB, r.hipB);
  if (drop > 0.0f) torso(*A, H, pose, drop, r);

  // Arms: the drawn arm's direction from the shoulder and its extension, on the picture's arm. Elbows bend down and
  // back, never forward, and never fold completely.
  auto arm = [&](const Limb& l, Vec2 sh, Vec2 psh, Vec2 pel, Vec2 phd, Vec2 nSh, Vec2 nHd, Vec2& elbow, Vec2& hand) {
    const float lp = (pel - psh).len() + (phd - pel).len(), la = (l.l1 + l.l2) * H;
    const Vec2 v = phd - psh;
    const float e = lp > 1e-4f ? clampf(v.len() / lp, kMinArm, 1.0f) : 1.0f;
    const Vec2 u = v.len2() > 1e-8f ? v.norm() : (nHd - nSh).norm();
    elbow = ik(sh, sh + u * (e * la), l.l1 * H, l.l2 * H, 1.0f, hand);
  };
  arm(A->armF, r.shF, P.shF, P.elbowF, P.handF, n.shF, n.handF, r.elbowF, r.handF);
  arm(A->armB, r.shB, P.shB, P.elbowB, P.handB, n.shB, n.handB, r.elbowB, r.handB);

  // Legs: knees bend forward; the foot stays flat on the ground, and follows the shin when it swings far.
  auto leg = [&](const Limb& l, const LegGoal& G, Vec2 hip, Vec2 nAnkle, Vec2 nFoot, float phi0, Vec2& knee,
                 Vec2& foot) {
    Vec2 target = hip + G.v;
    if (G.g > 0.0f) target.y = lerpf(target.y, ground, G.g);
    float th = 0.0f;
    Vec2 ankle;
    for (int it = 0; it < 2; ++it) {
      knee = ik(hip, target + rotate(L(nAnkle - nFoot), th), l.l1 * H, l.l2 * H, -1.0f, ankle);
      th = -(footPhi(target - knee) - phi0);
    }
    foot = ankle + rotate(L(nFoot - nAnkle), th);
  };
  leg(A->legF, gF, r.hipF, n.ankleF, n.footF, A->footPhiF, r.kneeF, r.footF);
  leg(A->legB, gB, r.hipB, n.ankleB, n.footB, A->footPhiB, r.kneeB, r.footB);
  return true;
}

bool stance(const CharacterDef& d, Pose& p, float bob) {
  const Art* A = get(d.hero);
  if (!A || !A->rigged) return false;
  p.lean -= 0.06f;  // the picture stands as it was drawn (the rest pose leans 0.06 forward)
  // Targets on the drawn build that solve() maps back onto the picture's own pose.
  const float H = d.height;
  const Joints& n = A->n;
  const Rig P = buildRig(d, p);
  auto handGoal = [&](const Limb& l, Vec2 psh, Vec2 pel, Vec2 phd, Vec2 nSh, Vec2 nHd) {
    const float lp = (pel - psh).len() + (phd - pel).len(), la = (l.l1 + l.l2) * H;
    return psh + (nHd - nSh) * (H * lp / la) + Vec2{0.0f, bob};
  };
  auto footGoal = [&](const Limb& l, Vec2 phip, Vec2 pkn, Vec2 pft, Vec2 nHip, Vec2 nAnkle, Vec2 nFoot) {
    const float lp = (pkn - phip).len() + (pft - pkn).len(), la = legReach(l, nAnkle, nFoot) * H;
    return Vec2{phip.x + (nFoot.x - nHip.x) * H * lp / la, 0.5f * H};
  };
  const Vec2 hF = handGoal(A->armF, P.shF, P.elbowF, P.handF, n.shF, n.handF);
  const Vec2 hB = handGoal(A->armB, P.shB, P.elbowB, P.handB, n.shB, n.handB);
  const Vec2 fF = footGoal(A->legF, P.hipF, P.kneeF, P.footF, n.hipF, n.ankleF, n.footF);
  const Vec2 fB = footGoal(A->legB, P.hipB, P.kneeB, P.footB, n.hipB, n.ankleB, n.footB);
  p.handF = hF;
  p.handB = hB;
  p.footF = fF;
  p.footB = fB;
  return true;
}

void drawFighter(const CharacterDef& d, const Pose& pose, const Rig& r, const Xf& X, float alpha, Color flashColor,
                 float flash, Layers layers) {
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

  Color tc;
  float amount = 0.0f;
  const bool shaded = tintOf(pose, flashColor, flash, tc, amount);
  const Vec2 trail = lerpv(pose.flow, pose.flowTip, 0.6f);
  auto place = [&](const Vert& v) {
    Vec2 p{};
    for (int i = 0; i < kInfluences; ++i) {
      if (v.w[i] <= 0.0f) continue;
      const BoneXf& b = T[v.bone[i]];
      const Vec2 dd = (v.n - b.bind) * H;
      p += Vec2{b.c * dd.x - b.s * dd.y + b.now.x, b.s * dd.x + b.c * dd.y + b.now.y} * v.w[i];
    }
    if (v.sway > 0.0f) p += trail * (v.sway * H);  // cloth trailing behind the motion
    return X(p);
  };
  auto picture = [&](unsigned kinds) {
    if (shaded) beginTint(tc, amount);
    drawParts(*A, place, withAlpha(WHITE, alpha), Mask{}, 1, kinds);
    if (shaded) EndShaderMode();
  };
  if (!A->volumes) {  // not measured: the picture's own limbs
    picture(layers == Layers::BelowWeapon ? ~(1u << kPartArmF) : layers == Layers::WeaponArm ? (1u << kPartArmF) : ~0u);
    return;
  }
  // The limbs are volumes drawn for this pose; the head, the torso and the cloth come from the picture.
  Flat F;
  F.tint = tc;
  F.amount = amount;
  F.alpha = alpha;
  const float art = pix::artUnit();
  const float ow = art > 0.0f ? art : 1.2f;
  const Vec2 ankleF = T[kFtF].now, ankleB = T[kFtB].now;
  const float footAF = std::atan2(T[kFtF].s, T[kFtF].c), footAB = std::atan2(T[kFtB].s, T[kFtB].c);
  const StripDraw SD{*A, X, H, float(X.facing), A->unit};
  const bool strips = A->strip[kArmF][0].ok;
  auto limbs = [&](int id) {
    const bool arm = id == kArmF || id == kArmB, front = id == kArmF || id == kLegF;
    const Vec2 j[3] = {arm ? (front ? r.shF : r.shB) : (front ? r.hipF : r.hipB),
                       arm ? (front ? r.elbowF : r.elbowB) : (front ? r.kneeF : r.kneeB),
                       arm ? (front ? r.handF : r.handB) : (front ? ankleF : ankleB)};
    // The outline, one art pixel around the limb. (rlgl: set the texture, then begin; a textured draw is in quads.)
    rlSetTexture(rlGetTextureIdDefault());
    rlBegin(RL_TRIANGLES);
    limbVolume(A->limb[id], j, H, X, F, ow, true);
    if (!strips) {
      limbVolume(A->limb[id], j, H, X, F, ow, false);
      if (arm) fistVolume(j[1], j[2], A->fistR[front ? 0 : 1] * H, A->fistC[front ? 0 : 1], X, F, ow, false);
      else shoeVolume(A->shoe[front ? 0 : 1], j[2], front ? footAF : footAB, H, X, F, ow, false);
    }
    rlEnd();
    rlSetTexture(0);
    if (!strips) return;
    // The limb itself, from the picture, lit by the game; then the fist or the shoe.
    if (shaded) beginTint(tc, amount);
    rlSetTexture(A->tex.id);
    rlBegin(RL_QUADS);
    SD.segment(A->strip[id][0], A->limb[id], 0, j[0], j[1], alpha);
    SD.segment(A->strip[id][1], A->limb[id], 1, j[1], j[2], alpha);
    if (arm) {
      const Vec2 nb = front ? n.handF - n.elbowF : n.handB - n.elbowB;
      const float ang = angleOf(j[2] - j[1]) - angleOf(nb);
      SD.piece(A->fist[front ? 0 : 1], j[2], ang, alpha);
    } else {
      SD.piece(A->foot[front ? 0 : 1], j[2], front ? footAF : footAB, alpha);
    }
    rlEnd();
    rlSetTexture(0);
    if (shaded) EndShaderMode();
  };
  if (layers != Layers::WeaponArm) {
    picture(1u << kPartCape);
    limbs(kLegB);
    limbs(kLegF);
    limbs(kArmB);
    picture(1u << kPartTorso);
    picture(1u << kPartSkirt);
    picture(1u << kPartHead);
  }
  if (layers != Layers::BelowWeapon) limbs(kArmF);
}

namespace {
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
      "victory"};
  static const char* const kMoves[MOVE_COUNT] = {
      "jab", "jab2", "jab3", "ftilt", "utilt", "dtilt", "dashattack", "fsmash", "usmash", "dsmash", "nair", "fair",
      "bair", "uair", "dair", "nspecial", "sspecial", "uspecial", "dspecial", "grab", "pummel", "fthrow", "bthrow",
      "uthrow", "dthrow", "ledgeattack", "getupattack", "extra1", "extra2", "extra3", "ultimate"};
  static std::array<std::string, kSheetCount> names = [] {
    std::array<std::string, kSheetCount> n;
    for (int i = 0; i < kSheetMove0; ++i) n[i] = kStates[i];
    for (int m = 0; m < MOVE_COUNT; ++m) {
      n[kSheetMove0 + m] = kMoves[m];
      n[kSheetWeapon0 + m] = std::string("w0_") + kMoves[m];
      n[kSheetWeapon1 + m] = std::string("w1_") + kMoves[m];
    }
    return n;
  }();
  return anim >= 0 && anim < kSheetCount ? names[anim].c_str() : "";
}

// The fighter's clips, loaded the first time they are needed (the menus only need the pictures).
const Art* clipsOf(Hero h) {
  Art* A = get(h);
  if (A && !A->clipsTried) {
    A->clipsTried = true;
    for (const Source& s : kSources)
      if (s.hero == h) loadSheets(*A, s.name);
  }
  return A;
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
  const std::string key = std::string(heroKey(d.hero)) + "_" + sheetName(sheet);
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

bool drawBustIn(Hero h, Vec2 center, float size, Rectangle box, float slant, float alpha) {
  const Art* A = get(h);
  if (!A) return false;
  const float headR = std::max(8.0f, A->px.head.y - A->top);
  Mask m;
  m.kind = Mask::Slant;
  m.r = box;
  m.a = slant;
  drawStill(*A, A->px.head, center + Vec2{size * 0.02f, -size * 0.12f}, size * 0.2f / headR, 1, alpha, m, 2);
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
