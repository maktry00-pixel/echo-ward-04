#include "WardBookWorld.h"
#include "WardEchoAssets.h"
#include "WardVisualGeometry67.h"
#include "Engine/StaticMesh.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
using namespace WardData;
bool AEWardBookWorld::AddCustomProp(int Id,Json Model,FVector At,int Turn)
{
    const auto& Measure37=Array(Model,TEXT("bounds"));
    const int OriginalId37=Id;
    const int Replacement37=Id==404&&Measure37.Num()==6&&Measure37[5]->AsNumber()-Measure37[2]->AsNumber()>1.5?3350:Id==242?3351:Id==310?3352:Id==393?3353:Id==283?3354:Id==311?3355:0;
    if(!Replacement37&&DeferProp37(Id,Model,At,Turn))return true;
    if(Replacement37)Id=Replacement37;
    UStaticMesh* Mesh=nullptr;
    if(Id==6767)Mesh=WardVisual67::WaterDispenser(this);
    else if(Id==6768)Mesh=WardVisual67::TunnelServices(this);
    else if(Id>=3300&&Id<=3355)
    {
        const FString Path=FString::Printf(TEXT("/Game/Campaign/HospitalV37/Meshes/SM_H37_%d.SM_H37_%d"),Id,Id);
        Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
    }
    else Mesh=WardEchoAssets::Mesh(Id);
    if(!Mesh)return false;
    const auto& B=Array(Model,TEXT("bounds"));if(B.Num()!=6)return false;
    const FBox Source=Mesh->GetBoundingBox(),Target(FVector(B[0]->AsNumber(),B[1]->AsNumber(),B[2]->AsNumber())*100,FVector(B[3]->AsNumber(),B[4]->AsNumber(),B[5]->AsNumber())*100);
    if(Source.GetSize().GetMin()<.01)return false;
    const FVector A=Source.GetSize(),DesiredSize=Target.GetSize();
    const bool Align=(A.X>A.Y)!=(DesiredSize.X>DesiredSize.Y)&&FMath::Max(A.X,A.Y)>FMath::Min(A.X,A.Y)*1.2f&&FMath::Max(DesiredSize.X,DesiredSize.Y)>FMath::Min(DesiredSize.X,DesiredSize.Y)*1.2f;
    // The supplemental workshop meshes face -Y, while the room schedule's
    // access points and all legacy cabinets face +Y.
    const FQuat Facing=FRotator(0,(Align?90:0)+(Id>=600||WardEchoAssets::ResolvedId(Id)>=700?180:0),0).Quaternion(),Room=FRotator(0,Turn*90,0).Quaternion();
    const FBox Rotated=Source.TransformBy(FTransform(Facing));FVector S=DesiredSize/Rotated.GetSize();if(Align)Swap(S.X,S.Y);
    const bool Preserve=WardData::Boolean(WardData::Object(WardEchoAssets::Realism30(),TEXT("preserve_proportions")),FString::FromInt(Id))||WardData::Boolean(WardData::Object(WardEchoAssets::RealArt(),TEXT("preserve_proportions")),FString::FromInt(Id));
    if(Preserve)S=FVector(S.GetMin());
    FVector Shift=Target.GetCenter()-Facing.RotateVector(Source.GetCenter()*S);
    // Small ceramic, paper and tool silhouettes keep their real proportions
    // and sit on the support plane rather than stretching into tall blocks.
    if(Preserve)Shift.Z=Target.Min.Z-Source.Min.Z*S.Z;
    const FTransform T(Room*Facing,At+Room.RotateVector(Shift),S);const FBox Actual=Source.TransformBy(T),Expected=Target.TransformBy(FTransform(Room,At));
    if(!Expected.ExpandBy(.25f).IsInsideOrOn(Actual.Min)||!Expected.ExpandBy(.25f).IsInsideOrOn(Actual.Max)){++CustomBoundsFailures;return false;}
    const int LampMat=Id==121?WhiteLampMaterial(At):0;
    const float Largest=DesiredSize.GetMax();
    const bool Small30=Largest<20;
    const float Cull30=Largest<12?1200.f:Largest<35?2200.f:Largest<80?3500.f:Id==640?14000.f:6500.f;
    const FString Key=FString::Printf(TEXT("echo_%d_%d_%d_%d_%d%s%s"),Id,LampMat,FMath::FloorToInt(At.X/2400),FMath::FloorToInt(At.Y/2400),FMath::FloorToInt(At.Z/480),AddingDetail27?TEXT("_detail27"):TEXT(""),Small30?TEXT("_small30"):TEXT(""));
    auto* G=Groups.FindRef(Key);if(!G)
    {
        G=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);G->SetupAttachment(RootComponent);G->SetMobility(EComponentMobility::Static);G->SetStaticMesh(Mesh);
        if(Id==121&&WardEchoAssets::ResolvedId(Id)==121&&Mesh->GetStaticMaterials().Num()>2)G->SetMaterial(2,Palette[LampMat]);
        G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetGenerateOverlapEvents(false);G->bAutoRebuildTreeOnInstanceChanges=false;G->SetAffectDynamicIndirectLighting(false);G->SetAffectDistanceFieldLighting(false);G->SetLightingChannels(true,true,false);G->SetCullDistance(Cull30);if(Small30)G->SetCastShadow(false);G->RegisterComponent();AddInstanceComponent(G);Groups.Add(Key,G);
    }
    // Shared groups retain the greatest required draw distance of their members.
    G->SetCullDistance(FMath::Max(G->LDMaxDrawDistance,Cull30));
    if(RenderingDeferred37){G->bAutoRebuildTreeOnInstanceChanges=false;DeferredTouched37.Add(G);}
    G->AddInstance(T,false);++MeshInstances;++CustomFurniture;CustomModelIds.Add(Id);return true;
}
bool AEWardBookWorld::AddCustomStain(FVector At,FVector Size,bool Wall)
{
    static Json Decay37=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/hospital_environment_v37.json"));
    bool Blood37=false;
    for(const auto& Room:Rooms)if(FMath::Abs(Room.Bounds.Min.Z-At.Z)<400&&At.X>=Room.Bounds.Min.X&&At.X<=Room.Bounds.Max.X&&At.Y>=Room.Bounds.Min.Y&&At.Y<=Room.Bounds.Max.Y)
    {Blood37=Array(Decay37,TEXT("blood_zones")).ContainsByPredicate([&](const auto& V){return V->AsString()==Room.Id;});break;}
    const bool Damp37=Number(Decay37,TEXT("version"))==37&&!Blood37;
    if(!Wall)At.Z=FMath::RoundToInt(At.Z/480)*480+.1f;
    const int Id=Wall?518:520;auto* Mesh=WardEchoAssets::Mesh(Id);if(!Mesh)return false;
    const bool ThinX=Size.X<Size.Y;
    const FRotator Facing=Wall?(ThinX?FRotator(90,0,0):FRotator(0,0,90)):FRotator::ZeroRotator;
    const FVector Scale=Wall?(ThinX?FVector(Size.Z/100,Size.Y/100,1):FVector(Size.X/100,Size.Z/100,1)):FVector(Size.X/100,Size.Y/100,1);
    const FString Key=FString::Printf(TEXT("echo_stain_%d_%d_%d_%d_%d"),Damp37?1:0,Id,FMath::FloorToInt(At.X/2400),FMath::FloorToInt(At.Y/2400),FMath::FloorToInt(At.Z/480));
    auto* G=Groups.FindRef(Key);if(!G)
    {
        G=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);G->SetupAttachment(RootComponent);G->SetMobility(EComponentMobility::Static);G->SetStaticMesh(Mesh);if(Damp37)G->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/HorrorV28/MI_Damp.MI_Damp")));G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetCastShadow(false);G->bAutoRebuildTreeOnInstanceChanges=false;G->SetAffectDynamicIndirectLighting(false);G->SetAffectDistanceFieldLighting(false);G->SetLightingChannels(true,true,false);G->SetCullDistance(5000);G->RegisterComponent();AddInstanceComponent(G);Groups.Add(Key,G);
    }
    G->AddInstance(FTransform(Facing,At-Facing.RotateVector(Mesh->GetBoundingBox().GetCenter()*Scale),Scale),false);++MeshInstances;++CustomDecals;CustomModelIds.Add(Id);return true;
}
