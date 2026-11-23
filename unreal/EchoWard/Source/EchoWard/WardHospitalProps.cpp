#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardCampaign.h"
#include "WardEchoAssets.h"
#include "WardContact98.h"
#include "WardDoor100.h"
#include "WardDecoy101.h"
#include "WardHideout102.h"
#include "WardOnline80.h"
#include "WardMotion107Math.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
using namespace WardData;

namespace
{
using WardContact98::EGrip;
constexpr float DoorDuration = .56f;
constexpr float DoorReachTime = .15f;
constexpr float MaxStep = 6.f;
constexpr float MinCartReach = .15f;
struct FDoorMotion { float Age = 0; bool Open = false; };
struct FHandleCheck { float Age = 0; bool Audible = true; bool HitStop = false; };
struct FRoll { FVector Velocity; float Rattle = 0; bool Distraction = false; };
bool MovedByMonster = false;
struct FReach
{
    EGrip Grip = EGrip::Door;
    int32 Index = INDEX_NONE;
    float Age = 0;
    FVector Origin = FVector::ZeroVector;
    FVector Face = FVector::ForwardVector;
    TWeakObjectPtr<AActor> Item;
    bool Pull = false;
    FVector StartView = FVector::ForwardVector;
};
struct FContactRuntime
{
    FTimerHandle Timer;
    double Updated = 0;
    TMap<FVector, FDoorMotion> Doors;
    TMap<FVector, FHandleCheck> Handles;
    TMap<int32, FRoll> Carts;
    FReach Reach;
};
TMap<TWeakObjectPtr<AEWardBookWorld>, FContactRuntime> ContactWorlds;

float DoorCurve(float Age)
{
    return WardMotion107::DoorSwing(Age);
}

FContactRuntime& Runtime(AEWardBookWorld& World)
{
    for (auto It = ContactWorlds.CreateIterator(); It; ++It)
        if (!It.Key().IsValid()) It.RemoveCurrent();
    auto& State = ContactWorlds.FindOrAdd(&World);
    if (!State.Timer.IsValid())
    {
        State.Updated = World.GetWorld()->GetTimeSeconds();
        World.GetWorldTimerManager().SetTimer(State.Timer, FTimerDelegate::CreateWeakLambda(&World, [&World]()
        {
            auto& Current = ContactWorlds.FindChecked(&World);
            const double Now = World.GetWorld()->GetTimeSeconds();
            const float Delta = float(Now - Current.Updated);
            if (Delta < .001f) return;
            Current.Updated = Now;
            WardContact98::Advance(World, FMath::Min(Delta, .09f));
        }), 1.f / 60.f, true);
    }
    return State;
}

bool RollingProp(const AEWardBookWorld::FMovable& Prop)
{
    return Prop.Pushable && (Prop.Type == TEXT("gurney") || Prop.Type == TEXT("wheelchair") || Prop.Type == TEXT("tray"));
}

struct FCartGrip
{
    FVector Right;
    FVector Left;
    FVector Facing;
};

FCartGrip CartGrip(const AEWardBookWorld::FMovable& Prop, const FVector& Approach)
{
    const bool AlongX = FMath::Abs(Approach.X) * Prop.Half.Y >= FMath::Abs(Approach.Y) * Prop.Half.X;
    const FVector Facing = AlongX ? FVector(FMath::Sign(Approach.X), 0, 0) : FVector(0, FMath::Sign(Approach.Y), 0);
    const FVector Side = FVector::CrossProduct(FVector::UpVector, Facing);
    const double Depth = AlongX ? Prop.Half.X : Prop.Half.Y;
    const double HalfWidth = AlongX ? Prop.Half.Y : Prop.Half.X;
    const double Height = Prop.Type == TEXT("gurney") ? 78 : Prop.Half.Z * 1.6;
    const FVector Centre = Prop.Body->GetComponentLocation() - Facing * (Depth + 4)
        + FVector(0, 0, Height - Prop.Half.Z);
    const FVector HandSpan = Side * FMath::Clamp(HalfWidth * .58, 5., 18.);
    return {Centre + HandSpan, Centre - HandSpan, Facing};
}

struct FDoorGrip
{
    FVector Centre;
    FQuat Panel;
    float Face;
};

FDoorGrip DoorHandle(const AEWardBookWorld::FPortal& Door, const FVector& Player, float Open, float PressDegrees=0)
{
    const bool NS = Door.Side == TEXT("north") || Door.Side == TEXT("south");
    const FVector Along = NS ? FVector::ForwardVector : FVector::RightVector;
    const bool Right = Door.Width > 160 && FVector::DotProduct(Player - Door.At, Along) > 0;
    const FVector Hinge = Door.At + Along * (Door.Width * .5 - 6) * (Right ? 1 : -1);
    const float Side = FMath::Sign(NS ? Player.Y - Door.At.Y : Door.At.X - Player.X) * (Right ? -1 : 1);
    const float Inward = Door.Side == TEXT("north") || Door.Side == TEXT("east") ? Door.OpenSign37 : -Door.OpenSign37;
    const FQuat Rotation=FRotator(0,(NS?0:90)+(Right?180:0)+Open*Inward*(Right?-90:90),0).Quaternion();
    const double Width=(Door.Width-12)/(Door.Width>160?2:1);
    const auto* Lever=Door.VisibleParts.FindByPredicate([&](const auto& Part){
        return Part.Material==WardDoor100::LeverMaterial&&Part.Leaf==(Right?1:0)&&Part.Centre.Y*Side>0;
    });
    FVector Local(Width-13,Side*9,115);
    if(Lever)
    {
        const FVector Pivot(Width-13,Lever->Centre.Y,115);
        const FQuat Press(FVector::RightVector,FMath::DegreesToRadians(-PressDegrees));
        Local=Pivot+Press.RotateVector(Lever->Centre-Pivot);
    }
    return {Hinge+Rotation.RotateVector(Local),Rotation,Side};
}

FVector Shoulder(const AEWardCampaignHero& Hero, bool Left = false)
{
    return Hero.GetActorLocation() + FVector(0, 0, Hero.bIsCrouched ? 29 : 50) + Hero.GetActorRightVector() * (Left ? -17 : 17);
}

float TargetReach(const AEWardCampaignHero& Hero, const FVector& Target, bool Left = false)
{
    return 1.f-FMath::SmoothStep(70.,103.,FVector::Dist(Shoulder(Hero,Left),Target));
}

float ViewReach(const AEWardCampaignHero& Hero, const FReach& Reach)
{
    const FVector View = Hero.GetControlRotation().Vector().GetSafeNormal2D();
    return FMath::SmoothStep(.10, .64, FVector::DotProduct(View, Reach.StartView));
}

float KeyLift(float Age)
{
    return FMath::SmoothStep(.16f, .38f, Age);
}

FVector KeyCarryPoint(const FReach& Reach)
{
    return Reach.Origin + (-Reach.Face * 25 + FVector(0,0,23)) * KeyLift(Reach.Age);
}

void RollSound(AEWardCampaignMode& Game, const AEWardBookWorld::FMovable& Prop, const FRoll& Roll, WardDecoy101::ESound Sound)
{
    using WardDecoy101::ESound;
    const FVector At = Prop.Body->GetComponentLocation();
    const bool Push = Sound==ESound::Push, Stop = Sound==ESound::Stop;
    const float Radius = Prop.Noise*(Push?1.f:Stop?.85f:1.2f);
    Game.Cue(Push?TEXT("scrape"):TEXT("metal_hit"), At, Push?.28f:Stop?.14f:.34f);
    if (Prop.Type==TEXT("gurney"))
    {
        if (Roll.Distraction) WardDecoy101::HearCart(Game,FName(*Prop.Id),Sound,At,Radius);
    }
    else if (!Stop) Game.BroadcastNoise(At,Radius);
}

// The movable's blocking child is not swept by SetWorldLocation. Sweep its box explicitly.
bool Travel(AEWardBookWorld& World, AEWardBookWorld::FMovable& Cart, const FVector& Offset, bool& Bump)
{
    auto* Game = WardCampaign(&World);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardCart98), false, Game->Hero());
    Query.AddIgnoredComponent(Cart.Body.Get());
    const FVector Start = Cart.Body->GetComponentLocation();
    FVector End = Start + Offset;
    const double Base = Cart.Home.Z - Cart.Half.Z;
    double Highest = Base - MaxStep;
    double Lowest = Base + MaxStep;
    FHitResult Hit;
    for (int X : {-1, 1}) for (int Y : {-1, 1})
    {
        const double XReach = Cart.Half.X + (X * Offset.X > 0 ? 1 : -2);
        const double YReach = Cart.Half.Y + (Y * Offset.Y > 0 ? 1 : -2);
        const FVector Foot(End.X + X * XReach, End.Y + Y * YReach, Base);
        if (!World.GetWorld()->LineTraceSingleByChannel(Hit, Foot + FVector(0,0,Cart.Half.Z*.5), Foot - FVector(0,0,MaxStep), ECC_Visibility, Query) ||
            Hit.ImpactNormal.Z < .6 || FMath::Abs(Hit.ImpactPoint.Z - Base) > MaxStep) return false;
        Highest = FMath::Max(Highest, Hit.ImpactPoint.Z);
        Lowest = FMath::Min(Lowest, Hit.ImpactPoint.Z);
    }
    if (Highest - Lowest > MaxStep) return false;
    const auto Shape = FCollisionShape::MakeBox(Cart.Half-FVector(1));
    End.Z = Highest + Cart.Half.Z;
    // The leading wheels can be past a narrow threshold while it is still below the frame.
    if (End.Z < Start.Z && World.GetWorld()->OverlapBlockingTestByChannel(End, FQuat::Identity, ECC_Pawn, Shape, Query))
        End.Z = Start.Z;
    if (auto* Hero = Game->Hero())
    {
        const FVector Player = Hero->GetActorLocation();
        const FVector Nearest(FMath::Clamp(Player.X, End.X-Cart.Half.X, End.X+Cart.Half.X),
            FMath::Clamp(Player.Y, End.Y-Cart.Half.Y, End.Y+Cart.Half.Y), Player.Z);
        const auto* Capsule = Hero->GetCapsuleComponent();
        if (FMath::Abs(Player.Z-End.Z)<Cart.Half.Z+Capsule->GetScaledCapsuleHalfHeight() &&
            FVector::DistSquared2D(Player, Nearest)<FMath::Square(Capsule->GetScaledCapsuleRadius()+2)) return false;
    }
    const FVector Lift(0, 0, FMath::Max(0., End.Z-Start.Z));
    if (World.GetWorld()->SweepSingleByChannel(Hit, Start+Lift, FVector(End.X, End.Y, Start.Z+Lift.Z), FQuat::Identity, ECC_Pawn, Shape, Query)) return false;
    Bump = FMath::Abs(End.Z - Start.Z) >= 1.4;
    Cart.Body->SetWorldLocation(End);
    return true;
}
}

