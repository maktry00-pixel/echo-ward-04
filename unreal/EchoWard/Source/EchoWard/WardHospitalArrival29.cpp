#include "WardHospitalGameplay.h"
#include "WardHomeAssets53.h"
#include "Components/AudioComponent.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardEchoAssets.h"
#include "WardOnline80.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/WidgetComponent.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
using namespace WardData;
namespace {
AActor* ArrivalActor(UWorld* W,FVector At)
{
    auto* A=W->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(A);Root->SetMobility(EComponentMobility::Movable);A->SetRootComponent(Root);Root->RegisterComponent();A->SetActorLocation(At);return A;
}
UStaticMeshComponent* ArrivalBox(AActor* A,USceneComponent* Parent,FVector At,FVector Size,UMaterialInterface* Mat)
{
    auto* C=NewObject<UStaticMeshComponent>(A);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(At);C->SetRelativeScale3D(Size/100);C->SetMaterial(0,Mat);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetAffectDynamicIndirectLighting(false);C->SetAffectDistanceFieldLighting(false);C->RegisterComponent();A->AddInstanceComponent(C);return C;
}
}
void AEWardHospitalGameplay::BuildArrival()
{
    ArrivalData=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_arrival_v29.json"));
    ArrivalCar=ArrivalActor(GetWorld(),Vector(Array(ArrivalData,TEXT("car_to")))*100);ArrivalActors.Add(ArrivalCar);
    const FVector CarSize=Vector(Array(ArrivalData,TEXT("car_size")))*100;
    BuildHeroVehicle31();
    ArrivalCar->SetActorRotation(FRotator(0,90,0));
    auto* Body=NewObject<UBoxComponent>(ArrivalCar);Body->SetupAttachment(ArrivalCar->GetRootComponent());Body->SetRelativeLocation(FVector(0,0,65));Body->SetBoxExtent(FVector(244,95,65));Body->SetCollisionProfileName(TEXT("BlockAll"));Body->RegisterComponent();ArrivalCar->AddInstanceComponent(Body);
    for(float Y:{-65.f,65.f})
    {
        auto* L=NewObject<USpotLightComponent>(ArrivalCar);L->SetupAttachment(ArrivalCar->GetRootComponent());L->SetRelativeLocation(FVector(220,Y,65));L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(1800);L->SetAttenuationRadius(2200);L->SetOuterConeAngle(29);L->SetInnerConeAngle(18);L->SetLightColor(FLinearColor(.82f,.86f,.76f));L->SetCastShadows(false);L->RegisterComponent();ArrivalCar->AddInstanceComponent(L);ArrivalHeadlights.Add(L);
    }
    ArrivalCamera=GetWorld()->SpawnActor<ACameraActor>();ArrivalCamera->GetCameraComponent()->SetFieldOfView(60);
    auto* Gate=ArrivalActor(GetWorld(),Vector(Array(ArrivalData,TEXT("barrier_pivot")))*100);ArrivalActors.Add(Gate);
    ArrivalBarrierPivot=NewObject<USceneComponent>(Gate);ArrivalBarrierPivot->SetupAttachment(Gate->GetRootComponent());ArrivalBarrierPivot->SetMobility(EComponentMobility::Movable);ArrivalBarrierPivot->RegisterComponent();Gate->AddInstanceComponent(ArrivalBarrierPivot);
    ArrivalBox(Gate,Gate->GetRootComponent(),FVector(0,0,-35),FVector(35,38,120),Mode->BookWorld->SurfaceMaterial(11));
    ArrivalBox(Gate,ArrivalBarrierPivot,FVector(850,0,0),FVector(1700,13,13),Mode->BookWorld->SurfaceMaterial(2));
    for(int I=0;I<14;++I)ArrivalBox(Gate,ArrivalBarrierPivot,FVector(45+I*120,0,0),FVector(45,14,14),Mode->BookWorld->SurfaceMaterial(12));
    ArrivalGateBlock=NewObject<UBoxComponent>(Gate);ArrivalGateBlock->SetupAttachment(Gate->GetRootComponent());ArrivalGateBlock->SetRelativeLocation(FVector(850,0,14));ArrivalGateBlock->SetBoxExtent(FVector(850,9,110));ArrivalGateBlock->SetCollisionProfileName(TEXT("BlockAll"));ArrivalGateBlock->RegisterComponent();Gate->AddInstanceComponent(ArrivalGateBlock);
    auto* Terminal=ArrivalActor(GetWorld(),Vector(Array(ArrivalData,TEXT("panel")))*100);ArrivalActors.Add(Terminal);
    WardEchoAssets::Attach(Terminal,Terminal->GetRootComponent(),149,FBox(FVector(-15,-10,0),FVector(15,10,35)));
    ArrivalBox(Terminal,Terminal->GetRootComponent(),FVector(0,-12,70),FVector(150,6,108),Mode->BookWorld->SurfaceMaterial(11));
    ArrivalNotice=NewObject<UWidgetComponent>(Terminal);ArrivalNotice->SetupAttachment(Terminal->GetRootComponent());ArrivalNotice->SetWidgetSpace(EWidgetSpace::World);ArrivalNotice->SetRelativeLocation(FVector(0,-17,70));ArrivalNotice->SetRelativeRotation(FRotator(0,-90,0));ArrivalNotice->SetRelativeScale3D(FVector(.11f));ArrivalNotice->SetDrawSize(FVector2D(1200,900));ArrivalNotice->SetPivot(FVector2D(.5f,.5f));ArrivalNotice->SetTwoSided(true);ArrivalNotice->SetManuallyRedraw(true);ArrivalNotice->SetCollisionEnabled(ECollisionEnabled::NoCollision);ArrivalNotice->SetCastShadow(false);ArrivalNotice->RegisterComponent();Terminal->AddInstanceComponent(ArrivalNotice);
    static FSlateColorBrush Paper(FLinearColor::White);const FString Font=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf");
    ArrivalNotice->SetSlateWidget(SNew(SBorder).BorderImage(&Paper).BorderBackgroundColor(FLinearColor(.075f,.08f,.07f,1)).Padding(55)
        [SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(0,20)[SNew(STextBlock).Text(FText::FromString(TEXT("БОЛЬНИЦА № 333"))).Font(FSlateFontInfo(Font,66)).ColorAndOpacity(FLinearColor(.74f,.76f,.69f)).Justification(ETextJustify::Center)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,25)[SNew(SBox).HeightOverride(7)[SNew(SBorder).BorderImage(&Paper).BorderBackgroundColor(FLinearColor(.42f,.09f,.065f))]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,25)[SNew(STextBlock).Text(FText::FromString(TEXT("СТАРОЕ УВЕДОМЛЕНИЕ\n\nЛабораторный блок закрыт\nпосле аварии.\n\nПосетителям вход запрещён."))).Font(FSlateFontInfo(Font,50)).ColorAndOpacity(FLinearColor(.64f,.67f,.59f)).Justification(ETextJustify::Center)]
        ]);
    ArrivalNotice->RequestRedraw();
    ArrivalAlarm=NewObject<UPointLightComponent>(Terminal);ArrivalAlarm->SetupAttachment(Terminal->GetRootComponent());ArrivalAlarm->SetRelativeLocation(FVector(0,-35,90));ArrivalAlarm->SetIntensityUnits(ELightUnits::Lumens);ArrivalAlarm->SetIntensity(180);ArrivalAlarm->SetAttenuationRadius(500);ArrivalAlarm->SetLightColor(FLinearColor(.8f,.3f,.12f));ArrivalAlarm->SetCastShadows(false);ArrivalAlarm->RegisterComponent();Terminal->AddInstanceComponent(ArrivalAlarm);
    // Old damage is a persistent remnant, never a newly triggered explosion.
    auto* Lab=ArrivalActor(GetWorld(),Vector(Array(ArrivalData,TEXT("lab_at")))*100);Lab->Tags.Add(TEXT("old_lab29"));ArrivalActors.Add(Lab);
    auto* Plate=ArrivalBox(Lab,Lab->GetRootComponent(),FVector(0,0,3),FVector(80,48,5),Mode->BookWorld->SurfaceMaterial(11));Plate->SetRelativeRotation(FRotator(0,17,0));
    LabAlarm=NewObject<UPointLightComponent>(Lab);LabAlarm->SetupAttachment(Lab->GetRootComponent());LabAlarm->SetRelativeLocation(FVector(0,0,180));LabAlarm->SetIntensityUnits(ELightUnits::Lumens);LabAlarm->SetIntensity(160);LabAlarm->SetAttenuationRadius(400);LabAlarm->SetLightColor(FLinearColor(.65f,.07f,.03f));LabAlarm->SetCastShadows(false);LabAlarm->RegisterComponent();Lab->AddInstanceComponent(LabAlarm);

    ResetArrival();
}
void AEWardHospitalGameplay::ResetArrival()
{
    StopHome52();
    if (IsValid(DriveAudio53)) DriveAudio53->Stop();
    DriveAudio53 = nullptr;
    VehicleCloseQuality32=false;for(auto& Pair:VehicleParts31)if(auto* Part=Pair.Value.Get()){Part->SetForcedLodModel(0);Part->SetTextureForceResidentFlag(false);}
    if(ArrivalCamera)ArrivalCamera->GetCameraComponent()->SetFieldOfView(60);
    ArrivalStage=-1;EntranceWaiting29=0;ArrivalClock=0;EntryClock=0;GateBlend=0;Seated31=false;VehicleYaw31=0;VehiclePitch31=0;VehicleAutoClose31=0;WheelRoll31=0;VehicleLastAt31=FVector::ZeroVector;
    for(auto& P:VehicleOpen31){P.Value=0;VehicleTarget31.Add(P.Key,0);if(auto* C=VehicleParts31.FindRef(P.Key).Get())C->SetRelativeRotation(FRotator::ZeroRotator);}
    if(Mode&&Mode->Hero())
    {
        auto* H=Mode->Hero();if(auto* PC=Cast<APlayerController>(H->Controller);PC&&PC->GetViewTarget()==ArrivalCamera)PC->SetViewTarget(H);
        H->SetActorHiddenInGame(false);H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
    ApplyArrivalState();
}
void AEWardHospitalGameplay::StartArrival()
{
    if(!Mode||!Mode->Hero())return;
    if(!Mode->Assessment&&!Mode->Completed.Contains(TEXT("HOME_CALL52"))){StartHome52();return;}
    ResetArrival();ArrivalStage=0;
    Mode->Hero()->SetActorLocation(Vector(Array(ArrivalData,TEXT("exit")))*100,false,nullptr,ETeleportType::TeleportPhysics);
    Mode->Options->SetBoolField(TEXT("inspection_bright"),false);Mode->BookWorld->SetBright(false);
    Mode->Hero()->NoticeTime = 0;
    ApplyArrivalState();
    Refresh();
    TickArrival(0);
    if (!Mode->Assessment)
    {
        if (auto* PC = Cast<APlayerController>(Mode->Hero()->Controller); PC && PC->PlayerCameraManager)
            PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, .5f, FLinearColor::Black, false);
    }
}
void AEWardHospitalGameplay::ApplyArrivalState()
{
    if(!ArrivalCar)return;
    for(auto A:ArrivalActors)if(A){const bool OldLab=A->Tags.Contains(TEXT("old_lab29"));A->SetActorHiddenInGame(OldLab?!Mode->Flag(TEXT("old_lab_accident29")):ArrivalStage<0);A->SetActorEnableCollision(!OldLab&&((A==ArrivalCar&&ArrivalStage>=2)||(ArrivalBarrierPivot&&A==ArrivalBarrierPivot->GetOwner()&&ArrivalStage>=0)));}
    if(ArrivalGateBlock){ArrivalGateBlock->SetCollisionEnabled(ArrivalStage==0||ArrivalStage==1||ArrivalStage==2?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);}
    if(ArrivalNotice&&ArrivalStage>=0)ArrivalNotice->RequestRedraw();
    if(ArrivalAlarm)ArrivalAlarm->SetVisibility(ArrivalStage>=0);
    if(LabAlarm)LabAlarm->SetVisibility(Mode->Flag(TEXT("old_lab_accident29")));
    for(auto L:ArrivalHeadlights)L->SetVisibility(ArrivalStage==0||ArrivalStage==1);
    if(ArrivalStage!=0)ArrivalCar->SetActorLocation(Vector(Array(ArrivalData,TEXT("car_to")))*100);
    for(auto& P:Mode->BookWorld->Portals)if(P.Zone==TEXT("N02")&&P.Side==TEXT("north"))P.Type=ArrivalStage>=4&&Chapter()<4?TEXT("shutter"):TEXT("ordinary");
    if(ArrivalStage>=0)
    {
        const bool Allow=ArrivalStage==3||Chapter()>=4;
        for(auto& P:Mode->BookWorld->Portals)if(P.Zone==TEXT("N02")&&P.Side==TEXT("north"))
        {if(P.Open!=Allow){P.Open=Allow;Mode->BookWorld->RefreshPortalVisual(P);}P.Locked=!Allow;P.InsideOnly=false;}
    }
}
FString AEWardHospitalGameplay::ArrivalObjective()const
{
    switch(ArrivalStage)
    {
    case 0:return TEXT("Прибытие в больницу № 333");
    case 1:return TEXT("Машина остановилась у КПП · F — выйти из машины");
    case 2:return TEXT("Подойди к терминалу КПП · F — связаться с дежурной");
    case 3:
        if (EntranceWaiting29 > 0)
            return FString::Printf(TEXT("Соберитесь в приёмной за тамбуром N02 · ожидаем игроков: %d"), EntranceWaiting29);
        return TEXT("На КПП — следы давней аварии. Войди в приёмную через тамбур N02");
    case 4:return TEXT("Защитная система заблокировала выход · послушай сообщение врача");
    default:return FString();
    }
}
bool AEWardHospitalGameplay::ArrivalInteract()
{
    if(ArrivalStage>=2&&VehicleInteract31())return true;
    if(ArrivalStage==0){ArrivalClock=DriveDuration53;TickArrival(0);return true;}
    if(ArrivalStage==1)
    {
        if(!VehicleExitClear31()){Mode->Notify(TEXT("Выход из машины перекрыт."));return true;}
        Seated31=false;VehicleTarget31.Add(TEXT("DoorFL"),1);VehicleAutoClose31=1.5f;
        ArrivalStage=2;auto* H=Mode->Hero();H->SetActorHiddenInGame(false);H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);H->SetActorLocation(Vector(Array(ArrivalData,TEXT("exit")))*100,false,nullptr,ETeleportType::TeleportPhysics);
        if(auto* PC=Cast<APlayerController>(H->Controller)){PC->SetViewTargetWithBlend(H,.65f);PC->SetControlRotation(FRotator(-5,65,0));}
        Mode->Completed.Add(TEXT("SCENE_01_01"));Mode->Values.Add(TEXT("hospital_step"),1);Mode->Cue(TEXT("door"),H->GetActorLocation(),.25f);Mode->Hero()->NoticeTime=0;ApplyArrivalState();Refresh();if(NarrativeEnabled35)BeginNarrative35(TEXT("SCENE_01_01"),true);Mode->AutoSave();return true;
    }
    return UseCheckpoint29(Mode->Hero()->GetActorLocation());
}

