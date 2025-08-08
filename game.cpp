#include "game.h"

static std::vector<P> border_walls(int cols, int rows) {
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
}
static std::vector<P> level_walls(int lvl, int cols, int rows) {
  std::vector<P> w = border_walls(cols, rows);
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
}
static std::deque<P> reset_snake(int cols, int rows) {
  std::deque<P> s;
  s.push_back({cols / 2, rows / 2});
  s.push_back({cols / 2 - 1, rows / 2});
  s.push_back({cols / 2 - 2, rows / 2});
  return s;
}
static bool collides_walls(const P &q, const std::vector<P> &walls) {
  for (auto &w : walls)
    if (w.x == q.x && w.y == q.y)
      return true;
  return false;
}

Game::Game() {
  defaults(cfg);
  theme = cfg.theme;
  win = nullptr;
  ren = nullptr;
  font = nullptr;
  dir = R;
  next_dir = R;
  score = 0;
  best = 0;
  level = 1;
  running = true;
  paused = false;
  over = false;
  show_settings = false;
  show_lb = false;
  tick_cur = cfg.tick_ms;
  last_tick = 0;
  sel_idx = 0;
  W = 960;
  H = 720;
  last_copy_ticks = 0;
  presets = {{{16, 16, 16},
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
}

bool Game::init_from_args(int argc, char **argv) {
  int tmp;
  if (!argval(argc, argv, "profile").empty()) {
    if (parse_int(argval(argc, argv, "profile"), tmp))
      cfg.profile = std::clamp(tmp, 1, 5);
  }
  load_cfg(cfg);
  if (!argval(argc, argv, "bg").empty()) {
    Col t;
    if (parse_hex(argval(argc, argv, "bg"), t))
      cfg.theme.bg = t;
  }
  if (!argval(argc, argv, "grid").empty()) {
    Col t;
    if (parse_hex(argval(argc, argv, "grid"), t))
      cfg.theme.grid = t;
  }
  if (!argval(argc, argv, "food").empty()) {
    Col t;
    if (parse_hex(argval(argc, argv, "food"), t))
      cfg.theme.food = t;
  }
  if (!argval(argc, argv, "head").empty()) {
    Col t;
    if (parse_hex(argval(argc, argv, "head"), t))
      cfg.theme.head = t;
  }
  if (!argval(argc, argv, "body").empty()) {
    Col t;
    if (parse_hex(argval(argc, argv, "body"), t))
      cfg.theme.body = t;
  }
  if (!argval(argc, argv, "cols").empty()) {
    if (parse_int(argval(argc, argv, "cols"), tmp))
      cfg.cols = std::clamp(tmp, 8, 96);
  }
  if (!argval(argc, argv, "rows").empty()) {
    if (parse_int(argval(argc, argv, "rows"), tmp))
      cfg.rows = std::clamp(tmp, 8, 72);
  }
  if (!argval(argc, argv, "speed").empty()) {
    if (parse_int(argval(argc, argv, "speed"), tmp))
      cfg.tick_ms = std::clamp(tmp, 30, 400);
  }
  if (!argval(argc, argv, "seed").empty()) {
    long long s = 0;
    try {
      s = std::stoll(argval(argc, argv, "seed"));
    } catch (...) {
      s = 0;
    }
    if (s < 0)
      s = -s;
    cfg.seed = (uint32_t)(s % 0xffffffffu);
  }
  if (hasflag(argc, argv, "wrap"))
    cfg.wrap = true;
  if (hasflag(argc, argv, "no-wrap"))
    cfg.wrap = false;
  if (!argval(argc, argv, "challenge").empty()) {
    uint32_t sd;
    int ccols, crows, cspeed, cpreset;
    bool cwrap;
    if (parse_challenge(argval(argc, argv, "challenge"), sd, ccols, crows,
                        cwrap, cspeed, cpreset)) {
      cfg.seed = sd;
      cfg.cols = std::clamp(ccols, 8, 96);
      cfg.rows = std::clamp(crows, 8, 72);
      cfg.wrap = cwrap;
      cfg.tick_ms = std::clamp(cspeed, 30, 400);
      cfg.preset_idx = std::clamp(cpreset, 0, (int)presets.size() - 1);
    }
  }
  theme = cfg.theme;
  if (cfg.preset_idx >= 0 && cfg.preset_idx < (int)presets.size())
    theme = presets[cfg.preset_idx];
  rng.seed(cfg.seed);
  return true;
}

bool Game::init_sdl() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
    return false;
  if (TTF_Init() != 0)
    return false;
  win = SDL_CreateWindow("Snake SDL2", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, W, H,
                         SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!win)
    return false;
  ren = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!ren)
    return false;
  if (!audio.init())
    return false;
  font = TTF_OpenFontIndex("/usr/share/fonts/TTF/DejaVuSans.ttf", 18, 0);
  if (!font)
    font = TTF_OpenFontIndex("/usr/share/fonts/dejavu/DejaVuSans.ttf", 18, 0);
  if (!font)
    font = TTF_OpenFontIndex("/usr/share/fonts/TTF/LiberationSans-Regular.ttf",
                             18, 0);
  best = load_highscore(cfg.profile);
  snake = reset_snake(cfg.cols, cfg.rows);
  prev_snake = snake;
  dir = R;
  next_dir = R;
  std::uniform_int_distribution<int> dx(1, cfg.cols - 2), dy(1, cfg.rows - 2);
  walls = level_walls(level, cfg.cols, cfg.rows);
  auto rand_cell = [&](int a, int b) {
    std::uniform_int_distribution<int> d(a, b);
    return d(rng);
  };
  while (true) {
    P f{rand_cell(1, cfg.cols - 2), rand_cell(1, cfg.rows - 2)};
    bool clash = false;
    for (auto &p : snake)
      if (p.x == f.x && p.y == f.y) {
        clash = true;
        break;
      }
    for (auto &w : walls)
      if (w.x == f.x && w.y == f.y) {
        clash = true;
        break;
      }
    if (!clash) {
      food = f;
      break;
    }
  }
  tick_cur = cfg.tick_ms;
  last_tick = SDL_GetTicks();
  return true;
}

