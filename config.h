#pragma once
#include "common.h"

struct AppConfig {
  Theme theme;
  bool wrap;
  int tick_ms;
  int cols;
  int rows;
  int overlay_alpha;
  int preset_idx;
  int profile;
  uint32_t seed;
};

std::string base_data();
std::string base_cfg();
std::string hs_path(int profile);
std::string cfg_path(int profile);
std::string lb_path();
std::string lb_html_path();
std::string lb_json_path();

void defaults(AppConfig &c);
bool load_cfg(AppConfig &c);
bool save_cfg(const AppConfig &c);
int load_highscore(int profile);
void save_highscore(int profile, int s);
