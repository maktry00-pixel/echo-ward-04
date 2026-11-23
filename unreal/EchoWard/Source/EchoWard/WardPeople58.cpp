#include "WardPeople58.h"
#include "WardMonster59.h"
#include "WardDialogue62.h"
#include "Algo/Reverse.h"
#include "WardPeople54.h"
#include "WardHospitalGameplay.h"
#include "WardStaging64.h"
#include "WardCampaign.h"
#include "WardAudio68.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardHomeAssets53.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Misc/Base64.h"

using namespace WardData;
namespace
{
    Json HumanData58()
    {
        static Json Data=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People58/roles.json"));
        return Data;
    }
    auto* HumanModel58(AEWardHospitalGameplay& Story,const FString& Id)
    {
        auto* Actor=Story.Props.FindRef(Id);
        return Actor?Actor->FindComponentByClass<UEWardPerson54>():nullptr;
    }
}
namespace WardPeople58
{
    Json Role(const FString& Id) { return Object(Object(HumanData58(),TEXT("people")),Id); }
    bool SceneMember(AEWardHospitalGameplay& Story,const FString& Id)
    {
        for(auto Actor:Array(Object(HumanData58(),TEXT("scenes")),Story.ActiveScene35))
            if(Actor->AsString()==Id)return true;
        return false;
    }
    void Sync(AEWardHospitalGameplay& Story)
    {
        for(const auto& Pair:Story.Props)
        {
            auto* Model=Pair.Value->FindComponentByClass<UEWardPerson54>();
            if(!Model)continue;
            const bool ScriptDeath=(Pair.Key==TEXT("PERSON_2")&&
                (!Story.NarrativeFlag35(TEXT("RUDIN_ALIVE"))||Story.Mode->Completed.Contains(TEXT("SCENE_01_04"))))||
                (Pair.Key==TEXT("PERSON_4")&&!Story.NarrativeFlag35(TEXT("NIKITA_ALIVE")));
            if(ScriptDeath)
            {
                Model->Impact58(100,FVector(-35,0,30),true);
                Pair.Value->SetActorHiddenInGame(false);
            }
        }
    }
    bool Command(AEWardCampaignMode& Mode,const FString& Action)
    {
        if(!Action.StartsWith(TEXT("npc58_")))return false;
        auto* Story=Mode.HospitalStory.Get();
        if(!Story||!Story->ActiveScene35.StartsWith(TEXT("NPC_")))return true;
        const FString Id=Story->ActiveScene35.RightChop(4);
        auto* Model=HumanModel58(*Story,Id);
        if(!Model)return true;
        if(Action==TEXT("npc58_follow")&&Model->CanEscort58())Model->SetTask58(TEXT("follow"));
        else if(Action==TEXT("npc58_wait"))Model->SetTask58(TEXT("wait"));
        else if(Action==TEXT("npc58_return"))Model->SetTask58(TEXT("routine"));
        else if(Action==TEXT("npc58_help")&&Model->IsFallen58()&&!Model->IsDead58())Model->GetUp58();
        else return true;
        Mode.Show(TEXT(""));
        Story->ActiveScene35.Empty();
        return true;
    }
    Json Snapshot(AEWardHospitalGameplay& Story)
    {
        Json Data=MakeShared<FJsonObject>();
        for(const auto& Pair:Story.Props)
            if(auto* Model=Pair.Value->FindComponentByClass<UEWardPerson54>())
                Data->SetObjectField(Pair.Key,Model->HumanSnapshot58());
        return Data;
    }
    void Restore(AEWardHospitalGameplay& Story,Json Data)
    {
        for(const auto& Pair:Story.Props)
            if(auto* Model=Pair.Value->FindComponentByClass<UEWardPerson54>())
                Model->RestoreHuman58(Object(Data,Pair.Key));
        Sync(Story);
    }
}

void UEWardPerson54::LoadHuman58()
{
    if(HumanLoaded58||Id==TEXT("PERSON_1"))return;
    HumanLoaded58=true;
    BlinkDelay58=2.f+(GetTypeHash(Id)%37)*.11f;
    LastLocation58=GetOwner()->GetActorLocation();
    const FString Path=FPaths::ProjectContentDir()/TEXT("CampaignData/People58")/Model/TEXT("rig.bin");
    TArray<uint8> Data;
    if(!FFileHelper::LoadFileToArray(Data,*Path)||Data.Num()>4*1024*1024)return;
    FMemoryReader Reader(Data);
    uint32 Magic=0,Bones=0,Nodes=0,Parts=0;
    Reader<<Magic<<Bones<<Nodes<<Parts;
    if(Magic!=0x52383545||Clips.IsEmpty()||Bones!=uint32(Clips[0].Frames[0].Joints.Num())||
        Nodes<10||Nodes>32||Parts!=uint32(Sections.Num()))return;
    MajorMap58.SetNum(Bones);
    Reader.Serialize(MajorMap58.GetData(),Bones*sizeof(uint16));
    BodyNodes58.SetNum(Nodes);
    bool Valid=true;
    for(uint16 Index:MajorMap58)Valid&=Index<Nodes;
    for(auto& Node:BodyNodes58)
    {
        uint32 Bone=0;
        Reader<<Bone<<Node.Parent<<Node.Pivot.X<<Node.Pivot.Y<<Node.Pivot.Z<<Node.Radius;
        Node.Bone=Bone;
        Valid&=Bone<Bones&&Node.Parent>=-1&&Node.Parent<int32(Nodes)&&Node.Radius>=4&&Node.Radius<=20;
    }
    for(auto& Section:Sections)
    {
        uint32 Count=0;Reader<<Count;
        if(Count!=uint32(Section.Vertices.Num())){Valid=false;break;}
        Section.Face58.SetNum(Count);
        Reader.Serialize(Section.Face58.GetData(),Count*sizeof(FWardFaceVertex58));
    }
    if(!Valid||Reader.IsError()||Reader.Tell()!=Reader.TotalSize())
    {
        BodyNodes58.Reset();MajorMap58.Reset();
        for(auto& Section:Sections)Section.Face58.Reset();
        UE_LOG(LogTemp,Warning,TEXT("HUMAN58_BAD_RIG %s"),*Id);
        return;
    }
    // Optional authored hand-to-mouth pose. No heavy model is loaded for the prop.
    if(FFileHelper::LoadFileToArray(Data,*(FPaths::ProjectContentDir()/TEXT("CampaignData/People58")/Model/TEXT("smoke.bin"))))
    {
        FMemoryReader SmokeReader(Data);
        uint32 SmokeMagic=0,SmokeBones=0;SmokeReader<<SmokeMagic<<SmokeBones;
        if(SmokeMagic==0x53383545&&SmokeBones==Bones&&Data.Num()==8+int32(Bones)*48+12)
        {
            SmokeSkin58.SetNum(Bones);
            SmokeReader.Serialize(SmokeSkin58.GetData(),Bones*sizeof(FWardPersonPose54));
            SmokeReader<<SmokeHand58.X<<SmokeHand58.Y<<SmokeHand58.Z;
        }
    }
    UE_LOG(LogTemp,Display,TEXT("HUMAN58_RIG %s nodes=%d"),*Id,BodyNodes58.Num());
}

