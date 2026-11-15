#pragma once
// Procedural one-shot audio. Windows: WinMM waveOut + software mixer thread.
// Every sound is synthesized at init — no asset files. Degrades to silence
// if the device can't be opened (HUD reports AUDIO OFF).
#include <string>

class Audio {
public:
  bool init();
  void shutdown();
  bool ok() const;
  void play(const std::string& name, float vol = 1.f, float pitch = 1.f);
  void setVolume(float v);
  void setMuted(bool m);
private:
  struct Impl;
  Impl* m_ = nullptr;
};
