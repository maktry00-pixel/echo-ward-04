#pragma once
#include "CoreMinimal.h"
class AActor;
class UAudioComponent;
class AEWardCampaignMode;
namespace WardAudio68
{
    UAudioComponent* Play(AActor* Owner,const FString& Key,float Volume,const FVector* At=nullptr,bool Loop=false);
    FString FileKey(const FString& Path);
    bool Cue(AEWardCampaignMode& G,const FString& Id,FVector At,float Volume);
    void Tick(AEWardCampaignMode& G,float Delta);
    void Stop(AEWardCampaignMode& G);
    void Page(AEWardCampaignMode& G,const FString& From,const FString& To);
    bool Command(AEWardCampaignMode& G,const FString& Id);
    void Human(AEWardCampaignMode& G,AActor* Person,const FString& Identity,const FString& Event,bool Female=false);
    void Assess(AEWardCampaignMode& G,float Delta);
}
