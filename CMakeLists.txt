cmake_minimum_required(VERSION 3.20)
project(snake_sdl_split CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  main.cpp
  game.cpp
  audio.cpp
  config.cpp
  leaderboard.cpp
  challenge.cpp
)

find_package(SDL2 QUIET)
find_package(SDL2_mixer QUIET)
find_package(SDL2_ttf QUIET)

if(NOT TARGET SDL2::SDL2 OR NOT TARGET SDL2_mixer::SDL2_mixer OR NOT TARGET SDL2_ttf::SDL2_ttf)
  find_package(PkgConfig REQUIRED)
  if(NOT TARGET SDL2::SDL2)
    pkg_check_modules(PC_SDL2 REQUIRED sdl2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_include_directories(SDL2::SDL2 INTERFACE ${PC_SDL2_INCLUDE_DIRS})
    target_link_libraries(SDL2::SDL2 INTERFACE ${PC_SDL2_LINK_LIBRARIES})
  endif()
  if(NOT TARGET SDL2_mixer::SDL2_mixer)
    pkg_check_modules(PC_SDL2_MIXER REQUIRED sdl2_mixer)
    add_library(SDL2_mixer::SDL2_mixer INTERFACE IMPORTED)
    target_include_directories(SDL2_mixer::SDL2_mixer INTERFACE ${PC_SDL2_MIXER_INCLUDE_DIRS})
    target_link_libraries(SDL2_mixer::SDL2_mixer INTERFACE ${PC_SDL2_MIXER_LINK_LIBRARIES})
  endif()
  if(NOT TARGET SDL2_ttf::SDL2_ttf)
    pkg_check_modules(PC_SDL2_TTF REQUIRED sdl2_ttf)
    add_library(SDL2_ttf::SDL2_ttf INTERFACE IMPORTED)
    target_include_directories(SDL2_ttf::SDL2_ttf INTERFACE ${PC_SDL2_TTF_INCLUDE_DIRS})
    target_link_libraries(SDL2_ttf::SDL2_ttf INTERFACE ${PC_SDL2_TTF_LINK_LIBRARIES})
  endif()
endif()

add_executable(snake_sdl_split ${SOURCES})
target_link_libraries(snake_sdl_split PRIVATE SDL2::SDL2 SDL2_mixer::SDL2_mixer SDL2_ttf::SDL2_ttf)

include(GNUInstallDirs)
install(TARGETS snake_sdl_split RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

