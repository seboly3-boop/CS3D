#pragma once
#include "Animation.h"
#include "Recoil.h"
#include "Renderer.h"
#include "Player.h"
#include "Camera.h"
#include "Input.h"

struct WeaponDef {
  const char* name;
  bool autoFire;
  float rpm;
  int magSize, reserveMax;
  float damage, headMult, legMult;
  float adsTime;          // seconds hip -> ADS (per weapon)
  float adsFov;
  float hipSpread, adsSpread;   // degrees
  float moveSpread, airSpread;
  float reloadTime, inspectTime;
  RecoilProfile recoil;
  WeaponVisual vis;
  const char* fireSound;
};

struct Weapon {
  const WeaponDef* def = nullptr;
  int mag = 0, reserve = 0;
  float cooldown = 0;
  RecoilSystem recoil;
};

struct ShotInfo { float damage, headMult, legMult; };

struct WSEvents {
  int shots = 0;
  ShotInfo shotInfo[2];
  bool dry = false;
  bool reloadStart = false, magOut = false, magIn = false, bolt = false, reloadDone = false;
  bool inspectStart = false, switchStart = false, raiseDone = false;
};

class WeaponSystem {
public:
  Weapon weapons[2];
  int current = 0;
  WAction action = WAction::None;
  float actionT = 0, actionDur = 1;
  float adsT = 0, adsBlend = 0;
  bool adsHeld = false;
  AnimOutput out;

  // Live-tunable global scales (settings panel).
  float swayScale = 1, recoilScale = 1, spreadScale = 1, adsTimeScale = 1;
  bool infiniteAmmo = false;

  // Stats.
  int shotsFired = 0, shotsHit = 0, kills = 0, headshots = 0;

  void init();
  void destroy();
  WSEvents update(float dt, Input& in, const Player& pl, const Camera& cam, bool allowFire, float time);
  void reportHit(bool target, bool head, bool kill);
  void refillAll();
  void resetStats();

  Weapon& cur() { return weapons[current]; }
  const WeaponDef& def() const { return *weapons[current].def; }
  float currentSpreadDeg(const Player& pl) const;
  float fireFlash() const { return fireFlash_; }

  void draw(const glm::mat4& vmProj, Shader& world, Shader& flat, FlatBatch& fx, float time);
  glm::vec3 muzzleWorld(const Camera& cam) const;

private:
  WeaponDef defs_[2];
  WeaponAnimator anim_;
  int pending_ = -1;
  bool magOutDone_ = false, ammoApplied_ = false, boltDone_ = false;
  float lastShotT_ = 9, dryCd_ = 0, fireFlash_ = 0, flashSeed_ = 0;
  Mesh body_[2], mag_[2], bolt_[2], sights_[2], handL_[2];
  bool hasLeft_[2] = {true, false};

  void buildDefs();
  void buildMeshes();
  void startSwitch(int want, WSEvents& e);
};
