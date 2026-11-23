#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WardTitleScene.generated.h"
class AEWardCampaignMode;
class ACameraActor;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UPointLightComponent;
class ULightComponent;
class UMaterialInstanceDynamic;
class UAudioComponent;
class UMaterialInterface;
class USkeletalMeshComponent;
class USpotLightComponent;

/** A paused-world presentation set built exclusively from the campaign's assets. */
UCLASS()
class ECHOWARD_API AEWardTitleScene : public AActor
{
    GENERATED_BODY()
public:
    AEWardTitleScene();
    void Build(AEWardCampaignMode* InMode);
    void Activate(bool Enabled);
    void BeginTransition(const FString& Action);
    void Click();
    void Hover();
    void SetGameBackdrop(bool Enabled);
    void TickGameBackdrop(float Delta);
    void SetTrainingBackdrop(bool Enabled);
    void TickTrainingBackdrop(float Delta);
    UPROPERTY() TObjectPtr<USceneComponent> TrainingSet;
    UPROPERTY() TObjectPtr<ACameraActor> TrainingCamera;
    UPROPERTY() TObjectPtr<USceneComponent> TrainingDoorHinge;
    UPROPERTY() TArray<TObjectPtr<ULightComponent>> TrainingLights;
    bool TrainingBackdropActive=false;
    float TrainingAge=0;
    int32 TrainingBloodDecals=0;
    void SetAboutBackdrop(bool Enabled);
    void TickAboutBackdrop(float Delta);
    UPROPERTY() TObjectPtr<USceneComponent> AboutSet;
    UPROPERTY() TObjectPtr<ACameraActor> AboutCamera;
    UPROPERTY() TObjectPtr<USceneComponent> AboutLampHinge;
    UPROPERTY() TArray<TObjectPtr<ULightComponent>> AboutLights;
    bool AboutBackdropActive=false;
    float AboutAge=0;
    int32 AboutBloodDecals=0;
    void SetSaveBackdrop(bool Enabled);
    void TickSaveBackdrop(float Delta);
    UPROPERTY() TObjectPtr<USceneComponent> SaveSet;
    UPROPERTY() TObjectPtr<ACameraActor> SaveCamera;
    UPROPERTY() TObjectPtr<USceneComponent> SaveLampHinge;
    UPROPERTY() TArray<TObjectPtr<ULightComponent>> SaveLights;
    bool SaveBackdropActive=false;
    float SaveAge=0;
    int32 SaveBloodDecals=0;
    UPROPERTY() TObjectPtr<USceneComponent> GameSet;
    UPROPERTY() TObjectPtr<ACameraActor> GameCamera;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> GameZombie;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> GameDoors;
    UPROPERTY() TArray<TObjectPtr<ULightComponent>> GameLights;
    bool GameBackdropActive=false,ZombieInside=false;
    float GameAge=0,ZombieRoomSeconds=0;
    int32 ZombieRoom=0,GameBloodDecals=0;
    int32 GameSoundBeat=-1;
    virtual void Tick(float Delta) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Wheelchair;
    UPROPERTY() TObjectPtr<UPointLightComponent> Alarm;
    UPROPERTY() TObjectPtr<ULightComponent> BrokenLamp;
    UPROPERTY() TObjectPtr<ULightComponent> SideLight;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> SideDoor;
    UPROPERTY() TObjectPtr<USceneComponent> SideHinge;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> FarHinges;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Nurse;
    UPROPERTY() TObjectPtr<USpotLightComponent> ScareLight;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Dust;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> AlarmMaterial;
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Loops;
    UPROPERTY() TMap<TObjectPtr<ULightComponent>,float> LightBase;
    bool Active=false,Transitioning=false,FlashWasVisible=true;
    float Age=0,TransitionAge=0,Fade=0;
    float NurseMoving=0,NurseSpeed=0,NursePeek=0,NurseScare=0,FarDoorOpen=0;
    int32 ScareCount=0;
    bool NurseInCorridor=false,ScareFired=false;
    FName NurseHeadBone;
    FString Pending;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> PeekingHead;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> ReachingHand;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RollingBall;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Forearm;
    float IdleCycle=0,IdleDoorOpen=0,IdleEntranceOpen=0,BallStillSeconds=0;
    FName IdleBeat=TEXT("quiet");
    int32 IdleLoopCount=0,BloodDecals=0;
    bool EventRigReady=false;
private:
    UPROPERTY() TObjectPtr<AEWardCampaignMode> Mode;
    // Keep every menu set outside the complete campus and its forest scenery.
    FVector Origin=FVector(300000,300000,2000);
    FVector CameraStart;
    double PreviousRealTime=0;
    bool PreviousCameraPaused=false;
    float NextCue=4;
    int32 CueIndex=0;
    float LastFootstep=0;
    bool DoorSoundFired=false;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> NurseHeadParts;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> NurseKnife;
    UPROPERTY() TObjectPtr<USceneComponent> KnifeGrip;
    FName NurseHandBone;
    FQuat NurseHeadRest=FQuat::Identity,NurseHandRest=FQuat::Identity;
    FName PeekBone,ReachBone;
    float PreviousIdleCycle=0,HandDirection=1;
    UStaticMeshComponent* Part(const FString& Mesh,FVector At,FVector Scale,UMaterialInterface* Material,FRotator Rotation=FRotator::ZeroRotator);
    void Sound(const FString& Id,float Volume,float Pan=0);
    void BuildNurse();
    void AnimateNurse(float Delta);
    void BuildIdleEvents();
    void AnimateIdleEvents(float Delta);
    void HideIdleEvents();
    void BuildGameBackdrop();
    void BuildSaveBackdrop();
    void BuildAboutBackdrop();
    void BuildTrainingBackdrop();
    void RefreshBackdrop();
};
