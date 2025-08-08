#pragma once
#include "audio.h"
#include "challenge.h"
#include "common.h"
#include "config.h"
#include "leaderboard.h"

struct Game {
  AppConfig cfg;
  Theme theme;
  SDL_Window *win;
  SDL_Renderer *ren;
  TTF_Font *font;
  Audio audio;
  std::mt19937 rng;
  std::deque<P> snake, prev_snake;
  Dir dir, next_dir;
  int score, best, level;
  std::vector<P> walls;
  P food;
  bool running, paused, over, show_settings, show_lb;
  int tick_cur;
  Uint32 last_tick;
  int sel_idx;
  int W, H;
  std::string last_challenge;
  uint32_t last_copy_ticks;
  std::vector<Theme> presets;

  Game();
  bool init_from_args(int argc, char **argv);
  bool init_sdl();
  void loop();
  void shutdown();
};