void UEWardPerson54::UpdateFace58(float Delta,bool Talking)
{
    if(Dead58){Blink58=1;Mouth58=0;return;}
    if(BlinkTime58>=0.f)
    {
        BlinkTime58+=Delta;
        Blink58=FMath::Sin(FMath::Clamp(BlinkTime58/.23f,0.f,1.f)*PI);
        if(BlinkTime58>=.23f){BlinkTime58=-1;Blink58=0;BlinkDelay58=3.1f+FMath::FRandRange(0.f,3.8f);}
    }
    else if((BlinkDelay58-=Delta)<=0.f)BlinkTime58=0.f;
    float Target=0;
    if(Talking&&Story&&Story->Mode&&IsValid(Story->Mode->Voice)&&Story->Mode->Voice->IsPlaying())
    {
        const FString File=FPaths::GetCleanFilename(String(Story->NarrativeState35,TEXT("aux_voice57"),VoicePath55(Id)));
        if(File!=VoiceKey58){VoiceKey58=File;SpeechTime58=0;}
        static const Json Envelopes=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People58/voices.json"));
        const auto& Values=Array(Envelopes,File);
        const int32 At=FMath::FloorToInt(SpeechTime58*30.f);
        if(Values.IsValidIndex(At))Target=Values[At]->AsNumber();
        SpeechTime58+=Delta;
    }
    else {VoiceKey58.Empty();SpeechTime58=0;}
    Target=FMath::Max(Target,WardDialogue62::Mouth(*Story,Id));
    Mouth58=FMath::FInterpTo(Mouth58,Target,Delta,23.f);
}

bool UEWardPerson54::CanEscort58() const
{
    return !Dead58&&!Fallen58&&Boolean(WardPeople58::Role(Id),TEXT("escort"))&&
        Story&&!Boolean(Story->Definitions.FindRef(Id),TEXT("past57"));
}

void UEWardPerson54::SetTask58(const FString& Task,const FVector& Goal)
{
    if(Dead58)return;
    if(Task!=TEXT("routine")&&Task!=TEXT("follow")&&Task!=TEXT("wait")&&Task!=TEXT("scene"))return;
    Task58=Task;TaskGoal58=Goal;TaskPath58.Reset();PathClock58=0;StairPath58=false;
    SmokeTime58=-1;
    if(Task==TEXT("routine"))Patrol57.Reset();
}