void AEWardBookWorld::BuildMovables()
{
    const auto Add = [&](FMovable P)
    {
        auto* Body=NewObject<UBoxComponent>(this);Body->SetupAttachment(RootComponent);Body->SetMobility(EComponentMobility::Movable);Body->SetBoxExtent(P.Half);Body->SetRelativeLocation(P.Home);Body->SetCollisionProfileName(TEXT("BlockAll"));Body->RegisterComponent();AddInstanceComponent(Body);P.Body=Body;
        auto Piece=[&](FVector At,FVector Size,int Mat,bool Wheel=false)
        {
            if (P.Type==TEXT("gurney") && P.Half.X>P.Half.Y){Swap(At.X,At.Y);Swap(Size.X,Size.Y);}
            auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(Body);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Wheel?TEXT("/Engine/BasicShapes/Cylinder.Cylinder"):TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(At);C->SetRelativeScale3D(Size/100);if(Wheel)C->SetRelativeRotation(FRotator(90,0,0));C->SetMaterial(0,Palette[Mat]);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetAffectDynamicIndirectLighting(false);C->SetAffectDistanceFieldLighting(false);C->SetLightingChannels(true,true,false);C->RegisterComponent();AddInstanceComponent(C);
        };
        const FVector H=P.Half;
        const int CustomId=P.Type==TEXT("wheelchair")?184:P.Type==TEXT("gurney")?183:P.Type==TEXT("screen")?189:P.Type==TEXT("tray")?212:639;
        if(WardEchoAssets::Attach(this,Body,CustomId,FBox(-H,H))){Movables.Add(P);return;}
        if(P.Type==TEXT("wheelchair"))
        {
            Piece(FVector(0,0,45-H.Z),FVector(60,55,8),5);Piece(FVector(-28,0,78-H.Z),FVector(8,55,60),5);
            for(int S:{-1,1}){Piece(FVector(-15,S*36,31-H.Z),FVector(60,60,7),11,true);Piece(FVector(30,S*28,10-H.Z),FVector(18,18,6),11,true);}
        }
        else if(P.Type==TEXT("gurney"))
        {
            Piece(FVector(0,0,73-H.Z),FVector(80,195,16),2);Piece(FVector(0,0,55-H.Z),FVector(75,180,8),11);
            for(int X:{-1,1})for(int Y:{-1,1}){Piece(FVector(X*32,Y*80,32-H.Z),FVector(7,7,60),11);Piece(FVector(X*32,Y*80,9-H.Z),FVector(17,17,6),5,true);}
        }
        else if(P.Type==TEXT("screen"))
        {Piece(FVector::ZeroVector,FVector(8,130,170),2);for(int S:{-1,1}){Piece(FVector(0,S*65,0),FVector(6,6,190),11);Piece(FVector(0,S*65,8-H.Z),FVector(32,8,8),11);}}
        else Piece(FVector::ZeroVector,H*2,11,P.Type==TEXT("bell"));
        Movables.Add(P);
    };
    for(auto V:Array(WardBookMapData(),TEXT("movable_props")))
    {
        const Json D=V->AsObject();
        FMovable P;
        P.Id=String(D,TEXT("id"));P.Type=String(D,TEXT("type"));P.Half=Vector(Array(D,TEXT("size")))*50;
        P.Home=Vector(Array(D,TEXT("at")))*100+FVector(0,0,P.Half.Z);P.Noise=Number(D,TEXT("noise_radius_m"),9)*100;
        P.Pushable=Boolean(D,TEXT("pushable"));
        Add(MoveTemp(P));
    }
    // The first patient encounters are on the treatment floor. Place a usable cart off the approach to F202.
    if (const auto* Room = Rooms.FindByPredicate([](const auto& Entry){return Entry.Id==TEXT("F202");}))
        Add({TEXT("PROP_gurney_F202"),TEXT("gurney"),FVector(100,42.5,42.5),
            Room->Door+FVector(540,385,42.5-98),900,true,nullptr});
    Sign(TEXT("INTERACTION BAY / F PUSH / SHIFT+F PULL"),FVector(-12000,5300,235),FRotator(0,-90,0),16);
}

