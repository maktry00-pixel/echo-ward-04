#include "WardCampaignUI.h"
#include "WardCampaign.h"
#include "WardAudio68.h"
#include "Brushes/SlateColorBrush.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "WardDialogue62.h"
#include "WardHospitalGameplay.h"
#include "WardPeople54.h"
#include "WardStaging64.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SBox.h"

using namespace WardData;

TSharedRef<SWidget> FWardCampaignUI::NarrativeMenu35()
{
    TitleActions.Reset();
    TitleMotion.Reset();
    auto* S = Mode->HospitalStory.Get();
    if (!S || !WardDialogue62::HasControls(*S)) return SNullWidget::NullWidget;

    auto Controls = SNew(SVerticalBox);
    if (WardDialogue62::AwaitingChoice(*S))
    {
        const bool Checkpoint = S->ActiveScene35 == TEXT("CHECKPOINT_69");
        Controls->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,5)
        [Text(Checkpoint ? TEXT("ВЫБЕРИТЕ ОТВЕТ") : TEXT("ВЫБЕРИТЕ ДЕЙСТВИЕ"), 12, FLinearColor(.68,.78,.78))];
        const auto& Options = Array(S->NarrativeScenes35.FindRef(S->ActiveScene35), TEXT("choices"));
        for (int Index = 0; Index < Options.Num(); ++Index)
        {
            const Json Option = Options[Index]->AsObject();
            const FString Id = String(Option, TEXT("id"));
            if (Id == TEXT("STAY") && S->ActiveScene35 == TEXT("SCENE_19_01") &&
                String(Object(S->NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN")) != TEXT("DESTROY")) continue;
            FString Missing;
            const bool Allowed = S->NarrativeRequirements35(Option, &Missing);
            static const FButtonStyle ChoiceStyle = FButtonStyle()
                .SetNormal(FSlateColorBrush(FLinearColor(.012f,.019f,.023f,.88f)))
                .SetHovered(FSlateColorBrush(FLinearColor(.20f,.035f,.031f,.97f)))
                .SetPressed(FSlateColorBrush(FLinearColor(.13f,.022f,.019f,.98f)))
                .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
            const FString Action = TEXT("story35_choice_") + Id;
            const FString Label = String(Option, TEXT("label")) + (Allowed ? FString() : TEXT("\n") + Missing);
            const FSlateFontInfo Font(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"), 16);
            auto Reply = SNew(SButton).ButtonStyle(&ChoiceStyle).IsEnabled(Allowed).ContentPadding(FMargin(16,9))
                .OnHovered_Lambda([Weak=Mode]()
                {
                    if (auto* G = Weak.Get()) WardAudio68::Play(G, TEXT("UI/hover"), .07f*G->Setting(TEXT("effects"), .8));
                })
                .OnClicked_Lambda([Weak=Mode,Action]()
                {
                    if (auto* G = Weak.Get()) G->Command(Action);
                    return FReply::Handled();
                })
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
                    [SNew(SBox).WidthOverride(32)[Text(FString::FromInt(Index+1), 15, FLinearColor(.58,.72,.72))]]
                    + SHorizontalBox::Slot().FillWidth(1)
                    [SNew(STextBlock).Text(FText::FromString(Mode->Tr(Label))).Font(Font).WrapTextAt(520)
                        .ColorAndOpacity(FLinearColor(.94,.95,.95))]
                ];
            TitleActions.Add(Action, Reply);
            Controls->AddSlot().AutoHeight().Padding(0,2)[Reply];
        }
    }
    if (WardStaging64::WaitingAction(*S))
        Controls->AddSlot().AutoHeight()[Button(TEXT("Подтвердить этап"), TEXT("story35_action64"))];
    if (S->ActiveScene35.StartsWith(TEXT("NPC_")))
    {
        if (auto* Actor = S->Props.FindRef(S->ActiveScene35.RightChop(4)))
            if (auto* Person = Actor->FindComponentByClass<UEWardPerson54>(); Person && !Person->IsDead58())
            {
                auto Actions = SNew(SHorizontalBox);
                if (Person->CanEscort58())
                {
                    Actions->AddSlot().AutoWidth().Padding(3,0)[Button(TEXT("Идём со мной"), TEXT("npc58_follow"))];
                    Actions->AddSlot().AutoWidth().Padding(3,0)[Button(TEXT("Жди здесь"), TEXT("npc58_wait"))];
                    Actions->AddSlot().AutoWidth().Padding(3,0)[Button(TEXT("Можешь идти"), TEXT("npc58_return"))];
                }
                if (Person->IsFallen58())
                    Actions->AddSlot().AutoWidth().Padding(3,0)[Button(TEXT("Помочь подняться"), TEXT("npc58_help"))];
                Controls->AddSlot().AutoHeight().HAlign(HAlign_Center)[Actions];
            }
    }
    return SNew(SVerticalBox).Visibility(EVisibility::SelfHitTestInvisible)
        + SVerticalBox::Slot().FillHeight(1)[SNullWidget::NullWidget]
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(32,0,32,45)
        [SNew(SBox).MinDesiredWidth(440).MaxDesiredWidth(660)[Controls]];
}
