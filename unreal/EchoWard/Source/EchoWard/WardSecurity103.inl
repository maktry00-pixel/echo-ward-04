#include "WardSecurity103.h"
#include "WardBookWorld.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"

namespace WardSecurity103
{
namespace
{
constexpr int32 Cameras = 3;
constexpr int32 FramesPerCamera = 12;
constexpr double ArchiveSeconds = 12.;
const FIntPoint ArchiveSize(360,202);
const FIntPoint LiveSize(640,360);

struct FChannel
{
    FString Name, Portal;
    FVector At, Focus;
};

struct FFrame
{
    TStrongObjectPtr<UTextureRenderTarget2D> Picture;
    double Time = -1;
    FString Hint;
    bool DoorOpen = false;
    bool DoorMoved = false;
    bool Creature = false;
};

struct FCameraRecorder
{
    TWeakObjectPtr<AEWardBookWorld> World;
    TWeakObjectPtr<AActor> Stage;
    TWeakObjectPtr<USceneCaptureComponent2D> Capture;
    FChannel Channels[Cameras];
    FFrame Archive[Cameras][FramesPerCamera];
    int32 WriteIndex[Cameras] = {};
    FFrame Live;
    double NextArchive = 0;
    double NextLive = 0;
    int32 RecordChannel = 0;
    int32 Selected = 0;
    double Playback = -1;
    bool Playing = false;
};

TMap<TWeakObjectPtr<AEWardCampaignMode>, TSharedPtr<FCameraRecorder>> Recorders;

const FWardBookRoom3D* SecurityRoom(const AEWardBookWorld& World)
{
    return World.Rooms.FindByPredicate([](const auto& Room){return Room.Id==TEXT("F101");});
}

void ApplyCameraSettings(USceneCaptureComponent2D& Capture)
{
    Capture.bCaptureEveryFrame=false;
    Capture.bCaptureOnMovement=false;
    Capture.CaptureSource=SCS_FinalColorLDR;
    Capture.FOVAngle=76;
    Capture.LODDistanceFactor=2;
    Capture.MaxViewDistanceOverride=2100;
    Capture.ShowFlags.SetMotionBlur(false);
    Capture.ShowFlags.SetBloom(false);
    Capture.ShowFlags.SetVolumetricFog(false);
    Capture.ShowFlags.SetAmbientOcclusion(false);
    Capture.ShowFlags.SetScreenSpaceReflections(false);
    Capture.ShowFlags.SetTemporalAA(false);
    auto& Post=Capture.PostProcessSettings;
    Post.bOverride_AutoExposureMethod=true;
    Post.AutoExposureMethod=AEM_Manual;
    Post.bOverride_AutoExposureBias=true;
    Post.AutoExposureBias=1.f;
    Post.bOverride_ColorSaturation=true;
    Post.ColorSaturation=FVector4(.55,.62,.60,1);
    Post.bOverride_VignetteIntensity=true;
    Post.VignetteIntensity=.28f;
    Post.bOverride_FilmGrainIntensity=true;
    Post.FilmGrainIntensity=.29f;
}

TSharedPtr<FCameraRecorder> GetRecorder(AEWardCampaignMode& Game, bool Create = false)
{
    auto* Book=Game.BookWorld.Get();
    if(!Book) return nullptr;
    if(const auto* Previous=Recorders.Find(&Game);Previous&&(*Previous)->World.Get()==Book&&(*Previous)->Capture.IsValid()) return *Previous;
    if(!Create) return nullptr;
    for(auto It=Recorders.CreateIterator();It;++It) if(!It.Key().IsValid()) It.RemoveCurrent();
    Reset(Game);
    auto Recorder=MakeShared<FCameraRecorder>();
    Recorder->World=Book;
    auto* Actor=Game.GetWorld()->SpawnActor<AActor>();
    if(!Actor) return nullptr;
    Actor->SetFlags(RF_Transient);
    auto* Capture=NewObject<USceneCaptureComponent2D>(Actor);
    Actor->SetRootComponent(Capture);
    ApplyCameraSettings(*Capture);
    Capture->RegisterComponent();
    Recorder->Stage=Actor;
    Recorder->Capture=Capture;

    const auto Install=[&](int32 Index,const TCHAR* Zone,const TCHAR* Label,FVector From,FVector To)
    {
        auto& Channel=Recorder->Channels[Index];
        Channel={Label,Zone,From,To};
        if(const auto* Portal=Book->Portals.FindByPredicate([&](const auto& Door){return Door.Id==Zone;}))
        {
            Channel.At+=Portal->At;
            Channel.Focus+=Portal->At;
        }
    };
    Install(0,TEXT("N01"),TEXT("01  Вестибюль"),FVector(-340,640,305),FVector(0,8,138));
    Install(1,TEXT("F101"),TEXT("02  Коридор у поста"),FVector(510,-290,314),FVector(-85,-25,139));
    Install(2,TEXT("F202"),TEXT("03  Лечебный этаж"),FVector(555,318,307),FVector(20,20,119));
    Recorders.Add(&Game,Recorder);
    return Recorder;
}

bool CanUse(AEWardCampaignMode& Game)
{
    if(!Game.BookWorld || !Game.Hero() || Game.Hero()->Dead || Game.BookWorld->FurnitureHiding || Game.HidingBed ||
        !Game.CampaignStarted || Game.TitleView || (Game.HospitalStory&&(Game.HospitalStory->HomeStage52>=0||Game.HospitalStory->ArrivalCinematic()))) return false;
    const auto* Room=SecurityRoom(*Game.BookWorld);
    const FVector Player=Game.Hero()->GetActorLocation();
    if(!Room||!Room->Bounds.IsInside(Player)||FVector::DistSquared2D(Player,Terminal(*Game.BookWorld))>FMath::Square(185.)) return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(SecurityTerminal103),false,Game.Hero());
    FHitResult Hit;
    return !Game.GetWorld()->LineTraceSingleByChannel(Hit,Player+FVector(0,0,48),Terminal(*Game.BookWorld),ECC_Visibility,Q);
}

bool CanSee(AEWardCampaignMode& Game, const FChannel& Channel, FVector At, const AActor* Target)
{
    const FVector Direction = At-Channel.At;
    const FRotationMatrix Basis((Channel.Focus-Channel.At).Rotation());
    const FVector Local=Basis.GetTransposed().TransformVector(Direction);
    const double Limit=FMath::Tan(FMath::DegreesToRadians(38.));
    if(Local.X<22||Local.X>1950||FMath::Abs(Local.Y)>Local.X*Limit||FMath::Abs(Local.Z)>Local.X*Limit*.5625) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SecurityVisibility103),false);
    FHitResult Hit;
    return !Game.GetWorld()->LineTraceSingleByChannel(Hit,Channel.At,At,ECC_Visibility,Query) || Hit.GetActor()==Target;
}

