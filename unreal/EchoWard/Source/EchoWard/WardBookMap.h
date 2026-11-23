#pragma once
#include "CoreMinimal.h"
#include "WardCampaignData.h"
#include "Widgets/SWidget.h"

class AEWardCampaignMode;
struct FWardBookMapState
{
    FString Location=TEXT("all");
    double Zoom=1;
    FVector2D Pan=FVector2D::ZeroVector;
    int32 PaintedRooms=0,PaintedLocations=0;
    int32 PlayerMarkers66=0,GoalMarkers66=0;
    FString HoveredRoom;
    void Reset(){Zoom=1;Pan=FVector2D::ZeroVector;}
};
WardData::Json WardBookMapData();
TSharedRef<SWidget> MakeWardBookMap(AEWardCampaignMode* Mode,TSharedRef<FWardBookMapState> State,TFunction<void(const FString&)> Select);
