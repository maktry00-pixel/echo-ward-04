#include "WardBookWorld.h"
#include "WardGraphics.h"
#include "WardBookMap.h"
#include "WardCampaign.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WardHospitalMaterials.h"
#include "WardHorror99.h"
#include "Engine/StaticMesh.h"
#include "Camera/CameraComponent.h"
#include "Components/LocalFogVolumeComponent.h"
using namespace WardData;
void PrepareWardBookMapConstruction();
void FinishWardBookMapConstruction();
namespace
{
    constexpr float Height=380,Pitch=480;
    bool Shaft(int X,int Y){return X>=48&&X<52&&Y>=4&&Y<10;}

}
AEWardBookWorld::AEWardBookWorld()
{
    PrimaryActorTick.bCanEverTick=false;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("BookHospital"));
    RootComponent->SetMobility(EComponentMobility::Static);
}
void AEWardBookWorld::Box(FVector At,FVector Size,int Mat,bool Collision,bool Visible)
{
    if(Size.GetMin()<=0)return;
    if(Mat==8&&Visible&&!Collision&&Size.Z<=3.01f)
    {
        if(At.Z<100&&!FinishingRoadPaint33){PendingRoadPaint33.Add({At,Size});return;}
        if(!FinishingRoadPaint33){At.Z-=Size.Z*.5f;At.Z+=.02f;}
        Size.Z=.02f;
    }
    const bool Fluorescent=Mat==4||Mat==DormantLampMaterial||Mat==FlickerLampMaterial;
    if(Fluorescent&&Visible&&!Collision&&Size.Z<12&&Size.X>70&&Size.Y>12&&WardEchoAssets::Mesh(121))
    {
        const FVector Extent(Size.X*.5,Size.Y*.5,FMath::Max(5.f,Size.Z*.5));Json Model=MakeShared<FJsonObject>();
        Model->SetArrayField(TEXT("bounds"),{MakeShared<FJsonValueNumber>(-Extent.X/100),MakeShared<FJsonValueNumber>(-Extent.Y/100),MakeShared<FJsonValueNumber>(-Extent.Z/100),MakeShared<FJsonValueNumber>(Extent.X/100),MakeShared<FJsonValueNumber>(Extent.Y/100),MakeShared<FJsonValueNumber>(Extent.Z/100)});
        if(AddCustomProp(121,Model,At,0))return;
    }
    const FString Key=FString::Printf(TEXT("%d_%d_%d_%d_%d_%d"),Mat,Collision?1:0,Visible?1:0,FMath::FloorToInt(At.X/2400),FMath::FloorToInt(At.Y/2400),FMath::FloorToInt(At.Z/480));
    auto* Group=Groups.FindRef(Key);
    if(!Group)
    {
        Group=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        Group->SetupAttachment(RootComponent);Group->SetMobility(EComponentMobility::Static);
        const bool Glass = Mat==13 || (Palette[Mat] && Palette[Mat]->GetFName() == FName(TEXT("WardFrostedPane")));
        Group->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Glass?TEXT("/Engine/BasicShapes/Cube.Cube"):TEXT("/Game/Campaign/BookWorld/SM_BookCube.SM_BookCube")));
        Group->SetMaterial(0,Palette[Mat]);Group->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));
        Group->SetGenerateOverlapEvents(false);Group->bAutoRebuildTreeOnInstanceChanges=false;
        // The campus uses screen-space lighting; keep its instanced
        // architecture out of the menu's distance-field scene.
        Group->SetAffectDynamicIndirectLighting(false);
        Group->SetAffectDistanceFieldLighting(false);
        Group->SetLightingChannels(true,true,false);
        Group->SetVisibility(Visible);Group->SetHiddenInGame(!Visible);Group->SetCastShadow(Visible&&Mat!=4&&Mat!=8);Group->RegisterComponent();AddInstanceComponent(Group);Groups.Add(Key,Group);
    }
    Group->AddInstance(FTransform(FQuat::Identity,At,Size/100),false);++MeshInstances;
}
void AEWardBookWorld::Sign(const FString& Text,FVector At,FRotator Rotation,float Size)
{
    auto* C=NewObject<UTextRenderComponent>(this);C->SetupAttachment(RootComponent);
    C->SetRelativeLocation(At);C->SetRelativeRotation(Rotation);C->SetText(FText::FromString(Text));
    C->SetWorldSize(Size);C->SetHorizontalAlignment(EHTA_Center);C->SetVerticalAlignment(EVRTA_TextCenter);
    C->SetTextRenderColor(FColor(178,205,200));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCullDistance(4500);C->RegisterComponent();AddInstanceComponent(C);
}
void AEWardBookWorld::Build(AEWardCampaignMode* Campaign)
{
    Mode=Campaign;
    PrepareWardBookMapConstruction();
    auto Material=[&](const TCHAR* Source,FLinearColor Tint,float Roughness)
    {
        auto* M=WardHospitalMaterials::Make(this,Source,Tint,Roughness);
        if (FCString::Strcmp(Source,TEXT("4ae6673fcc8b19e7"))==0)
        {
            const float Shade=FMath::Clamp(Tint.GetLuminance(),.25f,.72f);
            const FLinearColor Plaster=FMath::Lerp(FLinearColor(.97f,.94f,.92f)*Shade,Tint,.19f);
            M->SetVectorParameterValue(TEXT("Tint"),Plaster);
            M->SetScalarParameterValue(TEXT("NormalStrength"),.77f);
        }
        else if (FCString::Strcmp(Source,TEXT("05227396fb5ca2d8"))==0)
        {
            const float Shade=FMath::Clamp(Tint.GetLuminance(),.20f,.48f);
            const FLinearColor WallPaint=FMath::Lerp(FLinearColor(1.24f,.49f,.42f)*Shade,Tint,.23f);
            M->SetVectorParameterValue(TEXT("Tint"),WallPaint);
            M->SetScalarParameterValue(TEXT("NormalStrength"),.65f);
        }
        else if (FCString::Strcmp(Source,TEXT("527b19fd5563fa6d"))==0)
        {
            M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.46f,.434f,.408f));
            M->SetScalarParameterValue(TEXT("NormalStrength"),.83f);
        }
        Palette.Add(M);return M;
    };
    Material(TEXT("527b19fd5563fa6d"),FLinearColor(.38,.43,.41),.76); // floor
    Material(TEXT("4ae6673fcc8b19e7"),FLinearColor(.58,.62,.58),.92); // plaster
    Material(TEXT("05227396fb5ca2d8"),FLinearColor(.20,.32,.30),.8);  // lower wall
    Material(TEXT("4ae6673fcc8b19e7"),FLinearColor(.27,.31,.29),.92); // ceiling
    auto* Emissive=Material(TEXT(""),FLinearColor(.68,.65,.60),.7);
    Emissive->SetVectorParameterValue(TEXT("Emission"),FLinearColor(4.0,4.4,4.7));
    Material(TEXT(""),FLinearColor(.125,.055,.048),.62); // trim
    auto* Window=Material(TEXT(""),FLinearColor(.14,.085,.071),.25f); // 6: architectural glazing panels
    Window->SetVectorParameterValue(TEXT("Emission"),FLinearColor(.060,.028,.020));
    Material(TEXT("echo_asphalt"),FLinearColor(.13,.14,.15),.8f); // 7 wet aggregate asphalt
    Material(TEXT(""),FLinearColor(.75,.76,.65),.8f); // 8 painted markings
    Material(TEXT(""),FLinearColor(.12,.20,.13),.8f); // 9 landscape beds
    Material(TEXT(""),FLinearColor(.36,.24,.18),.8f); // 10 brick
    Material(TEXT("echo_metal"),FLinearColor(.28,.32,.31),.7f); // 11 aged steel / door frames
    Material(TEXT(""),FLinearColor(.60,.30,.10),.8f); // 12 services / hazard trim
    Cast<UMaterialInstanceDynamic>(Palette[0])->SetScalarParameterValue(TEXT("Surface"),1);
    if(Number(WardEchoAssets::HorrorArt28(),TEXT("version"))<28)Cast<UMaterialInstanceDynamic>(Palette[0])->SetScalarParameterValue(TEXT("Wetness"),.22f);
    Cast<UMaterialInstanceDynamic>(Palette[0])->SetScalarParameterValue(TEXT("Roughness"),WardEchoAssets::Surface(TEXT("527b19fd5563fa6d"))?.61f:.78f);
    Cast<UMaterialInstanceDynamic>(Palette[7])->SetScalarParameterValue(TEXT("Weathering"),.6f);
    Cast<UMaterialInstanceDynamic>(Palette[11])->SetScalarParameterValue(TEXT("Metallic"),.55f);
    Cast<UMaterialInstanceDynamic>(Palette[11])->SetScalarParameterValue(TEXT("Weathering"),.18f);
    Cast<UMaterialInstanceDynamic>(Palette[3])->SetScalarParameterValue(TEXT("Surface"),2);
    Cast<UMaterialInstanceDynamic>(Palette[10])->SetScalarParameterValue(TEXT("Surface"),3);
    Palette.Add(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalGlass.M_HospitalGlass")));
    auto QualityMat=[&](const TCHAR* Name){const int Id=Palette.Num();UMaterialInterface* M=nullptr;if(FCString::Strcmp(Name,TEXT("Facade"))==0&&Number(WardEchoAssets::DetailArt(),TEXT("version"))>=26)M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/HorrorV28/MI_Facade.MI_Facade"));if(!M)M=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Campaign/EchoCustom/Materials/MI_V22_%s.MI_V22_%s"),Name,Name));Palette.Add(M?M:Palette[1].Get());return Id;};
    QualityFacade=QualityMat(TEXT("Facade"));QualityTile=QualityMat(TEXT("DadoTile"));QualityWarmMaterial=QualityMat(TEXT("WindowWarm"));
    auto Refinish=[&](int32 Id,FLinearColor Tint,float Relief)
    {
        auto* Surface=WardHospitalMaterials::Variant(this,Palette[Id]);
        Surface->SetVectorParameterValue(TEXT("Tint"),Tint);
        Surface->SetScalarParameterValue(TEXT("NormalStrength"),Relief);
        Palette[Id]=Surface;
    };
    Refinish(QualityFacade,FLinearColor(.58f,.555f,.53f),.78f);
    Refinish(QualityTile,FLinearColor(.42f,.115f,.095f),.63f);
    for(auto V:Array(WardBookMapData(),TEXT("locations")))
    {
        Json L=V->AsObject(),I=Object(L,TEXT("identity"));const FString Id=String(L,TEXT("id"));
        auto Tint=[&](const TCHAR* Key){const auto& A=Array(I,Key);return A.Num()==3?FLinearColor(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()):FLinearColor(.5,.55,.5);};
        SiteWall.Add(Id,Palette.Num());Material(TEXT("4ae6673fcc8b19e7"),Tint(TEXT("wall_tint")),.9);
        SiteLower.Add(Id,Palette.Num());Material(TEXT("05227396fb5ca2d8"),Tint(TEXT("lower_tint")),.8);
    }
    DormantLampMaterial=Palette.Num();auto* DeadLamp=Material(TEXT(""),FLinearColor(.13f,.17f,.17f),.8f);DeadLamp->SetVectorParameterValue(TEXT("Emission"),FLinearColor::Black);
    FlickerLampMaterial=Palette.Num();FlickerLamp=Material(TEXT(""),FLinearColor(.77f,.25f,.17f),.7f);FlickerLamp->SetVectorParameterValue(TEXT("Emission"),FLinearColor(5.8f,1.6f,1.f));
    const int32 FrostedGlass = Palette.Num();
    Palette.Add(WardHorror99::FrostedPane(this, Palette[13]));
    auto Rect=[&](FVector O,const TArray<TSharedPtr<FJsonValue>>& R,int Room,bool Outdoor)
    {
        if(R.Num()!=4)return;
        const int X=FMath::RoundToInt(O.X/100+R[0]->AsNumber()),Y=FMath::RoundToInt(O.Y/100+R[1]->AsNumber());
        for(int A=X;A<X+R[2]->AsNumber();++A)for(int B=Y;B<Y+R[3]->AsNumber();++B)
        {
            const FIntVector Key(A,B,FMath::RoundToInt(O.Z));
            auto* Old=Cells.Find(Key);if(!Old||Old->Room==INDEX_NONE)Cells.Add(Key,{Room,Outdoor});
        }
    };
    for(auto V:Array(WardBookMapData(),TEXT("locations")))
    {
        Json L=V->AsObject();const FString Id=String(L,TEXT("id"));const auto& OA=Array(L,TEXT("origin"));const FVector O(OA[0]->AsNumber()*100,OA[1]->AsNumber()*100,OA[2]->AsNumber()*100);const bool Outdoor=Id==TEXT("ST")||Id==TEXT("RF");
        FWardBookSite3D Site;Site.Id=Id;Site.Name=String(L,TEXT("name"));Site.Origin=O;Site.Outdoor=Outdoor;
        const auto& B=Array(L,TEXT("bounds"));Site.Bounds=FBox(O+FVector(B[0]->AsNumber()*100,B[1]->AsNumber()*100,0),O+FVector((B[0]->AsNumber()+B[2]->AsNumber())*100,(B[1]->AsNumber()+B[3]->AsNumber())*100,Height));
        const auto& View=Array(L,TEXT("capture"));Site.Capture=O+FVector(View[0]->AsNumber()*100,View[1]->AsNumber()*100,View[2]->AsNumber()*100);Site.View=FRotator(View[3]->AsNumber(),View[4]->AsNumber(),0);Sites.Add(Site);
        for(auto R:Array(L,TEXT("rooms")))
        {
            Json Room=R->AsObject();const auto& A=Array(Room,TEXT("rect"));if(A.Num()!=4)continue;
            FWardBookRoom3D Row;Row.Id=String(Room,TEXT("id"));Row.Name=String(Room,TEXT("name"));Row.Location=Id;
            const FVector Min=O+FVector(A[0]->AsNumber()*100,A[1]->AsNumber()*100,0),Size(A[2]->AsNumber()*100,A[3]->AsNumber()*100,Height);
            Row.Bounds=FBox(Min,Min+Size);Row.Enclosed=Boolean(Room,TEXT("enclosed"),true);Row.MinimumArea=Number(Room,TEXT("minimum_area_m2"));const bool North=String(Room,TEXT("door_side"))==TEXT("south");
            const auto& Door=Array(Room,TEXT("door"));Row.Door=O+FVector(Door[0]->AsNumber()*100,Door[1]->AsNumber()*100,98);
            Row.Inside=Row.Door+FVector(0,North?-140:140,0);Row.Outside=Row.Door+FVector(0,North?140:-140,0);
            const int Index=Rooms.Add(Row);Rect(O,A,Row.Enclosed?Index:INDEX_NONE,!Row.Enclosed);
            FPortal P;P.Id=Row.Id;P.Zone=Row.Id;P.Type=String(Room,TEXT("door_type"),TEXT("ordinary"));P.Side=String(Room,TEXT("door_side"));P.At=Row.Door;P.At.Z=O.Z;if(Row.Id==TEXT("S06"))P.Width=400;if(Row.Enclosed||Row.Id==TEXT("S06"))Portals.Add(P);
            for(auto Extra:Array(Room,TEXT("extra_doors"))){Json E=Extra->AsObject();const auto& A2=Array(E,TEXT("at"));if(A2.Num()!=2)continue;FPortal Q;Q.Id=Row.Id+TEXT("_")+String(E,TEXT("role"))+FString::FromInt(Portals.Num());Q.Zone=Row.Id;Q.Type=String(E,TEXT("type"));Q.Side=String(E,TEXT("side"));Q.Role=String(E,TEXT("role"));Q.InsideOnly=Boolean(E,TEXT("inside_only"));Q.Width=Number(E,TEXT("width"),2)*100;Q.At=O+FVector(A2[0]->AsNumber()*100,A2[1]->AsNumber()*100,0);Portals.Add(Q);}
        }
        for(auto C:Array(L,TEXT("corridors")))Rect(O,C->AsArray(),INDEX_NONE,Outdoor);
        for(auto C:Array(L,TEXT("cores")))Rect(O,Array(C->AsObject(),TEXT("rect")),INDEX_NONE,Outdoor);
    }
    for(auto V:Array(WardBookMapData(),TEXT("site_entries")))
    {Json E=V->AsObject();FPortal P;P.Id=String(E,TEXT("id"));P.Role=TEXT("site_entry");P.Side=String(E,TEXT("side"));P.Type=TEXT("ordinary");P.At=Vector(Array(E,TEXT("at")))*100;P.Width=Number(E,TEXT("width"),2)*100;Portals.Add(P);}
    for(auto V:Array(WardBookMapData(),TEXT("escape_niches")))
    {Json N=V->AsObject();FPortal P;P.Id=String(N,TEXT("id"));P.Role=TEXT("niche");P.Side=String(N,TEXT("side"));P.Type=TEXT("niche");P.At=Vector(Array(N,TEXT("entry")))*100;Portals.Add(P);}
    // The book specifies six separate buildings around a covered gallery. Their
    // floor plans are preserved; the spacing of the buildings is chosen here.
    if(Object(WardBookMapData(),TEXT("isolation_airlock"))->HasField(TEXT("at")))
    {FPortal P;P.Id=TEXT("TUNNEL_ISOLATION");P.Zone=TEXT("E10");P.Type=TEXT("shutter");P.Side=TEXT("south");P.Width=400;P.At=Vector(Array(Object(WardBookMapData(),TEXT("isolation_airlock")),TEXT("at")))*100;Portals.Add(P);}
    for(auto V:Array(WardBookMapData(),TEXT("passages"))){const auto& A=V->AsArray();if(A.Num()!=5)continue;for(int X=FMath::RoundToInt(A[0]->AsNumber());X<FMath::RoundToInt(A[0]->AsNumber())+A[2]->AsNumber();++X)for(int Y=FMath::RoundToInt(A[1]->AsNumber());Y<FMath::RoundToInt(A[1]->AsNumber())+A[3]->AsNumber();++Y){const FIntVector K(X,Y,FMath::RoundToInt(A[4]->AsNumber()*100));const auto* Old=Cells.Find(K);if(!Old||Old->Room==INDEX_NONE)Cells.Add(K,{INDEX_NONE,false});else if(Old->Outdoor){const int CellOwner=Old->Room;Cells.Add(K,{CellOwner,false});}}}
    // Do not replace rooms in the outdoor plan when a gallery touches the site.
    TArray<FIntVector> Keys;Cells.GetKeys(Keys);Keys.Sort([](const FIntVector& A,const FIntVector& B){return A.Z==B.Z?(A.Y==B.Y?A.X<B.X:A.Y<B.Y):A.Z<B.Z;});
    struct FWallSeries{FVector At,Size;int Mat=0,Axis=0;bool Collision=true;TArray<double> Centres;};
    TMap<FString,FWallSeries> WallSeries;
    auto Wall=[&](FVector At,FVector Size,int Mat,bool Collision=true,int Direction=-1)
    {
        const int Axis=Direction>=0?Direction:Size.X>Size.Y?0:1;const double Fixed=Axis==0?At.Y:At.X;
        const FString Key=FString::Printf(TEXT("%d_%d_%d_%.1f_%.1f_%.1f_%.1f_%.1f"),Mat,Collision?1:0,Axis,Fixed,At.Z,Size.Z,Axis==0?Size.Y:Size.X,Axis==0?Size.X:Size.Y);
        auto& S=WallSeries.FindOrAdd(Key);S.At=At;S.Size=Size;S.Mat=Mat;S.Axis=Axis;S.Collision=Collision;S.Centres.Add(Axis==0?At.X:At.Y);
    };
    TMap<FIntVector,TArray<int32>> PortalLines;
    for(int32 I=0;I<Portals.Num();++I){const auto& P=Portals[I];const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");PortalLines.FindOrAdd(FIntVector(FMath::RoundToInt(NS?P.At.Y:P.At.X),FMath::RoundToInt(P.At.Z),NS?1:0)).Add(I);}
    TMap<FString,TArray<Json>> ZoneWindows;for(auto V:Array(WardBookMapData(),TEXT("windows"))){Json W=V->AsObject();ZoneWindows.FindOrAdd(String(W,TEXT("zone"))).Add(W);}
    for(const auto& K:Keys)
    {
        const auto& Cell=Cells[K];const FVector At(K.X*100+50,K.Y*100+50,K.Z);
        // Merge coplanar rows instead of creating tens of thousands of floor bodies.
        const auto* Previous=Cells.Find(K-FIntVector(1,0,0));
        if(!FloorHole(K.X,K.Y,K.Z)&&(!Previous||Previous->Outdoor!=Cell.Outdoor||FloorHole(K.X-1,K.Y,K.Z)||K.X%24==0))
        {
            int End=K.X+1;while(const auto* C=Cells.Find(FIntVector(End,K.Y,K.Z))){if(End%24==0||C->Outdoor!=Cell.Outdoor||FloorHole(End,K.Y,K.Z))break;++End;}
            Box(FVector((K.X+End)*50,K.Y*100+50,K.Z-10),FVector((End-K.X)*100,100,20),Cell.Outdoor?7:0);
        }
        auto CeilingHole=[&](int X){return FloorHole(X,K.Y,K.Z+480);};
        if(!Cell.Outdoor&&!CeilingHole(K.X)&&(!Previous||Previous->Outdoor||CeilingHole(K.X-1)||K.X%24==0))
        {
            int End=K.X+1;while(const auto* C=Cells.Find(FIntVector(End,K.Y,K.Z))){if(End%24==0||C->Outdoor||CeilingHole(End))break;++End;}
            Box(FVector((K.X+End)*50,K.Y*100+50,K.Z+Height+8),FVector((End-K.X)*100,100,16),3);
        }
        // One copy of every boundary. Corridors flow into each other freely.
        for(int Side=0;Side<4;++Side)
        {
            const FIntVector D=Side==0?FIntVector(1,0,0):Side==1?FIntVector(0,1,0):Side==2?FIntVector(-1,0,0):FIntVector(0,-1,0);
            const auto* Other=Cells.Find(K+D);if(Other&&Side>=2)continue;
            if(Other&&((Other->Room==Cell.Room&&Other->Outdoor==Cell.Outdoor)||(Other->Outdoor&&Cell.Outdoor)))continue;
            if(!Other&&Cell.Outdoor)continue;
            const FVector Mid=At+FVector(D.X*50,D.Y*50,0);
            const int Room=Cell.Room>=0?Cell.Room:Other?Other->Room:INDEX_NONE;
            // Clip each metre of wall to the exact aperture. Dropping a whole
            // cell at a fractional door centre left up to half-metre side gaps.
            TArray<FVector2D> Openings;const double Centre=D.Y!=0?Mid.X:Mid.Y,Lo=Centre-50,Hi=Centre+50;
            if(const auto* Candidates=PortalLines.Find(FIntVector(FMath::RoundToInt(D.Y!=0?Mid.Y:Mid.X),K.Z,D.Y!=0?1:0)))for(int32 I:*Candidates)
            {const auto& P=Portals[I];const double C=D.Y!=0?P.At.X:P.At.Y;const double A=FMath::Max(Lo,C-P.Width*.5),B=FMath::Min(Hi,C+P.Width*.5);if(B>A+.01)Openings.Add(FVector2D(A,B));}
            Openings.Sort([](const FVector2D& A,const FVector2D& B){return A.X<B.X;});
            const float H=Height;
            const int Shell=Room>=0&&Rooms[Room].Location==TEXT("RF")?11:Room>=0&&Rooms[Room].Location==TEXT("ST")?10:Room>=0?SiteWall.FindRef(Rooms[Room].Location):1;const int Lower=Room>=0&&SiteLower.Contains(Rooms[Room].Location)?SiteLower.FindRef(Rooms[Room].Location):2;
            bool Glazed=false,SecureWindow=false,BridgeGlass=false;
            if(Room<0)for(auto V:Array(Object(WardBookMapData(),TEXT("campus_upgrade")),TEXT("gallery_rects")))
            {
                const auto& A=V->AsArray();if(A.Num()!=5||FMath::Abs(At.Z-A[4]->AsNumber()*100)>1)continue;
                const double X=A[0]->AsNumber()*100,Y=A[1]->AsNumber()*100,GW=A[2]->AsNumber()*100,GD=A[3]->AsNumber()*100;
                if(Mid.X>=X-1&&Mid.X<=X+GW+1&&Mid.Y>=Y-1&&Mid.Y<=Y+GD+1&&(FMath::Abs(Mid.X-X)<1||FMath::Abs(Mid.X-X-GW)<1||FMath::Abs(Mid.Y-Y)<1||FMath::Abs(Mid.Y-Y-GD)<1)){Glazed=true;BridgeGlass=true;break;}
            }
            if(Room>=0)if(const auto* Windows=ZoneWindows.Find(Rooms[Room].Id))for(const Json& W:*Windows)
            {
                const auto& A=Array(W,TEXT("at"));if(A.Num()!=2)continue;FVector O=FVector::ZeroVector;
                for(const auto& S:Sites)if(S.Id==Rooms[Room].Location){O=S.Origin;break;}
                const bool NS=D.Y!=0;const FString WindowSide=String(W,TEXT("side"));
                if((NS&&(WindowSide!=TEXT("north")&&WindowSide!=TEXT("south")))||(!NS&&(WindowSide!=TEXT("west")&&WindowSide!=TEXT("east"))))continue;
                const double Fixed=NS?Mid.Y-O.Y:Mid.X-O.X,WallAt=A[NS?1:0]->AsNumber()*100;
                const double Along=NS?Mid.X-O.X:Mid.Y-O.Y,WindowAt=(A[NS?0:1]->AsNumber()+Number(W,TEXT("offset")))*100;
                if(FMath::Abs(Fixed-WallAt)<1&&FMath::Abs(Along-WindowAt)<Number(W,TEXT("width"))*50+5)
                {Glazed=true;SecureWindow=String(W,TEXT("protection"))==TEXT("security_mesh");break;}
            }
            auto Segment=[&](double A,double B,bool Opening)
            {
                if(B-A<.01)return;FVector M=Mid;if(D.Y!=0)M.X=(A+B)*.5;else M.Y=(A+B)*.5;
                auto Strip=[&](FVector P,FVector S,int Material,bool Collision=true)
                {
                    Wall(P,S,Material,Collision,D.Y?0:1);
                    const bool Exterior=K.Z>=0&&K.Z<1440&&(!Other||(Other->Outdoor!=Cell.Outdoor));
                    if(Exterior&&(Material==Shell||Material==Lower))
                    {
                        FVector Out(D.X,D.Y,0);if(Cell.Outdoor)Out=-Out;
                        FVector Skin=S;if(D.Y)Skin.Y=2;else Skin.X=2;
                        Wall(P+Out*10,Skin,Material==Lower?QualityTile:QualityFacade,false,D.Y?0:1);
                    }
                };
                const FVector Width=D.X!=0?FVector(16,B-A,H):FVector(B-A,16,H);
                if(Opening)Strip(M+FVector(0,0,(260+H)*.5),FVector(Width.X,Width.Y,H-260),Shell);
                else if(Glazed)
                {
                    const float Sill=BridgeGlass?85:110,Head=BridgeGlass?310:210;
                    const int32 Pane = Room>=0 && Other && !Other->Outdoor && !Cell.Outdoor && Palette[FrostedGlass] ? FrostedGlass : 13;
                    Strip(M+FVector(0,0,Sill*.5),FVector(Width.X,Width.Y,Sill),Lower);Strip(M+FVector(0,0,(Sill+Head)*.5),FVector(Width.X,Width.Y,Head-Sill),Pane);Strip(M+FVector(0,0,(Head+380)*.5),FVector(Width.X,Width.Y,380-Head),Shell);
                    for(float Z:{Sill,Head})Strip(M+FVector(0,0,Z),D.X?FVector(23,B-A,5):FVector(B-A,23,5),11,false);
                    FVector Mullion=M;if(D.Y)Mullion.X=A;else Mullion.Y=A;Wall(Mullion+FVector(0,0,(Sill+Head)*.5),D.X?FVector(22,4,Head-Sill):FVector(4,22,Head-Sill),11,false);
                    if(SecureWindow)
                    {
                        // External security screen remains behind the flush internal glazing.
                        for(double U=A+10;U<B;U+=18){FVector Bar=M;if(D.Y)Bar.X=U;else Bar.Y=U;Box(Bar+FVector(D.X*14,D.Y*14,160),D.X?FVector(3,2.8,96):FVector(2.8,3,96),11,false);}
                        for(float Z:{130.f,160.f,190.f})Box(M+FVector(D.X*14,D.Y*14,Z),D.X?FVector(3,B-A,2.8):FVector(B-A,3,2.8),11,false);
                    }
                }
                else
                {
                    Strip(M+FVector(0,0,65),FVector(Width.X,Width.Y,130),Lower);Strip(M+FVector(0,0,(130+H)*.5),FVector(Width.X,Width.Y,H-130),Shell);Strip(M+FVector(0,0,133),FVector(Width.X+2,Width.Y+2,6),5,false);
                    Strip(M+FVector(0,0,8),FVector(Width.X+2,Width.Y+2,16),5,false);
                    Strip(M+FVector(0,0,372),FVector(Width.X+2,Width.Y+2,12),3,false);
                }
            };
            double Cursor=Lo;for(const auto& Opening:Openings){Segment(Cursor,Opening.X,false);if(Opening.Y>Cursor){Segment(FMath::Max(Cursor,Opening.X),Opening.Y,true);Cursor=Opening.Y;}}Segment(Cursor,Hi,false);
            // A 3.8 m room sits on a 4.8 m storey pitch. Close the service
            // interstice at every exterior edge, including gallery boundaries.
            if((!Other&&!Cell.Outdoor)||(Other&&Other->Outdoor!=Cell.Outdoor))
            {
                const FVector Band=D.X!=0?FVector(24,100,100):FVector(100,24,100);
                Wall(Mid+FVector(0,0,430),Band,11);
                Wall(Mid+FVector(0,0,478),D.X!=0?FVector(32,100,8):FVector(100,32,8),5,false);
            }
        }
        if(!Cell.Outdoor&&Cell.Room==INDEX_NONE&&K.X%8==0&&K.Y%8==2)
        {
            LightAnchors.Add(At+FVector(0,0,342));
            const FVector Lamp=At+FVector(0,0,365);
            Box(Lamp,FVector(130,26,8),WhiteLampMaterial(Lamp),false);
        }
    }
    for(auto& Entry:WallSeries)
    {
        auto& S=Entry.Value;S.Centres.Sort();int Start=0;
        while(Start<S.Centres.Num())
        {
            int End=Start;while(End+1<S.Centres.Num()&&FMath::Abs(S.Centres[End+1]-S.Centres[End]-100)<.1)++End;
            FVector At=S.At,Size=S.Size;const double Centre=(S.Centres[Start]+S.Centres[End])*.5,Extra=S.Centres[End]-S.Centres[Start];
            if(S.Axis==0){At.X=Centre;Size.X+=Extra;}else{At.Y=Centre;Size.Y+=Extra;}
            Box(At,Size,S.Mat,S.Collision);Start=End+1;
        }
    }
    for(const auto& R:Rooms)
    {
        const bool Outdoor=R.Location==TEXT("ST")||R.Location==TEXT("RF");
        if(R.Enclosed){const FVector Centre=R.Bounds.GetCenter();LightAnchors.Add(FVector(Centre.X,Centre.Y,R.Bounds.Min.Z+342));const FVector Lamp(Centre.X,Centre.Y,R.Bounds.Min.Z+365);Box(Lamp,FVector(130,26,8),WhiteLampMaterial(Lamp),false);}
        const bool South=R.Inside.Y<R.Outside.Y;Sign(R.Id,R.Door+FVector(0,South?12:-12,188),FRotator(0,South?90:-90,0),24);
    }
    // A real two-flight staircase, 16 cm risers and 28 cm treads. No teleporters.
    for(auto V:Array(WardBookMapData(),TEXT("stairs")))
    {
    Json Stair=V->AsObject();const auto& R=Array(Stair,TEXT("rect"));const FVector Offset(R[0]->AsNumber()*100-2800,R[1]->AsNumber()*100,0);
    auto StairBox=[&](FVector P,FVector S,int M){Box(P+Offset,S,M);};
    for(int Z=FMath::RoundToInt(Number(Stair,TEXT("z_min"))*100);Z<Number(Stair,TEXT("z_max"))*100;Z+=480)
    {
        StairBox(FVector(2950,740,Z-10),FVector(300,120,20),0);
        StairBox(FVector(2950,200,Z+120),FVector(300,120,240),0);
        for(int I=0;I<15;++I)
        {
            const float Rise=(I+1)*16;
            StairBox(FVector(2878,680-(I+.5f)*28,Z+Rise*.5f),FVector(136,28,Rise),0);
            StairBox(FVector(3022,260+(I+.5f)*28,Z+240+Rise*.5f),FVector(136,28,Rise),0);
        }
        StairBox(FVector(2950,740,Z+Pitch-10),FVector(300,120,20),0);
        for(float X:{2800.f,3100.f})StairBox(FVector(X,340,Z+50),FVector(8,680,100),5);
        StairBox(FVector(2950,450,Z+245),FVector(8,400,90),5);
        Sign(TEXT("B2  /  B1  /  1F  /  2F  /  3F  /  RF"),FVector(3220,560,Z+245)+Offset,FRotator(0,0,0),14);
        LightAnchors.Add(FVector(2950,230,Z+360)+Offset);StairFlights+=2;
    }
    }
    BuildHospitalDetails();
    BuildStoryArchitecture();
    BuildCars31();
    BuildPrototypeFurnishings();
    BuildQualityDressing();
    BuildEnvironmentDressing();
    BuildDetailDressing();
    BuildHorrorPresentation28();
    BuildFacilities37();
    BuildLift();
    for(int I=0;I<14;++I)
    {
        auto* Light=NewObject<UPointLightComponent>(this);
        Light->SetupAttachment(RootComponent);
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Lumens);
        Light->SetIntensity(8000.f);
        Light->SetAttenuationRadius(1900.f);
        Light->SetLightColor(FLinearColor(.65f,.82f,.83f));
        Light->SetCastShadows(I<4);
        Light->SetLightingChannels(true,true,false);
        // The renderer stretches a point source along its local Z axis.
        // Rotate that axis onto the long side of the ceiling fixture.
        Light->SetRelativeRotation(FRotator(-90,0,0));
        Light->SetSourceLength(68.f);
        Light->SetSourceRadius(8.f);
        Light->SetSoftSourceRadius(24.f);
        Light->ContactShadowLengthInWS=true;
        Light->ContactShadowLength=I<2?26.f:0.f;
        Light->RegisterComponent();
        AddInstanceComponent(Light);
        LightPool.Add(Light);
    }
    auto* Moon=NewObject<UDirectionalLightComponent>(this);Moon->SetupAttachment(RootComponent);Moon->SetMobility(EComponentMobility::Movable);Moon->SetRelativeRotation(FRotator(-38,-25,0));Moon->SetIntensity(120.f);Moon->SetLightColor(FLinearColor(.45,.60,.72));Moon->SetLightingChannels(false,true,false);Moon->SetCastShadows(true);Moon->ContactShadowLengthInWS=true;Moon->ContactShadowLength=35.f;Moon->RegisterComponent();AddInstanceComponent(Moon);
    Daylight=Moon;
    for(int I=0;I<5;++I)
    {
        auto* Fog=NewObject<ULocalFogVolumeComponent>(this);Fog->SetupAttachment(RootComponent);Fog->SetMobility(EComponentMobility::Movable);
        Fog->RadialFogExtinction=.008f;Fog->HeightFogExtinction=.24f;Fog->HeightFogFalloff=16;Fog->HeightFogOffset=-.08f;Fog->FogAlbedo=FLinearColor(.43f,.19f,.15f);Fog->FogEmissive=FLinearColor(.00065f,.00023f,.00017f);Fog->SetRelativeScale3D(FVector(1.3f));Fog->SetVisibility(false);Fog->RegisterComponent();AddInstanceComponent(Fog);FloorMist.Add(Fog);
    }
    SetBright(Campaign->SettingBool(TEXT("inspection_bright"),false));
    for(auto& G:Groups){G.Value->bAutoRebuildTreeOnInstanceChanges=true;G.Value->BuildTreeIfOutdated(false,true);}
    FitCarsAndMarkings33();
    AuditFacilities37(); // Select safe swing directions in normal gameplay as well as checks.
    UpdateLighting(Start());
    FinishWardBookMapConstruction();
    UE_LOG(LogTemp,Display,TEXT("BOOK_WORLD_BUILT %d rooms, %d locations, %d stair flights, %d cells, %d instances"),Rooms.Num(),Sites.Num(),StairFlights,Cells.Num(),MeshInstances);
}
void AEWardBookWorld::UpdateLighting(FVector Player)
{
    // The animated intensity/fog still updates each frame. Selecting fixtures
    // is needed only at the same bounded rate in either viewing mode.
    if(LightSelectionClock<.24f)return;LightSelectionClock=0;
    const FString Region=LocationAt(Player);
    const bool Performance=EWardGraphics::Current()==2;
    bool OpenAir=Player.Z>1400||(Player.Z>-100&&Player.Z<400&&(Region==TEXT("ST")||Region==TEXT("gallery")));
    for(const auto& R:Rooms)if(R.Enclosed&&Player.Z>R.Bounds.Min.Z&&Player.Z<R.Bounds.Max.Z&&Player.X>R.Bounds.Min.X&&Player.X<R.Bounds.Max.X&&Player.Y>R.Bounds.Min.Y&&Player.Y<R.Bounds.Max.Y){OpenAir=false;break;}
    if(Daylight)
    {
        const FRotator Direction=Bright?FRotator(-38,-25,0):FRotator(-29,68,0);
        if(!Daylight->GetRelativeRotation().Equals(Direction,.01f))Daylight->SetRelativeRotation(Direction);
        if(!Bright)Daylight->SetLightColor(FLinearColor(.69f,.80f,1.f));
    }
    if(Mode.IsValid()&&Mode->Hero())
    {
        auto& P=Mode->Hero()->Camera->PostProcessSettings;
        const bool HighQuality = EWardGraphics::Current() == 0;
        const bool Indirect = HighQuality && !Bright;
        P.bOverride_DynamicGlobalIlluminationMethod = true;
        P.DynamicGlobalIlluminationMethod = Indirect ? EDynamicGlobalIlluminationMethod::ScreenSpace : EDynamicGlobalIlluminationMethod::None;
        P.bOverride_ReflectionMethod = true;
        P.ReflectionMethod = EReflectionMethod::ScreenSpace;
        P.bOverride_AutoExposureMinBrightness = true;
        P.bOverride_AutoExposureMaxBrightness = true;
        const float Exposure = Bright ? (OpenAir ? 9.5f : 4.8f) : (OpenAir ? 2.45f : 2.55f);
        P.AutoExposureMinBrightness = P.AutoExposureMaxBrightness = Exposure;
        P.AmbientCubemapIntensity = Bright ? 1.4f : OpenAir ? .36f : .135f;
        P.bOverride_AmbientCubemapTint = true;
        P.AmbientCubemapTint = Bright ? FLinearColor::White : OpenAir
            ? FLinearColor(.81f,.89f,1.f) : FLinearColor(.94f,.97f,1.f);
    }

    // Exclude anchors that every consumer below would reject anyway, before
    // sorting. Keep the full 22 m radius needed by floor mist in horror mode.
    TArray<int> Order;const double WhiteSearchSq=FMath::Square(Bright?4200.:2200.);
    for(int I=0;I<LightAnchors.Num();++I)if(FMath::Abs(LightAnchors[I].Z-Player.Z)<440&&FVector::DistSquared(Player,LightAnchors[I])<=WhiteSearchSq)Order.Add(I);
    Order.Sort([&](int A,int B){return FVector::DistSquared(Player,LightAnchors[A])<FVector::DistSquared(Player,LightAnchors[B]);});
    ActiveWhiteAnchors.Reset();
    ActiveWhiteIds.Reset();
    TArray<int> Selected;
    if(Bright)Selected=Order;
    else
    {
        FCollisionQueryParams Q(SCENE_QUERY_STAT(HorrorLampVisibility),false,Mode.IsValid()?Mode->Hero():nullptr);
        for(int Id:Order)
        {
            const FVector Anchor=LightAnchors[Id];
            if(FVector::DistSquared(Player,Anchor)>FMath::Square(OpenAir?2200.f:2050.f))break;
            if(LampPower(Anchor,-1)<.1f)continue;
            if(Selected.ContainsByPredicate([&](int Other){return FVector::Dist2D(Anchor,LightAnchors[Other])<280;}))continue;
            FHitResult Hit;if(GetWorld()->LineTraceSingleByChannel(Hit,Player+FVector(0,0,50),Anchor-FVector(0,0,35),ECC_Visibility,Q))continue;
            Selected.Add(Id);if(Selected.Num()>=(Performance?4:5))break;
        }
    }
    for(int I=0;I<LightPool.Num();++I)
    {
        auto* Light=LightPool[I].Get();
        const bool On=Selected.IsValidIndex(I);
        Light->SetVisibility(On);
        if(!On)continue;
        const FVector Anchor=LightAnchors[Selected[I]];
        Light->SetRelativeLocation(Anchor);
        // Ceiling tubes retain a cool neutral pool. The separate emergency lamps light the red areas.
        const int32 Fixture=FMath::Abs(FMath::FloorToInt(Anchor.X/100)*13+FMath::FloorToInt(Anchor.Y/100)*19);
        FLinearColor Tint=Bright?FLinearColor(.97f,.93f,.86f):FLinearColor(.82f,.9f,1.f);
        if(!Bright&&!OpenAir)
        {
            if(Region==TEXT("B1")||Region==TEXT("B2"))Tint=FLinearColor(.85f,.88f,.95f);
            if(Fixture%6==1)Tint=FLinearColor(1.f,.76f,.57f);
        }
        Light->SetLightColor(Tint);
        Light->SetAttenuationRadius(Bright?1900.f:1040.f);
        Light->SetCastShadows(I<(Performance?2:(Bright?4:3)));
        ActiveWhiteIds.Add(I);
        ActiveWhiteAnchors.Add(Anchor);
        if(Bright)Light->SetIntensity(8000.f);
    }
    MistAnchors.Reset();for(int Id:Order){const FVector A=LightAnchors[Id];if(FVector::DistSquared(Player,A)>FMath::Square(2200.f))break;if(MistAnchors.ContainsByPredicate([&](const FVector& B){return FVector::DistSquared(A,B)<FMath::Square(580.f);}))continue;MistAnchors.Add(A);if(MistAnchors.Num()>=FloorMist.Num())break;}
    TArray<int32> RedOrder;
    for(int32 I=0;I<EmergencyAnchors.Num();++I)
        if(FMath::Abs(EmergencyAnchors[I].Z-Player.Z)<360&&FVector::DistSquared(Player,EmergencyAnchors[I])<FMath::Square(3200.f))
            RedOrder.Add(I);
    RedOrder.Sort([&](int32 A,int32 B){return FVector::DistSquared(Player,EmergencyAnchors[A])<FVector::DistSquared(Player,EmergencyAnchors[B]);});
    TArray<int32, TInlineAllocator<8>> VisibleRed;
    const FVector Eye=Player+FVector(0,0,58);
    FCollisionQueryParams RedQuery(SCENE_QUERY_STAT(WardEmergencyOcclusion),false,Mode.IsValid()?Mode->Hero():nullptr);
    for(int32 I=0;I<FMath::Min(RedOrder.Num(),16)&&VisibleRed.Num()<EmergencyPool.Num();++I)
    {
        const int32 Id=RedOrder[I];
        const FVector Anchor=EmergencyAnchors[Id];
        FHitResult Hit;
        if(!Bright&&!OpenAir&&GetWorld()->LineTraceSingleByChannel(Hit,Eye,Anchor,ECC_Visibility,RedQuery)
            &&FVector::DistSquared(Hit.ImpactPoint,Anchor)>FMath::Square(48.f))
            continue;
        VisibleRed.Add(Id);
    }
    for(int32 I=0;I<EmergencyPool.Num();++I)
    {
        const bool On=VisibleRed.IsValidIndex(I);
        EmergencyPool[I]->SetVisibility(On);
        if(On)EmergencyPool[I]->SetRelativeLocation(EmergencyAnchors[VisibleRed[I]]);
    }
    if(Mode.IsValid()&&Describe(Player).Contains(TEXT("RH00")))Mode->Flag(TEXT("discovered_RH00"),true);
    UpdateQualityLights(Player,OpenAir);
}
FString AEWardBookWorld::LocationAt(FVector Player)const
{
    const int Z=FMath::RoundToInt((Player.Z-96)/Pitch)*Pitch;
    for(const auto& R:Rooms)if(FMath::Abs(R.Bounds.Min.Z-Z)<1&&Player.X>=R.Bounds.Min.X&&Player.X<=R.Bounds.Max.X&&Player.Y>=R.Bounds.Min.Y&&Player.Y<=R.Bounds.Max.Y)return R.Location;
    for(const auto& S:Sites)if(FMath::Abs(S.Origin.Z-Z)<1&&Player.X>=S.Bounds.Min.X&&Player.X<=S.Bounds.Max.X&&Player.Y>=S.Bounds.Min.Y&&Player.Y<=S.Bounds.Max.Y)return S.Id;
    return TEXT("gallery");
}
FString AEWardBookWorld::Describe(FVector Player)const
{
    const FString Id=LocationAt(Player);for(const auto& R:Rooms)if(R.Location==Id&&Player.X>=R.Bounds.Min.X&&Player.X<=R.Bounds.Max.X&&Player.Y>=R.Bounds.Min.Y&&Player.Y<=R.Bounds.Max.Y)return R.Location+TEXT(" / ")+R.Id+TEXT(" · ")+R.Name;
    for(const auto& S:Sites)if(S.Id==Id)return S.Id+TEXT(" · ")+S.Name;
    const auto& Landscape=Array(Object(WardBookMapData(),TEXT("campus_upgrade")),TEXT("landscape_rect"));
    if(Landscape.Num()==4&&Player.X>Landscape[0]->AsNumber()*100&&Player.Y>Landscape[1]->AsNumber()*100&&Player.X<(Landscape[0]->AsNumber()+Landscape[2]->AsNumber())*100&&Player.Y<(Landscape[1]->AsNumber()+Landscape[3]->AsNumber())*100&&(Player.X<-18000||Player.X>28000||Player.Y<-19000||Player.Y>38000))return TEXT("Лес за периметром больницы");
    return TEXT("Крытая галерея между корпусами");
}
bool AEWardBookWorld::ValidPosition(FVector Player)const
{
    if(Player.ContainsNaN()||Player.Z<-1100||Player.Z>1950)return false;
    // Authored guard platforms/stairs are between the standard storey levels.
    // Accept their actual walking surfaces, rather than snapping them to 2F.
    for(auto V:Array(WardBookMapData(),TEXT("raised_access")))
    {
        Json R=V->AsObject();const auto& B=Array(R,TEXT("rect"));if(B.Num()!=4)continue;
        if(Player.X>=B[0]->AsNumber()*100&&Player.X<=(B[0]->AsNumber()+B[2]->AsNumber())*100&&Player.Y>=B[1]->AsNumber()*100&&Player.Y<=(B[1]->AsNumber()+B[3]->AsNumber())*100&&FMath::Abs(Player.Z-Number(R,TEXT("z"))*100-98)<38)return true;
    }
    const int Z=FMath::RoundToInt((Player.Z-96)/Pitch)*Pitch;
    if(Cells.Contains(FIntVector(FMath::FloorToInt(Player.X/100),FMath::FloorToInt(Player.Y/100),Z)))return true;
    if(FMath::Abs(Player.Z-98)<180&&Player.X>-18000&&Player.X<28000&&Player.Y>-19000&&Player.Y<38000)return true;
    const auto& Landscape=Array(Object(WardBookMapData(),TEXT("campus_upgrade")),TEXT("landscape_rect"));
    if(Landscape.Num()==4&&FMath::Abs(Player.Z-98)<180&&Player.X>Landscape[0]->AsNumber()*100&&Player.Y>Landscape[1]->AsNumber()*100&&Player.X<(Landscape[0]->AsNumber()+Landscape[2]->AsNumber())*100&&Player.Y<(Landscape[1]->AsNumber()+Landscape[3]->AsNumber())*100)return true;
    return (StairVoid(FMath::FloorToInt(Player.X/100),FMath::FloorToInt(Player.Y/100))||Shaft(FMath::FloorToInt(Player.X/100),FMath::FloorToInt(Player.Y/100)));
}

FVector AEWardBookWorld::Start()const{return WardData::Vector(Array(WardBookMapData(),TEXT("start")),FVector(-4,-52,.98))*100;}
bool AEWardBookWorld::StairVoid(int X,int Y)const
{
    for(auto V:Array(WardBookMapData(),TEXT("stairs"))){const auto& R=Array(V->AsObject(),TEXT("rect"));if(R.Num()==4&&X>=R[0]->AsNumber()&&X<R[0]->AsNumber()+3&&Y>=R[1]->AsNumber()&&Y<R[1]->AsNumber()+7)return true;}return false;
}
bool AEWardBookWorld::FloorHole(int X,int Y,int Z)const
{
    if(Shaft(X,Y))return true;
    for(auto V:Array(WardBookMapData(),TEXT("stairs"))){Json S=V->AsObject();const auto& R=Array(S,TEXT("rect"));if(R.Num()==4&&Z>Number(S,TEXT("z_min"))*100&&Z<=Number(S,TEXT("z_max"))*100&&X>=R[0]->AsNumber()&&X<R[0]->AsNumber()+3&&Y>=R[1]->AsNumber()&&Y<R[1]->AsNumber()+7)return true;}return false;
}
