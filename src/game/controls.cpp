#include "game/controls.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>

#include "game/settings.hpp"
#include "raylib.h"

#ifdef CLASH_HAS_GLFW  // our own raylib build: its GLFW can be called directly
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

extern "C" {
extern const unsigned char gamecontrollerdb_txt[];  // assets/gamecontrollerdb.txt, embedded at build time
extern const unsigned int gamecontrollerdb_txt_size;
}

namespace {
bool anyDown(std::initializer_list<int> keys) {
  for (int k : keys)
    if (IsKeyDown(k)) return true;
  return false;
}
bool anyPressed(std::initializer_list<int> keys) {
  for (int k : keys)
    if (IsKeyPressed(k)) return true;
  return false;
}
std::string lower(std::string s) {
  for (char& c : s)
    if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
  return s;
}
bool has(const std::string& s, const char* part) { return s.find(part) != std::string::npos; }

// ---- Joysticks, as the system reports them --------------------------------------
constexpr int kMaxJoy = 16;
struct Joy {
  bool present = false;
  bool mapped = false;  // known gamepad (SDL database): standard layout
  std::string name, guid, mapName;
  int buttonCount = 0, axisCount = 0;
  bool btn[kPadButtonCount] = {};
  float axis[6] = {0, 0, 0, 0, 0, 0};  // LX LY RX RY, then the triggers LT RT (-1 = released)
};

#ifdef CLASH_HAS_GLFW
// Unmapped joysticks: rest value of each axis, measured when the joystick appears.
float g_rest[kMaxJoy][8];
bool g_restKnown[kMaxJoy];

// Generic layout for joysticks the database does not know (rest: axis values when it appeared).
void rawLayout(Joy& j, const float* ax, int na, const unsigned char* bt, int nb, const unsigned char* hats, int nh,
               const float* rest) {
  j.buttonCount = nb;
  j.axisCount = na;
  na = std::min(na, 8);
  // Faces 0-3, shoulders 4-5, then triggers 6-7 and select/start 8-9 (select/start 6-7 on small pads).
  static const int kBig[10] = {kPadA, kPadB, kPadX, kPadY, kPadLB, kPadRB, kPadLT, kPadRT, kPadBack, kPadStart};
  static const int kSmall[8] = {kPadA, kPadB, kPadX, kPadY, kPadLB, kPadRB, kPadBack, kPadStart};
  const int* layout = nb >= 10 ? kBig : kSmall;
  for (int i = 0; i < std::min(nb, nb >= 10 ? 10 : 8); ++i) j.btn[layout[i]] = bt[i] != 0;
  if (nh > 0) {
    j.btn[kPadUp] = hats[0] & GLFW_HAT_UP;
    j.btn[kPadDown] = hats[0] & GLFW_HAT_DOWN;
    j.btn[kPadLeft] = hats[0] & GLFW_HAT_LEFT;
    j.btn[kPadRight] = hats[0] & GLFW_HAT_RIGHT;
  }
  // Axes 0-1: left stick. After them, centred axes are the right stick and axes resting at
  // one end are triggers (read as a stick, a trigger would push it all the time).
  int sticks = 0, triggers = 0;
  for (int i = 0; i < na; ++i) {
    const bool centred = std::fabs(rest[i]) < 0.5f;
    if (i < 2) j.axis[i] = centred ? ax[i] : 0.0f;
    else if (centred && sticks < 2) j.axis[2 + sticks++] = ax[i];
    else if (!centred && triggers < 2) j.axis[4 + triggers++] = rest[i] > 0 ? -ax[i] : ax[i];
  }
}

void readRaw(int jid, Joy& j) {
  int na = 0, nb = 0, nh = 0;
  const float* ax = glfwGetJoystickAxes(jid, &na);
  const unsigned char* bt = glfwGetJoystickButtons(jid, &nb);
  const unsigned char* hats = glfwGetJoystickHats(jid, &nh);
  if (!ax) na = 0;
  if (!hats) nh = 0;
  nb = bt ? std::max(0, nb - 4 * nh) : 0;  // GLFW appends every hat as 4 more buttons
  if (!g_restKnown[jid]) {
    for (int i = 0; i < std::min(na, 8); ++i) g_rest[jid][i] = ax[i];
    g_restKnown[jid] = true;
  }
  rawLayout(j, ax, na, bt, nb, hats, nh, g_rest[jid]);
}

void pollJoysticks(Joy* joys) {
  // Our buttons -> GLFW gamepad buttons (-1: the triggers are axes).
  static const int kMap[kPadButtonCount] = {
      GLFW_GAMEPAD_BUTTON_A, GLFW_GAMEPAD_BUTTON_B, GLFW_GAMEPAD_BUTTON_X, GLFW_GAMEPAD_BUTTON_Y,
      GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, -1, -1,
      GLFW_GAMEPAD_BUTTON_BACK, GLFW_GAMEPAD_BUTTON_START,
      GLFW_GAMEPAD_BUTTON_LEFT_THUMB, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
      GLFW_GAMEPAD_BUTTON_DPAD_UP, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
      GLFW_GAMEPAD_BUTTON_DPAD_RIGHT};
  for (int jid = 0; jid < kMaxJoy; ++jid) {
    Joy& j = joys[jid];
    j.present = glfwJoystickPresent(jid);
    if (!j.present) {
      g_restKnown[jid] = false;
      continue;
    }
    const char* s = glfwGetJoystickName(jid);
    j.name = s ? s : "";
    s = glfwGetJoystickGUID(jid);
    j.guid = s ? s : "";
    GLFWgamepadstate st;
    if (glfwJoystickIsGamepad(jid) && glfwGetGamepadState(jid, &st)) {
      j.mapped = true;
      s = glfwGetGamepadName(jid);
      j.mapName = s ? s : "";
      for (int b = 0; b < kPadButtonCount; ++b) j.btn[b] = kMap[b] >= 0 && st.buttons[kMap[b]] == GLFW_PRESS;
      for (int a = 0; a < 6; ++a) j.axis[a] = st.axes[a];  // same order as ours
    } else {
      readRaw(jid, j);
    }
  }
}
#else
// raylib only: its gamepads 0-3, whatever they are (unmapped ones just stay silent).
void pollJoysticks(Joy* joys) {
  static const int kMap[kPadButtonCount] = {
      GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
      GAMEPAD_BUTTON_RIGHT_FACE_UP, GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, -1, -1,
      GAMEPAD_BUTTON_MIDDLE_LEFT, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_LEFT_THUMB,
      GAMEPAD_BUTTON_RIGHT_THUMB, GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_DOWN,
      GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT};
  for (int g = 0; g < 4; ++g) {
    Joy& j = joys[g];
    j.present = IsGamepadAvailable(g);
    if (!j.present) continue;
    const char* s = GetGamepadName(g);
    j.name = s ? s : "";
    j.mapped = true;
    for (int b = 0; b < kPadButtonCount; ++b) j.btn[b] = kMap[b] >= 0 && IsGamepadButtonDown(g, kMap[b]);
    for (int a = 0; a < 6; ++a) j.axis[a] = GetGamepadAxisMovement(g, a);
  }
}
#endif

// ---- Logical pads -------------------------------------------------------------------
struct Pad {
  int joy = -1;                    // joystick id, -1 = not connected
  bool used = false;               // this number was already given to a device
  std::string guid, name;          // that device: plugged back, it gets its number back
  PadFamily family = PadFamily::Xbox;
  std::string model;
  bool down[kPadButtonCount] = {}, was[kPadButtonCount] = {};
  Vec2 stick, cstick;
  bool trigReady[2] = {false, false};  // trigger seen released: guards against triggers stuck "pressed"
  bool trig[2] = {false, false};
  bool digitalTrig = false;  // LT/RT seen as buttons
  bool menuDir[4] = {false, false, false, false};
};
Pad g_pads[kMaxPads];
bool g_activated[kMaxJoy];  // unmapped joysticks only count once one of their buttons was pressed
bool g_started = false;
bool g_padMode = false, g_forced = false;
PadFamily g_family = PadFamily::Xbox;
std::string g_notice;

// Not gamepads, although Linux lists them as joysticks (PlayStation touchpads and motion sensors...).
bool looksLikePad(const Joy& j) {
  static const char* kNot[] = {"touchpad", "motion sensor", "accelerometer", " imu", "mouse", "keyboard",
                               "consumer control", "system control", "trackpad", "wacom", "stylus", "touchscreen"};
  const std::string n = lower(j.name);
  for (const char* k : kNot)
    if (has(n, k)) return false;
  return j.mapped || (j.buttonCount >= 4 && j.axisCount >= 2);
}

PadFamily familyOf(const Joy& j) {
  if (j.guid.size() == 32) {  // vendor id in the SDL GUID (or GLFW's "xinput" one on Windows)
    if (j.guid.compare(0, 12, "78696e707574") == 0) return PadFamily::Xbox;
    const std::string vendor = j.guid.substr(8, 4);
    if (vendor == "4c05") return PadFamily::PlayStation;
    if (vendor == "7e05") return PadFamily::Nintendo;
    if (vendor == "5e04") return PadFamily::Xbox;
  }
  const std::string n = lower(j.name + " " + j.mapName);
  if (has(n, "xbox") || has(n, "x-box") || has(n, "xinput")) return PadFamily::Xbox;
  for (const char* k : {"sony", "playstation", "ps3", "ps4", "ps5", "dualshock", "dualsense", "wireless controller"})
    if (has(n, k)) return PadFamily::PlayStation;
  for (const char* k : {"nintendo", "switch", "pro controller", "joy-con", "joycon"})
    if (has(n, k)) return PadFamily::Nintendo;
  return PadFamily::Xbox;
}

std::string modelOf(const Joy& j, PadFamily f) {
  const std::string n = lower(j.name + " " + j.mapName);
  if (has(n, "dualsense edge")) return "DualSense Edge";
  if (has(n, "dualsense") || has(n, "ps5")) return "DualSense";
  if (has(n, "dualshock 4") || has(n, "ps4")) return "DualShock 4";
  if (has(n, "dualshock 3") || has(n, "ps3") || has(n, "playstation(r)3")) return "DualShock 3";
  if (has(n, "360")) return "Manette Xbox 360";
  if (has(n, "xbox one")) return "Manette Xbox One";
  if (has(n, "xbox series")) return "Manette Xbox Series";
  if (has(n, "joy-con") || has(n, "joycon")) return "Joy-Con";
  if (has(n, "pro controller")) return "Manette Pro Switch";
  if (has(n, "8bitdo")) return "Manette 8BitDo";
  if (f == PadFamily::PlayStation && has(n, "wireless controller")) return "DualShock 4";  // its Bluetooth name
  if (has(n, "xbox") || has(n, "xinput")) return "Manette Xbox";
  const std::string& m = j.mapName.empty() ? j.name : j.mapName;
  if (!m.empty() && m.size() <= 24) return m;
  return f == PadFamily::PlayStation ? "Manette PlayStation" : f == PadFamily::Nintendo ? "Manette Nintendo" : "Manette";
}

// Free number for a new device: its old one, else one never given, else any free one.
int pickSlot(const Joy& j) {
  for (int s = 0; s < kMaxPads; ++s)
    if (g_pads[s].joy < 0 && g_pads[s].used && g_pads[s].guid == j.guid && g_pads[s].name == j.name) return s;
  for (int s = 0; s < kMaxPads; ++s)
    if (g_pads[s].joy < 0 && !g_pads[s].used) return s;
  for (int s = 0; s < kMaxPads; ++s)
    if (g_pads[s].joy < 0) return s;
  return -1;
}

// Radial dead zone, rescaled so that the stick still goes smoothly from 0 to 1 (and reaches 1
// on pads whose sticks never quite get there).
Vec2 deadzone(float x, float y, float inner, float outer = 0.92f) {
  const float m = std::sqrt(x * x + y * y);
  if (m <= inner) return {0, 0};
  const float k = std::min(1.0f, (m - inner) / (outer - inner)) / m;
  return {x * k, y * k};
}

void applyJoysticks(const Joy* joys) {
  for (int s = 0; s < kMaxPads; ++s) {  // unplugged
    Pad& p = g_pads[s];
    if (p.joy < 0) continue;
    const Joy& j = joys[p.joy];
    if (j.present && j.guid == p.guid && j.name == p.name) continue;
    p.joy = -1;
    std::fill(p.down, p.down + kPadButtonCount, false);
    std::fill(p.was, p.was + kPadButtonCount, false);
    p.stick = p.cstick = {0, 0};
    g_notice = "MANETTE " + std::to_string(s + 1) + " déconnectée";
  }
  for (int jid = 0; jid < kMaxJoy; ++jid) {  // plugged in
    const Joy& j = joys[jid];
    if (!j.present) {
      g_activated[jid] = false;
      continue;
    }
    for (bool b : j.btn) g_activated[jid] = g_activated[jid] || b;
    if (!looksLikePad(j) || (!j.mapped && !g_activated[jid])) continue;
    bool owned = false;
    for (const Pad& p : g_pads) owned = owned || p.joy == jid;
    const int s = owned ? -1 : pickSlot(j);
    if (s < 0) continue;
    Pad& p = g_pads[s];
    p = Pad();
    p.joy = jid;
    p.used = true;
    p.guid = j.guid;
    p.name = j.name;
    p.family = familyOf(j);
    p.model = modelOf(j, p.family);
    if (g_started) g_notice = "MANETTE " + std::to_string(s + 1) + " connectée : " + p.model;
    if (!g_forced) {
      g_padMode = true;
      g_family = p.family;
    }
  }
  if (g_padMode && !g_forced)  // a key pressed: back to keyboard prompts
    for (int k = KEY_SPACE; k <= KEY_KB_MENU; ++k)
      if (IsKeyPressed(k)) {
        g_padMode = false;
        break;
      }
  for (Pad& p : g_pads) {  // state
    if (p.joy < 0) continue;
    const Joy& j = joys[p.joy];
    std::copy(p.down, p.down + kPadButtonCount, p.was);
    std::copy(j.btn, j.btn + kPadButtonCount, p.down);
    p.digitalTrig = p.digitalTrig || p.down[kPadLT] || p.down[kPadRT] || (!j.mapped && j.buttonCount >= 10);
    for (int t = 0; t < 2; ++t) {  // analog triggers: 0..1 press, with hysteresis
      const float v = j.axis[4 + t];
      if (v < -0.5f) p.trigReady[t] = true;
      p.trig[t] = p.trigReady[t] && (v + 1.0f) * 0.5f > (p.trig[t] ? 0.25f : 0.35f);
      p.down[kPadLT + t] = p.down[kPadLT + t] || p.trig[t];
    }
    const Vec2 before = p.stick;
    p.stick = deadzone(j.axis[0], j.axis[1], 0.22f);
    p.cstick = deadzone(j.axis[2], j.axis[3], 0.3f);
    bool used = p.stick.len() > 0.5f && before.len() <= 0.5f;
    for (int b = 0; b < kPadButtonCount; ++b) used = used || (p.down[b] && !p.was[b]);
    if (used && !g_forced) {  // the last used device gives the prompts
      g_padMode = true;
      g_family = p.family;
    }
  }
  g_started = true;
}
}  // namespace

