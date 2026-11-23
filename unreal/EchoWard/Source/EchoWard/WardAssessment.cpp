// Optional native assessment, enabled only by explicit command-line flags.
// Benchmark teleports between fixed views. Smoke walks the room with real collision.
#include "WardPrototype.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
#include "HAL/IConsoleManager.h"
#include "WardGraphics.h"

namespace
{
    struct FAssessment
    {
        bool Started=false, Finished=false, Captured=false;
        int32 Stage=0, Frames=0;
        double Start=0, StageStart=0, Previous=0, FinishAt=0;
        TArray<double> Times;
        double GameMs=0, RenderMs=0, GpuMs=0;
        int32 GpuSamples=0;
        TArray<TSharedPtr<FJsonValue>> Results, Checks;
        FString Profile=TEXT("high");
    };
    FAssessment Assessment;
    FString Output(const FString& File) { return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Prototype")/File); }
    void Write(const FString& File,const TSharedPtr<FJsonObject>& Data)
    {
        FString Text; const auto Writer=TJsonWriterFactory<>::Create(&Text); FJsonSerializer::Serialize(Data.ToSharedRef(),Writer);
        FFileHelper::SaveStringToFile(Text,*Output(File));
        UE_LOG(LogTemp,Display,TEXT("WARD_ASSESSMENT %s"),*Output(File));
    }
    void Capture(const FString& Name) { FScreenshotRequest::RequestScreenshot(Output(Name+TEXT(".png")),true,false); }
    void Check(bool Passed,const FString& Name)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Passed);
        Assessment.Checks.Add(MakeShared<FJsonValueObject>(Row));
        UE_LOG(LogTemp,Display,TEXT("WARD_CHECK %s: %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Name);
    }
}

void AEWardGameMode::TickAssessment(float Delta)
{
    AEWardCharacter* Hero=Cast<AEWardCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    APlayerController* PC=UGameplayStatics::GetPlayerController(this,0);
    if (!Hero || !PC) return;
    FAssessment& S=Assessment; const double Now=FPlatformTime::Seconds();
    const bool Benchmark=FParse::Param(FCommandLine::Get(),TEXT("WardBenchmark"));
    if (!S.Started)
    {
        S.Started=true; S.Start=Now; S.StageStart=Now;S.Previous=Now;
        FParse::Value(FCommandLine::Get(),TEXT("WardProfile="),S.Profile);
        PC->SetIgnoreLookInput(true); PC->SetIgnoreMoveInput(true);
        if (!Benchmark)
        {
            auto Value=[](const TCHAR* Name){const IConsoleVariable* Var=IConsoleManager::Get().FindConsoleVariable(Name);return Var?Var->GetFloat():-1.f;};
            EWardGraphics::Apply(0,false);
            Check(Value(TEXT("sg.GlobalIlluminationQuality"))==3 && Value(TEXT("r.ScreenPercentage"))==100,TEXT("Quality profile uses Epic Lumen and native resolution"));
            EWardGraphics::Apply(2,false);
            Check(Value(TEXT("sg.GlobalIlluminationQuality"))==2 && Value(TEXT("r.ScreenPercentage"))==75,TEXT("Performance profile keeps Lumen and applies 75 percent TSR"));
            EWardGraphics::Apply(1,false);
            Check(Value(TEXT("r.ScreenPercentage"))==100 && Value(TEXT("sg.GlobalIlluminationQuality"))==2,TEXT("Balanced profile restores native resolution and High Lumen"));
            Check(GetWorld()->GetMapName().Contains(TEXT("Ward04")),TEXT("Native Ward04 map loaded"));
            Check(Effects.Num()==10,TEXT("All ten sounds are preloaded before interactions and combat"));
            Check(Hero->GetMesh()->GetSkeletalMeshAsset()!=nullptr,TEXT("Survivor skeletal mesh loaded"));
            Check(Hero->Idle && Hero->Walk && Hero->Run,TEXT("Idle/walk/run native animations loaded"));
            Check(Hero->GetMesh()->DoesSocketExist(Hero->Pistol->GetAttachSocketName()),TEXT("Weapon attaches to an existing hand bone"));
            Check(Hero->Pistol->GetStaticMesh()!=nullptr,TEXT("Pistol native mesh loaded"));
            Check(Monster && Monster->Body->GetStaticMesh() && Monster->Legs.Num()==6,TEXT("Six-legged monster native meshes loaded"));
            FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(WardTestWall),false,Hero);
            Check(GetWorld()->LineTraceSingleByChannel(Hit,FVector(500,0,140),FVector(700,0,140),ECC_Visibility,Query),TEXT("East wall physically blocks trace"));
        }
    }
    if (S.Finished) { if(Now-S.FinishAt>2) FPlatformMisc::RequestExit(false); return; }
    if (Benchmark)
    {
        const FVector Positions[]={FVector(-100,-130,100),FVector(330,80,100),FVector(0,800,100)};
        const FRotator Rotations[]={FRotator(-8,75,0),FRotator(-10,180,0),FRotator(-5,-90,0)};
        const TCHAR* Names[]={TEXT("ward"),TEXT("furniture"),TEXT("corridor")};
        if (S.Frames==0)
        {
            Hero->SetActorLocation(Positions[S.Stage],false,nullptr,ETeleportType::TeleportPhysics);
            Hero->GetCharacterMovement()->StopMovementImmediately(); PC->SetControlRotation(Rotations[S.Stage]);
        }
        ++S.Frames;
        const double Age=Now-S.StageStart;
        // Save the view during warmup: PNG readback must not pollute timed frames.
        if (Age>=2 && !S.Captured) { Capture(TEXT("benchmark_")+S.Profile+TEXT("_")+Names[S.Stage]);S.Captured=true; }
        if (Age>=5)
        {
            S.Times.Add((Now-S.Previous)*1000);
            S.GameMs+=FPlatformTime::ToMilliseconds(GGameThreadTime);
            S.RenderMs+=FPlatformTime::ToMilliseconds(GRenderThreadTime);
            const uint32 GpuCycles=RHIGetGPUFrameCycles();
            if(GpuCycles>0) { S.GpuMs+=FPlatformTime::ToMilliseconds(GpuCycles);++S.GpuSamples; }
        }
        S.Previous=Now;
        if(Age>=13)
        {
            S.Times.Sort(); double Sum=0; for(double Value:S.Times)Sum+=Value;
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("scene"),Names[S.Stage]);Row->SetNumberField(TEXT("frames"),S.Times.Num());
            Row->SetNumberField(TEXT("mean_ms"),Sum/S.Times.Num());Row->SetNumberField(TEXT("average_fps"),1000*S.Times.Num()/Sum);
            Row->SetNumberField(TEXT("median_ms"),S.Times[S.Times.Num()/2]);Row->SetNumberField(TEXT("p95_ms"),S.Times[FMath::FloorToInt(S.Times.Num()*.95)]);
            Row->SetNumberField(TEXT("process_ram_mb"),FPlatformMemory::GetStats().UsedPhysical/1048576.0);
            Row->SetNumberField(TEXT("game_thread_ms"),S.GameMs/S.Times.Num());Row->SetNumberField(TEXT("render_thread_ms"),S.RenderMs/S.Times.Num());
            Row->SetNumberField(TEXT("gpu_ms"),S.GpuSamples?S.GpuMs/S.GpuSamples:0);Row->SetNumberField(TEXT("gpu_samples"),S.GpuSamples);
            S.Results.Add(MakeShared<FJsonValueObject>(Row));S.Times.Reset();S.GameMs=0;S.RenderMs=0;S.GpuMs=0;S.GpuSamples=0;S.Stage++;S.Frames=0;S.Captured=false;S.StageStart=Now;
            if(S.Stage==3)
            {
                auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("profile"),S.Profile);Report->SetArrayField(TEXT("scenarios"),S.Results);
                const FIntPoint Size=GEngine->GameViewport->Viewport->GetSizeXY();Report->SetNumberField(TEXT("width"),Size.X);Report->SetNumberField(TEXT("height"),Size.Y);
                Report->SetNumberField(TEXT("warmup_seconds_per_scene"),5);Report->SetNumberField(TEXT("sample_seconds_per_scene"),8);
                auto Settings=MakeShared<FJsonObject>();
                for(const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("r.AntiAliasingMethod"),TEXT("sg.GlobalIlluminationQuality"),TEXT("sg.ReflectionQuality"),TEXT("sg.ShadowQuality"),TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"),TEXT("r.Shadow.Virtual.ResolutionLodBiasLocalMoving")})
                    if(IConsoleVariable* Var=IConsoleManager::Get().FindConsoleVariable(Name))Settings->SetNumberField(Name,Var->GetFloat());
                Report->SetObjectField(TEXT("render_settings"),Settings);
                Report->SetStringField(TEXT("note"),TEXT("Native editor -game, uncapped staged views, idle AI. Wall-clock frames; screenshot at 2s, sampling from 5s. GPU/CPU averages overlap, cannot be summed. Not a full campaign or packaged build."));
                Write(TEXT("benchmark_")+S.Profile+TEXT(".json"),Report);S.Finished=true;S.FinishAt=Now;
            }
        }
        return;
    }
    AEWInteractable *Drawer=nullptr,*Panel=nullptr,*Door=nullptr;
    for(TActorIterator<AEWInteractable> It(GetWorld());It;++It)
    {
        if(It->Kind==EWardInteraction::Drawer)Drawer=*It;
        if(It->Kind==EWardInteraction::Power)Panel=*It;
        if(It->Kind==EWardInteraction::Door)Door=*It;
    }
    auto Advance=[&](){Hero->StopInteract();S.Stage++;S.StageStart=Now;};
    auto Move=[&](FVector Goal)
    {
        const FVector Offset=Goal-Hero->GetActorLocation();
        if(Offset.Size2D()<18){Hero->GetCharacterMovement()->StopMovementImmediately();return true;}
        Hero->AddMovementInput(Offset.GetSafeNormal2D(),1,true);PC->SetControlRotation(Offset.Rotation());return false;
    };
    const double Age=Now-S.StageStart;
    if(!Drawer || !Panel || !Door || Now-S.Start>100 || Hero->Dead)
    {
        Check(false,FString::Printf(TEXT("Runtime route failed at stage %d"),S.Stage));S.Stage=99;
    }
    switch(S.Stage)
    {
    case 0: if(Move(FVector(-260,170,96))){Check(Hero->GetCharacterMovement()->IsMovingOnGround(),TEXT("Real walking reaches cabinet with floor collision"));Advance();}break;
    case 1: PC->SetControlRotation(FRotator(-6,180,0));if(Age>.6 && Hero->Focus==Drawer){Hero->StartInteract();Hero->StopInteract();Check(Drawer->Opened,TEXT("Real drawer interaction opens furniture"));Advance();}break;
    case 2: if(Age>.5 && !Hero->Held)Hero->StartInteract();if(Drawer->Found){Check(true,TEXT("Held search reveals fuse"));Check(Drawer->Moving->GetRelativeLocation().X<-31 && !Drawer->IsActorTickEnabled(),TEXT("Drawer reaches open position then stops ticking"));Advance();}break;
    case 3: Hero->StartInteract();Hero->StopInteract();if(Hero->HasFuse){Check(true,TEXT("Separate pickup takes actual fuse"));Advance();Capture(TEXT("smoke_search"));}break;
    case 4:
        if(Monster && !Monster->Dead)
        {
            Hero->AimPressed();PC->SetControlRotation((Monster->Head->Bounds.Origin-Hero->Camera->GetComponentLocation()).Rotation());
            if(Age>1)Hero->Fire();if(Hero->Rounds==0)Hero->Reload();
        }
        else {Hero->AimReleased();Check(Hero->Rounds<9,TEXT("Aimed shots consume real ammunition"));Check(Monster && Monster->Dead,TEXT("Line-traced bullets kill pursuing monster"));Advance();Capture(TEXT("smoke_combat"));}break;
    case 5: Hero->Reload();if(Hero->Rounds==9){Check(Hero->Reserve<18,TEXT("Reload transfers real reserve ammunition"));Advance();}break;
    case 6: if(Move(FVector(0,50,96)))Advance();break;
    case 7: if(Move(FVector(150,-270,96)))Advance();break;
    case 8: if(Move(FVector(430,-270,96)))Advance();break;
    case 9: PC->SetControlRotation(FRotator(-6,0,0));if(Age>.6 && Hero->Focus==Panel){Hero->StartInteract();Advance();}break;
    case 10: if(!Hero->Held)Hero->StartInteract();if(PowerOn){Check(!Hero->HasFuse && Panel->Used,TEXT("Held installation consumes fuse and unlocks exit"));Advance();}break;
    case 11: if(Move(FVector(150,-270,96)))Advance();break;
    case 12: if(Move(FVector(0,100,96)))Advance();break;
    case 13: if(Move(FVector(0,320,96)))Advance();break;
    case 14: PC->SetControlRotation(FRotator(-5,90,0));if(Age>.6 && Hero->Focus==Door){Hero->StartInteract();Hero->StopInteract();if(Door->Opened){Check(true,TEXT("Powered physical door opens"));Advance();}}break;
    case 15: if(Age>1 && Move(FVector(0,900,96))){Check(FMath::IsNearlyEqual(Door->Hinge->GetRelativeRotation().Yaw,90.f,.1f) && !Door->IsActorTickEnabled(),TEXT("Door completes opening and stops ticking without blocking exit"));Advance();}break;
    case 16: if(Move(FVector(0,1050,96)))Advance();break;
    case 17: if(Won){Check(true,TEXT("Physical return through corridor wins the room"));S.Stage=99;Capture(TEXT("smoke_win"));}break;
    }
    if(S.Stage==99)
    {
        auto Report=MakeShared<FJsonObject>();Report->SetArrayField(TEXT("checks"),S.Checks);Report->SetBoolField(TEXT("won"),Won);Report->SetNumberField(TEXT("seconds"),Now-S.Start);
        Report->SetNumberField(TEXT("health"),Hero->Health);Report->SetNumberField(TEXT("rounds"),Hero->Rounds);Report->SetNumberField(TEXT("reserve"),Hero->Reserve);
        Report->SetStringField(TEXT("note"),TEXT("Native live-room route uses character interaction/combat methods and physical movement. No teleports, resource gifts or invulnerability. Does not validate keyboard bindings or entire Godot campaign."));
        Write(TEXT("smoke_result.json"),Report);S.Finished=true;S.FinishAt=Now;
    }
}
