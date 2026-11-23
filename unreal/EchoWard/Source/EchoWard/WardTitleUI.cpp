#include "WardMedia78.inl"
#include "WardCampaignUI.h"
#include "WardCampaign.h"
#include "WardTitleScene.h"
#include "WardTitleMenuStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "Input/Reply.h"
#include "UObject/StrongObjectPtr.h"
namespace
{
    FSlateColorBrush White(FLinearColor::White);
    struct FMenuImage
    {
        explicit FMenuImage(UTexture2D* InTexture):Texture(InTexture){Brush.SetResourceObject(InTexture);Brush.ImageSize=FVector2D(InTexture->GetSizeX(),InTexture->GetSizeY());}
        TStrongObjectPtr<UTexture2D> Texture;
        FSlateBrush Brush;
    };
    TMap<FString,TUniquePtr<FMenuImage>> MenuImages;
    FButtonStyle MenuStyle=[](){FButtonStyle S;S.SetNormal(FSlateColorBrush(FLinearColor::Transparent));S.SetHovered(FSlateColorBrush(FLinearColor::Transparent));S.SetPressed(FSlateColorBrush(FLinearColor::Transparent));S.SetDisabled(FSlateColorBrush(FLinearColor::Transparent));return S;}();
    struct FMenuMotion {float Blend=0,Clock=0;bool Mouse=false;};
    class SMenuChrome : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SMenuChrome):_Compact(false){} SLATE_ARGUMENT(TSharedPtr<FMenuMotion>,Motion) SLATE_ARGUMENT(bool,Card) SLATE_ARGUMENT(bool,Selected) SLATE_ARGUMENT(bool,Compact) SLATE_END_ARGS()
        void Construct(const FArguments& A){Motion=A._Motion;Card=A._Card;Selected=A._Selected;Compact=A._Compact;ForceVolatile(true);SetVisibility(EVisibility::HitTestInvisible);}
        virtual FVector2D ComputeDesiredSize(float)const override{return Compact?FVector2D(0,42):Card?FVector2D(400,174):FVector2D(426,56);}
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle& Style,bool)const override
        {
            const FVector2D S=G.GetLocalSize();const float H=Motion->Blend,A=Style.GetColorAndOpacityTint().A;
            auto Line=[&](TArray<FVector2D> P,FLinearColor C,float W){C.A*=A;FSlateDrawElement::MakeLines(Out,L+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,W);};
            if(Card)
            {
                const float Bottom=S.Y-25,CenterY=Bottom*.5f;Line({{1,1},{S.X-1,1},{S.X-1,Bottom},{1,Bottom},{1,1}},FLinearColor(.55+H*.35,.53-H*.5,.50-H*.47,.55+H*.45),1);
                for(float X:{1.f,float(S.X-1)}){const float D=X<2?9:-9;auto C=FMath::Lerp(FLinearColor(.55,.53,.50),FLinearColor(.85,.035,.028),H);C.A=.26+H*.6;Line({{X,10},{X,1},{X+D,1}},C,1.2);}
                if(Selected){const float Icon=.60f+H*.4f;TArray<FVector2D> Ring;for(int I=0;I<=40;++I){const float T=I*2*PI/40;Ring.Add(FVector2D(S.X/2+FMath::Cos(T)*(27+H*2),CenterY+FMath::Sin(T)*(27+H*2)));}Line(Ring,FLinearColor(.9,.87,.8,Icon*.85),1.2);Line({{S.X/2-5,CenterY-8},{S.X/2+8,CenterY},{S.X/2-5,CenterY+8},{S.X/2-5,CenterY-8}},FLinearColor(.95,.92,.85,Icon),1.7);}
            }
            else
            {
                const float Pulse=.92f+.08f*FMath::Sin(Motion->Clock*1.7f),Energy=H,Glow=FMath::Clamp(Energy*Pulse,0.f,1.f)*A;
                TArray<FSlateGradientStop> Stops;Stops.Emplace(FVector2D(0,0),FLinearColor(.70,.008,.017,Glow));Stops.Emplace(FVector2D(S.X*.32,0),FLinearColor(.43,.005,.009,Glow*.9f));Stops.Emplace(FVector2D(S.X*.76,0),FLinearColor(.19,.001,.003,Glow*.48f));Stops.Emplace(FVector2D(S.X,0),FLinearColor::Transparent);FSlateDrawElement::MakeGradient(Out,L,G.ToPaintGeometry(),Stops,Orient_Vertical);
                if(Energy>.01f)
                {
                    const float Sweep=(.5f+.5f*FMath::Sin(Motion->Clock*.32f))*S.X*.7f;
                    for(int32 I=0;I<19;++I){const float Y=3+I*(S.Y-6)/19.f,X=18+FMath::Fmod(I*79.3f,230.f),W=28+FMath::Fmod(I*31.7f,90.f);Line({{X,Y},{FMath::Min(float(S.X-6),X+W),Y}},FLinearColor(.9,.025,.03,Energy*(.018f+.014f*FMath::Sin(Motion->Clock*.8f+I))),.7f);}
                    TArray<FSlateGradientStop> Sheen;Sheen.Emplace(FVector2D(FMath::Max(0.f,Sweep-55),0),FLinearColor::Transparent);Sheen.Emplace(FVector2D(Sweep,0),FLinearColor(.9,.025,.035,Glow*.085f));Sheen.Emplace(FVector2D(Sweep+65,0),FLinearColor::Transparent);FSlateDrawElement::MakeGradient(Out,L,G.ToPaintGeometry(),Sheen,Orient_Vertical);
                    Line({{12,9},{12,S.Y-9}},FLinearColor(1,.015,.025,Energy*.22f),6);
                }
                Line({{12,9},{12,S.Y-9}},H>.05?FLinearColor(.95,.85,.78,.95):FLinearColor(.78,.79,.74,.68),1.5f);
                if(H>.05)Line({{12,11},{12,S.Y-11}},FLinearColor(.9,.015,.02,H*.16),7);
            }
            return L+1;
        }
        TSharedPtr<FMenuMotion> Motion;bool Card=false,Selected=false,Compact=false;
    };
    class SMenuEntry : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SMenuEntry):_Card(false),_Selected(false),_Disabled(false),_Compact(false),_Enabled(true){}
            SLATE_ARGUMENT(TSharedPtr<FMenuMotion>,Motion)
            SLATE_ARGUMENT(TWeakObjectPtr<AEWardCampaignMode>,Mode)
            SLATE_ARGUMENT(FString,Action)
            SLATE_EVENT(FOnClicked,Handler)
            SLATE_ARGUMENT(bool,Card)
            SLATE_ARGUMENT(bool,Selected)
            SLATE_ARGUMENT(bool,Disabled)
            SLATE_ARGUMENT(bool,Compact)
            SLATE_ATTRIBUTE(bool,Enabled)
            SLATE_DEFAULT_SLOT(FArguments,Content)
        SLATE_END_ARGS()
        void Construct(const FArguments& A)
        {
            Motion=A._Motion;Mode=A._Mode;Action=A._Action;Card=A._Card;
            const auto Enabled=A._Disabled?TAttribute<bool>(false):A._Enabled;
            const auto Handler=A._Handler;
            const FMargin Padding=Card?FMargin(2):A._Compact?FMargin(27,8,16,8):FMargin(33,13,14,12);
            auto B=SNew(SButton).ButtonStyle(&MenuStyle).ContentPadding(0).IsEnabled(Enabled)
                .OnHovered_Lambda([this](){Motion->Mouse=true;}).OnUnhovered_Lambda([this](){Motion->Mouse=false;})
                .OnClicked_Lambda([this,Handler](){
                    if(Handler.IsBound())return Handler.Execute();
                    if(Mode.IsValid())Mode->Command(Action);
                    return FReply::Handled();
                })[SAssignNew(Content,SBox).Padding(Padding)[A._Content.Widget]];
            Focus=B;
            auto Chrome=SNew(SMenuChrome).Motion(Motion).Card(Card).Selected(A._Selected).Compact(A._Compact);
            auto Layers=SNew(SOverlay);if(Card){Layers->AddSlot()[B];Layers->AddSlot()[Chrome];}else{Layers->AddSlot()[Chrome];Layers->AddSlot()[B];}ChildSlot[Layers];
        }
        virtual void Tick(const FGeometry& G,double T,float D)override
        {
            SCompoundWidget::Tick(G,T,D);const bool Over=Focus->IsEnabled()&&Motion->Mouse;
            if(Over&&!Previous&&Mode.IsValid()&&Mode->TitleScene)Mode->TitleScene->Hover();Previous=Over;
            Motion->Clock+=D;Motion->Blend=FMath::FInterpTo(Motion->Blend,Over?1.f:0.f,D,12);
            if(!Card){const float Glitch=Over&&FMath::Fmod(T,6.1)>.0&&FMath::Fmod(T,6.1)<.06?FMath::Sin(T*75)*.8f:0;Content->SetRenderTransform(FSlateRenderTransform(FVector2D(Motion->Blend*5+Glitch,0)));}
        }
        TSharedPtr<FMenuMotion> Motion;TWeakObjectPtr<AEWardCampaignMode> Mode;TSharedPtr<SBox> Content;TSharedPtr<SButton> Focus;FString Action;bool Card=false,Previous=false;
    };
    class STitleShade : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(STitleShade){} SLATE_ARGUMENT(TWeakObjectPtr<AEWardCampaignMode>,Mode) SLATE_END_ARGS()
        void Construct(const FArguments& Args){Mode=Args._Mode;ForceVolatile(true);SetVisibility(EVisibility::HitTestInvisible);}
        virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1920,1080);}
        virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled)const override
        {
            const FVector2D S=G.GetLocalSize();
            const float Backdrop=Mode.IsValid()&&Mode->TitleScene&&Mode->TitleScene->Transitioning?FMath::Clamp(1-Mode->TitleScene->TransitionAge*2,0.f,1.f):1.f;
            TArray<FSlateGradientStop> Across;
            Across.Emplace(FVector2D(0,0),FLinearColor(.012,.0015,.002,.94));Across.Emplace(FVector2D(S.X*.23,0),FLinearColor(.009,.0015,.002,.76));Across.Emplace(FVector2D(S.X*.52,0),FLinearColor(.012,.001,.002,.16));Across.Emplace(FVector2D(S.X*.8,0),FLinearColor::Transparent);
            for(auto& Stop:Across)Stop.Color.A*=Backdrop;FSlateDrawElement::MakeGradient(Out,Layer,G.ToPaintGeometry(),Across,Orient_Vertical);
            TArray<FSlateGradientStop> Edges;Edges.Emplace(FVector2D(0,0),FLinearColor(0,0,0,.40));Edges.Emplace(FVector2D(0,S.Y*.25),FLinearColor::Transparent);Edges.Emplace(FVector2D(0,S.Y*.73),FLinearColor::Transparent);Edges.Emplace(FVector2D(0,S.Y),FLinearColor(0,0,0,.65));
            for(auto& Stop:Edges)Stop.Color.A*=Backdrop;FSlateDrawElement::MakeGradient(Out,Layer+1,G.ToPaintGeometry(),Edges,Orient_Horizontal);
            if(Mode.IsValid()&&Mode->TitleScene&&Mode->TitleScene->Fade>0)FSlateDrawElement::MakeBox(Out,Layer+2,G.ToPaintGeometry(),&White,ESlateDrawEffect::None,FLinearColor(0,0,0,Mode->TitleScene->Fade));
            return Layer+2;
        }
        TWeakObjectPtr<AEWardCampaignMode> Mode;
    };
    const FSlateBrush* Photo(const FString& Id)
    {
        if(const auto* Existing=MenuImages.Find(Id))return &(*Existing)->Brush;
        auto* T=LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/Campaign/Menu/T_Menu_")+Id));
        if(!T)return nullptr;auto Resource=MakeUnique<FMenuImage>(T);const auto* Brush=&Resource->Brush;MenuImages.Add(Id,MoveTemp(Resource));return Brush;
    }
}
TSharedRef<SWidget> WardTitleMenu::Shade(TWeakObjectPtr<AEWardCampaignMode> Mode)
{
    return SNew(STitleShade).Mode(Mode);
}

