#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardTitleScene.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/LightComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
using namespace WardData;
namespace {
bool SettingsText(TSharedRef<SWidget> W,const FString& S){if(W->GetType()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(S))return true;auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)if(SettingsText(C->GetChildAt(I),S))return true;return false;}
void SettingsPointer(TSharedPtr<SWidget> W,bool In){if(W){FPointerEvent E;if(In)W->OnMouseEnter(W->GetCachedGeometry(),E);else W->OnMouseLeave(E);}}
}
void AEWardCampaignMode::TickSettingsMenuAssessment(float Delta)
{
    static int Step=0;static double Since=FPlatformTime::Seconds();static FVector CameraAt,HeroAt;static float MinRed=1.e9f,MaxRed=0;static TArray<TSharedPtr<FJsonValue>> Checks;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);const double T=FPlatformTime::Seconds()-Since;auto Next=[&](){++Step;Since=FPlatformTime::Seconds();};
    auto Check=[&](FString N,bool B){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),N);J->SetBoolField(TEXT("passed"),B);Checks.Add(MakeShared<FJsonValueObject>(J));};
    auto Shot=[&](const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/SettingsCaptures/")+N+TEXT(".png"),true,false);};
    if(TitleScene&&TitleScene->TrainingBackdropActive&&!TitleScene->TrainingLights.IsEmpty()){MinRed=FMath::Min(MinRed,TitleScene->TrainingLights.Last()->Intensity);MaxRed=FMath::Max(MaxRed,TitleScene->TrainingLights.Last()->Intensity);}
    if(Step==0){Language=TEXT("ru");Show(TEXT("title"));Interface->Section=TEXT("graphics");Show(TEXT("settings"));Hero()->Notice=TEXT("SETTINGS_HINT_SENTINEL");Hero()->NoticeTime=999;CameraAt=TitleScene->TrainingCamera->GetActorLocation();Next();}
    else if(Step==1&&T>3)
    {
        Check(TEXT("settings_has_quiet_live_background"),TitleScene->TrainingBackdropActive&&PC->GetViewTarget()==TitleScene->TrainingCamera&&!TitleScene->GameBackdropActive&&Hero()->IsHidden());
        Check(TEXT("camera_and_light_animate_while_paused"),!CameraAt.Equals(TitleScene->TrainingCamera->GetActorLocation(),.1)&&MaxRed-MinRed>5);
        bool Empty=true;TArray<USceneComponent*> SetChildren;TitleScene->TrainingSet->GetChildrenComponents(true,SetChildren);for(auto* C:SetChildren)if(Cast<USkeletalMeshComponent>(C))Empty=false;
        Check(TEXT("no_menu_monsters_visible"),Empty&&!TitleScene->Nurse->IsVisible()&&(!TitleScene->GameZombie||!TitleScene->GameZombie->IsVisible()));
        Check(TEXT("four_tabs_and_real_sliders"),Interface->TitleActions.Contains(TEXT("section_audio"))&&Interface->TitleActions.Contains(TEXT("section_controls"))&&Interface->TitleActions.Contains(TEXT("section_interface"))&&Interface->TitleActions.Contains(TEXT("slider_brightness"))&&Interface->TitleActions.Contains(TEXT("slider_fov")));
        Check(TEXT("no_yellow_gameplay_hint"),!SettingsText(Interface->Root.ToSharedRef(),TEXT("SETTINGS_HINT_SENTINEL")));Shot(TEXT("settings_graphics_1080"));FSlateApplication::Get().SetKeyboardFocus(Interface->TitleActions[TEXT("setting_fullscreen")].Pin());Next();
    }
    else if(Step==2&&T>1){Check(TEXT("focus_does_not_turn_choice_red"),Interface->TitleMotion[TEXT("setting_fullscreen")]()<.01f);SettingsPointer(Interface->TitleActions[TEXT("setting_fullscreen")].Pin(),true);Next();}
    else if(Step==3&&T>1){Check(TEXT("choice_red_on_hover"),Interface->TitleMotion[TEXT("setting_fullscreen")]()>.8f);Shot(TEXT("settings_hover_1080"));Next();}
    else if(Step==4&&T>1){SettingsPointer(Interface->TitleActions[TEXT("setting_fullscreen")].Pin(),false);Next();}
    else if(Step==5&&T>1)
    {
        Check(TEXT("choice_neutral_after_pointer_leaves"),Interface->TitleMotion[TEXT("setting_fullscreen")]()<.05f);
        const FString Before=String(Options,TEXT("resolution"));CycleSetting(TEXT("resolution"));CycleSetting(TEXT("resolution"),-1);Check(TEXT("resolution_arrows_round_trip"),String(Options,TEXT("resolution"))==Before&&GEngine->GetGameUserSettings()->GetScreenResolution().X==1920);
        Options->SetNumberField(TEXT("graphics_preset"),1);CycleSetting(TEXT("graphics_preset"));Check(TEXT("performance_preset_really_uses_75_percent_tsr"),Setting(TEXT("screen_percentage"))==75&&FMath::IsNearlyEqual(IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"))->GetFloat(),75.f)&&IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"))->GetInt()==4);CycleSetting(TEXT("graphics_preset"),-1);CycleSetting(TEXT("msaa"));Check(TEXT("anti_aliasing_choice_really_disables_tsr"),IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"))->GetInt()==0);CycleSetting(TEXT("msaa"),-1);
        SetNumberSetting(TEXT("brightness"),1.2,false);SetNumberSetting(TEXT("fov"),999,false);Check(TEXT("slider_values_apply_and_clamp"),FMath::IsNearlyEqual(Setting(TEXT("brightness")),1.2)&&Setting(TEXT("fov"))==100&&Hero()->Camera->FieldOfView==100);SetNumberSetting(TEXT("fov"),60,false);SetNumberSetting(TEXT("brightness"),1,false);
        SetNumberSetting(TEXT("grain_amount"),0,false);Check(TEXT("grain_slider_can_disable_grain"),!SettingBool(TEXT("grain")));SetNumberSetting(TEXT("grain_amount"),.035,false);
        const bool Old=SettingBool(TEXT("motion_blur"),false);CycleSetting(TEXT("motion_blur"));Check(TEXT("motion_blur_switch_changes_real_option"),SettingBool(TEXT("motion_blur"))!=Old);CycleSetting(TEXT("motion_blur"));
        ApplySettings();const Json Saved=Read(FPaths::ProjectSavedDir()/TEXT("Config/CampaignOptions.json"));Check(TEXT("options_saved_to_disk"),Number(Saved,TEXT("fov"))==60&&String(Saved,TEXT("resolution"))==Before);
        Command(TEXT("section_audio"));Next();
    }
    else if(Step==6&&T>1){Check(TEXT("audio_controls_present"),Interface->TitleActions.Contains(TEXT("slider_music"))&&Interface->TitleActions.Contains(TEXT("slider_effects"))&&Interface->TitleActions.Contains(TEXT("setting_subtitles")));Shot(TEXT("settings_audio_1080"));Next();}
    else if(Step==7&&T>1){Command(TEXT("section_controls"));Next();}
    else if(Step==8&&T>1){Check(TEXT("mouse_settings_and_control_guide_retained"),Interface->TitleActions.Contains(TEXT("slider_sensitivity"))&&Interface->TitleActions.Contains(TEXT("setting_invert_y"))&&SettingsText(Interface->Root.ToSharedRef(),TEXT("F — подобрать")));Shot(TEXT("settings_controls_1080"));Next();}
    else if(Step==9&&T>1){Command(TEXT("section_interface"));Next();}
    else if(Step==10&&T>1){Check(TEXT("language_hud_and_admin_retained"),Interface->TitleActions.Contains(TEXT("setting_language"))&&Interface->TitleActions.Contains(TEXT("slider_hud_opacity"))&&Interface->TitleActions.Contains(TEXT("admin_enter"))&&Interface->TitleActions.Contains(TEXT("admin_password")));Shot(TEXT("settings_game_1080"));Next();}
    else if(Step==11&&T>1){Interface->PendingCode=TEXT("invalid");Command(TEXT("admin_enter"));Check(TEXT("wrong_admin_password_has_visible_feedback"),!Admin&&SettingsText(Interface->Root.ToSharedRef(),Tr(TEXT("Неверный пароль"))));Options->SetStringField(TEXT("language"),TEXT("kk"));ApplySettings(false);Interface->Refresh();Next();}
    else if(Step==12&&T>1){Check(TEXT("kazakh_copy_visible"),SettingsText(Interface->Root.ToSharedRef(),Tr(TEXT("Назад"))));Shot(TEXT("settings_kk_1080"));Next();}
    else if(Step==13&&T>1){Options->SetStringField(TEXT("language"),TEXT("en"));ApplySettings(false);Interface->Refresh();Next();}
    else if(Step==14&&T>1){Check(TEXT("english_copy_visible"),SettingsText(Interface->Root.ToSharedRef(),TEXT("Back")));Shot(TEXT("settings_en_1080"));Next();}
    else if(Step==15&&T>1){Command(TEXT("back"));Check(TEXT("back_restores_main_menu"),Screen==TEXT("title")&&!TitleScene->TrainingBackdropActive&&PC->GetViewTarget()==TitleScene->Camera);Options->SetStringField(TEXT("language"),TEXT("ru"));ApplySettings(false);Show(TEXT("settings"));Interface->Close();Next();}
    else if(Step==16&&T>2){Shot(TEXT("settings_room_1080"));Next();}
    else if(Step==17&&T>1){NewGame(TEXT("story"));Show(TEXT(""));HeroAt=Hero()->GetActorLocation();Show(TEXT("pause"));Show(TEXT("settings"));Command(TEXT("back"));Check(TEXT("settings_back_restores_live_pause_without_moving_hero"),Screen==TEXT("pause")&&Hero()->IsHidden()&&TitleScene->AboutBackdropActive&&PC->GetViewTarget()==TitleScene->AboutCamera&&HeroAt.Equals(Hero()->GetActorLocation(),1));Show(TEXT("settings"));Command(TEXT("settings_reset"));Check(TEXT("reset_restores_defaults"),FMath::IsNearlyEqual(Setting(TEXT("brightness")),1.)&&Setting(TEXT("max_fps"))==60&&Setting(TEXT("screen_percentage"))==100);Show(TEXT(""));bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/settings_menu_check.json"),R);FPlatformMisc::RequestExit(false);Next();}
}