bool AEWardHospitalGameplay::UseCheckpoint29(FVector Player)
{
    if (ArrivalStage != 2) return false;
    const FVector Target = Vector(Array(ArrivalData, TEXT("checkpoint"))) * 100;
    if (FVector::DistSquared(Player, Target) > FMath::Square(210.)) return false;

    FHitResult Hit;
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(ArrivalNotice), false, Mode->Hero());
    const FVector Notice = Vector(Array(ArrivalData, TEXT("panel"))) * 100 + FVector(0, -16, 86);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Player + FVector(0, 0, 35), Notice, ECC_Visibility, Query) &&
        Hit.GetActor() != ArrivalNotice->GetOwner())
    {
        Mode->Notify(TEXT("Подойди к уведомлению с наружной стороны КПП."));
        return true;
    }
    if (NarrativeEnabled35)
    {
        Mode->Hero()->NoticeTime = 0;
        if (ActiveScene35 == TEXT("CHECKPOINT_69")) SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"), false);
        return BeginNarrative35(TEXT("CHECKPOINT_69"), true);
    }
    FinishCheckpoint69();
    return true;
}

bool AEWardHospitalGameplay::PartyInsideEntrance29(FVector Player) const
{
    const auto& Book = *Mode->BookWorld;
    const auto* Lobby = Book.Rooms.FindByPredicate([](const auto& Room) { return Room.Id == TEXT("N01"); });
    if (!Lobby || Player.Y < Lobby->Bounds.Min.Y + 60) return false;
    if (Player.Y >= 0)
    {
        const FString Location = Book.LocationAt(Player);
        return Location == TEXT("1F") || Location == TEXT("2F") || Location == TEXT("3F");
    }
    return Player.X > Lobby->Bounds.Min.X && Player.X < Lobby->Bounds.Max.X &&
        Player.Y < Lobby->Bounds.Max.Y && FMath::Abs(Player.Z - Lobby->Bounds.Min.Z - 98) < 100;
}