bool WardDoor100::HasLever(const AEWardBookWorld::FPortal& Door)
{
    return Door.Role!=TEXT("niche") && (Door.Type==TEXT("ordinary") || Door.Type==TEXT("manual") || Door.Type==TEXT("bolt"));
}

bool WardDecoy101::MoveObstacle(AEWardBookWorld& World, int32 Index, FVector Direction)
{
    const TGuardValue<bool> Origin(MovedByMonster,true);
    return World.PushMovable(Index,Direction);
}

bool WardDoor100::Secured(const AEWardBookWorld& World, const AEWardBookWorld::FPortal& Door)
{
    return World.Portals.ContainsByPredicate([&](const auto& Leaf){
        return Leaf.At.Equals(Door.At,1) && (Leaf.Locked || Leaf.InsideOnly || World.RefugeBolts.Contains(Leaf.Zone));
    });
}

bool WardDoor100::Begin(AEWardBookWorld& World, const AEWardBookWorld::FPortal& Door, bool Audible, float Age)
{
    if (Door.Open || !Door.Leaf || !HasLever(Door) || !Secured(World,Door)) return false;
    if (!Door.VisibleParts.ContainsByPredicate([](const auto& Part){ return Part.Material == LeverMaterial; })) return false;
    auto& Handles = Runtime(World).Handles;
    if (Handles.Contains(Door.At)) return false;
    Handles.Add(Door.At,{FMath::Clamp(Age,0.f,Duration),Audible,Age>=.84f});
    return true;
}

