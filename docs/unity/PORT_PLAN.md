# Clash Horizon: Unity port plan

> **For agentic workers:** REQUIRED SUB-SKILL: use superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to run this plan phase by phase. Each phase starts by writing its own short task list
> (checkbox `- [ ]` steps, test first) from its section below, in `docs/unity/phases/Pn.md`.

**Goal:** Clash Horizon running in Unity 6 with the same rules, art, characters, timings and look as the raylib game,
then shipped from Unity (Steam and Epic first, mobile later).

**Architecture:** translate the C++ code file by file into C#. The simulation (`src/core`) becomes a pure C# library
with no Unity dependency, checked frame by frame against the C++ game. The renderer (`src/game`) keeps its
immediate-mode style through a small C# layer that offers the ~60 raylib calls the game uses, drawn with Unity command
buffers; the 18 GLSL shaders are rewritten in HLSL with the same math. One boot scene, everything else in code and
text files; tests and builds run from the command line (batchmode).

**Tech stack:** Unity 6000.6.9f1, URP 17.6, Input System 1.20.1, Unity Test Framework 1.8 (NUnit), TextMeshPro SDF
fonts, C#.

**Spec (source of truth):** the C++ game on branch `claude/project-thread-s2ccep`, commit `e25ed5f4` (Hakuen fixes).
Rules: `CLAUDE.md`, `docs/refonte/FICHES.md`, the character-sheet Claude Doc. When this plan and the C++ disagree, the
C++ wins.

## In short (for yoyo)
- **What stays the same:** the art (every sprite, clip and Blender effect strip), the rules, the characters and their
  kits, the hitboxes, the timings (60 game steps per second) and the look (same shaders, same bloom).
- **How we make sure:** the game logic is translated line by line, then the C# and the C++ versions play the same
  scripted fights and their numbers are compared frame by frame. Screens are compared pixel by pixel on screenshots.
  At the end you play both side by side.
- **What Unity adds:** upscalers built into Unity (FSR 1 now; STP and DLSS tested later), easy Windows, Mac and
  mobile builds, Steam and Epic SDKs, and gamepad detection (Xbox, PS4, PS5) built in.
- **The raylib game is not touched** until the Unity version matches it. Then you decide when to switch.
- **Order:** 8 phases (P0 to P7). Each is a short fresh thread that ends with a test and a push.
- **Cost (guess):** about 22,000 lines of C# in roughly 12 to 17 short threads. I measure the real token use of P1
  and give you the projection before going further.
- **What I need from you:** your go; let the PC push to GitHub (today it can read but not push: install the GitHub
  CLI and sign in once with `sudo apt install gh`, `gh auth login`, `gh auth setup-git`); in Unity Hub, add the module
  "Windows Build Support (Mono)" to Unity 6000.6.9f1 (needed for Steam and Epic builds); keep the PC on, unlocked,
  with Claude open while a Unity thread runs.
- **Not chosen:** rebuilding the game "the Unity way" (scenes, Animator, VFX Graph). It would change the look and the
  feel. New effects can use Unity tools later, once the port matches.

## Global constraints
- Art, rules and character designs unchanged: `assets/` (sprites, `sheets.txt`, audio, fonts), roster numbers, kits,
  `FICHES.md`.
- The raylib game stays intact and releasable until parity. Only additive test tooling in `tests/` (C++) is allowed.
- Game speed never depends on FPS: the sim steps at a fixed 60 Hz from a real-time accumulator (max 8 steps per
  frame, like `app.cpp`); menus and transitions use real time (`Time.unscaledDeltaTime`), never one tick per drawn
  frame.
- Effect rules stay: every effect has bloom, no effect tints fighters olive or dirty, no grey dust, effects come from
  the Blender strips (`fx_pfx_*`), no new code-drawn effect.
- One copy of the assets: Unity reads the repo's `assets/` through `scripts/unity_sync_assets.sh`; the synced copy in
  `unity/` is git-ignored and never edited.
- Imports and hitbox fitting keep using the existing tools (`import_clip.py`, `fitall.sh`, `--fit-hitboxes`) until
  P7; Unity loads what they generate.
