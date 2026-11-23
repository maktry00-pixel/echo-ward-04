#pragma once

#include "ProceduralMeshComponent.h"
#include "WardCampaignData.h"
#include "WardPeople54.generated.h"

class AEWardHospitalGameplay;
class UStaticMeshComponent;

struct FWardPersonVertex54
{
    FVector3f Position, Normal;
    FVector2f UV;
    uint8 Joints[4];
    float Weights[4];
};

struct FWardFaceVertex58
{
    FVector3f Blink=FVector3f::ZeroVector, Jaw=FVector3f::ZeroVector;
};
struct FWardBodyNode58
{
    int32 Bone=0, Parent=-1;
    FVector3f Pivot=FVector3f::ZeroVector;
    float Radius=8, Length=0;
    FVector Position=FVector::ZeroVector, Previous=FVector::ZeroVector, Reference=FVector::ZeroVector;
};
struct FWardPersonSection54
{
    FString Material;
    TArray<FWardPersonVertex54> Vertices;
    TArray<int32> Indices;
    TArray<FVector> Positions, Normals;
    TArray<FVector2D> UV;
    TArray<FVector3f> Shape55;
    TArray<FWardFaceVertex58> Face58;
};

struct FWardPersonPose54
{
    float M[12];
    FVector3f Apply(FVector3f V, bool bPoint) const;
};

struct FWardPersonFrame54
{
    TArray<FWardPersonPose54> Joints;
    FVector3f Hand;
};

struct FWardPersonClip54
{
    float Rate = 20.f;
    TArray<FWardPersonFrame54> Frames;
};

UCLASS()
class ECHOWARD_API UEWardPerson54 : public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    explicit UEWardPerson54(const FObjectInitializer& Initializer);
    void SetIdentity(AEWardHospitalGameplay* InStory, FString InId, FString InModel);
    bool LoadAppearance();
    void LoadHuman58();
    bool TickHuman58(float Delta, bool Talking);
    void SetTask58(const FString& Task, const FVector& Goal=FVector::ZeroVector);
    const FString& HumanState58() const { return State58; }
    bool CanEscort58() const;
    bool Vulnerable62()const;
    bool Escape62(const FVector& Danger);
    const FString& Identity62()const{return Id;}
    FVector EscapeGoal62()const{return DangerGoal58;}
    float Health62()const{return Health58;}
    float FleeSpeed62=0;
    bool WasDanger62=false;
    bool IsDead58() const { return Dead58; }
    bool IsFallen58() const { return Fallen58; }
    void Impact58(float Damage,const FVector& Impulse,bool ForceDeath=false);
    void GetUp58();
    WardData::Json HumanSnapshot58() const;
    void RestoreHuman58(WardData::Json State);
    void DebugFace58(float Blink,float Mouth) { Blink58=Blink; Mouth58=Mouth; }
    float BlinkWeight58() const { return Blink58; }
    float MouthWeight58() const { return Mouth58; }
    void TickRagdoll58(float Delta);
    void BeginRagdoll58(const FVector& Impulse);
    void PrepareHumanPose58();
    bool MoveTo58(const FVector& Goal,float Delta,float Speed);
    void UpdateFace58(float Delta,bool Talking);

    bool ResetPatrol57();
    bool AdvancePatrol57(float Delta);
    int32 PatrolPoints57() const { return Patrol57.Num(); }
    void BeginMemory57() { MemoryTime57=60.f; }
    void PresentPose(int32 Clip, float Time, float Blend = 1.f, int32 BlendBase = 0);
    static FString OpeningLine55(const FString& Id);
    static FString VoicePath55(const FString& Id);
    float TargetHeight55() const { return Height55; }
    float HabitLength55() const;
    const FString& ModelName() const { return Model; }
    int32 ClipCount() const { return Clips.Num(); }
    virtual void TickComponent(float Delta, ELevelTick Tick, FActorComponentTickFunction* Function) override;

private:
    TArray<uint16> MajorMap58;
    TArray<FWardBodyNode58> BodyNodes58;
    TArray<FWardPersonPose54> RigidSkin58;
    TArray<FTransform> RigidDelta58, RecoveryPose58;
    TArray<FVector> TaskPath58;
    FVector DangerGoal58=FVector::ZeroVector;
    FVector TaskGoal58=FVector::ZeroVector, PathGoal58=FVector::ZeroVector, LastLocation58=FVector::ZeroVector;
    FString Task58=TEXT("routine"), State58=TEXT("idle"), VoiceKey58;
    int32 PathIndex58=0;
    float Recovery58=-1;
    float Health58=100, PathClock58=0, DangerClock58=0, DangerHold58=0, Pain58=0;
    float Blink58=0, Mouth58=0, BlinkDelay58=2, BlinkTime58=-1, SpeechTime58=0;
    float SmokeClock58=35, SmokeTime58=-1, StepDistance58=0, PhysicsClock58=0;
    bool StairPath58=false;
    bool HumanLoaded58=false, Dead58=false, Fallen58=false;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Cigarette58;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Ember58;
    TArray<FWardPersonPose54> SmokeSkin58;
    FVector3f SmokeHand58=FVector3f::ZeroVector;
    UPROPERTY() TObjectPtr<AEWardHospitalGameplay> Story;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Document;
    FString Id, Model;
    TArray<FWardPersonSection54> Sections;
    TArray<FWardPersonClip54> Clips;
    TArray<FWardPersonPose54> Sample;
    bool bTriedLoad = false, bWasTalking = false;
    int32 ActiveClip = 0;
    bool bOverrideModel55 = false;
    float Height55 = 0, ReferenceHeight55 = 170, HeightScale55 = 1.f;
    float Shoulders55 = 1, Waist55 = 1, Hips55 = 1, Depth55 = 1, Face55 = 1, Jaw55 = 1;
    float HabitPeriod55 = 0, HabitDelay55 = 0, TalkTempo55 = 1.f;
    FVector3f BodyScale55(float Height, float Lateral) const;
    TArray<FVector> Patrol57;
    int32 PatrolIndex57=0;
    float PatrolWait57=0.f, MemoryTime57=0.f;
    bool Ground57(const FVector& At,FVector& Ground) const;
    bool Segment57(const FVector& From,const FVector& To) const;
    float UpdateDelay = 0, Phase = 0, DocumentTime = 0;
};
