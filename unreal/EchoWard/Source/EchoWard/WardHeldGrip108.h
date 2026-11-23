#pragma once

#include "CoreMinimal.h"

class AActor;
class AEWardCampaignHero;

namespace WardHeldGrip108
{
enum class EGrip : uint8 { Wrap, Pinch, Page, Handle, Cradle, Phone, Notebook };

struct FProfile
{
    EGrip Grip = EGrip::Wrap;
    FVector Half = FVector(3, 4, 8);
    FVector Rest = FVector(35, 11, -16);
    FVector Angles = FVector::ZeroVector;
    float Weight = .28f;
    bool TwoHands = false;
};

struct FHand
{
    FVector Wrist;
    FVector Forward;
    FVector Across;
};

FProfile Classify(const FString& Id, const FString& Kind, FVector Half);
FHand Anchor(const FProfile& Profile, bool Left);
FProfile Preview(const AActor* Actor);
FProfile Phone();
bool Place(const AEWardCampaignHero& Hero, AActor& Actor, const FProfile& Profile, float Raise=1.f);
}
