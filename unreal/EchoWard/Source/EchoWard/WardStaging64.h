#pragma once
#include "CoreMinimal.h"
#include "WardCampaignData.h"
class AEWardHospitalGameplay;
class AEWardCampaignMode;
struct FWardPersonPose54;
namespace WardStaging64
{
 WardData::Json Data();
 bool Enabled(const AEWardHospitalGameplay& S);
 bool Active(const AEWardHospitalGameplay& S);
 void Begin(AEWardHospitalGameplay& S,const FString& Id);
 void Tick(AEWardHospitalGameplay& S,float Delta);
 bool CanFinish(AEWardHospitalGameplay& S);
 void Finish(AEWardHospitalGameplay& S);
 bool Command(AEWardHospitalGameplay& S,const FString& Action);
 bool Goal(AEWardHospitalGameplay& S,const FString& Id,FVector& At,FString& Pose,float& Speed);
 bool Controls(AEWardHospitalGameplay& S,const FString& Id);
 void Pose(AEWardHospitalGameplay& S,const FString& Id,TArray<FWardPersonPose54>& Skin);
 FVector Focus(AEWardHospitalGameplay& S,const FString& Id);
 FString PoseName(AEWardHospitalGameplay& S,const FString& Id);
 FString Status(AEWardHospitalGameplay& S);
 bool ChoiceReady(AEWardHospitalGameplay& S);
 bool WaitingAction(AEWardHospitalGameplay& S);
 void RefreshAnchors(AEWardHospitalGameplay& S);
 void Reset(AEWardHospitalGameplay& S);
 void Assess(AEWardCampaignMode& G,float Delta);
}