void Snapshot(AEWardCampaignMode& Game, FCameraRecorder& Recorder,int32 Index,FFrame& Frame,FIntPoint Size,double Now)
{
    auto* Capture=Recorder.Capture.Get();
    if(!Capture||!FApp::CanEverRender()) return;
    if(!Frame.Picture.IsValid())
    {
        Frame.Picture.Reset(NewObject<UTextureRenderTarget2D>(GetTransientPackage()));
        Frame.Picture->ClearColor=FLinearColor(.009,.013,.014,1);
        Frame.Picture->InitCustomFormat(Size.X,Size.Y,PF_B8G8R8A8,false);
        Frame.Picture->UpdateResourceImmediate(true);
    }
    const auto& Channel=Recorder.Channels[Index];
    Capture->SetWorldLocationAndRotation(Channel.At,(Channel.Focus-Channel.At).Rotation());
    Capture->TextureTarget=Frame.Picture.Get();
    Capture->CaptureScene();
    const auto PreviousSlot=(Recorder.WriteIndex[Index]+FramesPerCamera-1)%FramesPerCamera;
    const auto& Previous=Recorder.Archive[Index][PreviousSlot];
    const auto* Door=Game.BookWorld->Portals.FindByPredicate([&](const auto& P){return P.Id==Channel.Portal;});
    Frame.Time=Now;
    Frame.DoorOpen=Door&&Door->Open;
    Frame.DoorMoved=Previous.Time>=0&&Previous.Time<Now&&Frame.DoorOpen!=Previous.DoorOpen;
    Frame.Creature=false;
    Frame.Hint.Empty();
    if(Game.HospitalStory) for(const auto& Mob : Game.HospitalStory->Mobs)
    {
        if(!IsValid(Mob)||!Mob->Active||Mob->Dead||Mob->IsHidden()||
            FVector::DistSquared(Channel.At,Mob->GetActorLocation())>FMath::Square(2100.)) continue;
        if(!CanSee(Game,Channel,Mob->GetActorLocation()+FVector(0,0,23),Mob.Get())) continue;
        Frame.Creature=true;
        const double Side=FVector::DotProduct(Mob->GetVelocity(),(Channel.Focus-Channel.At).Rotation().RotateVector(FVector::RightVector));
        Frame.Hint=Side>28?TEXT("Существо уходит вправо"):Side<-28?TEXT("Существо уходит влево"):TEXT("Существо в кадре");
        break;
    }
    if(Frame.Hint.IsEmpty()&&Frame.DoorMoved) Frame.Hint=Frame.DoorOpen?TEXT("Дверь открылась"):TEXT("Дверь закрылась");
}

