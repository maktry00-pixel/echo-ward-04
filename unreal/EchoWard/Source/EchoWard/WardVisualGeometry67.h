#pragma once
#include "WardRuntimeMeshUV75.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

namespace WardVisual67
{
struct FSection
{
    TArray<FVector> Points;
    TArray<FVector> Normals;
    TArray<int32> Triangles;

    void Triangle(FVector A,FVector B,FVector C,FVector NA=FVector::ZeroVector,
                  FVector NB=FVector::ZeroVector,FVector NC=FVector::ZeroVector)
    {
        const int32 Start=Points.Num();
        Points.Append({A,B,C});
        Normals.Append({NA,NB,NC});
        Triangles.Append({Start,Start+1,Start+2});
    }

    void Quad(FVector A,FVector B,FVector C,FVector D)
    {
        Triangle(A,B,C);
        Triangle(A,C,D);
    }

    void Box(FVector Center,FVector Size)
    {
        const FVector H=Size*.5;
        const FVector A=Center+FVector(-H.X,-H.Y,-H.Z), B=Center+FVector(H.X,-H.Y,-H.Z);
        const FVector C=Center+FVector(H.X,H.Y,-H.Z), D=Center+FVector(-H.X,H.Y,-H.Z);
        const FVector Z(0,0,Size.Z);
        Quad(D,C,B,A); Quad(A+Z,B+Z,C+Z,D+Z);
        Quad(A,B,B+Z,A+Z); Quad(B,C,C+Z,B+Z);
        Quad(C,D,D+Z,C+Z); Quad(D,A,A+Z,D+Z);
    }

