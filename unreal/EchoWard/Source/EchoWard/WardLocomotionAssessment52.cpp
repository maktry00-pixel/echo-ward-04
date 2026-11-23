#include "WardCampaign.h"
#include "WardHospitalGameplay.h"
#include "WardBookWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

using namespace WardData;

namespace
{
enum class EStage
{
    Start, Warmup, Accelerate, Walk, Brake, Idle, CrouchReady, CrouchWalk,
    SprintReady, Sprint, DirectionChange, Strafe, Teleport, Step,
    HospitalWarmup, LookDown, NoteReady, ReadWhileWalking, AimWhileWalking, Finish
};

struct FMotionAssessment
{
    EStage Stage = EStage::Start;
    float Clock = 0;
    int32 Steps = 0;
    FVector Mark = FVector::ZeroVector;
    double ViewPeak = 0;
    double CarryPeak = 0;
    double PreviousEyeHeight = 0;
    double EyeDeltaOnStep = 0;
    int32 GripSamples = 0;
    int32 GripVisible = 0;
    bool TurnLatched = false;
    bool AimEquipped = false;
    TArray<TSharedPtr<FJsonValue>> Checks;
    Json Values = MakeShared<FJsonObject>();

    void Next(EStage To)
    {
        Stage = To;
        Clock = 0;
    }

