#include "config.h"

std::string base_data() {
  const char *xdg = getenv("XDG_DATA_HOME");
  const char *home = getenv("HOME");
  std::filesystem::path base =
      xdg && *xdg ? xdg
                  : (home ? (std::filesystem::path(home) / ".local/share")
                          : std::filesystem::path("."));
  std::filesystem::path dir = base / "snake_sdl2";
  std::filesystem::create_directories(dir);
  return dir.string();
}
std::string base_cfg() {
  const char *xdg = getenv("XDG_CONFIG_HOME");
  const char *home = getenv("HOME");
  std::filesystem::path base =
      xdg && *xdg ? xdg
                  : (home ? (std::filesystem::path(home) / ".config")
                          : std::filesystem::path("."));
  std::filesystem::path dir = base / "snake_sdl2";
  std::filesystem::create_directories(dir);
  return dir.string();
}
std::string hs_path(int profile) {
  return (std::filesystem::path(base_data()) /
          ("highscore_" + std::to_string(profile) + ".txt"))
      .string();
}
std::string cfg_path(int profile) {
  return (std::filesystem::path(base_cfg()) /
          ("config_" + std::to_string(profile) + ".toml"))
      .string();
}
std::string lb_path() {
  return (std::filesystem::path(base_data()) / "leaderboard.csv").string();
}
std::string lb_html_path() {
  return (std::filesystem::path(base_data()) / "leaderboard.html").string();
}
std::string lb_json_path() {
  return (std::filesystem::path(base_data()) / "leaderboard.json").string();
}

void defaults(AppConfig &c) {
  c.theme.bg = {16, 16, 16};
  c.theme.grid = {40, 40, 40};
  c.theme.food = {220, 50, 47};
  c.theme.head = {38, 139, 210};
  c.theme.body = {133, 153, 0};
  c.wrap = false;
  c.tick_ms = 120;
  c.cols = 32;
  c.rows = 24;
  c.overlay_alpha = 160;
  c.preset_idx = 0;
  c.profile = 1;
  c.seed = (uint32_t)std::chrono::high_resolution_clock::now()
               .time_since_epoch()
               .count();
}

bool load_cfg(AppConfig &c) {
  std::ifstream f(cfg_path(c.profile));
  if (!f)
    return false;
  std::string line;
  while (std::getline(f, line)) {
    std::string s = trim(line);
    if (s.empty() || s[0] == '#' || s[0] == ';')
      continue;
    size_t eq = s.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = trim(s.substr(0, eq)), val = trim(s.substr(eq + 1));
    if (!val.empty() && val.front() == '"' && val.back() == '"')
      val = val.substr(1, val.size() - 2);
    int iv;
    if (key == "wrap") {
      bool b;
      if (parse_bool(val, b))
        c.wrap = b;
    } else if (key == "speed_ms") {
      if (parse_int(val, iv))
        c.tick_ms = iv;
    } else if (key == "cols") {
      if (parse_int(val, iv))
        c.cols = iv;
    } else if (key == "rows") {
      if (parse_int(val, iv))
        c.rows = iv;
    } else if (key == "overlay_alpha") {
      if (parse_int(val, iv))
        c.overlay_alpha = iv;
    } else if (key == "preset") {
      if (parse_int(val, iv))
        c.preset_idx = iv;
    } else if (key == "seed") {
      if (parse_int(val, iv))
        c.seed = (uint32_t)iv;
    } else if (key == "bg") {
      Col t;
      if (parse_hex(val, t))
        c.theme.bg = t;
    } else if (key == "grid") {
      Col t;
      if (parse_hex(val, t))
        c.theme.grid = t;
    } else if (key == "food") {
      Col t;
      if (parse_hex(val, t))
        c.theme.food = t;
    } else if (key == "head") {
      Col t;
      if (parse_hex(val, t))
        c.theme.head = t;
    } else if (key == "body") {
      Col t;
      if (parse_hex(val, t))
        c.theme.body = t;
    }
  }
  return true;
}

bool save_cfg(const AppConfig &c) {
  std::ofstream f(cfg_path(c.profile), std::ios::trunc);
  if (!f)
    return false;
  f << "wrap = " << (c.wrap ? "true" : "false") << "\n";
  f << "speed_ms = " << c.tick_ms << "\n";
  f << "cols = " << c.cols << "\n";
  f << "rows = " << c.rows << "\n";
  f << "overlay_alpha = " << c.overlay_alpha << "\n";
  f << "preset = " << c.preset_idx << "\n";
  f << "seed = " << c.seed << "\n";
  f << "bg = \"" << str_hex(c.theme.bg) << "\"\n";
  f << "grid = \"" << str_hex(c.theme.grid) << "\"\n";
  f << "food = \"" << str_hex(c.theme.food) << "\"\n";
  f << "head = \"" << str_hex(c.theme.head) << "\"\n";
  f << "body = \"" << str_hex(c.theme.body) << "\"\n";
  return true;
}

int load_highscore(int profile) {
  std::ifstream f(hs_path(profile));
  int s = 0;
  if (f)
    f >> s;
  return s;
}
void save_highscore(int profile, int s) {
  std::ofstream f(hs_path(profile), std::ios::trunc);
  if (f)
    f << s;
}