    void Tube(FVector A,FVector B,double RadiusA,double RadiusB,int32 Sides=16)
    {
        const FVector Axis=(B-A).GetSafeNormal();
        if(Axis.IsNearlyZero())return;
        const FVector Side=FVector::CrossProduct(Axis,FMath::Abs(Axis.Z)>.9?FVector::ForwardVector:FVector::UpVector).GetSafeNormal();
        const FVector Up=FVector::CrossProduct(Axis,Side);
        for(int32 I=0;I<Sides;++I)
        {
            const double U=I*2*PI/Sides,V=(I+1)*2*PI/Sides;
            const FVector D=FMath::Cos(U)*Side+FMath::Sin(U)*Up;
            const FVector E=FMath::Cos(V)*Side+FMath::Sin(V)*Up;
            Quad(A+D*RadiusA,A+E*RadiusA,B+E*RadiusB,B+D*RadiusB);
            Triangle(A,A+E*RadiusA,A+D*RadiusA);
            Triangle(B,B+D*RadiusB,B+E*RadiusB);
        }
    }
};

inline UStaticMesh* Build(UObject* Owner,const TArray<FSection>& Sections,
                          const TArray<FLinearColor>& Colors,const TArray<float>& Roughness)
{
    auto* Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_CampaignPBR.M_CampaignPBR"));
    if(!Parent||Sections.Num()!=Colors.Num()||Sections.Num()!=Roughness.Num())return nullptr;

    auto* Mesh=NewObject<UStaticMesh>(Owner);
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    Attributes.GetVertexInstanceUVs().SetNumChannels(1);

    for(int32 Index=0;Index<Sections.Num();++Index)
    {
        const FSection& Section=Sections[Index];
        if(Section.Triangles.IsEmpty())continue;
        auto* Material=UMaterialInstanceDynamic::Create(Parent,Mesh);
        Material->SetVectorParameterValue(TEXT("Tint"),Colors[Index]);
        Material->SetScalarParameterValue(TEXT("Roughness"),Roughness[Index]);
        if(Roughness[Index]<.28f)Material->SetScalarParameterValue(TEXT("Metallic"),.24f);
        const FName Slot(*FString::Printf(TEXT("Finish_%d"),Index));
        Mesh->GetStaticMaterials().Add(FStaticMaterial(Material,Slot));
        const FPolygonGroupID Group=Description.CreatePolygonGroup();
        Attributes.GetPolygonGroupMaterialSlotNames()[Group]=Slot;

        for(int32 I=0;I<Section.Triangles.Num();I+=3)
        {
            const FVector A=Section.Points[Section.Triangles[I]];
            const FVector B=Section.Points[Section.Triangles[I+1]];
            const FVector C=Section.Points[Section.Triangles[I+2]];
            const FVector3f Normal(FVector::CrossProduct(B-A,C-A).GetSafeNormal());
            if(Normal.IsNearlyZero())continue;
            const FVector3f Tangent(FVector(B-A).GetSafeNormal());
            const FVector Positions[]={A,B,C};
            const FVector2f UVs[]={{0,0},{1,0},{0,1}};
            FVertexInstanceID Instances[3];
            for(int32 Corner=0;Corner<3;++Corner)
            {
                const FVertexID Vertex=Description.CreateVertex();
                Attributes.GetVertexPositions()[Vertex]=FVector3f(Positions[Corner]);
                Instances[Corner]=Description.CreateVertexInstance(Vertex);
                const FVector Override=Section.Normals[Section.Triangles[I+Corner]];
                Attributes.GetVertexInstanceNormals()[Instances[Corner]]=Override.IsNearlyZero()?Normal:FVector3f(Override);
                Attributes.GetVertexInstanceTangents()[Instances[Corner]]=Tangent;
                Attributes.GetVertexInstanceBinormalSigns()[Instances[Corner]]=1;
                Attributes.GetVertexInstanceUVs().Set(Instances[Corner],0,UVs[Corner]);
            }
            Description.CreateTriangle(Group,MakeArrayView(Instances));
        }
    }

    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild=true;
    Params.bBuildSimpleCollision=false;
    Params.bMarkPackageDirty=false;
    const TArray<const FMeshDescription*> Descriptions={&Description};
    WardInitializeRuntimeMeshUV75(Mesh,Description);
    return Mesh->BuildFromMeshDescriptions(Descriptions,Params)?Mesh:nullptr;
}

inline UStaticMesh* TunnelServices(UObject* Owner)
{
    static TWeakObjectPtr<UStaticMesh> Cached;
    if(Cached.IsValid())return Cached.Get();
    TArray<FSection> Sections;
    Sections.SetNum(4);
    // Two ceiling lines, coupling sleeves, and hangers that reach the slab.
    for(double X:{-168.,168.})
    {
        for(double Y:{-15.,15.})
        {
            Sections[2].Tube(FVector(X,Y,11),FVector(X,Y,65),1.6,1.6,8);
            Sections[2].Box(FVector(X,Y,65),FVector(7,6,1.5));
        }
        Sections[2].Box(FVector(X,0,1),FVector(6,42,5));
    }
    Sections[0].Tube(FVector(-199,-12,10),FVector(199,-12,10),7.5,7.5,14);
    Sections[1].Tube(FVector(-199,13,13),FVector(199,13,13),4.5,4.5,12);
    for(double X:{-192.,175.})
    {
        Sections[2].Tube(FVector(X-5,-12,10),FVector(X+5,-12,10),9,9,14);
        Sections[3].Tube(FVector(X-2,13,13),FVector(X+2,13,13),5.6,5.6,12);
    }
    Cached=Build(Owner,Sections,{{.19f,.21f,.21f},{.20f,.092f,.049f},
                              {.11f,.12f,.13f},{.56f,.31f,.07f}},{.58f,.65f,.51f,.59f});
    return Cached.Get();
}

inline UStaticMesh* WaterDispenser(UObject* Owner)
{
    static TWeakObjectPtr<UStaticMesh> Cached;
    if(Cached.IsValid())return Cached.Get();

    TArray<FSection> S;
    S.SetNum(5);
    // The recessed bay has side jambs and a back wall, leaving space for a cup.
    S[0].Box(FVector(0,-5,48),FVector(34,25,92));
    S[0].Box(FVector(-14.5,11,58),FVector(5,12,69));
    S[0].Box(FVector(14.5,11,58),FVector(5,12,69));
    S[0].Box(FVector(0,11,23),FVector(34,12,18));
    S[0].Box(FVector(0,11,87),FVector(34,12,14));
    S[0].Box(FVector(0,0,95),FVector(36,35,5));
    S[1].Box(FVector(0,1,3),FVector(36,36,6));
    S[1].Box(FVector(0,11,39),FVector(25,12,2));
    for(int32 I=0;I<7;++I)S[2].Box(FVector(-10+I*3.3,11,40.1),FVector(.55,10,.25));
    for(int32 I=0;I<3;++I)S[2].Box(FVector(0,17.1,16+I*2.1),FVector(14,.35,.6));
    for(double X:{-5.,5.})
    {
        S[2].Tube(FVector(X,12,78),FVector(X,12,69),1.1,1.1,12);
        S[X<0?3:4].Box(FVector(X,17.3,86),FVector(6,.7,2.2));
    }
    S[1].Tube(FVector(0,0,97),FVector(0,0,101),7.2,7.2);
    S[3].Tube(FVector(0,0,101),FVector(0,0,106),6.3,12);
    S[3].Tube(FVector(0,0,106),FVector(0,0,129),12,12);
    S[3].Tube(FVector(0,0,129),FVector(0,0,133),12,9);
    for(double Z:{112.,123.})S[3].Tube(FVector(0,0,Z),FVector(0,0,Z+1),12.5,12.5);

    Cached=Build(Owner,S,{{.34f,.38f,.38f},{.045f,.055f,.06f},{.27f,.30f,.30f},
                         {.055f,.23f,.33f},{.43f,.07f,.04f}},{.56f,.74f,.32f,.2f,.48f});
    return Cached.Get();
}
}
