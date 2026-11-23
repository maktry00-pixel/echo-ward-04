#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardTitleScene.h"
#include "Camera/CameraActor.h"
#include "Components/LightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
using namespace WardData;
void TickWardAIDemoAssessment115(AEWardCampaignMode& Game);
namespace {
bool HasAboutText(TSharedRef<SWidget> W,const FString& S){if(W->GetType()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(S))return true;auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)if(HasAboutText(C->GetChildAt(I),S))return true;return false;}
bool HasStudioBadge(TSharedRef<SWidget> Widget)
{
    if (Widget->GetTag() == FName(TEXT("QaisarStudio"))) return true;
    FChildren* Children = Widget->GetChildren();
    for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
    {
        if (HasStudioBadge(Children->GetChildAt(Index))) return true;
    }
    return false;
}
void AboutPointer(TSharedPtr<SWidget> W,bool In){if(W){FPointerEvent E;if(In)W->OnMouseEnter(W->GetCachedGeometry(),E);else W->OnMouseLeave(E);}}
}
void AEWardCampaignMode::TickAboutMenuAssessment(float Delta)
{
    if (FParse::Param(FCommandLine::Get(), TEXT("WardAIDemoCheck115")))
    {
        TickWardAIDemoAssessment115(*this);
        return;
    }
    static double Start=FPlatformTime::Seconds();static int Step=0;static TArray<TSharedPtr<FJsonValue>> Checks;static FVector CameraAt,HeroAt;static FRotator Lamp;static float MinRed=1.e9f,MaxRed=0;
    const float T=FPlatformTime::Seconds()-Start;auto Next=[&](){++Step;Start=FPlatformTime::Seconds();};auto* PC=UGameplayStatics::GetPlayerController(this,0);
    auto Check=[&](FString Name,bool Pass){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));};
    auto Shot=[&](FString Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/AboutCaptures/")+Name+TEXT(".png"),true,false);};
    if(TitleScene&&TitleScene->AboutBackdropActive&&!TitleScene->AboutLights.IsEmpty()){MinRed=FMath::Min(MinRed,TitleScene->AboutLights.Last()->Intensity);MaxRed=FMath::Max(MaxRed,TitleScene->AboutLights.Last()->Intensity);}
    if(Step==0){Show(TEXT("title"));Command(TEXT("page_about"));Hero()->Notice=TEXT("YELLOW_HINT_SENTINEL");Hero()->NoticeTime=999;CameraAt=TitleScene->AboutCamera->GetActorLocation();Lamp=TitleScene->AboutLampHinge->GetRelativeRotation();Next();}
    else if(Step==1&&T>3){Check(TEXT("distinct_live_about_camera"),PC->GetViewTarget()==TitleScene->AboutCamera&&TitleScene->AboutBackdropActive&&!TitleScene->GameBackdropActive&&!TitleScene->SaveBackdropActive&&FVector::Dist(CameraAt,TitleScene->Camera->GetActorLocation())>15000);Check(TEXT("paused_camera_lamp_and_red_light_animate"),!CameraAt.Equals(TitleScene->AboutCamera->GetActorLocation(),.1)&&!Lamp.Equals(TitleScene->AboutLampHinge->GetRelativeRotation(),.05)&&MaxRed-MinRed>5);bool Empty=true;TArray<USceneComponent*> SetChildren;TitleScene->AboutSet->GetChildrenComponents(true,SetChildren);for(auto* C:SetChildren)if(Cast<USkeletalMeshComponent>(C))Empty=false;Check(TEXT("empty_set_no_visible_title_monsters"),Empty&&!TitleScene->Nurse->IsVisible()&&(!TitleScene->GameZombie||!TitleScene->GameZombie->IsVisible())&&!TitleScene->PeekingHead->IsVisible()&&Hero()->IsHidden());Check(TEXT("wall_and_floor_blood"),TitleScene->AboutBloodDecals==13);Check(TEXT("four_tabs_and_back"),Interface->TitleActions.Num()==5);Check(TEXT("old_yellow_hint_absent"),!HasAboutText(Interface->Root.ToSharedRef(),TEXT("YELLOW_HINT_SENTINEL")));Shot(TEXT("about_01_1080"));FSlateApplication::Get().SetKeyboardFocus(Interface->TitleActions[TEXT("guide_0")].Pin());Next();}
    else if(Step==2&&T>1){Check(TEXT("active_and_keyboard_focused_tab_remains_neutral"),Interface->TitleMotion[TEXT("guide_0")]()<.01f);AboutPointer(Interface->TitleActions[TEXT("guide_1")].Pin(),true);Next();}
    else if(Step==3&&T>1){Check(TEXT("pointer_hover_is_red"),Interface->TitleMotion[TEXT("guide_1")]()>.8f);Shot(TEXT("about_hover_1080"));Next();}
    else if(Step==4&&T>1){AboutPointer(Interface->TitleActions[TEXT("guide_1")].Pin(),false);Next();}
    else if(Step==5&&T>1){Check(TEXT("red_fades_when_pointer_leaves"),Interface->TitleMotion[TEXT("guide_1")]()<.05f);Command(TEXT("guide_1"));Check(TEXT("second_tab_displays_survival"),Interface->GuidePage==1&&HasAboutText(Interface->Root.ToSharedRef(),TEXT("У тебя есть выбор")));Next();}
    else if(Step==6&&T>1){Shot(TEXT("about_02_1080"));Next();}
    else if(Step==7&&T>1){Command(TEXT("guide_2"));Check(TEXT("third_tab_displays_credits"),HasAboutText(Interface->Root.ToSharedRef(),TEXT("Poly Haven")));Check(TEXT("third_tab_displays_qaisar_studio"),HasAboutText(Interface->Root.ToSharedRef(),TEXT("QAISAR STUDIO")));Next();}
    else if(Step==8&&T>1){Shot(TEXT("about_03_1080"));Next();}
    else if(Step==9&&T>1){Command(TEXT("guide_3"));Check(TEXT("fourth_tab_retains_three_syllabus_topics"),HasAboutText(Interface->Root.ToSharedRef(),TEXT("A*"))&&HasAboutText(Interface->Root.ToSharedRef(),TEXT("CSP"))&&HasAboutText(Interface->Root.ToSharedRef(),TEXT("Minimax")));Next();}
    else if(Step==10&&T>1){Shot(TEXT("about_04_1080"));Next();}
    else if(Step==11&&T>1){Command(TEXT("back"));Check(TEXT("back_restores_main_title"),Screen==TEXT("title")&&PC->GetViewTarget()==TitleScene->Camera&&!TitleScene->AboutSet->IsVisible());Show(TEXT("about"));Interface->Close();Next();}
    else if(Step==12&&T>2){Shot(TEXT("about_room_1080"));Next();}
    else if(Step==13&&T>1){NewGame(TEXT("story"));Show(TEXT(""));HeroAt=Hero()->GetActorLocation();Show(TEXT("pause"));Command(TEXT("page_about"));Check(TEXT("pause_about_uses_empty_room"),!TitleView&&PC->GetViewTarget()==TitleScene->AboutCamera);Command(TEXT("back"));Check(TEXT("back_to_live_pause_preserves_player_position"),Screen==TEXT("pause")&&PC->GetViewTarget()==TitleScene->AboutCamera&&TitleScene->AboutBackdropActive&&Hero()->IsHidden()&&HeroAt.Equals(Hero()->GetActorLocation(),1));Language=TEXT("kk");Show(TEXT("about"));Next();}
    else if(Step==14&&T>2){const auto* Translation=Translations.Find(TEXT("Назад"));Check(TEXT("kazakh_copy_really_visible"),Translation&&HasAboutText(Interface->Root.ToSharedRef(),Translation->Value));Shot(TEXT("about_kk_1080"));Next();}
    else if(Step==15&&T>1){Language=TEXT("en");Show(TEXT("about"));Next();}
    else if(Step==16&&T>2){const auto* Translation=Translations.Find(TEXT("Назад"));Check(TEXT("english_copy_replaces_kazakh_cached_text"),Translation&&Translation->Key!=Translation->Value&&HasAboutText(Interface->Root.ToSharedRef(),Translation->Key)&&!HasAboutText(Interface->Root.ToSharedRef(),Translation->Value));Shot(TEXT("about_en_1080"));Next();}
    else if(Step==17&&T>1){Show(TEXT(""));Check(TEXT("resume_hides_about_set"),!TitleScene->Active&&!TitleScene->AboutSet->IsVisible()&&PC->GetViewTarget()==Hero());Show(TEXT("title"));Next();}
    else if(Step==18&&T>2){Check(TEXT("title_displays_qaisar_studio"),HasStudioBadge(Interface->Root.ToSharedRef()));Shot(TEXT("title_qaisar"));Next();}
    else if(Step==19&&T>1){bool Pass=true;for(auto J:Checks)Pass&=Boolean(J->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/about_menu_check.json"),R);FPlatformMisc::RequestExit(false);Next();}
}
