#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
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

inline std::string trim(const std::string &s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace((unsigned char)s[a]))
    ++a;
  while (b > a && std::isspace((unsigned char)s[b - 1]))
    --b;
  return s.substr(a, b - a);
}
inline bool parse_bool(const std::string &v, bool &out) {
  std::string t = v;
  std::transform(t.begin(), t.end(), t.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (t == "true" || t == "1") {
    out = true;
    return true;
  }
  if (t == "false" || t == "0") {
    out = false;
    return true;
  }
  return false;
}
inline bool parse_int(const std::string &v, int &out) {
  try {
    out = std::stoi(v);
    return true;
  } catch (...) {
    return false;
  }
}
inline bool parse_hex(const std::string &s, Col &out) {
  if (s.size() != 7 || s[0] != '#')
    return false;
  auto h = [&](char c) {
    if ('0' <= c && c <= '9')
      return c - '0';
    if ('a' <= c && c <= 'f')
      return 10 + c - 'a';
    if ('A' <= c && c <= 'F')
      return 10 + c - 'A';
    return 0;
  };
  out.r = h(s[1]) * 16 + h(s[2]);
  out.g = h(s[3]) * 16 + h(s[4]);
  out.b = h(s[5]) * 16 + h(s[6]);
  return true;
}
inline std::string str_hex(Col c) {
  char b[16];
  snprintf(b, sizeof(b), "#%02X%02X%02X", c.r, c.g, c.b);
  return std::string(b);
}
inline uint64_t now_ts() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
inline std::string user_name() {
  const char *u = getenv("USER");
  if (u && *u)
    return u;
  const char *ln = getenv("LOGNAME");
  if (ln && *ln)
    return ln;
  return "player";
}
inline std::string argval(int argc, char **argv, const std::string &key) {
  std::string k = "--" + key + "=";
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a.rfind(k, 0) == 0)
      return a.substr(k.size());
  }
  return "";
}
inline bool hasflag(int argc, char **argv, const std::string &key) {
  std::string k = "--" + key;
  for (int i = 1; i < argc; ++i)
    if (k == argv[i])
      return true;
  return false;
}
