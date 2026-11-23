#pragma once

#include "CoreMinimal.h"

class AEWardBookWorld;
class AEWardCampaignMode;
class SWidget;

namespace WardSecurity103
{
inline constexpr const TCHAR* Page = TEXT("security103");

FVector Terminal(const AEWardBookWorld& World);
bool Interact(AEWardCampaignMode& Game);
void Tick(AEWardCampaignMode& Game, float Delta);
void Reset(AEWardCampaignMode& Game);
TSharedRef<SWidget> Build(AEWardCampaignMode& Game);
void Assess(AEWardCampaignMode& Game, float Delta);
}
