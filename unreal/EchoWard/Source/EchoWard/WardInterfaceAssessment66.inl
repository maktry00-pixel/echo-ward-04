#include "WardBookMap.h"
#include "WardCombat63.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Text/STextBlock.h"
#include "UnrealClient.h"
namespace WardInterface66
{
static bool Contains(TSharedRef<SWidget> W,const FString& Needle)
{
 if(W->GetType()==FName(TEXT("STextBlock"))&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString().Contains(Needle))return true;
 auto* Children=W->GetChildren();for(int I=0;I<Children->Num();++I)if(Contains(Children->GetChildAt(I),Needle))return true;return false;
}
void Assess(AEWardCampaignMode& G,float Delta)
{
 static int Phase=0;static float Time=0;static TArray<TSharedPtr<FJsonValue>> Checks;static FVector Home;auto* S=G.HospitalStory.Get();auto* H=G.Hero();if(!S||!H)return;auto* PC=G.GetWorld()->GetFirstPlayerController();Time+=Delta;
 auto Check=[&](const FString& Name,bool Pass){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));UE_LOG(LogTemp,Display,TEXT("UI66 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
 auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/UI66/")+Name+TEXT(".png"),true,false);};
 auto Select=[&](const FString& Id){const auto Bag=G.Inventory();for(int I=0;I<Bag.Num();++I)if(Bag[I].Key==Id){G.Command(TEXT("field_slot_")+FString::FromInt(I));return;}};
 auto Next=[&](){++Phase;Time=0;};
 if(Phase==0)
 {
  S->NarrativeTesting35=true;S->FreezeAI=true;G.NewGame(TEXT("normal"));S->HomeStage52=-1;S->ArrivalStage=-1;G.CampaignStarted=true;G.TitleView=false;G.Admin=false;G.Agent=false;G.Show(TEXT(""));G.Invulnerable=1000;Home=H->GetActorLocation();
  for(auto M:S->Mobs){M->Active=false;M->SetActorEnableCollision(false);M->SetActorHiddenInGame(true);}
  G.Flag(TEXT("weapon"),true);G.Items.Add(TEXT("medkit"),1);G.Items.Add(TEXT("battery"),1);H->Rounds=3;H->Reserve=12;H->Health=40;
  auto* Input=H->InputComponent.Get();int Press=0,Release=0,Actions=0;if(Input){for(const auto& B:Input->KeyBindings)if(B.Chord.Key==EKeys::F){Press+=B.KeyEvent==IE_Pressed;Release+=B.KeyEvent==IE_Released;}for(int I=0;I<Input->GetNumActionBindings();++I)Actions+=Input->GetActionBinding(I).GetActionName()==TEXT("Interact");}
  Check(TEXT("one_F_press_and_release_without_duplicate_prototype_interact"),Input&&Press==1&&Release==1&&Actions==0);
  Check(TEXT("I_opens_inventory"),Hotkey(G,TEXT("inventory"))&&G.Screen==TEXT("inventory")&&PC->IsPaused());
  Check(TEXT("I_again_closes_and_unpauses"),Hotkey(G,TEXT("inventory"))&&G.Screen.IsEmpty()&&!PC->IsPaused());
  Hotkey(G,TEXT("inventory"));Check(TEXT("field_screens_have_consistent_four_tabs"),G.Interface->TitleActions.Contains(TEXT("field_page_inventory"))&&G.Interface->TitleActions.Contains(TEXT("field_page_journal"))&&G.Interface->TitleActions.Contains(TEXT("field_page_map"))&&G.Interface->TitleActions.Contains(TEXT("field_page_quests")));
  Select(TEXT("weapon"));G.Command(TEXT("field_combine"));Select(TEXT("ammo"));Check(TEXT("inventory_combine_preserves_ammunition_total"),H->Rounds==9&&H->Reserve==6&&G.Interface->CombineItem.IsEmpty());
  Select(TEXT("ammo"));Check(TEXT("full_magazine_disables_ammo_use"),!CanUse(G,TEXT("ammo")));
  Select(TEXT("medkit"));G.Command(TEXT("field_use"));Check(TEXT("medkit_from_inventory_closes_screen_and_starts_real_healing"),G.Screen.IsEmpty()&&WardCombat63::Healing(G)&&G.Items.FindRef(TEXT("medkit"))==0);Next();
 }
 else if(Phase==1)
 {
  WardCombat63::Tick(G,Delta);if(Time<=3)return;
  Check(TEXT("inventory_heal_completes_in_real_game_time"),H->Health>40&&!WardCombat63::Healing(G));G.Show(TEXT("journal"));Check(TEXT("empty_documents_have_no_fake_unknown_entry"),Documents(G,TEXT(""),false).IsEmpty()&&!Contains(G.Interface->Root.ToSharedRef(),TEXT("Неизвестный документ")));
  G.Journal.Add(TEXT("Решение: проверка интерфейса"));G.Journal.Add(TEXT("Разговор: проверка"));S->UseId(TEXT("NOTE_01_01"));S->UseId(TEXT("NOTE_01_02"));G.Show(TEXT("journal"));
  const auto IDs=Documents(G,TEXT(""),false);Check(TEXT("documents_exclude_decisions_and_conversations"),IDs.Num()==2);Check(TEXT("document_search_is_case_insensitive_and_keeps_real_journal_indices"),Documents(G,TEXT("вестибюля"),false).Num()==1&&Documents(G,TEXT("note_01_02"),false).Num()==1);
  for(auto I:IDs)WardNotes65::Read(*S,G.Journal[I]);Check(TEXT("unread_filter_uses_persisted_read_state"),Documents(G,TEXT(""),true).IsEmpty());
  G.Interface->DocumentQuery66=TEXT("Журнал");G.Interface->Refresh();Check(TEXT("filtered_document_can_still_be_selected_by_original_index"),G.Interface->TitleActions.Contains(TEXT("field_doc_")+FString::FromInt(IDs[1]))&&!G.Interface->TitleActions.Contains(TEXT("field_doc_0")));
  Shot(TEXT("documents_1080"));Next();
 }
 else if(Phase==2&&Time>.5)
 {
  G.Interface->DocumentQuery66.Empty();G.Show(TEXT("quests"));Check(TEXT("O_task_page_does_not_leak_future_chapters"),Contains(G.Interface->Root.ToSharedRef(),TEXT("Текущая задача"))&&!Contains(G.Interface->Root.ToSharedRef(),TEXT("Первый пациент")));
  S->SetChapter(1);G.Values.Add(TEXT("hospital_step"),2);G.Interface->Refresh();Check(TEXT("task_page_lists_missing_real_items_and_rooms"),Contains(G.Interface->Root.ToSharedRef(),TEXT("R05"))&&Contains(G.Interface->Root.ToSharedRef(),TEXT("R06")));
  Shot(TEXT("quests_1080"));Next();
 }
 else if(Phase==3&&Time>.5)
 {
  G.Command(TEXT("ui66_quest_map"));G.Command(TEXT("ui66_map_goal"));Check(TEXT("task_map_selects_authored_current_goal_site"),G.Screen==TEXT("map")&&G.Interface->BookMapState->Location==TEXT("1F"));
  Check(TEXT("map_summary_uses_live_world_counts"),MapSummary(G).Contains(FString::FromInt(Array(WardBookMapData(),TEXT("locations")).Num()))&&!Contains(G.Interface->Root.ToSharedRef(),TEXT("15 локаций")));
  Check(TEXT("all_map_floor_tabs_have_names"),G.Interface->TitleActions.Contains(TEXT("ui66_floor_list"))&&Array(WardBookMapData(),TEXT("locations")).Num()==35);
  Shot(TEXT("map_1080"));Next();
 }
 else if(Phase==4&&Time>.5)
 {
  Check(TEXT("map_renders_current_goal_marker"),G.Interface->BookMapState->GoalMarkers66==1);G.Show(TEXT("inventory"));G.Command(TEXT("ui66_help"));Check(TEXT("help_open_from_inventory"),G.Screen==TEXT("controls66")&&Contains(G.Interface->Root.ToSharedRef(),TEXT("N — фонарик"))&&Contains(G.Interface->Root.ToSharedRef(),TEXT("O — задания"))&&!Contains(G.Interface->Root.ToSharedRef(),TEXT("Unreal:")));
  G.Command(TEXT("ui66_help"));G.Command(TEXT("ui66_back"));Check(TEXT("help_returns_to_parent_without_self_loop"),G.Screen==TEXT("inventory"));
  G.Show(TEXT("settings"));G.Command(TEXT("back"));Check(TEXT("settings_back_returns_to_originating_inventory"),G.Screen==TEXT("inventory"));
  G.Show(TEXT("inspect"));H->Pause();Check(TEXT("escape_from_item_inspection_returns_to_inventory"),G.Screen==TEXT("inventory"));
  Hotkey(G,TEXT("quests"));Check(TEXT("field_shortcuts_switch_while_paused"),G.Screen==TEXT("quests")&&PC->IsPaused());H->Pause();Check(TEXT("escape_field_page_resumes_game"),G.Screen.IsEmpty()&&!PC->IsPaused());
  G.Show(TEXT("pause"));G.Show(TEXT("settings"));G.Command(TEXT("back"));Check(TEXT("settings_from_pause_returns_to_pause"),G.Screen==TEXT("pause"));
  G.Show(TEXT("settings"));G.Command(TEXT("section_controls"));Shot(TEXT("controls_1080"));Next();
 }
 else if(Phase==5&&Time>.5)
 {
  Check(TEXT("normal_control_guide_has_no_admin_bindings"),!Contains(G.Interface->Root.ToSharedRef(),TEXT("F9 — агент"))&&Contains(G.Interface->Root.ToSharedRef(),TEXT("F — подобрать")));
  G.Options->SetBoolField(TEXT("subtitles"),false);G.Notify(TEXT("F — подобрать"));Check(TEXT("interaction_notice_survives_subtitles_off"),ShowNotice(G));S->DialogueKey62=TEXT("Проверочная речь");S->DialogueLeft62=1;G.Notify(S->DialogueKey62);Check(TEXT("spoken_subtitles_respect_off_setting"),!ShowNotice(G));S->DialogueKey62.Empty();S->DialogueLeft62=0;G.Options->SetBoolField(TEXT("subtitles"),true);
  G.Show(TEXT("inventory"));Next();
 }
 else if(Phase==6&&Time>.5)
 {
  Check(TEXT("inventory_preview_contains_visible_rendered_item"),PreviewBrightPixels66>100);Shot(TEXT("inventory_1080"));Next();
 }
 else if(Phase==7&&Time>.7){FSystemResolution::RequestResolutionChange(1280,720,EWindowMode::Windowed);G.Show(TEXT("quests"));Next();}
 else if(Phase==8&&Time>1)
 {
  Check(TEXT("720p_task_navigation_widgets_present"),G.Interface->TitleActions.Contains(TEXT("field_page_quests"))&&G.Interface->TitleActions.Contains(TEXT("field_close")));Shot(TEXT("quests_720"));Next();
 }
 else if(Phase==9&&Time>.6)
 {
  G.Show(TEXT(""));Check(TEXT("menus_do_not_move_the_character"),Home.Equals(H->GetActorLocation(),5));Json R=MakeShared<FJsonObject>();bool OK=true;for(auto V:Checks)OK&=Boolean(V->AsObject(),TEXT("passed"));R->SetBoolField(TEXT("success"),OK);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);WardData::Write(FPaths::ProjectSavedDir()/TEXT("Prototype/interface66_check.json"),R);FPlatformMisc::RequestExit(false);Next();
 }
}
}
