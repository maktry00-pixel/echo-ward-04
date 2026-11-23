#pragma once

#include "WardCampaignData.h"

class AActor;
class UAudioComponent;
class UStaticMesh;

namespace WardHomeAssets
{
    UStaticMesh* BuildMesh(AActor* Owner, const WardData::Json& Library, const FString& Name);
    UAudioComponent* PlaySpatialFile61(AActor* Owner,const FString& Path,float Volume,const FVector& Position);
    UAudioComponent* PlayDialogueFile(AActor* Owner, const FString& Path, float Volume);
    UAudioComponent* Play(AActor* Owner, const TCHAR* Clip, float Volume, const FVector* Position = nullptr);
}
