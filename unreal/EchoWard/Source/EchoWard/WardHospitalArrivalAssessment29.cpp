#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardEchoAssets.h"
#include "Engine/StaticMesh.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
using namespace WardData;
void AEWardHospitalGameplay::AssessArrival(float D)
{
    static int Stage=0;static float Clock=0;static TArray<TSharedPtr<FJsonValue>> Checks;static TArray<TSharedPtr<FJsonValue>> Shots;static FVector SavedPosition;
    auto Check=[&](const FString& Name,bool OK){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),OK);Checks.Add(MakeShared<FJsonValueObject>(C));UE_LOG(LogTemp,Display,TEXT("ARRIVAL_CHECK %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),*Name);};
    auto Shot=[&](const FString& Name){if(FParse::Param(FCommandLine::Get(),TEXT("WardArrivalPictures"))){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/ArrivalCaptures")/(Name+TEXT(".png")),true,false);Shots.Add(MakeShared<FJsonValueString>(Name+TEXT(".png")));}};
    auto* H=Mode->Hero();auto* PC=Cast<APlayerController>(H->Controller);Clock+=D;Mode->BookWorld->Update(D);Mode->BookWorld->UpdateLighting(H->GetActorLocation());
    if(Stage==0)
    {
        Mode->Command(TEXT("new_normal"));Check(TEXT("new_game_starts_car_arrival"),ArrivalStage==0&&Mode->Values.FindRef(TEXT("hospital_step"))==0);
        Check(TEXT("canon_accident_happened_long_before_arrival"),String(ArrivalData,TEXT("accident_timing"))==TEXT("long_before_hero_arrival")&&!Boolean(ArrivalData,TEXT("new_laboratory_accident_on_arrival")));
        Check(TEXT("car_model_and_checkpoint_text_use_native_assets"),WardEchoAssets::Mesh(Number(ArrivalData,TEXT("car_model")))&&ArrivalCar&&ArrivalNotice);
        auto* CarMesh=ArrivalCar?ArrivalCar->FindComponentByClass<UStaticMeshComponent>():nullptr;
        const FVector Travel=Vector(Array(ArrivalData,TEXT("car_to")))-Vector(Array(ArrivalData,TEXT("car_from")));
        Check(TEXT("v30_sedan_source_front_faces_forward_along_the_arrival_route"),WardEchoAssets::ResolvedId(Number(ArrivalData,TEXT("car_model")))!=920||(CarMesh&&FVector::DotProduct(-CarMesh->GetForwardVector(),Travel.GetSafeNormal())>.99f));
        Check(TEXT("v31_hero_front_and_hood_face_the_approach"),FVector::DotProduct(ArrivalCar->GetActorForwardVector(),Travel.GetSafeNormal())>.99f&&FVector::DotProduct(VehicleParts31[TEXT("Hood")]->GetComponentLocation()-ArrivalCar->GetActorLocation(),Travel.GetSafeNormal())>80&&FVector::DotProduct(VehicleParts31[TEXT("Trunk")]->GetComponentLocation()-ArrivalCar->GetActorLocation(),Travel.GetSafeNormal())<-150);
        Check(TEXT("v31_hero_has_body_four_doors_hood_trunk_wheels_and_steering"),VehicleParts31.Num()==12&&VehicleTarget31.Num()==6);
        Check(TEXT("cannot_save_while_seated_in_car"),!Mode->Save(TEXT("manual_6")));
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ArrivalCarRoute),false,H);Q.AddIgnoredActor(ArrivalCar);Mode->Notify(String(ArrivalData,TEXT("sister_message")),8);bool Clear=true;
        for(int I=0;I<=17;++I)
        {
            const FVector At=FMath::Lerp(Vector(Array(ArrivalData,TEXT("car_from"))),Vector(Array(ArrivalData,TEXT("car_to"))),I/17.f)*100+FVector(0,0,75);
            FHitResult Hit;const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,At,At+FVector(0,.1f,0),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(FVector(95,244,65)),Q);
            if(Blocked){Clear=false;UE_LOG(LogTemp,Error,TEXT("ARRIVAL_CAR_BLOCK %s / %s at %s"),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),*At.ToString());}
        }
        Check(TEXT("entire_vehicle_approach_is_clear_at_one_metre_intervals"),Clear);ArrivalClock=3;TickArrival(0);Clock=0;Stage=1;
    }
    else if(Stage==1&&Clock>1)
    {
        Check(TEXT("car_physically_moves_and_has_a_follow_camera"),ArrivalCar->GetActorLocation().Y>-7800&&ArrivalCar->GetActorLocation().Y<-6100&&PC->GetViewTarget()==ArrivalCamera&&H->IsHidden());
        const FVector Seat = ArrivalCar->GetActorTransform().TransformPosition(Vector(Array(Object(WardEchoAssets::Cars31(),TEXT("hero")),TEXT("driver_eye_cm"))));
        Check(TEXT("the_moving_camera_is_at_the_driver_seat"), ArrivalStage==0 && Seated31 && ArrivalCamera->GetActorLocation().Equals(Seat,.5));
        Shot(TEXT("01_drive"));Stage=2;Clock=0;
    }
    else if(Stage==2 && ArrivalStage==1)
    {
        Check(TEXT("cabin_drive_parks_automatically_after_six_to_seven_seconds"),
            ArrivalClock>=6.f && ArrivalClock<=7.f && Seated31 && !Mode->Completed.Contains(TEXT("SCENE_01_01")));Shot(TEXT("02_parked"));Stage=3;Clock=0;
    }
    else if(Stage==3&&Clock>.5f)
    {
        ArrivalInteract();Check(TEXT("F_exits_car_and_restores_walking"),ArrivalStage==2&&!H->IsHidden()&&H->GetCapsuleComponent()->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics&&H->GetCharacterMovement()->MovementMode==MOVE_Walking&&Mode->Completed.Contains(TEXT("SCENE_01_01"))&&Mode->Values.FindRef(TEXT("hospital_step"))==1);
        Check(TEXT("v31_exit_keeps_driver_door_in_reach"),NearestVehiclePart31()==TEXT("DoorFL"));
        UseId(TEXT("SCENE_01_02"));Check(TEXT("reception_cannot_skip_checkpoint"),ArrivalStage==2&&!Mode->Completed.Contains(TEXT("SCENE_01_02")));
        H->SetActorLocation(FVector(5500,-7400,98),false);
        Check(TEXT("remote_checkpoint_interaction_is_rejected"),!ArrivalInteract()&&!Mode->Flag(TEXT("old_lab_accident29")));
        Check(TEXT("free_walk_progress_can_be_saved"),Mode->Save(TEXT("manual_1")));ArrivalStage=5;Check(TEXT("save_resumes_before_checkpoint_without_replaying_drive"),Mode->Load(TEXT("manual_1"))&&ArrivalStage==2&&!Mode->Flag(TEXT("old_lab_accident29"))&&PC->GetViewTarget()==H);
        VehicleAutoClose31=0;for(auto& P:VehicleOpen31){P.Value=0;VehicleTarget31.Add(P.Key,0);}TickVehicle31(0);
        const auto* Door=VehicleParts31.FindRef(TEXT("DoorFL")).Get();const FVector DoorAt=ArrivalCar->GetActorTransform().TransformPosition(Door->GetRelativeLocation()+Door->GetStaticMesh()->GetBoundingBox().GetCenter()+FVector(0,-150,0));H->SetActorLocation(FVector(DoorAt.X,DoorAt.Y,98),false);Check(TEXT("v31_driver_door_is_selected_from_outside"),NearestVehiclePart31()==TEXT("DoorFL"));ToggleVehiclePart31();TickVehicle31(.7f);Check(TEXT("v31_door_rotates_about_its_own_hinge"),VehicleOpen31.FindRef(TEXT("DoorFL"))>.99f&&FMath::Abs(Door->GetRelativeRotation().Yaw-65)<.1f);ToggleVehiclePart31();TickVehicle31(.7f);Check(TEXT("v31_driver_door_closes"),FMath::Abs(Door->GetRelativeRotation().Yaw)<.1f);
        Check(TEXT("v31_F_enters_the_driver_seat"),VehicleInteract31()&&Seated31&&H->IsHidden()&&PC->GetViewTarget()==ArrivalCamera&&H->GetCharacterMovement()->MovementMode==MOVE_None);
        const FVector Eye=ArrivalCar->GetActorTransform().TransformPosition(Vector(Array(Object(WardEchoAssets::Cars31(),TEXT("hero")),TEXT("driver_eye_cm"))));UE_LOG(LogTemp,Display,TEXT("V31_SEAT_DIAGNOSTIC eye=%s camera=%s hero=%s collision=%d seated=%d"),*Eye.ToString(),*ArrivalCamera->GetActorLocation().ToString(),*H->GetActorLocation().ToString(),(int32)H->GetCapsuleComponent()->GetCollisionEnabled(),Seated31);Check(TEXT("v31_camera_is_inside_the_real_interior"),ArrivalCamera->GetActorLocation().Equals(Eye,.5)&&FVector::Dist(H->GetActorLocation(),Eye)<20&&H->GetCapsuleComponent()->GetCollisionEnabled()==ECollisionEnabled::NoCollision);Check(TEXT("v31_reentry_blocks_saving"),!Mode->Save(TEXT("manual_6")));const float Stamina=H->Stamina;Mode->Dash();Check(TEXT("v31_seat_blocks_dash"),H->Stamina==Stamina);
        VehicleYaw31=35;VehiclePitch31=-10;TickVehicle31(0);bool FullDetail32=VehicleCloseQuality32;for(const auto& Pair:VehicleParts31)FullDetail32&=Pair.Value&&Pair.Value->GetForcedLodModel()==1;Check(TEXT("v32_cabin_keeps_original_base_geometry"),FullDetail32);Check(TEXT("v32_capture_is_native_1080p"),GEngine->GameViewport->Viewport->GetSizeXY()==FIntPoint(1920,1080)&&IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"))->GetFloat()>=99);Check(TEXT("v31_interior_camera_can_look_around"),FMath::Abs(ArrivalCamera->GetActorRotation().Yaw-125)<.1f);Mode->Show(TEXT("pause"));Mode->Show(TEXT(""));TickVehicle31(0);Check(TEXT("v31_pause_resume_keeps_seated_camera"),Seated31&&H->IsHidden()&&PC->GetViewTarget()==ArrivalCamera);Stage=34;Clock=0;

    }
    else if(Stage==34&&Clock>2)
    {H->NoticeTime=0;Shot(TEXT("08_interior"));Stage=31;Clock=0;}
    else if(Stage==31&&Clock>.6f)
    {
        Check(TEXT("v31_F_exits_after_reentry"),VehicleInteract31()&&!Seated31&&!H->IsHidden()&&H->GetCharacterMovement()->MovementMode==MOVE_Walking);VehicleAutoClose31=0;
        for(const FString Name:{TEXT("DoorFL"),TEXT("DoorFR"),TEXT("DoorRL"),TEXT("DoorRR"),TEXT("Hood"),TEXT("Trunk")})
        {
            auto* C=VehicleParts31.FindRef(Name).Get();const FVector Local=C->GetRelativeLocation()+C->GetStaticMesh()->GetBoundingBox().GetCenter();const FVector Out=Name==TEXT("Hood")?FVector(130,0,0):Name==TEXT("Trunk")?FVector(-130,0,0):Name.EndsWith(TEXT("L"))?FVector(0,-150,0):FVector(0,150,0);FVector At=ArrivalCar->GetActorTransform().TransformPosition(Local+Out);At.Z=98;H->SetActorLocation(At,false);VehicleTarget31.Add(Name,0);VehicleOpen31.Add(Name,0);ToggleVehiclePart31();TickVehicle31(.8f);Check(TEXT("v31_interactive_panel_opens_")+Name,VehicleOpen31.FindRef(Name)>.99f);
        }
        Check(TEXT("v31_open_panels_save"),Mode->Save(TEXT("manual_3")));for(auto& P:VehicleTarget31)P.Value=0;TickVehicle31(1);Check(TEXT("v31_panel_states_restore_from_save"),Mode->Load(TEXT("manual_3"))&&VehicleTarget31.FindRef(TEXT("Hood"))==1&&VehicleTarget31.FindRef(TEXT("Trunk"))==1);TickVehicle31(0);
        const FVector At=ArrivalCar->GetActorLocation(),Eye=At+FVector(430,520,290);ArrivalCamera->SetActorLocationAndRotation(Eye,(At+FVector(0,0,70)-Eye).Rotation());PC->SetViewTarget(ArrivalCamera);Stage=32;Clock=0;
    }
    else if(Stage==32&&Clock>1)
    {
        Shot(TEXT("09_all_panels_open"));Stage=33;Clock=0;
    }
    else if(Stage==33&&Clock>.3f)
    {
        for(auto& P:VehicleTarget31)P.Value=0;TickVehicle31(1);H->SetActorLocation(Vector(Array(ArrivalData,TEXT("checkpoint")))*100,false);PC->SetViewTarget(H);PC->SetControlRotation(FRotator(-5,90,0));Stage=4;Clock=0;
    }
    else if(Stage==4&&Clock>1)
    {
        const int Entries=Mode->Journal.Num();Check(TEXT("checkpoint_reveals_old_accident_and_opens_gate"),ArrivalInteract()&&ArrivalStage==3&&Mode->Flag(TEXT("old_lab_accident29"))&&Mode->Journal.Num()==Entries+1);
        Check(TEXT("checkpoint_cannot_duplicate_record"),!ArrivalInteract()&&Mode->Journal.Num()==Entries+1);Shot(TEXT("03_checkpoint_record"));Stage=5;Clock=0;
    }
    else if(Stage==5&&Clock>1.9f)
    {
        Check(TEXT("barrier_opens_and_stops_blocking_walkway"),GateBlend>.99f&&ArrivalGateBlock->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
        const auto& Route=Array(ArrivalData,TEXT("route"));bool Clear=true;FCollisionQueryParams Q(SCENE_QUERY_STAT(ArrivalWalkingRoute),false,H);
        for(int I=1;I<Route.Num();++I)
        {
            const FVector A=Vector(Route[I-1]->AsArray())*100,B=Vector(Route[I]->AsArray())*100;FHitResult Hit;
            if(GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q)){Clear=false;UE_LOG(LogTemp,Error,TEXT("ARRIVAL_WALK_BLOCK segment=%d %s / %s at %s"),I,*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),*Hit.Location.ToString());}
        }
        Check(TEXT("continuous_walking_route_car_checkpoint_vestibule_reception_is_clear"),Clear);
        H->SetActorLocation(FVector(4800,-1100,98),false);PC->SetControlRotation(FRotator(-4,90,0));TickArrival(0);Check(TEXT("entry_activates_security_not_a_new_laboratory_accident"),ArrivalStage==4&&Mode->Flag(TEXT("arrival_isolated29"))&&Mode->Flag(TEXT("old_lab_accident29")));Stage=6;Clock=0;
    }
    else if(Stage==6&&Clock>1)
    {
        Shot(TEXT("04_reception_security"));Stage=7;Clock=0;
    }
    else if(Stage==7&&ArrivalStage==5&&Clock>1)
    {
        int Active=0;for(auto M:Mobs)Active+=M->Active;
        Check(TEXT("intro_continues_chapter_one_at_doctors_task"),Chapter()==1&&Mode->Values.FindRef(TEXT("hospital_step"))==2&&Mode->Completed.Contains(TEXT("SCENE_01_02")));
        Check(TEXT("prologue_keeps_monsters_inactive"),Active==0);
        bool Locked=false;for(const auto& P:Mode->BookWorld->Portals)if(P.Zone==TEXT("N02")&&P.Side==TEXT("north"))Locked|=!P.Open&&P.Locked;
        Check(TEXT("security_locks_only_after_player_reaches_reception"),Locked);
        SavedPosition=H->GetActorLocation();Check(TEXT("automatic_save_keeps_the_doctors_dialogue_visible"),Mode->Save(TEXT("auto_3"),true)&&H->NoticeTime>5);Shot(TEXT("05_doctor_task"));Stage=8;Clock=0;
    }
    else if(Stage==8&&Clock>.5f)
    {
        Check(TEXT("arrival_progress_after_entry_is_saved"),Mode->Save(TEXT("manual_2")));
        AEWardBookWorld::FPortal* Entrance=nullptr;for(auto& P:Mode->BookWorld->Portals)if(P.Zone==TEXT("N02")&&P.Side==TEXT("north")){Entrance=&P;break;}
        bool Sealed=false,LaterOpen=false;if(Entrance){H->SetActorLocation(Entrance->At+FVector(0,100,98),false);Mode->BookWorld->UseArchitecture();Sealed=!Entrance->Open&&Entrance->Type==TEXT("shutter");SetChapter(4);LaterOpen=Entrance->Open&&Entrance->Type==TEXT("ordinary");}
        Check(TEXT("security_seal_cannot_be_opened_from_inside_but_chapter_four_releases_it"),Sealed&&LaterOpen);
        const Json Saved=Mode->ReadSlot(TEXT("manual_2"));Object(Saved,TEXT("hospital_story"))->RemoveField(TEXT("arrival29"));Check(TEXT("older_saves_keep_their_position_and_skip_intro"),Mode->Restore(Saved)&&ArrivalStage==-1&&H->GetActorLocation().Equals(SavedPosition,5));
        Check(TEXT("current_save_restores_old_accident_and_security_state"),Mode->Load(TEXT("manual_2"))&&ArrivalStage==5&&Mode->Flag(TEXT("old_lab_accident29"))&&Mode->Flag(TEXT("arrival_isolated29"))&&Mode->Values.FindRef(TEXT("hospital_step"))==2);
        Mode->Show(TEXT("pause"));Mode->Show(TEXT(""));Check(TEXT("pause_resume_returns_to_hero_camera"),PC->GetViewTarget()==H&&Mode->Screen.IsEmpty());Shot(TEXT("06_resume"));Stage=9;Clock=0;
    }
    else if(Stage==9&&Clock>.5f)
    {
        const FVector At=Vector(Array(ArrivalData,TEXT("lab_at")))*100;H->SetActorLocation(At+FVector(-170,-230,98),false);ArrivalCamera->SetActorLocationAndRotation(At+FVector(-210,-320,170),(At+FVector(0,0,25)-(At+FVector(-210,-320,170))).Rotation());ArrivalCamera->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;PC->SetViewTarget(ArrivalCamera);Mode->BookWorld->UpdateLighting(H->GetActorLocation());Stage=10;Clock=0;
    }
    else if(Stage==10&&Clock>1)
    {
        Shot(TEXT("07_old_lab_remnant"));bool OK=true;for(auto C:Checks)OK&=Boolean(C->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),OK);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);R->SetArrayField(TEXT("screenshots"),Shots);R->SetStringField(TEXT("canon"),TEXT("The laboratory accident predates arrival. No new accident is triggered."));Write(FPaths::ProjectSavedDir()/TEXT("Prototype/arrival29_check.json"),R);Stage=11;Clock=0;
    }
    else if(Stage==11&&Clock>1)FPlatformMisc::RequestExit(false);
    if(Clock>35&&Stage!=11){Check(TEXT("arrival_test_did_not_stall"),false);Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),false);R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/arrival29_check.json"),R);FPlatformMisc::RequestExit(false);}
}
