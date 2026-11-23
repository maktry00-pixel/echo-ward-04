#include "WardDialogue62.h"
#include "WardStaging64.h"
#include "WardPeople54.h"
#include "WardMonster59.h"
#include "Algo/Reverse.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/PlatformMemory.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UnrealClient.h"
#include "Camera/PlayerCameraManager.h"

namespace WardPlaythrough71
{
struct FRun
{
    TArray<FVector> Path;
    TArray<double> Frames;
    TArray<TSharedPtr<FJsonValue>> Events;
    FVector Goal=FVector::ZeroVector,LastPosition=FVector::ZeroVector;
    FVector Standing=FVector::ZeroVector;
    FString Task,StandingFor,Obstacle,LastSceneStatus,Supply;
    double Began=0,LastFrame=0,NextKey=0,LastProgress=0,NextPlan=0,NextApproach=0,ClosestGoal=DBL_MAX,NextLocalPlan=0;
    double PeakPhysical=0,PeakCommit=0,Distance=0,NextMemory=0,NextTelemetry=0,NextObstacle=0,NextDanger=0,NextSupply=0,EpilogueAt74=0;
    int ArrivalPoint=0,Deaths=0,InputCount=0,LastChapter=0;
    int LastDialogueIndex=INDEX_NONE,LiftLanding=INDEX_NONE,ObservedLiftFloor=INDEX_NONE;
    int F202Passage=INDEX_NONE,N05Passage=0,BeltPassage=INDEX_NONE;
    int InitialShots=0,InitialHits=0,InitialKills=0;
    bool Walking=false,Started=false,WasDead=false,Done=false,LeavingLift=false,AimHeld=false;
    FKey Held=EKeys::Invalid;
};

static void Send(APlayerController& PC,FRun& Run,FKey Key,EInputEvent Event)
{
    PC.InputKey(FInputKeyParams(Key,Event,Event==IE_Released?0.0:1.0));
    if(Event==IE_Pressed)++Run.InputCount;
}
static void Walk(APlayerController& PC,FRun& Run,bool Moving)
{
    if(Run.Walking==Moving)return;
    Run.Walking=Moving;Send(PC,Run,EKeys::W,Moving?IE_Pressed:IE_Released);
}
static void Aim(APlayerController& PC,FRun& Run,bool Aiming)
{
    if(Run.AimHeld==Aiming)return;
    Run.AimHeld=Aiming;
    Send(PC,Run,EKeys::RightMouseButton,Aiming?IE_Pressed:IE_Released);
}
static void Press(APlayerController& PC,FRun& Run,FKey Key,double Now)
{
    if(Now<Run.NextKey||Run.Held.IsValid())return;
    Send(PC,Run,Key,IE_Pressed);
    Run.Held=Key;Run.NextKey=Now+.55;
}
static void Event(FRun& Run,const FString& Label,int Chapter,FVector At)
{
    Json Row=MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("event"),Label);Row->SetStringField(TEXT("at_cm"),At.ToString());
    Row->SetNumberField(TEXT("chapter"),Chapter);Row->SetNumberField(TEXT("seconds"),FPlatformTime::Seconds()-Run.Began);
    Run.Events.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("PLAY71 ch=%d %s at=%s"),Chapter,*Label,*At.ToString());
}
static void Finish(AEWardCampaignMode& G,APlayerController& PC,FRun& Run,const FString& Stop)
{
    Walk(PC,Run,false);Aim(PC,Run,false);Run.Done=true;
    if(!G.Hero()->Dead&&G.Hero()->Health>0)G.Save(TEXT("manual_6"));
    Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),G.HospitalStory->Chapter()==20);
    R->SetStringField(TEXT("stop"),Stop);R->SetStringField(TEXT("task"),Run.Task);
    FString Requested74;FParse::Value(FCommandLine::Get(),TEXT("WardPlayChoices71="),Requested74,false);
    R->SetStringField(TEXT("requested_choices"),Requested74);
    R->SetObjectField(TEXT("actual_choices"),Object(G.HospitalStory->NarrativeState35,TEXT("choices")));
    R->SetObjectField(TEXT("actual_facts"),Object(G.HospitalStory->NarrativeState35,TEXT("facts")));
    R->SetStringField(TEXT("ending_summary"),G.HospitalStory->Chapter()==20?G.HospitalStory->NarrativeEnding35():FString());
    R->SetNumberField(TEXT("movement_mode"),G.Hero()->GetCharacterMovement()->MovementMode);
    R->SetNumberField(TEXT("camera_fade"),PC.PlayerCameraManager?PC.PlayerCameraManager->FadeAmount:0);
    R->SetBoolField(TEXT("camera_fading"),PC.PlayerCameraManager&&PC.PlayerCameraManager->bEnableFading);
    R->SetStringField(TEXT("objective"),G.HospitalStory->Objective());R->SetStringField(TEXT("at_cm"),G.Hero()->GetActorLocation().ToString());
    R->SetStringField(TEXT("scene"),G.HospitalStory->ActiveScene35);
    R->SetStringField(TEXT("scene_status"),WardStaging64::Status(*G.HospitalStory));
    R->SetStringField(TEXT("interaction_focus"),GetNameSafe(G.Hero()->Focus));
    R->SetStringField(TEXT("last_notice"),G.Hero()->Notice);
    R->SetStringField(TEXT("interaction_prompt"),G.HospitalStory->Prompt());
    R->SetStringField(TEXT("speech_session"),G.HospitalStory->DialogueSession62);
    R->SetStringField(TEXT("speech_key"),G.HospitalStory->DialogueKey62);
    R->SetNumberField(TEXT("speech_cursor"),G.HospitalStory->DialogueIndex62);
    R->SetNumberField(TEXT("speech_page"),G.HospitalStory->DialoguePage35);
    R->SetNumberField(TEXT("speech_left"),G.HospitalStory->DialogueLeft62);
    R->SetStringField(TEXT("last_obstacle"),Run.Obstacle);
    R->SetStringField(TEXT("goal_cm"),Run.Goal.ToString());
    R->SetNumberField(TEXT("chapter"),G.HospitalStory->Chapter());R->SetNumberField(TEXT("step"),G.Values.FindRef(TEXT("hospital_step")));
    R->SetNumberField(TEXT("deaths"),Run.Deaths);R->SetNumberField(TEXT("health"),G.Hero()->Health);
    R->SetNumberField(TEXT("shots"),G.Shots-Run.InitialShots);
    R->SetNumberField(TEXT("hits"),G.Hits-Run.InitialHits);
    R->SetNumberField(TEXT("kills"),G.Kills-Run.InitialKills);
    R->SetNumberField(TEXT("wall_seconds"),FPlatformTime::Seconds()-Run.Began);R->SetNumberField(TEXT("walked_m"),Run.Distance/100);
    R->SetNumberField(TEXT("inputs"),Run.InputCount);R->SetNumberField(TEXT("peak_working_set_mib"),Run.PeakPhysical);
    R->SetNumberField(TEXT("peak_process_commit_mib"),Run.PeakCommit);R->SetArrayField(TEXT("events"),Run.Events);
    Json Staging=Object(G.HospitalStory->NarrativeState35,TEXT("staging64"));
    Json Goals=Object(Object(Staging,TEXT("active")),TEXT("goals"));
    for(const auto& Goal:Goals->Values)
    {
        if(const auto* Actor=G.HospitalStory->Props.FindRef(FString(Goal.Key)))
            Goal.Value->AsObject()->SetStringField(TEXT("observed_position_cm"),Actor->GetActorLocation().ToString());
    }
    R->SetObjectField(TEXT("active_staging_goals"),Goals);
    if(!Run.Frames.IsEmpty())
    {
        double Sum=0;for(double F:Run.Frames)Sum+=F;
        Run.Frames.Sort();R->SetNumberField(TEXT("frame_samples"),Run.Frames.Num());
        R->SetNumberField(TEXT("fps"),1000*Run.Frames.Num()/Sum);
        R->SetNumberField(TEXT("p95_ms"),Run.Frames[FMath::FloorToInt(.95*Run.Frames.Num())]);
        R->SetNumberField(TEXT("p99_ms"),Run.Frames[FMath::FloorToInt(.99*Run.Frames.Num())]);
    }
    Write(FPaths::ProjectSavedDir()/TEXT("Prototype/playthrough71.json"),R);
    UE_LOG(LogTemp,Display,TEXT("PLAY71 stopped: %s | %s"),*Stop,*Run.Task);
    FPlatformMisc::RequestExit(false);
}

