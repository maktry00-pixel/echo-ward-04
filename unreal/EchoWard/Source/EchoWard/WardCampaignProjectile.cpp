#include "WardCampaign.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
AEWardCampaignProjectile::AEWardCampaignProjectile()
{
    PrimaryActorTick.bCanEverTick=true;Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Acid"));SetRootComponent(Visual);Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetStaticMesh(ConstructorHelpers::FObjectFinderOptional<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere")).Get());Visual->SetRelativeScale3D(FVector(.1f));SetActorEnableCollision(false);
}
void AEWardCampaignProjectile::Tick(float D)
{
    Super::Tick(D);auto* G=WardCampaign(this);if(!G||!G->Screen.IsEmpty())return;Age+=D;
    if(Age>D&&Age<2*D+.001f){if(auto* Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_CampaignPBR.M_CampaignPBR"))){auto* M=UMaterialInstanceDynamic::Create(Parent,this);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.19f,.26f,.035f));M->SetVectorParameterValue(TEXT("Emission"),FLinearColor(.008f,.012f,.001f));M->SetScalarParameterValue(TEXT("Roughness"),.16f);Visual->SetMaterial(0,M);}}
    if(!Puddle)
    {
        const FVector Start=GetActorLocation(),End=Start+Velocity*D;FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(AcidFlight),false,this);if(GetOwner())Q.AddIgnoredActor(GetOwner());
        if(GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Q)||Age>2)
        {
            if(Hit.GetActor()==G->Hero())G->DamageHero(33.34f,GetOwner());
            FVector At=Hit.bBlockingHit?Hit.ImpactPoint:End;FHitResult Floor;GetWorld()->LineTraceSingleByChannel(Floor,At+FVector(0,0,40),At-FVector(0,0,400),ECC_WorldStatic,Q);
            SetActorLocation(Floor.bBlockingHit?Floor.ImpactPoint+FVector(0,0,1):At);Visual->SetWorldScale3D(FVector(1.7f,1.7f,.012f));Puddle=true;Age=0;G->Cue(TEXT("flesh_hit"),GetActorLocation(),.4f);
        }else SetActorLocation(End);
    }
    else {if(FVector::Dist2D(G->Hero()->GetActorLocation(),GetActorLocation())<85&&FMath::Abs(G->Hero()->GetActorLocation().Z-GetActorLocation().Z)<160)G->DamageHero(16,GetOwner());if(Age>5)Destroy();}
}
