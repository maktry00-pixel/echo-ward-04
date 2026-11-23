#pragma once

#include "CoreMinimal.h"

class AEWardBookWorld;
class AEWardCampaignHero;
class AEWardHospitalMob;
class UMaterialInterface;

namespace WardHideout102
{
constexpr int32 Inspect = -3;

void Add(AEWardBookWorld& World, FVector Origin, FVector Size, bool Bed, UMaterialInterface* Wood, UMaterialInterface* Steel);
void Reset(AEWardBookWorld& World);
bool View(const AEWardCampaignHero& Hero, FVector& WorldEye, FRotator& Centre, float& YawLimit);
bool TryExit(AEWardBookWorld& World, bool Fast);
bool Heard(AEWardHospitalMob& Mob, FVector Source);
bool Inspecting(const AEWardHospitalMob& Mob);
bool AdvanceInspection(AEWardHospitalMob& Mob, float Delta);
}