const FFrame* ReplayFrame(const FCameraRecorder& Recorder,double Now)
{
    const FFrame* Best=nullptr;
    for(const auto& Frame:Recorder.Archive[Recorder.Selected])
    {
        if(Frame.Time<0||Now-Frame.Time>ArchiveSeconds||!Frame.Picture.IsValid()) continue;
        if(!Best||FMath::Abs(Frame.Time-Recorder.Playback)<FMath::Abs(Best->Time-Recorder.Playback)) Best=&Frame;
    }
    return Best;
}

FText CleanText(const FString& Text) {return FText::FromString(Text);}
const FLinearColor Ink(.67,.80,.77), Dim(.34,.52,.49), Red(.82,.15,.14);
FSlateColorBrush ConsoleBack(FLinearColor(.003,.010,.010,.985)), ConsoleFrame(FLinearColor(.036,.067,.061)), MonitorBack(FLinearColor(.005,.012,.012));

class SSecurity103 final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSecurity103){} SLATE_ARGUMENT(AEWardCampaignMode*,Game) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Owner=Args._Game;
        auto Buttons=SNew(SHorizontalBox);
        const auto Camera=GetRecorder(*Args._Game);
        if(Camera) for(int32 Index=0;Index<Cameras;++Index)
        {
            Buttons->AddSlot().FillWidth(1).Padding(0,0,7,0)[SNew(SButton).ContentPadding(FMargin(12,9))
                .ButtonColorAndOpacity_Lambda([this,Index](){const auto Record=Recorder();return FSlateColor(Record&&Record->Selected==Index?FLinearColor(.39,.071,.064):FLinearColor(.075,.14,.13));})
                .OnClicked_Lambda([this,Index](){Select(Index);return FReply::Handled();})
                [SNew(STextBlock).Text(CleanText(Camera->Channels[Index].Name)).ColorAndOpacity(Ink).Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),16))]];
        }
        Buttons->AddSlot().AutoWidth()[SNew(SButton).ContentPadding(FMargin(11,9)).OnClicked_Lambda([this](){if(auto* G=Owner.Get())G->Show(TEXT(""));return FReply::Handled();})
            [SNew(STextBlock).Text(CleanText(TEXT("Закрыть  Esc"))).ColorAndOpacity(Ink)]];
        ChildSlot[SNew(SBorder).BorderImage(&ConsoleBack).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(22)
            [SNew(SBox).MaxDesiredWidth(990)
              [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Text(CleanText(TEXT("Пост охраны · Отделение 04"))).ColorAndOpacity(Ink).Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),26))]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)[Buttons]
                +SVerticalBox::Slot().FillHeight(1)[SNew(SBorder).BorderImage(&ConsoleFrame).Padding(3)
                    [SNew(SBorder).BorderImage(&MonitorBack).Padding(0)
                        [SNew(SOverlay)
                         +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SAssignNew(Picture,SImage).Image(&Feed)]]
                         +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(15,13)[SNew(STextBlock).Text_Lambda([this](){return CleanText(Status);}).ColorAndOpacity_Lambda([this](){return FSlateColor(Playback?Ink:Red);}).Font(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"),14))]
                         +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([this](){return CleanText(Empty);}).ColorAndOpacity(Dim).Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),17))]
                         +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(15,14)[SNew(STextBlock).Text_Lambda([this](){return CleanText(Incident);}).ColorAndOpacity(Ink).ShadowOffset(FVector2D(1,1))]
                        ]]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)[SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,7,0)[Tool(TEXT("На 2 сек назад"),-2)]
                    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,7,0)[SNew(SButton).ContentPadding(FMargin(13,8))
                        .OnClicked_Lambda([this](){if(const auto Record=Recorder()){if(Record->Playback<0)Record->Playback=Owner->GetWorld()->GetTimeSeconds()-6.;Record->Playing=!Record->Playing;}return FReply::Handled();})
                        [SNew(STextBlock).Text_Lambda([this](){const auto Record=Recorder();return CleanText(Record&&Record->Playing?TEXT("Пауза"):TEXT("Воспроизвести"));}).ColorAndOpacity(Ink)]]
                    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,7,0)[Tool(TEXT("На 2 сек вперёд"),2)]
                    +SHorizontalBox::Slot().AutoWidth()[Tool(TEXT("Прямой эфир"),0)]
                ]
              ]]];
    }

    virtual void Tick(const FGeometry& Geometry,double Time,float Delta) override
    {
        SCompoundWidget::Tick(Geometry,Time,Delta);
        const auto Record=Recorder();
        if(!Record||!Owner.IsValid()) return;
        const double Now=Owner->GetWorld()->GetTimeSeconds();
        if(Record->Playing)
        {
            Record->Playback=FMath::Min(Now,Record->Playback+Delta);
            if(Record->Playback>=Now-.05) Record->Playing=false;
        }
        Playback=Record->Playback>=0;
        const auto* Frame=Playback?ReplayFrame(*Record,Now):&Record->Live;
        const bool HasSignal=Frame&&Frame->Time>=0&&Frame->Picture.IsValid();
        Feed.SetResourceObject(HasSignal?Frame->Picture.Get():nullptr);
        Feed.ImageSize=FVector2D(896,504);
        Picture->SetVisibility(HasSignal?EVisibility::HitTestInvisible:EVisibility::Hidden);
        Status=Playback?(HasSignal?FString::Printf(TEXT("АРХИВ  ·  %.1f сек назад"),FMath::Max(0.,Now-Frame->Time)):TEXT("АРХИВ")):TEXT("●  ПРЯМОЙ ЭФИР");
        Empty=HasSignal?FString():Playback?TEXT("Запись ещё не поступила"):TEXT("Подключение к каналу…");
        Incident=HasSignal?Frame->Hint:FString();
    }

