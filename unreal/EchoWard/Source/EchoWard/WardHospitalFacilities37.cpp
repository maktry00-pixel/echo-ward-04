#include "WardBookWorld.h"
#include "WardCampaign.h"
#include "WardHospitalGameplay.h"
#include "WardEchoAssets.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
using namespace WardData;
namespace {
Json Facilities37(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/hospital_environment_v37.json"));return D;}
}
bool AEWardBookWorld::DeferProp37(int Id,Json Model,FVector At,int Turn)
{
    if(RenderingDeferred37||Id==121||Id>=2000||Id>=640&&Id<=644)return false;
    const FVector Near=Mode.IsValid()&&Mode->Hero()?Mode->Hero()->GetActorLocation():FVector(5000,-6100,98);
    if(FVector::DistSquared2D(At,Near)<FMath::Square(9500.f)&&FMath::Abs(At.Z-Near.Z)<600)return false;
    static TMap<int,Json> Metadata;
    if(Metadata.IsEmpty())for(auto V:Array(WardEchoAssets::Data(),TEXT("meshes")))Metadata.Add(Number(V->AsObject(),TEXT("id")),V->AsObject());
    const Json Meta=Metadata.FindRef(WardEchoAssets::ResolvedId(Id));const auto& Bounds=Array(Meta,TEXT("bounds_cm"));
    if(Bounds.Num()!=2||Array(Model,TEXT("bounds")).Num()!=6)return false;
    const FVector Size=Vector(Bounds[1]->AsArray())-Vector(Bounds[0]->AsArray());if(Size.GetMin()<.01)return false;
    Deferred37.Add({Id,Turn,Model,At,false});++DeferredInitial37;++CustomFurniture;++MeshInstances;CustomModelIds.Add(Id);return true;
}
void AEWardBookWorld::UpdateDeferred37(float Delta)
{
    DeferredClock37-=Delta;if(Deferred37.IsEmpty()||DeferredClock37>0||!Mode.IsValid()||!Mode->Hero())return;DeferredClock37=.05f;
    const FVector P=Mode->Hero()->GetActorLocation();TArray<int> Near;
    for(int I=0;I<Deferred37.Num();++I){const auto& Q=Deferred37[I];if(FVector::DistSquared2D(Q.At,P)<FMath::Square(10000.f)&&FMath::Abs(Q.At.Z-P.Z)<600)Near.Add(I);}
    if(Near.IsEmpty()){DeferredNearRemaining37=0;DeferredClock37=.20f;return;}
    Near.Sort([&](int A,int B){return FVector::DistSquared(Deferred37[A].At,P)<FVector::DistSquared(Deferred37[B].At,P);});
    RenderingDeferred37=true;
    const int32 Count=FMath::Min(32,Near.Num());
    for(int J=0;J<Count;++J)
    {
        const auto& Q=Deferred37[Near[J]];--CustomFurniture;--MeshInstances;
        if(AddCustomProp(Q.Id,Q.Model,Q.At,Q.Turn))++DeferredBuilt37;else {++FacilitiesFailures37;UE_LOG(LogTemp,Error,TEXT("FACILITIES37_DEFERRED_FAILED %d %s"),Q.Id,*Q.At.ToString());}
    }
    DeferredNearRemaining37=Near.Num()-Count;
    Near.SetNum(Count);
    Near.Sort(TGreater<int32>());
    for(const int32 Index:Near)Deferred37.RemoveAtSwap(Index,1,EAllowShrinking::No);
    if(Deferred37.IsEmpty())Deferred37.Empty();
    RenderingDeferred37=false;
    for(auto* G:DeferredTouched37){G->bAutoRebuildTreeOnInstanceChanges=true;G->BuildTreeIfOutdated(true,true);}DeferredTouched37.Reset();
}
void AEWardBookWorld::BuildFacilities37()
{
    const Json D=Facilities37();if(Number(D,TEXT("version"))!=37)return;
    // Separate departmental identity without replacing the approved grey/blue patina.
    for(const auto& Site:Sites)
    {
        const Json Profile=Object(Object(D,TEXT("profiles")),Site.Id);
        for(bool Lower:{false,true})
        {
            const int Index=Lower?SiteLower.FindRef(Site.Id):SiteWall.FindRef(Site.Id);auto* Mat=Cast<UMaterialInstanceDynamic>(SurfaceMaterial(Index));if(!Mat)continue;
            const auto& C=Array(Profile,Lower?TEXT("lower"):TEXT("wall"));
            if(C.Num()==3)Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor(C[0]->AsNumber(),C[1]->AsNumber(),C[2]->AsNumber()));
            Mat->SetScalarParameterValue(TEXT("Grime"),FMath::Min(.62,Number(Profile,TEXT("grime"),.25)*(Lower?1.65:1.3)));Mat->SetScalarParameterValue(TEXT("Desaturate"),.94f);
        }
    }
    // A dry base plus water patches beneath leaks, instead of a uniformly glossy ward.
    if(auto* Floor=Cast<UMaterialInstanceDynamic>(SurfaceMaterial(0))){Floor->SetScalarParameterValue(TEXT("Wetness"),Number(D,TEXT("base_floor_wetness"),.12));Floor->SetScalarParameterValue(TEXT("Roughness"),.91f);}
    TMap<int,UStaticMesh*> Meshes;
    for(auto V:Array(D,TEXT("props")))
    {
        const Json Q=V->AsObject();const int Id=Number(Q,TEXT("mesh"));UStaticMesh* Mesh=Meshes.FindRef(Id);
        if(!Mesh){Mesh=Id>=3300?LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Campaign/HospitalV37/Meshes/SM_H37_%d.SM_H37_%d"),Id,Id)):WardEchoAssets::Mesh(Id);Meshes.Add(Id,Mesh);}
        if(!Mesh){++FacilitiesFailures37;continue;}
        const FVector At=Vector(Array(Q,TEXT("at")))*100;const FQuat Rotation=FRotator(0,Number(Q,TEXT("yaw")),Number(Q,TEXT("roll"))).Quaternion();
        const FBox Source=Mesh->GetBoundingBox(),Rotated=Source.TransformBy(FTransform(Rotation));FVector Scale(1);
        const auto& Fit=Array(Q,TEXT("size_override"));
        if(Fit.Num()==3){Scale=Vector(Fit)*100/Rotated.GetSize();if(FMath::RoundToInt(Number(Q,TEXT("yaw"))/90)%2)Swap(Scale.X,Scale.Y);}
        const FBox B=Source.TransformBy(FTransform(Rotation,FVector::ZeroVector,Scale));const FVector Shift=At+FVector(0,0,B.GetSize().Z*.5)-B.GetCenter();
        const FTransform T(Rotation,Shift,Scale);const FBox World=Source.TransformBy(T);
        const FString Key=FString::Printf(TEXT("h37_%d_%d_%d_%d"),Id,FMath::FloorToInt(At.X/2400),FMath::FloorToInt(At.Y/2400),FMath::FloorToInt(At.Z/480));
        auto* G=Groups.FindRef(Key);
        if(!G){G=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);G->SetupAttachment(RootComponent);G->SetMobility(EComponentMobility::Static);G->SetStaticMesh(Mesh);G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetGenerateOverlapEvents(false);G->SetAffectDynamicIndirectLighting(false);G->SetAffectDistanceFieldLighting(false);G->SetLightingChannels(true,true,false);G->SetCastShadow(Id!=3324&&Id!=643);G->SetCullDistance(Id==3317?8500:Id==3345?6500:7000);G->bAutoRebuildTreeOnInstanceChanges=false;G->RegisterComponent();AddInstanceComponent(G);Groups.Add(Key,G);}
        G->AddInstance(T,false);++FacilitiesProps37;++MeshInstances;
        if(Boolean(Q,TEXT("solid"))){Box(World.GetCenter(),World.GetSize(),11,true,false);FacilitiesFloor37.Add(FVector(World.GetCenter().X,World.GetCenter().Y,World.Min.Z));}
    }
    for(auto V:Array(D,TEXT("light_anchors"))){const FVector P=Vector(V->AsArray())*100;WarmAnchors.Add(P);EmergencyAnchors.Add(P+FVector(65,0,15));}
    for(auto V:Array(D,TEXT("cold_light_anchors")))LightAnchors.Add(Vector(V->AsArray())*100);
    // New traces share the tested masked PBR material and sit 1 mm above the slab.
    for(auto V:Array(D,TEXT("traces")))
    {
        const Json Q=V->AsObject();auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/HorrorV28/SM_TracePlane.SM_TracePlane"));const FString Kind=String(Q,TEXT("kind"));
        auto* Mat=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Campaign/HorrorV28/MI_%s.MI_%s"),*Kind,*Kind));if(!Mesh||!Mat){++FacilitiesFailures37;continue;}
        auto* G=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);G->SetupAttachment(RootComponent);G->SetMobility(EComponentMobility::Static);G->SetStaticMesh(Mesh);G->SetMaterial(0,Mat);G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetCastShadow(false);G->SetAffectDynamicIndirectLighting(false);G->SetAffectDistanceFieldLighting(false);G->SetCullDistance(3200);G->SetLightingChannels(true,true,false);G->RegisterComponent();AddInstanceComponent(G);
        const FVector P=Vector(Array(Q,TEXT("at")))*100,S=Vector(Array(Q,TEXT("size")))*100;G->AddInstance(FTransform(FQuat::Identity,P,FVector(S.X/100,S.Y/100,1)),false);Groups.Add(String(Q,TEXT("id")),G);++MeshInstances;
    }
    // Fractional internal partitions leave a full-height gap for a portal.
    // The metre-grid shell supplies headers only at its own room boundaries.
    // Close only uncovered indoor transoms; never cap outdoor gates or crawlways.
    FCollisionQueryParams HeaderQ(SCENE_QUERY_STAT(FacilitiesDoorHeaders37),false,Mode->Hero());FHitResult HeaderHit;
    for(const auto& Portal:Portals)
    {
        if(Portal.Role==TEXT("niche")||Portal.Type==TEXT("shutter"))continue;
        const bool NS=Portal.Side==TEXT("north")||Portal.Side==TEXT("south");const FVector N=NS?FVector(0,1,0):FVector(1,0,0),Along=NS?FVector(1,0,0):FVector(0,1,0);
        const FVector CeilingProbe=Portal.At+N*40;
        if(!GetWorld()->LineTraceSingleByChannel(HeaderHit,CeilingProbe+FVector(0,0,370),CeilingProbe+FVector(0,0,410),ECC_Pawn,HeaderQ))continue;
        const float Top=HeaderHit.ImpactPoint.Z-Portal.At.Z;
        if(Top<375||Top>400)continue;
        bool Uncovered=false;
        for(float U:{-.32f,0.f,.32f})
        {
            const FVector At=Portal.At+Along*(Portal.Width*U)+FVector(0,0,320);
            if(!GetWorld()->LineTraceSingleByChannel(HeaderHit,At-N*40,At+N*40,ECC_Pawn,HeaderQ)){Uncovered=true;break;}
        }
        if(!Uncovered)continue;
        int Material=1;for(const auto& Room:Rooms)if(Room.Id==Portal.Zone){Material=SiteWall.FindRef(Room.Location);break;}
        Box(Portal.At+FVector(0,0,(260+Top)*.5),NS?FVector(Portal.Width,16,Top-260):FVector(16,Portal.Width,Top-260),Material);
        DoorHeaderRepairs37.Add(Portal.Id);
    }
    UE_LOG(LogTemp,Display,TEXT("FACILITIES37_HEADERS repaired=%d"),DoorHeaderRepairs37.Num());
    UE_LOG(LogTemp,Display,TEXT("FACILITIES37_BUILT props=%d failures=%d initially_deferred=%d"),FacilitiesProps37,FacilitiesFailures37,DeferredInitial37);
}
Json AEWardBookWorld::AuditFacilities37()
{
    Json Out=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> BadDoors,BadSwing,BadFloor;int Doors=0,Fixed=0,FloorOK=0;FHitResult Hit;static TSet<FString> RepairedDirections;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(Facilities37),false,Mode->Hero());
    for(FVector At:FacilitiesFloor37)
    {
        // Ignore the prop itself by tracing from just beneath its contact plane.
        if(GetWorld()->LineTraceSingleByChannel(Hit,At-FVector(0,0,.15),At-FVector(0,0,25),ECC_Pawn,Q)&&At.Z-Hit.ImpactPoint.Z<2)++FloorOK;
        else BadFloor.Add(MakeShared<FJsonValueString>(At.ToString()));
    }
    static const bool SaveReport=true;
    const Json Saved=ArchitectureSnapshot();for(auto& P:Portals){P.Open=true;RefreshPortalVisual(P);}
    for(auto& P:Portals)
    {
        if(P.Role==TEXT("niche"))continue;++Doors;
        const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");const FVector N=NS?FVector(0,1,0):FVector(1,0,0);
        if(GetWorld()->SweepSingleByChannel(Hit,P.At-N*90+FVector(0,0,98),P.At+N*90+FVector(0,0,98),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,86),Q))BadDoors.Add(MakeShared<FJsonValueString>(P.Id+TEXT(" / ")+GetNameSafe(Hit.GetComponent())));
        auto SwingClear=[&](float Sign)
        {
            const bool Double=P.Width>160;const float W=(P.Width-12)/(Double?2:1);const FVector Along=NS?FVector(1,0,0):FVector(0,1,0);const float Inward=(P.Side==TEXT("north")||P.Side==TEXT("east")?1:-1)*Sign;
            for(int Leaf=0;Leaf<(Double?2:1);++Leaf)for(float Angle:{30.f,60.f,90.f})for(float U:{.3f,.7f,.92f})
            {
                const FQuat R=FRotator(0,(NS?0:90)+(Leaf?180:0)+Inward*(Leaf?-Angle:Angle),0).Quaternion();const FVector C=P.At+Along*((Leaf?1:-1)*(P.Width*.5-6))+R.RotateVector(FVector(W*U,0,130));
                if(GetWorld()->OverlapBlockingTestByChannel(C,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(2),Q))return false;
            }
            return true;
        };
        if(P.Type==TEXT("shutter"))
        {
            bool Raised=true;for(const auto& Part:P.VisibleParts){FTransform T;DoorVisualGroups[Part.Material]->GetInstanceTransform(Part.Index,T,false);Raised&=T.GetLocation().Z>P.At.Z+255&&T.GetLocation().Z<P.At.Z+285;}
            if(!Raised)BadSwing.Add(MakeShared<FJsonValueString>(P.Id));continue;
        }
        if(!SwingClear(P.OpenSign37))
        {
            if(SwingClear(-P.OpenSign37)){P.OpenSign37=-P.OpenSign37;RefreshPortalVisual(P);++Fixed;RepairedDirections.Add(P.Id);}
            else BadSwing.Add(MakeShared<FJsonValueString>(P.Id));
        }
    }
    int CoveredHeaders=0;TArray<TSharedPtr<FJsonValue>> MissingHeaders,HeaderIds;
    for(const auto& Id:DoorHeaderRepairs37)HeaderIds.Add(MakeShared<FJsonValueString>(Id));
    for(const auto& P:Portals)
    {
        if(P.Role==TEXT("niche")||P.Type==TEXT("shutter"))continue;
        const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");const FVector N=NS?FVector(0,1,0):FVector(1,0,0),Along=NS?FVector(1,0,0):FVector(0,1,0);const FVector C=P.At+N*40;
        if(!GetWorld()->LineTraceSingleByChannel(Hit,C+FVector(0,0,370),C+FVector(0,0,410),ECC_Pawn,Q))continue;
        const float Top=Hit.ImpactPoint.Z-P.At.Z;if(Top<375||Top>400)continue;++CoveredHeaders;
        for(float U:{-.32f,0.f,.32f})
        {
            const FVector At=P.At+Along*(P.Width*U)+FVector(0,0,320);
            if(!GetWorld()->LineTraceSingleByChannel(Hit,At-N*40,At+N*40,ECC_Pawn,Q)){MissingHeaders.Add(MakeShared<FJsonValueString>(P.Id));break;}
        }
    }
    Out->SetNumberField(TEXT("indoor_headers_tested"),CoveredHeaders);Out->SetArrayField(TEXT("missing_indoor_headers"),MissingHeaders);Out->SetArrayField(TEXT("repaired_header_ids"),HeaderIds);
    RestoreArchitecture(Saved);
    Out->SetBoolField(TEXT("success"),BadDoors.IsEmpty()&&BadSwing.IsEmpty()&&BadFloor.IsEmpty()&&MissingHeaders.IsEmpty()&&FacilitiesFailures37==0);
    Out->SetNumberField(TEXT("doorways_tested"),Doors);Out->SetNumberField(TEXT("swing_direction_repairs"),RepairedDirections.Num());TArray<TSharedPtr<FJsonValue>> DirectionIds;for(const auto& Id:RepairedDirections)DirectionIds.Add(MakeShared<FJsonValueString>(Id));Out->SetArrayField(TEXT("repaired_door_ids"),DirectionIds);Out->SetArrayField(TEXT("blocked_open_doors"),BadDoors);Out->SetArrayField(TEXT("swing_obstacles"),BadSwing);
    Out->SetNumberField(TEXT("new_floor_props_tested"),FacilitiesFloor37.Num());Out->SetNumberField(TEXT("new_floor_props_supported"),FloorOK);Out->SetArrayField(TEXT("unsupported_props"),BadFloor);Out->SetNumberField(TEXT("new_props"),FacilitiesProps37);Out->SetNumberField(TEXT("native_failures"),FacilitiesFailures37);Out->SetNumberField(TEXT("initially_deferred_props"),DeferredInitial37);
    Write(FPaths::ProjectSavedDir()/TEXT("Prototype/facilities37_audit.json"),Out);return Out;
}
