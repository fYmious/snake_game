#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

struct P {
  int x, y;
};
enum Dir { U, D, L, R };

struct Col {
  int r, g, b;
};
struct Theme {
  Col bg, grid, food, head, body;
};

int C_OVERLAY_A = 160;

std::string hs_path() {
  const char *xdg = std::getenv("XDG_DATA_HOME");
  const char *home = std::getenv("HOME");
  std::filesystem::path base =
      xdg && *xdg ? xdg
                  : (home ? (std::filesystem::path(home) / ".local/share")
                          : std::filesystem::path("."));
  std::filesystem::path dir = base / "snake_sdl2";
  std::filesystem::create_directories(dir);
  return (dir / "highscore.txt").string();
}

int load_highscore() {
  std::ifstream f(hs_path());
  int s = 0;
  if (f)
    f >> s;
  return s;
}
void save_highscore(int s) {
  std::ofstream f(hs_path(), std::ios::trunc);
  if (f)
    f << s;
}

bool parse_hex(const std::string &s, Col &out) {
  if (s.size() != 7 || s[0] != '#')
    return false;
  auto hex = [&](char c) {
    if ('0' <= c && c <= '9')
      return c - '0';
    if ('a' <= c && c <= 'f')
      return 10 + c - 'a';
    if ('A' <= c && c <= 'F')
      return 10 + c - 'A';
    return 0;
  };
  out.r = hex(s[1]) * 16 + hex(s[2]);
  out.g = hex(s[3]) * 16 + hex(s[4]);
  out.b = hex(s[5]) * 16 + hex(s[6]);
  return true;
}

std::vector<Uint8> synth_pcm_s16_mono(int hz, int ms, int volume,
                                      int sample_rate) {
  int samples = sample_rate * ms / 1000;
  std::vector<Uint8> buf(samples * 2);
  double t = 0.0, dt = 1.0 / sample_rate;
  const double PI = 3.14159265358979323846;
  for (int i = 0; i < samples; ++i) {
    double v = sin(2.0 * PI * hz * t);
    int val = (int)(v * volume);
    buf[i * 2 + 0] = (Uint8)(val & 0xff);
    buf[i * 2 + 1] = (Uint8)((val >> 8) & 0xff);
    t += dt;
  }
  return buf;
}

