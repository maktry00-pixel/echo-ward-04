#pragma once

namespace WardVisual67
{
inline int32 CampusMesh(const FString& Kind)
{
    static const TMap<FString,int32> Models={
        {TEXT("water_dispenser"),6767},{TEXT("visitor_pair"),701},
        {TEXT("bedside_cabinet"),3304},{TEXT("wheelchair_bay"),3303},
        {TEXT("treatment_trolley"),3305},{TEXT("crash_cart"),3306},
        {TEXT("cleaning_cart"),194},{TEXT("mail_cart"),192},{TEXT("chart_cart"),192},
        {TEXT("corridor_bin"),270},{TEXT("bench"),466},
        {TEXT("personal_locker"),242},{TEXT("clinical_cabinet"),241},
        {TEXT("archive_drawers"),239},{TEXT("retail_chiller"),420},
        {TEXT("therapy_shelf"),240}
    };
    return Models.FindRef(Kind);
}

inline WardData::Json FitBounds(const FBox& Bounds)
{
    auto Fit=MakeShared<FJsonObject>();
    Fit->SetArrayField(TEXT("bounds"),{
        MakeShared<FJsonValueNumber>(Bounds.Min.X/100),MakeShared<FJsonValueNumber>(Bounds.Min.Y/100),
        MakeShared<FJsonValueNumber>(Bounds.Min.Z/100),MakeShared<FJsonValueNumber>(Bounds.Max.X/100),
        MakeShared<FJsonValueNumber>(Bounds.Max.Y/100),MakeShared<FJsonValueNumber>(Bounds.Max.Z/100)});
    return Fit;
}
}
