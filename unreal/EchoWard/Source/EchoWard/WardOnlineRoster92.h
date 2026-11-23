#pragma once

#include "WardBekaVisual50.h"
#include "WardBekaAnimation.h"
#include "WardGraphics.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "GroomComponent.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/MemoryReader.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SCompoundWidget.h"

namespace WardOnlineRoster92
{
struct FCharacter
{
    const TCHAR* Name;
    const TCHAR* Directory;
    float Height;
};

inline constexpr FCharacter Characters[] = {
    {TEXT("Бека"), TEXT(""), 175}, {TEXT("Таша"), TEXT("tasha"), 169}, {TEXT("Фрея"), TEXT("freya"), 169},
    {TEXT("Воительница"), TEXT("warrior"), 179}, {TEXT("Хинако"), TEXT("hinako"), 165},
    {TEXT("Шерри"), TEXT("sherry"), 162}, {TEXT("Мануэль"), TEXT("manuel"), 177},
    {TEXT("Алан"), TEXT("busy"), 180}, {TEXT("Мэтт"), TEXT("matt"), 182}, {TEXT("Дидо"), TEXT("dido"), 174}
};
inline constexpr int32 Count = UE_ARRAY_COUNT(Characters);

inline FString AssetFolder(int32 Index)
{
    const FString Loose = FPaths::ProjectDir() / TEXT("OnlineRoster94");
    const FString Root = FPaths::DirectoryExists(Loose) ? Loose :
        FPaths::ProjectContentDir() / TEXT("CampaignData/OnlineRoster94");
    return Root / Characters[Index].Directory;
}

inline FString DetailFolder(int32 Index)
{
    const FString Loose = FPaths::ProjectDir() / TEXT("OnlineDetail95");
    const FString Root = FPaths::DirectoryExists(Loose) ? Loose :
        FPaths::ProjectContentDir() / TEXT("CampaignData/OnlineDetail95");
    return Root / Characters[Index].Directory;
}

inline FString PolishFolder(const FString& Character)
{
    const FString Loose = FPaths::ProjectDir() / TEXT("OnlinePolish96");
    const FString Root = FPaths::DirectoryExists(Loose) ? Loose :
        FPaths::ProjectContentDir() / TEXT("CampaignData/OnlinePolish96");
    return Root / Character;
}

inline UMaterialInterface* Surface()
{
    return LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Campaign/EchoCustom/Materials/M_Real23_OPAQUE_2S.M_Real23_OPAQUE_2S"));
}

struct FIdleSection
{
    TArray<FVector3f> Rest, RestNormals;
    TArray<FVector> Positions, Normals;
    TArray<FVector2f> Weights;
    TArray<FProcMeshTangent> RestTangents, Tangents;
};

struct FIdleMesh
{
    TArray<FIdleSection> Sections;
    float Height = 0;

    void Prepare(UProceduralMeshComponent& Mesh)
    {
        for (int32 Index = 0; Index < Mesh.GetNumSections(); ++Index)
            if (const auto* Part = Mesh.GetProcMeshSection(Index))
                for (const auto& Vertex : Part->ProcVertexBuffer)
                    Height = FMath::Max(Height, float(Vertex.Position.Z));
        for (int32 Index = 0; Index < Mesh.GetNumSections(); ++Index)
        {
            auto& Section = Sections.AddDefaulted_GetRef();
            const auto* Part = Mesh.GetProcMeshSection(Index);
            if (!Part || Height < 1.f) continue;
            Section.Rest.Reserve(Part->ProcVertexBuffer.Num());
            Section.RestNormals.Reserve(Part->ProcVertexBuffer.Num());
            Section.Weights.Reserve(Part->ProcVertexBuffer.Num());
            Section.Positions.SetNumUninitialized(Part->ProcVertexBuffer.Num());
            Section.Normals.SetNumUninitialized(Part->ProcVertexBuffer.Num());
            Section.RestTangents.Reserve(Part->ProcVertexBuffer.Num());
            Section.Tangents.SetNumUninitialized(Part->ProcVertexBuffer.Num());
            for (const auto& Vertex : Part->ProcVertexBuffer)
            {
                const float Z = float(Vertex.Position.Z) / Height;
                Section.Rest.Emplace(Vertex.Position);
                Section.RestNormals.Emplace(Vertex.Normal);
                Section.RestTangents.Add(Vertex.Tangent);
                Section.Weights.Emplace(FMath::SmoothStep(.18f, .73f, Z), FMath::SmoothStep(.84f, .91f, Z));
            }
        }
    }

