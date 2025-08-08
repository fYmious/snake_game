#pragma once
#include "common.h"

struct Audio {
  int rate;
  Mix_Chunk *eat;
  Mix_Chunk *hit;
  Mix_Chunk *move;
  bool init();
  void quit();
};