void AEWardHospitalGameplay::UpdateArrivalBarrier29(float D)
{
    if (!ArrivalBarrierPivot) return;
    GateBlend = FMath::FInterpConstantTo(GateBlend, ArrivalStage >= 3 ? 1.f : 0.f, D, .6f);
    ArrivalBarrierPivot->SetRelativeRotation(FRotator(-82 * GateBlend, 0, 0));
}

void AEWardHospitalGameplay::PoseArrivalDrive29(float D)
{
    const float T = ArrivalClock / DriveDuration53;
    const float Approach = 1.f - FMath::Square(1.f - T);
    const FVector At = FMath::Lerp(Vector(Array(ArrivalData, TEXT("car_from"))),
        Vector(Array(ArrivalData, TEXT("car_to"))), Approach) * 100;
    ArrivalCar->SetActorLocation(At);
    VehiclePitch31 = -6.f + .38f * FMath::Sin(ArrivalClock * 4.f) * (1.f - T);
    VehicleYaw31 = 3.5f * FMath::Sin(T * PI);
    Seated31 = true;
    TickVehicle31(D);
}

void AEWardHospitalGameplay::ApplyCoopArrival29(int Stage, float DriveClock)
{
    if (ArrivalStage == Stage)
    {
        if (Stage == 0) ArrivalClock = FMath::Max(ArrivalClock, DriveClock);
        return;
    }
    const bool WasDriving = ArrivalStage == 0 || ArrivalStage == 1;
    ArrivalStage = Stage;
    ArrivalClock = FMath::Clamp(DriveClock, 0.f, DriveDuration53);
    ApplyArrivalState();
    auto* Hero = Mode->Hero();
    if (Stage == 0 || Stage == 1)
    {
        if (Stage == 0 && !IsValid(DriveAudio53))
            DriveAudio53 = WardHomeAssets::Play(this, TEXT("drive"), .55f * Mode->Setting(TEXT("effects"), .8));
        if (Stage == 1)
        {
            if (IsValid(DriveAudio53)) DriveAudio53->Stop();
            DriveAudio53 = nullptr;
            ArrivalClock = DriveDuration53;
        }
        PoseArrivalDrive29(0);
    }
    else if (WasDriving)
    {
        if (IsValid(DriveAudio53)) DriveAudio53->Stop();
        DriveAudio53 = nullptr;
        Seated31 = false;
        Hero->SetActorHiddenInGame(false);
        Hero->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Hero->MotionInitialized52 = false;
        if (auto* PC = Cast<APlayerController>(Hero->Controller))
        {
            PC->SetViewTargetWithBlend(Hero, .35f);
            PC->SetControlRotation(FRotator(-5, 65, 0));
        }
    }
}

