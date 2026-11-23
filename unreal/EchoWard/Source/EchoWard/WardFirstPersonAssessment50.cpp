#include "WardCampaign.h"
#include "WardBekaAnimation.h"
#include "WardBookWorld.h"
#include "WardHospitalGameplay.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "ContentStreaming.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif
using namespace WardData;

void AEWardCampaignMode::TickFirstPersonAssessment50(float Delta)
{
    static int32 Stage=0;static float Clock=0;static TArray<TSharedPtr<FJsonValue>> Checks;
    static FVector Start,WalkStart;static float StandingEye=0;static int32 BeforeShots=0,BeforeRounds=0,BeforeTotal=0;
    static ACameraActor* Portrait=nullptr;static bool Warmed=false,Placed=false;static FRotator PhotoView;static float RouteSight=0;
    const FVector Fixture(500000,0,0);
    auto Check=[&](const TCHAR* Name,bool Passed)
    {
        Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(C));
        UE_LOG(LogTemp,Display,TEXT("FIRSTPERSON50 %s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Name);
    };
    auto Next=[&](){++Stage;Clock=0;};
    auto Shot=[&](const TCHAR* Name)
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("nullrhi")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/FirstPerson50")/(FString(Name)+TEXT(".png")),true,false);
    };
    auto* H=Hero();auto* PC=H?Cast<APlayerController>(H->Controller):nullptr;if(!H||!PC)return;
    Clock+=Delta;
    if(Stage==0)
    {
        if(HospitalStory){HospitalStory->ArrivalStage=-1;HospitalStory->Seated31=false;}
        Screen.Empty();TitleView=false;CampaignStarted=true;PC->SetPause(false);PC->bShowMouseCursor=false;PC->SetInputMode(FInputModeGameOnly());
        H->SetActorHiddenInGame(false);H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);PC->SetViewTarget(H);
        if(!Placed)
        {
            Placed=true;Start=FVector(4900,2800,98);PhotoView=FRotator(0,90,0);FCollisionQueryParams Q(SCENE_QUERY_STAT(FirstPersonPhoto50),false,H);if(H->BekaVisual)Q.AddIgnoredActor(H->BekaVisual->GetChildActor());
            for(float X:{4800.f,4900.f})for(float Y:{2800.f,5000.f,7400.f})
            {
                const FVector At(X,Y,98);FHitResult Hit;
                if(GetWorld()->SweepSingleByChannel(Hit,At,At+FVector(.1,0,0),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,94),Q))continue;
                for(FVector Direction:{FVector(0,1,0),FVector(1,0,0),FVector(0,-1,0),FVector(-1,0,0)})
                {
                    const FVector Eye=At+FVector(0,0,68);const bool Block=GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Direction*5000,ECC_Visibility,Q);const float Sight=Block?Hit.Distance:5000;
                    if(Sight>RouteSight){RouteSight=Sight;Start=At;PhotoView=Direction.Rotation();}
                }
            }
            UE_LOG(LogTemp,Display,TEXT("FIRSTPERSON50 capture at %s view %s sight %.1fcm"),*Start.ToString(),*PhotoView.ToString(),RouteSight);
            H->SetActorLocation(Start);PC->SetControlRotation(PhotoView);H->SetActorRotation(FRotator(0,PhotoView.Yaw,0));if(BookWorld)BookWorld->SetBright(true);
        }
        if(!Warmed)
        {
            Warmed=true;auto* Settings=GEngine->GetGameUserSettings();Settings->SetScreenResolution(FIntPoint(1920,1080));Settings->SetResolutionScaleValueEx(100);Settings->ApplyResolutionSettings(false);
#if WITH_EDITOR
            FAssetCompilingManager::Get().FinishAllCompilation();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
#endif
            IStreamingManager::Get().StreamAllResources(5.f);
        }
        if(Clock<15)return;
        Check(TEXT("capture_has_at_least_five_metres_of_clear_view"),RouteSight>500);
        Check(TEXT("accepted_model_actor_is_instantiated"),H->BekaVisual&&H->BekaVisual->GetChildActor());
        Check(TEXT("accepted_body_face_and_outfit_are_present"),H->BekaBody&&H->BekaFace&&H->BekaOutfit);
        Check(TEXT("accepted_body_uses_live_campaign_retarget_driver"),H->BekaBody&&H->BekaBody->GetAnimInstance()&&H->BekaBody->GetAnimInstance()->IsA<UEWardBekaAnimation>());
        Check(TEXT("tailored_clothes_follow_accepted_body"),H->BekaOutfit&&H->BekaOutfit->LeaderPoseComponent.Get()==H->BekaBody);
        Check(TEXT("face_is_hidden_only_from_owner_view"),H->BekaFace&&H->BekaFace->bOwnerNoSee&&!H->BekaFace->bHiddenInGame);
        Check(TEXT("legacy_survivor_visual_is_hidden"),H->GetMesh()->bHiddenInGame);
        Check(TEXT("camera_has_zero_orbit_distance_and_no_lag"),H->Arm->TargetArmLength==0&&!H->Arm->bEnableCameraLag&&!H->Arm->bDoCollisionTest);
        Check(TEXT("standing_camera_is_at_164cm_eye_level"),FMath::IsNearlyEqual(H->Camera->GetComponentLocation().Z-(H->GetActorLocation().Z-H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),164.f,.5f));
        Next();
    }
    TickHero(Delta);H->UpdateFirstPerson50();
    if(BookWorld&&(Stage<4||Stage>=20)){BookWorld->Update(Delta);BookWorld->UpdateLighting(H->GetActorLocation());}
    if(Stage==1&&Clock>2)
    {
        Shot(TEXT("01_first_person"));Next();
    }
    else if(Stage==2)
    {
        Flag(TEXT("weapon"),true);H->Rounds=6;H->Reserve=12;H->Aiming=true;if(Clock<3)return;
        const FVector Mount=H->Camera->GetComponentTransform().InverseTransformPosition(H->WeaponPose().GetLocation());
        Check(TEXT("aimed_weapon_is_in_front_of_camera_and_centered"),Mount.X>20&&Mount.X<60&&FMath::Abs(Mount.Y)<2&&Mount.Z<0&&Mount.Z>-25);
        Shot(TEXT("02_first_person_aim"));Stage=20;Clock=0;
    }
    else if(Stage==20&&Clock>.4f)
    {
        Portrait=GetWorld()->SpawnActor<ACameraActor>();const FVector At=H->GetActorLocation()+PhotoView.RotateVector(FVector(260,40,45)),Target=H->GetActorLocation()+FVector(0,0,30);
        Portrait->SetActorLocationAndRotation(At,(Target-At).Rotation());Portrait->GetCameraComponent()->SetFieldOfView(45);PC->SetViewTarget(Portrait);Stage=21;Clock=0;
    }
    else if(Stage==21&&Clock>5)
    {
        Shot(TEXT("03_accepted_hero_in_game"));Stage=22;Clock=0;
    }
    else if(Stage==22&&Clock>.4f)
    {
        PC->SetViewTarget(H);H->Aiming=false;
        auto Box=[&](FVector At,FVector Extent)
        {
            auto* A=GetWorld()->SpawnActor<AActor>();auto* C=NewObject<UBoxComponent>(A);A->SetRootComponent(C);C->SetBoxExtent(Extent);C->SetCollisionProfileName(TEXT("BlockAll"));C->RegisterComponent();A->SetActorLocation(At);
        };
        Box(Fixture+FVector(0,0,-20),FVector(2000,2000,20));Box(Fixture+FVector(300,0,150),FVector(25,400,150));
        H->SetActorLocation(Fixture+FVector(0,0,96));H->GetCharacterMovement()->StopMovementImmediately();PC->SetControlRotation(FRotator::ZeroRotator);Stage=4;Clock=0;
    }
    else if(Stage==4&&Clock>.3f){WalkStart=H->GetActorLocation();StandingEye=H->Camera->GetComponentLocation().Z;Next();}
    else if(Stage==5)
    {
        H->Forward(1);
        if(Clock>.45f){Check(TEXT("forward_input_physically_moves_character"),H->GetActorLocation().X-WalkStart.X>60);Check(TEXT("body_follows_view_yaw_without_aim"),H->bUseControllerRotationYaw&&!H->GetCharacterMovement()->bOrientRotationToMovement);Next();}
    }
    else if(Stage==6)
    {
        H->Forward(1);
        if(Clock>1){Check(TEXT("capsule_stops_at_real_wall"),H->GetActorLocation().X>Fixture.X+220&&H->GetActorLocation().X<Fixture.X+242);Check(TEXT("eye_camera_stays_on_player_side_of_wall"),H->Camera->GetComponentLocation().X<Fixture.X+275);Next();}
    }
    else if(Stage==7)
    {
        H->Right(1);
        if(Clock>.45f){Check(TEXT("right_input_strafes_without_rotating_view"),H->GetActorLocation().Y-WalkStart.Y>60&&FMath::Abs(PC->GetControlRotation().Yaw)<1);H->Crouch();Next();}
    }
    else if(Stage==8&&Clock>.3f)
    {
        Check(TEXT("crouching_lowers_eye_camera"),H->bIsCrouched&&H->Camera->GetComponentLocation().Z<StandingEye-25);
        Check(TEXT("accepted_body_feet_follow_capsule_height"),H->BekaVisual&&FMath::IsNearlyEqual(H->BekaVisual->GetRelativeLocation().Z,-H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),.5f));
        H->UnCrouch();Next();
    }
    else if(Stage==9&&Clock>.3f)
    {
        H->Aiming=true;H->Rounds=6;H->Reserve=12;ShotCooldown=0;ReloadTime=0;BeforeShots=Shots;BeforeRounds=H->Rounds;FireWeapon();
        Check(TEXT("wall_blocks_barrel_without_spending_ammunition"),H->WeaponObstructed&&Shots==BeforeShots&&H->Rounds==BeforeRounds);
        H->SetActorLocation(Fixture+FVector(-100,100,96));Next();
    }
    else if(Stage==10&&Clock>.5f)
    {
        Check(TEXT("barrel_is_clear_after_stepping_back"),H->WeaponHasClearance());FireWeapon();
        Check(TEXT("clear_first_person_shot_spends_exactly_one_round"),Shots==BeforeShots+1&&H->Rounds==BeforeRounds-1);
        BeforeTotal=H->Rounds+H->Reserve;ReloadWeapon();Check(TEXT("reload_starts_in_first_person"),ReloadTime>0);Next();
    }
    else if(Stage==11&&Clock>2.1f)
    {
        Check(TEXT("reload_preserves_total_ammunition"),ReloadTime<=0&&H->Rounds==9&&H->Rounds+H->Reserve==BeforeTotal);
        Show(TEXT("pause"));Check(TEXT("pause_menu_opens"),PC->IsPaused()&&Screen==TEXT("pause"));Show(TEXT(""));H->UpdateFirstPerson50();
        Check(TEXT("resume_returns_to_first_person"),!PC->IsPaused()&&Screen.IsEmpty()&&PC->GetViewTarget()==H&&H->Arm->TargetArmLength==0);
        H->SetActorLocation(Start);H->GetCharacterMovement()->StopMovementImmediately();Next();
    }
    else if(Stage==12&&Clock>.25f)
    {
        bool Passed=true;for(auto C:Checks)Passed&=Boolean(C->AsObject(),TEXT("passed"));
        Json Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),Passed);Report->SetNumberField(TEXT("count"),Checks.Num());Report->SetStringField(TEXT("capture_position_cm"),Start.ToString());Report->SetBoolField(TEXT("captures_use_bright_inspection_mode"),true);Report->SetArrayField(TEXT("checks"),Checks);
        Report->SetStringField(TEXT("scope"),TEXT("Automated native movement/collision/weapon/menu checks with temporary fixtures; real in-game renders. Not a human playthrough."));
        Write(FPaths::ProjectSavedDir()/TEXT("Prototype/first_person50_check.json"),Report);FPlatformMisc::RequestExit(false);Next();
    }
}

