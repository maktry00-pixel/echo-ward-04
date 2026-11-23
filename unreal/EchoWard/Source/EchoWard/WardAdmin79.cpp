#include "WardAdmin79.h"
#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardBookWorld.h"
#include "WardHospitalGameplay.h"
#include "WardMonster59.h"
#include "WardMonsterVisual59.h"
#include "WardAudio68.h"
#include "WardGraphics.h"
#include "WardTitleScene.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "Containers/Ticker.h"

using namespace WardData;
namespace WardAdmin79
{
namespace
{
constexpr auto Terminal=TEXT("admin_terminal79");
constexpr auto Tools=TEXT("admin_tools79");
constexpr auto Chapters=TEXT("admin_chapters79");
constexpr auto Monsters=TEXT("admin_monsters79");
constexpr auto TestRun=TEXT("admin_chapter_run79");
constexpr int MaxPreviews=4;
struct FSession
{
    bool Auth=false,Focus=false;
    FString Return,Feedback,LastSave;
    TArray<FString> Lines;
    TWeakPtr<SEditableTextBox> Entry;
    TWeakPtr<SScrollBox> Output;
    TWeakObjectPtr<UAudioComponent> Sound;
    TArray<TWeakObjectPtr<UEWardMonsterVisual59>> Previews;
    TSharedPtr<IInputProcessor> Input;
    Json Original;
    float Age=0;
};
TMap<TWeakObjectPtr<AEWardCampaignMode>,FSession> Sessions;
FSession& Session(AEWardCampaignMode& G){return Sessions.FindOrAdd(&G);}
bool Authorized(AEWardCampaignMode& G){return G.Admin||Session(G).Auth;}
bool Playing(AEWardCampaignMode& G){return G.CampaignStarted&&!G.TitleView&&G.Hero()&&!G.Hero()->Dead&&!G.PendingRespawn&&!G.Won;}
bool SafeCamera(AEWardCampaignMode& G){return Playing(G)&&(!G.HospitalStory||!G.HospitalStory->ArrivalCinematic());}
const FLinearColor Ice(.71,.87,.87),Dim(.50,.66,.67),Red(.93,.27,.24);
const FSlateColorBrush Backdrop(FLinearColor(.008,.015,.018,.97)),Divider(FLinearColor(.15,.32,.34,1));
const FButtonStyle ButtonStyle=[]{
    FButtonStyle B;
    return B.SetNormal(FSlateColorBrush(FLinearColor(.026,.065,.074,1)))
        .SetHovered(FSlateColorBrush(FLinearColor(.27,.047,.046,1)))
        .SetPressed(FSlateColorBrush(FLinearColor(.42,.055,.05,1)));
}();
const FString Help=TEXT(
    "chapters  /  chapter <1–19>  /  chapter back — главы и возврат\n"
    "monsters  /  spawn <ID>  /  spawn clear — модели монстров\n"
    "items  /  docs  /  hold <ID>  /  read <NOTE_ID>  /  release — предметы и записки\n"
    "sounds  /  sound <ID>  /  sound stop — прослушивание\n"
    "admin on  /  fly  /  player — свободная камера и возврат к герою\n"
    "agent on|off  /  bright on|off  /  fps  /  graphics — диагностика\n"
    "panel  /  catalog  /  audio — открыть соответствующий раздел\n"
    "status  /  help  /  logout  /  close");

void Print(AEWardCampaignMode& G,const FString& Message)
{
    auto& S=Session(G);S.Lines.Add(Message);
    if(S.Lines.Num()>24)S.Lines.RemoveAt(0,S.Lines.Num()-24);
    if(auto Scroll=S.Output.Pin())Scroll->ScrollToEnd();
}
void Feedback(AEWardCampaignMode& G,const FString& Message)
{
    if(G.Screen==Terminal)Print(G,Message);
    else if(Page(G.Screen))Session(G).Feedback=Message;
    else G.Notify(Message,5);
}
void StopSound(FSession& S)
{
    if(auto* Audio=S.Sound.Get()){Audio->Stop();Audio->DestroyComponent();}
    S.Sound.Reset();
}
int Clear(AEWardCampaignMode& G)
{
    auto& P=Session(G).Previews;int Count=0;
    for(auto M:P)if(M.IsValid()&&IsValid(M->GetOwner())){M->GetOwner()->Destroy();++Count;}
    P.Reset();return Count;
}
void Close(AEWardCampaignMode& G)
{
    auto& S=Session(G);StopSound(S);
    FString Return=MoveTemp(S.Return);S.Focus=false;
    if(Return==Terminal)Return.Empty();
    G.Show(Return);
}
void Toggle(AEWardCampaignMode& G)
{
    if(G.Screen==Terminal){Close(G);return;}
    if(!G.Interface||(G.TitleScene&&G.TitleScene->Transitioning))return;
    auto& S=Session(G);S.Return=G.Screen;G.Show(Terminal);
}
class FKeys final:public IInputProcessor
{
public:
    explicit FKeys(AEWardCampaignMode& G):Mode(&G){}
    void Tick(float,FSlateApplication& Slate,TSharedRef<ICursor>)override
    {
        auto* G=Mode.Get();if(!G||G->Screen!=Terminal)return;
        auto& S=Session(*G);if(S.Focus)if(auto Entry=S.Entry.Pin())
        {S.Focus=false;Slate.SetKeyboardFocus(Entry,EFocusCause::SetDirectly);}
    }
    bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent& Event)override
    {
        auto* G=Mode.Get();if(!G)return false;
        const FKey Key=Event.GetKey();const FString Name=Key.GetFName().ToString();
        // Windows sends OEM_3 for the key below Esc. The key name/character
        // also covers the EN/RU mappings exposed by Slate.
        if(!Event.IsControlDown()&&!Event.IsAltDown()&&
            (Event.GetKeyCode()==0xC0||Key==EKeys::Tilde||Name==TEXT("ё")||Name==TEXT("Ё")||
                Event.GetCharacter()==0x451||Event.GetCharacter()==0x401))
        {if(!Event.IsRepeat())Toggle(*G);return true;}
        if(Event.IsRepeat())return false;
        if(G->Screen==Terminal&&Key==EKeys::Escape){Close(*G);return true;}
        if(Page(G->Screen)&&Key==EKeys::Escape){G->Command(TEXT("adm79_back"));return true;}
        return false;
    }
private:
    TWeakObjectPtr<AEWardCampaignMode> Mode;
};

Json Chapter(AEWardHospitalGameplay& S,int Ch)
{
    const auto& Rows=Array(S.Narrative35,TEXT("chapters"));
    return Rows.IsValidIndex(Ch-1)?Rows[Ch-1]->AsObject():Json();
}
FString ChapterLabel(AEWardHospitalGameplay& S,int Ch)
{
    return FString::Printf(TEXT("%02d  ·  %s"),Ch,*String(Chapter(S,Ch),TEXT("title")));
}
FString Expected(AEWardHospitalGameplay& S)
{
    const auto& Rows=Array(S.Data,TEXT("chapters"));
    const int Ch=S.Chapter()-1;
    if(!Rows.IsValidIndex(Ch))return {};
    const auto& Steps=Array(Rows[Ch]->AsObject(),TEXT("required"));
    const int Step=S.Mode->Values.FindRef(TEXT("hospital_step"));
    return Steps.IsValidIndex(Step)?Steps[Step]->AsString():FString();
}
bool ReplayPrefix(AEWardCampaignMode& G,int To,FString& Failed)
{
    auto& S=*G.HospitalStory;
    TGuardValue<bool> Fast(S.NarrativeTesting35,true),Frozen(S.FreezeAI,true);
    for(int Count=0;S.Chapter()<To&&Count<100;++Count)
    {
        Failed=Expected(S);
        Json Scene=S.NarrativeScenes35.FindRef(Failed);if(!Scene)return false;
        for(auto V:Array(Scene,TEXT("requires")))S.UseId(V->AsString());
        if(Failed==TEXT("SCENE_16_03"))
        {
            for(const TCHAR* Doc:{TEXT("NOTE_07_01"),TEXT("NOTE_07_04"),TEXT("NOTE_08_04"),TEXT("NOTE_04_02"),TEXT("NOTE_14_02"),TEXT("NOTE_13_02"),TEXT("NOTE_13_03")})S.UseId(Doc);
            S.PreparePlans35();
        }
        if(!S.BeginNarrative35(Failed)||S.ActiveScene35!=Failed)return false;
        const auto& Choices=Array(Scene,TEXT("choices"));
        if(!Choices.IsEmpty())S.NarrativeCommand35(TEXT("story35_choice_")+String(Choices[0]->AsObject(),TEXT("id")));
        for(int I=0;I<100&&!S.ActiveScene35.IsEmpty();++I)S.NarrativeCommand35(TEXT("story35_next"));
        if(!S.ActiveScene35.IsEmpty()||!G.Completed.Contains(Failed)||Expected(S)==Failed)return false;
    }
    return S.Chapter()==To&&G.Values.FindRef(TEXT("hospital_step"))==0;
}
bool Jump(AEWardCampaignMode& G,int To)
{
    auto* S=G.HospitalStory.Get();auto* H=G.Hero();
    auto* PC=H?Cast<APlayerController>(H->GetController()):nullptr;
    if(!S||!H||!PC||!Chapter(*S,To)||To<1||To>19)
    {Feedback(G,TEXT("Укажи номер главы от 1 до 19."));return false;}
    auto& State=Session(G);
    const bool WasTitle=G.TitleView||!G.CampaignStarted,WasAdmin=G.Admin;
    const FString OldPage=G.Screen,LastSave=G.LastSave;
    Json Prior=!WasTitle||G.CampaignStarted?G.Capture():Json();
    if(!G.ValidateSave(Prior))
    {
        // A dead player still has the ordinary on-disk checkpoints.
        Prior.Reset();
        if(!WasTitle&&!H->Dead&&!G.PendingRespawn)
        {Feedback(G,TEXT("Сейчас переход недоступен. Закончи текущую сцену и попробуй снова."));return false;}
    }
    const bool KeepOriginal=!G.Flag(TestRun)&&!WasTitle&&Prior.IsValid();
    if(G.Admin)G.ToggleAdmin();
    G.Agent=false;G.NewGame(G.Difficulty);G.Flag(TestRun,true);G.LastSave.Empty();
    // A chapter jump is a separate playthrough; earlier choices follow the
    // first authored option. The chosen chapter's scenes remain unplayed.
    FString Failed;
    if(To>1&&ReplayPrefix(G,To,Failed))
    {
        const FString First=Expected(*S);
        FString Target=First;
        for(auto V:Array(S->NarrativeScenes35.FindRef(First),TEXT("requires")))
        {
            const FString Key=V->AsString();
            if(!G.Completed.Contains(Key)&&!S->NarrativeFlag35(Key)&&S->Props.Contains(Key)){Target=Key;break;}
        }
        FVector At;
        auto* Prop=S->Props.FindRef(Target);
        if(IsValid(Prop)&&S->FindNarrativeStanding35(Target,At)&&G.BookWorld->ValidPosition(At))
        {
            H->SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);
            const FVector Aim=Prop->GetActorLocation()+FVector(0,0,18)-(At+FVector(0,0,65));
            H->SetActorRotation(FRotator(0,Aim.Rotation().Yaw,0),ETeleportType::TeleportPhysics);
            PC->SetControlRotation(Aim.Rotation());
            H->Arm->TickComponent(0,LEVELTICK_All,nullptr);
            G.CurrentRoom=G.BookWorld->Describe(At);
            G.BookWorld->RestoreLift(At);G.BookWorld->UpdateLighting(At);
            Failed.Empty();
        }
        else Failed=Target.IsEmpty()?TEXT("нет точки входа"):Target;
    }
    else if(To>1&&Failed.IsEmpty())Failed=TEXT("порядок событий");
    if(!Failed.IsEmpty())
    {
        if(Prior)G.Restore(Prior);else {G.NewGame(G.Difficulty);G.CampaignStarted=!WasTitle;}
        G.LastSave=LastSave;
        if(WasAdmin&&SafeCamera(G)&&!G.Admin)G.ToggleAdmin();
        G.Show(OldPage);Feedback(G,TEXT("Переход отменён: не удалось восстановить ")+Failed);
        return false;
    }
    if(KeepOriginal){State.Original=Prior;State.LastSave=LastSave;}
    G.Show(TEXT(""));
    if(To==1)S->StartArrival();
    else if(WasAdmin)G.ToggleAdmin();
    G.AutoSave();
    G.Notify(TEXT("Тестовый запуск · ")+ChapterLabel(*S,To)+(G.Admin?TEXT(" · F10 — управление героем"):TEXT("")),6);
    return true;
}
bool BackToGame(AEWardCampaignMode& G)
{
    if(!G.Flag(TestRun)){Feedback(G,TEXT("Сейчас открыт обычный сеанс."));return false;}
    auto& State=Session(G);
    if(G.Admin)G.ToggleAdmin();
    if(State.Original&&G.Restore(State.Original))
    {State.Original.Reset();G.LastSave=MoveTemp(State.LastSave);G.Notify(TEXT("Обычное прохождение восстановлено."));return true;}
    G.NewGame(G.Difficulty);G.CampaignStarted=false;G.LastSave.Empty();G.Show(TEXT("title"));return true;
}

