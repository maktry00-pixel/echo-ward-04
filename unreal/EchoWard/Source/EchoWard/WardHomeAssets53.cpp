#include "WardHomeAssets53.h"
#include "WardRuntimeMeshUV75.h"
#include "WardAudio68.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"
#include "StaticMeshAttributes.h"

namespace
{
    FString AssetPath(const FString& Name)
    {
        return FPaths::ProjectContentDir() / TEXT("CampaignData/Intro53") / Name;
    }
}

UStaticMesh* WardHomeAssets::BuildMesh(AActor* Owner, const WardData::Json& Library, const FString& Name)
{
    using namespace WardData;
    const auto& Sections = Array(Library, Name);
    if (Sections.IsEmpty()) return nullptr;

    auto* Parent = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Campaign/EchoCustom/Materials/M_Real23_OPAQUE_2S.M_Real23_OPAQUE_2S"));
    if (!Parent) return nullptr;

    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    Attributes.GetVertexInstanceUVs().SetNumChannels(1);
    auto* Mesh = NewObject<UStaticMesh>(Owner);
    for (const auto& SectionValue : Sections)
    {
        const auto Section = SectionValue->AsObject();
        const auto& Vertices = Array(Section, TEXT("vertices"));
        const auto& Indices = Array(Section, TEXT("indices"));
        if (Vertices.IsEmpty() || Vertices.Num() % 8 || Indices.Num() % 3) return nullptr;

        auto* Material = UMaterialInstanceDynamic::Create(Parent, Mesh);
        const auto& Color = Array(Section, TEXT("color"));
        if (Color.Num() == 4)
            Material->SetVectorParameterValue(TEXT("Tint"), FLinearColor(Color[0]->AsNumber(), Color[1]->AsNumber(), Color[2]->AsNumber()));
        Material->SetScalarParameterValue(TEXT("Roughness"), Number(Section, TEXT("roughness"), .8));
        Material->SetScalarParameterValue(TEXT("Metallic"), 0.f);
        const FString Texture = String(Section, TEXT("texture"));
        if (Texture == TEXT("table.jpg") || Texture == TEXT("phone76.jpg"))
        {
            if (auto* Albedo = FImageUtils::ImportFileAsTexture2D(Texture==TEXT("phone76.jpg")?FPaths::ProjectContentDir()/TEXT("CampaignData/Gameplay76/phone76.jpg"):AssetPath(Texture)))
                Material->SetTextureParameterValue(TEXT("Albedo"), Albedo);
        }
        const FName Slot(*FString::Printf(TEXT("Home_%d"), Mesh->GetStaticMaterials().Num()));
        Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, Slot));
        const FPolygonGroupID Group = Description.CreatePolygonGroup();
        Attributes.GetPolygonGroupMaterialSlotNames()[Group] = Slot;

        TArray<FVertexInstanceID> Instances;
        Instances.Reserve(Vertices.Num() / 8);
        for (int32 I = 0; I < Vertices.Num(); I += 8)
        {
            const FVertexID Vertex = Description.CreateVertex();
            Attributes.GetVertexPositions()[Vertex] = FVector3f(Vertices[I]->AsNumber(), Vertices[I+1]->AsNumber(), Vertices[I+2]->AsNumber());
            const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
            const FVector3f Normal(Vertices[I+3]->AsNumber(), Vertices[I+4]->AsNumber(), Vertices[I+5]->AsNumber());
            const FVector3f Axis = FMath::Abs(Normal.Z) < .95f ? FVector3f::UpVector : FVector3f::ForwardVector;
            Attributes.GetVertexInstanceNormals()[Instance] = Normal;
            Attributes.GetVertexInstanceTangents()[Instance] = FVector3f::CrossProduct(Axis, Normal).GetSafeNormal();
            Attributes.GetVertexInstanceBinormalSigns()[Instance] = 1.f;
            Attributes.GetVertexInstanceUVs().Set(Instance, 0, FVector2f(Vertices[I+6]->AsNumber(), Vertices[I+7]->AsNumber()));
            Instances.Add(Instance);
        }
        for (int32 I = 0; I < Indices.Num(); I += 3)
        {
            const int32 A = Indices[I]->AsNumber(), B = Indices[I+1]->AsNumber(), C = Indices[I+2]->AsNumber();
            if (!Instances.IsValidIndex(A) || !Instances.IsValidIndex(B) || !Instances.IsValidIndex(C)) return nullptr;
            if (A != B && A != C && B != C)
            {
                const FVertexInstanceID Corners[] = {Instances[A], Instances[B], Instances[C]};
                Description.CreateTriangle(Group, MakeArrayView(Corners));
            }
        }
    }

    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild = true;
    Params.bBuildSimpleCollision = false;
    Params.bMarkPackageDirty = false;
    TArray<const FMeshDescription*> Descriptions = { &Description };
    WardInitializeRuntimeMeshUV75(Mesh,Description);
    return Mesh->BuildFromMeshDescriptions(Descriptions, Params) ? Mesh : nullptr;
}

