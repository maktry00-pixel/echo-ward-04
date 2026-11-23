#include "WardHospitalGameplay.h"
#include "WardOnline80.h"
#include "Camera/CameraComponent.h"
#include "WardStaging64.h"
#include "WardNotes65.h"
#include "WardDialogue62.h"
#include "WardPeople54.h"
#include "WardPeople57.h"
#include "WardPeople58.h"
#include "WardMonster59.h"
#include "Misc/Paths.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardContact98.h"
#include "WardBookMap.h"
#include "WardEchoAssets.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
using namespace WardData;
namespace {
TArray<TSharedPtr<FJsonValue>> Vec(FVector P){return {MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)};}
UStaticMeshComponent* Part(AActor* Owner,FVector At,FVector Size,UMaterialInterface* Mat,bool Query=false)
{
    auto* C=NewObject<UStaticMeshComponent>(Owner);C->SetupAttachment(Owner->GetRootComponent());C->SetMobility(EComponentMobility::Movable);
    C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/BookWorld/SM_BookCube.SM_BookCube")));
    C->SetRelativeLocation(At);C->SetRelativeScale3D(Size/100);C->SetMaterial(0,Mat);
    C->SetCollisionEnabled(Query?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,Query?ECR_Block:ECR_Ignore);
    C->SetGenerateOverlapEvents(false);C->SetAffectDynamicIndirectLighting(false);C->SetAffectDistanceFieldLighting(false);C->SetLightingChannels(true,true,false);C->SetCullDistance(10000);C->RegisterComponent();Owner->AddInstanceComponent(C);return C;
}
UMaterialInstanceDynamic* Tint(UObject* Owner,FLinearColor Colour)
{
    auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalSurface.M_HospitalSurface")),Owner);
    M->SetVectorParameterValue(TEXT("Tint"),Colour);M->SetScalarParameterValue(TEXT("Roughness"),.85f);M->SetVectorParameterValue(TEXT("Emission"),Colour*.035f);return M;
}
}
AEWardHospitalGameplay::AEWardHospitalGameplay(){PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Story"));}
void AEWardHospitalGameplay::Build(AEWardCampaignMode* G)
{
    DetailedStoryProps=0;MissingStoryVisuals=0;
    Mode=G;Data=Object(WardBookMapData(),TEXT("story_gameplay"));BuildNarrative35();FreezeAI=FParse::Param(FCommandLine::Get(),TEXT("WardStoryCheck"));
    for(auto V:Array(Object(Data,TEXT("navigation")),TEXT("rows")))
    {const auto& A=V->AsArray();NavRows.FindOrAdd(FIntPoint(A[0]->AsNumber(),A[1]->AsNumber())).Add(FIntPoint(A[2]->AsNumber(),A[3]->AsNumber()));}
    const auto People = Read(FPaths::ProjectContentDir() / TEXT("CampaignData/People54/roster.json"));
    WardPeople57::Append(*this);
    for(auto V:Array(Data,TEXT("placements")))
    {
        Json D=V->AsObject();const FString Id=String(D,TEXT("id")),Kind=String(D,TEXT("kind"));Definitions.Add(Id,D);
        auto* A=GetWorld()->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(A);A->SetRootComponent(Root);Root->RegisterComponent();A->SetActorLocation(Vector(Array(D,TEXT("at")))*100);
        const bool Paper=Kind==TEXT("note")||Kind==TEXT("map")||Kind==TEXT("key");
        auto* Mat=Tint(A,Paper?FLinearColor(.8,.73,.53):Kind==TEXT("scene")?FLinearColor(.07,.38,.4):Kind==TEXT("medkit")?FLinearColor(.65,.09,.07):FLinearColor(.12,.15,.16));
        bool Detailed=false;
        if(Kind==TEXT("person"))
        {
            const FString PersonModel = Id.StartsWith(TEXT("FILE_"))?String(D,TEXT("model")):String(People, Id);
            if (!PersonModel.IsEmpty() && FPaths::FileExists(FPaths::ProjectContentDir() /
                    (Id.StartsWith(TEXT("FILE_"))?TEXT("CampaignData/People56"):TEXT("CampaignData/People54")) / PersonModel / TEXT("mesh.bin")))
            {
                auto* Visual = NewObject<UEWardPerson54>(A);
                Visual->SetIdentity(this, Id, PersonModel);
                Visual->SetupAttachment(Root);
                Visual->SetMobility(EComponentMobility::Movable);
                A->AddInstanceComponent(Visual);
                Visual->RegisterComponent();
                Detailed = true;
            }
            // Keep an actor visible in installations with older content.
            else if(auto* Asset=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Prototype/Character/SK_Survivor.SK_Survivor")))
            {
                auto* C=NewObject<USkeletalMeshComponent>(A);C->SetupAttachment(Root);C->SetSkeletalMeshAsset(Asset);
                const auto B=Asset->GetBounds();const float Scale=170.f/FMath::Max(1.f,float(B.BoxExtent.Z*2));
                C->SetRelativeScale3D(FVector(Scale));C->SetRelativeLocation(FVector(0,0,-(B.Origin.Z-B.BoxExtent.Z)*Scale));C->SetRelativeRotation(FRotator(0,-90,0));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetLightingChannels(true,true,false);C->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;C->RegisterComponent();A->AddInstanceComponent(C);
                C->PlayAnimation(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Prototype/Character/A_Idle.A_Idle")),true);Detailed=true;
            }
        }
        else
        {
            int Model=Kind==TEXT("weapon")?555:Kind==TEXT("shotgun")?559:Kind==TEXT("medkit")?554:Kind==TEXT("ammo")?556:Kind==TEXT("shells")?560:Kind==TEXT("node")?149:Kind==TEXT("scene")?260:262;
            if(Kind==TEXT("key"))Model=Id==TEXT("spare_fuse")?152:Id==TEXT("service_prybar")?638:Id==TEXT("evidence_case")?292:550;
            const FVector Size=Id==TEXT("service_prybar")?FVector(9,4,66):Id==TEXT("spare_fuse")?FVector(5,3,12):Kind==TEXT("weapon")?FVector(28,7,18):Kind==TEXT("shotgun")?FVector(78,8,20):Kind==TEXT("node")?FVector(25,15,28):Kind==TEXT("medkit")?FVector(25,20,12):Kind==TEXT("ammo")?FVector(9,5,17):Kind==TEXT("shells")?FVector(4,4,8):Kind==TEXT("key")?FVector(13,9,3):Kind==TEXT("scene")?FVector(28,20,4):FVector(28,20,1);
            Detailed=WardEchoAssets::Attach(A,Root,Model,FBox(FVector(-Size.X*.5,-Size.Y*.5,2),FVector(Size.X*.5,Size.Y*.5,2+Size.Z)))!=nullptr;
        }
        if(Detailed)
        {
            ++DetailedStoryProps;
            // Preserve the existing interaction target, without its placeholder visual.
            auto* Query=Part(A,FVector(0,0,Kind==TEXT("person")?100:Paper?3:12),Kind==TEXT("person")?FVector(42,25,65):Paper?FVector(28,20,2):FVector(25,22,22),Mat,true);Query->SetVisibility(false);
        }
        else if(Kind==TEXT("person"))
        {
            Part(A,FVector(0,0,100),FVector(42,25,65),Mat,true);Part(A,FVector(0,0,151),FVector(26,25,30),Mat);
            for(int S:{-1,1}){Part(A,FVector(0,S*15,36),FVector(15,15,72),Mat);Part(A,FVector(0,S*29,99),FVector(15,15,62),Mat);}
        }
        else if(Kind==TEXT("weapon")||Kind==TEXT("shotgun"))
        {Part(A,FVector(0,0,8),FVector(Kind==TEXT("shotgun")?78:28,6,7),Mat,true);Part(A,FVector(-6,0,4),FVector(8,6,15),Mat);}
        else {Part(A,FVector(0,0,Paper?3:12),Paper?FVector(28,20,2):FVector(25,22,22),Mat,true);if(Kind==TEXT("medkit")){auto* White=Tint(A,FLinearColor(.9,.9,.85));Part(A,FVector(0,0,24),FVector(16,5,1),White);Part(A,FVector(0,0,24),FVector(5,16,1),White);}}
        if(!Detailed){++MissingStoryVisuals;UE_LOG(LogTemp,Warning,TEXT("STORY_VISUAL_MISSING %s %s"),*Id,*Kind);}
        if(NarrativeEnabled35&&Kind==TEXT("person"))
        {
            // Reuse a valid nearby standing cell if a supplied point lies in furniture.
            FVector Standing=A->GetActorLocation()+FVector(0,0,91);
            FCollisionQueryParams Query(SCENE_QUERY_STAT(ScenarioPerson35),false,A);if(G->Hero())Query.AddIgnoredActor(G->Hero());
            if(GetWorld()->OverlapBlockingTestByChannel(Standing,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(30,86),Query))
                for(int R=1;R<=8;++R){bool Found=false;for(int J=0;J<8;++J){const FVector Trial=Standing+FVector(FMath::Cos(J*PI/4)*R*35,FMath::Sin(J*PI/4)*R*35,0);if(!GetWorld()->OverlapBlockingTestByChannel(Trial,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(30,86),Query)){A->SetActorLocation(Trial-FVector(0,0,91));Found=true;break;}}if(Found)break;}
        }
        if (Kind==TEXT("person")&&Id!=TEXT("PERSON_1"))
        {
            const auto Card=Object(Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People56/archive.json")),Id);
            const float Half=Number(Card,TEXT("height"),170)*.5f;
            auto* Collision=NewObject<UCapsuleComponent>(A);
            Collision->SetupAttachment(Root);
            Collision->SetRelativeLocation(FVector(0,0,Half));
            Collision->SetCapsuleSize(25,Half-2);
            Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
            Collision->SetCollisionResponseToChannel(ECC_Pawn,Boolean(D,TEXT("past57"))?ECR_Ignore:ECR_Block);
            A->AddInstanceComponent(Collision);
            Collision->RegisterComponent();
        }
        Props.Add(Id,A);PropActors.Add(A);
    }
    for(auto V:Array(Data,TEXT("mobs")))
    {
        Json Row=V->AsObject();bool Duplicate=false;
        if(NarrativeEnabled35)for(auto M:Mobs)if(!String(Row,TEXT("actor_guid")).IsEmpty()&&String(Row,TEXT("actor_guid"))==String(M->Definition,TEXT("actor_guid")))Duplicate=true;
        if(!Duplicate)SpawnMob(Row);
    }
    UE_LOG(LogTemp,Display,TEXT("STORY_VISUAL_COUNT detailed=%d missing=%d expected=%d"),DetailedStoryProps,MissingStoryVisuals,Array(Data,TEXT("placements")).Num());
    WardMonster59::AddControls(*this);BuildArrival();
    if(NarrativeEnabled35){BuildNarrativeGates35();WardNotes65::Fit(*this);}
    ResetStory();
}
int AEWardHospitalGameplay::Chapter()const{return Mode?FMath::Clamp(Mode->Values.FindRef(TEXT("hospital_chapter")),1,20):1;}
void AEWardHospitalGameplay::ResetStory()
{
    for(int I=Mobs.Num()-1;I>=0;--I)if(Mobs[I]->Id.StartsWith(TEXT("CHILD_"))){Mobs[I]->Destroy();Mobs.RemoveAt(I);}else Mobs[I]->ResetMob();
    for (const auto& Pair:Definitions)
    {
        if (Boolean(Pair.Value,TEXT("past57"))) Pair.Value->SetBoolField(TEXT("memory_active57"),false);
        if(String(Pair.Value,TEXT("kind"))==TEXT("person")&&Pair.Key!=TEXT("PERSON_1"))
            if(auto* Actor=Props.FindRef(Pair.Key))
            {
                Actor->SetActorLocation(Vector(Array(Pair.Value,TEXT("at")))*100);
                if(auto* Model=Actor->FindComponentByClass<UEWardPerson54>())
                {
                    Model->RestoreHuman58(MakeShared<FJsonObject>());
                    Model->ResetPatrol57();
                }
            }
    }
    ResetNarrative35();ResetArrival();SpawnedChildren=0;Mode->Values.Add(TEXT("hospital_step"),0);SetChapter(1);
}
void AEWardHospitalGameplay::SetChapter(int Ch)
{
    Mode->Values.Add(TEXT("hospital_chapter"),FMath::Clamp(Ch,1,20));Mode->Values.Add(TEXT("hospital_step"),0);
    if(Mode->BookWorld)
    {
        Mode->BookWorld->SetStoryChapter(FMath::Min(19,Ch));
        // A prototype scene may be assigned to a previously class-C room. Open
        // every scene route explicitly; story closures must not make it unwinnable.
        TSet<FString> Critical;for(auto C:Array(Data,TEXT("chapters")))for(auto Z:Array(C->AsObject(),TEXT("anchors")))Critical.Add(Z->AsString());
        // T12 connects the freight lift to the mandatory rooftop evacuation
        // route. The room's general class-C lock must yield once it is used.
        if(Ch>=14)Critical.Add(TEXT("T12"));
        for(auto& P:Mode->BookWorld->Portals)if(Critical.Contains(P.Zone)&&!Mode->BookWorld->RefugeBolts.Contains(P.Zone))
        {P.Locked=false;P.Open=true;P.InsideOnly=false;Mode->BookWorld->RefreshPortalVisual(P);}
    }
    Refresh();if(ArrivalStage>=0)ApplyArrivalState();
}
void AEWardHospitalGameplay::Refresh()
{
    for(const auto& E:Props)
    {Json D=Definitions.FindRef(E.Key);const FString Kind=String(D,TEXT("kind"));const bool Show=(Boolean(D,TEXT("past57"))?Boolean(D,TEXT("memory_active57")):Chapter()>=Number(D,TEXT("chapter"),1))&&(!Mode->Completed.Contains(E.Key)||Kind==TEXT("scene")||Kind==TEXT("person"));const bool IntroMarker=ArrivalStage>=0&&ArrivalStage<5&&(E.Key==TEXT("SCENE_01_01")||E.Key==TEXT("SCENE_01_02"));E.Value->SetActorHiddenInGame(!Show||IntroMarker);E.Value->SetActorEnableCollision(Show&&!IntroMarker);}
    WardMonster59::Refresh(*this);
    if(NarrativeEnabled35)RefreshNarrative35();
    RefreshNavigation();
}
FString AEWardHospitalGameplay::Objective()const
{
    if(HomeStage52>=0)return HomeObjective52();
    if(ArrivalStage>=0&&ArrivalStage<5)return ArrivalObjective();
    if(Chapter()>19)return NarrativeEnabled35?TEXT("История завершена · J — найденные документы"):TEXT("Маршрут завершён");
    if(NarrativeEnabled35&&!ActiveScene35.IsEmpty())return TEXT("F — продолжить сцену: ")+NarrativeTitle35();
    Json C=Array(Data,TEXT("chapters"))[Chapter()-1]->AsObject();const auto& Steps=Array(C,TEXT("required"));int Step=Mode->Values.FindRef(TEXT("hospital_step"));
    if(!Steps.IsValidIndex(Step))return TEXT("Исследуй комплекс");Json P=Definitions.FindRef(Steps[Step]->AsString());
    FString Place=String(P,TEXT("zone"));
    if(NarrativeEnabled35)
    {
        if(String(P,TEXT("site"))==TEXT("UT"))Place=TEXT("UT / центральный подземный шлюз");
        if(String(P,TEXT("id"))==TEXT("SCENE_19_03"))
        {
            const FString Plan=String(Object(NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"));
            const bool Inside=Plan==TEXT("REPLACE")||Plan==TEXT("CONTINUE")||(Plan==TEXT("DESTROY")&&NarrativeFlag35(TEXT("HERO_STAYS")));
            if(Inside){const Json Core=Definitions.FindRef(TEXT("SCENE_19_01"));Place=String(Core,TEXT("site"))+TEXT(" / ")+String(Core,TEXT("zone"));}
            else if(!Plan.IsEmpty()&&Plan!=TEXT("EVAC"))Place=TEXT("Наземный выход у приёмной");
        }
    }
    return FString::Printf(TEXT("Глава %d · %s · %s [%s] · F — взаимодействие"),Chapter(),*String(C,TEXT("title")),*String(P,TEXT("label")),*Place);
}
namespace
{
// Aim selects a visible pickup even when a nearby person's broad interaction
// capsule is closer. Prompt and F use the same resolver.
FString AimedPickupTarget(const AEWardHospitalGameplay& Story)
{
    const auto* Hero=Story.Mode?Story.Mode->Hero():nullptr;
    if(!Hero||!Hero->Camera)return {};
    const FVector Eye=Hero->Camera->GetComponentLocation();
    const FVector Forward=Hero->Camera->GetForwardVector();
    const FVector From=Hero->GetActorLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardAimedPickup73),true,Hero);
    for(const auto& E:Story.Props)
        if(String(Story.Definitions.FindRef(E.Key),TEXT("kind"))==TEXT("person"))Query.AddIgnoredActor(E.Value);
    FString Best;float Alignment=.985f;double Nearest=DBL_MAX;
    for(const auto& E:Story.Props)
    {
        if(!E.Value||E.Value->IsHidden()||Story.Mode->Completed.Contains(E.Key))continue;
        const Json Definition=Story.Definitions.FindRef(E.Key);
        const FString Kind=String(Definition,TEXT("kind"));
        if(Kind.IsEmpty()||Kind==TEXT("person")||Kind==TEXT("scene"))continue;
        const FVector At=E.Value->GetActorLocation();
        const float Floor=FMath::FloorToFloat((At.Z+10)/480)*480;
        const FVector UsePoint=At+FVector(0,0,Story.NarrativeEnabled35&&At.Z-Floor>60?15:65);
        const double Distance=FVector::DistSquared(From,UsePoint);
        if(Distance>=FMath::Square(190.f))continue;
        const float Dot=FVector::DotProduct((At+FVector(0,0,15)-Eye).GetSafeNormal(),Forward);
        if(Dot<Alignment||(FMath::IsNearlyEqual(Dot,Alignment,.0001f)&&Distance>=Nearest))continue;
        FHitResult Hit;
        if(Story.GetWorld()->LineTraceSingleByChannel(Hit,Eye,At+FVector(0,0,15),ECC_Visibility,Query)&&Hit.GetActor()!=E.Value)continue;
        Alignment=Dot;Nearest=Distance;Best=E.Key;
    }
    return Best;
}
FString RequiredSceneId74(const AEWardHospitalGameplay& Story)
{
    if(!Story.NarrativeEnabled35||Story.Chapter()>19)return {};
    const auto& Chapters=Array(Story.Data,TEXT("chapters"));
    if(!Chapters.IsValidIndex(Story.Chapter()-1))return {};
    const auto& Steps=Array(Chapters[Story.Chapter()-1]->AsObject(),TEXT("required"));
    const int Step=Story.Mode->Values.FindRef(TEXT("hospital_step"));
    return Steps.IsValidIndex(Step)?Steps[Step]->AsString():FString();
}
void IgnoreSceneMarkers74(const AEWardHospitalGameplay& Story,FCollisionQueryParams& Query)
{
    // Several authored scenes may reuse a room. Their small interaction
    // volumes must not occlude each other; walls and furniture still block.
    const FVector From=Story.Mode->Hero()->GetActorLocation();
    for(const auto& Entry:Story.Props)
        if(Entry.Value&&FVector::DistSquared(From,Entry.Value->GetActorLocation())<FMath::Square(450.f)&&
           String(Story.Definitions.FindRef(Entry.Key),TEXT("kind"))==TEXT("scene"))
            Query.AddIgnoredActor(Entry.Value);
}
FString NearbyStoryTarget(const AEWardHospitalGameplay& Story)
{
    if(!Story.Mode||!Story.Mode->Hero())return {};
    const auto* Hero=Story.Mode->Hero();const FVector H=Hero->GetActorLocation();
    double Best=190*190;FString Found;FCollisionQueryParams Q(SCENE_QUERY_STAT(WardNearbyUse73),false,Hero);
    const FString Required=RequiredSceneId74(Story);IgnoreSceneMarkers74(Story,Q);
    for(const auto& E:Story.Props)if(E.Value&&!E.Value->IsHidden()&&!Story.Mode->Completed.Contains(E.Key))
    {
        if(Story.NarrativeEnabled35&&String(Story.Definitions.FindRef(E.Key),TEXT("kind"))==TEXT("scene")&&E.Key!=Required)continue;
        const FVector At=E.Value->GetActorLocation()+FVector(0,0,15);
        const float Floor=FMath::FloorToFloat((At.Z-5)/480)*480;
        const double Distance=FVector::DistSquared(H,At+FVector(0,0,Story.NarrativeEnabled35&&At.Z-15-Floor>60?0:50));
        if(Distance>=Best)continue;
        FHitResult Hit;if(Story.GetWorld()->LineTraceSingleByChannel(Hit,H+FVector(0,0,25),At,ECC_Visibility,Q)&&Hit.GetActor()!=E.Value)continue;
        Best=Distance;Found=E.Key;
    }
    return Found;
}
FString ReadyStoryTarget(const AEWardHospitalGameplay& Story)
{
    const int Chapter=Story.Chapter();
    if(!Story.NarrativeEnabled35||Chapter>19||
        (Story.ArrivalStage>=0&&Story.ArrivalStage<5))return {};
    const auto& Chapters=Array(Story.Data,TEXT("chapters"));
    if(!Chapters.IsValidIndex(Chapter-1))return {};
    const auto& Steps=Array(Chapters[Chapter-1]->AsObject(),TEXT("required"));
    const int Step=Story.Mode->Values.FindRef(TEXT("hospital_step"));
    if(!Steps.IsValidIndex(Step))return {};
    const FString Id=Steps[Step]->AsString();
    const auto* Actor=Story.Props.FindRef(Id);
    const Json Scene=Story.NarrativeScenes35.FindRef(Id);
    if(!Actor||Actor->IsHidden()||!Scene||Story.Mode->Completed.Contains(Id))return {};
    const auto* Hero=Story.Mode->Hero();
    const FVector At=Actor->GetActorLocation(),From=Hero->GetActorLocation();
    const float Floor=FMath::FloorToFloat((At.Z+10)/480)*480;
    if(FVector::DistSquared(From,At+FVector(0,0,At.Z-Floor>60?15:65))>=FMath::Square(190.f)||
        !Story.NarrativeRequirements35(Scene))return {};

    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardRequiredScene),false,Hero);
    IgnoreSceneMarkers74(Story,Query);
    FHitResult Hit;
    if(Story.GetWorld()->LineTraceSingleByChannel(Hit,From+FVector(0,0,25),At+FVector(0,0,15),ECC_Visibility,Query)&&Hit.GetActor()!=Actor)
    {
        // A participant standing next to the trigger can cover it with their
        // interaction volume. Geometry still has to leave a clear line.
        for(const auto& Entry:Story.Props)
            if(String(Story.Definitions.FindRef(Entry.Key),TEXT("kind"))==TEXT("person"))Query.AddIgnoredActor(Entry.Value);
        if(Story.GetWorld()->LineTraceSingleByChannel(Hit,From+FVector(0,0,25),At+FVector(0,0,15),ECC_Visibility,Query)&&Hit.GetActor()!=Actor)return {};
    }
    return Id;
}
}
FString WardOnline80::StoryTarget(const AEWardHospitalGameplay& Story)
{
    const FString Pickup = AimedPickupTarget(Story);
    if (!Story.ActiveScene35.IsEmpty() && Story.Mode && PlayingMap(*Story.Mode))
    {
        if (ParallelPickup(*Story.Mode, Pickup)) return Pickup;
        const FString Nearby = NearbyStoryTarget(Story);
        return ParallelPickup(*Story.Mode, Nearby) ? Nearby : FString();
    }
    if (String(Story.Definitions.FindRef(Pickup), TEXT("kind")) == TEXT("node")) return Pickup;
    if (const FString Scene = ReadyStoryTarget(Story); !Scene.IsEmpty()) return Scene;
    return Pickup.IsEmpty() ? NearbyStoryTarget(Story) : Pickup;
}

FString AEWardHospitalGameplay::Prompt()const
{
    if(HomeStage52>=0)return HomeObjective52();
    if(!Mode||!Mode->Hero())return FString();const FString CarPrompt=VehiclePrompt31();if(!CarPrompt.IsEmpty())return CarPrompt;
    const FString Pickup=AimedPickupTarget(*this);
    if(String(Definitions.FindRef(Pickup),TEXT("kind"))==TEXT("node"))
        return TEXT("F — ")+String(Definitions.FindRef(Pickup),TEXT("label"));
    if(const FString Scene=ReadyStoryTarget(*this);!Scene.IsEmpty())return TEXT("F — ")+String(Definitions.FindRef(Scene),TEXT("label"));
    if(!Pickup.IsEmpty())return TEXT("F — ")+String(Definitions.FindRef(Pickup),TEXT("label"));
    if (Mode->BookWorld && WardContact98::AimedCart(*Mode->BookWorld, *Mode->Hero()) != INDEX_NONE)
        return TEXT("F — толкнуть · Shift + F — потянуть");
    const FString Target=NearbyStoryTarget(*this);
    return Target.IsEmpty()?FString():TEXT("F — ")+String(Definitions.FindRef(Target),TEXT("label"));
}
bool AEWardHospitalGameplay::Interact()
{
    if(HomeStage52>=0)return UseHome52();
    if(ArrivalInteract())return true;
    if (NarrativeEnabled35 && !ActiveScene35.IsEmpty())
    {
        if (NarrativeFlag35(TEXT("DIALOGUE_SUSPENDED")))
        {
            // A suspended conversation must not block the physical switch
            // needed to remove its danger. Preserve its page and cursor.
            FString Control = AimedPickupTarget(*this);
            if (Control.IsEmpty()) Control = NearbyStoryTarget(*this);
            const FString Kind=String(Definitions.FindRef(Control),TEXT("kind"));
            if(Kind==TEXT("monster_control59")||Kind==TEXT("node"))return UseId(Control);
            if (ActiveScene35 == TEXT("CHECKPOINT_69")) return false;
            const AActor* Anchor = Props.FindRef(ActiveScene35);
            if (ActiveScene35 == TEXT("SCENE_01_01") && ArrivalStage >= 0) Anchor = ArrivalCar;
            if (Anchor && FVector::Dist2D(Anchor->GetActorLocation(), Mode->Hero()->GetActorLocation()) > 240) return false;
        }
        SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"), false);
        Mode->Show(TEXT("story35"));
        return true;
    }
    if(Mode->BookWorld->FurnitureHiding)return false;
    const FString Pickup=AimedPickupTarget(*this);
    if(String(Definitions.FindRef(Pickup),TEXT("kind"))==TEXT("node"))return UseId(Pickup);
    if(const FString Scene=ReadyStoryTarget(*this);!Scene.IsEmpty())return UseId(Scene);
    if (Pickup.IsEmpty() && WardContact98::AimedCart(*Mode->BookWorld, *Mode->Hero()) != INDEX_NONE) return false;
    const FString Target=Pickup.IsEmpty()?NearbyStoryTarget(*this):Pickup;
    if (Target.IsEmpty()) return false;
    const bool Key = String(Definitions.FindRef(Target), TEXT("kind")) == TEXT("key") && !Mode->Completed.Contains(Target);
    auto* Item = Key ? Props.FindRef(Target) : nullptr;
    const bool Used = UseId(Target);
    if (Used && IsValid(Item) && Mode->Completed.Contains(Target) && Mode->Hero()) WardContact98::KeyPickup(*Mode->Hero(), *Item);
    return Used;
}
bool AEWardHospitalGameplay::UseId(const FString& Id)
{
    if(WardMonster59::UseControl(*this,Id))return true;
    const Json Definition=Definitions.FindRef(Id);
    if(String(Definition,TEXT("kind"))==TEXT("node"))
    {
        if(Mode->Completed.Contains(Id)||Chapter()<Number(Definition,TEXT("chapter"),1))return false;
        Mode->Completed.Add(Id);
        Mode->Notify(TEXT("Узел отключён. Сигнал в этом участке ослаблен."));
        Mode->Cue(TEXT("metal_hit"),Mode->Hero()->GetActorLocation(),.25f);
        Mode->BroadcastNoise(Mode->Hero()->GetActorLocation(),1100);
        Refresh();
        Mode->AutoSave();
        return true;
    }
    if(NarrativeEnabled35&&NarrativeUse35(Id))return true;
    Json P=Definitions.FindRef(Id);if(!P||Mode->Completed.Contains(Id)||Chapter()<Number(P,TEXT("chapter"),1))return false;
    const FString Kind=String(P,TEXT("kind"));bool Taken=true;
    if(Kind==TEXT("scene")&&ArrivalStage>=0&&ArrivalStage<5){Mode->Notify(ArrivalObjective(),4);return true;}
    if(Kind==TEXT("scene"))
    {
        if(Chapter()>19)return false;
        Json C=Array(Data,TEXT("chapters"))[Chapter()-1]->AsObject();const auto& Steps=Array(C,TEXT("required"));int Step=Mode->Values.FindRef(TEXT("hospital_step"));
        if(!Steps.IsValidIndex(Step)||Steps[Step]->AsString()!=Id){Mode->Notify(Objective());return true;}
        Mode->Completed.Add(Id);
        if(Id==TEXT("SCENE_01_03")){Mode->Notify(TEXT("Врач по связи: проверь питание нижнего блока. На фотографии сотрудников — твоя сестра. Дата на обороте: вчера."),9);Mode->Journal.AddUnique(TEXT("На фото сотрудников больницы — сестра, пропавшая три года назад. На обороте вчерашняя дата."));Mode->Cue(TEXT("radio"),Mode->Hero()->GetActorLocation(),.22f);}
        else if(Id==TEXT("SCENE_01_04")){Mode->Notify(TEXT("Из темноты: «Она тебя ждёт… Только сначала посмотри на её руки». Мужчину утаскивают за дверь. На полу остаётся пропуск."),9);Mode->Flag(TEXT("badge"),true);Mode->Cue(TEXT("drag"),Mode->Hero()->GetActorLocation()+FVector(350,0,0),.35f);}
        else if(Id==TEXT("SCENE_06_03")&&Mode->Flag(TEXT("old_lab_accident29"))){Mode->Notify(TEXT("Бокс повреждён после давней аварии. За стеклом движется заражённая ткань. Не открывай бокс; ищи ручное отключение вентиляции."),8);Mode->Cue(TEXT("isolation_pipe"),Vector(Array(ArrivalData,TEXT("lab_at")))*100,.35f);}
        else Mode->Notify(TEXT("Событие: ")+String(P,TEXT("label")),6);
        if(++Step>=Steps.Num()){SetChapter(Chapter()+1);Mode->AutoSave();}else Mode->Values.Add(TEXT("hospital_step"),Step);
    }
    else if(Kind==TEXT("note")){Mode->Journal.AddUnique(TEXT("@blank:")+Id);Mode->Notify(TEXT("Найдена пустая записка · J — документы"));}
    else if(Kind==TEXT("weapon")){Taken=Mode->CanTake(TEXT("weapon"));if(Taken){Mode->Flag(TEXT("weapon"),true);Mode->Hero()->Rounds=9;if(Mode->CanTake(TEXT("ammo"),18))Mode->Hero()->Reserve+=18;Mode->Hero()->Pistol->SetVisibility(true);}}
    else if(Kind==TEXT("shotgun")){Taken=Mode->CanTake(TEXT("shotgun"));if(Taken){Mode->Flag(TEXT("shotgun"),true);Mode->LoadedShells=5;const int Extra=FMath::Min(10,18-Mode->Items.FindRef(TEXT("shells")));if(Extra>0&&Mode->CanTake(TEXT("shells"),Extra))Mode->Items.FindOrAdd(TEXT("shells"))+=Extra;Mode->Notify(TEXT("Дробовик найден · Z — сменить оружие"));}}
    else if(Kind==TEXT("map"))Mode->Flag(TEXT("map"),true);
    else if(Kind==TEXT("key"))
    {
        const FString Supply=Id==TEXT("spare_fuse")?TEXT("fuse"):Id==TEXT("service_prybar")?TEXT("residence_prybar"):Id==TEXT("medical_clearance")?TEXT("isolation_tag"):TEXT("badge");
        Taken=Mode->Items.FindRef(Supply)>0||Mode->Take(Supply);if(Taken){Mode->Flag(TEXT("hospital_key_")+Id,true);if(Supply==TEXT("badge"))Mode->Flag(TEXT("badge"),true);}
    }
    else if(Kind==TEXT("person")){Mode->Notify(TEXT("Собеседник: ")+String(P,TEXT("label")));}
    else Taken=Mode->Take(Kind,WardOnline80::SupplyAmount(*Mode,Id,Number(P,TEXT("amount"),1)));
    if(Taken){Mode->Completed.Add(Id);Refresh();Mode->Cue(TEXT("reload"),Mode->Hero()->GetActorLocation(),.2f);}else Mode->Notify(TEXT("В инвентаре недостаточно места. Освободи ячейку и подбери предмет снова."));return true;
}
FString AEWardHospitalGameplay::DocumentTitle(const FString& Entry)const
{
    const bool Ref=Entry.StartsWith(TEXT("@doc:"))||Entry.StartsWith(TEXT("@blank:"));const FString Id=Entry.RightChop(Entry.StartsWith(TEXT("@doc:"))?5:7);
    if(Ref){Json Doc=Object(Object(Narrative35,TEXT("documents")),Id);if(Doc->HasField(TEXT("title")))return String(Doc,TEXT("title"));Json P=Definitions.FindRef(Id);if(P)return String(P,TEXT("label"));}
    return Entry;
}
FString AEWardHospitalGameplay::DocumentBody(const FString& Entry)const
{
    if(Entry.StartsWith(TEXT("@doc:"))||Entry.StartsWith(TEXT("@blank:"))){const FString Id=Entry.RightChop(Entry.StartsWith(TEXT("@doc:"))?5:7);Json Doc=Object(Object(Narrative35,TEXT("documents")),Id);if(Doc->HasField(TEXT("text")))return String(Doc,TEXT("author"))+TEXT("\n\n")+String(Doc,TEXT("text"));Json P=Definitions.FindRef(Id);return P?String(P,TEXT("body")):FString();}
    return Entry;
}
void AEWardHospitalGameplay::Tick(float D)
{
    Super::Tick(D);
    if (!Mode) return;
    if (WardOnline80::Guest(*Mode))
    {
        TickCoopArrival29(D);
        return;
    }
    WardDialogue62::Tick(*this,D);
    if(HomeStage52>=0){TickHome52(D);return;}
    if(NarrativeEnabled35)TickNarrative35(D);
    if(!Mode->Screen.IsEmpty()&&!WardOnline80::PlayingMap(*Mode))return;
    TickArrival(D);
    if(ArrivalStage>=0&&ArrivalStage<5)return;
    HintClock-=D;
    if(HintClock<=0){HintClock=1;RefreshNavigation();const FString Hint=Prompt();if(!Hint.IsEmpty()&&Mode->Hero()->NoticeTime<=0)Mode->Notify(Hint,1.1f);}
}
void AEWardHospitalGameplay::Hear(FVector At,float Radius){for(auto M:Mobs)if(M->Active&&!M->Dead)M->Hear(At,Radius);}
bool AEWardHospitalGameplay::NodesNear(FVector At,float Radius)const
{
    for(const auto& E:Definitions)if(String(E.Value,TEXT("kind"))==TEXT("node")&&Chapter()>=Number(E.Value,TEXT("chapter"),1)&&!Mode->Completed.Contains(E.Key)&&FVector::Dist(Vector(Array(E.Value,TEXT("at")))*100,At)<Radius)return true;return false;
}
FIntVector AEWardHospitalGameplay::CellAt(FVector P)const{return FIntVector(FMath::FloorToInt((P.X/100+181)/.5),FMath::FloorToInt((P.Y/100+191)/.5),FMath::RoundToInt((P.Z-88)/480)*480);}
FVector AEWardHospitalGameplay::WorldAt(FIntVector C)const{return FVector((-181+(C.X+.5)*.5)*100,(-191+(C.Y+.5)*.5)*100,C.Z+91);}
bool AEWardHospitalGameplay::FloorCell(FIntVector C,float Radius)const
{
    auto Free=[&](int X,int Y){const auto* Rows=NavRows.Find(FIntPoint(C.Z,Y));if(!Rows)return false;for(auto R:*Rows)if(X>=R.X&&X<R.X+R.Y)return true;return false;};
    if(!Free(C.X,C.Y))return false;if(Radius>40&&(!Free(C.X-1,C.Y)||!Free(C.X+1,C.Y)||!Free(C.X,C.Y-1)||!Free(C.X,C.Y+1)))return false;
    return !DoorBlocks.Contains(C);
}
void AEWardHospitalGameplay::RefreshNavigation()
{
    DoorBlocks.Reset();for(const auto& P:Mode->BookWorld->Portals)if(!P.Open)
    {const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");const FIntVector C=CellAt(P.At+FVector(0,0,88));const int Along=FMath::CeilToInt((P.Width/2+55)/50);
        for(int I=-Along;I<=Along;++I)for(int J=-1;J<=1;++J)DoorBlocks.Add(C+(NS?FIntVector(I,J,0):FIntVector(J,I,0)));}
}
TArray<FVector> AEWardHospitalGameplay::FindPath(FVector From,FVector To,float Radius)const
{
    const_cast<AEWardHospitalGameplay*>(this)->RefreshNavigation();
    TArray<FVector> Result;FIntVector A=CellAt(From),B=CellAt(To);if(A.Z!=B.Z)return Result;
    auto Nearest=[&](FIntVector C){if(FloorCell(C,Radius))return C;for(int R=1;R<5;++R)for(int X=-R;X<=R;++X)for(int Y=-R;Y<=R;++Y)if(FloorCell(C+FIntVector(X,Y,0),Radius))return C+FIntVector(X,Y,0);return C;};A=Nearest(A);B=Nearest(B);
    if(!FloorCell(A,Radius)||!FloorCell(B,Radius))return Result;
    TArray<FIntVector> Queue;TMap<FIntVector,FIntVector> Parent;Queue.Add(A);Parent.Add(A,A);int Head=0;bool Found=false;
    while(Head<Queue.Num()&&Head<9000)
    {FIntVector C=Queue[Head++];if(C==B){Found=true;break;}for(FIntVector D:{FIntVector(1,0,0),FIntVector(-1,0,0),FIntVector(0,1,0),FIntVector(0,-1,0)}){FIntVector N=C+D;if(Parent.Contains(N)||FMath::Abs(N.X-A.X)>70||FMath::Abs(N.Y-A.Y)>70||!FloorCell(N,Radius))continue;Parent.Add(N,C);Queue.Add(N);}}
    if(!Found)return Result;TArray<FIntVector> Reverse;for(FIntVector C=B;C!=A;C=Parent[C])Reverse.Add(C);
    FIntVector LastDir(0,0,0);for(int I=Reverse.Num()-1;I>=0;--I){FIntVector Dir=I?Reverse[I-1]-Reverse[I]:FIntVector(0,0,0);if(Dir!=LastDir||I==0)Result.Add(WorldAt(Reverse[I]));LastDir=Dir;}return Result;
}
AEWardHospitalMob* AEWardHospitalGameplay::SpawnChild(FVector Near,const FString& Species)
{
    if(SpawnedChildren>=24)return nullptr;
    for(int I=0;I<12;++I){FIntVector C=CellAt(Near)+FIntVector(I%4-2,I/4-1,0);if(!FloorCell(C))continue;FVector At=WorldAt(C);if(FVector::Dist(At,Mode->Hero()->GetActorLocation())<130)continue;FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(ChildSpawn),false);if(GetWorld()->OverlapBlockingTestByChannel(At,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(25,80),Q))continue;
        Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),TEXT("CHILD_")+FString::FromInt(++SpawnedChildren));Row->SetStringField(TEXT("species"),Species);Row->SetStringField(TEXT("name"),FString::Printf(TEXT("Құрт-%02d"),SpawnedChildren));Row->SetStringField(TEXT("combat_class"),TEXT("A"));Row->SetNumberField(TEXT("chapter"),Chapter());Row->SetNumberField(TEXT("radius"),.25);Row->SetNumberField(TEXT("health"),25);Row->SetNumberField(TEXT("speed"),2);Row->SetNumberField(TEXT("damage"),7);Row->SetArrayField(TEXT("at"),Vec((At-FVector(0,0,91))/100));auto* M=SpawnMob(Row);if(!M)return nullptr;M->Active=true;M->EnsureVisual61();M->SetActorHiddenInGame(false);M->SetActorEnableCollision(true);M->GetCharacterMovement()->SetMovementMode(MOVE_Walking);M->Hear(Near,3000);return M;}
    return nullptr;
}
Json AEWardHospitalGameplay::Snapshot()const{Json S=MakeShared<FJsonObject>();S->SetNumberField(TEXT("version"),35);if(NarrativeEnabled35)S->SetObjectField(TEXT("scenario35"),NarrativeSnapshot35());TArray<TSharedPtr<FJsonValue>> Rows;for(auto M:Mobs)Rows.Add(MakeShared<FJsonValueObject>(M->Snapshot()));S->SetArrayField(TEXT("mobs"),Rows);S->SetObjectField(TEXT("arrival29"),ArrivalSnapshot());return S;}
void AEWardHospitalGameplay::Restore(Json S)
{
    for(const auto& Pair:Definitions) if(Boolean(Pair.Value,TEXT("past57"))) Pair.Value->SetBoolField(TEXT("memory_active57"),false);
    RestoreNarrative35(Object(S,TEXT("scenario35")));
    RestoreArrival(Object(S,TEXT("arrival29")));
    WardPeople58::Restore(*this,Object(Object(S,TEXT("scenario35")),TEXT("humans58")));
    // Campaign Restore has already restored the architecture and chapter values.
    // Calling SetChapter here would reset all saved door positions and locks.
    if(!Mode->Values.Contains(TEXT("hospital_chapter")))Mode->Values.Add(TEXT("hospital_chapter"),1);
    TSet<FString> Critical;for(auto C:Array(Data,TEXT("chapters")))for(auto Z:Array(C->AsObject(),TEXT("anchors")))Critical.Add(Z->AsString());
    for(auto& P:Mode->BookWorld->Portals)if(Critical.Contains(P.Zone)&&!Mode->BookWorld->RefugeBolts.Contains(P.Zone)){P.Locked=false;P.InsideOnly=false;}
    int RestoredChildren=0;
    for(auto V:Array(S,TEXT("mobs")))
    {
        Json Row=V->AsObject();const FString Id=String(Row,TEXT("id"));AEWardHospitalMob* Found=nullptr;for(auto M:Mobs)if(M->Id==Id){Found=M;break;}
        if(!Found&&Id.StartsWith(TEXT("CHILD_"))&&RestoredChildren<24&&Row->HasField(TEXT("definition")))
        {Json Def=Object(Row,TEXT("definition"));Found=SpawnMob(Def);SpawnedChildren=FMath::Max(SpawnedChildren,FCString::Atoi(*Id.RightChop(6)));}
        if(Found){Found->Restore(Row);if(Id.StartsWith(TEXT("CHILD_")))++RestoredChildren;}
    }
    Refresh();ApplyArrivalState();
}

AEWardHospitalMob* AEWardHospitalGameplay::SpawnMob(Json Row)
{
    FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FVector At=Vector(Array(Row,TEXT("at")))*100+FVector(0,0,91);
    auto* M=GetWorld()->SpawnActor<AEWardHospitalMob>(AEWardHospitalMob::StaticClass(),FTransform(At),P);
    if(M){M->Build(this,Row);Mobs.Add(M);}return M;
}



#include "WardDialogue62.inl"

void WardOnline80::ApplyDialogue(AEWardHospitalGameplay& Story, const FString& Line)
{
    if (Line == Story.DialogueKey62) return;
    WardDialogue62::Stop(Story);
    if (!Line.IsEmpty()) WardDialogue62::Play(Story, Line);
}
#include "WardDialogueAssessment62.inl"

