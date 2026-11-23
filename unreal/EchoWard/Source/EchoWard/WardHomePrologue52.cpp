#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardGameplay76.h"
#include "WardBookWorld.h"
#include "WardEchoAssets.h"
#include "WardHomeAssets53.h"
#include "WardOnline80.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"

using namespace WardData;

namespace
{
    const FVector HomeOrigin(-80000, -80000, 0);
    const FVector FrontDoor(0, -350, 0);

    void PlayHomeLine(AEWardHospitalGameplay& Scene)
    {
        if (IsValid(Scene.HomeAudio53)) Scene.HomeAudio53->Stop();
        const FString Clip = FString::Printf(TEXT("line%d"), Scene.HomeLine52);
        Scene.HomeAudio53 = WardHomeAssets::Play(&Scene, *Clip, .82f*Scene.Mode->Setting(TEXT("effects"), .8));
        Scene.HomeSubtitle69 = Scene.HomeLines52()[Scene.HomeLine52];
        const auto* Sound = IsValid(Scene.HomeAudio53) ? Scene.HomeAudio53->GetSound() : nullptr;
        const float ReadTime = FMath::Clamp(Scene.HomeSubtitle69.Len() / 16.f, 1.4f, 10.f);
        Scene.HomeLineTime69 = Sound ? FMath::Max(ReadTime, Sound->GetDuration() + .25f) : ReadTime;
    }
}

TArray<FString> AEWardHospitalGameplay::HomeLines52() const
{
    return {
        TEXT("Бека: Алло?"),
        TEXT("Аружан: Бека, мне нужна твоя помощь. Приезжай к приёмной больницы № 333."),
        TEXT("Бека: Что случилось? Ты сейчас там?"),
        TEXT("Аружан: По телефону не объясню. Пожалуйста, не откладывай до завтра.")
    };
}

