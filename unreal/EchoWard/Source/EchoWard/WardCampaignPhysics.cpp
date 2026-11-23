#include "WardCampaign.h"
#include "WardPhysicsProp.h"
#include "WardCampaignScene.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DecalActor.h"
#include "EngineUtils.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
using namespace WardData;

AEWardPhysicsProp* AEWardCampaignMode::SpawnPhysics(const FString& Kind,FVector At,FVector Velocity,FVector Size,float Mass,UMaterialInterface* Material)
{
    if(!BrassMaterial){BrassMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_FXBrass.M_FXBrass"));ChipMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_FXChip.M_FXChip"));BottleMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_FXBottle.M_FXBottle"));BulletMarkMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_BulletMark.M_BulletMark"));}
    PhysicsProps.RemoveAll([](const auto& P){return !P.IsValid();});
    while(PhysicsProps.Num()>=64){if(PhysicsProps[0].IsValid())PhysicsProps[0]->Destroy();PhysicsProps.RemoveAt(0);}
    auto* A=GetWorld()->SpawnActor<AEWardPhysicsProp>(At,FRotator::ZeroRotator);
    if(!A)return nullptr;
    A->Configure(Kind,Size,Mass,Material?Material:Kind==TEXT("case")?BrassMaterial.Get():Kind==TEXT("bottle")?BottleMaterial.Get():ChipMaterial.Get());
    A->Body->SetPhysicsLinearVelocity(Velocity);A->Body->SetPhysicsAngularVelocityInDegrees(FVector(FMath::FRandRange(-500.f,500.f),FMath::FRandRange(-500.f,500.f),FMath::FRandRange(-500.f,500.f)));
    PhysicsProps.Add(A);return A;
}
void AEWardCampaignMode::SurfaceImpact(FVector At,FVector Normal,bool Flesh,bool Glass)
{
    Normal=Normal.GetSafeNormal();if(Normal.IsNearlyZero())Normal=FVector::UpVector;
    // Cosmetic fragments ignore pawn/camera/shot traces, so they cannot block routes.
    const int32 Count=Glass?8:Flesh?0:3;
    for(int32 I=0;I<Count;++I){auto* A=SpawnPhysics(TEXT("chip"),At+Normal*4,Normal*FMath::FRandRange(70.f,180.f)+FMath::VRand()*90,FVector(Glass?2.f:.6f,Glass?3.f:1.4f,Glass?.4f:1.f),Glass?.006f:.0025f,Glass?BottleMaterial.Get():nullptr);if(A)A->SetLifeSpan(Glass?10:3);}
    if(Glass||Flesh)return;
    if(!BulletMarkMaterial)BulletMarkMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_BulletMark.M_BulletMark"));
    if(BulletMarkMaterial)
    {
        ImpactMarks.RemoveAll([](const auto& P){return !P.IsValid();});while(ImpactMarks.Num()>=48){if(ImpactMarks[0].IsValid())ImpactMarks[0]->Destroy();ImpactMarks.RemoveAt(0);}
        auto* Mark=GetWorld()->SpawnActor<ADecalActor>(At+Normal*.3f,(-Normal).Rotation());Mark->SetDecalMaterial(BulletMarkMaterial);Mark->GetDecal()->DecalSize=FVector(2.5,4.5,4.5);Mark->GetDecal()->SetFadeOut(35,5,false);Mark->SetLifeSpan(41);ImpactMarks.Add(Mark);
    }
}
void AEWardCampaignMode::ClearPhysics()
{
    for(auto P:PhysicsProps)if(P.IsValid())P->Destroy();PhysicsProps.Reset();for(auto P:ImpactMarks)if(P.IsValid())P->Destroy();ImpactMarks.Reset();BottleImpacts=0;
}

