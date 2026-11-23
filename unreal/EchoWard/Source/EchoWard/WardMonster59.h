#pragma once
#include "WardCampaignData.h"
class AEWardHospitalGameplay;
class AEWardHospitalMob;
class AEWardCampaignMode;
namespace WardMonster59
{
    WardData::Json Data();
    WardData::Json Rule(const FString& Species);
    bool CameraDisabled(const AEWardHospitalMob& Mob);
    bool LinkedSignalDisabled(const AEWardHospitalMob& Mob);
    bool ContactThreatAt(const AEWardHospitalMob& Mob,const FVector& At,float Margin=80.f);
    FString ZoneState(const FString& Zone,int Chapter);
    void Refresh(AEWardHospitalGameplay& Story);
    void AddControls(AEWardHospitalGameplay& Story);
    bool UseControl(AEWardHospitalGameplay& Story,const FString& Id);
    TArray<FVector> Path(AEWardHospitalMob& Mob,FVector From,FVector To);
    void Assess(AEWardCampaignMode& Mode,float Delta);
}
