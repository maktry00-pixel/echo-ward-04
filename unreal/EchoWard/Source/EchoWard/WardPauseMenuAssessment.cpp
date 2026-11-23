#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardTitleScene.h"
#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/LightComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
using namespace WardData;
namespace {
bool PauseText(TSharedRef<SWidget> W,const FString& S){if(W->GetType()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(S))return true;auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)if(PauseText(C->GetChildAt(I),S))return true;return false;}
void PausePointer(TSharedPtr<SWidget> W,bool In){if(W){FPointerEvent E;if(In)W->OnMouseEnter(W->GetCachedGeometry(),E);else W->OnMouseLeave(E);}}
}
void AEWardCampaignMode::TickPauseMenuAssessment(float Delta)
{
    static int Step=0;static double Since=FPlatformTime::Seconds();static FVector At,HeroAt,EnemyAt;static float BeforeElapsed=0,Min=1.e9f,Max=0;static FRotator Lamp;static TArray<TSharedPtr<FJsonValue>> Checks;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);const double T=FPlatformTime::Seconds()-Since;auto Next=[&](){++Step;Since=FPlatformTime::Seconds();};
    auto Check=[&](FString N,bool B){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),N);J->SetBoolField(TEXT("passed"),B);Checks.Add(MakeShared<FJsonValueObject>(J));};
    auto Shot=[&](const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/PauseCaptures/")+N+TEXT(".png"),true,false);};
    if(TitleScene&&TitleScene->AboutBackdropActive&&!TitleScene->AboutLights.IsEmpty()){Min=FMath::Min(Min,TitleScene->AboutLights.Last()->Intensity);Max=FMath::Max(Max,TitleScene->AboutLights.Last()->Intensity);}
    if(Step==0){Language=TEXT("ru");NewGame(TEXT("normal"));Show(TEXT(""));Hero()->Health=73;Hero()->Rounds=5;HeroAt=Hero()->GetActorLocation();EnemyAt=Enemies[0]->GetActorLocation();BeforeElapsed=Elapsed;Hero()->Pause();Hero()->Notice=TEXT("PAUSE_YELLOW_SENTINEL");Hero()->NoticeTime=999;At=TitleScene->AboutCamera->GetActorLocation();Lamp=TitleScene->AboutLampHinge->GetRelativeRotation();Next();}
    else if(Step==1&&T>3)
    {
        Check(TEXT("escape_opens_live_pause"),Screen==TEXT("pause")&&!TitleView&&TitleScene->AboutBackdropActive&&PC->GetViewTarget()==TitleScene->AboutCamera);
        Check(TEXT("gameplay_and_enemy_frozen"),FMath::IsNearlyEqual(Elapsed,BeforeElapsed)&&EnemyAt.Equals(Enemies[0]->GetActorLocation(),.1f)&&HeroAt.Equals(Hero()->GetActorLocation(),.1f));
        Check(TEXT("camera_and_lamp_animate"),!At.Equals(TitleScene->AboutCamera->GetActorLocation(),.1f)&&!Lamp.Equals(TitleScene->AboutLampHinge->GetRelativeRotation(),.05f));Check(TEXT("red_light_pulses"),Max-Min>5);Check(TEXT("blood_on_walls_and_floor"),TitleScene->AboutBloodDecals==13);
        bool Empty=true;TArray<USceneComponent*> SetChildren;TitleScene->AboutSet->GetChildrenComponents(true,SetChildren);for(auto* C:SetChildren)if(Cast<USkeletalMeshComponent>(C))Empty=false;
        Check(TEXT("no_monsters_or_hero_in_background"),Empty&&!TitleScene->Nurse->IsVisible()&&(!TitleScene->GameZombie||!TitleScene->GameZombie->IsVisible())&&!TitleScene->PeekingHead->IsVisible()&&Hero()->IsHidden());
        Check(TEXT("five_navigation_buttons_and_resume"),Interface->TitleActions.Num()==6&&Interface->TitleActions.Contains(TEXT("page_title"))&&Interface->TitleActions.Contains(TEXT("resume")));
        bool OneRow=true;const float Y=Interface->TitleActions[TEXT("page_title")].Pin()->GetCachedGeometry().GetAbsolutePosition().Y;for(const TCHAR* Id:{TEXT("page_settings"),TEXT("page_training"),TEXT("page_about"),TEXT("page_quit")})OneRow&=FMath::Abs(Interface->TitleActions[Id].Pin()->GetCachedGeometry().GetAbsolutePosition().Y-Y)<2;
        Check(TEXT("navigation_stays_in_one_row"),OneRow);Check(TEXT("game_maps_online_removed"),!Interface->TitleActions.Contains(TEXT("page_game"))&&!Interface->TitleActions.Contains(TEXT("page_maps"))&&!Interface->TitleActions.Contains(TEXT("page_online")));
        Check(TEXT("yellow_notice_not_rendered"),!PauseText(Interface->Root.ToSharedRef(),TEXT("PAUSE_YELLOW_SENTINEL")));bool Neutral=true;for(const auto& E:Interface->TitleMotion)Neutral&=E.Value()<.01f;Check(TEXT("all_buttons_neutral_without_pointer"),Neutral);
        Shot(TEXT("pause_1080"));FSlateApplication::Get().SetKeyboardFocus(Interface->TitleActions[TEXT("resume")].Pin());Next();
    }
    else if(Step==2&&T>1){Check(TEXT("focus_does_not_turn_resume_red"),Interface->TitleMotion[TEXT("resume")]()<.01f);PausePointer(Interface->TitleActions[TEXT("resume")].Pin(),true);Next();}
    else if(Step==3&&T>1){Check(TEXT("only_hovered_resume_glows_red"),Interface->TitleMotion[TEXT("resume")]()>.8f&&Interface->TitleMotion[TEXT("page_title")]()<.01f);Shot(TEXT("pause_hover_1080"));Next();}
    else if(Step==4&&T>1){PausePointer(Interface->TitleActions[TEXT("resume")].Pin(),false);Next();}
    else if(Step==5&&T>1){Check(TEXT("resume_glow_fades_after_leave"),Interface->TitleMotion[TEXT("resume")]()<.05f);PausePointer(Interface->TitleActions[TEXT("page_settings")].Pin(),true);Next();}
    else if(Step==6&&T>1){Check(TEXT("navigation_hover_is_independent"),Interface->TitleMotion[TEXT("page_settings")]()>.8f&&Interface->TitleMotion[TEXT("resume")]()<.05f);Shot(TEXT("pause_nav_hover_1080"));Next();}
    else if(Step==7&&T>1)
    {
        for(const FString Page:{FString(TEXT("settings")),FString(TEXT("training")),FString(TEXT("about")),FString(TEXT("game"))}){Command(TEXT("page_")+Page);const bool Open=Screen==Page&&!TitleView;Command(TEXT("back"));Check(TEXT("navigation_roundtrip_")+Page,Open&&Screen==TEXT("pause")&&TitleScene->AboutBackdropActive&&HeroAt.Equals(Hero()->GetActorLocation(),1));}
        Command(TEXT("page_game"));Command(TEXT("page_saves"));const bool Saves=Screen==TEXT("saves")&&TitleScene->SaveBackdropActive;Command(TEXT("back"));Check(TEXT("saves_back_returns_to_live_pause"),Saves&&Screen==TEXT("pause")&&TitleScene->AboutBackdropActive);
        Language=TEXT("kk");Interface->Refresh();Next();
    }
    else if(Step==8&&T>1){Check(TEXT("kazakh_labels_visible"),PauseText(Interface->Root.ToSharedRef(),Tr(TEXT("Вернуться в игру"))));Shot(TEXT("pause_kk_1080"));Next();}
    else if(Step==9&&T>1){Language=TEXT("en");Interface->Refresh();Next();}
    else if(Step==10&&T>1){Check(TEXT("english_labels_visible"),PauseText(Interface->Root.ToSharedRef(),Tr(TEXT("Вернуться в игру"))));Shot(TEXT("pause_en_1080"));Next();}
    else if(Step==11&&T>1){Language=TEXT("ru");Interface->Close();Next();}
    else if(Step==12&&T>1){Shot(TEXT("pause_room_1080"));Next();}
    else if(Step==13&&T>1)
    {
        Interface->Open();Command(TEXT("resume"));Check(TEXT("resume_restores_camera_character_and_state"),Screen.IsEmpty()&&!Hero()->IsHidden()&&!TitleScene->Active&&PC->GetViewTarget()==Hero()&&HeroAt.Equals(Hero()->GetActorLocation(),1)&&Hero()->Health==73&&Hero()->Rounds==5);
        Hero()->Pause();Hero()->Pause();Check(TEXT("escape_resumes_without_state_loss"),Screen.IsEmpty()&&PC->GetViewTarget()==Hero()&&Hero()->Health==73);
        ToggleAdmin();Show(TEXT("pause"));Check(TEXT("admin_controls_retained"),Interface->TitleActions.Contains(TEXT("agent_toggle"))&&Interface->TitleActions.Contains(TEXT("admin_exit")));Agent=true;Next();
    }
    else if(Step==14&&T>2){Check(TEXT("agent_does_not_close_pause"),Screen==TEXT("pause")&&Agent&&Admin);Shot(TEXT("pause_admin_1080"));Next();}
    else if(Step==15&&T>1){Command(TEXT("resume"));Check(TEXT("admin_resume_restores_flying_camera"),Screen.IsEmpty()&&PC->GetViewTarget()==AdminCamera);Agent=false;ToggleAdmin();Show(TEXT("pause"));Command(TEXT("page_title"));Check(TEXT("main_menu_button_restores_title_scene_without_resetting_campaign"),Screen==TEXT("title")&&TitleView&&TitleScene->Active&&!TitleScene->AboutBackdropActive&&PC->GetViewTarget()==TitleScene->Camera&&TitleScene->Age<1&&Hero()->Health==73&&HeroAt.Equals(Hero()->GetActorLocation(),1));bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/pause_menu_check.json"),R);FPlatformMisc::RequestExit(false);Next();}
}
