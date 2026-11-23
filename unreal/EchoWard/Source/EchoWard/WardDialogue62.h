#pragma once
#include "WardCampaignData.h"
class AEWardHospitalGameplay;
class AEWardCampaignMode;
namespace WardDialogue62
{
 WardData::Json Data();
 void Tick(AEWardHospitalGameplay& Story,float Delta);
 bool PageFinished(const AEWardHospitalGameplay& Story);
 bool AwaitingChoice(AEWardHospitalGameplay& Story);
 bool HasControls(AEWardHospitalGameplay& Story);
 void Stop(AEWardHospitalGameplay& Story,bool Suspend=false);
 void Event(AEWardHospitalGameplay& Story,const FString& Actor,const FString& Kind);
 bool Speaking(const AEWardHospitalGameplay& Story,const FString& Actor);
 float Mouth(const AEWardHospitalGameplay& Story,const FString& Actor);
 void Assess(AEWardCampaignMode& Mode,float Delta);
}
