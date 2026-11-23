#include "WardBookWorld.h"
#include "WardCombat63.h"
#include "WardOnline80.h"
#include "WardHospitalGameplay.h"
#include "WardBookMap.h"
#include "WardCampaign.h"
#include "WardGameplay76.h"
#include "WardAudio68.h"
#include "WardContact98.h"
#include "WardDoor100.h"
#include "WardHideout102.h"
#include "WardSecurity103.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "WardEchoAssets.h"
#include "Misc/Paths.h"
using namespace WardData;

namespace
{
struct FHideout102
{
    FVector Origin, Size;
    bool Bed;
    TWeakObjectPtr<USceneComponent> Doors[2];
    FVector Mouth() const { return Origin+FVector(Size.X*.5,Size.Y+65,98); }
};

struct FHideoutState102
{
    TArray<FHideout102> Items;
    TSet<TWeakObjectPtr<AEWardHospitalMob>> Investigators;
    int32 Active = INDEX_NONE;
    int32 MovingDoor = INDEX_NONE;
    double OpenedAt = 0;
    float SwingTime = 0;
    float SwingAngle = 0;
    FTimerHandle Timer;
};
TMap<TWeakObjectPtr<AEWardBookWorld>,FHideoutState102> Hideouts102;

void SetDoors102(FHideout102& Hideout, float Angle)
{
    for (int32 I=0;I<2;++I)
        if (auto* Door=Hideout.Doors[I].Get()) Door->SetRelativeRotation(FRotator(0,I==0?Angle:-Angle,0));
}

void Swing102(AEWardBookWorld& World,FHideoutState102& State,int32 Index,float Angle,float Seconds)
{
    if (!State.Items.IsValidIndex(Index) || State.Items[Index].Bed) return;
    if (State.Items.IsValidIndex(State.MovingDoor)) SetDoors102(State.Items[State.MovingDoor],0);
    State.MovingDoor=Index;State.OpenedAt=World.GetWorld()->GetTimeSeconds();State.SwingTime=Seconds;State.SwingAngle=Angle;
    World.GetWorldTimerManager().SetTimer(State.Timer,FTimerDelegate::CreateWeakLambda(&World,[&World]()
    {
        auto* Current=Hideouts102.Find(&World);
        if (!Current || !Current->Items.IsValidIndex(Current->MovingDoor)) return;
        const float Age = (World.GetWorld()->GetTimeSeconds()-Current->OpenedAt)/Current->SwingTime;
        const float Open = FMath::SmoothStep(0.f,.45f,Age)*(1.f-FMath::SmoothStep(.52f,1.f,Age));
        SetDoors102(Current->Items[Current->MovingDoor],Current->SwingAngle*Open);
        if (Age>=1)
        {
            Current->MovingDoor=INDEX_NONE;
            World.GetWorldTimerManager().ClearTimer(Current->Timer);
        }
    }),1.f/45.f,true);
}

bool EligibleInspector102(const AEWardHospitalMob& Mob)
{
    return Mob.Active && !Mob.Dead && !Mob.IsHidden() && Mob.Stunned<=0 &&
        Mob.State59!=TEXT("windup") && Mob.State59!=TEXT("recovery") && !Boolean(Mob.Rule59,TEXT("stationary"));
}

void StartInspection102(AEWardHospitalMob& Mob,FHideoutState102& State,const FHideout102& Hideout)
{
    if (State.Investigators.Contains(&Mob)) return;
    State.Investigators.Add(&Mob);
    Mob.SearchStep59=WardHideout102::Inspect;
    Mob.Target=Hideout.Mouth();Mob.Target.Z=Mob.GetActorLocation().Z;
    Mob.SearchOrigin59=Mob.Target;
    Mob.PathClock=0;Mob.Memory=8;
    Mob.ChangeState59(TEXT("hideout_search"));
}

bool Leave102(AEWardBookWorld& World,FHideoutState102& State,bool Fast,bool Found=false)
{
    auto* Game=WardCampaign(&World);auto* Hero=Game?Game->Hero():nullptr;
    if (!Hero) return false;
    const auto* Hideout=State.Items.IsValidIndex(State.Active)?&State.Items[State.Active]:nullptr;
    FVector Exit=World.FurnitureExit;
    const auto* Capsule=Hero->GetCapsuleComponent();
    const float Radius=Capsule->GetScaledCapsuleRadius(),Half=Capsule->GetScaledCapsuleHalfHeight();
    if (Hideout && Hideout->Bed) Exit.Z=Hideout->Origin.Z+Half+3;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FurnitureExit102),false,Hero);
    FHitResult Hit;
    if (World.GetWorld()->SweepSingleByChannel(Hit,Hero->GetActorLocation(),Exit,FQuat::Identity,ECC_Pawn,
        FCollisionShape::MakeCapsule(Radius,Half),Query))
    {
        if (!Found) Game->Notify(TEXT("Выход из укрытия занят."));
        return false;
    }
    const int32 Index=State.Active;
    State.Active=INDEX_NONE;State.Investigators.Reset();World.FurnitureHiding=false;
    Hero->SetActorLocation(Exit,false);Hero->GetCharacterMovement()->SetCrouchedHalfHeight(58);
    if (!World.FurnitureExitCrouched){Hero->UnCrouch();Hero->GetCharacterMovement()->UnCrouch(false);}
    if (Hideout && !Hideout->Bed)
    {
        Swing102(World,State,Index,Fast?78:47,Fast?.35f:.75f);
        Game->Cue(TEXT("creak"),Hideout->Mouth(),Fast?.42f:.09f);
        if (Fast) Game->BroadcastNoise(Hideout->Mouth(),940);
    }
    if (!Found) Game->Notify(TEXT("Ты вышел из укрытия"));
    return true;
}
}