private:
    TSharedPtr<FCameraRecorder> Recorder() const {return Owner.IsValid()?GetRecorder(*Owner):nullptr;}
    void Select(int32 Index)
    {
        if(const auto Record=Recorder();Record&&Record->Selected!=Index)
        {Record->Selected=Index;Record->Live.Time=-1;Record->NextLive=0;}
    }
    TSharedRef<SWidget> Tool(const TCHAR* Title,float Direction)
    {
        return SNew(SButton).ContentPadding(FMargin(13,8)).OnClicked_Lambda([this,Direction]()
        {
            if(const auto Record=Recorder())
            {
                const double Now=Owner->GetWorld()->GetTimeSeconds();
                Record->Playing=false;
                if(Direction==0){Record->Playback=-1;Record->NextLive=0;}
                else Record->Playback=FMath::Clamp((Record->Playback<0?Now:Record->Playback)+Direction,Now-ArchiveSeconds,Now);
            }
            return FReply::Handled();
        })[SNew(STextBlock).Text(CleanText(Title)).ColorAndOpacity(Ink)];
    }
    TWeakObjectPtr<AEWardCampaignMode> Owner;
    TSharedPtr<SImage> Picture;
    FSlateBrush Feed;
    FString Status,Empty,Incident;
    bool Playback=false;
};
}

FVector Terminal(const AEWardBookWorld& World)
{
    if(const auto* Room=SecurityRoom(World)) return FVector(Room->Bounds.Max.X-51,Room->Bounds.GetCenter().Y+40,Room->Bounds.Min.Z+146);
    return FVector::ZeroVector;
}

bool Interact(AEWardCampaignMode& Game)
{
    if(!Game.Screen.IsEmpty()||!CanUse(Game)) return false;
    const auto Recorder=GetRecorder(Game,true);
    if(!Recorder) return false;
    Recorder->Playback=-1;Recorder->Playing=false;Recorder->NextLive=0;
    Game.Show(Page);
    return true;
}

