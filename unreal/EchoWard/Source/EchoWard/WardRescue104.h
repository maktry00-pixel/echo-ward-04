#pragma once

#include "CoreMinimal.h"

class AEWardCampaignMode;

namespace WardOnline80::Rescue104
{
constexpr float CrawlSpeed = 63.f;
constexpr float CrawlHalfHeight = 42.f;
constexpr float HoldSeconds = 2.4f;
constexpr float BleedSeconds = 35.f;

struct FPrompt
{
    FString Name;
    float Progress = 0;
    float Remaining = 0;
    float Distance = 0;
    bool LocalDown = false;
    bool CanHelp = false;
    bool Helping = false;
};

bool Downed(const AEWardCampaignMode& Game);
bool DownHost(AEWardCampaignMode& Game,bool FellOut);
void HostHit(AEWardCampaignMode& Game);
bool Begin(AEWardCampaignMode& Game);
void End(AEWardCampaignMode& Game);
FPrompt Prompt(AEWardCampaignMode& Game);
}

namespace WardRescue104 = WardOnline80::Rescue104;