TSharedRef<SWidget> WardTitleMenu::Logo(TWeakObjectPtr<AEWardCampaignMode> Weak)
{
    auto Logo=SNew(SOverlay);const auto* Brand=Photo(TEXT("logo_echo"));
    if(Brand)
    {
        Logo->AddSlot().Padding(FMargin(2,0,0,0))[SNew(SImage).Image(Brand).ColorAndOpacity_Lambda([Weak](){const float A=Weak.IsValid()&&Weak->TitleScene?Weak->TitleScene->Age:5;return FLinearColor(.8,.13,.12,A<1.8f&&FMath::Sin(A*71)>0?.45f:0);}).RenderTransform_Lambda([Weak](){const float A=Weak.IsValid()&&Weak->TitleScene?Weak->TitleScene->Age:5;return FSlateRenderTransform(FVector2D(A<1.8f?FMath::Sin(A*53)*5:0,0));})];
        Logo->AddSlot()[SNew(SImage).Image(Brand).ColorAndOpacity_Lambda([Weak](){const float T=Weak.IsValid()&&Weak->TitleScene?Weak->TitleScene->Age:0;const float P=FMath::Pow((FMath::Sin(T*2.2f)+1)*.5f,8);return FLinearColor(.91f+P*.09f,.61f+P*.035f,.57f+P*.025f,1);})];
    }
    else Logo->AddSlot()[SNew(STextBlock).Text(FText::FromString(TEXT("ЭХО")))
        .Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/Literata.ttf"),112))
        .ColorAndOpacity(FLinearColor(.95,.95,.9))];
    return Logo;
}