// ---- Devices ----------------------------------------------------------------------------
const char* deviceName(int d) {
  static const char* names[] = {"CLAVIER 1", "CLAVIER 2", "MANETTE 1", "MANETTE 2", "MANETTE 3", "MANETTE 4"};
  return (d >= 0 && d < kDeviceCount) ? names[d] : "?";
}

bool deviceAvailable(int d) {
  if (d == kKeyboard1 || d == kKeyboard2) return true;
  return padConnected(d - kPad1);
}

InputState readDevice(int d) {
  InputState in;
  if (d == kKeyboard1 || d == kKeyboard2) {
    // Rebindable keys (Paramètres > Commandes). Clavier 2 also accepts the numpad.
    const auto& k = settings().keys[d == kKeyboard1 ? 0 : 1];
    const bool pad = d == kKeyboard2;
    auto down = [&](int action, int numpadKey) {
      return IsKeyDown(k[action]) || (pad && numpadKey >= 0 && IsKeyDown(numpadKey));
    };
    in.stick.x = float(down(kActRight, -1)) - float(down(kActLeft, -1));
    in.stick.y = float(down(kActDown, -1)) - float(down(kActUp, -1));
    in.jump = down(kActJump, KEY_KP_0);
    in.attack = down(kActAttack, KEY_KP_1);
    in.special = down(kActSpecial, KEY_KP_2);
    in.shield = down(kActShield, KEY_KP_3);
    in.grab = down(kActGrab, KEY_KP_5);
    in.smash = down(kActSmash, KEY_KP_4);
    in.ultimate = down(kActUltimate, KEY_KP_6);
  } else if (d >= kPad1 && d < kDeviceCount && padConnected(d - kPad1)) {
    const Pad& p = g_pads[d - kPad1];
    in.stick = p.stick;
    if (p.down[kPadLeft]) in.stick.x = -1;
    if (p.down[kPadRight]) in.stick.x = 1;
    if (p.down[kPadUp]) in.stick.y = -1;
    if (p.down[kPadDown]) in.stick.y = 1;
    in.cstick = p.cstick;
    auto held = [&](int action) {
      for (int sl = 0; sl < 2; ++sl) {
        const int b = padBinding(d - kPad1, action, sl);
        if (b >= 0 && p.down[b]) return true;
      }
      return false;
    };
    in.attack = held(kActAttack);
    in.special = held(kActSpecial);
    in.jump = held(kActJump);
    in.grab = held(kActGrab);
    in.shield = held(kActShield);
    in.smash = held(kActSmash);
    in.ultimate = held(kActUltimate);
  }
  return in;
}

