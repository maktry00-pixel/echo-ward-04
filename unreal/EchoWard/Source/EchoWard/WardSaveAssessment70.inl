#include "WardPeople54.h"
#include "WardStaging64.h"
#include "Misc/FileHelper.h"

void WardSaveCheck70(AEWardCampaignMode& G,float)
{
    auto* Story=G.HospitalStory.Get();
    auto* World=G.BookWorld.Get();
    auto* Hero=G.Hero();
    if(!Story||!World||!Hero)return;
    static bool Ran=false;
    if(Ran)return;
    Ran=true;
    TArray<TSharedPtr<FJsonValue>> Checks;
    auto Check=[&Checks](const FString& Id,bool Passed)
    {
        Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Id);Row->SetBoolField(TEXT("passed"),Passed);
        Checks.Add(MakeShared<FJsonValueObject>(Row));
        UE_LOG(LogTemp,Display,TEXT("SAVE70 %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Id);
    };
    G.SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("Prototype/Save70")/FGuid::NewGuid().ToString();
    const FVector Safe(5000,-4000,98);
    auto Reset=[&]()
    {
        G.NewGame(TEXT("normal"));G.Show(TEXT(""));
        Story->NarrativeTesting35=true;Story->FreezeAI=true;G.Invulnerable=100;
        Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Hero->SetActorLocation(Safe,false);
    };
    Reset();
    Story->SetChapter(4);
    Story->ArrivalStage=5;Story->ApplyArrivalState();
    G.Values.Add(TEXT("hospital_step"),1);
    Story->UseId(TEXT("NOTE_01_01"));
    G.Items.Add(TEXT("medkit"),2);G.Stash.Add(TEXT("battery"),3);
    Hero->Health=61;Hero->Stamina=48;Hero->Rounds=4;Hero->Reserve=17;
    G.Flag(TEXT("weapon"),true);G.Flag(TEXT("old_lab_accident29"),true);
    G.Completed.Add(TEXT("SCENE_04_01"));
    Object(Story->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("POWER"),TEXT("LOCAL"));
    Story->SetNarrativeFlag35(TEXT("POWER_LOCAL"),true);
    const FString Goal=Story->Objective();
    auto* Drop=G.SpawnSupply(TEXT("battery"),2,Safe+FVector(75,0,-90));
    const FString DropId=Drop->ObjectId;
    const FVector DropPosition=Drop->GetActorLocation();
    auto* Door=World->Portals.FindByPredicate([](const auto& P){return P.Leaf&&P.Zone==TEXT("N02")&&P.Side==TEXT("north");});
    auto* OpenDoor=World->Portals.FindByPredicate([](const auto& P){return P.Leaf&&P.Zone!=TEXT("N02")&&P.Role!=TEXT("niche");});
    if(Door){Door->Open=false;Door->Locked=true;Door->InsideOnly=true;World->RefreshPortalVisual(*Door);}
    if(OpenDoor){OpenDoor->Open=true;World->RefreshPortalVisual(*OpenDoor);}
    World->RefugeBolts.Add(TEXT("IF08"));World->RestoreLift(FVector(5000,700,578));World->LiftSelection=4;
    auto* LiveActor=Story->Props.FindRef(TEXT("FILE_30"));
    auto* DeadActor=Story->Props.FindRef(TEXT("FILE_33"));
    auto* LiveNPC=LiveActor?LiveActor->FindComponentByClass<UEWardPerson54>():nullptr;
    auto* DeadNPC=DeadActor?DeadActor->FindComponentByClass<UEWardPerson54>():nullptr;
    const FVector WaitAt=Safe+FVector(240,0,-98);
    if(LiveNPC){LiveActor->SetActorLocation(WaitAt,false);LiveActor->SetActorRotation(FRotator(0,127,0));LiveNPC->SetTask58(TEXT("wait"),WaitAt);}
    if(DeadNPC)DeadNPC->Impact58(200,FVector(5,0,0),true);
    AEWardHospitalMob* Hunter=nullptr;
    for(auto Mob:Story->Mobs)if(Mob->Eligible59()&&World->ValidPosition(Mob->Home)){Hunter=Mob;break;}
    if(Hunter)
    {
        Hero->SetActorLocation(Hunter->Home,false);
        Hunter->Active=true;Hunter->State59=TEXT("chase");Hunter->Health=Hunter->MaxHealth*.5f;
        Hunter->Target=Safe;Hunter->LastSeen59=Safe;Hunter->Memory=4;Hunter->SearchStep59=3;
    }
    AEWardHospitalMob* Corpse=nullptr;
    for(auto Mob:Story->Mobs)if(Mob!=Hunter&&(!Hunter||Mob->Guid59!=Hunter->Guid59))
    {
        Corpse=Mob;Corpse->Health=0;Corpse->Dead=true;Corpse->BodyPresent59=true;
        Corpse->State59=TEXT("dead");break;
    }
    const FVector SavedHero=Hero->GetActorLocation();
    for(int Serial:{24,2})
    {
        Json Def=MakeShared<FJsonObject>();Def->SetStringField(TEXT("id"),FString::Printf(TEXT("CHILD_%d"),Serial));
        Def->SetStringField(TEXT("species"),TEXT("parasite"));Def->SetStringField(TEXT("combat_class"),TEXT("A"));
        Def->SetNumberField(TEXT("chapter"),4);Def->SetNumberField(TEXT("health"),25);
        Def->SetNumberField(TEXT("radius"),.25);Def->SetNumberField(TEXT("speed"),2);Def->SetNumberField(TEXT("damage"),7);
        Def->SetArrayField(TEXT("at"),V3((Safe-FVector(0,0,98))/100));
        Story->SpawnMob(Def);
    }
    const FString Slot=TEXT("manual_1");
    Check(TEXT("manual_slot_writes_to_disk"),G.Save(Slot));
    const Json Snapshot=G.Capture();
    G.Flag(TEXT("old_lab_accident29"),false);
    Check(TEXT("in_memory_snapshot_does_not_share_mutable_flags"),Boolean(Object(Snapshot,TEXT("flags")),TEXT("old_lab_accident29")));
    Reset();
    Check(TEXT("manual_slot_loads_after_world_reset"),G.Load(Slot));
    Check(TEXT("chapter_current_objective_and_prior_completion_restored"),Story->Chapter()==4&&
        G.Values.FindRef(TEXT("hospital_step"))==1&&Story->Objective()==Goal&&G.Completed.Contains(TEXT("SCENE_04_01")));
    Check(TEXT("choice_and_consequence_restored"),String(Object(Story->NarrativeState35,TEXT("choices")),TEXT("POWER"))==TEXT("LOCAL")&&
        Story->NarrativeFlag35(TEXT("POWER_LOCAL"))&&G.Flag(TEXT("old_lab_accident29")));
    Check(TEXT("health_ammo_position_inventory_and_stash_restored"),Hero->Health==61&&Hero->Stamina==48&&
        Hero->Rounds==4&&Hero->Reserve==17&&Hero->GetActorLocation().Equals(SavedHero,1)&&
        G.Items.FindRef(TEXT("medkit"))==2&&G.Stash.FindRef(TEXT("battery"))==3);
    Check(TEXT("journal_and_collected_note_restored"),G.Journal.Contains(TEXT("@doc:NOTE_01_01"))&&
        G.Completed.Contains(TEXT("NOTE_01_01"))&&Story->Props[TEXT("NOTE_01_01")]->IsHidden());
    auto* LoadedDrop=G.Objects.FindRef(DropId).Get();
    Check(TEXT("dropped_item_returns_once_at_saved_position"),LoadedDrop&&!LoadedDrop->Used&&
        LoadedDrop->GetActorLocation().Equals(DropPosition,1)&&Number(Object(LoadedDrop->Definition,TEXT("properties")),TEXT("amount"))==2);
    Check(TEXT("entrance_door_keeps_its_saved_lock_swing_and_collision"),Door&&!Door->Open&&Door->Locked&&
        Door->InsideOnly&&Door->Leaf->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
    Check(TEXT("open_door_refuge_bolt_and_lift_restored"),OpenDoor&&OpenDoor->Open&&
        OpenDoor->Leaf->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&World->RefugeBolts.Contains(TEXT("IF08"))&&
        World->LiftFloor==3&&World->LiftSelection==4);
    Check(TEXT("living_npc_wait_order_position_and_orientation_restored"),LiveNPC&&LiveActor->GetActorLocation().Equals(WaitAt,1)&&
        FMath::IsNearlyEqual(LiveActor->GetActorRotation().Yaw,127.0,1.0)&&String(LiveNPC->HumanSnapshot58(),TEXT("task"))==TEXT("wait"));
    Check(TEXT("dead_npc_remains_dead_and_fallen"),DeadNPC&&DeadNPC->IsDead58()&&DeadNPC->IsFallen58());
    Check(TEXT("active_monster_keeps_chase_damage_and_search_target"),Hunter&&Hunter->Active&&Hunter->State59==TEXT("chase")&&
        Hunter->Health==Hunter->MaxHealth*.5f&&Hunter->Memory==4&&Hunter->Target.Equals(Safe,1)&&
        Hunter->SearchStep59==3&&Hunter->GetCharacterMovement()->MovementMode==MOVE_Walking);
    Check(TEXT("killed_monster_remains_dead"),Corpse&&Corpse->Dead&&Corpse->Health==0&&Corpse->BodyPresent59&&!Corpse->Active);
    int Children=0;for(auto Mob:Story->Mobs)if(Mob->Id.StartsWith(TEXT("CHILD_")))++Children;
    Check(TEXT("dynamic_monsters_restore_independently_of_save_order"),Children==2&&Story->SpawnedChildren==24);
    G.Flag(TEXT("old_lab_accident29"),false);
    Check(TEXT("restoring_a_snapshot_does_not_mutate_its_source"),G.Restore(Snapshot)&&
        (G.Flag(TEXT("old_lab_accident29")))&&Boolean(Object(Snapshot,TEXT("flags")),TEXT("old_lab_accident29")));
    G.Flag(TEXT("old_lab_accident29"),false);
    Check(TEXT("restored_flags_are_detached_from_snapshot"),Boolean(Object(Snapshot,TEXT("flags")),TEXT("old_lab_accident29")));
    Json Invalid=G.ReadSlot(Slot);Invalid->SetStringField(TEXT("flags"),TEXT("invalid"));
    Check(TEXT("invalid_slot_cannot_partly_reset_running_game"),!G.Restore(Invalid)&&Hero->Health==61&&G.Items.FindRef(TEXT("medkit"))==2);
    Json Legacy=G.ReadSlot(Slot);
    Json OldDoors=Object(Legacy,TEXT("architecture"));
    for(const TCHAR* Key:{TEXT("locks"),TEXT("inside_only"),TEXT("lift_floor"),TEXT("lift_selection")})OldDoors->RemoveField(Key);
    Check(TEXT("v69_door_only_save_still_loads"),G.Restore(Legacy)&&Story->Chapter()==4&&G.Items.FindRef(TEXT("medkit"))==2);
    for(int Index=1;Index<=3;++Index)
    {
        Json Saved=G.ReadSlot(Slot);
        Saved->SetStringField(TEXT("saved_at"),FString::Printf(TEXT("2000-01-0%dT00:00:00.000Z"),Index));
        Write(G.SaveDirectory()/FString::Printf(TEXT("auto_%d.json"),Index),Saved);
    }
    G.NextAuto=3;G.AutoSave();G.TickAutoSave(0);
    Check(TEXT("autosave_rotation_overwrites_oldest_slot_across_sessions"),G.LastSave==TEXT("auto_1")&&
        String(G.ReadSlot(TEXT("auto_2")),TEXT("saved_at")).StartsWith(TEXT("2000"))&&
        String(G.ReadSlot(TEXT("auto_3")),TEXT("saved_at")).StartsWith(TEXT("2000")));
    FFileHelper::SaveStringToFile(TEXT("{"),*(G.SaveDirectory()/TEXT("manual_6.json")));
    Check(TEXT("latest_skips_corrupt_slot"),G.LoadLatest()&&G.LastSave!=TEXT("manual_6"));

    struct FEndingCase { const TCHAR* Id; const TCHAR* Plan; bool Stay,Memory,Breach; const TCHAR* Summary; };
    const FEndingCase Cases[]={
        {TEXT("CURE"),TEXT("CURE"),false,true,false,TEXT("прошли повторную проверку")},
        {TEXT("CURE_LOSS"),TEXT("CURE"),false,false,false,TEXT("с потерей части памяти")},
        {TEXT("DESTROY"),TEXT("DESTROY"),false,true,false,TEXT("Бека вышел наземным")},
        {TEXT("DESTROY_STAY"),TEXT("DESTROY"),true,true,false,TEXT("погибли внутри центра")},
        {TEXT("EVAC"),TEXT("EVAC"),false,true,false,TEXT("Гражданские эвакуированы")},
        {TEXT("REPLACE"),TEXT("REPLACE"),false,true,false,TEXT("остаётся носителем")},
        {TEXT("CONTINUE"),TEXT("CONTINUE"),false,true,false,TEXT("Проект продолжен")},
        {TEXT("OUTBREAK"),TEXT("CURE"),false,true,true,TEXT("Карантин нарушен")}
    };
    auto Start=[&](const FString& Id)
    {
        for(auto Required:Array(Story->NarrativeScenes35.FindRef(Id),TEXT("requires")))G.Completed.Add(Required->AsString());
        return Story->BeginNarrative35(Id)&&Story->ActiveScene35==Id;
    };
    auto Complete=[&]()
    {
        for(int Page=0;Page<150&&!Story->ActiveScene35.IsEmpty();++Page)Story->NarrativeCommand35(TEXT("story35_next"));
        return Story->ActiveScene35.IsEmpty();
    };
    for(const auto& Case:Cases)
    {
        Reset();Story->SetChapter(18);G.Values.Add(TEXT("hospital_step"),3);
        for(const TCHAR* Fact:{TEXT("COMPARISON_READY"),TEXT("MANUAL_POWER_READY"),TEXT("CONTAINMENT_READY"),
            TEXT("IRINA_CONSENT"),TEXT("KOVALEV_RESCUED"),TEXT("EVAC_LIST_CHECKED"),TEXT("ORLOVA_ALLY"),TEXT("PATIENT_RESCUED")})
            Story->SetNarrativeFlag35(Fact,true);
        Story->SetNarrativeFlag35(TEXT("LOG_PRESERVED"),Case.Memory);
        G.Completed.Add(TEXT("NEW_BASELINE_001"));G.Completed.Add(TEXT("SCENE_18_02"));
        Story->PreparePlans35();
        bool Route=Start(TEXT("SCENE_18_04"));
        Story->NarrativeCommand35(FString(TEXT("story35_choice_"))+Case.Plan);
        Route=Route&&Story->SelectedChoice35==Case.Plan&&Complete()&&Story->Chapter()==19;
        Route=Start(TEXT("SCENE_19_01"))&&Route;
        const FString Confirm=Case.Stay?TEXT("STAY"):TEXT("EXECUTE");
        Story->NarrativeCommand35(TEXT("story35_choice_")+Confirm);
        const bool Mid=G.Save(TEXT("manual_2"))&&G.Load(TEXT("manual_2"))&&
            Story->ActiveScene35==TEXT("SCENE_19_01")&&Story->SelectedChoice35==Confirm&&
            !Story->NarrativeFlag35(TEXT("HERO_STAYS"))&&Story->NarrativeFlag35(TEXT("MEMORY_SPARED"))==Case.Memory;
        Check(FString(Case.Id)+TEXT("_mid_choice_save"),Mid);
        Route=Complete()&&Route;
        Route=Start(TEXT("SCENE_19_02"))&&Route;
        Story->NarrativeCommand35(Case.Breach?TEXT("story35_choice_BREACH"):TEXT("story35_choice_CHECK"));
        Route=Complete()&&Route;
        for(const TCHAR* Id:{TEXT("SCENE_19_03"),TEXT("SCENE_19_04"),TEXT("SCENE_19_05")})
        {
            if(FString(Id)==TEXT("SCENE_19_03")&&(Case.Stay||FString(Case.Plan)==TEXT("REPLACE")||FString(Case.Plan)==TEXT("CONTINUE")))
                Check(FString(Case.Id)+TEXT("_interior_final_objective_matches_core"),Story->Objective().Contains(TEXT("[B2 / E09]"))&&!Story->Objective().Contains(TEXT("Наземный выход")));
            Route=Start(Id)&&Route;
            if(FString(Id)==TEXT("SCENE_19_04"))
            {
                const auto& Variant=Array(Object(Story->Narrative35,TEXT("final_scenes")),Case.Id);
                const FString Expected=Variant.IsValidIndex(3)?Variant[3]->AsString():
                    String(Story->NarrativeScenes35.FindRef(Id),TEXT("text"));
                Check(FString(Case.Id)+TEXT("_selects_authored_final_text"),!Expected.IsEmpty()&&
                    Story->NarrativeBody35(true).StartsWith(Expected)&&Story->NarrativeFlag35(TEXT("MEMORY_SPARED"))==Case.Memory);
            }
            Route=Complete()&&Route;
        }
        const FString Summary=Story->NarrativeEnding35();
        Check(FString(Case.Id)+TEXT("_reaches_distinct_epilogue"),Route&&Story->Chapter()==20&&
            Story->NarrativeFlag35(TEXT("EPILOGUE_COMPLETE"))&&Summary.Contains(Case.Summary)&&G.Journal.Contains(Summary));
        Check(FString(Case.Id)+TEXT("_finished_save_reloads_same_outcome"),G.Save(TEXT("manual_3"))&&
            G.Load(TEXT("manual_3"))&&Story->Chapter()==20&&Story->ActiveScene35.IsEmpty()&&
            Story->NarrativeFlag35(TEXT("EPILOGUE_COMPLETE"))&&Story->NarrativeEnding35()==Summary&&G.Journal.Contains(Summary)&&
            Story->NarrativeFlag35(TEXT("HERO_STAYS"))==Case.Stay&&Story->NarrativeFlag35(TEXT("OUTBREAK"))==Case.Breach);
        auto* EndPC=Cast<APlayerController>(Hero->Controller);
        auto* Camera=EndPC?EndPC->PlayerCameraManager.Get():nullptr;
        const bool Frozen=Case.Stay||FString(Case.Plan)==TEXT("REPLACE");
        Check(FString(Case.Id)+TEXT("_finished_save_restores_player_control_and_view"),
            Hero->GetCharacterMovement()->MovementMode==(Frozen?MOVE_None:MOVE_Walking)&&Camera&&
            (Case.Stay?(Camera->bEnableFading&&Camera->FadeAmount>=.99f):!Camera->bEnableFading));
        if(Case.Stay)
        {
            G.Show(TEXT("pause"));
            Check(TEXT("terminal_death_pause_menu_has_visible_backdrop"),G.Screen==TEXT("pause")&&Camera&&!Camera->bEnableFading);
            G.Show(TEXT("title"));
            Check(TEXT("terminal_death_main_menu_has_visible_backdrop"),G.TitleView&&G.Screen==TEXT("title")&&Camera&&!Camera->bEnableFading);
            G.Show(TEXT(""));
            Check(TEXT("return_from_terminal_menu_restores_death_view"),Hero->GetCharacterMovement()->MovementMode==MOVE_None&&Camera&&Camera->bEnableFading&&Camera->FadeAmount>=.99f);
            G.NewGame(TEXT("normal"));G.Show(TEXT(""));
            Check(TEXT("new_campaign_clears_terminal_fade_and_movement_lock"),Hero->GetCharacterMovement()->MovementMode==MOVE_Walking&&Camera&&!Camera->bEnableFading);
        }
    }

    bool Passed=true;for(const auto& Row:Checks)Passed&=Boolean(Row->AsObject(),TEXT("passed"));
    Json Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),Passed);
    Report->SetNumberField(TEXT("count"),Checks.Num());Report->SetArrayField(TEXT("checks"),Checks);
    Report->SetStringField(TEXT("save_directory"),G.SaveDirectory());
    Write(FPaths::ProjectSavedDir()/TEXT("Prototype/save70_check.json"),Report);
    FPlatformMisc::RequestExit(false);
}
