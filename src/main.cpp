// Super Clash Bros. - a platform fighter inspired by Super Smash Bros.
//
// Usage: super_clash [--demo] [--stage N] [--screen title|css|stage|results]
//                    [--shot FRAME FILE.png]... [--quit-after FRAMES] [--debug]
//                    [--export-audio DIR]
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game/app.hpp"

int main(int argc, char** argv) {
  Options opt;
  for (int i = 1; i < argc; ++i) {
    auto is = [&](const char* s) { return std::strcmp(argv[i], s) == 0; };
    if (is("--demo")) opt.demo = true;
    else if (is("--debug")) opt.debug = true;
    else if (is("--stage") && i + 1 < argc) opt.stage = std::atoi(argv[++i]);
    else if (is("--screen") && i + 1 < argc) opt.screen = argv[++i];
    else if (is("--quit-after") && i + 1 < argc) opt.quitAfter = std::atoi(argv[++i]);
    else if (is("--export-audio") && i + 1 < argc) {
      int n = Audio::exportAll(argv[++i]);
      std::printf("%d audio files written\n", n);
      return n > 0 ? 0 : 1;
    } else if (is("--shot") && i + 2 < argc) {
      int frame = std::atoi(argv[++i]);
      opt.shots.push_back({frame, argv[++i]});
    }
  }
  App app(opt);
  app.run();
  return 0;
}