float WardDoor100::CheckAge(const AEWardBookWorld& World, FVector At)
{
    if (const auto* State = ContactWorlds.Find(&World))
        if (const auto* Check = State->Handles.Find(At)) return Check->Age;
    return -1;
}

float WardDoor100::LeverAngle(const AEWardBookWorld& World, FVector At)
{
    const float Age = CheckAge(World,At);
    const float Check=32.f*FMath::SmoothStep(.18f,.88f,Age)*(1.f-FMath::SmoothStep(1.08f,1.48f,Age));
    if (const auto* State=ContactWorlds.Find(&World))
        if (const auto* Door=State->Doors.Find(At))
            return FMath::Max(Check,WardMotion107::LatchDegrees(Door->Age,Door->Open));
    return Check;
}

namespace WardContact98
{
void BeginDoor(AEWardBookWorld& World, int32 Index, const FVector& Player, bool Local, bool Open)
{
    if (!World.Portals.IsValidIndex(Index)) return;
    const auto& Door = World.Portals[Index];
    if (Door.Type == TEXT("shutter") || Door.Type == TEXT("biometric") || Door.VisibleParts.IsEmpty()) return;
    auto& State = Runtime(World);
    const auto* Previous = State.Doors.Find(Door.At);
    const float InitialAge = Previous && Previous->Open != Open && Previous->Age > DoorReachTime ?
        DoorReachTime + DoorDuration - Previous->Age : 0.f;
    State.Doors.Add(Door.At, {InitialAge, Open});
    auto* Game = WardCampaign(&World);
    if (!Local || !Game || !Game->Hero()) return;
    auto* Hero = Game->Hero();
    const FVector Handle = DoorHandle(Door, Player, Door.Open ? 1 : 0).Centre;
    if (FVector::DistSquared(Shoulder(*Hero), Handle) > FMath::Square(92.)) return;
    if (auto* Item = State.Reach.Item.Get()) Item->Destroy();
    State.Reach = {EGrip::Door, Index, 0, Player, (Handle-Player).GetSafeNormal2D(), nullptr, false,
        Hero->GetControlRotation().Vector().GetSafeNormal2D()};
}

float DoorOpenAmount(const AEWardBookWorld& World, const FVector& At, bool Open)
{
    if (const auto* State = ContactWorlds.Find(&World))
        if (const auto* Motion = State->Doors.Find(At); Motion && Motion->Open == Open)
        {
            const float Alpha = DoorCurve(Motion->Age);
            return Open ? Alpha : 1.f - Alpha;
        }
    return Open ? 1.f : 0.f;
}

int32 AimedCart(AEWardBookWorld& World, AEWardCampaignHero& Hero)
{
    const FVector Eye = Hero.Camera->GetComponentLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardCartFocus98), false, &Hero);
    FHitResult Hit;
    if (!World.GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Hero.GetControlRotation().Vector() * 190, ECC_Visibility, Query))
        return INDEX_NONE;
    return World.Movables.IndexOfByPredicate([&](const auto& Prop) { return Prop.Body.Get() == Hit.GetComponent() && RollingProp(Prop); });
}

