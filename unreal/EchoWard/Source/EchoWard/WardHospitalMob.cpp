#include "WardHospitalGameplay.h"
#include "WardPeople54.h"
#include "WardOnline80.h"
#include "WardMonster59.h"
#include "WardMonsterVisual59.h"
#include "WardMonsterMotion97.h"
#include "WardHorror99.h"
#include "WardDoor100.h"
#include "WardDecoy101.h"
#include "WardHideout102.h"
#include "WardSecurity103.h"
#include "WardContact98.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardHomeAssets53.h"
#include "Components/AudioComponent.h"
#include "Misc/Paths.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "UObject/StrongObjectPtr.h"
using namespace WardData;

namespace
{
constexpr int32 CheckLastSight = -1;
constexpr int32 CheckCart = -2;
constexpr int32 WardSearchStart = 1000;
constexpr float ListenSeconds = .88f;
bool FootstepInProgress = false;
struct FCartPulse { FName Cart; WardDecoy101::ESound Sound; };
const FCartPulse* CartPulse = nullptr;

struct FCartInterest
{
    FName Cart;
    double IgnoreUntil = 0;
    float Wait = 0;
    bool Settled = false;
};
TMap<TWeakObjectPtr<AEWardHospitalMob>, FCartInterest> CartInterests;

struct FCartReaction { float Range; float InspectSeconds; float Cooldown; bool CheckWards; };

FCartReaction CartReaction(const AEWardHospitalMob& Mob, WardDecoy101::ESound Sound)
{
    if (Boolean(Mob.Rule59,TEXT("stationary")) || Mob.Species==TEXT("pursuer") || Mob.Species==TEXT("parasite")) return {};
    const bool Sharp = Sound==WardDecoy101::ESound::Rattle || Sound==WardDecoy101::ESound::Impact;
    if (Mob.Species==TEXT("armoured")) return {Sharp?.78f:0.f,.62f,24.f,false};
    if (Mob.Species==TEXT("hound")) return {.69f,.58f,19.f,false};
    if (Mob.Species==TEXT("blind_surgeon") || Mob.Species==TEXT("listener")) return {1.15f,1.25f,8.f,true};
    return {1.f,.85f,14.f,true};
}

enum class EWardVisitPhase { Approach, Enter, Leave, CheckHandle };

struct FWardVisit
{
    FString Portal;
    FVector Outside;
    FVector Inside;
    float Pause = 0;
    EWardVisitPhase Phase = EWardVisitPhase::Approach;
};
TMap<TWeakObjectPtr<AEWardHospitalMob>, FWardVisit> WardVisits;

bool FrostedGlass(const FHitResult& Hit)
{
    const auto* Component = Hit.GetComponent();
    const auto* Material = Component ? Component->GetMaterial(0) : nullptr;
    return Material && Material->GetFName() == FName(TEXT("WardFrostedPane"));
}

bool SearchPassage(const AEWardBookWorld& World, const AEWardBookWorld::FPortal& Door)
{
    return Door.Open && !Door.Locked && !Door.InsideOnly && !World.RefugeBolts.Contains(Door.Zone);
}

bool CanCheckHandle(const AEWardHospitalMob& Mob, const AEWardBookWorld& World, const AEWardBookWorld::FPortal& Door)
{
    return Mob.Species!=TEXT("hound") && Mob.Species!=TEXT("parasite") && !Door.Open && Door.Leaf &&
        WardDoor100::HasLever(Door) && WardDoor100::Secured(World,Door);
}

FVector HandlePoint(const AEWardBookWorld::FPortal& Door, const FVector& Exterior)
{
    const bool NS = Door.Side==TEXT("south") || Door.Side==TEXT("north");
    const FVector Along = NS ? FVector::ForwardVector : FVector::RightVector;
    const bool Double = Door.Width>160;
    const double Offset = Double ? -19 : Door.Width*.5-25;
    return Door.At + Along*Offset + (Exterior-Door.At).GetSafeNormal2D()*11 + FVector(0,0,115);
}

bool ClearWardRoute(AEWardHospitalMob& Mob, FVector From, FVector To)
{
    const auto* Capsule = Mob.GetCapsuleComponent();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardSearch99), false, &Mob);
    Query.AddIgnoredActor(Mob.Runtime->Mode->Hero());
    FHitResult Hit;
    return !Mob.GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius()-2.f, Capsule->GetScaledCapsuleHalfHeight()-4.f), Query);
}

TArray<FVector> WardRoute(AEWardHospitalMob& Mob, FVector Goal)
{
    const FVector Start = Mob.GetActorLocation();
    if (ClearWardRoute(Mob, Start, Goal)) return {Goal};
    auto Path = WardMonster59::Path(Mob, Start, Goal);
    if (Path.IsEmpty()) return {};
    FVector From = Start;
    Path.Add(Goal);
    for (auto& Point : Path)
    {
        Point.Z = Goal.Z;
        if (!ClearWardRoute(Mob, From, Point)) return {};
        From = Point;
    }
    return Path;
}

bool NextWard(AEWardHospitalMob& Mob)
{
    const auto& World = *Mob.Runtime->Mode->BookWorld;
    struct FCandidate { const AEWardBookWorld::FPortal* Door; const FWardBookRoom3D* Room; double Score; };
    TArray<FCandidate, TInlineAllocator<24>> Candidates;
    const auto Floor = Mob.Runtime->CellAt(Mob.SearchOrigin59).Z;
    for (const auto& Door : World.Portals)
    {
        if (!Door.Role.IsEmpty() || FMath::Abs(Door.At.Z-Floor)>5 || FVector::DistSquared2D(Door.At,Mob.SearchOrigin59)>FMath::Square(780.)) continue;
        const auto* Room = World.Rooms.FindByPredicate([&](const auto& Entry) { return Entry.Id == Door.Zone; });
        if (!Room || !Room->Enclosed || Room->Location == TEXT("ST") || Room->Location == TEXT("RF")) continue;
        const double Distance = FVector::Dist2D(Door.At, Mob.SearchOrigin59);
        Candidates.Add({&Door, Room, Distance+(Room->Name.Contains(TEXT("палат")) ? 0 : 160)});
    }
    Candidates.Sort([](const auto& A, const auto& B) { return A.Score == B.Score ? A.Door->Id < B.Door->Id : A.Score < B.Score; });
    // Bounded route work. Check only the immediate neighbourhood of the last sighting.
    const int32 Limit = FMath::Min(Candidates.Num(), 5);
    for (int32 Index = FMath::Max(0, Mob.SearchStep59-WardSearchStart); Index < Limit; ++Index)
    {
        const auto& Candidate = Candidates[Index];
        const auto& Door = *Candidate.Door;
        const bool CanEnter = SearchPassage(World, Door);
        const bool CanCheck = CanCheckHandle(Mob,World,Door);
        if ((!CanEnter&&!CanCheck) || Door.Width < Mob.GetCapsuleComponent()->GetScaledCapsuleRadius()*2+16) continue;
        const bool NS = Door.Side == TEXT("south") || Door.Side == TEXT("north");
        const FVector ToRoom = Candidate.Room->Bounds.GetCenter()-Door.At;
        const FVector Axis = NS ? FVector(0,FMath::Sign(ToRoom.Y),0) : FVector(FMath::Sign(ToRoom.X),0,0);
        const FVector Threshold = Door.At + FVector(0,0,Mob.GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2);
        const FVector Handle = HandlePoint(Door, Door.At-Axis*100);
        const FVector Outside = CanEnter ? Threshold-Axis*132 : FVector(Handle.X,Handle.Y,Threshold.Z)-Axis*(Mob.GetCapsuleComponent()->GetScaledCapsuleRadius()+18);
        const FVector Inside = Threshold+Axis*126;
        if (CanEnter && (!Candidate.Room->Bounds.IsInside(Inside) || !ClearWardRoute(Mob, Outside, Inside))) continue;
        auto Path = WardRoute(Mob, Outside);
        if (Path.IsEmpty()) continue;
        for (auto It = WardVisits.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
        WardVisits.Add(&Mob, {Door.Id, Outside, Inside, 0.f, CanEnter?EWardVisitPhase::Approach:EWardVisitPhase::CheckHandle});
        Mob.Target = Outside;
        Mob.Path = MoveTemp(Path);
        Mob.PathIndex = 0;
        Mob.PathClock = 4.5f;
        Mob.StateClock59 = 0;
        Mob.SearchStep59 = WardSearchStart + Index + 1;
        return true;
    }
    return false;
}

void EndWardSearch(AEWardHospitalMob& Mob)
{
    WardVisits.Remove(&Mob);
    Mob.SearchStep59 = 0;
    Mob.Target = Mob.Home;
    Mob.PathClock = 0;
    Mob.ChangeState59(TEXT("return"));
}

float CartCheckDistance(const AEWardHospitalMob& Mob, FName Cart)
{
    const auto& Movables = Mob.Runtime->Mode->BookWorld->Movables;
    for (const auto& Prop : Movables)
        if (FName(*Prop.Id)==Cart)
            return FMath::Clamp(float(Prop.Half.Size2D()+Mob.GetCapsuleComponent()->GetScaledCapsuleRadius()+18),100.f,235.f);
    return 145.f;
}

bool InspectCart(AEWardHospitalMob& Mob, float Delta)
{
    auto* Interest = CartInterests.Find(&Mob);
    if (!Interest) { EndWardSearch(Mob); return false; }
    const FVector At = Mob.GetActorLocation();
    bool Nearby = FVector::Dist2D(At,Mob.SearchOrigin59) <= CartCheckDistance(Mob,Interest->Cart);
    if (Nearby)
    {
        const auto& Props = Mob.Runtime->Mode->BookWorld->Movables;
        FHitResult Hit;
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(WardCartInspect101),false,&Mob);
        if (Mob.GetWorld()->LineTraceSingleByChannel(Hit,At+FVector(0,0,22),Mob.SearchOrigin59,ECC_Visibility,Query))
            Nearby = Props.ContainsByPredicate([&](const auto& Prop){return Prop.Body.Get()==Hit.GetComponent() && FName(*Prop.Id)==Interest->Cart;});
    }
    if (!Nearby && Mob.StateClock59 < 9.f) return false;
    const auto Reaction = CartReaction(Mob,WardDecoy101::ESound::Rattle);
    Mob.GetCharacterMovement()->StopMovementImmediately();
    Mob.ConsumeMovementInputVector();
    const FVector Direction = (Mob.SearchOrigin59-At).GetSafeNormal2D();
    Interest->Wait+=Delta;
    const float Scanning = FMath::Sin(Interest->Wait*5.f)*19.f;
    if (!Direction.IsNearlyZero()) Mob.SetActorRotation(FMath::RInterpConstantTo(Mob.GetActorRotation(),
        Direction.Rotation()+FRotator(0,Scanning,0),Delta,145));
    if (Interest->Wait < Reaction.InspectSeconds+(Interest->Settled?0.f:.50f)) return true;
    Interest->IgnoreUntil = Mob.GetWorld()->GetTimeSeconds()+Reaction.Cooldown;
    if (Reaction.CheckWards && Nearby)
    {
        Mob.SearchStep59 = WardSearchStart;
        Mob.SearchLeft59 = FMath::Clamp(Mob.SearchLeft59+6.f,12.f,18.f);
        Mob.ChangeState59(TEXT("search"));
    }
    else EndWardSearch(Mob);
    return false;
}

