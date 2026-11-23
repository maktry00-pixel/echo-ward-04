#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "WardEchoAssets.h"
using namespace WardData;

void AEWardBookWorld::BuildPrototypeFurnishings()
{
    const Json Data=WardBookMapData(),Catalog=Object(Data,TEXT("prototype_catalog"));
    const Json Detailed=Object(Object(Data,TEXT("visual_realism")),TEXT("mesh_replacements"));
    TMap<FString,UMaterialInstanceDynamic*> ScannedMaterials;
    auto ScannedMaterial=[&](const FString& Asset)
    {
        if(auto* Existing=ScannedMaterials.FindRef(Asset))return Existing;
        auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_CampaignPBR.M_CampaignPBR")),this);
        auto Texture=[&](const TCHAR* Kind){return LoadObject<UTexture>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Textures/T_%s_%s.T_%s_%s"),*Asset,Kind,*Asset,Kind));};
        M->SetTextureParameterValue(TEXT("Albedo"),Texture(TEXT("color")));M->SetTextureParameterValue(TEXT("Normal"),Texture(TEXT("normal")));M->SetTextureParameterValue(TEXT("RoughnessMap"),Texture(TEXT("arm")));
        M->SetVectorParameterValue(TEXT("RoughnessChannel"),FLinearColor(0,1,0,0));M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::White);
        M->SetScalarParameterValue(TEXT("Roughness"),.9f);M->SetScalarParameterValue(TEXT("NormalStrength"),.65f);M->SetScalarParameterValue(TEXT("Metallic"),0);
        Palette.Add(M);ScannedMaterials.Add(Asset,M);return M;
    };
    auto DetailedProp=[&](Json Model,Json Spec,FVector At,int Turn)
    {
        const FString Name=String(Spec,TEXT("mesh")),Asset=String(Spec,TEXT("asset"));
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Meshes/%s.%s"),*Name,*Name));
        UStaticMesh* Drawer=Name==TEXT("SM_Nightstand")?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Prototype/Meshes/SM_Drawer.SM_Drawer")):nullptr;
        if(!Mesh)return false;
        FBox Source=Mesh->GetBoundingBox();if(Drawer)Source+=Drawer->GetBoundingBox();
        const FQuat Facing=FRotator(0,Number(Spec,TEXT("yaw")),0).Quaternion(),Room=FRotator(0,Turn*90,0).Quaternion();
        const FBox Rotated=Source.TransformBy(FTransform(Facing));const auto& B=Array(Model,TEXT("bounds"));
        const FBox Target(FVector(B[0]->AsNumber(),B[1]->AsNumber(),B[2]->AsNumber())*100,FVector(B[3]->AsNumber(),B[4]->AsNumber(),B[5]->AsNumber())*100);
        FVector S=Target.GetSize()/Rotated.GetSize();if(FMath::Abs(Number(Spec,TEXT("yaw")))==90)Swap(S.X,S.Y);
        const FVector Shift=Target.GetCenter()-Facing.RotateVector(Source.GetCenter()*S);
        const FTransform T(Room*Facing,At+Room.RotateVector(Shift),S);
        const FBox Actual=Source.TransformBy(T),Expected=Target.TransformBy(FTransform(Room,At));
        if(!Expected.ExpandBy(.2f).IsInsideOrOn(Actual.Min)||!Expected.ExpandBy(.2f).IsInsideOrOn(Actual.Max)){++DetailedFurnitureBoundsFailures;return false;}
        for(UStaticMesh* Part:{Mesh,Drawer})
        {
            if(!Part)continue;
            const FString Key=FString::Printf(TEXT("scanned_%s_%d_%d_%d"),*Part->GetName(),FMath::FloorToInt(At.X/2400),FMath::FloorToInt(At.Y/2400),FMath::FloorToInt(At.Z/480));
            auto* G=Groups.FindRef(Key);
            if(!G)
            {
                G=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);G->SetupAttachment(RootComponent);G->SetMobility(EComponentMobility::Static);G->SetStaticMesh(Part);
                for(int I=0;I<Part->GetStaticMaterials().Num();++I)G->SetMaterial(I,ScannedMaterial(Asset));
                G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetGenerateOverlapEvents(false);G->bAutoRebuildTreeOnInstanceChanges=false;
                G->SetAffectDynamicIndirectLighting(false);G->SetAffectDistanceFieldLighting(false);G->SetLightingChannels(true,true,false);G->SetCullDistance(6500);
                G->RegisterComponent();AddInstanceComponent(G);Groups.Add(Key,G);
            }
            G->AddInstance(T,false);++MeshInstances;
        }
        ++DetailedFurniture;return true;
    };
    TMap<FString,int> Materials;
    for(const auto& Entry:Object(Data,TEXT("prototype_materials"))->Values)
    {
        const auto& RGB=Entry.Value->AsArray();if(RGB.Num()!=3)continue;
        const FLinearColor Tint(RGB[0]->AsNumber(),RGB[1]->AsNumber(),RGB[2]->AsNumber());
        auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalSurface.M_HospitalSurface")),this);
        M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetScalarParameterValue(TEXT("Roughness"),Entry.Key==TEXT("steel")?.45f:.85f);
        M->SetScalarParameterValue(TEXT("Wetness"),0);M->SetVectorParameterValue(TEXT("Emission"),Entry.Key==TEXT("light")?Tint*2:Tint*.025f);
        if(Entry.Key==TEXT("steel")){M->SetScalarParameterValue(TEXT("Metallic"),.55f);M->SetScalarParameterValue(TEXT("Weathering"),.12f);}
        Materials.Add(FString(Entry.Key),Palette.Num());Palette.Add(M);
    }
    TArray<TSharedPtr<FJsonValue>> AllProps=Array(Data,TEXT("prototype_props"));AllProps.Append(Array(Data,TEXT("environment_props")));
    for(auto Value:AllProps)
    {
        Json Prop=Value->AsObject(),Model=Object(Catalog,FString::FromInt(Number(Prop,TEXT("model"))));
        const FVector At=Vector(Array(Prop,TEXT("at")))*100;const int Turn=Number(Prop,TEXT("turn"));
        if(Cars31Replacements.Contains(String(Prop,TEXT("id"))))
        {if(String(Prop,TEXT("mount"))==TEXT("floor")&&!String(Object(Detailed,FString::FromInt(Number(Prop,TEXT("model")))),TEXT("mesh")).IsEmpty())++DetailedFurniture;PrototypeParts+=Array(Model,TEXT("parts")).Num();if(String(Prop,TEXT("id")).StartsWith(TEXT("ENV9_")))++EnvironmentProps;else ++PrototypeProps;continue;}
        const FString Mount=String(Prop,TEXT("mount"));
        const Json Replacement=Object(Detailed,FString::FromInt(Number(Prop,TEXT("model"))));
        const int CustomId=Number(Object(WardEchoAssets::Bindings(),TEXT("prototype_models")),FString::FromInt(Number(Prop,TEXT("model"))));
        const bool Custom=CustomId>0&&AddCustomProp(CustomId,Model,At,Turn);
        if(!Custom)++UnresolvedCustomProps;
        const bool Replaced=Custom||(Mount==TEXT("floor")&&!String(Replacement,TEXT("mesh")).IsEmpty()&&DetailedProp(Model,Replacement,At,Turn));
        if(Custom&&Mount==TEXT("floor")&&!String(Replacement,TEXT("mesh")).IsEmpty())++DetailedFurniture;
        auto Rotate=[&](FVector V){switch(Turn){case 1:return FVector(-V.Y,V.X,V.Z);case 2:return FVector(-V.X,-V.Y,V.Z);case 3:return FVector(V.Y,-V.X,V.Z);default:return V;}};
        for(auto Part:Array(Model,TEXT("parts")))
        {
            const auto& A=Part->AsArray();if(A.Num()!=8)continue;
            FVector Centre(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()),Size(A[3]->AsNumber(),A[4]->AsNumber(),A[5]->AsNumber());
            bool Bedding=false;
            if(Custom&&CustomId==175&&(A[6]->AsString()==TEXT("white")||A[6]->AsString()==TEXT("paper")))
            {
                Json Fit=MakeShared<FJsonObject>();
                Fit->SetArrayField(TEXT("bounds"),{MakeShared<FJsonValueNumber>(-Size.X*.5),MakeShared<FJsonValueNumber>(-Size.Y*.5),MakeShared<FJsonValueNumber>(-Size.Z*.5),MakeShared<FJsonValueNumber>(Size.X*.5),MakeShared<FJsonValueNumber>(Size.Y*.5),MakeShared<FJsonValueNumber>(Size.Z*.5)});
                Bedding=AddCustomProp(A[6]->AsString()==TEXT("white")?176:177,Fit,At+Rotate(Centre)*100,Turn);
            }
            if(Turn%2)Swap(Size.X,Size.Y);
            // Small attached fixtures, sheets and grass are visual dressing.
            // Floor furniture retains real collision through its solid parts.
            const bool Solid=Mount==TEXT("floor")&&A[7]->AsBool();
            const bool Keep=!Replaced||(!Custom&&Array(Replacement,TEXT("keep_materials")).ContainsByPredicate([&](const TSharedPtr<FJsonValue>& V){return V->AsString()==A[6]->AsString();}))||(CustomId==175&&!Bedding&&(A[6]->AsString()==TEXT("white")||A[6]->AsString()==TEXT("paper")));
            // Retain the exact tested collision, including bed hide spaces.
            // Scanned visuals are fitted inside the same measured envelope.
            if(Keep||Solid)Box(At+Rotate(Centre)*100,Size*100,Materials.FindRef(A[6]->AsString()),Solid,Keep);++PrototypeParts;
        }
        if(Boolean(Prop,TEXT("support_ledge")))
        {
            if(Mount==TEXT("surface"))Box(At+FVector(0,0,-3),FVector(65,40,4),Materials.FindRef(TEXT("steel")),false);
            else Box(At+FVector(0,-7,0),FVector(4,4,280),Materials.FindRef(TEXT("steel")),false);
        }
        if(Number(Prop,TEXT("model"))==177)LightAnchors.Add(At-FVector(0,0,20));
        if(String(Prop,TEXT("id")).StartsWith(TEXT("ENV9_")))++EnvironmentProps;else ++PrototypeProps;
    }
    for(auto V:Array(Data,TEXT("garden_paths")))
    {
        Json Path=V->AsObject();const auto& A=Array(Path,TEXT("rect"));if(A.Num()!=4)continue;
        const float X=A[0]->AsNumber()*100,Y=A[1]->AsNumber()*100,W=A[2]->AsNumber()*100,H=A[3]->AsNumber()*100;
        Box(FVector(X+W/2,Y+H/2,Number(Path,TEXT("z"))*100+5),FVector(W,H,3),0,false);
    }
}
