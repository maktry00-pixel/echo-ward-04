#include "WardCampaignData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"

namespace WardData
{
    Json Read(const FString& File)
    {
        FString Text;Json D=MakeShared<FJsonObject>();
        if(FFileHelper::LoadFileToString(Text,*File))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),D);
        return D.IsValid()?D:MakeShared<FJsonObject>();
    }
    Json Campaign(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/campaign.json"));return D;}
    Json Object(Json D,const FString& Key){const Json* V=nullptr;return D.IsValid()&&D->TryGetObjectField(Key,V)&&V?*V:MakeShared<FJsonObject>();}
    FString String(Json D,const FString& Key,const FString& Default){FString V=Default;if(D)D->TryGetStringField(Key,V);return V;}
    double Number(Json D,const FString& Key,double Default){double V=Default;if(D)D->TryGetNumberField(Key,V);return V;}
    bool Boolean(Json D,const FString& Key,bool Default){bool V=Default;if(D)D->TryGetBoolField(Key,V);return V;}
    const TArray<TSharedPtr<FJsonValue>>& Array(Json D,const FString& Key){static TArray<TSharedPtr<FJsonValue>> Empty;const TArray<TSharedPtr<FJsonValue>>* V=nullptr;return D&&D->TryGetArrayField(Key,V)&&V?*V:Empty;}
    FVector Vector(const TArray<TSharedPtr<FJsonValue>>& A,FVector Default){return A.Num()>=3?FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()):Default;}
    FTransform Transform(Json D)
    {
        auto& Q=Array(D,TEXT("rotation"));FQuat Rotation=Q.Num()==4?FQuat(Q[0]->AsNumber(),Q[1]->AsNumber(),Q[2]->AsNumber(),Q[3]->AsNumber()):FQuat::Identity;
        return FTransform(Rotation,Vector(Array(D,TEXT("position"))),Vector(Array(D,TEXT("scale")),FVector::OneVector));
    }
    FString Serialize(Json D){FString Text;FJsonSerializer::Serialize(D.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));return Text;}
    bool Write(const FString& Path,Json D){IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);return FFileHelper::SaveStringToFile(Serialize(D),*Path);}
    FString MeshPath(const FString& Id){return TEXT("/Game/Campaign/Meshes/SM_")+Id+TEXT(".SM_")+Id;}
    FString MaterialPath(const FString& Id){return TEXT("/Game/Campaign/Materials/MI_")+Id+TEXT(".MI_")+Id;}
}