struct FModel {FString Id,Species,Name;float Height;};
TArray<FModel> Models()
{
    TArray<FModel> List;
    for(const auto& E:Object(WardMonster59::Data(),TEXT("models"))->Values)
    {
        auto Row=E.Value->AsObject();
        List.Add({FString(E.Key),String(Row,TEXT("species")),String(Row,TEXT("name"),FString(E.Key)),float(Number(Row,TEXT("height_cm"),178))});
    }
    List.Sort([](const FModel& A,const FModel& B){return A.Id<B.Id;});return List;
}
bool Spawn(AEWardCampaignMode& G,const FString& Key)
{
    if(!SafeCamera(G)||!G.HospitalStory)
    {Feedback(G,TEXT("Спавн доступен в игре после вступительной поездки."));return false;}
    auto& State=Session(G);State.Previews.RemoveAll([](auto M){return !M.IsValid();});
    if(State.Previews.Num()>=MaxPreviews){Feedback(G,TEXT("Лимит 4 модели. Используй spawn clear."));return false;}
    const auto List=Models();const auto* Model=List.FindByPredicate([&](const auto& M){return M.Id.Equals(Key,ESearchCase::IgnoreCase);});
    if(!Model)Model=List.FindByPredicate([&](const auto& M){return M.Species.Equals(Key,ESearchCase::IgnoreCase);});
    if(!Model){Feedback(G,TEXT("Неизвестный ID модели. Введи monsters."));return false;}
    auto* H=G.Hero();auto* W=G.GetWorld();
    const bool Free=G.Admin&&!G.AdminHands51&&G.AdminCamera;
    const FVector Eye=Free?G.AdminCamera->GetActorLocation():H->Camera->GetComponentLocation();
    const FRotator View=Free?G.AdminViewRotation:H->GetControlRotation();
    const FVector Forward=FRotator(0,View.Yaw,0).Vector();
    const float Radius=Model->Species==TEXT("central_organism")?60.f:36.f;
    const float Half=FMath::Max(Model->Height*.5f,Radius);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(AdminPreview79),false,H);
    if(H->BekaVisual&&H->BekaVisual->GetChildActor())Q.AddIgnoredActor(H->BekaVisual->GetChildActor());
    FVector Floor=FVector::ZeroVector;bool Found=false;
    for(float Angle:{0.f,22.f,-22.f,46.f,-46.f})
    {
        for(float Distance:{300.f,220.f,410.f,155.f})
        {
            const FVector Point=Eye+Forward.RotateAngleAxis(Angle,FVector::UpVector)*Distance;
            FHitResult Hit;
            if(!W->LineTraceSingleByChannel(Hit,Point,Point-FVector(0,0,360),ECC_WorldStatic,Q)||Hit.ImpactNormal.Z<.8||Eye.Z-Hit.ImpactPoint.Z<25)continue;
            const FVector Base=Hit.ImpactPoint,Center=Base+FVector(0,0,Half+4);
            if(W->OverlapBlockingTestByChannel(Center,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,Half),Q))continue;
            if(State.Previews.ContainsByPredicate([&](auto M){return M.IsValid()&&FVector::DistSquared2D(M->GetComponentLocation(),Center)<FMath::Square(115.);}))continue;
            FHitResult Sight;if(W->LineTraceSingleByChannel(Sight,Eye,Base+FVector(0,0,Half),ECC_Visibility,Q))continue;
            Floor=Base;Found=true;break;
        }
        if(Found)break;
    }
    if(!Found)
    {
        UE_LOG(LogTemp,Verbose,TEXT("ADMIN79 preview blocked at %s facing %s"),*Eye.ToString(),*View.ToString());
        Feedback(G,TEXT("Впереди нет свободного места на полу. Попробуй в коридоре или на улице."));return false;
    }
    // Render the same live rig as the enemy, without creating an AI pawn.
    auto* A=W->SpawnActor<AActor>();if(!A){Feedback(G,TEXT("Не удалось создать модель."));return false;}
    auto* Rig=NewObject<UEWardMonsterVisual59>(A);A->SetRootComponent(Rig);A->AddInstanceComponent(Rig);
    Rig->RegisterComponent();A->SetActorEnableCollision(false);
    A->SetActorLocationAndRotation(Floor,FRotator(0,(Eye-Floor).Rotation().Yaw,0));
    if(!Rig->Load59(Model->Id,Model->Height)||Rig->GetNumSections()==0)
    {A->Destroy();Feedback(G,TEXT("Не удалось загрузить модель: ")+Model->Id);return false;}
    State.Previews.Add(Rig);
    G.Show(TEXT(""));G.Notify(TEXT("Модель для осмотра: ")+Model->Name+TEXT(" · ")+Model->Id,5);
    return true;
}

