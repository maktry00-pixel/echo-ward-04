#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "WardPrototype.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USpotLightComponent;
class USoundBase;
class UAnimSequence;
class UStaticMeshComponent;
class USceneComponent;
class AEWInteractable;

UENUM()
enum class EWardInteraction : uint8 { Drawer, Power, Door };

UCLASS()
class ECHOWARD_API AEWardCharacter : public ACharacter
{
    GENERATED_BODY()
    friend class AEWardGameMode;
    friend class AEWardCampaignMode;
public:
    AEWardCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual float TakeDamage(float Damage, FDamageEvent const& Event, AController* Instigator, AActor* Causer) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Arm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> Flashlight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Pistol;
    float Health=100, Stamina=100, SearchProgress=0;
    int32 Rounds=9, Reserve=18;
    bool HasFuse=false, Aiming=false, Dead=false;
    FString Notice;
    float NoticeTime=0;
    UPROPERTY() TObjectPtr<AEWInteractable> Focus;
    void Announce(const FString& Text, float Seconds=4);
protected:
    UPROPERTY() TObjectPtr<AEWInteractable> Held;
    UPROPERTY() TObjectPtr<UAnimSequence> Idle;
    UPROPERTY() TObjectPtr<UAnimSequence> Walk;
    UPROPERTY() TObjectPtr<UAnimSequence> Run;
    UPROPERTY() TObjectPtr<UAnimSequence> CurrentAnimation;
    UPROPERTY() TObjectPtr<USoundBase> ShotSound;
    UPROPERTY() TObjectPtr<USoundBase> StepSound;
    UPROPERTY() TObjectPtr<USoundBase> HurtSound;
    float ShotCooldown=0, ReloadTime=0, Invulnerable=0, StepClock=0, RecoverClock=0;
    bool SprintHeld=false;
    virtual void Forward(float Value); virtual void Right(float Value); virtual void Turn(float Value); virtual void Look(float Value);
    void AimPressed(); void AimReleased(); virtual void Fire(); virtual void Reload();
    void StartSprint(); void StopSprint(); void StartCrouch(); void StopCrouch();
    void StartInteract(); void StopInteract(); void ToggleLight(); virtual void Pause(); void Stats(); void Quit(); void CycleGraphics();
};

UCLASS()
class ECHOWARD_API AEWInteractable : public AActor
{
    GENERATED_BODY()
public:
    AEWInteractable();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    UPROPERTY(EditAnywhere) EWardInteraction Kind=EWardInteraction::Drawer;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Pivot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Hinge;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Moving;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Loot;
    bool Opened=false, Found=false, Used=false;
    float Work=0;
    virtual FString Prompt(const AEWardCharacter* Hero) const;
    virtual FVector FocusPoint() const;
    virtual void Interact(AEWardCharacter* Hero);
    virtual void Hold(AEWardCharacter* Hero, float Delta);
    virtual void Cancel();
private:
    float Blend=0;
    bool NeedsRelease=false;
};

UCLASS()
class ECHOWARD_API AEWardMonster : public ACharacter
{
    GENERATED_BODY()
public:
    AEWardMonster();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual float TakeDamage(float Damage, FDamageEvent const& Event, AController* Instigator, AActor* Causer) override;
    virtual void HearNoise(const FVector& At, float Radius);
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Legs;
    bool Awake=false, Dead=false;
    float Health=100;
private:
    FVector LastKnown=FVector::ZeroVector;
    float Memory=0, Repath=0, Phase=0, Windup=0, AttackCooldown=0, Stun=0;
    bool Winding=false;
    TArray<FVector> Path;
    TArray<FVector> LegPositions;
    UPROPERTY() TObjectPtr<USoundBase> Growl;
    TArray<FVector> FindPath(const FVector& Goal) const;
};

UCLASS()
class ECHOWARD_API AEWardGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AEWardGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    bool PowerOn=false, Won=false;
    float Elapsed=0;
    UPROPERTY() TObjectPtr<AEWardMonster> Monster;
    UPROPERTY() TMap<FName,TObjectPtr<USoundBase>> Effects;
    virtual void BroadcastNoise(const FVector& At, float Radius);
    void TickAssessment(float Delta);
};

UCLASS()
class ECHOWARD_API AEWardHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
