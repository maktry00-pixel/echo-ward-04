#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"

class AEWardCampaignMode;

namespace WardTitleMenu
{
    TSharedRef<SWidget> Shade(TWeakObjectPtr<AEWardCampaignMode> Mode);
    TSharedRef<SWidget> Logo(TWeakObjectPtr<AEWardCampaignMode> Mode);
    TSharedRef<SWidget> Action(TWeakObjectPtr<AEWardCampaignMode> Mode, FOnClicked Handler,
        TSharedRef<SWidget> Content, TAttribute<bool> Enabled = TAttribute<bool>(true));
}
