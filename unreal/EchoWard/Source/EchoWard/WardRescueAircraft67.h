#pragma once
#include "WardVisualGeometry67.h"

namespace WardVisual67
{
inline UStaticMesh* RescueAircraft(UObject* Owner)
{
    static TWeakObjectPtr<UStaticMesh> Cached;
    if(Cached.IsValid())return Cached.Get();
    TArray<FSection> S;
    S.SetNum(6);
    struct FStation { double X,Width,Mid,Height; };
    const FStation Stations[]={
        {-239,26,144,37},{-207,69,143,70},{-138,96,148,91},{-35,104,150,94},
        {55,103,148,93},{127,94,143,85},{179,80,137,71},{222,51,130,46},{241,15,124,24}
    };
    constexpr int32 Sides=32;
    auto Point=[](const FStation& Station,int32 I,double Offset=0.)
    {
        const double A=I*2*PI/Sides;
        return FVector(Station.X,FMath::Cos(A)*(Station.Width+Offset),
                       Station.Mid+FMath::Sin(A)*(Station.Height+Offset));
    };
    auto SurfaceNormal=[&](int32 Row,int32 Side)
    {
        const FVector Around=Point(Stations[Row],Side+1)-Point(Stations[Row],Side-1);
        const FVector Along=Point(Stations[FMath::Min<int32>(Row+1,UE_ARRAY_COUNT(Stations)-1)],Side)-
                            Point(Stations[FMath::Max(Row-1,0)],Side);
        return FVector::CrossProduct(Around,Along).GetSafeNormal();
    };
    for(int32 Row=0;Row<UE_ARRAY_COUNT(Stations)-1;++Row)
    {
        for(int32 Side=0;Side<Sides;++Side)
        {
            const bool UpperSide=(Side>=1&&Side<=6)||(Side>=9&&Side<=14);
            const bool FrontWindow=Row>=5&&Row<=6&&Side>=1&&Side<=14;
            const bool SideWindow=Row>=2&&Row<=4&&UpperSide&&Side!=6&&Side!=9;
            const int32 Material=FrontWindow||SideWindow?1:Side==0||Side==15?2:Side>=19&&Side<=27?3:0;
            const FVector A=Point(Stations[Row],Side),B=Point(Stations[Row],Side+1);
            const FVector C=Point(Stations[Row+1],Side+1),D=Point(Stations[Row+1],Side);
            const FVector NA=SurfaceNormal(Row,Side),NB=SurfaceNormal(Row,Side+1);
            const FVector NC=SurfaceNormal(Row+1,Side+1),ND=SurfaceNormal(Row+1,Side);
            S[Material].Triangle(A,B,C,NA,NB,NC);
            S[Material].Triangle(A,C,D,NA,NC,ND);
        }
    }
    for(int32 Side=0;Side<Sides;++Side)
    {
        const FStation& First=Stations[0];
        const FStation& Last=Stations[UE_ARRAY_COUNT(Stations)-1];
        S[0].Triangle(FVector(First.X,0,First.Mid),Point(First,Side+1),Point(First,Side));
        S[0].Triangle(FVector(Last.X,0,Last.Mid),Point(Last,Side),Point(Last,Side+1));
    }
    for(int32 Side:{1,5,10,14})
    {
        for(int32 I=2;I<7;++I)
            S[3].Tube(Point(Stations[I],Side,.6),Point(Stations[I+1],Side,.6),1.2,1.2,6);
    }
    for(int32 I:{2,4,5,7})
    {
        for(int32 Side=1;Side<15;++Side)
            S[3].Tube(Point(Stations[I],Side,.7),Point(Stations[I],Side+1,.7),1,1,6);
    }
    for(double Side:{-1.,1.})
    {
        S[4].Tube(FVector(-183,Side*113,17),FVector(185,Side*113,17),6,6);
        S[4].Tube(FVector(185,Side*113,17),FVector(212,Side*113,26),6,4);
        S[4].Tube(FVector(212,Side*113,26),FVector(226,Side*113,44),4,3);
        for(double X:{-117.,113.})
        {
            S[4].Tube(FVector(X,Side*113,20),FVector(X,Side*92,56),4,4);
            S[4].Tube(FVector(X,Side*92,56),FVector(X,Side*56,80),4,4);
        }
        S[3].Box(FVector(35,Side*99,116),FVector(17,3,3.8));
        S[3].Box(FVector(-129,Side*86,111),FVector(19,3,3.8));
        S[4].Tube(FVector(-14,Side*50,232),FVector(-146,Side*47,223),21,13);
        S[3].Tube(FVector(-146,Side*47,223),FVector(-176,Side*52,219),13,10);
        S[5].Box(FVector(-176,Side*54,219),FVector(3,13,13));
    }
    S[0].Tube(FVector(-208,0,151),FVector(-564,0,197),37,14,20);
    S[2].Tube(FVector(-509,0,190),FVector(-539,0,194),17,17,20);
    const FVector FinA(-545,0,194),FinB(-612,0,273),FinC(-581,0,276),FinD(-510,0,207);
    for(double Y:{-5.,5.})
    {
        const FVector Offset(0,Y,0);
        if(Y>0)S[0].Quad(FinD+Offset,FinC+Offset,FinB+Offset,FinA+Offset);
        else S[0].Quad(FinA+Offset,FinB+Offset,FinC+Offset,FinD+Offset);
    }
    S[0].Quad(FinA-FVector(0,5,0),FinA+FVector(0,5,0),FinB+FVector(0,5,0),FinB-FVector(0,5,0));
    S[0].Quad(FinB-FVector(0,5,0),FinB+FVector(0,5,0),FinC+FVector(0,5,0),FinC-FVector(0,5,0));
    S[0].Quad(FinC-FVector(0,5,0),FinC+FVector(0,5,0),FinD+FVector(0,5,0),FinD-FVector(0,5,0));
    S[0].Box(FVector(-496,0,192),FVector(64,122,5));
    S[4].Tube(FVector(-6,0,224),FVector(-6,0,286),13,8,20);
    S[5].Box(FVector(188,0,85),FVector(18,22,8));
    Cached=Build(Owner,S,{{.18f,.22f,.19f},{.023f,.069f,.082f},{.39f,.12f,.037f},
                         {.032f,.038f,.039f},{.27f,.30f,.31f},{.48f,.45f,.38f}},
                        {.49f,.18f,.50f,.67f,.26f,.46f});
    return Cached.Get();
}

inline UStaticMesh* Rotor(UObject* Owner,bool Tail)
{
    static TWeakObjectPtr<UStaticMesh> Cached[2];
    auto& Result=Cached[Tail?1:0];
    if(Result.IsValid())return Result.Get();
    TArray<FSection> Sections;
    Sections.SetNum(2);
    const double Radius=Tail?58.:510.;
    Sections[0].Tube(FVector(0,0,-6),FVector(0,0,6),Tail?4:12,Tail?4:12,16);
    for(int32 I=0;I<(Tail?2:4);++I)
    {
        const FQuat Turn(FVector::UpVector,I*(Tail?PI:PI*.5));
        const double W=Tail?4.:12.;
        const FVector A=Turn.RotateVector(FVector(9,-W*.25,1));
        const FVector B=Turn.RotateVector(FVector(Radius*.88,-W,0));
        const FVector C=Turn.RotateVector(FVector(Radius*.88,W*.6,0));
        const FVector D=Turn.RotateVector(FVector(9,W*.4,1));
        const FVector TipA=Turn.RotateVector(FVector(Radius,-W*.6,0));
        const FVector TipB=Turn.RotateVector(FVector(Radius,W*.4,0));
        Sections[0].Quad(A,B,C,D); Sections[0].Quad(D,C,B,A);
        Sections[1].Quad(B,TipA,TipB,C); Sections[1].Quad(C,TipB,TipA,B);
    }
    Result=Build(Owner,Sections,{{.045f,.054f,.057f},{.54f,.40f,.13f}},{.61f,.55f});
    return Result.Get();
}
}
