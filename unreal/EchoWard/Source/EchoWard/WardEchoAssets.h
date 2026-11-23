#pragma once
#include "CoreMinimal.h"
#include "WardCampaignData.h"
class UStaticMesh;
class UMaterialInterface;
class AActor;
class USceneComponent;
class UStaticMeshComponent;
namespace WardEchoAssets
{
    WardData::Json Data();
    WardData::Json Bindings();
    WardData::Json Quality();
    WardData::Json RealArt();
    WardData::Json DetailArt();
    WardData::Json DetailArt27();
    WardData::Json HorrorArt28();
    WardData::Json Realism30();
    WardData::Json Cars31();
    int ResolvedId(int Id);
    UStaticMesh* Mesh(int Id,bool Leaf=false);
    UMaterialInterface* Surface(const FString& Source);
    UStaticMeshComponent* Attach(AActor* Owner,USceneComponent* Parent,int Id,const FBox& Bounds);
}
