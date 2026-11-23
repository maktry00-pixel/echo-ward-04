#include "WardCampaignUI.h"
#include "WardAdmin79.h"
#include "WardOnline80.h"
#include "WardDialogue62.h"
#include "GameFramework/PlayerController.h"
#include "WardAudio68.h"
#include "WardInterface66.h"
#include "WardHospitalGameplay.h"
#include "WardTitleScene.h"
#include "Widgets/SOverlay.h"
#include "WardCampaign.h"
#include "WardCampaignMap.h"
#include "WardCampaignXray.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "UnrealClient.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "WardSecurity103.inl"
using namespace WardData;
namespace
{
    FSlateColorBrush Back(FLinearColor(.005,.002,.003,.95)),PanelBrush(FLinearColor(.014,.003,.006,.96));
    TArray<TUniquePtr<FSlateBrush>> ImageBrushes;
    struct FGuideImage {TStrongObjectPtr<UTexture2D> Texture;FSlateBrush Brush;};
    TMap<FString,TUniquePtr<FGuideImage>> GuideImages;
    const FLinearColor Teal(.79,.10,.12),Gold(.76,.42,.32),Coral(.91,.27,.18);
    FButtonStyle FlatButton=[](){FButtonStyle Style;Style.SetNormal(FSlateColorBrush(FLinearColor::White));Style.SetHovered(FSlateColorBrush(FLinearColor::White));Style.SetPressed(FSlateColorBrush(FLinearColor::White));Style.SetDisabled(FSlateColorBrush(FLinearColor(.04,.07,.07,1)));return Style;}();
    class SNeutralMenuButton : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SNeutralMenuButton){} SLATE_ARGUMENT(TWeakObjectPtr<AEWardCampaignMode>,Mode) SLATE_ARGUMENT(FString,Action) SLATE_ARGUMENT(bool,Enabled) SLATE_DEFAULT_SLOT(FArguments,Content) SLATE_END_ARGS()
        void Construct(const FArguments& A)
        {
            Blend=MakeShared<float>(0);ChildSlot[SAssignNew(Button,SButton).ButtonStyle(&FlatButton).IsEnabled(A._Enabled).ButtonColorAndOpacity_Lambda([M=Blend](){return FMath::Lerp(FLinearColor(.035,.006,.010),FLinearColor(.42,.008,.018),*M);}).ContentPadding(FMargin(16,12)).OnHovered_Lambda([Weak=A._Mode](){if(Weak.IsValid())WardAudio68::Play(Weak.Get(),TEXT("UI/hover"),.07f*Weak->Setting(TEXT("effects"),.8));}).OnClicked_Lambda([Weak=A._Mode,Action=A._Action](){if(Weak.IsValid())Weak->Command(Action);return FReply::Handled();})[A._Content.Widget]];
        }
        virtual void Tick(const FGeometry& G,double T,float D)override{SCompoundWidget::Tick(G,T,D);*Blend=FMath::FInterpTo(*Blend,Button->IsEnabled()&&Button->IsHovered()?1.f:0.f,D,12);}
        TSharedPtr<SButton> Button;TSharedPtr<float> Blend;
    };
}
FWardCampaignUI::FWardCampaignUI(AEWardCampaignMode* InMode):Mode(InMode){}
void FWardCampaignUI::Open()
{
    Close();
    if (!GEngine || !GEngine->GameViewport) return;
    if (auto* G = Mode.Get(); G && G->Screen == TEXT("story35") && G->HospitalStory)
        if (auto* PC = G->GetWorld()->GetFirstPlayerController())
        {
            const bool Controls = WardDialogue62::HasControls(*G->HospitalStory);
            if (PC->bShowMouseCursor != Controls)
            {
                PC->bShowMouseCursor = Controls;
                if (Controls) PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
                else PC->SetInputMode(FInputModeGameOnly());
            }
        }
    Root=Build();
    GEngine->GameViewport->AddViewportWidgetContent(Root.ToSharedRef(),100);
}
void FWardCampaignUI::Close(){if(Root.IsValid()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Root.ToSharedRef());Root.Reset();}
void FWardCampaignUI::Refresh(){Open();}
TSharedRef<SWidget> FWardCampaignUI::Text(const FString& Value,int32 Size,FLinearColor Color)
{
    auto* G=Mode.Get();const FString Font=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf");
    FString Display=G?G->Tr(Value):Value;if(G&&G->BookWorld){Display.ReplaceInline(TEXT("Фонарь B"),TEXT("Фонарь N"));Display.ReplaceInline(TEXT("Flashlight B"),TEXT("Flashlight N"));if(Value.StartsWith(TEXT("B   /")))Display[0]='N';}Display.ReplaceInline(TEXT("←"),TEXT("<"));Display.ReplaceInline(TEXT("→"),TEXT(">"));Display.ReplaceInline(TEXT("↑"),TEXT("^"));Display.ReplaceInline(TEXT("↓"),TEXT("v"));
    return SNew(STextBlock).Text(FText::FromString(Display)).Font(FSlateFontInfo(Font,Size)).ColorAndOpacity(Color).AutoWrapText(true);
}
TSharedRef<SWidget> FWardCampaignUI::Button(const FString& Label,const FString& Action,bool Accent,bool Enabled)
{
    auto Control=SNew(SNeutralMenuButton).Mode(Mode).Action(Action).Enabled(Enabled)[Text(Label,17,FLinearColor(.82,.88,.86))];TitleActions.Add(Action,Control->Button);TitleMotion.Add(Action,[M=Control->Blend](){return *M;});return Control;
}
TSharedRef<SWidget> FWardCampaignUI::Panel(TSharedRef<SWidget> Body){return SNew(SBorder).BorderImage(&PanelBrush).Padding(24)[Body];}
TSharedRef<SWidget> FWardCampaignUI::Guide(const FString& Kind)
{
    auto* G=Mode.Get();const auto& Pages=Array(G->Data,Kind);if(Pages.IsEmpty())return Text(TEXT("Материалы загружаются"));GuidePage=FMath::Clamp(GuidePage,0,Pages.Num()-1);Json Page=Pages[GuidePage]->AsObject();auto Body=SNew(SVerticalBox);
    auto Tabs=SNew(SHorizontalBox);for(int32 I=0;I<Pages.Num();++I)Tabs->AddSlot().AutoWidth().Padding(3)[Button(FString::Printf(TEXT("%02d"),I+1),TEXT("guide_")+FString::FromInt(I),I==GuidePage)];Body->AddSlot().AutoHeight()[Tabs];
    Body->AddSlot().AutoHeight().Padding(0,16)[Text(String(Page,TEXT("title")),30,Teal)];
    const FString Image=String(Page,TEXT("image"));if(auto* Texture=LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/Campaign/Guide/T_Guide_")+Image+TEXT(".T_Guide_")+Image)))
    {if(!GuideImages.Contains(Image)){auto Cached=MakeUnique<FGuideImage>();Cached->Texture.Reset(Texture);Cached->Brush.SetResourceObject(Texture);Cached->Brush.ImageSize=FVector2D(1920,1080);GuideImages.Add(Image,MoveTemp(Cached));}auto* Raw=&GuideImages[Image]->Brush;Body->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(260)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Raw)]]];}
    Body->AddSlot().AutoHeight().Padding(0,12)[Text(String(Page,TEXT("intro")),20)];
    for(const auto& V:Array(Page,TEXT("details"))){const auto& Pair=V->AsArray();if(Pair.Num()>1){Body->AddSlot().AutoHeight().Padding(0,8)[Text(Pair[0]->AsString(),20,Teal)];Body->AddSlot().AutoHeight()[Text(Pair[1]->AsString())];}}
    for(const auto& V:Array(Page,TEXT("keys"))){const auto& Pair=V->AsArray();if(Pair.Num()>1)Body->AddSlot().AutoHeight().Padding(0,4)[Text(Pair[0]->AsString()+TEXT("   /   ")+Pair[1]->AsString(),17,Teal)];}
    if(Kind==TEXT("controls")){Body=SNew(SVerticalBox);Body->AddSlot().AutoHeight()[Text(WardInterface66::Controls(G->Admin),17)];}
    return Body;
}
TSharedRef<SWidget> FWardCampaignUI::Settings()
{
    auto* G=Mode.Get();auto Body=SNew(SVerticalBox);auto Tabs=SNew(SHorizontalBox);
    const TArray<TPair<FString,FString>> Sections={{TEXT("display"),TEXT("Экран")},{TEXT("graphics"),TEXT("Графика")},{TEXT("audio"),TEXT("Звук")},{TEXT("camera"),TEXT("Камера")},{TEXT("interface"),TEXT("Интерфейс")},{TEXT("controls"),TEXT("Управление")},{TEXT("admin"),TEXT("Администратор")}};
    for(const auto& E:Sections)Tabs->AddSlot().AutoWidth().Padding(2)[Button(E.Value,TEXT("section_")+E.Key,E.Key==Section)];Body->AddSlot().AutoHeight()[Tabs];
    if(Section==TEXT("controls")){Body->AddSlot().AutoHeight()[Guide(TEXT("controls"))];return Body;}
    if(Section==TEXT("admin"))
    {Body->AddSlot().AutoHeight().Padding(0,18)[Text(TEXT("Режим администратора · свободная камера и проверка прохождения"),24,Teal)];Body->AddSlot().AutoHeight()[SNew(SEditableTextBox).IsPassword(true).HintText(FText::FromString(TEXT("Пароль"))).OnTextChanged_Lambda([this](const FText& T){PendingCode=T.ToString();})];Body->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("Включить режим администратора"),TEXT("admin_enter"))];return Body;}
    TArray<FString> Keys;if(Section==TEXT("display"))Keys={TEXT("fullscreen"),TEXT("vsync"),TEXT("max_fps"),TEXT("language")};
    else if(Section==TEXT("graphics"))Keys={TEXT("texture_quality"),TEXT("shadow_quality"),TEXT("msaa"),TEXT("brightness"),TEXT("ambient"),TEXT("contrast"),TEXT("fog"),TEXT("flashlight_strength"),TEXT("vignette"),TEXT("grain")};
    else if(Section==TEXT("audio"))Keys={TEXT("music"),TEXT("effects"),TEXT("subtitles")};
    else if(Section==TEXT("camera"))Keys={TEXT("fov"),TEXT("sensitivity"),TEXT("invert_y"),TEXT("shake"),TEXT("camera_motion")};
    else Keys={TEXT("hud_opacity"),TEXT("crosshair"),TEXT("objective_hint"),TEXT("show_noise")};
    const TMap<FString,FString> Names={{TEXT("fullscreen"),TEXT("Полноэкранный режим")},{TEXT("vsync"),TEXT("Вертикальная синхронизация")},{TEXT("max_fps"),TEXT("Ограничение кадров")},{TEXT("language"),TEXT("Язык / Тіл / Language")},{TEXT("texture_quality"),TEXT("Качество текстур")},{TEXT("shadow_quality"),TEXT("Качество теней")},{TEXT("msaa"),TEXT("Сглаживание: 0/FXAA/TAA/TSR")},{TEXT("brightness"),TEXT("Яркость")},{TEXT("ambient"),TEXT("Освещение окружения")},{TEXT("contrast"),TEXT("Контраст")},{TEXT("fog"),TEXT("Плотность тумана")},{TEXT("flashlight_strength"),TEXT("Сила фонаря")},{TEXT("vignette"),TEXT("Виньетка")},{TEXT("grain"),TEXT("Зернистость")},{TEXT("music"),TEXT("Музыка и атмосфера")},{TEXT("effects"),TEXT("Громкость эффектов")},{TEXT("subtitles"),TEXT("Субтитры")},{TEXT("fov"),TEXT("Поле зрения")},{TEXT("sensitivity"),TEXT("Чувствительность мыши")},{TEXT("invert_y"),TEXT("Инверсия мыши")},{TEXT("shake"),TEXT("Отдача камеры")},{TEXT("camera_motion"),TEXT("Плавность камеры")},{TEXT("hud_opacity"),TEXT("Прозрачность интерфейса")},{TEXT("crosshair"),TEXT("Прицел")},{TEXT("objective_hint"),TEXT("Подсказки задачи")},{TEXT("show_noise"),TEXT("Индикатор шума")}};
    for(const FString& K:Keys){auto V=G->Options->TryGetField(K);const FString Value=V.IsValid()?(V->Type==EJson::Boolean?(V->AsBool()?TEXT("Вкл"):TEXT("Выкл")):V->Type==EJson::Number?FString::SanitizeFloat(V->AsNumber()):V->AsString()):TEXT("—");Body->AddSlot().AutoHeight().Padding(0,5)[Button(Names.FindRef(K)+TEXT("     ")+Value,TEXT("setting_")+K)];}
    Body->AddSlot().AutoHeight().Padding(0,12)[Button(TEXT("Сбросить настройки"),TEXT("settings_reset"))];return Body;
}
TSharedRef<SWidget> FWardCampaignUI::Saves()
{
    auto* G=Mode.Get();auto B=SNew(SVerticalBox);B->AddSlot().AutoHeight().Padding(0,10)[Text(TEXT("Шесть ручных слотов · три автоматические контрольные точки"),22,Teal)];
    for(const FString& Slot:G->SaveSlots())
    {Json S=G->ReadSlot(Slot);const bool Valid=G->ValidateSave(S);auto Row=SNew(SHorizontalBox);Row->AddSlot().FillWidth(1)[Text(Slot+TEXT("   ")+(Valid?String(S,TEXT("room"))+TEXT(" · ")+String(S,TEXT("saved_at")):TEXT("Пустой слот")),16)];Row->AddSlot().AutoWidth().Padding(3)[Button(TEXT("Загрузить"),TEXT("load_")+Slot,false,Valid)];if(Slot.StartsWith(TEXT("manual_")))Row->AddSlot().AutoWidth().Padding(3)[Button(TEXT("Сохранить"),TEXT("save_")+Slot,false,G->CampaignStarted)];B->AddSlot().AutoHeight().Padding(0,5)[Row];}
    return B;
}
TSharedRef<SWidget> FWardCampaignUI::Inventory()
{
    auto* G=Mode.Get();auto B=SNew(SVerticalBox);B->AddSlot().AutoHeight()[Text(TEXT("Снаряжение · 12 мест"),28,Teal)];
    for(const auto& E:G->Inventory()){auto Row=SNew(SHorizontalBox);Row->AddSlot().FillWidth(1)[Button(G->ItemName(E.Key)+TEXT(" × ")+FString::FromInt(E.Value),TEXT("inspect_")+E.Key)];Row->AddSlot().AutoWidth()[Button(TEXT("Использовать"),TEXT("use_")+E.Key)];Row->AddSlot().AutoWidth()[Button(G->Screen==TEXT("stash")?TEXT("В шкаф"):TEXT("Убрать"),(G->Screen==TEXT("stash")?TEXT("deposit_"):TEXT("drop_"))+E.Key)];B->AddSlot().AutoHeight().Padding(0,5)[Row];}
    if(G->Screen==TEXT("stash"))for(const FString& Id:TArray<FString>{TEXT("ammo"),TEXT("shells"),TEXT("medkit"),TEXT("battery"),TEXT("bottle"),TEXT("boards")})B->AddSlot().AutoHeight().Padding(0,5)[Button(TEXT("Взять из шкафа · ")+G->ItemName(Id)+TEXT(" × ")+FString::FromInt(G->Stash.FindRef(Id)),TEXT("withdraw_")+Id,false,G->Stash.FindRef(Id)>0)];
    B->AddSlot().AutoHeight().Padding(0,12)[Text(TEXT("Оружие и припасы занимают 12 ячеек. Патроны: 24 в ячейке; дробь: 8. Документы хранятся отдельно. Оставленные припасы можно подобрать снова."))];return B;
}
TSharedRef<SWidget> FWardCampaignUI::Map()
{
    auto* G=Mode.Get();auto B=SNew(SVerticalBox);B->AddSlot().AutoHeight()[Text(TEXT("План отделения 04"),28,Teal)];
    if(!G->Flag(TEXT("map"))){B->AddSlot().AutoHeight()[Text(TEXT("Найди план в приёмной."))];return B;}
    B->AddSlot().AutoHeight()[Text(G->Objective(),22,Gold)];
    auto Floors=SNew(SHorizontalBox);Floors->AddSlot().AutoWidth()[Button(TEXT("Верхний этаж"),TEXT("map_upper"))];Floors->AddSlot().AutoWidth()[Button(TEXT("Нижний сектор"),TEXT("map_lower"))];B->AddSlot().AutoHeight()[Floors];
    B->AddSlot().AutoHeight()[Text(TEXT("Колесо мыши — масштаб · перетащи план мышью · ! — задача · S — сохранение"),14)];
    const float MapHeight=GEngine&&GEngine->GameViewport?FMath::Clamp(GEngine->GameViewport->Viewport->GetSizeXY().Y*.40f,250.f,550.f):400.f;
    B->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(MapHeight)[MakeWardCampaignMap(G)]];
    return B;
}
TSharedRef<SWidget> FWardCampaignUI::Puzzle()
{
    auto* G=Mode.Get();auto B=SNew(SVerticalBox);
    if(G->Screen==TEXT("keypad")){B->AddSlot().AutoHeight()[Text(TEXT("Карантин · допуск сотрудника"),28,Teal)];B->AddSlot().AutoHeight().Padding(0,12)[Text(TEXT("Проверь оборот пропуска: X → перевернуть."))];B->AddSlot().AutoHeight()[SNew(SEditableTextBox).Text(FText::FromString(PendingCode)).HintText(FText::FromString(TEXT("Код доступа"))).OnTextChanged_Lambda([this](const FText& T){PendingCode=T.ToString();})];B->AddSlot().AutoHeight().Padding(0,12)[Button(TEXT("Подтвердить"),TEXT("keypad_submit"),true)];}
    else if(G->Screen==TEXT("xray"))
    {B->AddSlot().AutoHeight()[Text(TEXT("Рентген · совмести экспозиции"),28,Teal)];B->AddSlot().AutoHeight().Padding(0,16)[Text(FString::Printf(TEXT("Смещение X: %.0f   Y: %.0f   Поворот: %d°"),FilmOffset.X,FilmOffset.Y,FilmTurn*90),24,Gold)];auto R=SNew(SHorizontalBox);for(const auto& E:TArray<TPair<FString,FString>>{{TEXT("←"),TEXT("film_left")},{TEXT("→"),TEXT("film_right")},{TEXT("↑"),TEXT("film_up")},{TEXT("↓"),TEXT("film_down")},{TEXT("Повернуть"),TEXT("film_turn")}})R->AddSlot().AutoWidth().Padding(3)[Button(E.Key,E.Value)];B->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(300)[MakeWardCampaignXray(this)]];B->AddSlot().AutoHeight()[Text(TEXT("Перетащи плёнку мышью. Совмести контуры и метки."),16)];B->AddSlot().AutoHeight()[R];B->AddSlot().AutoHeight().Padding(0,12)[Button(TEXT("Проверить совмещение"),TEXT("xray_submit"),true)];}
    else{B->AddSlot().AutoHeight()[Text(TEXT("Процедурная · протокол пациента 04"),28,Teal)];B->AddSlot().AutoHeight()[Text(TEXT("Воспроизведи последовательность из карты наблюдения. Ошибка привлекает противников."))];for(const auto& E:TArray<TPair<FString,FString>>{{TEXT("Охлаждение"),TEXT("cool")},{TEXT("Седативное"),TEXT("dose")},{TEXT("Разблокировка"),TEXT("release")}})B->AddSlot().AutoHeight().Padding(0,6)[Button(E.Key,TEXT("treatment_")+E.Value)];B->AddSlot().AutoHeight()[Text(FString::Printf(TEXT("Выполнено шагов: %d / 3"),Treatment.Num()),20,Gold)];}return B;
}
TSharedRef<SWidget> MakeWardMedia78(AEWardCampaignMode*,const FString&);
#include "WardAIDemo115.inl"

