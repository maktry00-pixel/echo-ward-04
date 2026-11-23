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
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
using namespace WardData;
namespace {const FVector ConsultationOffset(0,-16000,0);}
void AEWardTitleScene::SetAboutBackdrop(bool Enabled)
{
    if(AboutBackdropActive==Enabled)return;if(Enabled&&!AboutSet)BuildAboutBackdrop();AboutBackdropActive=Enabled;
    if(Active&&Enabled){AboutAge=0;TickAboutBackdrop(0);}RefreshBackdrop();
}
void AEWardTitleScene::BuildAboutBackdrop()
{
    // A presentation copy of the campaign's consultation-room furniture.
    // No characters, skeletal meshes or enemy actors belong to this set.
    AboutSet=NewObject<USceneComponent>(this,TEXT("AboutConsultationRoom"));AboutSet->SetupAttachment(RootComponent);AboutSet->SetRelativeLocation(ConsultationOffset);AboutSet->RegisterComponent();AddInstanceComponent(AboutSet);
    auto Mat=[](const TCHAR* Id){return LoadObject<UMaterialInterface>(nullptr,*MaterialPath(Id));};
    auto Variant=[&](UMaterialInterface* Base,FLinearColor Tint,float Rough){auto* M=UMaterialInstanceDynamic::Create(Base,this);M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetScalarParameterValue(TEXT("Roughness"),Rough);return M;};
    auto* Plaster=Mat(TEXT("4ae6673fcc8b19e7"));auto* Wall=Variant(Plaster,FLinearColor(.45,.29,.26),.94);
    auto* Tile=Variant(Mat(TEXT("05227396fb5ca2d8")),FLinearColor(.33,.068,.061),.6);
    auto* Floor=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/GameMenu/M_ArchiveFloor"));
    auto* Steel=Variant(Plaster,FLinearColor(.045,.065,.062),.42);Steel->SetScalarParameterValue(TEXT("Metallic"),.72);
    auto* Door=Variant(Plaster,FLinearColor(.18,.25,.23),.72);
    auto* Lamp=Variant(Plaster,FLinearColor(.82,.43,.34),.3);Lamp->SetVectorParameterValue(TEXT("Emission"),FLinearColor(42,20,14));
    auto* AlarmGlass=Variant(Plaster,FLinearColor(.3,.002,.001),.25);AlarmGlass->SetVectorParameterValue(TEXT("Emission"),FLinearColor(18,.02,.004));
    const FString Cube=TEXT("/Engine/BasicShapes/Cube"),Cylinder=TEXT("/Engine/BasicShapes/Cylinder");
    auto Piece=[&](FString Mesh,FVector At,FVector Scale,UMaterialInterface* M,FRotator R=FRotator::ZeroRotator){auto* P=Part(Mesh,At,Scale,M,R);P->AttachToComponent(AboutSet,FAttachmentTransformRules::KeepRelativeTransform);return P;};
    auto Box=[&](FVector At,FVector Size,UMaterialInterface* M){return Piece(Cube,At,Size/100.f,M);};
    Box(FVector(760,0,-10),FVector(1560,930,20),Floor);Box(FVector(760,0,378),FVector(1560,950,20),Tile);
    Box(FVector(-30,0,190),FVector(20,950,380),Wall);
    for(int Side:{-1,1})
    {
        Box(FVector(760,Side*470,246),FVector(1560,24,268),Wall);Box(FVector(760,Side*455,58),FVector(1560,22,116),Tile);
        Box(FVector(760,Side*438,124),FVector(1560,8,7),Steel);Box(FVector(760,Side*439,15),FVector(1560,6,22),Steel);
        for(float Y:{320.f,350.f})Piece(Cylinder,FVector(760,Side*Y,347),FVector(.075,.075,15.6),Steel,FRotator(90,0,0));
        for(int I=0;I<7;++I)Box(FVector(100+I*220,Side*335,355),FVector(5,70,5),Steel);
    }
    // An open door with a real, empty examination alcove behind it.
    for(float Y:{-295.f,295.f})Box(FVector(1530,Y,190),FVector(20,350,380),Wall);
    Box(FVector(1530,0,329),FVector(20,240,102),Wall);
    for(float Y:{-124.f,124.f})Box(FVector(1514,Y,140),FVector(20,8,280),Steel);Box(FVector(1514,0,281),FVector(20,255,8),Steel);
    Box(FVector(1750,0,-9),FVector(440,750,18),Floor);Box(FVector(1950,0,190),FVector(20,760,380),Tile);
    for(float Y:{-370.f,370.f})Box(FVector(1750,Y,190),FVector(440,20,380),Wall);Box(FVector(1750,0,378),FVector(440,750,20),Tile);
    auto* Ajar=Box(FVector(1570,80,136),FVector(12,220,268),Door);Ajar->SetRelativeRotation(FRotator(0,-54,0));
    // Recessed frosted service windows and wall panels provide depth beside the UI.
    for(float X:{470.f,960.f})
    {
        Box(FVector(X,-437,234),FVector(285,14,165),Steel);Box(FVector(X,-426,234),FVector(268,4,148),Tile);
        for(float E:{-141.f,0.f,141.f})Box(FVector(X+E,-421,234),FVector(5,8,162),Steel);
        for(int I=0;I<11;++I)Box(FVector(X,-418,162+I*14),FVector(276,4,2),Steel);
    }
    auto Light=[&](FVector At,FRotator Rot,FLinearColor Color,float Power,float Radius){auto* L=NewObject<URectLightComponent>(this);L->SetupAttachment(AboutSet);L->SetRelativeLocation(At);L->SetRelativeRotation(Rot);L->SetIntensityUnits(ELightUnits::Lumens);L->SetLightColor(Color);L->SetIntensity(Power);L->SetAttenuationRadius(Radius);L->SetSourceWidth(105);L->SetSourceHeight(17);L->SetCastShadows(true);L->RegisterComponent();AddInstanceComponent(L);AboutLights.Add(L);return L;};
    for(float X:{310.f,1110.f}){Box(FVector(X,20,363),FVector(30,144,10),Steel);Box(FVector(X,20,356),FVector(20,126,3),Lamp);Light(FVector(X,20,345),FRotator(-90,0,0),FLinearColor(1,.31,.21),X<500?660:1050,1100);}
    AboutLampHinge=NewObject<USceneComponent>(this);AboutLampHinge->SetupAttachment(AboutSet);AboutLampHinge->SetRelativeLocation(FVector(720,20,365));AboutLampHinge->RegisterComponent();AddInstanceComponent(AboutLampHinge);
    auto Hanging=[&](FVector At,FVector Size,UMaterialInterface* M){auto* P=Box(FVector(720,20,365)+At,Size,M);P->AttachToComponent(AboutLampHinge,FAttachmentTransformRules::KeepRelativeTransform);P->SetRelativeLocation(At);};
    for(float Y:{-44.f,44.f})Hanging(FVector(0,Y,-20),FVector(1.2,1.2,40),Steel);Hanging(FVector(0,0,-43),FVector(28,140,8),Steel);Hanging(FVector(0,0,-48),FVector(19,125,2),Lamp);
    auto* SwingLight=Light(FVector(720,20,308),FRotator(-90,0,0),FLinearColor(1,.51,.35),410,900);SwingLight->AttachToComponent(AboutLampHinge,FAttachmentTransformRules::KeepRelativeTransform);SwingLight->SetRelativeLocation(FVector(0,0,-57));
    Light(FVector(1780,60,245),FRotator(-15,180,0),FLinearColor(1,.005,.002),1300,1250);
    auto* Red=NewObject<UPointLightComponent>(this);Red->SetupAttachment(AboutSet);Red->SetRelativeLocation(FVector(1290,412,240));Red->SetLightColor(FLinearColor(1,.004,.002));Red->SetIntensityUnits(ELightUnits::Lumens);Red->SetIntensity(650);Red->SetAttenuationRadius(950);Red->SetSourceRadius(11);Red->RegisterComponent();AddInstanceComponent(Red);AboutLights.Add(Red);Box(FVector(1290,433,240),FVector(18,14,42),AlarmGlass);
    auto Prop=[&](FString Find,FVector At,FRotator Rot,float Scale=1.f){Json A;for(const auto& V:Array(Mode->Data,TEXT("instances")))if(String(V->AsObject(),TEXT("path")).Contains(Find)){A=V->AsObject();break;}if(!A)return;const FString Path=String(A,TEXT("path")),Prefix=Path.Left(Path.Find(Find))+Find;const auto Base=Transform(Object(A,TEXT("transform")));for(const auto& V:Array(Mode->Data,TEXT("instances"))){const Json R=V->AsObject();if(!String(R,TEXT("path")).StartsWith(Prefix))continue;const auto T=Transform(Object(R,TEXT("transform")));const auto& Mats=Array(R,TEXT("materials"));Piece(MeshPath(String(R,TEXT("mesh"))),At+Rot.RotateVector((T.GetLocation()-Base.GetLocation())*Scale),T.GetScale3D()*Scale,Mats.Num()?Mat(*Mats[0]->AsString()):Steel,Rot+T.Rotator());}};
    Prop(TEXT("old_bed"),FVector(1260,250,0),FRotator(0,90,0),.92);
    Prop(TEXT("wheelchair_01"),FVector(1360,-300,1),FRotator(0,40,0),1);
    Prop(TEXT("steel_frame_shelves"),FVector(650,360,0),FRotator(0,-90,0),.85);
    Prop(TEXT("painted_wooden_cabinet_02_2k"),FVector(1070,-345,0),FRotator(0,-90,0),.8);
    Box(FVector(1140,345,79),FVector(150,65,7),Steel);for(float X:{1080.f,1200.f})for(float Y:{320.f,370.f})Box(FVector(X,Y,39),FVector(4,4,78),Steel);
    Prop(TEXT("desk_lamp_arm_01_2k"),FVector(1160,330,84),FRotator(0,-30,0),.8);Prop(TEXT("medical_box_2k"),FVector(1110,345,84),FRotator(0,5,0),.6);
    Box(FVector(800,-260,81),FVector(210,95,9),Door);for(float X:{715.f,885.f})for(float Y:{-295.f,-225.f})Box(FVector(X,Y,39),FVector(5,5,78),Steel);
    Prop(TEXT("desk_lamp_arm_01_2k"),FVector(840,-270,87),FRotator(0,70,0),1.f);
    for(int I=0;I<3;++I){Box(FVector(1010+I*82,300,112),FVector(3,95,215),Steel);Box(FVector(1010+I*82,300,128),FVector(2,90,169),Tile);}
    auto* Linen=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Menu/M_MenuLinen"));FRandomStream Paper(603);
    for(int I=0;I<14;++I){auto* P=Box(FVector(Paper.FRandRange(300,1450),Paper.FRandRange(-300,280),.6),FVector(21,29,.12),Linen);P->SetRelativeRotation(FRotator(0,Paper.FRandRange(-180,180),0));}
    auto Decal=[&](bool Blood,FVector At,FVector Size,FRotator Rot,int I,bool OnWall){auto* D=NewObject<UDecalComponent>(this);D->SetupAttachment(AboutSet);auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,Blood?TEXT("/Game/Campaign/MenuEvents/M_MenuBlood"):TEXT("/Game/Campaign/Menu/M_MenuWear")),this);M->SetScalarParameterValue(TEXT("Seed"),17+I*7.3f);M->SetScalarParameterValue(TEXT("Wall"),OnWall?1:0);D->SetDecalMaterial(M);D->SetRelativeLocation(At);D->SetRelativeRotation(Rot);D->DecalSize=Size;D->RegisterComponent();AddInstanceComponent(D);if(Blood)++AboutBloodDecals;};
    for(int Side:{-1,1})for(int I=0;I<6;++I){Decal(false,FVector(180+I*240,Side*437,155),FVector(10,105,160),FRotator(0,-Side*90,0),I,true);if(I%2==0)Decal(true,FVector(230+I*240,Side*438,110),FVector(8,64,85),FRotator(0,-Side*90,0),I+Side*9,true);}
    for(int I=0;I<7;++I)Decal(true,FVector(340+I*175,FMath::Sin(I*2.1f)*250,1),FVector(6,70,105),FRotator(90,21+I*47,0),I+25,false);
    for(int I=0;I<3;++I){auto* Fog=NewObject<ULocalFogVolumeComponent>(this);Fog->SetupAttachment(AboutSet);Fog->RadialFogExtinction=.025;Fog->HeightFogExtinction=1.4;Fog->HeightFogFalloff=11;Fog->HeightFogOffset=-.08;Fog->FogAlbedo=FLinearColor(.51,.21,.16);Fog->SetRelativeLocation(FVector(460+I*480,0,-100));Fog->SetRelativeScale3D(FVector(1.3));Fog->RegisterComponent();AddInstanceComponent(Fog);}
    AboutCamera=GetWorld()->SpawnActor<ACameraActor>();auto* C=AboutCamera->GetCameraComponent();C->SetConstraintAspectRatio(false);C->SetFieldOfView(76);C->PostProcessSettings=Camera->GetCameraComponent()->PostProcessSettings;C->PostProcessSettings.AutoExposureMinBrightness=4.1;C->PostProcessSettings.AutoExposureMaxBrightness=4.1;C->PostProcessSettings.DepthOfFieldFstop=2.8;C->PostProcessSettings.DepthOfFieldFocalDistance=1100;C->PostProcessSettings.BloomIntensity=.35;
    AboutSet->SetVisibility(false,true);
}
void AEWardTitleScene::TickAboutBackdrop(float Delta)
{
    AboutAge+=Delta;const float T=AboutAge;const bool Pause=Mode&&(Mode->Screen==TEXT("pause")||(!Mode->TitleView&&Mode->Screen==TEXT("game")));
    const FVector At=Origin+ConsultationOffset+FVector((Pause?370:90)+26*FMath::Sin(T*.018),(Pause?150:295)+FMath::Sin(T*.11)*1.7,144+FMath::Sin(T*.17)*.7);AboutCamera->SetActorLocationAndRotation(At,FRotator(-1.8,Pause?-5:-13,0));
    AboutLampHinge->SetRelativeRotation(FRotator(FMath::Sin(T*.63)*.7,0,FMath::Sin(T*.51)*.9));
    for(int I=0;I<AboutLights.Num();++I){const float Base=I==0?660:I==1?1050:I==2?410:I==3?1300:650;float B=1;if(I==2&&FMath::Fmod(T,14.3f)>11.4f)B=FMath::Sin(T*43)>0?.7f:.035f;if(I==4)B=.23f+.77f*FMath::Pow(FMath::Max(0.f,FMath::Sin(T*1.5f)),6);AboutLights[I]->SetIntensity(Base*B*(1-Fade));}
    TInlineComponentArray<ULocalFogVolumeComponent*> Fog;GetComponents(Fog);int I=0;for(auto* F:Fog)if(F->GetAttachParent()==AboutSet){F->SetRelativeLocation(FVector(460+I*480+FMath::Sin(T*.1f+I)*28,FMath::Sin(T*.13f+I)*18,-100));++I;}
}
