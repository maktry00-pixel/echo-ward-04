#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardEchoAssets.h"
#include "WardGraphics.h"
#include "WardHospitalMaterials.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
using namespace WardData;
namespace {
Json FitBox(FVector Min,FVector Max)
{
    Json M=MakeShared<FJsonObject>();M->SetArrayField(TEXT("bounds"),{MakeShared<FJsonValueNumber>(Min.X),MakeShared<FJsonValueNumber>(Min.Y),MakeShared<FJsonValueNumber>(Min.Z),MakeShared<FJsonValueNumber>(Max.X),MakeShared<FJsonValueNumber>(Max.Y),MakeShared<FJsonValueNumber>(Max.Z)});return M;
}
}
void AEWardBookWorld::BuildQualityDressing()
{
    const Json D=WardEchoAssets::Quality();if(Number(D,TEXT("version"))<22)return;
    for(auto V:Array(D,TEXT("props")))
    {
        Json P=V->AsObject(),Fit=MakeShared<FJsonObject>();
        if(Cars31Replacements.Contains(String(P,TEXT("id")))){++QualityProps;QualityReplacedItems.Add(String(P,TEXT("id")));continue;}
        const auto& Corrected=Array(Object(WardEchoAssets::RealArt(),TEXT("bounds_overrides")),String(P,TEXT("id")));
        Fit->SetArrayField(TEXT("bounds"),Corrected.Num()==6?Corrected:Array(P,TEXT("bounds")));
        if(AddCustomProp(Number(P,TEXT("mesh")),Fit,Vector(Array(P,TEXT("at")))*100,Number(P,TEXT("turn"))))
        {++QualityProps;QualityReplacedItems.Add(String(P,TEXT("id")));}
    }
    for(auto V:Array(D,TEXT("trees")))
    {
        Json P=V->AsObject();const float Height=Number(P,TEXT("height"));const int Id=Number(P,TEXT("mesh"));
        // Branch foliage is fitted to a bounded silhouette. The old trunk
        // collider remains, and crown spheres disappear only after success.
        const float W=Height*(Id==642?.46f:.36f);Json Fit=FitBox(FVector(-W*.5,-W*.5,0),FVector(W*.5,W*.5,Height));
        if(AddCustomProp(Id,Fit,Vector(Array(P,TEXT("at")))*100,GetTypeHash(String(P,TEXT("id")))%4)){++QualityTrees;QualityReplacedItems.Add(String(P,TEXT("id")));}
    }
    auto* SillMetal=WardHospitalMaterials::Variant(this,Palette[11]);
    SillMetal->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.19f,.155f,.135f));
    SillMetal->SetScalarParameterValue(TEXT("Roughness"),.54f);
    SillMetal->SetScalarParameterValue(TEXT("Weathering"),.65f);
    const int32 SillFinish=Palette.Add(SillMetal);
    for(auto V:Array(D,TEXT("windows")))
    {
        Json W=V->AsObject();FVector Base=Vector(Array(W,TEXT("at")))*100;const FString Side=String(W,TEXT("side"));const FVector Out=Side==TEXT("north")?FVector(0,-1,0):Side==TEXT("south")?FVector(0,1,0):Side==TEXT("west")?FVector(-1,0,0):FVector(1,0,0);const FVector Along(-Out.Y,Out.X,0);const bool NS=Out.Y!=0;
        const float Width=Number(W,TEXT("width"))*100,Sill=Number(W,TEXT("sill"))*100,Height=Number(W,TEXT("height"))*100;
        // Projecting sills, mullions, lintel shadow and drip edges produce
        // actual relief around the existing transparent aperture.
        auto Trim=[&](double AlongOffset,double Depth,double Z,double Length,double Projection,double Thick,int32 Material)
        {
            Box(Base+Out*Depth+Along*AlongOffset+FVector(0,0,Z),
                NS?FVector(Length,Projection,Thick):FVector(Projection,Length,Thick),Material,false);
        };
        Trim(0,20,Sill-2,Width+18,35,4,SillFinish);
        // Folded drip edge and the recessed seal beneath the sill give the
        // opening separate metal, plaster and shadow surfaces in grazing light.
        Trim(0,37,Sill-5,Width+18,2.4,5,SillFinish);
        Trim(0,14,Sill-8,Width+7,6,2.4,5);
        for(double SideSign:{-1.,1.})
        {
            const double Offset=SideSign*(Width*.5-8);
            Trim(Offset,23,Sill-8,8,16,8,11);
            Trim(Offset,30,Sill+.8,3,3,1.8,SillFinish);
        }
        Box(Base+Out*12+FVector(0,0,Sill+Height+7),NS?FVector(Width+20,12,7):FVector(12,Width+20,7),11,false);
        for(float Offset:{-Width*.5,Width*.5})Box(Base+Along*Offset+Out*13+FVector(0,0,Sill+Height*.5),NS?FVector(5,9,Height+10):FVector(9,5,Height+10),11,false);
        if(Width>150)Box(Base+Out*13+FVector(0,0,Sill+Height*.5),NS?FVector(4,9,Height):FVector(9,4,Height),11,false);
        if(Boolean(W,TEXT("lit")))
        {
            // Warm ceiling strip sits behind the window, keeping its centre
            // transparent and the actual room visible through the glazing.
            Box(Base-Out*55+FVector(0,0,Sill+Height+18),NS?FVector(Width*.6,9,3):FVector(9,Width*.6,3),QualityWarmMaterial,false);
            WarmAnchors.Add(Base+Out*65+FVector(0,0,Sill+Height+20));
            // Shaded, narrow external downlight over the window bay.
            Box(Base+Out*25+FVector(0,0,Sill+Height+26),NS?FVector(Width*.45,18,5):FVector(18,Width*.45,5),QualityWarmMaterial,false);
            // A few working rooms reveal the light fixture behind the glass.
            // Keep the lower part of the window open to the interior.
            if (Width>=140.f && QualityWindows%4==0)
            {
                const FVector Strip = Base-Out*45+Along*(Width*.12)+FVector(0,0,Sill+Height*.79);
                Box(Strip,NS?FVector(Width*.49,5,3):FVector(5,Width*.49,3),QualityWarmMaterial,false);
            }
        }
        if(QualityWindows%4==0)
        {
            Json Fit=FitBox(FVector(-.1,-.1,0),FVector(.1,.1,.3));AddCustomProp(643,Fit,Base+Out*25+Along*(Width*.5+40)+FVector(0,0,Sill+Height+40),Side==TEXT("north")?0:Side==TEXT("east")?1:Side==TEXT("south")?2:3);
        }
        ++QualityWindows;
    }
    // Fit the entrance hardware to the existing ground-floor portal. Keep the
    // rain cover above the active header and keep the walk-on plate flush.
    const auto* Entry=Portals.FindByPredicate([](const FPortal& Portal)
    {
        return Portal.Zone==TEXT("N02") && Portal.Side==TEXT("north") && Portal.At.Z==0;
    });
    if(Entry)
    {
        const FVector Door=Entry->At;
        const double Width=Entry->Width+118.;
        Box(Door+FVector(0,-69,310),FVector(Width,120,5),SillFinish,false);
        Box(Door+FVector(0,-129,308),FVector(Width,5,13),SillFinish,false);
        Box(Door+FVector(0,-15,314),FVector(Width,12,7),11,false);
        for(const double Side:{-1.,1.})
        {
            const double X=Side*(Entry->Width*.5+36.);
            Box(Door+FVector(X,-14,288),FVector(21,12,31),11,false);
            Box(Door+FVector(X,-62,302),FVector(7,113,9),SillFinish,false);
        }
        Box(Door+FVector(0,-24,.22),FVector(Entry->Width-13,45,.44),SillFinish,false);
        for(const double Y:{-41.,-31.,-21.,-11.})
            Box(Door+FVector(0,Y,.53),FVector(Entry->Width-21,1.45,.19),11,false);
    }

    auto* Rail=WardHospitalMaterials::Make(this,TEXT(""),FLinearColor(.31f,.265f,.19f),.64f);
    const int32 RailFinish=Palette.Add(Rail);
    // The corridor-facing 12 cm partitions are already built from this data.
    // Use only their solid long sections and leave the authored gaps free.
    for(const auto& Value:Array(WardBookMapData(),TEXT("partitions")))
    {
        const Json Partition=Value->AsObject();
        if(String(Partition,TEXT("site"))!=TEXT("1F") || String(Partition,TEXT("type"))!=TEXT("niche"))continue;
        const FVector A=Vector(Array(Partition,TEXT("a")))*100, B=Vector(Array(Partition,TEXT("b")))*100;
        const double Span=FMath::Abs(B.X-A.X);
        if(A.Y!=B.Y || A.Z!=0 || B.Z!=0 || Span<135. || FMath::Min(A.X,B.X)<1680. || FMath::Max(A.X,B.X)>4740.)continue;
        const double Facing=FMath::IsNearlyEqual(A.Y,3900.,.1)?1.:FMath::IsNearlyEqual(A.Y,4300.,.1)?-1.:0.;
        if(Facing==0)continue;
        const double Face=Number(Partition,TEXT("thickness"),.12)*50;
        const FVector Centre=(A+B)*.5;
        Box(Centre+FVector(0,Facing*(Face+6.5),94),FVector(Span-25,4,11),RailFinish,false);
        for(double X=FMath::Min(A.X,B.X)+42.;X<FMath::Max(A.X,B.X)-20.;X+=146.)
            Box(FVector(X,Centre.Y+Facing*(Face+2.5),94),FVector(10,6,15),SillFinish,false);
        for(const double EndSign:{-1.,1.})
            Box(Centre+FVector(EndSign*(Span*.5-13),Facing*(Face+4.5),94),FVector(3,8,11),SillFinish,false);
    }

    // Set the cast-iron grates into the paved S01 surface. The visual slots
    // use a dark recess and shallow raised bars, with no collision components.
    for(const double X:{-4250.,-3550.,-2850.})
    {
        const FVector Drain(X,-5470,0);
        const auto* Cell=Cells.Find(FIntVector(FMath::FloorToInt(Drain.X/100),FMath::FloorToInt(Drain.Y/100),0));
        if(!Cell||!Cell->Outdoor)continue;
        Box(Drain+FVector(0,0,.19),FVector(80,43,.38),SillFinish,false);
        Box(Drain+FVector(0,0,.42),FVector(69,35,.18),5,false);
        for(int32 I=-5;I<=5;++I)
            Box(Drain+FVector(I*5.9,0,.73),FVector(2,34,.46),11,false);
        for(const double Side:{-1.,1.})
            Box(Drain+FVector(0,Side*18.5,.76),FVector(72,2.5,.5),11,false);
    }
    for(auto V:Array(D,TEXT("outdoor_lamps")))
    {
        Json P=V->AsObject();const FVector At=Vector(Array(P,TEXT("at")))*100;
        AddCustomProp(644,FitBox(FVector(-.2,-.2,0),FVector(.9,.2,5.8)),At,FMath::RoundToInt(Number(P,TEXT("yaw"))/90));
        WarmAnchors.Add(At+FVector(60,0,535));
    }
    // Existing street lamp positions remain authoritative, no additional
    // obstacles are placed on the pavement or near gates.
    for(auto V:Array(Object(WardBookMapData(),TEXT("environment_dressing")),TEXT("boxes")))
    {
        Json B=V->AsObject();if(String(B,TEXT("kind"))!=TEXT("street_lamp"))continue;FVector At=Vector(Array(B,TEXT("at")))*100;
        WarmAnchors.Add(At-FVector(0,0,35));
        // The old pole stays, but the crude luminous slab gets a detailed hood.
        AddCustomProp(643,FitBox(FVector(-.33,-.15,-.045),FVector(.33,.15,.045)),At,0);
    }
    for(int I=0;I<12;++I)
    {
        auto* L=NewObject<UPointLightComponent>(this);L->SetupAttachment(RootComponent);L->SetMobility(EComponentMobility::Movable);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(3500);L->SetAttenuationRadius(1600);L->SetLightColor(FLinearColor(1,.57f,.25f));L->SetSourceRadius(18);L->SetCastShadows(I<4);L->SetLightingChannels(true,true,false);L->ContactShadowLengthInWS=true;L->ContactShadowLength=I<2?38.f:0.f;L->SetVisibility(false);L->RegisterComponent();AddInstanceComponent(L);WarmPool.Add(L);
    }
    // Quality meshes are instanced and partitioned by neighbourhood; tree
    // culling avoids submitting the entire 4,000-tree forest at once.
    for(auto& Pair:Groups)if(Pair.Key.StartsWith(TEXT("echo_641_"))||Pair.Key.StartsWith(TEXT("echo_642_")))Pair.Value->SetCullDistance(22000);
    for(auto V:Array(WardEchoAssets::RealArt(),TEXT("props")))
    {
        Json P=V->AsObject(),Fit=MakeShared<FJsonObject>();Fit->SetArrayField(TEXT("bounds"),Array(P,TEXT("bounds")));
        if(AddCustomProp(Number(P,TEXT("mesh")),Fit,Vector(Array(P,TEXT("at")))*100,Number(P,TEXT("turn"))))
        {
            ++RealProps;
            if(Boolean(P,TEXT("solid")))
            {
                const auto& B=Array(P,TEXT("world_bounds"));
                if(B.Num()==6){const FVector Lo(B[0]->AsNumber(),B[1]->AsNumber(),B[2]->AsNumber()),Hi(B[3]->AsNumber(),B[4]->AsNumber(),B[5]->AsNumber());Box((Lo+Hi)*50,(Hi-Lo)*100,0,true,false);++RealSolidProps;}
            }
        }
    }
    for(auto V:Array(WardEchoAssets::Realism30(),TEXT("props")))
    {
        const Json P=V->AsObject();Json Fit=MakeShared<FJsonObject>();Fit->SetArrayField(TEXT("bounds"),Array(P,TEXT("bounds")));
        if(AddCustomProp(Number(P,TEXT("mesh")),Fit,Vector(Array(P,TEXT("at")))*100,Number(P,TEXT("turn"))))
        {
            ++Realism30Props;
            if(Boolean(P,TEXT("solid")))
            {
                const auto& B=Array(P,TEXT("world_bounds"));
                const FVector Lo(B[0]->AsNumber(),B[1]->AsNumber(),B[2]->AsNumber()),Hi(B[3]->AsNumber(),B[4]->AsNumber(),B[5]->AsNumber());
                Box((Lo+Hi)*50,(Hi-Lo)*100,0,true,false);
            }
        }
    }

}
void AEWardBookWorld::UpdateQualityLights(FVector Player,bool OpenAir)
{
    TArray<int> Order;for(int I=0;I<WarmAnchors.Num();++I)if(FMath::Abs(WarmAnchors[I].Z-Player.Z)<650&&FVector::Dist2D(WarmAnchors[I],Player)<2300)Order.Add(I);
    Order.Sort([&](int A,int B){return FVector::DistSquared(Player,WarmAnchors[A])<FVector::DistSquared(Player,WarmAnchors[B]);});
    const int32 Preset=EWardGraphics::Current();
    const int32 LightLimit=OpenAir?(Preset==2?5:8):(Preset==2?2:4);
    // Ceiling fixtures supply contact shadows indoors. Decorative lamp pools
    // share that budget to avoid rendering the same room from many light faces.
    const int32 ShadowLimit=OpenAir?(Preset==2?2:3):(Preset==0?2:1);
    TArray<int> Selected;for(int Id:Order){if(Selected.ContainsByPredicate([&](int O){return FVector::Dist2D(WarmAnchors[Id],WarmAnchors[O])<260;}))continue;Selected.Add(Id);if(Selected.Num()>=LightLimit)break;}
    for(int I=0;I<WarmPool.Num();++I)
    {
        auto* L=WarmPool[I].Get();const bool On=Selected.IsValidIndex(I);L->SetVisibility(On);if(!On)continue;
        L->SetRelativeLocation(WarmAnchors[Selected[I]]);
        const int32 Fixture=Selected[I];
        L->SetLightColor(OpenAir?(Fixture%3==0?FLinearColor(1.f,.84f,.65f):FLinearColor(1.f,.64f,.40f)):
            FLinearColor(1.f,.75f,.51f));
        L->SetIntensity(Bright?2000.f:OpenAir?4000.f:800.f);
        L->SetSourceRadius(OpenAir?26.f:15.f);
        L->SetAttenuationRadius(OpenAir?1500.f:550.f);
        L->SetCastShadows(I<ShadowLimit);
        L->ContactShadowLength=I<ShadowLimit&&I==0?24.f:0.f;
    }
}