bool SearchWards(AEWardHospitalMob& Mob, float Delta)
{
    Mob.SearchLeft59 = FMath::Max(0.f, Mob.SearchLeft59-Delta);
    if (Mob.SearchLeft59 <= 0 || Boolean(Mob.Rule59,TEXT("stationary"))) { EndWardSearch(Mob); return false; }
    auto* Visit = WardVisits.Find(&Mob);
    auto& World = *Mob.Runtime->Mode->BookWorld;
    const AEWardBookWorld::FPortal* Door = nullptr;
    if (Visit)
    {
        Door = World.Portals.FindByPredicate([&](const auto& Entry){ return Entry.Id == Visit->Portal; });
        const bool Allowed = Door && (Visit->Phase==EWardVisitPhase::CheckHandle ? CanCheckHandle(Mob,World,*Door) : SearchPassage(World,*Door));
        if (!Allowed || Mob.StateClock59>5.f)
        { WardVisits.Remove(&Mob); Visit = nullptr; }
    }
    if (!Visit)
    {
        Mob.ChangeState59(TEXT("search"));
        if (!NextWard(Mob)) EndWardSearch(Mob);
        return false;
    }
    const bool Checking = Visit->Phase == EWardVisitPhase::CheckHandle;
    const bool AtDoor = FVector::DistSquared2D(Mob.GetActorLocation(),Mob.Target)<=FMath::Square(Checking?32.:56.);
    if (!AtDoor) return false;
    if (Visit->Phase == EWardVisitPhase::Leave)
    {
        WardVisits.Remove(&Mob);
        if (!NextWard(Mob)) EndWardSearch(Mob);
        return false;
    }
    Mob.GetCharacterMovement()->StopMovementImmediately();
    Mob.ConsumeMovementInputVector();
    if (Checking)
    {
        const FVector Facing = (HandlePoint(*Door,Visit->Outside)-Mob.GetActorLocation()).GetSafeNormal2D();
        if (!Facing.IsNearlyZero()) Mob.SetActorRotation(FMath::RInterpConstantTo(Mob.GetActorRotation(),Facing.Rotation(),Delta,195));
        if (Mob.State59!=TEXT("door_check"))
        {
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(WardDoorCheck100),false,&Mob);
            const FVector Handle = HandlePoint(*Door,Visit->Outside);
            const FVector Arm = FVector(Mob.GetActorLocation().X,Mob.GetActorLocation().Y,Handle.Z);
            const bool Blocked = Mob.GetWorld()->LineTraceSingleByChannel(Hit,Arm,Handle,ECC_Visibility,Query) && Hit.GetComponent()!=Door->Leaf;
            if (!Blocked && WardDoor100::Begin(World,*Door))
            {
                Mob.ChangeState59(TEXT("door_check"));
                Mob.SearchLeft59 = FMath::Max(Mob.SearchLeft59,WardDoor100::Duration+.1f);
                return true;
            }
        }
        else if (Mob.StateClock59 < WardDoor100::Duration) return true;
        Mob.ChangeState59(TEXT("search"));
        WardVisits.Remove(&Mob);
        if (!NextWard(Mob)) EndWardSearch(Mob);
        return false;
    }
    const FVector Look = (Visit->Inside-Visit->Outside).GetSafeNormal2D();
    const bool ArrivedAtDoor = Visit->Phase == EWardVisitPhase::Approach;
    Visit->Pause += Delta;
    const float ScanYaw = ArrivedAtDoor ? 0.f : FMath::Lerp(-32.f,32.f,FMath::Clamp(Visit->Pause/.38f,0.f,1.f));
    Mob.SetActorRotation(FMath::RInterpConstantTo(Mob.GetActorRotation(), Look.Rotation() + FRotator(0,ScanYaw,0), Delta, 165));
    if (Visit->Pause < (ArrivedAtDoor ? .42f : .38f)) return true;
    const FVector Goal = ArrivedAtDoor ? Visit->Inside : Visit->Outside;
    // Leave by the same doorway before selecting the next ward in the corridor.
    if (ClearWardRoute(Mob, Mob.GetActorLocation(), Goal))
    {
        Visit->Phase = ArrivedAtDoor ? EWardVisitPhase::Enter : EWardVisitPhase::Leave;
        Visit->Pause = 0;
        Mob.Target = Goal;
        Mob.Path = {Goal};
        Mob.PathIndex = 0;
        Mob.PathClock = 3.8f;
        Mob.StateClock59 = 0;
        return false;
    }
    WardVisits.Remove(&Mob);
    if (!NextWard(Mob)) EndWardSearch(Mob);
    return false;
}
}

bool WardDoor100::HandTarget(const AEWardHospitalMob& Mob, FVector& At)
{
    if (Mob.State59!=TEXT("door_check") || !Mob.Runtime || !Mob.Runtime->Mode || !Mob.Runtime->Mode->BookWorld) return false;
    const auto& World = *Mob.Runtime->Mode->BookWorld;
    const auto* Visit = WardVisits.Find(&Mob);
    const auto* Door = World.Portals.FindByPredicate([&](const auto& Entry){
        if (Visit) return Entry.Id==Visit->Portal;
        return !Entry.Open && CheckAge(World,Entry.At)>=0 && FVector::DistSquared2D(Mob.GetActorLocation(),Entry.At)<FMath::Square(160.);
    });
    if (!Door || Door->Open) return false;
    At = HandlePoint(*Door,Visit?Visit->Outside:Mob.GetActorLocation()) - FVector(0,0,12*FMath::Sin(FMath::DegreesToRadians(LeverAngle(World,Door->At))));
    return true;
}

