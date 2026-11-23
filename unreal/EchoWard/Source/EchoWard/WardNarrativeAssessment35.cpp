#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardDialogue62.h"
#include "WardBookWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
using namespace WardData;
void AEWardHospitalGameplay::AssessNarrative35(float Delta)
{
    static int Stage=0;static float Clock=0;static TArray<TSharedPtr<FJsonValue>> Checks;
    auto Check=[&](const FString& Name,bool Pass){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));UE_LOG(LogTemp,Display,TEXT("SCENARIO35_CHECK %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
    Clock+=Delta;
    if(Stage==0)
    {
        Check(TEXT("kazakh_names_survive_legacy_journal_display"),Mode->Tr(TEXT("Алексей и Ирина; Виктор Рудин")).Contains(TEXT("Бека и Аружан; Файзулаев Талғат")));
        Check(TEXT("kazakh_full_name_cases_keep_correct_family_members"),DisplayNames36(TEXT("Алексея Соловьёва / Ирины Соловьёвой / Егора Морозова / Алёны Морозовой"))==TEXT("Маулеш Беки / Құрбанәлі Аружан / Шаймердинов Аргына / Жаксымбетова Акботы"));
        Check(TEXT("name_substitution_does_not_change_ids_or_word_fragments"),DisplayNames36(TEXT("IRINA_ALIVE SCENE_01_04 НЕВиктор"))==TEXT("IRINA_ALIVE SCENE_01_04 НЕВиктор"));
        Check(TEXT("runtime_has_28_exact_user_names_and_extra_kazakh_cast"),Number(Narrative35,TEXT("names_revision"))==36&&Array(Object(Narrative35,TEXT("name_registry")),TEXT("people")).Num()==36&&String(Definitions[TEXT("PERSON_3")],TEXT("label"))==TEXT("Құрбанәлі Аружан"));
        NarrativeTesting35=true;FreezeAI=true;Mode->NewGame(TEXT("normal"));Mode->Show(TEXT(""));ArrivalStage=-1;Mode->CampaignStarted=true;
        Check(TEXT("runtime_data_is_pdf_v2_with_all_19_chapters"),NarrativeEnabled35&&Array(Narrative35,TEXT("chapters")).Num()==19);
        Check(TEXT("all_103_documents_have_real_text"),Object(Narrative35,TEXT("documents"))->Values.Num()==103);bool Bodies=true;for(const auto& E:Object(Narrative35,TEXT("documents"))->Values)Bodies&=!String(E.Value->AsObject(),TEXT("text")).IsEmpty();Check(TEXT("no_empty_note_body"),Bodies);
        int Required=0;for(auto C:Array(Data,TEXT("chapters")))Required+=Array(C->AsObject(),TEXT("required")).Num();Check(TEXT("78_bindings_76_required_events_exclude_alternative_airlock_branches"),Object(Narrative35,TEXT("scenes"))->Values.Num()==78&&Required==76);
        Check(TEXT("all_23_authored_alternatives_are_available_in_runtime"),Array(Narrative35,TEXT("alternatives")).Num()==23);
        int MonsterControls61=0;for(const auto& E:Definitions)if(String(E.Value,TEXT("kind"))==TEXT("monster_control59"))++MonsterControls61;
        Check(TEXT("real_props_include_new_items_and_named_npcs"),Definitions.Num()-MonsterControls61==471&&Props.Num()-MonsterControls61==471&&MonsterControls61==2&&Definitions.Contains(TEXT("NEW_NPC_MARINA"))&&Definitions.Contains(TEXT("NEW_UT_LEVER")));
        // Reproduce the evacuation follow-up: completed, current and future
        // scene markers share one room. Only the required scene is offered.
        const FVector TestAt74(-72000,-72000,0);
        auto* Floor74=GetWorld()->SpawnActor<AActor>();auto* FloorBox74=NewObject<UBoxComponent>(Floor74);
        Floor74->SetRootComponent(FloorBox74);FloorBox74->SetBoxExtent(FVector(400,400,10));FloorBox74->SetCollisionProfileName(TEXT("BlockAll"));FloorBox74->RegisterComponent();Floor74->SetActorLocation(TestAt74-FVector(0,0,10));
        TMap<FString,FVector> BeforeMarkers74;for(const TCHAR* Id:{TEXT("SCENE_16_01"),TEXT("SCENE_19_04"),TEXT("SCENE_19_05")})BeforeMarkers74.Add(Id,Props[Id]->GetActorLocation());
        SetChapter(19);Mode->Values.Add(TEXT("hospital_step"),3);
        for(const TCHAR* Id:{TEXT("SCENE_16_01"),TEXT("SCENE_19_01"),TEXT("SCENE_19_02"),TEXT("SCENE_19_03")})Mode->Completed.Add(Id);
        for(const TCHAR* Id:{TEXT("SCENE_16_01"),TEXT("SCENE_19_04"),TEXT("SCENE_19_05")}){Props[Id]->SetActorLocation(TestAt74);Props[Id]->SetActorHiddenInGame(false);Props[Id]->SetActorEnableCollision(true);}
        auto* Hero74=Mode->Hero();Hero74->SetActorLocation(TestAt74+FVector(0,-110,98));
        const FString Wanted74=TEXT("F — ")+String(Definitions.FindRef(TEXT("SCENE_19_04")),TEXT("label"));
        Check(TEXT("colocated_markers_offer_required_scene_not_future_epilogue"),Prompt()==Wanted74);
        Interact();Check(TEXT("colocated_completed_marker_does_not_block_required_interaction"),ActiveScene35==TEXT("SCENE_19_04"));
        ActiveScene35.Empty();SelectedChoice35.Empty();
        auto* Wall74=GetWorld()->SpawnActor<AActor>();auto* WallBox74=NewObject<UBoxComponent>(Wall74);
        Wall74->SetRootComponent(WallBox74);WallBox74->SetBoxExtent(FVector(100,8,140));WallBox74->SetCollisionProfileName(TEXT("BlockAll"));WallBox74->RegisterComponent();Wall74->SetActorLocation(TestAt74+FVector(0,-55,140));
        Interact();Check(TEXT("physical_wall_still_blocks_colocated_scene_interaction"),Prompt()!=Wanted74&&ActiveScene35.IsEmpty());
        for(const auto& Entry:BeforeMarkers74)Props[Entry.Key]->SetActorLocation(Entry.Value);
        Wall74->Destroy();Floor74->Destroy();Mode->NewGame(TEXT("normal"));Mode->Show(TEXT(""));ArrivalStage=-1;HomeStage52=-1;NarrativeTesting35=true;FreezeAI=true;
        UseId(TEXT("NOTE_01_01"));Check(TEXT("collected_note_readable_and_only_added_once"),Mode->Journal.Contains(TEXT("@doc:NOTE_01_01"))&&DocumentBody(TEXT("@doc:NOTE_01_01")).Contains(TEXT("Талғат")));const int J=Mode->Journal.Num();UseId(TEXT("NOTE_01_01"));Check(TEXT("note_pickup_does_not_duplicate"),Mode->Journal.Num()==J);
        SetChapter(1);const int StepBefore=Mode->Values.FindRef(TEXT("hospital_step"));BeginNarrative35(TEXT("SCENE_01_03"));Check(TEXT("future_scene_cannot_skip_current_objective"),ActiveScene35.IsEmpty()&&Mode->Values.FindRef(TEXT("hospital_step"))==StepBefore);
        Mode->Values.Add(TEXT("hospital_step"),2);BeginNarrative35(TEXT("SCENE_01_03"));Check(TEXT("power_scene_requires_actual_pickups"),ActiveScene35.IsEmpty());
        for(const TCHAR* Id:{TEXT("visitor_pass"),TEXT("first_pistol"),TEXT("spare_fuse")})UseId(Id);
        BeginNarrative35(TEXT("SCENE_01_03"));Check(TEXT("required_pickups_unlock_power_choice"),ActiveScene35==TEXT("SCENE_01_03"));NarrativeCommand35(TEXT("story35_next"));Check(TEXT("cannot_silently_choose_power_branch"),SelectedChoice35.IsEmpty()&&!Mode->Completed.Contains(TEXT("SCENE_01_03")));
        NarrativeCommand35(TEXT("story35_choice_LOCAL"));
        Mode->Show(TEXT("story35"));
        WardDialogue62::Tick(*this, .4f);
        WardDialogue62::Tick(*this, 60.f);
        const FString PendingLine = DialogueKey62;
        const int Cursor = DialogueIndex62;
        Json Saved=NarrativeSnapshot35();Write(FPaths::ProjectSavedDir()/TEXT("Prototype/scenario35_roundtrip.json"),Saved);
        ResetNarrative35();RestoreNarrative35(Read(FPaths::ProjectSavedDir()/TEXT("Prototype/scenario35_roundtrip.json")));
        WardDialogue62::Tick(*this, .4f);
        Check(TEXT("save_restores_current_subtitle_and_uncommitted_choice"), ActiveScene35==TEXT("SCENE_01_03") &&
            SelectedChoice35==TEXT("LOCAL") && Cursor>0 && DialogueIndex62==Cursor &&
            !PendingLine.IsEmpty() && DialogueKey62==PendingLine && !NarrativeFlag35(TEXT("POWER_LOCAL")));
        Mode->Show(TEXT(""));
        for(int I=0;I<80&&!ActiveScene35.IsEmpty();++I)NarrativeCommand35(TEXT("story35_next"));Check(TEXT("choice_effect_commits_once_after_conversation"),NarrativeFlag35(TEXT("POWER_LOCAL"))&&Mode->Completed.Contains(TEXT("SCENE_01_03")));
        SetChapter(10);Mode->Values.Add(TEXT("hospital_step"),1);ActiveScene35.Empty();BeginNarrative35(TEXT("SCENE_10_02"));NarrativeCommand35(TEXT("story35_choice_OPEN"));for(int I=0;I<80&&!ActiveScene35.IsEmpty();++I)NarrativeCommand35(TEXT("story35_next"));
        Check(TEXT("open_airlock_has_consequence_but_does_not_kill_irina"),NarrativeFlag35(TEXT("RECEIVER_DAMAGED"))&&NarrativeFlag35(TEXT("IMPOSTOR_RELEASED"))&&NarrativeFlag35(TEXT("IRINA_ALIVE")));
        Check(TEXT("airlock_truth_is_fixed"),NarrativeFlag35(TEXT("GLASS_IS_IMPOSTOR"))&&NarrativeFlag35(TEXT("RADIO_IS_LIVE_IRINA")));
        Check(TEXT("mutually_exclusive_radio_scene_is_skipped"),String(Object(NarrativeState35,TEXT("resolved")),TEXT("SCENE_10_04"))==TEXT("SKIPPED_BY_BRANCH"));
        Mode->NewGame(TEXT("normal"));Mode->Show(TEXT(""));ArrivalStage=-1;NarrativeTesting35=true;FreezeAI=true;
        bool Chain=true;int Count=0;
        while(Chapter()<=19&&Count<90)
        {
            const auto& Steps=Array(Array(Data,TEXT("chapters"))[Chapter()-1]->AsObject(),TEXT("required"));const int Step=Mode->Values.FindRef(TEXT("hospital_step"));if(!Steps.IsValidIndex(Step)){Chain=false;break;}const FString Id=Steps[Step]->AsString();Json Scene=NarrativeScenes35.FindRef(Id);
            for(auto V:Array(Scene,TEXT("requires")))UseId(V->AsString());
            if(Id==TEXT("SCENE_16_03")){for(const TCHAR* Doc:{TEXT("NOTE_07_01"),TEXT("NOTE_07_04"),TEXT("NOTE_08_04"),TEXT("NOTE_04_02"),TEXT("NOTE_14_02"),TEXT("NOTE_13_02"),TEXT("NOTE_13_03")})UseId(Doc);PreparePlans35();}
            if(!BeginNarrative35(Id)||ActiveScene35!=Id){Chain=false;UE_LOG(LogTemp,Error,TEXT("SCENARIO35_STUCK %s"),*Id);break;}
            
            const auto& Opts=Array(Scene,TEXT("choices"));if(Opts.Num())NarrativeCommand35(TEXT("story35_choice_")+String(Opts[0]->AsObject(),TEXT("id")));
            for(int I=0;I<100&&!ActiveScene35.IsEmpty();++I)NarrativeCommand35(TEXT("story35_next"));
            if(!Mode->Completed.Contains(Id)||!ActiveScene35.IsEmpty()){Chain=false;UE_LOG(LogTemp,Error,TEXT("SCENARIO35_DID_NOT_FINISH %s"),*Id);break;}++Count;
        }
        Check(TEXT("all_19_chapters_reach_epilogue_via_76_interactive_events"),Chain&&Chapter()==20&&Count==76&&NarrativeFlag35(TEXT("EPILOGUE_COMPLETE")));
        Check(TEXT("cure_plan_prepared_from_actual_required_state"),NarrativeFlag35(TEXT("PLAN_READY_CURE"))&&NarrativeFlag35(TEXT("CLEAN_EXIT")));
        Check(TEXT("memory_preservation_and_truth_are_independent_flags"),NarrativeFlag35(TEXT("MEMORY_SPARED"))&&NarrativeFlag35(TEXT("TRUTH_READY")));
        // Exercise every authored decision and its concrete effects independently.
        bool Choices=true;int Branches=0;
        for(const auto& Pair:NarrativeScenes35)
        {
            if (Pair.Key == TEXT("CHECKPOINT_69")) continue; // Exercised on the physical arrival route.
            Json Scene=Pair.Value;for(auto O:Array(Scene,TEXT("choices")))
            {
                Json Opt=O->AsObject();ActiveScene35.Empty();ResetNarrative35();SetChapter(Number(Scene,TEXT("chapter")));const auto& Steps=Array(Array(Data,TEXT("chapters"))[Chapter()-1]->AsObject(),TEXT("required"));int Index=0;for(int I=0;I<Steps.Num();++I)if(Steps[I]->AsString()==Pair.Key)Index=I;Mode->Values.Add(TEXT("hospital_step"),Index);Mode->Completed.Remove(Pair.Key);
                for(auto Q:Array(Scene,TEXT("requires")))Mode->Completed.Add(Q->AsString());for(auto Q:Array(Opt,TEXT("requires")))SetNarrativeFlag35(Q->AsString(),true);
                if(String(Opt,TEXT("id"))==TEXT("STAY"))Object(NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("DESTROY"));
                BeginNarrative35(Pair.Key);NarrativeCommand35(TEXT("story35_choice_")+String(Opt,TEXT("id")));for(int I=0;I<150&&!ActiveScene35.IsEmpty();++I)NarrativeCommand35(TEXT("story35_next"));
                bool Passed=Mode->Completed.Contains(Pair.Key)&&String(Object(NarrativeState35,TEXT("choices")),String(Scene,TEXT("choice_key")))==String(Opt,TEXT("id"));for(const auto& E:Object(Opt,TEXT("effects"))->Values)if(E.Value->Type==EJson::Boolean)Passed&=NarrativeFlag35(FString(E.Key))==E.Value->AsBool();Choices&=Passed;++Branches;
            }
        }
        Check(TEXT("every_authored_interactive_choice_commits_its_own_effects"),Choices&&Branches>=40);
        Mode->NewGame(TEXT("normal"));ArrivalStage=-1;NarrativeTesting35=true;Mode->Show(TEXT(""));SetChapter(13);
        for(int Side=1;Side<=6;++Side)for(int I=1;I<=3;++I){UseId(FString::Printf(TEXT("SIDE_%d_%d"),Side,I));for(int K=0;K<150&&!ActiveScene35.IsEmpty();++K)NarrativeCommand35(TEXT("story35_next"));}
        bool Sides=true;for(int Side=1;Side<=6;++Side)Sides&=NarrativeFlag35(TEXT("SIDE_DONE_")+FString::FromInt(Side));Check(TEXT("six_optional_stories_complete_after_their_three_evidence_items"),Sides);
        SetNarrativeFlag35(TEXT("GLASS_IS_IMPOSTOR"),false);RestoreNarrative35(NarrativeSnapshot35());Check(TEXT("loading_never_randomizes_impostor_identity"),NarrativeFlag35(TEXT("GLASS_IS_IMPOSTOR")));
        RestoreNarrative35(MakeShared<FJsonObject>());Check(TEXT("legacy_save_migration_does_not_select_a_final_plan"),NarrativeFlag35(TEXT("LEGACY_SAVE"))&&String(Object(NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN")).IsEmpty());
        Mode->NewGame(TEXT("normal"));Mode->Show(TEXT(""));NarrativeTesting35=true;FreezeAI=true;StartArrival();ArrivalInteract();ArrivalInteract();
        Check(TEXT("actual_arrival_opens_full_first_dialogue_after_car_exit"),ArrivalStage==2&&ActiveScene35==TEXT("SCENE_01_01"));
        for(int I=0;I<80&&!ActiveScene35.IsEmpty();++I)NarrativeCommand35(TEXT("story35_next"));
        Check(TEXT("arrival_dialogue_does_not_skip_external_post"),ArrivalStage==2&&Mode->Completed.Contains(TEXT("SCENE_01_01"))&&!Mode->Completed.Contains(TEXT("SCENE_01_02")));
        ArrivalStage=-1;SetChapter(1);Refresh();bool Closed=true;for(const auto& E:NarrativeGates35)Closed&=E.Value->GetCollisionEnabled()!=ECollisionEnabled::NoCollision;
        Check(TEXT("early_chapter_quarantine_has_physical_blocking_volumes"),Closed&&NarrativeGates35.Num()==12);
        SetChapter(8);bool Released=true;for(const auto& E:NarrativeGates35)Released&=E.Value->GetCollisionEnabled()==ECollisionEnabled::NoCollision;Check(TEXT("later_chapters_release_all_main_vertical_routes"),Released);
        auto* Hero=Mode->Hero();Hero->SetActorLocation(FVector(5000,-4000,98),false);Mode->CampaignStarted=true;Mode->Invulnerable=100;
        SetNarrativeFlag35(TEXT("VOICE_SAMPLE"),true);SetNarrativeFlag35(TEXT("RECEIVER_DAMAGED"),true);Json FullSave=Mode->Capture();SetNarrativeFlag35(TEXT("VOICE_SAMPLE"),false);SetNarrativeFlag35(TEXT("RECEIVER_DAMAGED"),false);Mode->Restore(FullSave);
        Check(TEXT("campaign_save_preserves_narrative_facts_without_shared_aliases"),NarrativeFlag35(TEXT("VOICE_SAMPLE"))&&NarrativeFlag35(TEXT("RECEIVER_DAMAGED")));
        Object(NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("CURE"));Refresh();auto Exit=Props.FindRef(TEXT("SCENE_19_03"));Check(TEXT("cure_uses_ground_exit_instead_of_rooftop"),Exit&&FMath::Abs(Exit->GetActorLocation().Z)<1);
        Object(NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("EVAC"));Refresh();Check(TEXT("evac_branch_restores_actual_rooftop_exit"),Exit&&FMath::Abs(Exit->GetActorLocation().Z-1440)<1);
        ActiveScene35.Empty();SetChapter(1);BeginNarrative35(TEXT("SCENE_01_01"));NarrativeCommand35(TEXT("story35_close"));Check(TEXT("dialogue_can_be_suspended_for_exploration"),Mode->Screen.IsEmpty()&&NarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"))&&!ActiveScene35.IsEmpty());ActiveScene35.Empty();
        SetChapter(20);Json Spatial=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> SpatialRows;int Accessible=0,Inaccessible=0;
        for(const auto& E:Props)
        {
            FVector Standing;const bool Clear=FindNarrativeStanding35(E.Key,Standing);Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),E.Key);Row->SetBoolField(TEXT("standing_and_sight_clear"),Clear);
            const FVector At=E.Value->GetActorLocation()/100;Row->SetArrayField(TEXT("at_m"),{MakeShared<FJsonValueNumber>(At.X),MakeShared<FJsonValueNumber>(At.Y),MakeShared<FJsonValueNumber>(At.Z)});
            if(Clear)++Accessible;else ++Inaccessible;SpatialRows.Add(MakeShared<FJsonValueObject>(Row));
        }
        Check(TEXT("every_scenario_object_has_clear_standing_position_and_sight_line"),Inaccessible==0);Spatial->SetNumberField(TEXT("accessible"),Accessible);Spatial->SetNumberField(TEXT("unresolved"),Inaccessible);Spatial->SetArrayField(TEXT("props"),SpatialRows);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/scenario35_spatial.json"),Spatial);
        ActiveScene35=TEXT("SCENE_01_03");SelectedChoice35=TEXT("FULL");DialoguePage35=0;
        Check(TEXT("early_power_choice_does_not_reveal_chapter_18_consequences"),!NarrativeBody35().Contains(TEXT("В главе 18")));
        ActiveScene35=TEXT("SCENE_19_02");SelectedChoice35=TEXT("BREACH");DialoguePage35=0;Object(NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("CURE"));SetNarrativeFlag35(TEXT("MEMORY_SPARED"),true);SetNarrativeFlag35(TEXT("OUTBREAK"),false);
        const FString BreachText=NarrativeBody35();Check(TEXT("selected_boundary_breach_displays_its_own_branch_before_commit"),!BreachText.IsEmpty()&&BreachText!=String(NarrativeScenes35[TEXT("SCENE_19_02")],TEXT("text")));
        ActiveScene35.Empty();SelectedChoice35.Empty();
        SetChapter(10);Mode->Values.Add(TEXT("hospital_step"),0);Check(TEXT("airlock_objective_names_actual_underground_location"),Objective().Contains(TEXT("UT /")));
        SetChapter(19);Mode->Values.Add(TEXT("hospital_step"),2);Object(NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("CURE"));Check(TEXT("cure_exit_objective_does_not_send_player_to_rooftop"),Objective().Contains(TEXT("Наземный выход")));
        ActiveScene35.Empty();SetChapter(1);Mode->BookWorld->LiftMoving=false;Mode->BookWorld->LiftFloor=2;
        Check(TEXT("lift_cannot_bypass_early_quarantine"),!Mode->BookWorld->RequestLift(0)&&!Mode->BookWorld->RequestLift(3));
        SetChapter(8);Check(TEXT("lift_releases_b2_after_medical_access_chapter"),Mode->BookWorld->RequestLift(0));Mode->BookWorld->LiftMoving=false;Mode->BookWorld->LiftTarget=Mode->BookWorld->LiftFloor;
        // Render through the real Slate UI using a representative long dialogue.
        ActiveScene35.Empty();SetChapter(10);Mode->Values.Add(TEXT("hospital_step"),1);BeginNarrative35(TEXT("SCENE_10_02"));NarrativeTesting35=false;Mode->Show(TEXT("story35"));
        Check(TEXT("choice_ui_stays_hidden_during_setup"),Mode->Interface.IsValid()&&!Mode->Interface->TitleActions.Contains(TEXT("story35_choice_VERIFY")));
        if(FParse::Param(FCommandLine::Get(),TEXT("WardScenarioPictures")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/scenario35_choices.png"),true,false);
        Stage=1;Clock=0;
    }
    else if(Stage==1&&Clock>.6)
    {
        if (!WardDialogue62::AwaitingChoice(*this) && Clock < 60.f) return;
        Check(TEXT("real_slate_choice_buttons_are_registered"),Mode->Interface.IsValid()&&Mode->Interface->TitleActions.Contains(TEXT("story35_choice_VERIFY"))&&Mode->Interface->TitleActions.Contains(TEXT("story35_choice_OPEN")));
        NarrativeCommand35(TEXT("story35_choice_VERIFY"));
        Check(TEXT("answer_selected_through_runtime_command"),SelectedChoice35==TEXT("VERIFY"));
        if(FParse::Param(FCommandLine::Get(),TEXT("WardScenarioPictures")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/scenario35_dialogue.png"),true,false);
        Stage=2;Clock=0;
    }
    else if(Stage==2&&Clock>.6)
    {
        bool OK=true;for(auto V:Checks)OK&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),OK);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);R->SetNumberField(TEXT("documents"),Object(Narrative35,TEXT("documents"))->Values.Num());Write(FPaths::ProjectSavedDir()/TEXT("Prototype/scenario35_check.json"),R);FPlatformMisc::RequestExit(false);Stage=3;
    }
}
