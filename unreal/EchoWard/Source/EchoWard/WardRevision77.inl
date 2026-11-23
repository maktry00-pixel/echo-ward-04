#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardGameplay76.h"
#include "WardBookWorld.h"
#include "WardHospitalGameplay.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
using namespace WardData;
void TickRevision77(AEWardCampaignMode& G,float Delta)
{
 static int Step=0;static double Last=0;static TArray<TSharedPtr<FJsonValue>> Checks;static Json Poses=MakeShared<FJsonObject>();static float StandHip=0,StandFloor=0;static ACameraActor* Side=nullptr;static int JournalCount=0;
 const double Now=FPlatformTime::Seconds();if(Now-Last<1.0)return;Last=Now;auto* H=G.Hero();auto* S=G.HospitalStory.Get();auto* PC=G.GetWorld()->GetFirstPlayerController();if(!H||!S||!PC||!G.BookWorld)return;
 auto Check=[&](FString Name,bool Pass){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("id"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));UE_LOG(LogTemp,Display,TEXT("REVISION77 %s %s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));};
 auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/Revision77")/(Name+TEXT(".png")),true,false);};
 auto Pose=[&](const FString& Id)
 {
  auto P=MakeShared<FJsonObject>();const float Floor=H->GetActorLocation().Z-H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();P->SetNumberField(TEXT("floor"),Floor);P->SetNumberField(TEXT("capsule_half"),H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());P->SetNumberField(TEXT("eye_height"),H->Camera->GetComponentLocation().Z-Floor);
  for(const TCHAR* Bone:{TEXT("pelvis"),TEXT("head"),TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l"),TEXT("thigh_r"),TEXT("calf_r"),TEXT("foot_r")}){const auto V=H->BekaBody->GetSocketLocation(Bone);P->SetArrayField(Bone,{MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)});}
  if(H->BekaOutfit)P->SetNumberField(TEXT("outfit_hip"),H->BekaOutfit->GetSocketLocation(TEXT("pelvis")).Z);
  Poses->SetObjectField(Id,P);return P;
 };
 if(Step==0)
 {
  G.Assessment=true;G.SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("Revision77TestSaves");G.NewGame(TEXT("normal"));G.Show(TEXT(""));S->SetActorTickEnabled(false);S->FreezeAI=true;S->StartHome52();G.Show(TEXT(""));
  WardGameplay76::Tick(G,6);Check(TEXT("home_has_no_text_message"),!G.Flag(TEXT("phone76_received"))&&!G.PhoneOpen76);S->TickHome52(5);
  Check(TEXT("home_phone_rings"),S->HomeStage52==0&&IsValid(S->HomeAudio53));H->SetActorLocation(S->HomePhone52+FVector(-80,0,5),false);S->UseHome52();Check(TEXT("F_answers_real_home_voice_call"),S->HomeStage52==1&&!S->HomeSubtitle69.IsEmpty()&&IsValid(S->HomeAudio53));H->Controller->SetControlRotation(FRotator(-20,0,0));Shot(TEXT("home_call"));++Step;return;
 }
 if(Step==1)
 {
  WardGameplay76::Skip(G,true);S->TickHome52(.1f);Check(TEXT("call_finishes_before_home_exit"),S->HomeStage52==2&&G.Completed.Contains(TEXT("HOME_INVITATION52"))&&!G.Flag(TEXT("phone76_received")));
  H->SetActorLocation(FVector(-80000,-80260,98),false);S->UseHome52(true);Check(TEXT("home_exit_starts_arrival"),S->HomeStage52<0&&S->ArrivalStage==0);
  S->ArrivalClock=S->DriveDuration53;S->TickArrival(0);S->ArrivalInteract();S->ActiveScene35.Empty();G.Show(TEXT(""));S->Seated31=false;S->ArrivalStage=2;
  const FVector C=Vector(Array(S->ArrivalData,TEXT("checkpoint")))*100;H->SetActorLocation(C+FVector(0,-1500,98),false);WardGameplay76::Tick(G,.1f);Check(TEXT("no_warning_far_from_checkpoint"),!G.Flag(TEXT("phone76_received")));
  H->SetActorLocation(C+FVector(0,-350,98),false);WardGameplay76::Tick(G,.1f);Check(TEXT("warning_arrives_before_checkpoint"),G.Flag(TEXT("phone76_received"))&&G.PhoneOpen76&&S->ArrivalStage==2);WardGameplay76::Tick(G,.5f);WardGameplay76::Tick(G,0);++Step;return;
 }
 if(Step==2)
 {
  int Icons=0;for(const TCHAR* Id:{TEXT("messages"),TEXT("insta"),TEXT("weather"),TEXT("calls"),TEXT("notes"),TEXT("settings")})Icons+=G.Interface->TitleActions.Contains(TEXT("phone76_")+FString(Id));Check(TEXT("six_clickable_vector_app_icons"),Icons==6&&G.Screen==TEXT("phone76"));Shot(TEXT("phone_apps"));++Step;return;
 }
 if(Step==3){G.Command(TEXT("phone76_messages"));G.Command(TEXT("phone76_aruzhan"));Check(TEXT("warning_chat_marks_read"),G.Flag(TEXT("phone76_read")));JournalCount=G.Journal.Num();PC->InputKey(FInputKeyParams(EKeys::F,IE_Pressed,1.0));Shot(TEXT("phone_warning"));++Step;return;}
 if(Step==4)
 {
  Check(TEXT("physical_F_key_sends_checkpoint_reply"),G.Flag(TEXT("phone76_sent")));G.Command(TEXT("phone76_send"));Check(TEXT("reply_cannot_repeat"),G.Journal.Num()==JournalCount+1);Shot(TEXT("phone_sent"));G.Command(TEXT("phone76_close"));Check(TEXT("phone_restores_game_controls"),G.Screen.IsEmpty()&&!PC->bShowMouseCursor);
  G.Options->SetBoolField(TEXT("inspection_bright"),true);G.ApplySettings(false);Check(TEXT("old_bright_setting_is_ignored_for_player"),!G.SettingBool(TEXT("inspection_bright"))&&!G.BookWorld->Bright);G.BookWorld->SetBright(true);Check(TEXT("world_cannot_enable_bright_for_player"),!G.BookWorld->Bright);
  G.Options->SetBoolField(TEXT("inspection_bright"),false);G.CycleSetting(TEXT("inspection_bright"));Check(TEXT("player_setting_cannot_enable_bright"),!G.SettingBool(TEXT("inspection_bright"))&&!G.BookWorld->Bright);G.ToggleAdmin();G.CycleSetting(TEXT("inspection_bright"));Check(TEXT("administrator_can_enable_bright"),G.Admin&&G.BookWorld->Bright);G.ToggleAdmin();Check(TEXT("admin_exit_forces_horror_lighting"),!G.Admin&&!G.BookWorld->Bright&&!G.SettingBool(TEXT("inspection_bright")));
  S->HomeStage52=-1;S->SetHomeVisible53(false);S->ArrivalStage=5;S->ApplyArrivalState();S->ActiveScene35.Empty();G.Show(TEXT(""));for(auto M:S->Mobs)if(M){M->SetActorTickEnabled(false);M->SetActorHiddenInGame(true);}
  H->UnCrouch();H->GetCharacterMovement()->UnCrouch(false);H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);H->SetActorLocation(G.BookWorld->Start(),false);H->SetActorHiddenInGame(false);PC->SetControlRotation(FRotator(0,0,0));H->UpdateFirstPerson50();
  Side=G.GetWorld()->SpawnActor<ACameraActor>();Side->SetActorLocationAndRotation(H->GetActorLocation()+FVector(235,-245,30),(H->GetActorLocation()+FVector(0,0,-5)-(H->GetActorLocation()+FVector(235,-245,30))).Rotation());Side->GetCameraComponent()->SetFieldOfView(43);Side->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;Side->GetCameraComponent()->PostProcessBlendWeight=1;PC->SetViewTarget(Side);
  auto* L=NewObject<UPointLightComponent>(Side);L->SetupAttachment(Side->GetRootComponent());L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(7000);L->SetAttenuationRadius(800);L->SetLightingChannels(true,true,false);L->SetCastShadows(false);L->RegisterComponent();++Step;return;
 }
 if(Step==5){const auto P=Pose(TEXT("standing"));StandHip=H->BekaBody->GetSocketLocation(TEXT("pelvis")).Z;StandFloor=Number(P,TEXT("floor"));Shot(TEXT("standing_side"));++Step;return;}
 if(Step==6){H->Crouch();H->GetCharacterMovement()->Crouch(false);++Step;return;}
 if(Step==7)
 {
  const auto P=Pose(TEXT("crouching"));const float Hip=H->BekaBody->GetSocketLocation(TEXT("pelvis")).Z,Floor=Number(P,TEXT("floor"));
  Check(TEXT("crouch_pelvis_drops_45_to_70_cm"),StandHip-Hip>45&&StandHip-Hip<70);Check(TEXT("crouch_capsule_is_116_cm_high"),H->bIsCrouched&&FMath::IsNearlyEqual(H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),58.f,1));Check(TEXT("crouch_camera_stays_inside_capsule"),Number(P,TEXT("eye_height"))<113&&Number(P,TEXT("eye_height"))>90);
  Check(TEXT("camera_and_head_remain_aligned"),FMath::Abs(H->Camera->GetComponentLocation().Z-H->BekaBody->GetSocketLocation(TEXT("head")).Z)<30);
  bool Feet=true,Knees=true;for(const FString SideName:{FString(TEXT("l")),FString(TEXT("r"))}){const FVector A=H->BekaBody->GetSocketLocation(FName(*(TEXT("thigh_")+SideName))),B=H->BekaBody->GetSocketLocation(FName(*(TEXT("calf_")+SideName))),C=H->BekaBody->GetSocketLocation(FName(*(TEXT("foot_")+SideName)));const float Angle=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct((A-B).GetSafeNormal(),(C-B).GetSafeNormal()),-1.,1.)));Feet&=C.Z-Floor>-3&&C.Z-Floor<19;Knees&=Angle>45&&Angle<150;}
  Check(TEXT("both_feet_stay_on_physical_floor"),Feet&&FMath::Abs(StandFloor-Floor)<4);Check(TEXT("both_knees_visibly_bend"),Knees);Check(TEXT("clothing_follows_crouched_body"),H->BekaOutfit&&FMath::Abs(H->BekaOutfit->GetSocketLocation(TEXT("pelvis")).Z-Hip)<1);Shot(TEXT("crouching_side"));++Step;return;
 }
 if(Step==8){PC->SetViewTarget(H);PC->SetControlRotation(FRotator(-65,0,0));H->UpdateFirstPerson50();H->Arm->TickComponent(0,LEVELTICK_All,nullptr);const float Ahead=FVector::DotProduct(H->Camera->GetComponentLocation()-H->BekaBody->GetSocketLocation(TEXT("head")),H->GetActorForwardVector());Check(TEXT("crouch_eye_is_in_front_of_collar"),Ahead>5);Shot(TEXT("crouching_first_person"));++Step;return;}
 if(Step==9){H->UnCrouch();H->GetCharacterMovement()->UnCrouch(false);++Step;return;}
 if(Step==10)
 {
  Pose(TEXT("standing_restored"));Check(TEXT("standing_body_restores_after_ctrl_release"),!H->bIsCrouched&&FMath::Abs(H->BekaBody->GetSocketLocation(TEXT("pelvis")).Z-StandHip)<4);PC->SetControlRotation(FRotator(-10,90,0));WardGameplay76::Tick(G,.1f);Shot(TEXT("hud_black_red"));++Step;return;
 }
 if(Step==11)
 {
  bool Success=true;for(auto C:Checks)Success&=Boolean(C->AsObject(),TEXT("passed"));auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Success);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);R->SetObjectField(TEXT("poses"),Poses);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/revision77_check.json"),R);UKismetSystemLibrary::QuitGame(&G,PC,EQuitPreference::Quit,false);++Step;
 }
}