std::string argval(int argc, char **argv, const std::string &key) {
  std::string k = "--" + key + "=";
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a.rfind(k, 0) == 0)
      return a.substr(k.size());
  }
  return "";
}
bool hasflag(int argc, char **argv, const std::string &key) {
  std::string k = "--" + key;
  for (int i = 1; i < argc; ++i)
    if (k == argv[i])
      return true;
  return false;
}

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
    return 1;
  if (TTF_Init() != 0)
    return 1;

  int cols =
      std::max(8, std::min(96, std::atoi(argval(argc, argv, "cols").c_str())));
  int rows =
      std::max(8, std::min(72, std::atoi(argval(argc, argv, "rows").c_str())));
  if (!cols)
    cols = 32;
  if (!rows)
    rows = 24;

  Theme theme;
  theme.bg = {16, 16, 16};
  theme.grid = {40, 40, 40};
  theme.food = {220, 50, 47};
  theme.head = {38, 139, 210};
  theme.body = {133, 153, 0};

  Col tmp;
  std::string v;
  if (!(v = argval(argc, argv, "bg")).empty() && parse_hex(v, tmp))
    theme.bg = tmp;
  if (!(v = argval(argc, argv, "grid")).empty() && parse_hex(v, tmp))
    theme.grid = tmp;
  if (!(v = argval(argc, argv, "food")).empty() && parse_hex(v, tmp))
    theme.food = tmp;
  if (!(v = argval(argc, argv, "head")).empty() && parse_hex(v, tmp))
    theme.head = tmp;
  if (!(v = argval(argc, argv, "body")).empty() && parse_hex(v, tmp))
    theme.body = tmp;

  std::vector<Theme> presets = {{{16, 16, 16},
                                 {40, 40, 40},
                                 {220, 50, 47},
                                 {38, 139, 210},
                                 {133, 153, 0}},
                                {{15, 15, 20},
                                 {55, 55, 70},
                                 {255, 203, 0},
                                 {0, 168, 255},
                                 {106, 255, 106}},
                                {{10, 10, 10},
                                 {50, 50, 50},
                                 {255, 105, 180},
                                 {173, 216, 230},
                                 {152, 251, 152}},
                                {{24, 24, 24},
                                 {60, 60, 60},
                                 {255, 87, 51},
                                 {88, 214, 141},
                                 {52, 152, 219}},
                                {{0, 0, 0},
                                 {70, 70, 70},
                                 {255, 59, 48},
                                 {255, 255, 255},
                                 {180, 180, 180}}};
  int preset_idx = 0;

  bool wrap =
      hasflag(argc, argv, "wrap") || (!hasflag(argc, argv, "no-wrap") && false);
  int tick_ms = std::atoi(argval(argc, argv, "speed").c_str());
  if (!tick_ms)
    tick_ms = 120;
  tick_ms = std::max(30, std::min(400, tick_ms));

  int W = 960, H = 720;
  SDL_Window *win = SDL_CreateWindow("Snake SDL2", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, W, H,
                                     SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!win)
    return 1;
  SDL_Renderer *ren = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!ren)
    return 1;

  int mix_rate = 22050;
  if (Mix_OpenAudio(mix_rate, AUDIO_S16SYS, 1, 1024) != 0)
    return 1;

  auto pcm_eat = synth_pcm_s16_mono(880, 80, 2000, mix_rate);
  auto pcm_hit = synth_pcm_s16_mono(110, 250, 2500, mix_rate);
  auto pcm_move = synth_pcm_s16_mono(660, 30, 1200, mix_rate);
  Mix_Chunk *s_eat = Mix_QuickLoad_RAW(pcm_eat.data(), (Uint32)pcm_eat.size());
  Mix_Chunk *s_hit = Mix_QuickLoad_RAW(pcm_hit.data(), (Uint32)pcm_hit.size());
  Mix_Chunk *s_move =
      Mix_QuickLoad_RAW(pcm_move.data(), (Uint32)pcm_move.size());

  TTF_Font *font =
      TTF_OpenFontIndex("/usr/share/fonts/TTF/DejaVuSans.ttf", 18, 0);
  if (!font)
    font = TTF_OpenFontIndex("/usr/share/fonts/dejavu/DejaVuSans.ttf", 18, 0);
  if (!font)
    font = TTF_OpenFontIndex("/usr/share/fonts/TTF/LiberationSans-Regular.ttf",
                             18, 0);

  std::mt19937 rng(std::random_device{}());
  auto rand_cell = [&](int a, int b) {
    std::uniform_int_distribution<int> d(a, b);
    return d(rng);
  };

  auto border_walls = [&]() {
    std::vector<P> w;
    for (int x = 0; x < cols; x++) {
      w.push_back({x, 0});
      w.push_back({x, rows - 1});
    }
    for (int y = 0; y < rows; y++) {
      w.push_back({0, y});
      w.push_back({cols - 1, y});
    }
    return w;
  };

  auto level_walls = [&](int lvl) {
    std::vector<P> w = border_walls();
    if (lvl >= 2)
      for (int x = 6; x < cols - 6; x++)
        w.push_back({x, rows / 2});
    if (lvl >= 3)
      for (int y = 4; y < rows - 4; y++)
        w.push_back({cols / 3, y});
    if (lvl >= 4)
      for (int y = 4; y < rows - 4; y++)
        w.push_back({2 * cols / 3, y});
    if (lvl >= 5)
      for (int x = 8; x < cols - 8; x++)
        if ((x / 2) % 2 == 0) {
          w.push_back({x, 5});
          w.push_back({x, rows - 6});
        }
    if (lvl >= 6)
      for (int y = 6; y < rows - 6; y++)
        if ((y / 2) % 2 == 0) {
          w.push_back({5, y});
          w.push_back({cols - 6, y});
        }
    return w;
  };

  auto reset = [&]() {
    std::deque<P> s;
    s.push_back({cols / 2, rows / 2});
    s.push_back({cols / 2 - 1, rows / 2});
    s.push_back({cols / 2 - 2, rows / 2});
    return s;
  };

  auto spawn_food = [&](const std::deque<P> &s, const std::vector<P> &walls) {
    P f;
    while (true) {
      f = {rand_cell(1, cols - 2), rand_cell(1, rows - 2)};
      bool clash = false;
      for (auto &p : s)
        if (p.x == f.x && p.y == f.y) {
          clash = true;
          break;
        }
      if (clash)
        continue;
      for (auto &w : walls)
        if (w.x == f.x && w.y == f.y) {
          clash = true;
          break;
        }
      if (!clash)
        return f;
    }
  };

  auto collides_walls = [&](const P &q, const std::vector<P> &walls) {
    for (auto &w : walls)
      if (w.x == q.x && w.y == q.y)
        return true;
    return false;
  };

  auto wrap_pos = [&](P &h) {
    if (wrap) {
      if (h.x < 0)
        h.x = cols - 1;
      if (h.x >= cols)
        h.x = 0;
      if (h.y < 0)
        h.y = rows - 1;
      if (h.y >= rows)
        h.y = 0;
    }
  };

  std::deque<P> snake = reset();
  std::deque<P> prev_snake = snake;
  Dir dir = R, next_dir = R;
  int score = 0;
  int best = load_highscore();
  int level = 1;
  std::vector<P> walls = level_walls(level);
  P food = spawn_food(snake, walls);
  bool running = true, paused = false, over = false;
  int tick_cur = tick_ms;
  Uint32 last_tick = SDL_GetTicks();
  bool show_settings = false;
  int sel_idx = 0;

  auto set_title = [&]() {
    std::string t = "Snake SDL2  |  Score: " + std::to_string(score) +
                    "  |  Best: " + std::to_string(best) +
                    "  |  Level: " + std::to_string(level);
    if (paused)
      t += "  |  Paused";
    if (over)
      t += "  |  Game Over (R to restart)";
    SDL_SetWindowTitle(win, t.c_str());
  };
  set_title();

  auto advance_level = [&]() {
    level = 1 + score / 5;
    if (level > 8)
      level = 8;
    walls = level_walls(level);
  };

  auto render_text = [&](const std::string &s, int x, int y, SDL_Color c) {
    if (!font)
      return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, s.c_str(), c);
    if (!surf)
      return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst{x, y, surf->w, surf->h};
    SDL_FreeSurface(surf);
    if (tex) {
      SDL_RenderCopy(ren, tex, nullptr, &dst);
      SDL_DestroyTexture(tex);
    }
  };

  auto color_str = [&](Col c) {
    char b[16];
    snprintf(b, sizeof(b), "#%02X%02X%02X", c.r, c.g, c.b);
    return std::string(b);
  };
  auto speed_str = [&](int ms) { return std::to_string(ms) + " ms"; };

  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        running = false;
      else if (e.type == SDL_KEYDOWN) {
        SDL_Keycode k = e.key.keysym.sym;
        if (k == SDLK_ESCAPE) {
          if (show_settings)
            show_settings = false;
          else if (over)
            running = false;
          else
            running = false;
        } else if (k == SDLK_q) {
          running = false;
        } else if (k == SDLK_s) {
          show_settings = !show_settings;
        } else if (show_settings) {
          if (k == SDLK_UP)
            sel_idx = (sel_idx + 5 - 1) % 5;
          else if (k == SDLK_DOWN)
            sel_idx = (sel_idx + 1) % 5;
          else if (k == SDLK_RETURN || k == SDLK_SPACE) {
            if (sel_idx == 0)
              wrap = !wrap;
            if (sel_idx == 1) {
              preset_idx = (preset_idx + 1) % presets.size();
              theme = presets[preset_idx];
            }
          } else if (k == SDLK_LEFT) {
            if (sel_idx == 2) {
              tick_ms = std::max(30, tick_ms - 5);
            }
            if (sel_idx == 3) {
              preset_idx = (preset_idx + presets.size() - 1) % presets.size();
              theme = presets[preset_idx];
            }
            if (sel_idx == 4) {
              C_OVERLAY_A = std::max(40, C_OVERLAY_A - 10);
            }
          } else if (k == SDLK_RIGHT) {
            if (sel_idx == 2) {
              tick_ms = std::min(400, tick_ms + 5);
            }
            if (sel_idx == 3) {
              preset_idx = (preset_idx + 1) % presets.size();
              theme = presets[preset_idx];
            }
            if (sel_idx == 4) {
              C_OVERLAY_A = std::min(240, C_OVERLAY_A + 10);
            }
          }
        } else if (k == SDLK_p && !over) {
          paused = !paused;
          set_title();
        } else if (k == SDLK_r) {
          if (score > best) {
            best = score;
            save_highscore(best);
          }
          snake = reset();
          prev_snake = snake;
          dir = R;
          next_dir = R;
          score = 0;
          level = 1;
          walls = level_walls(level);
          food = spawn_food(snake, walls);
          paused = false;
          over = false;
          tick_cur = tick_ms;
          last_tick = SDL_GetTicks();
          set_title();
        } else if (!over) {
          Dir prev = next_dir;
          if (k == SDLK_UP && dir != D)
            next_dir = U;
          else if (k == SDLK_DOWN && dir != U)
            next_dir = D;
          else if (k == SDLK_LEFT && dir != R)
            next_dir = L;
          else if (k == SDLK_RIGHT && dir != L)
            next_dir = R;
          if (next_dir != prev && !paused)
            Mix_PlayChannel(-1, s_move, 0);
        }
      } else if (e.type == SDL_WINDOWEVENT &&
                 e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        W = e.window.data1;
        H = e.window.data2;
      }
    }

    Uint32 now = SDL_GetTicks();
    bool stepped = false;
    if (!paused && !over && !show_settings &&
        now - last_tick >= (Uint32)tick_cur) {
      last_tick += tick_cur;
      prev_snake = snake;
      stepped = true;
      dir = next_dir;
      P head = snake.front();
      if (dir == U)
        head.y--;
      else if (dir == D)
        head.y++;
      else if (dir == L)
        head.x--;
      else if (dir == R)
        head.x++;
      wrap_pos(head);
      bool oob = head.x < 0 || head.x >= cols || head.y < 0 || head.y >= rows;
      if (oob || collides_walls(head, walls)) {
        over = true;
        Mix_PlayChannel(-1, s_hit, 0);
        if (score > best) {
          best = score;
          save_highscore(best);
          set_title();
        }
      } else {
        for (auto &p : snake)
          if (p.x == head.x && p.y == head.y) {
            over = true;
            break;
          }
        if (over) {
          Mix_PlayChannel(-1, s_hit, 0);
          if (score > best) {
            best = score;
            save_highscore(best);
            set_title();
          }
        }
      }
      if (!over) {
        bool ate = (head.x == food.x && head.y == food.y);
        snake.push_front(head);
        if (!ate) {
          snake.pop_back();
        } else {
          score++;
          if (tick_ms > 30)
            tick_ms = tick_ms - 3;
          tick_cur = tick_ms;
          advance_level();
          food = spawn_food(snake, walls);
          Mix_PlayChannel(-1, s_eat, 0);
          set_title();
        }
      }
    }

    double alpha = 0.0;
    if (!paused && !over) {
      Uint32 dt = SDL_GetTicks() - last_tick;
      if (dt > (Uint32)tick_cur)
        dt = tick_cur;
      alpha = (double)dt / (double)tick_cur;
    }

    SDL_SetRenderDrawColor(ren, theme.bg.r, theme.bg.g, theme.bg.b, 255);
    SDL_RenderClear(ren);

    int cell = std::min(W / cols, H / rows);
    if (cell < 6)
      cell = 6;
    int grid_w = cell * cols;
    int grid_h = cell * rows;
    int off_x = (W - grid_w) / 2;
    int off_y = (H - grid_h) / 2;

    SDL_Rect r;
    SDL_SetRenderDrawColor(ren, theme.grid.r, theme.grid.g, theme.grid.b, 255);
    for (int y = 0; y < rows; y++)
      for (int x = 0; x < cols; x++) {
        r = {off_x + x * cell, off_y + y * cell, cell - 1, cell - 1};
        SDL_RenderDrawRect(ren, &r);
      }

    SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
    for (auto &w : walls) {
      r = {off_x + w.x * cell + 1, off_y + w.y * cell + 1, cell - 2, cell - 2};
      SDL_RenderFillRect(ren, &r);
    }

    SDL_SetRenderDrawColor(ren, theme.food.r, theme.food.g, theme.food.b, 255);
    r = {off_x + food.x * cell + 1, off_y + food.y * cell + 1, cell - 2,
         cell - 2};
    SDL_RenderFillRect(ren, &r);

    auto lerp = [&](double a, double b, double t) { return a + (b - a) * t; };

    for (size_t i = 0; i < snake.size(); ++i) {
      int cx = snake[i].x, cy = snake[i].y;
      int px = (i < prev_snake.size()) ? prev_snake[i].x : cx;
      int py = (i < prev_snake.size()) ? prev_snake[i].y : cy;
      if (wrap) {
        int dx = cx - px;
        if (dx > 1)
          px += cols;
        if (dx < -1)
          px -= cols;
        int dy = cy - py;
        if (dy > 1)
          py += rows;
        if (dy < -1)
          py -= rows;
      }
      double fx = lerp(px, cx, alpha);
      double fy = lerp(py, cy, alpha);
      while (fx < 0)
        fx += cols;
      while (fy < 0)
        fy += rows;
      while (fx >= cols)
        fx -= cols;
      while (fy >= rows)
        fy -= rows;
      int rx = off_x + (int)std::round(fx * cell) + 1;
      int ry = off_y + (int)std::round(fy * cell) + 1;
      int rs = cell - 2;
      if (i == 0)
        SDL_SetRenderDrawColor(ren, theme.head.r, theme.head.g, theme.head.b,
                               255);
      else
        SDL_SetRenderDrawColor(ren, theme.body.r, theme.body.g, theme.body.b,
                               255);
      r = {rx, ry, rs, rs};
      SDL_RenderFillRect(ren, &r);
    }

    if (over || show_settings) {
      SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(ren, 0, 0, 0, C_OVERLAY_A);
      SDL_Rect overlay{0, 0, W, H};
      SDL_RenderFillRect(ren, &overlay);
    }

    if (show_settings) {
      int bx = off_x + 40, by = off_y + 40, bw = grid_w - 80, bh = grid_h - 80;
      SDL_SetRenderDrawColor(ren, 30, 30, 30, 230);
      SDL_Rect box{bx, by, bw, bh};
      SDL_RenderFillRect(ren, &box);
      SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
      SDL_RenderDrawRect(ren, &box);
      SDL_Color normal{220, 220, 220, 255};
      SDL_Color hint{180, 180, 180, 255};
      SDL_Color sel{255, 255, 255, 255};
      render_text("Settings", bx + 20, by + 20, sel);
      std::vector<std::string> rows_txt = {
          std::string("Wrap: ") + (wrap ? "On" : "Off"),
          std::string("Quick Preset: ") + std::to_string(preset_idx + 1) + "/" +
              std::to_string((int)presets.size()),
          std::string("Speed: ") + speed_str(tick_ms),
          std::string("Colors: bg ") + color_str(theme.bg) + " grid " +
              color_str(theme.grid),
          std::string("Overlay alpha: ") + std::to_string(C_OVERLAY_A)};
      int y = by + 70;
      for (int i = 0; i < (int)rows_txt.size(); ++i) {
        render_text(rows_txt[i], bx + 20, y, i == sel_idx ? sel : normal);
        y += 36;
      }
      render_text("Arrows to adjust, Enter/Space to toggle, S/Esc to close",
                  bx + 20, by + bh - 40, hint);
    }

    SDL_RenderPresent(ren);
    if (!stepped)
      SDL_Delay(1);
  }

  if (score > best)
    save_highscore(score);
  Mix_CloseAudio();
  if (font)
    TTF_CloseFont(font);
  TTF_Quit();
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}
