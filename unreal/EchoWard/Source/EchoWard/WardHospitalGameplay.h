#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "WardCampaignData.h"
#include "WardHospitalGameplay.generated.h"

class AEWardCampaignMode;
class AEWardHospitalGameplay;
class UStaticMeshComponent;

UCLASS()
class ECHOWARD_API AEWardHospitalMob : public ACharacter
{
    GENERATED_BODY()
public:
    AEWardHospitalMob();
    void Build(AEWardHospitalGameplay* Owner,WardData::Json Row);
    virtual void Tick(float Delta)override;
    virtual float TakeDamage(float Amount,const FDamageEvent& Event,AController* Instigator,AActor* Causer)override;
    void Hear(FVector At,float Radius);
    void ResetMob();
    void Revive();
    void Configure59();
    bool Eligible59()const;
    bool Sight59(FVector Point,bool IgnoreFov=false)const;
    bool AttackClear59()const;
    bool CameraSees59()const;
    void Stun59(float Seconds,bool Flash=false);
    void StepBrain59(float Delta);
    void Special59(float Delta,bool Visible);
    void Move59(float Delta,float Speed);
    void ChangeState59(const FString& State);
    void FinishDeath59();
    void EnsureVisual61();
    void Sound61(const FString& Event,float Volume=.35f);
    void Presentation61(float Delta);
    FString Model61;
    int32 SoundTake61=0,PatrolStep61=0;
    float StepDistance61=0,BreathClock61=3,SpeechClock61=12,AudioClock61=0;
    FVector LastPosition61=FVector::ZeroVector;
    FString NPCTarget62;
    WardData::Json Rule59;
    FString State59=TEXT("dormant"),Guid59,Zone59;
    FVector LastSeen59=FVector::ZeroVector,AttackAim59=FVector::ZeroVector,SearchOrigin59=FVector::ZeroVector;
    float SearchLeft59=0,StateClock59=0,Windup59=0,Alert59=0,HitGrace59=0,StunImmunity59=0,NoisePriority59=0;
    float DeathTime59=0,ProgressClock59=0;
    FVector ProgressAt59=FVector::ZeroVector;
    bool BodyPresent59=false;
    bool SpawnedOnDeath59=false,Revived59=false,NodePowered59=false,AttackResolved59=false,SeenPlayer59=false;
    int32 SearchStep59=0;
    UPROPERTY() TObjectPtr<class UEWardMonsterVisual59> Visual59;

    WardData::Json Snapshot()const;
    void Restore(WardData::Json State);
    UPROPERTY() TObjectPtr<AEWardHospitalGameplay> Runtime;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    WardData::Json Definition;
    FString Id,Species,Class;
    FVector Home,Target;
    TArray<FVector> Path;
    float Health=70,MaxHealth=70,Stunned=0,Memory=0,AttackClock=0,PathClock=0,SpecialClock=0,WalkClock=0,StrikeVisual=0;
    bool Dead=false,Active=false;
    int FirstChapter=2,PathIndex=0;
};

UCLASS()
class ECHOWARD_API AEWardHospitalGameplay : public AActor
{
    GENERATED_BODY()
public:
    AEWardHospitalGameplay();
    void Build(AEWardCampaignMode* Mode);
    int DetailedStoryProps=0,MissingStoryVisuals=0;
    void BuildNarrative35();
    void ResetNarrative35();
    bool NarrativeUse35(const FString& Id);
    bool BeginNarrative35(const FString& Id,bool Replay=false);
    bool NarrativeCommand35(const FString& Action);
    bool NarrativeFlag35(const FString& Key)const;
    void SetNarrativeFlag35(const FString& Key,bool Value);
    bool NarrativeRequirements35(WardData::Json Row,FString* Missing=nullptr)const;
    void NarrativeEffects35(WardData::Json Effects);
    void FinishNarrative35();
    void RefreshNarrative35();
    void PreparePlans35();
    void TickNarrative35(float Delta);
    void BuildNarrativeGates35();
    bool FindNarrativeStanding35(const FString& Id,FVector& Standing)const;
    UPROPERTY() TMap<FString,TObjectPtr<class UBoxComponent>> NarrativeGates35;
    void AssessNarrative35(float Delta);
    FString NarrativeTitle35()const;
    FString NarrativeBody35(bool FullText = false)const;
    FString NarrativeVoice35()const;
    FString NarrativeEnding35()const;
    WardData::Json NarrativeOptions35()const;
    WardData::Json NarrativeSnapshot35()const;
    void RestoreNarrative35(WardData::Json State);
    bool NarrativeEnabled35=false,NarrativeReplay35=false,NarrativeTesting35=false;
    UPROPERTY() TObjectPtr<class UAudioComponent> DialogueAudio62;
    FString DialogueSession62,DialogueKey62,DialogueActor62;
    TArray<FString> DialogueLines62;
    int32 DialogueIndex62=0;
    float DialogueLeft62=0,DialogueTime62=0,DialogueCooldown62=8;
    bool DialogueMain62=false;
    WardData::Json Narrative35,NarrativeState35;
    FString ActiveScene35,SelectedChoice35;
    int DialoguePage35=0;
    float NarrativeClock35=0;
    TMap<FString,WardData::Json> NarrativeScenes35;

