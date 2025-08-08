#include "game.h"

int main(int argc, char **argv) {
  Game g;
  g.init_from_args(argc, argv);
  if (!g.init_sdl())
    return 1;
  g.loop();
  g.shutdown();
  return 0;
}