- Code-first Unity: one scene (`Boot`), built by an editor script; no hand-edited scenes or prefabs.
- Git: branch `claude/unity-port`; merge only, never rebase or `--force`; commit and push after each task; update the
  Obsidian note `Projets/Clash Horizon/Tasks/Unity port.md` before pausing.
- Ponytail full; no package or dependency beyond the tech stack unless a phase names it.
- English everywhere (code comments, commits, notes), short and simple.

## Review Focus
What the phase tests could miss, most likely first. The owning phase adds a test for each.
1. **High FPS:** menu animations, transitions and the title screen take the same time in seconds at 30, 60, 144 and
   uncapped FPS (the C++ counts frames there). Test in P4.
2. **A short tap between two game steps:** at high FPS a frame can run zero steps; a press and release inside it must
   still reach the next step (input latch). Test in P3.
3. **Gamepads:** plugging or removing a pad mid-match, PS4/PS5/Xbox glyphs, two pads of the same kind. Test in P3
   with Input System test devices.
4. **Low resolution:** at render scale 0.5 or with FSR, menus and text stay sharp (UI at screen resolution, the fight
   at render resolution, like the C++). Test in P4.
5. **Broken settings file:** a missing or corrupt settings file loads the defaults without a crash; keys keep their
   C++ names. Test in P4.

