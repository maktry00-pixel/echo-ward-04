#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WardHospitalMaterials.h"
#include "WardCampusFinish67.h"
#include "Engine/StaticMesh.h"
using namespace WardData;

void AEWardBookWorld::Organic(FVector At,FVector Size,int Mat)
{
    const FString Key=FString::Printf(TEXT("organic_%d_%d_%d"),Mat,FMath::FloorToInt(At.X/4800),FMath::FloorToInt(At.Y/4800));
    auto* Group=Groups.FindRef(Key);
    if(!Group)
    {
        Group=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);Group->SetupAttachment(RootComponent);Group->SetMobility(EComponentMobility::Static);
        Group->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/BookWorld/SM_HospitalSky.SM_HospitalSky")));Group->SetMaterial(0,Palette[Mat]);
        Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);Group->SetGenerateOverlapEvents(false);Group->bAutoRebuildTreeOnInstanceChanges=false;
        Group->SetAffectDynamicIndirectLighting(false);Group->SetAffectDistanceFieldLighting(false);Group->SetLightingChannels(true,true,false);
        Group->SetCullDistance(42000);
        Group->RegisterComponent();AddInstanceComponent(Group);Groups.Add(Key,Group);
    }
    Group->AddInstance(FTransform(FQuat::Identity,At,Size/100),false);++MeshInstances;
}