void AEWardHospitalGameplay::BuildHome52()
{
    if (HomeActor52) return;
    auto* House = GetWorld()->SpawnActor<AActor>();
    HomeActor52 = House;
    auto* Root = NewObject<USceneComponent>(House);
    House->SetRootComponent(Root);
    Root->RegisterComponent();
    House->SetActorLocation(HomeOrigin);

    auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Campaign/BookWorld/SM_BookCube.SM_BookCube"));
    auto* Parent = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Campaign/EchoCustom/Materials/M_Real23_OPAQUE.M_Real23_OPAQUE"));
    const auto Library = Read(FPaths::ProjectContentDir() / TEXT("CampaignData/Intro53/home_meshes.json"));
    TMap<FString, UStaticMesh*> Meshes;
    for (const TCHAR* Name : {TEXT("wall"), TEXT("window"), TEXT("door"), TEXT("boards"), TEXT("sofa"),
                             TEXT("bookcase"), TEXT("curtain"), TEXT("lamp"), TEXT("table"), TEXT("phone")})
    {
        if (auto* Mesh = WardHomeAssets::BuildMesh(House, Library, Name)) Meshes.Add(Name, Mesh);
    }
    HomeImportedMeshes53 = Meshes.Num();
    UE_LOG(LogTemp, Display, TEXT("HOME53 built %d downloaded architecture and furniture meshes"), HomeImportedMeshes53);

    auto Material = [&](FLinearColor Color, FLinearColor Emission = FLinearColor::Black)
    {
        auto* Mat = UMaterialInstanceDynamic::Create(Parent, House);
        Mat->SetVectorParameterValue(TEXT("Tint"), Color);
        Mat->SetVectorParameterValue(TEXT("Emission"), Emission);
        Mat->SetScalarParameterValue(TEXT("Roughness"), .8f);
        Mat->SetScalarParameterValue(TEXT("Metallic"), 0.f);
        return Mat;
    };
    auto* Plaster = Material({.41f,.35f,.27f});
    auto* Wood = Material({.16f,.085f,.045f});
    auto* Dark = Material({.02f,.027f,.025f});
    auto* Iron = Material({.062f,.076f,.075f});
    auto* Key = Material({.42f,.39f,.30f});
    auto* Rug = Material({.12f,.20f,.18f});
    auto* GlassLight = Material({.18f,.13f,.075f}, {.55f,.26f,.07f});

    auto Place = [&](UStaticMesh* Mesh, FVector At, FVector Scale, FRotator Rotation)
    {
        auto* Component = NewObject<UStaticMeshComponent>(House);
        Component->SetupAttachment(Root);
        Component->SetStaticMesh(Mesh);
        Component->bDisallowNanite = true;
        Component->SetRelativeLocation(At);
        Component->SetRelativeScale3D(Scale);
        Component->SetRelativeRotation(Rotation);
        Component->SetCollisionProfileName(TEXT("NoCollision"));
        Component->SetLightingChannels(true, true, false);
        House->AddInstanceComponent(Component);
        Component->RegisterComponent();
        return Component;
    };
    auto Box = [&](FVector At, FVector Size, UMaterialInterface* Mat, bool Solid = false)
    {
        auto* Component = Place(Cube, At, Size / 100, FRotator::ZeroRotator);
        Component->SetMaterial(0, Mat);
        if (Solid) Component->SetCollisionProfileName(TEXT("BlockAll"));
        return Component;
    };
    auto Model = [&](const TCHAR* Name, FVector At, float Yaw = 0, FVector Scale = FVector(1))
    {
        if (auto* Mesh = Meshes.FindRef(Name)) return Place(Mesh, At, Scale, FRotator(0, Yaw, 0));
        return static_cast<UStaticMeshComponent*>(nullptr);
    };
    auto Wall = [&](const TCHAR* Name, FVector At, float Yaw = 0, float Width = 1.f)
    {
        const bool Imported = Model(Name, At, Yaw, FVector(Width, 1, 300.f / 270.f)) != nullptr;
        const FVector Size = Yaw == 0 ? FVector(200*Width, 18, 300) : FVector(18, 200*Width, 300);
        auto* Envelope = Box(At + FVector(0,0,150), Size, Plaster, true);
        Envelope->SetVisibility(!Imported);
        Envelope->SetCastShadow(!Imported);
    };
    auto Light = [&](FVector At, FLinearColor Color, float Power, float Radius, bool Shadows)
    {
        auto* Component = NewObject<UPointLightComponent>(House);
        Component->SetupAttachment(Root);
        Component->SetRelativeLocation(At);
        Component->SetIntensityUnits(ELightUnits::Lumens);
        Component->SetIntensity(Power);
        Component->SetAttenuationRadius(Radius);
        Component->SetLightColor(Color);
        Component->SetCastShadows(Shadows);
        Component->SetLightingChannels(true, true, false);
        House->AddInstanceComponent(Component);
        Component->RegisterComponent();
    };

    auto* Floor = Box({0,0,-10}, {820,720,20}, Wood, true);
    if (Meshes.Contains(TEXT("boards")))
    {
        Floor->SetVisibility(false);
        Floor->SetCastShadow(false);
        for (int X = -3; X <= 3; X += 2)
            for (int Y = -3; Y <= 3; Y += 2)
                Model(TEXT("boards"), FVector(X*100, Y*87.5f, -6.45f), 0, FVector(1,.875f,1));
    }
    Box({0,0,310}, {820,720,20}, Plaster, true);
    for (int Side : {-1,1})
        for (int Segment = -3; Segment <= 3; Segment += 2)
            Wall(TEXT("wall"), FVector(Side*400, Segment*87.5f, 0), 90, .875f);
    for (int X : {-300,-100,100,300})
        Wall((X == -100 || X == 100) ? TEXT("window") : TEXT("wall"), FVector(X,350,0));
    Wall(TEXT("door"), FrontDoor);
    for (int Side : {-1,1})
    {
        Wall(TEXT("wall"), FVector(Side*250,-350,0), 0, 1.5f);
        Wall(TEXT("wall"), FVector(Side*318,-195,0), 0, .82f);
        auto* Curtain = Model(TEXT("curtain"), FVector(Side*173,329,99), 0, FVector(.95f,1,1.12f));
        if (Curtain) HomeCurtains53.Add(Curtain);
    }
    Box({0,322,273},{388,3,3},Iron);

    Model(TEXT("sofa"), {-343,104,0}, 90);
    Box({-343,104,38},{75,227,76},Dark,true)->SetVisibility(false);
    Model(TEXT("bookcase"), {-270,315,0}, 180);
    Box({-270,315,84},{95,34,168},Dark,true)->SetVisibility(false);
    Box({-230,94,1.2},{258,263,1.4},Rug);
    WardEchoAssets::Attach(House, Root, 826, FBox(FVector(-265,72,1), FVector(-153,160,48)));
    Box({-209,116,20},{110,85,40},Dark,true)->SetVisibility(false);
    Box({0,-268,1.3},{112,103,1.6},Rug);
    if (!Model(TEXT("table"), {220,70,0}))
        Box({220,70,76},{114,70,8},Wood);
    Box({220,70,38},{112,69,76},Dark,true)->SetVisibility(false);
    Model(TEXT("lamp"), {255,86,80});
    WardEchoAssets::Attach(House, Root, 701, FBox(FVector(315,-275,0), FVector(370,-215,95)));
    for (int I = 0; I < 4; ++I)
        Box(FVector(-193,124,48+I*1.9), FVector(25+I,19,1.7), I%2 ? Wood : Key);

    if (!Model(TEXT("phone"), {215,64,80}, 180))
    {
        Box({215,64,83},{26,21,6},Dark);
        Box({215,73,90},{28,6,5},Iron);
    }

    Box({-235,-83,264},{38,38,4},GlassLight);
    Light({-235,-83,247},{1.f,.71f,.46f},560,820,true);
    Light({255,86,109},{1.f,.64f,.35f},75,330,false);
    Light({0,365,221},{.39f,.56f,1.f},290,680,false);
    Light({0,-278,247},{1.f,.67f,.38f},170,320,false);

    auto* Asphalt = Material({.034f,.043f,.054f});
    auto* Sidewalk = Material({.12f,.15f,.16f});
    auto* Facade = Material({.095f,.13f,.17f});
    Box({0,930,-18},{4200,1140,28},Asphalt);
    Box({0,510,-6},{4200,205,14},Sidewalk);
    Box({0,1500,-6},{4200,175,14},Sidewalk);
    for (int X = -1600; X <= 1600; X += 310)
        Box({double(X),1050,-3},{115,7,2},Key);
    for (int X = -2; X <= 2; ++X)
    {
        const FVector Front(X*640,1920,0);
        Box(Front+FVector(0,0,173),{470,375,346},Facade);
        Box(Front+FVector(0,-83,367),{502,255,24},Dark)->SetRelativeRotation(FRotator(0,0,-24));
        Box(Front+FVector(0,83,367),{502,255,24},Dark)->SetRelativeRotation(FRotator(0,0,24));
        for (int Side : {-1,1})
        {
            const FVector Window = Front + FVector(Side*126,-190,186);
            Box(Window,{115,9,128},Iron);
            Box(Window+FVector(0,-6,0),{96,4,104},GlassLight);
            Box(Window+FVector(0,-9,0),{5,4,105},Iron);
        }
        Box(Front+FVector(0,-193,106),{90,10,212},Dark);
        Box(Front+FVector(244,-435,221),{8,8,442},Iron);
        Box(Front+FVector(244,-449,435),{24,36,8},GlassLight);
        Light(Front+FVector(244,-455,416),{.78f,.66f,.41f},530,620,false);
    }
    auto* Twilight = Material({.018f,.027f,.072f},{.035f,.065f,.125f});
    Box({0,2480,1040},{5700,15,2100},Twilight)->SetCastShadow(false);
    HomeStreetCar53 = WardEchoAssets::Attach(House, Root, 920,
        FBox(FVector(-1950,805,0), FVector(-1520,990,143)));
    if (HomeStreetCar53) HomeStreetCarStart53 = HomeStreetCar53->GetRelativeLocation();

    StopHome52();
}

