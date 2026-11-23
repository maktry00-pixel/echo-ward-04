#include "WardCampaign.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "WardCampaignScene.h"
#include "EngineUtils.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Engine/DamageEvents.h"
using namespace WardData;
void AEWardCampaignMode::CheckRegression(TFunctionRef<void(const FString&,bool)> Check)
{
    bool Drawers=true,Disabled=true,Doors=true;int32 Count=0;
    for(const auto& E:Objects)
    {
        auto* O=E.Value.Get();if(O->HasStorage())
        {
            ++Count;Json Original=O->Snapshot();O->SetDoor(true,false);const FTransform Before=O->Hinge->GetRelativeTransform();
            TArray<FTransform> Parts;for(auto C:O->Parts)Parts.Add(C->GetComponentTransform());Json Open=O->Snapshot();O->Restore(Open);
            Drawers&=O->Hinge->GetRelativeTransform().Equals(Before,.01f);for(int32 I=0;I<Parts.Num();++I)Drawers&=O->Parts[I]->GetComponentTransform().Equals(Parts[I],.02f);O->Restore(Original);
        }
        for(int32 I=0;I<O->CollisionParts.Num();++I)if(O->InitialCollision[I]==ECollisionEnabled::NoCollision)Disabled&=O->CollisionParts[I]->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
        if(O->ObjectKind==TEXT("door")){Json Before=O->Snapshot();O->SetDoor(false,true);Doors&=O->Hinge->GetRelativeTransform().Equals(O->RestHinge,.01f);O->Restore(Before);}
    }
    Check(TEXT("all storage hinges and parts preserve open save pose"),Count>20&&Drawers);Check(TEXT("disabled colliders remain disabled after resets"),Disabled);Check(TEXT("closed doors preserve authored hinge basis"),Doors);
    auto* H=Hero();H->Focus=Objects[TEXT("weapon")];H->StartInteract();Show(TEXT("pause"));Check(TEXT("pause releases interaction and movement lock"),!H->Held&&H->GetVelocity().IsNearlyZero());Show(TEXT(""));
    H->Focus=Objects[TEXT("map")];H->StartInteract();NewGame(TEXT("normal"));Check(TEXT("new game clears held object and crouch state"),!H->Held&&!H->Focus&&!H->bIsCrouched);
    bool Grips=true;Flag(TEXT("weapon"),true);for(bool Long:{false,true})for(bool Aim:{false,true})for(float Pitch:{-45.f,0.f,45.f})
    {
        Shotgun=Long;H->Aiming=Aim;if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(FRotator(Pitch,0,0));TickHero(.016f);const FTransform Pose=H->WeaponPose();Grips&=H->Pistol->GetComponentTransform().Equals(Pose,.05f)&&!H->WeaponGrip(false).ContainsNaN()&&FVector::Dist(H->WeaponGrip(false),H->GetActorLocation())<110;
    }
    Check(TEXT("weapons and hand targets use a shared finite transform"),Grips);NewGame(TEXT("normal"));
    Show(TEXT(""));Flag(TEXT("weapon"),true);H->SetActorLocation(FVector(-1200,0,106));H->SetActorRotation(FRotator::ZeroRotator);if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(FRotator::ZeroRotator);
    H->Aiming=true;const FVector Lowered=H->WeaponPose().GetLocation();TickHero(.016f);Check(TEXT("aim transition moves continuously from lowered pose"),H->WeaponAimBlend>0&&H->WeaponAimBlend<1&&FVector::Dist(Lowered,H->WeaponPose().GetLocation())<15);
    auto* Blocker=GetWorld()->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Blocker);Blocker->SetRootComponent(Box);Box->SetBoxExtent(FVector(5,60,75));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Blocker->SetActorLocation(H->GetActorLocation()+FVector(40,0,25));
    ShotCooldown=0;H->Rounds=9;const int32 ShotsBefore=Shots;FireWeapon();Check(TEXT("wall before barrel blocks shot without consuming ammunition"),!H->WeaponHasClearance()&&H->Rounds==9&&Shots==ShotsBefore);
    Blocker->Destroy();ShotCooldown=0;FireWeapon();Check(TEXT("clear barrel permits exactly one shot"),H->WeaponHasClearance()&&H->Rounds==8&&Shots==ShotsBefore+1);
    H->Rounds=2;H->Reserve=12;ReloadTime=0;ReloadWeapon();TickHero(.65f);const FString MagPath=String(Object(Object(Object(Data,TEXT("equipment")),TEXT("pistol")),TEXT("magazine")),TEXT("node"));bool MagMoved=false;
    for(int32 I=0;I<H->Equipment.Num();++I)if(H->EquipmentPaths[I].StartsWith(MagPath)&&!MagPath.IsEmpty())MagMoved|=FVector::Dist(H->Equipment[I]->GetRelativeLocation(),H->EquipmentRest[I].GetLocation())>10;
    Check(TEXT("reload withdraws authored magazine before granting ammunition"),MagMoved&&H->Rounds==2&&H->Reserve==12&&!H->WeaponGrip(true).ContainsNaN());
    TickHero(1.2f);TickHero(.2f);Check(TEXT("reload completes once with exact ammunition transfer"),H->Rounds==9&&H->Reserve==5&&ReloadTime<=0&&H->MagazineTravel()==0);
    H->WeaponCloseBlend=1;H->WeaponKick=4;NewGame(TEXT("normal"));Check(TEXT("new game resets weapon transitions and obstruction state"),H->WeaponAimBlend==0&&H->WeaponCloseBlend==0&&H->WeaponKick==0&&!H->WeaponObstructed);
    int32 Blocks=0;bool Camera=true;for(TActorIterator<AEWardCampaignScene> It(GetWorld());It;++It){TArray<UBoxComponent*> Shapes;It->GetComponents(Shapes);for(auto* C:Shapes)if(C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision){++Blocks;Camera&=C->GetCollisionResponseToChannel(ECC_Camera)==ECR_Block&&C->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Block;}}
    Check(TEXT("all static walls and furniture block pawn and camera"),Blocks>400&&Camera);
    int32 Repairs=0;bool Clear=true;for(const auto& V:Array(Data,TEXT("instances"))){Json Row=V->AsObject();const FString Reason=String(Row,TEXT("visual_repair"));if(Reason.StartsWith(TEXT("door_overlap"))||Reason==TEXT("obsolete_flat_stain")){++Repairs;Clear&=!Boolean(Row,TEXT("visible"),true);}}
    Check(TEXT("door-overlap decoration and obsolete opaque floor stains are hidden"),Repairs>=35&&Clear);
    bool Leaves=true;for(const auto& Pair:Objects)if(Pair.Value->ObjectKind==TEXT("door")){auto* Door=Pair.Value.Get();Json Before=Door->Snapshot();Door->SetDoor(false,false);FBox LeafBounds(ForceInit);for(auto C:Door->MovingCollision)LeafBounds+=C->Bounds.GetBox();const bool Aligned=LeafBounds.IsValid&&FVector::Dist2D(LeafBounds.GetCenter(),Door->GetActorLocation())<15;if(!Aligned)UE_LOG(LogTemp,Warning,TEXT("DOOR_LEAF_MISALIGN %s %s"),*Door->ObjectId,*LeafBounds.GetCenter().ToString());Leaves&=Aligned;Door->Restore(Before);}Check(TEXT("all closed door leaves align with authored frame centers"),Leaves);
    bool Creatures=true;for(auto E:Enemies){auto* M=E->GetMesh()->GetSkeletalMeshAsset();Creatures&=M&&M->GetSkeleton()&&E->GetMesh()->GetPhysicsAsset()&&E->Geometry.IsEmpty();if(M)for(const TCHAR* Name:{TEXT("A_StalkerIdle"),TEXT("A_StalkerWalk"),TEXT("A_StalkerRun"),TEXT("A_StalkerAttack")}){auto* Clip=LoadObject<UAnimSequence>(nullptr,*(FString(TEXT("/Game/Campaign/Creatures/"))+Name));Creatures&=Clip&&Clip->GetSkeleton()==M->GetSkeleton();}}
    Check(TEXT("all enemies use the imported humanoid rig and compatible motion clips"),Creatures);
    auto* Enemy=Enemies[0].Get();const Json Alive=Enemy->Snapshot();Enemy->TakeDamage(Enemy->MaxHealth*10,FDamageEvent(),nullptr,H);Enemy->Animate(.016f);const bool Fell=Enemy->Dead&&Enemy->GetMesh()->IsSimulatingPhysics();Enemy->Restore(Alive);Check(TEXT("monster death enables ragdoll and restore returns an animated living body"),Fell&&!Enemy->Dead&&!Enemy->GetMesh()->IsSimulatingPhysics());NewGame(TEXT("normal"));
}
