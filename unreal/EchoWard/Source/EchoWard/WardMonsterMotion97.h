#pragma once

#include "CoreMinimal.h"

namespace WardMonsterMotion97
{
struct FStyle
{
    const TCHAR* Species;
    float TurnRate;
    float ShoulderLead;
    float GaitPitch;
    float Sway;
    float Load;
    bool Pounce = false;
};

inline const FStyle& Style(const FString& Species)
{
    static constexpr FStyle Styles[] = {
        {TEXT("patients"), 150, .47f, 3.5f, .013f, .10f},
        {TEXT("hound"), 250, .73f, 11.f, .010f, .12f, true},
        {TEXT("carrier"), 130, .48f, 4.3f, .022f, .76f},
        {TEXT("repeater"), 124, .67f, -2.f, .007f, .12f},
        {TEXT("transporter"), 121, .43f, 7.f, .030f, .78f},
        {TEXT("collector"), 178, .61f, 5.4f, .016f, .40f},
        {TEXT("stitched"), 117, .43f, 5.f, .024f, .87f},
        {TEXT("blind_surgeon"), 155, .71f, 2.5f, .011f, .48f},
        {TEXT("linked"), 111, .64f, 1.2f, .005f, .21f},
        {TEXT("armoured"), 105, .37f, 4.1f, .018f, 1.f},
        {TEXT("conductor"), 140, .67f, -.8f, .009f, .36f},
        {TEXT("listener"), 165, .82f, .4f, .006f, .18f},
        {TEXT("keeper"), 161, .49f, 3.8f, .014f, .59f},
        {TEXT("impostor"), 180, .52f, -.2f, .008f, .14f},
        {TEXT("pursuer"), 264, .68f, 8.f, .011f, .28f, true},
        {TEXT("parasite"), 284, .74f, 2.2f, .009f, 0.f},
        {TEXT("nest"), 0, .29f, 0.f, .023f, 0.f},
        {TEXT("central_organism"), 0, .28f, 0.f, .035f, 0.f},
    };
    for (const auto& Entry : Styles)
        if (Species == Entry.Species) return Entry;
    return Styles[0];
}

inline float Preload(float Progress)
{
    return FMath::SmoothStep(0.f, .69f, Progress) *
        (1.f - .38f * FMath::SmoothStep(.76f, 1.f, Progress));
}

inline float PounceHeight(float RecoverySeconds)
{
    const float Phase = FMath::Clamp(RecoverySeconds / .26f, 0.f, 1.f);
    return FMath::Sin(PI * Phase) * 23.f;
}

inline FVector CornerFacing(const FVector& Direction, const FVector& Next, float Distance)
{
    const FVector Forward = Direction.GetSafeNormal2D();
    if (Next.IsNearlyZero() || FVector::DotProduct(Forward, Next.GetSafeNormal2D()) < -.45f) return Forward;
    return FMath::Lerp(Forward, Next.GetSafeNormal2D(), .63f * (1.f - FMath::SmoothStep(28.f, 115.f, Distance))).GetSafeNormal2D();
}
}