TSharedRef<SWidget> WardTitleMenu::Action(TWeakObjectPtr<AEWardCampaignMode> Mode, FOnClicked Handler,
    TSharedRef<SWidget> Content, TAttribute<bool> Enabled)
{
    return SNew(SMenuEntry).Motion(MakeShared<FMenuMotion>()).Mode(Mode)
        .Compact(true).Handler(Handler).Enabled(Enabled)[Content];
}

TSharedRef<SWidget> FWardCampaignUI::Title()
{
    TWeakObjectPtr<AEWardCampaignMode> Weak=Mode;auto* G=Mode.Get();
    const FString Serif=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/Literata.ttf");
    TitleActions.Reset();TitleMotion.Reset();
    auto Stack=SNew(SVerticalBox);
    Stack->AddSlot().AutoHeight().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(606).HeightOverride(205)[WardTitleMenu::Logo(Weak)]];
    Stack->AddSlot().AutoHeight().Padding(4,4,0,40)[SNew(STextBlock).Text(FText::FromString(G->Tr(TEXT("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА")))).Font(FSlateFontInfo(Serif,12)).ColorAndOpacity(FLinearColor(.72,.72,.66))];
    const FString AIDemo = G->Language==TEXT("kk") ? TEXT("ЖИ демонстрациясы")
        : G->Language==TEXT("en") ? TEXT("AI demonstration") : TEXT("Демонстрация ИИ");
    const TArray<TPair<FString,FString>> Entries={{TEXT("Игра"),TEXT("game")},{TEXT("Настройки"),TEXT("settings")},{TEXT("Обучение"),TEXT("training")},{TEXT("Об игре"),TEXT("about")},{AIDemo,TEXT("ai_demo")},{TEXT("Карты"),TEXT("maps")},{TEXT("Онлайн"),TEXT("online")},{TEXT("Выйти"),TEXT("quit")}};
    for(const auto& E:Entries)
    {
        const bool Disabled=E.Value==TEXT("maps");auto Motion=MakeShared<FMenuMotion>();auto Row=SNew(SHorizontalBox);
        Row->AddSlot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(G->Tr(E.Key))).Font(FSlateFontInfo(Serif,18)).ColorAndOpacity_Lambda([Motion,Disabled](){return Disabled?FLinearColor(.27,.29,.28):FLinearColor(.87,.86,.80,.78f+Motion->Blend*.22f);})];
        if(Disabled)Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12,0)[SNew(STextBlock).Text(FText::FromString(G->Language==TEXT("en")?TEXT("· in development"):G->Language==TEXT("kk")?TEXT("· әзірленуде"):TEXT("· в разработке"))).Font(FSlateFontInfo(Serif,11)).ColorAndOpacity(FLinearColor(.30,.32,.31))];
        const FString Action=TEXT("page_")+E.Value;
        auto Control=SNew(SMenuEntry).Motion(Motion).Mode(Weak).Action(Action).Card(false).Selected(E.Value==TEXT("game")).Disabled(Disabled)[Row];
        TitleActions.Add(Action,Control->Focus);TitleMotion.Add(Action,[Motion](){return Motion->Blend;});
        Stack->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0,0,0,3)[SNew(SBox).WidthOverride(426).HeightOverride(56)[Control]];
    }
    auto Cards=SNew(SHorizontalBox);
    for(const auto& E:TArray<TPair<FString,FString>>{{TEXT("Трейлер"),TEXT("trailer")},{TEXT("Скриншоты"),TEXT("screenshots")}})
    {
        auto Motion=MakeShared<FMenuMotion>();auto Contents=SNew(SVerticalBox);auto Frame=SNew(SOverlay);
        if(const auto* Brush=WardMedia78::Cover78(E.Value==TEXT("trailer")))Frame->AddSlot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(Brush).ColorAndOpacity_Lambda([Motion](){const float V=.86f+Motion->Blend*.24f;return FLinearColor(V,V,V,1);}).RenderTransformPivot(FVector2D(.5,.5)).RenderTransform_Lambda([Motion](){return FSlateRenderTransform(FScale2D(1+Motion->Blend*.035f));})]];
        Frame->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(8)[Text(E.Value==TEXT("trailer")?TEXT("ПОСЛЕДНИЙ ЗВОНОК"):TEXT("20 КАДРОВ / АРХИВ"),10,FLinearColor(.78,.79,.74))];
        Contents->AddSlot().AutoHeight()[SNew(SBox).WidthOverride(396).HeightOverride(148).Clipping(EWidgetClipping::ClipToBounds)[Frame]];
        Contents->AddSlot().AutoHeight().Padding(0,6)[SNew(STextBlock).Text(FText::FromString(E.Value==TEXT("trailer")?(G->Language==TEXT("en")?TEXT("TRAILER"):TEXT("ТРЕЙЛЕР")):(G->Language==TEXT("en")?TEXT("SCREENSHOTS"):G->Language==TEXT("kk")?TEXT("СКРИНШОТТАР"):TEXT("СКРИНШОТЫ")))).Font(FSlateFontInfo(Serif,10)).ColorAndOpacity(FLinearColor(.70,.70,.65))];
        const FString Action=TEXT("page_")+E.Value;auto Control=SNew(SMenuEntry).Motion(Motion).Mode(Weak).Action(Action).Card(true).Selected(E.Value==TEXT("trailer")).Disabled(false)[Contents];
        TitleActions.Add(Action,Control->Focus);TitleMotion.Add(Action,[Motion](){return Motion->Blend;});
        Cards->AddSlot().AutoWidth().Padding(0,0,24,0)[SNew(SBox).WidthOverride(400).HeightOverride(174)[Control]];
    }
    Stack->AddSlot().AutoHeight().Padding(8,24,0,0)[Cards];
    auto Page=SNew(SOverlay);
    Page->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(88,118,0,0))[Stack];

    static TUniquePtr<FMenuImage> StudioImage;
    if (!StudioImage)
    {
        if (UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(WardMedia78::StudioBase()/TEXT("Studio.png")))
        {
            StudioImage = MakeUnique<FMenuImage>(Texture);
        }
    }
    TSharedRef<SWidget> StudioCredit = SNew(STextBlock)
        .Text(FText::FromString(TEXT("QAISAR STUDIO")))
        .Font(FSlateFontInfo(Serif, 16))
        .ColorAndOpacity(FLinearColor(.85f, .81f, .76f));
    if (StudioImage)
    {
        StudioCredit = SNew(SBox).WidthOverride(224).HeightOverride(50)
        [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(&StudioImage->Brush)]];
    }
    Page->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(FMargin(0, 67, 66, 0))
    [
        SNew(SBorder).Tag(FName(TEXT("QaisarStudio"))).Visibility(EVisibility::HitTestInvisible)
        .BorderImage(&White).BorderBackgroundColor(FLinearColor(.004f, .006f, .009f, .66f))
        .Padding(FMargin(16, 12))[StudioCredit]
    ];

    // Beta identity is separate from the original title art and never receives input.
    auto Beta=SNew(SHorizontalBox);
    Beta->AddSlot().AutoWidth().Padding(0,0,16,0)[SNew(SBox).WidthOverride(2)[SNew(SBorder).BorderImage(&White).BorderBackgroundColor(FLinearColor(.68f,.035f,.025f)).Padding(0)]];
    auto BetaText=SNew(SVerticalBox);
    BetaText->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(G->Language==TEXT("en")?TEXT("BETA"):TEXT("БЕТА"))).Font(FSlateFontInfo(Serif,20)).ColorAndOpacity(FLinearColor(.85f,.85f,.80f))];
    BetaText->AddSlot().AutoHeight().Padding(0,6,0,0)[SNew(STextBlock).Text(FText::FromString(G->Language==TEXT("en")?TEXT("TEST BUILD  /  0.2.0-b117"):G->Language==TEXT("kk")?TEXT("СЫНАҚ НҰСҚАСЫ  /  0.2.0-b117"):TEXT("ТЕСТОВАЯ ВЕРСИЯ  /  0.2.0-b117"))).Font(FSlateFontInfo(Serif,10)).ColorAndOpacity(FLinearColor(.62f,.34f,.31f))];
    Beta->AddSlot().AutoWidth()[BetaText];
    Page->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0,0,66,50))[SNew(SBorder).Visibility(EVisibility::HitTestInvisible).BorderImage(&White).BorderBackgroundColor(FLinearColor(.004f,.009f,.011f,.75f)).Padding(FMargin(18,13))[Beta]];
    auto Animated=MakeWardTitleFade(Page,G,true);
    return SNew(SOverlay)+SOverlay::Slot()[SNew(STitleShade).Mode(Weak)]+SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Animated]]];
}
TSharedRef<SWidget> FWardCampaignUI::Media(const FString& Kind)
{
    auto B=SNew(SVerticalBox);
    B->AddSlot().AutoHeight()[Text(Kind==TEXT("trailer")?TEXT("Трейлер · камера наблюдения 04"):TEXT("Скриншоты · отделение 04"),28,FLinearColor(.8,.15,.12))];
    B->AddSlot().AutoHeight().Padding(0,12)[Text(Kind==TEXT("trailer")?TEXT("В подготовке. Здесь появится короткая запись из коридора больницы."):TEXT("Кадры из текущей версии игры."),18)];
    for(const FString Id:Kind==TEXT("trailer")?TArray<FString>{TEXT("corridor")}:TArray<FString>{TEXT("corridor"),TEXT("ward"),TEXT("stalker")})
        if(const auto* Brush=Photo(Id))B->AddSlot().AutoHeight().Padding(0,12)[SNew(SBox).HeightOverride(420)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Brush)]]];
    return B;
}

