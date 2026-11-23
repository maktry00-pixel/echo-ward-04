#include "WardCampaignAI.h"
#include <queue>
#include "Algo/Reverse.h"
using namespace WardData;

namespace
{
constexpr int32 MaxTraceEvents = 512;

FORCEINLINE bool CanRecord(const TArray<FWardAITraceEvent>* Trace)
{
    // Keep room for the outcome, even when a large grid fills the trace.
    return Trace && Trace->Num() < MaxTraceEvents - 1;
}
}

void FWardCampaignGrid::Load(Json Data)
{
    Ground.Empty();Blocked.Empty();
    for(const auto& Value:Array(Data,TEXT("navigation_cells"))){FVector V=Vector(Value->AsArray());Ground.Add(Cell(V),V.Z);}
}
FIntPoint FWardCampaignGrid::Cell(FVector V)const{return FIntPoint(FMath::RoundToInt(V.X/100),FMath::RoundToInt(V.Y/100));}
FVector FWardCampaignGrid::Point(FIntPoint C)const{const float* Z=Ground.Find(C);return FVector(C.X*100,C.Y*100,(Z?*Z:0)+96);}
FIntPoint FWardCampaignGrid::Nearest(FVector V)const
{
    FIntPoint C=Cell(V);if(Ground.Contains(C)&&!Blocked.Contains(C))return C;
    double Best=MAX_dbl;FIntPoint Result=C;
    for(const auto& Entry:Ground)
    {
        if(Blocked.Contains(Entry.Key))continue;
        const double D=FVector::DistSquared(Point(Entry.Key),V);
        if(D<Best){Best=D;Result=Entry.Key;}
    }
    return Result;
}
TArray<FIntPoint> FWardCampaignGrid::Neighbors(FIntPoint C,bool Wait)const
{
    TArray<FIntPoint> Out;const FIntPoint Steps[]={{0,1},{1,0},{0,-1},{-1,0}};
    for(FIntPoint Step:Steps)if(Ground.Contains(C+Step)&&!Blocked.Contains(C+Step))Out.Add(C+Step);
    if(Wait)Out.Add(C);return Out;
}
TArray<FVector> FWardCampaignGrid::FindPath(FVector From,FVector To)
{
    return FindPath(From,To,nullptr);
}

TArray<FVector> FWardCampaignGrid::FindPath(FVector From,FVector To,TArray<FWardAITraceEvent>* Trace)
{
    if (Trace) Trace->Reset();
    Expanded=0;TArray<FVector> Path;
    if (Ground.IsEmpty())
    {
        if (Trace) Trace->Add({EWardAITraceKind::PathBlocked,Cell(From),Cell(To)});
        return Path;
    }
    const FIntPoint Start=Nearest(From),Goal=Nearest(To);
    auto H=[Goal](FIntPoint P){return FMath::Abs(P.X-Goal.X)+FMath::Abs(P.Y-Goal.Y);};
    struct FEntry{FIntPoint Cell;int32 Cost,Priority,Order;bool operator<(const FEntry& Other)const{return Priority==Other.Priority?Order>Other.Order:Priority>Other.Priority;}};
    std::priority_queue<FEntry> Open;TMap<FIntPoint,int32> Cost;TMap<FIntPoint,FIntPoint> Previous;TSet<FIntPoint> Closed;
    const int32 InitialEstimate=H(Start);
    int32 Order=0;Open.push({Start,0,InitialEstimate,Order++});Cost.Add(Start,0);
    if (CanRecord(Trace)) Trace->Add({EWardAITraceKind::Queue,Start,Start,{},0,InitialEstimate});
    while(!Open.empty() && Expanded<Ground.Num())
    {
        FEntry Entry=Open.top();Open.pop();FIntPoint Current=Entry.Cell;
        if(Closed.Contains(Current))continue;++Expanded;
        if (CanRecord(Trace)) Trace->Add({EWardAITraceKind::Expand,Current,Goal,{},Entry.Cost,Entry.Priority-Entry.Cost});
        if(Current==Goal)
        {
            while(Current!=Start)
            {
                Path.Add(Point(Current));
                const FIntPoint* P=Previous.Find(Current);
                if (!P)
                {
                    if (Trace) Trace->Add({EWardAITraceKind::PathBlocked,Start,Goal,{},Expanded});
                    return {};
                }
                Current=*P;
            }
            Algo::Reverse(Path);
            if (Trace) Trace->Add({EWardAITraceKind::PathFound,Goal,Start,{},Path.Num(),Expanded});
            return Path;
        }
        Closed.Add(Current);
        for(FIntPoint Next:Neighbors(Current))
        {
            if(Closed.Contains(Next))continue;
            const int32 New=Cost[Current]+1;const int32* Old=Cost.Find(Next);
            if(!Old||New<*Old)
            {
                const int32 Estimate=H(Next);
                Cost.Add(Next,New);
                Previous.Add(Next,Current);
                Open.push({Next,New,New+Estimate,Order++});
                if (CanRecord(Trace)) Trace->Add({EWardAITraceKind::Queue,Next,Current,{},New,Estimate});
            }
        }
    }
    if (Trace) Trace->Add({EWardAITraceKind::PathBlocked,Start,Goal,{},Expanded});
    return Path;
}
float FWardCampaignGrid::Search(FIntPoint H,FIntPoint P,int32 Depth,bool Max,float Alpha,float Beta,bool Pruning)
{
    ++TacticalNodes;if(H==P)return 100000.f+Depth;
    if(Depth<=0)return -(FMath::Abs(H.X-P.X)+FMath::Abs(H.Y-P.Y))*10.f-Neighbors(P).Num()*2;
    float Best=Max?-MAX_flt:MAX_flt;
    for(FIntPoint Candidate:Neighbors(Max?H:P,true))
    {
        const float Score=Search(Max?Candidate:H,Max?P:Candidate,Depth-1,!Max,Alpha,Beta,Pruning);
        if(Max){Best=FMath::Max(Best,Score);Alpha=FMath::Max(Alpha,Best);}else{Best=FMath::Min(Best,Score);Beta=FMath::Min(Beta,Best);}
        if(Pruning&&Beta<=Alpha){++TacticalPrunes;break;}
    }
    return Best;
}
FIntPoint FWardCampaignGrid::Tactical(FIntPoint Hunter,FIntPoint Seen,int32 Pairs,bool Pruning)
{
    return Tactical(Hunter,Seen,Pairs,Pruning,nullptr);
}

