#include "leaderboard.h"

void append_lb(const LBEntry &e) {
  std::ofstream f(lb_path(), std::ios::app);
  if (!f)
    return;
  f << e.score << "," << e.profile << "," << e.seed << "," << e.cols << ","
    << e.rows << "," << e.wrap << "," << e.speed << "," << e.preset << ","
    << e.name << "," << e.ts << "\n";
}

std::vector<LBEntry> load_lb() {
  std::vector<LBEntry> v;
  std::ifstream f(lb_path());
  if (!f)
    return v;
  std::string line;
  while (std::getline(f, line)) {
    std::stringstream ss(line);
    std::string t;
    LBEntry e{};
    if (!std::getline(ss, t, ','))
      continue;
    e.score = std::stoi(t);
    if (!std::getline(ss, t, ','))
      continue;
    e.profile = std::stoi(t);
    if (!std::getline(ss, t, ','))
      continue;
    e.seed = (uint32_t)std::stoul(t);
    if (!std::getline(ss, t, ','))
      continue;
    e.cols = std::stoi(t);
    if (!std::getline(ss, t, ','))
      continue;
    e.rows = std::stoi(t);
    if (!std::getline(ss, t, ','))
      continue;
    e.wrap = std::stoi(t);
    if (!std::getline(ss, t, ','))
      continue;
    e.speed = std::stoi(t);
    if (!std::getline(ss, t, ','))
      continue;
    e.preset = std::stoi(t);
    if (!std::getline(ss, t, ','))
      continue;
    e.name = t;
    if (!std::getline(ss, t, ','))
      continue;
    e.ts = (uint64_t)std::stoull(t);
    v.push_back(e);
  }
  std::sort(v.begin(), v.end(), [](const LBEntry &a, const LBEntry &b) {
    if (a.score != b.score)
      return a.score > b.score;
    return a.ts < b.ts;
  });
  if (v.size() > 2000)
    v.resize(2000);
  return v;
}

bool export_html(const std::vector<LBEntry> &lb) {
  std::ofstream f(lb_html_path(), std::ios::trunc);
  if (!f)
    return false;
  f << "<!doctype html><html><head><meta charset=\"utf-8\"><meta "
       "name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    << "<title>Snake SDL2 Leaderboard</title>"
    << "<style>body{font-family:system-ui,-apple-system,Segoe "
       "UI,Roboto,Ubuntu,Cantarell,Noto "
       "Sans,sans-serif;background:#0f1115;color:#e6e6e6;margin:24px}"
    << "h1{font-size:20px;margin:0 0 "
       "16px}table{width:100%;border-collapse:collapse;border:1px solid "
       "#2b2f3a;border-radius:8px;overflow:hidden}"
    << "th,td{padding:10px 12px;border-bottom:1px solid "
       "#2b2f3a;font-size:14px;text-align:left}th{background:#171a21}tr:nth-"
       "child(even){background:#12141a}"
    << ".sub{color:#b0b5c0;font-size:12px}code{background:#171a21;padding:2px "
       "6px;border-radius:6px}</style></head><body>";
  f << "<h1>Snake SDL2 Leaderboard</h1>";
  f << "<div class=\"sub\">Rows: " << lb.size() << "</div>";
  f << "<table><thead><tr><th>#</th><th>Name</th><th>Score</th><th>Profile</"
       "th><th>Grid</th><th>Wrap</th><th>Speed</th><th>Preset</th><th>Seed</"
       "th><th>Time</th></tr></thead><tbody>";
  int rank = 1;
  for (const auto &e : lb) {
    f << "<tr><td>" << rank++ << "</td><td>" << e.name << "</td><td>" << e.score
      << "</td><td>" << e.profile << "</td>"
      << "<td>" << e.cols << "×" << e.rows << "</td><td>"
      << (e.wrap ? "On" : "Off") << "</td><td>" << e.speed << " ms</td>"
      << "<td>" << e.preset << "</td><td><code>" << e.seed << "</code></td><td>"
      << e.ts << "</td></tr>";
    if (rank > 200)
      break;
  }
  f << "</tbody></table></body></html>";
  f.flush();
  return f.good();
}

bool export_json(const std::vector<LBEntry> &lb) {
  std::ofstream f(lb_json_path(), std::ios::trunc);
  if (!f)
    return false;
  f << "{\n  \"entries\": [\n";
  for (size_t i = 0; i < lb.size() && i < 1000; i++) {
    const auto &e = lb[i];
    f << "    {\"rank\": " << (int)(i + 1) << ", \"name\": \"" << e.name
      << "\", \"score\": " << e.score << ", \"profile\": " << e.profile
      << ", \"seed\": " << e.seed << ", \"cols\": " << e.cols
      << ", \"rows\": " << e.rows << ", \"wrap\": " << (e.wrap ? true : false)
      << ", \"speed_ms\": " << e.speed << ", \"preset\": " << e.preset
      << ", \"timestamp\": " << e.ts << "}";
    if (i + 1 < lb.size() && i + 1 < 1000)
      f << ",";
    f << "\n";
  }
  f << "  ]\n}\n";
  f.flush();
  return f.good();
}