namespace
{
USoundWaveProcedural* LoadWave(AActor* Owner, const FString& Path)
{
    TArray<uint8> File;
    FWaveModInfo WaveInfo;
    if (!FFileHelper::LoadFileToArray(File, *Path) ||
        !WaveInfo.ReadWaveInfo(File.GetData(), File.Num()) ||
        *WaveInfo.pBitsPerSample != 16 || *WaveInfo.pChannels != 1)
        return nullptr;

    auto* Wave = NewObject<USoundWaveProcedural>(Owner);
    Wave->SetSampleRate(*WaveInfo.pSamplesPerSec);
    Wave->NumChannels = 1;
    Wave->Duration = float(WaveInfo.SampleDataSize) / (2.f * *WaveInfo.pSamplesPerSec);
    Wave->QueueAudio(WaveInfo.SampleDataStart, WaveInfo.SampleDataSize);
    return Wave;
}
}

UAudioComponent* WardHomeAssets::PlayDialogueFile(AActor* Owner, const FString& Path, float Volume)
{
    if (Volume <= 0.f || !Owner->GetWorld()->GetAudioDevice()) return nullptr;
    auto* Wave = LoadWave(Owner, Path);
    if (!Wave) return nullptr;
    auto* Audio = NewObject<UAudioComponent>(Owner);
    Audio->bAutoDestroy = true;
    Audio->SetUISound(true);
    Audio->SetVolumeMultiplier(Volume);
    Audio->SetSound(Wave);
    Audio->RegisterComponent();
    Audio->Play();
    UE_LOG(LogTemp, Display, TEXT("PEOPLE55_SPEECH %s %.2fs"), *FPaths::GetCleanFilename(Path), Wave->Duration);
    return Audio;
}

UAudioComponent* WardHomeAssets::Play(AActor* Owner, const TCHAR* Clip, float Volume, const FVector* Position)
{
    if (Volume <= 0.f || !Owner->GetWorld()->GetAudioDevice()) return nullptr;
    const FString Replacement=WardAudio68::FileKey(AssetPath(FString(Clip)+TEXT(".wav")));
    if(!Replacement.IsEmpty())return WardAudio68::Play(Owner,Replacement,Volume,Position);
    auto* Wave = LoadWave(Owner, AssetPath(FString(Clip) + TEXT(".wav")));
    if (!Wave) return nullptr;
    UAudioComponent* Audio = Position
        ? UGameplayStatics::SpawnSoundAtLocation(Owner, Wave, *Position, FRotator::ZeroRotator, Volume)
        : UGameplayStatics::SpawnSound2D(Owner, Wave, Volume);
    if (Audio) UE_LOG(LogTemp, Display, TEXT("INTRO53_SOUND %s %.2fs"), Clip, Wave->Duration);
    return Audio;
}

UAudioComponent* WardHomeAssets::PlaySpatialFile61(AActor* Owner,const FString& Path,float Volume,const FVector& Position)
{
    if(Volume<=0||!Owner||!Owner->GetWorld()->GetAudioDevice())return nullptr;
    const FString Replacement=WardAudio68::FileKey(Path);
    if(!Replacement.IsEmpty())return WardAudio68::Play(Owner,Replacement,Volume,&Position);
    auto* Wave=LoadWave(Owner,Path);if(!Wave)return nullptr;
    auto* Audio=NewObject<UAudioComponent>(Owner);Audio->bAutoDestroy=true;Audio->bAllowSpatialization=true;
    Audio->bOverrideAttenuation=true;Audio->AttenuationOverrides.bAttenuate=true;Audio->AttenuationOverrides.bSpatialize=true;
    Audio->AttenuationOverrides.AttenuationShape=EAttenuationShape::Sphere;Audio->AttenuationOverrides.AttenuationShapeExtents=FVector(140);Audio->AttenuationOverrides.FalloffDistance=2400;
    Audio->SetVolumeMultiplier(Volume);Audio->SetSound(Wave);Audio->RegisterComponent();Audio->SetWorldLocation(Position);Audio->Play();
    return Audio;
}