Json AudioManifest(){static Json Data=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Audio68/manifest.json"));return Data;}
void ListModels(AEWardCampaignMode& G)
{
    TArray<FString> Lines;for(const auto& M:Models())Lines.Add(M.Id+TEXT("   —   ")+M.Name);
    Print(G,FString::Join(Lines,TEXT("\n")));
}
void ListItems(AEWardCampaignMode& G,bool Docs)
{
    TArray<FString> Lines;for(const auto& Item:G.PreviewCatalog51())if(Item.Document==Docs)
        Lines.Add((Docs?Item.Visual+TEXT("   /   hold ")+Item.Id:Item.Id)+TEXT("   —   ")+Item.Title);
    Lines.Sort();Print(G,FString::Join(Lines,TEXT("\n")));
}
void ListSounds(AEWardCampaignMode& G)
{
    TArray<FString> Keys;for(const auto& E:Object(AudioManifest(),TEXT("groups"))->Values)Keys.Add(FString(E.Key));
    Keys.Sort();Print(G,FString::Join(Keys,TEXT("\n")));
}
void Player(AEWardCampaignMode& G)
{
    if(G.Admin)G.ToggleAdmin();else Close(G);
    if(G.Flag(TestRun)&&G.Screen.IsEmpty())G.Notify(TEXT("Обычное управление · тестовая глава сохраняется отдельно"),4);
}
bool EnterAdmin(AEWardCampaignMode& G)
{
    if(!SafeCamera(G)){Feedback(G,TEXT("Свободная камера доступна во время игры, вне кат-сцены."));return false;}
    if(!G.Admin)G.ToggleAdmin();return G.Admin;
}
void Submit(AEWardCampaignMode& G,FString Input)
{
    auto& S=Session(G);Input.TrimStartAndEndInline();
    // Ignore any printable character Windows delivered after opening the panel.
    while(!Input.IsEmpty()&&(Input[0]==0x451||Input[0]==0x401||Input[0]==0x60||Input[0]==0x7E))Input.RightChopInline(1);
    Input.TrimStartAndEndInline();if(Input.IsEmpty())return;
    TArray<FString> Words;Input.ParseIntoArrayWS(Words);
    const FString Cmd=Words[0].ToLower(),Arg=Words.IsValidIndex(1)?Words[1]:FString(),Value=Arg.ToLower();
    if(Cmd==TEXT("close")){Close(G);return;}
    if(!Authorized(G))
    {
        if((Words.Num()==1&&Input==TEXT("0890"))||(Words.Num()==2&&Cmd==TEXT("login")&&Arg==TEXT("0890")))
        {S.Auth=true;S.Lines.Reset();Print(G,TEXT("Доступ открыт. Введи help."));G.Interface->Refresh();}
        else Print(G,TEXT("Неверный пароль администратора."));
        return;
    }
    if(Cmd==TEXT("login")){Print(G,TEXT("Доступ уже открыт."));return;}
    if(Cmd==TEXT("logout"))
    {StopSound(S);Clear(G);S.Auth=false;S.Lines.Reset();if(G.Admin)G.ToggleAdmin();else G.Interface->Refresh();return;}
    if(Input.Len()>160||Words.Num()>2){Print(G,TEXT("Одна команда и один аргумент. Список: help."));return;}
    Print(G,TEXT("> ")+Input);
    if(Cmd==TEXT("help"))Print(G,Help);
    else if(Cmd==TEXT("status"))Print(G,FString::Printf(TEXT("Глава: %d · камера: %s · режим: %s · модели: %d/%d"),
        G.HospitalStory?G.HospitalStory->Chapter():0,G.Admin?TEXT("админ"):TEXT("игрок"),G.Flag(TestRun)?TEXT("тест"):TEXT("обычный"),S.Previews.Num(),MaxPreviews));
    else if(Cmd==TEXT("chapters")||(Cmd==TEXT("chapter")&&(Value.IsEmpty()||Value==TEXT("list"))))
    {
        if(G.HospitalStory){TArray<FString> Rows;for(int Ch=1;Ch<=19;++Ch)Rows.Add(ChapterLabel(*G.HospitalStory,Ch));Print(G,FString::Join(Rows,TEXT("\n")));}
    }
    else if(Cmd==TEXT("chapter"))
    {
        if(Value==TEXT("back"))BackToGame(G);
        else if(Value.Len()<=2&&Value.IsNumeric()&&FCString::Atoi(*Value)>=1&&FCString::Atoi(*Value)<=19)Jump(G,FCString::Atoi(*Value));
        else Print(G,TEXT("chapter <1–19> или chapter back"));
    }
    else if(Cmd==TEXT("monsters"))ListModels(G);
    else if(Cmd==TEXT("spawn"))
    {
        if(Value==TEXT("clear"))Print(G,FString::Printf(TEXT("Убрано моделей: %d"),Clear(G)));
        else if(Value.IsEmpty())Print(G,TEXT("spawn <ID> · список: monsters"));
        else Spawn(G,Arg);
    }
    else if(Cmd==TEXT("items")||Cmd==TEXT("docs"))ListItems(G,Cmd==TEXT("docs"));
    else if(Cmd==TEXT("sounds"))ListSounds(G);
    else if(Cmd==TEXT("sound"))
    {
        StopSound(S);
        if(Value==TEXT("stop"))Print(G,TEXT("Прослушивание остановлено."));
        else
        {
            const auto Groups=Object(AudioManifest(),TEXT("groups"));
            for(const auto& E:Groups->Values)if(FString(E.Key).Equals(Arg,ESearchCase::IgnoreCase))
            {S.Sound=WardAudio68::Play(&G,FString(E.Key),.35f*G.Setting(TEXT("effects"),.8));break;}
            Print(G,S.Sound.IsValid()?TEXT("Воспроизводится: ")+Arg:FString(TEXT("Нет такого звука. Список: sounds.")));
        }
    }
    else if(Cmd==TEXT("hold")||Cmd==TEXT("read"))
    {
        const bool ReadDoc=Cmd==TEXT("read");FString Found;
        for(const auto& Item:G.PreviewCatalog51())
        {
            if(ReadDoc&&Item.Document&&Item.Visual.Equals(Arg,ESearchCase::IgnoreCase)){Found=Item.Visual;break;}
            if(!ReadDoc&&Item.Id.Equals(Arg,ESearchCase::IgnoreCase)){Found=Item.Id;break;}
        }
        if(Found.IsEmpty())Print(G,TEXT("ID не найден. Список: items или docs."));
        else if(EnterAdmin(G))
        {
            if(ReadDoc)G.PreviewCommand51(TEXT("preview_read_")+Found);
            else if(!G.HoldPreview51(Found))G.Notify(TEXT("Не удалось открыть модель предмета."));
        }
    }
    else if(Cmd==TEXT("release"))
    {if(G.Admin)G.PreviewCommand51(TEXT("preview_clear"));else Print(G,TEXT("Предмет для осмотра не выбран."));}
    else if(Cmd==TEXT("player")||(Cmd==TEXT("admin")&&Value==TEXT("off")))Player(G);
    else if(Cmd==TEXT("fly")||(Cmd==TEXT("admin")&&Value==TEXT("on")))
    {if(EnterAdmin(G))G.PreviewCommand51(TEXT("preview_fly"));}
    else if(Cmd==TEXT("panel")||Cmd==TEXT("catalog")||Cmd==TEXT("audio"))
    {
        if(EnterAdmin(G))
        {
            if(Cmd==TEXT("panel"))G.Show(Tools);
            else if(Cmd==TEXT("catalog"))G.PreviewCommand51(TEXT("preview_catalog"));
            else WardAudio68::Command(G,TEXT("audio68_catalog"));
        }
    }
    else if(Cmd==TEXT("agent")&&(Value==TEXT("on")||Value==TEXT("off")))
    {
        if(G.BookWorldEnabled&&Value==TEXT("on"))Print(G,TEXT("Агент старого режима недоступен в сюжетной кампании."));
        else if(G.Agent==(Value==TEXT("on")))Print(G,TEXT("Состояние агента уже установлено."));
        else if(EnterAdmin(G))G.ToggleAgent();
    }
    else if(Cmd==TEXT("bright")&&(Value==TEXT("on")||Value==TEXT("off")))
    {
        G.Options->SetBoolField(TEXT("inspection_bright"),Value==TEXT("on"));
        G.ApplySettings(false);Print(G,Value==TEXT("on")?TEXT("Яркий осмотр включён."):TEXT("Яркий осмотр выключен."));
    }
    else if(Cmd==TEXT("fps")||Cmd==TEXT("graphics"))
    {
        if(Cmd==TEXT("fps"))
        {
            if(auto* PC=G.GetWorld()->GetFirstPlayerController())PC->ConsoleCommand(TEXT("stat fps"));
            Print(G,TEXT("Счётчик кадров переключён."));
        }
        else
        {
            const int Next=(EWardGraphics::Current()+1)%3;
            G.Options->SetNumberField(TEXT("graphics_preset"),Next);G.Options->SetNumberField(TEXT("msaa"),3);
            G.Options->SetNumberField(TEXT("screen_percentage"),Next==2?75:100);G.ApplySettings();
            Print(G,TEXT("Графика: ")+EWardGraphics::Label());
        }
    }
    else Print(G,TEXT("Неизвестная команда или аргумент. Введи help."));
}

TSharedRef<STextBlock> Label(const FString& Value,int Size=18,FLinearColor Color=Ice)
{
    return SNew(STextBlock).Text(FText::FromString(Value)).ColorAndOpacity(Color)
        .Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),Size)).LineHeightPercentage(1.20f).AutoWrapText(true);
}
}
bool Page(const FString& Screen){return Screen==Terminal||Screen==Tools||Screen==Chapters||Screen==Monsters;}
bool PrimeChapterTwo(AEWardCampaignMode& G)
{
    if (!G.HospitalStory || !G.BookWorld || !G.Hero()) return false;
    FString Failed;
    if (!ReplayPrefix(G, 2, Failed)) return false;
    auto& Story = *G.HospitalStory;
    const FString First = Expected(Story);
    auto* Anchor = Story.Props.FindRef(First);
    FVector At;
    if (!IsValid(Anchor) || !Story.FindNarrativeStanding35(First, At) || !G.BookWorld->ValidPosition(At))
        return false;
    auto* Hero = G.Hero();
    Hero->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
    const FVector Aim = Anchor->GetActorLocation() + FVector(0, 0, 18) - (At + FVector(0, 0, 68));
    if (auto* PC = Cast<APlayerController>(Hero->GetController())) PC->SetControlRotation(Aim.Rotation());
    G.BookWorld->RestoreLift(At);
    G.BookWorld->UpdateLighting(At);
    G.CurrentRoom = G.BookWorld->Describe(At);
    return true;
}

