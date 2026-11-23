#pragma once
#include "WardCampaignData.h"
class AEWardHospitalGameplay;
class AEWardCampaignMode;
namespace WardPeople57
{
    WardData::Json Info(const FString& Id);
    void Append(AEWardHospitalGameplay& Story);
    bool Replay(AEWardCampaignMode& Mode, const FString& Id);
}
