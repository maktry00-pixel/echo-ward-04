#include "WardGraphics.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

namespace EWardGraphics
{
    static int32 Active=1;
    int32 Current() { return Active; }
    FString Label()
    {
        const TCHAR* Names[]={TEXT("КАЧЕСТВО · 100%"),TEXT("БАЛАНС · 100%"),TEXT("ПРОИЗВОДИТЕЛЬНОСТЬ · TSR 67%")};
        return Names[Active];
    }
    void Apply(int32 Preset,bool Persist)
    {
        Active=FMath::Clamp(Preset,0,2);
        UGameUserSettings* Settings=GEngine?GEngine->GetGameUserSettings():nullptr;
        if(!Settings)return;
        Settings->SetOverallScalabilityLevel(Active==0?3:Active==2?1:2);
        Settings->SetAntiAliasingQuality(Active==0?3:2);
        Settings->SetTextureQuality(Active==0?3:2);
        Settings->SetPostProcessingQuality(Active==0?3:2);
        Settings->SetResolutionScaleValueEx(Active==2?67.f:100.f);
        Settings->SetFrameRateLimit(Active==2?30:60);
        Settings->ApplyNonResolutionSettings();
        auto Set=[](const TCHAR* Name,int32 Value){if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))C->Set(Value,ECVF_SetByCode);};
        const int32 TexturePoolMB[] = {3072, 2560, 1400};
        Set(TEXT("r.Streaming.PoolSize"),TexturePoolMB[Active]);
        Set(TEXT("r.Streaming.LimitPoolSizeToVRAM"),1);
        Set(TEXT("r.Streaming.UsePerTextureBias"),1);
        Set(TEXT("r.MaxAnisotropy"),Active==2?8:16);
        Set(TEXT("r.VT.MaxAnisotropy"),Active==2?4:8);
        if(auto* Boost=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.Boost")))
            Boost->Set(Active==0?1.18f:Active==1?1.06f:1.0f,ECVF_SetByCode);
        Set(TEXT("r.Shadow.Virtual.MaxPhysicalPages"),Active==2?1024:2048);
        Set(TEXT("r.Shadow.Virtual.SMRT.RayCountLocal"),Active==0?5:Active==1?3:2);
        Set(TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayLocal"),Active==0?5:Active==1?3:2);
        Set(TEXT("r.SSGI.Quality"),Active==0?2:1);
        Set(TEXT("r.SSR.Quality"),Active==2?1:Active==0?3:2);
        Set(TEXT("r.SSR.HalfResSceneColor"),Active==2?1:0);
        Set(TEXT("r.SSR.Temporal"),Active==2?0:1);
        Set(TEXT("r.VolumetricFog.GridPixelSize"),Active==0?8:Active==1?12:16);
        Set(TEXT("r.VolumetricFog.GridSizeZ"),Active==0?96:Active==1?72:48);
        if(Persist)
        {
            Settings->SaveSettings();
            GConfig->SetInt(TEXT("EchoWard.Graphics"),TEXT("Preset"),Active,GGameUserSettingsIni);
            GConfig->Flush(false,GGameUserSettingsIni);
        }
    }
    void Load()
    {
        int32 Preset=1;GConfig->GetInt(TEXT("EchoWard.Graphics"),TEXT("Preset"),Preset,GGameUserSettingsIni);
        Apply(Preset,false);
    }
}
