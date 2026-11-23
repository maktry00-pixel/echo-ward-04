#include "WardCampaign.h"
#include "WardGraphics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "UnrealClient.h"
#include "RHI.h"
using namespace WardData;
void AEWardCampaignMode::TickCampaignBenchmark(float D)
{
    static bool Started=false,Captured=false;static double Last=0,Start=0;static int32 Stage=0,Preset=1;static TArray<double> Frames;static TArray<TSharedPtr<FJsonValue>> Results;static FString CSV=TEXT("stage,sample,frame_ms\n");
    const double Now=FPlatformTime::Seconds();
    auto Position=[&]()
    {
        const FVector Points[]={FVector(-1200,0,106),FVector(-1200,2300,106),FVector(15100,3700,106)};
        const float Yaws[]={5,0,175};Hero()->SetActorLocation(Points[Stage]);Hero()->SetActorRotation(FRotator(0,Yaws[Stage],0));Hero()->GetCharacterMovement()->DisableMovement();if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetControlRotation(FRotator(-10,Yaws[Stage],0));
    };
    if(!Started)
    {
        Started=true;FParse::Value(FCommandLine::Get(),TEXT("WardGraphics="),Preset);Preset=FMath::Clamp(Preset,0,2);EWardGraphics::Apply(Preset,false);
        auto* Settings=GEngine->GetGameUserSettings();Settings->SetVSyncEnabled(false);Settings->SetFrameRateLimit(0);Settings->ApplyNonResolutionSettings();if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("t.IdleWhenNotForeground")))C->Set(0,ECVF_SetByCode);
        Position();Last=Now;Start=Now;return;
    }
    const double Since=Now-Start,Frame=(Now-Last)*1000;Last=Now;
    if(Since>5&&Frame>0){Frames.Add(Frame);CSV+=FString::Printf(TEXT("%d,%d,%.6f\n"),Stage,Frames.Num(),Frame);}
    if(Since>2&&!Captured){Captured=true;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/full_campaign_%d_view_%d.png"),Preset,Stage),true,false);}
    if(Since<17)return;
    if(!Frames.IsEmpty())
    {
        double Total=0;for(double F:Frames)Total+=F;Frames.Sort();Json P=MakeShared<FJsonObject>();const TCHAR* Names[]={TEXT("reception"),TEXT("research"),TEXT("annex")};P->SetStringField(TEXT("view"),Names[Stage]);P->SetNumberField(TEXT("samples"),Frames.Num());P->SetNumberField(TEXT("fps"),Frames.Num()*1000/Total);P->SetNumberField(TEXT("mean_ms"),Total/Frames.Num());P->SetNumberField(TEXT("p95_ms"),Frames[FMath::Min(Frames.Num()-1,FMath::FloorToInt(Frames.Num()*.95))]);P->SetNumberField(TEXT("p99_ms"),Frames[FMath::Min(Frames.Num()-1,FMath::FloorToInt(Frames.Num()*.99))]);Results.Add(MakeShared<FJsonValueObject>(P));
    }
    ++Stage;
    if(Stage>=3)
    {
        Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Results.Num()==3);R->SetNumberField(TEXT("preset"),Preset);R->SetArrayField(TEXT("views"),Results);R->SetStringField(TEXT("scope"),TEXT("Full native campaign map, three uncapped fixed views, idle AI, wall-clock frame times; 5s warm-up and 12s sampling each. Not combat stress or a human playthrough."));R->SetStringField(TEXT("gpu"),GRHIAdapterName);Write(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/full_campaign_%d_benchmark.json"),Preset),R);FFileHelper::SaveStringToFile(CSV,*(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Prototype/full_campaign_%d_frames.csv"),Preset)));FPlatformMisc::RequestExit(false);return;
    }
    Frames.Reset();Captured=false;Start=Now;Position();
}
