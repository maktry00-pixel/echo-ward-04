#pragma once

#include "CoreMinimal.h"

class AEWardBookWorld;
class AEWardCampaignMode;

namespace WardDecoy101
{
enum class ESound : uint8 { Push, Rattle, Stop, Impact };

void HearCart(AEWardCampaignMode& Game, FName Cart, ESound Sound, FVector At, float Radius);
bool MoveObstacle(AEWardBookWorld& World, int32 Index, FVector Direction);
}