## Code map (C++ to C#)
| C++ | Unity (C#) | Notes |
|---|---|---|
| `src/core/*.hpp, *.cpp` | `unity/Assets/Scripts/Core/*.cs` | One file per C++ file, PascalCase names. Assembly `ClashHorizon.Core`, no engine references. |
| `roster_hitfit.inc` (generated) | `Core/RosterHitfit.cs` (generated) | `scripts/gen_hitfit_cs.py` converts it; rerun after each `fitall.sh`. |
| `tests/sim_test.cpp` | `unity/Assets/Tests/EditMode/SimTests.cs` | Same test names and assertions (NUnit). |
| raylib calls in `src/game` | `Scripts/Gfx/Rl.cs` | Only the ~60 calls the game uses: rectangles, circles, lines, rings, polygons, textures, `rlBegin`/`rlVertex` triangles, matrix stack, blend modes, render textures, shaders with uniforms, scissor, `Camera2D`. Batched dynamic meshes in a `CommandBuffer`, no allocation per frame. |
| `src/game/*.cpp` | `Scripts/Game/*.cs` | Same files and functions, translated line by line on top of `Rl`. |
| 18 GLSL shaders (`app.cpp`, `pixel.cpp`, `fighter_art.cpp`, `fonts.cpp`, `render_light.cpp`, `render_rtx.cpp`) | `unity/Assets/Shaders/*.shader` (HLSL) | Same math. The game's FXAA and FSR 1 (EASU + RCAS) are replaced by URP's own FXAA and FSR 1. |
| `fonts.cpp` (SDF fonts) | TextMeshPro SDF font assets + `Rl.DrawTextEx` | Font assets made from the 4 TTF files by an editor script. |
| `controls.cpp` | Unity Input System | Keyboard J1 and J2, gamepads, rebinding; pad family read from the Input System. |
| `audio.cpp` | `AudioSource` pool + `AudioClip.Create` | mp3 files as clips; synthesized sounds and music generated at start with the same formulas. |
| `settings.cpp` | same keys, file in `Application.persistentDataPath` | |
| `app.cpp`, `main.cpp` | `Scripts/Game/GameMain.cs` (one MonoBehaviour) | Same command line flags (`--demo`, `--fighters`, `--stage`, `--screen`, `--tab`, `--shot`, `--record`, `--quit-after`, `--ult`, `--skill`, `--weapon`...), so the `CLAUDE.md` demo commands work on both games. |
| `scripts/package_release.sh`, `release.yml` | `scripts/unity_build.sh` + a separate workflow (P6) | The raylib workflow is not changed. |

### Translation rules (the bugs a port usually adds)
- **Value or reference.** Small C++ value types become C# `struct`: `Vec2`, `Rect`, `Color3`, `InputState`,
  `HitboxDef`, `VelEvent`, `ProjectileDef`, `BeamDef`, `ProjShot`, `GameEvent`, `Platform`. Types holding lists
  become `class` with a deep `Clone()`: `MoveDef`, `WeaponDef`, `FormDef`, `CharacterDef`, `StageDef`. Wherever the
  C++ copies one of them (a by-value parameter such as `MoveDef piercing(MoveDef m)`, or `MoveDef m = c.moves[i];`),
  the C# calls `Clone()`. `Fighter`, `Match`, `Projectile`, `Orb`, `WeaponItem` are classes used by reference, like
  `Fighter&` in the C++.
- **`Match`** methods spread over several C++ files become one `partial class Match` over the same files.
- **`Rng`** is a class (never copied by accident), same xorshift32 on `uint`.
- **Floats:** `float` and `MathF` only (no `double`), same constants, same order of operations. `const char*` effect
  names become `string`, compared with `==` (ordinal).
- **Numbers in text:** every parse and print uses `CultureInfo.InvariantCulture`. yoyo's PC is in French, where
  `1.5` reads as `1,5`: without this, `sheets.txt` and the settings file break.
- **Order:** `List<T>` keeps the `std::vector` order; the only sort in core is a `std::stable_sort` (ranking), so use
  a stable sort with the same key.
- **No Unity in the sim:** no `Time`, no `UnityEngine.Random`, no physics. The sim only moves in `Match.Step(inputs)`.
- **Colour:** raylib does all colour math on raw (gamma) values. Keep that: the project is in Linear colour space,
  sprites are imported without sRGB, render textures do no conversion, and one final pass converts once for the
  screen. Result: the same numbers as raylib.
- **Pixel art:** sprites imported uncompressed, no mipmaps, clamped, never resized (an `AssetPostprocessor` enforces
  it); filters set at load time exactly like the C++ (`SetTextureFilter`).

## Phases
Each phase is one fresh thread (Opus 5.5; max effort for P1 and P3) that ends with its tests green, a push, and the
Obsidian note updated. Sizes: S < 1,500 lines, M 1,500 to 4,000, L 4,000 to 7,000, XL > 7,000.

### P0 · Project setup (S)
**Before:** `git push` works from the PC (risk 10). Work in `~/test-claude` on `claude/unity-port`, or in a second
clone if another thread uses that folder.
**Files:** create `unity/` (`Packages/manifest.json`, `ProjectSettings/`, `Assets/Scripts/{Core,Gfx,Game}`,
`Assets/Tests/EditMode`, `Assets/Editor`), `unity/.gitignore` (Library, Temp, Logs, Builds, UserSettings, synced
assets), `scripts/unity_sync_assets.sh`, `scripts/unity_test.sh`, `scripts/unity_build.sh`.
**Decisions:** assemblies `ClashHorizon.Core` (no engine references), `ClashHorizon.Game`, `ClashHorizon.Editor`,
`ClashHorizon.Tests`, with matching namespaces. Synced assets go to `unity/Assets/Resources/Sprites`,
`unity/Assets/Resources/Audio` and `unity/Assets/Fonts`, loaded with `Resources.Load`. `Editor/ProjectSetup.cs` builds
`Boot.unity` (camera + a `GameMain` stub that only handles `--quit-after`) and the URP asset. `Editor/Build.cs` has `Build.Linux()` and `Build.Windows()`
(Mono backend). The Unity binary comes from `$UNITY`, default `~/Unity/Hub/Editor/6000.6.9f1/Editor/Unity`.
**Test:** `scripts/unity_test.sh` runs the EditMode test `Setup_Works` (the `sheets.txt` loaded from Resources equals
the repo file byte for byte) and exits 0; `scripts/unity_build.sh linux` makes a player that opens a window and quits
with `--quit-after 60`.

### P1 · Simulation (L)
**Files:** `Scripts/Core/`: `MathUtil.cs`, `Input.cs`, `Moves.cs`, `Fighter.cs`, `Stage.cs`, `Swing.cs`,
`HitShapes.cs`, `RosterKit.cs`, `Roster.cs`, `RosterNormals.cs`, `RosterUltimates.cs`, `RosterHitfit.cs` (generated),
`Weapons.cs`, `FighterUpdate.cs`, `FighterPhysics.cs`, `Combat.cs`, `Match.cs`, `Ai.cs`;
`Tests/EditMode/SimTests.cs`, `Tests/EditMode/ParityTests.cs`; C++: a `trace` mode in `tests/sim_test.cpp`;
`scripts/gen_hitfit_cs.py`, `scripts/unity_parity.sh`.
**Interfaces:** `new Match(MatchConfig cfg)`, `void Match.Step(InputState[] inputs)` (4 ports),
`IReadOnlyList<CharacterDef> Roster.All()`, `IReadOnlyList<StageDef> Stage.List()`, and the AI with the same
signatures as `ai.hpp`.
**Trace format (both sides):** one line per fighter per frame:
`frame fighter state move moveFrame pos.x pos.y vel.x vel.y kb.x kb.y percent stocks`, floats printed with 4 decimals.
**Tests:** every test of `sim_test.cpp` ported with the same name and assertions; `Determinism_SameSeedSameHash`;
parity scenarios run by the C++ `sim_test trace NAME` and by the C# `ParityTests`: `jab_hit`, `smash_ko`,
`projectile`, `weapon_pickup_switch`, `ult_capture`, `hakuen_rift`, `cpu_vs_cpu_600`. States, moves, frames and
stocks must be equal, positions within 0.01 px for the scripted ones; for the CPU fight, report the first frame that
differs. Balance (`sim_test balance`): each win rate within 5 points of the C++ one.

### P2 · Sprites on screen (M)
**Files:** `Scripts/Gfx/Rl.cs`, `Scripts/Gfx/RlShaders.cs`, `Scripts/Game/FighterArt.cs` (from `fighter_art.cpp`),
`Scripts/Game/Pixel.cs` (from `pixel.cpp`), `Shaders/Tint.shader`, `Shaders/Pixel.shader`, `Shaders/Grey.shader`,
`Editor/SpriteImport.cs`, `scripts/imgdiff.py`.
**Interfaces:** `Rl` keeps the raylib names (`Rl.BeginTextureMode`, `Rl.DrawTexturePro`, `Rl.BeginBlendMode`,
`Rl.BeginShaderMode`...), so translated code reads the same as the C++.
**Test:** `--screen anim --tab N --shot 20 out.png --quit-after 25` works in the Unity build; `scripts/imgdiff.py`
against the raylib shot of the same page: at most 1% of pixels differ by more than 2/255, on 3 pages (Hakuen unarmed,
weapon A, weapon B).

### P3 · The fight (XL)
**Files:** `Scripts/Game/`: `MatchScreen.cs`, `RenderWorld.cs`, `RenderBody.cs`, `RenderFighter.cs`,
`RenderItems.cs`, `RenderWeapons.cs`, `RenderStages.cs`, `RenderHud.cs`, `RenderVfx.cs`, `RenderUlt.cs`, `Fx.cs`,
`RenderLight.cs`, `RenderRtx.cs`, `Paint.cs`, `Controls.cs`, `Audio.cs`, `GameMain.cs`; shaders `Lit`, `Mirror`,
`RtxMask`, `RtxSeed`, `RtxJump`, `RtxTrace`, `RtxResolve`, `RtxComposite`.
**Decisions:** each frame the loop reads input, latches presses until a step uses them, runs up to 8 steps from the
accumulator, then draws (same as `app.cpp`). The fight renders at render resolution, the HUD at screen resolution.
**Tests:** `--demo --fighters 0,1 --shot 300` against raylib within the P2 threshold; speed: at 30, 60, 144 and
uncapped FPS, 600 steps take 10 s (± 0.1 s); input latch; gamepad add and remove; no garbage per frame during a match
(Profiler recorder); RTX Medium and High FPS logged on the laptop. For yoyo: side-by-side videos of Hakuen's ults and
skills and of the other 4 characters.

### P4 · Menus and screens (L)
**Files:** `Ui.cs`, `UiPrestige.cs`, `Menus.cs`, `MenuTheme.cs`, `ScreenTitle.cs`, `ScreenLoading.cs`,
`ScreenVersus.cs`, `ScreenResults.cs`, `ScreenGallery.cs`, `ScreenSettings.cs`, `ScreenPoses.cs`, `MatchPause.cs`,
`Settings.cs`, `Fonts.cs`, `Shaders/Sdf.shader`, `Editor/FontAssets.cs`.
**Decisions:** settings use real Unity features only: resolution and fullscreen (`Screen.SetResolution`,
`FullScreenMode`), V-Sync (`QualitySettings.vSyncCount`), FSR 1 and STP (URP upscaling filter), FXAA (camera
anti-aliasing), SSAA x2 (render scale 2). An option with no real effect is removed, never faked.
**Tests:** screenshot diff of each screen (`--screen title|menu|css|stage|results|settings|training`) against raylib;
each setting changes the Unity value it names (asserted); Review Focus 1, 4 and 5.

### P5 · Tools and headless checks (S)
`--record FROM TO EVERY DIR`, `--pause-at`, `--uncapped` benchmark, `--teams`, `--palette`, `--pad-glyphs`. The hitbox
tools (`--check-hitboxes`, `--fit-impacts`, `--fit-hitboxes`, `--export-pose`) stay in C++ for now.
**Test:** the demo commands of `CLAUDE.md` give the same frames with both binaries (P2 threshold).

### P6 · Builds and release (M)
`scripts/unity_build.sh linux|windows`, the version number in one place, a new workflow `unity-release.yml` (GameCI;
it needs Unity licence secrets that yoyo adds) or local builds uploaded by hand. Test releases are pre-releases,
never "latest".
**Test:** both builds start, play a CPU match to the end and quit; archives uploaded.

### P7 · Parity check and switch (M)
Side-by-side videos of every character and stage; yoyo plays both; gaps fixed. Then yoyo decides the switch: Unity
becomes the main game, the raylib game is frozen on its branch, and we decide whether to port the hitbox tools.

### After parity (separate plans)
- DLSS and STP through URP's upscaler framework (DLSS: Windows and NVIDIA RTX only). Sprites need motion vectors, and
  temporal upscalers can blur pixel art (the reason TAA was refused), so test first and keep only if it looks right.
- Steamworks and Epic Online Services; mobile (touch controls, texture compression, 4096 px texture limit).

## Risks
| # | Risk | What we do |
|---|---|---|
| 1 | The feel changes: C++ and C# floats can differ a little, and the gap can grow | Line-by-line translation, same constants, `MathF`, frame-by-frame traces, balance check, side-by-side play. |
| 2 | Copy bugs: the C++ copies structs, C# shares objects | The value or reference rules above, `Clone()` at every C++ copy, weapon and form tests. |
| 3 | The look changes: colour space, blending, filters, bloom | Raw-value colour pipeline, same shaders, screenshot diffs. |
| 4 | Slow iteration: the Unity editor takes 1 to 2 min to start for each test run | Command-line test scripts; the sim has no Unity dependency. |
| 5 | Hitches from garbage collection | No allocation per frame in sim and renderer, checked with the Profiler in P3. |
| 6 | The raylib game keeps changing during the port (Hakuen feedback) | Port from a fixed commit; each later C++ change is listed in the task note and redone in C#. |
| 7 | Windows builds: from Linux only the Mono backend works (IL2CPP needs a Windows PC), and the module is not installed | yoyo adds "Windows Build Support (Mono)" in Unity Hub. |
| 8 | French number format breaks text parsing | `CultureInfo.InvariantCulture` everywhere (translation rules). |
| 9 | DLSS is not a switch to flip: motion vectors, pixel-art blur | After parity, tested, real Unity feature only. |
| 10 | The PC can't push to GitHub (no sign-in), so work could stay only on the laptop | yoyo signs in once with `gh` before P0; until then each commit also goes to the project files as a git bundle. |

## Cost
- About 24,000 lines of C++ to read (core 5,400, renderer and screens 17,500, tests 1,500) and about 22,000 lines of
  C# to write, plus 18 shaders.
- About 12 to 17 short threads: P0 1, P1 3, P2 2, P3 4, P4 3, P5 1, P6 1, P7 1 or 2.
- Tokens can't be measured in advance. After P1 I report its real use and the projection for the rest.
