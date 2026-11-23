#pragma once

#include "CoreMinimal.h"

class AEWardCampaignHero;

namespace WardGait111
{
constexpr float WalkSpeed = 225.f;
constexpr float CrouchSpeed = 125.f;
constexpr float AimSpeed = 145.f;
constexpr float SprintSpeed = 540.f;
constexpr float Acceleration = 920.f;
constexpr float Braking = 1150.f;

inline float RunBlend(float Speed)
{
    return FMath::SmoothStep(WalkSpeed + 30.f, SprintSpeed - 45.f, Speed);
}

float EyeHeightCorrection(const AEWardCampaignHero& Hero);
}
