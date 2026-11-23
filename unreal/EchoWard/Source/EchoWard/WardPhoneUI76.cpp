#include "WardCampaignUI.h"
#include "WardCampaign.h"
#include "WardHospitalGameplay.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"
#include "Misc/Paths.h"
namespace
{
 const FLinearColor Blood77(.72,.025,.055),Paper77(.93,.88,.84),Ash77(.57,.42,.43);
 class SPhoneArt77:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SPhoneArt77):_Icon(-1){} SLATE_ARGUMENT(int,Icon) SLATE_END_ARGS()
  void Construct(const FArguments& A){Icon=A._Icon;SetVisibility(EVisibility::HitTestInvisible);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(64,64);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle& Style,bool)const override
  {
   const auto S=G.GetLocalSize();const float Opacity=Style.GetColorAndOpacityTint().A;
   auto Line=[&](TArray<FVector2D> P,FLinearColor C,float Width=1.5f){C.A*=Opacity;FSlateDrawElement::MakeLines(Out,L+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Width);};
   auto Circle=[&](FVector2D Centre,float R,FLinearColor C,float Width=1.5f){TArray<FVector2D> P;for(int I=0;I<=48;++I){const float A=I*2*PI/48;P.Add(Centre+FVector2D(FMath::Cos(A),FMath::Sin(A))*R);}Line(P,C,Width);};
   if(Icon<0)
   {
    // Glass scratches and dried blood use the same dark red language as the title.
    FRandomStream R(7719);for(int I=0;I<36;++I){const float X=R.FRandRange(8,S.X-8),Y=I%2?R.FRandRange(6,32):R.FRandRange(S.Y-35,S.Y-7);Line({{X,Y},{X+R.FRandRange(1,9),Y+R.FRandRange(1,3)}},FLinearColor(.55,.025,.045,R.FRandRange(.15,.5)),.7f);}
    for(int I=0;I<6;++I){const float X=16+I*13.3;Line({{X,11},{X+1,24.f+I%3*15}},FLinearColor(.42,.008,.024,.65),I%2?2:3);Circle({X,25.f+I%3*15},1.5,Blood77,1);}
    Line({{12,62},{S.X-12,62}},FLinearColor(.38,.035,.05,.7),.8f);return L+1;
   }
   const FVector2D C=S*.5;auto P=[&](float X,float Y){return C+FVector2D(X,Y);};const FLinearColor Ink=Paper77;
   if(Icon==0){Line({P(-21,-17),P(21,-17),P(21,10),P(-5,10),P(-17,21),P(-17,10),P(-21,10),P(-21,-17)},Ink,2);for(int I=-1;I<=1;++I)Circle(P(I*10,-3),1.3,Ink,2);}
   if(Icon==1){Line({P(-20,-14),P(-8,-14),P(-4,-20),P(7,-20),P(11,-14),P(20,-14),P(20,19),P(-20,19),P(-20,-14)},Ink,2);Circle(P(0,2),10,Ink,2);Circle(P(0,2),6,Blood77,2);Circle(P(14,-7),1.2,Ink,2);}
   if(Icon==2){Circle(P(-8,-9),10,Ink,2);for(int I=0;I<8;++I){float A=I*PI/4;Line({P(-8+FMath::Cos(A)*14,-9+FMath::Sin(A)*14),P(-8+FMath::Cos(A)*18,-9+FMath::Sin(A)*18)},Ink,1.4);}Line({P(-20,15),P(20,15),P(22,5),P(16,-2),P(6,-2),P(1,3),P(-11,3),P(-20,10),P(-20,15)},Ink,2);}
   if(Icon==3){Line({P(-18,-18),P(-9,-21),P(-3,-10),P(-9,-4),P(-3,5),P(5,11),P(11,4),P(22,11),P(19,21),P(11,23),P(-1,16),P(-13,3),P(-20,-10),P(-18,-18)},Ink,2);}
   if(Icon==4){Line({P(-17,-23),P(8,-23),P(19,-12),P(19,23),P(-17,23),P(-17,-23)},Ink,2);Line({P(8,-23),P(8,-12),P(19,-12)},Ink,1.5);for(int I=0;I<3;++I)Line({P(-9,-4+I*9),P(11,-4+I*9)},Ink,1.7);}
   if(Icon==5){Circle(C,17,Ink,2);Circle(C,7,Blood77,2);for(int I=0;I<8;++I){const float A=I*PI/4;Line({C+FVector2D(FMath::Cos(A),FMath::Sin(A))*17,C+FVector2D(FMath::Cos(A),FMath::Sin(A))*24},Ink,3);}}
   return L+1;
  }
  int Icon=-1;
 };
}
TSharedRef<SWidget> FWardCampaignUI::PhoneMenu76()
{
 auto* G=Mode.Get();TitleActions.Reset();TitleMotion.Reset();
 static FSlateRoundedBoxBrush Shell(FLinearColor(.004,.002,.003),32.f,FLinearColor(.25,.025,.04),2.f);
 static FSlateRoundedBoxBrush Glass(FLinearColor(.012,.005,.008),25.f);
 static FSlateRoundedBoxBrush Bubble(FLinearColor(.075,.013,.021),13.f);
 static FSlateRoundedBoxBrush Sent(FLinearColor(.24,.014,.032),13.f);
 static FButtonStyle AppStyle=[](){FButtonStyle S;S.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.10,.012,.024),18.f,FLinearColor(.37,.035,.06),1.f));S.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.26,.015,.035),18.f,FLinearColor(.78,.07,.11),1.5f));S.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.38,.025,.05),18.f));return S;}();
 const FString Font=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf");
 auto Copy=[&](const FString& V,int Size,FLinearColor Color=Paper77,float Wrap=315){return SNew(STextBlock).Text(FText::FromString(V)).Font(FSlateFontInfo(Font,Size)).ColorAndOpacity(Color).WrapTextAt(Wrap).AutoWrapText(false);};
 auto Action=[&](const FString& Label,const FString& Id){auto B=SNew(SButton).ButtonStyle(&AppStyle).ContentPadding(FMargin(15,12)).OnClicked_Lambda([Weak=Mode,Id](){if(Weak.IsValid())Weak->Command(Id);return FReply::Handled();})[Copy(Label,15)];TitleActions.Add(Id,B);return B;};
 auto Body=SNew(SVerticalBox);const FString Page=G->PhonePage76;
 auto Status=SNew(SHorizontalBox);Status->AddSlot().FillWidth(1)[Copy(TEXT("21:47"),12,Ash77)];Status->AddSlot().AutoWidth()[Copy(TEXT("НЕТ СЕТИ   82%"),10,Ash77)];Body->AddSlot().AutoHeight().Padding(2,0,2,25)[Status];
 const FString Title=Page==TEXT("home")?TEXT("БЕКА"):Page==TEXT("messages")?TEXT("Сообщения"):Page==TEXT("aruzhan")?TEXT("Аружан"):Page==TEXT("weather")?TEXT("Погода"):Page==TEXT("calls")?TEXT("Вызовы"):Page==TEXT("notes")?TEXT("Заметки"):Page==TEXT("settings")?TEXT("Связь"):TEXT("Insta");
 Body->AddSlot().AutoHeight().Padding(0,0,0,5)[Copy(Title,Page==TEXT("home")?35:27)];
 Body->AddSlot().AutoHeight().Padding(0,0,0,19)[Copy(Page==TEXT("home")?TEXT("ОТДЕЛЕНИЕ 04 / ЛИЧНЫЙ ТЕЛЕФОН"):TEXT("Локальные данные · без интернета"),10,Ash77)];
 auto Content=SNew(SVerticalBox);
 if(Page==TEXT("home"))
 {
  Content->AddSlot().AutoHeight().Padding(0,3,0,14)[Copy(TEXT("21:47\nВторник, 10 ноября"),17,Ash77)];
  auto Grid=SNew(SUniformGridPanel).SlotPadding(FMargin(7,10));
  const TCHAR* Labels[]={TEXT("Сообщения"),TEXT("Insta"),TEXT("Погода"),TEXT("Вызовы"),TEXT("Заметки"),TEXT("Связь")};
  const TCHAR* Ids[]={TEXT("messages"),TEXT("insta"),TEXT("weather"),TEXT("calls"),TEXT("notes"),TEXT("settings")};
  for(int I=0;I<6;++I)
  {
   const FString Id=TEXT("phone76_")+FString(Ids[I]);auto App=SNew(SVerticalBox);
   auto B=SNew(SButton).ButtonStyle(&AppStyle).ContentPadding(10).OnClicked_Lambda([Weak=Mode,Id](){if(Weak.IsValid())Weak->Command(Id);return FReply::Handled();})[SNew(SBox).WidthOverride(62).HeightOverride(62)[SNew(SPhoneArt77).Icon(I)]];TitleActions.Add(Id,B);
   App->AddSlot().AutoHeight().HAlign(HAlign_Center)[B];App->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,9,0,0)[Copy(Labels[I],12,Paper77,140)];Grid->AddSlot(I%2,I/2)[App];
  }
  Content->AddSlot().AutoHeight()[Grid];
  if(G->Flag(TEXT("phone76_received"))&&!G->Flag(TEXT("phone76_read")))Content->AddSlot().AutoHeight().Padding(0,17)[Action(TEXT("1 новое · Аружан"),TEXT("phone76_aruzhan"))];
 }
 else if(Page==TEXT("messages"))
 {
  if(G->Flag(TEXT("phone76_received")))Content->AddSlot().AutoHeight().Padding(0,12)[Action(TEXT("Аружан\nЕсли услышишь мой голос…"),TEXT("phone76_aruzhan"))];
  else Content->AddSlot().AutoHeight().Padding(0,20)[Copy(TEXT("Сообщений пока нет."),17,Ash77)];
 }
 else if(Page==TEXT("aruzhan")&&G->Flag(TEXT("phone76_received")))
 {
  Content->AddSlot().AutoHeight()[Copy(TEXT("ПЕРЕД КПП · 21:47"),10,Ash77)];
  Content->AddSlot().AutoHeight().Padding(0,12,15,10)[SNew(SBorder).BorderImage(&Bubble).Padding(15)[Copy(TEXT("Бека, если услышишь мой голос — не иди на него.\n\nЭто не я. Я напишу, когда будет безопасно."),17,Paper77,287)]];
  if(G->Flag(TEXT("phone76_sent"))){Content->AddSlot().AutoHeight().Padding(15,5,0,6)[SNew(SBorder).BorderImage(&Sent).Padding(15)[Copy(TEXT("Понял. На голос не пойду. Я у КПП."),17,Paper77,287)]];Content->AddSlot().AutoHeight().HAlign(HAlign_Right)[Copy(TEXT("Отправлено"),10,Ash77)];}
  else {Content->AddSlot().AutoHeight().Padding(0,18)[Action(TEXT("F · Ответить Аружан"),TEXT("phone76_send"))];Content->AddSlot().AutoHeight()[Copy(TEXT("«Понял. На голос не пойду. Я у КПП.»"),12,Ash77)];}
 }
 else if(Page==TEXT("weather")){Content->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,15)[SNew(SBox).WidthOverride(90).HeightOverride(90)[SNew(SPhoneArt77).Icon(2)]];Content->AddSlot().AutoHeight()[Copy(TEXT("+8°\nПасмурно"),30)];Content->AddSlot().AutoHeight().Padding(0,18)[Copy(TEXT("Ветер 4 м/с\nСохранённый прогноз. Для обновления нет интернета."),15,Ash77)];}
 else if(Page==TEXT("calls"))Content->AddSlot().AutoHeight().Padding(0,20)[Copy(G->Completed.Contains(TEXT("HOME_INVITATION52"))?TEXT("Аружан\nВходящий звонок · дома\n\nНовые вызовы недоступны: нет сети."):TEXT("Пока нет завершённых вызовов."),17)];
 else if(Page==TEXT("notes"))Content->AddSlot().AutoHeight().Padding(0,20)[Copy(G->Objective(),17)];
 else if(Page==TEXT("settings"))Content->AddSlot().AutoHeight().Padding(0,20)[Copy(TEXT("Связь недоступна\nБатарея: 82%\nЗвук уведомлений: включён\n\nПолученные сообщения хранятся на устройстве."),17)];
 else Content->AddSlot().AutoHeight().Padding(0,25)[Copy(TEXT("Нет подключения к интернету.\n\nЛента Insta и новые публикации недоступны."),18)];
 auto Scroll=SNew(SScrollBox).ScrollBarThickness(FVector2D(3,3));Scroll->AddSlot()[Content];Body->AddSlot().FillHeight(1)[Scroll];
 if(Page!=TEXT("home"))Body->AddSlot().AutoHeight().Padding(0,10)[Action(TEXT("На главный экран"),TEXT("phone76_home"))];
 Body->AddSlot().AutoHeight().Padding(0,12,0,0)[Action(TEXT("P / Esc · убрать"),TEXT("phone76_close"))];
 auto Phone=SNew(SBorder).BorderImage(&Shell).Padding(9)[SNew(SOverlay)+SOverlay::Slot()[SNew(SBorder).BorderImage(&Glass)]+SOverlay::Slot()[SNew(SPhoneArt77).Icon(-1)]+SOverlay::Slot().Padding(25,20)[Body]];
 return SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0,0,130,0)[SNew(SBox).WidthOverride(430).HeightOverride(868)[Phone]]]];
}