// Routes only drive the same player keys. The map graph suggests turns but
// cannot open a locked door, move the pawn, or disable its collisions.
static TArray<FVector> Route(const AEWardHospitalGameplay& S,const AEWardBookWorld& W,const AActor* Hero,FVector At,FVector Goal,bool Physical)
{
    FIntVector Start=S.CellAt(At),End=S.CellAt(Goal);
    if(Start.Z!=End.Z)return {};
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardPlayerRouteEdge71),false,Hero);
    TMap<FIntVector,bool> Supported;
    TSet<FIntVector> Sealed;
    for(const auto& P:W.Portals)
    {
        if(P.Open||!(P.Locked||P.InsideOnly||P.Type==TEXT("biometric")||P.Type==TEXT("shutter")))continue;
        const bool AcrossX=P.Side==TEXT("north")||P.Side==TEXT("south");
        const auto C=S.CellAt(P.At+FVector(0,0,88));
        if(C.Z!=Start.Z)continue;
        const int Along=FMath::CeilToInt(P.Width/100);
        for(int I=-Along;I<=Along;++I)Sealed.Add(C+(AcrossX?FIntVector(I,0,0):FIntVector(0,I,0)));
    }
    auto Open=[&](FIntVector C)
    {
        if(Sealed.Contains(C))return false;
        if(Physical)
        {
            if(const bool* Cached=Supported.Find(C))return *Cached;
            FVector Point=S.WorldAt(C);
            Point.Z=C.Z;
            FHitResult Ground;
            const bool Valid=S.GetWorld()->LineTraceSingleByChannel(Ground,Point+FVector(0,0,45),
                Point-FVector(0,0,28),ECC_Visibility,Query)&&Ground.ImpactNormal.Z>.75&&
                !S.GetWorld()->OverlapBlockingTestByChannel(Point+FVector(0,0,98),FQuat::Identity,ECC_Pawn,
                    FCollisionShape::MakeCapsule(34,86),Query);
            Supported.Add(C,Valid);
            return Valid;
        }
        const auto* Runs=S.NavRows.Find(FIntPoint(C.Z,C.Y));
        if(!Runs)return false;
        return Runs->ContainsByPredicate([&](FIntPoint R){return C.X>=R.X&&C.X<R.X+R.Y;});
    };
    auto Nearest=[&](FIntVector C)
    {
        if(Open(C))return C;
        for(int R=1;R<=4;++R)for(int X=-R;X<=R;++X)for(int Y=-R;Y<=R;++Y)
        {
            const auto V=C+FIntVector(X,Y,0);
            if(Open(V))return V;
        }
        return C;
    };
    Start=Nearest(Start);End=Nearest(End);
    if(!Open(Start)||!Open(End))return {};
    auto Edge=[&](FIntVector A,FIntVector B)
    {
        if(!Physical)return true;
        FVector From=A==Start?At:S.WorldAt(A),To=S.WorldAt(B);
        From.Z=To.Z=At.Z+2;
        FHitResult Hit;
        if(!S.GetWorld()->SweepSingleByChannel(Hit,From,To,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(33,82),Query))return true;
        return W.Portals.ContainsByPredicate([&](const auto& P)
        {
            return P.Leaf==Hit.GetComponent()&&!P.Open&&!P.Locked&&!P.InsideOnly&&P.Type!=TEXT("biometric")&&P.Type!=TEXT("shutter");
        });
    };
    struct FNode{FIntVector Cell;int Cost;int Rank;};
    auto Lower=[](const FNode& A,const FNode& B){return A.Rank==B.Rank?A.Cost>B.Cost:A.Rank<B.Rank;};
    auto Estimate=[&](FIntVector C){return FMath::Abs(C.X-End.X)+FMath::Abs(C.Y-End.Y);};
    TArray<FNode> Queue;
    TMap<FIntVector,FIntVector> Parent;
    TMap<FIntVector,int> Cost;
    Queue.HeapPush({Start,0,Estimate(Start)},Lower);
    Cost.Add(Start,0);Parent.Add(Start,Start);
    // A journey between opposite pavilions can legitimately explore more
    // than 300,000 ground cells before finding an entrance.
    const int Limit=Physical?6000:900000;
    for(int Iteration=0;Iteration<Limit&&!Queue.IsEmpty();++Iteration)
    {
        FNode Node;
        Queue.HeapPop(Node,Lower,EAllowShrinking::No);
        if(Node.Cost!=Cost.FindRef(Node.Cell))continue;
        if(Node.Cell==End)
        {
            TArray<FVector> Points;
            FIntVector Previous=FIntVector::ZeroValue;
            for(auto C=End;C!=Start;C=Parent.FindChecked(C))
            {
                const auto Direction=C-Parent.FindChecked(C);
                if(Direction!=Previous)Points.Add(S.WorldAt(C));
                Previous=Direction;
            }
            Algo::Reverse(Points);
            return Points;
        }
        for(const auto Step:{FIntVector(1,0,0),FIntVector(-1,0,0),FIntVector(0,1,0),FIntVector(0,-1,0)})
        {
            const auto Next=Node.Cell+Step;
            if(Physical&&(FMath::Abs(Next.X-Start.X)>36||FMath::Abs(Next.Y-Start.Y)>36))continue;
            if(!Open(Next))continue;
            const int Score=Node.Cost+1;
            const int* Before=Cost.Find(Next);
            if(Before&&*Before<=Score)continue;
            if(!Edge(Node.Cell,Next))continue;
            Cost.Add(Next,Score);Parent.Add(Next,Node.Cell);
            Queue.HeapPush({Next,Score,Score+Estimate(Next)},Lower);
        }
    }
    return {};
}