bool UEWardPerson54::MoveTo58(const FVector& Goal,float Delta,float Speed)
{
    if(!Story||!Story->Mode)return false;
    const FVector At=GetOwner()->GetActorLocation();
    if(!StairPath58&&FVector::Dist2D(At,Goal)<(WardStaging64::Controls(*Story,Id)?23.f:90.f)&&FMath::Abs(At.Z-Goal.Z)<60.f)return false;
    PathClock58-=Delta;
    if(PathClock58<=0.f||(!StairPath58&&FVector::DistSquared(PathGoal58,Goal)>FMath::Square(120.f)))
    {
        TaskPath58.Reset();PathIndex58=0;PathClock58=1.f;PathGoal58=Goal;StairPath58=false;
        if(FMath::Abs(At.Z-Goal.Z)<180.f)
        {
            const FVector ShortGoal=At+(Goal-At).GetClampedToMaxSize(2600);
            if(WardStaging64::Controls(*Story,Id)&&Segment57(At,ShortGoal))TaskPath58.Add(ShortGoal+FVector(0,0,91));else TaskPath58=Story->FindPath(At+FVector(0,0,91),ShortGoal+FVector(0,0,91),25);
            for(auto& Point:TaskPath58)Point.Z-=91;
            if(TaskPath58.IsEmpty()&&Segment57(At,ShortGoal))TaskPath58.Add(ShortGoal);
            if(TaskPath58.IsEmpty()&&WardStaging64::Controls(*Story,Id))
            {
                // Short, grounded detours around other participants; no warp fallback.
                for(float Radius:{85.f,145.f,220.f,320.f})for(int A=0;A<12&&TaskPath58.IsEmpty();++A){const float Angle=A*PI/6;FVector Via;const FVector Trial=At+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0)*Radius;if(Ground57(Trial,Via)&&Segment57(At,Via)&&Segment57(Via,ShortGoal)){TaskPath58.Add(Via);TaskPath58.Add(ShortGoal);}}
            }
        }
        else
        {
            // Use the same two-flight stairs as the generated architecture.
            double Best=DBL_MAX;
            Json Chosen;
            for(auto Entry:Array(WardBookMapData(),TEXT("stairs")))
            {
                const Json Stair=Entry->AsObject();
                const auto& Rect=Array(Stair,TEXT("rect"));
                if(Rect.Num()!=4||At.Z<Number(Stair,TEXT("z_min"))*100-45||At.Z>Number(Stair,TEXT("z_max"))*100+45)continue;
                const FVector EntryPoint(Rect[0]->AsNumber()*100+150,Rect[1]->AsNumber()*100+740,At.Z);
                const double Distance=FVector::DistSquared2D(At,EntryPoint);
                if(Distance<Best){Best=Distance;Chosen=Stair;}
            }
            if(Chosen)
            {
                const auto& Rect=Array(Chosen,TEXT("rect"));
                const FVector Base(Rect[0]->AsNumber()*100,Rect[1]->AsNumber()*100,0);
                const float Floor=FMath::RoundToFloat(At.Z/480.f)*480.f;
                const bool Up=Goal.Z>At.Z;
                const FVector Entry=Base+FVector(150,740,Floor);
                if(Segment57(At,Entry))TaskPath58.Add(Entry);
                else
                {
                    TaskPath58=Story->FindPath(At+FVector(0,0,91),Entry+FVector(0,0,91),25);
                    for(auto& Point:TaskPath58)Point.Z-=91;
                }
                if(!TaskPath58.IsEmpty())
                {
                    const float Z=Up?Floor:Floor-480.f;
                    TArray<FVector> Flight={Base+FVector(78,680,Z),Base+FVector(78,200,Z+240),
                        Base+FVector(222,200,Z+240),Base+FVector(222,680,Z+480),Base+FVector(150,740,Z+480)};
                    if(!Up)
                    {
                        Algo::Reverse(Flight);
                        Flight.Add(Base+FVector(150,740,Z));
                    }
                    TaskPath58.Append(Flight);
                    PathClock58=90;StairPath58=true;
                }
            }
        }
    }
    if(!TaskPath58.IsValidIndex(PathIndex58)){PathClock58=0;StairPath58=false;State58=TEXT("wait_door");WardDialogue62::Event(*Story,Id,TEXT("door"));return false;}
    const FVector Target=TaskPath58[PathIndex58];
    if(FVector::Dist2D(At,Target)<(StairPath58?12.f:WardStaging64::Controls(*Story,Id)?18.f:45.f)&&FMath::Abs(At.Z-Target.Z)<35){++PathIndex58;return false;}
    FVector Next=FMath::VInterpConstantTo(At,Target,Delta,Speed);
    FVector Ground;
    bool Free=Ground57(Next,Ground)&&Segment57(At,Ground);
    if(!Free&&StairPath58)
    {
        // Sweep the whole body up, forward and down. A point trace under the
        // feet alone cannot step onto a riser touched by the capsule's front.
        const float Half=FMath::Max(48.f,Height55*.5f);
        const FCollisionShape Capsule=FCollisionShape::MakeCapsule(25,Half-2);
        FCollisionQueryParams Query(SCENE_QUERY_STAT(NpcStepUp58),false,GetOwner());
        const FVector Center=At+FVector(0,0,Half);
        const FVector Lift=Center+FVector(0,0,32);
        const FVector Forward(Next.X,Next.Y,Lift.Z);
        FHitResult Hit;
        if(!GetWorld()->SweepSingleByChannel(Hit,Center,Lift,FQuat::Identity,ECC_Pawn,Capsule,Query)&&
            !GetWorld()->SweepSingleByChannel(Hit,Lift,Forward,FQuat::Identity,ECC_Pawn,Capsule,Query)&&
            GetWorld()->SweepSingleByChannel(Hit,Forward,Forward-FVector(0,0,70),FQuat::Identity,ECC_Pawn,Capsule,Query)&&
            Hit.ImpactNormal.Z>.1f)
        {
            Ground=Hit.Location-FVector(0,0,Half)+FVector(0,0,2.7);
            FHitResult Underfoot;
            Free=FMath::Abs(Ground.Z-At.Z)<36&&
                GetWorld()->LineTraceSingleByChannel(Underfoot,Ground+FVector(0,0,40),Ground-FVector(0,0,50),ECC_Visibility,Query)&&
                Underfoot.ImpactNormal.Z>.72f;
        }
    }
    if(!Free&&!StairPath58&&Task58==TEXT("scene")&&Story->ActiveScene35.IsEmpty()&&
       Story->Mode->Screen.IsEmpty()&&Story->Mode->Hero())
    {
        const FVector HeroAt=Story->Mode->Hero()->GetActorLocation();
        if(FMath::Abs(HeroAt.Z-At.Z)<Height55&&FVector::DistSquared2D(HeroAt,At)<FMath::Square(220.))
        {
            // The authored grid can route through an open doorway at an angle
            // too tight for the capsule. Walk to a nearby clear approach point
            // and finish the detour before considering the original waypoint.
            const FVector Forward=(Target-At).GetSafeNormal2D();
            for(float Offset : {45.f,-45.f,75.f,-75.f,100.f,-100.f,130.f,-130.f})
            {
                const FVector Direction=Forward.RotateAngleAxis(Offset,FVector::UpVector);
                FVector Detour;
                if(!Ground57(At+Direction*100.f,Detour)||!Segment57(At,Detour)||
                   FVector::DistSquared2D(HeroAt,Detour)<FMath::Square(115.))continue;
                FVector Probe;
                if(!Ground57(FMath::VInterpConstantTo(At,Detour,Delta,Speed),Probe)||!Segment57(At,Probe))continue;
                if(FVector::DistSquared2D(HeroAt,Probe)<FVector::DistSquared2D(HeroAt,At))continue;
                TaskPath58.Insert(Detour,PathIndex58);
                PathClock58=FMath::Max(PathClock58,1.65f);
                Ground=Probe;
                Free=true;
                if(FParse::Param(FCommandLine::Get(),TEXT("WardPlaythrough71")))
                    UE_LOG(LogTemp,Display,TEXT("NPC74_DETOUR %s from=%s via=%s target=%s"),*Id,*At.ToString(),*Detour.ToString(),*Target.ToString());
                break;
            }
        }
    }
    if(!Free&&!StairPath58&&WardStaging64::Controls(*Story,Id))
    {
        const FVector Forward=(Target-At).GetSafeNormal2D(),Side(-Forward.Y,Forward.X,0);const float Step=FMath::Min(22.f,Speed*Delta);
        for(float Sign:{1.f,-1.f}){FVector TrialGround;const FVector Trial=At+(Side*Sign+Forward*.25f).GetSafeNormal()*Step;if(Ground57(Trial,TrialGround)&&Segment57(At,TrialGround)){Ground=TrialGround;Free=true;break;}}
    }
    if(!Free)
    {
        State58=TEXT("wait_door");WardDialogue62::Event(*Story,Id,TEXT("door"));
        if(!StairPath58)PathClock58=FMath::Min(PathClock58,.6f);
        if(StairPath58&&FParse::Param(FCommandLine::Get(),TEXT("WardHumansCheck58")))
        {
            static FVector Last=FVector::ZeroVector;
            if(!Last.Equals(At,.1))
            {
                UE_LOG(LogTemp,Display,TEXT("HUMAN58 stair wait at %s target %s path=%d/%d ground=%s"),*At.ToString(),*Target.ToString(),PathIndex58,TaskPath58.Num(),*Ground.ToString());
                Last=At;
            }
        }
        return false;
    }
    if(const auto* Hero=Story->Mode->Hero())
    {
        const FVector ToHero=Hero->GetActorLocation()-Ground;
        // At marked scene positions the approach may pass beside the player.
        // The capsule sweep above still prevents crossing the player's body.
        const float Clearance=WardStaging64::Controls(*Story,Id)
            ? Hero->GetCapsuleComponent()->GetScaledCapsuleRadius()+30.f
            : 115.f;
        // A scene can leave someone inside the comfort radius. Rejecting
        // every step there also rejects walking away, trapping both people in
        // a doorway. Ground/whole-capsule sweeps above still enforce collision.
        const float CurrentDistance=FVector::Dist2D(Hero->GetActorLocation(),At);
        const float NextDistance=ToHero.Size2D();
        if(NextDistance<Clearance&&FMath::Abs(ToHero.Z)<Height55&&
           NextDistance<=CurrentDistance+.001f)return false;
    }
    const FVector Direction=Ground-At;
    GetOwner()->SetActorLocation(Ground);
    if(Direction.Size2D()>.1f)SetWorldRotation(FMath::RInterpTo(GetComponentRotation(),Direction.Rotation(),Delta,6.f));
    return Direction.Size2D()>.1f;
}