TSharedRef<SWidget> FWardCampaignUI::Build()
{
    if (Mode.IsValid() && Mode->Screen == TEXT("ai_demo")) return WardAIDemo115::Build(Mode);
    if(Mode.IsValid()&&Mode->Screen==WardSecurity103::Page)return WardSecurity103::Build(*Mode.Get());
    if(Mode.IsValid()&&WardOnline80::IsPage(Mode->Screen))return WardOnline80::Build(*Mode.Get());
    if(Mode.IsValid()&&WardAdmin79::Page(Mode->Screen))return WardAdmin79::Build(*Mode.Get());
    if(Mode.IsValid()&&(Mode->Screen==TEXT("trailer")||Mode->Screen==TEXT("screenshots")))return MakeWardMedia78(Mode.Get(),Mode->Screen);
    if(Mode.IsValid()&&Mode->Screen==TEXT("phone76"))return PhoneMenu76();
    if(Mode.IsValid()&&Mode->Screen==TEXT("story35"))return NarrativeMenu35();
    if(Mode.IsValid()&&(Mode->Screen==TEXT("quests")||Mode->Screen==TEXT("controls66")||Mode->Screen==TEXT("inventory")||Mode->Screen==TEXT("journal")||Mode->Screen==TEXT("map")||Mode->Screen==TEXT("people56")))return FieldMenu();
    if(Mode.IsValid()&&Mode->Screen==TEXT("pause"))return PauseMenu();
    if(Mode.IsValid()&&Mode->Screen==TEXT("difficulty"))return DifficultyMenu();
    if(Mode.IsValid()&&Mode->Screen==TEXT("settings"))return SettingsMenu();
    if(Mode.IsValid()&&Mode->Screen==TEXT("saves"))return SaveMenu();
    if(Mode.IsValid()&&Mode->Screen==TEXT("about"))return AboutMenu();
    if(Mode.IsValid()&&Mode->Screen==TEXT("training"))return IllustratedGuide(TEXT("training"));
    if(Mode.IsValid()&&Mode->Screen==TEXT("game"))return GameMenu();
    if(Mode.IsValid()&&Mode->Screen==TEXT("title"))return Title();
    if(Mode.IsValid()&&(Mode->Screen==TEXT("admin_catalog")||Mode->Screen==TEXT("admin_document")||Mode->Screen==TEXT("admin_audio")))return AdminCatalog51();
    auto* G=Mode.Get();auto Body=SNew(SVerticalBox);Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(TEXT("ЭХО"),56,FLinearColor(.88,.94,.9))];Body->AddSlot().AutoHeight().Padding(0,0,0,22)[Text(TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА"),17,Teal)];
    if(G->Screen==TEXT("title")||G->Screen==TEXT("pause"))
    {
        auto Row=SNew(SHorizontalBox);const TArray<TPair<FString,FString>> Menu={{TEXT("Игра"),TEXT("game")},{TEXT("Настройки"),TEXT("settings")},{TEXT("Обучение"),TEXT("training")},{TEXT("Об игре"),TEXT("about")},{TEXT("Карты · в разработке"),TEXT("maps")},{TEXT("Онлайн · в разработке"),TEXT("online")},{TEXT("Выйти"),TEXT("quit")}};
        for(const auto& E:Menu)Row->AddSlot().FillWidth(1).Padding(4)[Button(E.Key,TEXT("page_")+E.Value,E.Value==TEXT("game"),E.Value!=TEXT("maps")&&E.Value!=TEXT("online"))];Body->AddSlot().AutoHeight()[Row];
        Body->AddSlot().AutoHeight().Padding(0,50)[Text(TEXT("Останься в тишине."),38,Gold)];Body->AddSlot().AutoHeight()[Text(TEXT("Исследуй заброшенное отделение, раскрой судьбу пациентов и сохрани силы для последней двери."),22)];
        if(G->CampaignStarted)Body->AddSlot().AutoHeight().Padding(0,24)[Button(TEXT("Вернуться в игру"),TEXT("resume"),true)];
        if(G->Admin){Body->AddSlot().AutoHeight().Padding(0,6)[Button(G->Agent?TEXT("Остановить агента"):TEXT("Запустить агента прохождения"),TEXT("agent_toggle"))];Body->AddSlot().AutoHeight().Padding(0,6)[Button(TEXT("Вернуться к персонажу"),TEXT("admin_exit"))];}
    }
    else
    {
        Body->AddSlot().AutoHeight().Padding(0,0,0,16)[Button(TEXT("← Назад"),TEXT("back"))];TSharedRef<SWidget> Content=SNullWidget::NullWidget;
        if(G->Screen==TEXT("game")){auto B=SNew(SVerticalBox);B->AddSlot().AutoHeight().Padding(0,6)[Button(TEXT("Загрузить игру · последнее сохранение"),TEXT("latest"))];B->AddSlot().AutoHeight().Padding(0,6)[Button(TEXT("Новая игра"),TEXT("page_difficulty"),true)];B->AddSlot().AutoHeight().Padding(0,6)[Button(TEXT("Все сохранения"),TEXT("page_saves"))];Content=B;}
        else if(G->Screen==TEXT("difficulty")){auto B=SNew(SVerticalBox);const TArray<FString> Ids={TEXT("story"),TEXT("normal"),TEXT("nightmare"),TEXT("hardcore")},Names={TEXT("Легко"),TEXT("Нормально"),TEXT("Сложно"),TEXT("Хардкор")},Descriptions={TEXT("Для знакомства с историей: больше припасов, медленные противники."),TEXT("Для первого прохождения: исследование и осторожный бой."),TEXT("Для опытных игроков: быстрые противники, меньше патронов."),TEXT("Для тех, кто знает отделение: минимальный запас и короткая передышка.")};for(int32 I=0;I<4;++I){B->AddSlot().AutoHeight().Padding(0,6)[Button(Names[I],TEXT("new_")+Ids[I],I==1)];B->AddSlot().AutoHeight()[Text(Descriptions[I],14)];}Content=B;}
        else if(G->Screen==TEXT("trailer")||G->Screen==TEXT("screenshots"))Content=Media(G->Screen);
        else if(G->Screen==TEXT("settings"))Content=Settings();else if(G->Screen==TEXT("training")||G->Screen==TEXT("controls")||G->Screen==TEXT("about"))Content=Guide(G->Screen);
        else if(G->Screen==TEXT("saves"))Content=Saves();else if(G->Screen==TEXT("inventory")||G->Screen==TEXT("stash"))Content=Inventory();else if(G->Screen==TEXT("map"))Content=Map();
        else if(G->Screen==TEXT("keypad")||G->Screen==TEXT("xray")||G->Screen==TEXT("treatment"))Content=Puzzle();
        else if(G->Screen==TEXT("journal")){auto B=SNew(SVerticalBox);B->AddSlot().AutoHeight()[Button(TEXT("Повторить последнюю запись"),TEXT("replay"),false,!G->StringFlag(TEXT("last_story")).IsEmpty())];for(const FString& J:G->Journal)B->AddSlot().AutoHeight().Padding(0,12)[Text(J,20)];Content=B;}
        else if(G->Screen==TEXT("inspect"))Content=Inspect();
        else if(G->Screen==TEXT("ending")){auto B=SNew(SVerticalBox);B->AddSlot().AutoHeight()[Text(G->Ending==2?TEXT("Свидетель последней смены"):TEXT("Выживший"),34,Gold)];B->AddSlot().AutoHeight().Padding(0,18)[Text(FString::Printf(TEXT("Время: %02d:%02d · найдено: %d · противников: %d · точность: %.0f%%"),int(G->Elapsed)/60,int(G->Elapsed)%60,G->Completed.Num(),G->Kills,G->Shots?100.f*G->Hits/G->Shots:0),22)];B->AddSlot().AutoHeight()[Button(TEXT("Главное меню"),TEXT("page_title"))];Content=B;}
        Body->AddSlot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[Panel(Content)]];
    }
    Body->AddSlot().AutoHeight().Padding(0,12)[SNew(STextBlock).Text_Lambda([Weak=Mode](){return FText::FromString(Weak.IsValid()&&Weak->Hero()&&Weak->Hero()->NoticeTime>0?Weak->Hero()->Notice:TEXT(""));}).Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),15)).ColorAndOpacity(Gold).AutoWrapText(true)];
    if(G->TitleView)return TitleSection(Body);
    auto Content=MakeWardTitleFade(SNew(SBorder).BorderImage(&Back).Padding(FMargin(55,35))[Body],G);

    return SNew(SOverlay)+SOverlay::Slot()[Content]+SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([Weak=Mode](){return FLinearColor(0,0,0,Weak.IsValid()&&Weak->TitleScene?Weak->TitleScene->Fade:0);}).Visibility(EVisibility::HitTestInvisible)];
}
TSharedRef<SWidget> FWardCampaignUI::Inspect()
{
    if(Mode.IsValid()&&(Mode->Screen==TEXT("admin_catalog")||Mode->Screen==TEXT("admin_document")||Mode->Screen==TEXT("admin_audio")))return AdminCatalog51();
    auto* G=Mode.Get();auto Body=SNew(SVerticalBox);Body->AddSlot().AutoHeight()[Text(G->ItemName(SelectedItem),28,Teal)];
    if(SelectedItem==TEXT("badge")&&G->Items.FindRef(TEXT("badge"))>0)
    {
        if(!G->Flag(TEXT("badge_examined")))
        {
            for(const auto& V:Array(G->Data,TEXT("textures")))
            {
                Json T=V->AsObject();if(!String(T,TEXT("source")).EndsWith(TEXT("staff_badge.svg")))continue;const FString Id=String(T,TEXT("id"));
                if(auto* Texture=LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/Campaign/Textures/T_")+Id+TEXT(".T_")+Id))){auto Brush=MakeUnique<FSlateBrush>();Brush->SetResourceObject(Texture);Brush->ImageSize=FVector2D(280,400);auto* Raw=Brush.Get();ImageBrushes.Add(MoveTemp(Brush));Body->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,20)[SNew(SBox).WidthOverride(280).HeightOverride(400)[SNew(SImage).Image(Raw)]];}break;
            }
        }
        Body->AddSlot().AutoHeight().Padding(0,24)[Panel(Text(G->Flag(TEXT("badge_examined"))?TEXT("Оборот пропуска · КАРАНТИН / 417"):TEXT("Пропуск сотрудника. На обороте есть потёртая надпись."),26,Gold))];
        Body->AddSlot().AutoHeight()[Button(TEXT("Перевернуть предмет"),TEXT("inspect_turn"))];
    }
    else Body->AddSlot().AutoHeight().Padding(0,24)[Text(TEXT("Осмотри предмет и прочитай найденные записи."),22)];
    return Body;
}

