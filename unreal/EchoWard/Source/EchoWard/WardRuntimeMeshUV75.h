#pragma once
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

// Fast runtime builds have no editor pass to compute UV streaming metadata.
// Derive the world-centimetres per UV unit from the actual triangles instead
// of leaving the material slots uninitialized or disabling texture streaming.
inline void WardInitializeRuntimeMeshUV75(UStaticMesh* Mesh,const FMeshDescription& Description)
{
    FStaticMeshConstAttributes A(Description);
    TArray<double> Weighted,Areas;
    Weighted.Init(0,Mesh->GetStaticMaterials().Num());Areas.Init(0,Weighted.Num());
    for(const FTriangleID T:Description.Triangles().GetElementIDs())
    {
        const int32 Slot=Description.GetTrianglePolygonGroup(T).GetValue();
        if(!Weighted.IsValidIndex(Slot))continue;
        const auto Corners=Description.GetTriangleVertexInstances(T);
        FVector P[3];FVector2D UV[3];
        for(int32 I=0;I<3;++I){P[I]=FVector(A.GetVertexPositions()[Description.GetVertexInstanceVertex(Corners[I])]);UV[I]=FVector2D(A.GetVertexInstanceUVs().Get(Corners[I],0));}
        const double Area=.5*FVector::CrossProduct(P[1]-P[0],P[2]-P[0]).Length();
        const FVector2D U=UV[1]-UV[0],V=UV[2]-UV[0];
        const double UVArea=.5*FMath::Abs(U.X*V.Y-U.Y*V.X);
        if(Area>UE_SMALL_NUMBER&&UVArea>UE_SMALL_NUMBER){const double Density=FMath::Sqrt(Area/UVArea);if(FMath::IsFinite(Density)){Weighted[Slot]+=Density*Area;Areas[Slot]+=Area;}}
    }
    for(int32 I=0;I<Weighted.Num();++I){auto& UV=Mesh->GetStaticMaterials()[I].UVChannelData;UV.bInitialized=true;UV.bOverrideDensities=false;UV.LocalUVDensities[0]=Areas[I]>UE_SMALL_NUMBER?Weighted[I]/Areas[I]:0;}
}