UMaterialInterface* WardHorror99::FrostedPane(UObject* Owner, UMaterialInterface* Fallback)
{
    auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign/EchoCustom/Materials/M_EchoGlassV2.M_EchoGlassV2"));
    if (!Base) Base = Fallback;
    if (!Base) return nullptr;
    auto* Pane = UMaterialInstanceDynamic::Create(Base, Owner, TEXT("WardFrostedPane"));
    Pane->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.35f,.44f,.46f,.49f));
    Pane->SetScalarParameterValue(TEXT("Roughness"), .86f);
    return Pane;
}

void WardHorror99::Footstep(AEWardCampaignMode& Game, const FVector& At, float Radius)
{
    if (WardOnline80::Guest(Game))
    {
        Game.Noise = FMath::Max(Game.Noise,FMath::Clamp(Radius/1400.f,0.f,1.f));
        return;
    }
    const TGuardValue<bool> Footstep(FootstepInProgress, true);
    Game.BroadcastNoise(At, Radius);
}

void WardDecoy101::HearCart(AEWardCampaignMode& Game, FName Cart, ESound Sound, FVector At, float Radius)
{
    if (WardOnline80::Guest(Game) || Cart.IsNone()) return;
    const FCartPulse Pulse{Cart,Sound};
    const TGuardValue<const FCartPulse*> SoundContext(CartPulse,&Pulse);
    Game.BroadcastNoise(At,Radius);
}
namespace {TArray<TSharedPtr<FJsonValue>> Vec(FVector P){return {MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)};}}
AEWardHospitalMob::AEWardHospitalMob()
{
    PrimaryActorTick.bCanEverTick=true;GetCapsuleComponent()->InitCapsuleSize(30,88);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetCharacterMovement()->bRunPhysicsWithNoController=true;GetCharacterMovement()->MaxStepHeight=28;GetCharacterMovement()->bOrientRotationToMovement=false;bUseControllerRotationYaw=false;
}
void AEWardHospitalMob::Build(AEWardHospitalGameplay* StoryOwner,Json Row)
{
    Runtime=StoryOwner;Definition=Row;Id=String(Row,TEXT("id"));Species=String(Row,TEXT("species"));Class=String(Row,TEXT("combat_class"),TEXT("A"));FirstChapter=Number(Row,TEXT("chapter"),2);
    Home=Vector(Array(Row,TEXT("at")))*100+FVector(0,0,91);MaxHealth=Number(Row,TEXT("health"),70);GetCapsuleComponent()->SetCapsuleSize(Number(Row,TEXT("radius"),.3)*100,88);
    GetCharacterMovement()->MaxWalkSpeed=Number(Row,TEXT("speed"),1.5)*100;
    Configure59();ResetMob();
}
void AEWardHospitalMob::Configure59()
{
    Rule59=WardMonster59::Rule(Species);Zone59=String(Definition,TEXT("zone"));
    Guid59=String(Object(Object(WardMonster59::Data(),TEXT("individuals")),Id),TEXT("guid"),Id);
    if(Guid59.IsEmpty())Guid59=Id;
    const FString RuleClass=String(Rule59,TEXT("combat_class"));if(!RuleClass.IsEmpty())Class=RuleClass;
    Rule59->HasField(TEXT("first_chapter"));
    NodePowered59=Runtime&&Runtime->NodesNear(Home,2200);
    const Json Individual=Object(Object(WardMonster59::Data(),TEXT("individuals")),Id);
    Model61=String(Individual,TEXT("model"),Species+TEXT("_01"));
    const Json Model=Object(Object(WardMonster59::Data(),TEXT("models")),Model61);
    const float Height61=Number(Model,TEXT("height_cm"),178);
    GetCharacterMovement()->MaxAcceleration=Species==TEXT("hound")?650:Species==TEXT("armoured")?160:340;
    GetCharacterMovement()->BrakingDecelerationWalking=420;GetCharacterMovement()->GroundFriction=5;
    if(Species==TEXT("hound"))
    {
        auto* Movement=GetCharacterMovement();
        Movement->bUseSeparateBrakingFriction=true;
        Movement->BrakingFriction=2.4f;
        Movement->BrakingFrictionFactor=1.f;
        Movement->BrakingDecelerationWalking=320.f;
        Movement->GroundFriction=4.f;
    }
    GetCharacterMovement()->bEnablePhysicsInteraction=false;
    const float HalfHeight=FMath::Max(Height61*.5f,GetCapsuleComponent()->GetScaledCapsuleRadius());
    GetCapsuleComponent()->SetCapsuleHalfHeight(HalfHeight);
    Home=Vector(Array(Definition,TEXT("at")))*100+FVector(0,0,HalfHeight+3);
    Visual59=NewObject<UEWardMonsterVisual59>(this);Visual59->SetupAttachment(GetRootComponent());
    Visual59->SetRelativeLocation(FVector(0,0,-HalfHeight));Visual59->RegisterComponent();AddInstanceComponent(Visual59);
    GetMesh()->SetVisibility(false);LastPosition61=Home;
}
void AEWardHospitalMob::EnsureVisual61()
{
    if(!Visual59||Visual59->Vertices59()>0)return;
    const Json Model=Object(Object(WardMonster59::Data(),TEXT("models")),Model61);
    if(!Visual59->Load59(Model61,Number(Model,TEXT("height_cm"),178)))UE_LOG(LogTemp,Warning,TEXT("MONSTER61 missing rig %s"),*Model61);
    if(Dead){TArray<FVector> Points;for(auto V:Array(Definition,TEXT("_settled61")))Points.Add(Vector(V->AsArray()));if(Points.IsEmpty())Visual59->BeginFall61(-GetActorForwardVector());else Visual59->RestoreFall61(Points);}
}
void AEWardHospitalMob::ChangeState59(const FString& State)
{
    if(State59==State)return;
    State59=State;StateClock59=0;
    if(State==TEXT("windup"))Sound61(TEXT("windup"),.48f);
    else if(State==TEXT("dead"))Sound61(TEXT("fall"),.48f);
    else if(State==TEXT("chase")&&AudioClock61<=0){Sound61(TEXT("notice"),.42f);AudioClock61=5;}

    if(State==TEXT("windup")||State==TEXT("recovery")||State==TEXT("stunned")||State==TEXT("dead")||State==TEXT("dormant"))GetCharacterMovement()->StopMovementImmediately();
}
bool AEWardHospitalMob::Eligible59()const
{
    if(!Runtime||!Runtime->Mode||Dead||Runtime->Chapter()>19)return false;
    const int Ch=Runtime->Chapter();bool Chapter=Ch==FirstChapter;
    if(!Id.StartsWith(TEXT("CHILD_")))for(auto V:Array(Rule59,TEXT("reappear_chapters")))Chapter|=Ch==int(V->AsNumber());
    if(!Chapter||WardMonster59::ZoneState(Zone59,Ch)==TEXT("sealed"))return false;
    if(Species==TEXT("impostor")&&Runtime->NarrativeFlag35(TEXT("IDENTITY_CHECKED")))return false;
    return true;
}
void AEWardHospitalMob::ResetMob()
{
    WardVisits.Remove(this);
    CartInterests.Remove(this);
    LastSeen59=SearchOrigin59=Home;
    Definition->RemoveField(TEXT("_settled61"));BodyPresent59=false;Health=MaxHealth;Dead=false;Active=false;Stunned=0;Memory=0;AttackClock=1;SpecialClock=8;PathClock=0;Path.Reset();PathIndex=0;Target=Home;
    NPCTarget62.Reset();State59=TEXT("dormant");SearchLeft59=0;Windup59=0;Alert59=0;StunImmunity59=0;SpawnedOnDeath59=false;Revived59=false;DeathTime59=0;SeenPlayer59=false;HitGrace59=0;NoisePriority59=0;SearchStep59=0;
    SetActorLocation(Home,false);GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->DisableMovement();SetActorHiddenInGame(true);SetActorEnableCollision(false);
    if(Visual59){Visual59->ResetFall61();Visual59->Present59(0,0,false,0);}
    WalkClock=0;StepDistance61=0;LastPosition61=Home;BreathClock61=3;SpeechClock61=12;PatrolStep61=0;
}
bool AEWardHospitalMob::Sight59(FVector Point,bool IgnoreFov)const
{
    if(!Runtime||FMath::Abs(Point.Z-GetActorLocation().Z)>210)return false;
    const FVector Offset=Point-GetActorLocation();
    if(WardMonster59::CameraDisabled(*this)||WardMonster59::LinkedSignalDisabled(*this))return false;
    const float Sight=Number(Rule59,TEXT("sight_cm"),1250);
    if(!IgnoreFov&&(Sight<=0||Offset.Size2D()>Sight))return false;
    if(!IgnoreFov&&Offset.Size2D()>160&&FVector::DotProduct(Offset.GetSafeNormal2D(),GetActorForwardVector())<FMath::Cos(FMath::DegreesToRadians(float(Number(Rule59,TEXT("half_fov_deg"),65)))))return false;
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(MonsterSight59),true,this);
    return !GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()+FVector(0,0,35),Point+FVector(0,0,20),ECC_Visibility,Query)||Hit.GetActor()==Runtime->Mode->Hero();
}
bool AEWardHospitalMob::AttackClear59()const
{
    if(!Runtime||!Runtime->Mode->Hero())return false;
    const auto* G=Runtime->Mode.Get();FVector Hero=WardOnline80::MobPlayer(*const_cast<AEWardHospitalMob*>(this),false);
    const auto* Victim=Runtime->Props.FindRef(NPCTarget62);if(Victim)Hero=Victim->GetActorLocation()+FVector(0,0,91);
    if(Victim){auto* Human=Victim->FindComponentByClass<UEWardPerson54>();if(!Human||Human->IsDead58()||(Human->IsFallen58()&&!Human->Vulnerable62())||Victim->IsHidden())return false;}
    if((!Victim&&!WardOnline80::MobTargetsGuest(*const_cast<AEWardHospitalMob*>(this))&&
        (G->BookWorld->FurnitureHiding||G->HidingBed))||FMath::Abs(Hero.Z-GetActorLocation().Z)>115)return false;
    if(FVector::Dist2D(Hero,GetActorLocation())>Number(Rule59,TEXT("attack_reach_cm"),145))return false;
    return Sight59(Hero,true);
}
void AEWardHospitalMob::Hear(FVector At,float Radius)
{
    if(WardMonster59::CameraDisabled(*this)||WardMonster59::LinkedSignalDisabled(*this))return;
    if(!Active||Dead||Radius<=0||At.ContainsNaN()||!Runtime||FMath::Abs(At.Z-GetActorLocation().Z)>230)return;
    const float Distance=FVector::Dist2D(At,GetActorLocation());
    float Range=Radius*Number(Rule59,TEXT("hearing_gain"),1);
    const auto* Interest = CartInterests.Find(this);
    const bool SameCart = CartPulse && Interest && Interest->Cart==CartPulse->Cart && SearchStep59==CheckCart;
    const auto CartProfile = CartPulse ? CartReaction(*this,CartPulse->Sound) : FCartReaction{};
    if (CartPulse)
    {
        if ((CartProfile.Range<=0 && !SameCart) || State59==TEXT("chase") || Stunned>0 || State59==TEXT("windup") || State59==TEXT("recovery")) return;
        if (Interest && Interest->Cart==CartPulse->Cart && Interest->IgnoreUntil>GetWorld()->GetTimeSeconds()) return;
        Range *= SameCart ? FMath::Max(.80f,CartProfile.Range) : CartProfile.Range;
    }
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(MonsterHearing59),false,this);
    const FVector Height(0,0,FootstepInProgress?40:20);
    bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()+Height,At+Height,ECC_Visibility,Query)&&Hit.GetActor()!=Runtime->Mode->Hero();
    if (Blocked && CartPulse)
        for (const auto& Prop : Runtime->Mode->BookWorld->Movables)
            if (Prop.Body.Get()==Hit.GetComponent() && FName(*Prop.Id)==CartPulse->Cart) {Blocked=false;break;}
    const bool Glass=FootstepInProgress&&Blocked&&FrostedGlass(Hit);
    if(Blocked)Range*=Glass?.72f:.38f;
    if(Distance>Range)return;
    const float Strength=Range/FMath::Max(Distance,80.f);
    if (WardHideout102::Inspecting(*this)) return;
    if (!CartPulse && WardHideout102::Heard(*this,At)) return;
    if(State59==TEXT("chase")&&SeenPlayer59)return;
    if(State59==TEXT("investigate")&&SearchStep59==CheckLastSight)return;
    if(State59==TEXT("door_check"))return;
    if(FootstepInProgress&&(State59==TEXT("listen")||(State59==TEXT("search")&&SearchStep59>=WardSearchStart)))return;
    if (SearchStep59==CheckCart && !CartPulse && FootstepInProgress && Species!=TEXT("hound") && Strength<NoisePriority59*1.35f) return;
    if(!SameCart&&NoisePriority59>Strength&&Memory>1.5f)return;
    WardVisits.Remove(this);SearchStep59=0;
    if (CartPulse)
    {
        for (auto It=CartInterests.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
        CartInterests.Add(this,{CartPulse->Cart,0,0,CartPulse->Sound==WardDecoy101::ESound::Stop||CartPulse->Sound==WardDecoy101::ESound::Impact});
        SearchStep59=CheckCart;
    }
    NoisePriority59=Strength;Target=At;SearchOrigin59=At;
    Memory=Number(Rule59,TEXT("memory_seconds"),5);SearchLeft59=Number(Rule59,TEXT("search_seconds"),8);PathClock=0;
    if(Stunned>0||State59==TEXT("windup")||State59==TEXT("recovery"))return;
    if(Glass && (State59==TEXT("patrol")||State59==TEXT("return")||State59==TEXT("dormant")))
    {
        ChangeState59(TEXT("listen"));
        GetCharacterMovement()->StopMovementImmediately();
        ConsumeMovementInputVector();
    }
    else ChangeState59(TEXT("investigate"));
}
void AEWardHospitalMob::Stun59(float Seconds,bool Flash)
{
    if(!Active||Dead||Seconds<=0||!FMath::IsFinite(Seconds)||(Flash&&Boolean(Rule59,TEXT("flash_immune"))))return;
    if(StunImmunity59>0)return;
    Stunned=FMath::Clamp(Seconds,.1f,10.f);Windup59=0;AttackResolved59=true;
    StunImmunity59=Seconds+(Class==TEXT("C")?3.f:.35f);ChangeState59(TEXT("stunned"));
}
float AEWardHospitalMob::TakeDamage(float Damage,const FDamageEvent& Event,AController* EventInstigator,AActor* Causer)
{
    if(Dead||!Active||!Runtime||!FMath::IsFinite(Damage)||Damage<=0)return 0;
    if(Species==TEXT("armoured")&&Causer&&FVector::DotProduct((Causer->GetActorLocation()-GetActorLocation()).GetSafeNormal2D(),GetActorForwardVector())>.25f)Damage*=.15f;
    if(Species==TEXT("collector")&&Runtime->NodesNear(GetActorLocation(),2200))Damage*=.3f;
    if(Class==TEXT("C")){Stun59(.8f);Runtime->Mode->Cue(TEXT("metal_hit"),GetActorLocation(),.3f);return 0;}
    Health=FMath::Max(0.f,Health-Damage);
    if(Causer){LastSeen59=Causer->GetActorLocation();Target=LastSeen59;Memory=Number(Rule59,TEXT("memory_seconds"),5);SearchLeft59=8;PathClock=0;}
    if(Health<=0)
    {
        if(Class==TEXT("B")&&Runtime->Chapter()<19){Health=MaxHealth;StunImmunity59=0;Stun59(10);}
        else FinishDeath59();
    }
    else Stun59(Number(Rule59,TEXT("stagger_seconds"),.22));
    for(auto Other:Runtime->Mobs)if(Other!=this&&Other->Guid59==Guid59){Other->Health=Health;Other->Stunned=Stunned;Other->StunImmunity59=StunImmunity59;}
    return Damage;
}
void AEWardHospitalMob::FinishDeath59()
{
    if(Dead)return;
    EnsureVisual61();if(Visual59)Visual59->BeginFall61(-GetActorForwardVector());
    BodyPresent59=true;Dead=true;Active=false;Health=0;DeathTime59=0;Windup59=0;ChangeState59(TEXT("dead"));
    SetActorEnableCollision(false);GetCharacterMovement()->DisableMovement();SetActorHiddenInGame(false);++Runtime->Mode->Kills;
    for(auto Other:Runtime->Mobs)if(Other!=this&&Other->Guid59==Guid59){Other->BodyPresent59=false;Other->Dead=true;Other->Active=false;Other->Health=0;Other->ChangeState59(TEXT("dead"));Other->GetCharacterMovement()->DisableMovement();Other->SetActorEnableCollision(false);Other->SetActorHiddenInGame(true);}
    if(Species==TEXT("carrier")&&!SpawnedOnDeath59)
    {
        SpawnedOnDeath59=true;
        Runtime->SpawnChild(GetActorLocation()+FVector(100,0,0),TEXT("parasite"));
        Runtime->SpawnChild(GetActorLocation()+FVector(-100,0,0),TEXT("parasite"));
    }
}
void AEWardHospitalMob::Revive()
{
    if(!Dead||Revived59||Class!=TEXT("A")||!Runtime||Runtime->Chapter()!=FirstChapter)return;
    if(Visual59)Visual59->ResetFall61();
    Health=MaxHealth*.5f;Dead=false;Revived59=true;DeathTime59=0;Active=true;Stunned=0;Memory=0;ChangeState59(TEXT("patrol"));
    SetActorHiddenInGame(false);SetActorEnableCollision(true);GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
bool AEWardHospitalMob::CameraSees59()const
{
    if(Species!=TEXT("keeper")||!Runtime||!Runtime->Mode->Hero())return false;
    const auto* G=Runtime->Mode.Get();if(G->BookWorld->FurnitureHiding||G->HidingBed)return false;
    if(G->Flag(TEXT("monster_camera_B2_off59"))&&GetActorLocation().Z<-600)return false;
    if(G->Flag(TEXT("monster_camera_3F_off59"))&&GetActorLocation().Z>600)return false;
    FVector Hero=G->Hero()->GetActorLocation();
    const auto* Victim=Runtime->Props.FindRef(NPCTarget62);if(Victim)Hero=Victim->GetActorLocation()+FVector(0,0,91);
    const float Gain=Runtime->NarrativeFlag35(TEXT("POWER_FULL"))?1.f:.55f;
    const float Angle=FMath::Sin(GetWorld()->GetTimeSeconds()*2*PI/6.f)*62.f;
    for(auto V:Array(WardMonster59::Data(),TEXT("network_devices")))
    {
        Json Device=V->AsObject();if(String(Device,TEXT("type"))!=TEXT("camera"))continue;
        const auto* Room=G->BookWorld->Rooms.FindByPredicate([&](const auto& R){return R.Id==String(Device,TEXT("zone"));});if(!Room)continue;
        if(!Room->Bounds.ExpandBy(80).IsInside(Hero)||FVector::Dist2D(GetActorLocation(),Hero)>2200*Gain)continue;
        const FVector Camera=Room->Bounds.GetCenter()+FVector(0,0,110),To=Hero-Camera;
        const FVector Direction=(Room->Door-Camera).GetSafeNormal2D().RotateAngleAxis(Angle,FVector::UpVector);
        if(FVector::DotProduct(Direction,To.GetSafeNormal2D())<.5f)continue;
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(KeeperCamera59),false,this);
        if(!GetWorld()->LineTraceSingleByChannel(Hit,Camera,Hero,ECC_Visibility,Q)||Hit.GetActor()==G->Hero())return true;
    }
    return false;
}
void AEWardHospitalMob::Special59(float Delta,bool Visible)
{
    const bool Powered=Runtime->NodesNear(GetActorLocation(),2200);
    if(NodePowered59&&!Powered&&(Species==TEXT("linked")||Species==TEXT("conductor")||Species==TEXT("pursuer")))
    {Memory=0;SearchLeft59=0;StunImmunity59=0;Stun59(2.5f);}
    NodePowered59=Powered;
    if(SpecialClock>0)return;
    SpecialClock=Species==TEXT("nest")?14.f:7.f;
    auto* G=Runtime->Mode.Get();const FVector P=GetActorLocation(),Hero=G->Hero()->GetActorLocation();
    if(Species==TEXT("nest")&&(Visible||Memory>0)&&FVector::Dist2D(P,Hero)<1500)
    {
        int Local=0;for(auto M:Runtime->Mobs)if(M->Species==TEXT("parasite")&&!M->Dead&&FVector::Dist(M->GetActorLocation(),P)<1600)++Local;
        if(Local<4)Runtime->SpawnChild(P+FVector(150,0,0),TEXT("parasite"));
    }
    if((Species==TEXT("linked")||Species==TEXT("conductor"))&&Powered&&Visible)
        for(auto Other:Runtime->Mobs)if(Other!=this&&Other->Active&&!Other->Dead&&FVector::Dist(Other->GetActorLocation(),P)<1800&&
            (Other->Species==TEXT("linked")||Species==TEXT("conductor")))Other->Hear(LastSeen59,2000);
    if(Species==TEXT("transporter"))for(auto Other:Runtime->Mobs)if(Other!=this&&Other->Dead&&!Other->Revived59&&Other->Class==TEXT("A")&&Other->FirstChapter==Runtime->Chapter()&&FVector::Dist(P,Other->GetActorLocation())<700)
    {Other->Revive();G->Cue(TEXT("drag"),P,.3f);break;}
    if(Species==TEXT("collector")&&Powered)Health=FMath::Min(MaxHealth,Health+12);
    if(Species==TEXT("repeater")&&FVector::Dist2D(P,Hero)<1600)G->Cue(TEXT("creak"),P,.22f);
}
void AEWardHospitalMob::Move59(float Delta,float Speed)
{
    if(Boolean(Rule59,TEXT("stationary"))){GetCharacterMovement()->StopMovementImmediately();return;}
    GetCharacterMovement()->MaxWalkSpeed=Speed;
    if(PathClock<=0)
    {
        PathClock=1.2f;Path=WardMonster59::Path(*this,GetActorLocation(),Target);PathIndex=0;
        // A closed ordinary door blocks the navigation grid. Walk to its near
        // side first; never grant routes through locks, bolts or refuge doors.
        if(Path.IsEmpty()&&Species==TEXT("patients"))
        {
            float Best=MAX_flt;
            for(const auto& Door:Runtime->Mode->BookWorld->Portals)
            {
                if(Door.Open||Door.Locked||Door.InsideOnly||Door.Type!=TEXT("ordinary")||Runtime->Mode->BookWorld->RefugeBolts.Contains(Door.Zone)||FMath::Abs(GetActorLocation().Z-Door.At.Z-91)>100||FVector::Dist2D(GetActorLocation(),Door.At)>750)continue;
                const FVector Near=Door.At+(GetActorLocation()-Door.At).GetSafeNormal2D()*190+FVector(0,0,91);
                auto Route=WardMonster59::Path(*this,GetActorLocation(),Near);
                const float Score=FVector::Dist2D(GetActorLocation(),Door.At)+FVector::Dist2D(Door.At,Target);
                if(!Route.IsEmpty()&&Score<Best){Path=MoveTemp(Route);Best=Score;}
            }
        }
    }
    if(Path.IsValidIndex(PathIndex))
    {
        FVector Direction=Path[PathIndex]-GetActorLocation();Direction.Z=0;
        if(Direction.Size2D()<28){++PathIndex;return;}
        const auto& Style=WardMonsterMotion97::Style(Species);
        const FVector Next=Path.IsValidIndex(PathIndex+1)?Path[PathIndex+1]-Path[PathIndex]:FVector::ZeroVector;
        const FVector Facing=WardMonsterMotion97::CornerFacing(Direction,Next,Direction.Size2D());
        const FRotator Rotation=FMath::RInterpConstantTo(GetActorRotation(),Facing.Rotation(),Delta,Style.TurnRate);
        SetActorRotation(Rotation);
        const float Angle=FMath::Abs(FMath::FindDeltaAngleDegrees(Rotation.Yaw,Direction.Rotation().Yaw));
        const float Load=FMath::Clamp(Angle/100.f,0.f,1.f);
        GetCharacterMovement()->MaxWalkSpeed=Speed*FMath::Lerp(1.f,Style.Pounce?.62f:.45f,Load);
        AddMovementInput(Direction.GetSafeNormal2D(),1);
    }
    ProgressClock59+=Delta;
    if(ProgressClock59>1.5f)
    {
        if(FVector::DistSquared2D(ProgressAt59,GetActorLocation())<FMath::Square(15.f))PathClock=0;
        ProgressAt59=GetActorLocation();ProgressClock59=0;
    }
    auto* World=Runtime->Mode->BookWorld.Get();
    if(Species==TEXT("patients"))for(auto& Door:World->Portals)if(!Door.Open&&!Door.Locked&&!Door.InsideOnly&&Door.Type==TEXT("ordinary")&&!World->RefugeBolts.Contains(Door.Zone)&&FVector::Dist2D(GetActorLocation(),Door.At)<205&&FMath::Abs(GetActorLocation().Z-Door.At.Z-91)<90)
    {
        if(WardDoor100::Secured(*World,Door))continue;
        WardContact98::BeginDoor(*World,&Door-World->Portals.GetData(),GetActorLocation(),false,true);
        for(auto& Other:World->Portals)if(Other.At.Equals(Door.At,1)&&!Other.Locked){Other.Open=true;World->RefreshPortalVisual(Other);}PathClock=0;Runtime->Mode->Cue(TEXT("door"),Door.At,.25f);break;
    }
    if(Boolean(Rule59,TEXT("barricade_break"))&&Memory>0)
        for(auto& Prop:World->Movables)if(Prop.Pushable&&Prop.Body&&FVector::Dist2D(GetActorLocation(),Prop.Body->GetComponentLocation())<170&&FMath::Abs(GetActorLocation().Z-Prop.Body->GetComponentLocation().Z)<150)
        {if(WardDecoy101::MoveObstacle(*World,&Prop-World->Movables.GetData(),(Target-GetActorLocation()).GetSafeNormal2D()))PathClock=0;break;}
}
void AEWardHospitalMob::StepBrain59(float D)
{
    auto* G=Runtime->Mode.Get();auto* H=G->Hero();if(!H)return;
    const FVector P=GetActorLocation();FVector Hero=WardOnline80::MobPlayer(*this,true);
    if(WardMonster59::LinkedSignalDisabled(*this))
    {
        Memory=0;SearchLeft59=0;Windup59=0;SeenPlayer59=false;
        NPCTarget62.Reset();AttackResolved59=true;
        Stunned=FMath::Max(0.f,Stunned-D);
        StunImmunity59=FMath::Max(0.f,StunImmunity59-D);
        ChangeState59(TEXT("dormant"));
        GetCharacterMovement()->StopMovementImmediately();
        return;
    }
    if(State59!=TEXT("windup"))
    {
        NPCTarget62.Empty();float Best=Sight59(Hero)?FVector::Dist2D(P,Hero):MAX_flt;
        for(const auto& Pair:Runtime->Props)
        {
            auto* A=Pair.Value;if(!A||A->IsHidden()||Boolean(Runtime->Definitions.FindRef(Pair.Key),TEXT("past57")))continue;
            // Human locomotion is simulated in the player's 16 m activity
            // bubble. Never kill an NPC whose avoidance tick is suspended.
            if(FVector::DistSquared(A->GetActorLocation(),H->GetActorLocation())>FMath::Square(1600.f))continue;
            auto* Human=A->FindComponentByClass<UEWardPerson54>();if(!Human||Human->IsDead58()||(Human->IsFallen58()&&!Human->Vulnerable62()))continue;
            FVector At=A->GetActorLocation()+FVector(0,0,91);float Dist=FVector::Dist2D(P,At);
            if(Dist<Best&&Dist<1500&&(Sight59(At)||(Dist<125&&Sight59(At,true)))){Best=Dist;NPCTarget62=Pair.Key;}
        }
    }
    if(auto* Victim=Runtime->Props.FindRef(NPCTarget62))Hero=Victim->GetActorLocation()+FVector(0,0,91);
    const float Distance=FVector::Dist2D(P,Hero);
    StateClock59+=D;AttackClock=FMath::Max(0.f,AttackClock-D);PathClock-=D;SpecialClock-=D;StunImmunity59=FMath::Max(0.f,StunImmunity59-D);NoisePriority59=FMath::Max(0.f,NoisePriority59-D*.25f);
    if(Stunned>0)
    {
        Stunned=FMath::Max(0.f,Stunned-D);ChangeState59(TEXT("stunned"));GetCharacterMovement()->StopMovementImmediately();
        if(Stunned<=0){Target=SearchOrigin59;SearchStep59=0;SearchLeft59=Number(Rule59,TEXT("search_seconds"),8);ChangeState59(TEXT("search"));}return;
    }
    const bool FinishingSearch=State59==TEXT("chase")||SearchStep59==CheckLastSight||SearchStep59==CheckCart||State59==TEXT("door_check")||
        (State59==TEXT("search")&&SearchStep59>=WardSearchStart);
    if(Distance>6000||(!FinishingSearch&&FMath::Abs(P.Z-Hero.Z)>230)){GetCharacterMovement()->StopMovementImmediately();return;}
    const bool Hidden=NPCTarget62.IsEmpty()&&!WardOnline80::MobTargetsGuest(*this)&&(G->BookWorld->FurnitureHiding||G->HidingBed);
    const bool Visible=!Hidden&&(Sight59(Hero)||CameraSees59());
    SeenPlayer59=Visible;
    if (WardHideout102::Inspecting(*this) && !Visible)
    {
        if (!WardHideout102::AdvanceInspection(*this,D)) Move59(D,112.f);
        return;
    }
    if(State59==TEXT("listen")&&!Visible)
    {
        if(StateClock59<ListenSeconds)
        {
            GetCharacterMovement()->StopMovementImmediately();
            ConsumeMovementInputVector();
            const FVector Facing=(Target-P).GetSafeNormal2D();
            if(!Facing.IsNearlyZero())SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),Facing.Rotation(),D,65));
            return;
        }
        ChangeState59(TEXT("investigate"));
    }
    if(State59==TEXT("windup"))
    {
        GetCharacterMovement()->StopMovementImmediately();Windup59-=D;
        if(!Boolean(Rule59,TEXT("stationary"))&&FVector::DistSquared2D(AttackAim59,P)>FMath::Square(18.f))
        {
            const auto& Style=WardMonsterMotion97::Style(Species);
            const FVector Aim=(AttackAim59-P).GetSafeNormal2D();
            SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),Aim.Rotation(),D,Style.TurnRate));
        }
        if(Windup59<=0)
        {
            if(!AttackResolved59&&AttackClear59()&&FVector::Dist2D(Hero,AttackAim59)<115)
            {
                const float Lunge=Number(Rule59,TEXT("attack_lunge_cm"),0);
                if(Lunge>0){FHitResult Hit;AddActorWorldOffset((AttackAim59-P).GetSafeNormal2D()*Lunge,true,&Hit);}
                if(auto* Victim=Runtime->Props.FindRef(NPCTarget62))
                {if(auto* Human=Victim->FindComponentByClass<UEWardPerson54>())Human->Impact58(Number(Definition,TEXT("damage"),12),(Hero-P).GetSafeNormal2D()*120);}
                else if (!WardOnline80::DamageGuest(*this,Number(Definition,TEXT("damage"),12)))
                    G->DamageHero(Number(Definition,TEXT("damage"),12),this);
                Sound61(TEXT("impact"),.46f);
            }
            AttackResolved59=true;AttackClock=Number(Rule59,TEXT("recovery_seconds"),1)*WardOnline80::RecoveryScale(*G);ChangeState59(TEXT("recovery"));
        }return;
    }
    if(State59==TEXT("recovery"))
    {
        if(AttackClock>0){GetCharacterMovement()->StopMovementImmediately();return;}
        if(!Visible&&Memory>0)
        {
            SearchStep59=CheckLastSight;Target=SearchOrigin59=LastSeen59;PathClock=0;
            ChangeState59(TEXT("investigate"));
        }
        else ChangeState59(TEXT("search"));
    }
    Memory=FMath::Max(0.f,Memory-D);
    if(Visible)
    {
        if(State59!=TEXT("chase")){WardVisits.Remove(this);SearchStep59=0;}
        LastSeen59=Hero;Target=Hero;SearchOrigin59=Hero;Memory=Number(Rule59,TEXT("memory_seconds"),5);SearchLeft59=Number(Rule59,TEXT("search_seconds"),8);ChangeState59(TEXT("chase"));
    }
    else if(Species==TEXT("hound")&&!Hidden&&H->Health<65&&Distance<700&&Sight59(Hero,true))
    {LastSeen59=Hero;Target=Hero;Memory=2;SearchLeft59=10;ChangeState59(TEXT("investigate"));}
    else if(State59==TEXT("chase"))
    {
        SearchStep59=CheckLastSight;Target=LastSeen59;SearchOrigin59=LastSeen59;PathClock=0;
        ChangeState59(TEXT("investigate"));
    }
    // Blind opponents can feel an exposed player at contact distance, but
    // neither sound memory nor contact is allowed to damage through a wall.
    if(!Hidden&&AttackClock<=0&&(Visible||Memory>0||Distance<100)&&AttackClear59())
    {
        AttackAim59=Hero;AttackResolved59=false;Windup59=Number(Rule59,TEXT("windup_seconds"),.65);StrikeVisual=Windup59+.25f;
        ChangeState59(TEXT("windup"));return;
    }
    Special59(D,Visible);if(Stunned>0)return;
    if(!Visible&&(State59==TEXT("investigate")||State59==TEXT("search")||State59==TEXT("door_check")))
    {
        if(SearchStep59==CheckCart && InspectCart(*this,D))return;
        if(SearchStep59==CheckLastSight)
        {
            if(FVector::Dist2D(P,LastSeen59)<85||StateClock59>FMath::Clamp(float(Number(Rule59,TEXT("memory_seconds")))+3,5.f,11.f))
            {
                SearchStep59=WardSearchStart;
                SearchLeft59=FMath::Clamp(SearchLeft59+7.f,12.f,18.f);
                ChangeState59(TEXT("search"));
            }
        }
        if(SearchStep59>=WardSearchStart)
        {
            if(SearchWards(*this,D))return;
        }
        else if(SearchStep59!=CheckLastSight&&SearchStep59!=CheckCart&&State59!=TEXT("return")&&(Memory<=0||FVector::Dist2D(P,Target)<85))
        {
            ChangeState59(TEXT("search"));SearchLeft59=FMath::Max(0.f,SearchLeft59-D);
            if(SearchLeft59<=0){Target=Home;PathClock=0;ChangeState59(TEXT("return"));}
            else if(StateClock59>1.5f||FVector::Dist2D(P,Target)<80)
            {
                const float Angle=SearchStep59++*137.5f;const FVector Offset=FVector(220,0,0).RotateAngleAxis(Angle,FVector::UpVector);
                const FVector Candidate=SearchOrigin59+Offset;const FIntVector Cell=Runtime->CellAt(Candidate);
                if(Runtime->FloorCell(Cell,GetCapsuleComponent()->GetScaledCapsuleRadius())){Target=Runtime->WorldAt(Cell);PathClock=0;}StateClock59=0;
            }
        }
    }
    if(State59==TEXT("return")&&FVector::Dist2D(P,Home)<100){ChangeState59(TEXT("patrol"));Target=Home;}
    if(State59==TEXT("patrol")&&(FVector::Dist2D(P,Target)<80||Path.IsEmpty())&&StateClock59>Number(Rule59,TEXT("patrol_pause_seconds"),1.4))
    {
        const float Radius=Number(Rule59,TEXT("patrol_cm"),550);const float Angle=(PatrolStep61++%8)*45.f+(GetTypeHash(Guid59)%360);
        const FVector Candidate=Home+FVector(Radius,0,0).RotateAngleAxis(Angle,FVector::UpVector);
        const FIntVector Cell=Runtime->CellAt(Candidate);if(Runtime->FloorCell(Cell,GetCapsuleComponent()->GetScaledCapsuleRadius())){Target=Runtime->WorldAt(Cell);PathClock=0;}StateClock59=0;
    }
    const float PatrolSpeed=Number(Definition,TEXT("speed"),1.5)*100*.65f;
    const float Speed=State59==TEXT("chase")?Number(Rule59,TEXT("chase_speed_cm"),210):
        SearchStep59==CheckLastSight?FMath::Max(PatrolSpeed,145.f):SearchStep59>=WardSearchStart?FMath::Max(PatrolSpeed,115.f):PatrolSpeed;
    Move59(D,Speed);
}
void AEWardHospitalMob::Tick(float D)
{
    Super::Tick(D);
    if(!Runtime||!Runtime->Mode)return;
    auto* G=Runtime->Mode.Get();auto* H=G->Hero();if(!H)return;
    const bool Online = WardOnline80::PlayingMap(*G);
    if (WardOnline80::Guest(*G))
    {
        GetCharacterMovement()->StopMovementImmediately();
        if (IsHidden() || (!Active && !BodyPresent59)) return;
        EnsureVisual61();
        StateClock59 += D;
        if (Dead) { DeathTime59+=D; if (Visual59) Visual59->TickFall61(D); }
        else Presentation61(D);
        return;
    }
    const bool Frozen=Runtime->FreezeAI||G->Won||(!Online&&(H->Dead||(!G->Screen.IsEmpty()&&G->Screen!=TEXT("story35")&&G->Screen!=WardSecurity103::Page)))||G->Admin||!G->CampaignStarted||G->TitleView||Runtime->HomeStage52>=0||Runtime->ArrivalCinematic();
    if(Frozen){GetCharacterMovement()->StopMovementImmediately();return;}
    const float Distance=FVector::Dist2D(GetActorLocation(),H->GetActorLocation());
    if((Active||BodyPresent59)&&Distance<4500)EnsureVisual61();
    else if(Visual59&&Distance>5500)
    {
        TArray<TSharedPtr<FJsonValue>> Fall;for(auto Point:Visual59->CaptureFall61())Fall.Add(MakeShared<FJsonValueArray>(Vec(Point)));if(!Fall.IsEmpty())Definition->SetArrayField(TEXT("_settled61"),Fall);Visual59->Unload61();
    }
    if(Dead){DeathTime59+=D;if(Visual59&&!IsHidden())Visual59->TickFall61(D);return;}
    if(!Active)return;
    Presentation61(D);
    StepBrain59(FMath::Min(D,.1f));
}
Json AEWardHospitalMob::Snapshot()const
{
    Json S=MakeShared<FJsonObject>();S->SetStringField(TEXT("id"),Id);S->SetNumberField(TEXT("health"),Health);S->SetBoolField(TEXT("dead"),Dead);S->SetArrayField(TEXT("at"),Vec(GetActorLocation()));
    S->SetBoolField(TEXT("active70"),Active);
    S->SetBoolField(TEXT("seen70"),SeenPlayer59);S->SetStringField(TEXT("npc_target70"),NPCTarget62);
    S->SetNumberField(TEXT("state_clock70"),StateClock59);S->SetNumberField(TEXT("search_step70"),SearchStep59);
    if (const auto* Interest = CartInterests.Find(this))
    {
        const float Cooldown = FMath::Max(0.,Interest->IgnoreUntil-GetWorld()->GetTimeSeconds());
        if (SearchStep59==CheckCart || Cooldown>0)
        {
            const auto Cart = MakeShared<FJsonObject>();
            Cart->SetStringField(TEXT("id"),Interest->Cart.ToString());
            Cart->SetNumberField(TEXT("ignore_seconds"),Cooldown);
            S->SetObjectField(TEXT("cart101"),Cart);
        }
    }
    S->SetBoolField(TEXT("body_present59"),BodyPresent59);S->SetStringField(TEXT("brain59"),State59);S->SetNumberField(TEXT("memory59"),Memory);S->SetNumberField(TEXT("search59"),SearchLeft59);S->SetNumberField(TEXT("stun59"),Stunned);S->SetNumberField(TEXT("stun_immunity59"),StunImmunity59);
    S->SetNumberField(TEXT("attack_clock59"),AttackClock);S->SetNumberField(TEXT("special_clock59"),SpecialClock);S->SetBoolField(TEXT("spawned59"),SpawnedOnDeath59);S->SetBoolField(TEXT("revived59"),Revived59);
    S->SetArrayField(TEXT("target59"),Vec(Target));S->SetArrayField(TEXT("last_seen59"),Vec(LastSeen59));S->SetArrayField(TEXT("search_origin59"),Vec(SearchOrigin59));S->SetNumberField(TEXT("yaw59"),GetActorRotation().Yaw);
    S->SetNumberField(TEXT("gait61"),WalkClock);S->SetNumberField(TEXT("patrol61"),PatrolStep61);
    TArray<TSharedPtr<FJsonValue>> Fall;if(Visual59)for(auto Point:Visual59->CaptureFall61())Fall.Add(MakeShared<FJsonValueArray>(Vec(Point)));if(Fall.IsEmpty())Fall=Array(Definition,TEXT("_settled61"));S->SetArrayField(TEXT("fall61"),Fall);
    if(Id.StartsWith(TEXT("CHILD_")))S->SetObjectField(TEXT("definition"),Definition);return S;
}
void AEWardHospitalMob::Restore(Json S)
{
    WardVisits.Remove(this);
    CartInterests.Remove(this);
    Health=FMath::Clamp(float(Number(S,TEXT("health"),MaxHealth)),0.f,MaxHealth);Dead=Boolean(S,TEXT("dead"));BodyPresent59=Boolean(S,TEXT("body_present59"),Dead);
    FVector At=Vector(Array(S,TEXT("at")),Home);if(!S->HasField(TEXT("brain59")))At.Z+=GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-88;if(!At.ContainsNaN()&&Runtime->Mode->BookWorld->ValidPosition(At))SetActorLocation(At);
    const float Yaw=Number(S,TEXT("yaw59"),0);if(FMath::IsFinite(Yaw))SetActorRotation(FRotator(0,Yaw,0));
    State59=String(S,TEXT("brain59"),Dead?TEXT("dead"):TEXT("patrol"));
    if(State59==TEXT("door_check"))State59=TEXT("search");
    if(State59==TEXT("hideout_search")||State59==TEXT("hideout_check")) {State59=TEXT("investigate");}
    if(!TArray<FString>{TEXT("dead"),TEXT("dormant"),TEXT("patrol"),TEXT("chase"),TEXT("listen"),TEXT("investigate"),TEXT("search"),TEXT("return"),TEXT("stunned"),TEXT("windup"),TEXT("recovery")}.Contains(State59))State59=TEXT("patrol");
    Memory=FMath::Clamp(float(Number(S,TEXT("memory59"))),0.f,30.f);SearchLeft59=FMath::Clamp(float(Number(S,TEXT("search59"))),0.f,30.f);Stunned=FMath::Clamp(float(Number(S,TEXT("stun59"))),0.f,10.f);StunImmunity59=FMath::Clamp(float(Number(S,TEXT("stun_immunity59"))),0.f,15.f);
    AttackClock=FMath::Clamp(float(Number(S,TEXT("attack_clock59"),1)),.4f,5.f);SpecialClock=FMath::Clamp(float(Number(S,TEXT("special_clock59"),7)),0.f,20.f);
    SpawnedOnDeath59=Boolean(S,TEXT("spawned59"),Dead);Revived59=Boolean(S,TEXT("revived59"));
    Target=Vector(Array(S,TEXT("target59")),Home);LastSeen59=Vector(Array(S,TEXT("last_seen59")),Home);SearchOrigin59=Vector(Array(S,TEXT("search_origin59")),LastSeen59);
    if(Target.ContainsNaN())Target=Home;if(LastSeen59.ContainsNaN())LastSeen59=Home;if(SearchOrigin59.ContainsNaN())SearchOrigin59=Home;
    WalkClock=FMath::Max(0.f,float(Number(S,TEXT("gait61"))));PatrolStep61=FMath::Clamp(int(Number(S,TEXT("patrol61"))),0,100000);LastPosition61=GetActorLocation();
    if(Visual59)Visual59->ResetFall61();
    Definition->SetArrayField(TEXT("_settled61"),Array(S,TEXT("fall61")));
    if(Dead){EnsureVisual61();TArray<FVector> Points;for(auto V:Array(S,TEXT("fall61")))Points.Add(Vector(V->AsArray()));if(Visual59){if(Points.IsEmpty())Visual59->BeginFall61(-GetActorForwardVector());else Visual59->RestoreFall61(Points);}}
    // Loading cannot deliver a half-finished melee hit without a new warning.
    if(State59==TEXT("windup"))State59=TEXT("recovery");Windup59=0;AttackResolved59=true;Path.Reset();PathClock=0;PathIndex=0;StateClock59=FMath::Max(0.f,float(Number(S,TEXT("state_clock70"))));DeathTime59=Dead?1.f:0;
    SeenPlayer59=Boolean(S,TEXT("seen70"));SearchStep59=FMath::Clamp(int(Number(S,TEXT("search_step70"))),CheckCart,WardSearchStart+5);
    const auto Cart = Object(S,TEXT("cart101"));
    const FString CartId = String(Cart,TEXT("id"));
    const bool KnownCart = Runtime->Mode->BookWorld->Movables.ContainsByPredicate([&](const auto& Prop){return Prop.Type==TEXT("gurney") && Prop.Id==CartId;});
    if (KnownCart) CartInterests.Add(this,{FName(*CartId),GetWorld()->GetTimeSeconds()+FMath::Clamp(Number(Cart,TEXT("ignore_seconds")),0.,30.),0,true});
    else if(SearchStep59==CheckCart) SearchStep59=0;
    if(State59==TEXT("search")&&SearchStep59>WardSearchStart)--SearchStep59;
    NPCTarget62=String(S,TEXT("npc_target70"));
    if(!Runtime->Props.Contains(NPCTarget62))NPCTarget62.Reset();
    Active=Boolean(S,TEXT("active70"),!Dead&&State59!=TEXT("dormant"))&&Eligible59();
    GetCharacterMovement()->SetMovementMode(Active?MOVE_Walking:MOVE_None);
}