bool UEWardPerson54::TickHuman58(float Delta,bool Talking)
{
    UpdateFace58(Delta,Talking);
    if(Recovery58>=0.f)
    {
        Recovery58+=Delta;
        const float Alpha=FMath::SmoothStep(0.f,1.f,Recovery58);
        for(int32 Index=0;Index<RigidDelta58.Num();++Index)
        {
            const auto& Start=RecoveryPose58[Index];
            RigidDelta58[Index]=FTransform(FQuat::Slerp(Start.GetRotation(),FQuat::Identity,Alpha),
                FMath::Lerp(Start.GetTranslation(),FVector::ZeroVector,Alpha));
        }
        State58=TEXT("get_up");
        if(Recovery58>=1.f){Recovery58=-1;RigidSkin58.Reset();RigidDelta58.Reset();RecoveryPose58.Reset();}
        return false;
    }
    if(Dead58||Fallen58){State58=Dead58?TEXT("dead"):TEXT("fallen");TickRagdoll58(Delta);return false;}
    if(!Story||!Story->Mode)return false;
    const FVector At=GetOwner()->GetActorLocation();
    Pain58=FMath::Max(0.f,Pain58-Delta);
    if(Pain58>0.f){State58=TEXT("hurt");return false;}
    DangerClock58-=Delta;
    if(DangerClock58<=0.f&&!Boolean(Story->Definitions.FindRef(Id),TEXT("past57")))
    {
        DangerClock58=.45f;
        for(const auto& Pair:Story->Props)
        {
            if(Pair.Value==GetOwner()||Pair.Value->IsHidden()||FVector::Dist2D(At,Pair.Value->GetActorLocation())>450||FMath::Abs(At.Z-Pair.Value->GetActorLocation().Z)>80)continue;
            auto* Other=Pair.Value->FindComponentByClass<UEWardPerson54>();
            if(Other&&Other->IsDead58()){WardDialogue62::Event(*Story,Id,TEXT("death_seen"));break;}
        }
        double Nearest=FMath::Square(750.f);
        FVector Danger;
        bool Found=false;
        for(auto Mob:Story->Mobs)
        {
            if(!Mob||!Mob->Active||Mob->Dead||Mob->IsHidden()||Mob->Stunned>0||WardMonster59::CameraDisabled(*Mob)||WardMonster59::LinkedSignalDisabled(*Mob)||FMath::Abs(Mob->GetActorLocation().Z-At.Z)>180)continue;
            const double Distance=FVector::DistSquared2D(Mob->GetActorLocation(),At);
            if(Distance>=Nearest||!WardMonster59::ContactThreatAt(*Mob,At))continue;
            FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(NpcThreat58),true,GetOwner());
            if(GetWorld()->LineTraceSingleByChannel(Hit,At+FVector(0,0,100),Mob->GetActorLocation()+FVector(0,0,40),ECC_Visibility,Query)&&Hit.GetActor()!=Mob)continue;
            Nearest=Distance;Danger=Mob->GetActorLocation();Found=true;
        }
        if(Found)
        {
            DangerHold58=5.f;SmokeTime58=-1;
            if(!WasDanger62)WardDialogue62::Event(*Story,Id,TEXT("danger"));
            WasDanger62=true;Escape62(Danger);
        }
    }
    if(Dead58||Fallen58){TickRagdoll58(Delta);return false;}
    if(DangerHold58>0.f)
    {
        DangerHold58-=Delta;State58=TEXT("flee");
        FleeSpeed62=FMath::FInterpTo(FleeSpeed62,Health58<35?95:240,Delta,4);
        return MoveTo58(DangerGoal58,Delta,FleeSpeed62);
    }
    if(WasDanger62){WasDanger62=false;FleeSpeed62=0;WardDialogue62::Event(*Story,Id,TEXT("safe"));TaskPath58.Reset();PathClock58=0;}
    FVector SceneGoal64;FString ScenePose64;float SceneSpeed64=95;
    if(WardStaging64::Goal(*Story,Id,SceneGoal64,ScenePose64,SceneSpeed64)){State58=TEXT("stage64_")+ScenePose64;SmokeTime58=-1;return SceneSpeed64>0?MoveTo58(SceneGoal64,Delta,SceneSpeed64):false;}
    if(Talking){State58=TEXT("talk");SmokeTime58=-1;return false;}
    if(WardPeople58::SceneMember(*Story,Id))
    {
        if(auto* Marker=Story->Props.FindRef(Story->ActiveScene35))
        {
            State58=TEXT("scene");
            const FVector Offset(120.f,float(int32(GetTypeHash(Id)%3)-1)*110.f,0);
            return MoveTo58(Marker->GetActorLocation()+Offset,Delta,Health58<35?40:80);
        }
    }
    if(Task58==TEXT("routine"))
    {
        const bool AutoEscort=(Id==TEXT("PERSON_3")&&Story->Mode->Completed.Contains(TEXT("SCENE_03_04"))&&Story->Chapter()<19)||
            (Id==TEXT("NEW_NPC_SEMYON")&&Story->NarrativeFlag35(TEXT("SEMYON_HELPED"))&&Story->Chapter()<16);
        if(AutoEscort&&CanEscort58())Task58=TEXT("follow");
    }
    if(Task58==TEXT("follow"))
    {
        const FVector Hero=Story->Mode->Hero()->GetActorLocation()-FVector(0,0,91);
        State58=TEXT("follow");
        if(StairPath58||FVector::Dist2D(At,Hero)>220.f||FMath::Abs(At.Z-Hero.Z)>100)
            return MoveTo58(Hero,Delta,Health58<35?45:105);
        return false;
    }
    if(Task58==TEXT("wait")){State58=TEXT("wait");return false;}
    if(Task58==TEXT("scene")){State58=TEXT("scene");return MoveTo58(TaskGoal58,Delta,80);}
    if(Boolean(WardPeople58::Role(Id),TEXT("smoker"))&&!SmokeSkin58.IsEmpty())
    {
        SmokeClock58-=Delta;
        if(SmokeTime58>=0)
        {
            SmokeTime58+=Delta;State58=TEXT("smoke");
            if(SmokeTime58>7.f){SmokeTime58=-1;SmokeClock58=35+FMath::FRandRange(0.f,25.f);}
            return false;
        }
        if(SmokeClock58<=0){SmokeTime58=0;State58=TEXT("smoke");return false;}
    }
    State58=TEXT("idle");
    return false;
}

