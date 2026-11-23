#pragma once

#include "CoreMinimal.h"

class AEWardCampaignMode;
class UMaterialInterface;

namespace WardHorror99
{
void Footstep(AEWardCampaignMode& Game, const FVector& At, float Radius);
UMaterialInterface* FrostedPane(UObject* Owner, UMaterialInterface* Fallback);
}