static bool WalkTo(AEWardCampaignMode& G,APlayerController& PC,FRun& Run,FVector Goal,double Now,bool Indoor)
{
    auto* H=G.Hero();auto* S=G.HospitalStory.Get();
    const FVector At=H->GetActorLocation();
    if(Now>=Run.NextTelemetry)
    {
        Run.NextTelemetry=Now+10;
        UE_LOG(LogTemp,Display,TEXT("PLAY71_STATE %s at=%s goal=%s stand=%s path=%d first=%s screen=%s status=%s hp=%.0f ammo=%d/%d"),
            *Run.Task,*At.ToString(),*Run.Goal.ToString(),*Run.Standing.ToString(),Run.Path.Num(),
            *(Run.Path.IsEmpty()?FVector::ZeroVector:Run.Path[0]).ToString(),*G.Screen,*WardStaging64::Status(*S),H->Health,H->Rounds,H->Reserve);
    }
    if(FVector::Dist2D(At,Goal)<28&&FMath::Abs(At.Z-Goal.Z)<140){Walk(PC,Run,false);return true;}
    const bool SameFloor=FMath::Abs(At.Z-Goal.Z)<170;
    if(!SameFloor){Walk(PC,Run,false);return false;}
    if(!Run.Goal.Equals(Goal,20)){Run.Goal=Goal;Run.Path.Reset();Run.NextPlan=0;Run.ClosestGoal=DBL_MAX;}
    const double DistanceToGoal=FVector::Dist2D(At,Goal);
    if(DistanceToGoal+100<Run.ClosestGoal){Run.ClosestGoal=DistanceToGoal;Run.LastProgress=Now;}
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardPlayerRoute71),false,H);
    FHitResult Hit;
    FCollisionQueryParams FloorQuery=Query;
    for(const auto& Entry:S->Props)FloorQuery.AddIgnoredActor(Entry.Value);
    for(auto Mob:S->Mobs)FloorQuery.AddIgnoredActor(Mob.Get());
    auto SupportedAhead=[&](FVector End)
    {
        // Check only the segment the player is about to walk. Long straight
        // shortcuts must not send the player into an open stairwell or shaft.
        const double Distance=FMath::Min(220.,FVector::Dist2D(At,End));
        const FVector Direction=(End-At).GetSafeNormal2D();
        for(double Offset=FMath::Min(55.,Distance);Offset>0;)
        {
            const FVector Point=At+Direction*Offset;
            FHitResult FloorHit;
            if(!G.GetWorld()->LineTraceSingleByChannel(FloorHit,Point-FVector(0,0,30),
                Point-FVector(0,0,175),ECC_Pawn,FloorQuery)||FloorHit.ImpactNormal.Z<.65)
                return false;
            if(Offset>=Distance)break;
            Offset=FMath::Min(Offset+55,Distance);
        }
        return true;
    };
    auto Clear=[&](FVector End)
    {
        End.Z=At.Z;
        return !G.GetWorld()->SweepSingleByChannel(Hit,At+FVector(0,0,4),End+FVector(0,0,4),FQuat::Identity,
            ECC_Pawn,FCollisionShape::MakeCapsule(32,82),Query)&&(!Indoor||SupportedAhead(End));
    };
    FVector PlanGoal=Goal;
    if(Indoor&&!Clear(Goal))
    {
        auto InRoom=[&](const FWardBookRoom3D& R,FVector P)
        {
            return R.Enclosed&&FMath::Abs(P.Z-98-R.Bounds.Min.Z)<170&&
                P.X>R.Bounds.Min.X&&P.X<R.Bounds.Max.X&&P.Y>R.Bounds.Min.Y&&P.Y<R.Bounds.Max.Y;
        };
        // Use real doorways as intermediate goals. Fine navigation cells near
        // cupboards and clutter can be disconnected from the corridor graph.
        const auto* Leaving=G.BookWorld->Rooms.FindByPredicate([&](const auto& R){return InRoom(R,At)&&!InRoom(R,Goal);});
        const auto* Entering=G.BookWorld->Rooms.FindByPredicate([&](const auto& R){return InRoom(R,Goal)&&!InRoom(R,At);});
        if(Leaving&&FVector::Dist2D(At,Leaving->Outside)>90)PlanGoal=FVector(Leaving->Outside.X,Leaving->Outside.Y,At.Z);
        else if(Entering&&FVector::Dist2D(At,Entering->Outside)>160)PlanGoal=FVector(Entering->Outside.X,Entering->Outside.Y,At.Z);
    }
    FVector Aim=PlanGoal;
    if(Indoor&&!Clear(Goal))
    {
        if(Now>=Run.NextPlan)
        {
            const bool Nearby=FVector::DistSquared2D(At,Goal)<FMath::Square(1600.);
            Run.Path=Nearby?Route(*S,*G.BookWorld.Get(),H,At,Goal,true):TArray<FVector>{};
            if(Run.Path.IsEmpty())Run.Path=Route(*S,*G.BookWorld.Get(),H,At,Goal,false);
            // A room's primary door is only a fallback for disconnected local
            // cells. Through-rooms such as the entrance vestibule have another
            // valid exit; forcing their primary door causes a walk-out loop.
            if(Run.Path.IsEmpty()&&!PlanGoal.Equals(Goal,1))
            {
                Run.Path=Route(*S,*G.BookWorld.Get(),H,At,PlanGoal,true);
                if(Run.Path.IsEmpty())Run.Path=Route(*S,*G.BookWorld.Get(),H,At,PlanGoal,false);
            }
            Run.NextPlan=Now+(Run.Path.IsEmpty()?7:4);
        }
        while(!Run.Path.IsEmpty()&&FVector::Dist2D(At,Run.Path[0])<24)Run.Path.RemoveAt(0);
        if(!Run.Path.IsEmpty())Aim=Run.Path[0];
        for(int I=Run.Path.Num()-1;I>=0;--I)if(Clear(Run.Path[I]))
        {
            Aim=Run.Path[I];Run.Path.RemoveAt(0,I,EAllowShrinking::No);break;
        }
        if(!Clear(Aim))
        {
            if(Now>=Run.NextLocalPlan&&!Run.Path.IsEmpty())
            {
                Run.NextLocalPlan=Now+4;
                for(int I=0;I<Run.Path.Num();++I)
                {
                    const FVector Segment=At+(Run.Path[I]-At).GetClampedToMaxSize(1400);
                    auto Local=Route(*S,*G.BookWorld.Get(),H,At,Segment,true);
                    if(Local.IsEmpty())continue;
                    if(Segment.Equals(Run.Path[I],1))Run.Path.RemoveAt(0,I+1,EAllowShrinking::No);
                    Run.Path.Insert(Local,0);
                    Aim=Run.Path[0];
                    break;
                }
            }
            if(Clear(Aim))
            {
                PC.SetControlRotation(FRotator(-5,(Aim-At).Rotation().Yaw,0));
                Walk(PC,Run,true);
                return false;
            }
            const auto* Door=G.BookWorld->Portals.FindByPredicate([&](const auto& P){return P.Leaf&&P.Leaf==Hit.GetComponent()&&!P.Open&&!P.Locked;});
            Run.Obstacle=GetNameSafe(Hit.GetComponent());
            if(Now>=Run.NextObstacle)
            {
                Run.NextObstacle=Now+20;
                FString Mesh;
                FTransform Instance;
                if(auto* Part=Cast<UInstancedStaticMeshComponent>(Hit.GetComponent()))
                {
                    Mesh=GetPathNameSafe(Part->GetStaticMesh());
                    if(Hit.Item>=0)Part->GetInstanceTransform(Hit.Item,Instance,true);
                }
                FString PersonId;
                for(const auto& Entry:S->Props)if(Entry.Value==Hit.GetActor()){PersonId=Entry.Key;break;}
                UE_LOG(LogTemp,Display,TEXT("PLAY71_OBSTACLE %s owner=%s id=%s item=%d mesh=%s transform=%s hit=%s"),
                    *Run.Obstacle,*GetNameSafe(Hit.GetActor()),*PersonId,Hit.Item,*Mesh,*Instance.ToString(),*Hit.ImpactPoint.ToString());
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/route_obstacle.png"),true,false);
            }
            if(Door&&FVector::Dist2D(At,Door->At)<210)
            {
                Walk(PC,Run,false);Press(PC,Run,EKeys::F,Now);return false;
            }
            const FVector Direction=(Aim-At).GetSafeNormal2D();
            // Backward side steps let ordinary collision movement separate
            // the player from a pair of NPCs beside a corridor wall.
            for(float Offset:{35.f,-35.f,65.f,-65.f,95.f,-95.f,115.f,-115.f,125.f,-125.f,130.f,-130.f,155.f,-155.f,180.f})
            {
                const FVector Detour=At+Direction.RotateAngleAxis(Offset,FVector::UpVector)*120;
                if(Clear(Detour)){Aim=Detour;break;}
            }
        }
    }
    if(Indoor&&!SupportedAhead(Aim)){Walk(PC,Run,false);return false;}
    const auto Look=(Aim-At).Rotation();
    PC.SetControlRotation(FRotator(-5,Look.Yaw,0));
    Walk(PC,Run,true);
    return false;
}

