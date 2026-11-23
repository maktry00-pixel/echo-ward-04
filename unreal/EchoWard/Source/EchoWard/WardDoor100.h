#pragma once

#include "WardBookWorld.h"

class AEWardHospitalMob;

namespace WardDoor100
{
constexpr int32 LeverMaterial = -100;
constexpr float Duration = 2.5f;

bool HasLever(const AEWardBookWorld::FPortal& Door);
bool Secured(const AEWardBookWorld& World, const AEWardBookWorld::FPortal& Door);
bool Begin(AEWardBookWorld& World, const AEWardBookWorld::FPortal& Door, bool Audible = true, float Age = 0);
float LeverAngle(const AEWardBookWorld& World, FVector At);
float CheckAge(const AEWardBookWorld& World, FVector At);
bool HandTarget(const AEWardHospitalMob& Mob, FVector& At);
}
