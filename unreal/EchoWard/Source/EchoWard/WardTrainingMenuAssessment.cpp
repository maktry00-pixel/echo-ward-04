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
namespace {
bool HasTrainingText(TSharedRef<SWidget> W,const FString& S){if(W->GetType()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(S))return true;auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)if(HasTrainingText(C->GetChildAt(I),S))return true;return false;}
bool VisibleTrainingText(TSharedRef<SWidget> W,const FString& Text,FVector2D ClipMin=FVector2D(-1.e9,-1.e9),FVector2D ClipMax=FVector2D(1.e9,1.e9))
{
    const FGeometry& G=W->GetCachedGeometry();const FVector2D At=G.GetAbsolutePosition(),End=G.LocalToAbsolute(G.GetLocalSize());
    if(W->GetType()==TEXT("SScrollBox")){ClipMin.X=FMath::Max(ClipMin.X,At.X);ClipMin.Y=FMath::Max(ClipMin.Y,At.Y);ClipMax.X=FMath::Min(ClipMax.X,End.X);ClipMax.Y=FMath::Min(ClipMax.Y,End.Y);}
    if(W->GetType()==TEXT("STextBlock")&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(Text))return G.GetLocalSize().Y>0&&At.X>=ClipMin.X-1&&At.Y>=ClipMin.Y-1&&End.X<=ClipMax.X+1&&End.Y<=ClipMax.Y+1;
    auto* C=W->GetChildren();for(int I=0;C&&I<C->Num();++I)if(VisibleTrainingText(C->GetChildAt(I),Text,ClipMin,ClipMax))return true;return false;
}
void TrainingPointer(TSharedPtr<SWidget> W,bool In){if(W){FPointerEvent E;if(In)W->OnMouseEnter(W->GetCachedGeometry(),E);else W->OnMouseLeave(E);}}
}
void AEWardCampaignMode::TickTrainingMenuAssessment(float Delta)
{
    static double Start=FPlatformTime::Seconds();static int Step=0;static TArray<TSharedPtr<FJsonValue>> Checks;static FVector CameraAt,HeroAt;static FRotator Lamp;static float MinRed=1.e9f,MaxRed=0;
    const float T=FPlatformTime::Seconds()-Start;auto Next=[&](){++Step;Start=FPlatformTime::Seconds();};auto* PC=UGameplayStatics::GetPlayerController(this,0);
    auto Check=[&](FString Name,bool Pass){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),Name);J->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(J));};
    auto Shot=[&](FString Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/TrainingCaptures/")+Name+TEXT(".png"),true,false);};
    if(TitleScene&&TitleScene->TrainingBackdropActive&&!TitleScene->TrainingLights.IsEmpty()){MinRed=FMath::Min(MinRed,TitleScene->TrainingLights.Last()->Intensity);MaxRed=FMath::Max(MaxRed,TitleScene->TrainingLights.Last()->Intensity);}
    if(Step==0){Show(TEXT("title"));Command(TEXT("page_training"));Hero()->Notice=TEXT("YELLOW_HINT_SENTINEL");Hero()->NoticeTime=999;CameraAt=TitleScene->TrainingCamera->GetActorLocation();Lamp=TitleScene->TrainingDoorHinge->GetRelativeRotation();Next();}
    else if(Step==1&&T>3){Check(TEXT("distinct_live_training_camera"),PC->GetViewTarget()==TitleScene->TrainingCamera&&TitleScene->TrainingBackdropActive&&!TitleScene->GameBackdropActive&&!TitleScene->SaveBackdropActive&&FVector::Dist(CameraAt,TitleScene->Camera->GetActorLocation())>15000);Check(TEXT("paused_camera_door_and_red_light_animate"),!CameraAt.Equals(TitleScene->TrainingCamera->GetActorLocation(),.1)&&!Lamp.Equals(TitleScene->TrainingDoorHinge->GetRelativeRotation(),.05)&&MaxRed-MinRed>5);bool Empty=true;TArray<USceneComponent*> SetChildren;TitleScene->TrainingSet->GetChildrenComponents(true,SetChildren);for(auto* C:SetChildren)if(Cast<USkeletalMeshComponent>(C))Empty=false;Check(TEXT("empty_set_no_visible_title_monsters"),Empty&&!TitleScene->Nurse->IsVisible()&&(!TitleScene->GameZombie||!TitleScene->GameZombie->IsVisible())&&!TitleScene->PeekingHead->IsVisible()&&Hero()->IsHidden());Check(TEXT("wall_and_floor_blood"),TitleScene->TrainingBloodDecals==24);Check(TEXT("eight_tabs_and_back"),Interface->TitleActions.Num()==9);Check(TEXT("native_map_binding_documented"),HasTrainingText(Interface->Root.ToSharedRef(),TEXT("по M"))&&!HasTrainingText(Interface->Root.ToSharedRef(),TEXT("по Tab")));Check(TEXT("first_page_key_labels_inside_scroll_clip"),VisibleTrainingText(Interface->Root.ToSharedRef(),TEXT("План отделения")));Check(TEXT("keyboard_reference_retained"),HasTrainingText(Interface->Root.ToSharedRef(),TEXT("W A S D")));Check(TEXT("old_yellow_hint_absent"),!HasTrainingText(Interface->Root.ToSharedRef(),TEXT("YELLOW_HINT_SENTINEL")));Shot(TEXT("training_01_1080"));FSlateApplication::Get().SetKeyboardFocus(Interface->TitleActions[TEXT("guide_0")].Pin());Next();}
    else if(Step==2&&T>1){Check(TEXT("active_and_keyboard_focused_tab_remains_neutral"),Interface->TitleMotion[TEXT("guide_0")]()<.01f);TrainingPointer(Interface->TitleActions[TEXT("guide_1")].Pin(),true);Next();}
    else if(Step==3&&T>1){Check(TEXT("pointer_hover_is_red"),Interface->TitleMotion[TEXT("guide_1")]()>.8f);Shot(TEXT("training_hover_1080"));Next();}
    else if(Step==4&&T>1){TrainingPointer(Interface->TitleActions[TEXT("guide_1")].Pin(),false);Next();}
    else if(Step==5&&T>1){Check(TEXT("red_fades_when_pointer_leaves"),Interface->TitleMotion[TEXT("guide_1")]()<.05f);Command(TEXT("guide_1"));Check(TEXT("second_tab_displays_search"),Interface->GuidePage==1&&HasTrainingText(Interface->Root.ToSharedRef(),TEXT("Обыскивай мебель")));Next();}
    else if(Step==6&&T>1){Shot(TEXT("training_02_1080"));Next();}
    else if(Step==7&&T>1){Command(TEXT("guide_2"));Check(TEXT("third_tab_displays_combat"),HasTrainingText(Interface->Root.ToSharedRef(),TEXT("Целься осознанно")));Next();}
    else if(Step==8&&T>1){Shot(TEXT("training_03_1080"));Next();}
    else if(Step==9&&T>1){Command(TEXT("guide_3"));Check(TEXT("fourth_tab_displays_hiding"),HasTrainingText(Interface->Root.ToSharedRef(),TEXT("Разорви видимость")));Next();}
    else if(Step==10&&T>1){Shot(TEXT("training_04_1080"));Next();}
    else if(Step>=11&&Step<=18){if(T>1){if(Step%2==1){const int Page=(Step-11)/2+4;Command(TEXT("guide_")+FString::FromInt(Page));const Json PageData=Array(Data,TEXT("training"))[Page]->AsObject();Check(TEXT("training_page_")+FString::FromInt(Page+1)+TEXT("_content_retained"),Interface->GuidePage==Page&&HasTrainingText(Interface->Root.ToSharedRef(),String(PageData,TEXT("title"))));}else Shot(FString::Printf(TEXT("training_%02d_1080"),(Step-12)/2+5));Next();}}
    else if(Step==19&&T>1){Command(TEXT("back"));Check(TEXT("back_restores_main_title"),Screen==TEXT("title")&&PC->GetViewTarget()==TitleScene->Camera&&!TitleScene->TrainingSet->IsVisible());Show(TEXT("training"));Interface->Close();Next();}
    else if(Step==20&&T>2){Shot(TEXT("training_room_1080"));Next();}
    else if(Step==21&&T>1){NewGame(TEXT("story"));Show(TEXT(""));HeroAt=Hero()->GetActorLocation();Show(TEXT("pause"));Command(TEXT("page_training"));Check(TEXT("pause_training_uses_empty_room"),!TitleView&&PC->GetViewTarget()==TitleScene->TrainingCamera);Command(TEXT("back"));Check(TEXT("back_to_live_pause_preserves_player_position"),Screen==TEXT("pause")&&PC->GetViewTarget()==TitleScene->AboutCamera&&TitleScene->AboutBackdropActive&&Hero()->IsHidden()&&HeroAt.Equals(Hero()->GetActorLocation(),1));Language=TEXT("kk");Show(TEXT("training"));Next();}
    else if(Step==22&&T>2){const auto* Translation=Translations.Find(TEXT("Назад"));Check(TEXT("kazakh_copy_really_visible"),Translation&&HasTrainingText(Interface->Root.ToSharedRef(),Translation->Value));Shot(TEXT("training_kk_1080"));Next();}
    else if(Step==23&&T>1){Language=TEXT("en");Show(TEXT("training"));Next();}
    else if(Step==24&&T>2){const auto* Translation=Translations.Find(TEXT("Назад"));Check(TEXT("english_copy_replaces_kazakh_cached_text"),Translation&&Translation->Key!=Translation->Value&&HasTrainingText(Interface->Root.ToSharedRef(),Translation->Key)&&!HasTrainingText(Interface->Root.ToSharedRef(),Translation->Value));Shot(TEXT("training_en_1080"));Next();}
    else if(Step==25&&T>1){Show(TEXT(""));Check(TEXT("resume_hides_training_set"),!TitleScene->Active&&!TitleScene->TrainingSet->IsVisible()&&PC->GetViewTarget()==Hero());bool Pass=true;for(auto J:Checks)Pass&=Boolean(J->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/training_menu_check.json"),R);FPlatformMisc::RequestExit(false);Next();}
}