static bool Defend(AEWardCampaignMode& G,APlayerController& PC,FRun& Run,double Now)
{
    auto* H=G.Hero();
    const FVector Eye=H->Camera->GetComponentLocation();
    FVector AimAt;
    AEWardHospitalMob* Opponent=nullptr;
    double Best=FMath::Square(1000.);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(WardPlayerCombat71),true,H);
    for(auto M:G.HospitalStory->Mobs)
    {
        if(!M||!M->Active||M->Dead||M->IsHidden()||WardMonster59::LinkedSignalDisabled(*M))continue;
        const FVector At=M->GetActorLocation();
        const double D=FVector::DistSquared(Eye,At);
        if(D>=Best||FMath::Abs(At.Z-Eye.Z)>200)continue;
        if(M->Class==TEXT("C")||M->Stunned>0)continue;
        if(M->Class==TEXT("B")&&D>FMath::Square(400.))continue;
        const bool Pursuing=M->State59==TEXT("chase")||M->State59==TEXT("windup")||M->State59==TEXT("recovery");
        if(!Pursuing&&D>FMath::Square(450.))continue;
        for(float Height:{.52f,.18f,-.28f})
        {
            const FVector Point=At+FVector(0,0,M->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()*Height);
            FHitResult Hit;
            if(G.GetWorld()->LineTraceSingleByChannel(Hit,Eye,Point,ECC_Visibility,Q)&&Hit.GetActor()==M)
            {
                Best=D;AimAt=Point;Opponent=M;break;
            }
        }
    }
    if(Best==FMath::Square(1000.)){Aim(PC,Run,false);return false;}
    Walk(PC,Run,false);
    const FRotator Facing=(AimAt-Eye).Rotation();
    const bool LinedUp=PC.GetControlRotation().Equals(Facing,2);
    PC.SetControlRotation(Facing);
    if(Opponent&&Opponent->Class==TEXT("B")&&G.FlashCharges>0)
    {Aim(PC,Run,false);Press(PC,Run,EKeys::E,Now);return true;}
    if(H->Health<45&&G.Items.FindRef(TEXT("medkit"))>0){Aim(PC,Run,false);Press(PC,Run,EKeys::H,Now);return true;}
    if(G.ReloadTime>0){Aim(PC,Run,false);return true;}
    if(!G.Flag(TEXT("weapon"))||(H->Rounds<=0&&H->Reserve<=0))
    {
        Aim(PC,Run,false);
        if(Best<FMath::Square(160.))Press(PC,Run,EKeys::V,Now);
        else Walk(PC,Run,true);
        return true;
    }
    if(H->Rounds<=0){Aim(PC,Run,false);Press(PC,Run,EKeys::R,Now);return true;}
    Aim(PC,Run,true);
    if(H->Aiming&&LinedUp)Press(PC,Run,EKeys::LeftMouseButton,Now);
    return true;
}

