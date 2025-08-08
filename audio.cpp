#include "audio.h"

static std::vector<Uint8> synth(int hz, int ms, int volume, int sample_rate) {
  int samples = sample_rate * ms / 1000;
  std::vector<Uint8> buf(samples * 2);
  double t = 0.0, dt = 1.0 / sample_rate, PI = 3.14159265358979323846;
  for (int i = 0; i < samples; ++i) {
    double v = sin(2.0 * PI * hz * t);
    int val = (int)(v * volume);
    buf[i * 2] = Uint8(val & 0xff);
    buf[i * 2 + 1] = Uint8((val >> 8) & 0xff);
    t += dt;
  }
  return buf;
}

bool Audio::init() {
  rate = 22050;
  if (Mix_OpenAudio(rate, AUDIO_S16SYS, 1, 1024) != 0)
    return false;
  auto pcm_eat = synth(880, 80, 2000, rate);
  auto pcm_hit = synth(110, 250, 2500, rate);
  auto pcm_move = synth(660, 30, 1200, rate);
  eat = Mix_QuickLoad_RAW(pcm_eat.data(), (Uint32)pcm_eat.size());
  hit = Mix_QuickLoad_RAW(pcm_hit.data(), (Uint32)pcm_hit.size());
  move = Mix_QuickLoad_RAW(pcm_move.data(), (Uint32)pcm_move.size());
  return eat && hit && move;
}

void Audio::quit() { Mix_CloseAudio(); }
