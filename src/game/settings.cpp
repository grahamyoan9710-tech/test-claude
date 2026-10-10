#include "game/settings.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "raylib.h"

#ifdef CLASH_HAS_GLFW
extern "C" const char* glfwGetKeyName(int key, int scancode);
#endif

namespace {
Settings g_settings;

const int kDefaultKeys[2][kActionCount] = {
    {KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE, KEY_J, KEY_K, KEY_L, KEY_I, KEY_O, KEY_U},
    {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_RIGHT_SHIFT, KEY_RIGHT_CONTROL,
     KEY_SEMICOLON, KEY_APOSTROPHE},
};
const char* kActionIds[kActionCount] = {"left", "right", "up", "down", "jump",
                                        "attack", "special", "shield", "grab", "smash", "ultimate"};

int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
}  // namespace

const char* actionName(int a) {
  static const char* names[kActionCount] = {"Gauche", "Droite", "Haut", "Bas", "Saut",
                                            "Attaque", "Spéciale", "Bouclier", "Saisie", "Attaque smash",
                                            "Ultime"};
  return (a >= 0 && a < kActionCount) ? names[a] : "?";
}

Settings::Settings() {
  resetKeys(0);
  resetKeys(1);
}

void Settings::resetKeys(int kb) {
  for (int a = 0; a < kActionCount; ++a) keys[kb & 1][a] = kDefaultKeys[kb & 1][a];
}

Settings& settings() { return g_settings; }

std::string settingsPath() {
  std::string dir = GetApplicationDirectory();
  if (dir.empty()) dir = "./";
  return dir + "super_clash.cfg";
}

float shakeScale() {
  static const float k[3] = {0.0f, 0.5f, 1.0f};
  return k[clampi(g_settings.screenShake, 0, 2)];
}

void loadSettings() {
  std::string path = settingsPath();
  if (!FileExists(path.c_str())) return;
  char* text = LoadFileText(path.c_str());
  if (!text) return;
  Settings& s = g_settings;
  bool loaded[2][kActionCount] = {};
  char* line = std::strtok(text, "\r\n");
  while (line) {
    char key[64];
    int value;
    if (std::sscanf(line, "%63[^=]=%d", key, &value) == 2) {
      std::string k = key;
      if (k == "master_volume") s.masterVolume = clampi(value, 0, 100);
      else if (k == "music_volume") s.musicVolume = clampi(value, 0, 100);
      else if (k == "sfx_volume") s.sfxVolume = clampi(value, 0, 100);
      else if (k == "fullscreen") s.fullscreen = value != 0;
      else if (k == "high_quality") s.highQuality = value != 0;
      else if (k == "screen_shake") s.screenShake = clampi(value, 0, 2);
      else if (k == "show_fps") s.showFps = value != 0;
      else if (k == "show_hitboxes") s.showHitboxes = value != 0;
      else if (k == "player_tags") s.playerTags = value != 0;
      else if (k == "stocks") s.stocks = clampi(value, 1, 5);
      else if (k == "items") s.items = value != 0;
      else if (k == "cpu_level") s.cpuLevel = clampi(value, 1, 9);
      else {
        for (int kb = 0; kb < 2; ++kb)
          for (int a = 0; a < kActionCount; ++a)
            if (k == std::string("kb") + char('1' + kb) + "." + kActionIds[a] && keyBindable(value)) {
              s.keys[kb][a] = value;
              loaded[kb][a] = true;
            }
      }
    }
    line = std::strtok(nullptr, "\r\n");
  }
  UnloadFileText(text);
  // Actions added since the file was written (e.g. "ultimate") keep their default key only if
  // no saved binding already uses it; otherwise they get a free key.
  auto used = [&](int key, int kb0, int a0) {
    for (int kb = 0; kb < 2; ++kb)
      for (int a = 0; a < kActionCount; ++a)
        if (!(kb == kb0 && a == a0) && s.keys[kb][a] == key) return true;
    return false;
  };
  static const int spare[] = {KEY_U, KEY_Y, KEY_H, KEY_N, KEY_B, KEY_T, KEY_G, KEY_APOSTROPHE, KEY_RIGHT_BRACKET,
                              KEY_BACKSLASH, KEY_END, KEY_PAGE_DOWN, KEY_INSERT, KEY_HOME};
  for (int kb = 0; kb < 2; ++kb)
    for (int a = 0; a < kActionCount; ++a) {
      if (loaded[kb][a] || !used(s.keys[kb][a], kb, a)) continue;
      for (int key : spare)
        if (!used(key, kb, a)) {
          s.keys[kb][a] = key;
          break;
        }
    }
}