bool TestAdvanceCampaign(AEWardCampaignMode& G, int32 Chapter)
{
    FString Failed;
    const bool Result = ReplayPrefix(G, Chapter, Failed);
    if (!Result) UE_LOG(LogTemp, Error, TEXT("ONLINE85 scene failed: %s"), *Failed);
    return Result;
}

void Install(AEWardCampaignMode& G)
{
    if(!FSlateApplication::IsInitialized())return;
    auto& S=Session(G);S.Input=MakeShared<FKeys>(G);FSlateApplication::Get().RegisterInputPreProcessor(S.Input,0);
}
void Uninstall(AEWardCampaignMode& G)
{
    if(auto* S=Sessions.Find(&G))
    {if(S->Input&&FSlateApplication::IsInitialized())FSlateApplication::Get().UnregisterInputPreProcessor(S->Input);StopSound(*S);Sessions.Remove(&G);}
}
void ResetWorld(AEWardCampaignMode& G)
{
    if(auto* S=Sessions.Find(&G)){Clear(G);StopSound(*S);}
}
void Tick(AEWardCampaignMode& G,float Delta)
{
    auto* S=Sessions.Find(&G);if(!S||S->Previews.IsEmpty())return;
    S->Age+=Delta;
    const FVector Eye=G.Admin&&!G.AdminHands51&&G.AdminCamera?G.AdminCamera->GetActorLocation():G.Hero()?G.Hero()->GetActorLocation():FVector::ZeroVector;
    for(auto M:S->Previews)if(M.IsValid()&&FVector::DistSquared(M->GetComponentLocation(),Eye)<FMath::Square(3600.))
        M->Present59(0,S->Age,false,0);
}
bool Command(AEWardCampaignMode& G,const FString& Action)
{
    if(!Action.StartsWith(TEXT("adm79_")))return false;
    if(Action==TEXT("adm79_terminal")){Toggle(G);return true;}
    if(!G.Admin){G.Notify(TEXT("Включи режим администратора."));return true;}
    Session(G).Feedback.Empty();
    if(Action==TEXT("adm79_tools"))G.Show(Tools);
    else if(Action==TEXT("adm79_chapters"))G.Show(Chapters);
    else if(Action==TEXT("adm79_monsters"))G.Show(Monsters);
    else if(Action==TEXT("adm79_back"))G.Show(G.Screen==Tools?TEXT("pause"):Tools);
    else if(Action==TEXT("adm79_restore"))BackToGame(G);
    else if(Action==TEXT("adm79_clear"))Feedback(G,FString::Printf(TEXT("Убрано моделей: %d"),Clear(G)));
    else if(Action.StartsWith(TEXT("adm79_jump:")))Jump(G,FCString::Atoi(*Action.RightChop(11)));
    else if(Action.StartsWith(TEXT("adm79_spawn:")))Spawn(G,Action.RightChop(12));
    return true;
}
TSharedRef<SWidget> Build(AEWardCampaignMode& G)
{
    auto& S=Session(G);auto Body=SNew(SVerticalBox);
    TWeakObjectPtr<AEWardCampaignMode> Weak(&G);
    auto Row=[&](const FString& Name,const FString& Action)
    {
        auto Button=SNew(SButton).ButtonStyle(&ButtonStyle).ContentPadding(FMargin(14,9))
            .OnClicked_Lambda([Weak,Action]{if(auto* M=Weak.Get())M->Command(Action);return FReply::Handled();})[Label(Name)];
        G.Interface->TitleActions.Add(Action,Button);
        return Button;
    };
    G.Interface->TitleActions.Reset();
    const FString Title=G.Screen==Terminal?TEXT("ТЕРМИНАЛ  /  ОТДЕЛЕНИЕ 04"):G.Screen==Chapters?TEXT("ТЕСТОВЫЙ ЗАПУСК ГЛАВЫ"):G.Screen==Monsters?TEXT("КАТАЛОГ МОНСТРОВ"):TEXT("ПАНЕЛЬ АДМИНИСТРАТОРА");
    Body->AddSlot().AutoHeight()[Label(Title,28,Ice)];
    Body->AddSlot().AutoHeight().Padding(0,13)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&Divider)]];
    if(G.Screen==Terminal)
    {
        const bool Allowed=Authorized(G);
        Body->AddSlot().AutoHeight().Padding(0,0,0,12)[Label(Allowed?TEXT("Enter — выполнить · Ё / Esc — закрыть · help — команды · латиница"):TEXT("Введи пароль администратора и нажми Enter.  Ё / Esc — закрыть."),16,Dim)];
        auto Scroll=SNew(SScrollBox);S.Output=Scroll;
        Scroll->AddSlot()[SNew(STextBlock).Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),17))
            .ColorAndOpacity(Ice).LineHeightPercentage(1.28f).AutoWrapText(true).Text_Lambda([Weak]{
                auto* M=Weak.Get();if(!M)return FText();
                const auto& Lines=Session(*M).Lines;
                return FText::FromString(Lines.IsEmpty()?(Authorized(*M)?Help:FString()):FString::Join(Lines,TEXT("\n")));
            })];
        Body->AddSlot().FillHeight(1).Padding(0,0,0,12)[Scroll];
        auto Entry=SNew(SEditableTextBox).IsPassword(!Allowed).ClearKeyboardFocusOnCommit(false)
            .HintText(FText::FromString(Allowed?TEXT("Введи команду, например chapter 2"):TEXT("Пароль")))
            .OnTextCommitted_Lambda([Weak](const FText& Text,ETextCommit::Type Type){
                auto* M=Weak.Get();if(!M||M->Screen!=Terminal||Type!=ETextCommit::OnEnter)return;
                auto Entry=Session(*M).Entry.Pin();const FString Line=Text.ToString();
                if(Entry)Entry->SetText(FText::GetEmpty());Submit(*M,Line);
            });
        S.Entry=Entry;S.Focus=true;Scroll->ScrollToEnd();
        Body->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(43)[Entry]];
    }
    else if(G.Admin)
    {
        Body->AddSlot().AutoHeight().Padding(0,0,0,9)[Row(TEXT("‹  Назад"),TEXT("adm79_back"))];
        auto Scroll=SNew(SScrollBox);
        auto Add=[&](const FString& Name,const FString& Action){Scroll->AddSlot().Padding(0,0,0,5)[Row(Name,Action)];};
        if(G.Screen==Chapters&&G.HospitalStory)
        {
            Scroll->AddSlot().Padding(0,3,0,13)[Label(TEXT("Предыдущие события восстановятся с исходными вариантами выбора. Сохранения этой попытки отдельные. Глава 01 начинается дома. В остальных F10 вернёт управление Бекой."),16,Dim)];
            if(G.Flag(TestRun))Add(TEXT("Вернуться к обычному прохождению  ·  chapter back"),TEXT("adm79_restore"));
            for(int Ch=1;Ch<=19;++Ch)Add(ChapterLabel(*G.HospitalStory,Ch),TEXT("adm79_jump:")+FString::FromInt(Ch));
        }
        else if(G.Screen==Monsters)
        {
            Scroll->AddSlot().Padding(0,3,0,13)[Label(TEXT("Модель появится на свободном полу перед камерой. Она не нападает, не блокирует проход и не попадает в сохранения."),16,Dim)];
            Add(TEXT("Убрать все модели  ·  spawn clear"),TEXT("adm79_clear"));
            for(const auto& M:Models())
                Add(M.Name+TEXT("  ·  ")+String(WardMonster59::Rule(M.Species),TEXT("name"),M.Species)+TEXT("  /  ")+M.Id,TEXT("adm79_spawn:")+M.Id);
        }
        else
        {
            Add(TEXT("Выбрать главу  ·  chapter <номер>"),TEXT("adm79_chapters"));
            Add(TEXT("Поставить монстра  ·  spawn <ID>"),TEXT("adm79_monsters"));
            Add(TEXT("Предметы и записки  ·  F8"),TEXT("preview_catalog"));
            Add(TEXT("Проверка звуков"),TEXT("audio68_catalog"));
            Add(TEXT("Терминал  ·  Ё"),TEXT("adm79_terminal"));
            if(G.Flag(TestRun))Add(TEXT("Вернуться к обычному прохождению"),TEXT("adm79_restore"));
            Add(TEXT("Вернуться к персонажу  ·  F10"),TEXT("admin_exit"));
        }
        Body->AddSlot().FillHeight(1)[Scroll];
        Body->AddSlot().AutoHeight().Padding(0,8,0,0)[SNew(STextBlock).ColorAndOpacity(Red).AutoWrapText(true)
            .Text_Lambda([Weak]{return Weak.IsValid()?FText::FromString(Session(*Weak.Get()).Feedback):FText();})
            .Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),16))];
    }
    return SNew(SOverlay)+SOverlay::Slot().Padding(32)
        [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(1060).HeightOverride(G.Screen==Terminal?620:720)[SNew(SBorder).BorderImage(&Backdrop).Padding(28)[Body]]]];
}
void Assess(AEWardCampaignMode& G)
{
    if(!G.HospitalStory||!G.Hero()||!G.Hero()->GetController())return;
    static bool Started=false;if(Started)return;Started=true;
    auto& S=*G.HospitalStory;auto& State=Session(G);
    TArray<TSharedPtr<FJsonValue>> Checks;
    bool OK=true;
    auto Check=[&](const FString& Id,bool Pass){
        Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),Id);Row->SetBoolField(TEXT("passed"),Pass);
        Checks.Add(MakeShared<FJsonValueObject>(Row));OK&=Pass;UE_LOG(LogTemp,Display,TEXT("ADMIN79 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Id);
    };
    G.Assessment=false;G.Show(TEXT(""));
    auto& Slate=FSlateApplication::Get();
    auto Key=[&](FKey K,uint32 C,uint32 Scan){return State.Input&&State.Input->HandleKeyDownEvent(Slate,FKeyEvent(K,FModifierKeysState(),0,false,C,Scan));};
    const bool Open=Key(EKeys::Invalid,0x451,0xC0);
    Check(TEXT("russian_physical_key_opens_custom_terminal"),Open&&G.Screen==Terminal&&State.Entry.IsValid());
    const bool CloseKey=Key(EKeys::Tilde,0x60,0xC0);
    Check(TEXT("english_key_closes_and_restores_previous_screen"),CloseKey&&G.Screen.IsEmpty());
    Toggle(G);
    Submit(G,TEXT("chapter 2"));Check(TEXT("locked_terminal_does_not_jump"),S.Chapter()==1&&!G.Flag(TestRun)&&!State.Auth);
    Submit(G,TEXT("0000"));Check(TEXT("bad_password_rejected"),!State.Auth);
    Submit(G,TEXT("login 0890"));Check(TEXT("login_does_not_enable_free_camera"),State.Auth&&!G.Admin);
    Submit(G,TEXT("help"));Check(TEXT("help_lists_gameplay_and_preview_commands"),State.Lines.Last().Contains(TEXT("chapter back"))&&State.Lines.Last().Contains(TEXT("sound stop")));
    Submit(G,TEXT("chapter 20"));Check(TEXT("invalid_chapter_keeps_state"),!G.Flag(TestRun)&&S.Chapter()==1);
    const FString NormalDirectory=G.SaveDirectory();
    const FVector Original=G.Hero()->GetActorLocation();
    Submit(G,TEXT("chapter 2"));
    Check(TEXT("terminal_jump_starts_as_normal_player"),S.Chapter()==2&&G.Flag(TestRun)&&!G.Admin&&G.Screen.IsEmpty());
    Check(TEXT("test_save_uses_separate_directory"),G.SaveDirectory()!=NormalDirectory&&G.Save(TEXT("manual_1"))&&G.ValidateSave(G.ReadSlot(TEXT("manual_1"))));
    Check(TEXT("load_test_save_restores_chapter_without_admin"),G.Load(TEXT("manual_1"))&&S.Chapter()==2&&G.Flag(TestRun)&&!G.Admin);
    Check(TEXT("chapter_back_restores_original_player_and_save_directory"),BackToGame(G)&&S.Chapter()==1&&G.SaveDirectory()==NormalDirectory&&G.Hero()->GetActorLocation().Equals(Original,1));

    Jump(G,1);
    Check(TEXT("chapter_one_opens_home_prologue"),S.HomeStage52>=0&&!G.Admin&&G.Flag(TestRun));
    for(int Ch=2;Ch<=19;++Ch)
    {
        const bool Jumped=Jump(G,Ch);
        const FString First=Expected(S);
        Check(FString::Printf(TEXT("chapter_%02d_start_position_and_state"),Ch),Jumped&&S.Chapter()==Ch&&
            G.Values.FindRef(TEXT("hospital_step"))==0&&!G.Completed.Contains(First)&&S.ActiveScene35.IsEmpty()&&G.ValidateSave(G.Capture()));
        if(Jumped)
        {
            if(Ch==15)
            {
                const bool Prompt=S.Prompt().Contains(String(S.Definitions.FindRef(TEXT("NEW_BASELINE_001")),TEXT("label")));
                G.BookWorld->UseArchitecture();
                Check(TEXT("chapter_15_starts_at_required_sample"),Prompt&&G.Completed.Contains(TEXT("NEW_BASELINE_001")));
            }
            else
            {
                G.BookWorld->UseArchitecture();
                Check(FString::Printf(TEXT("chapter_%02d_first_objective_responds_to_interact"),Ch),S.ActiveScene35==First);
            }
        }
    }
    Jump(G,2);G.ToggleAdmin();G.Command(TEXT("adm79_tools"));G.Command(TEXT("adm79_chapters"));
    Check(TEXT("panel_exposes_all_chapter_buttons"),G.Interface->TitleActions.Contains(TEXT("adm79_jump:1"))&&G.Interface->TitleActions.Contains(TEXT("adm79_jump:19")));
    G.Command(TEXT("adm79_jump:2"));
    Check(TEXT("panel_jump_keeps_admin_camera_until_player_exit"),G.Admin&&S.Chapter()==2&&G.AdminCamera);
    G.Command(TEXT("admin_exit"));
    Check(TEXT("admin_exit_returns_to_beka"),!G.Admin&&G.Hero()->GetController<APlayerController>()->GetViewTarget()==G.Hero());

    // Provide a known floor away from the campaign to exercise all model
    // variants independently of the clearance at a particular chapter spawn.
    const FVector Arena(-70000,-70000,96);
    auto* Floor=G.GetWorld()->SpawnActor<AActor>();
    auto* Box=NewObject<UBoxComponent>(Floor);Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(1200,1200,10));
    Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Floor->SetActorLocation(Arena-FVector(0,0,106));
    G.Hero()->SetActorLocation(Arena,false,nullptr,ETeleportType::TeleportPhysics);
    auto* PC=G.Hero()->GetController<APlayerController>();PC->SetControlRotation(FRotator(0,90,0));
    G.Hero()->Arm->TickComponent(0,LEVELTICK_All,nullptr);
    const int StoryMobs=S.Mobs.Num();
    for(const auto& M:Models())
    {
        const bool Spawned=Spawn(G,M.Id);auto* Preview=Spawned?State.Previews.Last().Get():nullptr;
        Check(TEXT("model_")+M.Id,Preview&&Preview->Vertices59()>0&&Preview->LoadedModel61()==M.Id&&!Preview->GetOwner()->GetActorEnableCollision()&&!Preview->GetOwner()->IsHidden()&&S.Mobs.Num()==StoryMobs);
        Clear(G);
    }
    Check(TEXT("preview_clear_leaves_story_encounters_unchanged"),State.Previews.IsEmpty()&&S.Mobs.Num()==StoryMobs);
    G.ToggleAdmin();G.Command(TEXT("adm79_monsters"));
    Check(TEXT("panel_exposes_monster_models"),G.Interface->TitleActions.Contains(TEXT("adm79_spawn:patients_01")));
    G.ToggleAdmin();Toggle(G);Submit(G,TEXT("read NOTE_01_01"));
    Check(TEXT("terminal_can_open_document"),G.Screen==TEXT("admin_document")&&G.PreviewDocument51==TEXT("NOTE_01_01"));
    G.ToggleAdmin();Toggle(G);Submit(G,TEXT("hold weapon"));
    Check(TEXT("terminal_can_hold_weapon"),G.AdminHands51&&G.PreviewItem51==TEXT("weapon"));
    Toggle(G);Submit(G,TEXT("release"));Check(TEXT("release_clears_preview"),G.PreviewItem51.IsEmpty());
    if(G.Admin)G.ToggleAdmin();
    Toggle(G);Submit(G,TEXT("sound UI/paper_open"));
    Check(TEXT("terminal_can_play_catalogued_sound"),State.Sound.IsValid());
    Submit(G,TEXT("sound stop"));Check(TEXT("terminal_can_stop_sound"),!State.Sound.IsValid());
    Submit(G,TEXT("logout"));Check(TEXT("logout_relocks_terminal"),!State.Auth&&!G.Admin);
    Close(G);BackToGame(G);
    Check(TEXT("test_chapter_writes_no_ordinary_save"),G.SaveDirectory()==NormalDirectory&&!G.ValidateSave(G.ReadSlot(TEXT("manual_1"))));
    Toggle(G);
    Submit(G,TEXT("0890"));Submit(G,TEXT("help"));
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Prototype");
    Json Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),OK);Result->SetNumberField(TEXT("count"),Checks.Num());Result->SetArrayField(TEXT("checks"),Checks);
    Write(Directory/TEXT("admin79_check.json"),Result);
    FScreenshotRequest::RequestScreenshot(Directory/TEXT("admin79_terminal.png"),true,false);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float){FPlatformMisc::RequestExit(false);return false;}),1.5f);
}
}
