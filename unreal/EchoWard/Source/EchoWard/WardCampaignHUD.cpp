#include "WardCampaign.h"
#include "WardOnline80.h"
#include "WardRescue104.h"
#include "WardGameplay76.h"
#include "WardDialogue62.h"
#include "WardInterface66.h"
#include "WardCombat63.h"
#include "WardHospitalGameplay.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
using namespace WardData;
void AEWardCampaignHUD::DrawHUD()
{
    Super::DrawHUD();if(FParse::Param(FCommandLine::Get(),TEXT("WardCinematicCapture78")))return;auto* G=WardCampaign(this);
    const bool Audit=FParse::Param(FCommandLine::Get(),TEXT("WardHUDCheck"));
    if(Audit){LastFrame=MakeShared<FJsonObject>();LastFrame->SetBoolField(TEXT("visible"),false);}
    if(!Canvas||!G||!G->Hero())return;
    WardGameplay76::Draw(*G,Canvas);
    const bool StoryView = G->Screen == TEXT("story35");
    if (!G->Screen.IsEmpty() && !StoryView) return;
    auto* H=G->Hero();const float W=Canvas->SizeX,Height=Canvas->SizeY,S=FMath::Min(W/1920.f,Height/1080.f),Opacity=FMath::Clamp(float(G->Setting(TEXT("hud_opacity"),1)),0.f,1.f);
    static UFont* Font=nullptr;if(!Font){Font=NewObject<UFont>();Font->AddToRoot();Font->FontCacheType=EFontCacheType::Runtime;FTypefaceEntry Entry;Entry.Name=TEXT("Regular");Entry.Font=FFontData(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);Font->CompositeFont.DefaultTypeface.Fonts.Add(Entry);Font->LegacyFontSize=20;}
    const FLinearColor White(.89,.85,.83,Opacity),Red(.78,.075,.06,Opacity),Teal(.77,.12,.14,Opacity),Muted(.15,.035,.045,.9f*Opacity),Back(.008,.002,.004,.78f*Opacity);
    const bool Critical=H->Health<=25;
    const float Heartbeat=.5f+.5f*FMath::Sin(float(FMath::Fmod(FPlatformTime::Seconds(),3600.))*2*PI*(Critical?1.3f:.75f));
    const float HealthPulse=Critical?.35f+.65f*Heartbeat:.6f+.4f*Heartbeat;
    const FLinearColor HealthRed(Red.R*HealthPulse,Red.G*HealthPulse,Red.B*HealthPulse,Opacity);
    Json Regions;if(Audit)Regions=MakeShared<FJsonObject>();bool Bounded=true;
    auto Mark=[&](const TCHAR* Id,float X,float Y,float Width,float Tall){if(!Audit)return;TArray<TSharedPtr<FJsonValue>> A;for(float V:{X,Y,Width,Tall})A.Add(MakeShared<FJsonValueNumber>(V));Regions->SetArrayField(Id,A);Bounded&=X>=-1&&Y>=-1&&X+Width<=W+1&&Y+Tall<=Height+1;};
    auto Rect=[&](float X,float Y,float Width,float Tall,FLinearColor C){FCanvasTileItem I(FVector2D(X,Y),FVector2D(Width,Tall),C);I.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(I);};
    auto Line=[&](FVector2D A,FVector2D B,FLinearColor C,float Thickness=1.f){FCanvasLineItem I(A,B);I.SetColor(C);I.LineThickness=Thickness*S;Canvas->DrawItem(I);};
    auto Size=[&](const FString& T,float Scale){float X=0,Y=0;Canvas->StrLen(Font,T,X,Y);return FVector2D(X,Y)*Scale*S;};
    auto Text=[&](const FString& T,float X,float Y,float Scale,FLinearColor C){FCanvasTextItem I(FVector2D(X,Y),FText::FromString(T),Font,C);I.Scale=FVector2D(Scale*S);I.EnableShadow(FLinearColor(0,0,0,.8f*Opacity));Canvas->DrawItem(I);if(Audit){const auto V=Size(T,Scale);Bounded&=X>=-1&&Y>=-1&&X+V.X<=W+1&&Y+V.Y<=Height+1;}};
    auto Wrap=[&](FString T,float Scale,float MaxWidth,int Limit){TArray<FString> Words,Out;T.ReplaceInline(TEXT("\n"),TEXT(" "));T.ParseIntoArrayWS(Words);FString Current;
        for(FString Word:Words){while(Size(Word,Scale).X>MaxWidth){int N=Word.Len();while(N>1&&Size(Word.Left(N),Scale).X>MaxWidth)--N;if(!Current.IsEmpty()){Out.Add(Current);Current.Empty();}Out.Add(Word.Left(N));Word.RightChopInline(N);}const FString Next=Current.IsEmpty()?Word:Current+TEXT(" ")+Word;if(Size(Next,Scale).X>MaxWidth&&!Current.IsEmpty()){Out.Add(Current);Current=Word;}else Current=Next;}if(!Current.IsEmpty())Out.Add(Current);if(Out.Num()>Limit){Out.SetNum(Limit);FString& Last=Out.Last();while(Last.Len()>1&&Size(Last+TEXT("…"),Scale).X>MaxWidth)Last.LeftChopInline(1);Last+=TEXT("…");}return Out;};
    auto Copy=[&](FString Value,float X,float Y,float Scale,float MaxWidth,int Limit,FLinearColor Color){auto Lines=Wrap(Value,Scale,MaxWidth,Limit);for(int I=0;I<Lines.Num();++I)Text(Lines[I],X,Y+I*29*S*Scale,Scale,Color);return FMath::Max(1,Lines.Num())*29*S*Scale;};
    auto Frame=[&](float X,float Y,float Width,float Tall){Rect(X,Y,Width,Tall,Back);const FLinearColor Edge(.43,.06,.08,.75f*Opacity);Line({X,Y},{X+Width,Y},Edge);Line({X,Y+Tall},{X+Width,Y+Tall},Edge);Line({X,Y},{X,Y+Tall},Edge);Line({X+Width,Y},{X+Width,Y+Tall},Edge);};
    auto* Story = G->HospitalStory.Get();
    const bool Controls = StoryView && Story && WardDialogue62::HasControls(*Story);
    FString Subtitle;
    if (Story && G->SettingBool(TEXT("subtitles"), true))
    {
        Subtitle = Story->HomeStage52 == 1 ? Story->HomeSubtitle69 : Story->DialogueKey62;
        if (Subtitle.IsEmpty() && StoryView && Story->ActiveScene35 == TEXT("CHECKPOINT_69") && WardDialogue62::AwaitingChoice(*Story))
            Subtitle = TEXT("Әсел: Фамилия человека, которого вы ищете?");
    }
    if (Subtitle.IsEmpty() && WardInterface66::ShowNotice(*G)) Subtitle = H->Notice;
    const auto SubtitleLines = Wrap(G->Tr(Subtitle), 1.07f, FMath::Min(980*S, W*.72f), 3);
    const float SubtitleBottom = Height - (Controls ? (Story->ActiveScene35 == TEXT("CHECKPOINT_69") ? 202.f : 145.f) : 94.f)*S;
    const float SubtitleY = SubtitleBottom - SubtitleLines.Num()*30*S;
    auto DrawSubtitle = [&]()
    {
        float MaxWidth = 0;
        for (int I=0; I<SubtitleLines.Num(); ++I)
        {
            const FString& LineText = SubtitleLines[I];
            const float Width = Size(LineText, 1.07f).X;
            MaxWidth = FMath::Max(MaxWidth, Width);
            FCanvasTextItem Caption(FVector2D((W-Width)*.5f, SubtitleY+I*30*S), FText::FromString(LineText),
                Font, FLinearColor(.96f,.97f,.96f,1));
            Caption.Scale = FVector2D(1.07f*S);
            Caption.EnableShadow(FLinearColor(0,0,0,.95f), FVector2D(1.5f*S, 2*S));
            Caption.bOutlined = true;
            Caption.OutlineColor = FLinearColor(0,0,0,.80f);
            Canvas->DrawItem(Caption);
        }
        if (SubtitleLines.Num()) Mark(TEXT("subtitle"), (W-MaxWidth)*.5f, SubtitleY, MaxWidth, SubtitleLines.Num()*30*S);
    };
    if (StoryView)
    {
        DrawSubtitle();
        return;
    }
    if (G->HospitalStory && G->HospitalStory->ArrivalStage == 0)
    {
        Rect(0, 0, W, 61*S, FLinearColor::Black);
        Rect(0, Height-61*S, W, 61*S, FLinearColor::Black);
        const float Clock = G->HospitalStory->ArrivalClock;
        const float Alpha = FMath::Clamp((Clock-.45f)*1.5f,0.f,1.f) * FMath::Clamp((3.8f-Clock)*1.2f,0.f,1.f);
        Text(TEXT("БОЛЬНИЦА № 333"), 67*S, Height-47*S, .87f, FLinearColor(.86f,.88f,.83f,Alpha));
        const FString Skip = TEXT("F — пропустить");
        Text(Skip, W-52*S-Size(Skip,.74f).X, Height-45*S, .74f, FLinearColor(.60f,.64f,.63f,.85f));
        return;
    }
    const float Left=34*S,Top=28*S,CopyX=56*S,ObjectiveWidth=FMath::Min(1080*S,W-440*S);
    Text(G->Tr(TEXT("ЗАДАЧА")),CopyX,Top,.85f,Red);
    Rect(CopyX,Top+38*S,14*S,14*S,Teal);Rect(CopyX+2*S,Top+40*S,10*S,10*S,Back);
    const FString Goal = WardOnline80::Reconnecting(*G) ? TEXT("Восстанавливаем связь с хозяином…") :
        WardOnline80::Guest(*G) && Story && !Story->ActiveScene35.IsEmpty() ?
        TEXT("Диалог ведёт Бека · ") + Story->NarrativeTitle35() :
        G->Objective();
    const float GoalHeight=Copy(Goal,CopyX+26*S,Top+28*S,1.12f,ObjectiveWidth-26*S,3,White);
    const float TaskHeight=GoalHeight+57*S;Rect(Left,Top,4*S,TaskHeight,Red);Mark(TEXT("objective"),Left,Top,ObjectiveWidth+22*S,TaskHeight);
    if(G->SettingBool(TEXT("objective_hint")))Copy(G->RoomName(),CopyX,Top+GoalHeight+32*S,.84f,ObjectiveWidth,1,Teal);
    if(G->SettingBool(TEXT("show_noise"))){const float X=W-318*S;Text(G->Tr(TEXT("ШУМ")),X,Top,.85f,White);for(int I=0;I<6;++I)Rect(X+I*47*S,Top+29*S,40*S,14*S,I<FMath::CeilToInt(FMath::Clamp(G->Noise,0.f,1.f)*6)?Red:Muted);Mark(TEXT("noise"),X,Top,275*S,48*S);}
    auto Bar=[&](const TCHAR* Id,const FString& Label,float Value,float Y,FLinearColor Color){Rect(Left,Y+3*S,4*S,40*S,Red);Text(G->Tr(Label),CopyX,Y,.85f,White);const FString Number=FString::FromInt(FMath::RoundToInt(FMath::Clamp(Value,0.f,100.f)));Text(Number,CopyX+330*S-Size(Number,.85f).X,Y,.85f,White);Rect(CopyX,Y+34*S,330*S,9*S,Muted);Rect(CopyX,Y+34*S,330*S*FMath::Clamp(Value/100.f,0.f,1.f),9*S,Color);Mark(Id,Left,Y,352*S,45*S);};
    Bar(TEXT("health"),TEXT("ЗДОРОВЬЕ"),H->Health,Height-126*S,HealthRed);Bar(TEXT("breath"),TEXT("ДЫХАНИЕ"),H->Stamina,Height-72*S,FLinearColor(.87,.43,.30,Opacity));
    const auto Rescue=WardRescue104::Prompt(*G);
    if(WardGameplay76::Armed(*G)&&!Rescue.LocalDown)
    {
        const float X=W-308*S,Y=Height-130*S;Frame(X,Y,280*S,100*S);Text(G->Tr(G->Shotgun?TEXT("ДРОБОВИК"):TEXT("ПИСТОЛЕТ")),X+20*S,Y+12*S,.8f,Teal);
        Text(FString::Printf(TEXT("%02d / %02d"),G->Shotgun?G->LoadedShells:H->Rounds,G->Shotgun?G->Items.FindRef(TEXT("shells")):H->Reserve),X+20*S,Y+43*S,1.65f,White);
        const float IX=X+206*S,IY=Y+25*S;const FLinearColor Icon(.5,.56,.55,.65f*Opacity);Rect(IX,IY,(G->Shotgun?59:49)*S,8*S,Icon);Rect(IX+4*S,IY+8*S,25*S,4*S,Icon);
        if(G->Shotgun){Line({IX+4*S,IY+10*S},{IX-6*S,IY+23*S},Icon,7);Rect(IX+35*S,IY+7*S,18*S,5*S,Icon);}
        else {Line({IX+30*S,IY+11*S},{IX+39*S,IY+28*S},Icon,9);Line({IX+14*S,IY+13*S},{IX+14*S,IY+20*S},Icon,2);Line({IX+14*S,IY+20*S},{IX+31*S,IY+20*S},Icon,2);}
        Mark(TEXT("weapon"),X,Y,280*S,100*S);
    }
    if(Rescue.LocalDown||Rescue.Remaining>0)
    {
        const float Width=FMath::Min(610*S,W*.68f), X=(W-Width)*.5f, Y=Height*.72f;
        FString Label, Detail;
        if(Rescue.LocalDown)
        {
            Label=FString::Printf(TEXT("ТЯЖЁЛОЕ РАНЕНИЕ · %d сек"),FMath::CeilToInt(Rescue.Remaining));
            Detail=Rescue.Helping?TEXT("Напарник помогает тебе встать"):TEXT("WASD — отползти в укрытие");
        }
        else
        {
            Label=FString::Printf(TEXT("%s ранен · %d сек"),*Rescue.Name,FMath::CeilToInt(Rescue.Remaining));
            Detail=Rescue.Helping?TEXT("Держи F и оставайся рядом"):Rescue.CanHelp?TEXT("Удерживай F — помочь встать"):
                FString::Printf(TEXT("Подойди к напарнику · %.0f м"),Rescue.Distance);
        }
        Frame(X,Y,Width,85*S);
        Copy(Label,X+15*S,Y+8*S,.92f,Width-30*S,1,White);
        Copy(Detail,X+15*S,Y+41*S,.83f,Width-30*S,1,Teal);
        Rect(X,Y+81*S,Width,4*S,Muted);
        const float Fill=Rescue.Progress>0?Rescue.Progress:Rescue.Remaining/WardRescue104::BleedSeconds;
        Rect(X,Y+81*S,Width*FMath::Clamp(Fill,0.f,1.f),4*S,Red);
        Mark(TEXT("rescue"),X,Y,Width,85*S);
    }
    else if(H->Focus)
    {
        const FString Prompt=H->Focus->Prompt(H);if(!Prompt.IsEmpty()){const float Width=FMath::Min(900*S,FMath::Max(360*S,Size(Prompt,.95f).X+56*S));const auto Lines=Wrap(Prompt,.95f,Width-40*S,3);const float Tall=28*S+Lines.Num()*28*S;const float X=(W-Width)*.5f,Y=SubtitleLines.Num()?FMath::Min(Height*.73f,SubtitleY-18*S-Tall):Height*.73f;
            Frame(X,Y,Width,Tall);Rect(X,Y,3*S,Tall,Teal);for(int I=0;I<Lines.Num();++I)Text(Lines[I],X+20*S,Y+12*S+I*28*S,.95f,White);if(H->SearchProgress>0)Rect(X,Y+Tall-3*S,Width*FMath::Clamp(H->SearchProgress,0.f,1.f),3*S,Teal);Mark(TEXT("interaction"),X,Y,Width,Tall);}
    }
    if(G->SettingBool(TEXT("crosshair"))&&H->Aiming){const float X=W*.5f,Y=Height*.5f;if(H->WeaponObstructed){Line({X-6*S,Y-6*S},{X+6*S,Y+6*S},Red,1.5f);Line({X+6*S,Y-6*S},{X-6*S,Y+6*S},Red,1.5f);}else Rect(X-1.5f*S,Y-1.5f*S,3*S,3*S,White);Mark(TEXT("crosshair"),X-7*S,Y-7*S,14*S,14*S);}
    DrawSubtitle();
    if(G->ReloadTime>0){const FString T=G->Tr(TEXT("Перезарядка…"));Text(T,(W-Size(T,.85f).X)*.5f,Height*.6f,.85f,Teal);}
    if(WardCombat63::Healing(*G)){const float Progress=1-float(G->NumberFlag(TEXT("combat63_heal")))/2.4f;Rect(W*.4f,Height*.64f,W*.2f,3*S,Muted);Rect(W*.4f,Height*.64f,W*.2f*FMath::Clamp(Progress,0.f,1.f),3*S,Teal);}
    if(G->SaveIcon>0)Text(G->Tr(TEXT("Сохранение")),W-290*S,105*S,.8f,Teal);
    if(G->EscapeTime>=0){const FString T=G->Tr(FString::Printf(TEXT("ЭВАКУАЦИЯ  %02d:%02d"),int(G->EscapeTime)/60,int(G->EscapeTime)%60));Text(T,(W-Size(T,1.05f).X)*.5f,Top,1.05f,Red);}
    if(G->Admin){Copy(G->Agent?G->Tr(TEXT("АДМИНИСТРАТОР · АГЕНТ / "))+G->AgentStatus:G->Tr(TEXT("АДМИНИСТРАТОР · F8 — предметы и записки · F9 — агент · F10 — персонаж")),CopyX,Height*.45f,.8f,W-120*S,2,Teal);Text(FString::Printf(TEXT("A*: %d  |  CSP: %d / %d  |  Minimax: %d  |  alpha-beta: %d"),G->Grid.Expanded,G->Grid.Assignments,G->Grid.ConstraintChecks,G->Grid.TacticalNodes,G->Grid.TacticalPrunes),CopyX,Height*.45f+55*S,.7f,Teal);}
    if(Audit){LastFrame->SetBoolField(TEXT("visible"),true);LastFrame->SetBoolField(TEXT("bounded"),Bounded);LastFrame->SetNumberField(TEXT("width"),W);LastFrame->SetNumberField(TEXT("height"),Height);LastFrame->SetNumberField(TEXT("health"),H->Health);LastFrame->SetNumberField(TEXT("stamina"),H->Stamina);LastFrame->SetNumberField(TEXT("rounds"),G->Shotgun?G->LoadedShells:H->Rounds);LastFrame->SetNumberField(TEXT("reserve"),G->Shotgun?G->Items.FindRef(TEXT("shells")):H->Reserve);LastFrame->SetStringField(TEXT("weapon"),G->Shotgun?TEXT("shotgun"):TEXT("pistol"));LastFrame->SetObjectField(TEXT("regions"),Regions);}
}