    void ResetStory();
    void BuildArrival();
    void ResetArrival();
    void StartArrival();
    void BuildHome52();
    void StartHome52();
    void StopHome52();
    void TickHome52(float Delta, bool Replica = false);
    bool UseHome52(bool DoorKey = false);
    bool UseHomeAt52(FVector Player, bool DoorKey);
    bool HomeContains52(FVector Player) const;
    void ApplyCoopHome52(int Stage, const FString& Subtitle);
    void SetHomeVisible53(bool Visible);
    int32 HomeImportedMeshes53 = 0;
    float HomeStreetClock53 = 0;
    FVector HomeStreetCarStart53 = FVector::ZeroVector;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> HomeStreetCar53;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HomeCurtains53;
    UPROPERTY() TObjectPtr<class UAudioComponent> HomeAudio53;
    UPROPERTY() TObjectPtr<class UAudioComponent> DriveAudio53;
    static constexpr float DriveDuration53 = 6.65f;
    FString HomeObjective52()const;
    TArray<FString> HomeLines52()const;
    int HomeStage52=-1,HomeLine52=0;
    float HomeRing52=0, HomeLineTime69=0;
    FString HomeSubtitle69;
    UPROPERTY() TObjectPtr<AActor> HomeActor52;
    FVector HomePhone52=FVector(-79780,-79930,83);

    void TickArrival(float Delta);
    void ApplyArrivalState();
    bool ArrivalInteract();
    bool UseCheckpoint29(FVector Player);
    bool PartyInsideEntrance29(FVector Player) const;
    void ApplyCoopArrival29(int Stage, float DriveClock);
    void TickCoopArrival29(float Delta);
    void UpdateArrivalBarrier29(float Delta);
    void PoseArrivalDrive29(float Delta);
    void FinishCheckpoint69();
    bool ArrivalCinematic()const {return ArrivalStage==0||ArrivalStage==1||Seated31;}
    void BuildHeroVehicle31();
    void TickVehicle31(float Delta);
    bool VehicleInteract31();
    bool VehicleExitClear31()const;
    void ToggleVehiclePart31();
    FString VehiclePrompt31()const;
    FString NearestVehiclePart31()const;
    bool Seated31=false;
    bool VehicleCloseQuality32=false;
    float VehicleYaw31=0,VehiclePitch31=0,VehicleAutoClose31=0,WheelRoll31=0;
    FVector VehicleLastAt31=FVector::ZeroVector;
    UPROPERTY() TMap<FString,TObjectPtr<UStaticMeshComponent>> VehicleParts31;
    TMap<FString,float> VehicleOpen31,VehicleTarget31;
    UPROPERTY() TObjectPtr<class UPointLightComponent> VehicleCabin31;
    FString ArrivalObjective()const;
    WardData::Json ArrivalSnapshot()const;
    void RestoreArrival(WardData::Json State);
    void AssessArrival(float Delta);
    WardData::Json ArrivalData;
    int ArrivalStage=-1;
    int EntranceWaiting29=0;
    float ArrivalClock=0,EntryClock=0,GateBlend=0;
    UPROPERTY() TObjectPtr<AActor> ArrivalCar;
    UPROPERTY() TObjectPtr<class ACameraActor> ArrivalCamera;
    UPROPERTY() TObjectPtr<USceneComponent> ArrivalBarrierPivot;
    UPROPERTY() TObjectPtr<class UBoxComponent> ArrivalGateBlock;
    UPROPERTY() TObjectPtr<class UWidgetComponent> ArrivalNotice;
    UPROPERTY() TObjectPtr<class UPointLightComponent> ArrivalAlarm;
    UPROPERTY() TObjectPtr<class UPointLightComponent> LabAlarm;
    UPROPERTY() TArray<TObjectPtr<class USpotLightComponent>> ArrivalHeadlights;
    UPROPERTY() TArray<TObjectPtr<AActor>> ArrivalActors;
    virtual void Tick(float Delta)override;
    bool Interact();
    bool UseId(const FString& Id);
    FString Objective()const;
    FString Prompt()const;
    FString DocumentTitle(const FString& Entry)const;
    FString DocumentBody(const FString& Entry)const;
    void Refresh();
    void SetChapter(int Chapter);
    int Chapter()const;
    void Hear(FVector At,float Radius);
    bool NodesNear(FVector At,float Radius)const;
    TArray<FVector> FindPath(FVector From,FVector To,float Radius)const;
    bool FloorCell(FIntVector Cell,float Radius=30)const;
    FIntVector CellAt(FVector At)const;
    FVector WorldAt(FIntVector Cell)const;
    WardData::Json Snapshot()const;
    void Restore(WardData::Json State);
    UPROPERTY() TObjectPtr<AEWardCampaignMode> Mode;
    UPROPERTY() TArray<TObjectPtr<AEWardHospitalMob>> Mobs;
    UPROPERTY() TArray<TObjectPtr<AActor>> PropActors;
    TMap<FString,AActor*> Props;
    TMap<FString,WardData::Json> Definitions;
    TMap<FIntPoint,TArray<FIntPoint>> NavRows;
    TSet<FIntVector> DoorBlocks;
    WardData::Json Data;
    bool FreezeAI=false;
    float HintClock=0;
    int SpawnedChildren=0;
    AEWardHospitalMob* SpawnChild(FVector Near,const FString& Species);
    AEWardHospitalMob* SpawnMob(WardData::Json Row);
    void RefreshNavigation();
    void Assess(float Delta);
};