TSharedRef<SWidget> FWardCampaignUI::TitleSection(TSharedRef<SWidget> Body)
{
    TWeakObjectPtr<AEWardCampaignMode> Weak=Mode;
    auto Content=MakeWardTitleFade(SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(85,58,0,0))[SNew(SBox).WidthOverride(1120).HeightOverride(970)[SNew(SBorder).BorderImage(&White).BorderBackgroundColor(FLinearColor(.002,.006,.007,.82)).Padding(28)[Body]]],Mode.Get());
    return SNew(SOverlay)+SOverlay::Slot()[SNew(STitleShade).Mode(Weak)]+SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Content]]];
}

class SWardTitleFade : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWardTitleFade){} SLATE_ARGUMENT(TWeakObjectPtr<AEWardCampaignMode>,Mode) SLATE_ARGUMENT(bool,Intro) SLATE_DEFAULT_SLOT(FArguments,Content) SLATE_END_ARGS()
    void Construct(const FArguments& A){Mode=A._Mode;Intro=A._Intro;ChildSlot[A._Content.Widget];}
    virtual void Tick(const FGeometry& G,double T,float D)override
    {
        SCompoundWidget::Tick(G,T,D);float Alpha=1;
        if(Mode.IsValid()&&Mode->TitleScene){auto* S=Mode->TitleScene.Get();Alpha=S->Transitioning?FMath::Clamp(1-S->TransitionAge*2,0.f,1.f):Intro?FMath::Clamp((S->Age-.3f)/1.5f,0.f,1.f):1;}
        SetRenderOpacity(Alpha);
    }
    TWeakObjectPtr<AEWardCampaignMode> Mode;bool Intro=false;
};
TSharedRef<SWidget> MakeWardTitleFade(TSharedRef<SWidget> Body,AEWardCampaignMode* Mode,bool Intro){return SNew(SWardTitleFade).Mode(Mode).Intro(Intro)[Body];}
