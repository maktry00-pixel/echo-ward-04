#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardTitleScene.h"
#include "Camera/CameraActor.h"
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
bool DifficultyText(TSharedRef<SWidget> W,const FString& S){if(W->GetType()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(S))return true;auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)if(DifficultyText(C->GetChildAt(I),S))return true;return false;}
void DifficultyPointer(TSharedPtr<SWidget> W,bool In){if(W){FPointerEvent E;if(In)W->OnMouseEnter(W->GetCachedGeometry(),E);else W->OnMouseLeave(E);}}
}
void AEWardCampaignMode::TickDifficultyMenuAssessment(float Delta)
{
    static int Step=0;static double Since=FPlatformTime::Seconds();static FVector At;static float Min=1.e9f,Max=0;static TArray<TSharedPtr<FJsonValue>> Checks;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);const double T=FPlatformTime::Seconds()-Since;auto Next=[&](){++Step;Since=FPlatformTime::Seconds();};
    auto Check=[&](FString N,bool B){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),N);J->SetBoolField(TEXT("passed"),B);Checks.Add(MakeShared<FJsonValueObject>(J));};
    auto Shot=[&](const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/DifficultyCaptures/")+N+TEXT(".png"),true,false);};
    if(TitleScene&&TitleScene->SaveBackdropActive&&!TitleScene->SaveLights.IsEmpty()){Min=FMath::Min(Min,TitleScene->SaveLights.Last()->Intensity);Max=FMath::Max(Max,TitleScene->SaveLights.Last()->Intensity);}
    if(Step==0){Language=TEXT("ru");Show(TEXT("title"));Show(TEXT("game"));Command(TEXT("page_difficulty"));Hero()->Notice=TEXT("DIFFICULTY_HINT_SENTINEL");Hero()->NoticeTime=999;At=TitleScene->SaveCamera->GetActorLocation();Next();}
    else if(Step==1&&T>3)
    {
        Check(TEXT("new_game_opens_quiet_difficulty_background"),Screen==TEXT("difficulty")&&TitleScene->SaveBackdropActive&&!TitleScene->GameBackdropActive&&PC->GetViewTarget()==TitleScene->SaveCamera);
        Check(TEXT("camera_moves_while_paused"),!At.Equals(TitleScene->SaveCamera->GetActorLocation(),.1));Check(TEXT("red_light_pulses"),Max-Min>5);Check(TEXT("blood_on_walls_and_floor"),TitleScene->SaveBloodDecals>=13);
        bool Empty=true;TArray<USceneComponent*> SetChildren;TitleScene->SaveSet->GetChildrenComponents(true,SetChildren);for(auto* C:SetChildren)if(Cast<USkeletalMeshComponent>(C))Empty=false;
        Check(TEXT("background_has_no_monsters"),Empty&&!TitleScene->Nurse->IsVisible()&&!TitleScene->GameZombie->IsVisible()&&Hero()->IsHidden());
        Check(TEXT("four_difficulty_actions"),Interface->TitleActions.Contains(TEXT("new_story"))&&Interface->TitleActions.Contains(TEXT("new_normal"))&&Interface->TitleActions.Contains(TEXT("new_nightmare"))&&Interface->TitleActions.Contains(TEXT("new_hardcore")));
        Check(TEXT("descriptions_inside_cards"),DifficultyText(Interface->Root.ToSharedRef(),Tr(TEXT("Для первого прохождения: исследование и осторожный бой.")))&&DifficultyText(Interface->Root.ToSharedRef(),Tr(TEXT("Для опытных игроков: быстрые противники, меньше патронов."))));
        Check(TEXT("no_yellow_gameplay_hint"),!DifficultyText(Interface->Root.ToSharedRef(),TEXT("DIFFICULTY_HINT_SENTINEL")));
        bool Neutral=true;for(const auto& E:Interface->TitleMotion)Neutral&=E.Value()<.01f;Check(TEXT("all_cards_neutral_without_mouse"),Neutral);
        Shot(TEXT("difficulty_1080"));FSlateApplication::Get().SetKeyboardFocus(Interface->TitleActions[TEXT("new_story")].Pin());Next();
    }
    else if(Step==2&&T>1){Check(TEXT("keyboard_focus_not_red"),Interface->TitleMotion[TEXT("new_story")]()<.01f);DifficultyPointer(Interface->TitleActions[TEXT("new_story")].Pin(),true);Next();}
    else if(Step==3&&T>1){Check(TEXT("hover_only_target_card_red"),Interface->TitleMotion[TEXT("new_story")]()>.8f&&Interface->TitleMotion[TEXT("new_normal")]()<.01f);Shot(TEXT("difficulty_hover_1080"));Next();}
    else if(Step==4&&T>1){DifficultyPointer(Interface->TitleActions[TEXT("new_story")].Pin(),false);Next();}
    else if(Step==5&&T>1){Check(TEXT("red_fades_after_mouse_leaves"),Interface->TitleMotion[TEXT("new_story")]()<.05f);Interface->Close();Next();}
    else if(Step==6&&T>1){Shot(TEXT("difficulty_room_1080"));Next();}
    else if(Step==7&&T>1){Language=TEXT("kk");Interface->Open();Next();}
    else if(Step==8&&T>1){Check(TEXT("kazakh_labels_localized"),DifficultyText(Interface->Root.ToSharedRef(),Tr(TEXT("Назад"))));Shot(TEXT("difficulty_kk_1080"));Next();}
    else if(Step==9&&T>1){Language=TEXT("en");Interface->Refresh();Next();}
    else if(Step==10&&T>1){Check(TEXT("english_labels_localized"),DifficultyText(Interface->Root.ToSharedRef(),TEXT("Back")));Shot(TEXT("difficulty_en_1080"));Next();}
    else if(Step==11&&T>1)
    {
        Command(TEXT("back"));Check(TEXT("back_restores_game_menu_zombie"),Screen==TEXT("game")&&TitleScene->GameBackdropActive&&!TitleScene->SaveBackdropActive);
        Language=TEXT("ru");Show(TEXT("difficulty"));Options->SetBoolField(TEXT("fullscreen"),true);ApplySettings(false);auto* S=GEngine->GetGameUserSettings();Check(TEXT("fullscreen_is_borderless"),S->GetFullscreenMode()==EWindowMode::WindowedFullscreen&&IConsoleManager::Get().FindConsoleVariable(TEXT("r.FullScreenMode"))->GetInt()==1);
        Options->SetBoolField(TEXT("fullscreen"),false);ApplySettings(false);Check(TEXT("windowed_option_retained"),S->GetFullscreenMode()==EWindowMode::Windowed);Options->SetBoolField(TEXT("fullscreen"),true);ApplySettings(false);
        Command(TEXT("new_normal"));Check(TEXT("selection_keeps_start_transition"),TitleScene->Transitioning);Next();
    }
    else if(Step==12&&T>4&&!TitleScene->Transitioning)
    {
        Check(TEXT("normal_starts_real_campaign"),CampaignStarted&&Difficulty==TEXT("normal")&&Screen.IsEmpty()&&!Hero()->IsHidden()&&PC->GetViewTarget()==Hero());
        for(const FString Id:{FString(TEXT("story")),FString(TEXT("nightmare")),FString(TEXT("hardcore"))}){Show(TEXT("difficulty"));TitleActionExecuting=true;Command(TEXT("new_")+Id);TitleActionExecuting=false;Check(TEXT("start_")+Id,Difficulty==Id&&Screen.IsEmpty());}
        bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/difficulty_menu_check.json"),R);FPlatformMisc::RequestExit(false);Next();
    }
}