bool PushCart(AEWardBookWorld& World, int32 Index, const FVector& Player, bool Pull, bool Local)
{
    auto* Game = WardCampaign(&World);
    if (!Game || !World.Movables.IsValidIndex(Index)) return false;
    auto& Prop = World.Movables[Index];
    if (!Prop.Body || !RollingProp(Prop)) return false;
    const FVector At = Prop.Body->GetComponentLocation();
    const FVector Facing = (At - Player).GetSafeNormal2D();
    if (Facing.IsNearlyZero() || FMath::Abs(Player.Z-At.Z)>140 || FVector::DistSquared2D(Player, At)>FMath::Square(210.)) return false;
    const FCartGrip Targets = CartGrip(Prop,Facing);
    if (Local && Game->Hero() && TargetReach(*Game->Hero(),Targets.Right) > MinCartReach &&
        TargetReach(*Game->Hero(),Targets.Left,true) > MinCartReach)
    {
        auto& State = Runtime(World);
        if (State.Reach.Grip == EGrip::Cart && State.Reach.Index != INDEX_NONE && State.Reach.Age < .50f) return true;
        if (auto* Item = State.Reach.Item.Get()) Item->Destroy();
        State.Reach = {EGrip::Cart, Index, 0, Player, Facing, nullptr, Pull,
            Game->Hero()->GetControlRotation().Vector().GetSafeNormal2D()};
        return true;
    }
    return World.PushMovable(Index, Facing * (Pull ? -1 : 1));
}

void KeyPickup(AEWardCampaignHero& Hero, AActor& Item)
{
    auto* Game = WardCampaign(&Hero);
    if (!Game || !Game->BookWorld || !Game->Screen.IsEmpty()) return;
    const FVector At = Item.GetActorLocation() + FVector(0, 0, 4);
    if (FVector::DistSquared(Shoulder(Hero), At) > FMath::Square(96.)) return;
    const FVector Facing = (At - Hero.GetActorLocation()).GetSafeNormal2D(UE_SMALL_NUMBER,Hero.GetActorForwardVector());
    auto* Visual = Hero.GetWorld()->SpawnActor<AActor>();
    if (!Visual) return;
    Visual->SetFlags(RF_Transient);
    auto* Root = NewObject<USceneComponent>(Visual);
    Visual->SetRootComponent(Root);
    Root->RegisterComponent();
    Visual->SetActorLocation(Item.GetActorLocation());
    TInlineComponentArray<UStaticMeshComponent*> Meshes(&Item);
    for (const auto* Mesh : Meshes)
    {
        if (!Mesh->GetStaticMesh() || !Mesh->GetVisibleFlag()) continue;
        auto* Copy = NewObject<UStaticMeshComponent>(Visual);
        Copy->SetStaticMesh(Mesh->GetStaticMesh());
        Copy->SetupAttachment(Root);
        Copy->SetRelativeTransform(Mesh->GetComponentTransform().GetRelativeTransform(Item.GetActorTransform()));
        Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Copy->SetCastShadow(false);
        Copy->SetLightingChannels(true, true, false);
        for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index) Copy->SetMaterial(Index, Mesh->GetMaterial(Index));
        Copy->RegisterComponent();
        Visual->AddInstanceComponent(Copy);
    }
    Visual->SetActorRotation(Item.GetActorRotation());
    Visual->SetLifeSpan(.74f);
    auto& Reach = Runtime(*Game->BookWorld).Reach;
    if (auto* Previous = Reach.Item.Get()) Previous->Destroy();
    Reach = {EGrip::Key, INDEX_NONE, 0, At, Facing, Visual, false,
        Hero.GetControlRotation().Vector().GetSafeNormal2D()};
}