void Tick(AEWardCampaignMode& Game,float Delta)
{
    if(Game.Screen==Page&&!CanUse(Game)){Game.Show(TEXT(""));return;}
    if(!Game.BookWorld||!Game.Hero()||!Game.CampaignStarted||Game.TitleView||Game.Won||Game.Hero()->Dead||
        (Game.HospitalStory&&(Game.HospitalStory->HomeStage52>=0||Game.HospitalStory->ArrivalCinematic()))||!FApp::CanEverRender()) return;
    const FVector Post=Terminal(*Game.BookWorld), At=Game.Hero()->GetActorLocation();
    if(Post.IsNearlyZero()||FVector::DistSquared2D(Post,At)>FMath::Square(7000.)||FMath::Abs(Post.Z-At.Z)>1450) return;
    if(Game.Screen.IsEmpty()&&Game.Hero()->NoticeTime<=0&&CanUse(Game))Game.Notify(TEXT("F — видеонаблюдение"),1.2f);
    const auto Recorder=GetRecorder(Game,true);
    if(!Recorder) return;
    const double Now=Game.GetWorld()->GetTimeSeconds();
    if(Now>=Recorder->NextArchive)
    {
        const int32 Index=Recorder->RecordChannel;
        auto& Slot=Recorder->WriteIndex[Index];
        Snapshot(Game,*Recorder,Index,Recorder->Archive[Index][Slot],ArchiveSize,Now);
        Slot=(Slot+1)%FramesPerCamera;Recorder->RecordChannel=(Index+1)%Cameras;
        Recorder->NextArchive=Now+.34;
    }
    else if(Game.Screen==Page&&Recorder->Playback<0&&Now>=Recorder->NextLive)
    {
        Snapshot(Game,*Recorder,Recorder->Selected,Recorder->Live,LiveSize,Now);
        Recorder->NextLive=Now+.20;
    }
}

void Reset(AEWardCampaignMode& Game)
{
    if(const auto* State=Recorders.Find(&Game))
    {
        if(auto* Actor=(*State)->Stage.Get()) Actor->Destroy();
        Recorders.Remove(&Game);
    }
}

TSharedRef<SWidget> Build(AEWardCampaignMode& Game) {return SNew(SSecurity103).Game(&Game);}

