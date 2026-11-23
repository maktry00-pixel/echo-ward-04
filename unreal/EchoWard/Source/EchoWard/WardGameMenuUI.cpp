#include "WardCampaignUI.h"
#include "WardInterface66.h"
#include "WardCampaign.h"
#include "WardTitleScene.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/Paths.h"
#include "WardSettings.h"
#include "Widgets/Input/SSlider.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SEditableTextBox.h"
using namespace WardData;
namespace
{
    FSlateColorBrush GlassWhite(FLinearColor::White);
    const FSliderStyle& SettingsSliderStyle()
    {
        // CoreStyle is initialized by Slate; defer access until the UI is built.
        static const FSliderStyle Style=[](){auto S=FCoreStyle::Get().GetWidgetStyle<FSliderStyle>(TEXT("Slider"));S.SetNormalBarImage(FSlateColorBrush(FLinearColor::White));S.SetHoveredBarImage(FSlateColorBrush(FLinearColor::White));S.SetDisabledBarImage(FSlateColorBrush(FLinearColor::White));S.BarThickness=2;return S;}();
        return Style;
    }
    FButtonStyle GlassButton=[](){FButtonStyle S;S.SetNormal(FSlateColorBrush(FLinearColor::Transparent));S.SetHovered(FSlateColorBrush(FLinearColor::Transparent));S.SetPressed(FSlateColorBrush(FLinearColor(0,0,0,.15)));S.SetDisabled(FSlateColorBrush(FLinearColor::Transparent));return S;}();
    struct FGlassMotion {float Hover=0,Clock=0;};
    struct FGlassGrain
    {
        TStrongObjectPtr<UTexture2D> Texture;FSlateBrush Brush;
        explicit FGlassGrain(int Icon)
        {
            // Cache original procedural glass once; thousands of separate Slate boxes per
            // frame are expensive on the laptop. This is one brush draw per panel.
            const int W=1024,H=Icon==-7?768:256;Texture.Reset(UTexture2D::CreateTransient(W,H,PF_B8G8R8A8));Texture->SRGB=false;Texture->NeverStream=true;
            auto& Mip=Texture->GetPlatformData()->Mips[0];auto* Pixels=static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));FMemory::Memzero(Pixels,W*H*sizeof(FColor));FRandomStream R(491+Icon*37);
            auto Dot=[&](int X,int Y,uint8 A){if(X>=2&&X<W-2&&Y>=2&&Y<H-2){auto& P=Pixels[Y*W+X];P=FColor(255,255,255,FMath::Max(P.A,A));}};
            for(int I=0;I<(Icon==-7?18600:6200);++I){const int X=R.RandRange(3,W-4);int Y=R.RandRange(3,H-4);if(I%3==0)Y=I%2==0?R.RandRange(3,36):H-R.RandRange(3,36);const int E=FMath::Min(FMath::Min(X,W-X),FMath::Min(Y,H-Y));const uint8 A=R.RandRange(10,94)*(E<28?1:E<50?.4f:.09f);Dot(X,Y,A);if(I%4==0)Dot(X,Y+1,A);if(I%29==0&&E<45)for(int J=0;J<R.RandRange(3,14);++J)Dot(X+J,Y-J/3,A/2);}
            Mip.BulkData.Unlock();Texture->UpdateResource();Brush.SetResourceObject(Texture.Get());Brush.ImageSize=FVector2D(W,H);
        }
    };
    TMap<int,TUniquePtr<FGlassGrain>> GlassGrains;
    const FSlateBrush* Grain(int Icon){if(!GlassGrains.Contains(Icon))GlassGrains.Add(Icon,MakeUnique<FGlassGrain>(Icon));return &GlassGrains[Icon]->Brush;}
    /** Glass and distress are drawn over the live viewport, never a baked screenshot. */
    class SArchiveGlass : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SArchiveGlass){} SLATE_ARGUMENT(TSharedPtr<FGlassMotion>,Motion) SLATE_ARGUMENT(int32,Icon) SLATE_ARGUMENT(bool,Selected) SLATE_END_ARGS()
        void Construct(const FArguments& A){Motion=A._Motion;Icon=A._Icon;Selected=A._Selected;Noise=Grain(Icon);SetVisibility(EVisibility::HitTestInvisible);ForceVolatile(true);}
        virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1400,100);}
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool)const override
        {
            const FVector2D S=G.GetLocalSize();const float Alpha=Style.GetColorAndOpacityTint().A;
            const float E=Motion?Motion->Hover:0,Time=Motion?Motion->Clock:0;
            const FLinearColor Tint=FMath::Lerp(FLinearColor(.68,.27,.25),FLinearColor(1,.095,.065),E);
            auto Line=[&](TArray<FVector2D> P,FLinearColor C,float W=1){C.A*=Alpha;FSlateDrawElement::MakeLines(Out,Layer+2,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,W);};
            auto Box=[&](FVector2D P,FVector2D Size,FLinearColor C){C.A*=Alpha;FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(Size,FSlateLayoutTransform(P)),&GlassWhite,ESlateDrawEffect::None,C);};
            TArray<FSlateGradientStop> Stops;
            Stops.Emplace(FVector2D(0,0),FLinearColor(.025+E*.12,.006,.009,.68));Stops.Emplace(FVector2D(S.X*.46,0),FLinearColor(.039+E*.025,.010-E*.005,.013-E*.007,.32+E*.1));Stops.Emplace(FVector2D(S.X,0),FLinearColor(.053+E*.24,.013-E*.008,.017-E*.011,.6));for(auto& P:Stops)P.Color.A*=Alpha;FSlateDrawElement::MakeGradient(Out,Layer,G.ToPaintGeometry(),Stops,Orient_Vertical);
            const TArray<FVector2D> Frame={{5,1},{S.X-5,1},{S.X-1,5},{S.X-1,S.Y-5},{S.X-5,S.Y-1},{5,S.Y-1},{1,S.Y-5},{1,5},{5,1}};
            if(E>.01f)for(float W:{11.f,6.f,3.f})Line(Frame,FLinearColor(1,.04,.025,E*.026f*(.9f+.1f*FMath::Sin(Time*1.5f))),W);
            Line(Frame,FLinearColor(Tint.R,Tint.G,Tint.B,.52f+E*.38f),1.1f);Line({{7,3},{S.X-7,3}},FLinearColor(.82+E*.14,.37-E*.22,.33-E*.21,.28+E*.38),.7f);
            // Seeded fine flecks and short scratches, concentrated at the chipped glass edges.
            FRandomStream R(491+Icon*37);FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(),Noise,ESlateDrawEffect::None,FLinearColor(Tint.R,Tint.G,Tint.B,Alpha));
            if(Icon>0)for(int I=0;I<13;++I){const float X=R.FRandRange(9,S.X-9),Y=I%2?4:S.Y-4,D=I%2?1:-1;Line({{X,Y},{X+5,Y+D*7},{X+2,Y+D*12},{X+13,Y+D*19}},FLinearColor(Tint.R,Tint.G,Tint.B,.11f+E*.10f),.7f);}
            if(Icon>=0)
            {
                const float CY=S.Y/2;const FLinearColor C(Tint.R+.25f,Tint.G+.2f,Tint.B+.18f,.95);
                if(Icon==0)Line({{53,CY-9},{43,CY},{53,CY+9}},C,1.8);
                else
                {
                    Box(FVector2D(1,1),FVector2D(130,S.Y-2),FLinearColor(0,.006,.008,.20));Line({{132,2},{132,S.Y-2}},FLinearColor(Tint.R,Tint.G,Tint.B,.15));
                    if(Icon==1){Line({{69,CY-22},{69,CY+8},{58,CY-3}},C,2);Line({{69,CY+8},{80,CY-3}},C,2);Line({{51,CY+9},{51,CY+20},{87,CY+20},{87,CY+9}},C,2);}
                    if(Icon==2){Line({{55,CY-22},{76,CY-22},{87,CY-11},{87,CY+22},{55,CY+22},{55,CY-22}},C,1.8);Line({{76,CY-22},{76,CY-10},{87,CY-10}},C,1.5);}
                    if(Icon==3)for(int I=-1;I<=1;++I){Line({{62,CY+I*14},{90,CY+I*14}},C,1.8);Box(FVector2D(51,CY+I*14-1.4),FVector2D(3,3),C);}
                }
            }
            if(Icon==-2){if(E>.01f)Box(FVector2D(24,19),FVector2D(6,S.Y-38),FLinearColor(1,.06,.025,E*.13f));Box(FVector2D(26,19),FVector2D(2,S.Y-38),FLinearColor(Tint.R,Tint.G,Tint.B,.86));}
            if(Icon==-3){Line({{48,S.Y/2-12},{66,S.Y/2},{48,S.Y/2+12},{48,S.Y/2-12}},FLinearColor(Tint.R+.2f,Tint.G+.15f,Tint.B+.15f,.95),1.8f);Box(FVector2D(21,14),FVector2D(2,S.Y-28),FLinearColor(Tint.R,Tint.G,Tint.B,.8));}
            return Layer+2;
        }
        TSharedPtr<FGlassMotion> Motion;int32 Icon=-1;bool Selected=false;const FSlateBrush* Noise=nullptr;
    };
    class SArchiveRow : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SArchiveRow):_Enabled(true),_Compact(false),_Sans(false),_FontSize(0),_LeftAligned(false){} SLATE_ARGUMENT(FString,Label) SLATE_ARGUMENT(FString,Description) SLATE_ARGUMENT(FString,Action) SLATE_ARGUMENT(TWeakObjectPtr<AEWardCampaignMode>,Mode) SLATE_ARGUMENT(int32,Icon) SLATE_ARGUMENT(bool,Selected) SLATE_ARGUMENT(bool,Enabled) SLATE_ARGUMENT(bool,Compact) SLATE_ARGUMENT(bool,Sans) SLATE_ARGUMENT(int32,FontSize) SLATE_ARGUMENT(bool,LeftAligned) SLATE_END_ARGS()
        void Construct(const FArguments& A)
        {
            Mode=A._Mode;Motion=MakeShared<FGlassMotion>();FSlateFontInfo Font(FPaths::ProjectContentDir()/(A._Sans?TEXT("CampaignData/Fonts/NotoSans.ttf"):TEXT("CampaignData/Fonts/Literata.ttf")),A._Compact?16:25);Font.LetterSpacing=A._Compact?0:110;SetRenderOpacity(A._Enabled?1.f:.35f);
            if(!A._Description.IsEmpty()){Font.Size=31;Font.LetterSpacing=110;}
            if(A._FontSize>0)Font.Size=A._FontSize;
            auto Copy=SNew(SVerticalBox);Copy->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(A._Label)).Font(Font).AutoWrapText(true).ColorAndOpacity_Lambda([M=Motion](){return FMath::Lerp(FLinearColor(.58,.84,.85),FLinearColor(1,.77,.71),M->Hover);})];
            if(!A._Description.IsEmpty())Copy->AddSlot().AutoHeight().Padding(0,7,0,0)[SNew(STextBlock).Text(FText::FromString(A._Description)).Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),15)).ColorAndOpacity(FLinearColor(.48,.66,.67)).AutoWrapText(true)];
            ChildSlot[SNew(SOverlay)+SOverlay::Slot()[SNew(SArchiveGlass).Motion(Motion).Icon(A._Compact?-1:A._Icon)]+SOverlay::Slot()[SAssignNew(Button,SButton).IsEnabled(A._Enabled).ButtonStyle(&GlassButton).ContentPadding(0).OnHovered_Lambda([this](){if(Mode.IsValid()&&Mode->TitleScene)Mode->TitleScene->Hover();}).OnClicked_Lambda([Weak=Mode,Action=A._Action](){if(Weak.IsValid())Weak->Command(Action);return FReply::Handled();})[SAssignNew(LabelBox,SBox).Padding(A._Compact?FMargin(A._LeftAligned?22:10,0):!A._Description.IsEmpty()?FMargin(72,0,24,0):A._Icon==0||A._Icon==-3?FMargin(85,0,24,0):FMargin(162,0,24,0)).HAlign(A._Compact&&!A._LeftAligned?HAlign_Center:HAlign_Fill).VAlign(VAlign_Center)[Copy]]]];
        }
        virtual void Tick(const FGeometry& G,double T,float D)override{SCompoundWidget::Tick(G,T,D);Motion->Clock+=D;Motion->Hover=FMath::FInterpTo(Motion->Hover,Button->IsEnabled()&&Button->IsHovered()?1:0,D,10);LabelBox->SetRenderTransform(FSlateRenderTransform(FVector2D(Motion->Hover*4,0)));}
        TSharedPtr<SButton> Button;TSharedPtr<SBox> LabelBox;TSharedPtr<FGlassMotion> Motion;TWeakObjectPtr<AEWardCampaignMode> Mode;
    };
}
TSharedRef<SWidget> FWardCampaignUI::GameMenu()
{
    auto* G=Mode.Get();TitleActions.Reset();TitleMotion.Reset();auto Page=SNew(SOverlay);auto Stack=SNew(SVerticalBox);
    // Keep the resource rooted across paused-world garbage collections.
    static TStrongObjectPtr<UTexture2D> LogoTexture;static FSlateBrush Logo;
    if(!LogoTexture.IsValid()){LogoTexture.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Campaign/Menu/T_Menu_logo_echo")));if(LogoTexture.IsValid()){Logo.SetResourceObject(LogoTexture.Get());Logo.ImageSize=FVector2D(2158,729);}}
    Stack->AddSlot().AutoHeight().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(345).HeightOverride(117)[SNew(SImage).Image(&Logo).ColorAndOpacity(FLinearColor(.85,.94,.94))]];
    FSlateFontInfo Subtitle(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/Literata.ttf"),15);Subtitle.LetterSpacing=340;
    Stack->AddSlot().AutoHeight().Padding(4,17,0,35)[SNew(STextBlock).Text(FText::FromString(G->Tr(TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА")))).Font(Subtitle).ColorAndOpacity(FLinearColor(.90,.45,.40))];
    auto Row=[&](FString Label,FString Action,int Icon,bool Selected,float Height){auto R=SNew(SArchiveRow).Label(G->Tr(Label)).Action(Action).Mode(Mode).Icon(Icon).Selected(Selected);TitleActions.Add(Action,R->Button);TitleMotion.Add(Action,[M=R->Motion](){return M->Hover;});return SNew(SBox).HeightOverride(Height)[R];};
    Stack->AddSlot().AutoHeight()[Row(TEXT("Назад"),TEXT("back"),0,false,78)];
    auto Rows=SNew(SVerticalBox);Rows->AddSlot().AutoHeight()[Row(TEXT("Загрузить игру · последнее сохранение"),TEXT("latest"),1,false,104)];Rows->AddSlot().AutoHeight().Padding(0,15,0,0)[Row(TEXT("Новая игра"),TEXT("page_difficulty"),2,true,104)];Rows->AddSlot().AutoHeight().Padding(0,15,0,0)[Row(TEXT("Все сохранения"),TEXT("page_saves"),3,false,104)];
    Stack->AddSlot().AutoHeight().Padding(0,29,0,0)[SNew(SOverlay)+SOverlay::Slot()[SNew(SArchiveGlass).Icon(-1)]+SOverlay::Slot().Padding(32,24)[Rows]];
    Page->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(225,55,0,0)[SNew(SBox).WidthOverride(1470)[Stack]];
    return SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[MakeWardTitleFade(Page,G)]];
}

TSharedRef<SWidget> FWardCampaignUI::PauseMenu()
{
    auto* G=Mode.Get();TitleActions.Reset();TitleMotion.Reset();auto Stack=SNew(SVerticalBox);
    const FString Serif=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/Literata.ttf");const FLinearColor Ink(.77,.87,.86),Teal(.90,.45,.40);
    auto Label=[&](FString Value,int Size,FLinearColor Color){return SNew(STextBlock).Text(FText::FromString(G->Tr(Value))).Font(FSlateFontInfo(Serif,Size)).ColorAndOpacity(Color).AutoWrapText(true);};
    static TStrongObjectPtr<UTexture2D> Texture;static FSlateBrush Logo;
    if(!Texture.IsValid()){Texture.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Campaign/Menu/T_Menu_logo_echo")));if(Texture.IsValid()){Logo.SetResourceObject(Texture.Get());Logo.ImageSize=FVector2D(2158,729);}}
    Stack->AddSlot().AutoHeight().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(360).HeightOverride(122)[SNew(SImage).Image(&Logo)]];
    FSlateFontInfo Subtitle(Serif,15);Subtitle.LetterSpacing=300;
    Stack->AddSlot().AutoHeight().Padding(6,10,0,32)[SNew(STextBlock).Text(FText::FromString(G->Tr(TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА")))).Font(Subtitle).ColorAndOpacity(Teal)];
    auto Control=[&](FString Value,FString Action,bool Compact,bool Enabled=true){auto R=SNew(SArchiveRow).Label(G->Tr(Value)).Action(Action).Mode(Mode).Icon(-3).Compact(Compact).FontSize(Compact?20:29).LeftAligned(true).Enabled(Enabled);TitleActions.Add(Action,R->Button);TitleMotion.Add(Action,[M=R->Motion](){return M->Hover;});return R;};
    const TArray<TPair<FString,FString>> Entries={{TEXT("Главное меню"),TEXT("title")},{TEXT("Настройки"),TEXT("settings")},{TEXT("Обучение"),TEXT("training")},{TEXT("Об игре"),TEXT("about")},{TEXT("Выйти"),TEXT("quit")}};
    auto Nav=SNew(SHorizontalBox);for(const auto& E:Entries)Nav->AddSlot().FillWidth(1).Padding(0,0,E.Value==TEXT("quit")?0:10,0)[SNew(SBox).HeightOverride(88)[Control(E.Key,TEXT("page_")+E.Value,true,E.Value!=TEXT("maps")&&E.Value!=TEXT("online"))]];
    Stack->AddSlot().AutoHeight()[Nav];
    const FString Heading=G->Tr(TEXT("Останься в тишине."));FString First,Rest;Heading.Split(TEXT(" "),&First,&Rest);auto Head=SNew(SHorizontalBox);
    Head->AddSlot().AutoWidth()[Label(First.IsEmpty()?Heading:First,65,Ink)];Head->AddSlot().FillWidth(1).Padding(16,0,0,0)[Label(Rest,65,FLinearColor(.64,.09,.075))];
    Stack->AddSlot().AutoHeight().Padding(0,52,0,20)[Head];
    Stack->AddSlot().AutoHeight()[Label(TEXT("Исследуй заброшенное отделение, раскрой судьбу пациентов и сохрани силы для последней двери."),22,Ink)];
    Stack->AddSlot().AutoHeight().Padding(0,30,0,0)[SNew(SBox).HeightOverride(94)[Control(TEXT("Вернуться в игру"),TEXT("resume"),false)]];
    if(G->Admin){Stack->AddSlot().AutoHeight().Padding(0,12)[Control(TEXT("Панель администратора · главы, монстры и предметы"),TEXT("adm79_tools"),true)];auto Admin=SNew(SHorizontalBox);Admin->AddSlot().FillWidth(1).Padding(0,0,10,0)[SNew(SBox).HeightOverride(60)[Control(G->Agent?TEXT("Остановить агента"):TEXT("Запустить агента прохождения"),TEXT("agent_toggle"),true)]];Admin->AddSlot().FillWidth(1)[SNew(SBox).HeightOverride(60)[Control(TEXT("Вернуться к персонажу"),TEXT("admin_exit"),true)]];Stack->AddSlot().AutoHeight().Padding(0,22,0,0)[Admin];}
    auto Page=SNew(SOverlay);Page->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(125,36,125,0)[SNew(SBox).WidthOverride(1670)[Stack]];
    return SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[MakeWardTitleFade(Page,G)]];
}
TSharedRef<SWidget> FWardCampaignUI::AboutMenu(){return IllustratedGuide(TEXT("about"));}
TSharedRef<SWidget> FWardCampaignUI::DifficultyMenu()
{
    auto* G=Mode.Get();TitleActions.Reset();TitleMotion.Reset();auto Stack=SNew(SVerticalBox);
    static TStrongObjectPtr<UTexture2D> Texture;static FSlateBrush Logo;
    if(!Texture.IsValid()){Texture.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Campaign/Menu/T_Menu_logo_echo")));if(Texture.IsValid()){Logo.SetResourceObject(Texture.Get());Logo.ImageSize=FVector2D(2158,729);}}
    Stack->AddSlot().AutoHeight().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(360).HeightOverride(122)[SNew(SImage).Image(&Logo)]];
    FSlateFontInfo Font(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/Literata.ttf"),15);Font.LetterSpacing=300;
    Stack->AddSlot().AutoHeight().Padding(6,10,0,30)[SNew(STextBlock).Text(FText::FromString(G->Tr(TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА")))).Font(Font).ColorAndOpacity(FLinearColor(.90,.45,.40))];
    auto Row=[&](FString Name,FString Description,FString Action,int Icon){auto R=SNew(SArchiveRow).Label(G->Tr(Name)).Description(G->Tr(Description)).Action(Action).Mode(Mode).Icon(Icon);TitleActions.Add(Action,R->Button);TitleMotion.Add(Action,[M=R->Motion](){return M->Hover;});return R;};
    Stack->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(72)[Row(TEXT("Назад"),TEXT(""),TEXT("back"),0)]];
    const TArray<FString> Ids={TEXT("story"),TEXT("normal"),TEXT("nightmare"),TEXT("hardcore")},Names={TEXT("Легко"),TEXT("Нормально"),TEXT("Сложно"),TEXT("Хардкор")},Descriptions={TEXT("Для знакомства с историей: больше припасов, медленные противники."),TEXT("Для первого прохождения: исследование и осторожный бой."),TEXT("Для опытных игроков: быстрые противники, меньше патронов."),TEXT("Для тех, кто знает отделение: минимальный запас и короткая передышка.")};
    auto Rows=SNew(SVerticalBox);for(int I=0;I<4;++I)Rows->AddSlot().AutoHeight().Padding(0,I?16:0,0,0)[SNew(SBox).HeightOverride(124)[Row(Names[I],Descriptions[I],TEXT("new_")+Ids[I],-2)]];
    Stack->AddSlot().AutoHeight().Padding(0,23,0,0)[SNew(SOverlay)+SOverlay::Slot()[SNew(SArchiveGlass).Icon(-7)]+SOverlay::Slot().Padding(20,18)[Rows]];
    auto Page=SNew(SOverlay);Page->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(145,38,0,0)[SNew(SBox).WidthOverride(1125)[Stack]];
    return SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[MakeWardTitleFade(Page,G)]];
}
TSharedRef<SWidget> FWardCampaignUI::SettingsMenu()
{
    auto* G=Mode.Get();WardSettings::Complete(G->Options);TitleActions.Reset();TitleMotion.Reset();
    const FString Font=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf");const FLinearColor Ink(.78,.85,.84),Muted(.73,.47,.43);
    auto Label=[&](FString Value,int Size,FLinearColor Color){return SNew(STextBlock).Text(FText::FromString(G->Tr(Value))).Font(FSlateFontInfo(Font,Size)).ColorAndOpacity(Color).AutoWrapText(true);};
    auto Control=[&](FString Value,FString Action){auto R=SNew(SArchiveRow).Label(G->Tr(Value)).Action(Action).Mode(Mode).Compact(true).Sans(true);TitleActions.Add(Action,R->Button);TitleMotion.Add(Action,[M=R->Motion](){return M->Hover;});return R;};
    auto Stack=SNew(SVerticalBox);Stack->AddSlot().AutoHeight()[Label(TEXT("Настройки"),31,Ink)];
    auto Tabs=SNew(SHorizontalBox);const FString Group=Section==TEXT("audio")?TEXT("audio"):Section==TEXT("controls")?TEXT("controls"):Section==TEXT("interface")||Section==TEXT("admin")?TEXT("interface"):TEXT("graphics");
    const TArray<TPair<FString,FString>> Groups={{TEXT("graphics"),TEXT("Графика")},{TEXT("audio"),TEXT("Звук")},{TEXT("controls"),TEXT("Управление")},{TEXT("interface"),TEXT("Игра")}};
    for(const auto& E:Groups){auto Tab=SNew(SVerticalBox);Tab->AddSlot().AutoHeight()[SNew(SBox).WidthOverride(174).HeightOverride(43)[Control(E.Value,TEXT("section_")+E.Key)]];Tab->AddSlot().AutoHeight().Padding(15,4)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&GlassWhite).BorderBackgroundColor(E.Key==Group?Muted:FLinearColor::Transparent)]];Tabs->AddSlot().AutoWidth().Padding(0,0,8,0)[Tab];}
    Stack->AddSlot().AutoHeight().Padding(0,20,0,12)[Tabs];auto Rows=SNew(SVerticalBox);
    const TMap<FString,FString> Names={{TEXT("inspection_bright"),TEXT("Яркий осмотр карты · B")},{TEXT("resolution"),TEXT("Разрешение")},{TEXT("graphics_preset"),TEXT("Качество графики")},{TEXT("screen_percentage"),TEXT("Масштаб рендера")},{TEXT("motion_blur"),TEXT("Размытие в движении")},{TEXT("grain_amount"),TEXT("Зернистость плёнки")},{TEXT("brightness"),TEXT("Яркость")},{TEXT("fov"),TEXT("Поле зрения (FOV)")},{TEXT("fullscreen"),TEXT("Полноэкранный режим")},{TEXT("vsync"),TEXT("Вертикальная синхронизация")},{TEXT("max_fps"),TEXT("Ограничение кадров")},{TEXT("msaa"),TEXT("Сглаживание")},{TEXT("texture_quality"),TEXT("Качество текстур")},{TEXT("shadow_quality"),TEXT("Качество теней")},{TEXT("contrast"),TEXT("Контраст")},{TEXT("ambient"),TEXT("Освещение окружения")},{TEXT("fog"),TEXT("Плотность тумана")},{TEXT("vignette"),TEXT("Виньетка")},{TEXT("music"),TEXT("Музыка и атмосфера")},{TEXT("effects"),TEXT("Громкость эффектов")},{TEXT("subtitles"),TEXT("Субтитры")},{TEXT("language"),TEXT("Язык / Тіл / Language")},{TEXT("hud_opacity"),TEXT("Прозрачность интерфейса")},{TEXT("crosshair"),TEXT("Прицел")},{TEXT("objective_hint"),TEXT("Подсказки задачи")},{TEXT("show_noise"),TEXT("Индикатор шума")},{TEXT("sensitivity"),TEXT("Чувствительность мыши")},{TEXT("invert_y"),TEXT("Инверсия мыши")},{TEXT("shake"),TEXT("Отдача камеры")},{TEXT("camera_motion"),TEXT("Плавность камеры")},{TEXT("flashlight_strength"),TEXT("Сила фонаря")}};
    TArray<FString> Keys;TSet<FString> Sliders={TEXT("grain_amount"),TEXT("brightness"),TEXT("fov"),TEXT("contrast"),TEXT("ambient"),TEXT("fog"),TEXT("music"),TEXT("effects"),TEXT("hud_opacity"),TEXT("sensitivity"),TEXT("shake"),TEXT("flashlight_strength")};
    if(Group==TEXT("graphics"))Keys={TEXT("resolution"),TEXT("graphics_preset"),TEXT("screen_percentage"),TEXT("motion_blur"),TEXT("grain_amount"),TEXT("brightness"),TEXT("fov"),TEXT("fullscreen"),TEXT("vsync"),TEXT("max_fps"),TEXT("msaa"),TEXT("texture_quality"),TEXT("shadow_quality"),TEXT("contrast"),TEXT("ambient"),TEXT("fog"),TEXT("vignette"),TEXT("flashlight_strength")};
    else if(Group==TEXT("audio"))Keys={TEXT("music"),TEXT("effects"),TEXT("subtitles")};
    else if(Group==TEXT("controls"))Keys={TEXT("sensitivity"),TEXT("invert_y"),TEXT("shake"),TEXT("camera_motion")};
    else Keys={TEXT("language"),TEXT("hud_opacity"),TEXT("crosshair"),TEXT("objective_hint"),TEXT("show_noise")};
    if(Group==TEXT("graphics")&&G->BookWorldEnabled&&G->Admin)Keys.Insert(TEXT("inspection_bright"),0);
    auto Value=[G](const FString& Key){auto V=G->Options->TryGetField(Key);if(!V)return FString(TEXT("—"));if(V->Type==EJson::Boolean)return G->Tr(V->AsBool()?TEXT("Вкл"):TEXT("Выкл"));if(Key==TEXT("graphics_preset")){const TCHAR* C[]={TEXT("Высокое"),TEXT("Сбалансированное"),TEXT("Производительность")};return G->Tr(C[FMath::Clamp(int(V->AsNumber()),0,2)]);}if(Key==TEXT("msaa")){const TCHAR* C[]={TEXT("Выкл"),TEXT("FXAA"),TEXT("TAA"),TEXT("TSR")};return G->Tr(C[FMath::Clamp(int(V->AsNumber()),0,3)]);}if(Key==TEXT("max_fps"))return V->AsNumber()==0?G->Tr(TEXT("Без ограничений")):FString::FromInt(V->AsNumber());if(Key==TEXT("screen_percentage"))return FString::FromInt(V->AsNumber())+TEXT(" %");if(V->Type==EJson::String){const FString S=V->AsString();return S==TEXT("high")?G->Tr(TEXT("Высокое")):S==TEXT("standard")?G->Tr(TEXT("Сбалансированное")):S==TEXT("medium")?G->Tr(TEXT("Среднее")):S==TEXT("off")?G->Tr(TEXT("Выкл")):S==TEXT("ru")?FString(TEXT("Русский")):S==TEXT("kk")?FString(TEXT("Қазақша")):S==TEXT("en")?FString(TEXT("English")):S.Replace(TEXT("x"),TEXT(" × "));}return FString::SanitizeFloat(V->AsNumber());};
    for(const FString& Key:Keys)
    {
        auto Row=SNew(SHorizontalBox);Row->AddSlot().FillWidth(1).VAlign(VAlign_Center).Padding(10,0,15,0)[Label(Names.FindRef(Key),16,Ink)];
        if(Sliders.Contains(Key))
        {
            const auto R=WardSettings::Range(Key);auto Slider=SNew(SSlider).Style(&SettingsSliderStyle()).Value(FMath::Clamp((float(G->Setting(Key))-R.Min)/(R.Max-R.Min),0.f,1.f)).StepSize(R.Step/(R.Max-R.Min)).SliderBarColor(FLinearColor(.42,.57,.57)).SliderHandleColor(Ink).OnValueChanged_Lambda([Weak=Mode,Key,R](float V){if(Weak.IsValid())Weak->SetNumberSetting(Key,FMath::Lerp(R.Min,R.Max,V),false);}).OnMouseCaptureEnd_Lambda([Weak=Mode](){if(Weak.IsValid())Weak->ApplySettings();}).OnControllerCaptureEnd_Lambda([Weak=Mode](){if(Weak.IsValid())Weak->ApplySettings();});
            TitleActions.Add(TEXT("slider_")+Key,Slider);Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(284)[Slider]];
            Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(20,0,8,0)[SNew(SBox).WidthOverride(66)[SNew(STextBlock).Text_Lambda([Weak=Mode,Key](){if(!Weak.IsValid())return FText();const double V=Weak->Setting(Key);return FText::FromString(Key==TEXT("fov")?FString::Printf(TEXT("%.0f°"),V):FString::Printf(TEXT("%.2f"),V));}).Font(FSlateFontInfo(Font,14)).ColorAndOpacity(Muted)]];
        }
        else
        {
            Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(38).HeightOverride(35)[Control(TEXT("‹"),TEXT("setting_prev_")+Key)]];
            Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(6,0)[SNew(SBox).WidthOverride(282).HeightOverride(35)[Control(Value(Key),TEXT("setting_")+Key)]];
            Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(38).HeightOverride(35)[Control(TEXT("›"),TEXT("setting_")+Key)]];
        }
        Rows->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(47)[Row]];
    }
    if(Group==TEXT("controls")){Rows->AddSlot().AutoHeight().Padding(10,25)[Label(WardInterface66::Controls(G->Admin),17,Ink)];}
    if(Group==TEXT("interface"))
    {
        Rows->AddSlot().AutoHeight().Padding(10,26,0,12)[Label(TEXT("Режим администратора"),20,Ink)];auto Password=SNew(SEditableTextBox).IsPassword(true).HintText(FText::FromString(G->Tr(TEXT("Пароль")))).OnTextChanged_Lambda([this](const FText& T){PendingCode=T.ToString();});TitleActions.Add(TEXT("admin_password"),Password);
        Rows->AddSlot().AutoHeight().Padding(10,0)[SNew(SBox).HeightOverride(38)[Password]];Rows->AddSlot().AutoHeight().Padding(10,12)[SNew(SBox).HeightOverride(42)[Control(TEXT("Включить режим администратора"),TEXT("admin_enter"))]];
        Rows->AddSlot().AutoHeight().Padding(10,0)[SNew(STextBlock).Text_Lambda([Weak=Mode](){if(!Weak.IsValid()||!Weak->Hero())return FText();const FString Error=Weak->Tr(TEXT("Неверный пароль"));return Weak->Hero()->Notice==Error||Weak->Hero()->Notice==TEXT("Неверный пароль")?FText::FromString(Error):FText();}).Font(FSlateFontInfo(Font,14)).ColorAndOpacity(Ink)];
    }
    auto Scroll=SNew(SScrollBox).ScrollBarThickness(FVector2D(3,3)).ScrollBarPadding(FMargin(12,0,0,0));Scroll->AddSlot()[Rows];Stack->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(535)[Scroll]];
    auto Footer=SNew(SHorizontalBox);Footer->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(420).HeightOverride(42)[Control(TEXT("Сбросить настройки"),TEXT("settings_reset"))]];Stack->AddSlot().AutoHeight().Padding(10,18,10,0)[Footer];
    auto Page=SNew(SOverlay);auto Outer=SNew(SVerticalBox);Outer->AddSlot().AutoHeight().Padding(0,0,0,18)[SNew(SBox).HeightOverride(47)[Control(TEXT("Назад"),TEXT("back"))]];Outer->AddSlot().AutoHeight()[SNew(SOverlay)+SOverlay::Slot()[SNew(SArchiveGlass).Icon(-7)]+SOverlay::Slot().Padding(28,25)[Stack]];
    Page->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(125,100,0,0)[SNew(SBox).WidthOverride(1090)[Outer]];
    return SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[MakeWardTitleFade(Page,G)]];
}
TSharedRef<SWidget> FWardCampaignUI::IllustratedGuide(const FString& Kind)
{
    auto* G=Mode.Get();TitleActions.Reset();TitleMotion.Reset();const bool Training=Kind==TEXT("training");const auto& Pages=Array(G->Data,Kind);
    if(Pages.IsEmpty())return Text(TEXT("Материалы загружаются"));GuidePage=FMath::Clamp(GuidePage,0,Pages.Num()-1);const Json Data=Pages[GuidePage]->AsObject();
    const FString Serif=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/Literata.ttf");
    auto Label=[&](FString Value,int Size,FLinearColor Color,float Spacing=0){FSlateFontInfo Font(Serif,Size);Font.LetterSpacing=Spacing;FString Localized=G->Tr(Value);if(G->BookWorld){Localized.ReplaceInline(TEXT("Фонарь B"),TEXT("Фонарь N"));Localized.ReplaceInline(TEXT("Flashlight B"),TEXT("Flashlight N"));if(Value==TEXT("B"))Localized=TEXT("N");}if(Training)Localized.ReplaceInline(TEXT("Tab"),TEXT("M"),ESearchCase::CaseSensitive);return SNew(STextBlock).Text(FText::FromString(Localized)).Font(Font).ColorAndOpacity(Color).WrapTextAt(Size>=30||Size==19?700:1540).AutoWrapText(false);};
    const FLinearColor White(.77,.87,.86),Teal(.92,.48,.41);
    auto Control=[&](FString Label,FString Action,bool Compact){auto R=SNew(SArchiveRow).Label(G->Tr(Label)).Action(Action).Mode(Mode).Icon(0).Compact(Compact);TitleActions.Add(Action,R->Button);TitleMotion.Add(Action,[M=R->Motion](){return M->Hover;});return R;};
    static TStrongObjectPtr<UTexture2D> LogoTexture;static FSlateBrush Logo;
    if(!LogoTexture.IsValid()){LogoTexture.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Campaign/Menu/T_Menu_logo_echo")));if(LogoTexture.IsValid()){Logo.SetResourceObject(LogoTexture.Get());Logo.ImageSize=FVector2D(2158,729);}}
    auto Stack=SNew(SVerticalBox);Stack->AddSlot().AutoHeight().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(360).HeightOverride(122)[SNew(SImage).Image(&Logo).ColorAndOpacity(FLinearColor(.88,.95,.95))]];
    Stack->AddSlot().AutoHeight().Padding(6,9,0,20)[Label(TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА"),15,Teal,300)];
    Stack->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(70)[Control(TEXT("Назад"),TEXT("back"),false)]];
    auto Content=SNew(SVerticalBox);auto Tabs=SNew(SHorizontalBox);
    for(int I=0;I<Pages.Num();++I){const FString Id=TEXT("guide_")+FString::FromInt(I);auto Tab=SNew(SVerticalBox);Tab->AddSlot().AutoHeight()[SNew(SBox).WidthOverride(Training?72:94).HeightOverride(63)[Control(FString::Printf(TEXT("%02d"),I+1),Id,true)]];
        // Current tab gets a quiet cyan underline; red belongs exclusively to hover.
        Tab->AddSlot().AutoHeight().Padding(15,5)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&GlassWhite).BorderBackgroundColor(I==GuidePage?Teal:FLinearColor::Transparent)]];
        Tabs->AddSlot().AutoWidth().Padding(0,0,Training?8:16,0)[Tab];}
    if(!Training)Content->AddSlot().AutoHeight()[Tabs];
    const auto& Details=Array(Data,TEXT("details"));auto Columns=SNew(SHorizontalBox);auto Copy=SNew(SVerticalBox);if(Training)Copy->AddSlot().AutoHeight()[Tabs];
    Copy->AddSlot().AutoHeight().Padding(0,16,0,12)[Label(String(Data,TEXT("title")),GuidePage==0?49:38,White)];
    Copy->AddSlot().AutoHeight().Padding(0,0,30,24)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&GlassWhite).BorderBackgroundColor(FLinearColor(.62,.22,.20,.6))]];
    Copy->AddSlot().AutoHeight().Padding(0,0,25,20)[Label(String(Data,TEXT("intro")),19,White,!Training&&GuidePage==0?150:0)];
    if (!Training && GuidePage == 2)
    {
        const TCHAR* Developer = G->Language == TEXT("kk") ? TEXT("Әзірлеуші")
            : G->Language == TEXT("en") ? TEXT("Developer") : TEXT("Разработчик");
        Copy->AddSlot().AutoHeight().Padding(0, 0, 25, 14)
        [Label(FString(Developer) + TEXT(" · QAISAR STUDIO"), 22, Teal)];
    }
    if(!Training&&Details.Num()){const auto& Pair=Details[0]->AsArray();if(Pair.Num()>1){Copy->AddSlot().AutoHeight().Padding(0,0,25,8)[Label(Pair[0]->AsString(),24,Teal)];Copy->AddSlot().AutoHeight().Padding(0,0,25,0)[Label(Pair[1]->AsString(),20,White)];}}
    Columns->AddSlot().FillWidth(.47f).VAlign(VAlign_Top)[Copy];
    struct FAboutImage {TStrongObjectPtr<UTexture2D> Texture;FSlateBrush Brush;};static TMap<FString,TUniquePtr<FAboutImage>> Images;
    const FString ImageId=String(Data,TEXT("image")),CacheId=Kind+TEXT("_")+ImageId;if(!Images.Contains(CacheId)){auto Image=MakeUnique<FAboutImage>();const FString Path=TEXT("/Game/Campaign/Guide/T_Guide_")+ImageId;Image->Texture.Reset(LoadObject<UTexture2D>(nullptr,*Path));if(!Image->Texture.IsValid())Image->Texture.Reset(LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/Campaign/Guide/T_Guide_")+ImageId)));if(Image->Texture.IsValid()){Image->Brush.SetResourceObject(Image->Texture.Get());Image->Brush.ImageSize=FVector2D(1920,1080);}Images.Add(CacheId,MoveTemp(Image));}
    Columns->AddSlot().FillWidth(.53f).VAlign(VAlign_Top).Padding(20,Training?0:18,0,0)[SNew(SBox).HeightOverride(Training?332:361).Clipping(EWidgetClipping::ClipToBounds)[SNew(SOverlay)+SOverlay::Slot()[SNew(SArchiveGlass).Icon(-1)]+SOverlay::Slot().Padding(3)[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&Images[CacheId]->Brush)]]]];
    Content->AddSlot().AutoHeight().Padding(0,8,0,16)[Columns];
    for(int I=Training?0:1;I<Details.Num();++I){const auto& Pair=Details[I]->AsArray();if(Pair.Num()<2)continue;Content->AddSlot().AutoHeight().Padding(0,Training?6:14,0,Training?6:14)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&GlassWhite).BorderBackgroundColor(FLinearColor(.52,.17,.15,.4))]];Content->AddSlot().AutoHeight().Padding(0,0,0,Training?4:8)[Label(Pair[0]->AsString(),Training?20:25,Teal)];Content->AddSlot().AutoHeight()[Label(Pair[1]->AsString(),Training?16:20,White)];}
    if(Training){auto Keys=SNew(SHorizontalBox);for(const auto& V:Array(Data,TEXT("keys"))){const auto& Pair=V->AsArray();if(Pair.Num()<2)continue;FString Key=Pair[0]->AsString();if(Key==TEXT("Tab"))Key=TEXT("M");Keys->AddSlot().FillWidth(1).Padding(10,5)[Label(Key+TEXT(" · ")+G->Tr(Pair[1]->AsString()),13,Teal)];}Content->AddSlot().AutoHeight().Padding(0,16,0,0)[Keys];}
    auto Scroll=SNew(SScrollBox).ScrollBarThickness(FVector2D(3,3)).ScrollBarPadding(FMargin(10,0,0,0));Scroll->AddSlot()[Content];
    Stack->AddSlot().AutoHeight().Padding(0,18,0,0)[SNew(SBox).HeightOverride(Training?737:705)[SNew(SOverlay)+SOverlay::Slot()[SNew(SArchiveGlass).Icon(-7)]+SOverlay::Slot().Padding(45,24,30,26)[Scroll]]];
    auto Page=SNew(SOverlay);Page->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(125,31,0,0)[SNew(SBox).WidthOverride(1670)[Stack]];
    return SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[MakeWardTitleFade(Page,G)]];
}