    void Animate(UProceduralMeshComponent& Mesh, float Clock, int32 Character)
    {
        const float Phase = Clock + Character * 1.71f;
        const float Breath = FMath::Sin(Phase * 1.71f);
        const float Transfer = FMath::Sin(Phase * .76f) + .35f * FMath::Sin(Phase * 1.29f);
        // The feet hold their positions while the torso and head move independently.
        const FVector3f Chest(-1.55f * Transfer, .28f * Breath, .43f * Breath);
        const FQuat4f ChestTurn(FVector3f::UpVector, .033f * Transfer);
        const FQuat4f HeadTurn = FQuat4f(FVector3f::UpVector, .135f * FMath::Sin(Phase * .92f)) *
            FQuat4f(FVector3f::RightVector, .035f * FMath::Sin(Phase * 1.08f));
        const FVector3f Hinge(0, 0, Height * .865f);
        for (int32 Index = 0; Index < Sections.Num(); ++Index)
        {
            auto& Section = Sections[Index];
            for (int32 Vertex = 0; Vertex < Section.Rest.Num(); ++Vertex)
            {
                const auto P = Section.Rest[Vertex];
                const auto N = Section.RestNormals[Vertex];
                const auto Weight = Section.Weights[Vertex];
                const FVector3f Lift = (ChestTurn.RotateVector(P) - P + Chest) * Weight.X;
                const FVector3f Head = P - Hinge;
                Section.Positions[Vertex] = FVector(P + Lift + (HeadTurn.RotateVector(Head) - Head) * Weight.Y);
                Section.Normals[Vertex] = FVector(N + (ChestTurn.RotateVector(N) - N) * Weight.X +
                    (HeadTurn.RotateVector(N) - N) * Weight.Y);
                const auto& Tangent = Section.RestTangents[Vertex];
                const FVector3f T(Tangent.TangentX);
                Section.Tangents[Vertex] = FProcMeshTangent(FVector(T + (ChestTurn.RotateVector(T) - T) * Weight.X +
                    (HeadTurn.RotateVector(T) - T) * Weight.Y), Tangent.bFlipTangentY);
            }
            Mesh.UpdateMeshSection(Index, Section.Positions, Section.Normals, TArray<FVector2D>(),
                TArray<FColor>(), Section.Tangents);
        }
    }
};

inline UTexture2D* LoadSurfaceMap(const FString& Path, bool Normal)
{
    TArray<uint8> Data;
    if (!FFileHelper::LoadFileToArray(Data, *Path) || Data.Num() < 12 ||
        FMemory::Memcmp(Data.GetData(), "ET94", 4)) return nullptr;
    uint32 Width, Height;
    FMemory::Memcpy(&Width, Data.GetData() + 4, sizeof Width);
    FMemory::Memcpy(&Height, Data.GetData() + 8, sizeof Height);
    if (Width < 4 || Height < 4 || Width > 1024 || Height > 1024 || Width % 4 || Height % 4 ||
        Data.Num() - 12 != int32((Width / 4) * (Height / 4) * (Normal ? 16 : 8))) return nullptr;
    const auto Bytes = TConstArrayView64<uint8>(Data.GetData() + 12, Data.Num() - 12);
    auto* Texture = UTexture2D::CreateTransient(Width, Height, Normal ? PF_BC5 : PF_BC4, NAME_None, Bytes);
    if (!Texture) return nullptr;
    Texture->SRGB = false;
    Texture->CompressionSettings = Normal ? TC_Normalmap : TC_Masks;
    Texture->Filter = TF_Bilinear;
    Texture->UpdateResource();
    return Texture;
}

inline UTexture2D* LoadMippedImage(const FString& Path, bool Color)
{
    FImage Source;
    if (!FImageUtils::LoadImage(*Path, Source) || Source.SizeX < 1 || Source.SizeY < 1 ||
        Source.SizeX > 4096 || Source.SizeY > 4096 || Source.NumSlices != 1) return nullptr;
    const bool Mask = !Color;
    const auto Format = Color ? ERawImageFormat::BGRA8 : ERawImageFormat::G8;
    const auto Gamma = Color ? EGammaSpace::sRGB : EGammaSpace::Linear;
    // Coverage images already store linear data, even when the PNG decoder reports sRGB.
    if (Mask) Source.GammaSpace = EGammaSpace::Linear;
    FImage Pixels;
    Source.CopyTo(Pixels, Format, Gamma);
    auto* Texture = UTexture2D::CreateTransient(Pixels.SizeX, Pixels.SizeY, Color ? PF_B8G8R8A8 : PF_G8);
    if (!Texture) return nullptr;
    Texture->SRGB = Color;
    Texture->NeverStream = true;
    Texture->LODGroup = Color ? TEXTUREGROUP_Character : TEXTUREGROUP_CharacterSpecular;
    Texture->Filter = Color ? TF_Default : TF_Trilinear;
    auto& Mips = Texture->GetPlatformData()->Mips;
    for (int32 Level = 0;; ++Level)
    {
        if (Level > 0) Mips.Add(new FTexture2DMipMap(Pixels.SizeX, Pixels.SizeY, 1));
        auto& Mip = Mips[Level];
        Mip.BulkData.Lock(LOCK_READ_WRITE);
        FMemory::Memcpy(Mip.BulkData.Realloc(Pixels.RawData.Num()), Pixels.RawData.GetData(), Pixels.RawData.Num());
        Mip.BulkData.Unlock();
        if (Pixels.SizeX == 1 && Pixels.SizeY == 1) break;
        FImage Smaller;
        FImageCore::ResizeImageAllocDest(Pixels, Smaller, FMath::Max(1, Pixels.SizeX / 2),
            FMath::Max(1, Pixels.SizeY / 2), Format, Gamma, FImageCore::EResizeImageFilter::Box);
        Pixels = MoveTemp(Smaller);
    }
    Texture->UpdateResource();
    return Texture;
}

inline TArray<FProcMeshTangent> SurfaceTangents(const TArray<FVector>& P, const TArray<FVector>& N,
    const TArray<FVector2D>& UV, const TArray<int32>& Corners)
{
    TArray<FVector> U, V;
    U.AddZeroed(P.Num());
    V.AddZeroed(P.Num());
    for (int32 I = 0; I < Corners.Num(); I += 3)
    {
        const int32 A = Corners[I], B = Corners[I + 1], C = Corners[I + 2];
        const FVector EdgeB = P[B] - P[A], EdgeC = P[C] - P[A];
        const FVector2D TextureB = UV[B] - UV[A], TextureC = UV[C] - UV[A];
        const double Denominator = TextureB.X * TextureC.Y - TextureB.Y * TextureC.X;
        if (FMath::Abs(Denominator) < UE_SMALL_NUMBER) continue;
        const FVector AlongU = ((EdgeB * TextureC.Y - EdgeC * TextureB.Y) / Denominator).GetSafeNormal();
        const FVector AlongV = ((EdgeC * TextureB.X - EdgeB * TextureC.X) / Denominator).GetSafeNormal();
        for (const int32 Vertex : {A, B, C})
        {
            U[Vertex] += AlongU;
            V[Vertex] += AlongV;
        }
    }
    TArray<FProcMeshTangent> Tangents;
    Tangents.Reserve(P.Num());
    for (int32 I = 0; I < P.Num(); ++I)
    {
        FVector Direction = FVector::VectorPlaneProject(U[I], N[I]).GetSafeNormal();
        if (Direction.IsNearlyZero())
            Direction = FVector::CrossProduct(FMath::Abs(N[I].Z) > .9 ? FVector::RightVector : FVector::UpVector,
                N[I]).GetSafeNormal();
        const bool Flip = FVector::DotProduct(FVector::CrossProduct(N[I], Direction), V[I]) < 0;
        Tangents.Emplace(Direction, Flip);
    }
    return Tangents;
}

inline bool ReadMesh(UProceduralMeshComponent& Mesh, const FString& Folder)
{
    TArray<uint8> Packed, Raw;
    if (!FFileHelper::LoadFileToArray(Packed, *(Folder / TEXT("model.ewmesh"))) || Packed.Num() < 12)
        return false;
    uint32 Size = 0;
    FMemory::Memcpy(&Size, Packed.GetData(), sizeof Size);
    if (Size < 8 || Size > 32 * 1024 * 1024) return false;
    Raw.SetNumUninitialized(Size);
    if (!FCompression::UncompressMemory(NAME_Zlib, Raw.GetData(), Raw.Num(), Packed.GetData() + 4,
        Packed.Num() - 4)) return false;
    FMemoryReader Reader(Raw);
    ANSICHAR Magic[4];
    Reader.Serialize(Magic, sizeof Magic);
    int32 Sections = 0;
    Reader << Sections;
    const bool SurfaceMaps = FMemory::Memcmp(Magic, "EW94", 4) == 0;
    if ((!SurfaceMaps && FMemory::Memcmp(Magic, "EW92", 4)) || Sections < 1 || Sections > 96) return false;
    auto* Parent = Surface();
    if (!Parent) return false;
    TMap<int32, UTexture2D*> Textures;
    int32 LoadedNormals = 0, LoadedRoughness = 0;
    int32 ColorWithMips = 0, MasksWithMips = 0, SourceOverrides = 0;
    const FString Character = FPaths::GetCleanFilename(Folder);
    const FString Polish = PolishFolder(Character);
    for (int32 Section = 0; Section < Sections; ++Section)
    {
        int32 Vertices = 0, Indices = 0, Texture = -1;
        float R = 0, G = 0, B = 0;
        float Roughness = .78f, Metallic = 0;
        uint32 Maps = 0;
        Reader << Vertices << Indices << Texture << R << G << B;
        if (SurfaceMaps) Reader << Roughness << Metallic << Maps;
        if (Vertices <= 0 || Vertices > 250000 || Indices <= 0 || Indices > 750000 || Indices % 3 ||
            Texture < -1 || Texture >= Sections || Reader.Tell() + int64(Vertices) * 32 + int64(Indices) * 4 > Raw.Num())
            return false;
        TArray<FVector> Positions, Normals;
        TArray<FVector2D> UV;
        TArray<int32> Triangles;
        Positions.Reserve(Vertices);
        Normals.Reserve(Vertices);
        UV.Reserve(Vertices);
        Triangles.SetNumUninitialized(Indices);
        for (int32 Vertex = 0; Vertex < Vertices; ++Vertex)
        {
            float X, Y, Z, NX, NY, NZ, U, V;
            Reader << X << Y << Z << NX << NY << NZ << U << V;
            Positions.Emplace(X, Y, Z);
            Normals.Emplace(NX, NY, NZ);
            UV.Emplace(U, V);
        }
        Reader.Serialize(Triangles.GetData(), int64(Indices) * sizeof(int32));
        if (Triangles.ContainsByPredicate([Vertices](int32 Slot) { return Slot < 0 || Slot >= Vertices; }))
            return false;
        for (int32 Corner = 0; Corner < Indices; Corner += 3)
            Swap(Triangles[Corner + 1], Triangles[Corner + 2]);
        auto* Normal = Maps & 1 ? LoadSurfaceMap(Folder / (FString::FromInt(Section) + TEXT(".normal.ewtex")), true) : nullptr;
        auto* Gloss = Maps & 2 ? LoadSurfaceMap(Folder / (FString::FromInt(Section) + TEXT(".rough.ewtex")), false) : nullptr;
        const auto Tangents = Normal ? SurfaceTangents(Positions, Normals, UV, Triangles) : TArray<FProcMeshTangent>();
        Mesh.CreateMeshSection(Section, Positions, Triangles, Normals, UV, TArray<FColor>(), Tangents, false);
        UTexture2D* Alpha = nullptr;
        const FString SlotName = FString::FromInt(Section);
        const FString SourceAlpha = Polish / (SlotName + TEXT(".alpha.png"));
        if (FPaths::FileExists(SourceAlpha))
        {
            Alpha = LoadMippedImage(SourceAlpha, false);
            if (Alpha) ++SourceOverrides;
        }
        if (!Alpha && Texture >= 0)
        {
            const FString Mask = Folder / (FString::FromInt(Texture) + TEXT(".cutout.png"));
            if (FPaths::FileExists(Mask)) Alpha = LoadMippedImage(Mask, false);
        }
        auto* Masked = Alpha ? LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Campaign/EchoCustom/Materials/M_Real23_MASK_2S.M_Real23_MASK_2S")) : nullptr;
        auto* Material = UMaterialInstanceDynamic::Create(Masked ? Masked : Parent, &Mesh);
        const FString SourceColor = Polish / (SlotName + TEXT(".color.png"));
        UTexture2D* OriginalColor = FPaths::FileExists(SourceColor) ? LoadMippedImage(SourceColor, true) : nullptr;
        Material->SetVectorParameterValue(TEXT("Tint"), OriginalColor ? FLinearColor::White : FLinearColor(R, G, B));
        if (OriginalColor)
        {
            Material->SetTextureParameterValue(TEXT("Albedo"), OriginalColor);
            ++SourceOverrides;
        }
        Material->SetVectorParameterValue(TEXT("Emission"), FLinearColor::Black);
        Material->SetScalarParameterValue(TEXT("Roughness"), Gloss ? 1.f : Roughness);
        Material->SetScalarParameterValue(TEXT("Metallic"), Metallic);
        if (Normal)
        {
            Material->SetTextureParameterValue(TEXT("Normal"), Normal);
            ++LoadedNormals;
        }
        if (Gloss)
        {
            Material->SetTextureParameterValue(TEXT("RoughnessMap"), Gloss);
            ++LoadedRoughness;
        }
        if (Masked)
        {
            Material->SetTextureParameterValue(TEXT("Alpha"), Alpha);
            if (Alpha->GetNumMips() > 1) ++MasksWithMips;
        }
        if (!OriginalColor && Texture >= 0)
        {
            if (!Textures.Contains(Texture))
                Textures.Add(Texture, LoadMippedImage(Folder / (FString::FromInt(Texture) + TEXT(".png")), true));
            if (auto* Image = Textures[Texture])
            {
                Material->SetTextureParameterValue(TEXT("Albedo"), Image);
                if (Image->GetNumMips() > 1) ++ColorWithMips;
            }
        }
        Mesh.SetMaterial(Section, Material);
    }
    if (SurfaceMaps && FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80")))
    {
        UE_LOG(LogTemp, Display, TEXT("ONLINE94_SURFACE %s normals=%d roughness=%d"),
            *Character, LoadedNormals, LoadedRoughness);
        UE_LOG(LogTemp, Display, TEXT("ONLINE96_QUALITY %s colors=%d masks=%d restored=%d"),
            *Character, ColorWithMips, MasksWithMips, SourceOverrides);
    }
    return !Reader.IsError() && Reader.Tell() == Raw.Num();
}

inline bool ReadDetailedMesh(UProceduralMeshComponent& Mesh, const UProceduralMeshComponent& Base,
    const FString& Folder, int32 Character)
{
    TArray<uint8> Packed, Raw;
    if (!FFileHelper::LoadFileToArray(Packed, *(Folder / TEXT("detail.ewmesh"))) || Packed.Num() < 12) return false;
    uint32 Size = 0;
    FMemory::Memcpy(&Size, Packed.GetData(), sizeof Size);
    if (Size < 8 || Size > 52 * 1024 * 1024) return false;
    Raw.SetNumUninitialized(Size);
    if (!FCompression::UncompressMemory(NAME_Zlib, Raw.GetData(), Raw.Num(), Packed.GetData() + 4,
        Packed.Num() - 4)) return false;
    FMemoryReader Reader(Raw);
    ANSICHAR Magic[4];
    Reader.Serialize(Magic, sizeof Magic);
    int32 Sections = 0;
    Reader << Sections;
    if (FMemory::Memcmp(Magic, "HD95", 4) || Sections <= 0 || Sections > Base.GetNumSections()) return false;
    int32 TrianglesLoaded = 0;
    TSet<int32> UsedSlots;
    for (int32 Section = 0; Section < Sections; ++Section)
    {
        int32 Slot = 0, Vertices = 0, Indices = 0;
        Reader << Slot << Vertices << Indices;
        if (Slot < 0 || Slot >= Base.GetNumSections() || UsedSlots.Contains(Slot) ||
            Vertices <= 0 || Vertices > 520000 || Indices <= 0 || Indices > 1800000 || Indices % 3 ||
            Reader.Tell() + int64(Vertices) * 32 + int64(Indices) * 4 > Raw.Num()) return false;
        UsedSlots.Add(Slot);
        TArray<FVector> Positions, Normals;
        TArray<FVector2D> UV;
        TArray<int32> Corners;
        Positions.Reserve(Vertices);
        Normals.Reserve(Vertices);
        UV.Reserve(Vertices);
        for (int32 Vertex = 0; Vertex < Vertices; ++Vertex)
        {
            float X, Y, Z, NX, NY, NZ, U, V;
            Reader << X << Y << Z << NX << NY << NZ << U << V;
            Positions.Emplace(X, Y, Z);
            Normals.Emplace(NX, NY, NZ);
            UV.Emplace(U, V);
        }
        Corners.SetNumUninitialized(Indices);
        Reader.Serialize(Corners.GetData(), int64(Indices) * sizeof(int32));
        if (Corners.ContainsByPredicate([Vertices](int32 Vertex) { return Vertex < 0 || Vertex >= Vertices; }))
            return false;
        for (int32 Corner = 0; Corner < Indices; Corner += 3)
            Swap(Corners[Corner + 1], Corners[Corner + 2]);
        const auto Tangents = SurfaceTangents(Positions, Normals, UV, Corners);
        Mesh.CreateMeshSection(Section, Positions, Corners, Normals, UV, TArray<FColor>(), Tangents, false);
        Mesh.SetMaterial(Section, Base.GetMaterial(Slot));
        TrianglesLoaded += Indices / 3;
    }
    const bool Valid = !Reader.IsError() && Reader.Tell() == Raw.Num();
    if (Valid && FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80")))
        UE_LOG(LogTemp, Display, TEXT("ONLINE95_DETAIL %s triangles=%d"), Characters[Character].Directory, TrianglesLoaded);
    return Valid;
}

class FStage
{
public:
    bool IsInspecting() const { return LastPreview != INDEX_NONE; }

    void DragOrbit(float Horizontal, float Vertical)
    {
        if (!IsInspecting()) return;
        OrbitYaw = FRotator::NormalizeAxis(OrbitYaw - Horizontal * .46f);
        OrbitPitch = FMath::Clamp(OrbitPitch + Vertical * .28f, -11.f, 24.f);
        CaptureTimer = 0;
    }

    void ZoomOrbit(float Steps)
    {
        if (!IsInspecting()) return;
        OrbitDistance = FMath::Clamp(OrbitDistance * FMath::Pow(.84f, Steps), 245.f, 910.f);
        CaptureTimer = 0;
    }

    void ResetOrbit()
    {
        OrbitYaw = -21.f;
        OrbitPitch = 4.f;
        OrbitDistance = 670.f;
        CaptureTimer = 0;
    }

    FVector2D OrbitView() const { return FVector2D(ActiveYaw, ActiveDistance); }

    ~FStage()
    {
        if (Beka.IsValid()) Beka->Destroy();
        if (Stage.IsValid()) Stage->Destroy();
    }

    bool Create(UWorld& World, USkeletalMeshComponent* HeroBody, USkeletalMeshComponent* HeroOutfit)
    {
        Stage = World.SpawnActor<AActor>();
        if (!Stage.IsValid()) return false;
        auto* Root = NewObject<USceneComponent>(Stage.Get());
        Stage->SetRootComponent(Root);
        Root->RegisterComponent();
        Stage->SetActorLocation(FVector(0, 0, 390000));
        auto* TargetObject = NewObject<UTextureRenderTarget2D>(Stage.Get());
        Target.Reset(TargetObject);
        Target->ClearColor = FLinearColor(.009, .004, .006);
        const FIntPoint PreviewSize = TargetSize(false);
        Target->InitCustomFormat(PreviewSize.X, PreviewSize.Y, PF_B8G8R8A8, false);
        Target->UpdateResourceImmediate(true);
        Capture = NewObject<USceneCaptureComponent2D>(Stage.Get());
        Capture->SetupAttachment(Root);
        Capture->TextureTarget = Target.Get();
        Capture->CaptureSource = SCS_FinalColorLDR;
        Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
        Capture->MaxViewDistanceOverride = 2800.f;
        Capture->bCaptureEveryFrame = false;
        Capture->bCaptureOnMovement = false;
        Capture->bAlwaysPersistRenderingState = true;
        Capture->FOVAngle = 39.f;
        Capture->ShowFlags.SetAtmosphere(false);
        Capture->ShowFlags.SetFog(false);
        Capture->ShowFlags.SetMotionBlur(false);
        // Use the target pixel dimensions even when the in-game view uses TSR.
        Capture->ShowFlags.SetScreenPercentage(false);
        Capture->ShowFlags.SetTemporalAA(true);
        Capture->ShowFlags.SetHair(true);
        Capture->ShowFlags.SetBloom(false);
        Capture->ShowFlags.SetDynamicShadows(false);
        Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
        Capture->PostProcessSettings.AutoExposureMethod = AEM_Manual;
        Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
        Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
        Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
        Capture->PostProcessSettings.AutoExposureBias = -.7f;
        Capture->PostProcessSettings.bOverride_VignetteIntensity = true;
        Capture->PostProcessSettings.VignetteIntensity = .21f;
        Capture->PostProcessSettings.bOverride_FilmGrainIntensity = true;
        Capture->PostProcessSettings.FilmGrainIntensity = 0.f;
        Capture->PostProcessSettings.bOverride_AmbientOcclusionIntensity = true;
        Capture->PostProcessSettings.bOverride_AmbientOcclusionRadius = true;
        Capture->PostProcessSettings.AmbientOcclusionRadius = 42.f;
        Capture->RegisterComponent();
        BuildRoom();
        BuildInspectionRoom();
        Brush.SetResourceObject(Target.Get());
        Brush.ImageSize = FVector2D(PreviewSize);
        SourceBody = HeroBody;
        SourceOutfit = HeroOutfit;
        return true;
    }

    void Arrange(const TArray<int32>& Selection, int32 Preview)
    {
        if (!Stage.IsValid() || !Capture.IsValid() || (Selection == LastSelection && Preview == LastPreview)) return;
        const bool PreviewChanged = Preview != LastPreview;
        const bool Inspecting = Preview != INDEX_NONE;
        if (PreviewChanged)
        {
            if (Detailed.IsValid()) Detailed->DestroyComponent();
            Detailed.Reset();
            DetailedIdle = FIdleMesh();
            DetailAttempted = false;
            ResetOrbit();
            ActiveYaw = OrbitYaw;
            ActivePitch = OrbitPitch;
            ActiveDistance = OrbitDistance;
        }
        LastSelection = Selection;
        LastPreview = Preview;
        const bool Performance = EWardGraphics::Current() == 2;
        Capture->ShowFlags.SetDynamicShadows(Inspecting && !Performance);
        Capture->PostProcessSettings.AmbientOcclusionIntensity = Inspecting && !Performance ? .56f : 0.f;
        const FIntPoint Size = TargetSize(Inspecting);
        if (Target->SizeX != Size.X || Target->SizeY != Size.Y)
            Target->ResizeTarget(Size.X, Size.Y);
        Capture->MaxViewDistanceOverride = Inspecting ? 3600.f : 2800.f;
        Capture->PostProcessSettings.AutoExposureBias = Preview > 0 ? -1.08f : Inspecting ? -.93f : -.88f;
        Capture->PostProcessSettings.VignetteIntensity = Inspecting ? .15f : .23f;
        if (PreviewChanged) Capture->bCameraCutThisFrame = true;
        for (const auto& Lamp : InspectionLights)
            if (Lamp.IsValid()) Lamp->SetVisibility(Inspecting);
        for (int32 Index = 1; Index < Count; ++Index)
            if (Meshes[Index].IsValid()) Meshes[Index]->SetVisibility(false);
        if (Beka.IsValid()) Beka->SetActorHiddenInGame(true);
        TArray<int32> Display = Selection;
        if (Preview > 0 && !Display.Contains(Preview) && Display.Num() < Count) Display.Add(Preview);
        if (Display.IsEmpty()) Display.Add(0);
        const bool Wide = Display.Num() > 5;
        ViewLocation = FVector(0, Wide ? 1250 : Display.Num() <= 3 ? 785 : 1080, Wide ? 268 : 185);
        ViewAim = FVector(0, Wide ? -76 : 0, Wide ? 123 : 96);
        if (Inspecting) UpdateInspectionView(ActiveYaw, ActivePitch, ActiveDistance);
        for (int32 Seat = 0; Seat < Display.Num(); ++Seat)
        {
            const int32 Index = Display[Seat];
            const int32 Columns = Display.Num() > 5 ? (Display.Num() + 1) / 2 : Display.Num();
            const int32 Row = Seat / Columns;
            const int32 InRow = Row ? Display.Num() - Columns : Columns;
            const float X = (Seat % Columns - (InRow - 1) * .5f) * 151.f + (Wide ? (Row ? 78.f : -54.f) : 0.f);
            const FVector At(X, Row ? -235.f : 8.f, Row ? 96.f : 0.f);
            const float Facing = Inspecting ? -10.f : FMath::Clamp(X * .025f, -9.f, 9.f);
            const FRotator Pose(0, Facing + (Index == 9 ? -65.f : 0.f), 0);
            const bool Visible = !Inspecting || Index == Preview;
            const FVector Position = Inspecting && Visible ? InspectionMark : At;
            if (Index == 0)
            {
                if (!Beka.IsValid())
                {
                    Beka = Stage->GetWorld()->SpawnActor<AEWardBekaVisual50>();
                    if (Beka.IsValid())
                    {
                        Beka->SetActorEnableCollision(false);
                        Beka->Tags.Add(TEXT("OnlineLobbyBeka"));
                        Beka->Body->SetAnimInstanceClass(UEWardBekaAnimation::StaticClass());
                        Beka->Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
                        Beka->PrimaryActorTick.bTickEvenWhenPaused = true;
                        Beka->Body->PrimaryComponentTick.bTickEvenWhenPaused = true;
                        Beka->Face->PrimaryComponentTick.bTickEvenWhenPaused = true;
                        DressBeka();
                        TArray<UPrimitiveComponent*> Parts;
                        Beka->GetComponents(Parts);
                        for (auto* Part : Parts)
                        {
                            Part->SetVisibleInSceneCaptureOnly(true);
                            Part->SetOwnerNoSee(false);
                            Part->SetCastShadow(false);
                        }
                        FitBekaHair();
                    }
                }
                if (Beka.IsValid())
                {
                    Beka->SetActorLocationAndRotation(Stage->GetActorLocation() + Position, Pose);
                    Beka->SetActorHiddenInGame(!Visible);
                    TArray<UPrimitiveComponent*> Parts;
                    Beka->GetComponents(Parts);
                    for (auto* Part : Parts) Part->SetCastShadow(Inspecting && Visible && Part->IsVisible());
                }
            }
            else if (Index > 0 && Index < Count && Visible)
            {
                if (!Meshes[Index].IsValid() && !Failed.Contains(Index))
                {
                    auto* Model = NewObject<UProceduralMeshComponent>(Stage.Get());
                    Model->SetupAttachment(Stage->GetRootComponent());
                    Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Model->SetVisibleInSceneCaptureOnly(true);
                    Model->SetCastShadow(false);
                    if (ReadMesh(*Model, AssetFolder(Index)))
                    {
                        Model->RegisterComponent();
                        Meshes[Index] = Model;
                        Idle[Index].Prepare(*Model);
                        if (FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80")))
                            UE_LOG(LogTemp, Display, TEXT("ONLINE92_STAGE loaded=%s"), Characters[Index].Directory);
                    }
                    else
                    {
                        Failed.Add(Index);
                        Model->DestroyComponent();
                        UE_LOG(LogTemp, Warning, TEXT("Online roster mesh unavailable: %s"), Characters[Index].Directory);
                    }
                }
                if (auto* Model = Meshes[Index].Get())
                {
                    if (Inspecting && !DetailAttempted) PrepareDetail(Index, *Model);
                    auto* VisibleModel = Inspecting && Detailed.IsValid() ? Detailed.Get() : Model;
                    VisibleModel->SetRelativeLocationAndRotation(Position, Pose);
                    VisibleModel->SetCastShadow(Inspecting);
                    VisibleModel->SetVisibility(true);
                }
            }
        }
        if (!HasView || PreviewChanged)
        {
            Capture->SetRelativeLocationAndRotation(ViewLocation, (ViewAim - ViewLocation).Rotation());
            HasView = true;
        }
        CaptureTimer = 0;
    }

    void Tick(float Delta)
    {
        if (!Capture.IsValid() || !HasView) return;
        Clock += FMath::Min(Delta, .1f);
        CaptureTimer -= Delta;
        if (CaptureTimer > 0) return;
        const bool Focus = LastPreview != INDEX_NONE;
        CaptureTimer = Focus ? 1.f / (EWardGraphics::Current() == 2 ? 24.f : 30.f) : 1.f / 20.f;
        const float Step = FMath::Max(CaptureTimer, FMath::Min(.12f, Delta));
        if (Focus)
        {
            ActiveYaw = FMath::FixedTurn(ActiveYaw, OrbitYaw, Step * 540.f);
            ActivePitch = FMath::FInterpTo(ActivePitch, OrbitPitch, Step, 17.f);
            ActiveDistance = FMath::FInterpTo(ActiveDistance, OrbitDistance, Step, 14.f);
            UpdateInspectionView(ActiveYaw, ActivePitch, ActiveDistance);
            if (Detailed.IsValid() && Frame % 2 == 0)
                DetailedIdle.Animate(*Detailed.Get(), Clock, LastPreview);
        }
        const FVector RoomDrift(35.f * FMath::Sin(Clock * .33f), 0, 6.f * FMath::Sin(Clock * .41f));
        const FVector Eye = Focus ? ViewLocation : FMath::VInterpTo(Capture->GetRelativeLocation(), ViewLocation + RoomDrift, Step, 10.f);
        const FRotator Aim = (ViewAim - Eye).Rotation();
        Capture->SetRelativeLocationAndRotation(Eye, Aim);
        for (int32 Index = 1; Index < Count; ++Index)
        {
            auto* Model = Meshes[Index].Get();
            if (Model && Model->IsVisible() && (Focus || LastSelection.Num() < 5 || (Index + Frame) % 2 == 0))
                Idle[Index].Animate(*Model, Clock, Index);
        }
        if (++Frame == 12 && FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80")))
            UE_LOG(LogTemp, Display, TEXT("ONLINE93_IDLE capture=%d clock=%.2f"), Frame, Clock);
        Capture->CaptureScene();
    }

    FSlateBrush Brush;

private:
    static FIntPoint TargetSize(bool Inspecting)
    {
        const FIntPoint Group[] = {{2048, 942}, {1728, 794}, {1536, 706}};
        const FIntPoint Solo[] = {{2304, 1060}, {2048, 942}, {1536, 706}};
        const int32 Preset = EWardGraphics::Current();
        return Inspecting ? Solo[Preset] : Group[Preset];
    }

    void UpdateInspectionView(float Yaw, float Pitch, float Distance)
    {
        const float ActorHeight = Characters[LastPreview].Height;
        const float Close = 1.f - FMath::Clamp((Distance - 245.f) / (670.f - 245.f), 0.f, 1.f);
        const float Portrait = FMath::SmoothStep(0.f, 1.f, Close);
        ViewAim = InspectionMark + FVector(0, 0, ActorHeight * FMath::Lerp(.51f, .86f, Portrait));
        const float YawRadians = FMath::DegreesToRadians(Yaw);
        const float PitchRadians = FMath::DegreesToRadians(Pitch);
        const float Horizontal = Distance * FMath::Cos(PitchRadians);
        ViewLocation = ViewAim + FVector(FMath::Sin(YawRadians) * Horizontal, FMath::Cos(YawRadians) * Horizontal,
            FMath::Sin(PitchRadians) * Distance);
        ViewLocation.Z = FMath::Max(ViewLocation.Z, InspectionMark.Z + 46.0);
    }

    void PrepareDetail(int32 Index, const UProceduralMeshComponent& Base)
    {
        DetailAttempted = true;
        auto* Model = NewObject<UProceduralMeshComponent>(Stage.Get());
        Model->SetupAttachment(Stage->GetRootComponent());
        Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Model->SetVisibleInSceneCaptureOnly(true);
        if (!ReadDetailedMesh(*Model, Base, DetailFolder(Index), Index))
        {
            Model->DestroyComponent();
            UE_LOG(LogTemp, Warning, TEXT("Online inspection geometry unavailable: %s"), Characters[Index].Directory);
            return;
        }
        Model->RegisterComponent();
        Detailed = Model;
        DetailedIdle.Prepare(*Model);
    }

    void FitBekaHair()
    {
        auto* Hair = NewObject<UProceduralMeshComponent>(Beka.Get());
        if (!ReadMesh(*Hair, AssetFolder(0) / TEXT("beka_hair")))
        {
            Hair->DestroyComponent();
            return;
        }
        Hair->SetupAttachment(Beka->Body, TEXT("head"));
        Hair->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Hair->SetCastShadow(false);
        Hair->SetVisibleInSceneCaptureOnly(true);
        Beka->AddInstanceComponent(Hair);
        Hair->RegisterComponent();
        Hair->SetWorldTransform(Beka->GetActorTransform());
        Beka->Hair->SetVisibility(false);
        if (FParse::Param(FCommandLine::Get(), TEXT("WardOnlineCheck80")))
            UE_LOG(LogTemp, Display, TEXT("ONLINE93_BEKA fitted hair sections=%d"), Hair->GetNumSections());
    }

    void DressBeka()
    {
        auto* Body = Beka->Body.Get();
        if (auto* HeroBody = SourceBody.Get())
            for (int32 Slot = 0; Slot < HeroBody->GetNumMaterials(); ++Slot)
                Body->SetMaterial(Slot, HeroBody->GetMaterial(Slot));
        auto* HeroOutfit = SourceOutfit.Get();
        auto* Asset = HeroOutfit ? HeroOutfit->GetSkeletalMeshAsset() : LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/Hero/PhotoTailoredV26/SK_BekaPhotoTailored.SK_BekaPhotoTailored"));
        if (!Asset) return;
        auto* Outfit = NewObject<USkeletalMeshComponent>(Beka.Get(), TEXT("LobbyBekaOutfit"));
        Outfit->SetupAttachment(Body);
        Outfit->SetSkeletalMesh(Asset);
        Outfit->SetLeaderPoseComponent(Body);
        Outfit->SetForcedLOD(1);
        Outfit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (HeroOutfit)
            for (int32 Slot = 0; Slot < HeroOutfit->GetNumMaterials(); ++Slot)
                Outfit->SetMaterial(Slot, HeroOutfit->GetMaterial(Slot));
        Beka->AddInstanceComponent(Outfit);
        Outfit->RegisterComponent();
    }

    void Block(const FVector& Location, const FVector& Extent, FLinearColor Tint, FLinearColor Emission = FLinearColor::Black,
        bool Round = false)
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, Round ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") :
            TEXT("/Engine/BasicShapes/Cube.Cube"));
        auto* Parent = Surface();
        if (!Mesh || !Parent) return;
        auto* Component = NewObject<UStaticMeshComponent>(Stage.Get());
        Component->SetupAttachment(Stage->GetRootComponent());
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetVisibleInSceneCaptureOnly(true);
        Component->SetRelativeLocation(Location);
        Component->SetRelativeScale3D(Extent / 100.f);
        Component->SetCastShadow(false);
        auto* Material = UMaterialInstanceDynamic::Create(Parent, Component);
        Material->SetVectorParameterValue(TEXT("Tint"), Tint);
        Material->SetVectorParameterValue(TEXT("Emission"), Emission);
        Material->SetScalarParameterValue(TEXT("Roughness"), .66f);
        Component->SetMaterial(0, Material);
        Component->RegisterComponent();
    }

    void Light(const FVector& Position, const FLinearColor& Color, float Lumens, float Radius, bool Inspection = false)
    {
        auto* Lamp = NewObject<UPointLightComponent>(Stage.Get());
        Lamp->SetupAttachment(Stage->GetRootComponent());
        Lamp->SetIntensityUnits(ELightUnits::Lumens);
        Lamp->SetIntensity(Lumens);
        Lamp->SetLightColor(Color);
        Lamp->SetAttenuationRadius(Radius);
        Lamp->SetCastShadows(false);
        Lamp->SetSourceRadius(Inspection ? 48.f : 35.f);
        Lamp->SetRelativeLocation(Position);
        if (Inspection)
        {
            Lamp->SetVisibility(false);
            InspectionLights.Add(Lamp);
        }
        Lamp->RegisterComponent();
    }

    void BuildRoom()
    {
        const FLinearColor Concrete(.06, .048, .043), Steel(.020, .025, .029), Oxide(.16, .012, .008);
        Block(FVector(0, 0, -9), FVector(1800, 1900, 18), Concrete);
        Block(FVector(0, -475, 190), FVector(1800, 40, 400), Steel);
        Block(FVector(0, -304, 48), FVector(1450, 320, 96), Concrete);
        Block(FVector(0, -130, 22), FVector(1450, 30, 44), Concrete);
        Block(FVector(0, -151, 37), FVector(1450, 16, 5), Oxide);
        Block(FVector(0, -449, 125), FVector(1700, 14, 11), Oxide);
        Block(FVector(0, -401, 333), FVector(1700, 13, 14), Steel);
        Block(FVector(0, -395, 351), FVector(1700, 7, 7), Oxide);
        for (int32 Bay = -4; Bay <= 4; ++Bay)
        {
            const float X = Bay * 185.f;
            Block(FVector(X, -447, 175), FVector(14, 24, 350), Concrete);
            if (Bay % 2 == 0)
            {
                Block(FVector(X + 90, -433, 286), FVector(81, 27, 19), Steel);
                Block(FVector(X + 90, -415, 283), FVector(67, 8, 6), Oxide, FLinearColor(1.6, .018, .009));
            }
        }
        for (int32 Panel : {-1, 1})
        {
            for (int32 Bar = 0; Bar < 6; ++Bar)
                Block(FVector(Panel * (513 + Bar * 28), -407, 220), FVector(4, 14, 185), Steel);
            Block(FVector(Panel * 585, -402, 302), FVector(160, 13, 7), Steel);
            Block(FVector(Panel * 585, -402, 160), FVector(160, 13, 7), Steel);
            Block(FVector(Panel * 620, -355, 54), FVector(75, 45, 108), Steel);
            Block(FVector(Panel * 620, -329, 86), FVector(55, 7, 34), Concrete);
        }
        Light(FVector(-440, 265, 300), FLinearColor(.83, .90, 1), 430, 1000);
        Light(FVector(420, 235, 260), FLinearColor(1, .91, .81), 270, 950);
        Light(FVector(0, -115, 335), FLinearColor(.62, .76, 1), 420, 850);
        Light(FVector(-470, -351, 245), FLinearColor(1, .07, .035), 350, 630);
        Light(FVector(460, -351, 242), FLinearColor(1, .07, .035), 350, 630);
    }

    void BuildInspectionRoom()
    {
        const FVector Center(InspectionMark.X, InspectionMark.Y, 0);
        const FLinearColor Floor(.027, .034, .039), Wall(.033, .043, .052), FrameTint(.016, .02, .024);
        const FLinearColor Red(.14, .014, .01);
        Block(Center - FVector(0, 0, 17), FVector(2300, 2300, 28), FrameTint);
        for (int32 X = -3; X <= 3; ++X)
            for (int32 Y = -3; Y <= 3; ++Y)
            {
                const float Tint = 1.f + float((X * X + Y * Y + X * Y) % 4) * .13f;
                Block(Center + FVector(X * 300, Y * 300, -3), FVector(298.3, 298.3, 5), Floor * Tint);
            }
        Block(Center + FVector(0, 0, 4), FVector(234, 234, 16), Wall, FLinearColor::Black, true);
        Block(Center + FVector(0, 0, .4), FVector(248, 248, 2), Red, FLinearColor(.18, .012, .007), true);
        for (const int32 Side : {-1, 1})
        {
            Block(Center + FVector(0, Side * 1080, 235), FVector(2200, 32, 470), Wall);
            Block(Center + FVector(Side * 1080, 0, 235), FVector(32, 2200, 470), Wall);
            Block(Center + FVector(0, Side * 1054, 86), FVector(2180, 16, 19), FrameTint);
            Block(Center + FVector(Side * 1054, 0, 86), FVector(16, 2180, 19), FrameTint);
            for (int32 Part = -2; Part <= 2; ++Part)
            {
                const float Offset = Part * 380.f;
                Block(Center + FVector(Offset, Side * 1059, 240), FVector(22, 33, 480), FrameTint);
                Block(Center + FVector(Side * 1059, Offset, 240), FVector(33, 22, 480), FrameTint);
                if (Part % 2 == 0)
                {
                    Block(Center + FVector(Offset + 97, Side * 1027, 318), FVector(122, 8, 6),
                        Red, FLinearColor(1.1, .035, .015));
                    Block(Center + FVector(Side * 1027, Offset + 97, 318), FVector(8, 122, 6),
                        Red, FLinearColor(1.1, .035, .015));
                }
            }
        }
        auto* Key = NewObject<USpotLightComponent>(Stage.Get());
        Key->SetupAttachment(Stage->GetRootComponent());
        const FVector KeyPosition = Center + FVector(-215, 282, 349);
        Key->SetRelativeLocationAndRotation(KeyPosition, (Center + FVector(0, 0, 78) - KeyPosition).Rotation());
        Key->SetIntensityUnits(ELightUnits::Lumens);
        Key->SetIntensity(165);
        Key->SetLightColor(FLinearColor(1, .96, .91));
        Key->SetAttenuationRadius(1150);
        Key->SetInnerConeAngle(24);
        Key->SetOuterConeAngle(42);
        Key->SetSourceRadius(38);
        Key->SetCastShadows(true);
        Key->SetVisibility(false);
        Key->RegisterComponent();
        InspectionLights.Add(Key);
        Light(Center + FVector(208, 235, 170), FLinearColor(.78, .86, 1), 122, 800, true);
        Light(Center + FVector(150, -255, 244), FLinearColor(1, .30, .19), 153, 830, true);
        Light(Center + FVector(-278, -210, 286), FLinearColor(.74, .84, 1), 165, 870, true);
        Light(Center + FVector(0, -919, 255), FLinearColor(.57, .71, 1), 135, 500, true);
        Light(Center + FVector(-925, 148, 180), FLinearColor(1, .14, .09), 145, 500, true);
        Light(Center + FVector(907, -190, 244), FLinearColor(.62, .78, 1), 126, 500, true);
        Light(Center + FVector(75, 931, 195), FLinearColor(.92, .31, .22), 116, 500, true);
    }

    TWeakObjectPtr<AActor> Stage;
    TWeakObjectPtr<AEWardBekaVisual50> Beka;
    TWeakObjectPtr<USkeletalMeshComponent> SourceBody, SourceOutfit;
    TWeakObjectPtr<UProceduralMeshComponent> Meshes[Count];
    TWeakObjectPtr<UProceduralMeshComponent> Detailed;
    FIdleMesh Idle[Count];
    FIdleMesh DetailedIdle;
    TWeakObjectPtr<USceneCaptureComponent2D> Capture;
    TArray<TWeakObjectPtr<ULightComponent>> InspectionLights;
    TStrongObjectPtr<UTextureRenderTarget2D> Target;
    TArray<int32> LastSelection;
    TSet<int32> Failed;
    int32 LastPreview = INDEX_NONE;
    FVector ViewLocation, ViewAim;
    const FVector InspectionMark = FVector(0, 5650, 12);
    int32 Frame = 0;
    float Clock = 0, CaptureTimer = 0;
    float OrbitYaw = -21.f, OrbitPitch = 4.f, OrbitDistance = 670.f;
    float ActiveYaw = -21.f, ActivePitch = 4.f, ActiveDistance = 670.f;
    bool DetailAttempted = false;
    bool HasView = false;
};

class SOrbitImage final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SOrbitImage) {}
        SLATE_ARGUMENT(FStage*, PreviewStage)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        PreviewStage = Args._PreviewStage;
        ChildSlot[SNew(SImage).Image(&PreviewStage->Brush).Visibility(EVisibility::HitTestInvisible)];
    }

    FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Pointer) override
    {
        if (!PreviewStage->IsInspecting() || Pointer.GetEffectingButton() != EKeys::LeftMouseButton)
            return FReply::Unhandled();
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Pointer) override
    {
        if (HasMouseCapture() && Pointer.GetEffectingButton() == EKeys::LeftMouseButton)
            return FReply::Handled().ReleaseMouseCapture();
        return FReply::Unhandled();
    }

    FReply OnMouseMove(const FGeometry&, const FPointerEvent& Pointer) override
    {
        if (!HasMouseCapture()) return FReply::Unhandled();
        const auto Delta = Pointer.GetCursorDelta();
        PreviewStage->DragOrbit(float(Delta.X), float(Delta.Y));
        return FReply::Handled();
    }

    FReply OnMouseWheel(const FGeometry&, const FPointerEvent& Pointer) override
    {
        if (!PreviewStage->IsInspecting()) return FReply::Unhandled();
        PreviewStage->ZoomOrbit(Pointer.GetWheelDelta());
        return FReply::Handled();
    }

    FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override
    {
        if (!PreviewStage->IsInspecting()) return FCursorReply::Unhandled();
        return FCursorReply::Cursor(HasMouseCapture() ? EMouseCursor::GrabHandClosed : EMouseCursor::GrabHand);
    }

private:
    FStage* PreviewStage = nullptr;
};
}
