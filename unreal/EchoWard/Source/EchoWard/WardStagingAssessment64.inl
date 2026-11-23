#include "WardPeople58.h"
#include "Components/BoxComponent.h"
#include "HighResScreenshot.h"
#include "Camera/CameraActor.h"
#include "UnrealClient.h"
namespace WardStaging64
{
void Assess(AEWardCampaignMode& G,float Delta)
{
 static int Phase=0;static float Time=0;static bool Captured=false,Restored=false;static FVector Before;static float MaxStep=0;static TArray<TSharedPtr<FJsonValue>> Checks;static Json Snapshot;
 auto* S=G.HospitalStory.Get();auto* H=G.Hero();if(!S||!H)return;Time+=FApp::GetDeltaTime();
 auto Check=[&](const FString& Name,bool Pass){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));UE_LOG(LogTemp,Display,TEXT("STAGING64 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
 const FVector Arena(-70000,-70000,0);
 if(Phase==0)
 {
  G.CampaignStarted=true;G.TitleView=false;G.Admin=false;G.Agent=false;G.Show(TEXT(""));S->HomeStage52=-1;S->ArrivalStage=-1;S->NarrativeTesting35=false;S->FreezeAI=true;G.Invulnerable=1000;
  for(auto M:S->Mobs){M->Active=false;M->SetActorHiddenInGame(true);M->SetActorEnableCollision(false);M->GetCharacterMovement()->DisableMovement();}
  Check(TEXT("78_authored_scene_records"),Object(Data(),TEXT("scenes"))->Values.Num()==78);
  for(auto E:Object(Data(),TEXT("scenes"))->Values)
  {
   const Json Def=E.Value->AsObject();bool Resolved=S->NarrativeScenes35.Contains(FString(E.Key))&&S->Props.Contains(FString(E.Key));for(auto Id:Array(Def,TEXT("cast")))Resolved&=S->Props.Contains(Id->AsString())&&!Boolean(S->Definitions.FindRef(Id->AsString()),TEXT("past57"));Check(FString(E.Key)+TEXT("_source_and_cast_resolve"),Resolved&&!Array(Def,TEXT("lead")).IsEmpty());
   S->ActiveScene35=FString(E.Key);Root(*S)->SetObjectField(TEXT("active"),MakeShared<FJsonObject>());FVector At;Check(FString(E.Key)+TEXT("_has_supported_clear_standing_spot"),Spot(*S,S->Props[FString(E.Key)]->GetActorLocation(),0,At));
  }
  S->ActiveScene35.Empty();Reset(*S);
  auto* Floor=G.GetWorld()->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(Floor);Floor->SetRootComponent(B);B->SetBoxExtent(FVector(2200,2200,10));B->SetCollisionProfileName(TEXT("BlockAll"));B->RegisterComponent();Floor->SetActorLocation(Arena-FVector(0,0,10));
  auto* Wall=G.GetWorld()->SpawnActor<AActor>();auto* WM=NewObject<UStaticMeshComponent>(Wall);Wall->SetRootComponent(WM);WM->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));WM->SetCollisionEnabled(ECollisionEnabled::NoCollision);WM->RegisterComponent();Wall->SetActorLocation(Arena+FVector(700,0,140));Wall->SetActorScale3D(FVector(.1,8,3));
  auto* LightActor=G.GetWorld()->SpawnActor<AActor>();auto* L=NewObject<UPointLightComponent>(LightActor);LightActor->SetRootComponent(L);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(800);L->SetAttenuationRadius(1800);L->SetLightingChannels(true,true,false);L->RegisterComponent();LightActor->SetActorLocation(Arena+FVector(200,-80,280));
  H->SetActorLocation(Arena+FVector(0,-450,91));H->Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;H->Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;H->Camera->PostProcessSettings.AutoExposureMinBrightness=6;H->Camera->PostProcessSettings.AutoExposureMaxBrightness=6;H->Camera->PostProcessSettings.bOverride_AutoExposureBias=true;H->Camera->PostProcessSettings.AutoExposureBias=-1;
  if(auto* PC=G.GetWorld()->GetFirstPlayerController())PC->SetControlRotation((Arena+FVector(450,0,100)-H->Camera->GetComponentLocation()).Rotation());
  for(auto Pair:S->Props)if(Person(*S,Pair.Key)){Pair.Value->SetActorHiddenInGame(true);Pair.Value->SetActorEnableCollision(false);}
  // A participant placed close to the player must be able to leave a
  // narrow doorway, while approach spacing and actual collisions still hold.
  auto* Yield74=Person(*S,TEXT("PERSON_3"));auto* YieldActor74=S->Props[TEXT("PERSON_3")];
  Yield74->LoadAppearance();Yield74->RestoreHuman58(MakeShared<FJsonObject>());YieldActor74->SetActorHiddenInGame(false);YieldActor74->SetActorEnableCollision(true);
  H->SetActorLocation(Arena+FVector(0,0,98));YieldActor74->SetActorLocation(Arena+FVector(100,0,.7));
  Yield74->SetTask58(TEXT("scene"),Arena+FVector(450,0,.7));float Step74=0;
  for(int I=0;I<20;++I){const FVector Old74=YieldActor74->GetActorLocation();Yield74->MoveTo58(Arena+FVector(450,0,.7),.1f,80);Step74=FMath::Max(Step74,float(FVector::Dist(Old74,YieldActor74->GetActorLocation())));}
  Check(TEXT("nearby_npc_can_walk_away_from_player_to_clear_passage"),YieldActor74->GetActorLocation().X>Arena.X+150);
  Check(TEXT("clearing_passage_keeps_bounded_walk_steps"),Step74<=8.1f);
  YieldActor74->SetActorLocation(Arena+FVector(100,0,.7));Yield74->SetTask58(TEXT("scene"),Arena+FVector(0,0,.7));
  Yield74->MoveTo58(Arena+FVector(0,0,.7),.1f,80);
  Check(TEXT("nearby_npc_still_keeps_spacing_when_approaching_player"),YieldActor74->GetActorLocation().Equals(Arena+FVector(100,0,.7),.01));
  auto* YieldWall74=G.GetWorld()->SpawnActor<AActor>();auto* YieldBox74=NewObject<UBoxComponent>(YieldWall74);YieldWall74->SetRootComponent(YieldBox74);YieldBox74->SetBoxExtent(FVector(5,180,120));YieldBox74->SetCollisionProfileName(TEXT("BlockAll"));YieldBox74->RegisterComponent();YieldWall74->SetActorLocation(Arena+FVector(132,0,120));
  Yield74->SetTask58(TEXT("scene"),Arena+FVector(450,0,.7));Yield74->MoveTo58(Arena+FVector(450,0,.7),.1f,80);
  Check(TEXT("npc_clearing_passage_does_not_cross_solid_wall"),YieldActor74->GetActorLocation().Equals(Arena+FVector(100,0,.7),.01));
  YieldBox74->SetCollisionEnabled(ECollisionEnabled::NoCollision);YieldWall74->Destroy();YieldActor74->SetActorHiddenInGame(true);YieldActor74->SetActorEnableCollision(false);Yield74->RestoreHuman58(MakeShared<FJsonObject>());
  WardDialogue62::Stop(*S,true);H->SetActorLocation(Arena+FVector(0,-450,91));
  S->Props[TEXT("SCENE_02_02")]->SetActorLocation(Arena+FVector(420,0,0));S->ActiveScene35=TEXT("SCENE_02_02");S->SelectedChoice35.Empty();Root(*S)->SetObjectField(TEXT("active"),MakeShared<FJsonObject>());
  int N=0;for(auto V:Array(Scene(*S),TEXT("cast"))){const FString Id=V->AsString();auto* P=Person(*S,Id);P->LoadAppearance();P->RestoreHuman58(MakeShared<FJsonObject>());S->Props[Id]->SetActorLocation(Arena+FVector(400,N++*130-160,0.7));S->Props[Id]->SetActorEnableCollision(true);S->Props[Id]->SetActorHiddenInGame(false);}
  Person(*S,TEXT("PERSON_4"))->Impact58(35,FVector::ZeroVector);
  G.Values.Add(TEXT("hospital_chapter"),2);G.Values.Add(TEXT("hospital_step"),1);Begin(*S,S->ActiveScene35);G.Show(TEXT("story35"));Before=S->Props[TEXT("NEW_NPC_MARINA")]->GetActorLocation();
  Check(TEXT("scene_does_not_teleport_participants_on_begin"),FVector::Dist(Before,S->Props[TEXT("NEW_NPC_MARINA")]->GetActorLocation())<.1);
  Check(TEXT("finish_blocked_before_physical_action"),!CanFinish(*S));Check(TEXT("fallen_patient_is_not_ordered_to_walk_to_a_marker"),Vector(Array(Object(Child(Current(*S),TEXT("goals")),TEXT("PERSON_4")),TEXT("at"))).Equals(S->Props[TEXT("PERSON_4")]->GetActorLocation(),.1));Time=0;Phase=1;
 }
 else if(Phase==1)
 {
  const FVector At=S->Props[TEXT("NEW_NPC_MARINA")]->GetActorLocation();MaxStep=FMath::Max(MaxStep,float(FVector::Dist(At,Before)));Before=At;
  if(Number(Current(*S),TEXT("index"))==1&&Number(Current(*S),TEXT("elapsed"))>2.5&&!Captured){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/staging64_help.png"),true,false);Captured=true;}
  if(Number(Current(*S),TEXT("index"))==1&&Number(Current(*S),TEXT("elapsed"))>1&&!Restored)
  {
   Snapshot=S->NarrativeSnapshot35();const double Clock=Number(Current(*S),TEXT("elapsed"));G.Show(TEXT("pause"));Tick(*S,5);Check(TEXT("pause_does_not_advance_staging_clock"),Number(Current(*S),TEXT("elapsed"))==Clock);G.Show(TEXT("story35"));
   S->RestoreNarrative35(Snapshot);Check(TEXT("save_restores_action_index_and_time"),Number(Current(*S),TEXT("index"))==1&&Number(Current(*S),TEXT("elapsed"))==Clock);Restored=true;
  }
  if(CanFinish(*S)||Time>35)
  {
   if(!CanFinish(*S)){UE_LOG(LogTemp,Warning,TEXT("STAGE64_DIAG %s %s"),*Serialize(Current(*S)),*S->ActiveScene35);for(auto E:Child(Current(*S),TEXT("goals"))->Values)if(auto A=S->Props.FindRef(FString(E.Key)))UE_LOG(LogTemp,Warning,TEXT("STAGE64_ACTOR %s at=%s goal=%s state=%s"),*FString(E.Key),*A->GetActorLocation().ToString(),*Vector(Array(E.Value->AsObject(),TEXT("at"))).ToString(),*Person(*S,FString(E.Key))->HumanState58());}
   Check(TEXT("real_component_ticks_complete_help_scene"),CanFinish(*S));Check(TEXT("scene_actor_movement_is_bounded_per_frame"),MaxStep<55);Check(TEXT("mid_scene_save_was_exercised"),Restored);Check(TEXT("helper_really_starts_getting_fallen_patient_up"),!Person(*S,TEXT("PERSON_4"))->IsFallen58());Check(TEXT("runtime_uses_real_medical_models"),LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/HospitalV37/Meshes/SM_H37_3302.SM_H37_3302"))!=nullptr);
   for(auto E:Child(Current(*S),TEXT("goals"))->Values)if(Alive(*S,FString(E.Key)))Person(*S,FString(E.Key))->SetTask58(TEXT("wait"));
   Phase=2;Time=0;
  }
 }
 else if(Phase==2)
 {
  // Explicit fixtures isolate each ending's effects; live characters reached their places above.
  G.Show(TEXT("story35"));
  for(const FString P:{TEXT("CURE"),TEXT("DESTROY"),TEXT("EVAC"),TEXT("REPLACE"),TEXT("CONTINUE")})
  {
   Reset(*S);Root(*S)->SetObjectField(TEXT("active"),MakeShared<FJsonObject>());Root(*S)->SetObjectField(TEXT("events"),MakeShared<FJsonObject>());Root(*S)->SetObjectField(TEXT("evacuated"),MakeShared<FJsonObject>());S->SetNarrativeFlag35(TEXT("HERO_STAYS"),false);S->SetNarrativeFlag35(TEXT("IRINA_ALIVE"),true);Object(S->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),P);
   S->ActiveScene35=TEXT("SCENE_19_01");S->SelectedChoice35=TEXT("EXECUTE");S->Props[S->ActiveScene35]->SetActorLocation(Arena+FVector(420,0,0));
   for(const FString Id:{TEXT("PERSON_3"),TEXT("NEW_NPC_001")}){Person(*S,Id)->RestoreHuman58(MakeShared<FJsonObject>());S->Props[Id]->SetActorLocation(Arena+FVector(400,130,0.7));}
   auto C=Current(*S);C->SetStringField(TEXT("id"),S->ActiveScene35);C->SetNumberField(TEXT("index"),Array(Scene(*S),TEXT("lead")).Num());C->SetNumberField(TEXT("elapsed"),0);int Confirm=0;
   for(int I=0;I<240;++I){if(WaitingAction(*S)){Command(*S,TEXT("story35_action64"));++Confirm;}Tick(*S,.1f);}
   Check(P+TEXT("_three_explicit_operation_confirmations"),Confirm==3);Check(P+TEXT("_all_three_circuits_committed"),Boolean(Child(Root(*S),TEXT("circuits")),P+TEXT("/0"))&&Boolean(Child(Root(*S),TEXT("circuits")),P+TEXT("/1"))&&Boolean(Child(Root(*S),TEXT("circuits")),P+TEXT("/2")));
   Check(P+TEXT("_operation_saved_once"),String(Root(*S),TEXT("operation_done"))==P);
   if(P==TEXT("DESTROY")){Check(TEXT("destruction_timer_starts_after_last_isolation"),Number(Root(*S),TEXT("escape_seconds"))==180);Check(TEXT("001_physically_dies_after_isolation"),Person(*S,TEXT("NEW_NPC_001"))->IsDead58());}
   if(P==TEXT("CURE"))Check(TEXT("cure_does_not_kill_sister"),!Person(*S,TEXT("PERSON_3"))->IsDead58());
   Check(P+TEXT("_no_automatic_helicopter_for_other_plans"),!World(*S).Vehicle.IsValid());
  }
  H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Root(*S)->SetNumberField(TEXT("escape_seconds"),-1);
  Object(S->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("EVAC"));S->ActiveScene35=TEXT("SCENE_19_03");S->SelectedChoice35.Empty();S->Props[S->ActiveScene35]->SetActorLocation(Arena+FVector(300,0,0));Reset(*S);auto C=MakeShared<FJsonObject>();Root(*S)->SetObjectField(TEXT("active"),C);C->SetStringField(TEXT("id"),S->ActiveScene35);C->SetNumberField(TEXT("index"),0);C->SetNumberField(TEXT("elapsed"),0);
  for(const FString Id:{TEXT("NEW_NPC_MARINA"),TEXT("NEW_NPC_RAYA"),TEXT("NEW_NPC_LEONID"),TEXT("PERSON_4"),TEXT("FILE_33"),TEXT("FILE_36")}){Person(*S,Id)->RestoreHuman58(MakeShared<FJsonObject>());S->Props[Id]->SetActorLocation(Anchor(*S)+FVector(330,130,.7));S->Props[Id]->SetActorHiddenInGame(false);}
  Child(C,TEXT("boarding"))->SetBoolField(TEXT("FILE_30"),false);
  Person(*S,TEXT("FILE_30"))->Impact58(200,FVector::ZeroVector,true);
  for(int I=0;I<100;++I)Tick(*S,.1);
  Check(TEXT("boarding_casualty_does_not_block_departure_or_count_as_evacuated"),
      Boolean(Child(C,TEXT("boarding")),TEXT("FILE_30"))&&!Boolean(Child(Root(*S),TEXT("evacuated")),TEXT("FILE_30")));
  Check(TEXT("evacuation_has_3D_aircraft_and_rotor"),World(*S).Vehicle.IsValid()&&World(*S).Rotor.IsValid());Check(TEXT("civilians_board_and_are_saved_as_evacuated"),Child(Root(*S),TEXT("evacuated"))->Values.Num()>=4);Check(TEXT("hero_and_sister_never_board_civilian_flight"),!Boolean(Child(Root(*S),TEXT("evacuated")),TEXT("PERSON_3"))&&!H->IsHidden());Check(TEXT("aircraft_really_rises_from_roof"),World(*S).Vehicle.IsValid()&&World(*S).Vehicle->GetActorLocation().Z>Anchor(*S).Z+1000);
  auto* Aircraft=World(*S).Vehicle.Get();
  TArray<UStaticMeshComponent*> Parts;
  if(Aircraft)Aircraft->GetComponents(Parts);
  const auto* Body=Parts.FindByPredicate([](const UStaticMeshComponent* Part)
  {
   const UStaticMesh* Mesh=Part->GetStaticMesh();
   return Mesh&&Mesh->GetStaticMaterials().Num()==6&&Mesh->GetBoundingBox().GetSize().X>800;
  });
  Check(TEXT("evac_aircraft_has_six_materials_and_two_independent_rotors"),
        Body&&World(*S).Rotor.IsValid()&&World(*S).TailRotor.IsValid());
  Snapshot=S->NarrativeSnapshot35();S->RestoreNarrative35(Snapshot);RefreshAnchors(*S);Check(TEXT("evacuated_people_remain_hidden_after_load"),S->Props[TEXT("NEW_NPC_MARINA")]->IsHidden());
  Object(S->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("REPLACE"));RefreshAnchors(*S);Check(TEXT("replacement_exit_remains_in_centre"),S->Props[TEXT("SCENE_19_03")]->GetActorLocation().Equals(S->Props[TEXT("SCENE_19_01")]->GetActorLocation(),1));
  Object(S->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("EVAC"));RefreshAnchors(*S);Check(TEXT("evacuation_followup_is_quarantine_not_hero_car"),S->Props[TEXT("SCENE_19_04")]->GetActorLocation().Equals(S->Props[TEXT("SCENE_16_01")]->GetActorLocation(),1));
  Object(S->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("DESTROY"));S->SetNarrativeFlag35(TEXT("HERO_STAYS"),true);RefreshAnchors(*S);Check(TEXT("staying_ending_does_not_move_hero_to_parking"),S->Props[TEXT("SCENE_19_04")]->GetActorLocation().Equals(S->Props[TEXT("SCENE_19_01")]->GetActorLocation(),1));
  Object(S->NarrativeState35,TEXT("choices"))->SetStringField(TEXT("FINAL_PLAN"),TEXT("CURE"));
  S->SetNarrativeFlag35(TEXT("HERO_STAYS"),false);S->RefreshNarrative35();
  bool Returned=true;for(const TCHAR* Id:{TEXT("SCENE_19_02"),TEXT("SCENE_19_04"),TEXT("SCENE_19_05")})
      Returned&=S->Props[Id]->GetActorLocation().Equals(Vector(Array(S->Definitions[Id],TEXT("at")))*100,1);
  Check(TEXT("earlier_plan_does_not_leave_ending_anchors_in_wrong_locations"),Returned);
  S->ActiveScene35=TEXT("SCENE_19_01");Root(*S)->SetObjectField(TEXT("active"),MakeShared<FJsonObject>());Current(*S)->SetStringField(TEXT("id"),S->ActiveScene35);World(*S).Portal=-1;
  int DoorId=-1;for(int I=0;I<G.BookWorld->Portals.Num();++I)if(!G.BookWorld->Portals[I].Locked&&G.BookWorld->Portals[I].Role!=TEXT("niche")&&!G.BookWorld->Portals[I].VisibleParts.IsEmpty()){DoorId=I;break;}
  if(DoorId>=0){auto& P=G.BookWorld->Portals[DoorId];S->Props[S->ActiveScene35]->SetActorLocation(P.At+FVector(120,0,0));H->SetActorLocation(P.At+FVector(0,0,91));Check(TEXT("closing_door_waits_for_player_in_opening"),!Door(*S,false,1));H->SetActorLocation(P.At+FVector(450,0,91));Check(TEXT("native_door_moves_and_updates_collision"),Door(*S,false,1)&&!P.Open);Door(*S,true,1);}else Check(TEXT("native_door_fixture_exists"),false);
  if(FParse::Param(FCommandLine::Get(),TEXT("WardVisualAircraft67")))
  {
   Reset(*S);
   G.Show(TEXT(""));
   S->Props[S->ActiveScene35]->SetActorLocation(FVector(4220,4800,1440));
   Vehicle(*S,true);
   S->ActiveScene35.Empty();
   G.Show(TEXT(""));
   const FVector Eye(5200,4070,1800);
   H->SetActorLocation(Eye-FVector(0,0,60),false);
   H->SetActorHiddenInGame(true);
   G.BookWorld->SetBright(true);
   auto* Camera=G.GetWorld()->SpawnActor<ACameraActor>();
   Camera->SetActorLocationAndRotation(Eye,(FVector(4470,4800,1570)-Eye).Rotation());
   Camera->GetCameraComponent()->SetFieldOfView(72);
   Camera->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;
   G.GetWorld()->GetFirstPlayerController()->SetViewTarget(Camera);
  }
  else FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/staging64_final.png"),true,false);
  Phase=3;Time=0;
 }
 else if(Phase==3&&Time>1)
 {
  if(FParse::Param(FCommandLine::Get(),TEXT("WardVisualAircraft67"))&&Time<2)
  {
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/aircraft67.png"),true,false);
   Time=2;
   return;
  }
  bool All=true;for(auto V:Checks)All&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),All);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/staging64_check.json"),R);Phase=4;FPlatformMisc::RequestExit(false);
 }
}
}
