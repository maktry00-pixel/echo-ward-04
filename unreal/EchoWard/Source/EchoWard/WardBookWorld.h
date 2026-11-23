#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WardCampaignData.h"
#include "WardBookWorld.generated.h"

class AEWardCampaignMode;
class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UDirectionalLightComponent;
class UMaterialInstanceDynamic;
class UBoxComponent;
class ULocalFogVolumeComponent;

struct FWardBookRoom3D
{
    FString Id,Name,Location;
    FBox Bounds;
    FVector Door,Inside,Outside;
    bool Enclosed=true;
    double MinimumArea=0;
};
struct FWardBookSite3D
{
    FString Id,Name;
    FVector Origin;
    bool Outdoor=false;
    FBox Bounds;
    FVector Capture;
    FRotator View;
};
struct FWardBookCell
{
    int Room=INDEX_NONE;
    bool Outdoor=false;
};

struct FWardCarPlacement33
{
    FString Id,Slug;
    WardData::Json Spec;
    FTransform Transform;
    FBox Local;
    TArray<TPair<UHierarchicalInstancedStaticMeshComponent*,int32>> Parts;
    UBoxComponent* Collider=nullptr;
    bool Fitted=false;
};

UCLASS()
class ECHOWARD_API AEWardBookWorld : public AActor
{
    GENERATED_BODY()
public:
    UMaterialInterface* SurfaceMaterial(int Index) const {return Palette.IsValidIndex(Index)?Palette[Index].Get():nullptr;}
    AEWardBookWorld();
    void Build(AEWardCampaignMode* Campaign);
    void UpdateLighting(FVector Player);
    void Update(float Delta);
    void SetBright(bool Enabled);
    void UpdateHorrorAtmosphere(float Delta);
    void BuildHorrorPresentation28();
    void BuildFacilities37();
    WardData::Json AuditFacilities37();
    bool DeferProp37(int Id,WardData::Json Model,FVector At,int Turn);
    void UpdateDeferred37(float Delta);
    struct FDeferred37 {int Id=0,Turn=0;WardData::Json Model;FVector At;bool Built=false;};
    TArray<FDeferred37> Deferred37;
    bool RenderingDeferred37=false;
    float DeferredClock37=0;
    int DeferredInitial37=0,DeferredBuilt37=0,DeferredNearRemaining37=0,FacilitiesProps37=0,FacilitiesFailures37=0;
    TSet<UHierarchicalInstancedStaticMeshComponent*> DeferredTouched37;
    TArray<FVector> FacilitiesFloor37;
    TArray<FString> DoorHeaderRepairs37;