void WardHideout102::Add(AEWardBookWorld& World,FVector Origin,FVector Size,bool Bed,UMaterialInterface* Wood,UMaterialInterface* Steel)
{
    for (auto It=Hideouts102.CreateIterator();It;++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    auto& Hideout=Hideouts102.FindOrAdd(&World).Items.Add_GetRef({Origin,Size,Bed,{}});
    if (Bed) return;
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/BookWorld/SM_BookCube.SM_BookCube"));
    const double Width=(Size.X-30-7)*.5;
    for (int32 I=0;I<2;++I)
    {
        const float Side=I==0?1.f:-1.f;
        auto* Pivot=NewObject<USceneComponent>(&World);Pivot->SetupAttachment(World.GetRootComponent());
        Pivot->SetMobility(EComponentMobility::Movable);
        Pivot->SetRelativeLocation(Origin+FVector(I==0?15:Size.X-15,Size.Y-4,0));
        Pivot->RegisterComponent();World.AddInstanceComponent(Pivot);Hideout.Doors[I]=Pivot;
        for (const bool Handle : {false,true})
        {
            auto* Panel=NewObject<UStaticMeshComponent>(&World);Panel->SetupAttachment(Pivot);Panel->SetMobility(EComponentMobility::Movable);
            Panel->SetStaticMesh(Cube);Panel->SetMaterial(0,Handle?Steel:Wood);
            Panel->SetRelativeLocation(Handle?FVector(Side*(Width-5),5,109):FVector(Side*Width/2,0,(Size.Z-18)*.5+4));
            Panel->SetRelativeScale3D((Handle?FVector(3.8,6,18):FVector(Width,4,Size.Z-18))/100);
            Panel->SetCollisionEnabled(Handle?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);
            Panel->SetCollisionResponseToAllChannels(ECR_Ignore);
            Panel->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
            Panel->SetGenerateOverlapEvents(false);
            Panel->SetLightingChannels(true,true,false);Panel->SetAffectDistanceFieldLighting(false);Panel->SetAffectDynamicIndirectLighting(false);
            Panel->RegisterComponent();World.AddInstanceComponent(Panel);
        }
    }
}

void WardHideout102::Reset(AEWardBookWorld& World)
{
    if (auto* State=Hideouts102.Find(&World))
    {
        if (State->Items.IsValidIndex(State->MovingDoor)) SetDoors102(State->Items[State->MovingDoor],0);
        World.GetWorldTimerManager().ClearTimer(State->Timer);
        State->MovingDoor=State->Active=INDEX_NONE;State->Investigators.Reset();
    }
    World.FurnitureHiding=false;
}

bool WardHideout102::View(const AEWardCampaignHero& Hero,FVector& WorldEye,FRotator& Centre,float& YawLimit)
{
    const auto* Game=WardCampaign(&Hero);const auto* World=Game?Game->BookWorld.Get():nullptr;
    const auto* State=World&&World->FurnitureHiding?Hideouts102.Find(World):nullptr;
    if (!State || !State->Items.IsValidIndex(State->Active)) return false;
    const auto& Hideout=State->Items[State->Active];
    WorldEye=Hideout.Origin+FVector(Hideout.Size.X*.5,Hideout.Bed?Hideout.Size.Y*.66:Hideout.Size.Y-25,
        Hideout.Bed?32:FMath::Clamp(Hideout.Size.Z-37.,100.,154.));
    Centre=FRotator(Hideout.Bed?-2.f:0.f,90,0);YawLimit=Hideout.Bed?61:14;
    return true;
}

bool WardHideout102::TryExit(AEWardBookWorld& World,bool Fast)
{
    auto* State=World.FurnitureHiding?Hideouts102.Find(&World):nullptr;
    return State&&Leave102(World,*State,Fast);
}

bool WardHideout102::Heard(AEWardHospitalMob& Mob,FVector Source)
{
    if (!EligibleInspector102(Mob)) return false;
    auto& World=*Mob.Runtime->Mode->BookWorld;
    auto* State=World.FurnitureHiding?Hideouts102.Find(&World):nullptr;
    if (!State || !State->Items.IsValidIndex(State->Active)) return false;
    const auto& Hideout=State->Items[State->Active];const auto At=Hideout.Mouth();
    if (FMath::Abs(Source.Z-At.Z)>145 || FVector::DistSquared2D(Source,At)>FMath::Square(Hideout.Bed?120.:95.)) return false;
    StartInspection102(Mob,*State,Hideout);
    return true;
}

bool WardHideout102::Inspecting(const AEWardHospitalMob& Mob)
{
    return Mob.SearchStep59==Inspect && (Mob.State59==TEXT("hideout_search") || Mob.State59==TEXT("hideout_check"));
}

bool WardHideout102::AdvanceInspection(AEWardHospitalMob& Mob,float Delta)
{
    auto& World=*Mob.Runtime->Mode->BookWorld;
    auto* State=World.FurnitureHiding?Hideouts102.Find(&World):nullptr;
    const auto Stop = [&]() { Mob.SearchStep59=0;Mob.Target=Mob.Home;Mob.PathClock=0;Mob.ChangeState59(TEXT("return")); };
    if (!State || !State->Items.IsValidIndex(State->Active)) {Stop();return false;}
    const auto& Hideout=State->Items[State->Active];const auto Mouth=Hideout.Mouth();const auto At=Mob.GetActorLocation();
    const bool Near=FVector::DistSquared2D(At,Mouth)<FMath::Square(103.)&&FMath::Abs(At.Z-Mouth.Z)<130;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HideoutLook102),false,&Mob);
    const bool CanReach=Near&&!Mob.GetWorld()->LineTraceSingleByChannel(Hit,At+FVector(0,0,17),Mouth,ECC_Visibility,Query);
    if (!CanReach)
    {
        if(Mob.StateClock59>=8.f){Stop();return false;}
        return false;
    }
    Mob.GetCharacterMovement()->StopMovementImmediately();Mob.ConsumeMovementInputVector();
    const FVector Facing=Hideout.Origin+FVector(Hideout.Size.X*.5,Hideout.Size.Y,0)-At;
    Mob.SetActorRotation(FMath::RInterpConstantTo(Mob.GetActorRotation(),Facing.GetSafeNormal2D().Rotation(),Delta,170));
    if (Mob.State59!=TEXT("hideout_check")) {Mob.ChangeState59(TEXT("hideout_check"));return true;}
    if (Mob.StateClock59<(Hideout.Bed?1.30f:1.05f)) return true;
    auto& Game=*Mob.Runtime->Mode;
    Swing102(World,*State,State->Active,75,.54f);
    Game.Cue(Hideout.Bed?TEXT("scrape"):TEXT("creak"),Mouth,.43f);
    Game.Notify(TEXT("Монстр нашёл укрытие. Выбирайся!"));
    Game.DamageHero(Number(Mob.Definition,TEXT("damage"),12),&Mob);
    if (Game.Hero()&&!Game.Hero()->Dead) Leave102(World,*State,false,true);
    Stop();
    return true;
}

