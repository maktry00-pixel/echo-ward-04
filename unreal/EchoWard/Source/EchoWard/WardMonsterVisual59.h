#pragma once
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "WardPeople54.h"
#include "WardMonsterVisual59.generated.h"
UCLASS()
class ECHOWARD_API UEWardMonsterVisual59: public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    bool Load59(const FString& Species,float Height);
    void Present59(int Clip,float Time,bool Dead,float DeathTime);
    int Vertices59()const{return VertexCount59;}
    void Unload61();
    void BeginFall61(FVector Direction);
    void TickFall61(float Delta);
    void ResetFall61();
    bool HasFall61()const{return !Particles61.IsEmpty();}
    TArray<FVector> CaptureFall61()const;
    void RestoreFall61(const TArray<FVector>& Points);
    FString LoadedModel61()const{return Model61;}
    FVector MotionJoint97(int32 Index, bool Tip = false) const;
    FVector MotionChestForward97() const;
private:
    void ApplyMotion97(float Phase, int32 Clip, float Now);
    struct FBone61 {int32 Parent=-1;FVector3f Pivot,Tip;float Radius=4;};
    struct FParticle61 {FVector Position,Previous;float Radius=4;};
    void DrawPose61();
    void ApplyFall61();
    TArray<FBone61> Bones61;
    TArray<FParticle61> Particles61;
    TArray<FWardPersonPose54> BlendFrom61;
    FString Model61;
    float BlendAt61=0,FallAge61=0;
    int LastClip61=-1;
    TArray<FWardPersonSection54> Sections59;
    TArray<FWardPersonClip54> Clips59;
    TArray<FWardPersonPose54> Sample59;
    float Scale59=1,Height59=178,NextPose59=0;
    int VertexCount59=0;
};
