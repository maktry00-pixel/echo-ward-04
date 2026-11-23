#pragma once
#include "CoreMinimal.h"
class AEWardCampaignMode;
class SWidget;
namespace WardAdmin79
{
    void Install(AEWardCampaignMode& G);
    void Uninstall(AEWardCampaignMode& G);
    void ResetWorld(AEWardCampaignMode& G);
    void Tick(AEWardCampaignMode& G,float Delta);
    void Assess(AEWardCampaignMode& G);
    bool PrimeChapterTwo(AEWardCampaignMode& G);
    bool TestAdvanceCampaign(AEWardCampaignMode& G, int32 Chapter);
    bool Command(AEWardCampaignMode& G,const FString& Action);
    bool Page(const FString& Screen);
    TSharedRef<SWidget> Build(AEWardCampaignMode& G);
}
