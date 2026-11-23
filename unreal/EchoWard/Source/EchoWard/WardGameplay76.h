#pragma once
#include "CoreMinimal.h"
class AEWardCampaignMode;
class AEWardBookWorld;
class UCanvas;
namespace WardGameplay76
{
 void Tick(AEWardCampaignMode& G,float Delta);
 void Draw(AEWardCampaignMode& G,UCanvas* Canvas);
 void Skip(AEWardCampaignMode& G,bool Scene=false);
 void Slot(AEWardCampaignMode& G,int Index);
 void Phone(AEWardCampaignMode& G);
 bool Command(AEWardCampaignMode& G,const FString& Id);
 int Door(const AEWardBookWorld& World);
 FString Prompt(AEWardCampaignMode& G);
 bool Armed(const AEWardCampaignMode& G);
 void Reset(AEWardCampaignMode& G);
 void Assess(AEWardCampaignMode& G,float Delta);
}