    WardData::Json HorrorProfile(const FString& Region)const;
    int HorrorTraces28=0,HorrorMarkers28=0;
    UPROPERTY() TObjectPtr<UPointLightComponent> HorrorBounce28;
    int WhiteLampMaterial(FVector At)const;
    float LampPower(FVector At,float Time)const;
    int DormantLampMaterial=INDEX_NONE,FlickerLampMaterial=INDEX_NONE;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlickerLamp;
    UPROPERTY() TArray<TObjectPtr<ULocalFogVolumeComponent>> FloorMist;
    TArray<FVector> ActiveWhiteAnchors,MistAnchors;
    TArray<int> ActiveWhiteIds;
    int AmbientEventCount=0;
    FString LastAmbientCue;
    FVector LastAmbientAt=FVector::ZeroVector;
    float LightSelectionClock=1;
    WardData::Json AuditEnvelope()const;
    WardData::Json AuditEveryDoor();
    bool Bright=true;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> Daylight;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkyMaterial;
    void UseLift();
    void SelectLift(int Direction);
    bool RequestLift(int Floor);
    void RestoreLift(FVector Player);
    bool InsideLift(FVector Player)const;
    bool LiftMoving=false;
    int LiftFloor=2,LiftTarget=2,LiftSelection=3;
    float LiftZ=0,LiftDoor=1,LiftWait=0;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LiftPlatform;
    UPROPERTY() TObjectPtr<USceneComponent> LiftCabin;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> LiftDoors;
    UPROPERTY() TObjectPtr<UTextRenderComponent> LiftDisplay;
    FString Describe(FVector Player)const;
    FString LocationAt(FVector Player)const;
    bool ValidPosition(FVector Player)const;
    FVector Start()const;
    void UseArchitecture();
    void ToggleRefugeBolt();
    bool ToggleBoltForZone(const FString& Zone,FVector Player);
    bool PushMovable(int Index,FVector Direction);
    bool UseMovable(FVector Player);
    bool UseFurnitureHide(FVector Player);
    bool FurnitureHiding=false;
    bool FurnitureExitCrouched=false;
    FVector FurnitureExit;
    void ResetMovables();
    struct FMovable { FString Id,Type; FVector Half,Home; float Noise=900; bool Pushable=true; TObjectPtr<UBoxComponent> Body=nullptr; };
    TArray<FMovable> Movables;
    FString EnvironmentState=TEXT("clean");
    TSet<FString> RefugeBolts;
    void SetStoryChapter(int Chapter);
    WardData::Json ArchitectureSnapshot()const;
    void RestoreArchitecture(WardData::Json State);
    int StoryChapter=0;
    float SoundClock=5;
    FString SoundRegion;
    struct FDoorPart { int Material=0,Index=0,Leaf=0; FVector Centre,Size; };
    struct FPortal { FString Id,Type,Zone,Side,Role; FVector At; float Width=200,OpenSign37=1; bool Open=true,Locked=false,InsideOnly=false; TObjectPtr<UStaticMeshComponent> Leaf=nullptr; TArray<FDoorPart> VisibleParts; };
    TArray<FPortal> Portals;
    void RefreshPortalVisual(FPortal& Portal);
    UPROPERTY() TMap<int,TObjectPtr<UInstancedStaticMeshComponent>> DoorVisualGroups;
    int VisibleDoorLeaves=0,ForestTrees=0;
    TMap<FString,int> SiteWall,SiteLower;
    TArray<FWardBookRoom3D> Rooms;
    TArray<FWardBookSite3D> Sites;
    TMap<FIntVector,FWardBookCell> Cells;
    TArray<FVector> LightAnchors;
    int StairFlights=0,MeshInstances=0;
    int PrototypeProps=0,PrototypeParts=0;
    int DetailedFurniture=0,DetailedFurnitureBoundsFailures=0;
    int CustomFurniture=0,CustomBoundsFailures=0,CustomDoorLeaves=0,CustomDecals=0,UnresolvedCustomProps=0;
    TSet<int> CustomModelIds;
    int QualityProps=0,QualityTrees=0,QualityWindows=0,RealProps=0,RealSolidProps=0,QualityWarmMaterial=INDEX_NONE,QualityFacade=INDEX_NONE,QualityTile=INDEX_NONE;
    TSet<FString> QualityReplacedItems;
    TArray<FVector> WarmAnchors;
    UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> WarmPool;
    void BuildQualityDressing();
    void BuildDetailDressing();
    int DetailProps=0,DetailWallProps=0,DetailOpeningProps=0,DetailExteriorProps=0;
    int Detail27Props=0,Detail27Supported=0,Detail27Services=0,Detail27Wall=0;
    bool AddingDetail27=false;
    int Realism30Props=0;
    void BuildCars31();
    void FitCarsAndMarkings33();
    void UpdateCars33(float Delta);
    WardData::Json VehicleAudit33;
    TArray<FWardCarPlacement33> VehiclePlacements33;
    TArray<TPair<FVector,FVector>> PendingRoadPaint33;
    TSet<UHierarchicalInstancedStaticMeshComponent*> NearCars33;
    float CarQualityClock33=1;
    int RoadPaintCount33=0,RoadPaintRejected33=0;
    bool FinishingRoadPaint33=false;

    int Cars31Count=0;
    TSet<FString> Cars31Sources,Cars31Replacements;
    TArray<FBox> Cars31Bounds;
    void UpdateQualityLights(FVector Player,bool OpenAir);
    bool AddCustomProp(int Id,WardData::Json Model,FVector At,int Turn);
    bool AddCustomStain(FVector At,FVector Size,bool Wall);
    int EnvironmentProps=0,EnvironmentBoxes=0,EnvironmentCameras=0;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> VisibleMoon;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EmergencyMaterial;
    UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> EmergencyPool;
    TArray<FVector> EmergencyAnchors;
    float EmergencyClock=0;
    UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> LightPool;
    TMap<FString,UHierarchicalInstancedStaticMeshComponent*> Groups;
private:
    void Box(FVector Centre,FVector Size,int Material,bool Collision=true,bool Visible=true);
    void Organic(FVector Centre,FVector Size,int Material);
    void Sign(const FString& Text,FVector At,FRotator Rotation,float Size=22);
    void BuildHospitalDetails();
    void BuildStoryArchitecture();
    void BuildPrototypeFurnishings();
    void BuildEnvironmentDressing();
    void BuildMovables();
    bool FloorHole(int X,int Y,int Z)const;
    bool StairVoid(int X,int Y)const;
    void BuildLift();
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> Palette;
    TWeakObjectPtr<AEWardCampaignMode> Mode;
};
