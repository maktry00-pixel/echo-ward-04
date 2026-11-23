#pragma once

#include "CoreMinimal.h"

namespace WardPistolPose
{
constexpr float ReloadDuration=1.8f;
constexpr float ReleaseTime=.18f;
constexpr float SeatTime=1.18f;
constexpr float SlideTime=1.58f;

struct FReloadPose
{
    float GripBlend=0;
    float FingerOpen=0;
    float MagazineDrop=0;
    float WristTurn=0;
    float SlidePull=0;
    float SlideGrip=0;
};

inline FReloadPose Reload(float Remaining)
{
    if(Remaining<=0)return {};
    const float Elapsed=FMath::Clamp(ReloadDuration-Remaining,0.f,ReloadDuration);
    FReloadPose Pose;
    Pose.GripBlend=FMath::SmoothStep(.02f,ReleaseTime,Elapsed)*(1-FMath::SmoothStep(SeatTime+.08f,1.55f,Elapsed));
    Pose.FingerOpen=.66f*FMath::SmoothStep(.02f,.10f,Elapsed)*(1-FMath::SmoothStep(.11f,ReleaseTime,Elapsed));
    Pose.MagazineDrop=13.f*FMath::SmoothStep(.12f,.38f,Elapsed)*(1-FMath::SmoothStep(.83f,SeatTime,Elapsed));
    Pose.WristTurn=FMath::SmoothStep(.05f,.36f,Elapsed)*(1-FMath::SmoothStep(SeatTime,ReloadDuration,Elapsed));
    Pose.SlidePull=1.45f*FMath::SmoothStep(1.37f,1.51f,Elapsed)*(1-FMath::SmoothStep(SlideTime,1.68f,Elapsed));
    Pose.SlideGrip=FMath::SmoothStep(1.30f,1.47f,Elapsed)*(1-FMath::SmoothStep(1.63f,ReloadDuration,Elapsed));
    return Pose;
}

inline FVector Mount(bool LongGun,float Aim)
{
    return FMath::Lerp(LongGun?FVector(34,6,2):FVector(32,6,2),
        LongGun?FVector(36,0,-.5):FVector(34,0,-.5),Aim);
}

inline float SprintLowering(float GaitWeight)
{
    return FMath::SmoothStep(1.06f,1.44f,GaitWeight);
}
}