FIntPoint FWardCampaignGrid::Tactical(FIntPoint Hunter,FIntPoint Seen,int32 Pairs,bool Pruning,TArray<FWardAITraceEvent>* Trace)
{
    if (Trace) Trace->Reset();
    TacticalNodes=0;TacticalPrunes=0;FIntPoint Action=Hunter;float Score=-MAX_flt,Alpha=-MAX_flt;
    for(FIntPoint Candidate:Neighbors(Hunter,true))
    {
        const int32 NodesBefore=TacticalNodes, PrunesBefore=TacticalPrunes;
        const float Value=Search(Candidate,Seen,FMath::Clamp(Pairs,1,3)*2-1,false,Alpha,MAX_flt,Pruning);
        if (CanRecord(Trace))
        {
            const int32 Cutoffs=TacticalPrunes-PrunesBefore;
            // A move that fails low after pruning may give only an upper bound.
            const bool UpperBound=Cutoffs>0 && Value<=Alpha;
            Trace->Add({EWardAITraceKind::ScoreMove,Candidate,Seen,{},TacticalNodes-NodesBefore,Cutoffs,Value,UpperBound});
        }
        if(Value>Score){Score=Value;Action=Candidate;}Alpha=FMath::Max(Alpha,Score);
    }
    if (Trace) Trace->Add({EWardAITraceKind::ChooseMove,Action,Seen,{},TacticalNodes,TacticalPrunes,Score});
    return Action;
}
bool FWardCampaignGrid::Allocate(const TMap<FString,TArray<FVector>>& Domains,TMap<FString,FVector>& Result,float Separation)
{
    return Allocate(Domains,Result,Separation,nullptr);
}

bool FWardCampaignGrid::Allocate(const TMap<FString,TArray<FVector>>& Domains,TMap<FString,FVector>& Result,float Separation,TArray<FWardAITraceEvent>* Trace)
{
    if (Trace) Trace->Reset();
    Assignments=0;Backtracks=0;ConstraintChecks=0;Result.Empty();
    TFunction<bool()> Recurse=[&]()
    {
        if(Result.Num()==Domains.Num())return true;
        FString Selected;TArray<FVector> Remaining;int32 Count=MAX_int32;
        // MRV and forward checking are shared with the course implementation.
        TArray<FString> Keys;Domains.GetKeys(Keys);Keys.Sort();
        for(const FString& Key:Keys)
        {
            if(Result.Contains(Key))continue;TArray<FVector> Legal;
            for(FVector Candidate:Domains[Key])
            {
                bool OK=true;
                for(const auto& Entry:Result)
                {
                    ++ConstraintChecks;
                    const double Distance=FVector::Dist(Candidate,Entry.Value);
                    if(Distance<Separation)
                    {
                        if (CanRecord(Trace))
                            Trace->Add({EWardAITraceKind::RejectLocation,Cell(Candidate),Cell(Entry.Value),Key,FMath::RoundToInt(Separation),0,float(Distance)});
                        OK=false;
                        break;
                    }
                }
                if(OK)Legal.Add(Candidate);
            }
            if(Legal.IsEmpty())
            {
                if (CanRecord(Trace)) Trace->Add({EWardAITraceKind::ChooseItem,{},{},Key,0});
                return false;
            }
            if(Legal.Num()<Count){Selected=Key;Remaining=Legal;Count=Legal.Num();}
        }
        if (CanRecord(Trace)) Trace->Add({EWardAITraceKind::ChooseItem,{},{},Selected,Remaining.Num()});
        for(FVector Candidate:Remaining)
        {
            ++Assignments;
            Result.Add(Selected,Candidate);
            if (CanRecord(Trace)) Trace->Add({EWardAITraceKind::PlaceItem,Cell(Candidate),{},Selected});
            if(Recurse())return true;
            Result.Remove(Selected);
            ++Backtracks;
            if (CanRecord(Trace)) Trace->Add({EWardAITraceKind::Backtrack,Cell(Candidate),{},Selected});
        }
        return false;
    };
    const bool Solved=Recurse();
    if (Trace) Trace->Add({Solved?EWardAITraceKind::SuppliesSolved:EWardAITraceKind::SuppliesImpossible,{},{},{},Assignments,Backtracks});
    if(Solved)return true;Result.Empty();return false;
}