void AEWardHospitalGameplay::SetHomeVisible53(bool Visible)
{
    if (!HomeActor52) return;
    HomeActor52->SetActorHiddenInGame(!Visible);
    HomeActor52->SetActorEnableCollision(Visible);
    TArray<UPointLightComponent*> Lights;
    HomeActor52->GetComponents(Lights);
    for (auto* Light : Lights) Light->SetVisibility(Visible);
}

void AEWardHospitalGameplay::StopHome52()
{
    HomeStage52 = -1;
    HomeLine52 = 0;
    HomeLineTime69 = 0;
    HomeSubtitle69.Reset();
    HomeRing52 = 0;
    HomeStreetClock53 = 0;
    if (IsValid(HomeAudio53)) HomeAudio53->Stop();
    HomeAudio53 = nullptr;
    SetHomeVisible53(false);
}

void AEWardHospitalGameplay::StartHome52()
{
    ResetArrival();
    BuildHome52();
    HomeStage52 = 0;
    HomeRing52 = .35f;
    SetHomeVisible53(true);
    auto* Hero = Mode->Hero();
    Hero->SetActorHiddenInGame(false);
    Hero->Flashlight->SetVisibility(false);
    Hero->SetActorLocation(HomeOrigin + FVector(0,-130,98), false, nullptr, ETeleportType::TeleportPhysics);
    Hero->GetCharacterMovement()->StopMovementImmediately();
    Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    Hero->MotionInitialized52 = false;
    Mode->Options->SetBoolField(TEXT("inspection_bright"), false);
    Mode->BookWorld->SetBright(false);
    if (auto* PC = Cast<APlayerController>(Hero->Controller))
    {
        PC->SetViewTarget(Hero);
        PC->SetControlRotation(FRotator(-4,63,0));
    }
    Mode->Notify(TEXT("Звонит телефон на столе у окна."), 5);
}

