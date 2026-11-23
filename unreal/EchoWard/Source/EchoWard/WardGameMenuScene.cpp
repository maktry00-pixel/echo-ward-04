#include "WardTitleScene.h"
#include "WardCampaign.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/DecalComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
using namespace WardData;
namespace {const FVector Annex(0,8000,0);const FVector Rooms[]={{720,-650,0},{1450,650,0},{2150,-650,0}};}
void AEWardTitleScene::BuildGameBackdrop()
{
    GameSet=NewObject<USceneComponent>(this,TEXT("ArchiveMenuSet"));GameSet->SetupAttachment(RootComponent);GameSet->SetRelativeLocation(Annex);GameSet->RegisterComponent();AddInstanceComponent(GameSet);
    auto Mat=[](const TCHAR* Id){return LoadObject<UMaterialInterface>(nullptr,*MaterialPath(Id));};
    auto Variant=[&](UMaterialInterface* Base,FLinearColor Tint,float Rough){auto* M=UMaterialInstanceDynamic::Create(Base,this);M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetScalarParameterValue(TEXT("Roughness"),Rough);return M;};
    auto* Plaster=Mat(TEXT("4ae6673fcc8b19e7"));auto* Wall=Variant(Plaster,FLinearColor(.48,.31,.28),.93);
    auto* Tile=Variant(Mat(TEXT("05227396fb5ca2d8")),FLinearColor(.34,.064,.054),.65);
    auto* Floor=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/GameMenu/M_ArchiveFloor"));
    auto* Metal=Variant(Plaster,FLinearColor(.10,.054,.049),.43);Metal->SetScalarParameterValue(TEXT("Metallic"),.72);
    auto* Door=Variant(Plaster,FLinearColor(.25,.078,.065),.65);
    auto* Lamp=Variant(Plaster,FLinearColor(.82,.43,.34),.4);Lamp->SetVectorParameterValue(TEXT("Emission"),FLinearColor(49,21,15));
    auto* Red=Variant(Plaster,FLinearColor(.25,.003,.001),.35);Red->SetVectorParameterValue(TEXT("Emission"),FLinearColor(25,.02,.005));
    auto Piece=[&](FString Mesh,FVector At,FVector Scale,UMaterialInterface* M,FRotator Rot=FRotator::ZeroRotator){auto* P=Part(Mesh,At,Scale,M,Rot);P->AttachToComponent(GameSet,FAttachmentTransformRules::KeepRelativeTransform);return P;};
    const FString Cube=TEXT("/Engine/BasicShapes/Cube"),Cylinder=TEXT("/Engine/BasicShapes/Cylinder");
    auto Box=[&](FVector At,FVector Size,UMaterialInterface* M){return Piece(Cube,At,Size/100.f,M);};
    Box(FVector(1300,0,-12),FVector(2600,800,24),Floor);Box(FVector(1300,0,370),FVector(2600,830,20),Tile);
    // A wider archive wing with recessed treatment rooms, different from the title corridor.
    for(int Side:{-1,1})
    {
        TArray<float> Openings=Side<0?TArray<float>{720,2150}:TArray<float>{1450};float Last=0;
        for(float X:Openings)
        {
            const float End=X-90;Box(FVector((Last+End)/2,Side*410,240),FVector(End-Last,24,260),Wall);Box(FVector((Last+End)/2,Side*397,58),FVector(End-Last,20,116),Tile);
            Box(FVector((Last+End)/2,Side*381,122),FVector(End-Last,8,7),Metal);Box(FVector(X,Side*410,322),FVector(180,24,96),Wall);
            for(float E:{-94.f,94.f})Box(FVector(X+E,Side*393,139),FVector(9,24,278),Metal);Box(FVector(X,Side*393,281),FVector(198,24,9),Metal);
            auto* H=NewObject<USceneComponent>(this);H->SetupAttachment(GameSet);const FVector Pivot(X-87,Side*420,0);H->SetRelativeLocation(Pivot);H->RegisterComponent();AddInstanceComponent(H);GameDoors.Add(H);
            auto Leaf=[&](FVector At,FVector Size,UMaterialInterface* M){auto* P=Box(At,Size,M);P->AttachToComponent(H,FAttachmentTransformRules::KeepRelativeTransform);P->SetRelativeLocation(At-Pivot);};
            Leaf(FVector(X,Side*420,136),FVector(174,10,268),Door);Leaf(FVector(X-62,Side*408,111),FVector(6,14,24),Metal);Leaf(FVector(X,Side*411,220),FVector(75,3,42),Metal);
            // Genuine room depth, so the model disappears behind a door instead of popping away.
            Box(FVector(X,Side*675,-9),FVector(430,540,18),Floor);Box(FVector(X,Side*945,185),FVector(440,20,370),Tile);
            for(float E:{-218.f,218.f})Box(FVector(X+E,Side*675,185),FVector(16,540,370),Wall);
            Box(FVector(X,Side*675,370),FVector(450,540,20),Tile);Last=X+90;
        }
        Box(FVector((Last+2600)/2,Side*410,240),FVector(2600-Last,24,260),Wall);Box(FVector((Last+2600)/2,Side*397,58),FVector(2600-Last,20,116),Tile);
        // Old utility pipes, collars, wall protection and high moulding.
        for(float Y:{295.f,330.f})Piece(Cylinder,FVector(1300,Side*Y,343),FVector(.07,.07,26),Metal,FRotator(90,0,0));
        for(int I=0;I<11;++I)Box(FVector(I*240+100,Side*321,349),FVector(5,65,6),Metal);
    }
    // Glazed double door at the end, with an illuminated cross-passage behind it.
    for(float Y:{-292.f,292.f})Box(FVector(2600,Y,185),FVector(22,216,370),Wall);
    Box(FVector(2600,0,325),FVector(22,370,90),Wall);
    for(float Y:{-91.f,91.f}){Box(FVector(2580,Y,140),FVector(12,178,275),Door);Box(FVector(2569,Y,208),FVector(3,120,80),Metal);Box(FVector(2560,Y<0?-16:16,115),FVector(9,7,34),Metal);}
    auto Light=[&](FVector At,FLinearColor Color,float Power,float Radius,FRotator Rot){auto* L=NewObject<URectLightComponent>(this);L->SetupAttachment(GameSet);L->SetRelativeLocation(At);L->SetRelativeRotation(Rot);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(Power);L->SetLightColor(Color);L->SetAttenuationRadius(Radius);L->SetSourceWidth(115);L->SetSourceHeight(16);L->SetCastShadows(true);L->SetVolumetricScatteringIntensity(.65);L->RegisterComponent();AddInstanceComponent(L);GameLights.Add(L);return L;};
    for(int I=0;I<4;++I){const float X=480+I*610;Box(FVector(X,0,353),FVector(30,142,9),Metal);Box(FVector(X,0,347),FVector(22,128,3),Lamp);Light(FVector(X,0,339),FLinearColor(1,.31,.21),I==0?850:I==1?1350:I==2?350:900,1050,FRotator(-90,0,0));}
    Light(FVector(1420,690,190),FLinearColor(1,.004,.002),1800,1200,FRotator(0,-90,0));
    Light(FVector(2130,-660,210),FLinearColor(.94,.16,.10),650,950,FRotator(0,90,0));
    auto* AlarmPoint=NewObject<UPointLightComponent>(this);AlarmPoint->SetupAttachment(GameSet);AlarmPoint->SetRelativeLocation(FVector(1940,365,265));AlarmPoint->SetIntensityUnits(ELightUnits::Lumens);AlarmPoint->SetIntensity(800);AlarmPoint->SetLightColor(FLinearColor(1,.005,.002));AlarmPoint->SetAttenuationRadius(1100);AlarmPoint->SetSourceRadius(14);AlarmPoint->RegisterComponent();AddInstanceComponent(AlarmPoint);GameLights.Add(AlarmPoint);Box(FVector(1940,380,265),FVector(20,15,46),Red);
    // Reuse the scanned campaign furniture, preserving each mesh's original PBR material.
    auto Prop=[&](FString Find,FVector At,FRotator Rot,float Scale=1.f)
    {
        Json A;for(const auto& V:Array(Mode->Data,TEXT("instances")))if(String(V->AsObject(),TEXT("path")).Contains(Find)){A=V->AsObject();break;}if(!A)return;
        const FString Path=String(A,TEXT("path")),Prefix=Path.Left(Path.Find(Find))+Find;const FTransform Base=Transform(Object(A,TEXT("transform")));
        for(const auto& V:Array(Mode->Data,TEXT("instances"))){const Json R=V->AsObject();if(!String(R,TEXT("path")).StartsWith(Prefix))continue;const auto T=Transform(Object(R,TEXT("transform")));const auto& M=Array(R,TEXT("materials"));Piece(MeshPath(String(R,TEXT("mesh"))),At+Rot.RotateVector((T.GetLocation()-Base.GetLocation())*Scale),T.GetScale3D()*Scale,M.Num()?Mat(*M[0]->AsString()):Metal,Rot+T.Rotator());}
    };
    Prop(TEXT("wheelchair_01"),FVector(710,285,1),FRotator(0,-35,0),1);
    Prop(TEXT("old_bed"),FVector(500,-290,0),FRotator(0,3,0),.9);
    Prop(TEXT("painted_wooden_cabinet_02_2k"),FVector(1850,325,0),FRotator(0,90,0),.8);
    Prop(TEXT("steel_frame_shelves"),FVector(1100,-325,0),FRotator(0,90,0),.9);
    auto* Linen=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Menu/M_MenuLinen"));FRandomStream Rng(2164);
    for(int I=0;I<29;++I){auto* P=Box(FVector(Rng.FRandRange(360,2450),Rng.FRandRange(-310,300),.4),FVector(Rng.FRandRange(16,24),29,.15),Linen);P->SetRelativeRotation(FRotator(0,Rng.FRandRange(-180,180),0));}
    auto Decal=[&](FString Name,FVector At,FVector Size,FRotator Rot,int I,bool Blood,bool OnWall){auto* D=NewObject<UDecalComponent>(this);D->SetupAttachment(GameSet);auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,*Name),this);M->SetScalarParameterValue(TEXT("Seed"),I*3.17f+2);M->SetScalarParameterValue(TEXT("Wall"),OnWall?1:0);D->SetDecalMaterial(M);D->DecalSize=Size;D->SetRelativeLocation(At);D->SetRelativeRotation(Rot);D->RegisterComponent();AddInstanceComponent(D);if(Blood)++GameBloodDecals;};
    for(int Side:{-1,1})for(int I=0;I<10;++I){const float X=170+I*250;Decal(TEXT("/Game/Campaign/Menu/M_MenuWear"),FVector(X,Side*380,170),FVector(12,110,170),FRotator(0,-Side*90,0),I,false,true);if(I%3==0)Decal(TEXT("/Game/Campaign/MenuEvents/M_MenuBlood"),FVector(X+50,Side*382,125),FVector(8,58,85),FRotator(0,-Side*90,0),I+Side*7,true,true);}
    for(int I=0;I<9;++I)Decal(TEXT("/Game/Campaign/MenuEvents/M_MenuBlood"),FVector(530+I*210,FMath::Sin(I*3.7f)*260,1),FVector(6,70+I%3*15,110),FRotator(90,I*39,0),I+23,true,false);
    for(int I=0;I<4;++I){auto* Fog=NewObject<ULocalFogVolumeComponent>(this);Fog->SetupAttachment(GameSet);Fog->RadialFogExtinction=.025;Fog->HeightFogExtinction=1.5;Fog->HeightFogFalloff=10;Fog->HeightFogOffset=-.08;Fog->FogAlbedo=FLinearColor(.53,.21,.16);Fog->FogEmissive=FLinearColor(.002,.0008,.0006);Fog->SetRelativeLocation(FVector(650+I*600,0,-100));Fog->SetRelativeScale3D(FVector(1.45));Fog->RegisterComponent();AddInstanceComponent(Fog);}
    GameZombie=NewObject<USkeletalMeshComponent>(this,TEXT("ArchiveZombie"));GameZombie->SetupAttachment(GameSet);GameZombie->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Campaign/GameMenu/SK_MenuZombie")));GameZombie->SetCollisionEnabled(ECollisionEnabled::NoCollision);GameZombie->SetReceivesDecals(false);GameZombie->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;GameZombie->RegisterComponent();AddInstanceComponent(GameZombie);GameZombie->PlayAnimation(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/GameMenu/A_MenuZombieWalk")),true);GameZombie->SetComponentTickEnabled(false);GameZombie->SetRelativeScale3D(FVector(8));
    GameCamera=GetWorld()->SpawnActor<ACameraActor>();auto* C=GameCamera->GetCameraComponent();C->SetConstraintAspectRatio(false);C->SetFieldOfView(71);C->PostProcessSettings=Camera->GetCameraComponent()->PostProcessSettings;
    auto& PP=C->PostProcessSettings;PP.AutoExposureMinBrightness=4.0;PP.AutoExposureMaxBrightness=4.0;PP.DepthOfFieldFstop=2.8;PP.DepthOfFieldFocalDistance=1250;PP.VignetteIntensity=.24;PP.FilmGrainIntensity=.045;PP.BloomIntensity=.35;
    GameSet->SetVisibility(false,true);
}
void AEWardTitleScene::SetGameBackdrop(bool Enabled)
{
    if(GameBackdropActive==Enabled)return;if(Enabled&&!GameSet)BuildGameBackdrop();GameBackdropActive=Enabled;
    if(Active&&Enabled){GameAge=0;TickGameBackdrop(0);}RefreshBackdrop();
}
void AEWardTitleScene::TickGameBackdrop(float Delta)
{
    GameAge+=Delta;const float Cycle=FMath::Fmod(GameAge,72.f),Segment=FMath::Fmod(Cycle,24.f);const int From=FMath::FloorToInt(Cycle/24),To=(From+1)%3;
    ZombieInside=Segment>=14;ZombieRoom=ZombieInside?To:From;ZombieRoomSeconds=ZombieInside?Segment-14:0;
    const FVector A=Rooms[From],B=Rooms[To],Exit(A.X,A.Y<0?-245:245,0),Entry(B.X,B.Y<0?-245:245,0);
    FVector At=A,Direction=Exit-A;
    if(Segment<3){At=FMath::Lerp(A,Exit,Segment/3);}
    else if(Segment<11){At=FMath::Lerp(Exit,Entry,(Segment-3)/8);Direction=Entry-Exit;}
    else if(Segment<14){At=FMath::Lerp(Entry,B,(Segment-11)/3);Direction=B-Entry;}
    else{At=B;Direction=B-Entry;}
    // Doors array is built left-near, left-far, right-middle; route uses near,middle,far.
    const int DoorForRoom[]={0,2,1};for(int I=0;I<3;++I){float Open=0;if(I==From&&Segment<4)Open=FMath::Min(FMath::Clamp(Segment/.8f,0.f,1.f),FMath::Clamp((4-Segment),0.f,1.f));if(I==To&&Segment>9&&Segment<15)Open=FMath::Min(FMath::Clamp((Segment-9),0.f,1.f),FMath::Clamp((15-Segment),0.f,1.f));GameDoors[DoorForRoom[I]]->SetRelativeRotation(FRotator(0,(Rooms[I].Y<0?-1:1)*92*Open,0));}
    float HeadingYaw=Direction.Rotation().Yaw;
    if(Segment>=3&&Segment<3.65f)HeadingYaw=FMath::Lerp((Exit-A).Rotation().Yaw,(Exit-A).Rotation().Yaw+FMath::FindDeltaAngleDegrees((Exit-A).Rotation().Yaw,(Entry-Exit).Rotation().Yaw),FMath::SmoothStep(0.f,1.f,(Segment-3)/.65f));
    if(Segment>=11&&Segment<11.65f)HeadingYaw=FMath::Lerp((Entry-Exit).Rotation().Yaw,(Entry-Exit).Rotation().Yaw+FMath::FindDeltaAngleDegrees((Entry-Exit).Rotation().Yaw,(B-Entry).Rotation().Yaw),FMath::SmoothStep(0.f,1.f,(Segment-11)/.65f));
    const FRotator Heading(0,HeadingYaw,0);const FVector MeshAnchor(-16.38435*8,9.13535*8,1.8);
    GameZombie->SetRelativeLocation(At+Heading.RotateVector(MeshAnchor));GameZombie->SetRelativeRotation(Heading);GameZombie->SetVisibility(true);
    if(!ZombieInside){GameZombie->TickAnimation(Delta*.85f,false);GameZombie->RefreshBoneTransforms();GameZombie->UpdateComponentToWorld();}
    LastFootstep-=Delta;if(!ZombieInside&&LastFootstep<=0){Sound(TEXT("MENU/zombie_walk"),.055f);LastFootstep=.84f;}
    const int Beat=int(Segment<1?From*2:Segment>=10&&Segment<11?To*2+1:-1);if(Beat>=0&&Beat!=GameSoundBeat){Sound(TEXT("creak"),.065f);GameSoundBeat=Beat;}else if(Beat<0)GameSoundBeat=-1;
    const float T=GameAge;GameCamera->SetActorLocationAndRotation(Origin+Annex+FVector(60+35*FMath::Sin(T*.018),-34+FMath::Sin(T*.12)*1.4,128+FMath::Sin(T*.17)*.55),FRotator(-1.5,-1,0));
    for(int I=0;I<GameLights.Num();++I){float Power=I==0?850:I==1?1350:I==2?350:I==3?900:I==4?1800:I==5?650:800;float F=1;if(I==1&&FMath::Fmod(T,13.7f)>11.8f)F=FMath::Sin(T*43)>0?.62f:.035f;if(I==6)F=.22f+.78f*FMath::Pow(FMath::Max(0.f,FMath::Sin(T*1.8f)),6);GameLights[I]->SetIntensity(Power*F*(1-Fade));}
    TInlineComponentArray<ULocalFogVolumeComponent*> Fogs;GetComponents(Fogs);int I=0;for(auto* Fog:Fogs)if(Fog->GetAttachParent()==GameSet){Fog->SetRelativeLocation(FVector(650+I*600+FMath::Sin(T*.1f+I)*35,FMath::Sin(T*.12f+I)*20,-100));++I;}
}
