#include "WardCampaignUI.h"
#include "WardCampaign.h"
#include "WardHospitalGameplay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/Paths.h"
using namespace WardData;
TSharedRef<SWidget> FWardCampaignUI::AdminCatalog51()
{
    auto* G=Mode.Get();TitleActions.Reset();auto Body=SNew(SVerticalBox);
    if(!G->Admin)return Panel(Text(TEXT("Каталог доступен только администратору")));
    auto Top=SNew(SHorizontalBox);Top->AddSlot().FillWidth(1)[Text(TEXT("ПРОВЕРКА ПРЕДМЕТОВ И ДОКУМЕНТОВ"),25)];Top->AddSlot().AutoWidth().Padding(0,0,8,0)[Button(TEXT("Панель администратора"),TEXT("adm79_tools"))];Top->AddSlot().AutoWidth()[Button(TEXT("Закрыть · F8"),TEXT("preview_close"))];Body->AddSlot().AutoHeight()[Top];
    if(G->Screen==TEXT("admin_audio"))
    {
        Body->AddSlot().AutoHeight().Padding(0,10)[Text(TEXT("ЗВУКИ: выбрать строку для прослушивания. Следующий выбор останавливает предыдущий."),16)];
        Body->AddSlot().AutoHeight()[Button(TEXT("Стоп / к предметам"),TEXT("audio68_back"))];
        auto Scroll=SNew(SScrollBox);const auto M=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Audio68/manifest.json"));
        TMap<FString,FString> Labels;for(auto V:Array(M,TEXT("catalog"))){auto Row=V->AsObject();Labels.Add(String(Row,TEXT("id")),String(Row,TEXT("title")));}
        TArray<FString> Keys;for(auto E:Object(M,TEXT("groups"))->Values)Keys.Add(FString(E.Key));Keys.Sort();
        for(const auto& Key:Keys)Scroll->AddSlot().Padding(0,2)[Button(Key+TEXT("  ")+Labels.FindRef(Key),TEXT("audio68_play:")+Key)];
        Body->AddSlot().FillHeight(1)[Scroll];
    }
    else if(G->Screen==TEXT("admin_document"))
    {
        const FString Key=G->PreviewDocument51;Body->AddSlot().AutoHeight().Padding(0,15)[Text(G->HospitalStory->DocumentTitle(TEXT("@doc:")+Key),25)];
        auto Scroll=SNew(SScrollBox);Scroll->AddSlot()[SNew(SBox).MaxDesiredWidth(1760)[SNew(STextBlock).Text(FText::FromString(G->HospitalStory->DocumentBody(TEXT("@doc:")+Key))).Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),22)).ColorAndOpacity(FLinearColor(.9,.95,.94)).WrapTextAt(1720).AutoWrapText(true)]];Body->AddSlot().FillHeight(1).Padding(0,12)[Scroll];
        auto Row=SNew(SHorizontalBox);Row->AddSlot().FillWidth(1)[Button(TEXT("Взять лист в руки"),TEXT("preview_hold_doc:")+Key)];Row->AddSlot().FillWidth(1)[Button(TEXT("К списку"),TEXT("preview_catalog"))];Body->AddSlot().AutoHeight()[Row];
    }
    else
    {
        Body->AddSlot().AutoHeight().Padding(0,12)[Text(TEXT("Взять — осмотр от первого лица. Читать — полный текст. Инвентарь, журнал и сюжетный прогресс не меняются."),16)];
        Body->AddSlot().AutoHeight().Padding(0,0,0,12)[SNew(SEditableTextBox).Text(FText::FromString(PreviewSearch51)).HintText(FText::FromString(TEXT("Поиск по названию или ID"))).OnTextChanged_Lambda([this](const FText& T){PreviewSearch51=T.ToString();})];
        auto Items=SNew(SScrollBox),Docs=SNew(SScrollBox);int32 NI=0,ND=0;
        for(const auto& E:G->PreviewCatalog51())
        {
            auto Row=SNew(SHorizontalBox);Row->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Text(E.Title+TEXT("  /  ")+E.Id,16)];Row->AddSlot().AutoWidth()[Button(TEXT("Взять в руки"),TEXT("preview_hold_")+E.Id)];
            if(E.Document){++ND;Row->AddSlot().AutoWidth().Padding(6,0)[Button(TEXT("Читать"),TEXT("preview_read_")+E.Visual)];Docs->AddSlot().Padding(0,3)[SNew(SBox).Visibility_Lambda([this,E](){return PreviewSearch51.IsEmpty()||(E.Title+E.Id).Contains(PreviewSearch51)?EVisibility::Visible:EVisibility::Collapsed;})[Row]];}else{++NI;Items->AddSlot().Padding(0,3)[SNew(SBox).Visibility_Lambda([this,E](){return PreviewSearch51.IsEmpty()||(E.Title+E.Id).Contains(PreviewSearch51)?EVisibility::Visible:EVisibility::Collapsed;})[Row]];}
        }
        auto Columns=SNew(SHorizontalBox);auto Left=SNew(SVerticalBox),Right=SNew(SVerticalBox);Left->AddSlot().AutoHeight()[Text(FString::Printf(TEXT("Предметы: %d"),NI),20)];Left->AddSlot().FillHeight(1)[Items];Right->AddSlot().AutoHeight()[Text(FString::Printf(TEXT("Все записки: %d"),ND),20)];Right->AddSlot().FillHeight(1)[Docs];
        Columns->AddSlot().FillWidth(1).Padding(0,0,12,0)[Left];Columns->AddSlot().FillWidth(1)[Right];Body->AddSlot().FillHeight(1)[Columns];
        auto Bottom=SNew(SHorizontalBox);Bottom->AddSlot().FillWidth(1)[Button(TEXT("Убрать из рук"),TEXT("preview_clear"))];Bottom->AddSlot().FillWidth(1)[Button(TEXT("Проверка звуков"),TEXT("audio68_catalog"))];Bottom->AddSlot().FillWidth(1).Padding(8,0)[Button(TEXT("Свободная камера"),TEXT("preview_fly"))];Bottom->AddSlot().FillWidth(1)[Button(TEXT("Выйти из администратора"),TEXT("admin_exit"))];Body->AddSlot().AutoHeight().Padding(0,12)[Bottom];
    }
    static FSlateColorBrush Backdrop(FLinearColor(.006,.012,.014,1));
    return SNew(SBorder).BorderImage(&Backdrop).Padding(28)[SNew(SBox).HeightOverride(850)[Body]];
}