FPose Pose(const AEWardCampaignHero& Hero)
{
    const auto* Game = WardCampaign(&Hero);
    if (!Game || !Game->BookWorld || Hero.Dead || Hero.IsHidden() || !Game->Screen.IsEmpty()) return {};
    const auto* State = ContactWorlds.Find(Game->BookWorld.Get());
    if (!State) return {};
    const auto& Reach = State->Reach;
    if (!Reach.Item.IsValid() && Reach.Index == INDEX_NONE) return {};
    const float Release = Reach.Grip == EGrip::Door ? .39f : .43f;
    FPose Result;
    Result.Grip = Reach.Grip;
    Result.Reach = FMath::SmoothStep(0.f, .15f, Reach.Age) * (1.f-FMath::SmoothStep(Release, Release+.21f, Reach.Age));
    Result.GripAmount = FMath::SmoothStep(.08f, .18f, Reach.Age) * (1.f-FMath::SmoothStep(Release, Release+.10f, Reach.Age));
    FVector Touch = Reach.Origin;
    float Pitch = -8.f;
    float Yaw=Reach.Face.Rotation().Yaw;
    if (Reach.Grip == EGrip::Door && Game->BookWorld->Portals.IsValidIndex(Reach.Index))
    {
        const auto& Door = Game->BookWorld->Portals[Reach.Index];
        const float Swing=DoorOpenAmount(*Game->BookWorld,Door.At,Door.Open);
        const float Press=WardDoor100::LeverAngle(*Game->BookWorld,Door.At);
        const auto Grip=DoorHandle(Door,Reach.Origin,Swing,Press);
        Touch=Grip.Centre+Grip.Panel.RotateVector(FVector(0,Grip.Face*5,-1));
        const float Tangent=Grip.Panel.RotateVector(FVector(0,-Grip.Face,0)).Rotation().Yaw;
        Yaw+=FMath::Clamp(FMath::FindDeltaAngleDegrees(Yaw,Tangent),-52.f,52.f)*.64f;
        Pitch-=Press*.58f;
    }
    else if (Reach.Grip == EGrip::Cart)
    {
        if (!Game->BookWorld->Movables.IsValidIndex(Reach.Index)) return {};
        const auto& Cart = Game->BookWorld->Movables[Reach.Index];
        if (!Cart.Body) return {};
        const FCartGrip Grips = CartGrip(Cart, Reach.Face);
        Result.Right = Grips.Right;
        Result.Left = Grips.Left;
        Yaw = Grips.Facing.Rotation().Yaw;
        Pitch = -31.f;
    }
    else if (Reach.Grip == EGrip::Key)
    {
        const float Lift = KeyLift(Reach.Age);
        Touch = KeyCarryPoint(Reach) - Reach.Face * 9;
        Pitch = -44.f + 24.f * Lift;
    }
    if (Reach.Grip != EGrip::Cart) Result.Right = Touch;
    float Accessible = TargetReach(Hero,Result.Right);
    if (Reach.Grip == EGrip::Cart) Accessible = FMath::Min(Accessible,TargetReach(Hero,Result.Left,true));
    Accessible *= ViewReach(Hero,Reach);
    Result.Reach *= Accessible;
    Result.GripAmount *= Accessible;
    Result.Rotation = FRotator(Pitch, Yaw, Reach.Grip == EGrip::Door ? 5 : 82).Quaternion();
    return Result;
}