void AEWardBookWorld::RefreshPortalVisual(FPortal& P)
{
    if(!P.Leaf)return;
    // The closed collision box is independent of the visible split panels.
    // Open doors remain visible beside the frame, with no blocking swing box.
    P.Leaf->SetVisibility(false);P.Leaf->SetHiddenInGame(true);
    const float Open = WardContact98::DoorOpenAmount(*this, P.At, P.Open);
    const FName Collision = (P.Open ? Open > .54f : Open > .12f) ? TEXT("NoCollision") : TEXT("BlockAll");
    if (P.Leaf->GetCollisionProfileName() != Collision) P.Leaf->SetCollisionProfileName(Collision);
    const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");
    const FVector Along=NS?FVector(1,0,0):FVector(0,1,0);
    const bool Double=P.Width>160;const float W=(P.Width-12)/(Double?2:1);
    const float Inward=P.Side==TEXT("north")||P.Side==TEXT("east")?P.OpenSign37:-P.OpenSign37;
    const float HandleAngle = WardDoor100::LeverAngle(*this, P.At);
    for(const auto& Part:P.VisibleParts)
    {
        const bool Right=Part.Leaf==1;
        if(P.Type==TEXT("shutter"))
        {
            const FQuat R=FRotator(0,(NS?0:90)+(Right?180:0),0).Quaternion();const FVector Hinge=P.At+Along*((Right?1:-1)*(P.Width*.5-6));
            FVector Centre=Hinge+R.RotateVector(Part.Centre),Scale=Part.Size/100;
            // The shutter rolls into its header cassette instead of protruding through the floor above.
            if(P.Open){Centre.Z=P.At.Z+260+(Part.Centre.Z-4)*.07f;Scale.Z*=.07f;}
            DoorVisualGroups[Part.Material]->UpdateInstanceTransform(Part.Index,FTransform(R,Centre,Scale),false,false,true);continue;
        }
        const float Yaw=(NS?0:90)+(Right?180:0)+Open*Inward*(Right?-90:90);
        const FQuat R=FRotator(0,Yaw,0).Quaternion();const FVector Hinge=P.At+Along*((Right?1:-1)*(P.Width*.5-6));
        FVector Centre = Part.Centre;
        FQuat PartRotation = R;
        if (Part.Material == WardDoor100::LeverMaterial)
        {
            const FVector Pivot(W-13,Centre.Y,115);
            const FQuat Press(FVector::RightVector,FMath::DegreesToRadians(-HandleAngle));
            Centre = Pivot + Press.RotateVector(Centre-Pivot);
            PartRotation = R*Press;
        }
        const FTransform T(PartRotation,Hinge+R.RotateVector(Centre),Part.Size/100);
        DoorVisualGroups[Part.Material]->UpdateInstanceTransform(Part.Index,T,false,false,true);
    }
    for(const auto& Part:P.VisibleParts)DoorVisualGroups[Part.Material]->MarkRenderStateDirty();
}

