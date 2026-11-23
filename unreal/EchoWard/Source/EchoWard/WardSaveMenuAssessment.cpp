#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardTitleScene.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/LightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
using namespace WardData;
namespace
{
    void Pointer(TSharedPtr<SWidget> W,bool Enter){if(!W)return;const FPointerEvent E;if(Enter)W->OnMouseEnter(W->GetCachedGeometry(),E);else W->OnMouseLeave(E);}
    bool ContainsText(TSharedRef<SWidget> W,const FString& Text){if(W->GetType()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(Text))return true;auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)if(ContainsText(C->GetChildAt(I),Text))return true;return false;}
}
void AEWardCampaignMode::TickSaveMenuAssessment(float Delta)
{
    static int Step=0;static double Start=FPlatformTime::Seconds();static FVector InitialCamera,HeroAt;static FRotator InitialLamp;static float MinRed=1.e9f,MaxRed=0;static TArray<TSharedPtr<FJsonValue>> Checks;
    const float T=FPlatformTime::Seconds()-Start;auto* PC=UGameplayStatics::GetPlayerController(this,0);
    if(TitleScene&&TitleScene->SaveBackdropActive&&!TitleScene->SaveLights.IsEmpty()){MinRed=FMath::Min(MinRed,TitleScene->SaveLights.Last()->Intensity);MaxRed=FMath::Max(MaxRed,TitleScene->SaveLights.Last()->Intensity);}
    auto Check=[&](FString Name,bool Pass){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));};
    auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/SaveMenuCaptures/")+Name,true,false);};
    auto Enabled=[&](const TCHAR* Id){const auto W=Interface->TitleActions.FindRef(Id).Pin();return W&&W->IsEnabled();};
    if(Step==0)
    {
        SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("SaveMenuCheckSaves")/FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));NewGame(TEXT("story"));Show(TEXT(""));Hero()->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Hero()->Health=73;Elapsed=125;
        Check(TEXT("isolated_manual_seed_written"),Save(TEXT("manual_1")));Check(TEXT("isolated_auto_seed_written"),Save(TEXT("auto_2"),true));HeroAt=Hero()->GetActorLocation();CampaignStarted=false;Show(TEXT("title"));Show(TEXT("saves"));Hero()->Notice=TEXT("UI_TEST_YELLOW_HINT_SHOULD_NOT_RENDER");Hero()->NoticeTime=999;
        InitialCamera=TitleScene->SaveCamera->GetActorLocation();InitialLamp=TitleScene->SaveLampHinge->GetRelativeRotation();++Step;
    }
    else if(Step==1&&T>3)
    {
        Check(TEXT("save_page_uses_separate_live_camera"),TitleScene->SaveBackdropActive&&!TitleScene->GameBackdropActive&&PC->GetViewTarget()==TitleScene->SaveCamera&&FVector::Dist(TitleScene->Camera->GetActorLocation(),TitleScene->SaveCamera->GetActorLocation())>7000);
        Check(TEXT("camera_and_hanging_lamp_animate_during_pause"),!InitialCamera.Equals(TitleScene->SaveCamera->GetActorLocation(),.1f)&&!InitialLamp.Equals(TitleScene->SaveLampHinge->GetRelativeRotation(),.05f)&&FMath::IsNearlyEqual(Elapsed,125.f));
        Check(TEXT("red_light_changes_during_pause"),MaxRed-MinRed>5);
        Check(TEXT("blood_decals_belong_to_save_set"),TitleScene->SaveBloodDecals==13);
        bool NoSkeleton=true;TArray<USceneComponent*> SetChildren;TitleScene->SaveSet->GetChildrenComponents(true,SetChildren);for(auto* C:SetChildren)if(Cast<USkeletalMeshComponent>(C))NoSkeleton=false;
        Check(TEXT("save_background_has_no_characters_or_monsters"),NoSkeleton&&Hero()->IsHidden()&&!TitleScene->Nurse->IsVisible()&&(!TitleScene->GameZombie||!TitleScene->GameZombie->IsVisible())&&!TitleScene->PeekingHead->IsVisible()&&!TitleScene->RollingBall->IsVisible());
        Check(TEXT("six_manual_three_auto_and_sixteen_actions"),SaveSlots().Num()==9&&Interface->TitleActions.Num()==16);
        Check(TEXT("only_valid_slots_can_load"),Enabled(TEXT("load_manual_1"))&&!Enabled(TEXT("load_manual_2"))&&Enabled(TEXT("load_auto_2"))&&!Enabled(TEXT("load_auto_3")));
        Check(TEXT("saving_disabled_without_active_campaign"),!Enabled(TEXT("save_manual_1"))&&!Enabled(TEXT("save_manual_6")));
        Check(TEXT("yellow_gameplay_hint_absent"),Interface->Root.IsValid()&&!ContainsText(Interface->Root.ToSharedRef(),TEXT("UI_TEST_YELLOW_HINT_SHOULD_NOT_RENDER")));
        Shot(TEXT("saves_1080.png"));if(auto W=Interface->TitleActions.FindRef(TEXT("load_manual_1")).Pin())FSlateApplication::Get().SetKeyboardFocus(W);++Step;
    }
    else if(Step==2&&T>4){Check(TEXT("save_buttons_neutral_with_keyboard_focus"),Interface->TitleMotion[TEXT("load_manual_1")]()<.01f);Pointer(Interface->TitleActions.FindRef(TEXT("load_manual_1")).Pin(),true);++Step;}
    else if(Step==3&&T>5){Check(TEXT("save_button_red_on_pointer_hover"),Interface->TitleMotion[TEXT("load_manual_1")]()>.8f);Shot(TEXT("saves_hover_1080.png"));Pointer(Interface->TitleActions.FindRef(TEXT("load_manual_1")).Pin(),false);++Step;}
    else if(Step==4&&T>6){Check(TEXT("save_button_fades_after_pointer_leaves"),Interface->TitleMotion[TEXT("load_manual_1")]()<.05f);Command(TEXT("back"));Check(TEXT("title_save_back_returns_to_game_menu"),Screen==TEXT("game")&&TitleView&&TitleScene->GameBackdropActive&&!TitleScene->SaveBackdropActive);Command(TEXT("page_saves"));Interface->Close();++Step;}
    else if(Step==5&&T>8){Shot(TEXT("save_room_1080.png"));++Step;}
    else if(Step==6&&T>9){Interface->Open();Hero()->Health=24;Command(TEXT("load_manual_1"));Check(TEXT("load_uses_fade_from_quiet_room"),TitleScene->Transitioning&&Screen==TEXT("saves"));++Step;}
    else if(Step==7&&T>13){Check(TEXT("load_restores_health_and_player_camera"),Screen.IsEmpty()&&Hero()->Health>=73&&PC->GetViewTarget()==Hero()&&!Hero()->IsHidden()&&!TitleScene->Active&&!TitleScene->SaveBackdropActive);HeroAt=Hero()->GetActorLocation();Show(TEXT("pause"));Show(TEXT("saves"));Check(TEXT("in_game_saves_also_use_quiet_3d_room"),!TitleView&&TitleScene->SaveBackdropActive&&Hero()->IsHidden()&&Enabled(TEXT("save_manual_6")));Hero()->Health=61;Command(TEXT("save_manual_6"));Check(TEXT("manual_save_updates_slot_without_moving_hero"),ValidateSave(ReadSlot(TEXT("manual_6")))&&Number(ReadSlot(TEXT("manual_6")),TEXT("health"))==61&&HeroAt.Equals(Hero()->GetActorLocation(),1.f)&&Enabled(TEXT("load_manual_6"))&&!Interface->SaveFeedback.IsEmpty());++Step;}
    else if(Step==8&&T>14){Shot(TEXT("saves_ingame_1080.png"));Command(TEXT("back"));Check(TEXT("in_game_save_back_returns_to_pause"),Screen==TEXT("pause")&&!TitleView&&!TitleScene->SaveBackdropActive&&TitleScene->AboutBackdropActive&&Hero()->IsHidden()&&PC->GetViewTarget()==TitleScene->AboutCamera);Language=TEXT("kk");Show(TEXT("saves"));++Step;}
    else if(Step==9&&T>16){Shot(TEXT("saves_kk_1080.png"));Language=TEXT("en");Show(TEXT("saves"));++Step;}
    else if(Step==10&&T>18){Shot(TEXT("saves_en_1080.png"));++Step;}
    else if(Step==11&&T>20){Show(TEXT(""));Check(TEXT("save_set_hidden_after_resume"),!TitleScene->Active&&!TitleScene->SaveSet->IsVisible()&&PC->GetViewTarget()==Hero());Json R=MakeShared<FJsonObject>();bool Pass=true;for(auto J:Checks)Pass&=Boolean(J->AsObject(),TEXT("passed"));R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/save_menu_check.json"),R);FPlatformMisc::RequestExit(false);++Step;}
}
