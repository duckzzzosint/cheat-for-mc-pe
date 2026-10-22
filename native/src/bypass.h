// native/src/bypass.h
#pragma once
#include "types.h"

float BypassClickDelay(float cps, float variance);
Vec3  BypassSmooth(Vec3 current, Vec3 target, float alpha);
Vec3  BypassJitter(Vec3 vel, float amount);
