cmake_minimum_required(VERSION 3.16)
project(SuperClashBros LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(CLASH_BUILD_GAME "Build the graphical game (needs raylib)" ON)

# ---------------------------------------------------------------------------
# Core simulation: pure C++, no graphics dependency (testable headless)
# ---------------------------------------------------------------------------
add_library(clash_core STATIC
  src/core/roster_normals.cpp
  src/core/roster.cpp
  src/core/stage.cpp
  src/core/match.cpp
  src/core/fighter_update.cpp
  src/core/fighter_physics.cpp
  src/core/combat.cpp
  src/core/ai.cpp
)
target_include_directories(clash_core PUBLIC src)
if(MSVC)
  target_compile_options(clash_core PRIVATE /W3)
else()
  target_compile_options(clash_core PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
endif()

enable_testing()
add_executable(sim_test tests/sim_test.cpp)
target_link_libraries(sim_test PRIVATE clash_core)
add_test(NAME sim_test COMMAND sim_test)

# ---------------------------------------------------------------------------
# Game: raylib front-end
# ---------------------------------------------------------------------------
if(CLASH_BUILD_GAME)
  find_package(raylib 5.0 QUIET)
  if(NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
      GIT_REPOSITORY https://github.com/raysan5/raylib.git
      GIT_TAG 5.5
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(raylib)
  endif()

  add_executable(super_clash
    src/main.cpp
    src/game/app.cpp
    src/game/menus.cpp
    src/game/match_screen.cpp
    src/game/controls.cpp
    src/game/audio.cpp
    src/game/render_fighter.cpp
    src/game/render_body.cpp
    src/game/render_world.cpp
    src/game/render_hud.cpp
    src/game/fx.cpp
  )
  target_link_libraries(super_clash PRIVATE clash_core raylib)
  if(APPLE)
    target_link_libraries(super_clash PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
  endif()
endif()