namespace
{
    class SSaveGlyph : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SSaveGlyph){} SLATE_ARGUMENT(bool,Automatic) SLATE_END_ARGS()
        void Construct(const FArguments& A){Automatic=A._Automatic;SetVisibility(EVisibility::HitTestInvisible);}
        virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(28,30);}
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool)const override
        {
            auto Line=[&](TArray<FVector2D> P){FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,FLinearColor(.94,.62,.56,Style.GetColorAndOpacityTint().A),true,1.2);};
            if(Automatic){TArray<FVector2D> Gear;for(int I=0;I<=32;++I){const float A=I*2*PI/32,R=I%4<2?11:8.4f;Gear.Add(FVector2D(14+FMath::Cos(A)*R,15+FMath::Sin(A)*R));}Line(Gear);}
            else{Line({{7,3},{18,3},{24,9},{24,27},{7,27},{7,3}});Line({{18,3},{18,10},{24,10}});}return Layer;
        }
        bool Automatic=false;
    };
}
TSharedRef<SWidget> FWardCampaignUI::SaveMenu()
{
    auto* G=Mode.Get();TitleActions.Reset();TitleMotion.Reset();auto Page=SNew(SOverlay);auto Stack=SNew(SVerticalBox);
    static TStrongObjectPtr<UTexture2D> LogoTexture;static FSlateBrush Logo;
    if(!LogoTexture.IsValid()){LogoTexture.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Campaign/Menu/T_Menu_logo_echo")));if(LogoTexture.IsValid()){Logo.SetResourceObject(LogoTexture.Get());Logo.ImageSize=FVector2D(2158,729);}}
    const FString Serif=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/Literata.ttf");
    auto Label=[&](FString Value,int Size,FLinearColor Color){return SNew(STextBlock).Text(FText::FromString(G->Tr(Value))).Font(FSlateFontInfo(Serif,Size)).ColorAndOpacity(Color).OverflowPolicy(ETextOverflowPolicy::Ellipsis);};
    auto Control=[&](FString Text,FString Action,bool Enabled,bool Compact){auto C=SNew(SArchiveRow).Label(G->Tr(Text)).Action(Action).Mode(Mode).Icon(0).Enabled(Enabled).Compact(Compact);TitleActions.Add(Action,C->Button);TitleMotion.Add(Action,[M=C->Motion](){return M->Hover;});return C;};
    Stack->AddSlot().AutoHeight().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(310).HeightOverride(105)[SNew(SImage).Image(&Logo).ColorAndOpacity(FLinearColor(.86,.93,.93))]];
    FSlateFontInfo Sub(Serif,13);Sub.LetterSpacing=320;
    Stack->AddSlot().AutoHeight().Padding(4,7,0,20)[SNew(STextBlock).Text(FText::FromString(G->Tr(TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА")))).Font(Sub).ColorAndOpacity(FLinearColor(.90,.45,.40))];
    Stack->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(64)[Control(TEXT("Назад"),TEXT("back"),true,false)]];
    auto Inside=SNew(SVerticalBox);FSlateFontInfo Heading(Serif,20);Heading.LetterSpacing=180;
    Inside->AddSlot().AutoHeight().Padding(9,0,0,18)[SNew(STextBlock).Text(FText::FromString(G->Tr(TEXT("Шесть ручных слотов · три автоматические контрольные точки")))).Font(Heading).ColorAndOpacity(FLinearColor(.53,.81,.82))];
    auto List=SNew(SVerticalBox);
    for(const FString& Slot:G->SaveSlots())
    {
        const bool Manual=Slot.StartsWith(TEXT("manual_"));const Json S=G->ReadSlot(Slot);const bool Valid=G->ValidateSave(S);
        const FString Room=String(S,TEXT("room")),Date=String(S,TEXT("saved_at"));const FString Details=Valid?(Room.IsEmpty()?Date:Room+TEXT(" · ")+Date):G->Tr(TEXT("Пустой слот"));
        if(Slot==TEXT("auto_1"))List->AddSlot().AutoHeight().Padding(0,14,0,0)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&GlassWhite).BorderBackgroundColor(FLinearColor(.50,.15,.14,.3))]];
        auto Row=SNew(SHorizontalBox);
        Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(16,0,16,0)[SNew(SSaveGlyph).Automatic(!Manual)];
        Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(138)[Label(Slot,16,FLinearColor(.93,.67,.60))]];
        Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,28,0)[SNew(SBox).WidthOverride(1).HeightOverride(26)[SNew(SBorder).BorderImage(&GlassWhite).BorderBackgroundColor(FLinearColor(.34,.53,.53,.45))]];
        Row->AddSlot().FillWidth(1).VAlign(VAlign_Center).Padding(0,0,12,0)[Label(Details,15,Valid?FLinearColor(.64,.74,.73):FLinearColor(.36,.46,.45))];
        Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)[SNew(SBox).WidthOverride(153).HeightOverride(46)[Control(TEXT("Загрузить"),TEXT("load_")+Slot,Valid,true)]];
        if(Manual)Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)[SNew(SBox).WidthOverride(153).HeightOverride(46)[Control(TEXT("Сохранить"),TEXT("save_")+Slot,G->CampaignStarted,true)]];
        auto Framed=SNew(SOverlay)+SOverlay::Slot()[SNew(SArchiveGlass).Icon(-1)]+SOverlay::Slot()[Row];
        List->AddSlot().AutoHeight().Padding(0,0,0,2)[SNew(SBox).HeightOverride(61)[Framed]];
    }
    auto Scroll=SNew(SScrollBox).ScrollBarThickness(FVector2D(3,3)).ScrollBarPadding(FMargin(9,0,0,0)).ScrollBarVisibility(EVisibility::Visible);Scroll->AddSlot()[List];
    Inside->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(603)[Scroll]];
    // Feedback is restricted to saving/loading results; never show the gameplay hint here.
    Inside->AddSlot().AutoHeight().Padding(10,8,0,0)[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(SaveFeedback);}).Font(FSlateFontInfo(Serif,12)).ColorAndOpacity(FLinearColor(.53,.77,.74))];
    Stack->AddSlot().AutoHeight().Padding(0,18,0,0)[SNew(SOverlay)+SOverlay::Slot()[SNew(SArchiveGlass).Icon(-7)]+SOverlay::Slot().Padding(28,24)[Inside]];
    Page->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(235,26,0,0)[SNew(SBox).WidthOverride(1450)[Stack]];
    auto Faded=MakeWardTitleFade(Page,G);
    return SNew(SOverlay)+SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Faded]]]+SOverlay::Slot()[SNew(SBorder).BorderImage(&GlassWhite).BorderBackgroundColor_Lambda([Weak=Mode](){return FLinearColor(0,0,0,Weak.IsValid()&&Weak->TitleScene?Weak->TitleScene->Fade:0);}).Visibility(EVisibility::HitTestInvisible)];
}
