#include "WardPeopleAssessment54.h"
#include "WardDialogue62.h"
#include "WardPeople54.h"
#include "WardPeople57.h"
#include "WardBookWorld.h"
#include "WardCampaign.h"
#include "WardHospitalGameplay.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UnrealClient.h"

using namespace WardData;

void AssessPeople54(AEWardCampaignMode& Mode, float Delta)
{
    static FVector LiveStart57;
    static int32 Stage = 0;
    static float Clock = 0;
    static TArray<TSharedPtr<FJsonValue>> Checks;
    Clock += Stage >= 3 ? FApp::GetDeltaTime() : Delta;
    auto Check = [&](const FString& Name, bool Pass)
    {
        Json Result = MakeShared<FJsonObject>();
        Result->SetStringField(TEXT("name"), Name);
        Result->SetBoolField(TEXT("passed"), Pass);
        Checks.Add(MakeShared<FJsonValueObject>(Result));
        UE_LOG(LogTemp, Display, TEXT("PEOPLE54 %s %s"), Pass?TEXT("PASS"):TEXT("FAIL"), *Name);
    };
    auto* Story = Mode.HospitalStory.Get();
    if (Stage == 0 && Story && Mode.Hero())
    {
        Mode.Show(TEXT(""));
        Mode.TitleView = false;
        Mode.CampaignStarted=true;Story->HomeStage52=-1;Story->ArrivalStage=-1;
        Story->FreezeAI = true;
        TSet<FString> Models;
        TSet<FString> Lines;
        TSet<int32> Heights;
        int32 Count = 0;
        const int32 InitialChapter = Story->Chapter();
        Story->SetChapter(19);
        Story->NarrativeTesting35 = true;
        for (const auto& Pair : Story->Props)
        {
            if (String(Story->Definitions.FindRef(Pair.Key), TEXT("kind")) != TEXT("person")) continue;
            ++Count;
            auto* Visual = Pair.Value->FindComponentByClass<UEWardPerson54>();
            const bool Human = Pair.Key != TEXT("PERSON_1");
            const bool Loaded = Visual && Visual->LoadAppearance() && Visual->ClipCount() == (Human ? 6 : 5);
            Check(Pair.Key+TEXT("_clips_loaded"), Loaded);
            if (!Loaded) continue;
            Models.Add(Visual->ModelName());
            if (Human)
            {
                const FString Opening = UEWardPerson54::OpeningLine55(Pair.Key);
                if (!Opening.IsEmpty()) Lines.Add(Opening);
                Heights.Add(FMath::RoundToInt(Visual->TargetHeight55()));
                Story->ActiveScene35.Empty();
                const bool Opens = Story->NarrativeUse35(Pair.Key);
                Check(Pair.Key+TEXT("_personal_dialogue_and_voice"), Opens && !Opening.IsEmpty()
                    && Story->ActiveScene35 == TEXT("NPC_")+Pair.Key
                    && Story->NarrativeBody35().Contains(Opening)
                    && FPaths::FileExists(UEWardPerson54::VoicePath55(Pair.Key)));
            }
            bool Articulated = true;
            for (int32 Clip = 0; Clip < Visual->ClipCount(); ++Clip)
            {
                Visual->PresentPose(Clip, 0.f);
                TArray<TArray<FVector>> Start;
                for (int32 Part = 0; Part < Visual->GetNumSections(); ++Part)
                {
                    auto& Vertices = Start.AddDefaulted_GetRef();
                    for (const auto& Vertex : Visual->GetProcMeshSection(Part)->ProcVertexBuffer)
                        Vertices.Add(Vertex.Position);
                }
                Visual->PresentPose(Clip, Clip==5 ? Visual->HabitLength55()*.54f : Clip==4 ? 2.f : .65f);
                float Difference = 0;
                FBox Bounds(ForceInit);
                for (int32 Part = 0; Part < Start.Num(); ++Part)
                {
                    const auto& Vertices = Visual->GetProcMeshSection(Part)->ProcVertexBuffer;
                    for (int32 I = 0; I < Vertices.Num(); ++I)
                    {
                        const FVector At = Vertices[I].Position;
                        Difference = FMath::Max(Difference, float(FVector::Dist(Start[Part][I],At)));
                        Bounds += At;
                    }
                }
                Articulated &= Difference > .2f && Bounds.IsValid && !Bounds.GetSize().ContainsNaN()
                    && Bounds.GetSize().GetMax()<240 && Bounds.Min.Z>-8 && Bounds.Max.Z<230;
            }
            Check(Pair.Key+TEXT("_rig_moves_in_bounds"), Articulated);
        }
        Check(TEXT("all_36_world_actors_have_distinct_models"), Count==36 && Models.Num()==36);
        Check(TEXT("all_35_npcs_have_different_opening"), Lines.Num()==35 && Heights.Num()>=21);
        Story->NarrativeTesting35 = false;
        Story->SetChapter(InitialChapter);
        Story->ActiveScene35.Empty();
        const bool Opened = Story->NarrativeUse35(TEXT("PERSON_2"));
        WardDialogue62::Tick(*Story,.5f);WardDialogue62::Tick(*Story,.5f);
        Check(TEXT("talgat_voice_plays_in_dialogue"), Opened && WardDialogue62::Speaking(*Story,TEXT("PERSON_2")));
        Mode.Show(TEXT(""));
        Check(TEXT("voice_stops_when_dialogue_closes"), !WardDialogue62::Speaking(*Story,TEXT("PERSON_2")));
        Story->ActiveScene35.Empty();
        const FString ArchivePath = FPaths::ProjectContentDir()/TEXT("CampaignData/People56");
        const auto Cards = Read(ArchivePath/TEXT("archive.json"));
        TSet<int32> RegistryNumbers;
        TSet<FString> Lines56;
        if (Cards)
        {
            for (const auto& Entry : Cards->Values)
            {
                const FString ArchiveId(*Entry.Key);
                const Json Card = Entry.Value->AsObject();
                const auto& Registry = Array(Object(Story->Narrative35, TEXT("name_registry")), TEXT("people"));
                const int32 Number = int32(WardData::Number(Card, TEXT("number")));
                const FString Name = String(Card, TEXT("name"));
                if (Registry.IsValidIndex(Number-1) && Name == String(Registry[Number-1]->AsObject(), TEXT("name")))
                    RegistryNumbers.Add(Number);
                auto* Actor = Mode.GetWorld()->SpawnActor<AActor>();
                auto* Model = NewObject<UEWardPerson54>(Actor);
                Actor->SetRootComponent(Model);
                Model->SetIdentity(nullptr, ArchiveId, FString());
                Model->SetVisibleInSceneCaptureOnly(true);
                Model->RegisterComponent();
                const bool Loaded = Model->LoadAppearance();
                FBox Bounds(ForceInit);
                if (Loaded)
                    for (int32 Part=0; Part<Model->GetNumSections(); ++Part)
                        for (const auto& Vertex : Model->GetProcMeshSection(Part)->ProcVertexBuffer) Bounds += Vertex.Position;
                Check(ArchiveId+TEXT("_distinct_3d_model"), Loaded && Models.Contains(Model->ModelName())
                    && Model->ClipCount()==6 && Model->HabitLength55() > 0.f
                    && Bounds.IsValid && Bounds.Min.Z > -8.f && FMath::Abs(Bounds.Max.Z-Model->TargetHeight55()) < 20.f);
                Models.Add(Model->ModelName());
                const auto& Replies = Array(Card, TEXT("lines"));
                for (auto Line : Replies) Lines56.Add(Line->AsString());
                Check(ArchiveId+TEXT("_two_lines"), Replies.Num()==2 &&
                    FPaths::FileExists(ArchivePath/(ArchiveId+TEXT("_1.wav"))) &&
                    FPaths::FileExists(ArchivePath/(ArchiveId+TEXT("_2.wav"))));
                Actor->Destroy();
            }
        }
        Check(TEXT("archive_14_named_people_and_28_distinct_lines"), RegistryNumbers.Num()==14 && Lines56.Num()==28);
        Mode.CampaignStarted=true;
        Story->HomeStage52=-1;
        Story->ArrivalStage=-1;
        Story->SetChapter(19);
        Story->ActiveScene35.Empty();
        Story->NarrativeTesting35=true;
        const auto Walkers=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People57/placements.json"));
        int32 LiveCount=0,PastCount=0;
        for (auto Entry:Array(Walkers,TEXT("people")))
        {
            const Json Description=Entry->AsObject();
            const FString Id=String(Description,TEXT("id"));
            auto* Actor=Story->Props.FindRef(Id);
            auto* Model=Actor?Actor->FindComponentByClass<UEWardPerson54>():nullptr;
            const bool Past=Boolean(Description,TEXT("past57"));
            Check(Id+TEXT("_chronology"), Actor && (Past?Actor->IsHidden():!Actor->IsHidden()));
            if (!Model) continue;
            if(Past)++PastCount;else ++LiveCount;
            const FVector Anchor=Vector(Array(Description,TEXT("at")))*100;
            Mode.Hero()->SetActorLocation(Anchor+FVector(550,0,91));
            if(Past)
            {
                WardPeople57::Replay(Mode,Id);
                Story->NarrativeTesting35=true;
                Story->ActiveScene35.Empty();
            }
            const bool HasRoute=Model->ResetPatrol57();
            const FVector Start=Actor->GetActorLocation();
            Mode.Hero()->SetActorLocation(Start+FVector(650,650,91));
            double Travel=0;
            for(int32 Step=0;Step<240;++Step)
            {
                const FVector Before=Actor->GetActorLocation();
                Model->AdvancePatrol57(.05f);
                Travel+=FVector::Dist2D(Before,Actor->GetActorLocation());
            }
            Check(Id+TEXT("_walks_on_real_floor"),HasRoute && Model->PatrolPoints57()>1 && Travel>45 &&
                FMath::Abs(Actor->GetActorLocation().Z-Start.Z)<45.f);
            const FVector BeforeStop=Actor->GetActorLocation();
            Mode.Hero()->SetActorLocation(BeforeStop+FVector(80,0,91));
            Model->AdvancePatrol57(.1f);
            Check(Id+TEXT("_stops_for_player"),Actor->GetActorLocation().Equals(BeforeStop,.1));
            const auto Card=Object(Cards,Id);
            const auto& Replies=Array(Card,TEXT("lines"));
            auto D=Story->Definitions[Id];
            D->SetNumberField(TEXT("spoken57"),0);
            const auto CompletedBefore=Mode.Completed;
            bool Speech=Replies.Num()==2;
            for(int32 LineIndex=0;LineIndex<2;++LineIndex)
            {
                Story->ActiveScene35.Empty();
                Story->NarrativeTesting35=true;
                const bool Used=Story->NarrativeUse35(Id);
                const FString Body=Story->NarrativeBody35();
                Speech &= Used && Replies.IsValidIndex(LineIndex) && Body.Contains(Replies[LineIndex]->AsString())
                    && String(Story->NarrativeState35,TEXT("aux_voice57")).EndsWith(
                        Id+TEXT("_")+FString::FromInt(LineIndex+1)+TEXT(".wav"));
            }
            Check(Id+TEXT("_two_in_world_lines"),Speech && Mode.Completed.Num()==CompletedBefore.Num());
            Story->ActiveScene35.Empty();
            if(Past)
            {
                D->SetBoolField(TEXT("memory_active57"),false);
                Actor->SetActorHiddenInGame(true);
                Actor->SetActorEnableCollision(false);
            }
        }
        Check(TEXT("nine_living_and_five_past_performances"),LiveCount==9 && PastCount==5);
        Story->NarrativeTesting35=false;
        Story->SetChapter(1);
        Story->ActiveScene35.Empty();
        auto* PastActor=Story->Props.FindRef(TEXT("FILE_28"));
        if(PastActor)
        {
            Mode.Hero()->SetActorLocation(FVector(7500,-1700,91));
            WardPeople57::Replay(Mode,TEXT("FILE_28"));
            Story->Refresh();
            Story->NarrativeTesting35=true;
            Story->ActiveScene35.Empty();
            Check(TEXT("past_scene_can_play_before_its_story_chapter"),!PastActor->IsHidden() &&
                Story->NarrativeUse35(TEXT("FILE_28")) && Story->ActiveScene35==TEXT("NPC_FILE_28"));
            Story->ActiveScene35.Empty();
            Story->Definitions[TEXT("FILE_28")]->SetBoolField(TEXT("memory_active57"),false);
        }
        Story->NarrativeTesting35=false;
        Story->SetChapter(19);
        const FVector StageOrigin(105000,105000,1200);
        auto* Camera = Mode.GetWorld()->SpawnActor<ACameraActor>();
        Camera->SetActorLocationAndRotation(StageOrigin+FVector(500,0,115), FRotator(-2,180,0));
        Camera->GetCameraComponent()->SetFieldOfView(54);
        if (auto* PC = Cast<APlayerController>(Mode.Hero()->GetController()))
        {
            PC->SetViewTarget(Camera);
            if (auto* HUD = PC->GetHUD()) HUD->bShowHUD = false;
        }
        const TArray<FString> Cast = {TEXT("PERSON_2"),TEXT("PERSON_3"),TEXT("NEW_NPC_MARINA"),TEXT("PERSON_4"),TEXT("NEW_NPC_DEMIN")};
        for (int32 I=0; I<Cast.Num(); ++I)
        {
            auto* Actor = Story->Props.FindRef(Cast[I]);
            if (!Actor) continue;
            Actor->SetActorHiddenInGame(false);
            Actor->SetActorLocation(StageOrigin+FVector(0, (I-2)*92, 0));
            if (auto* Model = Actor->FindComponentByClass<UEWardPerson54>())
            {
                Model->SetComponentTickEnabled(false);
                Model->SetRelativeTransform(FTransform::Identity);
                Model->PresentPose(I==2 ? 4 : I==3 ? 3 : I==1 ? 2 : 0, I==2 ? 4.f : .4f);
            }
        }
        auto* Lighting = Mode.GetWorld()->SpawnActor<AActor>();
        auto* Root = NewObject<USceneComponent>(Lighting);
        Lighting->SetRootComponent(Root);
        Root->RegisterComponent();
        Lighting->SetActorLocation(StageOrigin);
        for (float Side : {-210.f, 210.f})
        {
            auto* Light = NewObject<UPointLightComponent>(Lighting);
            Light->SetupAttachment(Root);
            Light->SetRelativeLocation(FVector(250,Side,235));
            Light->SetIntensityUnits(ELightUnits::Lumens);
            Light->SetIntensity(3100);
            Light->SetAttenuationRadius(900);
            Light->SetCastShadows(false);
            Lighting->AddInstanceComponent(Light);
            Light->RegisterComponent();
        }
        Clock=0;
        Stage=1;
    }
    else if (Stage==1 && Clock>2.f)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/People57/lineup.png"),false,false);
        Clock=0;
        Stage=2;
    }
    else if (Stage==2 && Clock>.7f)
    {
        Mode.Show(TEXT("journal"));
        Mode.Command(TEXT("field_page_people56"));
        Check(TEXT("archive_opens_from_document_navigation"), Mode.Screen==TEXT("people56"));
        Clock=0;
        Stage=3;
    }
    else if (Stage==3 && Clock>1.6f)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/People57/archive.png"),true,false);
        Clock=0;
        Stage=4;
    }
    else if (Stage==4 && Clock>.6f)
    {
        Mode.Command(TEXT("field_page_journal"));
        Check(TEXT("archive_returns_to_documents"), Mode.Screen==TEXT("journal"));
        Mode.Show(TEXT(""));
        Story->ActiveScene35.Empty();
        auto* Walker=Story->Props.FindRef(TEXT("FILE_30"));
        if(Walker)
        {
            Walker->SetActorHiddenInGame(false);
            auto* Model=Walker->FindComponentByClass<UEWardPerson54>();
            Model->ResetPatrol57();
            const FVector At=Walker->GetActorLocation();
            auto* Camera=Mode.GetWorld()->SpawnActor<ACameraActor>();
            const FVector From=At+FVector(320,-210,140);
            Camera->SetActorLocationAndRotation(From,(At+FVector(0,0,95)-From).Rotation());
            Camera->GetCameraComponent()->SetFieldOfView(65);
            if(auto* PC=Cast<APlayerController>(Mode.Hero()->GetController()))PC->SetViewTarget(Camera);
            Mode.Hero()->SetActorLocation(At+FVector(700,500,91));
            LiveStart57=At;
            Model->SetComponentTickEnabled(true);
            Mode.BookWorld->SetBright(true);
            Mode.BookWorld->UpdateLighting(At);
            auto* Fill=NewObject<UPointLightComponent>(Camera);
            Fill->SetupAttachment(Camera->GetRootComponent());
            Fill->SetIntensityUnits(ELightUnits::Lumens);
            Fill->SetIntensity(1800);
            Fill->SetAttenuationRadius(1100);
            Fill->SetCastShadows(false);
            Fill->RegisterComponent();
        }
        Clock=0;Stage=5;
    }
    else if(Stage==5 && Clock>2.f)
    {
        if(auto* Walker=Story->Props.FindRef(TEXT("FILE_30")))
        {
            auto* Model=Walker->FindComponentByClass<UEWardPerson54>();
            Check(TEXT("quarantine_guard_walks_during_actual_game_ticks"),
                FVector::Dist2D(LiveStart57,Walker->GetActorLocation())>20.f);
        }
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/People57/walking.png"),false,false);
        Clock=0;Stage=6;
    }
    else if(Stage==6 && Clock>.7f)
    {
        bool Pass=true;
        for (const auto& Row:Checks) Pass &= Boolean(Row->AsObject(),TEXT("passed"));
        Json Result=MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("success"),Pass);
        Result->SetNumberField(TEXT("count"),Checks.Num());
        Result->SetArrayField(TEXT("checks"),Checks);
        Write(FPaths::ProjectSavedDir()/TEXT("Prototype/people57_check.json"),Result);
        Stage=7;
        FPlatformMisc::RequestExit(false);
    }
}





