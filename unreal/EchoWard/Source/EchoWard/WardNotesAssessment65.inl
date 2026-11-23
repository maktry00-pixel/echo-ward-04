#include "UnrealClient.h"
namespace WardNotes65
{
static void Fixture(AEWardHospitalGameplay& S,const FString& Id)
{
 const Json P=S.Definitions.FindRef(Id),R=Object(P,TEXT("rule65"));S.SetChapter(Number(P,TEXT("chapter"),1));S.ActiveScene35.Empty();S.SelectedChoice35.Empty();
 for(auto V:Array(R,TEXT("requires")))S.Mode->Completed.Add(V->AsString());
 for(auto V:Array(R,TEXT("flags")))S.SetNarrativeFlag35(V->AsString(),true);
 for(auto V:Array(R,TEXT("not_flags")))S.SetNarrativeFlag35(V->AsString(),false);
 for(auto V:Array(R,TEXT("any_flags")))S.SetNarrativeFlag35(V->AsString(),true);
 if(!Array(R,TEXT("plans")).IsEmpty())Object(S.NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),Array(R,TEXT("plans"))[0]->AsString());
 if(Boolean(R,TEXT("operation_done")))Field(S.NarrativeState35,TEXT("staging64"))->SetStringField(TEXT("operation_done"),String(Object(S.NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN")));
 // Only this fixture sets scene prerequisites; it is not a chapter playthrough.
 if(Id==TEXT("DOC_V2_05"))S.SetNarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65"),true);
}
void Assess(AEWardCampaignMode& G,float Delta)
{
 static int Phase=0;static float Wait=0;static TArray<TSharedPtr<FJsonValue>> Checks,Spatial;auto* S=G.HospitalStory.Get();if(!S||!G.Hero())return;
 auto Check=[&](const FString& Name,bool Pass){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));if(!Pass)UE_LOG(LogTemp,Error,TEXT("NOTES65 FAIL %s"),*Name);};
 if(Phase==0)
 {
  S->NarrativeTesting35=true;S->FreezeAI=true;G.NewGame(TEXT("normal"));G.CampaignStarted=true;G.TitleView=false;G.Admin=false;G.Agent=false;G.Show(TEXT(""));S->HomeStage52=-1;S->ArrivalStage=-1;G.Invulnerable=1000;
  for(auto M:S->Mobs){M->Active=false;M->SetActorHiddenInGame(true);M->SetActorEnableCollision(false);}
  Check(TEXT("103_documents_have_complete_texts"),Object(S->Narrative35,TEXT("documents"))->Values.Num()==103);
  Check(TEXT("21_practical_hints_are_in_real_document_bodies"),Number(Data(),TEXT("hints_added"))==21&&S->DocumentBody(TEXT("@doc:NOTE_01_02")).Contains(TEXT("R06"))&&S->DocumentBody(TEXT("@doc:NOTE_16_03")).Contains(TEXT("HS17")));
  Check(TEXT("old_wrong_names_and_prop_labels_repaired"),S->DocumentBody(TEXT("@doc:NOTE_04_02")).Contains(TEXT("Сотрудницу Құрбанәлі"))&&!S->DocumentBody(TEXT("@doc:NOTE_03_04")).Contains(TEXT("Вика"))&&String(S->Definitions.FindRef(TEXT("NEW_TOY_01")),TEXT("label"))==TEXT("Жестяная птица Аружан"));
  S->SetChapter(11);bool SideEntrance=false;for(auto& P:G.BookWorld->Portals)if(P.Zone==TEXT("PS11")&&!P.InsideOnly)SideEntrance|=!P.Locked;Check(TEXT("PS11_side_story_has_an_outside_usable_entrance"),SideEntrance);S->SetChapter(1);
  S->UseId(TEXT("visitor_pass"));Check(TEXT("visitor_pass_does_not_grant_staff_badge"),G.Completed.Contains(TEXT("visitor_pass"))&&!G.Flag(TEXT("badge")));
  S->UseId(TEXT("staff_pass"));Check(TEXT("staff_pass_waits_for_actual_death_scene"),!G.Completed.Contains(TEXT("staff_pass")));
  S->SetChapter(7);S->UseId(TEXT("medical_clearance"));S->UseId(TEXT("evidence_case"));S->UseId(TEXT("quarantine_code"));Check(TEXT("clearance_case_and_quarantine_code_are_distinct_from_staff_badge"),!G.Flag(TEXT("badge"))&&G.Flag(TEXT("hospital_key_medical_clearance"))&&G.Flag(TEXT("hospital_key_evidence_case"))&&G.Flag(TEXT("hospital_key_quarantine_code")));
  G.Completed.Add(TEXT("SCENE_01_04"));S->UseId(TEXT("staff_pass"));Check(TEXT("actual_staff_pass_grants_staff_access"),G.Flag(TEXT("badge")));
  S->SetChapter(5);for(const FString Id:{TEXT("DOC_V2_03"),TEXT("DOC_V2_04"),TEXT("DOC_V2_05"),TEXT("DOC_V2_07")})
  {const int N=G.Journal.Num();S->UseId(Id);Check(Id+TEXT("_not_available_in_chapter_five"),!G.Completed.Contains(Id)&&G.Journal.Num()==N);}
  S->SetChapter(18);S->SetNarrativeFlag35(TEXT("IRINA_CONSENT"),true);G.Completed.Add(TEXT("SCENE_18_04"));Object(S->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("CURE"));S->SetNarrativeFlag35(TEXT("MEMORY_SPARED"),true);
  Check(TEXT("only_preserving_consent_visible_for_preserving_cure"),Allowed(*S,TEXT("DOC_V2_03"))&&!Allowed(*S,TEXT("DOC_V2_04")));
  S->SetNarrativeFlag35(TEXT("MEMORY_SPARED"),false);Check(TEXT("only_basic_consent_visible_when_memory_table_missing"),!Allowed(*S,TEXT("DOC_V2_03"))&&Allowed(*S,TEXT("DOC_V2_04")));
  Check(TEXT("shell_cannot_be_collected_before_operation"),!Allowed(*S,TEXT("NEW_FINAL_SHEATH")));
  Field(S->NarrativeState35,TEXT("staging64"))->SetStringField(TEXT("operation_done"),TEXT("CURE"));S->SetChapter(19);Check(TEXT("shell_available_only_after_completed_cure_operation"),Allowed(*S,TEXT("NEW_FINAL_SHEATH")));
  Object(S->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("DESTROY"));Check(TEXT("destroy_branch_cannot_collect_cure_shell_or_consent"),!Allowed(*S,TEXT("NEW_FINAL_SHEATH"))&&!Allowed(*S,TEXT("DOC_V2_03"))&&!Allowed(*S,TEXT("DOC_V2_04")));
  S->SetChapter(10);G.Completed.Add(TEXT("SCENE_10_02"));S->SetNarrativeFlag35(TEXT("IDENTITY_CHECKED"),true);S->SetNarrativeFlag35(TEXT("IMPOSTOR_RELEASED"),false);Check(TEXT("alarm_report_does_not_appear_in_verified_branch"),!Allowed(*S,TEXT("NOTE_10_03"))&&Allowed(*S,TEXT("NOTE_10_05")));
  S->SetNarrativeFlag35(TEXT("IDENTITY_CHECKED"),false);S->SetNarrativeFlag35(TEXT("IMPOSTOR_RELEASED"),true);Check(TEXT("release_branch_has_own_report_not_verified_report"),Allowed(*S,TEXT("NOTE_10_03"))&&!Allowed(*S,TEXT("NOTE_10_05")));
  G.Completed.Empty();G.Journal.Empty();S->ResetNarrative35();S->SetChapter(16);S->SetNarrativeFlag35(TEXT("TRUTH_SENT"),true);S->SetNarrativeFlag35(TEXT("SEMYON_HELPED"),true);S->SetNarrativeFlag35(TEXT("FULL_PROTOCOL_FRAGMENT"),true);S->PreparePlans35();
  Check(TEXT("plot_flags_alone_do_not_fabricate_documentary_evidence"),!S->NarrativeFlag35(TEXT("TRUTH_READY"))&&!S->NarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65")));
  Json Send;for(auto V:Array(S->NarrativeScenes35[TEXT("SCENE_16_03")],TEXT("choices")))if(String(V->AsObject(),TEXT("id"))==TEXT("SEND"))Send=V->AsObject();FString Why;
  Check(TEXT("send_requires_actual_documents_and_names_missing_locations"),Send&&!S->NarrativeRequirements35(Send,&Why)&&Why.Contains(TEXT("E17"))&&Why.Contains(TEXT("HS17")));
  const int BeforeRead=Object(State(*S),TEXT("read"))->Values.Num();S->DocumentBody(TEXT("@doc:NOTE_07_01"));Read(*S,TEXT("@doc:NOTE_07_01"));Check(TEXT("previewing_uncollected_note_does_not_grant_read_or_evidence"),Object(State(*S),TEXT("read"))->Values.Num()==BeforeRead&&!S->NarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65")));

  // Audit local support/standing at the chapter declared for every actual pickup.
  // This checks furnished scene geometry, not complete routes across all chapters.
  TArray<FString> IDs;for(auto E:S->Definitions)if(TArray<FString>{TEXT("note"),TEXT("key"),TEXT("story_object"),TEXT("map"),TEXT("weapon"),TEXT("shotgun"),TEXT("ammo"),TEXT("medkit"),TEXT("shells")}.Contains(String(E.Value,TEXT("kind"))))IDs.Add(E.Key);IDs.Sort();
  for(const auto& Id:IDs)
  {
   const Json P=S->Definitions[Id];Fixture(*S,Id);FVector Feet;const bool Clear=S->FindNarrativeStanding35(Id,Feet);Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),Id);Row->SetNumberField(TEXT("chapter"),Number(P,TEXT("chapter")));Row->SetStringField(TEXT("zone"),String(P,TEXT("zone")));Row->SetStringField(TEXT("site"),String(P,TEXT("site")));Row->SetStringField(TEXT("support"),String(P,TEXT("support65")));Row->SetBoolField(TEXT("local_approach_clear"),Clear);const FVector At=S->Props[Id]->GetActorLocation()/100;Row->SetArrayField(TEXT("at_m"),{MakeShared<FJsonValueNumber>(At.X),MakeShared<FJsonValueNumber>(At.Y),MakeShared<FJsonValueNumber>(At.Z)});
   Check(Id+TEXT("_local_approach_at_declared_chapter"),Clear);
   if(String(P,TEXT("kind"))==TEXT("note"))
   {
    G.Completed.Remove(Id);S->ActiveScene35.Empty();G.Hero()->SetActorLocation(Feet,false);for(auto E:S->Props){E.Value->SetActorHiddenInGame(E.Key!=Id);E.Value->SetActorEnableCollision(E.Key==Id);}S->Props[Id]->SetActorHiddenInGame(false);const bool Used=Clear&&S->Interact();
    const int N=G.Journal.Num();S->ActiveScene35.Empty();S->UseId(Id);const bool Unique=G.Journal.Num()==N;const bool Taken=G.Completed.Contains(Id)&&G.Journal.Contains(TEXT("@doc:")+Id);Check(Id+TEXT("_real_interaction_collects_once"),Used&&Taken&&Unique);Row->SetBoolField(TEXT("collected_by_interaction"),Used&&Taken&&Unique);
    const int ReadBefore=Object(State(*S),TEXT("read"))->Values.Num();Read(*S,TEXT("@doc:")+Id);Read(*S,TEXT("@doc:")+Id);Check(Id+TEXT("_journal_read_is_persistent_and_unique"),Boolean(Object(State(*S),TEXT("read")),Id)&&Object(State(*S),TEXT("read"))->Values.Num()<=ReadBefore+1);
   }
   Spatial.Add(MakeShared<FJsonValueObject>(Row));
  }
  S->ActiveScene35.Empty();S->SetChapter(16);Sync(*S);Check(TEXT("all_three_evidence_groups_unlock_full_packet"),S->NarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65"))&&S->NarrativeRequirements35(Send));
  G.Completed.Remove(TEXT("NOTE_04_02"));Sync(*S);Check(TEXT("missing_source_disables_packet_even_if_index_note_collected"),!S->NarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65"))&&G.Completed.Contains(TEXT("NOTE_16_03")));
  S->UseId(TEXT("NOTE_04_02"));S->SetNarrativeFlag35(TEXT("TRUTH_SENT"),true);S->SetNarrativeFlag35(TEXT("SEMYON_HELPED"),true);S->SetNarrativeFlag35(TEXT("FULL_PROTOCOL_FRAGMENT"),true);S->PreparePlans35();Check(TEXT("actual_evidence_and_authorial_choices_unlock_truth_ending"),S->NarrativeFlag35(TEXT("TRUTH_READY")));
  Json Save=G.Capture();const int ReadCount=Object(State(*S),TEXT("read"))->Values.Num();S->ResetNarrative35();G.Restore(Save);Check(TEXT("campaign_save_restores_collected_read_and_evidence_states"),Object(State(*S),TEXT("read"))->Values.Num()==ReadCount&&ReadCount==103&&S->NarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65"))&&G.Journal.Contains(TEXT("@doc:NOTE_16_03")));
  S->SetNarrativeFlag35(TEXT("TRUTH_SENT"),false);S->PreparePlans35();Check(TEXT("finding_every_note_does_not_force_transmission_or_a_final_plan"),!S->NarrativeFlag35(TEXT("TRUTH_READY")));
  S->ActiveScene35.Empty();S->SetChapter(16);G.Show(TEXT("journal"));if(G.Interface.IsValid()){G.Interface->DocumentIndex=G.Journal.IndexOfByKey(TEXT("@doc:NOTE_16_03"));G.Interface->Refresh();}
  Phase=1;Wait=0;
 }
 else if(Phase==1)
 {
  Wait+=Delta;if(Wait<.5)return;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/notes65_journal.png"),true,false);Phase=2;Wait=0;
 }
 else if(Phase==2)
 {
  Wait+=Delta;if(Wait<.7)return;bool OK=true;for(auto V:Checks)OK&=Boolean(V->AsObject(),TEXT("passed"));Json Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),OK);Result->SetNumberField(TEXT("count"),Checks.Num());Result->SetArrayField(TEXT("checks"),Checks);Result->SetArrayField(TEXT("spatial"),Spatial);Result->SetNumberField(TEXT("documents"),103);Result->SetBoolField(TEXT("full_manual_playthrough"),false);WardData::Write(FPaths::ProjectSavedDir()/TEXT("Prototype/notes65_check.json"),Result);UE_LOG(LogTemp,Display,TEXT("NOTES65 COMPLETE %d checks success=%d"),Checks.Num(),OK);FPlatformMisc::RequestExit(false);Phase=3;
 }
}
}