void AEWardBookWorld::BuildStoryArchitecture()
{
    BuildMovables();
    const FVector Monitor=WardSecurity103::Terminal(*this);
    if(!Monitor.IsNearlyZero())
    {
        Box(Monitor+FVector(6,0,0),FVector(11,76,49),11,false);
        Box(Monitor+FVector(-1,0,2),FVector(2,67,38),13,false);
        Box(Monitor+FVector(9,0,-31),FVector(25,63,7),11,false);
        Box(Monitor+FVector(-1,28,-16),FVector(1.5,2,2),4,false);
        Sign(TEXT("ВИДЕОНАБЛЮДЕНИЕ  ·  F"),Monitor+FVector(-2,0,38),FRotator(0,180,0),7);
    }
    auto Fitted=[&](int Id,FVector At,FVector Min,FVector Max)
    {
        Json M=MakeShared<FJsonObject>();M->SetArrayField(TEXT("bounds"),{MakeShared<FJsonValueNumber>(Min.X/100),MakeShared<FJsonValueNumber>(Min.Y/100),MakeShared<FJsonValueNumber>(Min.Z/100),MakeShared<FJsonValueNumber>(Max.X/100),MakeShared<FJsonValueNumber>(Max.Y/100),MakeShared<FJsonValueNumber>(Max.Z/100)});
        return AddCustomProp(Id,M,At,0);
    };
    for(auto V:Array(WardBookMapData(),TEXT("structural_members")))
    {Json S=V->AsObject();Box(Vector(Array(S,TEXT("at")))*100,Vector(Array(S,TEXT("size")))*100,11);}
    for(auto V:Array(WardBookMapData(),TEXT("hideouts")))
    {
        Json N=V->AsObject();const FVector O=Vector(Array(N,TEXT("at")))*100,S=Vector(Array(N,TEXT("size")))*100;
        const FString Kind=String(N,TEXT("kind"));
        if(Kind==TEXT("wardrobe")||Kind==TEXT("under_bed")) WardHideout102::Add(*this,O,S,Kind==TEXT("under_bed"),Palette[10],Palette[11]);
        if(Kind==TEXT("under_bed"))
        {
            const bool Frame=Fitted(175,O,FVector::ZeroVector,FVector(S.X,S.Y,172.5));
            const bool Mattress=Fitted(176,O,FVector(4,4,123),FVector(S.X-4,S.Y-4,143));
            Box(O+FVector(S.X*.5,S.Y*.5,114),FVector(S.X,S.Y,18),11,true,!Frame);
            Box(O+FVector(S.X*.5,S.Y*.5,133),FVector(S.X-8,S.Y-8,20),2,true,!Mattress);
            for(float X:{7.5f,float(S.X-7.5)})for(float Y:{7.5f,float(S.Y-7.5)})Box(O+FVector(X,Y,52.5),FVector(15,15,105),11,true,!Frame);
            Box(O+FVector(S.X*.5,7.5,150),FVector(S.X,8,45),11,true,!Frame);
            Sign(TEXT("BED / CROUCH"),O+FVector(S.X*.5,S.Y+2,125),FRotator(0,90,0),8);continue;
        }
        Box(O+FVector(S.X*.5,7.5,S.Z*.5),FVector(S.X,15,S.Z),11);
        for(float X:{7.5f,float(S.X-7.5)})Box(O+FVector(X,S.Y*.5,S.Z*.5),FVector(15,S.Y,S.Z),11);
        Box(O+FVector(S.X*.5,S.Y*.5,S.Z+5),FVector(S.X,S.Y,10),11);
        if(Kind==TEXT("wardrobe"))
        {
            Box(O+FVector(S.X*.5,S.Y-4,S.Z-10),FVector(S.X-30,6,10),10);
            for(float X:{float(S.X*.5-22),float(S.X*.5+22)})Box(O+FVector(X,S.Y-9,S.Z-36),FVector(5,5,12),11,false);
        }
        Sign(Kind==TEXT("wardrobe")?TEXT("WARDROBE / F"):TEXT("0.90 / HIDE"),O+FVector(S.X*.5,S.Y+2,S.Z-12),FRotator(0,90,0),7);
    }
    for(auto V:Array(WardBookMapData(),TEXT("furnishings")))
    {
        Json N=V->AsObject();const FVector O=Vector(Array(N,TEXT("at")))*100,S=Vector(Array(N,TEXT("size")))*100;
        const bool Detailed=Fitted(244,O,FVector(-S.X*.5,-S.Y*.5,0),FVector(S.X*.5,S.Y*.5,S.Z));
        Box(O+FVector(0,0,S.Z*.5),S,11,true,!Detailed);
        if(!Detailed)
        {
            for(float Z:{float(S.Z*.32),float(S.Z*.65)}){Box(O+FVector(0,S.Y*.5+1,Z),FVector(S.X-5,3,3),5,false);Box(O+FVector(0,S.Y*.5+4,Z+13),FVector(13,4,4),5,false);}
            Box(O+FVector(0,0,S.Z+3),FVector(S.X+4,S.Y+4,6),2,false);
        }
    }
    for(auto V:Array(WardBookMapData(),TEXT("escape_niches")))
    {
        Json N=V->AsObject();const FVector P=Vector(Array(N,TEXT("entry")))*100;
        const FString Side=String(N,TEXT("side"));const bool NS=Side==TEXT("north")||Side==TEXT("south");
        const FVector Along=NS?FVector(1,0,0):FVector(0,1,0);
        for(int SignValue:{-1,1})Box(P+Along*(SignValue*72.5)+FVector(0,0,130),NS?FVector(55,22,260):FVector(22,55,260),11);
        Sign(TEXT("СЛУЖЕБНАЯ НИША"),P+FVector(0,0,245),FRotator(0,NS?90:0,0),9);
    }
    int32 RemovedWallStubs=0;
    for(auto V:Array(WardBookMapData(),TEXT("partitions")))
    {
        Json P=V->AsObject();const FVector A=Vector(Array(P,TEXT("a")))*100,B=Vector(Array(P,TEXT("b")))*100;
        const float H=Number(P,TEXT("height"),3.2)*100,T=Number(P,TEXT("thickness"),.12)*100;
        const FString Type=String(P,TEXT("type"));const int Mat=Type==TEXT("breakable")?1:SiteLower.FindRef(String(P,TEXT("site")));
        const FVector Base=(A+B)*.5,Width(FMath::Abs(A.X-B.X)+T,FMath::Abs(A.Y-B.Y)+T,0);
        // Short residual strips between corridor openings obscure sight lines.
        // Keep the authored partitions and the approaches to interactive doors and hiding niches.
        if(Type==TEXT("niche")&&FVector::DistSquared(A,B)<=FMath::Square(55.01))
        {
            const bool BesidePortal=Portals.ContainsByPredicate([&](const FPortal& Portal)
            {
                return FVector::DistSquared(Portal.At,Base)<=FMath::Square(150.0);
            });
            if(!BesidePortal)
            {
                ++RemovedWallStubs;
                continue;
            }
        }
        bool Damaged=false;
        static Json Damage37=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/hospital_environment_v37.json"));
        for(auto V37:Array(Damage37,TEXT("wall_damage")))
        {
            const Json Hole=V37->AsObject();if(!Vector(Array(Hole,TEXT("a"))).Equals(A/100,.001)||!Vector(Array(Hole,TEXT("b"))).Equals(B/100,.001))continue;
            const bool X=Width.X>Width.Y;const float Length=X?Width.X:Width.Y,Opening=Number(Hole,TEXT("width")) *100,Low=Number(Hole,TEXT("bottom"))*100,High=Number(Hole,TEXT("top"))*100;
            const int Upper=SiteWall.FindRef(String(P,TEXT("site")));const FVector Along=X?FVector(1,0,0):FVector(0,1,0);
            auto Piece=[&](float Centre,float W,float Bottom,float Top)
            {
                if(W<=0||Top<=Bottom)return;const FVector C=Base+Along*Centre+FVector(0,0,(Bottom+Top)*.5);const FVector S=X?FVector(W,T,Top-Bottom):FVector(T,W,Top-Bottom);Box(C,S,Bottom<110?Mat:Upper,true);
            };
            const float Side=(Length-Opening)*.5;
            Piece(-(Opening+Side)*.5,Side,0,H);Piece((Opening+Side)*.5,Side,0,H);Piece(0,Opening,0,Low);Piece(0,Opening,High,H);
            // Irregular plaster teeth give the cut actual thickness and a broken silhouette.
            for(int I=0;I<7;++I){const float C=-Opening*.5+(I+.5)*Opening/7,Depth=3+(I*11%9);Piece(C,Opening/7,Low,Low+Depth);Piece(C,Opening/7,High-Depth,High);}
            Damaged=true;break;
        }
        if(Damaged)continue;
        // Build one collision body with separate plaster, painted dado and protective rail.
        Box(Base+FVector(0,0,H*.5),FVector(Width.X,Width.Y,H),Mat,true,false);
        const float Dado=FMath::Min(108.f,H);const int Upper=Type==TEXT("breakable")?1:SiteWall.FindRef(String(P,TEXT("site")));
        Box(Base+FVector(0,0,Dado*.5),FVector(Width.X,Width.Y,Dado),Mat,false);
        if(H>Dado)
        {
            Box(Base+FVector(0,0,(Dado+H)*.5),FVector(Width.X,Width.Y,H-Dado),Upper,false);
            Box(Base+FVector(0,0,Dado),FVector(Width.X+4,Width.Y+4,6),5,false);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("WARD89 short corridor wall stubs removed=%d"),RemovedWallStubs);
    // A separate real door mesh is used for each portal. Chapter zero is the
    // inspection shell: all doors open, including the future class-C rooms.
    for(auto& P:Portals)
    {
        const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");
        auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/BookWorld/SM_BookCube.SM_BookCube")));
        C->SetRelativeScale3D((NS?FVector(P.Width-12,9,252):FVector(9,P.Width-12,252))/100);
        C->SetMaterial(0,Palette[P.Type==TEXT("biometric")?11:P.Type==TEXT("bolt")?10:5]);
        C->SetRelativeLocation(P.At+FVector(0,0,130));C->SetCollisionProfileName(TEXT("NoCollision"));C->SetVisibility(false);
        C->SetAffectDynamicIndirectLighting(false);C->SetAffectDistanceFieldLighting(false);C->SetLightingChannels(true,true,false);
        C->RegisterComponent();AddInstanceComponent(C);P.Leaf=C;P.Open=true;
        if(P.Role!=TEXT("niche"))
        {
            const bool Double=P.Width>160;const float W=(P.Width-12)/(Double?2:1);
            int Surface=P.Type==TEXT("biometric")?11:10;
            for(const auto& Room:Rooms)if(Room.Id==P.Zone){Surface=SiteLower.FindRef(Room.Location);break;}
            auto Part=[&](int Leaf,FVector At,FVector Size,int Mat)
            {
                UInstancedStaticMeshComponent* Group=DoorVisualGroups.FindRef(Mat);
                if(!Group)
                {
                    Group=NewObject<UInstancedStaticMeshComponent>(this);Group->SetupAttachment(RootComponent);Group->SetMobility(EComponentMobility::Movable);
                    if(Mat>=100000)Group->SetStaticMesh(WardEchoAssets::Mesh(Mat-100000,true));
                    else {Group->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Mat==13?TEXT("/Engine/BasicShapes/Cube.Cube"):TEXT("/Game/Campaign/BookWorld/SM_BookCube.SM_BookCube")));Group->SetMaterial(0,Palette[Mat==WardDoor100::LeverMaterial?11:Mat]);}
                    Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);Group->SetGenerateOverlapEvents(false);
                    Group->SetAffectDynamicIndirectLighting(false);Group->SetAffectDistanceFieldLighting(false);Group->SetLightingChannels(true,true,false);
                    Group->RegisterComponent();AddInstanceComponent(Group);DoorVisualGroups.Add(Mat,Group);
                }
                FDoorPart A;A.Material=Mat;A.Index=Group->AddInstance(FTransform::Identity);A.Leaf=Leaf;A.Centre=At;A.Size=Size;P.VisibleParts.Add(A);
            };
            for(int L=0;L<(Double?2:1);++L)
            {
                ++VisibleDoorLeaves;const float Glass=FMath::Min(36.f,W*.45f),Side=(W-Glass)*.5f;
                if (WardDoor100::HasLever(P)) for (const int Face : {-1,1})
                {
                    Part(L,FVector(W-13,Face*6,115),FVector(6,4,10),11);
                    Part(L,FVector(W-13,Face*9,115),FVector(4,5,4),11);
                    Part(L,FVector(W-19,Face*11,115),FVector(15,3.4,3.1),WardDoor100::LeverMaterial);
                }
                const int Id=P.Type==TEXT("biometric")?69:P.Type==TEXT("grille")?72:P.Type==TEXT("bolt")?71:TArray<int>{61,63,67,68}[GetTypeHash(P.Zone)%4];
                if(auto* Mesh=WardEchoAssets::Mesh(Id,true))
                {
                    const FBox B=Mesh->GetBoundingBox();const FVector Scale=FVector(W,9,252)/B.GetSize();
                    Part(L,FVector(-B.Min.X*Scale.X,-B.GetCenter().Y*Scale.Y,4-B.Min.Z*Scale.Z),Scale*100,100000+Id);++CustomDoorLeaves;CustomModelIds.Add(Id);continue;
                }
                Part(L,FVector(W*.5,0,79),FVector(W,8,150),Surface);
                Part(L,FVector(W*.5,0,232.5),FVector(W,8,47),Surface);
                for(float X:{Side*.5f,W-Side*.5f})Part(L,FVector(X,0,181.5),FVector(Side,8,55),Surface);
                Part(L,FVector(W*.5,0,181.5),FVector(Glass,1,55),13);
                for(int S:{-1,1}){Part(L,FVector(W-13,S*4.8,115),FVector(13,1.4,29),11);Part(L,FVector(W*.5,S*4.3,23),FVector(W-6,.8,28),11);}
            }
        }
        RefreshPortalVisual(P);
        const FVector Along=NS?FVector(1,0,0):FVector(0,1,0);
        for(int S:{-1,1})Box(P.At+Along*(S*P.Width*.5)+FVector(0,0,130),NS?FVector(12,22,260):FVector(22,12,260),11,false);
        Box(P.At+FVector(0,0,259),NS?FVector(P.Width+12,22,10):FVector(22,P.Width+12,10),11,false);
        if(P.Type==TEXT("biometric"))Box(P.At+Along*(P.Width*.5+22)+FVector(0,0,135),NS?FVector(16,8,24):FVector(8,16,24),4,false);
        if(P.Type==TEXT("bolt"))Box(P.At+Along*(P.Width*.5+16)+FVector(0,0,116),NS?FVector(24,12,6):FVector(12,24,6),12,false);
    }
    // Visible endpoints correspond to the manifest's camera/telephone segments.
    for(auto V:Array(WardBookMapData(),TEXT("network_devices")))
    {
        Json D=V->AsObject();for(const auto& R:Rooms)if(R.Id==String(D,TEXT("zone")))
        {
            const bool South=R.Inside.Y<R.Outside.Y;const FVector Base=R.Door+FVector(-190,South?-26:26,-98);
            const FString Type=String(D,TEXT("type"));
            if(Type==TEXT("phone")){Box(Base+FVector(0,0,140),FVector(24,12,36),5,false);Box(Base+FVector(0,South?-9:9,142),FVector(8,10,28),11,false);}
            if(Type==TEXT("camera")){Box(Base+FVector(40,0,295),FVector(28,40,20),11,false);Box(Base+FVector(40,South?-22:22,295),FVector(16,6,12),6,false);}
            if(Type==TEXT("power_switch")){Box(Base+FVector(85,0,140),FVector(25,12,42),11,false);Box(Base+FVector(85,South?-8:8,140),FVector(7,6,13),12,false);}
            break;
        }
    }
    for(auto V:Array(WardBookMapData(),TEXT("threat_nodes")))
    {
        Json N=V->AsObject();for(const auto& R:Rooms)if(R.Id==String(N,TEXT("zone")))
        {
            const bool South=R.Inside.Y<R.Outside.Y;const FVector P=R.Door+FVector(240,South?-40:40,35);
            Box(P,FVector(36,30,80),11);Box(P+FVector(0,0,47),FVector(20,20,12),12,false);
            Sign(String(N,TEXT("id")),P+FVector(0,South?-18:18,0),FRotator(0,South?-90:90,0),9);break;
        }
    }
    // South perimeter is explicit: an entrance aperture, not an absent side.
    for(auto V:Array(WardBookMapData(),TEXT("locations")))
    {
        Json L=V->AsObject();const FString Refuge=String(L,TEXT("refuge")),Landmark=String(Object(L,TEXT("identity")),TEXT("landmark"));
        for(const auto& R:Rooms)if(R.Id==Refuge){const bool South=R.Inside.Y<R.Outside.Y;Box(R.Door+FVector(0,South?15:-15,228),FVector(135,10,30),2,false);Sign(TEXT("REST / ")+R.Id+TEXT(" / K BOLT"),R.Door+FVector(0,South?23:-23,228),FRotator(0,South?90:-90,0),10);}
        for(const auto& R:Rooms)if(R.Id==Landmark){const bool South=R.Inside.Y<R.Outside.Y;Sign(Landmark+TEXT(" / ")+String(Object(L,TEXT("identity")),TEXT("material")),R.Door+FVector(0,South?25:-25,238),FRotator(0,South?90:-90,0),15);}
    }
    Box(FVector(5000,22400,-479),FVector(140,140,2),11,false);
    Sign(TEXT("SERVICE HATCH"),FVector(5000,22400,-476),FRotator(90,0,0),13);
    // Covered ambulance drop-off directly beside the reception vestibule.
    Box(FVector(7400,-1400,430),FVector(2800,2400,24),11);
    for(float X:{6100.f,8700.f})Box(FVector(X,-2500,210),FVector(24,24,420),11);
    Box(FVector(-7400,-19000,110),FVector(21200,16,220),5);
    Box(FVector(17100,-19000,110),FVector(21800,16,220),5);
    Sign(TEXT("333 / WARD 04"),FVector(4700,-18980,285),FRotator(0,90,0),38);
    SetStoryChapter(0);
}

