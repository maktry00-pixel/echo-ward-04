#include "WardCampaignScene.h"
#include "WardCampaignData.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

using namespace WardData;
AEWardCampaignScene::AEWardCampaignScene()
{
    PrimaryActorTick.bCanEverTick=false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));
    GetRootComponent()->SetMobility(EComponentMobility::Static);
}
void AEWardCampaignScene::RebuildScene()
{
    TArray<UActorComponent*> Previous;GetComponents(Previous);
    for(UActorComponent* C:Previous)if(C!=GetRootComponent())C->DestroyComponent();
    VisualInstances=0;CollisionShapes=0;
    Json D=Campaign();TMap<FString,UHierarchicalInstancedStaticMeshComponent*> Groups;
    for(const auto& Value:Array(D,TEXT("instances")))
    {
        Json Row=Value->AsObject();if(!String(Row,TEXT("owner")).IsEmpty() || !Boolean(Row,TEXT("visible"),true))continue;
        const FTransform At=Transform(Object(Row,TEXT("transform")));
        const FString MeshId=String(Row,TEXT("mesh"));FString Key=MeshId;
        for(const auto& M:Array(Row,TEXT("materials")))Key+=TEXT("_")+M->AsString();
        // Small clusters preserve occlusion/culling instead of one map-wide bound.
        Key+=FString::Printf(TEXT("_%d_%d_%d_%d"),FMath::FloorToInt(At.GetLocation().X/2000),FMath::FloorToInt(At.GetLocation().Y/2000),FMath::FloorToInt(At.GetLocation().Z/400),Boolean(Row,TEXT("shadow"))?1:0);
        UHierarchicalInstancedStaticMeshComponent* Group=Groups.FindRef(Key);
        if(!Group)
        {
            UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath(MeshId));if(!Mesh){UE_LOG(LogTemp,Error,TEXT("CAMPAIGN_MISSING_MESH %s"),*MeshId);continue;}
            Group=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);Group->CreationMethod=EComponentCreationMethod::Instance;
            Group->SetupAttachment(GetRootComponent());Group->SetMobility(EComponentMobility::Static);Group->SetStaticMesh(Mesh);
            Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);Group->SetCastShadow(Boolean(Row,TEXT("shadow"),true));
            int32 Index=0;for(const auto& M:Array(Row,TEXT("materials"))){auto* Material=LoadObject<UMaterialInterface>(nullptr,*MaterialPath(M->AsString()));Group->SetMaterial(Index++,Material);if(Material&&Material->GetBlendMode()!=BLEND_Opaque&&Material->GetBlendMode()!=BLEND_Masked)Group->bDisallowNanite=true;}
            Group->bAutoRebuildTreeOnInstanceChanges=false;AddInstanceComponent(Group);Group->RegisterComponent();Groups.Add(Key,Group);
        }
        Group->AddInstance(At,true);++VisualInstances;
    }
    for(auto& Entry:Groups){Entry.Value->bAutoRebuildTreeOnInstanceChanges=true;Entry.Value->BuildTreeIfOutdated(false,true);}
    for(const auto& Value:Array(D,TEXT("collisions")))
    {
        Json Row=Value->AsObject();if(!String(Row,TEXT("owner")).IsEmpty() || Boolean(Row,TEXT("disabled")) || (((int32)Number(Row,TEXT("layer")))&1)==0)continue;
        UShapeComponent* Shape=nullptr;const FString Type=String(Row,TEXT("type"));
        if(Type==TEXT("BoxShape3D"))
        {
            UBoxComponent* Box=NewObject<UBoxComponent>(this);FVector S=Vector(Array(Row,TEXT("size")));Box->InitBoxExtent(FVector(S.Z,S.X,S.Y)*50);Shape=Box;
        }
        else if(Type==TEXT("CapsuleShape3D")||Type==TEXT("CylinderShape3D"))
        {
            UCapsuleComponent* Capsule=NewObject<UCapsuleComponent>(this);Capsule->InitCapsuleSize(Number(Row,TEXT("radius"))*100,Number(Row,TEXT("height"))*50);Shape=Capsule;
        }
        else if(Type==TEXT("SphereShape3D")){USphereComponent* Sphere=NewObject<USphereComponent>(this);Sphere->InitSphereRadius(Number(Row,TEXT("radius"))*100);Shape=Sphere;}
        if(!Shape){UE_LOG(LogTemp,Warning,TEXT("CAMPAIGN_UNSUPPORTED_COLLISION %s %s"),*Type,*String(Row,TEXT("path")));continue;}
        Shape->CreationMethod=EComponentCreationMethod::Instance;Shape->SetupAttachment(GetRootComponent());Shape->SetMobility(EComponentMobility::Static);
        Shape->SetWorldTransform(Transform(Object(Row,TEXT("transform"))));Shape->SetCollisionProfileName(TEXT("BlockAll"));Shape->SetGenerateOverlapEvents(false);
        AddInstanceComponent(Shape);Shape->RegisterComponent();++CollisionShapes;
    }
    UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_SCENE_BUILT %d visuals %d collisions %d clusters"),VisualInstances,CollisionShapes,Groups.Num());
}