void UEWardPerson54::BeginRagdoll58(const FVector& Impulse)
{
    if(BodyNodes58.IsEmpty()||Sample.IsEmpty())return;
    RigidSkin58=Sample;
    RigidDelta58.SetNum(BodyNodes58.Num());
    const auto Transform=GetComponentTransform();
    for(auto& Node:BodyNodes58)
    {
        const FVector3f Pos=Sample[Node.Bone].Apply(Node.Pivot,true);
        Node.Reference=FVector(Pos*BodyScale55(Node.Pivot.Z,Node.Pivot.Y));
        Node.Position=Transform.TransformPosition(Node.Reference);
        const float HeightFraction=FMath::Clamp(float(Node.Reference.Z/Height55),0.f,1.f);
        Node.Previous=Node.Position-(Impulse+Impulse.GetSafeNormal2D()*HeightFraction*65.f)*.016f;
    }
    for(auto& Node:BodyNodes58)
        Node.Length=Node.Parent>=0?FVector::Dist(Node.Position,BodyNodes58[Node.Parent].Position):0;
    TInlineComponentArray<UPrimitiveComponent*> Parts(GetOwner());
    for(auto* Primitive:Parts)Primitive->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    PhysicsClock58=0;
    TickRagdoll58(.016f);
}

void UEWardPerson54::TickRagdoll58(float Delta)
{
    if(RigidSkin58.IsEmpty()||BodyNodes58.IsEmpty())return;
    const int32 Substeps=FMath::Clamp(FMath::CeilToInt(Delta/.016f),1,5);
    const float Step=FMath::Min(Delta,.08f)/Substeps;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HumanBody58),false,GetOwner());
    if(Story&&Story->Mode&&Story->Mode->Hero())Query.AddIgnoredActor(Story->Mode->Hero());
    for(int32 Substep=0;Substep<Substeps;++Substep)
    {
        for(auto& Node:BodyNodes58)
        {
            const FVector Velocity=(Node.Position-Node.Previous)*.965f;
            Node.Previous=Node.Position;
            Node.Position+=Velocity+FVector(0,0,-980.f)*Step*Step;
        }
        // Preserve the chest cage, restrict folding at two-link joints and
        // keep non-neighbouring limbs from passing through the torso.
        auto DistanceConstraint=[&](int32 A,int32 B,double Minimum,double Maximum)
        {
            auto& One=BodyNodes58[A];auto& Two=BodyNodes58[B];
            const FVector Direction=Two.Position-One.Position;
            const double Length=Direction.Size();
            if(Length<1.e-5)return;
            const double Wanted=FMath::Clamp(Length,Minimum,Maximum);
            const FVector Correction=Direction*((Length-Wanted)/Length)*.5;
            One.Position+=Correction;Two.Position-=Correction;
        };
        for(int32 Iteration=0;Iteration<12;++Iteration)
        {
            for(int32 I=0;I<BodyNodes58.Num();++I)
            {
                const auto& Node=BodyNodes58[I];
                if(Node.Parent<0)continue;
                DistanceConstraint(I,Node.Parent,Node.Length,Node.Length);
                const int32 Grandparent=BodyNodes58[Node.Parent].Parent;
                if(Grandparent>=0)
                {
                    const double Span=FVector::Dist(Node.Reference,BodyNodes58[Grandparent].Reference);
                    DistanceConstraint(I,Grandparent,Span*.78,Span*1.02);
                }
            }
            for(int32 A=0;A<BodyNodes58.Num();++A)
                for(int32 B=A+1;B<BodyNodes58.Num();++B)
                {
                    const auto& One=BodyNodes58[A];const auto& Two=BodyNodes58[B];
                    if(One.Parent==B||Two.Parent==A)continue;
                    const double Rest=FVector::Dist(One.Reference,Two.Reference);
                    const float Pelvis=BodyNodes58[0].Pivot.Z;
                    const bool Core=FMath::Abs(One.Pivot.Y)<25&&FMath::Abs(Two.Pivot.Y)<25&&
                        One.Pivot.Z>=Pelvis-.5f&&Two.Pivot.Z>=Pelvis-.5f;
                    if(Core)DistanceConstraint(A,B,Rest*.98,Rest*1.02);
                    else DistanceConstraint(A,B,FMath::Min(double((One.Radius+Two.Radius)*.88f),Rest*.86),DBL_MAX);
                }
        }
        for(auto& Node:BodyNodes58)
        {
            FHitResult Hit;
            if(GetWorld()->SweepSingleByChannel(Hit,Node.Previous,Node.Position,FQuat::Identity,ECC_Visibility,
                    FCollisionShape::MakeSphere(Node.Radius),Query))
            {
                const FVector Velocity=Node.Position-Node.Previous;
                Node.Position=Hit.Location+Hit.ImpactNormal*.3f;
                Node.Previous=Node.Position-(Velocity-FVector::DotProduct(Velocity,Hit.ImpactNormal)*Hit.ImpactNormal)*.35f;
            }
        }
        PhysicsClock58+=Step;
    }
    const auto Transform=GetComponentTransform();
    for(int32 Index=0;Index<BodyNodes58.Num();++Index)
    {
        const auto& Node=BodyNodes58[Index];
        int32 Child=INDEX_NONE;
        for(int32 Other=0;Other<BodyNodes58.Num();++Other)if(BodyNodes58[Other].Parent==Index){Child=Other;break;}
        FVector ReferenceDirection=FVector::UpVector,Direction=FVector::UpVector;
        if(Child!=INDEX_NONE)
        {
            ReferenceDirection=BodyNodes58[Child].Reference-Node.Reference;
            Direction=Transform.InverseTransformVectorNoScale(BodyNodes58[Child].Position-Node.Position);
        }
        else if(Node.Parent>=0)
        {
            ReferenceDirection=Node.Reference-BodyNodes58[Node.Parent].Reference;
            Direction=Transform.InverseTransformVectorNoScale(Node.Position-BodyNodes58[Node.Parent].Position);
        }
        const FQuat Rotation=FQuat::FindBetweenNormals(ReferenceDirection.GetSafeNormal(),Direction.GetSafeNormal());
        const FVector Position=Transform.InverseTransformPosition(Node.Position);
        RigidDelta58[Index]=FTransform(Rotation,Position-Rotation.RotateVector(Node.Reference));
    }
    // The pelvis, rib cage and head share a fitted torso frame. Independent
    // per-spine rotations would otherwise tear collars and jacket seams.
    int32 Top=0;
    const float Pelvis=BodyNodes58[0].Pivot.Z;
    for(int32 I=1;I<BodyNodes58.Num();++I)
        if(FMath::Abs(BodyNodes58[I].Pivot.Y)<11&&BodyNodes58[I].Pivot.Z>BodyNodes58[Top].Pivot.Z)Top=I;
    const auto& Base=BodyNodes58[0];const auto& Head=BodyNodes58[Top];
    const FQuat TorsoRotation=FQuat::FindBetweenNormals((Head.Reference-Base.Reference).GetSafeNormal(),
        Transform.InverseTransformVectorNoScale(Head.Position-Base.Position).GetSafeNormal());
    const FVector TorsoOrigin=Transform.InverseTransformPosition(Base.Position)-TorsoRotation.RotateVector(Base.Reference);
    const FTransform Torso(TorsoRotation,TorsoOrigin);
    for(int32 I=0;I<BodyNodes58.Num();++I)
        if(FMath::Abs(BodyNodes58[I].Pivot.Y)<11&&
            (I==0||BodyNodes58[I].Pivot.Z>Pelvis+2.f))RigidDelta58[I]=Torso;
}

