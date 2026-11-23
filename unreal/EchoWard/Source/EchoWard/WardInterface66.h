#pragma once
#include "WardCampaignData.h"
class AEWardCampaignMode;
class FWardCampaignUI;
namespace WardInterface66
{
 bool IsField(const FString& Page);
 bool Hotkey(AEWardCampaignMode& G,const FString& Page);
 bool Back(AEWardCampaignMode& G);
 bool Command(AEWardCampaignMode& G,const FString& Action);
 TArray<int32> Documents(const AEWardCampaignMode& G,const FString& Query,bool Unread);
 FString ObjectiveId(const AEWardCampaignMode& G);
 FString Site(const AEWardCampaignMode& G);
 FString MapSummary(const AEWardCampaignMode& G);
 FString Controls(bool Admin=false);
 FString Description(const FString& Id);
 FString PickupName(AEWardCampaignMode& G,const FString& Id);
 bool CanUse(AEWardCampaignMode& G,const FString& Id);
 bool ShowNotice(const AEWardCampaignMode& G);
 void Assess(AEWardCampaignMode& G,float Delta);
}
