#pragma once
#include "CoreMinimal.h"
#include "WardCampaignData.h"

enum class EWardAITraceKind : uint8
{
    Expand, Queue, PathFound, PathBlocked,
    ChooseItem, RejectLocation, PlaceItem, Backtrack, SuppliesSolved, SuppliesImpossible,
    ScoreMove, ChooseMove
};

struct FWardAITraceEvent
{
    EWardAITraceKind Kind;
    FIntPoint Cell = FIntPoint::ZeroValue;
    FIntPoint Other = FIntPoint::ZeroValue;
    FString Item;
    // A*: Count = g, Estimate = h. CSP: remaining candidates, or the minimum distance
    // for a rejection. Minimax: Count = visited nodes, Estimate = cutoffs.
    int32 Count = 0;
    int32 Estimate = 0;
    float Value = 0.f; // CSP rejection distance in cm or a Minimax score.
    bool Bound = false; // The scored move may be worse than this upper bound.
};

struct FWardCampaignGrid
{
    TMap<FIntPoint,float> Ground;
    TSet<FIntPoint> Blocked;
    int32 Expanded=0, TacticalNodes=0, TacticalPrunes=0, Assignments=0, Backtracks=0, ConstraintChecks=0;
    void Load(WardData::Json Data);
    FIntPoint Cell(FVector V)const;
    FVector Point(FIntPoint C)const;
    FIntPoint Nearest(FVector V)const;
    TArray<FIntPoint> Neighbors(FIntPoint C,bool Wait=false)const;
    TArray<FVector> FindPath(FVector From,FVector To);
    TArray<FVector> FindPath(FVector From,FVector To,TArray<FWardAITraceEvent>* Trace);
    FIntPoint Tactical(FIntPoint Hunter,FIntPoint Seen,int32 Pairs=2,bool Pruning=true);
    FIntPoint Tactical(FIntPoint Hunter,FIntPoint Seen,int32 Pairs,bool Pruning,TArray<FWardAITraceEvent>* Trace);
    bool Allocate(const TMap<FString,TArray<FVector>>& Domains,TMap<FString,FVector>& Result,float Separation=300);
    bool Allocate(const TMap<FString,TArray<FVector>>& Domains,TMap<FString,FVector>& Result,float Separation,TArray<FWardAITraceEvent>* Trace);
private:
    float Search(FIntPoint H,FIntPoint P,int32 Depth,bool Max,float Alpha,float Beta,bool Pruning);
};