bool saveSettings() {
  const Settings& s = g_settings;
  std::string out = "# Super Clash Bros. - parametres\n";
  auto put = [&](const char* k, int v) { out += std::string(k) + "=" + std::to_string(v) + "\n"; };
  put("master_volume", s.masterVolume);
  put("music_volume", s.musicVolume);
  put("sfx_volume", s.sfxVolume);
  put("fullscreen", s.fullscreen);
  put("high_quality", s.highQuality);
  put("screen_shake", s.screenShake);
  put("show_fps", s.showFps);
  put("show_hitboxes", s.showHitboxes);
  put("player_tags", s.playerTags);
  put("stocks", s.stocks);
  put("items", s.items);
  put("cpu_level", s.cpuLevel);
  for (int kb = 0; kb < 2; ++kb)
    for (int a = 0; a < kActionCount; ++a)
      put((std::string("kb") + char('1' + kb) + "." + kActionIds[a]).c_str(), s.keys[kb][a]);
  std::string path = settingsPath();
  return SaveFileText(path.c_str(), const_cast<char*>(out.c_str()));
}

bool keyBindable(int key) {
  if (key <= 0) return false;
  switch (key) {
    case KEY_ESCAPE: case KEY_F1: case KEY_F11: case KEY_ENTER: case KEY_KP_ENTER:
      return false;  // reserved: pause/back, hitboxes, fullscreen, menus
    default:
      return true;
  }
}

std::string keyName(int key) {
  switch (key) {
    case KEY_SPACE: return "ESPACE";
    case KEY_ENTER: return "ENTRÉE";
    case KEY_TAB: return "TAB";
    case KEY_BACKSPACE: return "RET. ARR.";
    case KEY_ESCAPE: return "ÉCHAP";
    case KEY_LEFT: return "FLÈCHE G";
    case KEY_RIGHT: return "FLÈCHE D";
    case KEY_UP: return "FLÈCHE H";
    case KEY_DOWN: return "FLÈCHE B";
    case KEY_LEFT_SHIFT: return "MAJ G";
    case KEY_RIGHT_SHIFT: return "MAJ D";
    case KEY_LEFT_CONTROL: return "CTRL G";
    case KEY_RIGHT_CONTROL: return "CTRL D";
    case KEY_LEFT_ALT: return "ALT";
    case KEY_RIGHT_ALT: return "ALT GR";
    case KEY_CAPS_LOCK: return "VERR MAJ";
    case KEY_INSERT: return "INSER";
    case KEY_DELETE: return "SUPPR";
    case KEY_HOME: return "DÉBUT";
    case KEY_END: return "FIN";
    case KEY_PAGE_UP: return "PG HAUT";
    case KEY_PAGE_DOWN: return "PG BAS";
    case KEY_KP_DECIMAL: return "PAVÉ .";
    case KEY_KP_DIVIDE: return "PAVÉ /";
    case KEY_KP_MULTIPLY: return "PAVÉ *";
    case KEY_KP_SUBTRACT: return "PAVÉ -";
    case KEY_KP_ADD: return "PAVÉ +";
    case KEY_KP_ENTER: return "PAVÉ ENTRÉE";
    default: break;
  }
  if (key >= KEY_KP_0 && key <= KEY_KP_9) return "PAVÉ " + std::to_string(key - KEY_KP_0);
  if (key >= KEY_F1 && key <= KEY_F12) return "F" + std::to_string(key - KEY_F1 + 1);
#ifdef CLASH_HAS_GLFW
  if (key >= 32 && key <= 96) {
    const char* n = glfwGetKeyName(key, 0);
    if (n && *n) {
      std::string s = n;
      for (char& c : s)
        if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
      if (s == "é") return "É";
      if (s == "è") return "È";
      if (s == "à") return "À";
      if (s == "ç") return "Ç";
      if (s == "ù") return "Ù";
      return s;
    }
  }
#endif
  if (key >= 32 && key < 127) return std::string(1, char(key));
  return "#" + std::to_string(key);
}