bool devicePausePressed(int d) {
  if (d == kKeyboard1 || d == kKeyboard2) return anyPressed({KEY_ESCAPE, KEY_P});
  return d >= kPad1 && d < kDeviceCount && padPressed(d - kPad1, kPadStart);
}

// ---- Gamepads ---------------------------------------------------------------------------
void initPads() {
#ifdef CLASH_HAS_GLFW
  // raylib's hot-plug callback copies the joystick name into a 4-entry table even for
  // joystick 5 and more (a PlayStation pad alone can be 3 joysticks): memory corruption.
  glfwSetJoystickCallback(nullptr);
#endif
  const std::string db(reinterpret_cast<const char*>(gamecontrollerdb_txt), gamecontrollerdb_txt_size);
  SetGamepadMappings(db.c_str());
  updatePads();  // pads already plugged in: gamepad prompts, but no "connected" notice
}

void updatePads() {
  Joy joys[kMaxJoy];
  pollJoysticks(joys);
  applyJoysticks(joys);
}

bool padConnected(int p) { return p >= 0 && p < kMaxPads && g_pads[p].joy >= 0; }
bool padDown(int p, int b) { return padConnected(p) && b >= 0 && b < kPadButtonCount && g_pads[p].down[b]; }
bool padPressed(int p, int b) { return padDown(p, b) && !g_pads[p].was[b]; }
bool padAnyPressed(int b) {
  for (int p = 0; p < kMaxPads; ++p)
    if (padPressed(p, b)) return true;
  return false;
}
PadFamily padFamily(int p) { return padConnected(p) ? g_pads[p].family : PadFamily::Xbox; }
int padBinding(int p, int action, int slot) {
  if (action < kActJump || action >= kActionCount || slot < 0 || slot > 1) return -1;
  const Settings& s = settings();
  if (s.padIsDefault() && !padHasTriggers(p)) {  // SNES-style pads: RB shields, Back = ultimate
    if (action == kActShield) return slot == 0 ? kPadRB : -1;
    if (action == kActUltimate) return slot == 0 ? kPadBack : -1;
  }
  return s.pad[action][slot];
}