void AEWardBookWorld::UseArchitecture()
{
    if(!Mode.IsValid()||!Mode->Hero())return;
    if(FurnitureHiding){UseFurnitureHide(Mode->Hero()->GetActorLocation());return;}
    if(WardSecurity103::Interact(*Mode.Get()))return;
    const int Aimed76=WardGameplay76::Door(*this);
    if(Aimed76==INDEX_NONE&&Mode->HospitalStory&&Mode->HospitalStory->Interact())return;
    const FVector Player=Mode->Hero()->GetActorLocation();
    if(Aimed76==INDEX_NONE&&UseFurnitureHide(Player))return;
    if(Aimed76==INDEX_NONE&&UseMovable(Player))return;
    if(InsideLift(Player)||(FMath::Abs(Player.X-5000)<250&&Player.Y>=1000&&Player.Y<1350)){UseLift();return;}
    if (Portals.IsValidIndex(Aimed76)) WardOnline80::OperateDoor(*this, Aimed76, Player, !Portals[Aimed76].Open);
}

bool WardOnline80::OperateDoor(AEWardBookWorld& World, int32 Index, FVector Player, bool Open)
{
    auto* Game = WardCampaign(&World);
    if (!Game || !World.Portals.IsValidIndex(Index)) return false;
    auto& Door = World.Portals[Index];
    if (Door.Open == Open) return true;
    const auto Deny = [Game](const TCHAR* Message) { Game->Notify(Message); return false; };
    if (!Open && Game->HospitalStory && Game->HospitalStory->ArrivalStage == 3 &&
        PlayingMap(*Game) && World.Portals.ContainsByPredicate([&](const auto& Portal) {
            return Portal.At.Equals(Door.At, 1) && Portal.Zone == TEXT("N02") && Portal.Side == TEXT("north");
        }) && PlayersOutsideEntrance(*Game) > 0)
        return Deny(TEXT("Вход останется открыт, пока вся команда не соберётся в приёмной."));
    if (Door.Role == TEXT("niche")) return Deny(TEXT("Укрытие: проход 90 см. Крупные противники здесь не проходят."));
    for (const auto& Portal : World.Portals)
        if (Portal.At.Equals(Door.At, 1) && World.RefugeBolts.Contains(Portal.Zone))
            return Deny(TEXT("Засов закрыт. Изнутри убежища: K — снять засов."));
    const bool Inside = World.Rooms.ContainsByPredicate([&](const auto& Room) {
        return Room.Id == Door.Zone && Room.Bounds.IsInside(Player);
    });
    if ((Door.Locked || Door.InsideOnly) && !Inside)
        return Deny(TEXT("Заперто. Нужен другой путь или открытие изнутри."));
    if (World.StoryChapter > 0 && (Door.Type == TEXT("biometric") || Door.Type == TEXT("shutter")))
        return Deny(TEXT("Доступ закрыт системой. Продолжи текущую задачу или найди другой путь."));
    const bool NS = Door.Side == TEXT("north") || Door.Side == TEXT("south");
    if (!Open && (FMath::Abs(NS ? Player.Y - Door.At.Y : Player.X - Door.At.X) < 65 ||
        DoorwayOccupied(*Game, Door.At, NS, Door.Width)))
        return Deny(TEXT("Отойди от дверного проёма"));
    const bool Local = Game->Hero() && Game->Hero()->GetActorLocation().Equals(Player, 1.f);
    WardContact98::BeginDoor(World, Index, Player, Local, Open);
    for (auto& Portal : World.Portals) if (Portal.At.Equals(Door.At, 1))
    {
        Portal.Open = Open;
        World.RefreshPortalVisual(Portal);
    }
    const FString DoorKey = Door.Type == TEXT("bolt") ? TEXT("DOOR/lock") : Door.Type == TEXT("grate") ? TEXT("DOOR/grate") :
        Door.Type == TEXT("shutter") ? TEXT("DOOR/shutter") : Door.Type == TEXT("biometric") ? TEXT("DOOR/biometric") :
        Door.Type == TEXT("manual") ? TEXT("DOOR/steel") : TEXT("DOOR/medical");
    Game->Cue(DoorKey + (Open || DoorKey == TEXT("DOOR/lock") ? TEXT("") : TEXT("_close")), Door.At, .22f);
    Game->Notify(Open ? TEXT("Дверь открыта") : TEXT("Дверь закрыта · F — открыть"));
    return true;
}

