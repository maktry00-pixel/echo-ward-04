#include "WardBookWorld.h"
#include "WardEchoAssets.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
using namespace WardData;
void AEWardBookWorld::BuildDetailDressing()
{
    const Json D=WardEchoAssets::DetailArt();if(Number(D,TEXT("version"))<26)return;
    TSet<FString> ExistingGroups;for(const auto& Pair:Groups)ExistingGroups.Add(Pair.Key);
    for(auto V:Array(D,TEXT("props")))
    {
        const Json P=V->AsObject();Json Fit=MakeShared<FJsonObject>();Fit->SetArrayField(TEXT("bounds"),Array(P,TEXT("bounds")));
        if(!AddCustomProp(Number(P,TEXT("mesh")),Fit,Vector(Array(P,TEXT("at")))*100,Number(P,TEXT("turn"))))continue;
        ++DetailProps;const FString Kind=String(P,TEXT("category"));
        if(Kind==TEXT("exterior_air_conditioner"))++DetailExteriorProps;
        else if(Kind==TEXT("door_closer")||Kind==TEXT("metal_threshold")||Kind==TEXT("corner_protector")||Kind==TEXT("window_handle")||Kind==TEXT("partly_lowered_blind"))++DetailOpeningProps;
        else ++DetailWallProps;
    }
    // Flush millimetre-thick fittings do not need their own dynamic shadow
    // depth submissions. Preserve every pre-existing group's shadow policy.
    for(auto& Pair:Groups)
    {
        if(ExistingGroups.Contains(Pair.Key))continue;
        for(int Id:{46,108,136,138,139,140})if(Pair.Key.StartsWith(FString::Printf(TEXT("echo_%d_"),Id))){Pair.Value->SetCastShadow(false);break;}
    }
    const Json D27=WardEchoAssets::DetailArt27();
    if(Number(D27,TEXT("version"))<27)return;
    TSet<FString> GroupsBefore27;for(const auto& Pair:Groups)GroupsBefore27.Add(Pair.Key);
    AddingDetail27=true;
    for(auto V:Array(D27,TEXT("props")))
    {
        const Json P=V->AsObject();Json Fit=MakeShared<FJsonObject>();Fit->SetArrayField(TEXT("bounds"),Array(P,TEXT("bounds")));
        if(!AddCustomProp(Number(P,TEXT("mesh")),Fit,Vector(Array(P,TEXT("at")))*100,Number(P,TEXT("turn"))))continue;
        ++Detail27Props;const FString Kind=String(P,TEXT("category"));
        if(Kind.StartsWith(TEXT("supported_")))++Detail27Supported;
        else if(Kind==TEXT("ceiling_services"))++Detail27Services;
        else if(Kind==TEXT("hygiene_station")||Kind==TEXT("fire_hose_cabinet"))++Detail27Wall;
    }
    AddingDetail27=false;
    // Small table supplies and overhead pipes use contact/AO shading; retaining
    // their own moving shadow submissions is expensive across the full campus.
    for(auto& Pair:Groups)
    {
        if(GroupsBefore27.Contains(Pair.Key))continue;
        Pair.Value->SetCullDistance(2200);
        for(int Id:{901,902,909,910})if(Pair.Key.StartsWith(FString::Printf(TEXT("echo_%d_"),Id))){Pair.Value->SetCullDistance(4500);break;}
        for(int Id:{645,764,765,766,768,769,770,772,777,780,781,782,783,786,788,789,790,792,798,800,801,864,867,868,870,908,909,911,912})
            if(Pair.Key.StartsWith(FString::Printf(TEXT("echo_%d_"),Id))){Pair.Value->SetCastShadow(false);break;}
    }
}
