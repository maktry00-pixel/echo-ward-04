#include "Components/BoxComponent.h"
#include "Misc/App.h"
#include "HighResScreenshot.h"
#include "UnrealClient.h"
namespace WardCombat63
{
void Assess(AEWardCampaignMode& G,float Delta)
{
 static int Stage=0;static float Clock=0;static TArray<TSharedPtr<FJsonValue>> Checks;auto* H=G.Hero();auto* S=G.HospitalStory.Get();if(!H||!S)return;
 auto Check=[&](const FString& Name,bool Pass){Json R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));UE_LOG(LogTemp,Display,TEXT("COMBAT63 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
 Clock+=FApp::GetDeltaTime();
 if(Stage==0)
 {
  G.CampaignStarted=true;G.TitleView=false;G.Admin=false;G.Agent=false;G.Won=false;G.PendingRespawn=false;G.Show(TEXT(""));S->HomeStage52=-1;S->ArrivalStage=-1;S->FreezeAI=true;
  for(auto M:S->Mobs){M->Active=false;M->SetActorHiddenInGame(true);M->SetActorEnableCollision(false);M->GetCharacterMovement()->DisableMovement();}
  for(auto E:G.Enemies){E->Active=false;E->SetActorHiddenInGame(true);E->SetActorEnableCollision(false);}
  const FVector Arena(-70000,-70000,0);if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(FRotator::ZeroRotator);auto* F=G.GetWorld()->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(F);F->SetRootComponent(B);B->SetBoxExtent(FVector(1800,1800,10));B->SetCollisionProfileName(TEXT("BlockAll"));B->RegisterComponent();F->SetActorLocation(Arena-FVector(0,0,10));
  H->SetActorLocation(Arena+FVector(0,0,91));H->Dead=false;H->Health=100;H->Stamina=100;H->Rounds=2;H->Reserve=15;G.Invulnerable=0;G.Flag(TEXT("weapon"),true);G.Flag(TEXT("shotgun"),true);G.Shotgun=false;G.HidingBed=nullptr;if(G.BookWorld)G.BookWorld->FurnitureHiding=false;G.ReloadTime=0;
  Check(TEXT("pistol_range_close_full"),FMath::IsNearlyEqual(RangeScale(false,500),1.f));
  Check(TEXT("pistol_range_60m_reduced"),FMath::IsNearlyEqual(RangeScale(false,6000),.7f));
  Check(TEXT("shotgun_8m_full"),FMath::IsNearlyEqual(RangeScale(true,800),1.f));
  Check(TEXT("shotgun_30m_reduced"),FMath::IsNearlyEqual(RangeScale(true,3000),.18f));
  Check(TEXT("head_relative_height"),BodyScale(.9)==2&&BodyScale(.5)==1&&BodyScale(.15)==.65f);
  G.ReloadWeapon();Check(TEXT("reload_not_instant"),H->Rounds==2&&H->Reserve==15&&G.ReloadTime>1.7);
  G.TickHero(.3);Check(TEXT("magazine_moves_with_reload"),H->MagazineTravel()>5);
  const FVector Left=H->WeaponGrip(true);Check(TEXT("reload_left_target_finite"),!Left.ContainsNaN());
  const FVector ReloadGrip=H->WeaponPose().InverseTransformPosition(Left);
  Check(TEXT("support_hand_tracks_magazine_base"),ReloadGrip.Equals(FVector(-5,-2.5,-13.3f-H->MagazineTravel()),.05));
  G.ReloadTime=WardPistolPose::ReloadDuration-1.56f;
  Check(TEXT("magazine_is_seated_before_slide_finishes"),H->MagazineTravel()<.05f&&ActionTravel(G)>1.f);
  Check(TEXT("support_hand_follows_slide_before_regrip"),H->WeaponPose().InverseTransformPosition(H->WeaponGrip(true)).Z>2);
  G.TickHero(1.6);Check(TEXT("pistol_transfers_once_capacity9"),H->Rounds==9&&H->Reserve==8);
  G.TickHero(.2);Check(TEXT("no_duplicate_ammo_next_tick"),H->Rounds==9&&H->Reserve==8);
  G.ReloadTime=0;H->Rounds=3;G.ReloadWeapon();G.TickHero(.4);G.ReloadTime=0;Check(TEXT("cancel_before_transfer_keeps_ammo"),H->Rounds==3&&H->Reserve==8);
  G.Shotgun=true;G.LoadedShells=0;G.Items.Add(TEXT("shells"),3);G.ReloadTime=0;G.ReloadWeapon();G.TickHero(.71);Check(TEXT("shotgun_inserts_one_shell"),G.LoadedShells==1&&G.Items.FindRef(TEXT("shells"))==2);
  G.TickHero(.71);Check(TEXT("shotgun_second_shell_individual"),G.LoadedShells==2&&G.Items.FindRef(TEXT("shells"))==1);G.ReloadTime=0;
  H->Aiming=true;G.ShotCooldown=0;G.FireWeapon();Check(TEXT("shotgun_spends_one_shell"),G.LoadedShells==1&&G.ShotCooldown>0);
  Check(TEXT("shotgun_extraction_delayed"),G.NumberFlag(TEXT("combat63_pump"))>0);G.TickHero(.2);Check(TEXT("pump_foreend_moves"),ActionTravel(G)>0);G.TickHero(.1);Check(TEXT("shell_ejected_once_after_pump"),G.NumberFlag(TEXT("combat63_pump"))==0);G.TickHero(.1);
  G.Shotgun=false;G.ShotCooldown=0;G.ReloadTime=0;H->Rounds=1;G.FireWeapon();Check(TEXT("pistol_spends_one_round"),H->Rounds==0);Check(TEXT("empty_slide_locks_back"),ActionTravel(G)==2);
  const int Shots=G.Shots;G.ShotCooldown=0;G.FireWeapon();Check(TEXT("empty_fire_no_shot"),G.Shots==Shots&&H->Rounds==0);
  G.ShotCooldown=0;H->Rounds=4;G.Show(TEXT("inventory"));G.FireWeapon();Check(TEXT("menu_blocks_fire"),H->Rounds==4);G.ReloadWeapon();Check(TEXT("menu_blocks_reload"),G.ReloadTime<=0);const float Stamina=H->Stamina;G.Knife();Check(TEXT("menu_blocks_knife"),H->Stamina==Stamina);G.Show(TEXT(""));
  const FVector Eye=H->Camera->GetComponentLocation();auto* Wall=G.GetWorld()->SpawnActor<AActor>();auto* WB=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(WB);WB->SetBoxExtent(FVector(4,100,140));WB->SetCollisionProfileName(TEXT("BlockAll"));WB->RegisterComponent();Wall->SetActorLocation(Eye+H->Camera->GetForwardVector()*22);
  Check(TEXT("close_wall_blocks_barrel"),!H->WeaponHasClearance());G.FireWeapon();Check(TEXT("blocked_barrel_does_not_spend_round"),H->Rounds==4);Wall->Destroy();
  const FTransform Ready=H->WeaponPoseAt(0,0),AtWall=H->WeaponPoseAt(0,1);
  Check(TEXT("wall_pose_pulls_gun_back_and_points_down"),Ready.InverseTransformPosition(AtWall.GetLocation()).X<-8&&AtWall.GetRotation().GetForwardVector().Z<Ready.GetRotation().GetForwardVector().Z-.3);
  H->MotionWeight52=1.5f;const FTransform Sprint=H->WeaponPoseAt(0,0);
  H->MotionWeight52=1.26f;const FTransform Returning=H->WeaponPoseAt(0,0);
  H->MotionWeight52=0;
  Check(TEXT("pistol_returns_smoothly_from_sprint_to_ready"),Sprint.GetLocation().Z<Returning.GetLocation().Z-3&&Returning.GetLocation().Z<Ready.GetLocation().Z-3);
  const float KickBefore=H->WeaponKick;H->WeaponKick=KickBefore+2;
  const FTransform Recoil=H->WeaponPoseAt(0,0);H->WeaponKick=KickBefore;
  Check(TEXT("recoil_rotates_weapon_and_anchored_wrists_up"),Recoil.GetRotation().GetForwardVector().Z>Ready.GetRotation().GetForwardVector().Z+.03&&Ready.InverseTransformPosition(Recoil.GetLocation()).X<-1);
  G.Invulnerable=0;G.DamageHero(-10,nullptr);Check(TEXT("negative_damage_ignored"),H->Health==100);G.DamageHero(20,nullptr);Check(TEXT("accepted_hit_adds_wound"),H->Health==80&&G.NumberFlag(TEXT("combat63_blood"))>.1);
  G.DamageHero(20,nullptr);Check(TEXT("invulnerability_stops_duplicate_hit"),H->Health==80);
  G.Items.Add(TEXT("medkit"),2);G.ReloadTime=0;G.Heal();Check(TEXT("heal_reserves_one_medkit"),Healing(G)&&G.Items.FindRef(TEXT("medkit"))==1&&H->Health==80);G.Heal();Check(TEXT("repeated_heal_cannot_spend_twice"),G.Items.FindRef(TEXT("medkit"))==1);
  H->Aiming=true;G.ShotCooldown=0;G.FireWeapon();Check(TEXT("bandage_blocks_fire"),H->Rounds==4);G.TickHero(1);Check(TEXT("bandage_takes_time"),H->Health==80);G.Invulnerable=0;G.DamageHero(10,nullptr);Check(TEXT("hit_interrupts_bandage"),!Healing(G)&&H->Health==70);
  G.Invulnerable=0;G.Heal();G.TickHero(2.5);Check(TEXT("finished_heal_restores_health"),H->Health>99&&G.Items.FindRef(TEXT("medkit"))==0);Check(TEXT("bandage_stops_blood_drips"),G.NumberFlag(TEXT("combat63_bleed"))==0);
  G.Invulnerable=0;G.DamageHero(15,nullptr);G.TickHero(.016);Check(TEXT("blood_marks_limited"),State(G).Stains.Num()<=32);Check(TEXT("body_blood_attached_to_skeleton"),State(G).BodyStains.Num()==3);
  Check(TEXT("damage_postprocess_bounded"),H->Camera->PostProcessSettings.VignetteIntensity<=.58f&&H->Camera->PostProcessSettings.SceneFringeIntensity<=1.1f);
  Json Saved=G.Capture();Check(TEXT("blood_state_in_save"),Number(Object(Saved,TEXT("flags")),TEXT("combat63_blood"))>0);
  G.NumberFlag(TEXT("combat63_kick"),0);G.NumberFlag(TEXT("combat63_kick_velocity"),100);for(int I=0;I<240;++I)Tick(G,1.f/120);Check(TEXT("recoil_spring_settles"),FMath::Abs(G.NumberFlag(TEXT("combat63_kick")))<.02);
  Check(TEXT("paper_grip_is_wide"),FVector::Dist(PreviewGrip(TEXT("doc:test"),true),PreviewGrip(TEXT("doc:test"),false))==20);
  Check(TEXT("key_grip_is_smaller_than_container"),PreviewGrip(TEXT("badge"),false).Y<PreviewGrip(TEXT("fuel"),false).Y);
  H->Aiming=false;G.ReloadTime=0;Check(TEXT("trigger_finger_out_at_rest"),Trigger(G)==0);
  auto* Mob=S->Mobs[0].Get();Mob->Active=true;Mob->Dead=false;Mob->Memory=4;Mob->LastSeen59=H->GetActorLocation();Mob->SetActorLocation(H->GetActorLocation()+FVector(200,0,0));Mob->SetActorHiddenInGame(false);Check(TEXT("new_monster_witness_blocks_hiding"),Observed(G));Mob->Active=false;Mob->SetActorHiddenInGame(true);Check(TEXT("broken_contact_allows_hiding"),!Observed(G));
  for(const TCHAR* Clip:{TEXT("pistol"),TEXT("shotgun"),TEXT("magout"),TEXT("magin"),TEXT("slide"),TEXT("pump"),TEXT("shell"),TEXT("case"),TEXT("bandage"),TEXT("pain"),TEXT("breath"),TEXT("heartbeat")})Check(FString(TEXT("sound_file_"))+Clip,FPaths::FileExists(FPaths::ProjectContentDir()/TEXT("CampaignData/Combat63/audio")/(FString(Clip)+TEXT(".wav"))));
  // Exercise real collision traces and point damage, beyond validating the formulas.
  auto* Target=S->Mobs[0].Get();Target->Active=true;Target->Dead=false;Target->Class=TEXT("A");Target->Species=TEXT("test63");Target->Health=500;Target->MaxHealth=500;Target->SetActorEnableCollision(true);Target->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Target->SetActorLocation(Arena+FVector(600,0,91));
  const float Half=Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
  G.Shotgun=false;G.DamageAt(Arena+FVector(0,0,91),FVector::ForwardVector,34,false);Check(TEXT("real_trace_applies_pistol_torso_damage"),FMath::IsNearlyEqual(Target->Health,466.f,.1f));
  Target->Health=500;G.DamageAt(Arena+FVector(0,0,91+Half*.8f),FVector::ForwardVector,34,false);Check(TEXT("real_trace_applies_relative_head_damage"),FMath::IsNearlyEqual(Target->Health,432.f,.1f));
  Target->Health=500;G.DamageAt(Arena+FVector(0,0,91-Half*.7f),FVector::ForwardVector,34,false);Check(TEXT("real_trace_applies_leg_reduction"),FMath::IsNearlyEqual(Target->Health,477.9f,.1f));
  const int Marks=G.ImpactMarks.Num();Target->Health=500;G.DamageAt(Arena+FVector(0,0,91),FVector::ForwardVector,16,true);Check(TEXT("shotgun_single_pellet_is_16_close"),FMath::IsNearlyEqual(Target->Health,484.f,.1f));Check(TEXT("flesh_does_not_get_wall_bullet_decal"),G.ImpactMarks.Num()==Marks);
  auto* Shield=G.GetWorld()->SpawnActor<AActor>();auto* SB=NewObject<UBoxComponent>(Shield);Shield->SetRootComponent(SB);SB->SetBoxExtent(FVector(8,80,130));SB->SetCollisionProfileName(TEXT("BlockAll"));SB->RegisterComponent();Shield->SetActorLocation(Arena+FVector(300,0,100));Target->Health=500;G.DamageAt(Arena+FVector(0,0,91),FVector::ForwardVector,34,false);Check(TEXT("solid_wall_prevents_damage_to_target_behind_it"),Target->Health==500);Shield->Destroy();Target->Active=false;Target->SetActorHiddenInGame(true);Target->SetActorEnableCollision(false);
  G.ReloadTime=0;G.Show(TEXT(""));G.ShotCooldown=0;H->Health=70;G.Items.Add(TEXT("medkit"),1);G.Heal();const float BeforeDash=H->Stamina;G.Dash();Check(TEXT("bandage_blocks_dash"),H->Stamina==BeforeDash);G.Items.Add(TEXT("bottle"),1);G.ThrowBottle();Check(TEXT("bandage_blocks_bottle_throw"),G.Items.FindRef(TEXT("bottle"))==1);G.NumberFlag(TEXT("combat63_heal"),0);
  Check(TEXT("healing_reload_save_guard"),[&](){G.ReloadTime=1;const bool Saved=G.Save(TEXT("manual_1"));G.ReloadTime=0;return !Saved;}());
  // Verify physical cosmetic objects do not obstruct players or bullets, and remain bounded.
  bool CasesSafe=true;for(auto P:G.PhysicsProps)if(P.IsValid())CasesSafe&=P->Body->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Ignore&&P->Body->GetCollisionResponseToChannel(ECC_Visibility)==ECR_Ignore;
  Check(TEXT("ejected_cases_ignore_player_and_shot_rays"),CasesSafe&&G.PhysicsProps.Num()<=64);
  // Real render frames after setup, with the accepted character and production IK.
  for(const auto& Offset:{FVector(8,-55,36),FVector(36,68,18)})
  {
   auto* Lamp=G.GetWorld()->SpawnActor<AActor>();auto* Light=NewObject<UPointLightComponent>(Lamp);Lamp->SetRootComponent(Light);
   Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(Offset.Y<0?420:120);Light->SetAttenuationRadius(350);
   Light->SetCastShadows(false);Light->SetLightingChannels(true,true,false);Light->RegisterComponent();Lamp->SetActorLocation(H->Camera->GetComponentLocation()+Offset);
  }
  auto* Panel=G.GetWorld()->SpawnActor<AActor>();auto* WallMesh=NewObject<UStaticMeshComponent>(Panel);Panel->SetRootComponent(WallMesh);WallMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);WallMesh->RegisterComponent();Panel->SetActorScale3D(FVector(.2,6,3));Panel->SetActorLocation(H->GetActorLocation()+FVector(180,0,30));
  S->FreezeAI=true;G.Invulnerable=100;G.Shotgun=false;H->Rounds=5;G.ReloadTime=0;G.ShotCooldown=0;H->Aiming=true;H->WeaponAimBlend=1;H->NoticeTime=0;G.Noise=0;H->Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;H->Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;H->Camera->PostProcessSettings.AutoExposureMinBrightness=4;H->Camera->PostProcessSettings.AutoExposureMaxBrightness=4;H->Camera->PostProcessSettings.bOverride_AutoExposureBias=true;H->Camera->PostProcessSettings.AutoExposureBias=0;H->UpdateFirstPerson50();G.TickHero(.016f);Stage=1;Clock=0;
 }
 else if(Stage==1)
 {
  H->Aiming=true;H->UpdateFirstPerson50();G.TickHero(Delta);
  if(Clock>1){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/combat63_aim.png"),false,false);Stage=2;Clock=0;}
 }
 else if(Stage==2)
 {
  if(G.ReloadTime<=0)G.ReloadWeapon();H->UpdateFirstPerson50();G.TickHero(Delta);
  if(Clock>.32f){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/combat63_reload.png"),false,false);Stage=3;Clock=0;}
 }
 else if(Stage==3&&Clock>1)
 {
  bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/combat63_check.json"),R);Stage=4;FPlatformMisc::RequestExit(false);
 }
}
}