void Advance(AEWardBookWorld& World, float Delta)
{
    auto* State = ContactWorlds.Find(&World);
    auto* Game = WardCampaign(&World);
    if (!State || !Game || !Game->Hero()) return;
    auto& Reach = State->Reach;
    if (Reach.Item.IsValid() || Reach.Index != INDEX_NONE)
    {
        const float Before = Reach.Age;
        Reach.Age += Delta;
        if (Reach.Grip == EGrip::Cart && Before < .15f && Reach.Age >= .15f && World.Movables.IsValidIndex(Reach.Index) &&
            Game->Screen.IsEmpty() && !Game->Hero()->Dead && !WardOnline80::Guest(*Game))
        {
            const auto& Cart = World.Movables[Reach.Index];
            const bool HasGrip = Cart.Body && (WardOnline80::PlayingMap(*Game) ?
                FVector::DistSquared(Game->Hero()->GetActorLocation(), Cart.Body->GetComponentLocation()) < FMath::Square(210.) :
                Pose(*Game->Hero()).Reach > MinCartReach);
            if (HasGrip)
                if (!World.PushMovable(Reach.Index, Reach.Face * (Reach.Pull ? -1 : 1)))
                    Game->Notify(TEXT("Каталка упирается в препятствие."));
        }
        if (auto* Item = Reach.Item.Get())
        {
            FVector At = KeyCarryPoint(Reach) - FVector(0,0,4);
            if (Reach.Age > .18f && Game->Hero()->BekaBody)
            {
                auto* Mesh = Game->Hero()->BekaBody.Get();
                const FName Index(TEXT("index_03_r")), Thumb(TEXT("thumb_03_r"));
                if (Mesh->DoesSocketExist(Index) && Mesh->DoesSocketExist(Thumb) && !Mesh->IsBoneHiddenByName(TEXT("hand_r")))
                {
                    const FVector Pinch = (Mesh->GetSocketLocation(Index)+Mesh->GetSocketLocation(Thumb))*.5;
                    const float Lock = FMath::SmoothStep(.18f,.32f,Reach.Age)*Pose(*Game->Hero()).Reach;
                    At += (Pinch-At).GetClampedToMaxSize(11)*Lock;
                }
            }
            Item->SetActorLocation(At);
            if (Reach.Age >= .60f || Game->Hero()->Dead || !Game->Screen.IsEmpty()) Item->Destroy();
        }
        if (Reach.Age >= .66f) Reach = {};
    }
    for (auto It = State->Doors.CreateIterator(); It; ++It)
    {
        It.Value().Age += Delta;
        bool Valid = false;
        for (auto& Door : World.Portals) if (Door.At == It.Key() && Door.Open == It.Value().Open)
        {
            if (!Door.Open && It.Value().Age >= .45f && !WardOnline80::Guest(*Game))
            {
                const bool NS = Door.Side == TEXT("north") || Door.Side == TEXT("south");
                const auto Offset = Game->Hero()->GetActorLocation() - Door.At;
                const bool Occupied = FMath::Abs(Offset.Z - 100) < 100 &&
                    FMath::Abs(NS ? Offset.X : Offset.Y) < Door.Width * .5 + 30 &&
                    FMath::Abs(NS ? Offset.Y : Offset.X) < 45;
                if (Occupied || WardOnline80::DoorwayOccupied(*Game, Door.At, NS, Door.Width)) Door.Open = true;
            }
            World.RefreshPortalVisual(Door);
            Valid |= Door.Open == It.Value().Open;
        }
        if (!Valid || It.Value().Age >= DoorDuration) It.RemoveCurrent();
    }
    for (auto It = State->Handles.CreateIterator(); It; ++It)
    {
        auto& Check = It.Value();
        Check.Age += Delta;
        const auto* Door = World.Portals.FindByPredicate([&](const auto& Leaf){return Leaf.At==It.Key() && !Leaf.Open;});
        const bool Done = !Door || Door->Open || Check.Age >= WardDoor100::Duration;
        if (Door && !Check.HitStop && Check.Age >= .84f)
        {
            Check.HitStop = true;
            if (Check.Audible) Game->Cue(WardDoor100::Secured(World,*Door)?TEXT("DOOR/refuge"):TEXT("DOOR/lock"),Door->At+FVector(0,0,115),.20f);
        }
        if (Done) Check.Age = WardDoor100::Duration;
        for (auto& Leaf : World.Portals) if (Leaf.At == It.Key()) World.RefreshPortalVisual(Leaf);
        if (Done) It.RemoveCurrent();
    }
    if (WardOnline80::Guest(*Game)) return;
    for (auto It = State->Carts.CreateIterator(); It; ++It)
    {
        if (!World.Movables.IsValidIndex(It.Key())) { It.RemoveCurrent(); continue; }
        auto& Cart = World.Movables[It.Key()];
        auto& Roll = It.Value();
        Roll.Rattle -= Delta;
        const int32 Steps = FMath::Max(1,FMath::CeilToInt(Roll.Velocity.Size2D()*Delta/5.));
        bool Bump = false;
        bool Blocked = false;
        for (int32 Step=0; Step<Steps; ++Step)
        {
            bool CrossedEdge = false;
            if (!Travel(World,Cart,Roll.Velocity*(Delta/Steps),CrossedEdge)) { Blocked=true; break; }
            Bump |= CrossedEdge;
        }
        if (Blocked)
        {
            const bool Hard = Roll.Velocity.Size2D() > 45 && Roll.Rattle <= 0;
            RollSound(*Game,Cart,Roll,Hard?WardDecoy101::ESound::Impact:WardDecoy101::ESound::Stop);
            It.RemoveCurrent();
            continue;
        }
        if (Bump && Roll.Rattle <= 0)
        {
            RollSound(*Game,Cart,Roll,WardDecoy101::ESound::Rattle);
            Roll.Rattle = .18f;
            Roll.Velocity *= .78;
        }
        const double Friction = Roll.Distraction ? 120. : 170.;
        const double Speed = FMath::Max(0., Roll.Velocity.Size2D() - Friction * Delta);
        if (Speed < 7)
        {
            RollSound(*Game,Cart,Roll,WardDecoy101::ESound::Stop);
            It.RemoveCurrent();
        }
        else Roll.Velocity = Roll.Velocity.GetSafeNormal2D() * Speed;
    }
}

void ClearMotion(AEWardBookWorld& World)
{
    if (auto* State = ContactWorlds.Find(&World))
    {
        if (auto* Item = State->Reach.Item.Get()) Item->Destroy();
        TArray<FVector> CheckedDoors;
        State->Handles.GetKeys(CheckedDoors);
        World.GetWorldTimerManager().ClearTimer(State->Timer);
        ContactWorlds.Remove(&World);
        for (auto& Door : World.Portals) if (CheckedDoors.Contains(Door.At)) World.RefreshPortalVisual(Door);
    }
}
}