void AEWardCampaignMode::TickPhysicsAssessment(float D)
{
    static float Time=0;static bool Started=false,MassChecked=false,MassOK=false;static TWeakObjectPtr<AEWardPhysicsProp> Light,Heavy,WallProbe;static FVector WallPoint,WallNormal;static float WallExtent=0;Time+=D;
    if(!Started)
    {
        Started=true;Light=SpawnPhysics(TEXT("chip"),FVector(-1200,-150,250),FVector::ZeroVector,FVector(4),1);Heavy=SpawnPhysics(TEXT("chip"),FVector(-1200,150,250),FVector::ZeroVector,FVector(4),5);
        Light->Body->AddImpulse(FVector(300,0,0));Heavy->Body->AddImpulse(FVector(300,0,0));SpawnPhysics(TEXT("bottle"),FVector(-1300,50,180),FVector(0,0,-200),FVector(6,6,18),.45f);
        for(TActorIterator<AEWardCampaignScene> It(GetWorld());It&&!WallProbe.IsValid();++It){TArray<UBoxComponent*> Shapes;It->GetComponents(Shapes);for(auto* Box:Shapes){const FVector Ext=Box->GetScaledBoxExtent();if(Ext.Z<110||FVector::Dist(Box->GetComponentLocation(),Hero()->GetActorLocation())>4000)continue;const int32 Thin=Ext.X<Ext.Y?0:1;if(Ext[Thin]>40)continue;WallNormal=Box->GetComponentQuat().RotateVector(Thin==0?FVector::ForwardVector:FVector::RightVector);WallPoint=Box->GetComponentLocation();WallExtent=Ext[Thin];WallProbe=SpawnPhysics(TEXT("chip"),WallPoint+WallNormal*(WallExtent+100),-WallNormal*3500,FVector(4),1);break;}}
        return;
    }
    if(Time>.18f&&!MassChecked){MassChecked=true;MassOK=Light.IsValid()&&Heavy.IsValid()&&Light->Body->GetPhysicsLinearVelocity().X>Heavy->Body->GetPhysicsLinearVelocity().X*2;}
    if(Time<2.5f)return;
    TArray<TSharedPtr<FJsonValue>> Checks;bool All=true;auto Check=[&](FString Name,bool Passed){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("check"),Name);C->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(C));All&=Passed;};
    Check(TEXT("equal impulse moves light body faster"),MassChecked&&MassOK);
    Check(TEXT("bottle shatters on physical contact"),BottleImpacts==1);
    Check(TEXT("CCD body cannot cross an authored wall at 35 m/s"),WallProbe.IsValid()&&FVector::DotProduct(WallProbe->GetActorLocation()-WallPoint,WallNormal)>WallExtent-5);
    Check(TEXT("debris settles above the floor"),Light.IsValid()&&Heavy.IsValid()&&Light->GetActorLocation().Z>0&&Light->GetActorLocation().Z<15&&Heavy->GetActorLocation().Z>0&&Heavy->GetActorLocation().Z<15);
    bool Safe=PhysicsProps.Num()<=64;for(auto P:PhysicsProps)if(P.IsValid())Safe&=!P->GetActorLocation().ContainsNaN()&&P->Body->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Ignore&&P->Body->GetCollisionResponseToChannel(ECC_Camera)==ECR_Ignore;Check(TEXT("cosmetic physics stays finite and cannot trap player or camera"),Safe);
    SurfaceImpact(FVector(-1200,0,2),FVector::UpVector);Check(TEXT("surface hits create a material-backed decal"),BulletMarkMaterial&&ImpactMarks.Num()==1&&ImpactMarks[0].IsValid());
    for(int32 I=0;I<70;++I)SpawnPhysics(TEXT("chip"),FVector(-1200,0,400+I*5),FVector::ZeroVector);
    int32 Live=0;for(TActorIterator<AEWardPhysicsProp> It(GetWorld());It;++It)++Live;Check(TEXT("physics population remains bounded under repeated impacts"),PhysicsProps.Num()<=64&&Live<=64);
    ClearPhysics();Check(TEXT("new-game physics reset removes transient references"),PhysicsProps.IsEmpty()&&ImpactMarks.IsEmpty());
    Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),All);R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/physics_assessment.json"),R);FPlatformMisc::RequestExit(false);
}
