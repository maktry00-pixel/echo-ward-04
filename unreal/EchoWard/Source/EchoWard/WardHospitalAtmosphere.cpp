#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardCampaign.h"
#include "WardGraphics.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
using namespace WardData;
namespace
{
int Circuit(FVector P)
{
    return FMath::Abs(FMath::FloorToInt(P.X/100)*17+FMath::FloorToInt(P.Y/100)*31+FMath::RoundToInt((P.Z-300)/480)*7)%23;
}
}
float AEWardBookWorld::LampPower(FVector At,float Time)const
{
    const int C=Circuit(At);if(C==0||C==2||C==7)return 0;
    if(Time<0)return 1;
    // A single faulty circuit drops briefly every ~14s. This is not a rapid
    // full-screen strobe; the remaining lights and torch stay available.
    if(C==6){const float Phase=FMath::Fmod(Time,14.3f);if(Phase>8.1f&&Phase<8.25f)return .07f;if(Phase>8.48f&&Phase<8.58f)return .28f;}
    return .96f+.04f*FMath::Sin(Time*.67f+C*1.71f);
}
int AEWardBookWorld::WhiteLampMaterial(FVector At)const
{
    const int C=Circuit(At);if((C==0||C==2||C==7)&&DormantLampMaterial!=INDEX_NONE)return DormantLampMaterial;
    return C==6&&FlickerLampMaterial!=INDEX_NONE?FlickerLampMaterial:4;
}
void AEWardBookWorld::UpdateHorrorAtmosphere(float Delta)
{
    LightSelectionClock+=Delta;
    auto* H=Mode.IsValid()?Mode->Hero():nullptr;if(!H)return;
    const FVector Player=H->GetActorLocation();const FString Region=LocationAt(Player);
    const Json Profile=HorrorProfile(Region);
    const int32 Preset=EWardGraphics::Current();
    const bool Exterior=Region==TEXT("ST")||Region==TEXT("RF")||(Region==TEXT("gallery")&&Player.Z>-100&&Player.Z<400);
    if(!Bright)
    {
        if(Daylight)
        {
            const float Target=Exterior?8.6f:4.6f;
            const float Blend=1.f-FMath::Exp(-3.f*FMath::Clamp(Delta,0.f,.12f));
            const float NewIntensity=FMath::Lerp(Daylight->Intensity,Target,Blend);
            if(FMath::Abs(NewIntensity-Daylight->Intensity)>.015f)
                Daylight->SetIntensity(NewIntensity);
        }
        const float Output=Number(Profile,TEXT("white_lumens"),1800);
        for(int J=0;J<ActiveWhiteIds.Num();++J)
        {
            const FVector At=ActiveWhiteAnchors[J];
            const float Distance=FVector::Dist2D(At,Player);
            // Bring down the closest hotspot while keeping distant openings readable.
            const float Balance=FMath::GetMappedRangeValueClamped(FVector2D(120,1450),FVector2D(.70f,.91f),Distance);
            LightPool[ActiveWhiteIds[J]]->SetIntensity(Output*Balance*LampPower(At,EmergencyClock));
        }
    }
    if(FlickerLamp)FlickerLamp->SetVectorParameterValue(TEXT("Emission"),FLinearColor(2.2f,2.6f,2.85f)*(Bright?1.f:LampPower(FVector(2200,0,300),EmergencyClock)));
    const float FogStrength=Mode->Setting(TEXT("fog"),1);const float Floor=FMath::RoundToInt((Player.Z-96)/480)*480;
    for(int I=0;I<FloorMist.Num();++I)
    {
        const bool Visible=!Bright&&!Exterior&&FogStrength>.001f&&I<(Preset==2?2:4)&&MistAnchors.IsValidIndex(I);
        auto* F=FloorMist[I].Get();F->SetVisibility(Visible);if(!Visible)continue;
        const FVector A=MistAnchors[I];F->SetRelativeLocation(FVector(A.X+FMath::Sin(EmergencyClock*.10f+I)*24,A.Y+FMath::Sin(EmergencyClock*.13f+I)*18,Floor-100));
        F->SetHeightFogExtinction(Number(Profile,TEXT("floor_mist"),.24)*FogStrength*.68f);
        F->SetRadialFogExtinction(.0035f*FogStrength);
        F->FogAlbedo=FLinearColor(.29f,.31f,.35f);
        F->FogEmissive=FLinearColor(.00018f,.00022f,.00028f);
    }
    auto& P=H->Camera->PostProcessSettings;
    P.bOverride_ColorSaturation=true;P.ColorSaturation=Bright?FVector4(1,1,1,1):FVector4(1.02f,.96f,.98f,1);
    P.bOverride_ColorContrast=true;const float C=Mode->Setting(TEXT("contrast"),1)*(Bright?1.f:1.018f);P.ColorContrast=FVector4(C,C,C,1);
    P.bOverride_ColorGammaShadows=true;
    P.ColorGammaShadows=Bright?FVector4(1,1,1,1):Exterior?FVector4(.97f,1.01f,1.045f,1):FVector4(.99f,1.01f,1.025f,1);
    P.bOverride_ColorGain=true;P.ColorGain=Bright?FVector4(1,1,1,1):FVector4(1.02f,.99f,.99f,1);
    P.bOverride_BloomIntensity=true;P.BloomIntensity=Bright?.09f:Exterior?.26f:.22f;
    P.bOverride_BloomThreshold=true;P.BloomThreshold=1.2f;
    P.bOverride_VignetteIntensity=true;P.VignetteIntensity=Mode->SettingBool(TEXT("vignette"),true)?(Bright?.04f:.16f):0;
    P.bOverride_FilmGrainIntensity=true;P.FilmGrainIntensity=Mode->SettingBool(TEXT("grain"),true)?Mode->Setting(TEXT("grain_amount"),.035)*(Bright?.2f:.55f):0;
    P.bOverride_ScreenSpaceReflectionQuality=true;P.ScreenSpaceReflectionQuality=Preset==2?35:Preset==1?65:90;
    P.bOverride_ScreenSpaceReflectionIntensity=true;P.ScreenSpaceReflectionIntensity=Exterior?94:79;
    P.bOverride_ScreenSpaceReflectionMaxRoughness=true;P.ScreenSpaceReflectionMaxRoughness=Exterior?.53f:.45f;
    P.bOverride_AmbientOcclusionIntensity=true;P.AmbientOcclusionIntensity=Bright?.6f:.96f;
    P.bOverride_AmbientOcclusionRadius=true;P.AmbientOcclusionRadius=Exterior?65:78;
    if(HorrorBounce28)
    {
        const bool UseFill=!Bright&&!Exterior&&Mode->CampaignStarted&&Mode->Screen.IsEmpty();
        HorrorBounce28->SetVisibility(UseFill);
        if(UseFill)
        {
            const FVector FillAt=Player+FVector(0,0,95);
            if(FVector::DistSquared(HorrorBounce28->GetRelativeLocation(),FillAt)>FMath::Square(25.f))
                HorrorBounce28->SetRelativeLocation(FillAt);
            const bool Indirect=P.DynamicGlobalIlluminationMethod==EDynamicGlobalIlluminationMethod::ScreenSpace;
            HorrorBounce28->SetIntensity(Indirect?75.f:124.f);
            HorrorBounce28->SetLightColor(FLinearColor(.70f,.77f,.88f));
        }
    }
    if(!Mode->CampaignStarted||!Mode->Screen.IsEmpty())return;
    const float Duck=Mode->Voice&&Mode->Voice->IsPlaying()?.3f:1.f;
    const float Volume=Mode->Setting(TEXT("music"),.7)*Duck;
    // Quiet room tone carries the scene. Score remains far below effects.
    if(Mode->Music)Mode->Music->SetVolumeMultiplier((Bright?.012f:.025f)*Volume);
    if(Mode->Ambience)Mode->Ambience->SetVolumeMultiplier(Number(Profile,TEXT("room_tone"),.14)*Volume);
    if(Region!=SoundRegion){SoundRegion=Region;SoundClock=7.f;}
    if(Bright)return;
    if((SoundClock-=Delta)>0)return;
    const auto& Events=Array(Profile,TEXT("events"));if(Events.IsEmpty()){SoundClock=30;return;}
    const int Seed=FMath::Abs(int(GetTypeHash(Region)))%97;
    const int Choice=(AmbientEventCount+Seed)%Events.Num();LastAmbientCue=Events[Choice]->AsString();
    TArray<FVector> Sources;
    for(FVector A:LightAnchors){const float Distance=FVector::Dist2D(A,Player);if(FMath::Abs(A.Z-Player.Z)<350&&Distance>450&&Distance<1800)Sources.Add(A-FVector(0,0,90));}
    if(Sources.IsEmpty()){SoundClock=24;return;}
    LastAmbientAt=Sources[(AmbientEventCount*7+Seed)%Sources.Num()];
    Mode->Cue(LastAmbientCue,LastAmbientAt,LastAmbientCue==TEXT("whisper")?.075f:.13f);
    ++AmbientEventCount;SoundClock=24.f+(AmbientEventCount*11+Seed)%19;
}
