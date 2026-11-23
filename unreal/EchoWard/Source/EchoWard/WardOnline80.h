#pragma once

#include "CoreMinimal.h"

class AEWardCampaignMode;
class AEWardHospitalGameplay;
class AEWardHospitalMob;
class AEWardBookWorld;
class SWidget;

namespace WardOnline80
{
    bool IsPage(const FString& Page);
    bool PlayingMap(AEWardCampaignMode& Game);
    bool Reconnecting(AEWardCampaignMode& Game);
    bool SaveCheckpoint(AEWardCampaignMode& Game, bool Manual);
    bool Guest(AEWardCampaignMode& Game);
    bool Interact(AEWardCampaignMode& Game);
    bool SelectLift(AEWardCampaignMode& Game, int32 Direction);
    FVector RespawnPoint(AEWardCampaignMode& Game);
    bool PersonalAction(AEWardCampaignMode& Game, const TCHAR* Action);
    FString StoryTarget(const AEWardHospitalGameplay& Story);
    void ApplyDialogue(AEWardHospitalGameplay& Story, const FString& Line);
    bool OperateDoor(AEWardBookWorld& World, int32 Index, FVector Player, bool Open);
    bool DoorwayOccupied(AEWardCampaignMode& Game, FVector Door, bool NorthSouth, float Width);
    int32 PlayersOutsideEntrance(AEWardCampaignMode& Game);
    FVector MobPlayer(AEWardHospitalMob& Mob, bool Select);
    bool MobTargetsGuest(AEWardHospitalMob& Mob);
    bool DamageGuest(AEWardHospitalMob& Mob, float Damage);
    double PartyDistanceSquared(AEWardCampaignMode& Game, FVector At);
    float RecoveryScale(AEWardCampaignMode& Game);
    int32 SupplyAmount(AEWardCampaignMode& Game, const FString& Id, int32 Amount);
    void RecordShot(AEWardCampaignMode& Game, FVector Origin);
    bool ParallelPickup(AEWardCampaignMode& Game, const FString& Id);
    bool Command(AEWardCampaignMode& Game, const FString& Action);
    void Shutdown(AEWardCampaignMode& Game);
    void StartAssessment(AEWardCampaignMode& Game);
    TSharedRef<SWidget> Build(AEWardCampaignMode& Game);
}