void AEWardBookWorld::BuildEnvironmentDressing()
{
    const Json E=Object(WardBookMapData(),TEXT("environment_dressing"));
    TMap<FString,int> Materials;
    for(const auto& Entry:Object(E,TEXT("materials"))->Values)
    {
        Json Spec=Entry.Value->AsObject();const FVector RGB=Vector(Array(Spec,TEXT("tint")));
        auto* M=WardHospitalMaterials::Make(this,String(Spec,TEXT("source")),FLinearColor(RGB.X,RGB.Y,RGB.Z),Number(Spec,TEXT("roughness"),.75));
        M->SetScalarParameterValue(TEXT("Weathering"),Number(Spec,TEXT("weathering"),.35));
        M->SetScalarParameterValue(TEXT("Roughness"),Number(Spec,TEXT("roughness"),.75));
        M->SetScalarParameterValue(TEXT("Wetness"),Number(Spec,TEXT("wetness"),0));
        M->SetScalarParameterValue(TEXT("Metallic"),Number(Spec,TEXT("metallic"),0));
        Materials.Add(FString(Entry.Key),Palette.Num());Palette.Add(M);
        if(Entry.Key==TEXT("emergency"))EmergencyMaterial=M;
    }
    const auto& Dressing=Array(E,TEXT("boxes"));
    TMap<FString,FVector2D> OpenFenceSpans;
    for(const auto& Entry:Dressing)
    {
        const Json Row=Entry->AsObject();
        if(String(Row,TEXT("kind"))!=TEXT("campus_perimeter_wall"))continue;
        const FVector At=Vector(Array(Row,TEXT("at")))*100;
        const FVector Size=Vector(Array(Row,TEXT("size")))*100;
        if(!FMath::IsNearlyEqual(At.Y,-5000.0,1.0)||Size.X<250||!FMath::IsNearlyEqual(Size.Z,580.0,1.0))continue;
        // The elevated security posts and the short return at the corner still need masonry.
        const double HalfLength=Size.X*.5;
        if(FMath::Abs(At.X-7000)<HalfLength+390||FMath::Abs(At.X-22500)<HalfLength+390)continue;
        OpenFenceSpans.Add(String(Row,TEXT("item_id")),FVector2D(At.X-HalfLength,At.X+HalfLength));
    }
    auto AtOpenFence=[&](const FVector& At)
    {
        if(FMath::Abs(At.Y+5000)>=50)return false;
        for(const auto& Panel:OpenFenceSpans)
            if(At.X>Panel.Value.X-.1&&At.X<Panel.Value.Y+.1)return true;
        return false;
    };
    auto* RustedSteel=WardHospitalMaterials::Variant(this,Palette[Materials.FindRef(TEXT("camp_steel"))]);
    RustedSteel->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.22f,.135f,.099f));
    RustedSteel->SetScalarParameterValue(TEXT("Roughness"),.77f);
    RustedSteel->SetScalarParameterValue(TEXT("Weathering"),.69f);
    const int32 Frame=Palette.Add(RustedSteel);
    const int32 Concrete=Materials.FindRef(TEXT("camp_concrete"));
    TSet<int32> FencePosts;
    for(const auto& Panel:OpenFenceSpans)
    {
        const double Start=Panel.Value.X,End=Panel.Value.Y,Length=End-Start,Centre=(Start+End)*.5;
        Box(FVector(Centre,-5000,23),FVector(Length,32,46),Concrete);
        Box(FVector(Centre,-5000,47),FVector(Length,38,5),Concrete,false);
        // A capsule cannot fit through the lattice. The proxy is confined to its height.
        auto* Fence=NewObject<UBoxComponent>(this);
        Fence->SetupAttachment(RootComponent);
        Fence->SetMobility(EComponentMobility::Static);
        Fence->SetRelativeLocation(FVector(Centre,-5000,125));
        Fence->SetBoxExtent(FVector(Length*.5,6,125));
        Fence->SetCollisionProfileName(TEXT("BlockAll"));
        Fence->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
        Fence->SetGenerateOverlapEvents(false);
        Fence->RegisterComponent();
        AddInstanceComponent(Fence);
        for(double Z:{53.,246.})Box(FVector(Centre,-5000,Z),FVector(Length,5,5),Frame,false);
        for(double Z=68;Z<242;Z+=22)Box(FVector(Centre,-5000,Z),FVector(Length,1.2,1.2),Frame,false);
        const int32 Pickets=FMath::CeilToInt(Length/12.5);
        for(int32 I=1;I<Pickets;++I)
            Box(FVector(Start+Length*I/Pickets,-5000,150),FVector(1.2,1.2,194),Frame,false);
        for(double X:{Start,End,Centre})
        {
            const int32 Key=FMath::RoundToInt(X);
            if(FencePosts.Contains(Key))continue;
            FencePosts.Add(Key);
            Box(FVector(X,-5000,150),FVector(8,8,204),Frame,false);
            Box(FVector(X,-5000,252),FVector(9,9,2),Frame,false);
            Box(FVector(X,-5000,49.5),FVector(15,16,2),Frame,false);
            for(int32 Side:{-1,1})
                Box(FVector(X+Side*5.5,-5000,51),FVector(2.2,2.2,1.4),11,false);
        }
    }
    // Finish the measured furniture kit as a whole. The original pieces still
    // supply the same collision, including clear space between legs.
    TSet<FString> FinishedItems;
    TMap<FString,int32> FinishedBySite;
    int32 FailedItems=0;
    for(const TCHAR* Layer:{TEXT("campus_plan"),TEXT("campus_upgrade"),TEXT("realism_upgrade"),TEXT("finish_upgrade")})
    {
        for(const auto& Entry:Array(Object(WardBookMapData(),Layer),TEXT("items")))
        {
            const Json Item=Entry->AsObject();
            const FString Kind=String(Item,TEXT("kind")), Id=String(Item,TEXT("id"));
            const int32 MeshId=WardVisual67::CampusMesh(Kind);
            const auto& B=Array(Item,TEXT("bounds"));
            if(!MeshId||Id.IsEmpty()||B.Num()!=6||QualityReplacedItems.Contains(Id)||FinishedItems.Contains(Id))continue;

            const int32 Turn=Number(Item,TEXT("turn"));
            const FQuat Facing=FRotator(0,Turn*90,0).Quaternion();
            const FBox Bounds(FVector(B[0]->AsNumber(),B[1]->AsNumber(),B[2]->AsNumber())*100,
                              FVector(B[3]->AsNumber(),B[4]->AsNumber(),B[5]->AsNumber())*100);
            const FVector Bottom(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Min.Z);
            const FBox Local=Bounds.ShiftBy(-Bottom).TransformBy(FTransform(Facing.Inverse()));
            bool Built=false;
            if(Kind==TEXT("visitor_pair")||Kind==TEXT("personal_locker"))
            {
                const double HalfWidth=Local.GetSize().X*(Kind==TEXT("visitor_pair")?.1875:.247);
                const double Offset=Local.GetSize().X*(Kind==TEXT("visitor_pair")?.3125:.25);
                const FBox Single(FVector(-HalfWidth,Local.Min.Y,Local.Min.Z),
                                  FVector(HalfWidth,Local.Max.Y,Local.Max.Z));
                const Json Fit=WardVisual67::FitBounds(Single);
                const bool A=AddCustomProp(MeshId,Fit,Bottom+Facing.RotateVector(FVector(-Offset,0,0)),Turn);
                const bool Second=AddCustomProp(MeshId,Fit,Bottom+Facing.RotateVector(FVector(Offset,0,0)),Turn);
                Built=A&&Second;
            }
            else
            {
                Built=AddCustomProp(MeshId,WardVisual67::FitBounds(Local),Bottom,Turn);
            }

            if(!Built)
            {
                ++FailedItems;
                UE_LOG(LogTemp,Warning,TEXT("VISUAL67_FURNITURE_FALLBACK %s mesh=%d"),*Id,MeshId);
                continue;
            }
            FinishedItems.Add(Id);
            ++FinishedBySite.FindOrAdd(LocationAt(Bottom+FVector(0,0,96)));
        }
    }

    int32 ReplacedParts=0;
    int32 RemovedShrubs=0;
    int32 RemovedForecourtParts=0;
    TArray<TPair<FVector,FVector>> MovedCheckpointLights;
    for(auto V:Dressing)
    {
        Json B=V->AsObject();const FString Mat=String(B,TEXT("material"));
        const FString Kind=String(B,TEXT("kind"));
        if(Kind==TEXT("shrub")&&!Boolean(B,TEXT("solid")))
        {
            ++RemovedShrubs;
            continue;
        }
        if(Kind==TEXT("campus_perimeter_wall")&&OpenFenceSpans.Contains(String(B,TEXT("item_id"))))
        {
            ++RemovedForecourtParts;
            continue;
        }
        if(Kind==TEXT("campus_perimeter_mesh")&&AtOpenFence(Vector(Array(B,TEXT("at")))*100))
        {
            ++RemovedForecourtParts;
            continue;
        }
        if(Kind==TEXT("checkpoint_wall_light"))
        {
            const FVector Old=Vector(Array(B,TEXT("at")))*100;
            if(AtOpenFence(Old))
            {
                const FVector Head(Old.X,-4970,228);
                Box(Head+FVector(0,-15,17),FVector(17,37,5),Frame,false);
                Box(Head+FVector(0,0,10),FVector(17,8,16),Frame,false);
                Box(Head,Vector(Array(B,TEXT("size")))*100,4,false);
                MovedCheckpointLights.Emplace(Old,Head-FVector(0,0,18));
                ++EnvironmentBoxes;
                continue;
            }
        }
        if(Mat==TEXT("road_paint")&&Kind==TEXT("road_dash")){PendingRoadPaint33.Add({Vector(Array(B,TEXT("at")))*100,Vector(Array(B,TEXT("size")))*100});++EnvironmentBoxes;continue;}
        const bool Finished=FinishedItems.Contains(String(B,TEXT("item_id")));
        ReplacedParts+=Finished?1:0;
        const bool Replaced=Finished||QualityReplacedItems.Contains(String(B,TEXT("item_id")))||QualityReplacedItems.Contains(String(B,TEXT("tree_id")));
        if(Replaced)
        {
            // Retain solid trunk/furniture collision, replacing only visuals.
            if(Boolean(B,TEXT("solid"))&&!Cars31Replacements.Contains(String(B,TEXT("item_id"))))Box(Vector(Array(B,TEXT("at")))*100,Vector(Array(B,TEXT("size")))*100,Materials.FindRef(Mat),true,false);
            if(Kind==TEXT("forest_trunk"))++ForestTrees;++EnvironmentBoxes;continue;
        }
        if(Kind==TEXT("grass_tuft"))
        {
            // Replace the old upright rectangles with tapered blade clusters.
            // Keep their ground contact; decorative foliage has no collision.
            const FVector Size=Vector(Array(B,TEXT("size"))),At=Vector(Array(B,TEXT("at")));
            const double Width=FMath::Clamp(FMath::Max(Size.X,Size.Y),.16,.36),Height=FMath::Clamp(Size.Z,.12,.42);
            Json Fit=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Bounds;
            for(double N:TArray<double>{-Width*.5,-Width*.5,0,Width*.5,Width*.5,Height})Bounds.Add(MakeShared<FJsonValueNumber>(N));
            Fit->SetArrayField(TEXT("bounds"),Bounds);
            if(AddCustomProp(501,Fit,FVector(At.X,At.Y,At.Z-Size.Z*.5)*100,FMath::Abs(FMath::RoundToInt(At.X*17+At.Y*11))%4)){++EnvironmentBoxes;continue;}
        }
        if(Mat==TEXT("blood")&&(Kind==TEXT("floor_stain")||Kind==TEXT("wall_stain"))&&AddCustomStain(Vector(Array(B,TEXT("at")))*100,Vector(Array(B,TEXT("size")))*100,Kind==TEXT("wall_stain"))){++EnvironmentBoxes;continue;}
        const int Material=Mat==TEXT("lamp")?(String(B,TEXT("kind"))==TEXT("ceiling_lamp")?WhiteLampMaterial(Vector(Array(B,TEXT("at")))*100):4):Materials.FindRef(Mat);
        if(String(B,TEXT("mesh"))==TEXT("sphere"))Organic(Vector(Array(B,TEXT("at")))*100,Vector(Array(B,TEXT("size")))*100,Material);
        else Box(Vector(Array(B,TEXT("at")))*100,Vector(Array(B,TEXT("size")))*100,Material,Boolean(B,TEXT("solid")));
        if(String(B,TEXT("kind"))==TEXT("forest_trunk"))++ForestTrees;++EnvironmentBoxes;
    }
    UE_LOG(LogTemp,Display,TEXT("VISUAL67_FURNITURE items=%d parts=%d failures=%d"),FinishedItems.Num(),ReplacedParts,FailedItems);
    E->SetNumberField(TEXT("runtime_removed_shrubs"),RemovedShrubs);
    E->SetNumberField(TEXT("runtime_removed_forecourt_parts"),RemovedForecourtParts);
    E->SetNumberField(TEXT("runtime_open_fence_sections"),OpenFenceSpans.Num());
    UE_LOG(LogTemp,Display,TEXT("WARD90 forecourt fence sections=%d old_parts=%d"),OpenFenceSpans.Num(),RemovedForecourtParts);
    UE_LOG(LogTemp,Display,TEXT("WARD90 checkpoint lamps remounted=%d"),MovedCheckpointLights.Num());
    UE_LOG(LogTemp,Display,TEXT("WARD89 decorative shrubs removed=%d"),RemovedShrubs);
    for(const auto& Pair:FinishedBySite)
        UE_LOG(LogTemp,Display,TEXT("VISUAL67_SITE %s furnishings=%d"),*Pair.Key,Pair.Value);

    for(auto V:Array(E,TEXT("white_lights")))
    {
        const FVector At=Vector(V->AsArray())*100;
        if(!MovedCheckpointLights.ContainsByPredicate([&](const auto& Light){return FVector::DistSquared(At,Light.Key)<FMath::Square(65.0);}))
            LightAnchors.Add(At);
    }
    for(const auto& Light:MovedCheckpointLights)LightAnchors.Add(Light.Value);
    for(auto V:Array(E,TEXT("red_lights")))EmergencyAnchors.Add(Vector(V->AsArray())*100);
    EnvironmentCameras=Number(E,TEXT("camera_count"));
    for(int I=0;I<4;++I)
    {
        auto* L=NewObject<UPointLightComponent>(this);L->SetupAttachment(RootComponent);L->SetMobility(EComponentMobility::Movable);
        L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(2200);L->SetAttenuationRadius(1300);L->SetLightColor(FLinearColor(1,.022f,.011f));
        L->SetCastShadows(false);L->SetLightingChannels(true,true,false);L->SetSourceRadius(16);L->RegisterComponent();AddInstanceComponent(L);EmergencyPool.Add(L);
    }
    // Uses the already-cooked sphere: a visible, non-blocking moon inside the
    // sky shell. Its own material is independent of the inspection fill light.
    auto* MoonMat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalSurface.M_HospitalSurface")),this);
    MoonMat->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.82f,.63f,.50f));MoonMat->SetVectorParameterValue(TEXT("Emission"),FLinearColor(11800,9000,7400));
    VisibleMoon=NewObject<UStaticMeshComponent>(this);VisibleMoon->SetupAttachment(RootComponent);
    VisibleMoon->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/BookWorld/SM_HospitalSky.SM_HospitalSky")));
    VisibleMoon->SetMaterial(0,MoonMat);VisibleMoon->SetRelativeLocation(FVector(100000,-120000,110000));VisibleMoon->SetRelativeScale3D(FVector(46));
    VisibleMoon->SetCollisionEnabled(ECollisionEnabled::NoCollision);VisibleMoon->SetCastShadow(false);VisibleMoon->SetAffectDynamicIndirectLighting(false);VisibleMoon->SetAffectDistanceFieldLighting(false);
    VisibleMoon->RegisterComponent();AddInstanceComponent(VisibleMoon);
}