void AEWardHospitalGameplay::TickCoopArrival29(float D)
{
    if (HomeStage52 >= 0)
    {
        TickHome52(D, true);
        return;
    }
    if (ArrivalStage < 0) return;
    UpdateArrivalBarrier29(D);
    if (ArrivalStage == 0 || ArrivalStage == 1)
    {
        if (ArrivalStage == 0) ArrivalClock = FMath::Min(ArrivalClock + D, DriveDuration53);
        PoseArrivalDrive29(D);
    }
    else
        TickVehicle31(D);
}
void AEWardHospitalGameplay::FinishCheckpoint69()
{
    if (ArrivalStage != 2) return;
    ArrivalStage = 3;
    Mode->Flag(TEXT("old_lab_accident29"), true);
    Mode->Journal.AddUnique(String(ArrivalData, TEXT("checkpoint_record")));
    Mode->Notify(TEXT("Лаборатория закрыта после давней аварии. Проход к приёмной открыт."), 5);
    Mode->Cue(TEXT("isolation_relay"), Vector(Array(ArrivalData, TEXT("checkpoint"))) * 100, .25f);
    ApplyArrivalState();
    Mode->AutoSave();
}
void AEWardHospitalGameplay::TickArrival(float D)
{
    if(ArrivalStage<0||!Mode||!Mode->Hero()||!Mode->CampaignStarted||
        (!Mode->Screen.IsEmpty()&&!WardOnline80::PlayingMap(*Mode)))return;
    auto* H=Mode->Hero();
    if (ArrivalStage != 0) TickVehicle31(D);
    UpdateArrivalBarrier29(D);
    if(ArrivalCinematic())
    {
        H->GetCharacterMovement()->DisableMovement();H->SetActorHiddenInGame(true);H->GetCharacterMovement()->StopMovementImmediately();
        if(Seated31&&ArrivalStage>=2)return;
        if (ArrivalStage == 0)
        {
            if (ArrivalClock == 0.f && D > 0.f)
                DriveAudio53 = WardHomeAssets::Play(this, TEXT("drive"), .55f*Mode->Setting(TEXT("effects"), .8));
            ArrivalClock = FMath::Min(ArrivalClock+D, DriveDuration53);
            PoseArrivalDrive29(D);
            if (ArrivalClock >= DriveDuration53)
            {
                ArrivalStage = 1;
                if (IsValid(DriveAudio53)) DriveAudio53->Stop();
                DriveAudio53 = nullptr;
                Mode->Cue(TEXT("scrape"), ArrivalCar->GetActorLocation(), .14f);
                Mode->Notify(TEXT("Ты приехал в больницу № 333. F — выйти из машины."), 8);
            }
        }
        else if (!Seated31)
        {
            Seated31 = true;
            VehiclePitch31 = -6.f;
            TickVehicle31(0.f);
        }
        return;
    }
    if(ArrivalStage==3)
    {
        const FVector At=H->GetActorLocation()/100;const auto& R=Array(ArrivalData,TEXT("entry_rect"));
        const bool InEntry = R.Num()==4&&At.X>R[0]->AsNumber()&&At.X<R[0]->AsNumber()+R[2]->AsNumber()&&At.Y>R[1]->AsNumber()&&At.Y<R[1]->AsNumber()+R[3]->AsNumber()&&FMath::Abs(At.Z-.98)<1;
        const bool Online = WardOnline80::PlayingMap(*Mode);
        EntranceWaiting29 = Online ? WardOnline80::PlayersOutsideEntrance(*Mode) : 0;
        if(Online ? EntranceWaiting29 == 0 : InEntry)
        {
            ArrivalStage=4;EntryClock=0;Mode->Flag(TEXT("arrival_isolated29"),true);Mode->Cue(TEXT("door"),FVector(5000,-2600,100),.25f);Mode->Cue(TEXT("radio"),H->GetActorLocation(),.18f);Mode->Notify(TEXT("За спиной срабатывают замки."), 3);ApplyArrivalState();
        }
    }
    else if(ArrivalStage==4)
    {
        EntryClock+=D;
        if(EntryClock>=Number(ArrivalData,TEXT("entry_seconds"),5))
        {
            ArrivalStage=5;Mode->Completed.Add(TEXT("SCENE_01_02"));Mode->Values.Add(TEXT("hospital_step"),2);const FString Message=String(ArrivalData,TEXT("doctor_message"));Mode->Journal.AddUnique(TEXT("Врач по внутренней связи: ")+Message);if(!NarrativeEnabled35)Mode->Notify(Message,10);Mode->AutoSave();Refresh();ApplyArrivalState();if(NarrativeEnabled35)BeginNarrative35(TEXT("SCENE_01_02"),true);
        }
    }
}
Json AEWardHospitalGameplay::ArrivalSnapshot()const
{
    Json S=MakeShared<FJsonObject>();S->SetNumberField(TEXT("home_stage52"),HomeStage52);S->SetNumberField(TEXT("home_line52"),HomeLine52);S->SetNumberField(TEXT("version"),31);S->SetNumberField(TEXT("stage"),ArrivalStage);S->SetNumberField(TEXT("drive_clock"),ArrivalClock);S->SetNumberField(TEXT("entry_clock"),EntryClock);S->SetNumberField(TEXT("gate_blend"),GateBlend);Json Doors=MakeShared<FJsonObject>();for(const auto& P:VehicleTarget31)Doors->SetNumberField(P.Key,P.Value);S->SetObjectField(TEXT("vehicle_doors31"),Doors);return S;
}
void AEWardHospitalGameplay::RestoreArrival(Json S)
{
    // Old saves retain their original location and progress, without replaying the intro.
    StopHome52();
    if (IsValid(DriveAudio53)) DriveAudio53->Stop();
    DriveAudio53 = nullptr;
    const int HomeStage=int(Number(S,TEXT("home_stage52"),-1));
    if(HomeStage>=0&&HomeStage<=2){BuildHome52();HomeStage52=HomeStage;HomeLine52=FMath::Clamp(int(Number(S,TEXT("home_line52"))),0,HomeLines52().Num()-1);SetHomeVisible53(true);HomeRing52=.3f;}
    ArrivalStage=S->HasField(TEXT("stage"))?FMath::Clamp(int(Number(S,TEXT("stage"))),-1,5):-1;
    Seated31=false;VehicleAutoClose31=0;
    for(auto& P:VehicleOpen31){P.Value=FMath::Clamp(float(Number(Object(S,TEXT("vehicle_doors31")),P.Key)),0.f,1.f);VehicleTarget31.Add(P.Key,P.Value);}
    ArrivalClock=FMath::Clamp(float(Number(S,TEXT("drive_clock"))),0.f,DriveDuration53);EntryClock=FMath::Clamp(float(Number(S,TEXT("entry_clock"))),0.f,5.f);GateBlend=FMath::Clamp(float(Number(S,TEXT("gate_blend"))),0.f,1.f);ApplyArrivalState();
}
