#pragma once
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"
#include "WardCampaignData.h"
#include "WardEchoAssets.h"

// Reuse cooked, world-projected PBR surfaces. The former Source argument in
// BookWorld was ignored, so imported plaster/terrazzo never reached this map.
namespace WardHospitalMaterials
{
inline UMaterialInstanceDynamic* Variant(UObject* Owner,UMaterialInterface* Source)
{
    // Unreal refuses a MID as another MID's parent. Keep the valid static
    // parent, then copy the complete uniform parameter values explicitly.
    auto* Dynamic=Cast<UMaterialInstanceDynamic>(Source);
    UMaterialInterface* Parent=Dynamic?Dynamic->Parent.Get():Source;
    auto* Result=UMaterialInstanceDynamic::Create(Parent,Owner);
    if(Dynamic)Result->CopyMaterialUniformParameters(Dynamic);
    return Result;
}
// Keep the title scene's approved grey plaster / blue-grey paint. The v20
// large green peeling pattern is intentionally not used for any wall.
inline UMaterialInstanceDynamic* Title(UObject* Owner,const FString& Source,FLinearColor Tint,float Roughness)
{
    auto* Parent=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Campaign/Materials/MI_%s.MI_%s"),*Source,*Source));
    auto* M=UMaterialInstanceDynamic::Create(Parent,Owner);
    M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetScalarParameterValue(TEXT("Roughness"),Roughness);
    return M;
}
inline UMaterialInstanceDynamic* Make(UObject* Owner,const FString& Source,FLinearColor Tint,float Roughness)
{
    UMaterialInterface* Parent=nullptr;
    const TCHAR* Detail=Source==TEXT("4ae6673fcc8b19e7")?TEXT("Plaster"):Source==TEXT("05227396fb5ca2d8")?TEXT("Paint"):Source==TEXT("527b19fd5563fa6d")?TEXT("Floor"):nullptr;
    if(Detail&&WardData::Number(WardEchoAssets::DetailArt(),TEXT("version"))>=26)
    {
        const TCHAR* Folder=WardData::Number(WardEchoAssets::HorrorArt28(),TEXT("version"))>=28?TEXT("HorrorV28"):TEXT("DetailV26");
        Parent=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Campaign/%s/MI_%s.MI_%s"),Folder,Detail,Detail));
        if(Parent)
        {
            auto* M=UMaterialInstanceDynamic::Create(Parent,Owner);
            if(Source!=TEXT("527b19fd5563fa6d"))M->SetVectorParameterValue(TEXT("Tint"),FMath::Lerp(M->K2_GetVectorParameterValue(TEXT("Tint")),Tint,.08f));
            return M;
        }
    }
    if(Source==TEXT("echo_metal"))
    {
        Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/EchoCustom/Materials/MI_V22_Metal.MI_V22_Metal"));
        if(Parent)return UMaterialInstanceDynamic::Create(Parent,Owner);
    }
    if(Source==TEXT("echo_asphalt"))
    {
        if(WardData::Number(WardEchoAssets::Realism30(),TEXT("version"))>=30)
        {
            Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/RealismV30/MI_Road30.MI_Road30"));
            if(Parent)return UMaterialInstanceDynamic::Create(Parent,Owner);
        }

        Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/EchoCustom/Materials/MI_V22_Road.MI_V22_Road"));
        if(Parent)return UMaterialInstanceDynamic::Create(Parent,Owner);
    }
    const bool Wall=Source==TEXT("4ae6673fcc8b19e7")||Source==TEXT("05227396fb5ca2d8");
    if(!Wall)if(auto* Custom=WardEchoAssets::Surface(Source))
    {
        auto* M=UMaterialInstanceDynamic::Create(Custom,Owner);
        // Authored albedo already carries peeling paint and damp patches.
        // Keep a little of each department's identity without tinting twice.
        const bool Floor=Source==TEXT("527b19fd5563fa6d");
        M->SetVectorParameterValue(TEXT("Tint"),Floor?FLinearColor(.62f,.72f,.72f):FMath::Lerp(FLinearColor(.72f,.78f,.76f),Tint,.24f));
        M->SetScalarParameterValue(TEXT("Roughness"),Floor?.72f:.88f);
        M->SetScalarParameterValue(TEXT("NormalStrength"),Floor?.62f:.42f);
        M->SetVectorParameterValue(TEXT("Emission"),FLinearColor::Black);return M;
    }
    WardData::Json Surface;
    if(!Source.IsEmpty())for(auto V:WardData::Array(WardData::Campaign(),TEXT("materials")))if(WardData::String(V->AsObject(),TEXT("id"))==Source){Surface=V->AsObject();break;}
    if(!Source.IsEmpty())Parent=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Campaign/Materials/MI_%s.MI_%s"),*Source,*Source));
    // Bind to the known textured master explicitly. Older cooked material
    // instances can retain a different parent or stale texture parameters.
    if(Surface.IsValid()&&!WardData::String(Surface,TEXT("albedo")).IsEmpty())Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_CampaignWorldPBR.M_CampaignWorldPBR"));
    if(!Parent)Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalSurface.M_HospitalSurface"));
    auto* M=UMaterialInstanceDynamic::Create(Parent,Owner);
    M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetScalarParameterValue(TEXT("Roughness"),Roughness);
    M->SetScalarParameterValue(TEXT("Wetness"),0);M->SetVectorParameterValue(TEXT("Emission"),FLinearColor::Black);
    if(Surface.IsValid()&&!WardData::String(Surface,TEXT("albedo")).IsEmpty())
    {
        for(const auto& Pair:TArray<TPair<FString,FString>>{{TEXT("Albedo"),TEXT("albedo")},{TEXT("Normal"),TEXT("normal")},{TEXT("RoughnessMap"),TEXT("roughness_texture")}})
        {
            const FString Id=WardData::String(Surface,*Pair.Value);M->SetTextureParameterValue(*Pair.Key,LoadObject<UTexture>(nullptr,*FString::Printf(TEXT("/Game/Campaign/Textures/T_%s.T_%s"),*Id,*Id)));
        }
        const auto& UV=WardData::Array(Surface,TEXT("uv"));M->SetVectorParameterValue(TEXT("Tiling"),FLinearColor(UV[0]->AsNumber(),UV[1]->AsNumber(),1,0));
        M->SetVectorParameterValue(TEXT("RoughnessChannel"),FLinearColor(1,0,0,0));M->SetScalarParameterValue(TEXT("NormalStrength"),WardData::Number(Surface,TEXT("normal_scale")));
    }
    if(Wall)
    {
        const bool Lower=Source==TEXT("05227396fb5ca2d8");
        // Same textures and restrained colours as the former main menu;
        // world projection prevents stretching across merged corridor walls.
        M->SetVectorParameterValue(TEXT("Tint"),Lower?FLinearColor(.20f,.32f,.30f):FLinearColor(.58f,.62f,.58f));
        M->SetScalarParameterValue(TEXT("NormalStrength"),Lower?.28f:.32f);
    }
    if(Surface.IsValid()&&WardData::Boolean(Surface,TEXT("wood")))
    {
        // The original grain was authored for a small mesh. On a 100cm
        // instanced cube it produced a dense repeating checker-like pattern.
        M->SetScalarParameterValue(TEXT("Wood"),1);M->SetVectorParameterValue(TEXT("Grain"),FLinearColor(.14f,.18f,.14f,1));
    }
    return M;
}
}