    void Check(const TCHAR* Name, bool Pass)
    {
        Json Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("name"), Name);
        Entry->SetBoolField(TEXT("passed"), Pass);
        Checks.Add(MakeShared<FJsonValueObject>(Entry));
        UE_LOG(LogTemp, Display, TEXT("LOCOMOTION52 %s %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), Name);
    }

    void Measure(const TCHAR* Name, double Value)
    {
        Values->SetNumberField(Name, Value);
    }
};

const FVector TestArea(80000, 80000, 0);

void AddTestSurface(UWorld& World, const FVector& Center, const FVector& HalfSize)
{
    auto* Actor = World.SpawnActor<AActor>();
    auto* Box = NewObject<UBoxComponent>(Actor);
    Actor->SetRootComponent(Box);
    Box->SetBoxExtent(HalfSize);
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Box->RegisterComponent();
    Actor->SetActorLocation(Center);
}

void CaptureMotionFrame(const TCHAR* Name)
{
    const FString Folder = FPaths::ProjectSavedDir() / TEXT("Prototype/Locomotion52");
    IFileManager::Get().MakeDirectory(*Folder, true);
    FScreenshotRequest::RequestScreenshot(Folder / Name, true, false);
}
}

void AEWardCampaignHero::TickLocomotionAssessment52(float Delta)
{
    auto* Campaign = WardCampaign(this);
    auto* Player = Cast<APlayerController>(Controller);
    if (!Campaign || !Player || !Campaign->BookWorld || !Campaign->HospitalStory)
        return;

    static FMotionAssessment Test;
    auto& Story = *Campaign->HospitalStory;
    auto* Move = GetCharacterMovement();
    const FVector At = GetActorLocation();
    const FVector Velocity = GetVelocity();
    const double Speed = Velocity.Size2D();
    if (Test.Stage >= EStage::HospitalWarmup)
        Campaign->TickHero(Delta);
    const auto EmptyHands = [&]
    {
        return BekaBody && BekaBody->IsBoneHiddenByName(TEXT("hand_l")) &&
            BekaBody->IsBoneHiddenByName(TEXT("hand_r"));
    };
    const auto TrackMotion = [&]
    {
        Test.ViewPeak = FMath::Max(Test.ViewPeak, ViewMotion52.Size());
        Test.CarryPeak = FMath::Max(Test.CarryPeak, WeaponMotion52.Size());
    };

    Test.Clock += Delta;
    switch (Test.Stage)
    {
    case EStage::Start:
        Campaign->CampaignStarted = true;
        Campaign->TitleView = false;
        Campaign->Flag(TEXT("hands_empty76"), true);
        Story.ArrivalStage = -1;
        Story.Seated31 = false;
        Story.FreezeAI = true;
        Campaign->Show(TEXT(""));
        Player->SetPause(false);
        Player->SetViewTarget(this);
        Player->SetControlRotation(FRotator::ZeroRotator);
        SetActorHiddenInGame(false);
        AddTestSurface(*GetWorld(), TestArea - FVector(0, 0, 20), FVector(3500, 3500, 20));
        SetActorLocation(TestArea + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
        Move->StopMovementImmediately();
        Move->SetMovementMode(MOVE_Walking);
        MotionInitialized52 = false;
        Test.Next(EStage::Warmup);
        break;

    case EStage::Warmup:
        if (Test.Clock < .7f) break;
        Test.Check(TEXT("separate_acceleration_and_braking"), Move->bUseSeparateBrakingFriction &&
            Move->BrakingFriction > 0.f && Move->MaxAcceleration < Move->BrakingDecelerationWalking);
        Test.Check(TEXT("empty_hands_hidden_at_rest"), EmptyHands());
        Test.Steps = Footsteps52;
        Test.Mark = At;
        Test.Next(EStage::Accelerate);
        break;

    case EStage::Accelerate:
        Forward(1.f);
        if (Test.Clock < .155f) break;
        Test.Measure(TEXT("start_speed_cm_s"), Speed);
        Test.Measure(TEXT("start_travel_cm"), FVector::Dist2D(At, Test.Mark));
        Test.Check(TEXT("starts_without_reaching_top_speed_instantly"), Speed > 25. && Speed < Move->GetMaxSpeed() * .92);
        Test.Next(EStage::Walk);
        break;

    case EStage::Walk:
        Forward(1.f);
        TrackMotion();
        if (Test.Clock < 1.95f) break;
        Test.Measure(TEXT("walk_speed_cm_s"), Speed);
        Test.Measure(TEXT("walk_contacts"), Footsteps52 - Test.Steps);
        Test.Check(TEXT("normal_walk_speed"), Speed >= 210. && Speed <= 240.);
        Test.Check(TEXT("contacts_follow_distance_travelled"), Footsteps52 - Test.Steps >= 4 && Footsteps52 - Test.Steps <= 9);
        Test.Check(TEXT("empty_hands_hidden_while_walking"), EmptyHands());
        Test.Mark = At;
        Test.Next(EStage::Brake);
        break;

    case EStage::Brake:
        if (Test.Clock < .65f) break;
        Test.Measure(TEXT("braking_travel_cm"), FVector::Dist2D(At, Test.Mark));
        Test.Check(TEXT("released_input_stops_without_snap_or_slide"), Speed < 2. &&
            FVector::Dist2D(At, Test.Mark) > 2. && FVector::Dist2D(At, Test.Mark) < 70.);
        Test.Steps = Footsteps52;
        Test.Next(EStage::Idle);
        break;

    case EStage::Idle:
        if (Test.Clock < .75f) break;
        Test.Check(TEXT("no_extra_footsteps_at_rest"), Footsteps52 == Test.Steps);
        Test.Check(TEXT("body_and_held_motion_settle_at_rest"), ViewMotion52.Size() < .45 && WeaponMotion52.Size() < .85);
        Crouch();
        Test.Next(EStage::CrouchReady);
        break;

    case EStage::CrouchReady:
        if (Test.Clock < .4f) break;
        Test.Check(TEXT("crouch_capsule_with_visible_body"), bIsCrouched && BekaBody && BekaOutfit &&
            FMath::IsNearlyEqual(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), 58.f, 1.f));
        Test.Steps = Footsteps52;
        Test.Next(EStage::CrouchWalk);
        break;

    case EStage::CrouchWalk:
        Forward(1.f);
        TrackMotion();
        if (Test.Clock < 1.15f) break;
        Test.Measure(TEXT("crouch_speed_cm_s"), Speed);
        Test.Check(TEXT("crouch_walk_remains_slow_with_real_contacts"), Speed >= 110. && Speed <= 137. && Footsteps52 > Test.Steps);
        UnCrouch();
        Test.Next(EStage::SprintReady);
        break;

    case EStage::SprintReady:
        if (Test.Clock < .5f) break;
        SprintHeld = true;
        Test.Steps = Footsteps52;
        Test.Next(EStage::Sprint);
        break;

    case EStage::Sprint:
        Forward(1.f);
        TrackMotion();
        if (Test.Clock < 1.45f) break;
        Test.Measure(TEXT("sprint_speed_cm_s"), Speed);
        Test.Check(TEXT("sprint_reaches_run_speed_on_ground"), Speed > 510. && Speed <= 545. && Move->IsMovingOnGround() && Footsteps52 > Test.Steps);
        Test.Measure(TEXT("maximum_camera_offset_cm"), Test.ViewPeak);
        Test.Measure(TEXT("maximum_held_offset_cm"), Test.CarryPeak);
        Test.Check(TEXT("camera_and_held_motion_stay_within_bounds"), Test.ViewPeak < 2.4 && Test.CarryPeak < 5.);
        SprintHeld = false;
        Test.Next(EStage::DirectionChange);
        break;

    case EStage::DirectionChange:
        Right(1.f);
        if (!Test.TurnLatched && Test.Clock > .08f)
        {
            Test.TurnLatched = true;
            Test.Check(TEXT("turn_preserves_momentum_during_weight_transfer"), Velocity.X > 30. && Velocity.Y < 215.);
        }
        if (Test.Clock < .9f) break;
        Test.Next(EStage::Strafe);
        break;

    case EStage::Strafe:
        Right(1.f);
        if (Test.Clock < .6f) break;
        Test.Measure(TEXT("lateral_speed_cm_s"), Velocity.Y);
        Test.Check(TEXT("lateral_motion_reaches_new_direction"), Velocity.Y > 140. && FMath::Abs(Velocity.X) < 12.);
        Move->StopMovementImmediately();
        Test.Steps = Footsteps52;
        SetActorLocation(TestArea + FVector(-1200, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
        Test.Next(EStage::Teleport);
        break;

    case EStage::Teleport:
        if (Test.Clock < .5f) break;
        Test.Check(TEXT("teleport_does_not_synthesize_footsteps"), Footsteps52 == Test.Steps);
        Test.Mark = At;
        AddTestSurface(*GetWorld(), FVector(At.X + 290, At.Y, 7.5), FVector(70, 150, 7.5));
        Test.PreviousEyeHeight = Camera->GetComponentLocation().Z;
        Test.Next(EStage::Step);
        break;

    case EStage::Step:
        Forward(1.f);
        Test.EyeDeltaOnStep = FMath::Max(Test.EyeDeltaOnStep, FMath::Abs(Camera->GetComponentLocation().Z - Test.PreviousEyeHeight));
        Test.PreviousEyeHeight = Camera->GetComponentLocation().Z;
        if (Test.Clock < 1.47f) break;
        Test.Measure(TEXT("step_gain_cm"), At.Z - Test.Mark.Z);
        Test.Measure(TEXT("step_peak_eye_change_per_frame_cm"), Test.EyeDeltaOnStep);
        Test.Check(TEXT("climbs_15_cm_step_without_jump"), At.X - Test.Mark.X > 225. &&
            At.Z - Test.Mark.Z > 10. && At.Z - Test.Mark.Z < 20. && Move->IsMovingOnGround());
        Move->StopMovementImmediately();
        SetActorLocation(FVector(4800, 2800, 98));
        Player->SetControlRotation(FRotator(0, 90, 0));
        SetActorRotation(FRotator(0, 90, 0));
        Campaign->BookWorld->SetBright(true);
        Test.Next(EStage::HospitalWarmup);
        break;

    case EStage::HospitalWarmup:
        Campaign->BookWorld->Update(Delta);
        Campaign->BookWorld->UpdateLighting(At);
        if (Test.Clock < 4.f) break;
        Player->SetControlRotation(FRotator(-78, 90, 0));
        Test.Next(EStage::LookDown);
        break;

    case EStage::LookDown:
        Forward(.50f);
        if (Test.Clock < .4f) break;
        Test.Check(TEXT("body_visible_and_empty_hands_hidden_looking_down"), EmptyHands() && BekaOutfit && !BekaOutfit->bHiddenInGame);
        CaptureMotionFrame(TEXT("01_look_down_walking.png"));
        Test.Next(EStage::NoteReady);
        break;

    case EStage::NoteReady:
        if (Test.Clock < .15f) break;
        Player->SetControlRotation(FRotator(0, 90, 0));
        Campaign->Journal.AddUnique(TEXT("@doc:NOTE_01_01"));
        Campaign->Completed.Add(TEXT("NOTE_01_01"));
        Test.Check(TEXT("collected_note_equips_on_normal_hero"), Campaign->HoldPreview51(TEXT("doc:NOTE_01_01")));
        Test.Next(EStage::ReadWhileWalking);
        break;

    case EStage::ReadWhileWalking:
        // A slow approach allows the item to settle while still moving via normal input.
        Forward(.40f);
        if (Test.Clock > .32f && Campaign->PreviewActor51 && BekaBody && Speed > 15.)
        {
            ++Test.GripSamples;
            const auto* Paper = Campaign->PreviewActor51.Get();
            if (!Paper->IsHidden() && !BekaBody->IsBoneHiddenByName(TEXT("hand_r")) &&
                BekaBody->IsBoneHiddenByName(TEXT("hand_l")) &&
                FVector::Dist(BekaBody->GetSocketLocation(TEXT("hand_r")), Paper->GetActorLocation()) < 21.)
                ++Test.GripVisible;
        }
        if (Test.Clock < .75f) break;
        Test.Measure(TEXT("held_note_moving_samples"), Test.GripSamples);
        Test.Measure(TEXT("held_note_grip_samples"), Test.GripVisible);
        Test.Check(TEXT("held_note_keeps_its_grip_while_walking"), Test.GripSamples > 1 &&
            Test.GripVisible >= Test.GripSamples * .75);
        CaptureMotionFrame(TEXT("02_held_note_walking.png"));
        Test.Next(EStage::AimWhileWalking);
        break;

    case EStage::AimWhileWalking:
        if (Test.Clock < .35f) break;
        if (!Test.AimEquipped)
        {
            Test.AimEquipped = true;
            Campaign->ClearPreview51();
            Campaign->Flag(TEXT("weapon"), true);
            Campaign->Flag(TEXT("hands_empty76"), false);
            Campaign->Shotgun = false;
            Aiming = true;
        }
        Forward(.55f);
        if (Test.Clock < 1.15f) break;
        Test.Check(TEXT("aimed_item_shows_both_hands"), Aiming && BekaBody &&
            !BekaBody->IsBoneHiddenByName(TEXT("hand_l")) && !BekaBody->IsBoneHiddenByName(TEXT("hand_r")));
        Test.Check(TEXT("pistol_remains_visible_while_aim_walking"), Speed > 15. && WeaponAimBlend > .7f &&
            Equipment.ContainsByPredicate([](const auto& Part) { return Part && Part->IsVisible(); }));
        CaptureMotionFrame(TEXT("03_pistol_aim_walking.png"));
        Test.Next(EStage::Finish);
        break;

    case EStage::Finish:
        if (Test.Clock < .5f) break;
        Json Report = MakeShared<FJsonObject>();
        Report->SetBoolField(TEXT("success"), !Test.Checks.ContainsByPredicate([](const auto& Check)
        {
            return !Boolean(Check->AsObject(), TEXT("passed"));
        }));
        Report->SetNumberField(TEXT("count"), Test.Checks.Num());
        Report->SetArrayField(TEXT("checks"), Test.Checks);
        Report->SetObjectField(TEXT("measurements"), Test.Values);
        Report->SetStringField(TEXT("scope"), TEXT("Scripted movement through normal hero input on a flat floor and 15 cm obstruction; actual map renders with empty hands, held note and aiming. No manual playthrough."));
        Write(FPaths::ProjectSavedDir() / TEXT("Prototype/locomotion52_check.json"), Report);
        FPlatformMisc::RequestExit(false);
        break;
    }
}
