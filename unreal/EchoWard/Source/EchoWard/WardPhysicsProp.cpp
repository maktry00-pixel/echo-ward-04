#include "WardPhysicsProp.h"
#include "WardCampaign.h"
#include "WardCombat63.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/StaticMesh.h"

AEWardPhysicsProp::AEWardPhysicsProp()
{
    PrimaryActorTick.bCanEverTick=true;
    Body=CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsBody"));SetRootComponent(Body);
    Body->SetCollisionProfileName(TEXT("PhysicsActor"));Body->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);Body->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);Body->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
    Body->SetNotifyRigidBodyCollision(true);Body->BodyInstance.bUseCCD=true;
    Body->BodyInstance.PositionSolverIterationCount=8;Body->BodyInstance.VelocitySolverIterationCount=4;
    Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));Visual->SetupAttachment(Body);Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->OnComponentHit.AddDynamic(this,&AEWardPhysicsProp::Impact);
}
void AEWardPhysicsProp::Configure(const FString& Type,FVector Size,float Mass,UMaterialInterface* Material)
{
    Kind=Type;Body->SetBoxExtent(Size*.5f);
    const bool Round=Kind==TEXT("bottle")||Kind==TEXT("case");
    Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Round?TEXT("/Engine/BasicShapes/Cylinder.Cylinder"):TEXT("/Engine/BasicShapes/Cube.Cube")));
    Visual->SetRelativeScale3D(Size/100.f);if(Material)Visual->SetMaterial(0,Material);
    auto* Physical=NewObject<UPhysicalMaterial>(this);Physical->Friction=Kind==TEXT("case")?.48f:.72f;Physical->Restitution=Kind==TEXT("case")?.32f:.12f;Body->SetPhysMaterialOverride(Physical);
    Body->SetLinearDamping(.18f);Body->SetAngularDamping(.65f);Body->SetSimulatePhysics(true);Body->SetMassOverrideInKg(NAME_None,Mass,true);Body->SetPhysicsMaxAngularVelocityInDegrees(1200);
    SetLifeSpan(Kind==TEXT("bottle")?8:18);
    if(Kind==TEXT("bottle"))
    {
        auto* Neck=NewObject<UStaticMeshComponent>(this);Neck->SetupAttachment(Body);Neck->SetStaticMesh(Visual->GetStaticMesh());Neck->SetCollisionEnabled(ECollisionEnabled::NoCollision);Neck->SetRelativeLocation(FVector(0,0,Size.Z*.6));Neck->SetRelativeScale3D(FVector(Size.X*.42,Size.Y*.42,Size.Z*.35)/100.f);if(Material)Neck->SetMaterial(0,Material);Neck->RegisterComponent();
    }
}
void AEWardPhysicsProp::Tick(float D)
{
    Super::Tick(D);Age+=D;PreviousVelocity=Body->GetPhysicsLinearVelocity();
    if(GetActorLocation().Z<-2500)Destroy();
    // Settled casings/debris need no per-frame gameplay work; Chaos can sleep them.
    if(Kind!=TEXT("bottle")&&Age>3&&PreviousVelocity.SizeSquared()<4)SetActorTickEnabled(false);
}
void AEWardPhysicsProp::Impact(UPrimitiveComponent*,AActor*,UPrimitiveComponent*,FVector Impulse,const FHitResult& Hit)
{
    if(Shattered||Age<.06f)return;
    if(Kind==TEXT("bottle")&&(PreviousVelocity.Size()>180||Impulse.Size()>65))
    {
        Shattered=true;if(auto* G=WardCampaign(this)){++G->BottleImpacts;G->Cue(TEXT("bottle"),Hit.ImpactPoint,.85f);G->BroadcastNoise(Hit.ImpactPoint,2600);G->SurfaceImpact(Hit.ImpactPoint,Hit.ImpactNormal,false,true);}Destroy();
    }
    else if(Kind==TEXT("case")&&Age<1.4f&&Impulse.Size()>1.5f){if(auto* G=WardCampaign(this))WardCombat63::Sound(*G,TEXT("case"),Hit.ImpactPoint,.07f);}
}