static FString RequestedChoice74(const FString& Scene)
{
    static const TMap<FString,FString> Plan=[]()
    {
        TMap<FString,FString> Out;FString Text;
        FParse::Value(FCommandLine::Get(),TEXT("WardPlayChoices71="),Text,false);
        TArray<FString> Entries;Text.ParseIntoArray(Entries,TEXT(","),true);
        for(const FString& Entry:Entries){FString Id,Choice;if(Entry.Split(TEXT(":"),&Id,&Choice)&&!Id.IsEmpty()&&!Choice.IsEmpty())Out.Add(Id,Choice);}
        return Out;
    }();
    return Plan.FindRef(Scene);
}

static void Tick(AEWardCampaignMode& G)
{
    static FRun Run;
    auto* PC=G.GetWorld()->GetFirstPlayerController();auto* H=G.Hero();auto* S=G.HospitalStory.Get();
    if(!PC||!H||!S||!G.BookWorld||Run.Done)return;
    const double Now=FPlatformTime::Seconds();
    if(Run.Held.IsValid()){Send(*PC,Run,Run.Held,IE_Released);Run.Held=EKeys::Invalid;}
    if(!Run.Started)
    {
        Run.Started=true;Run.Began=Now;Run.LastFrame=Now;Run.LastProgress=Now;
        G.SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("Prototype/Play71Saves");
        const bool Resume=FParse::Param(FCommandLine::Get(),TEXT("WardPlayResume71"))&&G.LoadLatest();
        if(!Resume)G.Command(TEXT("new_normal"));
        Run.InitialShots=G.Shots;Run.InitialHits=G.Hits;Run.InitialKills=G.Kills;
        Event(Run,Resume?TEXT("resume_normal"):TEXT("new_normal"),S->Chapter(),H->GetActorLocation());
        return;
    }
    const FVector At=H->GetActorLocation();
    const double Step=(Now-Run.LastFrame)*1000;Run.LastFrame=Now;
    if(G.CampaignStarted&&!G.TitleView&&Step>0)Run.Frames.Add(Step);
    if(Now>=Run.NextMemory)
    {
        const auto Mem=FPlatformMemory::GetStats();
        Run.PeakPhysical=FMath::Max(Run.PeakPhysical,Mem.UsedPhysical/1048576.);
        Run.PeakCommit=FMath::Max(Run.PeakCommit,Mem.UsedVirtual/1048576.);
        Run.NextMemory=Now+.5;
    }
    const double Travel=FVector::Dist2D(At,Run.LastPosition);
    if(Travel<150&&Run.Walking)Run.Distance+=Travel;
    Run.LastPosition=At;
    if(S->Chapter()!=Run.LastChapter)
    {
        Run.LastChapter=S->Chapter();Event(Run,TEXT("chapter"),S->Chapter(),At);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/play_chapter_%02d.png"),S->Chapter()),true,false);
    }
    if(H->Dead&&!Run.WasDead){++Run.Deaths;Event(Run,TEXT("death"),S->Chapter(),At);}Run.WasDead=H->Dead;
    if(S->Chapter()>19)
    {
        Walk(*PC,Run,false);Aim(*PC,Run,false);
        if(Run.EpilogueAt74<=0){Run.EpilogueAt74=Now;return;}
        if(Now-Run.EpilogueAt74<2)return;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/epilogue74.png"),true,false);
        Finish(G,*PC,Run,TEXT("epilogue"));return;
    }
    int StopStep=INDEX_NONE;
    FParse::Value(FCommandLine::Get(),TEXT("WardPlayStopStep71="),StopStep);
    if(StopStep>=0&&S->Chapter()==2&&G.Values.FindRef(TEXT("hospital_step"))>=StopStep)
    {Finish(G,*PC,Run,TEXT("checkpoint"));return;}
    int StopChapter=20;
    FParse::Value(FCommandLine::Get(),TEXT("WardPlayStopChapter71="),StopChapter);
    if(StopChapter>1&&S->Chapter()>=StopChapter){Finish(G,*PC,Run,TEXT("checkpoint"));return;}
    double Budget=480;FParse::Value(FCommandLine::Get(),TEXT("WardPlayBudget71="),Budget);
    if(Now-Run.Began>Budget||Run.Deaths>=3||Now-Run.LastProgress>85)
    {Finish(G,*PC,Run,Run.Deaths>=3?TEXT("repeated_deaths"):Now-Run.Began>Budget?TEXT("time_budget"):TEXT("objective_timeout"));return;}
    auto Task=[&](const FString& Value)
    {
        if(Value==Run.Task)return;
        Run.Task=Value;Run.LastProgress=Now;
        Event(Run,Value,S->Chapter(),At);
    };
    if(!G.CampaignStarted||G.TitleView||H->Dead){Walk(*PC,Run,false);Aim(*PC,Run,false);return;}
    if(G.Screen==TEXT("story35"))
    {
        Walk(*PC,Run,false);Aim(*PC,Run,false);Task(TEXT("dialogue/")+S->ActiveScene35);
        const FString Status=WardStaging64::Status(*S);
        if(Now>=Run.NextDanger)
        {
            Run.NextDanger=Now+8;
            UE_LOG(LogTemp,Display,TEXT("PLAY71_SPEECH scene=%s session=%s page=%d cursor=%d/%d left=%.2f main=%d finished=%d action=%d choice=%d key=%s"),
                *S->ActiveScene35,*S->DialogueSession62,S->DialoguePage35,S->DialogueIndex62,S->DialogueLines62.Num(),S->DialogueLeft62,S->DialogueMain62,
                WardDialogue62::PageFinished(*S),WardStaging64::CanFinish(*S),WardDialogue62::AwaitingChoice(*S),*S->DialogueKey62);
        }
        if(S->DialogueIndex62!=Run.LastDialogueIndex||Status!=Run.LastSceneStatus)
        {
            Run.LastDialogueIndex=S->DialogueIndex62;
            Run.LastSceneStatus=Status;
            Run.LastProgress=Now;
        }
        if(WardDialogue62::AwaitingChoice(*S))
        {
            const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five};
            const auto& Choices=Array(S->NarrativeScenes35.FindRef(S->ActiveScene35),TEXT("choices"));
            FString PauseAt74;FParse::Value(FCommandLine::Get(),TEXT("WardPlayPauseChoice71="),PauseAt74);
            if(PauseAt74==S->ActiveScene35&&S->SelectedChoice35.IsEmpty())
            {Finish(G,*PC,Run,TEXT("before_choice"));return;}
            const FString Requested=RequestedChoice74(S->ActiveScene35);
            bool Found=false;
            for(int I=0;I<Choices.Num()&&I<5;++I)
            {
                const Json Option=Choices[I]->AsObject();
                if(!Requested.IsEmpty()&&String(Option,TEXT("id"))!=Requested)continue;
                if(!S->NarrativeRequirements35(Option))
                {
                    if(!Requested.IsEmpty()){Finish(G,*PC,Run,TEXT("requested_choice_unavailable"));return;}
                    continue;
                }
                Found=true;Press(*PC,Run,Keys[I],Now);break;
            }
            if(!Found&&!Requested.IsEmpty()){Finish(G,*PC,Run,TEXT("requested_choice_missing"));return;}
        }
        if(WardStaging64::WaitingAction(*S)&&Now>=Run.NextKey)
        {G.Command(TEXT("story35_action64"));Run.NextKey=Now+.55;}
        return;
    }
    if(!G.Screen.IsEmpty()){Walk(*PC,Run,false);return;}
    if(S->Chapter()>=2&&Defend(G,*PC,Run,Now))return;
    if(S->HomeStage52>=0)
    {
        Task(FString::Printf(TEXT("home/%d"),S->HomeStage52));
        if(S->HomeStage52==1){Walk(*PC,Run,false);return;}
        const bool Door=S->HomeStage52==2;
        const FVector Goal=Door?FVector(-80000,-80260,98):FVector(-79890,-80005,98);
        if(WalkTo(G,*PC,Run,Goal,Now,false))Press(*PC,Run,Door?EKeys::E:EKeys::F,Now);
        return;
    }
    if(S->ArrivalStage>=0&&S->ArrivalStage<5)
    {
        Task(FString::Printf(TEXT("arrival/%d"),S->ArrivalStage));
        if(S->ArrivalStage==0||S->ArrivalStage==4){Walk(*PC,Run,false);return;}
        if(S->ArrivalStage==1){Walk(*PC,Run,false);Press(*PC,Run,EKeys::F,Now);return;}
        const auto& Route=Array(S->ArrivalData,TEXT("route"));
        if(S->ArrivalStage==3)Run.ArrivalPoint=FMath::Max(4,Run.ArrivalPoint);
        if(!Route.IsValidIndex(Run.ArrivalPoint))return;
        if(WalkTo(G,*PC,Run,Vector(Route[Run.ArrivalPoint]->AsArray())*100,Now,false))
        {
            if(Run.ArrivalPoint==3)Press(*PC,Run,EKeys::F,Now);
            else ++Run.ArrivalPoint;
        }
        return;
    }
    if(!S->ActiveScene35.IsEmpty())
    {
        Task(TEXT("interrupted/")+S->ActiveScene35);
        if(Now>=Run.NextDanger)
        {
            Run.NextDanger=Now+8;
            for(auto M:S->Mobs)if(M&&M->Active&&!M->Dead&&FVector::Dist2D(At,M->GetActorLocation())<1600)
                UE_LOG(LogTemp,Display,TEXT("PLAY71_DANGER %s %s class=%s at=%s stun=%.1f hero_sight=%d"),*M->Id,*M->State59,*M->Class,*M->GetActorLocation().ToString(),M->Stunned,M->Sight59(At,true));
            for(const auto& E:Object(Object(Object(S->NarrativeState35,TEXT("staging64")),TEXT("active")),TEXT("goals"))->Values)
                if(auto* A=S->Props.FindRef(FString(E.Key)))if(auto* P=A->FindComponentByClass<UEWardPerson54>())
                    UE_LOG(LogTemp,Display,TEXT("PLAY71_CAST %s state=%s at=%s"),*FString(E.Key),*P->HumanState58(),*A->GetActorLocation().ToString());
        }
        for(const auto& Entry:Object(Object(Object(S->NarrativeState35,TEXT("staging64")),TEXT("active")),TEXT("goals"))->Values)
        {
            auto* Actor=S->Props.FindRef(FString(Entry.Key));
            auto* Person=Actor?Actor->FindComponentByClass<UEWardPerson54>():nullptr;
            if(!Person||Person->HumanState58()!=TEXT("flee"))continue;
            if(FVector::Dist2D(At,Actor->GetActorLocation())>320)
            {
                const FVector Towards=Actor->GetActorLocation()+(At-Actor->GetActorLocation()).GetSafeNormal2D()*220;
                WalkTo(G,*PC,Run,FVector(Towards.X,Towards.Y,At.Z),Now,true);
                return;
            }
        }
        if(const auto* Anchor=S->Props.FindRef(S->ActiveScene35);Anchor&&FVector::Dist2D(At,Anchor->GetActorLocation())>235)
        {
            const FVector Center=Anchor->GetActorLocation();
            FVector Near=Center+(At-Center).GetSafeNormal2D()*205;
            Near.Z=At.Z;
            // A participant may occupy the direct approach after an interruption.
            // Walk around their capsule into the normal 240 cm resume radius.
            FCollisionQueryParams Query(SCENE_QUERY_STAT(ResumeApproach73),false,H);
            const auto Shape=FCollisionShape::MakeCapsule(H->GetCapsuleComponent()->GetScaledCapsuleRadius(),
                H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-2);
            double Best=DBL_MAX;
            for(int I=0;I<16;++I)
            {
                const double Angle=I*PI/8;
                const FVector Candidate(Center.X+205*FMath::Cos(Angle),Center.Y+205*FMath::Sin(Angle),At.Z);
                const double Distance=FVector::DistSquared2D(At,Candidate);
                if(Distance>=Best||G.GetWorld()->OverlapBlockingTestByChannel(Candidate,FQuat::Identity,ECC_Pawn,Shape,Query))continue;
                // A loaded stance can touch the participant capsule. Let
                // CharacterMovement resolve that contact while walking toward
                // a free endpoint rather than rejecting every initial sweep.
                Best=Distance;Near=Candidate;
            }
            WalkTo(G,*PC,Run,Near,Now,true);
            return;
        }
        Walk(*PC,Run,false);Press(*PC,Run,EKeys::F,Now);
        return;
    }
    const auto& Chapters=Array(S->Data,TEXT("chapters"));
    if(!Chapters.IsValidIndex(S->Chapter()-1))return;
    const auto& Required=Array(Chapters[S->Chapter()-1]->AsObject(),TEXT("required"));
    const int Index=G.Values.FindRef(TEXT("hospital_step"));
    if(!Required.IsValidIndex(Index))return;
    FString Id=Required[Index]->AsString();
    if(S->Chapter()==13&&Index<2)
    {
        const FString Relay=Index==0?TEXT("LINK_01"):TEXT("LINK_02");
        if(!G.Completed.Contains(Relay)&&S->Props.Contains(Relay))Id=Relay;
    }
    for(auto Value:Array(S->NarrativeScenes35.FindRef(Id),TEXT("requires")))
    {
        const FString Need=Value->AsString();
        if(!G.Completed.Contains(Need)&&!S->NarrativeFlag35(Need)&&S->Props.Contains(Need)){Id=Need;break;}
    }
    // Use the authored local camera controls through their normal F action.
    // The keeper is intentionally invulnerable; shooting cannot clear this route.
    if(S->Chapter()>=7)
    {
        const FString Camera=At.Z<-600?TEXT("MONSTER_CAMERA_E18"):At.Z>600&&At.Z<1300?TEXT("MONSTER_CAMERA_R40"):TEXT("");
        const FString CameraFlag=At.Z<-600?TEXT("monster_camera_B2_off59"):TEXT("monster_camera_3F_off59");
        if(!Camera.IsEmpty()&&!G.Flag(CameraFlag)&&S->Props.Contains(Camera)&&!S->Props[Camera]->IsHidden())Id=Camera;
    }
    // Talk at the current quest marker before looking for optional supplies.
    if(S->NarrativeScenes35.Contains(Id)&&S->Prompt()==TEXT("F — ")+String(S->Definitions.FindRef(Id),TEXT("label")))
    {
        Walk(*PC,Run,false);Task(Id);Press(*PC,Run,EKeys::F,Now);return;
    }
    // Collect placed supplies through F, without awarding inventory in code.
    // Keep detours on the current floor, near the player and behind unlocked doors.
    const bool NeedAmmo=G.Flag(TEXT("weapon"))&&H->Rounds+H->Reserve<12;
    const bool NeedMedical=H->Health<65&&G.Items.FindRef(TEXT("medkit"))==0;
    if(!Run.Supply.IsEmpty()&&(G.Completed.Contains(Run.Supply)||!S->Props.Contains(Run.Supply)))Run.Supply.Reset();
    if(Run.Supply.IsEmpty()&&(NeedAmmo||NeedMedical)&&Now>=Run.NextSupply)
    {
        Run.NextSupply=Now+3;
        double Best=FMath::Square(1800.);
        for(const auto& Entry:S->Props)
        {
            if(!Entry.Key.StartsWith(TEXT("SUPPLY_")))continue;
            const Json Def=S->Definitions.FindRef(Entry.Key);
            const FString Kind=String(Def,TEXT("kind"));
            if((Kind!=TEXT("ammo")||!NeedAmmo)&&(Kind!=TEXT("medkit")||!NeedMedical))continue;
            if(!Entry.Value||Entry.Value->IsHidden()||G.Completed.Contains(Entry.Key))continue;
            const FVector P=Entry.Value->GetActorLocation();
            const double Distance=FVector::DistSquared2D(At,P);
            if(Distance>=Best||FMath::Abs(At.Z-P.Z-98)>100)continue;
            bool Locked=false;
            for(const auto& Door:G.BookWorld->Portals)if(Door.Zone==String(Def,TEXT("zone"))&&Door.Locked)Locked=true;
            if(Locked||!S->NarrativeRequirements35(Def)||S->FindPath(At,P+FVector(0,0,98),35).IsEmpty())continue;
            Best=Distance;Run.Supply=Entry.Key;
        }
    }
    if(!Run.Supply.IsEmpty())Id=Run.Supply;
    if(H->Health<65&&G.Items.FindRef(TEXT("medkit"))>0)
    {Walk(*PC,Run,false);Press(*PC,Run,EKeys::H,Now);return;}
    Task(Id);
    auto* Target=S->Props.FindRef(Id);
    if(!Target)return;
    if(S->NarrativeScenes35.Contains(Id)&&S->Prompt()==TEXT("F — ")+String(S->Definitions.FindRef(Id),TEXT("label")))
    {
        Walk(*PC,Run,false);
        PC->SetControlRotation((Target->GetActorLocation()+FVector(0,0,15)-H->Camera->GetComponentLocation()).Rotation());
        Press(*PC,Run,EKeys::F,Now);
        return;
    }
    if(Id==TEXT("SCENE_02_04")&&FMath::Abs(At.Z-578)<70&&Run.N05Passage<2&&
        FVector::Dist2D(At,FVector(8365,1090,578))<500)
    {
        const FVector Passage[]={FVector(8460,1090,578),FVector(8460,990,578)};
        if(WalkTo(G,*PC,Run,Passage[Run.N05Passage],Now,true))
        {
            Event(Run,FString::Printf(TEXT("n05_passage/%d"),Run.N05Passage),S->Chapter(),At);
            ++Run.N05Passage;Run.LastProgress=Now;
        }
        return;
    }
    // F202's north bay is separated from the scene marker by two short
    // partitions. Traverse the central aisle and its doorway in order.
    if(Id==TEXT("SCENE_02_02")&&FMath::Abs(At.Z-578)<70)
    {
        if(Run.F202Passage==INDEX_NONE&&At.Y>5940&&At.Y<6180&&At.X>1750&&At.X<2400)
            Run.F202Passage=0;
        const FVector Passage[]={FVector(1900,6040,578),FVector(1900,5640,578),FVector(2130,5640,578)};
        if(Run.F202Passage>=0&&Run.F202Passage<UE_ARRAY_COUNT(Passage))
        {
            if(WalkTo(G,*PC,Run,Passage[Run.F202Passage],Now,true))
            {
                Event(Run,FString::Printf(TEXT("f202_passage/%d"),Run.F202Passage),S->Chapter(),At);
                ++Run.F202Passage;
                Run.LastProgress=Now;
            }
            return;
        }
    }
    // B111's north tool cupboard is reached through the central aisle and
    // the two west-facing subroom doors, not through the dividing wall.
    if(Id==TEXT("NEW_LIFT_BELT")&&FMath::Abs(At.Z+382)<80&&FVector::Dist2D(At,FVector(6500,5500,At.Z))<1600)
    {
        if(Run.BeltPassage==INDEX_NONE)Run.BeltPassage=At.X<6370?2:At.Y<5460?4:0;
        const FVector Passage[]={FVector(6500,5690,At.Z),FVector(6300,5690,At.Z),FVector(6300,5240,At.Z),FVector(6500,5240,At.Z)};
        if(Run.BeltPassage<4)
        {
            if(WalkTo(G,*PC,Run,Passage[Run.BeltPassage],Now,true))
            {Event(Run,FString::Printf(TEXT("belt_passage/%d"),Run.BeltPassage),S->Chapter(),At);++Run.BeltPassage;Run.LastProgress=Now;}
            return;
        }
    }
    const FVector P=Target->GetActorLocation();
    auto& W=*G.BookWorld.Get();
    if(Run.ObservedLiftFloor!=W.LiftFloor)
    {
        Run.ObservedLiftFloor=W.LiftFloor;
        Event(Run,FString::Printf(TEXT("lift_floor/%d"),W.LiftFloor),S->Chapter(),At);
    }
    if(FMath::Abs(P.Z+98-At.Z)>230)
    {
        const int Floor=FMath::Clamp(FMath::RoundToInt((At.Z-98+960)/480),0,5);
        const int Dest=FMath::Clamp(FMath::RoundToInt((P.Z+960)/480),0,5);
        if(W.LiftMoving){Walk(*PC,Run,false);return;}
        if(W.InsideLift(At))
        {
            Walk(*PC,Run,false);
            Press(*PC,Run,W.LiftSelection==Dest?EKeys::F:W.LiftSelection<Dest?EKeys::PageUp:EKeys::PageDown,Now);
        }
        else
        {
            const FVector Landing(5000,1220,-960+Floor*480+98);
            if(W.LiftFloor!=Floor||Run.LiftLanding!=Floor)
            {
                if(WalkTo(G,*PC,Run,Landing,Now,true))
                {
                    if(Run.LiftLanding!=Floor)Event(Run,TEXT("lift_landing"),S->Chapter(),At);
                    Run.LiftLanding=Floor;
                    if(W.LiftFloor!=Floor)Press(*PC,Run,EKeys::F,Now);
                }
            }
            else WalkTo(G,*PC,Run,FVector(5000,700,Landing.Z),Now,true);
        }
        return;
    }
    if(W.InsideLift(At)||Run.LeavingLift)
    {
        Run.LeavingLift=true;
        if(W.LiftMoving){Walk(*PC,Run,false);return;}
        if(WalkTo(G,*PC,Run,FVector(5000,1220,At.Z),Now,true))Run.LeavingLift=false;
        return;
    }
    if(Run.StandingFor!=Id)
    {
        Run.StandingFor=Id;Run.Standing=P+FVector(0,0,98);
        if(S->FindNarrativeStanding35(Id,Run.Standing))Run.Standing.Z=At.Z;
    }
    FCollisionQueryParams UseQuery(SCENE_QUERY_STAT(WardPlayerUse71),false,H);
    FHitResult UseHit;
    const float Floor=FMath::FloorToFloat((P.Z+10)/480)*480;
    const FVector UsePoint=P+FVector(0,0,P.Z-Floor>60?15:65);
    const float InteractionRadius=189.f;
    auto CanUse=[&](FVector Position)
    {
        return FVector::DistSquared(Position,UsePoint)<FMath::Square(InteractionRadius)&&
            (!G.GetWorld()->LineTraceSingleByChannel(UseHit,Position+FVector(0,0,25),P+FVector(0,0,15),ECC_Visibility,UseQuery)||UseHit.GetActor()==Target);
    };
    const bool Reachable=CanUse(At)||(S->NarrativeScenes35.Contains(Id)&&FVector::DistSquared(At,UsePoint)<FMath::Square(InteractionRadius)&&
        S->Prompt()==TEXT("F — ")+String(S->Definitions.FindRef(Id),TEXT("label")));
    if(!Reachable&&FVector::Dist2D(At,P)<550&&Now>=Run.NextApproach)
    {
        Run.NextApproach=Now+.8;
        double Best=DBL_MAX;
        const auto* Capsule=H->GetCapsuleComponent();
        const auto Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()-2);
        for(float Radius:{100.f,145.f,178.f})for(int I=0;I<16;++I)
        {
            const float Angle=I*PI/8;
            const FVector Position(P.X+FMath::Cos(Angle)*Radius,P.Y+FMath::Sin(Angle)*Radius,At.Z);
            const double Distance=FVector::DistSquared(At,Position);
            if(Distance>=Best||!CanUse(Position))continue;
            if(G.GetWorld()->SweepSingleByChannel(UseHit,At,Position,FQuat::Identity,ECC_Pawn,Shape,UseQuery))continue;
            Best=Distance;Run.Standing=Position;
        }
    }
    if(!Reachable)
    {
        if(WalkTo(G,*PC,Run,Run.Standing,Now,true))
        {
            const FVector Near=P+(At-P).GetSafeNormal2D()*105;
            Run.Standing=FVector(Near.X,Near.Y,At.Z);
            Run.NextApproach=Now+1;
        }
        return;
    }
    if(Reachable)
    {
        Walk(*PC,Run,false);
        PC->SetControlRotation((P+FVector(0,0,15)-H->Camera->GetComponentLocation()).Rotation());
        if(Now>=Run.NextDanger)
        {
            Run.NextDanger=Now+8;
            UE_LOG(LogTemp,Display,TEXT("PLAY71_USE id=%s focus=%s prompt=%s notice=%s"),*Id,*GetNameSafe(H->Focus),*S->Prompt(),*H->Notice);
        }
        Press(*PC,Run,EKeys::F,Now);
    }
}
}