bool padHasTriggers(int p) {
  if (!padConnected(p)) return true;
  const Pad& pad = g_pads[p];
  return pad.trigReady[0] || pad.trigReady[1] || pad.digitalTrig;
}
const char* padModel(int p) { return padConnected(p) ? g_pads[p].model.c_str() : ""; }
std::string takePadNotice() {
  std::string n;
  n.swap(g_notice);
  return n;
}

bool padPrompts() { return g_padMode; }
PadFamily promptFamily() { return g_family; }
void forcePadPrompts(PadFamily family) {
  g_forced = g_padMode = true;
  g_family = family;
}

// ---- Menus ------------------------------------------------------------------------------
MenuNav readMenuNav() {
  static int held[4] = {0, 0, 0, 0};
  bool dir[4] = {
      anyDown({KEY_UP, KEY_W}), anyDown({KEY_DOWN, KEY_S}),
      anyDown({KEY_LEFT, KEY_A}), anyDown({KEY_RIGHT, KEY_D})};
  MenuNav nav;
  const bool kb1 = anyPressed({KEY_ENTER, KEY_SPACE, KEY_J}), kb2 = anyPressed({KEY_KP_ENTER, KEY_KP_1, KEY_PERIOD});
  nav.confirm = kb1 || kb2;
  nav.back = anyPressed({KEY_ESCAPE, KEY_BACKSPACE, KEY_K, KEY_KP_2, KEY_SLASH});
  nav.start = anyPressed({KEY_ENTER, KEY_KP_ENTER});
  nav.prev = anyPressed({KEY_K, KEY_KP_2, KEY_SLASH});
  if (nav.confirm) nav.device = kb1 ? kKeyboard1 : kKeyboard2;
  for (int i = 0; i < kMaxPads; ++i) {
    Pad& p = g_pads[i];
    if (p.joy < 0) continue;
    // Stick: dominant axis only, with hysteresis (no double steps on a diagonal or near the threshold).
    const bool horizontal = std::fabs(p.stick.x) > std::fabs(p.stick.y);
    const float v[4] = {-p.stick.y, p.stick.y, -p.stick.x, p.stick.x};
    for (int k = 0; k < 4; ++k) {
      p.menuDir[k] = (k >= 2) == horizontal && v[k] > (p.menuDir[k] ? 0.35f : 0.55f);
      dir[k] = dir[k] || p.menuDir[k] || p.down[kPadUp + k];
    }
    const bool a = padPressed(i, kPadA), start = padPressed(i, kPadStart);
    nav.confirm = nav.confirm || a;
    nav.back = nav.back || padPressed(i, kPadB);
    nav.start = nav.start || start;
    nav.prev = nav.prev || padPressed(i, kPadX);
    if ((a || start) && nav.device < 0) nav.device = kPad1 + i;
  }
  bool out[4];
  for (int i = 0; i < 4; ++i) {
    held[i] = dir[i] ? held[i] + 1 : 0;
    out[i] = held[i] == 1 || (held[i] > 18 && held[i] % 6 == 0);  // key repeat
  }
  nav.up = out[0];
  nav.down = out[1];
  nav.left = out[2];
  nav.right = out[3];
  return nav;
}
