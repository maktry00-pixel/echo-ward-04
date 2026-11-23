#include "WardEchoAssets.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
using namespace WardData;
Json WardEchoAssets::Data(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_custom_runtime.json"));return D;}
Json WardEchoAssets::Bindings(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_custom_bindings.json"));return D;}
Json WardEchoAssets::Quality(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_quality_v22.json"));return D;}
Json WardEchoAssets::RealArt(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_real_v23.json"));return D;}
Json WardEchoAssets::DetailArt(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_detail_v26.json"));return D;}
Json WardEchoAssets::DetailArt27(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_detail_v27.json"));return D;}
Json WardEchoAssets::HorrorArt28(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_horror_v28.json"));return D;}
Json WardEchoAssets::Realism30(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_realism_v30.json"));return D;}
Json WardEchoAssets::Cars31(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_cars_v31.json"));return D;}
int WardEchoAssets::ResolvedId(int Id)
{
    const int RealAlias=Number(Object(Realism30(),TEXT("mesh_aliases")),FString::FromInt(Id));
    if(RealAlias>=920&&Number(Data(),TEXT("version"))>=30)return RealAlias;
    const int DetailAlias=Number(Object(DetailArt27(),TEXT("mesh_aliases")),FString::FromInt(Id));
    if(DetailAlias>=900&&Number(Data(),TEXT("version"))>=27)return DetailAlias;
    const int Alias=Number(Object(RealArt(),TEXT("mesh_aliases")),FString::FromInt(Id));
    return Alias>0&&Number(Data(),TEXT("version"))>=23?Alias:Id;
}
UStaticMesh* WardEchoAssets::Mesh(int Id,bool Leaf)
{
    if(!Boolean(Data(),TEXT("success"))||Id<=0)return nullptr;
    if(!Leaf)Id=ResolvedId(Id);
    static TMap<FString,TStrongObjectPtr<UStaticMesh>> Cache;
    const FString Name=FString::Printf(TEXT("SM_EH_%03d%s"),Id,Leaf?TEXT("_Leaf"):TEXT(""));
    if(auto* P=Cache.Find(Name))return P->Get();
    auto* M=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Campaign/EchoCustom/Meshes/%s.%s"),*Name,*Name));
    Cache.Add(Name,TStrongObjectPtr<UStaticMesh>(M));return M;
}
UMaterialInterface* WardEchoAssets::Surface(const FString& Source)
{
    const FString Id=Source==TEXT("527b19fd5563fa6d")?TEXT("34"):Source==TEXT("4ae6673fcc8b19e7")?TEXT("25"):Source==TEXT("05227396fb5ca2d8")?TEXT("26"):Source==TEXT("echo_metal")?TEXT("31"):TEXT("");
    const FString Path=String(Object(Data(),TEXT("surfaces")),Id);return Path.IsEmpty()?nullptr:LoadObject<UMaterialInterface>(nullptr,*Path);
}
UStaticMeshComponent* WardEchoAssets::Attach(AActor* Owner,USceneComponent* Parent,int Id,const FBox& Target)
{
    auto* Asset=Mesh(Id);if(!Asset)return nullptr;
    const FBox Source=Asset->GetBoundingBox();if(Source.GetSize().GetMin()<.001f)return nullptr;
    auto* C=NewObject<UStaticMeshComponent>(Owner);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Asset);
    // Imported props do not share a forward axis. Rotate long objects before
    // fitting (notably pistols, shotguns and the folding privacy screen).
    const FVector A=Source.GetSize(),B=Target.GetSize();
    const bool Turn=(A.X>A.Y)!=(B.X>B.Y)&&FMath::Max(A.X,A.Y)>FMath::Min(A.X,A.Y)*1.2f;
    // This sedan source points toward -X; the arrival headlights and route use +X.
    const float ForwardCorrection=ResolvedId(Id)==920?180.f:0.f;
    const FQuat Facing=FRotator(0,(Turn?90.f:0.f)+ForwardCorrection,0).Quaternion();const FBox Rotated=Source.TransformBy(FTransform(Facing));
    FVector Scale=B/Rotated.GetSize();if(Turn)Swap(Scale.X,Scale.Y);
    C->SetRelativeScale3D(Scale);C->SetRelativeRotation(Facing);C->SetRelativeLocation(Target.GetCenter()-Facing.RotateVector(Source.GetCenter()*Scale));
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);C->SetAffectDynamicIndirectLighting(false);C->SetAffectDistanceFieldLighting(false);C->SetLightingChannels(true,true,false);C->SetCullDistance(8000);C->RegisterComponent();Owner->AddInstanceComponent(C);return C;
}
