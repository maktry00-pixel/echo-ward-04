#pragma once
#include "CoreMinimal.h"

namespace EWardGraphics
{
    // 0: Epic/native; 1: High/native; 2: High/75% TSR.
    int32 Current();
    FString Label();
    void Load();
    void Apply(int32 Preset, bool Persist);
}
