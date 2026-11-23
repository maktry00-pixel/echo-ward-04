#include "WardDialogue62.h"
#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardPeople54.h"
#include "WardMonster59.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
using namespace WardData;
namespace WardDialogue62
{
void Assess(AEWardCampaignMode& G,float Delta)
{
 static int Stage=0;static float Time=0;static TArray<TSharedPtr<FJsonValue>> Checks;
 static UEWardPerson54* Runner=nullptr;static FVector Before;
 auto* S=G.HospitalStory.Get();auto* H=G.Hero();if(!S||!H)return;
 auto Check=[&](const FString& Name,bool Pass){Json R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));UE_LOG(LogTemp,Display,TEXT("DIALOGUE62 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
 Time+=FApp::GetDeltaTime();
 if(Stage==0)
 {
  G.CampaignStarted=true;G.TitleView=false;G.Admin=false;S->HomeStage52=-1;S->ArrivalStage=-1;G.Show(TEXT(""));S->FreezeAI=false;
  Check(TEXT("all_authored_and_new_spoken_lines_have_files"),Object(Data(),TEXT("spoken"))->Values.Num()>=1200);
  bool Audio=true,Durations=true;for(const auto& E:Object(Data(),TEXT("spoken"))->Values){Json R=E.Value->AsObject();Audio&=FPaths::FileExists(FPaths::ProjectContentDir()/TEXT("CampaignData/Dialogue62/voice")/(String(R,TEXT("id"))+TEXT(".wav")));Durations&=Number(R,TEXT("duration"))>0&&!Array(R,TEXT("envelope")).IsEmpty();}
  Check(TEXT("1205_audio_files_and_lip_envelopes_resolve"),Audio&&Durations);
  Check(TEXT("six_authored_pair_conversations"),Array(Data(),TEXT("groups")).Num()==6);
  const auto Direct = SpokenLines(TEXT("Бека: Продолжим.\n\nБека читает акт закрытия корпуса: служебная ремарка."));
  Check(TEXT("stage_direction_is_not_character_speech"), Direct.Num() == 1 && Direct[0] == TEXT("Бека: Продолжим."));
  const auto Quoted = SpokenLines(TEXT("Саловат выполняет новое действие и спрашивает: «Так?» Из вентиляции снова слышно «не помню». Разница проста и страшна."));
  Check(TEXT("quoted_question_and_vent_reply_keep_their_own_speakers"), Quoted.Num() == 2 && Quoted[0] == TEXT("Саловат: Так?") && Quoted[1] == TEXT("Голос из вентиляции: Не помню."));
  const FVector Arena(-70000,-70000,0);
  auto* Floor=G.GetWorld()->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(2000,2000,10));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Floor->SetActorLocation(Arena-FVector(0,0,10));
  for(auto M:S->Mobs){M->Active=false;M->SetActorHiddenInGame(true);M->SetActorEnableCollision(false);M->GetCharacterMovement()->DisableMovement();}
  for(auto Pair:S->Props)if(Pair.Value->FindComponentByClass<UEWardPerson54>()){Pair.Value->SetActorHiddenInGame(true);Pair.Value->SetActorEnableCollision(false);}
  auto* A=S->Props.FindRef(TEXT("FILE_30"));Runner=A->FindComponentByClass<UEWardPerson54>();Runner->LoadAppearance();Runner->RestoreHuman58(MakeShared<FJsonObject>());A->SetActorHiddenInGame(false);A->SetActorLocation(Arena+FVector(0,0,.7));
  H->SetActorLocation(Arena+FVector(900,0,91));H->Health=100;H->Dead=false;G.Invulnerable=100;
  auto* Mob=S->Mobs[0].Get();Mob->ResetMob();Mob->SetActorLocation(Arena+FVector(-100,0,88));Mob->Active=true;Mob->SetActorHiddenInGame(false);Mob->SetActorEnableCollision(false);Mob->AttackClock=0;
  Mob->StepBrain59(.01f);Check(TEXT("monster_selects_exposed_npc_not_farther_hero"),Mob->NPCTarget62==TEXT("FILE_30")&&Mob->State59==TEXT("windup"));
  Check(TEXT("npc_hit_waits_for_telegraph"),Runner->Health62()==100);
  Mob->StepBrain59(float(Number(Mob->Rule59,TEXT("windup_seconds")))+.05f);Check(TEXT("monster_melee_damages_npc"),Runner->Health62()<100);
  const float Health=Runner->Health62();Mob->StepBrain59(.01f);Check(TEXT("npc_hit_not_repeated_each_frame"),Runner->Health62()==Health);
  H->SetActorLocation(Arena+FVector(3000,0,91));Mob->ChangeState59(TEXT("patrol"));Mob->StepBrain59(.01f);Check(TEXT("monster_does_not_attack_npc_with_suspended_distant_ai"),Mob->NPCTarget62.IsEmpty());H->SetActorLocation(Arena+FVector(900,0,91));
  Mob->Active=false;Mob->SetActorHiddenInGame(true);Runner->RestoreHuman58(MakeShared<FJsonObject>());
  Check(TEXT("escape_finds_grounded_route_away_from_threat"),Runner->Escape62(Arena+FVector(-200,0,70))&&FVector::Dist2D(Runner->EscapeGoal62(),Arena+FVector(-200,0,70))>450);
  Runner->Impact58(200,FVector(90,0,0));Check(TEXT("optional_npc_dies_when_attack_is_lethal"),Runner->IsDead58());
  Json Death=Runner->HumanSnapshot58();Runner->RestoreHuman58(Death);Check(TEXT("npc_death_and_fallen_pose_survive_save"),Runner->IsDead58()&&Runner->IsFallen58());
  Runner->RestoreHuman58(MakeShared<FJsonObject>());
  auto* Core=S->Props.FindRef(TEXT("PERSON_3"))->FindComponentByClass<UEWardPerson54>();Core->LoadAppearance();Core->Impact58(200,FVector(30,0,0));Check(TEXT("critical_story_npc_is_wounded_without_softlocking_plot"),!Core->IsDead58()&&Core->IsFallen58());Core->RestoreHuman58(MakeShared<FJsonObject>());
  Event(*S,TEXT("FILE_30"),TEXT("danger"));Tick(*S,.4f);Check(TEXT("event_plays_spatial_voice"),Speaking(*S,TEXT("FILE_30")));
  Check(TEXT("event_audio_has_spatial_attenuation"),IsValid(S->DialogueAudio62)&&S->DialogueAudio62->bAllowSpatialization&&S->DialogueAudio62->bOverrideAttenuation);
  S->DialogueTime62=.5f;Check(TEXT("mouth_envelope_is_finite"),FMath::IsFinite(Mouth(*S,TEXT("FILE_30"))));
  const FString First=S->DialogueSession62;Event(*S,TEXT("FILE_30"),TEXT("danger"));Check(TEXT("danger_line_respects_repeat_cooldown"),S->DialogueSession62==First);
  G.Show(TEXT("pause"));Tick(*S,.02f);Check(TEXT("pause_stops_world_speech"),!IsValid(S->DialogueAudio62));G.Show(TEXT(""));
  S->ActiveScene35=TEXT("NPC_FILE_30");S->DialoguePage35=0;S->SelectedChoice35.Empty();
  const Json KeyObject=Object(Object(Data(),TEXT("events")),TEXT("FILE_30"));const FString Key=Array(KeyObject,TEXT("safe"))[0]->AsString();S->NarrativeState35->SetStringField(TEXT("aux_text"),Key);G.Show(TEXT("story35"));Tick(*S,.5f);Tick(*S,.5f);
  Check(TEXT("story_ui_does_not_pause_world"),!G.GetWorld()->GetFirstPlayerController()->IsPaused());
  Check(TEXT("main_dialogue_resolves_speaker_in_world"),S->DialogueActor62==TEXT("FILE_30"));
  const FString OtherLine=Array(KeyObject,TEXT("door"))[0]->AsString();S->NarrativeState35->SetStringField(TEXT("aux_text"),OtherLine);Tick(*S,.5f);Check(TEXT("different_personal_line_does_not_inherit_finished_page_cursor"),S->DialogueKey62==OtherLine);
  const int Cursor=S->DialogueIndex62;Stop(*S,true);Check(TEXT("interruption_does_not_mark_unfinished_line_complete"),S->DialogueIndex62==Cursor);
  G.Show(TEXT(""));S->ActiveScene35.Empty();
  Json Saved=S->NarrativeSnapshot35();Check(TEXT("conversation_cooldowns_in_narrative_save"),Object(Saved,TEXT("dialogue62"))->HasField(TEXT("event:FILE_30:danger")));
  S->RestoreNarrative35(Saved);Check(TEXT("conversation_progress_roundtrips"),Object(S->NarrativeState35,TEXT("dialogue62"))->HasField(TEXT("event:FILE_30:danger")));
  Stop(*S,true);G.Show(TEXT(""));S->ActiveScene35.Empty();G.Values.Add(TEXT("hospital_chapter"),3);
  auto* One=S->Props.FindRef(TEXT("NEW_NPC_MARINA"));auto* Two=S->Props.FindRef(TEXT("NEW_NPC_RAYA"));
  One->FindComponentByClass<UEWardPerson54>()->RestoreHuman58(MakeShared<FJsonObject>());Two->FindComponentByClass<UEWardPerson54>()->RestoreHuman58(MakeShared<FJsonObject>());
  One->SetActorLocation(Arena+FVector(300,0,.7));Two->SetActorLocation(Arena+FVector(480,0,.7));One->SetActorHiddenInGame(false);Two->SetActorHiddenInGame(false);H->SetActorLocation(Arena+FVector(700,-200,91));
  Json Group=Array(Data(),TEXT("groups"))[0]->AsObject();Check(TEXT("nearby_npc_pair_can_converse_without_player_interaction"),PairReady(*S,Group));
  auto* Divider=G.GetWorld()->SpawnActor<AActor>();auto* DividerBox=NewObject<UBoxComponent>(Divider);Divider->SetRootComponent(DividerBox);DividerBox->SetBoxExtent(FVector(8,180,150));DividerBox->SetCollisionProfileName(TEXT("BlockAll"));DividerBox->RegisterComponent();Divider->SetActorLocation(Arena+FVector(390,0,140));Check(TEXT("background_pair_does_not_converse_through_closed_wall"),!PairReady(*S,Group));Divider->Destroy();
  Two->SetActorLocation(Arena+FVector(480,0,480));Check(TEXT("background_dialogue_rejects_different_floors"),!PairReady(*S,Group));Two->SetActorLocation(Arena+FVector(480,0,.7));
  Begin(*S,String(Group,TEXT("id")),Array(Group,TEXT("lines")),false);Tick(*S,.5f);Check(TEXT("background_first_speaker_is_correct"),S->DialogueActor62==TEXT("NEW_NPC_RAYA"));
  Tick(*S,60);Check(TEXT("background_second_speaker_waits_then_answers"),S->DialogueActor62==TEXT("NEW_NPC_MARINA"));Tick(*S,60);
  Check(TEXT("finished_background_conversation_is_saved_once"),Boolean(Object(S->NarrativeState35,TEXT("dialogue62")),TEXT("done:WORLD_RECEPTION")));
  Event(*S,TEXT("FILE_30"),TEXT("safe"));Tick(*S,.5f);Runner->Impact58(200,FVector(30,0,0));Tick(*S,.1f);Check(TEXT("dying_speaker_stops_talking"),!Speaking(*S,TEXT("FILE_30")));Runner->RestoreHuman58(MakeShared<FJsonObject>());
  One->SetActorHiddenInGame(true);Two->SetActorHiddenInGame(true);G.Values.Add(TEXT("hospital_chapter"),1);
  Stop(*S,true);A->SetActorLocation(Arena+FVector(0,0,.7));Runner->RestoreHuman58(MakeShared<FJsonObject>());Mob->Active=true;Mob->SetActorHiddenInGame(false);Mob->SetActorLocation(Arena+FVector(-380,0,88));Mob->AttackClock=100;
  Runner->TickHuman58(.6f,false);Before=A->GetActorLocation();Mob->Active=false;Mob->SetActorHiddenInGame(true);Time=0;Stage=1;
 }
 else if(Stage==1)
 {
  if(Time>1.5f)
  {
   Check(TEXT("npc_runs_away_with_real_component_ticks"),Runner->HumanState58()==TEXT("flee")&&FVector::Dist2D(Before,Runner->GetOwner()->GetActorLocation())>80);
   const auto& SafeLines74=Array(Object(Object(Data(),TEXT("events")),TEXT("FILE_30")),TEXT("door"));
   Begin(*S,TEXT("event/final_probe74"),SafeLines74,false);
   G.Values.Add(TEXT("hospital_chapter"),20);S->SetNarrativeFlag35(TEXT("EPILOGUE_COMPLETE"),true);S->ActiveScene35.Empty();
   Tick(*S,.1f);Check(TEXT("epilogue_stops_incidental_speech_and_subtitle"),S->DialogueSession62.IsEmpty()&&S->DialogueKey62.IsEmpty()&&!IsValid(S->DialogueAudio62));
   State(*S)->RemoveField(TEXT("event:FILE_30:door"));Event(*S,TEXT("FILE_30"),TEXT("door"));
   Check(TEXT("epilogue_does_not_restart_death_or_door_reactions"),S->DialogueSession62.IsEmpty()&&!State(*S)->HasField(TEXT("event:FILE_30:door")));
   Stop(*S,true);bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/dialogue62_check.json"),R);Stage=2;FPlatformMisc::RequestExit(false);
  }
 }
}
}
