#include "challenge.h"

std::string make_challenge(uint32_t seed, int cols, int rows, bool wrap,
                           int speed, int preset) {
  std::stringstream ss;
  ss << "v1:" << seed << ":" << cols << ":" << rows << ":" << (wrap ? 1 : 0)
     << ":" << speed << ":" << preset;
  return ss.str();
}

bool parse_challenge(const std::string &s, uint32_t &seed, int &cols, int &rows,
                     bool &wrap, int &speed, int &preset) {
  if (s.rfind("v1:", 0) != 0)
    return false;
  std::vector<std::string> parts;
  std::string t;
  std::stringstream ss(s);
  while (std::getline(ss, t, ':'))
    parts.push_back(t);
  if (parts.size() != 7)
    return false;
  try {
    seed = (uint32_t)std::stoul(parts[1]);
    cols = std::stoi(parts[2]);
    rows = std::stoi(parts[3]);
    wrap = std::stoi(parts[4]) != 0;
    speed = std::stoi(parts[5]);
    preset = std::stoi(parts[6]);
    return true;
  } catch (...) {
    return false;
  }
}