FString AEWardHospitalGameplay::HomeObjective52() const
{
    if (HomeStage52 == 0) return TEXT("Дом Беки · подойди к телефону · F — ответить");
    if (HomeStage52 == 1) return TEXT("Звонок Аружан");
    return TEXT("Дом Беки · подойди к входной двери · E / F — выйти");
}

bool AEWardHospitalGameplay::HomeContains52(FVector Player) const
{
    const FVector Local = Player - HomeOrigin;
    return FMath::Abs(Local.X) < 400 && FMath::Abs(Local.Y) < 350 && Local.Z > 45 && Local.Z < 300;
}

void AEWardHospitalGameplay::ApplyCoopHome52(int Stage, const FString& Subtitle)
{
    if (Stage < 0)
    {
        if (HomeStage52 >= 0) StopHome52();
        return;
    }
    if (HomeStage52 < 0)
    {
        BuildHome52();
        SetHomeVisible53(true);
        HomeRing52 = .35f;
    }
    if (HomeStage52 != Stage || HomeSubtitle69 != Subtitle)
    {
        if (IsValid(HomeAudio53)) HomeAudio53->Stop();
        HomeAudio53 = nullptr;
        if (Stage == 1)
        {
            const int32 Line = HomeLines52().IndexOfByKey(Subtitle);
            if (Line != INDEX_NONE)
            {
                const FString Clip = FString::Printf(TEXT("line%d"), Line);
                HomeAudio53 = WardHomeAssets::Play(this, *Clip, .82f * Mode->Setting(TEXT("effects"), .8));
            }
        }
    }
    HomeStage52 = Stage;
    HomeSubtitle69 = Subtitle;
}