bool AEWardBookWorld::PushMovable(int Index,FVector Direction)
{
    if(!Movables.IsValidIndex(Index)||!Mode.IsValid())return false;
    auto& P=Movables[Index];const FVector A=P.Body->GetComponentLocation(),B=A+Direction.GetSafeNormal2D()*60;
    if (RollingProp(P))
    {
        const FVector Facing = Direction.GetSafeNormal2D();
        if (Facing.IsNearlyZero()) return false;
        auto& State = Runtime(*this);
        bool Bump = false;
        if (!Travel(*this, P, Facing*12, Bump))
        {
            if (!State.Carts.Contains(Index)) Mode->Cue(TEXT("metal_hit"), A, .20f);
            return false;
        }
        const bool Distraction = P.Type==TEXT("gurney") && !MovedByMonster;
        auto& Roll = State.Carts.Add(Index,{Facing*(Distraction?230:125),Bump?.18f:0.f,Distraction});
        RollSound(*Mode.Get(),P,Roll,Bump?WardDecoy101::ESound::Rattle:WardDecoy101::ESound::Push);
        return true;
    }
    if(P.Pushable)
    {
        // Pulling must stop before the moved box overlaps the player's capsule.
        if(auto* H=Mode->Hero())
        {
            const FVector Player=H->GetActorLocation();const float Radius=H->GetCapsuleComponent()->GetScaledCapsuleRadius()+2;
            const FVector Closest(FMath::Clamp(Player.X,B.X-P.Half.X,B.X+P.Half.X),FMath::Clamp(Player.Y,B.Y-P.Half.Y,B.Y+P.Half.Y),Player.Z);
            if(FMath::Abs(Player.Z-B.Z)<P.Half.Z+H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()&&FVector::DistSquared2D(Player,Closest)<Radius*Radius)return false;
        }
        FCollisionQueryParams Q(SCENE_QUERY_STAT(HospitalPush),false,Mode->Hero());Q.AddIgnoredComponent(P.Body.Get());FHitResult Hit;
        if(GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(P.Half-FVector(1)),Q))return false;
        // Never push a prop across a ledge or an open stair shaft.
        for(int X:{-1,1})for(int Y:{-1,1})
        {const FVector Top=B+FVector(X*P.Half.X,Y*P.Half.Y,0);if(!GetWorld()->LineTraceSingleByChannel(Hit,Top,Top-FVector(0,0,P.Half.Z+50),ECC_Visibility,Q)||FMath::Abs(Hit.ImpactPoint.Z-(B.Z-P.Half.Z))>3)return false;}
        P.Body->SetWorldLocation(B);
    }
    Mode->Cue(P.Type==TEXT("bell")||P.Type==TEXT("tray")?TEXT("metal_hit"):TEXT("scrape"),P.Body->GetComponentLocation(),.3f);
    Mode->BroadcastNoise(P.Body->GetComponentLocation(),P.Noise);return true;
}
bool AEWardBookWorld::UseMovable(FVector Player)
{
    if(!Mode.IsValid()||!Mode->Hero())return false;
    auto* Hero = Mode->Hero();
    const auto* PC = Cast<APlayerController>(Hero->Controller);
    const bool Pull = PC && PC->IsInputKeyDown(EKeys::LeftShift);
    const int32 Cart = WardContact98::AimedCart(*this, *Hero);
    if (Cart != INDEX_NONE)
    {
        if (!WardContact98::PushCart(*this, Cart, Player, Pull, true)) Mode->Notify(TEXT("Каталка упирается в препятствие."));
        return true;
    }
    int Index=INDEX_NONE;double Best=210*210;
    for(int I=0;I<Movables.Num();++I)
    {
        const FVector P=Movables[I].Body->GetComponentLocation();const double D=FVector::DistSquared2D(Player,P);
        if (RollingProp(Movables[I])) continue;
        if(FMath::Abs(Player.Z-98-(P.Z-Movables[I].Half.Z))<100&&D<Best)
        {
            FCollisionQueryParams Q(SCENE_QUERY_STAT(HospitalPropFocus),false,Mode->Hero());FHitResult Hit;
            if(GetWorld()->LineTraceSingleByChannel(Hit,Player,P,ECC_Visibility,Q)&&Hit.GetComponent()!=Movables[I].Body.Get())continue;
            Index=I;Best=D;
        }
    }
    if(Index==INDEX_NONE)return false;
    FVector Direction=Movables[Index].Body->GetComponentLocation()-Player;
    if(Pull)Direction=-Direction;
    Mode->Notify(PushMovable(Index,Direction)?TEXT("Предмет сдвинут. Шум привлекает внимание."):TEXT("Предмет упирается в препятствие."));return true;
}
void AEWardBookWorld::ResetMovables()
{WardContact98::ClearMotion(*this);for(auto& P:Movables)if(P.Body)P.Body->SetRelativeLocation(P.Home);RefugeBolts.Reset();WardHideout102::Reset(*this);}
