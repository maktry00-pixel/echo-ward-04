#pragma once
#include "CoreMinimal.h"
#include "WardPrototype.h"
#include "WardCampaignAI.h"
#include "WardCampaign.generated.h"

class UPrimitiveComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UAudioComponent;
class ACameraActor;
class USoundAttenuation;
class ULightComponent;
class UPointLightComponent;
class FWardCampaignUI;
class AEWardCampaignMode;
class AEWardPhysicsProp;
class AEWardTitleScene;
class AEWardBookWorld;
class AEWardHospitalGameplay;

UCLASS()
class ECHOWARD_API AEWardCampaignProjectile : public AActor
{
    GENERATED_BODY()
public:
    AEWardCampaignProjectile();
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    FVector Velocity;
    float Age=0;
    bool Puddle=false;
    virtual void Tick(float Delta)override;
};

UCLASS()
class ECHOWARD_API AEWardCampaignObject : public AEWInteractable
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) FString ObjectId;
    UPROPERTY(EditAnywhere) FString ObjectKind;
    UPROPERTY(EditAnywhere) FString Descriptor;
    bool Locked=false,Broken=false,NeedsRelease=false;
    int32 Value=0,Selected=0,Searched=0,Reward=2,Barricade=0;
    float Motion=0,Duration=0;
    FVector Stance;
    FTransform RestHinge;
    WardData::Json Definition;
    TArray<TObjectPtr<UPrimitiveComponent>> Parts,MovingCollision;
    TArray<FString> PartPaths;
    TArray<bool> InitialVisibility;
    TArray<TObjectPtr<UPrimitiveComponent>> CollisionParts;
    TArray<TEnumAsByte<ECollisionEnabled::Type>> InitialCollision;
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual FString Prompt(const AEWardCharacter* Hero)const override;
    virtual FVector FocusPoint()const override;
    virtual void Interact(AEWardCharacter* Hero) override;
    virtual void Hold(AEWardCharacter* Hero,float Delta) override;
    virtual void Cancel() override;
    void ResetState();
    void ToggleDoor();
    void SetDoor(bool Open,bool Lock);
    void SetContents(bool Visible);
    void Break();
    bool HasStorage()const;
    void RefreshVisuals();
    WardData::Json Snapshot()const;
    void Restore(WardData::Json Data);
private:
    bool MovingPart(const FString& Path)const;
    FString MovingPath,ContentsPath;
    bool HingeBlocked()const;
};

UCLASS()
class ECHOWARD_API AEWardCampaignHero : public AEWardCharacter
{
    GENERATED_BODY()
public:
    AEWardCampaignHero();
    virtual void SetActorHiddenInGame(bool Hidden) override;
    void InitializeAcceptedHero50();
    void UpdateFirstPerson50();
    void UpdateLocomotion52(float Delta);
    void TickLocomotionAssessment52(float Delta);
    FVector LastGroundPosition52=FVector::ZeroVector,ViewMotion52=FVector::ZeroVector;
    FVector WeaponMotion52=FVector::ZeroVector;
    FRotator LastView52=FRotator::ZeroRotator,WeaponSway52=FRotator::ZeroRotator;
    float GaitPhase52=0,StrideTravel52=0,MotionWeight52=0,Landing52=0,LastVertical52=0;
    bool Grounded52=false,MotionInitialized52=false;
    int Footsteps52=0;
    FString StepSurface52=TEXT("tile");
    FTransform FirstPersonWeaponPose50(float Aim,float Close) const;
    UPROPERTY() TObjectPtr<class UChildActorComponent> BekaVisual;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> BekaBody;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> BekaFace;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> BekaOutfit;
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual float TakeDamage(float Damage,FDamageEvent const& Event,AController* Instigator,AActor* Causer) override;
    virtual void Fire() override;
    virtual void Reload() override;
    virtual void Pause() override;
    virtual void Forward(float Value) override;
    virtual void Right(float Value) override;
    virtual void Look(float Value) override;
    virtual void Turn(float Value) override;
    FTransform MeshRest;
    void UpdateTorchTransform();
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Equipment;
    UPROPERTY() TObjectPtr<UPointLightComponent> MuzzleLight;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> MuzzleFlame;
    float MuzzleRemaining=0;
    TArray<FString> EquipmentPaths;
    TArray<FTransform> EquipmentRest;
    void BuildEquipment();
    FTransform WeaponPose()const;
    FVector WeaponGrip(bool Left)const;
    FTransform WeaponPoseAt(float AimBlend,float CloseBlend)const;
    bool WeaponHasClearance()const;
    float MagazineTravel()const;
    float ReloadBlend()const;
    bool SprintExhausted=false;
    float WeaponKick=0,WeaponAimBlend=0,WeaponCloseBlend=0;
    bool WeaponObstructed=false;
};

struct FWardCreaturePart
{
    UHierarchicalInstancedStaticMeshComponent* Group=nullptr;
    int32 Instance=0,Leg=-1;
    FTransform Rest;
    FVector Pivot;
    FString Path;
};

