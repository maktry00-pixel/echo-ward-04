#include "WardBookWorld.h"
#include "WardCampaign.h"
#include "Misc/Paths.h"
using namespace WardData;

Json AEWardBookWorld::AuditEnvelope()const
{
    // Independently derive expected occupancy from the full cell map. Query the
    // real physics scene, including the storey band which room-centre tests miss.
    Json Result=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Missing;
    TMap<FString,int> Tested,Failed;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(HospitalEnvelope),false,Mode.IsValid()?Mode->Hero():nullptr);
    auto Probe=[&](const FString& Kind,FVector P)
    {
        ++Tested.FindOrAdd(Kind);
        if(GetWorld()->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(1),Q))return;
        ++Failed.FindOrAdd(Kind);
        if(Missing.Num()<160){Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("kind"),Kind);Row->SetStringField(TEXT("at_cm"),P.ToString());Missing.Add(MakeShared<FJsonValueObject>(Row));}
    };
    TMap<FIntVector,TArray<int>> Lines;
    for(int I=0;I<Portals.Num();++I){const auto& P=Portals[I];const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");Lines.FindOrAdd(FIntVector(FMath::RoundToInt(NS?P.At.Y:P.At.X),FMath::RoundToInt(P.At.Z),NS?1:0)).Add(I);}
    for(const auto& E:Cells)
    {
        const FIntVector K=E.Key;const auto& Cell=E.Value;const FVector At(K.X*100+50,K.Y*100+50,K.Z);
        if(!FloorHole(K.X,K.Y,K.Z))Probe(TEXT("floors"),At+FVector(0,0,-3));
        if(!Cell.Outdoor&&!FloorHole(K.X,K.Y,K.Z+480))Probe(TEXT("ceilings"),At+FVector(0,0,388));
        for(FIntVector D:{FIntVector(1,0,0),FIntVector(-1,0,0),FIntVector(0,1,0),FIntVector(0,-1,0)})
        {
            const auto* Other=Cells.Find(K+D);
            if(Cell.Outdoor||Other&&!Other->Outdoor)continue;
            const FVector Mid=At+FVector(D.X*50,D.Y*50,0),Along=D.Y?FVector(1,0,0):FVector(0,1,0);
            Probe(TEXT("storey_bands"),Mid+FVector(0,0,430));
            const auto* Candidates=Lines.Find(FIntVector(FMath::RoundToInt(D.Y?Mid.Y:Mid.X),K.Z,D.Y?1:0));
            for(float Offset:{-38.f,0.f,38.f})
            {
                const FVector P=Mid+Along*Offset;bool Opening=false;
                if(Candidates)for(int I:*Candidates){const auto& Door=Portals[I];if(FMath::Abs(D.Y?P.X-Door.At.X:P.Y-Door.At.Y)<Door.Width*.5+1){Opening=true;break;}}
                if(!Opening)for(float Z:{65.f,200.f,350.f})Probe(TEXT("outer_walls"),P+FVector(0,0,Z));
            }
        }
    }
    Probe(TEXT("shaft_roof"),FVector(5000,700,1828));
    bool OK=true;Json Counts=MakeShared<FJsonObject>(),Failures=MakeShared<FJsonObject>();
    for(const auto& E:Tested){Counts->SetNumberField(E.Key,E.Value);Failures->SetNumberField(E.Key,Failed.FindRef(E.Key));OK&=Failed.FindRef(E.Key)==0;}
    Result->SetBoolField(TEXT("success"),OK);Result->SetObjectField(TEXT("tested"),Counts);Result->SetObjectField(TEXT("failed"),Failures);Result->SetArrayField(TEXT("missing"),Missing);
    Write(FPaths::ProjectSavedDir()/TEXT("Prototype/envelope_check.json"),Result);return Result;
}
