#pragma once
#include "common.h"
#include "config.h"

struct LBEntry {
  int score;
  int profile;
  uint32_t seed;
  int cols, rows;
  int wrap;
  int speed;
  int preset;
  std::string name;
  uint64_t ts;
};

void append_lb(const LBEntry &e);
std::vector<LBEntry> load_lb();
bool export_html(const std::vector<LBEntry> &lb);
bool export_json(const std::vector<LBEntry> &lb);
