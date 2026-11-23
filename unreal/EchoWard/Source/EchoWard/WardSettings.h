#pragma once
#include "WardCampaignData.h"
namespace WardSettings
{
    void AddTranslations(TMap<FString,TPair<FString,FString>>& Catalog);
    inline void Complete(WardData::Json O)
    {
        if(!O->HasField(TEXT("inspection_bright")))O->SetBoolField(TEXT("inspection_bright"),false);
        if(!O->HasField(TEXT("atmosphere_profile")))O->SetNumberField(TEXT("atmosphere_profile"),19);
        if(!O->HasField(TEXT("resolution")))O->SetStringField(TEXT("resolution"),TEXT("1920x1080"));
        if(!O->HasField(TEXT("graphics_preset")))O->SetNumberField(TEXT("graphics_preset"),1);
        if(!O->HasField(TEXT("screen_percentage")))O->SetNumberField(TEXT("screen_percentage"),100);
        if(!O->HasField(TEXT("motion_blur")))O->SetBoolField(TEXT("motion_blur"),false);
        if(!O->HasField(TEXT("grain_amount")))O->SetNumberField(TEXT("grain_amount"),.035);
    }
    struct FRange{float Min,Max,Step;};
    inline FRange Range(const FString& K)
    {
        if(K==TEXT("fov"))return {55,100,1};if(K==TEXT("grain_amount"))return {0,.3f,.005f};
        if(K==TEXT("music")||K==TEXT("effects")||K==TEXT("shake"))return {0,1,.01f};
        if(K==TEXT("hud_opacity"))return {.2f,1,.01f};if(K==TEXT("sensitivity"))return {.25f,2.5f,.05f};
        if(K==TEXT("fog"))return {0,1.5f,.05f};return {.5f,1.5f,.01f};
    }
}
