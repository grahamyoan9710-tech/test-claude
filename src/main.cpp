// Super Clash Bros. - a platform fighter inspired by Super Smash Bros.
//
// Usage: super_clash [--demo] [--fighters A,B,..] [--stage N]
//                    [--screen title|menu|css|stage|results|settings|training]
//                    [--tab N] [--training] [--pause-at FRAME]
//                    [--pad-glyphs xbox|ps|switch]   (gamepad button prompts, no gamepad needed)
//                    [--shot FRAME FILE.png]... [--record FROM TO EVERY DIR] [--fullres] [--quit-after FRAMES] [--debug]
//                    [--ult SLOT]   (with --demo: the first fighter unleashes its ultimate, SLOT -1/0/1)
//                    [--teams]   (team battle: red against blue)
//                    [--palette N]   (colours of the menus, see menu_theme.hpp; 0 = the game's)
//                    [--export-audio DIR]
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game/app.hpp"
#include "game/menu_theme.hpp"

int main(int argc, char** argv) {
  Options opt;
  for (int i = 1; i < argc; ++i) {
    auto is = [&](const char* s) { return std::strcmp(argv[i], s) == 0; };
    if (is("--demo")) opt.demo = true;
    else if (is("--fighters") && i + 1 < argc) {  // demo line-up: roster indices, comma separated
      for (const char* c = argv[++i]; *c;) {
        opt.fighters.push_back(std::atoi(c));
        while (*c && *c != ',') ++c;
        if (*c == ',') ++c;
      }
    }
    else if (is("--debug")) opt.debug = true;
    else if (is("--teams")) opt.teams = true;
    else if (is("--palette") && i + 1 < argc) g_menuTheme = std::atoi(argv[++i]);
    else if (is("--fullres")) opt.fullRes = true;
    else if (is("--ult") && i + 1 < argc) opt.ultShow = std::atoi(argv[++i]);
    else if (is("--stage") && i + 1 < argc) opt.stage = std::atoi(argv[++i]);
    else if (is("--screen") && i + 1 < argc) opt.screen = argv[++i];
    else if (is("--tab") && i + 1 < argc) opt.tab = std::atoi(argv[++i]);
    else if (is("--training")) opt.training = true;
    else if (is("--pause-at") && i + 1 < argc) opt.pauseAt = std::atoi(argv[++i]);
    else if (is("--quit-after") && i + 1 < argc) opt.quitAfter = std::atoi(argv[++i]);
    else if (is("--pad-glyphs") && i + 1 < argc) {
      const char* f = argv[++i];
      opt.padGlyphs = int(std::strcmp(f, "ps") == 0       ? PadFamily::PlayStation
                          : std::strcmp(f, "switch") == 0 ? PadFamily::Nintendo
                                                          : PadFamily::Xbox);
    } else if (is("--export-audio") && i + 1 < argc) {
      int n = Audio::exportAll(argv[++i]);
      std::printf("%d audio files written\n", n);
      return n > 0 ? 0 : 1;
    } else if (is("--shot") && i + 2 < argc) {
      int frame = std::atoi(argv[++i]);
      opt.shots.push_back({frame, argv[++i]});
    } else if (is("--record") && i + 4 < argc) {  // FROM TO EVERY DIR
      opt.recordFrom = std::atoi(argv[++i]);
      opt.recordTo = std::atoi(argv[++i]);
      opt.recordEvery = std::max(1, std::atoi(argv[++i]));
      opt.recordDir = argv[++i];
    }
  }
  App app(opt);
  app.run();
  return 0;
}
