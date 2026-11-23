#pragma once
#include "WardCampaignData.h"
class AEWardHospitalGameplay;
class AEWardCampaignMode;
namespace WardNotes65
{
 WardData::Json Data();
 void Apply(AEWardHospitalGameplay& S);
 bool Allowed(const AEWardHospitalGameplay& S,const FString& Id);
 void Refresh(AEWardHospitalGameplay& S);
 void Fit(AEWardHospitalGameplay& S);
 void Sync(AEWardHospitalGameplay& S);
 void Collected(AEWardHospitalGameplay& S,const FString& Id);
 void Read(AEWardHospitalGameplay& S,const FString& Entry);
 bool UseKey(AEWardHospitalGameplay& S,const FString& Id);
 FString Summary(const AEWardHospitalGameplay& S);
 FString Missing(const AEWardHospitalGameplay& S);
 void Assess(AEWardCampaignMode& G,float Delta);
}
