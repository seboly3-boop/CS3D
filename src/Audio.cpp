#include "Audio.h"
#include <functional>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <random>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>

static const int SR = 48000;
static const int BUF_FRAMES = 480;   // 10 ms
static const int NUM_BUFS = 4;

struct Voice {
  const int16_t* data = nullptr;
  size_t len = 0;
  float pos = 0, rate = 1, vol = 1;
  bool active = false;
};

struct Audio::Impl {
  std::unordered_map<std::string, std::vector<int16_t>> bank;
  HWAVEOUT hwo = nullptr;
  HANDLE evt = nullptr;
  WAVEHDR hdr[NUM_BUFS]{};
  int16_t buf[NUM_BUFS][BUF_FRAMES]{};
  Voice voices[32];
  CRITICAL_SECTION cs;
  HANDLE thread = nullptr;
  volatile LONG stop = 0;
  float volume = 0.6f;
  bool muted = false;
  bool ready = false;
};

static DWORD WINAPI mixThread(LPVOID param) {
  Audio::Impl* m = (Audio::Impl*)param;
  while (!m->stop) {
    WaitForSingleObject(m->evt, 60);
    if (m->stop) break;
    for (int b = 0; b < NUM_BUFS; ++b) {
      if (!(m->hdr[b].dwFlags & WHDR_DONE)) continue;
      EnterCriticalSection(&m->cs);
      float master = m->muted ? 0.f : m->volume;
      for (int f = 0; f < BUF_FRAMES; ++f) {
        float sum = 0;
        for (auto& v : m->voices) {
          if (!v.active) continue;
          size_t i0 = (size_t)v.pos;
          if (i0 + 1 >= v.len) { v.active = false; continue; }
          float frac = v.pos - (float)i0;
          float s = (v.data[i0] * (1 - frac) + v.data[i0 + 1] * frac) / 32768.f;
          sum += s * v.vol;
          v.pos += v.rate;
        }
        sum *= master;
        sum = std::max(-1.f, std::min(1.f, sum));
        m->buf[b][f] = (int16_t)(sum * 32767.f);
      }
      LeaveCriticalSection(&m->cs);
      waveOutWrite(m->hwo, &m->hdr[b], sizeof(WAVEHDR));
    }
  }
  return 0;
}

// ---- synthesis helpers -----------------------------------------------------
struct Synth {
  std::mt19937 rng{1234};
  float noise() { return (float)((int)(rng() % 20001) - 10000) / 10000.f; }
  static float env(float t, float decay) { return std::exp(-t * decay); }
};

static std::vector<int16_t> render(float dur, const std::function<float(float, Synth&)>& fn) {
  Synth s;
  std::vector<int16_t> out((size_t)(dur * SR));
  float lp = 0;
  for (size_t i = 0; i < out.size(); ++i) {
    float t = (float)i / SR;
    float v = fn(t, s);
    v = std::max(-1.f, std::min(1.f, v));
    out[i] = (int16_t)(v * 32000.f);
    (void)lp;
  }
  return out;
}

static void buildBank(std::unordered_map<std::string, std::vector<int16_t>>& bank) {
  float lpN = 0, lpS = 0;
  auto shot = [&](float dur, float bodyF0, float bodyF1, float nDecay, float lpA, float crackDecay) {
    lpN = lpS = 0;
    return render(dur, [&](float t, Synth& s) {
      float n = s.noise();
      lpN += lpA * (n - lpN);
      float crack = n - lpN;
      float f = bodyF0 + (bodyF1 - bodyF0) * std::min(1.f, t * 18.f);
      lpS += 0.12f * (std::sin(6.2831853f * f * t) - lpS);
      return lpN * Synth::env(t, nDecay) * 0.95f
           + std::sin(6.2831853f * f * t) * Synth::env(t, 30.f) * 0.65f
           + crack * Synth::env(t, crackDecay) * 0.5f;
    });
  };
  bank["rifle"]  = shot(0.18f, 150, 55, 38, 0.28f, 260);
  bank["pistol"] = shot(0.14f, 190, 70, 46, 0.40f, 300);

  lpN = 0;
  bank["step"] = render(0.09f, [&](float t, Synth& s) {
    lpN += 0.12f * (s.noise() - lpN);
    return lpN * Synth::env(t, 55) * 0.55f + std::sin(6.2831853f * 85 * t) * Synth::env(t, 45) * 0.35f;
  });
  lpN = 0;
  bank["jump"] = render(0.12f, [&](float t, Synth& s) {
    lpN += 0.25f * (s.noise() - lpN);
    return lpN * Synth::env(t, 26) * 0.30f;
  });
  lpN = 0;
  bank["land"] = render(0.16f, [&](float t, Synth& s) {
    lpN += 0.15f * (s.noise() - lpN);
    return lpN * Synth::env(t, 34) * 0.85f + std::sin(6.2831853f * 65 * t) * Synth::env(t, 26) * 0.7f;
  });
  bank["dry"] = render(0.035f, [&](float t, Synth& s) {
    return std::sin(6.2831853f * 1900 * t) * Synth::env(t, 320) * 0.35f
         + s.noise() * Synth::env(t, 500) * 0.30f;
  });
  lpN = 0;
  bank["magout"] = render(0.09f, [&](float t, Synth& s) {
    lpN += 0.5f * (s.noise() - lpN);
    return std::sin(6.2831853f * 820 * t) * Synth::env(t, 90) * 0.4f
         + lpN * Synth::env(t, 140) * 0.4f + std::sin(6.2831853f * 2400 * t) * Synth::env(t, 400) * 0.2f;
  });
  lpN = 0;
  bank["magin"] = render(0.09f, [&](float t, Synth& s) {
    lpN += 0.4f * (s.noise() - lpN);
    return std::sin(6.2831853f * 560 * t) * Synth::env(t, 80) * 0.5f
         + lpN * Synth::env(t, 120) * 0.35f + std::sin(6.2831853f * 1800 * t) * Synth::env(t, 350) * 0.25f;
  });
  bank["bolt"] = render(0.08f, [&](float t, Synth& s) {
    float v = std::sin(6.2831853f * 1300 * t) * Synth::env(t, 200) * 0.4f
            + s.noise() * Synth::env(t, 300) * 0.2f;
    if (t > 0.03f) v += std::sin(6.2831853f * 900 * (t - 0.03f)) * Synth::env(t - 0.03f, 220) * 0.4f;
    return v;
  });
  bank["hit"]  = render(0.05f, [&](float t, Synth&) {
    return std::sin(6.2831853f * 1150 * t) * Synth::env(t, 140) * 0.5f;
  });
  bank["head"] = render(0.10f, [&](float t, Synth&) {
    return std::sin(6.2831853f * 1650 * t) * Synth::env(t, 80) * 0.5f
         + std::sin(6.2831853f * 2200 * t) * Synth::env(t, 110) * 0.3f;
  });
  bank["kill"] = render(0.24f, [&](float t, Synth&) {
    float v = 0;
    if (t < 0.12f) v += std::sin(6.2831853f * 780 * t) * Synth::env(t, 30) * 0.4f;
    if (t > 0.07f) v += std::sin(6.2831853f * 1170 * (t - 0.07f)) * Synth::env(t - 0.07f, 26) * 0.45f;
    return v;
  });
  lpN = 0;
  bank["switch"] = render(0.14f, [&](float t, Synth& s) {
    lpN += 0.22f * (s.noise() - lpN);
    return lpN * Synth::env(t, 28) * 0.45f;
  });
}