void UEWardPerson54::Impact58(float Damage,const FVector& Impulse,bool ForceDeath)
{
    if(Dead58||!FMath::IsFinite(Damage)||Damage<=0)return;
    if(!LoadAppearance())return;
    const bool Vulnerable=Vulnerable62();
    Health58=ForceDeath?0.f:FMath::Max(Vulnerable?0.f:1.f,Health58-Damage);
    if(Story&&Story->Mode)WardAudio68::Human(*Story->Mode,GetOwner(),Id,Health58<=0?TEXT("death"):TEXT("hurt"),Model.StartsWith(TEXT("f_"))||Model.Contains(TEXT("Female")));
    Pain58=.65f;SmokeTime58=-1;
    if(Story)WardDialogue62::Event(*Story,Id,TEXT("hurt"));
    if(!Vulnerable&&Health58<=1)Damage=FMath::Max(Damage,35.f);
    if(Health58<=0.f||Damage>=35)
    {
        Dead58=Health58<=0.f;Fallen58=true;State58=Dead58?TEXT("dead"):TEXT("fallen");
        if(RigidSkin58.IsEmpty())BeginRagdoll58(Impulse);
    }
}

void UEWardPerson54::GetUp58()
{
    if(Dead58||!Fallen58)return;
    Recovery58=0;RecoveryPose58=RigidDelta58;Fallen58=false;Pain58=.7f;
    Health58=FMath::Max(Health58,25.f);
    Patrol57.Reset();TaskPath58.Reset();State58=TEXT("hurt");
    if(Story&&!Boolean(Story->Definitions.FindRef(Id),TEXT("past57")))
    {
        TInlineComponentArray<UCapsuleComponent*> Capsules(GetOwner());
        for(auto* Capsule:Capsules)Capsule->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
    }
}

