#include "WardCampaign.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "UObject/StrongObjectPtr.h"
using namespace WardData;
namespace {FVector GodotPoint(const TArray<TSharedPtr<FJsonValue>>& A){FVector P=Vector(A);return FVector(-P.Z,P.X,P.Y)*100;}}
void AEWardCampaignEnemy::BeginPlay()
{
    ACharacter::BeginPlay();auto* G=WardCampaign(this);if(!G)return;
    Body->SetVisibility(false);Head->SetVisibility(false);for(auto L:Legs)L->SetVisibility(false);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
    Json P=Object(Definition,TEXT("properties"));Crawler=Boolean(P,TEXT("crawler"));Blind=Boolean(P,TEXT("blind_hunter"));Spitter=Boolean(P,TEXT("can_spit"));Tactical=Boolean(P,TEXT("use_minimax"));Guardian=Number(P,TEXT("max_health"))>15;Ceiling=EnemyId==TEXT("hunter_17");
    Guardian=EnemyId==TEXT("hunter_5");MaxHealth=Number(P,TEXT("max_health"),6)*25;GetCharacterMovement()->MaxWalkSpeed=Number(P,TEXT("move_speed"),3.2)*100;
    SpawnDefaultController();for(const auto& V:Array(P,TEXT("patrol_points")))Patrol.Add(GodotPoint(V->AsArray())+FVector(0,0,96));
    if(auto* Creature=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Campaign/Creatures/SK_Stalker.SK_Stalker")))
    {
        GetMesh()->SetSkeletalMeshAsset(Creature);GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);GetMesh()->SetReceivesDecals(false);GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;GetMesh()->bEnableUpdateRateOptimizations=true;ResetEnemy();return;
    }
    TMap<FString,UHierarchicalInstancedStaticMeshComponent*> Groups;
    const FTransform Author=Transform(Object(Definition,TEXT("transform")));
    if(const auto* Rows=G->Visuals.Find(EnemyId))for(Json Row:*Rows)
    {
        FString Key=String(Row,TEXT("mesh"));for(const auto& M:Array(Row,TEXT("materials")))Key+=M->AsString();
        auto*& Group=Groups.FindOrAdd(Key);if(!Group)
        {
            Group=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);Group->bDisallowNanite=true;Group->SetupAttachment(GetRootComponent());Group->SetMobility(EComponentMobility::Movable);Group->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*MeshPath(String(Row,TEXT("mesh")))));Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);Group->bAutoRebuildTreeOnInstanceChanges=false;
            int32 I=0;for(const auto& M:Array(Row,TEXT("materials"))){auto* Material=LoadObject<UMaterialInterface>(nullptr,*MaterialPath(M->AsString()));Group->SetMaterial(I++,Material);if(Material&&Material->GetBlendMode()!=BLEND_Opaque&&Material->GetBlendMode()!=BLEND_Masked)Group->bDisallowNanite=true;}Group->RegisterComponent();
        }
        FWardCreaturePart Part;Part.Group=Group;Part.Path=String(Row,TEXT("path"));Part.Rest=Transform(Object(Row,TEXT("transform"))).GetRelativeTransform(Author);Part.Rest.AddToTranslation(FVector(0,0,-96));Part.Instance=Group->AddInstance(Part.Rest);Part.Pivot=Part.Rest.GetLocation();
        const auto& Limbs=Array(Object(Object(Definition,TEXT("rig")),TEXT("properties")),TEXT("limbs"));for(int32 I=0;I<Limbs.Num();++I)if(Part.Path.StartsWith(String(Limbs[I]->AsObject(),TEXT("node"))))Part.Leg=I;
        Geometry.Add(Part);
    }
    for(auto& Pair:Groups)Pair.Value->BuildTreeIfOutdated(false,true);ResetEnemy();
}
void AEWardCampaignEnemy::ResetEnemy()
{
    Health=MaxHealth;Dead=false;Active=Boolean(Object(Definition,TEXT("properties")),TEXT("active"));Awake=Active;Memory=0;Stunned=0;AttackTime=0;AttackCooldown=3;PatrolIndex=0;Injuries=0;DeathTime=0;Route.Reset();
    FTransform At=Transform(Object(Definition,TEXT("transform")));At.AddToTranslation(FVector(0,0,96));SetActorTransform(At,false,nullptr,ETeleportType::TeleportPhysics);SetActorHiddenInGame(!Active);SetActorEnableCollision(Active);GetCharacterMovement()->SetMovementMode(MOVE_Walking);SetActorTickEnabled(Active);
    if(auto* Creature=GetMesh()->GetSkeletalMeshAsset())
    {
        auto* CreatureMesh=GetMesh();CreatureMesh->SetSimulatePhysics(false);CreatureMesh->AttachToComponent(GetCapsuleComponent(),FAttachmentTransformRules::KeepRelativeTransform);CreatureMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);CreatureMesh->SetCollisionResponseToAllChannels(ECR_Ignore);CreatureMesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        const auto Bounds=Creature->GetBounds();const float Scale=(Guardian?235.f:Crawler?165.f:205.f)/FMath::Max(1.f,float(Bounds.BoxExtent.Z*2));
        CreatureMesh->SetRelativeScale3D(FVector(Scale));CreatureMesh->SetRelativeRotation(FRotator(0,-90,0));CreatureMesh->SetRelativeLocation(FVector(0,0,-101-(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale));
    }
}
void AEWardCampaignEnemy::Wake(){if(Dead)return;Active=true;Awake=true;SetActorHiddenInGame(false);SetActorEnableCollision(true);SetActorTickEnabled(true);}
void AEWardCampaignEnemy::HearNoise(const FVector& At,float Radius){if(!Active||Dead)return;if(FVector::Dist(GetActorLocation(),At)<Radius){Observed=At;Memory=Blind?11:7;PathClock=0;}}
float AEWardCampaignEnemy::TakeDamage(float D,const FDamageEvent& Event,AController* C,AActor* A)
{
    if(Guardian&&WardCampaign(this)->Flag(TEXT("guardian_started"))&&Exposed<=0)
    {
        const auto* Point=Event.IsOfType(FPointDamageEvent::ClassID)?static_cast<const FPointDamageEvent*>(&Event):nullptr;
        if(!Point||Point->HitInfo.ImpactPoint.Z>GetActorLocation().Z-40){WardCampaign(this)->Notify(TEXT("Броня держит. Уйди от рывка и стреляй в открытую грудь."),3);return 0;}
    }
    if(Dead)return 0;Wake();Health=FMath::Max(0.f,Health-D);Stunned=Guardian?.12f:.28f;if(A){Observed=A->GetActorLocation();Memory=12;}if(auto* G=WardCampaign(this))G->Cue(TEXT("creature_hit"),GetActorLocation());
    if(Health<=0){Dead=true;DeathTime=0;GetCharacterMovement()->StopMovementImmediately();GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);if(auto* G=WardCampaign(this)){++G->Kills;G->Score+=150;G->Cue(TEXT("creature_death"),GetActorLocation());if(Guardian)G->Notify(TEXT("Хранитель повержен. Охранный узел операционной доступен."),6);}}return D;
}
void AEWardCampaignEnemy::Tick(float D)
{
    ACharacter::Tick(D);auto* G=WardCampaign(this);if(!G||!G->Hero())return;Clock+=D;Animate(D);if(Dead){DeathTime+=D;if(DeathTime>2)SetActorTickEnabled(false);return;}if(!Active||!G->Screen.IsEmpty()||G->PendingRespawn)return;
    Stunned=FMath::Max(0.f,Stunned-D);AttackCooldown=FMath::Max(0.f,AttackCooldown-D);Memory=FMath::Max(0.f,Memory-D);if(Stunned>0)return;
    const FVector Hero=G->Hero()->GetActorLocation(),P=GetActorLocation();const float Dist=FVector::Dist(P,Hero);Exposed=FMath::Max(0.f,Exposed-D);
    if(G->Flag(TEXT("in_refuge"))){Memory=0;Route.Reset();return;}
    if(G->HidingBed&&Memory>0&&Dist<190){BedSearch+=D;if(BedSearch>2.3f){G->ToggleHide(G->HidingBed);G->DamageHero(33.34f,this);AttackCooldown=2;BedSearch=0;}return;}else BedSearch=0;
    if(ChargeTime>0)
    {
        ChargeTime-=D;GetCharacterMovement()->MaxWalkSpeed=Health<=MaxHealth*.5f?1400:1100;AddMovementInput(ChargeHeading);
        if(FVector::Dist2D(P,Hero)<135)G->DamageHero(42,this);
        FHitResult Wall;FCollisionQueryParams ChargeQuery(SCENE_QUERY_STAT(GuardianCharge),false,this);
        if(GetWorld()->LineTraceSingleByChannel(Wall,P,P+ChargeHeading*110,ECC_Visibility,ChargeQuery)&&Wall.GetActor()!=G->Hero())ChargeTime=0;
        if(ChargeTime<=0){Exposed=Health<=MaxHealth*.5f?1.8f:2.8f;Stunned=.45f;AttackCooldown=2.8f;G->Cue(TEXT("scrape"),P);G->Notify(TEXT("Он потерял равновесие! Оранжевая ткань на груди открыта."),3);}return;
    }
    FCollisionQueryParams Q(SCENE_QUERY_STAT(CampaignSight),false,this);FHitResult Hit;
    const bool Visible=!Blind&&!G->HidingBed&&Dist<1800&&!(G->Admin&&!G->Agent)&&(!GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,Crawler?-15:55),Hero+FVector(0,0,25),ECC_Visibility,Q)||Hit.GetActor()==G->Hero());
    if(Visible){Observed=Hero;Memory=8;}
    if(AttackTime>0)
    {
        AttackTime-=D;if(AttackTime<=0){if(Guardian){ChargeHeading=(Observed-P).GetSafeNormal2D();ChargeTime=.85f;}else if(Spitter){auto* Acid=GetWorld()->SpawnActor<AEWardCampaignProjectile>(P+GetActorForwardVector()*65+FVector(0,0,15),GetActorRotation());Acid->SetOwner(this);Acid->Velocity=(Observed-Acid->GetActorLocation()).GetSafeNormal()*850;}else if(FVector::Dist2D(Observed,Hero)<145&&FMath::Abs(P.Z-Hero.Z)<170)G->DamageHero(33.34f,this);AttackCooldown=Guardian?1.6f:Spitter?3.4f:2.f;G->Cue(TEXT("claw"),P);}return;
    }
    if(AttackCooldown<=0&&Dist<(Guardian?1000:Spitter?750:145)&&(Visible||Blind&&Memory>0)&&!G->HidingBed&&Exposed<=0)
    {AttackTime=(Spitter?1.2f:.8f)*G->DifficultyValue(TEXT("windup"));Observed=Hero;GetCharacterMovement()->StopMovementImmediately();G->Cue(TEXT("growl"),P,.8f);return;}
    if(Patrol.IsEmpty())Patrol.Add(P);if(Memory<=0&&FVector::Dist2D(P,Patrol[PatrolIndex%Patrol.Num()])<80)PatrolIndex=(PatrolIndex+1)%Patrol.Num();
    FVector Target=Memory>0?Observed:Patrol[PatrolIndex%Patrol.Num()];
    if((PathClock-=D)<=0)
    {
        PathClock=Memory>0?.55f:1.5f;if(Tactical&&Memory>0&&Dist<700){const FIntPoint Step=G->Grid.Tactical(G->Grid.Nearest(P),G->Grid.Nearest(Observed));Target=G->Grid.Point(Step);}
        Route=G->Grid.FindPath(P,Target);if(Route.Num()>0&&FVector::Dist2D(Route[0],P)<70)Route.RemoveAt(0);
    }
    while(Route.Num()>0&&FVector::Dist2D(P,Route[0])<42)Route.RemoveAt(0);
    if(Route.Num()>0)
    {
        const FVector Heading=(Route[0]-P).GetSafeNormal2D();SetActorRotation(FMath::RInterpTo(GetActorRotation(),Heading.Rotation(),D,Crawler?10:6));GetCharacterMovement()->MaxWalkSpeed=Number(Object(Definition,TEXT("properties")),TEXT("move_speed"),3.2)*100*G->DifficultyValue(TEXT("speed"))*FMath::Max(.45f,1-Injuries*.08f);AddMovementInput(Heading);
        FHitResult DoorHit;if(GetWorld()->LineTraceSingleByChannel(DoorHit,P,P+Heading*130,ECC_Visibility,Q))if(auto* Door=Cast<AEWardCampaignObject>(DoorHit.GetActor());Door&&Door->ObjectKind==TEXT("door")&&!Door->Locked&&!Door->Opened&&Door->Motion<=0)
        {if(Door->Barricade>0){if(AttackCooldown<=0){--Door->Barricade;AttackCooldown=1.4f;G->Cue(TEXT("claw"),Door->GetActorLocation());}}else Door->ToggleDoor();}
    }
}
void AEWardCampaignEnemy::Animate(float D)
{
    if(GetMesh()->GetSkeletalMeshAsset())
    {
        auto* CreatureMesh=GetMesh();
        if(Dead){if(!CreatureMesh->IsSimulatingPhysics()&&CreatureMesh->GetPhysicsAsset()){CreatureMesh->SetCollisionProfileName(TEXT("Ragdoll"));CreatureMesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);CreatureMesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);CreatureMesh->SetSimulatePhysics(true);}return;}
        static TStrongObjectPtr<UAnimSequence> Idle(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/Creatures/A_StalkerIdle"))),Walk(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/Creatures/A_StalkerWalk"))),Run(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/Creatures/A_StalkerRun"))),Attack(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/Creatures/A_StalkerAttack")));
        const float Speed=GetVelocity().Size2D();const float PreviousGait=Gait;Gait+=D*Speed/160.f*2*PI;
        auto* Game=WardCampaign(this);
        if(Active&&Game->Screen.IsEmpty()&&Speed>40&&FMath::FloorToInt(Gait/PI)>FMath::FloorToInt(PreviousGait/PI))Game->Cue(TEXT("heavy_step"),GetActorLocation()-FVector(0,0,90),Guardian?.38f:.22f);
        if(Active&&Memory>0&&Game->Screen.IsEmpty()&&FMath::FloorToInt(Clock/7)>FMath::FloorToInt((Clock-D)/7)&&FVector::DistSquared(GetActorLocation(),Game->Hero()->GetActorLocation())<640000)Game->Cue(TEXT("growl"),GetActorLocation()+FVector(0,0,65),.12f);
        UAnimSequence* Clip=AttackTime>0?Attack.Get():Speed>280?Run.Get():Speed>10?Walk.Get():Idle.Get();
        if(Clip){auto* Player=CreatureMesh->GetSingleNodeInstance();if(!Player||Player->GetCurrentAsset()!=Clip)CreatureMesh->PlayAnimation(Clip,AttackTime<=0);CreatureMesh->SetPlayRate(AttackTime>0?Clip->GetPlayLength()/FMath::Max(.1f,.8f*WardCampaign(this)->DifficultyValue(TEXT("windup"))):Speed>10?FMath::Clamp(Speed/(Speed>280?350.f:160.f),.55f,1.6f):1.f);}
        return;
    }
    const float Amount=FMath::Clamp(GetVelocity().Size2D()/300.f,0.f,1.f);Gait+=D*GetVelocity().Size2D()/100*6.28318f/(Crawler?.9f:1.1f);
    if((PoseClock-=D)>0)return;PoseClock=FVector::DistSquared(GetActorLocation(),WardCampaign(this)->Hero()->GetActorLocation())>4000000?.12f:.033f;
    TSet<UHierarchicalInstancedStaticMeshComponent*> Changed;
    for(const auto& P:Geometry)
    {
        FTransform At=P.Rest;FVector Pos=At.GetLocation();const float LegPhase=Gait+(P.Leg%2?PI:0)+(P.Leg/2)*.3f;
        if(P.Leg>=0){Pos.X+=FMath::Sin(LegPhase)*16*Amount;Pos.Z+=FMath::Max(0.f,FMath::Cos(LegPhase))*12*Amount;At.ConcatenateRotation(FQuat(FVector::RightVector,FMath::Sin(LegPhase)*.08f*Amount));}
        else{Pos.Z+=FMath::Sin(Gait*2)*2*Amount+FMath::Sin(Clock*2.1f)*.5f;At.ConcatenateRotation(FQuat(FVector::ForwardVector,FMath::Sin(Gait)*.018f*Amount));}
        if(AttackTime>0){Pos.X+=FMath::Sin(AttackTime*8)*7;Pos.Z-=5;}
        if(Dead){const float T=FMath::Clamp(DeathTime/1.1f,0.f,1.f);Pos.Z-=T*(Crawler?30:75);At.ConcatenateRotation(FQuat(FVector::ForwardVector,T*.9f));}
        At.SetLocation(Pos);P.Group->UpdateInstanceTransform(P.Instance,At,false,false,true);Changed.Add(P.Group);
    }
    for(auto* C:Changed){C->MarkRenderStateDirty();C->BuildTreeIfOutdated(true,false);}
}
Json AEWardCampaignEnemy::Snapshot()const
{
    Json R=MakeShared<FJsonObject>();R->SetStringField(TEXT("id"),EnemyId);R->SetNumberField(TEXT("health"),Health);R->SetBoolField(TEXT("active"),Active);R->SetBoolField(TEXT("dead"),Dead);R->SetNumberField(TEXT("injuries"),Injuries);TArray<TSharedPtr<FJsonValue>> At;for(double N:{GetActorLocation().X,GetActorLocation().Y,GetActorLocation().Z})At.Add(MakeShared<FJsonValueNumber>(N));R->SetArrayField(TEXT("at"),At);
    R->SetNumberField(TEXT("memory"),Memory);R->SetNumberField(TEXT("yaw"),GetActorRotation().Yaw);R->SetNumberField(TEXT("cooldown"),AttackCooldown);R->SetNumberField(TEXT("patrol"),PatrolIndex);R->SetNumberField(TEXT("exposed"),Exposed);R->SetNumberField(TEXT("stun"),Stunned);TArray<TSharedPtr<FJsonValue>> Known;for(double N:{Observed.X,Observed.Y,Observed.Z})Known.Add(MakeShared<FJsonValueNumber>(N));R->SetArrayField(TEXT("known"),Known);return R;
}
void AEWardCampaignEnemy::Restore(Json S){ResetEnemy();Health=Number(S,TEXT("health"),MaxHealth);Injuries=Number(S,TEXT("injuries"));if(Boolean(S,TEXT("active")))Wake();Dead=Boolean(S,TEXT("dead"))||Health<=0;SetActorLocation(Vector(Array(S,TEXT("at")),GetActorLocation()));SetActorRotation(FRotator(0,Number(S,TEXT("yaw")),0));Memory=FMath::Clamp((float)Number(S,TEXT("memory")),0.f,30.f);Observed=Vector(Array(S,TEXT("known")),GetActorLocation());AttackCooldown=Number(S,TEXT("cooldown"),2);PatrolIndex=FMath::Max(0,(int32)Number(S,TEXT("patrol")));Exposed=Number(S,TEXT("exposed"));Stunned=Number(S,TEXT("stun"));if(Dead){DeathTime=1.5f;GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);Animate(.04f);}}