UCLASS()
class ECHOWARD_API AEWardCampaignEnemy : public AEWardMonster
{
    GENERATED_BODY()
public:
    UPROPERTY() FString EnemyId;
    WardData::Json Definition;
    bool Active=false,Blind=false,Crawler=false,Spitter=false,Tactical=false,Guardian=false,Ceiling=false;
    float MaxHealth=150,Stunned=0,Memory=0,Clock=0,Gait=0,AttackTime=0,AttackCooldown=0,PathClock=0,PoseClock=0,DeathTime=0,Exposed=0,ChargeTime=0,BedSearch=0;
    FVector ChargeHeading;
    int32 PatrolIndex=0,Injuries=0;
    FVector Observed;
    TArray<FVector> Patrol,Route;
    TArray<FWardCreaturePart> Geometry;
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual float TakeDamage(float Damage,FDamageEvent const& Event,AController* Instigator,AActor* Causer) override;
    virtual void HearNoise(const FVector& At,float Radius) override;
    void Wake();
    void ResetEnemy();
    void Animate(float Delta);
    WardData::Json Snapshot()const;
    void Restore(WardData::Json Data);
};

UCLASS()
class ECHOWARD_API AEWardCampaignMode : public AEWardGameMode
{
    GENERATED_BODY()
public:
    AEWardCampaignMode();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;
    virtual void Tick(float Delta) override;
    virtual void BroadcastNoise(const FVector& At,float Radius) override;
    UPROPERTY() TMap<FString,TObjectPtr<AEWardCampaignObject>> Objects;
    UPROPERTY() TArray<TObjectPtr<AEWardCampaignEnemy>> Enemies;
    UPROPERTY() TObjectPtr<ACameraActor> AdminCamera;
    UPROPERTY() TObjectPtr<AEWardTitleScene> TitleScene;
    UPROPERTY() TObjectPtr<AEWardBookWorld> BookWorld;
    UPROPERTY() TObjectPtr<AEWardHospitalGameplay> HospitalStory;
    UPROPERTY() TObjectPtr<AActor> PhoneActor76;
    UPROPERTY() TObjectPtr<AActor> ScareActor76;
    bool PhoneOpen76=false;
    float PhoneRaise76=0,ScareClock76=0,ScareCooldown76=24,TutorialClock76=0,ObjectiveFade76=0;
    FString PhonePage76=TEXT("home"),HeldSlot76,PreviousObjective76,CompletedObjective76;
    int LastCompleted76=0;
    bool BookWorldEnabled=true;
    bool TitleView=false,TitleActionExecuting=false;
    UPROPERTY() TObjectPtr<UAudioComponent> Music;
    UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
    UPROPERTY() TObjectPtr<UAudioComponent> Pursuit;
    UPROPERTY() TObjectPtr<UAudioComponent> Voice;
    UPROPERTY() TObjectPtr<UAudioComponent> RefugeMusic;
    UPROPERTY() TObjectPtr<USoundAttenuation> SpatialSound;
    UPROPERTY() TObjectPtr<AEWardCampaignObject> HidingBed;
    WardData::Json Data,Flags,Options;
    TMap<FString,TArray<WardData::Json>> Visuals,Colliders;
    TMap<FString,int32> Items,Values,Stash;
    TMap<TWeakObjectPtr<ULightComponent>,float> LightLevels;
    TMap<FString,TPair<FString,FString>> Translations;
    mutable TMap<FString,FString> TranslationCache;
    TSet<FString> Completed,Visited,AutoVisited;
    TArray<FString> Journal,AgentRoute;
    FWardCampaignGrid Grid;
    TSharedPtr<FWardCampaignUI> Interface;
    FString Screen=TEXT("title"),Language=TEXT("ru"),Difficulty=TEXT("normal"),CurrentRoom,Fault;
    FString InspectItem,LastSave,AgentStatus,SaveFolderOverride;
    bool CampaignStarted=false,Admin=false,Agent=false,PendingRespawn=false,Assessment=false;
    float Invulnerable=0,ReloadTime=0,ShotCooldown=0,DashCooldown=0,DashTime=0,Noise=0,Breath=3,SaveIcon=0,RespawnTime=0;
    float ReactorTime=45,GeneratorTime=0,EscapeTime=-1,AgentClock=0,AgentWait=0,WorldCueTime=10;
    int32 LoadedShells=0,FlashCharges=1,AgentIndex=0,Score=0,NextAuto=1,Shots=0,Hits=0,Kills=0,Replay=0,Ending=0;
    bool Shotgun=false,ShotgunReload=false;
    FVector DashHeading,HideExit;
    FRotator AdminViewRotation;
    TArray<FVector> AgentPath;
    AEWardCampaignHero* Hero()const;
    bool Flag(const FString& Key)const;
    void Flag(const FString& Key,bool Value);
    double NumberFlag(const FString& Key,double Default=0)const;
    void NumberFlag(const FString& Key,double Value);
    FString StringFlag(const FString& Key)const;
    void StringFlag(const FString& Key,const FString& Value);
    FString Tr(const FString& Russian)const;
    FString ItemName(const FString& Id)const;
    double Setting(const FString& Key,double Default=1)const;
    bool SettingBool(const FString& Key,bool Default=true)const;
    float DifficultyValue(const FString& Key)const;
    void NewGame(const FString& Id);
    void Show(const FString& Page);
    void Command(const FString& Id);
    void ApplySettings(bool Persist=true);
    void SetNumberSetting(const FString& Key,double Value,bool Persist=true);
    void CycleSetting(const FString& Key,int Direction=1);
    void Notify(const FString& Text,float Seconds=5);
    void Cue(const FString& Id,FVector At,float Volume=1);
    void Use(AEWardCampaignObject* Object,bool FinishedWork=false);
    float WorkDuration(const FString& Id)const;
    FString Prompt(AEWardCampaignObject* Object)const;
    void Finish(const FString& Id);
    void SyncDoors(bool Restore=false);
    void Wake(int32 Index);
    void Open(const FString& Id,bool Open=true,bool Locked=false);
    void TickQuests(float Delta);
    void TickAtmosphere(float Delta);
    void StashTransfer(const FString& Id,bool Deposit);
    FString Goal()const;
    FString Objective()const;
    FString RoomName()const;
    void TickHero(float Delta);
    void FireWeapon();void ReloadWeapon();void Knife();void Dash();void Flash();void ThrowBottle();void Heal();void Battery();
    void DamageHero(float Damage,AActor* Source);
    void DamageAt(FVector From,FVector Direction,float Power,bool Pellet=false);
    bool CanTake(const FString& Id,int32 Amount=1)const;
    bool Take(const FString& Id,int32 Amount=1);
    static constexpr int32 InventoryCapacity=12;
    TArray<TPair<FString,int32>> Inventory()const;
    void Drop(const FString& Id);
    void Consume(const FString& Id,int32 Amount=1);
    AEWardCampaignObject* SpawnSupply(const FString& Id,int32 Amount,FVector At,const FString& Name=TEXT(""));
    void ToggleHide(AEWardCampaignObject* Bed);
    void Barricade();
    WardData::Json Capture()const;
    bool ValidateSave(WardData::Json Save)const;
    bool Restore(WardData::Json Save);
    bool Save(const FString& Slot,bool Automatic=false);
    bool Load(const FString& Slot);
    bool LoadLatest();
    TArray<FString> SaveSlots()const;
    WardData::Json ReadSlot(const FString& Slot)const;
    FString SaveDirectory()const;
    void AutoSave();
    void ImportGodotSaves();
    void TickAutoSave(float Delta);
    struct FPreview51 {FString Id,Title,Kind,Visual;bool Document=false;};
    TArray<FPreview51> PreviewCatalog51() const;
    bool PreviewCommand51(const FString& Id);
    bool HoldPreview51(const FString& Id);
    void ClearPreview51();
    void UpdatePreview51();
    void TickBodyPreviewAssessment51(float Delta);
    bool AdminHands51=false;
    FString PreviewItem51,PreviewDocument51;
    UPROPERTY() TObjectPtr<AActor> PreviewActor51;
    void ToggleAdmin();void ToggleAgent();void TickAdmin(float Delta);void TickAgent(float Delta);
    void UpdateBlocked();void AllocateResources();
    void TickCampaignAssessment(float Delta);
    void TickSaveMenuAssessment(float Delta);
    void TickAboutMenuAssessment(float Delta);
    void TickTrainingMenuAssessment(float Delta);
    void TickSettingsMenuAssessment(float Delta);
    void TickDifficultyMenuAssessment(float Delta);
    void TickPauseMenuAssessment(float Delta);
    void TickHUDMenuAssessment(float Delta);
    void TickFieldMenuAssessment(float Delta);
    void TickBookMapAssessment(float Delta);
    void TickBookWorldAssessment(float Delta);
    void TickGuideMediaCapture(float Delta);
    void TickCampaignBenchmark(float Delta);
    void TickHospitalBenchmark(float Delta);
    void TickFirstPersonAssessment50(float Delta);
    void CheckRegression(TFunctionRef<void(const FString&,bool)> Check);
    int32 BottleImpacts=0;
    UPROPERTY() TObjectPtr<UMaterialInterface> BrassMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> ChipMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> BottleMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> BulletMarkMaterial;
    TArray<TWeakObjectPtr<AEWardPhysicsProp>> PhysicsProps;
    TArray<TWeakObjectPtr<AActor>> ImpactMarks;
    AEWardPhysicsProp* SpawnPhysics(const FString& Kind,FVector At,FVector Velocity,FVector Size=FVector(2),float Mass=.1f,UMaterialInterface* Material=nullptr);
    void SurfaceImpact(FVector At,FVector Normal,bool Flesh=false,bool Glass=false);
    void ClearPhysics();
    void TickPhysicsAssessment(float Delta);
};

UCLASS()
class ECHOWARD_API AEWardCampaignHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    WardData::Json LastFrame;
};

AEWardCampaignMode* WardCampaign(const UObject* Context);