bool AEWardBookWorld::UseFurnitureHide(FVector Player)
{
    if(!Mode.IsValid()||!Mode->Hero())return false;
    auto* H=Mode->Hero();auto* State=Hideouts102.Find(this);
    if(!State)return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FurnitureHide),false,H);FHitResult Hit;
    auto* PC=Cast<APlayerController>(H->GetController());
    if(FurnitureHiding)
    {
        WardHideout102::TryExit(*this,PC&&(PC->IsInputKeyDown(EKeys::LeftShift)||PC->IsInputKeyDown(EKeys::RightShift)));
        return true;
    }
    for(int32 Index=0;Index<State->Items.Num();++Index)
    {
        const auto& Hideout=State->Items[Index];
        const FVector O=Hideout.Origin,S=Hideout.Size,Entry=Hideout.Mouth();
        const bool Bed=Hideout.Bed;
        if(FMath::Abs(Player.Z-Entry.Z)>70||FVector::DistSquared2D(Player,Entry)>170*170)continue;
        const auto* Capsule=H->GetCapsuleComponent();
        const float Half=Bed?48.f:Capsule->GetScaledCapsuleHalfHeight();
        const FVector A=Bed?FVector(Player.X,Player.Y,O.Z+51):Player;
        const FVector B=O+FVector(S.X*.5,Bed?S.Y*.55:FMath::Min(60.,S.Y*.5),Bed?51:FMath::Clamp(Player.Z-O.Z,double(Half+2),double(Half+14)));
        if(GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Half),Q))continue;
        State->Active=Index;State->Investigators.Reset();
        if (Mode->HospitalStory) for (auto Mob : Mode->HospitalStory->Mobs)
            if (IsValid(Mob) && EligibleInspector102(*Mob) && Mob->Sight59(Player)) StartInspection102(*Mob,*State,Hideout);
        FurnitureExit=Player;FurnitureExitCrouched=H->bIsCrouched;H->GetCharacterMovement()->StopMovementImmediately();
        if(Bed){H->GetCharacterMovement()->SetCrouchedHalfHeight(48);H->Crouch();H->GetCharacterMovement()->Crouch(false);}
        H->SetActorLocation(B,false);FurnitureHiding=true;
        if (PC) PC->SetControlRotation(FRotator(Bed?-2.f:0.f,90,0));
        Swing102(*this,*State,Index,47,.65f);
        Mode->Notify(Bed?TEXT("Под койкой · F — выбраться"):TEXT("В шкафу · F — тихий выход · Shift + F — резко открыть"));return true;
    }
    return false;
}