void UEWardPerson54::PrepareHumanPose58()
{
    if(!RigidSkin58.IsEmpty()){Sample=RigidSkin58;return;}
    if(Story)WardStaging64::Pose(*Story,Id,Sample);
    if(SmokeTime58>=0.f&&SmokeSkin58.Num()==Sample.Num())
    {
        const float Blend=FMath::SmoothStep(0.f,.9f,SmokeTime58)*(1.f-FMath::SmoothStep(5.7f,6.8f,SmokeTime58));
        for(int32 Bone=0;Bone<Sample.Num();++Bone)
            for(int32 K=0;K<12;++K)Sample[Bone].M[K]=FMath::Lerp(Sample[Bone].M[K],SmokeSkin58[Bone].M[K],Blend);
    }
}

Json UEWardPerson54::HumanSnapshot58() const
{
    Json State=MakeShared<FJsonObject>();
    State->SetNumberField(TEXT("yaw70"),GetOwner()->GetActorRotation().Yaw);
    State->SetNumberField(TEXT("health"),Health58);State->SetBoolField(TEXT("dead"),Dead58);
    State->SetBoolField(TEXT("fallen"),Fallen58||Recovery58>=0.f);State->SetStringField(TEXT("task"),Task58);
    State->SetArrayField(TEXT("goal"),{MakeShared<FJsonValueNumber>(TaskGoal58.X),MakeShared<FJsonValueNumber>(TaskGoal58.Y),MakeShared<FJsonValueNumber>(TaskGoal58.Z)});
    if(!RigidSkin58.IsEmpty())
    {
        TArray<uint8> Bytes;
        FMemoryWriter Writer(Bytes);
        uint32 Count=RigidSkin58.Num(),Nodes=BodyNodes58.Num();
        Writer<<Count<<Nodes;
        Writer.Serialize(const_cast<FWardPersonPose54*>(RigidSkin58.GetData()),Count*sizeof(FWardPersonPose54));
        for(const auto& ConstNode:BodyNodes58)
        {
            auto Node=ConstNode;
            Writer<<Node.Position<<Node.Previous<<Node.Reference<<Node.Length;
        }
        State->SetStringField(TEXT("body58"),FBase64::Encode(Bytes));
    }
    State->SetNumberField(TEXT("danger_hold62"),DangerHold58);State->SetBoolField(TEXT("was_danger62"),WasDanger62);
    State->SetArrayField(TEXT("danger_goal62"),{MakeShared<FJsonValueNumber>(DangerGoal58.X),MakeShared<FJsonValueNumber>(DangerGoal58.Y),MakeShared<FJsonValueNumber>(DangerGoal58.Z)});
    return State;
}
void UEWardPerson54::RestoreHuman58(Json State)
{
    const float Yaw=Number(State,TEXT("yaw70"),GetOwner()->GetActorRotation().Yaw);
    if(FMath::IsFinite(Yaw))GetOwner()->SetActorRotation(FRotator(0,Yaw,0));
    Health58=FMath::Clamp(float(Number(State,TEXT("health"),100)),0.f,100.f);
    Dead58=Boolean(State,TEXT("dead"));Fallen58=Boolean(State,TEXT("fallen"))||Dead58;
    Task58=String(State,TEXT("task"),TEXT("routine"));
    if(Task58!=TEXT("routine")&&Task58!=TEXT("follow")&&Task58!=TEXT("wait")&&Task58!=TEXT("scene"))Task58=TEXT("routine");
    TaskGoal58=Vector(Array(State,TEXT("goal")));
    if(TaskGoal58.ContainsNaN())TaskGoal58=FVector::ZeroVector;
    TaskPath58.Reset();Patrol57.Reset();PathClock58=0;SmokeTime58=-1;SmokeClock58=35;DangerClock58=0;DangerHold58=0;Pain58=0;FleeSpeed62=0;
    DangerHold58=FMath::Clamp(float(Number(State,TEXT("danger_hold62"))),0.f,5.f);WasDanger62=Boolean(State,TEXT("was_danger62"));DangerGoal58=Vector(Array(State,TEXT("danger_goal62")),GetOwner()->GetActorLocation());if(DangerGoal58.ContainsNaN())DangerGoal58=GetOwner()->GetActorLocation();
    RigidSkin58.Reset();RigidDelta58.Reset();RecoveryPose58.Reset();Recovery58=-1;State58=TEXT("idle");
    if(Fallen58&&LoadAppearance())
    {
        BeginRagdoll58(FVector::ZeroVector);
        TArray<uint8> Bytes;
        if(FBase64::Decode(String(State,TEXT("body58")),Bytes)&&Bytes.Num()<32000)
        {
            FMemoryReader Reader(Bytes);
            uint32 Count=0,Nodes=0;Reader<<Count<<Nodes;
            if(Count==uint32(RigidSkin58.Num())&&Nodes==uint32(BodyNodes58.Num()))
            {
                TArray<FWardPersonPose54> Skin;Skin.SetNum(Count);
                Reader.Serialize(Skin.GetData(),Count*sizeof(FWardPersonPose54));
                auto SavedNodes=BodyNodes58;
                bool Valid=true;
                for(const auto& Pose:Skin)for(float Value:Pose.M)Valid&=FMath::IsFinite(Value);
                for(auto& Node:SavedNodes)
                {
                    Reader<<Node.Position<<Node.Previous<<Node.Reference<<Node.Length;
                    Valid&=!Node.Position.ContainsNaN()&&!Node.Previous.ContainsNaN()&&!Node.Reference.ContainsNaN()
                        &&FVector::Dist(Node.Position,GetOwner()->GetActorLocation())<1800&&Node.Length>=0&&Node.Length<150;
                }
                if(Valid&&!Reader.IsError()&&Reader.Tell()==Reader.TotalSize())
                {
                    RigidSkin58=MoveTemp(Skin);BodyNodes58=MoveTemp(SavedNodes);
                    TickRagdoll58(.001f);
                }
            }
        }
    }
    else if(Story&&!Boolean(Story->Definitions.FindRef(Id),TEXT("past57")))
    {
        TInlineComponentArray<UCapsuleComponent*> Capsules(GetOwner());
        for(auto* Capsule:Capsules)Capsule->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
    }
}

