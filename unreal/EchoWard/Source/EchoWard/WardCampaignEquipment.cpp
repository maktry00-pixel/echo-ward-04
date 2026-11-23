#include "WardCampaign.h"
#include "WardCombat63.h"
#include "WardPistolPose.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "WardHospitalGameplay.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SkeletalMeshComponent.h"
using namespace WardData;
FTransform AEWardCampaignHero::WeaponPoseAt(float AimBlend,float CloseBlend)const
{
    return FirstPersonWeaponPose50(AimBlend,CloseBlend);
}
FTransform AEWardCampaignHero::WeaponPose()const{return WeaponPoseAt(WeaponAimBlend,WeaponCloseBlend);}
float AEWardCampaignHero::ReloadBlend()const
{
    auto* G=WardCampaign(this);if(!G||G->ReloadTime<=0)return 0;
    const float Duration=G->Shotgun?.7f:WardPistolPose::ReloadDuration,Elapsed=Duration-G->ReloadTime;
    const float Blend=FMath::Min(FMath::Clamp(Elapsed/.12f,0.f,1.f),FMath::Clamp(G->ReloadTime/.18f,0.f,1.f));
    return FMath::SmoothStep(0.f,1.f,Blend);
}
float AEWardCampaignHero::MagazineTravel()const
{
    auto* G=WardCampaign(this);
    return G&&!G->Shotgun?WardPistolPose::Reload(G->ReloadTime).MagazineDrop:0.f;
}
bool AEWardCampaignHero::WeaponHasClearance()const
{
    auto* G=WardCampaign(this);if(!G||!GetWorld())return true;
    // Trace from inside the player's safe capsule to the raised barrel, before
    // applying the close-wall pose. Never start a shot beyond an obstruction.
    const FVector Start=Camera->GetComponentLocation();
    const FVector Barrel=WardPistolPose::Mount(G->Shotgun,1.f)+FVector(G->Shotgun?82:28,0,0);
    const FVector End=G->Shotgun?FirstPersonWeaponPose50(1.f,0.f).TransformPosition(FVector(82,0,0)):
        Start+GetControlRotation().RotateVector(Barrel);
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(WeaponClearance),false,this);
    for(auto Enemy:G->Enemies)Query.AddIgnoredActor(Enemy.Get());
    if(G->HospitalStory)for(auto Mob:G->HospitalStory->Mobs)Query.AddIgnoredActor(Mob.Get());
    return !GetWorld()->SweepSingleByChannel(Hit,Start,End,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(5),Query);
}
FVector AEWardCampaignHero::WeaponGrip(bool Left)const
{
    auto* G=WardCampaign(this);const bool Long=G&&(G->PreviewItem51.IsEmpty()?G->Shotgun:G->PreviewItem51==TEXT("shotgun"));
    // Converted from the authored Godot grip points, in the same local frame as the gun.
    FVector Grip=Long?(Left?FVector(10,-4,-8):FVector(-6,0,-10)):(Left?FVector(-5,-3.4,-10.5):FVector(-5,3,-10));
    if(Left&&G)
    {
        if(Long)
        {
            Grip.X-=WardCombat63::ActionTravel(*G);
            if(G->ReloadTime>0)Grip=FMath::Lerp(Grip,FVector(-8,-4,-9),ReloadBlend());
        }
        else if(G->PreviewItem51.IsEmpty())
        {
            const auto Reload=WardPistolPose::Reload(G->ReloadTime);
            const float Low=FMath::Max(WardPistolPose::SprintLowering(MotionWeight52),WeaponCloseBlend);
            Grip+=FVector(-1.5,-1.8,-1)*Low*(1-Reload.GripBlend);
            Grip=FMath::Lerp(Grip,FVector(-5,-2.5,-13.3f-Reload.MagazineDrop),Reload.GripBlend);
            Grip=FMath::Lerp(Grip,FVector(0,-5,2.8)-FVector(Reload.SlidePull,0,0),Reload.SlideGrip);
        }
    }
    return WeaponPose().TransformPosition(Grip);
}
void AEWardCampaignHero::BuildEquipment()
{
    auto* G=WardCampaign(this);if(!G||!G->Visuals.Contains(TEXT("equipped_pistol")))return;
    Pistol->AttachToComponent(GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);Pistol->SetStaticMesh(nullptr);Pistol->SetWorldScale3D(FVector::OneVector);
    for(const FString& Id:TArray<FString>{TEXT("pistol"),TEXT("shotgun")})if(auto* Rows=G->Visuals.Find(TEXT("equipped_")+Id))for(Json Row:*Rows)
    {
        auto* Part=NewObject<UStaticMeshComponent>(this);Part->bDisallowNanite=true;Part->SetupAttachment(Pistol);Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*MeshPath(String(Row,TEXT("mesh")))));Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        int32 Index=0;for(const auto& M:Array(Row,TEXT("materials"))){auto* Material=LoadObject<UMaterialInterface>(nullptr,*MaterialPath(M->AsString()));Part->SetMaterial(Index++,Material);if(Material&&Material->GetBlendMode()!=BLEND_Opaque&&Material->GetBlendMode()!=BLEND_Masked)Part->bDisallowNanite=true;}
        const FTransform At=Transform(Object(Row,TEXT("transform")));Part->SetRelativeTransform(At);Part->ComponentTags.Add(FName(*Id));if(Boolean(Row,TEXT("visible"),true))Part->ComponentTags.Add(TEXT("authored_visible"));Part->SetLightingChannels(false,true,false);Part->SetVisibility(false);Part->RegisterComponent();Equipment.Add(Part);EquipmentPaths.Add(String(Row,TEXT("path")));EquipmentRest.Add(At);
    }
    MuzzleLight=NewObject<UPointLightComponent>(this);MuzzleLight->SetupAttachment(Pistol);MuzzleLight->SetIntensityUnits(ELightUnits::Lumens);MuzzleLight->SetIntensity(0);MuzzleLight->SetAttenuationRadius(350);MuzzleLight->SetLightColor(FLinearColor(1,.53f,.2f));MuzzleLight->SetCastShadows(false);MuzzleLight->SetLightingChannels(true,true,false);MuzzleLight->RegisterComponent();
    MuzzleFlame=NewObject<UStaticMeshComponent>(this);MuzzleFlame->SetupAttachment(Pistol);MuzzleFlame->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone")));MuzzleFlame->SetRelativeRotation(FRotator(90,0,0));MuzzleFlame->SetRelativeScale3D(FVector(.035f,.035f,.16f));MuzzleFlame->SetCollisionEnabled(ECollisionEnabled::NoCollision);MuzzleFlame->SetCastShadow(false);MuzzleFlame->SetVisibility(false);
    if(auto* Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_CampaignPBR.M_CampaignPBR"))){auto* Material=UMaterialInstanceDynamic::Create(Parent,this);Material->SetVectorParameterValue(TEXT("Emission"),FLinearColor(15,7,2));Material->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.7f,.28f,.05f));MuzzleFlame->SetMaterial(0,Material);}MuzzleFlame->RegisterComponent();
}
