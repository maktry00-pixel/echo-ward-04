#include "WardGraphics.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardEchoAssets.h"
#include "WardHospitalGameplay.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "UnrealClient.h"
#include "RHI.h"
using namespace WardData;
void AEWardCampaignMode::TickHospitalBenchmark(float Delta)
{
    static bool Started=false,Shot=false;static int32 Index=0;static double Start=0,Last=0;static FString Label=TEXT("run");
    static TArray<Json> Views;static TArray<double> Frames,GameMs,RenderMs;static TArray<TSharedPtr<FJsonValue>> Results;static FString Csv=TEXT("view,sample,frame_ms,game_ms,render_ms\n");static ACameraActor* ViewCamera=nullptr;
    static const bool Showcase=FParse::Param(FCommandLine::Get(),TEXT("WardShowcase88"));
    static const bool Compare=Showcase&&FParse::Param(FCommandLine::Get(),TEXT("WardCompare88"));
    static constexpr float ShowcaseSeconds=14.f;
    auto* H=Hero();auto* PC=Cast<APlayerController>(H->Controller);const double Now=FPlatformTime::Seconds();
    auto ConfigurePreset=[&](int32 Preset)
    {
        if(Preset>=0)
        {
            Options->SetNumberField(TEXT("graphics_preset"),FMath::Clamp(Preset,0,2));
            Options->SetNumberField(TEXT("screen_percentage"),Preset==2?67:100);
            Options->SetStringField(TEXT("shadow_quality"),Preset==2?TEXT("medium"):TEXT("high"));
            ApplySettings(false);
        }
        auto* S=GEngine->GetGameUserSettings();
        S->SetVSyncEnabled(false);S->SetFrameRateLimit(0);S->SetResolutionScaleValueEx(Preset==2?67:100);S->ApplyNonResolutionSettings();
        for(const TCHAR* Name:{TEXT("t.MaxFPS"),TEXT("r.VSync"),TEXT("t.IdleWhenNotForeground")})
            if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))C->Set(0,ECVF_SetByCode);
    };
    auto Place=[&]()
    {
        const Json V=Views[Index];const FVector At=Vector(Array(V,TEXT("at")))*100,Target=Vector(Array(V,TEXT("target")))*100;
        H->SetActorLocation(At-FVector(0,0,65),false,nullptr,ETeleportType::TeleportPhysics);H->GetCharacterMovement()->DisableMovement();PC->SetControlRotation((Target-At).Rotation());
        BookWorld->SetBright(!Boolean(V,TEXT("horror_mode")));BookWorld->LightSelectionClock=1;BookWorld->UpdateLighting(H->GetActorLocation());
        ViewCamera->SetActorLocationAndRotation(At,(Target-At).Rotation());PC->SetViewTarget(ViewCamera);PC->PlayerCameraManager->UpdateCamera(.016f);Start=Now;Last=Now;Shot=false;
    };
    if(!Started)
    {
        Started=true;FParse::Value(FCommandLine::Get(),TEXT("WardPerfLabel="),Label);Show(TEXT(""));CampaignStarted=true;HospitalStory->FreezeAI=true;
        const TArray<FString> Ids=Showcase?TArray<FString>{TEXT("v23_court_S01"),TEXT("v23_lab_clutter")}:
            TArray<FString>{TEXT("v23_court_S01"),TEXT("v23_lab_clutter"),TEXT("v23_detail_126"),TEXT("v23_court_S06")};
        for(const auto& Id:Ids)for(auto V:Array(WardEchoAssets::RealArt(),TEXT("captures")))if(String(V->AsObject(),TEXT("id"))==Id)Views.Add(V->AsObject());
        Json Corridor=MakeShared<FJsonObject>();Corridor->SetStringField(TEXT("id"),TEXT("corridor_horror"));Corridor->SetArrayField(TEXT("at"),{MakeShared<FJsonValueNumber>(42.1),MakeShared<FJsonValueNumber>(40.9),MakeShared<FJsonValueNumber>(1.7)});Corridor->SetArrayField(TEXT("target"),{MakeShared<FJsonValueNumber>(17.7),MakeShared<FJsonValueNumber>(40.9),MakeShared<FJsonValueNumber>(1.5)});Corridor->SetBoolField(TEXT("horror_mode"),true);Views.Add(Corridor);
        if(Showcase)
        {
            Json Entrance=MakeShared<FJsonObject>();
            Entrance->SetStringField(TEXT("id"),TEXT("arrival_courtyard"));
            Entrance->SetArrayField(TEXT("at"),{MakeShared<FJsonValueNumber>(51),MakeShared<FJsonValueNumber>(-43),MakeShared<FJsonValueNumber>(1.7)});
            Entrance->SetArrayField(TEXT("target"),{MakeShared<FJsonValueNumber>(49),MakeShared<FJsonValueNumber>(-20),MakeShared<FJsonValueNumber>(2.6)});
            Entrance->SetBoolField(TEXT("horror_mode"),true);
            Views.Add(Entrance);
            if(auto* Hud=PC->GetHUD())Hud->bShowHUD=false;
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("WardHorrorBenchmark28")))
            for(const FString Id:{TEXT("v28_v23_lab_clutter_horror"),TEXT("v28_v23_detail_126_horror"),TEXT("v28_v23_court_S01_horror")})
                for(auto V:Array(WardEchoAssets::HorrorArt28(),TEXT("captures")))if(String(V->AsObject(),TEXT("id"))==Id)Views.Add(V->AsObject());
        int32 RequestedPreset=-1;
        FParse::Value(FCommandLine::Get(),TEXT("WardPerfPreset75="),RequestedPreset);
        ConfigurePreset(Compare?0:RequestedPreset);
        ViewCamera=GetWorld()->SpawnActor<ACameraActor>();ViewCamera->GetCameraComponent()->SetFieldOfView(80);H->SetActorHiddenInGame(true);Place();return;
    }
    BookWorld->Update(Delta);BookWorld->UpdateLighting(H->GetActorLocation());CurrentRoom=BookWorld->Describe(H->GetActorLocation());ViewCamera->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;
    if(BookWorld->DeferredNearRemaining37>0){Start=Now;Last=Now;return;} // Wait for all local furnishings before timing or capture.
    const double Frame=(Now-Last)*1000;Last=Now;const double Since=Now-Start;
    if(Since>=6&&Frame>0){Frames.Add(Frame);GameMs.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));RenderMs.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));Csv+=FString::Printf(TEXT("%d,%d,%.6f,%.6f,%.6f\n"),Index,Frames.Num(),Frame,GameMs.Last(),RenderMs.Last());}
    if(Since>=3&&!Shot){Shot=true;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/perf_%s_%d.png"),*Label,Index),true,false);}
    if(Since<(Showcase?ShowcaseSeconds:20.f))return;
    if(Frames.Num())
    {
        auto Mean=[](const TArray<double>& A){double Sum=0;for(double V:A)Sum+=V;return Sum/A.Num();};Json R=MakeShared<FJsonObject>();R->SetStringField(TEXT("view"),String(Views[Index],TEXT("id")));R->SetNumberField(TEXT("samples"),Frames.Num());R->SetNumberField(TEXT("mean_ms"),Mean(Frames));R->SetNumberField(TEXT("fps"),1000/Mean(Frames));R->SetNumberField(TEXT("game_ms"),Mean(GameMs));R->SetNumberField(TEXT("render_ms"),Mean(RenderMs));Frames.Sort();R->SetNumberField(TEXT("p95_ms"),Frames[FMath::Min(Frames.Num()-1,FMath::FloorToInt(Frames.Num()*.95))]);R->SetNumberField(TEXT("process_physical_mb"),FPlatformMemory::GetStats().UsedPhysical/1048576.);
        const auto& Settings=ViewCamera->GetCameraComponent()->PostProcessSettings;
        R->SetBoolField(TEXT("indirect_screen_space"),Settings.DynamicGlobalIlluminationMethod==EDynamicGlobalIlluminationMethod::ScreenSpace);
        R->SetBoolField(TEXT("reflections_screen_space"),Settings.ReflectionMethod==EReflectionMethod::ScreenSpace);
        Results.Add(MakeShared<FJsonValueObject>(R));
    }
    Frames.Reset();GameMs.Reset();RenderMs.Reset();++Index;
    if(Index<Views.Num()){Place();return;}
    Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Results.Num()==Views.Num());R->SetStringField(TEXT("label"),Label);R->SetArrayField(TEXT("views"),Results);R->SetStringField(TEXT("gpu"),GRHIAdapterName);R->SetNumberField(TEXT("screen_percentage"),IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"))->GetFloat());R->SetNumberField(TEXT("quality_instances"),BookWorld->QualityProps);R->SetNumberField(TEXT("real_instances"),BookWorld->RealProps);R->SetNumberField(TEXT("detail_instances"),BookWorld->DetailProps);R->SetNumberField(TEXT("detail27_instances"),BookWorld->Detail27Props);R->SetNumberField(TEXT("horror_traces28"),BookWorld->HorrorTraces28);R->SetNumberField(TEXT("graphics_preset"),EWardGraphics::Current());R->SetNumberField(TEXT("streaming_pool_mb"),IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.PoolSize"))->GetInt());R->SetStringField(TEXT("scope"),Showcase?TEXT("Four fixed views with scripted camera and idle AI; six-second warm-up and eight-second uncapped sample for each."):
        TEXT("Fixed views with scripted camera and idle AI; six-second warm-up and fourteen-second uncapped sample for each."));
    Write(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/hospital_perf_%s.json"),*Label),R);
    FFileHelper::SaveStringToFile(Csv,*(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/hospital_perf_%s.csv"),*Label)));
    if(Compare&&EWardGraphics::Current()==0)
    {
        ConfigurePreset(2);
        Results.Reset();Csv=TEXT("view,sample,frame_ms,game_ms,render_ms\n");
        Label=TEXT("performance");Index=0;Place();return;
    }
    FPlatformMisc::RequestExit(false);
}