bool Audio::init() {
  m_ = new Impl();
  InitializeCriticalSection(&m_->cs);
  buildBank(m_->bank);
  m_->evt = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  WAVEFORMATEX fmt{};
  fmt.wFormatTag = WAVE_FORMAT_PCM;
  fmt.nChannels = 1;
  fmt.nSamplesPerSec = SR;
  fmt.wBitsPerSample = 16;
  fmt.nBlockAlign = 2;
  fmt.nAvgBytesPerSec = SR * 2;
  MMRESULT r = waveOutOpen(&m_->hwo, WAVE_MAPPER, &fmt, (DWORD_PTR)m_->evt, 0, CALLBACK_EVENT);
  if (r != MMSYSERR_NOERROR) {
    fprintf(stderr, "[Audio] waveOutOpen failed (%u) — running silent\n", (unsigned)r);
    return false;
  }
  for (int b = 0; b < NUM_BUFS; ++b) {
    m_->hdr[b].lpData = (LPSTR)m_->buf[b];
    m_->hdr[b].dwBufferLength = BUF_FRAMES * 2;
    waveOutPrepareHeader(m_->hwo, &m_->hdr[b], sizeof(WAVEHDR));
    waveOutWrite(m_->hwo, &m_->hdr[b], sizeof(WAVEHDR));
  }
  m_->thread = CreateThread(nullptr, 0, mixThread, m_, 0, nullptr);
  m_->ready = true;
  return true;
}

bool Audio::ok() const { return m_ && m_->ready; }

void Audio::play(const std::string& name, float vol, float pitch) {
  if (!ok()) return;
  auto it = m_->bank.find(name);
  if (it == m_->bank.end() || it->second.empty()) return;
  EnterCriticalSection(&m_->cs);
  for (auto& v : m_->voices) {
    if (v.active) continue;
    v.data = it->second.data(); v.len = it->second.size();
    v.pos = 0; v.rate = pitch; v.vol = vol; v.active = true;
    break;
  }
  LeaveCriticalSection(&m_->cs);
}

void Audio::setVolume(float v) { if (m_) m_->volume = std::max(0.f, std::min(1.f, v)); }
void Audio::setMuted(bool mu) { if (m_) m_->muted = mu; }

void Audio::shutdown() {
  if (!m_) return;
  InterlockedExchange(&m_->stop, 1);
  if (m_->evt) SetEvent(m_->evt);
  if (m_->hwo) waveOutReset(m_->hwo);
  if (m_->thread) { WaitForSingleObject(m_->thread, 500); CloseHandle(m_->thread); }
  if (m_->hwo) {
    for (auto& h : m_->hdr)
      if (h.dwFlags & WHDR_PREPARED) waveOutUnprepareHeader(m_->hwo, &h, sizeof(WAVEHDR));
    waveOutClose(m_->hwo);
  }
  if (m_->evt) CloseHandle(m_->evt);
  DeleteCriticalSection(&m_->cs);
  delete m_; m_ = nullptr;
}

#else // non-Windows: silent stub, game stays fully playable

struct Audio::Impl {};
bool Audio::init() { return false; }
bool Audio::ok() const { return false; }
void Audio::play(const std::string&, float, float) {}
void Audio::setVolume(float) {}
void Audio::setMuted(bool) {}
void Audio::shutdown() {}

#endif