void AEWardHospitalGameplay::TickHome52(float Delta, bool Replica)
{
    if (!Mode->CampaignStarted || (!Mode->Screen.IsEmpty() && !WardOnline80::PlayingMap(*Mode))) return;
    HomeStreetClock53 += Delta;
    if (HomeStreetCar53)
    {
        const double Travel = FMath::Fmod(HomeStreetClock53*405., 3650.);
        HomeStreetCar53->SetRelativeLocation(HomeStreetCarStart53 + FVector(Travel,0,0));
    }
    for (int32 I = 0; I < HomeCurtains53.Num(); ++I)
        HomeCurtains53[I]->SetRelativeRotation(FRotator(0, 2.4f*FMath::Sin(HomeStreetClock53*1.08f+I), 0));

    if (HomeStage52 == 0 && (HomeRing52 -= Delta) <= 0)
    {
        HomeRing52 = 4.6f;
        if (IsValid(HomeAudio53)) HomeAudio53->Stop();
        HomeAudio53 = WardHomeAssets::Play(this, TEXT("ring"), .46f*Mode->Setting(TEXT("effects"), .8), &HomePhone52);
    }
    if (Replica || HomeStage52 != 1) return;
    if (HomeSubtitle69.IsEmpty())
    {
        PlayHomeLine(*this); // Resume the saved line after loading.
        return;
    }
    if ((HomeLineTime69 -= Delta) > 0.f) return;
    if (++HomeLine52 < HomeLines52().Num())
    {
        PlayHomeLine(*this);
        return;
    }
    HomeStage52 = 2;
    HomeLine52 = HomeLines52().Num() - 1;
    HomeSubtitle69.Reset();
    if (IsValid(HomeAudio53)) HomeAudio53->Stop();
    HomeAudio53 = nullptr;
    Mode->Completed.Add(TEXT("HOME_INVITATION52"));
    Mode->Journal.AddUnique(TEXT("Домашний звонок: Аружан попросила приехать к приёмной больницы № 333 и не откладывать до завтра."));
    Mode->Notify(TEXT("Связь прервана. Подойди к двери и нажми E."), 5);
}

bool AEWardHospitalGameplay::UseHome52(bool DoorKey)
{
    auto* Hero = Mode->Hero();
    if (!Hero || !Mode->Screen.IsEmpty()) return true;
    return UseHomeAt52(Hero->GetActorLocation(), DoorKey);
}

bool AEWardHospitalGameplay::UseHomeAt52(FVector Player, bool DoorKey)
{
    if (HomeStage52 < 0 || !HomeContains52(Player)) return false;
    if(!DoorKey&&HomeStage52==2&&FVector::Dist2D(Player,HomeOrigin+FrontDoor)<155.f)DoorKey=true;
    if (DoorKey)
    {
        if (HomeStage52 != 2)
        {
            if (HomeStage52 == 0) Mode->Notify(TEXT("Сначала ответь на телефон."), 3);
            return true;
        }
        if (FVector::Dist2D(Player, HomeOrigin + FrontDoor) > 155.f)
        {
            Mode->Notify(TEXT("Подойди к входной двери и нажми E."), 3);
            return true;
        }
        Mode->Cue(TEXT("door"), HomeOrigin + FrontDoor + FVector(0,0,110), .28f);
        Mode->Completed.Add(TEXT("HOME_CALL52"));
        StartArrival();
        return true;
    }

    if (HomeStage52 == 0)
    {
        if (FVector::Dist2D(Player, HomePhone52) > 175.f)
        {
            Mode->Notify(TEXT("Телефон звонит на столе у окна."), 3);
            return true;
        }
        HomeStage52 = 1;
        HomeLine52 = 0;
    }
    else
    {
        if (HomeStage52 == 2) Mode->Notify(HomeObjective52(), 4);
        return true;
    }
    Mode->Hero()->NoticeTime = 0;
    HomeRing52 = 0;
    PlayHomeLine(*this);
    return true;
}
