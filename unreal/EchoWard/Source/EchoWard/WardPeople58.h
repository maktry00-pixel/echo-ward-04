#pragma once
#include "WardCampaignData.h"
class AEWardHospitalGameplay;
class AEWardCampaignMode;
namespace WardPeople58
{
    WardData::Json Role(const FString& Id);
    bool SceneMember(AEWardHospitalGameplay& Story,const FString& Id);
    bool Command(AEWardCampaignMode& Mode,const FString& Action);
    void Sync(AEWardHospitalGameplay& Story);
    WardData::Json Snapshot(AEWardHospitalGameplay& Story);
    void Restore(AEWardHospitalGameplay& Story,WardData::Json Data);
}

void AssessHumans58(AEWardCampaignMode& Mode,float Delta);