void AEWardBookWorld::SetStoryChapter(int Chapter)
{
    StoryChapter=FMath::Clamp(Chapter,0,19);
    TMap<FString,FString> ZoneStates;TSet<FString> ClassCZones;
    for(auto L:Array(WardBookMapData(),TEXT("locations")))for(auto V:Array(L->AsObject(),TEXT("rooms")))
    {
        Json R=V->AsObject();const FString Id=String(R,TEXT("id"));FString State=TEXT("open");
        if(String(R,TEXT("production_class"))==TEXT("C"))ClassCZones.Add(Id);
        for(auto S:Array(R,TEXT("states")))if(Number(S->AsObject(),TEXT("chapter"))<=StoryChapter)State=String(S->AsObject(),TEXT("state"));
        ZoneStates.Add(Id,State);
    }
    for(auto& P:Portals)
    {
        const FString* ZoneState=ZoneStates.Find(P.Zone);const FString State=ZoneState?*ZoneState:TEXT("open");const bool ClassC=ClassCZones.Contains(P.Zone);
        P.Locked=State==TEXT("sealed")||(State==TEXT("closed")&&ClassC);
        P.Open=State==TEXT("open")||State==TEXT("destroyed");
        if(P.Role==TEXT("niche"))P.Open=true;
        if(StoryChapter==0)P.Locked=false;
        if(RefugeBolts.Contains(P.Zone))P.Open=false;
        RefreshPortalVisual(P);
    }
    EnvironmentState=StoryChapter>=18?TEXT("collapsed"):StoryChapter>=15?TEXT("damaged"):StoryChapter>=6?TEXT("infected"):TEXT("clean");
    Json Profile=Object(Object(WardBookMapData(),TEXT("environment_profiles")),EnvironmentState);
    for(const FString Key:{FString(TEXT("wall")),FString(TEXT("lower"))})
    {
        const auto& A=Array(Profile,Key);if(A.Num()!=3)continue;
        const int Index=Key==TEXT("wall")?SiteWall.FindRef(TEXT("B2")):SiteLower.FindRef(TEXT("B2"));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Palette[Index])){const FLinearColor Tint(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber());M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetVectorParameterValue(TEXT("Emission"),Tint*(Bright?.22f:.014f));}
    }
}
Json AEWardBookWorld::ArchitectureSnapshot()const
{
    Json State=MakeShared<FJsonObject>(),Doors=MakeShared<FJsonObject>();State->SetNumberField(TEXT("chapter"),StoryChapter);
    for(const auto& P:Portals)Doors->SetBoolField(P.Id,P.Open);State->SetObjectField(TEXT("doors"),Doors);
    Json Locks=MakeShared<FJsonObject>(),Inside=MakeShared<FJsonObject>();
    for(const auto& P:Portals){Locks->SetBoolField(P.Id,P.Locked);Inside->SetBoolField(P.Id,P.InsideOnly);}
    State->SetObjectField(TEXT("locks"),Locks);State->SetObjectField(TEXT("inside_only"),Inside);
    State->SetNumberField(TEXT("lift_floor"),LiftFloor);State->SetNumberField(TEXT("lift_selection"),LiftSelection);
    TArray<TSharedPtr<FJsonValue>> Bolts;for(const auto& Z:RefugeBolts)Bolts.Add(MakeShared<FJsonValueString>(Z));State->SetArrayField(TEXT("bolts"),Bolts);
    Json Props=MakeShared<FJsonObject>();for(const auto& P:Movables){const FVector V=P.Body->GetRelativeLocation();TArray<TSharedPtr<FJsonValue>> A;for(double X:{V.X,V.Y,V.Z})A.Add(MakeShared<FJsonValueNumber>(X));Props->SetArrayField(P.Id,A);}State->SetObjectField(TEXT("props"),Props);return State;
}
void AEWardBookWorld::RestoreArchitecture(Json State)
{
    WardContact98::ClearMotion(*this);
    WardHideout102::Reset(*this);
    RefugeBolts.Reset();for(auto V:Array(State,TEXT("bolts")))RefugeBolts.Add(V->AsString());SetStoryChapter(Number(State,TEXT("chapter")));Json Doors=Object(State,TEXT("doors"));
    const Json Locks=Object(State,TEXT("locks")),Inside=Object(State,TEXT("inside_only"));
    for(auto& P:Portals)
    {
        if(Doors->HasField(P.Id))P.Open=Boolean(Doors,P.Id);
        if(Locks->HasField(P.Id))P.Locked=Boolean(Locks,P.Id);
        if(Inside->HasField(P.Id))P.InsideOnly=Boolean(Inside,P.Id);
        RefreshPortalVisual(P);
    }
    if(State->HasField(TEXT("lift_floor")))
    {
        const int Floor=FMath::Clamp(int(Number(State,TEXT("lift_floor"))),0,5);
        RestoreLift(FVector(5000,700,-960+Floor*480+98));
        LiftSelection=FMath::Clamp(int(Number(State,TEXT("lift_selection"),Floor)),0,5);
    }
    Json Props=Object(State,TEXT("props"));for(auto& P:Movables){const auto& A=Array(Props,P.Id);if(A.Num()==3){FVector At=Vector(A);if(!At.ContainsNaN()&&FVector::DistSquared(At,P.Home)<FMath::Square(40000.0)&&FMath::Abs(At.Z-P.Home.Z)<=6)P.Body->SetRelativeLocation(At);}}
}

