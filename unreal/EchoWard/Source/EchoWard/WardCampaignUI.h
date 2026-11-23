#pragma once
#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

class AEWardCampaignMode;
class SVerticalBox;
struct FWardBookMapState;
class FWardCampaignUI : public TSharedFromThis<FWardCampaignUI>
{
public:
    explicit FWardCampaignUI(AEWardCampaignMode* InMode);
    void Open();
    void Close();
    void Refresh();
    TSharedPtr<SWidget> Root;
    TMap<FString,TWeakPtr<SWidget>> TitleActions;
    TMap<FString,TFunction<float()>> TitleMotion;
    FString Section=TEXT("display"),SelectedItem=TEXT("badge"),PendingCode;
    int32 GuidePage=0,FilmTurn=1;
    FVector2D FilmOffset=FVector2D(72,-48);
    TArray<FString> Treatment;
    FString SaveFeedback;
    FString PreviewSearch51;
    int32 InventorySlot=0,DocumentIndex=0;
    FString CombineItem,FieldFeedback;
    FString DocumentQuery66,SettingsReturn66,HelpReturn66;
    bool UnreadOnly66=false;
    TSharedRef<SWidget> QuestMenu66();
    bool BookMapVisible=true;
    TSharedPtr<FWardBookMapState> BookMapState;
    TSharedRef<SWidget> PhoneMenu76();
    TSharedRef<SWidget> FieldMenu();
    bool FieldCommand(const FString& Action);
private:
    TWeakObjectPtr<AEWardCampaignMode> Mode;
    TSharedRef<SWidget> Build();
    TSharedRef<SWidget> AdminCatalog51();
    TSharedRef<SWidget> NarrativeMenu35();
    TSharedRef<SWidget> Title();
    TSharedRef<SWidget> GameMenu();
    TSharedRef<SWidget> PauseMenu();
    TSharedRef<SWidget> DifficultyMenu();
    TSharedRef<SWidget> SaveMenu();
    TSharedRef<SWidget> AboutMenu();
    TSharedRef<SWidget> IllustratedGuide(const FString& Kind);
    TSharedRef<SWidget> TitleSection(TSharedRef<SWidget> Body);
    TSharedRef<SWidget> Media(const FString& Kind);
    TSharedRef<SWidget> Button(const FString& Label,const FString& Action,bool Accent=false,bool Enabled=true);
    TSharedRef<SWidget> Text(const FString& Value,int32 Size=18,FLinearColor Color=FLinearColor(.78f,.84f,.81f));
    TSharedRef<SWidget> Panel(TSharedRef<SWidget> Body);
    TSharedRef<SWidget> Guide(const FString& Kind);
    TSharedRef<SWidget> Settings();
    TSharedRef<SWidget> SettingsMenu();
    TSharedRef<SWidget> Saves();
    TSharedRef<SWidget> Inventory();
    TSharedRef<SWidget> Map();
    TSharedRef<SWidget> Puzzle();
    TSharedRef<SWidget> Inspect();
};

TSharedRef<SWidget> MakeWardTitleFade(TSharedRef<SWidget> Body,AEWardCampaignMode* Mode,bool Intro=false);
