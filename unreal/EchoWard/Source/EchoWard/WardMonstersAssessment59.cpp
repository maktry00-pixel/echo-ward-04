#include "WardMonster59.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "WardMonsterVisual59.h"
#include "WardMonsterMotion97.h"
#include "WardContact98.h"
#include "WardHorror99.h"
#include "WardDoor100.h"
#include "WardDecoy101.h"
#include "WardHideout102.h"
#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/PlatformMemory.h"
using namespace WardData;
namespace WardMonster59
{
void CaptureModel59(AEWardCampaignMode& G,AEWardHospitalMob* Mob,const FString& Name = FString())
{
    auto* Stage=G.GetWorld()->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Stage);Stage->SetRootComponent(Root);Root->RegisterComponent();Stage->SetActorLocation(Mob->GetActorLocation());
    auto* Target=NewObject<UTextureRenderTarget2D>(Stage);Target->InitCustomFormat(1280,960,PF_B8G8R8A8,false);Target->UpdateResourceImmediate(true);
    auto* Light=NewObject<UPointLightComponent>(Stage);Light->SetupAttachment(Root);Light->SetRelativeLocation(FVector(160,-160,180));Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(500);Light->SetAttenuationRadius(900);Light->SetCastShadows(false);Light->RegisterComponent();
    auto* Rim=NewObject<UPointLightComponent>(Stage);Rim->SetupAttachment(Root);Rim->SetRelativeLocation(FVector(-90,140,150));Rim->SetIntensityUnits(ELightUnits::Lumens);Rim->SetIntensity(250);Rim->SetAttenuationRadius(600);Rim->SetLightColor(FLinearColor(.7,.05,.035));Rim->SetCastShadows(false);Rim->RegisterComponent();
    const FVector At=Mob->GetActorLocation();const float Height=Number(Object(Object(Data(),TEXT("models")),Mob->Model61),TEXT("height_cm"),178);
    auto* Capture=NewObject<USceneCaptureComponent2D>(Stage);Capture->SetupAttachment(Root);Capture->TextureTarget=Target;Capture->CaptureSource=SCS_FinalColorLDR;Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->ShowOnlyComponent(Mob->Visual59);Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;Capture->FOVAngle=48;
    const FVector Aim(0,0,-Mob->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+Height*.52),From=Aim+FVector(Height*1.5,-Height*.65,Height*.18);Capture->SetRelativeLocationAndRotation(From,(Aim-From).Rotation());
    Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetAtmosphere(false);Capture->ShowFlags.SetBloom(false);Capture->ShowFlags.SetMotionBlur(false);
    Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;Capture->PostProcessSettings.AutoExposureMethod=AEM_Manual;Capture->PostProcessSettings.bOverride_AutoExposureBias=true;Capture->PostProcessSettings.AutoExposureBias=-2;
    Capture->RegisterComponent();Mob->SetActorHiddenInGame(false);
    if(Name.IsEmpty())Mob->Visual59->Present59(0,0,false,0);
    const float Moon=G.BookWorld->Daylight->Intensity;G.BookWorld->Daylight->SetIntensity(0);Capture->CaptureScene();G.BookWorld->Daylight->SetIntensity(Moon);
    FBufferArchive Bytes;
    FlushRenderingCommands();if(FImageUtils::ExportRenderTarget2DAsPNG(Target,Bytes))
    {
        const FString Path=FPaths::ProjectSavedDir()/TEXT("Prototype/Monsters59")/((Name.IsEmpty()?Mob->Species:Name)+TEXT(".png"));IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);FFileHelper::SaveArrayToFile(Bytes,*Path);
    }
    Mob->SetActorHiddenInGame(true);Stage->Destroy();
}
void Assess(AEWardCampaignMode& G,float Delta)
{
    static int Stage=0;static float Clock=0;static AEWardHospitalMob* Walker=nullptr;static FVector Before;
    static TArray<TSharedPtr<FJsonValue>> Checks;static TArray<float> FrameTimes;static bool FirstWalkFrame=true;
    auto Check=[&](const FString& Name,bool Pass){Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(Row));UE_LOG(LogTemp,Display,TEXT("MONSTER59 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
    auto* Story=G.HospitalStory.Get();auto* H=G.Hero();if(!Story||!H)return;Clock+=FApp::GetDeltaTime();
    auto Wall=[&](FVector At,FVector Size)
    {
        auto* A=G.GetWorld()->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(A);A->SetRootComponent(Box);Box->SetBoxExtent(Size);Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();A->SetActorLocation(At);return A;
    };
    if(Stage==0)
    {
        G.CampaignStarted=true;G.TitleView=false;G.Show(TEXT(""));G.Admin=false;G.Won=false;Story->HomeStage52=-1;Story->ArrivalStage=-1;Story->FreezeAI=true;
        Check(TEXT("18_complete_species_profiles_include_parasite"),Object(Data(),TEXT("rules"))->Values.Num()==18);
        Check(TEXT("46_encounters_keep_authored_zones_and_kazakh_names"),Array(Data(),TEXT("encounters")).Num()==46&&Object(Data(),TEXT("individuals"))->Values.Num()==46);
        Check(TEXT("listener_keeps_B_and_finale_only_rule"),String(Rule(TEXT("listener")),TEXT("combat_class"))==TEXT("B")&&String(Rule(TEXT("listener")),TEXT("kill_condition"))==TEXT("finale_only"));
        bool Unique=true,Chapter1=false;
        for(int Ch=1;Ch<=19;++Ch)
        {
            Story->SetChapter(Ch);TSet<FString> Active;
            for(auto M:Story->Mobs)if(M->Active){Unique&=!Active.Contains(M->Guid59);Active.Add(M->Guid59);}
            if(Ch==1)Chapter1=Active.IsEmpty();
        }
        Check(TEXT("chapter_one_has_no_monsters"),Chapter1);Check(TEXT("unique_nicknamed_monster_has_one_active_route_anchor"),Unique);
        Check(TEXT("E18_and_R40_camera_controls_exist"),Story->Props.Contains(TEXT("MONSTER_CAMERA_E18"))&&Story->Props.Contains(TEXT("MONSTER_CAMERA_R40")));
        Story->SetChapter(7);
        if(auto* Control=Story->Props.FindRef(TEXT("MONSTER_CAMERA_E18")))
        {
            H->SetActorLocation(Control->GetActorLocation()+FVector(100,0,0));G.Flag(TEXT("monster_camera_B2_off59"),false);
            const bool Used=UseControl(*Story,TEXT("MONSTER_CAMERA_E18"));
            Check(TEXT("local_camera_switch_changes_real_saved_flag"),Used&&G.Flag(TEXT("monster_camera_B2_off59")));
            G.Flag(TEXT("monster_camera_B2_off59"),false);
            Story->ActiveScene35=TEXT("SCENE_07_03");Story->DialoguePage35=1;
            Story->SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),true);
            auto* Controller=G.GetWorld()->GetFirstPlayerController();
            Controller->SetControlRotation((Control->GetActorLocation()+FVector(0,0,15)-H->Camera->GetComponentLocation()).Rotation());
            const bool SuspendedUse=Story->Interact();
            Check(TEXT("suspended_scene_allows_nearby_physical_camera_control"),SuspendedUse&&G.Flag(TEXT("monster_camera_B2_off59")));
            Check(TEXT("physical_camera_control_preserves_suspended_dialogue_page"),Story->ActiveScene35==TEXT("SCENE_07_03")&&Story->DialoguePage35==1&&Story->NarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"))&&G.Screen.IsEmpty());
            Story->ActiveScene35.Empty();Story->DialoguePage35=0;Story->SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),false);
            const FString SavePath=FPaths::ProjectSavedDir()/TEXT("Prototype/monster59_control_save.json");
            Write(SavePath,G.Capture());const auto Saved=Read(SavePath);G.Flag(TEXT("monster_camera_B2_off59"),false);
            const bool Restored=G.Restore(Saved);Check(TEXT("control_save_is_valid_at_real_game_position"),Restored);
            Check(TEXT("camera_power_state_survives_campaign_save"),G.Flag(TEXT("monster_camera_B2_off59")));
        }
        Story->SetChapter(13);
        AEWardHospitalMob* West=nullptr;
        AEWardHospitalMob* East=nullptr;
        for(auto Mob:Story->Mobs)
        {
            if(Mob->Id==TEXT("MOB_linked_04"))West=Mob;
            if(Mob->Id==TEXT("MOB_linked_01"))East=Mob;
        }
        if(auto* Relay=Story->Props.FindRef(TEXT("LINK_01"));Relay&&West&&East)
        {
            // Stance reached with movement inputs in the normal chapter-13 run.
            // Set the camera component as well as the controller because this
            // fixture invokes interaction before the next camera update.
            const FVector Standing(-11102.18,25597.49,98.15);
            H->SetActorLocation(Standing);
            Story->ActiveScene35=TEXT("SCENE_13_01");
            Story->SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),true);
            auto* Controller=G.GetWorld()->GetFirstPlayerController();
            const FRotator Look=(Relay->GetActorLocation()+FVector(0,0,15)-H->Camera->GetComponentLocation()).Rotation();
            Controller->SetControlRotation(Look);
            H->Camera->SetWorldRotation(Look);
            const bool Used=Story->Interact();
            Check(TEXT("staff_relay_can_be_used_during_interrupted_dialogue"),
                Used&&G.Completed.Contains(TEXT("LINK_01"))&&Story->ActiveScene35==TEXT("SCENE_13_01")&&G.Screen.IsEmpty());
            West->ChangeState59(TEXT("windup"));West->Memory=5;West->Windup59=.1f;
            West->StepBrain59(.02f);
            Check(TEXT("staff_relay_stops_local_group_without_disabling_other_group"),
                LinkedSignalDisabled(*West)&&!LinkedSignalDisabled(*East)&&West->State59==TEXT("dormant")
                &&West->AttackResolved59&&West->Memory==0);
            Story->SetChapter(19);
            Check(TEXT("staff_relay_does_not_disable_finale_monsters"),!LinkedSignalDisabled(*West));
            G.Completed.Remove(TEXT("LINK_01"));Story->ActiveScene35.Reset();
            Story->SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),false);
        }
        else Check(TEXT("staff_relays_and_groups_resolve"),false);
        Story->SetChapter(14);
        auto* RoofDoor=G.BookWorld->Portals.FindByPredicate([](const auto& Portal){return Portal.Id==TEXT("T12");});
        Check(TEXT("chapter_fourteen_unlocks_roof_lift_exit"),RoofDoor&&!RoofDoor->Locked&&!RoofDoor->InsideOnly);
        if(RoofDoor)
        {
            RoofDoor->Open=false;RoofDoor->Locked=true;
            G.BookWorld->RefreshPortalVisual(*RoofDoor);
            Json OldSave=G.Capture();
            const bool Loaded=G.Restore(OldSave);
            Check(TEXT("legacy_roof_save_keeps_door_closed_but_unlocks_exit"),
                Loaded&&!RoofDoor->Open&&!RoofDoor->Locked&&!RoofDoor->InsideOnly);
        }
        const FVector Arena(-70000,-70000,91);Wall(Arena+FVector(0,0,-101),FVector(2000,2000,10));
        if (auto* Patient=Story->Mobs.FindByPredicate([](const auto& Entry){return Entry->Species==TEXT("patients");}))
        {
            auto& Book=*G.BookWorld;auto& Enemy=**Patient;
            const FVector Bed(Arena.X+800,Arena.Y-1000,0),Cabinet=Bed+FVector(460,0,0);
            WardHideout102::Add(Book,Bed,FVector(120,175,170),true,nullptr,nullptr);
            WardHideout102::Add(Book,Cabinet,FVector(112,112,202),false,
                LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalSurface.M_HospitalSurface")),nullptr);
            auto* Deck=Wall(Bed+FVector(60,87.5,114),FVector(60,87.5,9));
            const FVector BedEntry=Bed+FVector(60,240,98),ClosetEntry=Cabinet+FVector(56,177,98);
            const auto Ready = [&](FVector At, float Yaw)
            {
                Enemy.ResetMob();Enemy.Active=true;Enemy.SetActorHiddenInGame(false);Enemy.SetActorEnableCollision(false);
                Enemy.SetActorLocation(At);Enemy.SetActorRotation(FRotator(0,Yaw,0));Enemy.ChangeState59(TEXT("patrol"));
                Enemy.GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                H->Dead=false;H->Health=100;G.Invulnerable=0;G.Won=false;
            };
            for (auto Mob : Story->Mobs) if(Mob!=&Enemy) Mob->Active=false;
            Ready(BedEntry+FVector(0,290,0),90);
            H->SetActorLocation(BedEntry);H->UnCrouch();H->GetCharacterMovement()->UnCrouch(false);
            const bool BedEntered=Book.UseFurnitureHide(H->GetActorLocation())&&Book.FurnitureHiding;
            FVector Eye;FRotator Centre;float Yaw=0;
            const bool LowView=WardHideout102::View(*H,Eye,Centre,Yaw)&&FMath::IsNearlyEqual(Eye.Z,32.,1.)&&Yaw>50;
            FHitResult ViewHit;const FCollisionQueryParams Q(SCENE_QUERY_STAT(Hideout102Test),false,H);
            const bool SeeLegs=!G.GetWorld()->LineTraceSingleByChannel(ViewHit,Eye,BedEntry+FVector(0,115,-63),ECC_Visibility,Q);
            Check(TEXT("hideout102_under_bed_view_is_below_mattress_and_faces_hall"),BedEntered&&LowView&&SeeLegs);
            Check(TEXT("hideout102_unseen_entry_preserves_cover"),BedEntered&&!WardHideout102::Inspecting(Enemy));
            Enemy.Hear(BedEntry+FVector(-190,0,0),1800);
            Check(TEXT("hideout102_noise_away_from_entrance_does_not_expose_cover"),!WardHideout102::Inspecting(Enemy));
            Enemy.Hear(BedEntry+FVector(22,0,0),1600);
            Check(TEXT("hideout102_noise_near_entrance_triggers_search"),WardHideout102::Inspecting(Enemy)&&Enemy.Target.Equals(FVector(BedEntry.X,BedEntry.Y,Enemy.GetActorLocation().Z),1));
            Enemy.SetActorLocation(BedEntry+FVector(0,59,0));
            Enemy.StepBrain59(.09f);
            const bool Warned=Enemy.State59==TEXT("hideout_check")&&H->Health==100&&Book.FurnitureHiding;
            for(int I=0;I<14;++I) Enemy.StepBrain59(.10f);
            Check(TEXT("hideout102_monster_checks_before_hurting_player"),Warned&&H->Health<100&&!Book.FurnitureHiding);
            WardHideout102::Reset(Book);H->UnCrouch();H->GetCharacterMovement()->UnCrouch(false);

            Ready(ClosetEntry+FVector(0,280,0),270);
            H->SetActorLocation(ClosetEntry);
            const bool ClosetEntered=Book.UseFurnitureHide(ClosetEntry)&&Book.FurnitureHiding;
            Check(TEXT("hideout102_witnessed_entry_brings_enemy_to_same_cabinet"),ClosetEntered&&WardHideout102::Inspecting(Enemy)&&
                FVector::Dist2D(Enemy.Target,ClosetEntry)<1);
            const bool Narrow=WardHideout102::View(*H,Eye,Centre,Yaw)&&Eye.Z>100&&Yaw<20;
            const FVector Forward=Centre.Vector();
            const bool Slit=!G.GetWorld()->LineTraceSingleByChannel(ViewHit,Eye,Eye+Forward*130,ECC_Visibility,Q)&&
                G.GetWorld()->LineTraceSingleByChannel(ViewHit,Eye,Eye+(Forward+FVector(.38,0,0))*130,ECC_Visibility,Q);
            Check(TEXT("hideout102_cabinet_has_a_real_slit_and_limits_yaw"),ClosetEntered&&Narrow&&Slit);
            G.Noise=0;
            const bool Quiet=WardHideout102::TryExit(Book,false)&&G.Noise==0;
            Enemy.StepBrain59(.1f);
            Check(TEXT("hideout102_exiting_in_time_breaks_inspection"),Quiet&&!WardHideout102::Inspecting(Enemy)&&!Book.FurnitureHiding);
            H->SetActorLocation(ClosetEntry);G.Noise=0;
            Book.UseFurnitureHide(ClosetEntry);
            Check(TEXT("hideout102_fast_exit_creaks_and_emits_gameplay_noise"),WardHideout102::TryExit(Book,true)&&G.Noise>.6f);
            WardHideout102::Reset(Book);Enemy.ResetMob();Deck->Destroy();
        }
        else Check(TEXT("hideout102_patient_fixture_exists"),false);
        int32 FrostedPanes = 0;
        for (const auto& Entry : G.BookWorld->Groups)
            if (auto* Material = Entry.Value->GetMaterial(0); Material && Material->GetFName()==FName(TEXT("WardFrostedPane")) &&
                Material->GetBlendMode()==BLEND_Translucent && Entry.Value->GetCollisionEnabled()!=ECollisionEnabled::NoCollision)
                FrostedPanes += Entry.Value->GetInstanceCount();
        UE_LOG(LogTemp,Display,TEXT("HORROR99 frosted panes in hospital=%d"),FrostedPanes);
        Check(TEXT("horror99_frosted_panes_are_present_in_playable_map"),FrostedPanes>0);
        if (const auto* Found = Story->Mobs.FindByPredicate([](const auto& Mob){ return Mob->Species==TEXT("patients"); }))
        {
            auto& Patient = **Found;
            auto& Book = *G.BookWorld;
            const FVector GlassAt = Arena + FVector(-600,-480,48);
            auto* Pane = G.GetWorld()->SpawnActor<AActor>();
            auto* Glass = NewObject<UStaticMeshComponent>(Pane);
            auto* Material = WardHorror99::FrostedPane(Pane, LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalGlass.M_HospitalGlass")));
            Glass->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
            Glass->SetMaterial(0, Material);
            Pane->SetRootComponent(Glass);
            Glass->SetCollisionProfileName(TEXT("BlockAll"));
            Glass->RegisterComponent();
            Glass->SetWorldScale3D(FVector(.12,2.2,1.7));
            Pane->SetActorLocation(GlassAt);
            const FVector Standing = GlassAt + FVector(95,0,-48);
            const FVector Behind = GlassAt + FVector(-95,0,-48);
            const auto Ready = [&]() {
                Patient.ResetMob();Patient.Active=true;Patient.SetActorLocation(Behind);Patient.SetActorRotation(FRotator::ZeroRotator);
                Patient.GetCharacterMovement()->SetMovementMode(MOVE_Walking);Patient.ChangeState59(TEXT("patrol"));
                H->SetActorLocation(Standing);
            };
            Ready();
            WardHorror99::Footstep(G,Standing,90);
            Check(TEXT("horror99_quiet_crouch_step_does_not_trigger_listening"), Patient.State59==TEXT("patrol"));
            WardHorror99::Footstep(G,Standing,360);
            Check(TEXT("horror99_real_hospital_glass_blocks_sight_but_carries_steps"), Material &&
                !Patient.Sight59(Standing,true) && Patient.State59==TEXT("listen") && Patient.Target.Equals(Standing,1));
            Patient.StepBrain59(.43f);
            WardHorror99::Footstep(G,Standing,360);
            Check(TEXT("horror99_second_step_does_not_extend_freeze"), Patient.State59==TEXT("listen") &&
                Patient.StateClock59>=.4f && Patient.GetVelocity().IsNearlyZero() && Patient.GetPendingMovementInputVector().IsNearlyZero());
            Patient.StepBrain59(.49f);
            Check(TEXT("horror99_listens_then_investigates"), Patient.State59==TEXT("investigate"));
            Ready(); Patient.Hear(Standing,1600);
            Check(TEXT("horror99_generic_noise_is_not_treated_as_footsteps"), Patient.State59==TEXT("investigate"));
            Pane->Destroy();

            const int32 StartRoom = Book.Rooms.Num(), StartPortal = Book.Portals.Num();
            const FVector LastSight = Arena + FVector(0,410,0);
            const auto Ward = [&](const TCHAR* Id, float X, bool Locked)
            {
                const FVector Floor(LastSight.X+X,LastSight.Y+180,0);
                FWardBookRoom3D Room;Room.Id=Id;Room.Name=TEXT("Палата");Room.Location=TEXT("1F");
                Room.Bounds=FBox(Floor+FVector(-120,0,0),Floor+FVector(120,420,380));
                Book.Rooms.Add(Room);
                AEWardBookWorld::FPortal Door;Door.Id=Id;Door.Zone=Id;Door.At=Floor;Door.Side=TEXT("north");
                Door.Type=TEXT("ordinary");Door.Locked=Locked;Door.Open=!Locked;
                Book.Portals.Add(Door);
            };
            Ward(TEXT("QA_99_locked"),0,true);Ward(TEXT("QA_99_blocked"),-220,false);
            Ward(TEXT("QA_99_first"),260,false);Ward(TEXT("QA_99_second"),-370,false);
            auto* WallAcrossDoor=Wall(LastSight+FVector(-220,180,6), FVector(120,15,97));
            Patient.SetActorLocation(LastSight-FVector(0,350,0));
            Patient.SetActorRotation(FRotator(0,90,0));Patient.AttackClock=5;
            Patient.NoisePriority59=0;Patient.ChangeState59(TEXT("patrol"));
            H->SetActorLocation(LastSight);
            Patient.StepBrain59(.06f);
            const bool Saw = Patient.State59==TEXT("chase") && Patient.LastSeen59.Equals(LastSight,1);
            H->SetActorLocation(LastSight+FVector(480,200,480));
            Patient.StepBrain59(.10f);
            WardHorror99::Footstep(G,LastSight+FVector(420,0,0),1350);
            Check(TEXT("horror99_pursues_last_sighting_after_player_changes_floors"), Saw &&
                Patient.State59==TEXT("investigate") && Patient.Target.Equals(LastSight,1));
            Patient.SetActorLocation(LastSight);
            Patient.StepBrain59(.1f);
            const bool PickedFirst = Patient.State59==TEXT("search") && FMath::IsNearlyEqual(Patient.Target.X,LastSight.X+260,1);
            Check(TEXT("horror99_skips_locked_and_physically_blocked_wards"), PickedFirst);
            if (PickedFirst)
            {
                Patient.SetActorLocation(Patient.Target);
                Patient.StepBrain59(.1f);
                const bool Paused = FMath::IsNearlyEqual(Patient.Target.Y,LastSight.Y+48,1) && Patient.GetVelocity().IsNearlyZero();
                for (int I=0;I<4;++I) Patient.StepBrain59(.1f);
                const bool PeeksInside = Book.Rooms[StartRoom+2].Bounds.IsInside(Patient.Target);
                Check(TEXT("horror99_pauses_at_threshold_then_steps_into_ward"), Paused && PeeksInside);
                Patient.SetActorLocation(Patient.Target);
                for (int I=0;I<4;++I) Patient.StepBrain59(.10f);
                const bool ReturnedThroughDoor = FMath::IsNearlyEqual(Patient.Target.X,LastSight.X+260,1) &&
                    FMath::IsNearlyEqual(Patient.Target.Y,LastSight.Y+48,1);
                Patient.SetActorLocation(Patient.Target);
                Patient.StepBrain59(.1f);
                Check(TEXT("horror99_checks_the_next_ward"), PeeksInside && ReturnedThroughDoor &&
                    FMath::IsNearlyEqual(Patient.Target.X,LastSight.X-370,1));
            }
            else Check(TEXT("horror99_pauses_at_threshold_then_steps_into_ward"), false);
            Patient.ResetMob();
            Book.Portals.SetNum(StartPortal);Book.Rooms.SetNum(StartRoom);
            WallAcrossDoor->Destroy();
        }
        else Check(TEXT("horror99_patient_fixture_available"),false);
        {
            auto& Book = *G.BookWorld;
            const int32 RefugeIndex = Book.Portals.IndexOfByPredicate([](const auto& Door){return Door.Id==TEXT("RH08");});
            const auto* Refuge = Book.Rooms.FindByPredicate([](const auto& Room){return Room.Id==TEXT("RH08");});
            auto* Patient = Story->Mobs.FindByPredicate([](const auto& Mob){return Mob->Species==TEXT("patients");});
            if (Refuge && Patient && Book.Portals.IsValidIndex(RefugeIndex))
            {
                auto& Door = Book.Portals[RefugeIndex];
                const Json Architecture = Book.ArchitectureSnapshot();
                if (Book.RefugeBolts.Contains(Door.Zone)) Book.ToggleBoltForZone(Door.Zone,Refuge->Bounds.GetCenter());
                const bool Bolted = Book.ToggleBoltForZone(Door.Zone,Refuge->Bounds.GetCenter());
                const auto* Lever = Door.VisibleParts.FindByPredicate([](const auto& Part){return Part.Material==WardDoor100::LeverMaterial;});
                Check(TEXT("door100_refuge_has_a_working_bolt_and_separate_lever"), Bolted&&Lever&&Book.RefugeBolts.Contains(Door.Zone)&&!Door.Open);
                if (Lever)
                {
                    auto* Group = Book.DoorVisualGroups.FindRef(Lever->Material).Get();
                    FTransform Rest,Pressed,Released;
                    Group->GetInstanceTransform(Lever->Index,Rest);
                    const bool Started = WardDoor100::Begin(Book,Door,false);
                    WardContact98::Advance(Book,.52f);
                    Group->GetInstanceTransform(Lever->Index,Pressed);
                    Check(TEXT("door100_real_handle_turns_down_without_moving_leaf"),Started && Pressed.GetLocation().Z<Rest.GetLocation().Z-1 &&
                        !Pressed.GetRotation().Equals(Rest.GetRotation(),.02) && !Door.Open && Door.Leaf->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
                    const bool NoRepeat = !WardDoor100::Begin(Book,Door,false);
                    WardContact98::Advance(Book,.45f);
                    Check(TEXT("door100_bolt_stays_locked_under_full_pressure"),NoRepeat && WardDoor100::LeverAngle(Book,Door.At)>30.f &&
                        Book.RefugeBolts.Contains(Door.Zone) && !Door.Open && Door.Leaf->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
                    WardContact98::Advance(Book,.85f);
                    Group->GetInstanceTransform(Lever->Index,Released);
                    Check(TEXT("door100_lever_springs_back_while_monster_still_waits"), Released.Equals(Rest,.01) &&
                        WardDoor100::CheckAge(Book,Door.At)>=1.7f && !Door.Open);
                    WardContact98::ClearMotion(Book);

                    const int32 RoomsBefore=Book.Rooms.Num(), DoorsBefore=Book.Portals.Num();
                    const FVector Hall = Arena+FVector(-350,1120,0);
                    auto Fixture = Door;
                    Fixture.At=FVector(Hall.X,Hall.Y+180,0);Fixture.Side=TEXT("north");Fixture.Zone=Fixture.Id=TEXT("QA_DOOR100");
                    Fixture.Locked=true;Fixture.Open=false;Fixture.InsideOnly=false;Fixture.Role.Empty();
                    FWardBookRoom3D Room;Room.Id=Fixture.Zone;Room.Name=TEXT("Палата");Room.Location=TEXT("1F");
                    Room.Bounds=FBox(Fixture.At+FVector(-160,0,0),Fixture.At+FVector(160,420,380));
                    Book.Rooms.Add(Room);Book.Portals.Add(Fixture);
                    auto& Mob = **Patient;
                    Mob.ResetMob();Mob.Active=true;Mob.LastSeen59=Hall;Mob.SearchOrigin59=Hall;
                    Mob.ChangeState59(TEXT("chase"));Mob.GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                    Mob.SetActorLocation(Hall);Mob.SetActorRotation(FRotator(0,90,0));
                    H->SetActorLocation(Hall+FVector(0,0,480));Mob.StepBrain59(.1f);
                    const FVector Outside = Mob.Target;
                    Check(TEXT("door100_search_leads_monster_to_locked_door"),Mob.State59==TEXT("search")&&Outside.Y>Hall.Y+70&&Outside.Y<Fixture.At.Y);
                    Mob.SetActorLocation(Outside);Mob.StepBrain59(.1f);
                    FVector Hand;
                    Check(TEXT("door100_monster_reaches_for_handle_and_stops_walking"),Mob.State59==TEXT("door_check")&&
                        WardDoor100::HandTarget(Mob,Hand)&&FMath::IsNearlyEqual(Hand.Z,115.,1.)&&Mob.GetPendingMovementInputVector().IsNearlyZero());
                    Mob.StepBrain59(.9f);
                    Check(TEXT("door100_monster_waits_silently_without_crossing_door"),Mob.State59==TEXT("door_check") &&
                        Mob.GetVelocity().IsNearlyZero()&&Mob.GetActorLocation().Equals(Outside,1)&&!Book.Portals.Last().Open);
                    Mob.StepBrain59(1.7f);
                    Check(TEXT("door100_monster_eventually_moves_on_with_door_locked"),Mob.State59==TEXT("return")&&!Book.Portals.Last().Open);
                    Mob.ResetMob();Book.Rooms.SetNum(RoomsBefore);Book.Portals.SetNum(DoorsBefore);
                    Book.RestoreArchitecture(Architecture);
                }
            }
            else Check(TEXT("door100_refuge_exists_in_campaign"),false);
        }
        {
            auto& Book = *G.BookWorld;
            const FVector Home(Arena.X, Arena.Y+460, 45);
            auto* Prop = Wall(Home, FVector(32, 62, 45));
            Prop->GetRootComponent()->SetMobility(EComponentMobility::Movable);
            const int32 Index = Book.Movables.Add({TEXT("contact98_fixture"), TEXT("gurney"), FVector(32,62,45),
                Home, 870, true, Cast<UBoxComponent>(Prop->GetRootComponent())});
            auto* Step = Wall(Home+FVector(57,0,-43.5), FVector(6,90,1.5));
            auto* Stop = Wall(Home+FVector(144,0,22), FVector(8,96,72));
            auto* Mob = Story->Mobs[0].Get();
            Mob->SetActorLocation(Home+FVector(0,230,46));
            Mob->Active = true; Mob->Dead = false; Mob->NoisePriority59 = 0; Mob->Memory = 0; Mob->State59 = TEXT("patrol");
            H->SetActorLocation(Home+FVector(-100,0,46));
            const bool Queued = WardContact98::PushCart(Book, Index, H->GetActorLocation(), false, true);
            WardContact98::Advance(Book, .09f);
            const auto Reach = WardContact98::Pose(*H);
            Check(TEXT("contact98_both_hands_reach_cart_before_impulse"), Queued && Reach.Grip==WardContact98::EGrip::Cart &&
                Reach.Reach>.1f && FVector::Dist2D(Reach.Right,Reach.Left)>25 && Prop->GetActorLocation().Equals(Home));
            G.Noise = 0;
            WardContact98::Advance(Book, .08f);
            const FVector Shove = Prop->GetActorLocation();
            Check(TEXT("contact98_cart_alerts_real_monster_at_cart_not_hero"), Shove.X>Home.X && Mob->Memory>0 &&
                FVector::Dist2D(Mob->Target, Shove)<15 && FVector::Dist2D(Mob->Target, H->GetActorLocation())>70);
            bool MountedThreshold = Shove.Z > Home.Z+1;
            for (int32 Frame=0; Frame<9; ++Frame)
            {
                WardContact98::Advance(Book, .033f);
                MountedThreshold |= Prop->GetActorLocation().Z > Home.Z+1;
            }
            UE_LOG(LogTemp, Display, TEXT("CONTACT98 step=%d shove=%.1f rolled=%.1f noise=%.2f"),
                MountedThreshold, Shove.X-Home.X, Prop->GetActorLocation().X-Home.X, G.Noise);
            Check(TEXT("contact98_rolls_after_shove_and_rattles_over_threshold"), MountedThreshold &&
                Prop->GetActorLocation().X > Shove.X+15 && G.Noise>.68f);
            for (int32 Frame=0; Frame<100; ++Frame) WardContact98::Advance(Book, .033f);
            const FVector Rest = Prop->GetActorLocation();
            WardContact98::Advance(Book, .09f);
            Check(TEXT("contact98_cart_stops_without_crossing_wall"), Rest.Equals(Prop->GetActorLocation(),.01) &&
                Rest.X+32 < Stop->GetActorLocation().X-2);
            Check(TEXT("contact98_hand_pose_ends_when_object_released"), WardContact98::Pose(*H).Reach==0.f);
            WardContact98::ClearMotion(Book);
            Book.Movables.RemoveAt(Index);
            Prop->Destroy(); Step->Destroy(); Stop->Destroy();
            Mob->Active = false;

            const int32 DoorIndex = Book.Portals.IndexOfByPredicate([](const auto& Door) {
                return Door.Width>80 && Door.Type!=TEXT("biometric") && Door.Type!=TEXT("shutter") && !Door.VisibleParts.IsEmpty(); });
            if (Book.Portals.IsValidIndex(DoorIndex))
            {
                auto& Door = Book.Portals[DoorIndex];
                const bool WasOpen = Door.Open;
                const bool NS = Door.Side==TEXT("north") || Door.Side==TEXT("south");
                const double Latch = Door.Width>160 ? 18 : Door.Width*.5-19;
                H->SetActorLocation(Door.At+FVector(NS ? Latch : -66, NS ? -66 : Latch, 98));
                Door.Open=false;
                WardContact98::BeginDoor(Book,DoorIndex,H->GetActorLocation(),true,true);
                Door.Open=true; Book.RefreshPortalVisual(Door);
                WardContact98::Advance(Book,.13f);
                const auto Contact = WardContact98::Pose(*H);
                const bool Reaching = Contact.Reach>.10f && Contact.Grip==WardContact98::EGrip::Door &&
                    WardContact98::DoorOpenAmount(Book,Door.At,true)==0 && Door.Leaf->GetCollisionEnabled()!=ECollisionEnabled::NoCollision;
                WardContact98::Advance(Book,.17f);
                const float Amount = WardContact98::DoorOpenAmount(Book,Door.At,true);
                UE_LOG(LogTemp, Display, TEXT("CONTACT98 door=%s width=%.1f reach=%.2f swing=%.2f"), *Door.Id, Door.Width, Contact.Reach, Amount);
                Check(TEXT("contact98_door_starts_closed_then_swings_with_reach"), Reaching && Amount>0.f && Amount<1.f);
                WardContact98::Advance(Book,.29f);
                Check(TEXT("contact98_door_open_after_swing"), Door.Leaf->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
                WardContact98::ClearMotion(Book);
                Door.Open=WasOpen; Book.RefreshPortalVisual(Door);
            }
            else Check(TEXT("contact98_door_starts_closed_then_swings_with_reach"), false);

            if (auto* Key = Story->Props.FindRef(TEXT("spare_fuse")))
            {
                const auto At = Key->GetActorLocation();
                H->SetActorLocation(At-FVector(56,0,0));
                WardContact98::KeyPickup(*H,*Key);
                WardContact98::Advance(Book,.16f);
                const auto BeforeLift = WardContact98::Pose(*H);
                WardContact98::Advance(Book,.18f);
                const auto Lift = WardContact98::Pose(*H);
                Check(TEXT("contact98_key_pinch_lifts_toward_player"), BeforeLift.Reach>.1f && Lift.Grip==WardContact98::EGrip::Key &&
                    Lift.Right.Z>BeforeLift.Right.Z+12 && FVector::Dist2D(Lift.Right,H->GetActorLocation())<FVector::Dist2D(BeforeLift.Right,H->GetActorLocation()));
                WardContact98::ClearMotion(Book);
            }
            else Check(TEXT("contact98_key_pinch_lifts_toward_player"), false);
        }
        {
            auto& Book = *G.BookWorld;
            const FVector Floor = Arena+FVector(-890,-910,-91);
            const FVector CartAt = Floor+FVector(0,0,44);
            auto* CartActor = Wall(CartAt,FVector(38,69,44));
            CartActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
            const FName CartName(TEXT("QA_cart101"));
            const int32 CartIndex = Book.Movables.Add({CartName.ToString(),TEXT("gurney"),FVector(38,69,44),CartAt,900,true,
                Cast<UBoxComponent>(CartActor->GetRootComponent())});
            auto* Patient = Story->Mobs.FindByPredicate([](const auto& Mob){return Mob->Species==TEXT("patients");});
            auto* Hound = Story->Mobs.FindByPredicate([](const auto& Mob){return Mob->Species==TEXT("hound");});
            auto* Armoured = Story->Mobs.FindByPredicate([](const auto& Mob){return Mob->Species==TEXT("armoured");});
            auto* Pursuer = Story->Mobs.FindByPredicate([](const auto& Mob){return Mob->Species==TEXT("pursuer");});
            if (Patient&&Hound&&Armoured&&Pursuer)
            {
                const auto Prepare = [&](AEWardHospitalMob& Creature, FVector Position){
                    Creature.ResetMob();Creature.Active=true;Creature.SetActorLocation(Position);Creature.ChangeState59(TEXT("patrol"));
                    Creature.GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                };
                Prepare(**Patient,Floor+FVector(0,320,91));
                Prepare(**Hound,Floor+FVector(-38,285,70));
                Prepare(**Armoured,Floor+FVector(50,-350,115));
                Prepare(**Pursuer,Floor+FVector(-90,-310,104));
                H->SetActorLocation(Floor+FVector(-160,-150,91));
                const bool Pushed = Book.PushMovable(CartIndex,FVector(1,0,0));
                const FVector Initial = CartActor->GetActorLocation();
                Check(TEXT("decoy101_patient_hears_actual_cart_rather_than_player"),Pushed&&(*Patient)->Target.Equals(Initial,1)&&
                    (*Patient)->State59==TEXT("investigate")&&FVector::Dist2D(Initial,H->GetActorLocation())>155);
                Check(TEXT("decoy101_armoured_ignores_gentle_roll_and_pursuer_ignores_cart"),
                    (*Armoured)->State59==TEXT("patrol")&&(*Pursuer)->State59==TEXT("patrol"));
                WardDecoy101::HearCart(G,CartName,WardDecoy101::ESound::Rattle,Initial,1080);
                Check(TEXT("decoy101_armoured_reacts_to_sharp_clatter"),(*Armoured)->State59==TEXT("investigate"));
                H->SetActorLocation(Floor+FVector(-240,-280,571));
                const FVector ProtectedTarget = (*Patient)->Target;
                WardHorror99::Footstep(G,Floor+FVector(-70,240,91),90);
                Check(TEXT("decoy101_soft_steps_do_not_override_cart"),(*Patient)->Target.Equals(ProtectedTarget,1));
                for(int I=0;I<80;++I) WardContact98::Advance(Book,.033f);
                const FVector Stop = CartActor->GetActorLocation();
                Check(TEXT("decoy101_noise_tracks_actual_stop_after_player_leaves"),
                    Stop.X>Initial.X+135&&(*Patient)->Target.Equals(Stop,1)&&(*Hound)->Target.Equals(Stop,1));
                const int32 RoomCount=Book.Rooms.Num(), PortalCount=Book.Portals.Num();
                FWardBookRoom3D Room;Room.Id=TEXT("QA_ward101");Room.Name=TEXT("Палата");Room.Location=TEXT("1F");
                const FVector DoorAt(Stop.X+260,Stop.Y+220,0);
                Room.Bounds=FBox(DoorAt+FVector(-140,0,0),DoorAt+FVector(140,360,380));
                Book.Rooms.Add(Room);
                AEWardBookWorld::FPortal Door;Door.Id=Door.Zone=Room.Id;Door.At=DoorAt;Door.Type=TEXT("ordinary");Door.Side=TEXT("north");
                Book.Portals.Add(Door);
                (*Patient)->SetActorLocation(FVector(Stop.X,Stop.Y+112,91));
                (*Hound)->SetActorLocation(FVector(Stop.X,Stop.Y-108,70));
                (*Patient)->StepBrain59(.25f);
                Check(TEXT("decoy101_patient_first_stops_to_inspect_the_cart"),(*Patient)->Target.Equals(Stop,1)&&
                    (*Patient)->GetPendingMovementInputVector().IsNearlyZero());
                for (int I=0;I<10;++I){(*Patient)->StepBrain59(.1f);(*Hound)->StepBrain59(.1f);}
                Check(TEXT("decoy101_patient_searches_ward_near_cart_stop"),(*Patient)->State59==TEXT("search")&&
                    FMath::IsNearlyEqual((*Patient)->Target.X,DoorAt.X,1.)&&(*Patient)->Target.Y<DoorAt.Y);
                Check(TEXT("decoy101_hound_checks_briefly_then_gives_up"),(*Hound)->State59==TEXT("return"));
                const auto BeforeRepeat = (*Patient)->Target;
                WardDecoy101::HearCart(G,CartName,WardDecoy101::ESound::Rattle,Initial,1080);
                Check(TEXT("decoy101_same_cart_cannot_immediately_restart_search"),(*Patient)->Target.Equals(BeforeRepeat,1));
                Book.Portals.SetNum(PortalCount);Book.Rooms.SetNum(RoomCount);
                (*Patient)->ResetMob();(*Hound)->ResetMob();(*Armoured)->ResetMob();(*Pursuer)->ResetMob();
            }
            else Check(TEXT("decoy101_required_monster_fixtures_exist"),false);
            WardContact98::ClearMotion(Book);Book.Movables.RemoveAt(CartIndex);CartActor->Destroy();
        }
        auto* RigProbe=G.GetWorld()->SpawnActor<AActor>();auto* Probe=NewObject<UEWardMonsterVisual59>(RigProbe);RigProbe->SetRootComponent(Probe);Probe->RegisterComponent();RigProbe->SetActorLocation(Arena);
        Check(TEXT("36_unique_creature_models_resolved"),Object(Data(),TEXT("models"))->Values.Num()==36);
        for(const auto& Entry:Object(Data(),TEXT("models"))->Values)
        {
            const FString Key=FString(Entry.Key);const auto Spec=Entry.Value->AsObject();const bool Loaded=Probe->Load59(Key,Number(Spec,TEXT("height_cm"),178));
            Check(Key+TEXT("_runtime_rig_and_albedo_load"),Loaded&&Probe->Vertices59()>1000);
            if(!Loaded)continue;
            for(int Clip=0;Clip<6;++Clip){Probe->Present59(Clip,.32f,false,0);bool Finite=true;for(int S=0;S<Probe->GetNumSections();++S)for(auto V:Probe->GetProcMeshSection(S)->ProcVertexBuffer)Finite&=!V.Position.ContainsNaN();Check(Key+FString::Printf(TEXT("_clip%d_finite"),Clip),Finite);}
            Probe->BeginFall61(FVector(1,0,0));for(int F=0;F<150;++F)Probe->TickFall61(1.f/60);
            auto Points=Probe->CaptureFall61();bool Safe=!Points.IsEmpty();for(auto P:Points)Safe&=!P.ContainsNaN()&&P.Z>=-15&&FVector::Dist(P,Arena)<650;
            Check(Key+TEXT("_constrained_fall_collides_with_floor"),Safe);Probe->ResetFall61();Probe->RestoreFall61(Points);Check(Key+TEXT("_settled_fall_restores"),Probe->CaptureFall61()==Points);Probe->Unload61();
        }
        RigProbe->Destroy();
        for(auto M:Story->Mobs){M->Active=false;M->SetActorEnableCollision(false);M->SetActorHiddenInGame(true);M->GetCharacterMovement()->DisableMovement();}
        TMap<FString,AEWardHospitalMob*> Types;for(auto M:Story->Mobs)if(!Types.Contains(M->Species))Types.Add(M->Species,M);
        // Spawn a parasite at a real, navigable authored position for visual loading.
        Story->SetChapter(6);auto* Carrier=Types.FindRef(TEXT("carrier"));
        auto* Parasite=Story->SpawnChild(Carrier->Home+FVector(100,0,0),TEXT("parasite"));if(Parasite)Types.Add(TEXT("parasite"),Parasite);
        Check(TEXT("spawned_parasite_gets_its_own_visual_and_rules"),Parasite&&Parasite->Visual59&&Parasite->Visual59->Vertices59()>50);
        for(auto& Pair:Types)
        {
            auto* M=Pair.Value;M->EnsureVisual61();const FString Prefix=M->Species+TEXT("_");
            M->SetActorLocation(Arena);M->SetActorRotation(FRotator::ZeroRotator);M->SetActorEnableCollision(false);
            auto Reset=[&]()
            {
                M->Active=true;M->Dead=false;M->Health=M->MaxHealth;M->Stunned=0;M->Memory=0;M->SearchLeft59=0;M->AttackClock=0;M->SpecialClock=100;M->StunImmunity59=0;M->Windup59=0;M->State59=TEXT("patrol");M->StateClock59=0;M->NoisePriority59=0;M->Target=Arena;M->LastSeen59=Arena;M->SearchOrigin59=Arena;M->SetActorLocation(Arena);M->SetActorRotation(FRotator::ZeroRotator);G.BookWorld->FurnitureHiding=false;G.HidingBed=nullptr;H->Health=100;H->Dead=false;G.Invulnerable=0;G.Won=false;
            };
            Reset();H->SetActorLocation(Arena+FVector(500,0,0));
            Check(Prefix+TEXT("blender_skinned_visual_loaded"),M->Visual59&&M->Visual59->Vertices59()>50);
            const bool Blind=Number(M->Rule59,TEXT("sight_cm"))==0;
            Check(Prefix+TEXT("forward_detection_matches_senses"),M->Sight59(H->GetActorLocation())!=Blind);
            H->SetActorLocation(Arena+FVector(-500,0,0));Check(Prefix+TEXT("no_vision_behind_back"),!M->Sight59(H->GetActorLocation()));
            H->SetActorLocation(Arena+FVector(500,0,0));auto* Blocker=Wall(Arena+FVector(250,0,30),FVector(12,180,150));
            Check(Prefix+TEXT("opaque_wall_blocks_sight"),!M->Sight59(H->GetActorLocation()));
            M->Hear(H->GetActorLocation(),100);Check(Prefix+TEXT("quiet_occluded_noise_is_not_heard"),M->Memory<=0);
            M->Hear(H->GetActorLocation(),2000);Check(Prefix+TEXT("loud_noise_starts_investigation"),M->Memory>0&&M->State59==TEXT("investigate"));
            Blocker->Destroy();Reset();H->SetActorLocation(Arena+FVector(90,0,0));M->Hear(H->GetActorLocation(),1000);M->StepBrain59(.01);
            Check(Prefix+TEXT("melee_has_visible_windup_not_instant_damage"),M->State59==TEXT("windup")&&H->Health==100);
            H->SetActorLocation(Arena+FVector(650,0,0));M->StepBrain59(float(Number(M->Rule59,TEXT("windup_seconds")))+.05f);
            Check(Prefix+TEXT("moving_away_dodges_the_strike"),H->Health==100&&M->State59==TEXT("recovery"));
            Reset();H->SetActorLocation(Arena+FVector(90,0,0));M->Hear(H->GetActorLocation(),1000);M->StepBrain59(.01);G.Invulnerable=0;
            M->StepBrain59(float(Number(M->Rule59,TEXT("windup_seconds")))+.05f);
            Check(Prefix+TEXT("strike_lands_once_after_windup"),H->Health<100&&H->Health>=100-Number(M->Definition,TEXT("damage"),12)-.1f);
            const float After=H->Health;M->StepBrain59(.01);Check(Prefix+TEXT("recovery_prevents_repeated_frame_damage"),H->Health==After);
            Reset();H->SetActorLocation(Arena+FVector(90,0,0));M->Hear(H->GetActorLocation(),1000);M->StepBrain59(.01);
            Blocker=Wall(Arena+FVector(45,0,30),FVector(5,180,150));M->StepBrain59(float(Number(M->Rule59,TEXT("windup_seconds")))+.05f);
            Check(Prefix+TEXT("strike_cannot_hit_through_closed_geometry"),H->Health==100);Blocker->Destroy();
            Reset();H->SetActorLocation(Arena+FVector(500,0,0));M->Hear(H->GetActorLocation(),1000);const FVector Heard=M->Target;
            G.BookWorld->FurnitureHiding=true;H->SetActorLocation(Arena+FVector(-500,700,0));M->StepBrain59(.05);
            Check(Prefix+TEXT("loss_of_sight_does_not_track_hidden_player"),M->Target.Equals(Heard,.1));
            M->Memory=0;M->SearchLeft59=.1f;M->StepBrain59(.2);Check(Prefix+TEXT("search_expires_and_returns_home"),M->State59==TEXT("return"));
            Reset();M->Stun59(3,true);Check(Prefix+TEXT("flash_respects_blindness"),Boolean(M->Rule59,TEXT("flash_immune"))?M->Stunned<=0:M->Stunned>2.9f);
            Reset();M->Hear(Arena+FVector(500,0,0),2000);M->Stun59(2);const Json Saved=M->Snapshot();M->Memory=0;M->Stunned=0;M->Restore(Saved);
            Check(Prefix+TEXT("save_restores_search_target_and_stun"),M->Memory>0&&M->Stunned>1.9f&&M->Target.Equals(Vector(Array(Saved,TEXT("target59"))),.1));
            M->Active=false;M->SetActorHiddenInGame(true);
        }
        G.BookWorld->FurnitureHiding=false;
        for(const FString Species:{FString(TEXT("patients")),FString(TEXT("blind_surgeon")),FString(TEXT("armoured")),FString(TEXT("nest"))})
        {
            auto* M=Types.FindRef(Species);M->ResetMob();M->EnsureVisual61();M->SetActorLocation(Arena);M->SetActorRotation(FRotator::ZeroRotator);H->SetActorLocation(Arena+FVector(500,0,0));CaptureModel59(G,M);
            bool Geometry=true;FBox Bounds(ForceInit);
            for(int I=0;I<M->Visual59->GetNumSections();++I)for(const auto& V:M->Visual59->GetProcMeshSection(I)->ProcVertexBuffer){Geometry&=!V.Position.ContainsNaN();Bounds+=V.Position;}
            Check(Species+TEXT("_posed_mesh_is_finite_and_human_scale"),Geometry&&Bounds.IsValid&&Bounds.GetSize().GetMax()<500);
        }
        {
            using namespace WardMonsterMotion97;
            const auto ArmouredStyle=Style(TEXT("armoured"));
            const auto HoundStyle=Style(TEXT("hound"));
            bool Profiles=Types.Num()==18;
            for(const auto& Pair:Types)Profiles&=Pair.Key==Style(Pair.Key).Species;
            Check(TEXT("motion97_profiles_cover_every_story_species"),Profiles);
            Check(TEXT("motion97_approach_faces_the_corner_before_feet_turn"), 
                CornerFacing(FVector::ForwardVector,FVector::RightVector,38).Y>.4 &&
                CornerFacing(FVector::ForwardVector,FVector::RightVector,130).Equals(FVector::ForwardVector,.01) &&
                HoundStyle.TurnRate>ArmouredStyle.TurnRate);
            auto Pose=[&](AEWardHospitalMob& Mob,const TCHAR* State,float ClockSeconds,int32 Clip,float Time)
            {
                Mob.State59=State;Mob.StateClock59=ClockSeconds;
                Mob.Visual59->ResetFall61();
                Mob.Visual59->Present59(Clip,Time,false,0);
            };
            auto* Hound=Types.FindRef(TEXT("hound"));Hound->ResetMob();Hound->EnsureVisual61();
            Hound->SetActorLocation(Arena);Hound->SetActorRotation(FRotator::ZeroRotator);
            Hound->AttackAim59=Arena+FVector(100,0,0);
            Pose(*Hound,TEXT("patrol"),0,0,0);
            const FVector HoundIdle=Hound->Visual59->MotionJoint97(0);
            Pose(*Hound,TEXT("windup"),Number(Hound->Rule59,TEXT("windup_seconds"))*.7,2,.27f);
            const FVector Low=Hound->Visual59->MotionJoint97(0);
            Check(TEXT("motion97_hound_crouches_before_pounce"), HoundIdle.Z-Low.Z>9.0 && HoundIdle.X-Low.X>2.0);
            CaptureModel59(G,Hound,TEXT("hound97_crouch"));
            Pose(*Hound,TEXT("recovery"),0,2,.78f);
            const auto Takeoff=Hound->Visual59->MotionJoint97(0);
            Pose(*Hound,TEXT("recovery"),.13f,2,.78f);
            Check(TEXT("motion97_hound_leaves_ground_and_lands_in_recovery"),
                Hound->Visual59->MotionJoint97(0).Z-Takeoff.Z>17.0 && FMath::Abs(PounceHeight(.26f))<.01f);
            CaptureModel59(G,Hound,TEXT("hound97_pounce"));
            Hound->GetCharacterMovement()->Velocity=FVector(275,0,0);
            Pose(*Hound,TEXT("chase"),0,5,0);
            Hound->GetCharacterMovement()->StopMovementImmediately();
            Hound->State59=TEXT("patrol");Hound->Visual59->Present59(0,0,false,0);
            const FVector Skid=Hound->Visual59->MotionJoint97(0);
            Check(TEXT("motion97_hound_braces_after_a_sharp_stop"),Skid.X-HoundIdle.X>4.0 &&
                Hound->GetCharacterMovement()->bUseSeparateBrakingFriction);
            CaptureModel59(G,Hound,TEXT("hound97_brake"));
            auto* Heavy=Types.FindRef(TEXT("armoured"));Heavy->ResetMob();Heavy->EnsureVisual61();
            Heavy->SetActorLocation(Arena);Heavy->SetActorRotation(FRotator::ZeroRotator);
            Heavy->AttackAim59=Arena+FVector(100,0,0);
            Pose(*Heavy,TEXT("patrol"),0,0,0);
            const FVector HeavyIdle=Heavy->Visual59->MotionJoint97(0);
            Pose(*Heavy,TEXT("windup"),Number(Heavy->Rule59,TEXT("windup_seconds"))*.7,2,.25f);
            Check(TEXT("motion97_heavy_loads_one_leg_before_strike"), FMath::Abs(Heavy->Visual59->MotionJoint97(0).Y-HeavyIdle.Y)>8.0);
            CaptureModel59(G,Heavy,TEXT("armoured97_brace"));

            auto* Patient=Types.FindRef(TEXT("patients"));Patient->ResetMob();Patient->EnsureVisual61();
            Patient->SetActorLocation(Arena);Patient->SetActorRotation(FRotator::ZeroRotator);
            Pose(*Patient,TEXT("patrol"),0,0,0);
            const FVector Left=Patient->Visual59->MotionJoint97(12,true), Right=Patient->Visual59->MotionJoint97(15,true);
            const double Separation=FMath::Abs(Right.Y-Left.Y);
            const double Base=Patient->Visual59->GetComponentLocation().Z;
            auto* LeftSupport=Wall(FVector(Left.X,Left.Y,Base-6),FVector(35,FMath::Max(1.,Separation*.41),6));
            auto* RightSupport=Wall(FVector(Right.X,Right.Y,Base+11),FVector(35,FMath::Max(1.,Separation*.41),6));
            Pose(*Patient,TEXT("patrol"),0,0,0);
            const FVector SteppedLeft=Patient->Visual59->MotionJoint97(12,true), SteppedRight=Patient->Visual59->MotionJoint97(15,true);
            Check(TEXT("motion97_feet_take_independent_heights_on_treads"),Separation>3 &&
                (SteppedRight.Z-SteppedLeft.Z)-(Right.Z-Left.Z)>8.0);
            LeftSupport->Destroy();RightSupport->Destroy();
            Patient->Path={Arena+FVector(41,0,0),Arena+FVector(180,0,0)};Patient->PathIndex=0;
            Pose(*Patient,TEXT("chase"),0,1,.13f);
            const FVector Straight=Patient->Visual59->MotionChestForward97();
            Patient->Path[1]=Arena+FVector(41,140,0);
            Pose(*Patient,TEXT("chase"),0,1,.13f);
            const FVector Lead=Patient->Visual59->MotionChestForward97();
            const double YawDelta=FMath::FindDeltaAngleDegrees(Straight.Rotation().Yaw,Lead.Rotation().Yaw);
            UE_LOG(LogTemp,Display,TEXT("MOTION97 chest straight=%s lead=%s delta=%.2f actor_yaw=%.2f"),
                *Straight.ToString(),*Lead.ToString(),YawDelta,Patient->GetActorRotation().Yaw);
            Check(TEXT("motion97_shoulders_begin_turn_while_capsule_still_faces_forward"),
                YawDelta>4.0 && FMath::Abs(Patient->GetActorRotation().Yaw)<.01);
        }
        FDamageEvent Event;
        for(auto& Pair:Types)
        {
            auto* M=Pair.Value;M->ResetMob();M->SetActorLocation(Arena);M->Active=true;M->StunImmunity59=0;H->SetActorLocation(Arena+FVector(0,500,0));
            Story->SetChapter(FMath::Min(M->FirstChapter,18));M->Active=true;M->TakeDamage(10000,Event,H->GetController(),H);
            Check(M->Species+TEXT("_death_class_rule_before_finale"),M->Class==TEXT("A")?M->Dead:M->Class==TEXT("B")?!M->Dead&&M->Stunned>=9:!M->Dead&&M->Health>0);
            if(M->Class==TEXT("B")){Story->SetChapter(19);M->Active=true;M->Stunned=0;M->TakeDamage(10000,Event,H->GetController(),H);Check(M->Species+TEXT("_finale_allows_death"),M->Dead);}
        }
        // Species-specific counterplay is checked against the actual node and
        // encounter data, independently of the shared perception tests.
        auto* Armoured=Types.FindRef(TEXT("armoured"));
        Story->SetChapter(14);Armoured->ResetMob();Armoured->Active=true;Armoured->SetActorLocation(Arena);Armoured->SetActorRotation(FRotator::ZeroRotator);
        H->SetActorLocation(Arena+FVector(300,0,0));Armoured->TakeDamage(20,Event,H->GetController(),H);const float FrontDamage=Armoured->MaxHealth-Armoured->Health;
        Armoured->Health=Armoured->MaxHealth;Armoured->StunImmunity59=0;H->SetActorLocation(Arena-FVector(300,0,0));Armoured->TakeDamage(20,Event,H->GetController(),H);
        Check(TEXT("armoured_back_is_more_vulnerable_than_front"),Armoured->MaxHealth-Armoured->Health>FrontDamage*4);
        Story->SetChapter(15);auto* Collector=Types.FindRef(TEXT("collector"));Collector->ResetMob();Collector->Active=true;Collector->Health=10;Collector->SpecialClock=0;
        FString NodeId;FVector NodePosition;
        for(const auto& E:Story->Definitions)if(String(E.Value,TEXT("kind"))==TEXT("node")&&Number(E.Value,TEXT("chapter"))<=15){NodeId=E.Key;NodePosition=Vector(Array(E.Value,TEXT("at")))*100+FVector(0,0,91);break;}
        Collector->SetActorLocation(NodePosition);Story->Mode->Completed.Remove(NodeId);Collector->Special59(.1,false);
        Check(TEXT("collector_regenerates_near_live_node"),!NodeId.IsEmpty()&&Collector->Health>10);
        TSet<FString> NearNodes;
        for(const auto& E:Story->Definitions)if(String(E.Value,TEXT("kind"))==TEXT("node")&&FVector::Dist(Vector(Array(E.Value,TEXT("at")))*100,NodePosition)<2300){NearNodes.Add(E.Key);G.Completed.Add(E.Key);}
        Collector->Health=10;Collector->SpecialClock=0;Collector->Special59(.1,false);Check(TEXT("collector_stops_regeneration_when_local_nodes_are_off"),Collector->Health==10);
        auto* Linked=Types.FindRef(TEXT("linked"));Linked->ResetMob();Linked->Active=true;Linked->SetActorLocation(NodePosition);Linked->NodePowered59=true;Linked->Special59(.1,false);
        Check(TEXT("cutting_node_stuns_linked_network_monster"),Linked->Stunned>2);
        for(const auto& Id:NearNodes)G.Completed.Remove(Id);
        Story->SetChapter(6);Carrier->ResetMob();Carrier->Active=true;Carrier->FinishDeath59();const int ChildrenOnce=Story->SpawnedChildren;Carrier->FinishDeath59();
        Check(TEXT("carrier_emits_children_only_once_per_death"),Story->SpawnedChildren==ChildrenOnce&&ChildrenOnce<=24);
        const Json CarrierSave=Carrier->Snapshot();Carrier->Restore(CarrierSave);Check(TEXT("carrier_split_latch_survives_save"),Carrier->SpawnedOnDeath59);
        Story->SetChapter(9);auto* Hauler=Types.FindRef(TEXT("transporter"));Hauler->ResetMob();Hauler->Active=true;
        Carrier->FirstChapter=9;Carrier->SetActorLocation(Hauler->Home+FVector(120,0,0));Carrier->Dead=true;Carrier->Revived59=false;Hauler->SpecialClock=0;Hauler->Special59(.1,false);
        Check(TEXT("transporter_can_revive_eligible_class_A_once"),!Carrier->Dead&&Carrier->Revived59);
        Carrier->Dead=true;Carrier->Active=false;Hauler->SpecialClock=0;Hauler->Special59(.1,false);Check(TEXT("same_corpse_cannot_be_revived_repeatedly"),Carrier->Dead);Carrier->FirstChapter=6;
        Story->SetChapter(19);auto* Pursuer=Types.FindRef(TEXT("pursuer"));Pursuer->ResetMob();Pursuer->Active=true;Pursuer->TakeDamage(100000,Event,H->GetController(),H);
        Check(TEXT("pursuer_never_becomes_killable_by_unconfirmed_finale_variant"),!Pursuer->Dead&&Pursuer->Class==TEXT("C"));
        Story->SetChapter(7);auto* Keeper=Types.FindRef(TEXT("keeper"));Keeper->ResetMob();Keeper->Active=true;Keeper->SetActorLocation(FVector(-70000,-70000,-869));H->SetActorLocation(Keeper->GetActorLocation()+FVector(500,0,0));G.Flag(TEXT("monster_camera_B2_off59"),true);
        Check(TEXT("local_camera_power_off_disables_keeper_visual_detection"),!Keeper->Sight59(H->GetActorLocation())&&!Keeper->CameraSees59());
        H->SetActorLocation(Keeper->GetActorLocation()+FVector(100,0,0));
        Check(TEXT("camera_power_off_cannot_be_bypassed_by_attack_or_dialogue_sight"),!Keeper->Sight59(H->GetActorLocation(),true)&&!Keeper->AttackClear59());
        Keeper->Memory=0;Keeper->Hear(H->GetActorLocation(),2000);
        Check(TEXT("disconnected_keeper_cannot_reacquire_target_through_sound"),Keeper->Memory==0);
        G.Flag(TEXT("monster_camera_B2_off59"),false);
        Check(TEXT("reenabling_camera_segment_restores_keeper_detection_and_contact"),Keeper->Sight59(H->GetActorLocation(),true)&&Keeper->AttackClear59());
        auto* Organism=Types.FindRef(TEXT("central_organism"));
        const float Reach=Number(Organism->Rule59,TEXT("attack_reach_cm"),145);
        Check(TEXT("stationary_contact_still_threatens_nearby_participants"),ContactThreatAt(*Organism,Organism->GetActorLocation()+FVector(Reach-10,0,0)));
        Check(TEXT("stationary_contact_cannot_interrupt_distant_conversation"),!ContactThreatAt(*Organism,Organism->GetActorLocation()+FVector(Reach+150,0,0)));
        Check(TEXT("mobile_enemy_retains_long_range_conversation_threat"),ContactThreatAt(*Keeper,Keeper->GetActorLocation()+FVector(700,0,0)));
        bool SealedFound=false,SealedBlocked=false;
        for(auto L:Array(WardBookMapData(),TEXT("locations")))for(auto R:Array(L->AsObject(),TEXT("rooms")))for(auto State:Array(R->AsObject(),TEXT("states")))
            if(!SealedFound&&String(State->AsObject(),TEXT("state"))==TEXT("sealed"))
            {
                SealedFound=true;const FString Old=Keeper->Zone59;const int OldCh=Keeper->FirstChapter;Keeper->Zone59=String(R->AsObject(),TEXT("id"));Keeper->FirstChapter=Number(State->AsObject(),TEXT("chapter"));
                G.Values.Add(TEXT("hospital_chapter"),Keeper->FirstChapter);SealedBlocked=!Keeper->Eligible59();Keeper->Zone59=Old;Keeper->FirstChapter=OldCh;
            }
        Check(TEXT("sealed_zone_prevents_encounter_activation"),SealedFound&&SealedBlocked);
        // Physical route check runs in ordinary game frames on the hospital map.
        for(auto M:Story->Mobs){M->ResetMob();M->Active=false;M->SetActorEnableCollision(false);}
        Story->SetChapter(2);Walker=Types.FindRef(TEXT("patients"));
        TArray<FVector> Route;
        for(int I=8;I<=16&&Route.IsEmpty();++I)for(FIntVector Offset:{FIntVector(I,0,0),FIntVector(-I,0,0),FIntVector(0,I,0),FIntVector(0,-I,0)})
        {Route=Story->FindPath(Walker->Home,Story->WorldAt(Story->CellAt(Walker->Home)+Offset),30);if(!Route.IsEmpty())break;}
        Check(TEXT("real_hospital_patrol_route_exists"),!Route.IsEmpty());
        if(!Route.IsEmpty())
        {
            H->SetActorLocation(Route.Last());H->Health=100;H->Dead=false;G.Invulnerable=100;G.Won=false;G.Show(TEXT(""));
            Walker->Target=Route.Last();Walker->LastSeen59=Route.Last();Walker->Memory=12;Walker->ChangeState59(TEXT("investigate"));Walker->Active=true;Walker->SetActorHiddenInGame(false);Walker->SetActorEnableCollision(true);Walker->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Before=Walker->GetActorLocation();
        }
        Story->FreezeAI=false;Clock=0;Stage=1;
    }
    else if(Stage==1)
    {
        if(FirstWalkFrame){FirstWalkFrame=false;Clock=0;return;}
        if(Clock>.5f)FrameTimes.Add(FApp::GetDeltaTime());
        // Use ordinary actor/movement ticks here. A frozen-world manual brain
        // step clears acceleration every frame and cannot test real locomotion.
        if(Clock>6)
        {
            Check(TEXT("monster_physically_walks_real_map_without_teleport"),FVector::Dist2D(Before,Walker->GetActorLocation())>45&&Walker->GetCharacterMovement()->IsMovingOnGround());
            UE_LOG(LogTemp,Display,TEXT("WALK61 before=%s after=%s target=%s state=%s mode=%d active=%d path=%d/%d speed=%s radius=%.1f height=%.1f"),*Before.ToString(),*Walker->GetActorLocation().ToString(),*Walker->Target.ToString(),*Walker->State59,int(Walker->GetCharacterMovement()->MovementMode),Walker->Active,Walker->PathIndex,Walker->Path.Num(),*Walker->GetVelocity().ToString(),Walker->GetCapsuleComponent()->GetScaledCapsuleRadius(),Walker->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
            bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));
            Json Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),Pass);Result->SetNumberField(TEXT("count"),Checks.Num());Result->SetArrayField(TEXT("checks"),Checks);
            FrameTimes.Sort();double Sum=0;for(float T:FrameTimes)Sum+=T;
            Json Timing=MakeShared<FJsonObject>();Timing->SetStringField(TEXT("scope"),TEXT("6-second automated hospital walk after setup and 0.5-second warmup, offscreen 1920x1080, two CPU cores; not whole-game benchmark"));
            if(FrameTimes.Num()){Timing->SetNumberField(TEXT("average_frame_ms"),Sum/FrameTimes.Num()*1000);Timing->SetNumberField(TEXT("p95_frame_ms"),FrameTimes[FMath::Min(FrameTimes.Num()-1,FMath::FloorToInt(FrameTimes.Num()*.95))]*1000);}
            Timing->SetNumberField(TEXT("process_physical_MB"),FPlatformMemory::GetStats().UsedPhysical/1048576.0);Result->SetObjectField(TEXT("performance_sample"),Timing);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/monsters59_check.json"),Result);
            Stage=2;FPlatformMisc::RequestExit(false);
        }
    }
}
}

