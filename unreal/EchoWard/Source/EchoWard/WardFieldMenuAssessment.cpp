#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Widgets/Text/STextBlock.h"
using namespace WardData;
namespace
{
    bool FieldEnglishText(TSharedRef<SWidget> W)
    {
        if(W->GetType()==FName(TEXT("STextBlock"))){for(TCHAR C:StaticCastSharedRef<STextBlock>(W)->GetText().ToString())if((C>=0x0400&&C<=0x04ff))return false;}
        auto* Children=W->GetChildren();for(int I=0;I<Children->Num();++I)if(!FieldEnglishText(Children->GetChildAt(I)))return false;return true;
    }
    bool FieldSingleLine(TSharedRef<SWidget> W)
    {
        if(W->GetType()==FName(TEXT("STextBlock"))&&W->GetDesiredSize().Y>45)return false;
        auto* Children=W->GetChildren();for(int I=0;I<Children->Num();++I)if(!FieldSingleLine(Children->GetChildAt(I)))return false;return true;
    }
}
void AEWardCampaignMode::TickFieldMenuAssessment(float Delta)
{
    static int Step=0;static double Since=FPlatformTime::Seconds();static TArray<TSharedPtr<FJsonValue>> Checks;static FVector Position;static int Reserve=0;static Json Saved;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);const double T=FPlatformTime::Seconds()-Since;
    auto Next=[&](){++Step;Since=FPlatformTime::Seconds();};auto Check=[&](const TCHAR* N,bool B){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),N);C->SetBoolField(TEXT("passed"),B);Checks.Add(MakeShared<FJsonValueObject>(C));};
    auto Shot=[&](const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/FieldCaptures/")+N+TEXT(".png"),true,false);};
    auto Select=[&](const FString& Id){const auto Bag=Inventory();for(int I=0;I<Bag.Num();++I)if(Bag[I].Key==Id){Command(TEXT("field_slot_")+FString::FromInt(I));return;}};
    if(Step==0){NewGame(TEXT("normal"));Show(TEXT(""));Language=TEXT("ru");Flag(TEXT("weapon"),true);Items.Add(TEXT("medkit"),1);Items.Add(TEXT("battery"),1);Items.Add(TEXT("badge"),1);Items.Add(TEXT("fuse"),1);Hero()->Reserve=12;Hero()->Rounds=3;Hero()->Health=40;Hero()->NoticeTime=0;Position=Hero()->GetActorLocation();Show(TEXT("inventory"));Next();}
    else if(Step==1&&T>4){Check(TEXT("inventory_pauses_game_and_has_12_cells"),PC->IsPaused()&&Interface->TitleActions.Contains(TEXT("field_slot_11"))&&InventoryCapacity==12);Check(TEXT("inventory_selects_real_pistol"),Interface->SelectedItem==TEXT("weapon"));Shot(TEXT("inventory_1080"));Next();}
    else if(Step==2&&T>1){Select(TEXT("medkit"));Command(TEXT("field_use"));Check(TEXT("use_medkit_consumes_and_heals"),Items.FindRef(TEXT("medkit"))==0&&Hero()->Health>40);Select(TEXT("weapon"));Reserve=Hero()->Reserve;Command(TEXT("field_combine"));Select(TEXT("ammo"));Check(TEXT("combine_loads_real_magazine_without_creating_ammunition"),Hero()->Rounds==9&&Hero()->Reserve==Reserve-6&&Interface->CombineItem.IsEmpty());Next();}
    else if(Step==3&&T>1){Select(TEXT("battery"));Reserve=Hero()->Reserve;Command(TEXT("field_combine"));Select(TEXT("fuse"));Check(TEXT("invalid_combine_preserves_items"),Items.FindRef(TEXT("battery"))==1&&Items.FindRef(TEXT("fuse"))==1&&Hero()->Reserve==Reserve);Select(TEXT("fuse"));Command(TEXT("field_drop"));Check(TEXT("quest_item_cannot_be_discarded"),Items.FindRef(TEXT("fuse"))==1);Select(TEXT("bottle"));Command(TEXT("field_drop"));Check(TEXT("drop_creates_pickup_in_world"),Items.FindRef(TEXT("bottle"))==0&&Objects.Contains(TEXT("dropped_1")));Next();}
    else if(Step==4&&T>1){Select(TEXT("badge"));Command(TEXT("field_use"));Command(TEXT("inspect_turn"));Check(TEXT("badge_clue_remains_accessible"),Screen==TEXT("inspect")&&Flag(TEXT("badge_examined")));Show(TEXT("journal"));Next();}
    else if(Step==5&&T>1){Check(TEXT("uncollected_documents_not_revealed"),Journal.IsEmpty()&&!Interface->TitleActions.Contains(TEXT("field_doc_0")));Shot(TEXT("documents_empty_1080"));Next();}
    else if(Step==6&&T>1){for(int I=0;I<2;++I)Journal.Add(Array(Data,TEXT("stories"))[I]->AsString());Interface->Refresh();Next();}
    else if(Step==7&&T>2){Check(TEXT("only_collected_documents_listed"),Interface->TitleActions.Contains(TEXT("field_doc_0"))&&Interface->TitleActions.Contains(TEXT("field_doc_1"))&&!Interface->TitleActions.Contains(TEXT("field_doc_2")));Command(TEXT("field_doc_1"));Check(TEXT("document_selection_changes_content"),Interface->DocumentIndex==1);Next();}
    else if(Step==8&&T>2){Shot(TEXT("documents_1080"));Next();}
    else if(Step==9&&T>1){Command(TEXT("field_page_map"));Command(TEXT("field_map_current"));Check(TEXT("map_is_locked_until_found"),!Flag(TEXT("map")));Next();}
    else if(Step==10&&T>1){Shot(TEXT("map_locked_1080"));Next();}
    else if(Step==11&&T>1){Flag(TEXT("map"),true);CurrentRoom=TEXT("reception");Visited.Add(CurrentRoom);Visited.Add(TEXT("ward04"));Visited.Add(TEXT("ward05"));Command(TEXT("field_map_ward"));Next();}
    else if(Step==12&&T>2){Check(TEXT("ward_map_tabs_available"),Interface->TitleActions.Contains(TEXT("field_map_ward"))&&Interface->TitleActions.Contains(TEXT("field_map_basement"))&&Interface->TitleActions.Contains(TEXT("field_map_archive")));Shot(TEXT("map_ward_1080"));Next();}
    else if(Step==13&&T>1){Command(TEXT("field_map_basement"));Check(TEXT("basement_switches_real_floor"),Values.FindRef(TEXT("map_floor"))==-1);Next();}
    else if(Step==14&&T>2){Shot(TEXT("map_basement_1080"));Next();}
    else if(Step==15&&T>1){Command(TEXT("field_map_archive"));Check(TEXT("archive_switches_real_area"),Values.FindRef(TEXT("map_floor"))==0&&Values.FindRef(TEXT("map_area"))==2);Next();}
    else if(Step==16&&T>2){Shot(TEXT("map_archive_1080"));Next();}
    else if(Step==17&&T>1){Saved=Capture();const auto Notes=Journal;Journal.Reset();Restore(Saved);Check(TEXT("collected_documents_survive_restore"),Journal==Notes);Show(TEXT("inventory"));Select(TEXT("weapon"));Next();}
    else if(Step==18&&T>3){auto W=Interface->TitleActions[TEXT("field_use")].Pin();const FPointerEvent E;if(W)W->OnMouseEnter(W->GetCachedGeometry(),E);Next();}
    else if(Step==19&&T>1){Check(TEXT("use_button_responds_to_pointer_hover"),Interface->TitleActions[TEXT("field_use")].Pin()->IsHovered());Shot(TEXT("inventory_hover_1080"));Next();}
    else if(Step==20&&T>1){Command(TEXT("field_close"));Check(TEXT("close_resumes_same_character"),Screen.IsEmpty()&&!PC->IsPaused()&&Hero()->GetActorLocation().Equals(Position,5));Show(TEXT("inventory"));Items.Add(TEXT("medkit"),12);Check(TEXT("capacity_refuses_13th_slot"),!CanTake(TEXT("battery")));Items.Remove(TEXT("medkit"));Language=TEXT("en");Interface->Refresh();Next();}
    else if(Step==21&&T>3){Check(TEXT("english_inventory_contains_no_russian_fragments"),FieldEnglishText(Interface->Root.ToSharedRef()));Shot(TEXT("inventory_en_1080"));Next();}
    else if(Step==22&&T>1){Language=TEXT("kk");Interface->Refresh();Next();}
    else if(Step==23&&T>2){Check(TEXT("kazakh_field_labels_are_localized"),Tr(TEXT("ДОКУМЕНТЫ"))==TEXT("ҚҰЖАТТАР")&&Tr(TEXT("Объединить"))==TEXT("Біріктіру")&&Tr(TEXT("Выбросить"))==TEXT("Тастау"));Shot(TEXT("inventory_kk_1080"));Next();}
    else if(Step==24&&T>1){Language=TEXT("ru");Interface->Refresh();FSystemResolution::RequestResolutionChange(1280,720,EWindowMode::Windowed);Next();}
    else if(Step==25&&T>2){bool Single=true;for(const FString& Id:TArray<FString>{TEXT("field_page_inventory"),TEXT("field_page_journal"),TEXT("field_page_map"),TEXT("field_close"),TEXT("field_use"),TEXT("field_combine"),TEXT("field_drop")}){auto W=Interface->TitleActions.FindRef(Id).Pin();Single&=W.IsValid()&&FieldSingleLine(W.ToSharedRef());}Check(TEXT("720p_navigation_and_actions_stay_single_line"),Single);Shot(TEXT("inventory_720"));Next();}
    else if(Step==26&&T>1){bool Pass=true;for(auto C:Checks)Pass&=Boolean(C->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/field_menu_check.json"),R);FPlatformMisc::RequestExit(false);Next();}
}
