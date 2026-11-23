#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "WardTitleScene.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformMisc.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Components/LightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
using namespace WardData;
namespace
{
    // Deliver native pointer enter/leave events, independently of keyboard focus.
    void MenuHover(TSharedPtr<SWidget> W,bool Enter){if(!W)return;const FPointerEvent Event;if(Enter)W->OnMouseEnter(W->GetCachedGeometry(),Event);else W->OnMouseLeave(Event);}
}
void AEWardCampaignMode::TickCampaignAssessment(float D)
{
    static float Time=0;Time+=D;
    if(FParse::Param(FCommandLine::Get(),TEXT("WardBookMapCheck"))){TickBookMapAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardFieldMenuCheck"))){TickFieldMenuAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardGuideMediaCapture"))){TickGuideMediaCapture(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardSettingsMenuCheck"))){TickSettingsMenuAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardDifficultyMenuCheck"))){TickDifficultyMenuAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardPauseMenuCheck"))){TickPauseMenuAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardHUDCheck"))){TickHUDMenuAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardTrainingMenuCheck"))){TickTrainingMenuAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardAboutMenuCheck"))){TickAboutMenuAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardSaveMenuCheck"))){TickSaveMenuAssessment(D);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardArchiveCheck")))
    {
        static int Stage=0,MenuFrames=0;static double Start=FPlatformTime::Seconds(),Wait=0,MenuStart=0,MenuEnd=0;static float WaitSeconds=0;static FVector Cam,Pose;static TArray<TSharedPtr<FJsonValue>> Checks;
        const float T=FPlatformTime::Seconds()-Start;
        if(T>=1&&T<8){if(!MenuFrames)MenuStart=FPlatformTime::Seconds();++MenuFrames;MenuEnd=FPlatformTime::Seconds();}
        auto Check=[&](FString Name,bool Pass){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));};
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/ArchiveCaptures/")+Name,true,false);};
        auto* PC=UGameplayStatics::GetPlayerController(this,0);
        if(Stage==10){TitleScene->GameAge=4;TitleScene->TickGameBackdrop(0);const FVector At=TitleScene->GetActorLocation()+FVector(540,7630,145),Target=TitleScene->GetActorLocation()+FVector(811.25,7816.25,95);PC->GetViewTarget()->SetActorLocationAndRotation(At,(Target-At).Rotation());}
        if(Stage==2&&T>5&&T<6)if(auto W=Interface->TitleActions.FindRef(TEXT("latest")).Pin())FSlateApplication::Get().SetKeyboardFocus(W);
        if(Stage==4&&T>7.2f)MenuHover(Interface->TitleActions.FindRef(TEXT("latest")).Pin(),false);
        if(Stage==0){SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("ArchiveCheckSaves");Show(TEXT("title"));Show(TEXT("game"));TitleScene->GameAge=4;TitleScene->TickGameBackdrop(0);Cam=TitleScene->GameCamera->GetActorLocation();++Stage;}
        else if(Stage==1&&T>4)
        {
            Check(TEXT("different_live_3d_camera_for_game_page"),TitleScene->GameBackdropActive&&PC->GetViewTarget()==TitleScene->GameCamera&&FVector::Dist(TitleScene->Camera->GetActorLocation(),TitleScene->GameCamera->GetActorLocation())>7000);
            Check(TEXT("archive_camera_moves_in_paused_world"),!Cam.Equals(TitleScene->GameCamera->GetActorLocation(),.1f)&&Hero()->IsHidden());
            Check(TEXT("three_real_openings_and_room_doors"),TitleScene->GameDoors.Num()==3);
            Check(TEXT("blood_decals_only_remote_menu_set"),TitleScene->GameBloodDecals==17&&TitleScene->GetActorLocation().X>25000);
            Check(TEXT("licensed_zombie_and_original_walk_loaded"),TitleScene->GameZombie->GetSkeletalMeshAsset()&&TitleScene->GameZombie->GetSingleNodeInstance()&&TitleScene->GameZombie->GetSingleNodeInstance()->GetLength()>1);
            Check(TEXT("game_menu_actions_keep_existing_save_flow"),Interface->TitleActions.Contains(TEXT("latest"))&&Interface->TitleActions.Contains(TEXT("page_difficulty"))&&Interface->TitleActions.Contains(TEXT("page_saves"))&&Interface->TitleActions.Contains(TEXT("back")));
            auto* M=TitleScene->GameZombie->GetMaterial(0);auto* R=M?M->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;Check(TEXT("zombie_gpu_material_valid"),R&&R->GetGameThreadShaderMap()&&R->GetGameThreadShaderMap()->IsValidForRendering());
            auto* Floor=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/GameMenu/M_ArchiveFloor"));auto* FR=Floor?Floor->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;Check(TEXT("archive_floor_gpu_material_valid"),FR&&FR->GetGameThreadShaderMap()&&FR->GetGameThreadShaderMap()->IsValidForRendering());
            Shot(TEXT("game_glass_1080.png"));Pose=TitleScene->GameZombie->GetSocketTransform(TitleScene->GameZombie->GetBoneName(5),RTS_Component).GetLocation();++Stage;
        }
        else if(Stage==2&&T>6){Check(TEXT("zombie_pose_animates_while_paused"),!Pose.Equals(TitleScene->GameZombie->GetSocketTransform(TitleScene->GameZombie->GetBoneName(5),RTS_Component).GetLocation(),.01f));Check(TEXT("glass_rows_neutral_without_pointer_despite_focus"),Interface->TitleMotion[TEXT("latest")]()<.01f&&Interface->TitleMotion[TEXT("page_difficulty")]()<.01f);MenuHover(Interface->TitleActions.FindRef(TEXT("latest")).Pin(),true);++Stage;}
        else if(Stage==3&&T>7){Check(TEXT("glass_row_pointer_hover_animates"),Interface->TitleMotion.Contains(TEXT("latest"))&&Interface->TitleMotion[TEXT("latest")]()>.8);Shot(TEXT("game_glass_focus_1080.png"));++Stage;}
        else if(Stage==4&&T>8){Check(TEXT("glass_row_glow_fades_after_pointer_leaves"),Interface->TitleMotion[TEXT("latest")]()<.05f);Interface->Close();++Stage;}
        else if(Stage==5&&T>9){Shot(TEXT("archive_corridor_1080.png"));TitleScene->GameAge=13.95f;TitleScene->TickGameBackdrop(0);++Stage;}
        else if(Stage==6&&TitleScene->ZombieInside){Wait=FPlatformTime::Seconds();++Stage;}
        else if(Stage==7&&!TitleScene->ZombieInside){WaitSeconds=FPlatformTime::Seconds()-Wait;Check(TEXT("zombie_stays_in_room_ten_real_seconds"),WaitSeconds>=9.8f&&WaitSeconds<10.8f);Check(TEXT("zombie_exits_the_second_room"),TitleScene->ZombieRoom==1);TitleScene->GameAge=30;TitleScene->TickGameBackdrop(0);++Stage;}
        else if(Stage==8&&T>22){Shot(TEXT("archive_second_room_1080.png"));TitleScene->GameAge=54;TitleScene->TickGameBackdrop(0);++Stage;}
        else if(Stage==9&&T>24){Check(TEXT("zombie_visits_third_room"),TitleScene->ZombieRoom==2);Shot(TEXT("archive_third_room_1080.png"));TitleScene->GameAge=71.95;TitleScene->TickGameBackdrop(.1);Check(TEXT("zombie_route_loops_to_first_room"),TitleScene->ZombieRoom==0&&TitleScene->GameAge>72&&!TitleScene->ZombieInside);Show(TEXT("game"));Command(TEXT("page_difficulty"));Check(TEXT("new_game_opens_difficulty"),Screen==TEXT("difficulty")&&TitleScene->SaveBackdropActive&&!TitleScene->GameBackdropActive);Command(TEXT("back"));Command(TEXT("page_saves"));Check(TEXT("all_saves_open_separate_quiet_background"),Screen==TEXT("saves")&&TitleScene->SaveBackdropActive&&!TitleScene->GameBackdropActive);Command(TEXT("back"));Command(TEXT("back"));Check(TEXT("back_restores_original_title_camera"),Screen==TEXT("title")&&!TitleScene->GameBackdropActive&&PC->GetViewTarget()==TitleScene->Camera);Show(TEXT("game"));Interface->Close();auto* View=GetWorld()->SpawnActor<ACameraActor>();const FVector At=TitleScene->GetActorLocation()+FVector(0,8000,0)+FVector(640,-120,150);TitleScene->GameAge=4;TitleScene->TickGameBackdrop(0);View->SetActorLocationAndRotation(At,(TitleScene->GameZombie->Bounds.Origin-At).Rotation());View->GetCameraComponent()->SetFieldOfView(50);View->GetCameraComponent()->PostProcessSettings=TitleScene->GameCamera->GetCameraComponent()->PostProcessSettings;View->GetCameraComponent()->PostProcessSettings.DepthOfFieldFstop=8;PC->SetViewTarget(View);++Stage;}
        else if(Stage==10&&T>26){Shot(TEXT("archive_zombie_1080.png"));++Stage;}
        else if(Stage==11&&T>28){Show(TEXT("settings"));if(auto W=Interface->TitleActions.FindRef(TEXT("setting_fullscreen")).Pin())FSlateApplication::Get().SetKeyboardFocus(W);++Stage;}
        else if(Stage==12&&T>29){Check(TEXT("settings_button_neutral_despite_keyboard_focus"),Interface->TitleMotion.Contains(TEXT("setting_fullscreen"))&&Interface->TitleMotion[TEXT("setting_fullscreen")]()<.01f);MenuHover(Interface->TitleActions.FindRef(TEXT("setting_fullscreen")).Pin(),true);++Stage;}
        else if(Stage==13&&T>30){Check(TEXT("settings_button_red_only_on_pointer_hover"),Interface->TitleMotion[TEXT("setting_fullscreen")]()>.8f);Shot(TEXT("settings_hover_1080.png"));MenuHover(Interface->TitleActions.FindRef(TEXT("setting_fullscreen")).Pin(),false);++Stage;}
        else if(Stage==14&&T>31){Check(TEXT("settings_button_neutral_after_pointer_leave"),Interface->TitleMotion[TEXT("setting_fullscreen")]()<.05f);Show(TEXT(""));Check(TEXT("archive_hidden_in_gameplay"),!TitleScene->Active&&!TitleScene->GameBackdropActive&&!TitleScene->GameZombie->IsVisible());Json R=MakeShared<FJsonObject>();bool Pass=true;for(auto J:Checks)Pass&=Boolean(J->AsObject(),TEXT("passed"));R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);R->SetNumberField(TEXT("room_wait_real_seconds"),WaitSeconds);R->SetNumberField(TEXT("menu_observed_fps_including_capture"),(MenuFrames-1)/FMath::Max(.01,MenuEnd-MenuStart));Write(FPaths::ProjectSavedDir()/TEXT("Prototype/archive_check.json"),R);FPlatformMisc::RequestExit(false);++Stage;}
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WardMenuCheck")))
    {
        static double FlowStart=0,WaitStart=0,EntranceStart=0;static float WaitDuration=0,EntranceDuration=0;
        const float F=FlowStart>0?FPlatformTime::Seconds()-FlowStart:0,C=TitleScene?TitleScene->IdleCycle:0;
        static int Step=0;static float Frozen=0;static FString SavedMenuScene68;static FVector Initial,BallDuringWait,RollingStart;static FQuat RollingRotation;
        static FTransform FirstVisibleNurseHead;static int32 NursePoseSamples=0;static double NurseHeadTravel=0,NurseHeadTurn=0;
        static TSet<FName> Done;static TArray<TSharedPtr<FJsonValue>> Checks;
        auto Once=[&](FName Id){if(Done.Contains(Id))return false;Done.Add(Id);return true;};
        auto Check=[&](const FString& Name,bool Pass){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));};
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/MenuCaptures/")+Name,true,false);};
        if(Step>0&&Step<=4)
        {
            // Compare evaluated poses while she is visible. A single head-pivot
            // position compared with the initial rest pose misses rotation-only
            // animation and can coincide with an idle clip's starting position.
            if(C>25&&C<29.2f&&TitleScene->Nurse->IsVisible())
            {
                const FTransform Pose=TitleScene->Nurse->GetSocketTransform(TitleScene->NurseHeadBone,RTS_Component);
                if(NursePoseSamples==0)FirstVisibleNurseHead=Pose;
                else
                {
                    NurseHeadTravel=FMath::Max(NurseHeadTravel,FVector::Dist(FirstVisibleNurseHead.GetLocation(),Pose.GetLocation()));
                    NurseHeadTurn=FMath::Max(NurseHeadTurn,FirstVisibleNurseHead.GetRotation().AngularDistance(Pose.GetRotation()));
                }
                ++NursePoseSamples;
            }
            if(C>6&&C<8&&Once(TEXT("focus_item"))){if(auto W=Interface->TitleActions.FindRef(TEXT("page_settings")).Pin())FSlateApplication::Get().SetKeyboardFocus(W,EFocusCause::SetDirectly);}
            if(C>6.5f&&C<8&&Once(TEXT("pointer_enter"))){auto* B=Interface->TitleMotion.Find(TEXT("page_settings"));Check(TEXT("main_menu_keyboard_focus_does_not_glow"),B&&(*B)()<.01f&&Interface->TitleMotion[TEXT("page_game")]()<.01f);MenuHover(Interface->TitleActions.FindRef(TEXT("page_settings")).Pin(),true);}
            if(C>7.3f&&C<8&&Once(TEXT("focus_shot"))){auto* B=Interface->TitleMotion.Find(TEXT("page_settings"));Check(TEXT("menu_mouse_hover_animates_highlight_and_text"),B&&(*B)()>.8f);Shot(TEXT("menu_hover_1080.png"));MenuHover(Interface->TitleActions.FindRef(TEXT("page_settings")).Pin(),false);}
            if(C>11&&Once(TEXT("roll_start"))){RollingStart=TitleScene->RollingBall->GetRelativeLocation();RollingRotation=TitleScene->RollingBall->GetRelativeRotation().Quaternion();}
            if(C>12.5f&&Once(TEXT("roll_check"))){Check(TEXT("ball_rolls_and_rotates_out_of_ward"),TitleScene->RollingBall->IsVisible()&&FVector::Dist(RollingStart,TitleScene->RollingBall->GetRelativeLocation())>25&&RollingRotation.AngularDistance(TitleScene->RollingBall->GetRelativeRotation().Quaternion())>.1);Shot(TEXT("ball_roll_1080.png"));}
            if(TitleScene->IdleBeat==TEXT("ball_wait")&&Once(TEXT("wait_start"))){WaitStart=FPlatformTime::Seconds();BallDuringWait=TitleScene->RollingBall->GetRelativeLocation();}
            if(C>17.8f&&Once(TEXT("ball_still"))){Check(TEXT("ball_stays_on_floor_before_hand"),BallDuringWait.Equals(TitleScene->RollingBall->GetRelativeLocation(),.01f)&&!TitleScene->ReachingHand->IsVisible()&&FMath::IsNearlyEqual(float(BallDuringWait.Z),12.f));Shot(TEXT("ball_wait_1080.png"));}
            if(TitleScene->IdleBeat==TEXT("hand_reach")&&Once(TEXT("wait_end"))){WaitDuration=FPlatformTime::Seconds()-WaitStart;Check(TEXT("ball_waits_five_seconds_before_hand"),WaitStart>0&&WaitDuration>4.7f&&WaitDuration<5.3f&&TitleScene->BallStillSeconds>=4.99f);}
            if(C>19.5f&&Once(TEXT("hand_shot"))){Check(TEXT("real_3d_hand_reaches_ball"),TitleScene->ReachingHand->IsVisible()&&TitleScene->Forearm->IsVisible()&&TitleScene->EventRigReady&&TitleScene->RollingBall->IsVisible());Shot(TEXT("hand_reach_1080.png"));}
            if(C>20.6f&&Once(TEXT("ball_pull"))){Check(TEXT("hand_drags_ball_toward_door"),TitleScene->RollingBall->GetRelativeLocation().Y>BallDuringWait.Y+60&&TitleScene->ReachingHand->IsVisible());}
            if(C>23.3f&&Once(TEXT("ball_gone"))){Check(TEXT("ball_and_hand_return_before_door_closes"),!TitleScene->RollingBall->IsVisible()&&!TitleScene->ReachingHand->IsVisible()&&!TitleScene->Forearm->IsVisible()&&TitleScene->IdleDoorOpen<.001f);}
            if(C>29.2f&&Once(TEXT("nurse_out"))){Check(TEXT("nurse_exits_after_ball_sequence"),TitleScene->NurseInCorridor&&TitleScene->Nurse->IsVisible()&&!TitleScene->RollingBall->IsVisible()&&!TitleScene->PeekingHead->IsVisible());Check(TEXT("nurse_animation_refreshes_during_pause"),NursePoseSamples>=2&&(NurseHeadTravel>.1||NurseHeadTurn>.01)&&FMath::IsNearlyEqual(Frozen,Elapsed));Interface->Open();Shot(TEXT("menu_1080.png"));}
            if(C>47.3f&&Once(TEXT("nurse_gone"))){Check(TEXT("nurse_returns_to_ward_before_entrance"),!TitleScene->Nurse->IsVisible()&&TitleScene->IdleDoorOpen<.001f);}
            if(TitleScene->IdleEntranceOpen>.999f&&Once(TEXT("entrance_start"))){EntranceStart=FPlatformTime::Seconds();Check(TEXT("main_entrance_opens_after_nurse"),!TitleScene->Nurse->IsVisible()&&TitleScene->FarHinges.Num()==2);Shot(TEXT("entrance_idle_1080.png"));}
            if(C>52.02f&&Once(TEXT("entrance_hold"))){EntranceDuration=FPlatformTime::Seconds()-EntranceStart;Check(TEXT("main_entrance_holds_three_seconds"),EntranceStart>0&&EntranceDuration>2.8f&&EntranceDuration<3.3f);}
            if(C>53.5f&&Once(TEXT("entrance_closed"))){Check(TEXT("main_entrance_closes_after_hold"),TitleScene->IdleEntranceOpen<.001f&&TitleScene->FarHinges[1]->GetRelativeRotation().IsNearlyZero());}
            if(TitleScene->IdleLoopCount>=1&&Once(TEXT("loop_repeated"))){Check(TEXT("ambient_story_repeats_from_quiet_start"),C<1&&TitleScene->IdleBeat==TEXT("quiet")&&!TitleScene->PeekingHead->IsVisible()&&!TitleScene->Nurse->IsVisible()&&!TitleScene->RollingBall->IsVisible());}
        }
        if(Step==5)
        {
            if(TitleScene->TransitionAge>=3&&Once(TEXT("movie_gate"))){Check(TEXT("submenu_waits_for_door_and_scare"),Screen==TEXT("title")&&TitleScene->Transitioning&&!TitleScene->ScareFired);Check(TEXT("play_interrupts_idle_props"),!TitleScene->PeekingHead->IsVisible()&&!TitleScene->ReachingHand->IsVisible()&&!TitleScene->RollingBall->IsVisible()&&TitleScene->IdleEntranceOpen==0);}
            if(TitleScene->TransitionAge>=3.9f&&Once(TEXT("movie_door_shot")))Shot(TEXT("door_open_1080.png"));
            if(TitleScene->TransitionAge>=4.65f&&Once(TEXT("movie_door")))Check(TEXT("cinematic_opens_both_door_leaves"),TitleScene->FarDoorOpen>.95f&&TitleScene->FarHinges.Num()==2);
            if(TitleScene->TransitionAge>=5.95f&&Once(TEXT("movie_scare"))){Check(TEXT("camera_enters_doorway_before_scare"),TitleScene->Camera->GetActorLocation().X>32800);Check(TEXT("scare_is_close_animated_3d_nurse"),TitleScene->NurseScare>.9f&&TitleScene->Nurse->GetMorphTarget(TEXT("AU_26_JawDrop"))>.8f&&TitleScene->Nurse->IsVisible()&&FVector::Dist(TitleScene->Nurse->GetComponentLocation()+FVector(0,0,161),TitleScene->Camera->GetActorLocation())<90);Shot(TEXT("scare_1080.png"));}
        }
        if(F>30&&F<32&&Once(TEXT("focus_card")))MenuHover(Interface->TitleActions.FindRef(TEXT("page_trailer")).Pin(),true);
        if(F>31&&F<32&&Once(TEXT("card_shot"))){auto* B=Interface->TitleMotion.Find(TEXT("page_trailer"));Check(TEXT("media_mouse_hover_animates_frame_zoom_play"),B&&(*B)()>.8f);Shot(TEXT("menu_card_hover_1080.png"));MenuHover(Interface->TitleActions.FindRef(TEXT("page_trailer")).Pin(),false);}
        if(Step==18)TitleScene->Age=31.3f;
        if(Step==0){SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("MenuCheckSaves");CampaignStarted=false;Show(TEXT("title"));Frozen=Elapsed;Initial=TitleScene->Camera->GetActorLocation();++Step;}
        else if(Step==1&&C>4.5f)
        {
            Check(TEXT("title_has_live_camera_no_player"),TitleScene->Active&&Hero()->IsHidden()&&UGameplayStatics::GetPlayerController(this,0)->GetViewTarget()==TitleScene->Camera);
            Check(TEXT("title_and_all_submenu_sets_are_outside_the_campus_forest"),TitleScene->GetActorLocation().X>250000&&TitleScene->GetActorLocation().Y>250000);
            Check(TEXT("rendered_camera_tracks_live_camera"),FVector::Dist(UGameplayStatics::GetPlayerController(this,0)->PlayerCameraManager->GetCameraLocation(),TitleScene->Camera->GetActorLocation())<2);
            Check(TEXT("camera_moves_while_campaign_paused"),FVector::Dist(Initial,TitleScene->Camera->GetActorLocation())>.5&&FMath::IsNearlyEqual(Frozen,Elapsed));
            Check(TEXT("wheelchair_and_menu_audio_loaded"),TitleScene->Wheelchair&&Effects.Contains(TEXT("menu_vent"))&&Effects.Contains(TEXT("menu_drip"))&&Effects.Contains(TEXT("menu_click")));
            for(const TCHAR* Name:{TEXT("M_MenuWear"),TEXT("M_MenuDust"),TEXT("M_MenuLinen"),TEXT("../MenuNurse/M_Nurse_body"),TEXT("../MenuNurse/M_Nurse_head"),TEXT("../MenuNurse/M_NurseCloth"),TEXT("../MenuEvents/M_MenuZombieCrop"),TEXT("../MenuEvents/M_MenuBall"),TEXT("../MenuEvents/M_MenuArm"),TEXT("../MenuEvents/M_MenuBlood")}){auto* M=LoadObject<UMaterialInterface>(nullptr,*(FString(Name).StartsWith(TEXT("../"))?FString(TEXT("/Game/Campaign/"))+FString(Name).Mid(3):FString(TEXT("/Game/Campaign/Menu/"))+Name));const auto* R=M?M->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;const auto* Map=R?R->GetGameThreadShaderMap():nullptr;Check(FString(TEXT("menu_gpu_"))+Name,Map&&Map->IsValidForRendering());}
            Check(TEXT("nurse_has_real_skeletal_rig"),TitleScene->Nurse&&TitleScene->Nurse->GetNumBones()>50&&TitleScene->Nurse->GetAnimInstance());
            Check(TEXT("zombie_head_peeks_from_open_door"),TitleScene->PeekingHead->IsVisible()&&TitleScene->EventRigReady&&TitleScene->IdleDoorOpen>.4f);
            Check(TEXT("nurse_and_ball_wait_for_head_to_leave"),!TitleScene->Nurse->IsVisible()&&!TitleScene->RollingBall->IsVisible());
            Check(TEXT("door_and_scare_audio_loaded"),Effects.Contains(TEXT("menu_door"))&&Effects.Contains(TEXT("menu_scare")));
            Check(TEXT("blood_is_confined_to_title_set"),TitleScene->BloodDecals==11&&TitleScene->GetActorLocation().X>25000);
            Shot(TEXT("zombie_head_1080.png"));++Step;
        }
        else if(Step==2&&C>8){Check(TEXT("main_menu_glow_fades_after_pointer_leave"),Interface->TitleMotion[TEXT("page_settings")]()<.05f);Interface->Close();++Step;}
        else if(Step==3&&C>9){Shot(TEXT("corridor_1080.png"));++Step;}
        else if(Step==4&&TitleScene->Age>64){Interface->Open();Command(TEXT("page_game"));Check(TEXT("game_button_starts_cinematic"),TitleScene->Transitioning&&Screen==TEXT("title"));FlowStart=FPlatformTime::Seconds();++Step;}
        else if(Step==5&&F>11){Check(TEXT("game_submenu_after_cinematic"),Screen==TEXT("game")&&TitleScene->Active&&!TitleScene->Transitioning&&!CampaignStarted);Check(TEXT("scare_plays_once_before_submenu"),TitleScene->ScareCount==1);Shot(TEXT("game_1080.png"));Command(TEXT("page_difficulty"));Check(TEXT("difficulty_only_after_new_game"),Screen==TEXT("difficulty"));++Step;}
        else if(Step==6&&F>13){Command(TEXT("new_story"));Check(TEXT("new_game_uses_cinematic"),TitleScene->Transitioning&&Screen==TEXT("difficulty"));++Step;}
        else if(Step==7&&F>18){bool ArrivalOK=true;if(HospitalStory){ArrivalOK=HospitalStory->ArrivalStage==0&&Hero()->IsHidden()&&UGameplayStatics::GetPlayerController(this,0)->GetViewTarget()==HospitalStory->ArrivalCamera;HospitalStory->ArrivalInteract();HospitalStory->ArrivalInteract();ArrivalOK&=HospitalStory->ArrivalStage==2&&!Hero()->IsHidden();}else ArrivalOK=UGameplayStatics::GetPlayerController(this,0)->GetViewTarget()==Hero();const bool IntroScreen68=Screen.IsEmpty()||(Screen==TEXT("story35")&&HospitalStory&&HospitalStory->ActiveScene35==TEXT("SCENE_01_01"));UE_LOG(LogTemp,Display,TEXT("AUDIO68_MENU_INTRO stage=%d screen=%s hidden=%d walking=%d arrival=%d"),HospitalStory?HospitalStory->ArrivalStage:-1,*Screen,Hero()->IsHidden(),Hero()->GetCharacterMovement()->MovementMode==MOVE_Walking,ArrivalOK);Check(TEXT("new_game_enters_arrival_and_exit_restores_walking"),ArrivalOK&&IntroScreen68&&CampaignStarted&&!TitleScene->Active&&!Hero()->IsHidden()&&Hero()->GetCharacterMovement()->MovementMode==MOVE_Walking&&Difficulty==TEXT("story"));Check(TEXT("intro_dialogue_is_not_discarded_by_menu_flow"),!HospitalStory||!HospitalStory->NarrativeEnabled35||HospitalStory->ActiveScene35==TEXT("SCENE_01_01"));Check(TEXT("menu_props_hidden_during_gameplay"),TitleScene->IsHidden()&&!TitleScene->PeekingHead->IsVisible()&&!TitleScene->RollingBall->IsVisible());Show(TEXT("title"));Frozen=Elapsed;++Step;}
        else if(Step==8&&F>20){Command(TEXT("page_screenshots"));Check(TEXT("screenshot_gallery_opens"),Screen==TEXT("screenshots")&&Interface->Root.IsValid());Shot(TEXT("gallery_1080.png"));++Step;}
        else if(Step==9&&F>22){Command(TEXT("page_trailer"));Check(TEXT("trailer_placeholder_opens"),Screen==TEXT("trailer"));Command(TEXT("resume"));Check(TEXT("resume_preserves_campaign"),Screen.IsEmpty()&&!TitleScene->Active&&FMath::IsNearlyEqual(Frozen,Elapsed));++Step;}
        else if(Step==10&&F>24){SavedMenuScene68=HospitalStory?HospitalStory->ActiveScene35:FString();Hero()->Health=73;Check(TEXT("isolated_menu_save_written"),Save(TEXT("manual_6")));Hero()->Health=25;Show(TEXT("title"));TitleScene->Age=19.3f;TitleScene->Tick(0);Command(TEXT("latest"));Check(TEXT("latest_save_uses_cinematic"),TitleScene->Transitioning);Check(TEXT("load_interrupts_ball_and_hand"),!TitleScene->RollingBall->IsVisible()&&!TitleScene->ReachingHand->IsVisible());++Step;}
        else if(Step==11&&F>29){const bool RestoredScreen68=Screen.IsEmpty()||(Screen==TEXT("story35")&&HospitalStory&&HospitalStory->ActiveScene35==SavedMenuScene68&&!SavedMenuScene68.IsEmpty());Check(TEXT("latest_save_restores_player_and_state"),!TitleScene->Active&&RestoredScreen68&&!Hero()->IsHidden()&&Hero()->Health>=73&&LastSave==TEXT("manual_6"));Check(TEXT("menu_load_preserves_saved_intro_dialogue"),!HospitalStory||HospitalStory->ActiveScene35==SavedMenuScene68);Check(TEXT("load_does_not_replay_scare"),TitleScene->ScareCount==1);Show(TEXT("title"));Language=TEXT("kk");Show(TEXT("title"));++Step;}
        else if(Step==12&&F>32){Shot(TEXT("menu_kk_1080.png"));++Step;}
        else if(Step==13&&F>34){Language=TEXT("en");Show(TEXT("title"));++Step;}
        else if(Step==14&&F>36){Shot(TEXT("menu_en_1080.png"));++Step;}
        else if(Step==15&&F>38){Interface->Close();auto* View=GetWorld()->SpawnActor<ACameraActor>();View->SetActorLocationAndRotation(TitleScene->GetActorLocation()+FVector(1300,335,160),FRotator(-12,90,0));View->GetCameraComponent()->SetFieldOfView(65);UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(View);++Step;}
        else if(Step==16&&F>42){Shot(TEXT("room_photo_1080.png"));++Step;}
        else if(Step==17&&F>44){auto* View=GetWorld()->SpawnActor<ACameraActor>();const FVector Target=TitleScene->GetActorLocation()+FVector(1345,60,130),At=TitleScene->GetActorLocation()+FVector(1175,-150,155);View->SetActorLocationAndRotation(At,(Target-At).Rotation());View->GetCameraComponent()->SetFieldOfView(52);View->GetCameraComponent()->PostProcessSettings=TitleScene->Camera->GetCameraComponent()->PostProcessSettings;View->GetCameraComponent()->PostProcessSettings.DepthOfFieldFstop=5.6;View->GetCameraComponent()->PostProcessSettings.DepthOfFieldFocalDistance=270;UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(View);++Step;}
        else if(Step==18&&F>46){Shot(TEXT("nurse_1080.png"));++Step;}
        else if(Step==19&&F>47){Json R=MakeShared<FJsonObject>();bool Pass=true;for(const auto& J:Checks)Pass&=Boolean(J->AsObject(),TEXT("passed"));R->SetBoolField(TEXT("success"),Pass);R->SetArrayField(TEXT("checks"),Checks);R->SetNumberField(TEXT("count"),Checks.Num());R->SetNumberField(TEXT("ball_wait_real_seconds"),WaitDuration);R->SetNumberField(TEXT("entrance_hold_real_seconds"),EntranceDuration);R->SetNumberField(TEXT("nurse_pose_samples"),NursePoseSamples);R->SetNumberField(TEXT("nurse_head_travel_cm"),NurseHeadTravel);R->SetNumberField(TEXT("nurse_head_turn_rad"),NurseHeadTurn);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/menu_check.json"),R);FPlatformMisc::RequestExit(false);++Step;}
        return;
    }

    if(FParse::Param(FCommandLine::Get(),TEXT("WardCampaignCapture")))
    {
        static int32 Index=FParse::Param(FCommandLine::Get(),TEXT("WardVisualReview"))?17:FParse::Param(FCommandLine::Get(),TEXT("WardPoseCapture"))?10:-1;static float Since=Index>=10?8:0;static bool Requested=false;Since+=D;Invulnerable=100;
        if(Index>=0&&Since>(Index==17?1.52f:6.f)&&!Requested){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/CampaignCaptures/view_%d.png"),Index),true,false);Requested=true;
            if(Index>=11){Json Pose=MakeShared<FJsonObject>(),Joints=MakeShared<FJsonObject>();TArray<FName> Bones;Hero()->GetMesh()->GetBoneNames(Bones);FVector Wrist=FVector::ZeroVector,IndexBase=FVector::ZeroVector,PinkyBase=FVector::ZeroVector;const FTransform Gun=Hero()->WeaponPose();for(FName Bone:Bones){const FString N=Bone.ToString().Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "));const FVector P=Hero()->GetMesh()->GetBoneLocation(Bone);if(N==TEXT("Bip01 R Hand")||N==TEXT("Bip01 L Hand")){const bool Left=N.Contains(TEXT(" L "));Pose->SetNumberField(Left?TEXT("left_grip_error_cm"):TEXT("right_grip_error_cm"),FVector::Dist(P,Hero()->WeaponGrip(Left)));if(!Left)Wrist=P;}if(N==TEXT("Bip01 R Finger1"))IndexBase=P;if(N==TEXT("Bip01 R Finger4"))PinkyBase=P;if(N.Contains(TEXT(" R Finger"))){const FVector Local=Gun.InverseTransformPosition(P);TArray<TSharedPtr<FJsonValue>> V;for(double C:{Local.X,Local.Y,Local.Z})V.Add(MakeShared<FJsonValueNumber>(C));Joints->SetArrayField(N,V);}}const FVector Palm=Gun.InverseTransformVectorNoScale(((IndexBase+PinkyBase)*.5-Wrist).GetSafeNormal());Pose->SetNumberField(TEXT("right_palm_down_alignment"),-Palm.Z);Pose->SetObjectField(TEXT("right_finger_joints_weapon_cm"),Joints);Write(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/CampaignCaptures/pose_%d.json"),Index),Pose);}
        }
        if(Index==-1||Since>7)
        {
            ++Index;Since=0;Requested=false;
            if(Index==0){NewGame(TEXT("normal"));Show(TEXT(""));for(TActorIterator<ALight> It(GetWorld());It;++It){auto* L=It->GetLightComponent();UE_LOG(LogTemp,Display,TEXT("LIGHT_AUDIT %s pos=%s dir=%s intensity=%.1f visible=%d affects=%d"),*It->GetName(),*It->GetActorLocation().ToCompactString(),*It->GetActorForwardVector().ToCompactString(),L->Intensity,L->IsVisible(),L->bAffectsWorld);}}
            else if(Index==1){Hero()->SetActorLocation(Objects[TEXT("ammo_morgue")]->Stance+FVector(0,0,96));}
            else if(Index==2){Hero()->SetActorLocation(Objects[TEXT("annex_brief")]->Stance+FVector(0,0,96));}
            else if(Index==3)Show(TEXT("title"));
            else if(Index==4){Interface->GuidePage=0;Show(TEXT("training"));}
            else if(Index==5){Language=TEXT("kk");Show(TEXT("title"));}
            else if(Index==6){Language=TEXT("ru");Interface->Section=TEXT("graphics");Show(TEXT("settings"));}
            else if(Index==7){Flag(TEXT("map"),true);Show(TEXT("map"));}
            else if(Index==8){Interface->FilmOffset=FVector2D(72,-48);Interface->FilmTurn=1;Show(TEXT("xray"));}
            else if(Index==9){Items.Add(TEXT("badge"),1);Interface->SelectedItem=TEXT("badge");Show(TEXT("inspect"));}
            else if(Index==10)Show(TEXT("saves"));
            else if(Index>=11&&Index<=16){Show(TEXT(""));Flag(TEXT("weapon"),true);Flag(TEXT("shotgun"),true);Shotgun=Index>=14;Hero()->SetActorLocation(FVector(-1200,0,106));Hero()->UnCrouch();Hero()->Aiming=Index==12||Index==15;ReloadTime=Index==13||Index==16?30:0;}
            else if(Index==17){Show(TEXT(""));Flag(TEXT("weapon"),true);Shotgun=false;ReloadTime=0;Hero()->Rounds=9;Hero()->Aiming=true;Hero()->SetActorLocation(FVector(-1200,0,106));UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(Hero());}
            else if(Index>=18&&Index<=21){Show(TEXT(""));Flag(TEXT("weapon"),true);Shotgun=false;ReloadTime=0;Hero()->SetActorLocation(FVector(-1200,0,106));Hero()->Aiming=Index>=20;Hero()->WeaponAimBlend=Hero()->Aiming?1:0;Hero()->WeaponCloseBlend=0;}
            else if(Index==22||Index==23){Hero()->Aiming=false;Hero()->SetActorLocation(FVector(-1400,500,106));auto* View=GetWorld()->SpawnActor<ACameraActor>();const FVector At=Index==22?FVector(-440,-1000,190):FVector(-950,-400,300),Target=Index==22?FVector(-440,-1500,190):FVector(-700,-100,5);View->SetActorLocationAndRotation(At,(Target-At).Rotation());View->GetCameraComponent()->SetFieldOfView(65);UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(View);}
            else if(Index>=24&&Index<=27){for(auto Enemy:Enemies){Enemy->SetActorHiddenInGame(true);Enemy->SetActorTickEnabled(false);}auto* Enemy=Enemies[0].Get();Enemy->ResetEnemy();Enemy->Wake();Enemy->SetActorLocation(FVector(-600,0,96));Enemy->SetActorRotation(FRotator(0,180,0));Enemy->SetActorTickEnabled(false);Enemy->GetCharacterMovement()->StopMovementImmediately();Enemy->GetCharacterMovement()->DisableMovement();const TCHAR* Clip=Index==25?TEXT("A_StalkerWalk"):Index==26?TEXT("A_StalkerAttack"):TEXT("A_StalkerIdle");Enemy->GetMesh()->PlayAnimation(LoadObject<UAnimSequence>(nullptr,*(FString(TEXT("/Game/Campaign/Creatures/"))+Clip)),true);if(Index==27){Enemy->Dead=true;Enemy->Animate(.016f);}auto* View=GetWorld()->SpawnActor<ACameraActor>();const FVector At(-1020,120,150),Target(-600,0,100);View->SetActorLocationAndRotation(At,(Target-At).Rotation());View->GetCameraComponent()->SetFieldOfView(52);UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(View);}
            else {
                Json Audit=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Bad;int32 Count=0;
                auto Validate=[&](const FString& Path){++Count;auto* M=LoadObject<UMaterialInterface>(nullptr,*Path);const auto* Resource=M?M->GetMaterialResource(GMaxRHIShaderPlatform):nullptr;const auto* Map=Resource?Resource->GetGameThreadShaderMap():nullptr;bool Valid=Map&&Map->IsValidForRendering();
#if WITH_EDITOR
                    Valid=Valid&&Resource->GetCompileErrors().IsEmpty();
#endif
                    if(!Valid)Bad.Add(MakeShared<FJsonValueString>(Path));};
                for(const auto& Row:Array(Data,TEXT("materials")))Validate(MaterialPath(String(Row->AsObject(),TEXT("id"))));for(const TCHAR* FX:{TEXT("M_FXBrass"),TEXT("M_FXChip"),TEXT("M_FXBottle"),TEXT("M_BulletMark")})Validate(FString(TEXT("/Game/Campaign/Materials/"))+FX);
                Validate(TEXT("/Game/Campaign/Creatures/M_Stalker"));
                Audit->SetBoolField(TEXT("success"),Bad.IsEmpty());Audit->SetNumberField(TEXT("materials_checked"),Count);Audit->SetArrayField(TEXT("failed"),Bad);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/shader_validation.json"),Audit);FPlatformMisc::RequestExit(false);return;
            }
            const float Yaw=Index==0||Index>=11?5:160;Hero()->SetActorRotation(FRotator(0,Yaw,0));if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetControlRotation(FRotator(-12,Yaw,0));
            if(Index>=11&&Index<=16){auto* View=GetWorld()->SpawnActor<ACameraActor>();const FVector Target=Hero()->GetActorLocation()+FVector(10,0,20),At=Target+FVector(160,180,35);View->SetActorLocationAndRotation(At,(Target-At).Rotation());View->GetCameraComponent()->SetFieldOfView(55);UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(View);}
            if(Index>=18&&Index<=21){UGameplayStatics::GetPlayerController(this,0)->SetControlRotation(FRotator(Index==20?-50:Index==21?45:0,5,0));auto* View=GetWorld()->SpawnActor<ACameraActor>();View->GetCameraComponent()->SetFieldOfView(40);UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(View);}
        }
        if(Index>=11){if(Index==13||Index==16){ReloadTime=Index==13?.95f:.35f;ShotgunReload=Index==16;}TickHero(D);if(Index==12||Index==15||Index==17)Hero()->Aiming=true;}
        if(Index>=18&&Index<=21){const FVector Target=Hero()->WeaponPose().TransformPosition(FVector(0,0,-6)),Offset=Index==18?FVector(100,100,30):Index==19?FVector(0,130,10):Index==20?FVector(30,90,70):FVector(50,110,10);auto* View=UGameplayStatics::GetPlayerController(this,0)->GetViewTarget();View->SetActorLocationAndRotation(Target+Offset,(-Offset).Rotation());}
        if(Index==26&&Enemies[0]->GetMesh()->GetSingleNodeInstance())Enemies[0]->GetMesh()->GetSingleNodeInstance()->SetPosition(.45f,false);
        static bool Fired=false;if(Index==17&&Since>1.5f&&!Fired){Fired=true;FireWeapon();}
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WardCampaignAgent")))
    {
        if(!Agent&&Time<3){const bool Resume=FParse::Param(FCommandLine::Get(),TEXT("WardAgentResume"));SaveFolderOverride=FPaths::ProjectSavedDir()/(Resume?FString(TEXT("CampaignAgentSaves")):FString(TEXT("CampaignAgentRuns/"))+FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")));if(Resume)LoadLatest();ToggleAgent();}
        static float ProgressTime=0;static FString Previous;ProgressTime+=D;
        if(ProgressTime>10||Previous!=AgentStatus)
        {
            Previous=AgentStatus;ProgressTime=0;Json P=MakeShared<FJsonObject>();P->SetNumberField(TEXT("time"),Time);P->SetNumberField(TEXT("elapsed"),Elapsed);P->SetNumberField(TEXT("index"),AgentIndex);P->SetStringField(TEXT("step"),AgentStatus);P->SetNumberField(TEXT("completed"),Completed.Num());P->SetNumberField(TEXT("health"),Hero()->Health);P->SetNumberField(TEXT("reserve"),Hero()->Reserve);P->SetNumberField(TEXT("rounds"),Hero()->Rounds);P->SetNumberField(TEXT("x"),Hero()->GetActorLocation().X);P->SetNumberField(TEXT("y"),Hero()->GetActorLocation().Y);P->SetNumberField(TEXT("z"),Hero()->GetActorLocation().Z);P->SetStringField(TEXT("notice"),Hero()->Notice);
            P->SetNumberField(TEXT("inventory_slots"),Inventory().Num());P->SetStringField(TEXT("save_directory"),SaveDirectory());P->SetNumberField(TEXT("path_count"),AgentPath.Num());if(AgentPath.Num())P->SetStringField(TEXT("next_point"),AgentPath[0].ToString());
            FHitResult Obstacle;FCollisionQueryParams Query(SCENE_QUERY_STAT(AgentAudit),false,Hero());if(AgentPath.Num()&&GetWorld()->LineTraceSingleByChannel(Obstacle,Hero()->GetActorLocation(),Hero()->GetActorLocation()+(AgentPath[0]-Hero()->GetActorLocation()).GetSafeNormal2D()*160,ECC_Visibility,Query)){P->SetStringField(TEXT("blocked_actor"),GetNameSafe(Obstacle.GetActor()));P->SetStringField(TEXT("blocked_component"),GetNameSafe(Obstacle.GetComponent()));if(auto* Door=Cast<AEWardCampaignObject>(Obstacle.GetActor())){P->SetStringField(TEXT("blocked_door"),Door->ObjectId);P->SetBoolField(TEXT("door_open"),Door->Opened);P->SetBoolField(TEXT("door_locked"),Door->Locked);}}
            Write(FPaths::ProjectSavedDir()/TEXT("Prototype/campaign_agent_progress.json"),P);UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_AGENT %.1f %d %s (%s)"),Time,AgentIndex,*AgentStatus,*Hero()->GetActorLocation().ToCompactString());
        }
        if(Won||Time>2400)
        {
            Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Won);R->SetBoolField(TEXT("resumed"),FParse::Param(FCommandLine::Get(),TEXT("WardAgentResume")));R->SetStringField(TEXT("save_directory"),SaveDirectory());R->SetNumberField(TEXT("simulated_seconds"),Time);R->SetNumberField(TEXT("elapsed"),Elapsed);R->SetNumberField(TEXT("agent_index"),AgentIndex);R->SetStringField(TEXT("step"),AgentStatus);R->SetNumberField(TEXT("completed"),Completed.Num());R->SetNumberField(TEXT("health"),Hero()->Health);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/campaign_agent.json"),R);FPlatformMisc::RequestExit(false);
        }
        return;
    }
    if(Time<2)return;static bool Done=false;if(Done)return;Done=true;
    Json R=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Checks;bool All=true;
    auto Check=[&](const FString& Name,bool OK){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("check"),Name);C->SetBoolField(TEXT("passed"),OK);Checks.Add(MakeShared<FJsonValueObject>(C));All&=OK;UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_CHECK %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),*Name);};
    Check(TEXT("54 authored regions"),Array(Data,TEXT("rooms")).Num()==54);Check(TEXT("199 gameplay objects"),Objects.Num()==199);Check(TEXT("25 native enemies"),Enemies.Num()==25);Check(TEXT("navigation exported"),Grid.Ground.Num()>15000);
    FWardCampaignGrid Test;for(int32 X=0;X<7;++X)for(int32 Y=0;Y<7;++Y)Test.Ground.Add(FIntPoint(X,Y),0);Test.Blocked.Add(FIntPoint(1,0));auto Path=Test.FindPath(FVector(0,0,96),FVector(200,0,96));Check(TEXT("A* shortest detour, no wall crossing"),Path.Num()==4);
    const auto A=Test.Tactical(FIntPoint(0,0),FIntPoint(4,4),2,true);const int32 Pruned=Test.TacticalNodes;const auto B=Test.Tactical(FIntPoint(0,0),FIntPoint(4,4),2,false);Check(TEXT("Minimax alpha-beta agrees with exhaustive search"),A==B&&Pruned<=Test.TacticalNodes);
    TMap<FString,TArray<FVector>> Domains;Domains.Add(TEXT("a"),{FVector(0,0,0),FVector(500,0,0)});Domains.Add(TEXT("b"),{FVector(0,0,0)});TMap<FString,FVector> Allocation;Check(TEXT("CSP MRV finds separated solution"),Test.Allocate(Domains,Allocation,300)&&Allocation.Num()==2&&FVector::Dist(Allocation[TEXT("a")],Allocation[TEXT("b")])>=300);Domains[TEXT("a")]={FVector::ZeroVector};Check(TEXT("CSP rejects impossible allocation"),!Test.Allocate(Domains,Allocation,300));Check(TEXT("campaign CSP solved"),Flag(TEXT("csp_solved")));
    Check(TEXT("nine save slots"),SaveSlots().Num()==9);Check(TEXT("RU KK EN catalog"),Translations.Num()>1000);
    Open(TEXT("quarantine_gate"),false,true);UpdateBlocked();Check(TEXT("A* blocks full six-metre quarantine shutter"),Grid.Blocked.Contains(Grid.Cell(Objects[TEXT("quarantine_gate")]->GetActorLocation()+FVector(0,200,0)))&&Grid.Blocked.Contains(Grid.Cell(Objects[TEXT("quarantine_gate")]->GetActorLocation()+FVector(0,-200,0))));
    const FVector Start=Hero()->GetActorLocation();Take(TEXT("weapon"));Take(TEXT("badge"));Flag(TEXT("badge"),true);Interface->PendingCode=TEXT("417");Command(TEXT("keypad_submit"));Check(TEXT("keypad opens quarantine"),Objects.Contains(TEXT("quarantine_gate"))&&!Objects[TEXT("quarantine_gate")]->Locked&&Objects[TEXT("quarantine_gate")]->Opened);
    Interface->FilmOffset=FVector2D::ZeroVector;Interface->FilmTurn=0;Command(TEXT("xray_submit"));Command(TEXT("treatment_cool"));Command(TEXT("treatment_dose"));Command(TEXT("treatment_release"));Check(TEXT("medical puzzle progression"),Flag(TEXT("xray"))&&Flag(TEXT("diagnostics_complete")));
    SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("CampaignSmokeSaves");Hero()->Health=37;Hero()->Reserve=11;Hero()->Rounds=4;Flag(TEXT("autosave_pending"),false);Json S=Capture();Check(TEXT("valid native snapshot"),ValidateSave(S));Hero()->Health=100;Check(TEXT("restore exact health and ammunition"),Restore(S)&&Hero()->Health==37&&Hero()->Reserve==11&&Hero()->Rounds==4);
    Check(TEXT("save atomic slot"),Save(TEXT("manual_6")));Hero()->Health=90;Check(TEXT("latest save reload preserves health"),LoadLatest()&&Hero()->Health==37);
    Invulnerable=0;DamageHero(1000,nullptr);Check(TEXT("death schedules latest checkpoint respawn"),PendingRespawn&&Hero()->Dead);PendingRespawn=false;Restore(S);
    ToggleAdmin();Check(TEXT("independent admin camera"),Admin&&AdminCamera&&Hero()->GetActorLocation().Equals(Start,5));Check(TEXT("admin cannot save"),!Save(TEXT("manual_2")));ToggleAdmin();
    Check(TEXT("reject truncated save"),!ValidateSave(MakeShared<FJsonObject>()));NewGame(TEXT("normal"));Show(TEXT(""));
    const int32 OriginalReserve=Hero()->Reserve;Drop(TEXT("ammo"));Check(TEXT("discarded ammunition remains in world"),Hero()->Reserve==0&&Objects.Contains(TEXT("dropped_1")));
    Json Dropped=Capture();Restore(Dropped);Check(TEXT("discarded supplies survive save restore"),Objects.Contains(TEXT("dropped_1"))&&!Objects[TEXT("dropped_1")]->Used);
    Use(Objects[TEXT("dropped_1")]);Check(TEXT("collecting dropped supplies restores exact amount"),Hero()->Reserve==OriginalReserve&&Objects[TEXT("dropped_1")]->Used);
    Flash();Check(TEXT("flash uses its own charge"),FlashCharges==0);Take(TEXT("battery"));Battery();Check(TEXT("battery recharges flash"),FlashCharges==1&&Items.FindRef(TEXT("battery"))==0);
    NewGame(TEXT("normal"));Flag(TEXT("weapon"),true);Flag(TEXT("shotgun"),true);Items.Add(TEXT("shells"),9);Hero()->Reserve=25;Items.Add(TEXT("medkit"),1);Items.Add(TEXT("battery"),1);Check(TEXT("inventory counts weapons and ammunition stacks"),Inventory().Num()==9&&CanTake(TEXT("fuse")));NewGame(TEXT("normal"));
    // Isolated quest state integration. Physical traversal is tested by WardCampaignAgent.
    auto Action=[&](const FString& Id){if(Objects.Contains(Id))Use(Objects[Id],true);else Check(TEXT("required quest object: ")+Id,false);};
    Action(TEXT("weapon"));Action(TEXT("fuse"));Action(TEXT("power"));Action(TEXT("badge"));
    for(const FString Department:{FString(TEXT("morgue")),FString(TEXT("lab")),FString(TEXT("theatre"))})
    {
        const auto& Circuit=Array(Object(Data,TEXT("circuits")),Department);for(int32 I=0;I<3;++I){auto* O=Objects[FString::Printf(TEXT("dial_%s_%d"),*Department,I)].Get();for(int32 N=0;N<4&&O->Value!=(int32)Circuit[I]->AsNumber();++N)Use(O);}
        if(Department==TEXT("theatre"))UGameplayStatics::ApplyDamage(Enemies[5],1000,nullptr,Hero(),nullptr);Action(TEXT("relay_")+Department);
    }
    Check(TEXT("three department circuits unlock lower sector"),Completed.Contains(TEXT("relay_lab"))&&Completed.Contains(TEXT("relay_morgue"))&&Completed.Contains(TEXT("relay_theatre"))&&!Objects[TEXT("technical_gate")]->Locked);
    Action(TEXT("fuel"));Action(TEXT("coolant"));Action(TEXT("reactor"));Hero()->SetActorLocation(Objects[TEXT("reactor")]->Stance+FVector(0,0,96));TickQuests(16);const float Halted=ReactorTime;TickQuests(2);Check(TEXT("reactor waits for real fault service"),Fault==TEXT("purge_valve_a")&&FMath::IsNearlyEqual(Halted,ReactorTime));Action(TEXT("purge_valve_a"));TickQuests(15);Action(TEXT("purge_valve_b"));TickQuests(20);Check(TEXT("reactor requires both valves"),Flag(TEXT("purge_complete")));
    Action(TEXT("residence_panel"));Action(TEXT("residence_prybar"));Action(TEXT("residence_panel"));Action(TEXT("res_west_service"));Action(TEXT("residence_panel"));
    TArray<UPrimitiveComponent*> HitShapes;Objects[TEXT("residence_lock")]->GetComponents(HitShapes);bool CanHit=false;for(auto* C:HitShapes)CanHit|=C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision&&C->GetCollisionResponseToChannel(ECC_Visibility)==ECR_Block;Check(TEXT("residential lock has a weapon hit shape"),CanHit);
    Objects[TEXT("residence_lock")]->Break();Action(TEXT("residence_core"));Action(TEXT("residence_plate"));Action(TEXT("residence_lift"));Check(TEXT("residential power and repair chain completes"),Flag(TEXT("res_complete")));
    Action(TEXT("chapter_xray"));Interface->FilmOffset=FVector2D::ZeroVector;Interface->FilmTurn=0;Command(TEXT("xray_submit"));Action(TEXT("chapter_observe"));Action(TEXT("chapter_treatment"));Command(TEXT("treatment_cool"));Command(TEXT("treatment_dose"));Command(TEXT("treatment_release"));Check(TEXT("diagnostics opens isolation"),Flag(TEXT("diagnostics_complete")));
    Action(TEXT("isolation_case"));Action(TEXT("isolation_light"));Action(TEXT("isolation_coil_light"));Action(TEXT("isolation_tag"));Action(TEXT("isolation_generator"));TickQuests(8);Action(TEXT("isolation_vent_a"));TickQuests(9);Action(TEXT("isolation_vent_b"));TickQuests(9);Action(TEXT("isolation_log"));Action(TEXT("isolation_release"));Check(TEXT("isolation generator and log chain completes"),Flag(TEXT("isolation_complete")));
    for(const TCHAR* Id:{TEXT("brief"),TEXT("wheel"),TEXT("manual"),TEXT("intake"),TEXT("drain"),TEXT("pump")})Action(FString(TEXT("annex_"))+Id);
    const TCHAR* DialIds[]={TEXT("annex_dial_a"),TEXT("annex_dial_b"),TEXT("annex_dial_c")};const int32 Wanted[]={2,1,3};for(int32 I=0;I<3;++I)for(int32 N=0;N<4&&Objects[DialIds[I]]->Value!=Wanted[I];++N)Action(DialIds[I]);
    for(const TCHAR* Id:{TEXT("boiler"),TEXT("key"),TEXT("register"),TEXT("files"),TEXT("antenna")})Action(FString(TEXT("annex_"))+Id);Action(TEXT("annex_frequency"));Action(TEXT("annex_frequency"));Action(TEXT("annex_radio"));Action(TEXT("annex_release"));Check(TEXT("annex pump boiler archive and radio chain completes"),Flag(TEXT("annex_complete")));
    Action(TEXT("terminal"));Action(TEXT("chapter_final_reset"));Action(TEXT("chapter_exit_crank"));Check(TEXT("final evacuation requires terminal reset and crank"),Flag(TEXT("terminal"))&&Flag(TEXT("breaker"))&&Flag(TEXT("exit_released"))&&EscapeTime>0);
    NewGame(TEXT("normal"));Show(TEXT(""));CheckRegression(Check);
    R->SetBoolField(TEXT("success"),All);R->SetArrayField(TEXT("checks"),Checks);R->SetNumberField(TEXT("count"),Checks.Num());Write(FPaths::ProjectSavedDir()/TEXT("Prototype/campaign_smoke.json"),R);UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_SMOKE_FINISHED %s"),All?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExit(false);
}