void Game::loop() {
  auto set_title = [&]() {
    std::string t = "Snake SDL2 | Score: " + std::to_string(score) +
                    " | Best[" + std::to_string(cfg.profile) +
                    "]: " + std::to_string(best) +
                    " | Level: " + std::to_string(level) +
                    " | Seed: " + std::to_string(cfg.seed);
    if (paused)
      t += " | Paused";
    if (over)
      t += " | Game Over (R to restart)";
    SDL_SetWindowTitle(win, t.c_str());
  };
  auto apply_preset = [&]() {
    if (cfg.preset_idx >= 0 && cfg.preset_idx < (int)presets.size())
      theme = presets[cfg.preset_idx];
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
  auto rand_cell = [&](int a, int b) {
    std::uniform_int_distribution<int> d(a, b);
    return d(rng);
  };
  auto spawn_food = [&]() {
    for (;;) {
      P f{rand_cell(1, cfg.cols - 2), rand_cell(1, cfg.rows - 2)};
      bool clash = false;
      for (auto &p : snake)
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
      if (!clash) {
        food = f;
        return;
      }
    }
  };
  auto wrap_pos = [&](P &h) {
    if (cfg.wrap) {
      if (h.x < 0)
        h.x = cfg.cols - 1;
      if (h.x >= cfg.cols)
        h.x = 0;
      if (h.y < 0)
        h.y = cfg.rows - 1;
      if (h.y >= cfg.rows)
        h.y = 0;
    }
  };
  auto advance_level = [&]() {
    level = 1 + score / 5;
    if (level > 8)
      level = 8;
    walls = level_walls(level, cfg.cols, cfg.rows);
  };
  auto submit_score = [&]() {
    LBEntry e;
    e.score = score;
    e.profile = cfg.profile;
    e.seed = cfg.seed;
    e.cols = cfg.cols;
    e.rows = cfg.rows;
    e.wrap = cfg.wrap ? 1 : 0;
    e.speed = cfg.tick_ms;
    e.preset = cfg.preset_idx;
    e.name = user_name();
    e.ts = now_ts();
    append_lb(e);
  };
  set_title();

  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        running = false;
      else if (e.type == SDL_KEYDOWN) {
        SDL_Keycode k = e.key.keysym.sym;
        if (k == SDLK_ESCAPE) {
          if (show_lb)
            show_lb = false;
          else if (show_settings)
            show_settings = false;
          else
            running = false;
        } else if (k == SDLK_q) {
          running = false;
        } else if (k == SDLK_s) {
          show_settings = !show_settings;
          show_lb = false;
        } else if (k == SDLK_l) {
          show_lb = !show_lb;
          show_settings = false;
        } else if (k == SDLK_c) {
          last_challenge =
              make_challenge(cfg.seed, cfg.cols, cfg.rows, cfg.wrap,
                             cfg.tick_ms, cfg.preset_idx);
          SDL_SetClipboardText(last_challenge.c_str());
          last_copy_ticks = SDL_GetTicks();
        } else if (k == SDLK_e) {
          auto lb = load_lb();
          if (export_html(lb)) {
            std::string p = lb_html_path();
            std::string cmd = "xdg-open \"" + p + "\" >/dev/null 2>&1 &";
            system(cmd.c_str());
          }
        } else if (k == SDLK_j) {
          auto lb = load_lb();
          export_json(lb);
        } else if (k == SDLK_n) {
          cfg.seed = (uint32_t)std::chrono::high_resolution_clock::now()
                         .time_since_epoch()
                         .count();
          rng.seed(cfg.seed);
          snake = reset_snake(cfg.cols, cfg.rows);
          prev_snake = snake;
          dir = R;
          next_dir = R;
          score = 0;
          level = 1;
          walls = level_walls(level, cfg.cols, cfg.rows);
          spawn_food();
          paused = false;
          over = false;
          tick_cur = cfg.tick_ms;
          last_tick = SDL_GetTicks();
          set_title();
        } else if (show_settings) {
          if (k == SDLK_UP)
            sel_idx = (sel_idx + 11 - 1) % 11;
          else if (k == SDLK_DOWN)
            sel_idx = (sel_idx + 1) % 11;
          else if (k == SDLK_RETURN || k == SDLK_SPACE) {
            if (sel_idx == 0)
              cfg.wrap = !cfg.wrap;
            if (sel_idx == 3)
              save_cfg(cfg);
            if (sel_idx == 4) {
              AppConfig t = cfg;
              load_cfg(t);
              cfg = t;
              theme = cfg.theme;
              apply_preset();
            }
            if (sel_idx == 5) {
              defaults(cfg);
              theme = cfg.theme;
            }
            if (sel_idx == 6) {
              apply_preset();
            }
            if (sel_idx == 9) {
              save_cfg(cfg);
            }
            if (sel_idx == 10) {
              snake = reset_snake(cfg.cols, cfg.rows);
              prev_snake = snake;
              dir = R;
              next_dir = R;
              score = 0;
              level = 1;
              walls = level_walls(level, cfg.cols, cfg.rows);
              spawn_food();
              paused = false;
              over = false;
              tick_cur = cfg.tick_ms;
              last_tick = SDL_GetTicks();
              set_title();
            }
          } else if (k == SDLK_LEFT) {
            if (sel_idx == 1)
              cfg.tick_ms = std::max(30, cfg.tick_ms - 5);
            if (sel_idx == 2)
              cfg.overlay_alpha = std::max(40, cfg.overlay_alpha - 10);
            if (sel_idx == 6)
              cfg.preset_idx =
                  (cfg.preset_idx + (int)presets.size() - 1) % presets.size();
          } else if (k == SDLK_RIGHT) {
            if (sel_idx == 1)
              cfg.tick_ms = std::min(400, cfg.tick_ms + 5);
            if (sel_idx == 2)
              cfg.overlay_alpha = std::min(240, cfg.overlay_alpha + 10);
            if (sel_idx == 6)
              cfg.preset_idx = (cfg.preset_idx + 1) % presets.size();
          }
        } else if (k == SDLK_p && !over) {
          paused = !paused;
          set_title();
        } else if (k == SDLK_r) {
          if (score > best) {
            best = score;
            save_highscore(cfg.profile, best);
          }
          snake = reset_snake(cfg.cols, cfg.rows);
          prev_snake = snake;
          dir = R;
          next_dir = R;
          score = 0;
          level = 1;
          walls = level_walls(level, cfg.cols, cfg.rows);
          spawn_food();
          paused = false;
          over = false;
          tick_cur = cfg.tick_ms;
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
            Mix_PlayChannel(-1, audio.move, 0);
        }
      } else if (e.type == SDL_WINDOWEVENT &&
                 e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        W = e.window.data1;
        H = e.window.data2;
      }
    }

    Uint32 now = SDL_GetTicks();
    bool stepped = false;
    if (!paused && !over && !show_settings && !show_lb &&
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
      bool oob =
          head.x < 0 || head.x >= cfg.cols || head.y < 0 || head.y >= cfg.rows;
      if (oob || collides_walls(head, walls)) {
        over = true;
        Mix_PlayChannel(-1, audio.hit, 0);
        if (score > best) {
          best = score;
          save_highscore(cfg.profile, best);
        }
        submit_score();
        set_title();
      } else {
        for (auto &p : snake)
          if (p.x == head.x && p.y == head.y) {
            over = true;
            break;
          }
        if (over) {
          Mix_PlayChannel(-1, audio.hit, 0);
          if (score > best) {
            best = score;
            save_highscore(cfg.profile, best);
          }
          submit_score();
          set_title();
        }
      }
      if (!over) {
        bool ate = (head.x == food.x && head.y == food.y);
        snake.push_front(head);
        if (!ate)
          snake.pop_back();
        else {
          score++;
          if (cfg.tick_ms > 30)
            cfg.tick_ms -= 3;
          tick_cur = cfg.tick_ms;
          level = 1 + score / 5;
          if (level > 8)
            level = 8;
          walls = level_walls(level, cfg.cols, cfg.rows);
          spawn_food();
          Mix_PlayChannel(-1, audio.eat, 0);
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
    int cell = std::min(W / cfg.cols, H / cfg.rows);
    if (cell < 6)
      cell = 6;
    int grid_w = cell * cfg.cols, grid_h = cell * cfg.rows;
    int off_x = (W - grid_w) / 2, off_y = (H - grid_h) / 2;

    SDL_Rect r;
    SDL_SetRenderDrawColor(ren, theme.grid.r, theme.grid.g, theme.grid.b, 255);
    for (int y = 0; y < cfg.rows; y++)
      for (int x = 0; x < cfg.cols; x++) {
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
      if (cfg.wrap) {
        int dx = cx - px;
        if (dx > 1)
          px += cfg.cols;
        if (dx < -1)
          px -= cfg.cols;
        int dy = cy - py;
        if (dy > 1)
          py += cfg.rows;
        if (dy < -1)
          py -= cfg.rows;
      }
      double fx = lerp(px, cx, alpha), fy = lerp(py, cy, alpha);
      while (fx < 0)
        fx += cfg.cols;
      while (fy < 0)
        fy += cfg.rows;
      while (fx >= cfg.cols)
        fx -= cfg.cols;
      while (fy >= cfg.rows)
        fy -= cfg.rows;
      int rx = off_x + (int)std::round(fx * cell) + 1,
          ry = off_y + (int)std::round(fy * cell) + 1, rs = cell - 2;
      if (i == 0)
        SDL_SetRenderDrawColor(ren, theme.head.r, theme.head.g, theme.head.b,
                               255);
      else
        SDL_SetRenderDrawColor(ren, theme.body.r, theme.body.g, theme.body.b,
                               255);
      r = {rx, ry, rs, rs};
      SDL_RenderFillRect(ren, &r);
    }

    if (over || show_settings || show_lb) {
      SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(ren, 0, 0, 0, cfg.overlay_alpha);
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
      SDL_Color normal{220, 220, 220, 255}, hint{180, 180, 180, 255},
          sel{255, 255, 255, 255};
      std::vector<std::string> rows_txt = {
          std::string("Wrap: ") + (cfg.wrap ? "On" : "Off"),
          std::string("Speed: ") + std::to_string(cfg.tick_ms) + " ms",
          std::string("Overlay alpha: ") + std::to_string(cfg.overlay_alpha),
          std::string("Save config (profile ") + std::to_string(cfg.profile) +
              ")",
          std::string("Load config (profile ") + std::to_string(cfg.profile) +
              ")",
          std::string("Reset to defaults"),
          std::string("Preset: ") + std::to_string(cfg.preset_idx + 1) + "/" +
              std::to_string((int)presets.size()),
          std::string("Apply preset"),
          std::string("Apply and restart"),
          std::string("Export leaderboard HTML (E)"),
          std::string("Export leaderboard JSON (J)")};
      int y = by + 30 + 10;
      render_text("Settings", bx + 20, by + 10, sel);
      for (int i = 0; i < (int)rows_txt.size(); ++i) {
        render_text(rows_txt[i], bx + 20, y, i == sel_idx ? sel : normal);
        y += 36;
      }
      render_text("Arrows adjust, Enter/Space activate, S/Esc close, E export, "
                  "J export JSON, L leaderboard",
                  bx + 20, by + bh - 40, hint);
    }

    if (show_lb) {
      auto lb = load_lb();
      int bx = off_x + 40, by = off_y + 40, bw = grid_w - 80, bh = grid_h - 80;
      SDL_SetRenderDrawColor(ren, 30, 30, 30, 230);
      SDL_Rect box{bx, by, bw, bh};
      SDL_RenderFillRect(ren, &box);
      SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
      SDL_RenderDrawRect(ren, &box);
      SDL_Color headc{255, 255, 255, 255}, rowc{220, 220, 220, 255},
          hint{180, 180, 180, 255};
      render_text("Leaderboard (Top 20)", bx + 20, by + 10, headc);
      int show = std::min(20, (int)lb.size()), y = by + 40;
      for (int i = 0; i < show; i++) {
        const auto &e = lb[i];
        std::stringstream line;
        line << i + 1 << ". " << e.name << "  S:" << e.score
             << "  P:" << e.profile << "  " << e.cols << "x" << e.rows
             << (e.wrap ? " W" : " B") << "  " << e.speed
             << "ms  pr:" << e.preset << "  sd:" << e.seed;
        render_text(line.str(), bx + 20, y, rowc);
        y += 28;
      }
      render_text("L close • E export HTML • J export JSON • C copy challenge",
                  bx + 20, by + bh - 40, hint);
    }

    if (!last_challenge.empty()) {
      uint32_t t = SDL_GetTicks();
      if (t - last_copy_ticks < 2000) {
        SDL_Color c{255, 255, 255, 255};
        render_text(std::string("Challenge copied: ") + last_challenge,
                    off_x + 20, off_y + grid_h - 30, c);
      }
    }

    SDL_RenderPresent(ren);
    if (!stepped)
      SDL_Delay(1);
  }
}

void Game::shutdown() {
  if (score > best)
    save_highscore(cfg.profile, score);
  audio.quit();
  if (font)
    TTF_CloseFont(font);
  TTF_Quit();
  if (ren)
    SDL_DestroyRenderer(ren);
  if (win)
    SDL_DestroyWindow(win);
  SDL_Quit();
}