void AEWardHospitalMob::Sound61(const FString& Event,float Volume)
{
    if(!Runtime||!Runtime->Mode||!Runtime->Mode->Hero()||IsHidden())return;
    const FVector Hero=Runtime->Mode->Hero()->GetActorLocation();
    if(FVector::DistSquared(Hero,GetActorLocation())>FMath::Square(2600.f))return;
    const int Take=SoundTake61++%(Event==TEXT("foot")?3:2);
    const FString File=FPaths::ProjectContentDir()/TEXT("CampaignData/Monsters61/audio")/Species/(Event+FString::FromInt(Take)+TEXT(".wav"));
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(CreatureAudio61),false,this);
    if(GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()+FVector(0,0,25),Hero,ECC_Visibility,Q)&&Hit.GetActor()!=Runtime->Mode->Hero())Volume*=.28f;
    WardHomeAssets::PlaySpatialFile61(this,File,Volume*Runtime->Mode->Setting(TEXT("effects"),.8f),GetActorLocation());
    if(Event==TEXT("speech"))
    {
        const auto& Lines=Array(Rule59,TEXT("speech"));
        if(Lines.IsValidIndex(Take)&&FVector::Dist2D(Hero,GetActorLocation())<1100&&Runtime->Mode->SettingBool(TEXT("subtitles"),true))Runtime->Mode->Notify(Lines[Take]->AsString(),3);
    }
}
void AEWardHospitalMob::Presentation61(float D)
{
    AudioClock61=FMath::Max(0.f,AudioClock61-D);BreathClock61-=D;SpeechClock61-=D;
    const bool Replica=WardOnline80::Guest(*Runtime->Mode);
    const float Speed=Replica?Number(Definition,TEXT("_coop_speed")):GetVelocity().Size2D();
    float Travel=Replica?Speed*D:FVector::Dist2D(GetActorLocation(),LastPosition61);LastPosition61=GetActorLocation();if(Travel>100)Travel=0;
    const Json Model=Object(Object(WardMonster59::Data(),TEXT("models")),Model61);
    const float Stride=FMath::Max(18.f,float(Number(Model,TEXT("stride_cm"),70)));
    const bool Listening=State59==TEXT("listen")||State59==TEXT("door_check")||State59==TEXT("hideout_check");
    const int Clip=Listening?0:State59==TEXT("windup")||State59==TEXT("recovery")?2:Stunned>0?3:Speed>180?5:Speed>8?1:0;
    const float Period=Clip==5?(Species==TEXT("parasite")?.42f:.70f):Number(Model,TEXT("period"),1.0f);
    WalkClock+=Speed>8?Travel/Stride*Period:D;
    StepDistance61+=Travel;
    if(!Listening&&StepDistance61>Stride*.5f&&Speed>8&&!Boolean(Rule59,TEXT("stationary"))){StepDistance61=FMath::Fmod(StepDistance61,Stride*.5f);Sound61(TEXT("foot"),State59==TEXT("chase")?.42f:.24f);}
    if(!Listening&&BreathClock61<=0){BreathClock61=4+(GetTypeHash(Id)%6);Sound61(TEXT("breath"),.22f);}
    if(!Listening&&SpeechClock61<=0){SpeechClock61=24+(GetTypeHash(Id)%17);if(!Array(Rule59,TEXT("speech")).IsEmpty()&&State59!=TEXT("windup")&&Stunned<=0)Sound61(TEXT("speech"),.35f);}
    if(Visual59){float T=Listening?.38f:Clip==2?(State59==TEXT("windup")?StateClock59/FMath::Max(.1f,float(Number(Rule59,TEXT("windup_seconds"),.65)))*.74f:.74f+FMath::Min(.26f,StateClock59*.26f)):Clip==3?StateClock59:WalkClock;Visual59->Present59(Clip,T,false,0);}
}

