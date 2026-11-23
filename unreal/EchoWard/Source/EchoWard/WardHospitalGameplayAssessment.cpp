#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Engine/DamageEvents.h"
using namespace WardData;
void AEWardHospitalGameplay::Assess(float D)
{
    static int Stage=0;static float Clock=0;static TArray<TSharedPtr<FJsonValue>> Checks;static AEWardHospitalMob* Walker=nullptr;static FVector Before;
    auto Check=[&](const FString& Name,bool OK){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),OK);Checks.Add(MakeShared<FJsonValueObject>(C));UE_LOG(LogTemp,Display,TEXT("STORY_CHECK %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),*Name);};
    auto* H=Mode->Hero();Clock+=D;
    if(Stage==0)
    {
        Check(TEXT("19_chapters_78_scene_markers_17_species"),Array(Data,TEXT("chapters")).Num()==19&&Array(Data,TEXT("scenes")).Num()==78&&Array(WardBookMapData(),TEXT("monster_rules")).Num()==17);
        bool Blank=true;int Notes=0;for(auto V:Array(Data,TEXT("placements")))if(String(V->AsObject(),TEXT("kind"))==TEXT("note")){++Notes;Blank&=String(V->AsObject(),TEXT("body")).IsEmpty();}
        Check(TEXT("96_notes_exist_and_remain_intentionally_blank"),Blank&&Notes==96);
        int Active=0;for(auto M:Mobs)Active+=M->Active;
        Check(TEXT("chapter_one_has_no_open_monster_encounters"),Chapter()==1&&Active==0&&Mobs.Num()==46);
        Check(TEXT("all_395_story_props_use_detailed_visuals_and_keep_interaction_targets"),DetailedStoryProps==Array(Data,TEXT("placements")).Num()&&MissingStoryVisuals==0);
        bool Animated=true;for(auto M:Mobs)Animated&=M->GetMesh()->GetSkeletalMeshAsset()!=nullptr;
        for(const TCHAR* Clip:{TEXT("Idle"),TEXT("Walk"),TEXT("Run"),TEXT("Attack")})Animated&=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Campaign/Creatures/A_Stalker%s"),Clip))!=nullptr;
        Check(TEXT("all_46_mobs_have_native_rigged_bodies_and_idle_walk_run_attack_clips"),Animated);
        bool SpawnClear=true;FCollisionQueryParams Q(SCENE_QUERY_STAT(StorySpawnAudit),false,H);
        for(auto M:Mobs){Q.AddIgnoredActor(M.Get());SpawnClear&=Mode->BookWorld->ValidPosition(M->Home)&&!GetWorld()->OverlapBlockingTestByChannel(M->Home,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(M->GetCapsuleComponent()->GetScaledCapsuleRadius(),86),Q);}
        Check(TEXT("all_46_authored_mob_capsules_fit_real_furnished_geometry"),SpawnClear);
        Check(TEXT("future_chapter_scene_cannot_be_completed_early"),!UseId(TEXT("SCENE_02_01")));
        H->SetActorLocation(Props.FindRef(TEXT("first_pistol"))->GetActorLocation()+FVector(0,0,91),false);
        const bool Picked=Interact();Check(TEXT("pistol_pickup_with_real_distance_and_visibility_equips_weapon"),Picked&&Mode->Flag(TEXT("weapon"))&&H->Rounds==9&&H->Reserve==18);
        const int Ammo=H->Reserve;Check(TEXT("pickup_cannot_be_duplicated"),!UseId(TEXT("first_pistol"))&&H->Reserve==Ammo);
        UseId(TEXT("NOTE_01_01"));Check(TEXT("collected_note_has_title_and_blank_document_page"),Mode->Journal.Contains(TEXT("@blank:NOTE_01_01"))&&!DocumentTitle(TEXT("@blank:NOTE_01_01")).IsEmpty()&&DocumentBody(TEXT("@blank:NOTE_01_01")).IsEmpty());
        for(auto V:Array(Array(Data,TEXT("chapters"))[0]->AsObject(),TEXT("required")))UseId(V->AsString());
        Active=0;for(auto M:Mobs)Active+=M->Active;Check(TEXT("finishing_first_chapter_activates_only_patients"),Chapter()==2&&Active==8);
        for(auto M:Mobs)if(M->Species==TEXT("patients")){Walker=M;break;}
        if(!Walker){Check(TEXT("patient_actor_exists"),false);Stage=3;return;}
        const FIntVector Start=CellAt(Walker->Home);TArray<FVector> Path;
        for(int I=4;I<=12&&Path.IsEmpty();++I)for(FIntVector Offset:{FIntVector(I,0,0),FIntVector(-I,0,0),FIntVector(0,I,0),FIntVector(0,-I,0)})
        {auto Trial=FindPath(Walker->Home,WorldAt(Start+Offset),30);if(Trial.Num()>0){Path=Trial;break;}}
        Check(TEXT("patient_finds_path_on_actual_half_metre_floor"),!Path.IsEmpty());
        if(!Path.IsEmpty())
        {H->SetActorLocation(Path.Last()+FVector(0,0,7),false);Mode->Invulnerable=100;Walker->Target=H->GetActorLocation();Walker->Memory=10;Before=Walker->GetActorLocation();FreezeAI=false;Clock=0;Stage=1;}
        else Stage=2;
    }
    else if(Stage==1&&Clock>5)
    {
        FreezeAI=true;Check(TEXT("patient_physically_walks_without_teleport"),FVector::Dist2D(Before,Walker->GetActorLocation())>45&&Walker->GetCharacterMovement()->IsMovingOnGround());
        Stage=2;Clock=0;
    }
    else if(Stage==2)
    {
        Mode->DamageAt(Walker->GetActorLocation()+FVector(0,0,180),FVector(0,0,-1),1000);
        Check(TEXT("real_weapon_trace_damages_and_kills_class_A"),Walker->Dead&&Mode->Hits>0);
        SetChapter(6);AEWardHospitalMob* Carrier=nullptr;AEWardHospitalMob* Bulky=nullptr;AEWardHospitalMob* Listener=nullptr;
        for(auto M:Mobs){if(M->Species==TEXT("carrier"))Carrier=M;if(M->Species==TEXT("stitched"))Bulky=M;if(M->Species==TEXT("listener"))Listener=M;}
        FDamageEvent Event;
        if(Bulky)Bulky->TakeDamage(10000,Event,H->GetController(),H);
        Check(TEXT("class_B_is_delayed_not_killed_before_finale"),Bulky&&!Bulky->Dead&&Bulky->Stunned>=9);
        if(Carrier)Carrier->TakeDamage(10000,Event,H->GetController(),H);
        Check(TEXT("carrier_death_spawns_bounded_secondary_threat"),Carrier&&Carrier->Dead&&SpawnedChildren>0&&SpawnedChildren<=24);
        if(Listener)Listener->Hear(Listener->GetActorLocation()+FVector(150,0,0),400);
        Check(TEXT("listener_remembers_heard_position"),Listener&&Listener->Memory>0);
        UseId(TEXT("first_shotgun"));Check(TEXT("shotgun_and_shells_are_real_pickups"),Mode->Flag(TEXT("shotgun"))&&Mode->LoadedShells==5&&Mode->Items.FindRef(TEXT("shells"))>=10);
        UseId(Array(Array(Data,TEXT("chapters"))[5]->AsObject(),TEXT("required"))[0]->AsString());
        if(Mode->BookWorld->Portals.Num()){auto& Door=Mode->BookWorld->Portals[0];Door.Open=!Door.Open;Mode->BookWorld->RefreshPortalVisual(Door);}
        Json Save=Mode->Capture();const int SavedChapter=Chapter(),SavedStep=Mode->Values.FindRef(TEXT("hospital_step")),SavedReserve=H->Reserve;SetChapter(1);Check(TEXT("save_restores_chapter_pickups_ammo_and_blank_notes"),Mode->Restore(Save)&&Chapter()==SavedChapter&&Mode->Flag(TEXT("weapon"))&&Mode->Completed.Contains(TEXT("NOTE_01_01"))&&Mode->Journal.Contains(TEXT("@blank:NOTE_01_01"))&&H->Reserve==SavedReserve);
        bool DoorsRestored=true;Json SavedDoors=Object(Object(Save,TEXT("architecture")),TEXT("doors"));for(const auto& Door:Mode->BookWorld->Portals)DoorsRestored&=Door.Open==Boolean(SavedDoors,Door.Id);
        Check(TEXT("loading_preserves_each_door_and_mid_chapter_step"),DoorsRestored&&SavedStep==1&&Mode->Values.FindRef(TEXT("hospital_step"))==SavedStep);
        Check(TEXT("loading_preserves_dead_monsters_and_spawned_children"),Walker->Dead&&Mobs.Num()==Array(Object(Save,TEXT("hospital_story")),TEXT("mobs")).Num()&&SpawnedChildren>0);
        SetChapter(7);AEWardHospitalMob* Keeper=nullptr;for(auto M:Mobs)if(M->Species==TEXT("keeper")){Keeper=M;break;}
        if(Keeper)Keeper->TakeDamage(10000,Event,H->GetController(),H);
        Check(TEXT("class_C_remains_unkillable_by_ordinary_damage"),Keeper&&!Keeper->Dead);
        SetChapter(19);for(auto M:Mobs)if(M->Species==TEXT("stitched")){M->TakeDamage(10000,Event,H->GetController(),H);Check(TEXT("class_B_can_be_killed_during_finale"),M->Dead);break;}
        // Replay the route in a fresh progress state; the earlier save test has
        // already completed chapter one, and collected scenes are single-use.
        for(const auto& E:Definitions)if(String(E.Value,TEXT("kind"))==TEXT("scene"))Mode->Completed.Remove(E.Key);
        SetChapter(1);bool Progress=true;
        for(auto C:Array(Data,TEXT("chapters")))for(auto Id:Array(C->AsObject(),TEXT("required")))
        {const bool Used=UseId(Id->AsString());Progress&=Used&&Mode->Completed.Contains(Id->AsString());if(!Used||!Mode->Completed.Contains(Id->AsString()))UE_LOG(LogTemp,Error,TEXT("STORY_CHAIN %s chapter=%d step=%d used=%d"),*Id->AsString(),Chapter(),Mode->Values.FindRef(TEXT("hospital_step")),Used);}
        Check(TEXT("all_19_spatial_chapter_chains_can_reach_epilogue"),Progress&&Chapter()==20);
        SetChapter(16);Json Node=Definitions.FindRef(TEXT("LINK_03"));if(Node){FVector At=Vector(Array(Node,TEXT("at")))*100;UseId(TEXT("LINK_03"));Check(TEXT("link_node_can_be_disabled_and_saved"),Mode->Completed.Contains(TEXT("LINK_03")));}
        SetChapter(2);Refresh();H->SetActorLocation(Mode->BookWorld->Start(),false);Mode->Invulnerable=100;Clock=0;Stage=3;
        if(FParse::Param(FCommandLine::Get(),TEXT("WardStoryPictures")))
        {Walker->ResetMob();Refresh();auto* PC=Cast<APlayerController>(H->Controller);H->SetActorLocation(Walker->Home+FVector(-300,0,7),false);PC->SetControlRotation(FRotator(-8,0,0));Mode->BookWorld->UpdateLighting(H->GetActorLocation());}
    }
    else if(Stage==3&&Clock>1)
    {
        if(FParse::Param(FCommandLine::Get(),TEXT("WardStoryPictures")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/story_v7.png"),true,false);
        bool OK=true;for(auto V:Checks)OK&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),OK);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/story_check.json"),R);Stage=4;Clock=0;
    }
    else if(Stage==4&&Clock>1)FPlatformMisc::RequestExit(false);
}
