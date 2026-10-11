cmake_minimum_required(VERSION 3.16)
project(SuperClashBros LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(CLASH_BUILD_GAME "Build the graphical game (needs raylib)" ON)
option(CLASH_PORTABLE "Self-contained executable (static C++ runtime) for distribution" OFF)

# ---------------------------------------------------------------------------
# Core simulation: pure C++, no graphics dependency (testable headless)
# ---------------------------------------------------------------------------
add_library(clash_core STATIC
  src/core/roster_normals.cpp
  src/core/roster.cpp
  src/core/stage.cpp
  src/core/match.cpp
  src/core/weapons.cpp
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
  find_package(raylib 5.5 QUIET)  # older versions lack APIs we use: fetch 5.5 instead
  set(CLASH_HAS_GLFW OFF)
  if(NOT raylib_FOUND)
    set(CLASH_HAS_GLFW ON)  # our own raylib build: its GLFW can be called (key names, joysticks)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
      GIT_REPOSITORY https://github.com/raysan5/raylib.git
      GIT_TAG 5.5
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(raylib)
  endif()

  # Fonts, icon and controller database are compiled into the executable (see cmake/embed.cmake).
  include(cmake/embed.cmake)
  set(CLASH_ASSETS_C "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_assets.c")
  embed_files(OUTPUT "${CLASH_ASSETS_C}" FILES
    font_title_ttf "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LuckiestGuy-Regular.ttf"
    font_ui_ttf "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/Poppins-Bold.ttf"
    icon_png "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
    gamecontrollerdb_txt "${CMAKE_CURRENT_SOURCE_DIR}/assets/gamecontrollerdb.txt")
  set(CLASH_EXTRA_SOURCES "${CLASH_ASSETS_C}")
  if(WIN32)  # icon of the .exe in the file explorer
    set(CLASH_RC "${CMAKE_CURRENT_BINARY_DIR}/generated/icon.rc")
    file(WRITE "${CLASH_RC}" "IDI_ICON1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico\"\n")
    list(APPEND CLASH_EXTRA_SOURCES "${CLASH_RC}")
  endif()

  add_executable(super_clash
    src/main.cpp
    src/game/app.cpp
    src/game/screen_title.cpp
    src/game/screen_settings.cpp
    src/game/screen_results.cpp
    src/game/menus.cpp
    src/game/match_screen.cpp
    src/game/controls.cpp
    src/game/settings.cpp
    src/game/fonts.cpp
    src/game/ui.cpp
    src/game/audio.cpp
    src/game/render_fighter.cpp
    src/game/render_body.cpp
    src/game/render_world.cpp
    src/game/render_hud.cpp
    src/game/fx.cpp
    ${CLASH_EXTRA_SOURCES}
  )
  target_link_libraries(super_clash PRIVATE clash_core raylib)
  if(MINGW)
    # One self-contained .exe, no console window.
    target_link_options(super_clash PRIVATE -static -static-libgcc -static-libstdc++ -mwindows)
  elseif(CLASH_PORTABLE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE)
    target_link_options(super_clash PRIVATE -static-libstdc++ -static-libgcc)
  endif()
  if(CLASH_HAS_GLFW)
    target_compile_definitions(super_clash PRIVATE CLASH_HAS_GLFW=1)
    target_include_directories(super_clash PRIVATE "${raylib_SOURCE_DIR}/src/external/glfw/include")
  endif()
  if(APPLE)
    target_link_libraries(super_clash PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
  endif()
endif()