bool UEWardPerson54::Vulnerable62()const
{
    if(!Story||Boolean(Story->Definitions.FindRef(Id),TEXT("past57")))return false;
    return Boolean(WardPeople58::Role(Id),TEXT("vulnerable"));
}
bool UEWardPerson54::Escape62(const FVector& Danger)
{
    const FVector At=GetOwner()->GetActorLocation();float Best=-MAX_flt;FVector Goal=At;
    const FVector Away=(At-Danger).GetSafeNormal2D();
    // Candidate routes must actually exist. Score separation and cover rather
    // than running directly into the first wall opposite the monster.
    for(float Radius:{420.f,800.f})for(float Angle:{0.f,45.f,-45.f,90.f,-90.f,135.f,-135.f})
    {
        FVector Ground;const FVector Candidate=At+Away.RotateAngleAxis(Angle,FVector::UpVector)*Radius;
        if(!Ground57(Candidate,Ground))continue;
        auto Route=Story->FindPath(At+FVector(0,0,91),Ground+FVector(0,0,91),25);
        if(Route.IsEmpty()&&!Segment57(At,Ground))continue;
        if(FVector::Dist2D(Ground,Danger)<FVector::Dist2D(At,Danger)+80)continue;
        bool Unsafe=false;for(auto Point:Route)if(FVector::Dist2D(Point,Danger)<130){Unsafe=true;break;}if(Unsafe)continue;
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(EscapeCover62),false,GetOwner());
        const bool Cover=GetWorld()->LineTraceSingleByChannel(Hit,Ground+FVector(0,0,100),Danger+FVector(0,0,40),ECC_Visibility,Q);
        const float Score=FVector::Dist2D(Ground,Danger)+(Cover?450:0)-Radius*.12f;
        if(Score>Best){Best=Score;Goal=Ground;}
    }
    DangerGoal58=Goal;TaskPath58.Reset();PathClock58=0;
    return Best>-MAX_flt;
}
