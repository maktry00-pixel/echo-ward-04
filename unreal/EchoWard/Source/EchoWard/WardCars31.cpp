#include "WardBookWorld.h"
#include "WardCampaign.h"
#include "WardBookMap.h"
#include "WardEchoAssets.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "Misc/Paths.h"
using namespace WardData;
namespace {
TArray<TSharedPtr<FJsonValue>> CarVector33(FVector P){return {MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)};}
}
void AEWardBookWorld::BuildCars31()
{
    const Json D=WardEchoAssets::Cars31();if(Number(D,TEXT("version"))<31)return;
    auto* Tint=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/EchoCustom/Materials/M_Cars31B_OPAQUE.M_Cars31B_OPAQUE")),this);
    Tint->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.0025f,.0035f,.005f,1));Tint->SetScalarParameterValue(TEXT("Roughness"),.13f);Tint->SetScalarParameterValue(TEXT("Metallic"),.15f);Tint->SetScalarParameterValue(TEXT("Coat"),.75f);Tint->SetVectorParameterValue(TEXT("Emission"),FLinearColor::Black);
    for(auto V:Array(D,TEXT("placements")))
    {
        const Json P=V->AsObject(),Model=Object(Object(D,TEXT("vehicles")),String(P,TEXT("slug")));
        const int Body=Number(Model,TEXT("body")),Glass=Number(Model,TEXT("glass"));
        if(!WardEchoAssets::Mesh(Body)||(Glass&&!WardEchoAssets::Mesh(Glass)))continue;
        FWardCarPlacement33 Record;Record.Id=String(P,TEXT("id"));Record.Slug=String(P,TEXT("slug"));Record.Spec=P;Record.Transform=FTransform(FRotator(0,Number(P,TEXT("yaw")),0),Vector(Array(P,TEXT("at")))*100,FVector(1));
        const auto& B=Array(Model,TEXT("bounds_cm"));Record.Local=FBox(Vector(B[0]->AsArray()),Vector(B[1]->AsArray()));
        TArray<int> Parts={Body};for(auto Extra:Array(Model,TEXT("body_extra")))Parts.Add(Extra->AsNumber());if(Glass)Parts.Add(Glass);
        for(int Id:Parts)
        {
            if(!Id)continue;auto* Mesh=WardEchoAssets::Mesh(Id);const FString Key=FString::Printf(TEXT("car33_%s_%d"),*Record.Id,Id);
            auto* G=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);G->SetupAttachment(RootComponent);G->SetMobility(EComponentMobility::Static);G->SetStaticMesh(Mesh);G->SetCastShadow(Id!=Glass);G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetGenerateOverlapEvents(false);G->bAutoRebuildTreeOnInstanceChanges=false;G->SetAffectDynamicIndirectLighting(false);G->SetAffectDistanceFieldLighting(false);G->SetLightingChannels(true,true,false);G->SetCullDistance(Id==Glass?6500:18000);
            if(Boolean(Model,TEXT("tint33")))for(int Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot)if(Object(D,TEXT("tint_materials33"))->HasField(Mesh->GetStaticMaterials()[Slot].MaterialSlotName.ToString()))G->SetMaterial(Slot,Tint);
            G->RegisterComponent();AddInstanceComponent(G);Groups.Add(Key,G);Record.Parts.Add({G,G->AddInstance(Record.Transform,false)});++MeshInstances;++CustomFurniture;CustomModelIds.Add(Id);
        }
        auto* Collider=NewObject<UBoxComponent>(this);Collider->SetupAttachment(RootComponent);Collider->SetMobility(EComponentMobility::Movable);Collider->SetCollisionProfileName(TEXT("BlockAll"));Collider->SetGenerateOverlapEvents(false);Collider->SetBoxExtent(Record.Local.GetExtent()*FVector(.96,.96,.92));Collider->SetWorldLocationAndRotation(Record.Transform.TransformPosition(Record.Local.GetCenter()),Record.Transform.GetRotation());Collider->RegisterComponent();AddInstanceComponent(Collider);Record.Collider=Collider;
        VehiclePlacements33.Add(Record);++Cars31Count;Cars31Sources.Add(Record.Slug);
        const FString Replaces=String(P,TEXT("replaces"));if(!Replaces.IsEmpty()){Cars31Replacements.Add(Replaces);QualityReplacedItems.Add(Replaces);}
    }
    UE_LOG(LogTemp,Display,TEXT("CARS31_BUILT instances=%d sources=%d replacements=%d"),Cars31Count,Cars31Sources.Num(),Cars31Replacements.Num());
}
void AEWardBookWorld::FitCarsAndMarkings33()
{
    if(VehiclePlacements33.IsEmpty())return;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(CarSurface33),false);for(const auto& R:VehiclePlacements33)Q.AddIgnoredComponent(R.Collider);
    auto OnSurface=[&](FVector XY,bool Garage,float& Z)->bool
    {
        FHitResult Hit;const FVector Start(XY.X,XY.Y,60),End(XY.X,XY.Y,-80);
        if(!GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Q)||Hit.ImpactNormal.Z<.95f)return false;
        auto* Component=Cast<UPrimitiveComponent>(Hit.GetComponent());if(!Component)return false;
        if(Component->GetMaterial(0)!=SurfaceMaterial(7)&&!(Garage&&Component->GetMaterial(0)==SurfaceMaterial(0)))return false;
        Z=Hit.ImpactPoint.Z;return true;
    };
    TArray<FBox> Accepted;TArray<TSharedPtr<FJsonValue>> Results;bool AllOK=true;int Relocated=0;
    for(auto& R:VehiclePlacements33)
    {
        const FVector Original=R.Transform.GetLocation();const auto& Region=Array(R.Spec,TEXT("region33"));FBox Area(FVector(Region[0]->AsNumber()*100,Region[1]->AsNumber()*100,-100),FVector((Region[0]->AsNumber()+Region[2]->AsNumber())*100,(Region[1]->AsNumber()+Region[3]->AsNumber())*100,1000));
        const bool Garage=Boolean(R.Spec,TEXT("garage33"));FString LastReason;float MaxGap=0;int Attempts=0;
        auto Try=[&](FVector Candidate,const FBox& Allowed,bool Indoor)->bool
        {
            ++Attempts;Candidate.Z=0;FTransform T(R.Transform.GetRotation(),Candidate);FBox Box=R.Local.TransformBy(T);
            if(Box.Min.X<Allowed.Min.X+35||Box.Max.X>Allowed.Max.X-35||Box.Min.Y<Allowed.Min.Y+35||Box.Max.Y>Allowed.Max.Y-35){LastReason=TEXT("outside_parking_region");return false;}
            if(!Indoor)for(auto L:Array(Object(WardBookMapData(),TEXT("environment_dressing")),TEXT("lawns")))
            {const auto& A=L->AsArray();if(Box.Max.X>A[0]->AsNumber()*100&&Box.Min.X<(A[0]->AsNumber()+A[2]->AsNumber())*100&&Box.Max.Y>A[1]->AsNumber()*100&&Box.Min.Y<(A[1]->AsNumber()+A[3]->AsNumber())*100){LastReason=TEXT("lawn");return false;}}
            float Highest=-1000,Lowest=1000;
            for(int X:{-1,1})for(int Y:{-1,1})
            {
                const FVector P=T.TransformPosition(R.Local.GetCenter()+FVector(X*R.Local.GetExtent().X*.65,Y*R.Local.GetExtent().Y*.70,0));float Z=0;if(!OnSurface(P,Indoor,Z)){LastReason=TEXT("unsupported_or_not_paved");return false;}Highest=FMath::Max(Highest,Z);Lowest=FMath::Min(Lowest,Z);
            }
            if(Highest-Lowest>1){LastReason=TEXT("uneven_wheel_support");return false;}Candidate.Z=Highest-Number(R.Spec,TEXT("support_bottom_cm33"),R.Local.Min.Z)+.05f;T.SetLocation(Candidate);Box=R.Local.TransformBy(T);
            for(const FBox& Other:Accepted)if(Box.ExpandBy(FVector(15,15,0)).Intersect(Other)){LastReason=TEXT("other_vehicle");return false;}
            FBox Probe=Box.ExpandBy(FVector(8,8,0));Probe.Min.Z=Highest+12;Probe.Max.Z=FMath::Max(Probe.Min.Z+5,Box.Max.Z-3);
            TArray<FOverlapResult> Hits;GetWorld()->OverlapMultiByChannel(Hits,Probe.GetCenter(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(Probe.GetExtent()),Q);
            for(const auto& Hit:Hits)if(Hit.bBlockingHit){LastReason=TEXT("obstacle:")+GetNameSafe(Hit.GetComponent());return false;}
            R.Transform=T;R.Fitted=true;MaxGap=Highest-Lowest+.05f;return true;
        };
        auto Search=[&](const FBox& Allowed,bool Indoor)
        {
            if(Try(Original,Allowed,Indoor))return;
            TArray<FVector> Candidates;for(float X=Allowed.Min.X+150;X<Allowed.Max.X-100;X+=75)for(float Y=Allowed.Min.Y+200;Y<Allowed.Max.Y-150;Y+=75)Candidates.Add(FVector(X,Y,0));
            Candidates.Sort([&](const FVector& A,const FVector& B){return FVector::DistSquared2D(A,Original)<FVector::DistSquared2D(B,Original);});for(FVector At:Candidates)if(Try(At,Allowed,Indoor))break;
        };
        Search(Area,Garage);
        if(!R.Fitted&&Garage){Area=FBox(FVector(6000,-2400,-100),FVector(8800,-200,1000));Search(Area,false);}
        AllOK&=R.Fitted;if(!Original.Equals(R.Transform.GetLocation(),.1f))++Relocated;
        for(const auto& Part:R.Parts)Part.Key->UpdateInstanceTransform(Part.Value,R.Transform,false,true,true);
        R.Collider->SetWorldLocationAndRotation(R.Transform.TransformPosition(R.Local.GetCenter()),R.Transform.GetRotation());Accepted.Add(R.Local.TransformBy(R.Transform));Cars31Bounds.Add(Accepted.Last());
        Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),R.Id);Row->SetStringField(TEXT("slug"),R.Slug);Row->SetBoolField(TEXT("passed"),R.Fitted);Row->SetArrayField(TEXT("before_cm"),CarVector33(Original));Row->SetArrayField(TEXT("at_cm"),CarVector33(R.Transform.GetLocation()));Row->SetNumberField(TEXT("wheel_height_spread_cm"),MaxGap);Row->SetNumberField(TEXT("attempts"),Attempts);Row->SetStringField(TEXT("last_rejection"),LastReason);Results.Add(MakeShared<FJsonValueObject>(Row));
        UE_LOG(LogTemp,Display,TEXT("CAR_FIT33 %s %s at=%s tries=%d %s"),R.Fitted?TEXT("PASS"):TEXT("FAIL"),*R.Id,*R.Transform.GetLocation().ToString(),Attempts,*LastReason);
        for(auto V:Array(WardEchoAssets::Cars31(),TEXT("captures")))
        {
            Json C=V->AsObject();if(String(C,TEXT("car33_id"))!=R.Id)continue;const FVector Offset=(R.Transform.GetLocation()-Original)/100;C->SetArrayField(TEXT("at"),CarVector33(Vector(Array(C,TEXT("at")))+Offset));C->SetArrayField(TEXT("target"),CarVector33(Vector(Array(C,TEXT("target")))+Offset));
        }
        if(String(R.Spec,TEXT("zone33"))==TEXT("S01"))
        {
            const FVector At=R.Transform.GetLocation();const float HalfY=FMath::Max(300.f,float(Accepted.Last().GetExtent().Y+40));
            for(float X:{-200.f,200.f})PendingRoadPaint33.Add({FVector(At.X+X,At.Y,0),FVector(10,HalfY*2,1)});
            const float Back=R.Transform.Rotator().Yaw>0?-HalfY:HalfY;PendingRoadPaint33.Add({FVector(At.X,At.Y+Back,0),FVector(400,10,1)});
        }
    }
    // Thin paint follows the actual paved support. It has no collision or shadow.
    FinishingRoadPaint33=true;float MaxPaintGap=0;
    for(const auto& Paint:PendingRoadPaint33)
    {
        FVector At=Paint.Key;const FVector Size=Paint.Value;bool OK=true;float High=-1000,Low=1000;
        const bool AlongX=Size.X>Size.Y;for(float S:{-.48f,0.f,.48f}){float Z=0;const FVector P=At+(AlongX?FVector(Size.X*S,0,0):FVector(0,Size.Y*S,0));if(!OnSurface(P,false,Z)){OK=false;break;}High=FMath::Max(High,Z);Low=FMath::Min(Low,Z);}
        if(!OK||High-Low>.2f){++RoadPaintRejected33;continue;}At.Z=High+.025f;Box(At,FVector(Size.X,Size.Y,.02f),8,false);MaxPaintGap=FMath::Max(MaxPaintGap,High-Low+.035f);++RoadPaintCount33;
    }
    FinishingRoadPaint33=false;

    int32 RainPatches = 0;
    auto* WaterMesh = LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/HorrorV28/SM_TracePlane.SM_TracePlane"));
    auto* WaterMaterial = LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/HorrorV28/MI_Water.MI_Water"));
    if (WaterMesh && WaterMaterial)
    {
        struct FPatch { FVector2D Centre, Size; float Yaw; };
        const FPatch Patches[] = {
            {{-2540,-5770},{260,108},-18}, {{-2905,-5610},{190,96},20},
            {{-3250,-5320},{300,120},-7}, {{-2260,-5230},{173,82},17},
            {{-3380,-6140},{248,75},-27}, {{-3800,-6500},{180,68},5},
            {{4990,-5700},{240,102},84}, {{5650,-5620},{172,72},25},
            {{5100,-5210},{218,94},71}, {{5130,-4740},{280,118},83},
            {{4910,-4210},{172,65},69}, {{4690,-3500},{226,92},97},
            {{4830,-2920},{187,75},12}, {{5310,-3200},{246,88},60}
        };
        auto* Water = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        auto* WetAsphalt = UMaterialInstanceDynamic::Create(WaterMaterial,this);
        WetAsphalt->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.107f,.119f,.128f,1.f));
        WetAsphalt->SetScalarParameterValue(TEXT("Roughness"),.29f);
        Water->SetupAttachment(RootComponent);
        Water->SetMobility(EComponentMobility::Static);
        Water->SetStaticMesh(WaterMesh);
        Water->SetMaterial(0,WetAsphalt);
        Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Water->SetGenerateOverlapEvents(false);
        Water->SetCastShadow(false);
        Water->SetAffectDynamicIndirectLighting(false);
        Water->SetAffectDistanceFieldLighting(false);
        Water->SetLightingChannels(true,true,false);
        Water->SetCullDistance(4800);
        Water->bAutoRebuildTreeOnInstanceChanges = false;
        for (const auto& Patch : Patches)
        {
            const FRotator Rotation(0,Patch.Yaw,0);
            const FVector Centre(Patch.Centre,0);
            float Surface = 0;
            if (!OnSurface(Centre,false,Surface)) continue;
            bool Supported = true;
            for (const float X : {-.47f,.47f}) for (const float Y : {-.47f,.47f})
            {
                float Height = 0;
                const FVector Sample = Centre + Rotation.RotateVector(FVector(Patch.Size.X*X,Patch.Size.Y*Y,0));
                Supported &= OnSurface(Sample,false,Height) && FMath::Abs(Height-Surface)<.25f;
            }
            if (!Supported) continue;
            Water->AddInstance(FTransform(Rotation,FVector(Patch.Centre,Surface+.12f),
                FVector(Patch.Size.X/100,Patch.Size.Y/100,1)),false);
            ++RainPatches;
        }
        AddInstanceComponent(Water);
        Water->RegisterComponent();
        Water->bAutoRebuildTreeOnInstanceChanges = true;
        Water->BuildTreeIfOutdated(false,true);
        MeshInstances += RainPatches;
    }
    for(auto& G:Groups){G.Value->bAutoRebuildTreeOnInstanceChanges=true;G.Value->BuildTreeIfOutdated(false,true);}
    VehicleAudit33=MakeShared<FJsonObject>();VehicleAudit33->SetBoolField(TEXT("success"),AllOK&&RoadPaintCount33>100);VehicleAudit33->SetArrayField(TEXT("cars"),Results);VehicleAudit33->SetNumberField(TEXT("relocated"),Relocated);VehicleAudit33->SetNumberField(TEXT("paint_count"),RoadPaintCount33);VehicleAudit33->SetNumberField(TEXT("paint_rejected_off_pavement"),RoadPaintRejected33);VehicleAudit33->SetNumberField(TEXT("paint_max_gap_cm"),MaxPaintGap);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/car_fit33.json"),VehicleAudit33);
    UE_LOG(LogTemp,Display,TEXT("WARD88 road water patches=%d"),RainPatches);
}
void AEWardBookWorld::UpdateCars33(float Delta)
{
    CarQualityClock33+=Delta;if(CarQualityClock33<.3f||!Mode.IsValid()||!Mode->Hero())return;CarQualityClock33=0;TSet<UHierarchicalInstancedStaticMeshComponent*> Near;
    const FVector Player=Mode->Hero()->GetActorLocation();for(const auto& R:VehiclePlacements33)if(FVector::DistSquared(Player,R.Transform.GetLocation())<FMath::Square(1000.f))for(auto Part:R.Parts)Near.Add(Part.Key);
    bool Changed=Near.Num()!=NearCars33.Num();if(!Changed)for(auto* Part:Near)if(!NearCars33.Contains(Part)){Changed=true;break;}if(!Changed)return;
    // Reset first, then request the union of nearby textures, including shared material maps.
    for(const auto& R:VehiclePlacements33)for(auto Part:R.Parts){Part.Key->SetTextureForceResidentFlag(false);Part.Key->SetForcedLodModel(Near.Contains(Part.Key)?1:0);}
    for(auto* Part:Near)Part->SetTextureForceResidentFlag(true);NearCars33=MoveTemp(Near);
}
