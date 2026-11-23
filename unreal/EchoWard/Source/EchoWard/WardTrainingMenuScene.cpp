#include "WardTitleScene.h"
#include "WardCampaign.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
using namespace WardData;
namespace {const FVector TrainingOffset(0,16000,0);}
void AEWardTitleScene::SetTrainingBackdrop(bool Enabled)
{
    if(TrainingBackdropActive==Enabled)return;if(Enabled&&!TrainingSet)BuildTrainingBackdrop();TrainingBackdropActive=Enabled;
    if(Active&&Enabled){TrainingAge=0;TickTrainingBackdrop(0);}RefreshBackdrop();
}
void AEWardTitleScene::BuildTrainingBackdrop()
{
    // Empty ward waiting passage, assembled from the campaign's surfaces and props.
    // Characters and enemy components are deliberately absent from this set.
    TrainingSet=NewObject<USceneComponent>(this,TEXT("TrainingWaitingPassage"));TrainingSet->SetupAttachment(RootComponent);TrainingSet->SetRelativeLocation(TrainingOffset);TrainingSet->RegisterComponent();AddInstanceComponent(TrainingSet);
    auto Mat=[](const TCHAR* Id){return LoadObject<UMaterialInterface>(nullptr,*MaterialPath(Id));};
    auto Variant=[&](FLinearColor Tint,float Rough,float Metal=0.f){auto* M=UMaterialInstanceDynamic::Create(Mat(TEXT("4ae6673fcc8b19e7")),this);M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetScalarParameterValue(TEXT("Roughness"),Rough);M->SetScalarParameterValue(TEXT("Metallic"),Metal);return M;};
    auto* Wall=Variant(FLinearColor(.50,.32,.29),.94f);auto* Lower=Variant(FLinearColor(.30,.068,.061),.64f);auto* Metal=Variant(FLinearColor(.028,.044,.045),.4f,.72f);auto* Door=Variant(FLinearColor(.15,.22,.20),.72f);
    auto* Floor=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/GameMenu/M_ArchiveFloor"));auto* Lamp=Variant(FLinearColor(.82,.43,.34),.3f);Lamp->SetVectorParameterValue(TEXT("Emission"),FLinearColor(44,21,15));auto* AlarmGlass=Variant(FLinearColor(.25,.001,.001),.3f);AlarmGlass->SetVectorParameterValue(TEXT("Emission"),FLinearColor(20,.02,.002));
    const FString Cube=TEXT("/Engine/BasicShapes/Cube");
    auto Piece=[&](FString Mesh,FVector At,FVector Scale,UMaterialInterface* M,FRotator R=FRotator::ZeroRotator){auto* P=Part(Mesh,At,Scale,M,R);P->AttachToComponent(TrainingSet,FAttachmentTransformRules::KeepRelativeTransform);return P;};
    auto Box=[&](FVector At,FVector Size,UMaterialInterface* M){return Piece(Cube,At,Size/100.f,M);};
    Box(FVector(800,0,-10),FVector(1640,850,20),Floor);Box(FVector(800,0,367),FVector(1640,850,20),Wall);Box(FVector(-30,0,180),FVector(20,850,360),Wall);
    for(int Side:{-1,1}){Box(FVector(800,Side*425,245),FVector(1640,20,245),Wall);Box(FVector(800,Side*418,58),FVector(1640,18,116),Lower);Box(FVector(800,Side*401,125),FVector(1640,8,7),Metal);Box(FVector(800,Side*402,12),FVector(1640,7,18),Metal);}
    // The end opens into a perpendicular passage, rather than the other menus' room.
    Box(FVector(1860,0,-10),FVector(500,1540,20),Floor);Box(FVector(2120,0,180),FVector(20,1540,360),Lower);Box(FVector(1860,0,367),FVector(500,1540,20),Wall);
    for(float Y:{-465.f,465.f})Box(FVector(1610,Y,180),FVector(26,490,360),Wall);Box(FVector(1610,0,321),FVector(26,470,90),Wall);
    for(float Y:{-230.f,230.f})Box(FVector(1590,Y,140),FVector(26,10,280),Metal);Box(FVector(1590,0,281),FVector(26,470,10),Metal);
    // Recessed clinic doors and the reception window break the long side walls.
    for(float X:{430.f,1030.f}){Box(FVector(X,400,144),FVector(174,15,284),Metal);Box(FVector(X,389,140),FVector(152,8,267),Door);Box(FVector(X,382,203),FVector(73,4,56),Metal);Box(FVector(X-48,378,126),FVector(6,8,22),Metal);}
    Box(FVector(760,-399,233),FVector(390,16,170),Metal);Box(FVector(760,-387,233),FVector(370,4,150),Lower);for(int I=0;I<12;++I)Box(FVector(760,-383,163+I*13),FVector(373,3,2),Metal);Box(FVector(760,-362,145),FVector(410,74,7),Door);
    for(int Side:{-1,1})for(float Y:{315.f,345.f})Piece(TEXT("/Engine/BasicShapes/Cylinder"),FVector(810,Side*Y,337),FVector(.07,.07,16.2),Metal,FRotator(90,0,0));
    auto Light=[&](FVector At,FRotator R,FLinearColor Color,float Power){auto* L=NewObject<URectLightComponent>(this);L->SetupAttachment(TrainingSet);L->SetRelativeLocation(At);L->SetRelativeRotation(R);L->SetIntensityUnits(ELightUnits::Lumens);L->SetLightColor(Color);L->SetIntensity(Power);L->SetAttenuationRadius(1000);L->SetSourceWidth(110);L->SetSourceHeight(16);L->SetCastShadows(true);L->RegisterComponent();AddInstanceComponent(L);TrainingLights.Add(L);};
    for(float X:{360.f,920.f,1460.f}){Box(FVector(X,0,352),FVector(27,144,10),Metal);Box(FVector(X,0,345),FVector(19,130,3),Lamp);Light(FVector(X,0,332),FRotator(-90,0,0),FLinearColor(1,.32,.22),X<500?650:950);}
    Light(FVector(1960,450,220),FRotator(-15,-90,0),FLinearColor(.99,.51,.36),1000);
    auto* Red=NewObject<UPointLightComponent>(this);Red->SetupAttachment(TrainingSet);Red->SetRelativeLocation(FVector(1420,-384,224));Red->SetLightColor(FLinearColor(1,.004,.002));Red->SetIntensityUnits(ELightUnits::Lumens);Red->SetIntensity(1250);Red->SetAttenuationRadius(1150);Red->SetSourceRadius(10);Red->RegisterComponent();AddInstanceComponent(Red);TrainingLights.Add(Red);Box(FVector(1420,-401,224),FVector(21,14,42),AlarmGlass);
    // The settings camera looks along the quiet cross passage, a different view of this set.
    Box(FVector(1860,770,180),FVector(520,20,360),Wall);Box(FVector(1860,-770,180),FVector(520,20,360),Wall);
    Box(FVector(1940,725,140),FVector(165,15,280),Metal);Box(FVector(1940,713,136),FVector(145,8,268),Door);
    Box(FVector(2085,600,230),FVector(14,21,42),AlarmGlass);Light(FVector(2040,600,220),FRotator(-10,-90,0),FLinearColor(1,.005,.002),500);
    Box(FVector(1860,-330,350),FVector(150,28,9),Metal);Box(FVector(1860,-330,344),FVector(135,19,3),Lamp);
    TrainingDoorHinge=NewObject<USceneComponent>(this);TrainingDoorHinge->SetupAttachment(TrainingSet);TrainingDoorHinge->SetRelativeLocation(FVector(1620,220,0));TrainingDoorHinge->RegisterComponent();AddInstanceComponent(TrainingDoorHinge);auto* Swing=Box(FVector(1620,112,134),FVector(12,216,268),Door);Swing->AttachToComponent(TrainingDoorHinge,FAttachmentTransformRules::KeepRelativeTransform);Swing->SetRelativeLocation(FVector(0,-108,134));
    auto Prop=[&](FString Find,FVector At,FRotator Rot,float Scale=1.f){Json A;for(const auto& V:Array(Mode->Data,TEXT("instances")))if(String(V->AsObject(),TEXT("path")).Contains(Find)){A=V->AsObject();break;}if(!A)return;const FString Path=String(A,TEXT("path")),Prefix=Path.Left(Path.Find(Find))+Find;const auto Base=Transform(Object(A,TEXT("transform")));for(const auto& V:Array(Mode->Data,TEXT("instances"))){const Json R=V->AsObject();if(!String(R,TEXT("path")).StartsWith(Prefix))continue;const auto T=Transform(Object(R,TEXT("transform")));const auto& Mats=Array(R,TEXT("materials"));Piece(MeshPath(String(R,TEXT("mesh"))),At+Rot.RotateVector((T.GetLocation()-Base.GetLocation())*Scale),T.GetScale3D()*Scale,Mats.Num()?Mat(*Mats[0]->AsString()):Metal,Rot+T.Rotator());}};
    Prop(TEXT("old_bed"),FVector(1130,282,0),FRotator(0,0,0),.9f);Prop(TEXT("wheelchair_01"),FVector(670,270,1),FRotator(0,-55,0));Prop(TEXT("steel_frame_shelves"),FVector(1440,310,0),FRotator(0,-90,0),.8f);Prop(TEXT("painted_wooden_cabinet_02_2k"),FVector(1220,-315,0),FRotator(0,90,0),.8f);Prop(TEXT("desk_lamp_arm_01_2k"),FVector(760,-350,150),FRotator(0,10,0),.8f);
    Prop(TEXT("wheelchair_01"),FVector(2010,430,1),FRotator(0,110,0));Prop(TEXT("painted_wooden_cabinet_02_2k"),FVector(2025,-130,0),FRotator(0,180,0),.8f);
    auto* Linen=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Menu/M_MenuLinen"));FRandomStream Paper(824);for(int I=0;I<18;++I){auto* P=Box(FVector(Paper.FRandRange(350,1800),Paper.FRandRange(-290,290),.7),FVector(21,29,.12),Linen);P->SetRelativeRotation(FRotator(0,Paper.FRandRange(-180,180),0));}
    auto Decal=[&](bool Blood,FVector At,FVector Size,FRotator R,int I,bool OnWall){auto* D=NewObject<UDecalComponent>(this);D->SetupAttachment(TrainingSet);auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,Blood?TEXT("/Game/Campaign/MenuEvents/M_MenuBlood"):TEXT("/Game/Campaign/Menu/M_MenuWear")),this);M->SetScalarParameterValue(TEXT("Seed"),37+I*8.3f);M->SetScalarParameterValue(TEXT("Wall"),OnWall?1:0);D->SetDecalMaterial(M);D->SetRelativeLocation(At);D->SetRelativeRotation(R);D->DecalSize=Size;D->RegisterComponent();AddInstanceComponent(D);if(Blood)++TrainingBloodDecals;};
    for(int Side:{-1,1})for(int I=0;I<4;++I){Decal(false,FVector(250+I*380,Side*401,158),FVector(12,125,160),FRotator(0,-Side*90,0),I,true);Decal(true,FVector(300+I*370,Side*402,110),FVector(8,66,85),FRotator(0,-Side*90,0),I+Side*9,true);}
    for(int I=0;I<8;++I)Decal(true,FVector(300+I*180,FMath::Sin(I*2.3f)*235,1),FVector(6,70,98),FRotator(90,31+I*49,0),I+25,false);
    for(int I=0;I<4;++I){Decal(true,FVector(1900+FMath::Sin(float(I))*100,-400+I*280,1),FVector(6,60,95),FRotator(90,21+I*43,0),90+I,false);Decal(true,FVector(2104,-380+I*290,120),FVector(8,75,95),FRotator(0,180,0),100+I,true);}
    for(int I=0;I<4;++I){auto* F=NewObject<ULocalFogVolumeComponent>(this);F->SetupAttachment(TrainingSet);F->RadialFogExtinction=.025f;F->HeightFogExtinction=1.35f;F->HeightFogFalloff=11;F->HeightFogOffset=-.08f;F->FogAlbedo=FLinearColor(.51,.21,.16);F->SetRelativeLocation(FVector(430+I*420,0,-100));F->SetRelativeScale3D(FVector(1.2));F->RegisterComponent();AddInstanceComponent(F);}
    TrainingCamera=GetWorld()->SpawnActor<ACameraActor>();auto* C=TrainingCamera->GetCameraComponent();C->SetConstraintAspectRatio(false);C->SetFieldOfView(74);C->PostProcessSettings=Camera->GetCameraComponent()->PostProcessSettings;C->PostProcessSettings.AutoExposureMinBrightness=4.1f;C->PostProcessSettings.AutoExposureMaxBrightness=4.1f;C->PostProcessSettings.DepthOfFieldFstop=2.8f;C->PostProcessSettings.DepthOfFieldFocalDistance=1100;TrainingSet->SetVisibility(false,true);
}
void AEWardTitleScene::TickTrainingBackdrop(float Delta)
{
    TrainingAge+=Delta;const float T=TrainingAge;if(Mode->Screen==TEXT("settings"))TrainingCamera->SetActorLocationAndRotation(Origin+TrainingOffset+FVector(1930+FMath::Sin(T*.11f)*1.5f,-635+25*FMath::Sin(T*.015f),144+FMath::Sin(T*.17f)*.7f),FRotator(-1.5,90,0));else TrainingCamera->SetActorLocationAndRotation(Origin+TrainingOffset+FVector(80+28*FMath::Sin(T*.015f),-118+FMath::Sin(T*.11f)*1.5f,145+FMath::Sin(T*.17f)*.7f),FRotator(-1.5,5,0));TrainingDoorHinge->SetRelativeRotation(FRotator(0,-57+FMath::Sin(T*.45f)*1.8f,0));
    for(int I=0;I<TrainingLights.Num();++I){const float Base=I==0?650:I<3?950:I==3?1000:I==4?1250:500;float B=1;if(I==1&&FMath::Fmod(T,13.7f)>10.8f)B=FMath::Sin(T*41)>0?.65f:.025f;if(I==4||I==5)B=.18f+.82f*FMath::Pow(FMath::Max(0.f,FMath::Sin(T*1.4f)),5);TrainingLights[I]->SetIntensity(Base*B*(1-Fade));}
    TInlineComponentArray<ULocalFogVolumeComponent*> Fog;GetComponents(Fog);int I=0;for(auto* F:Fog)if(F->GetAttachParent()==TrainingSet){F->SetRelativeLocation(FVector(430+I*420+FMath::Sin(T*.11f+I)*30,FMath::Sin(T*.13f+I)*19,-100));++I;}
}
