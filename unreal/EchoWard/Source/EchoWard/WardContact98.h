#pragma once

#include "CoreMinimal.h"

class AEWardBookWorld;
class AEWardCampaignHero;
class AActor;

namespace WardContact98
{
enum class EGrip : uint8 { Door, Cart, Key };

struct FPose
{
    EGrip Grip = EGrip::Door;
    FVector Right = FVector::ZeroVector;
    FVector Left = FVector::ZeroVector;
    FQuat Rotation = FQuat::Identity;
    float Reach = 0;
    float GripAmount = 0;
};

FPose Pose(const AEWardCampaignHero& Hero);
void BeginDoor(AEWardBookWorld& World, int32 Index, const FVector& Player, bool Local, bool Open);
float DoorOpenAmount(const AEWardBookWorld& World, const FVector& At, bool Open);
void KeyPickup(AEWardCampaignHero& Hero, AActor& Item);
int32 AimedCart(AEWardBookWorld& World, AEWardCampaignHero& Hero);
bool PushCart(AEWardBookWorld& World, int32 Index, const FVector& Player, bool Pull, bool Local);
void ClearMotion(AEWardBookWorld& World);
void Advance(AEWardBookWorld& World, float Delta);
}