void Assess(AEWardCampaignMode& Game,float Delta)
{
    static int32 Stage=0;
    static float Clock=0;
    static TArray<TSharedPtr<FJsonValue>> Checks;
    const auto Check=[&](const TCHAR* Name,bool Pass)
    {
        const auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Pass);
        Checks.Add(MakeShared<FJsonValueObject>(Row));
        UE_LOG(LogTemp,Display,TEXT("SECURITY103 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Name);
    };
    if(!Game.BookWorld||!Game.HospitalStory||!Game.Hero()||!Game.Interface)return;
    Clock+=Delta;
    auto& Book=*Game.BookWorld;
    const auto* Room=SecurityRoom(Book);
    auto* Door=Book.Portals.FindByPredicate([](const auto& P){return P.Id==TEXT("F202");});
    auto* PC=Game.GetWorld()->GetFirstPlayerController();
    if(Stage==0)
    {
        Game.TitleView=false;Game.CampaignStarted=true;Game.Admin=false;
        Game.HospitalStory->HomeStage52=-1;Game.HospitalStory->ArrivalStage=-1;Game.HospitalStory->FreezeAI=true;
        Game.Show(TEXT(""));
        Game.Hero()->SetActorLocation(Book.Start());
        Check(TEXT("console_cannot_open_remotely"),!Interact(Game));
        if(Room&&Door&&PC)
        {
            FVector Player=Terminal(Book)-FVector(111,0,48);
            Player.Z=Room->Bounds.Min.Z+98;
            Game.Hero()->SetActorLocation(Player);PC->SetControlRotation(FRotator(0,0,0));
            Check(TEXT("guard_post_opens_live_console_without_pausing_game"),Interact(Game)&&Game.Screen==Page&&!PC->IsPaused());
        }
        const auto Recorder=GetRecorder(Game);
        if(!Recorder||!Door){Check(TEXT("camera_fixtures_available"),false);Stage=3;}
        else
        {
            Recorder->Selected=2;
            Door->Open=false;Book.RefreshPortalVisual(*Door);
            Snapshot(Game,*Recorder,2,Recorder->Archive[2][0],ArchiveSize,Game.GetWorld()->GetTimeSeconds());
            Recorder->WriteIndex[2]=1;
            Stage=1;
        }
        Clock=0;
    }
    else if(Stage==1&&Clock>.6f)
    {
        const auto Recorder=GetRecorder(Game);
        const FFrame& Earlier=Recorder->Archive[2][0];
        TArray<FColor> Pixels;
        const bool Recorded=Earlier.Picture.IsValid()&&Earlier.Picture->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
        int32 Low=255,High=0;
        for(const auto& Pixel:Pixels)
        {
            const int32 Value=(int(Pixel.R)+Pixel.G+Pixel.B)/3;
            Low=FMath::Min(Low,Value);High=FMath::Max(High,Value);
        }
        UE_LOG(LogTemp,Display,TEXT("SECURITY103 archive pixels=%d range=%d..%d"),Pixels.Num(),Low,High);
        Check(TEXT("archive_contains_scene_pixels_not_placeholder"),Recorded&&Pixels.Num()==ArchiveSize.X*ArchiveSize.Y&&High-Low>14);
        Check(TEXT("recording_preserves_closed_door_state"),!Earlier.DoorOpen&&Earlier.Time>=0);
        Door->Open=true;Book.RefreshPortalVisual(*Door);
        if(auto* Found=Game.HospitalStory->Mobs.FindByPredicate([](const auto& Mob){return Mob->Species==TEXT("patients");}))
        {
            auto& Mob=**Found;
            Mob.Active=true;Mob.EnsureVisual61();Mob.SetActorHiddenInGame(false);
            const auto& Channel=Recorder->Channels[2];
            FVector InView=FMath::Lerp(Channel.At,Channel.Focus,.56);
            InView.Z=Door->At.Z+91;
            Mob.SetActorLocation(InView,false);
            Mob.GetCharacterMovement()->Velocity=FVector(0,110,0);
        }
        const double Now=Game.GetWorld()->GetTimeSeconds();
        Snapshot(Game,*Recorder,2,Recorder->Archive[2][1],ArchiveSize,Now);
        Snapshot(Game,*Recorder,2,Recorder->Live,LiveSize,Now);
        Recorder->WriteIndex[2]=2;
        Check(TEXT("new_frame_marks_opened_door_and_visible_creature"),Recorder->Archive[2][1].DoorMoved&&Recorder->Archive[2][1].DoorOpen&&Recorder->Archive[2][1].Creature);
        Check(TEXT("live_capture_does_not_overwrite_archive"),Recorder->Live.Picture.Get()!=Earlier.Picture.Get()&&Earlier.Time<Recorder->Live.Time&&!Earlier.DoorOpen);
        Recorder->Playback=Earlier.Time;
        Check(TEXT("rewind_retrieves_previous_view"),ReplayFrame(*Recorder,Now)==&Earlier);
        Recorder->Playback=Now;
        Check(TEXT("archive_switches_to_the_more_recent_frame"),ReplayFrame(*Recorder,Now)==&Recorder->Archive[2][1]);
        Recorder->Selected=0;
        Snapshot(Game,*Recorder,0,Recorder->Archive[0][0],ArchiveSize,Now);
        Check(TEXT("switching_channels_shows_a_separate_camera_and_buffer"),!Recorder->Channels[0].At.Equals(Recorder->Channels[2].At,1)&&ReplayFrame(*Recorder,Now)==&Recorder->Archive[0][0]);
        Recorder->Selected=2;
        Recorder->Playback=Now;
        Stage=2;Clock=0;
    }
    else if(Stage==2&&Clock>.65f)
    {
        IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Prototype")),true);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/security103_console.png"),true,false);
        Stage=3;Clock=0;
    }
    else if(Stage==3&&Clock>.35f)
    {
        if(Game.Screen==Page)
        {
            Game.Hero()->SetActorLocation(Book.Start());
            Tick(Game,Delta);
            Check(TEXT("moving_away_closes_console"),Game.Screen.IsEmpty());
        }
        bool Passed=true;
        for(const auto& Value:Checks)Passed&=WardData::Boolean(Value->AsObject(),TEXT("passed"));
        const auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("success"),Passed);Report->SetArrayField(TEXT("checks"),Checks);
        WardData::Write(FPaths::ProjectSavedDir()/TEXT("Prototype/security103.json"),Report);
        Stage=4;FPlatformMisc::RequestExit(false);
    }
}
}
