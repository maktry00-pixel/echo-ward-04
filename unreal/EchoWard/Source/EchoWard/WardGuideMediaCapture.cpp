#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardTitleScene.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
using namespace WardData;
void AEWardCampaignMode::TickGuideMediaCapture(float Delta)
{
    struct FView{const TCHAR* Id;const TCHAR* Page;FVector Eye,Target;};
    static const TArray<FView> Views={
        {TEXT("exploration"),TEXT("title"),{110,-50,154},{2250,0,145}},
        {TEXT("search"),TEXT("saves"),{730,50,145},{1140,340,100}},
        {TEXT("combat"),TEXT("title"),{850,90,150},{2630,0,145}},
        {TEXT("hiding"),TEXT("about"),{900,0,115},{1260,260,65}},
        {TEXT("medicine"),TEXT("about"),{490,-70,150},{800,-260,105}},
        {TEXT("generator"),TEXT("training"),{1900,-10,150},{1940,700,130}},
        {TEXT("saving"),TEXT("saves"),{800,0,145},{850,350,100}},
        {TEXT("annex"),TEXT("settings"),{1930,-635,144},{1900,600,135}},
        {TEXT("ward"),TEXT("saves"),{15,-95,139},{1420,0,140}}};
    static TWeakObjectPtr<ACameraActor> View;static int Index=-1;static bool Shot=false;static double Since=0;static float Exposure=3.8f;static TArray<TSharedPtr<FJsonValue>> Entries;
    static FVector CapturedEye;
    FVector DesiredEye;
    auto Place=[&](){const auto& V=Views[Index];Show(V.Page);Interface->Close();TitleScene->SetActorTickEnabled(false);if(TitleScene->SaveBackdropActive)TitleScene->TickSaveBackdrop(1.5f);if(TitleScene->AboutBackdropActive)TitleScene->TickAboutBackdrop(1.5f);if(TitleScene->TrainingBackdropActive)TitleScene->TickTrainingBackdrop(1.5f);
        TInlineComponentArray<USkeletalMeshComponent*> Skeletons;TitleScene->GetComponents(Skeletons);for(auto* M:Skeletons)M->SetVisibility(false,true);TitleScene->RollingBall->SetVisibility(false);TitleScene->Forearm->SetVisibility(false);TitleScene->ScareLight->SetIntensity(0);
        const FString Page=V.Page;const FVector Offset=Page==TEXT("saves")?FVector(0,-8000,0):Page==TEXT("about")?FVector(0,-16000,0):Page==TEXT("training")||Page==TEXT("settings")?FVector(0,16000,0):FVector::ZeroVector;
        const FVector Base=TitleScene->GetActorLocation()+Offset;DesiredEye=Base+V.Eye;CapturedEye=DesiredEye;View->SetActorLocationAndRotation(DesiredEye,(V.Target-V.Eye).Rotation());UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(View.Get());Since=FPlatformTime::Seconds();Shot=false;};
    if(Index<0)
    {
        NewGame(TEXT("story"));GetWorld()->bIsCameraMoveableWhenPaused=true;Hero()->SetActorHiddenInGame(true);for(auto E:Enemies){E->SetActorHiddenInGame(true);E->SetActorTickEnabled(false);}
        for(TActorIterator<AActor> It(GetWorld());It;++It){TInlineComponentArray<UTextRenderComponent*> Labels;It->GetComponents(Labels);for(auto* L:Labels)L->SetVisibility(false);}
        View=GetWorld()->SpawnActor<ACameraActor>();auto* C=View->GetCameraComponent();C->SetConstraintAspectRatio(false);C->SetFieldOfView(72);
        FParse::Value(FCommandLine::Get(),TEXT("WardGuideEV="),Exposure);auto& P=C->PostProcessSettings;
        P.bOverride_AutoExposureMinBrightness=P.bOverride_AutoExposureMaxBrightness=true;P.AutoExposureMinBrightness=Exposure;P.AutoExposureMaxBrightness=Exposure;
        P.bOverride_ColorSaturation=true;P.ColorSaturation=FVector4(.88,.90,.94,1);P.bOverride_ColorContrast=true;P.ColorContrast=FVector4(1.06,1.06,1.06,1);
        P.bOverride_VignetteIntensity=true;P.VignetteIntensity=.18f;P.bOverride_BloomIntensity=true;P.BloomIntensity=.25f;P.bOverride_MotionBlurAmount=true;P.MotionBlurAmount=0;P.bOverride_FilmGrainIntensity=true;P.FilmGrainIntensity=.015f;
        P.bOverride_DepthOfFieldFstop=true;P.DepthOfFieldFstop=8;P.bOverride_DepthOfFieldFocalDistance=true;P.DepthOfFieldFocalDistance=1100;
        UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(View.Get());Index=0;Place();return;
    }
    auto* PC=UGameplayStatics::GetPlayerController(this,0);PC->PlayerCameraManager->UpdateCamera(.016f);
    const double T=FPlatformTime::Seconds()-Since;
    if(!Shot&&T>3)
    {
        const auto& V=Views[Index];FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/GuideMedia/")+V.Id+TEXT(".png"),false,false);Shot=true;
        Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("id"),V.Id);J->SetStringField(TEXT("camera"),CapturedEye.ToString());J->SetStringField(TEXT("set"),V.Page);J->SetNumberField(TEXT("exposure_ev"),Exposure);J->SetNumberField(TEXT("fov"),72);J->SetNumberField(TEXT("camera_error_cm"),FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(),CapturedEye));Entries.Add(MakeShared<FJsonValueObject>(J));
    }
    if(Shot&&T>4.2)
    {
        if(++Index<Views.Num()){Place();return;}Json R=MakeShared<FJsonObject>();bool Valid=true;for(auto E:Entries)Valid&=Number(E->AsObject(),TEXT("camera_error_cm"))<2;R->SetBoolField(TEXT("success"),Valid);R->SetStringField(TEXT("source"),TEXT("native Unreal hospital presentation sets using campaign assets; no HUD or characters"));R->SetNumberField(TEXT("count"),Entries.Num());R->SetArrayField(TEXT("views"),Entries);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/GuideMedia/capture.json"),R);FPlatformMisc::RequestExit(false);
    }
}
