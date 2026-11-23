#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardEchoAssets.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
using namespace WardData;
Json AEWardBookWorld::HorrorProfile(const FString& Region)const
{
    Json P=Object(Object(WardEchoAssets::HorrorArt28(),TEXT("profiles")),Region);
    return P.IsValid()?P:Object(Object(Object(WardBookMapData(),TEXT("horror_atmosphere")),TEXT("profiles")),Region);
}
void AEWardBookWorld::BuildHorrorPresentation28()
{
    const Json D=WardEchoAssets::HorrorArt28();if(Number(D,TEXT("version"))<28)return;
    auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/HorrorV28/SM_TracePlane.SM_TracePlane"));if(!Plane)return;
    for(auto V:Array(D,TEXT("traces")))
    {
        const Json T=V->AsObject();FString Kind=String(T,TEXT("kind"));const FString Wall=String(T,TEXT("wall"));
        static Json Decay37=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/hospital_environment_v37.json"));
        if(Number(Decay37,TEXT("version"))==37&&(Kind==TEXT("Blood")||Kind==TEXT("Hand")||Kind==TEXT("Drag"))&&!Array(Decay37,TEXT("blood_zones")).ContainsByPredicate([&](const auto& V37){return V37->AsString()==String(T,TEXT("zone"));}))Kind=TEXT("Damp");FVector At=Vector(Array(T,TEXT("at")))*100;const FVector Size=Vector(Array(T,TEXT("size")))*100;
        if(Wall.IsEmpty())At.Z-=2.5f; // Plane now rests 1 mm above the actual floor, previously 26 mm.
        const FString Key=FString::Printf(TEXT("trace28_%s_%d_%d_%d"),*Kind,FMath::FloorToInt(At.X/2400),FMath::FloorToInt(At.Y/2400),FMath::FloorToInt(At.Z/480));
        auto* G=Groups.FindRef(Key);
        if(!G)
        {
            auto* Material=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Campaign/HorrorV28/MI_%s.MI_%s"),*Kind,*Kind));if(!Material)continue;
            G=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);G->SetupAttachment(RootComponent);G->SetMobility(EComponentMobility::Static);G->SetStaticMesh(Plane);G->SetMaterial(0,Material);
            G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetGenerateOverlapEvents(false);G->SetCastShadow(false);G->SetAffectDynamicIndirectLighting(false);G->SetAffectDistanceFieldLighting(false);G->SetLightingChannels(true,true,false);G->bAutoRebuildTreeOnInstanceChanges=false;G->SetCullDistance(3200);G->RegisterComponent();AddInstanceComponent(G);Groups.Add(Key,G);
        }
        FRotator Rotation=FRotator::ZeroRotator;FVector Scale(Size.X/100,Size.Y/100,1);
        if(!Wall.IsEmpty())
        {
            const bool ThinX=Size.X<Size.Y;
            // Keep local V vertical so the finger/leak silhouette has the
            // same orientation on either wall axis; normals are two-sided.
            Rotation=ThinX?FRotator(0,90,90):FRotator(0,0,90);
            Scale=FVector((ThinX?Size.Y:Size.X)/100,Size.Z/100,1);
        }
        G->AddInstance(FTransform(Rotation,At,Scale),false);++HorrorTraces28;++MeshInstances;
    }
    for(auto V:Array(D,TEXT("emergency_markers")))
    {
        const Json P=V->AsObject();Json Fit=MakeShared<FJsonObject>();Fit->SetArrayField(TEXT("bounds"),Array(P,TEXT("bounds")));
        if(AddCustomProp(129,Fit,Vector(Array(P,TEXT("at")))*100,Number(P,TEXT("turn")))){EmergencyAnchors.Add(Vector(Array(P,TEXT("anchor")))*100);++HorrorMarkers28;}
    }
    HorrorBounce28=NewObject<UPointLightComponent>(this);HorrorBounce28->SetupAttachment(RootComponent);HorrorBounce28->SetMobility(EComponentMobility::Movable);HorrorBounce28->SetIntensityUnits(ELightUnits::Lumens);HorrorBounce28->SetIntensity(200);HorrorBounce28->SetAttenuationRadius(330);HorrorBounce28->SetLightColor(FLinearColor(.66f,.38f,.32f));HorrorBounce28->SetCastShadows(false);HorrorBounce28->SetSourceRadius(110);HorrorBounce28->SetLightingChannels(true,true,false);HorrorBounce28->SetVisibility(false);HorrorBounce28->RegisterComponent();AddInstanceComponent(HorrorBounce28);
}
