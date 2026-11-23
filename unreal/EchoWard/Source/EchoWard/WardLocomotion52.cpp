#include "WardCampaign.h"
#include "WardAudio68.h"
#include "WardHospitalGameplay.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardHorror99.h"
#include "WardGait111.h"
#include "WardMotion107.h"
#include "WardMotion107Math.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
using namespace WardData;

namespace
{
struct FLocomotion
{
    WardMotion107::FFrame Pose;
    FVector PreviousVelocity = FVector::ZeroVector;
    FVector Acceleration = FVector::ZeroVector;
    float Respiration = 0;
    float Impact = 0;
    float EyeCorrection = 0;
    float NominalEye = 0;
    bool Crouched = false;
};
TMap<TWeakObjectPtr<AEWardCampaignHero>, FLocomotion> HeroMotion;
}

WardMotion107::FFrame WardMotion107::Frame(const AEWardCampaignHero& Hero)
{
    const auto* Motion = HeroMotion.Find(&Hero);
    return Motion ? Motion->Pose : FFrame{};
}

float WardGait111::EyeHeightCorrection(const AEWardCampaignHero& Hero)
{
    const auto* Motion=HeroMotion.Find(&Hero);
    return Motion?Motion->EyeCorrection:0.f;
}

void AEWardCampaignHero::UpdateLocomotion52(float Delta)
{
    auto* G=WardCampaign(this);if(!G)return;
    using namespace WardMotion107;
    const float DT = FMath::Clamp(Delta, 0.f, .10f);
    if (!MotionInitialized52)
        for (auto It = HeroMotion.CreateIterator(); It; ++It)
            if (!It.Key().IsValid()) It.RemoveCurrent();
    auto& Motion = HeroMotion.FindOrAdd(this);
    const FVector Position=GetActorLocation();const FRotator View=GetControlRotation();
    const bool Ground=GetCharacterMovement()->IsMovingOnGround();
    const bool Active=!Dead&&!G->Won&&G->CampaignStarted&&G->Screen.IsEmpty()&&(!G->Admin||G->AdminHands51)&&!G->HidingBed&&!(G->BookWorld&&G->BookWorld->FurnitureHiding)&&!(G->HospitalStory&&G->HospitalStory->ArrivalCinematic());
    const float HalfHeight=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float NominalEye=float(Position.Z)+(bIsCrouched?FMath::Min(44.f,HalfHeight-10.f):68.f);
    if(!MotionInitialized52)
    {
        Motion = {};
        Motion.PreviousVelocity=GetVelocity();
        Motion.NominalEye=NominalEye;
        Motion.Crouched=bIsCrouched;
        LastGroundPosition52=Position;LastView52=View;Grounded52=Ground;MotionInitialized52=true;
        GaitPhase52=0;Landing52=0;LastVertical52=0;
    }
    const float Distance=FVector::Dist2D(Position,LastGroundPosition52),Speed=GetVelocity().Size2D();
    const bool Discontinuity = Distance >= 100 || FMath::Abs(Position.Z-LastGroundPosition52.Z)>72;
    const bool Travel=Active&&Ground&&!Discontinuity&&Speed>8&&Distance>.02f;
    const float RunningWeight=WardGait111::RunBlend(Speed);
    const float WalkStride=FMath::Lerp(94.f,179.f,Ease(35.f,WardGait111::WalkSpeed,Speed));
    const float Stride=bIsCrouched?FMath::Lerp(78.f,106.f,Ease(40.f,WardGait111::CrouchSpeed,Speed)):
        FMath::Lerp(WalkStride,292.f,RunningWeight);
    if(Travel)
    {
        const auto Cycle = AdvanceCycle(Motion.Pose.Gait, Distance / Stride, 0.f, 2.f);
        Motion.Pose.Gait = Cycle.Phase;
        GaitPhase52 = Cycle.Phase * 2 * PI;
        StrideTravel52 = FMath::Fmod(Cycle.Phase, .5f) * Stride;
        if(Cycle.Contact)
        {
            ++Footsteps52;
            Motion.Impact = bIsCrouched ? .21f : FMath::GetMappedRangeValueClamped(
                FVector2f(160.f, 540.f), FVector2f(.42f, 1.f), Speed);
            FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(HeroFootstep52),false,this);
            FString Material;
            const FVector Foot = Position + GetActorRightVector() * (Footsteps52 % 2 ? -10.f : 10.f);
            if(GetWorld()->LineTraceSingleByChannel(Hit,Foot,Foot-FVector(0,0,130),ECC_Pawn,Q)&&Hit.GetComponent())
            {
                Material=Hit.GetComponent()->GetName();if(auto* M=Hit.GetComponent()->GetMaterial(0))Material+=M->GetName();
            }
            Material=Material.ToLower();const FString Region=G->BookWorld?G->BookWorld->LocationAt(Position):TEXT("");
            StepSurface52=Material.Contains(TEXT("metal"))||Region==TEXT("RF")||Region==TEXT("UT")?TEXT("metal"):Material.Contains(TEXT("glass"))?TEXT("glass"):Region.StartsWith(TEXT("PS"))||Region.StartsWith(TEXT("RH"))?TEXT("vinyl"):Region==TEXT("ST")?TEXT("exterior"):TEXT("tile");
            const bool Fast=Speed>380;const float Volume=bIsCrouched?.07f:Fast?.4f:.2f;
            FString Audio=StepSurface52==TEXT("exterior")?TEXT("STEP/asphalt"):TEXT("step_")+StepSurface52;
            if(Material.Contains(TEXT("grass"))||Material.Contains(TEXT("foliage")))Audio=TEXT("STEP/grass");
            else if(Material.Contains(TEXT("gravel")))Audio=TEXT("STEP/gravel");
            else if(Material.Contains(TEXT("wood")))Audio=TEXT("STEP/wood");
            else if(Material.Contains(TEXT("wet"))||Material.Contains(TEXT("water")))Audio=TEXT("STEP/tile_wet");
            else if(Material.Contains(TEXT("concrete")))Audio=TEXT("STEP/concrete");
            G->Cue(Audio,Hit.bBlockingHit?Hit.ImpactPoint:Position,Volume);
            WardHorror99::Footstep(*G,Position,bIsCrouched?90:Fast?1350:360);
        }
    }
    else if(!Active||!Ground||Discontinuity){StrideTravel52=0;}
    if(Active&&Ground&&!Grounded52&&!Discontinuity&&LastVertical52<-180){Landing52=FMath::Clamp(-LastVertical52/650.f,0.f,1.f);G->Cue(TEXT("heavy_step"),Position,.16f*Landing52);G->BroadcastNoise(Position,250+Landing52*450);}
    Landing52=Follow(Landing52,0.f,DT,9.f);
    Motion.Impact=Follow(Motion.Impact,0.f,DT,19.f);
    const float WalkWeight=FMath::Clamp(Speed/WardGait111::WalkSpeed,0.f,1.f);
    const float Weight=Travel?(bIsCrouched?WalkWeight*.82f:WalkWeight+.5f*RunningWeight):0.f;
    MotionWeight52=Follow(MotionWeight52,Weight,DT,Travel?9.f:14.f);
    const float Load = FMath::Max3(Ease(310.f, 535.f, Speed) * .74f,
        FMath::Clamp((75.f - Stamina) / 70.f, 0.f, 1.f), Health < 35.f ? .52f : 0.f);
    auto& Pose = Motion.Pose;
    Pose.Active=Active;
    Pose.Effort=Follow(Pose.Effort,Active?Load:0.f,DT,Load>Pose.Effort?2.6f:.48f);
    if (Active)
    {
        const auto Respiration=AdvanceCycle(Motion.Respiration,DT*FMath::Lerp(.235f,.68f,Pose.Effort),.48f,1.f);
        Motion.Respiration=Respiration.Phase;
        if (Respiration.Contact) ++Pose.Exhale;
    }
    const float Chest=Ease(0.f,.48f,Motion.Respiration)*(1.f-Ease(.48f,1.f,Motion.Respiration));
    Pose.Breath=Active?(Chest-.5f)*FMath::Lerp(.35f,1.f,Pose.Effort):0.f;
    const float Steady=Aiming?.18f:1.f;
    const float Swing=FMath::Cos(GaitPhase52);
    const float Rise=.5f-.5f*FMath::Cos(2.f*GaitPhase52);
    const float Bob=MotionWeight52;
    ViewMotion52=(FVector(-.24f*Motion.Impact,.39f*Swing*Bob,
        (.32f+.18f*RunningWeight)*(Rise-.5f)*Bob-.43f*Motion.Impact+Pose.Breath*.24f)*Steady+
        FVector(0,0,-Landing52*1.14f)).GetClampedToMaxSize(1.9f);
    const bool StanceChanged=bIsCrouched!=Motion.Crouched;
    const float ViewChange=NominalEye-Motion.NominalEye;
    if (!Active||!Ground||!Grounded52||Discontinuity)
        Motion.EyeCorrection=0.f;
    else if (StanceChanged||(Travel&&FMath::Abs(ViewChange)>2.f&&FMath::Abs(ViewChange)<=28.f))
        Motion.EyeCorrection=FMath::Clamp(Motion.EyeCorrection-ViewChange,-24.f,24.f);
    Motion.EyeCorrection=Follow(Motion.EyeCorrection,0.f,DT,13.f);
    Motion.Crouched=bIsCrouched;
    Motion.NominalEye=NominalEye;
    const float YawRate=Active&&Delta>.001f&&!Discontinuity?FMath::FindDeltaAngleDegrees(LastView52.Yaw,View.Yaw)/Delta:0;
    const float PitchRate=Active&&Delta>.001f&&!Discontinuity?FMath::FindDeltaAngleDegrees(LastView52.Pitch,View.Pitch)/Delta:0;
    Pose.Turn=Follow(Pose.Turn,FMath::Clamp(YawRate/260.f,-1.f,1.f),DT,8.f);
    const FRotator Desired=Active?FRotator(FMath::Clamp(-PitchRate*.005f,-1.1f,1.1f),FMath::Clamp(-YawRate*.004f,-1.4f,1.4f),FMath::Clamp(-YawRate*.002f,-.7f,.7f)):FRotator::ZeroRotator;
    WeaponSway52=FMath::RInterpTo(WeaponSway52,Desired,DT,10);
    const FVector Velocity=GetVelocity();
    const FVector Acceleration=(!Active||!Ground||Discontinuity||DT<=0)?FVector::ZeroVector:
        ((Velocity-Motion.PreviousVelocity)/FMath::Max(DT,1.f/120.f)).GetClampedToMaxSize(1550.f);
    Motion.PreviousVelocity=Velocity;
    Motion.Acceleration=FMath::Lerp(Motion.Acceleration,Acceleration,1.f-FMath::Exp(-16.f*DT));
    if (Discontinuity) Motion.Acceleration=FVector::ZeroVector;
    const FVector Inertia=GetActorTransform().InverseTransformVectorNoScale(Motion.Acceleration);
    const FVector Carry(-.24f*Bob-.00078f*Inertia.X,.53f*Swing*Bob-.00065f*Inertia.Y,
        .62f*(Rise-.5f)*Bob-.68f*Motion.Impact+Pose.Breath*.36f-.25f*Landing52);
    WeaponMotion52=FMath::Lerp(WeaponMotion52,Active?Carry:FVector::ZeroVector,1.f-FMath::Exp(-13.f*DT));
    if(!G->SettingBool(TEXT("camera_motion"),true)){ViewMotion52=FVector::ZeroVector;Motion.EyeCorrection=0.f;}
    if(G->ScareClock76>0&&G->SettingBool(TEXT("camera_motion"),true)){const float Shock=G->ScareClock76/1.8f;ViewMotion52+=FVector(0,FMath::Sin(G->ScareClock76*49)*.45f,FMath::Cos(G->ScareClock76*37)*.35f)*Shock;}
    LastGroundPosition52=Position;LastView52=View;Grounded52=Ground;LastVertical52=GetVelocity().Z;
}