bool AEWardBookWorld::ToggleBoltForZone(const FString& Zone,FVector Player)
{
    bool Allowed=false;
    for(auto V:Array(WardBookMapData(),TEXT("locations")))if(String(V->AsObject(),TEXT("refuge"))==Zone)Allowed=true;
    for(const auto& R:Rooms)if(R.Id==Zone)Allowed&=R.Bounds.IsInside(Player);
    if(!Allowed)return false;
    const bool Close=!RefugeBolts.Contains(Zone);
    if(Close)for(const auto& P:Portals)if(P.Zone==Zone)
    {const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");if(FMath::Abs(Player.Z-P.At.Z-98)<120&&FVector::DistSquared2D(Player,P.At)<300*300&&FMath::Abs(NS?Player.Y-P.At.Y:Player.X-P.At.X)<65)return false;}
    if(Close)RefugeBolts.Add(Zone);else RefugeBolts.Remove(Zone);
    for(auto& P:Portals)
    {
        bool Same=P.Zone==Zone;
        for(const auto& Q:Portals)if(Q.Zone==Zone&&P.At.Equals(Q.At,1))Same=true;
        if(Same){P.Open=!Close;RefreshPortalVisual(P);}
    }
    return true;
}
void AEWardBookWorld::ToggleRefugeBolt()
{
    if(!Mode.IsValid()||!Mode->Hero())return;
    const FVector Player=Mode->Hero()->GetActorLocation();
    for(const auto& R:Rooms)if(R.Bounds.IsInside(Player)&&ToggleBoltForZone(R.Id,Player))
    {Mode->Cue(TEXT("DOOR/refuge"),Player,.16f);Mode->Notify(RefugeBolts.Contains(R.Id)?TEXT("Убежище заперто на засов · K — открыть"):TEXT("Засов снят"));return;}
    Mode->Notify(TEXT("K — засов внутри убежища. Отойди от дверного проёма."));
}
