#include "WardTitleScene.h"
#include "WardCampaign.h"
#include "WardAudio68.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Sound/SoundBase.h"
#include "Engine/StaticMesh.h"
#include "WardHospitalMaterials.h"
using namespace WardData;
AEWardTitleScene::AEWardTitleScene()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bTickEvenWhenPaused=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("TitleSet"));
}
UStaticMeshComponent* AEWardTitleScene::Part(const FString& Mesh,FVector At,FVector Scale,UMaterialInterface* Material,FRotator Rotation)
{
    auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Mesh));
    if(Material)C->SetMaterial(0,Material);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetRelativeLocation(At);C->SetRelativeScale3D(Scale);C->SetRelativeRotation(Rotation);C->RegisterComponent();AddInstanceComponent(C);return C;
}
void AEWardTitleScene::Build(AEWardCampaignMode* InMode)
{
    Mode=InMode;SetActorLocation(Origin);
    for(const TCHAR* Id:{TEXT("menu_vent"),TEXT("menu_drip"),TEXT("menu_click"),TEXT("menu_hover"),TEXT("menu_electric")})if(auto* S=LoadObject<USoundBase>(nullptr,*(FString(TEXT("/Game/Campaign/Audio/A_"))+Id)))Mode->Effects.Add(FName(Id),S);
    for(const TCHAR* Id:{TEXT("menu_door"),TEXT("menu_scare"),TEXT("menu_footstep")})if(auto* S=LoadObject<USoundBase>(nullptr,*(FString(TEXT("/Game/Campaign/MenuNurse/A_"))+Id)))Mode->Effects.Add(FName(Id),S);
    auto Mat=[](const TCHAR* Id){return LoadObject<UMaterialInterface>(nullptr,*MaterialPath(Id));};
    auto* Floor=WardHospitalMaterials::Title(this,TEXT("527b19fd5563fa6d"),FLinearColor(.33,.21,.19),.41f);
    Floor->SetScalarParameterValue(TEXT("Wetness"),.73);Floor->SetVectorParameterValue(TEXT("Tiling"),FLinearColor(1,2.9,0,0));
    auto* Plaster=WardHospitalMaterials::Title(this,TEXT("4ae6673fcc8b19e7"),FLinearColor(.62,.43,.37),.9f);
    auto* Lower=WardHospitalMaterials::Title(this,TEXT("05227396fb5ca2d8"),FLinearColor(.38,.075,.065),.8f);
    auto* Metal=WardHospitalMaterials::Variant(this,Plaster);Metal->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.11,.06,.052));Metal->SetScalarParameterValue(TEXT("Metallic"),.7);Metal->SetScalarParameterValue(TEXT("Roughness"),.48);
    auto* Door=WardHospitalMaterials::Variant(this,Plaster);Door->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.27,.085,.069));
    auto* White=WardHospitalMaterials::Variant(this,Plaster);White->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.85,.45,.36));White->SetVectorParameterValue(TEXT("Emission"),FLinearColor(68,30,21));
    AlarmMaterial=WardHospitalMaterials::Variant(this,Plaster);AlarmMaterial->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.4,.005,.003));
    const FString Cube=TEXT("/Engine/BasicShapes/Cube.Cube"),Cylinder=TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
    auto Box=[&](FVector At,FVector Size,UMaterialInterface* M){return Part(Cube,At,Size/100.f,M);};
    // Continuous architecture, with inset ward doors rather than surfaces crossing openings.
    for(int I=0;I<14;++I){Box(FVector(I*200+100,0,-10),FVector(200,580,20),Floor);Box(FVector(I*200+100,0,350),FVector(200,580,20),Lower);}
    for(int Side:{-1,1})
    {
        float Last=0;
        for(float X:{650.f,1300.f,1950.f,2550.f})
        {
            const float Start=X-75,Width=Start-Last;
            Box(FVector(Last+Width/2,Side*290,210),FVector(Width,24,260),Plaster);
            Box(FVector(Last+Width/2,Side*277,54),FVector(Width,18,108),Lower);
            Box(FVector(Last+Width/2,Side*264,105),FVector(Width,7,8),Metal);
            Box(FVector(X,Side*290,300),FVector(150,24,80),Plaster);
            const bool Ajar=Side==1&&X==1300;const FVector Hinge(X-72,Side*296,0);const FRotator Swing(0,Ajar?-44:0,0);
            if(Ajar){SideHinge=NewObject<USceneComponent>(this);SideHinge->SetupAttachment(RootComponent);SideHinge->SetRelativeLocation(Hinge);SideHinge->SetRelativeRotation(Swing);SideHinge->RegisterComponent();AddInstanceComponent(SideHinge);}
            auto DoorPart=[&](FVector At,FVector Size,UMaterialInterface* M){auto* P=Box(Hinge+Swing.RotateVector(At-Hinge),Size,M);P->SetRelativeRotation(Swing);if(Ajar){P->AttachToComponent(SideHinge,FAttachmentTransformRules::KeepRelativeTransform);P->SetRelativeLocation(At-Hinge);P->SetRelativeRotation(FRotator::ZeroRotator);}return P;};
            auto* Leaf=DoorPart(FVector(X,Side*296,129),FVector(144,10,254),Door);if(Ajar)SideDoor=Leaf;
            DoorPart(FVector(X,Side*284,220),FVector(66,3,34),Metal);
            for(float Edge:{-78.f,78.f})Box(FVector(X+Edge,Side*279,130),FVector(8,17,260),Metal);
            Box(FVector(X,Side*279,262),FVector(164,17,8),Metal);
            DoorPart(FVector(X-46,Side*285,107),FVector(5,12,20),Metal);
            Last=X+75;
        }
        Box(FVector((Last+2800)/2,Side*290,175),FVector(2800-Last,24,350),Plaster);
        for(float Z:{315.f,333.f})Part(Cylinder,FVector(1400,Side*200,Z),FVector(.10,.10,28),Metal,FRotator(90,0,0));
        for(int I=0;I<12;++I)Box(FVector(I*235+50,Side*210,326),FVector(4,52,4),Metal);
    }
    Box(FVector(-20,0,175),FVector(20,600,350),Plaster);
    // A real opening and a short dark vestibule beyond it; the camera never crosses a wall.
    for(float Y:{-220.f,220.f})Box(FVector(2800,Y,175),FVector(20,160,350),Plaster);
    Box(FVector(2800,0,313),FVector(20,280,76),Plaster);
    Box(FVector(3100,0,-10),FVector(600,580,20),Floor);Box(FVector(3100,0,350),FVector(600,580,20),Lower);Box(FVector(3400,0,175),FVector(20,600,350),Lower);
    for(float Y:{-290.f,290.f})Box(FVector(3100,Y,175),FVector(600,20,350),Plaster);
    auto* Beyond=NewObject<UPointLightComponent>(this);Beyond->SetupAttachment(RootComponent);Beyond->SetIntensityUnits(ELightUnits::Lumens);Beyond->SetRelativeLocation(FVector(3150,0,280));Beyond->SetLightColor(FLinearColor(.74,.24,.16));Beyond->SetIntensity(200);Beyond->SetAttenuationRadius(650);Beyond->SetSourceRadius(12);Beyond->RegisterComponent();AddInstanceComponent(Beyond);LightBase.Add(Beyond,200);
    for(int Side:{-1,1})
    {
        const FVector Pivot(2780,Side*130,0);auto* H=NewObject<USceneComponent>(this);H->SetupAttachment(RootComponent);H->SetRelativeLocation(Pivot);H->RegisterComponent();AddInstanceComponent(H);FarHinges.Add(H);
        auto MovingPart=[&](FVector At,FVector Size,UMaterialInterface* M){auto* P=Box(At,Size,M);P->AttachToComponent(H,FAttachmentTransformRules::KeepRelativeTransform);P->SetRelativeLocation(At-Pivot);};
        MovingPart(FVector(2780,Side*65,132),FVector(12,128,260),Door);MovingPart(FVector(2769,Side*67,208),FVector(3,56,40),Metal);MovingPart(FVector(2768,Side*13,112),FVector(6,6,28),Metal);
    }
    for(float Y:{-137.f,137.f})Box(FVector(2770,Y,135),FVector(25,8,270),Metal);
    Box(FVector(2770,0,270),FVector(25,280,9),Metal);
    for(float X:{380.f,1100.f,1820.f,2480.f})
    {
        Box(FVector(X,0,337),FVector(30,150,9),Metal);Box(FVector(X,0,331),FVector(21,126,3),White);
        auto* L=NewObject<URectLightComponent>(this);L->SetupAttachment(RootComponent);L->SetIntensityUnits(ELightUnits::Lumens);L->SetRelativeLocation(FVector(X,0,324));L->SetRelativeRotation(FRotator(-90,0,0));L->SetIntensity(X==380?1250:X==1100?680:X==1820?120:600);L->SetLightColor(X==380?FLinearColor(1,.57,.39):FLinearColor(1,.18,.10));L->SetSourceWidth(110);L->SetSourceHeight(15);L->SetAttenuationRadius(850);L->SetCastShadows(true);L->RegisterComponent();AddInstanceComponent(L);LightBase.Add(L,L->Intensity);if(X==1100)BrokenLamp=L;
    }
    Box(FVector(2690,0,300),FVector(16,55,14),AlarmMaterial);
    Alarm=NewObject<UPointLightComponent>(this);Alarm->SetupAttachment(RootComponent);Alarm->SetIntensityUnits(ELightUnits::Lumens);Alarm->SetRelativeLocation(FVector(2600,0,280));Alarm->SetLightColor(FLinearColor(1,.008,.003));Alarm->SetIntensity(900);Alarm->SetAttenuationRadius(1000);Alarm->SetSourceRadius(15);Alarm->RegisterComponent();AddInstanceComponent(Alarm);
    auto* Rim=NewObject<UPointLightComponent>(this);Rim->SetupAttachment(RootComponent);Rim->SetIntensityUnits(ELightUnits::Lumens);Rim->SetRelativeLocation(FVector(950,220,125));Rim->SetLightColor(FLinearColor(.9,.06,.015));Rim->SetIntensity(110);Rim->SetAttenuationRadius(500);Rim->SetSourceRadius(25);Rim->RegisterComponent();AddInstanceComponent(Rim);LightBase.Add(Rim,Rim->Intensity);
    // Scanned campaign props retain their original PBR textures and centimetre scale.
    for(const auto& V:Array(Mode->Data,TEXT("instances")))
    {
        const Json Row=V->AsObject();const FString Path=String(Row,TEXT("path"));
        if(Path.Contains(TEXT("wheelchair_01")))
        {
            const auto& M=Array(Row,TEXT("materials"));Wheelchair=Part(MeshPath(String(Row,TEXT("mesh"))),FVector(1470,100,2),FVector(.95),Mat(*M[0]->AsString()),FRotator(0,-50,0));break;
        }
    }
    auto CopyProp=[&](const FString& Find,FVector At,FRotator Rot,float Scale=1.f)
    {
        Json Anchor;
        for(const auto& V:Array(Mode->Data,TEXT("instances")))if(String(V->AsObject(),TEXT("path" )).Contains(Find)){Anchor=V->AsObject();break;}
        if(!Anchor)return;const FString Prefix=String(Anchor,TEXT("path")).Left(String(Anchor,TEXT("path")).Find(Find))+Find;
        const FTransform Base=Transform(Object(Anchor,TEXT("transform")));
        for(const auto& V:Array(Mode->Data,TEXT("instances")))
        {
            const Json Row=V->AsObject();if(!String(Row,TEXT("path")).StartsWith(Prefix))continue;
            const FTransform T=Transform(Object(Row,TEXT("transform")));const auto& M=Array(Row,TEXT("materials"));
            Part(MeshPath(String(Row,TEXT("mesh"))),At+Rot.RotateVector((T.GetLocation()-Base.GetLocation())*Scale),T.GetScale3D()*Scale,M.Num()?Mat(*M[0]->AsString()):Metal,Rot+T.Rotator());
        }
    };
    auto* Linen=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Menu/M_MenuLinen"));
    auto* Rubber=WardHospitalMaterials::Variant(this,Metal);Rubber->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.012,.014,.013));Rubber->SetScalarParameterValue(TEXT("Metallic"),0);Rubber->SetScalarParameterValue(TEXT("Roughness"),.88);
    // A wheeled patient trolley with padded mattress, guard rails and caster forks.
    const FVector Gurney(760,197,0);const FRotator GRot(0,-7,0);
    auto GBox=[&](FVector At,FVector Size,UMaterialInterface* M){auto* P=Box(Gurney+GRot.RotateVector(At),Size,M);P->SetRelativeRotation(GRot);return P;};
    auto GCyl=[&](FVector At,FVector Size,UMaterialInterface* M,FRotator R){return Part(Cylinder,Gurney+GRot.RotateVector(At),Size/100.f,M,GRot+R);};
    GBox(FVector(0,0,72),FVector(190,70,6),Metal);GBox(FVector(0,0,81),FVector(182,66,13),Linen);GBox(FVector(62,0,91),FVector(37,61,10),Linen);
    for(float X:{-73.f,73.f})for(float Y:{-26.f,26.f}){GCyl(FVector(X,Y,42),FVector(3,3,60),Metal,FRotator::ZeroRotator);GCyl(FVector(X,Y,10),FVector(14,14,5),Rubber,FRotator(0,0,90));GBox(FVector(X,Y,18),FVector(5,7,18),Metal);}
    for(float Y:{-36.f,36.f}){GCyl(FVector(0,Y,105),FVector(3,3,180),Metal,FRotator(90,0,0));for(float X:{-84.f,-30.f,30.f,84.f})GCyl(FVector(X,Y,92),FVector(2.2,2.2,26),Metal,FRotator::ZeroRotator);}
    GBox(FVector(0,0,28),FVector(150,52,3),Metal);
    // Stainless medical cart: raised tray lips, a lower shelf, wheels, original medical case.
    const FVector Cart(1980,205,0);for(float Z:{31.f,88.f}){Box(Cart+FVector(0,0,Z),FVector(69,48,3),Metal);for(float Y:{-24.f,24.f})Box(Cart+FVector(0,Y,Z+3),FVector(69,2,6),Metal);}
    for(float X:{-30.f,30.f})for(float Y:{-20.f,20.f}){Part(Cylinder,Cart+FVector(X,Y,48),FVector(.025,.025,.8),Metal);Part(Cylinder,Cart+FVector(X,Y,7),FVector(.1,.1,.035),Rubber,FRotator(0,0,90));}
    CopyProp(TEXT("medical_box_2k"),Cart+FVector(-17,0,92),FRotator(0,18,0),.65f);
    for(int I=0;I<3;++I){Part(Cylinder,Cart+FVector(12+I*9,12,98),FVector(.055,.055,.18),Door);Box(Cart+FVector(12+I*9,12,108),FVector(4,4,3),Metal);}
    // The ajar ward has real depth; the red spill and its shadows cross the doorway.
    Box(FVector(1300,525,-7),FVector(380,480,14),Floor);Box(FVector(1300,765,175),FVector(400,18,350),Lower);
    for(float X:{1105.f,1495.f})Box(FVector(X,525,175),FVector(18,480,350),Plaster);
    Box(FVector(1300,525,350),FVector(400,480,18),Lower);
    auto* Red=NewObject<URectLightComponent>(this);Red->SetupAttachment(RootComponent);Red->SetIntensityUnits(ELightUnits::Lumens);Red->SetRelativeLocation(FVector(1300,570,155));Red->SetRelativeRotation(FRotator(0,-90,0));Red->SetLightColor(FLinearColor(1,.006,.002));Red->SetIntensity(1850);Red->SetAttenuationRadius(1550);Red->SetSourceWidth(65);Red->SetSourceHeight(90);Red->SetBarnDoorAngle(58);Red->SetCastShadows(true);Red->SetVolumetricScatteringIntensity(1.8f);Red->RegisterComponent();AddInstanceComponent(Red);LightBase.Add(Red,1850);SideLight=Red;
    auto* Spill=NewObject<UPointLightComponent>(this);Spill->SetupAttachment(RootComponent);Spill->SetIntensityUnits(ELightUnits::Lumens);Spill->SetRelativeLocation(FVector(1325,238,95));Spill->SetLightColor(FLinearColor(1,.006,.002));Spill->SetIntensity(480);Spill->SetAttenuationRadius(680);Spill->SetSourceRadius(7);Spill->SetSourceLength(65);Spill->SetCastShadows(false);Spill->RegisterComponent();AddInstanceComponent(Spill);LightBase.Add(Spill,480);

    Box(FVector(1300,650,86),FVector(150,65,8),Metal);
    for(float X:{1240.f,1360.f})for(float Y:{625.f,675.f})Box(FVector(X,Y,43),FVector(5,5,86),Metal);
    CopyProp(TEXT("desk_lamp_arm_01_2k"),FVector(1345,650,92),FRotator(0,-35,0),.9f);
    Box(FVector(1255,645,101),FVector(32,24,18),Linen);Box(FVector(1255,645,110.2),FVector(17,4,.3),Door);Box(FVector(1255,645,110.4),FVector(4,17,.3),Door);
    for(int I=0;I<4;++I){auto* P=Box(FVector(1290+I*5,635+I*3,91+I*.2),FVector(22,29,.12),Linen);P->SetRelativeRotation(FRotator(0,I*13,0));}
    auto* DeskGlow=NewObject<UPointLightComponent>(this);DeskGlow->SetupAttachment(RootComponent);DeskGlow->SetIntensityUnits(ELightUnits::Lumens);DeskGlow->SetRelativeLocation(FVector(1335,635,135));DeskGlow->SetLightColor(FLinearColor(1,.48,.16));DeskGlow->SetIntensity(360);DeskGlow->SetAttenuationRadius(500);DeskGlow->SetSourceRadius(14);DeskGlow->RegisterComponent();AddInstanceComponent(DeskGlow);LightBase.Add(DeskGlow,360);
    // Irregular wall damp, drip marks and floor scuffs; no opaque oval puddle geometry.
    auto* Wear=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Menu/M_MenuWear"));
    for(int Side:{-1,1})for(int I=0;I<9;++I){auto* D=NewObject<UDecalComponent>(this);D->SetupAttachment(RootComponent);D->SetDecalMaterial(Wear);D->DecalSize=FVector(10,120+I%3*20,130);D->SetRelativeLocation(FVector(260+I*278,Side*275,130+(I%3)*25));D->SetRelativeRotation(FRotator(0,-Side*90,I%2*180));D->RegisterComponent();AddInstanceComponent(D);}
    for(int I=0;I<5;++I){auto* D=NewObject<UDecalComponent>(this);D->SetupAttachment(RootComponent);D->SetDecalMaterial(Wear);D->DecalSize=FVector(5,90,145);D->SetRelativeLocation(FVector(480+I*460,FMath::Sin(I*4.3)*150,2));D->SetRelativeRotation(FRotator(90,I*47,0));D->RegisterComponent();AddInstanceComponent(D);}
    Dust=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);Dust->SetupAttachment(RootComponent);Dust->bDisallowNanite=true;Dust->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane")));Dust->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Menu/M_MenuDust")));Dust->SetCollisionEnabled(ECollisionEnabled::NoCollision);Dust->SetCastShadow(false);Dust->RegisterComponent();AddInstanceComponent(Dust);
    FRandomStream Specks(404);for(int I=0;I<42;++I)Dust->AddInstance(FTransform(FRotator(90,0,0),FVector(Specks.FRandRange(350,2100),Specks.FRandRange(-200,200),Specks.FRandRange(60,275)),FVector(Specks.FRandRange(.005,.012))));
    CopyProp(TEXT("painted_wooden_cabinet_02_2k"),FVector(1980,-230,2),FRotator(0,90,0),.8f);
    FRandomStream Papers(27);for(int I=0;I<18;++I){auto* Paper=Box(FVector(Papers.FRandRange(650,2630),Papers.FRandRange(-190,180),1),FVector(17+I%3*2,25+I%4,.12),Linen);Paper->SetRelativeRotation(FRotator(0,Papers.FRandRange(-180,180),0));}
    for(int I=0;I<3;++I)
    {
        auto* F=NewObject<ULocalFogVolumeComponent>(this);F->SetupAttachment(RootComponent);F->RadialFogExtinction=.015;F->HeightFogExtinction=1.35;F->HeightFogFalloff=11;F->HeightFogOffset=-.05;F->FogAlbedo=FLinearColor(.53,.18,.13);F->FogEmissive=FLinearColor(.002,.0007,.0005);F->SetRelativeLocation(FVector(950+I*750,0,-80));F->SetRelativeScale3D(FVector(1.3));F->RegisterComponent();AddInstanceComponent(F);
    }
    Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(66);Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
    auto& PP=Camera->GetCameraComponent()->PostProcessSettings;
    PP.bOverride_AutoExposureMethod=true;PP.AutoExposureMethod=AEM_Histogram;PP.bOverride_AutoExposureBias=true;PP.AutoExposureBias=0;
    PP.bOverride_AutoExposureMinBrightness=true;PP.bOverride_AutoExposureMaxBrightness=true;PP.AutoExposureMinBrightness=4.4f;PP.AutoExposureMaxBrightness=4.4f;
    PP.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP.AutoExposureApplyPhysicalCameraExposure=false;
    PP.bOverride_AmbientCubemapIntensity=true;PP.AmbientCubemapIntensity=.075;
    PP.bOverride_VignetteIntensity=true;PP.VignetteIntensity=.31;
    PP.bOverride_DepthOfFieldFstop=true;PP.DepthOfFieldFstop=1.8f;PP.bOverride_DepthOfFieldFocalDistance=true;PP.DepthOfFieldFocalDistance=950;PP.bOverride_DepthOfFieldSensorWidth=true;PP.DepthOfFieldSensorWidth=36;
    PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=0;
    PP.bOverride_FilmGrainIntensity=true;PP.FilmGrainIntensity=.055;
    PP.bOverride_ColorSaturation=true;PP.ColorSaturation=FVector4(1.04,.90,.85,1);
    PP.bOverride_BloomIntensity=true;PP.BloomIntensity=.4;
    Camera->GetCameraComponent()->PostProcessBlendWeight=1;
    BuildNurse();BuildIdleEvents();Active=true;Activate(false);
}
void AEWardTitleScene::Activate(bool Enabled)
{
    if(Active==Enabled)return;Active=Enabled;SetActorTickEnabled(Enabled);SetActorHiddenInGame(!Enabled);RootComponent->SetVisibility(Enabled,true);Camera->SetActorHiddenInGame(!Enabled);
    if(GameSet)GameSet->SetVisibility(Enabled&&GameBackdropActive,true);
    if(SaveSet)SaveSet->SetVisibility(Enabled&&SaveBackdropActive,true);
    if(AboutSet)AboutSet->SetVisibility(Enabled&&AboutBackdropActive,true);
    if(TrainingSet)TrainingSet->SetVisibility(Enabled&&TrainingBackdropActive,true);
    if(Enabled)
    {
        FlashWasVisible=Mode->Hero()->Flashlight->IsVisible();Mode->Hero()->Flashlight->SetVisibility(false);Age=0;PreviousIdleCycle=0;Fade=0;TransitionAge=0;Transitioning=false;NextCue=4;CueIndex=0;PreviousRealTime=FPlatformTime::Seconds();
        PreviousCameraPaused=GetWorld()->bIsCameraMoveableWhenPaused;GetWorld()->bIsCameraMoveableWhenPaused=true;
        CameraStart=Origin+FVector(110,-50,154);Camera->SetActorLocationAndRotation(CameraStart,FRotator(-1,-7,0));
        UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(Camera);
        for(const TCHAR* Id:{TEXT("menu_vent"),TEXT("menu_electric")})
        {
            const FString Key=FString(Id)==TEXT("menu_vent")?TEXT("MENU/vent"):TEXT("MENU/electric");
            auto* A=WardAudio68::Play(this,Key,.001f,nullptr,true);
            if(!A)if(auto* Sound=Mode->Effects.Find(Id))A=UGameplayStatics::SpawnSound2D(this,Sound->Get(),0,1,0,nullptr,true,false);
            if(A){A->bIsUISound=true;A->SetVolumeMultiplier(0);Loops.Add(A);}
        }
    }
    else
    {
        for(auto A:Loops)if(A){A->Stop();A->DestroyComponent();}Loops.Reset();Transitioning=false;Fade=0;if(Mode&&Mode->Hero())Mode->Hero()->Flashlight->SetVisibility(FlashWasVisible);
        if(auto* PC=UGameplayStatics::GetPlayerController(this,0)){GetWorld()->bIsCameraMoveableWhenPaused=PreviousCameraPaused;if(PC->GetViewTarget()==Camera||PC->GetViewTarget()==GameCamera||PC->GetViewTarget()==SaveCamera||PC->GetViewTarget()==AboutCamera||PC->GetViewTarget()==TrainingCamera)PC->SetViewTarget(Mode->Hero());}
    }
}
void AEWardTitleScene::Sound(const FString& Id,float Volume,float Pan)
{
    static const auto AudioMap=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Audio68/manifest.json"));
    const FString Key=String(Object(AudioMap,TEXT("aliases")),Id,Id);
    if(Object(AudioMap,TEXT("groups"))->HasField(Key)){WardAudio68::Play(this,Key,Volume*Mode->Setting(TEXT("effects"),.8));return;}
    if(auto* S=Mode->Effects.Find(FName(*Id)))UGameplayStatics::PlaySound2D(this,S->Get(),Volume*Mode->Setting(TEXT("effects"),.8),1,0,nullptr,nullptr,true);
}
void AEWardTitleScene::Click(){Sound(TEXT("menu_click"),.28f);}
void AEWardTitleScene::Hover(){if(Active&&!Transitioning)Sound(TEXT("menu_hover"),.10f);}
void AEWardTitleScene::BeginTransition(const FString& Action){if(Transitioning)return;Pending=Action;CameraStart=Camera->GetActorLocation();Transitioning=true;TransitionAge=0;ScareFired=false;DoorSoundFired=false;HideIdleEvents();IdleDoorOpen=0;IdleEntranceOpen=0;FarDoorOpen=0;if(SideHinge)SideHinge->SetRelativeRotation(FRotator::ZeroRotator);for(auto H:FarHinges)H->SetRelativeRotation(FRotator::ZeroRotator);}
void AEWardTitleScene::Tick(float Delta)
{
    Super::Tick(Delta);if(!Active||!Mode)return;
    // Keep presentation time on the wall clock. Clamping the timeline itself made
    // five-second story beats last longer during a screenshot or rendering hitch.
    const double Now=FPlatformTime::Seconds();const float RealDelta=FMath::Max(0.f,float(Now-PreviousRealTime));const float D=FMath::Clamp(RealDelta,0.f,.1f);PreviousRealTime=Now;Age+=RealDelta;
    if(GameBackdropActive||SaveBackdropActive||AboutBackdropActive||TrainingBackdropActive)
    {
        if(TrainingBackdropActive)TickTrainingBackdrop(D);else if(AboutBackdropActive)TickAboutBackdrop(D);else if(SaveBackdropActive)TickSaveBackdrop(D);else TickGameBackdrop(D);
        for(int32 I=0;I<Loops.Num();++I)if(Loops[I])Loops[I]->SetVolumeMultiplier((I==0?.14f:.04f)*Mode->Setting(TEXT("music"),.7)*(1-Fade));
        NextCue-=D;if(NextCue<0&&!Transitioning){Sound(CueIndex%2==0?TEXT("menu_drip"):TEXT("knock"),.065f);++CueIndex;NextCue=13+CueIndex%3*5;}
        if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->PlayerCameraManager->UpdateCamera(D);
        if(Transitioning){TransitionAge+=D;Fade=FMath::Clamp((TransitionAge-1.6f)/1.2f,0.f,1.f);if(TransitionAge>=3.2f){const FString Action=Pending;Transitioning=false;Mode->TitleActionExecuting=true;Mode->Command(Action);Mode->TitleActionExecuting=false;Fade=0;}}
        return;
    }
    const bool Entrance=Transitioning&&Pending==TEXT("page_game");
    const float Travel=FMath::Clamp(float(Origin.X+2440-CameraStart.X),0.f,1750.f);
    const float Advance=Transitioning?FMath::InterpEaseInOut(0.f,Travel,FMath::Clamp(TransitionAge/3.2f,0.f,1.f),2.f):72.f*(1-FMath::Exp(-Age/65.f));
    if(Entrance){const float P=FMath::InterpEaseInOut(0.f,1.f,FMath::Clamp((TransitionAge-.5f)/4.8f,0.f,1.f),2.f);Camera->SetActorLocation(FMath::Lerp(CameraStart,Origin+FVector(2870,0,157),P));Camera->SetActorRotation(FRotator(0,FMath::Lerp(-7.f,0.f,P),0));}
    else Camera->SetActorLocation(CameraStart+FVector(Advance,FMath::Sin(Age*.14f)*2,FMath::Sin(Age*.19f)*.7f));
    AnimateIdleEvents(D);AnimateNurse(D);
    auto& Focus=Camera->GetCameraComponent()->PostProcessSettings;Focus.DepthOfFieldFocalDistance=Entrance?(NurseScare>.5f?65.f:FMath::Max(110.f,float(Origin.X+2780-Camera->GetActorLocation().X))):950.f;Focus.DepthOfFieldFstop=Entrance?4.5f:1.8f;
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->PlayerCameraManager->UpdateCamera(D);
    if(Wheelchair)Wheelchair->SetRelativeRotation(FRotator(FMath::Sin(Age*.65f)*.6f,-50+FMath::Sin(Age*.33f)*.6f,0));
    const float Phase=FMath::Fmod(Age,9.2f);const float Flicker=Phase>6.8f&&Phase<7.5f?(FMath::Sin(Age*51)>0?1.f:.04f):.24f;
    Alarm->SetIntensity(3400*Flicker*(1-Fade));AlarmMaterial->SetVectorParameterValue(TEXT("Emission"),FLinearColor(150,.2,.05)*Flicker);
    for(int32 I=0;I<Loops.Num();++I)if(Loops[I]){Loops[I]->SetVolumeMultiplier((I==0?.16f:.055f)*Mode->Setting(TEXT("music"),.7)*FMath::Clamp(Age/3.f,0.f,1.f)*(1-Fade));if(!Loops[I]->IsPlaying())Loops[I]->Play();}
    for(const auto& Entry:LightBase)if(Entry.Key){float B=1;if(Entry.Key==BrokenLamp){float P=FMath::Fmod(Age,15.7f);if(P>11.7f&&P<12.35f)B=FMath::Sin(Age*39)>0?.34f:.045f;}if(Entry.Key==SideLight)B=.78f+.22f*FMath::Pow((FMath::Sin(Age*1.4f)+1)*.5f,3);Entry.Key->SetIntensity(Entry.Value*B*(1-Fade));}
    if(Dust){FRandomStream S(404);for(int I=0;I<42;++I){const float X=S.FRandRange(350,2100),Y=S.FRandRange(-200,200),Z=S.FRandRange(60,275),Scale=S.FRandRange(.005,.012);Dust->UpdateInstanceTransform(I,FTransform(FRotator(90,0,0),FVector(X+FMath::Sin(Age*.08f+I)*8,Y+FMath::Sin(Age*.17f+I)*5,Z+FMath::Sin(Age*.11f+I)*4),FVector(Scale)),false,I==41,true);}}
    TInlineComponentArray<ULocalFogVolumeComponent*> Fog;GetComponents(Fog);for(int32 I=0;I<Fog.Num();++I)Fog[I]->SetRelativeLocation(FVector(950+I*750+FMath::Sin(Age*.09f+I)*24,FMath::Sin(Age*.13f+I)*18,-80+FMath::Sin(Age*.11f+I)*2));
    NextCue-=D;if(NextCue<0&&!Transitioning){static const TCHAR* Sounds[]={TEXT("radio"),TEXT("menu_drip"),TEXT("knock"),TEXT("whisper"),TEXT("creak")};Sound(Sounds[CueIndex%5],CueIndex%5==3?.07f:.11f,1);++CueIndex;NextCue=CueIndex%5==3?21:9+CueIndex%3*4;}
    if(Transitioning)
    {
        TransitionAge+=D;
        if(Entrance)
        {
            if(!DoorSoundFired&&TransitionAge>2.8f){Sound(TEXT("menu_door"),.7f);DoorSoundFired=true;}
            FarDoorOpen=FMath::SmoothStep(0.f,1.f,FMath::Clamp((TransitionAge-3.25f)/1.25f,0.f,1.f));
            for(int32 I=0;I<FarHinges.Num();++I)FarHinges[I]->SetRelativeRotation(FRotator(0,(I==0?-1:1)*95*FarDoorOpen,0));
            Fade=TransitionAge<5.38f?0:TransitionAge<5.65f?FMath::Clamp((TransitionAge-5.38f)/.12f,0.f,1.f):TransitionAge<6.6f?0:FMath::Clamp((TransitionAge-6.6f)/.5f,0.f,1.f);
            if(!ScareFired&&TransitionAge>=5.65f){ScareFired=true;++ScareCount;Sound(TEXT("menu_scare"),1.f);}
        }
        else Fade=FMath::Clamp((TransitionAge-1.6f)/1.2f,0.f,1.f);
        if(TransitionAge>=(Entrance?7.6f:3.2f))
        {
            const FString Action=Pending;Transitioning=false;Mode->TitleActionExecuting=true;Mode->Command(Action);Mode->TitleActionExecuting=false;
            if(Active){CameraStart=Origin+FVector(110,-50,154);Camera->SetActorLocationAndRotation(CameraStart,FRotator(-1,-7,0));Age=0;Fade=0;NurseScare=0;FarDoorOpen=0;for(auto H:FarHinges)H->SetRelativeRotation(FRotator::ZeroRotator);if(ScareLight)ScareLight->SetIntensity(0);}
        }
    }
}
void AEWardTitleScene::EndPlay(const EEndPlayReason::Type Reason){for(auto A:Loops)if(A){A->Stop();A->DestroyComponent();}if(Camera)Camera->Destroy();if(GameCamera)GameCamera->Destroy();if(SaveCamera)SaveCamera->Destroy();if(AboutCamera)AboutCamera->Destroy();if(TrainingCamera)TrainingCamera->Destroy();Super::EndPlay(Reason);}
