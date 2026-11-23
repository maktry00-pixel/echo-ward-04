#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace WardData
{
    using Json=TSharedPtr<FJsonObject>;
    Json Read(const FString& File);
    Json Campaign();
    FString DisplayNames36(const FString& Text);
    Json Object(Json D,const FString& Key);
    FString String(Json D,const FString& Key,const FString& Default=TEXT(""));
    double Number(Json D,const FString& Key,double Default=0);
    bool Boolean(Json D,const FString& Key,bool Default=false);
    const TArray<TSharedPtr<FJsonValue>>& Array(Json D,const FString& Key);
    FVector Vector(const TArray<TSharedPtr<FJsonValue>>& A,FVector Default=FVector::ZeroVector);
    FTransform Transform(Json D);
    FString Serialize(Json D);
    bool Write(const FString& Path,Json D);
    FString MeshPath(const FString& Id);
    FString MaterialPath(const FString& Id);
}
