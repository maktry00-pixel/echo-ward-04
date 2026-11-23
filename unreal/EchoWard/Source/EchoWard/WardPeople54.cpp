#include "WardPeople54.h"
#include "WardDialogue62.h"
#include "WardPeople58.h"
#include "WardOnline80.h"
#include "WardHospitalGameplay.h"
#include "WardStaging64.h"
#include "WardCampaign.h"
#include "WardEchoAssets.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "TextureResource.h"

namespace
{
    const TArray<FColor> NoColors;
    const TArray<FProcMeshTangent> NoTangents;
    enum { Idle, Walk, Talk, Injured, Use, Habit };

    FString PeoplePath55()
    {
        return FPaths::ProjectContentDir() / TEXT("CampaignData/People55");
    }

    WardData::Json Profile55(const FString& Id)
    {
        if (Id.StartsWith(TEXT("FILE_")))
        {
            static const auto Archive = WardData::Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People56/archive.json"));
            return WardData::Object(Archive, Id);
        }
        static const auto Profiles = WardData::Read(PeoplePath55() / TEXT("profiles.json"));
        return WardData::Object(Profiles, Id);
    }

    UTexture2D* LoadPersonTexture(const FString& Path, bool bColor)
    {
        // The story reuses several source models. Keep one live atlas per file
        // while allowing Unreal to collect it once no material references it.
        static TMap<FString, TWeakObjectPtr<UTexture2D>> Textures;
        if (const auto* Cached = Textures.Find(Path))
            if (auto* Texture = Cached->Get()) return Texture;

        if (!bColor)
        {
            // Preserve the original cutout coverage of the hair cards.
            if (auto* Mask = FImageUtils::ImportFileAsTexture2D(Path))
            {
                Textures.Add(Path, Mask);
                return Mask;
            }
            return nullptr;
        }

        FImage Image;
        if (!FImageUtils::LoadImage(*Path, Image) || Image.SizeX < 1 ||
            Image.SizeY < 1 || Image.SizeX > 2048 || Image.SizeY > 2048 || Image.NumSlices != 1)
            return FImageUtils::ImportFileAsTexture2D(Path);

        // Character atlases are loose files, so the cooker didn't generate
        // distance mips. Filter in linear space to keep skin and fabric stable.
        Image.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
        auto* Texture = UTexture2D::CreateTransient(Image.SizeX, Image.SizeY, PF_B8G8R8A8);
        if (!Texture) return nullptr;
        Texture->SRGB = true;
        Texture->NeverStream = true;
        Texture->LODGroup = TEXTUREGROUP_Character;
        Texture->Filter = TF_Trilinear;

        auto& Mips = Texture->GetPlatformData()->Mips;
        for (int32 Level = 0; ; ++Level)
        {
            if (Level) Mips.Add(new FTexture2DMipMap(Image.SizeX, Image.SizeY, 1));
            auto& Bulk = Mips[Level].BulkData;
            Bulk.Lock(LOCK_READ_WRITE);
            FMemory::Memcpy(Bulk.Realloc(Image.RawData.Num()), Image.RawData.GetData(), Image.RawData.Num());
            Bulk.Unlock();
            if (Image.SizeX == 1 && Image.SizeY == 1) break;

            FImage Smaller;
            Image.ResizeTo(Smaller, FMath::Max(1, Image.SizeX / 2), FMath::Max(1, Image.SizeY / 2),
                ERawImageFormat::BGRA8, EGammaSpace::sRGB);
            Image = MoveTemp(Smaller);
        }
        Texture->UpdateResource();
        Textures.Add(Path, Texture);
        return Texture;
    }

    bool ReadClip54(FMemoryReader& Reader, uint32 Bones, FWardPersonClip54& Clip)
    {
        uint32 Frames = 0;
        Reader << Frames << Clip.Rate;
        if (Reader.IsError() || Frames < 2 || Frames > 600 || !(Clip.Rate >= 10 && Clip.Rate <= 60))
            return false;
        Clip.Frames.SetNum(Frames);
        for (auto& Frame : Clip.Frames)
        {
            Frame.Joints.SetNumUninitialized(Bones);
            Reader.Serialize(Frame.Joints.GetData(), Bones*sizeof(FWardPersonPose54));
            Reader << Frame.Hand.X << Frame.Hand.Y << Frame.Hand.Z;
        }
        return !Reader.IsError();
    }

    bool ReadHabit55(const FString& Path, int32 ExpectedBones, FWardPersonClip54& Clip)
    {
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *Path) ||
            Bytes.Num() > 2*1024*1024) return false;
        FMemoryReader Reader(Bytes);
        uint32 Magic = 0, Bones = 0;
        Reader << Magic << Bones;
        return Magic == 0x48353545 && Bones == uint32(ExpectedBones) &&
            ReadClip54(Reader, Bones, Clip) && Reader.Tell() == Reader.TotalSize();
    }

    bool ReadMesh(const FString& Path, TArray<FWardPersonSection54>& Sections, TArray<FWardPersonClip54>& Clips)
    {
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.Num() > 16*1024*1024) return false;
        FMemoryReader Reader(Bytes);
        uint32 Magic = 0, Bones = 0, SectionCount = 0, ClipCount = 0;
        Reader << Magic << Bones << SectionCount << ClipCount;
        if (Magic != 0x4D343545 || Bones < 1 || Bones > 200 || SectionCount < 1 || SectionCount > 20 || ClipCount != 5)
            return false;
        Sections.SetNum(SectionCount);
        for (auto& Section : Sections)
        {
            uint32 Vertices = 0, Indices = 0, NameLength = 0;
            Reader << Vertices << Indices << NameLength;
            if (!Vertices || Vertices > 30000 || !Indices || Indices > 100000 || Indices % 3 || NameLength > 32)
                return false;
            ANSICHAR Name[33] = {};
            Reader.Serialize(Name, NameLength);
            Section.Material = ANSI_TO_TCHAR(Name);
            Section.Vertices.SetNumUninitialized(Vertices);
            Section.Positions.SetNumUninitialized(Vertices);
            Section.Normals.SetNumUninitialized(Vertices);
            Section.UV.Reserve(Vertices);
            for (auto& Vertex : Section.Vertices)
            {
                Reader << Vertex.Position.X << Vertex.Position.Y << Vertex.Position.Z
                       << Vertex.Normal.X << Vertex.Normal.Y << Vertex.Normal.Z << Vertex.UV.X << Vertex.UV.Y;
                Reader.Serialize(Vertex.Joints, 4);
                for (int32 I = 0; I < 4; ++I)
                {
                    Reader << Vertex.Weights[I];
                    if (Vertex.Joints[I] >= Bones || !FMath::IsFinite(Vertex.Weights[I])) return false;
                }
                Section.UV.Add(FVector2D(Vertex.UV));
            }
            Section.Indices.SetNumUninitialized(Indices);
            Reader.Serialize(Section.Indices.GetData(), Indices*sizeof(int32));
            for (int32 Index : Section.Indices) if (!Section.Vertices.IsValidIndex(Index)) return false;
        }
        Clips.SetNum(ClipCount);
        for (auto& Clip : Clips)
            if (!ReadClip54(Reader, Bones, Clip)) return false;
        return !Reader.IsError() && Reader.Tell() == Reader.TotalSize();
    }
}

FVector3f FWardPersonPose54::Apply(FVector3f V, bool bPoint) const
{
    return FVector3f(
        M[0]*V.X+M[1]*V.Y+M[2]*V.Z+(bPoint?M[3]:0.f),
        M[4]*V.X+M[5]*V.Y+M[6]*V.Z+(bPoint?M[7]:0.f),
        M[8]*V.X+M[9]*V.Y+M[10]*V.Z+(bPoint?M[11]:0.f));
}

UEWardPerson54::UEWardPerson54(const FObjectInitializer& Initializer) : Super(Initializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetLightingChannels(true,true,false);
    SetAffectDistanceFieldLighting(false);
    SetAffectDynamicIndirectLighting(false);
}

void UEWardPerson54::SetIdentity(AEWardHospitalGameplay* InStory, FString InId, FString InModel)
{
    Story = InStory;
    Id = MoveTemp(InId);
    const auto Profile = Profile55(Id);
    const FString Replacement = WardData::String(Profile, TEXT("model"));
    const FString ProfileRoot = Id.StartsWith(TEXT("FILE_")) ? FPaths::ProjectContentDir()/TEXT("CampaignData/People56") : PeoplePath55();
    bOverrideModel55 = !Replacement.IsEmpty() && FPaths::FileExists(ProfileRoot / Replacement / TEXT("mesh.bin"));
    Model = bOverrideModel55 ? Replacement : MoveTemp(InModel);
    Height55 = WardData::Number(Profile, TEXT("height"));
    Shoulders55 = WardData::Number(Profile, TEXT("shoulders"), 1);
    Waist55 = WardData::Number(Profile, TEXT("waist"), 1);
    Hips55 = WardData::Number(Profile, TEXT("hips"), 1);
    Depth55 = WardData::Number(Profile, TEXT("depth"), 1);
    Face55 = WardData::Number(Profile, TEXT("face"), 1);
    Jaw55 = WardData::Number(Profile, TEXT("jaw"), 1);
    HabitPeriod55 = WardData::Number(Profile, TEXT("period"));
    HabitDelay55 = HabitPeriod55 * .19f;
    TalkTempo55 = FMath::Clamp(1.f+.09f*float(WardData::Number(Profile, TEXT("rate"))), .65f, 1.2f);
}

FString UEWardPerson54::OpeningLine55(const FString& Id)
{
    if (Id.StartsWith(TEXT("FILE_")))
    {
        const auto& Lines=WardData::Array(Profile55(Id),TEXT("lines"));
        return Lines.IsEmpty()?FString():Lines[0]->AsString();
    }
    return WardData::String(Profile55(Id), TEXT("line"));
}

FString UEWardPerson54::VoicePath55(const FString& Id)
{
    if (Id.StartsWith(TEXT("FILE_")))
        return FPaths::ProjectContentDir()/TEXT("CampaignData/People56")/(Id+TEXT("_1.wav"));
    return PeoplePath55() / (Id + TEXT(".wav"));
}

float UEWardPerson54::HabitLength55() const
{
    return Clips.IsValidIndex(Habit) ? (Clips[Habit].Frames.Num()-1)/Clips[Habit].Rate : 0.f;
}

FVector3f UEWardPerson54::BodyScale55(float Height, float Lateral) const
{
    const float Z = Height / ReferenceHeight55;
    const float Hip = FMath::SmoothStep(.27f,.49f,Z);
    const float Waist = FMath::SmoothStep(.49f,.61f,Z);
    const float Chest = FMath::SmoothStep(.63f,.79f,Z);
    const float Head = FMath::SmoothStep(.84f,.90f,Z);
    const float Cheek = FMath::SmoothStep(.88f,.95f,Z);
    float Width = FMath::Lerp(1.f,Hips55,Hip);
    Width = FMath::Lerp(Width,Waist55,Waist);
    Width = FMath::Lerp(Width,Shoulders55,Chest);
    Width = FMath::Lerp(Width,FMath::Lerp(Jaw55,Face55,Cheek),Head);
    const float Volume = FMath::SmoothStep(.26f,.51f,Z)*(1.f-Head);
    const float NearBody = 1.f-FMath::SmoothStep(27.f,59.f,FMath::Abs(Lateral));
    const float Depth = FMath::Lerp(1.f,Depth55,Volume*NearBody);
    return FVector3f(Depth,Width,HeightScale55);
}

bool UEWardPerson54::LoadAppearance()
{
    if (bTriedLoad) return Clips.Num() >= 5;
    bTriedLoad = true;
    const FString ProfileRoot = Id.StartsWith(TEXT("FILE_")) ? FPaths::ProjectContentDir()/TEXT("CampaignData/People56") : PeoplePath55();
    const FString Directory = (bOverrideModel55 ? ProfileRoot :
        FPaths::ProjectContentDir() / TEXT("CampaignData/People54")) / Model;
    if (!ReadMesh(Directory / TEXT("mesh.bin"), Sections, Clips))
    {
        Sections.Reset();
        Clips.Reset();
        UE_LOG(LogTemp, Error, TEXT("PEOPLE54_BAD_MESH %s %s"), *Id, *Model);
        return false;
    }
    if (Height55 > 0)
    {
        ReferenceHeight55 = 1.f;
        for (const auto& Section : Sections)
            for (const auto& Vertex : Section.Vertices)
                ReferenceHeight55 = FMath::Max(ReferenceHeight55, Vertex.Position.Z);
        HeightScale55 = Height55 / ReferenceHeight55;
    }
    if (HabitPeriod55 > 0)
    {
        FWardPersonClip54 HabitClip;
        if (ReadHabit55(ProfileRoot/(Id+TEXT(".bin")), Clips[Idle].Frames[0].Joints.Num(), HabitClip)) Clips.Add(MoveTemp(HabitClip));
        else UE_LOG(LogTemp, Warning, TEXT("PEOPLE55_MISSING_HABIT %s"), *Id);
    }
    for (auto& Section : Sections)
    {
        Section.Shape55.Reserve(Section.Vertices.Num());
        for (const auto& Vertex : Section.Vertices)
            Section.Shape55.Add(BodyScale55(Vertex.Position.Z, Vertex.Position.Y));
    }
    for (int32 I = 0; I < Sections.Num(); ++I)
    {
        const FString& Part = Sections[I].Material;
        const bool bMannequin = Id == TEXT("PERSON_1");
        const bool bMask = !bMannequin && Part == TEXT("opacity") && FPaths::FileExists(Directory / TEXT("mask.png"));
        auto* Parent = LoadObject<UMaterialInterface>(nullptr, bMask
            ? TEXT("/Game/Campaign/EchoCustom/Materials/M_Real23_MASK_2S.M_Real23_MASK_2S")
            : TEXT("/Game/Campaign/EchoCustom/Materials/M_Real23_OPAQUE_2S.M_Real23_OPAQUE_2S"));
        if (!Parent) continue;
        auto* Material = UMaterialInstanceDynamic::Create(Parent, this);
        const bool bFace = Part == TEXT("head");
        const bool bHardSurface = Part == TEXT("helmet") || Part == TEXT("pistol") || Part == TEXT("stetoskop");
        const float Roughness = bMannequin ? .86f : bFace ? .62f : bHardSurface ? .57f : bMask ? .73f : .83f;
        Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
        Material->SetScalarParameterValue(TEXT("Metallic"), 0.f);
        Material->SetVectorParameterValue(TEXT("Tint"), bMannequin
            ? FLinearColor(.59f,.55f,.47f) : FLinearColor(.94f,.93f,.92f));
        if (!bMannequin)
        {
            const FString Texture = Directory / (Part + (Part == TEXT("opacity") ? TEXT(".png") : TEXT(".jpg")));
            if (FPaths::FileExists(Texture))
                if (auto* Color = LoadPersonTexture(Texture, true))
                    Material->SetTextureParameterValue(TEXT("Albedo"), Color);
            if (bMask)
                if (auto* Mask = LoadPersonTexture(Directory / TEXT("mask.png"), false))
                    Material->SetTextureParameterValue(TEXT("Alpha"), Mask);
        }
        SetMaterial(I, Material);
    }
    LoadHuman58();
    PresentPose(Idle, 0.f);
    return true;
}

void UEWardPerson54::PresentPose(int32 ClipIndex, float Time, float Blend, int32 BlendBase)
{
    if (!Clips.IsValidIndex(ClipIndex)) return;
    const auto& Clip = Clips[ClipIndex];
    const float Frame = FMath::Clamp(Time*Clip.Rate, 0.f, float(Clip.Frames.Num()-1));
    const int32 A = FMath::FloorToInt(Frame);
    const int32 B = FMath::Min(A+1, Clip.Frames.Num()-1);
    const float Alpha = Frame-A;
    Sample.SetNumUninitialized(Clip.Frames[A].Joints.Num());
    const auto& Base = Clips[BlendBase].Frames[0].Joints;
    for (int32 J = 0; J < Sample.Num(); ++J)
        for (int32 K = 0; K < 12; ++K)
        {
            const float Posed = FMath::Lerp(Clip.Frames[A].Joints[J].M[K], Clip.Frames[B].Joints[J].M[K], Alpha);
            Sample[J].M[K] = FMath::Lerp(Base[J].M[K], Posed, Blend);
        }
    PrepareHumanPose58();
    for (int32 I = 0; I < Sections.Num(); ++I)
    {
        auto& Section = Sections[I];
        for (int32 V = 0; V < Section.Vertices.Num(); ++V)
        {
            const auto& Vertex = Section.Vertices[V];
            FVector3f Position(0), Normal(0);
            const FVector3f Shape = Section.Shape55[V];
            const FVector3f Face = Section.Face58.IsValidIndex(V) ?
                Section.Face58[V].Blink*Blink58+Section.Face58[V].Jaw*Mouth58 : FVector3f::ZeroVector;
            for (int32 J = 0; J < 4; ++J)
            {
                if (Vertex.Weights[J] <= 0.f) continue;
                const int32 Bone=Vertex.Joints[J];
                const auto& Joint = Sample[Bone];
                FVector3f Point=Joint.Apply(Vertex.Position+Face,true)*Shape;
                FVector3f Direction=Joint.Apply(Vertex.Normal,false)/Shape;
                if(!RigidSkin58.IsEmpty()&&MajorMap58.IsValidIndex(Bone)&&RigidDelta58.IsValidIndex(MajorMap58[Bone]))
                {
                    const auto& Delta=RigidDelta58[MajorMap58[Bone]];
                    Point=FVector3f(Delta.TransformPosition(FVector(Point)));
                    Direction=FVector3f(Delta.TransformVectorNoScale(FVector(Direction)));
                }
                Position+=Point*Vertex.Weights[J];
                Normal+=Direction*Vertex.Weights[J];
            }
            Section.Positions[V] = FVector(Position);
            Section.Normals[V] = FVector(Normal.GetSafeNormal());
        }
        if (I >= GetNumSections())
            CreateMeshSection(I, Section.Positions, Section.Indices, Section.Normals, Section.UV, NoColors, NoTangents, false);
        else
            UpdateMeshSection(I, Section.Positions, Section.Normals, Section.UV, NoColors, NoTangents);
    }
    FVector3f Hand = FMath::Lerp(Clip.Frames[A].Hand,Clip.Frames[B].Hand,Alpha);
    const bool Smoking=SmokeTime58>=0.f&&!SmokeSkin58.IsEmpty()&&!Dead58&&!Fallen58;
    if(Smoking)
    {
        const float Raise=FMath::SmoothStep(0.f,.9f,SmokeTime58)*(1.f-FMath::SmoothStep(5.7f,6.8f,SmokeTime58));
        Hand=FMath::Lerp(Hand,SmokeHand58,Raise);
    }
    const FVector Grip(Hand*BodyScale55(Hand.Z, Hand.Y));
    for(auto Child:GetAttachChildren())if(Child&&Child->ComponentTags.Contains(TEXT("stage64_hand")))Child->SetRelativeLocation(Grip);
    if(Smoking&&!Cigarette58)
    {
        auto MakeProp=[&](const FVector& Size,const FLinearColor& Color)->UStaticMeshComponent*
        {
            auto* Prop=NewObject<UStaticMeshComponent>(GetOwner());
            Prop->SetupAttachment(this);
            Prop->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
            Prop->SetRelativeScale3D(Size/100);
            Prop->SetRelativeRotation(FRotator(90,0,0));
            Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Prop->ComponentTags.Add(TEXT("npc_smoking58"));
            auto* Material=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Campaign/BookWorld/M_HospitalSurface.M_HospitalSurface")),Prop);
            Material->SetVectorParameterValue(TEXT("Tint"),Color);
            Material->SetVectorParameterValue(TEXT("Emission"),Color.R>.5f&&Color.G<.2f?Color*.5f:FLinearColor::Black);
            Prop->SetMaterial(0,Material);
            GetOwner()->AddInstanceComponent(Prop);
            Prop->RegisterComponent();
            return Prop;
        };
        Cigarette58=MakeProp(FVector(.6,.6,6.5),FLinearColor(.75f,.71f,.61f));
        Ember58=MakeProp(FVector(.62,.62,.45),FLinearColor(.8f,.07f,.012f));
    }
    if(Cigarette58){Cigarette58->SetVisibility(Smoking);Cigarette58->SetRelativeLocation(Grip+FVector(2.9,0,0));}
    if(Ember58){Ember58->SetVisibility(Smoking);Ember58->SetRelativeLocation(Grip+FVector(6.2,0,0));}
    const bool bHolding = !Smoking&&ClipIndex == Use && Grip.X > 12 && Grip.Z > 99;
    if (bHolding && !Document)
    {
        Document = WardEchoAssets::Attach(GetOwner(), this, 262, FBox(FVector(-8.5,-5.5,0),FVector(8.5,5.5,.5)));
        if (Document) Document->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (Document)
    {
        Document->SetVisibility(bHolding);
        if (bHolding)
        {
            Document->SetRelativeLocation(Grip + FVector(2,0,1.4));
            Document->SetRelativeRotation(FRotator(-32,12,8));
        }
    }
}

void UEWardPerson54::TickComponent(float Delta, ELevelTick Tick, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Tick, Function);
    if (!Story || !Story->Mode || !Story->Mode->Hero() || GetOwner()->IsHidden()) return;
    auto* Mode = Story->Mode.Get();
    if (!Mode->CampaignStarted || Mode->TitleView || Story->HomeStage52 >= 0 || Story->ArrivalCinematic()) return;
    const FVector ToHero = Mode->Hero()->GetActorLocation() - GetOwner()->GetActorLocation();
    if (MemoryTime57>0.f && Mode->Screen.IsEmpty())
    {
        MemoryTime57-=Delta;
        if (MemoryTime57<=0.f)
        {
            Story->Definitions[Id]->SetBoolField(TEXT("memory_active57"),false);
            GetOwner()->SetActorHiddenInGame(true);
            GetOwner()->SetActorEnableCollision(false);
            return;
        }
    }
    if (ToHero.SizeSquared() > FMath::Square(1600.f)) return;
    const bool bTalking = (Mode->Screen == TEXT("story35") && Story->ActiveScene35 == TEXT("NPC_")+Id)||WardDialogue62::Speaking(*Story,Id);
    const bool Managed64=WardStaging64::Controls(*Story,Id);const bool SceneActor=WardPeople58::SceneMember(*Story,Id)||Managed64;
    if (!Mode->Screen.IsEmpty()&&Mode->Screen!=TEXT("story35")) return;
    if (!LoadAppearance() || Id == TEXT("PERSON_1")) return;
    const float Step = bTalking||SceneActor ? FMath::Min(float(FApp::GetDeltaTime()), .06f) : Delta;
    UpdateDelay += Step;
    if (UpdateDelay < (ToHero.SizeSquared()<FMath::Square(650.f)?.033f:.066f)) return;
    const float Elapsed = UpdateDelay;
    UpdateDelay = 0;

    const bool bInjured = (Id == TEXT("PERSON_6") && !Story->NarrativeFlag35(TEXT("KOVALEV_RESCUED"))) ||
                          (Id == TEXT("NEW_NPC_SEMYON") && !Story->NarrativeFlag35(TEXT("SEMYON_HELPED"))) ||
                          (Id == TEXT("PERSON_4") && !Mode->Completed.Contains(TEXT("SCENE_02_01")));
    if (bWasTalking && !bTalking && (Id == TEXT("NEW_NPC_MARINA") || Id == TEXT("NEW_NPC_GALINA") || Id == TEXT("NEW_NPC_ZOTOV")))
        DocumentTime = (Clips[Use].Frames.Num()-1)/Clips[Use].Rate;
    bWasTalking = bTalking;

    const FVector BeforeMove58=GetOwner()->GetActorLocation();
    const bool Replica = WardOnline80::Guest(*Mode);
    if (Replica && Fallen58) TickRagdoll58(Elapsed);
    const bool HumanWalking58=Replica ? WardData::Boolean(Story->Definitions.FindRef(Id),TEXT("_coop_walk")) :
        TickHuman58(Elapsed,bTalking);
    const bool Driven58=Task58!=TEXT("routine")||DangerHold58>0||Dead58||Fallen58||Recovery58>=0||SmokeTime58>=0||SceneActor||Pain58>0;
    const bool AddedPerson57=true;
    const bool Walking57=HumanWalking58||(!Replica&&!Driven58&&!bTalking&&!bInjured&&AdvancePatrol57(Elapsed));
    FVector Offset = GetRelativeLocation();
    const FVector Desired = !AddedPerson57 && !bTalking && !bInjured && DocumentTime <= 0.f &&
        ToHero.Size2D() > 145.f && ToHero.Size2D() < 300.f && FMath::Abs(ToHero.Z) < 140.f
        ? ToHero.GetSafeNormal2D()*34.f : FVector::ZeroVector;
    FVector Next = FMath::VInterpConstantTo(Offset, Desired, Elapsed, 65.f);
    if (!Next.Equals(Offset, .5))
    {
        FCollisionQueryParams Query(SCENE_QUERY_STAT(NpcStep54),false,GetOwner());
        const FVector Start = GetOwner()->GetActorLocation()+Offset+FVector(0,0,91);
        FHitResult Hit;
        if (GetWorld()->SweepSingleByChannel(Hit, Start, Start+Next-Offset, FQuat::Identity,
                ECC_Pawn, FCollisionShape::MakeCapsule(25,82), Query) ||
            !GetWorld()->LineTraceSingleByChannel(Hit, Start+Next-Offset, Start+Next-Offset-FVector(0,0,105), ECC_Visibility, Query))
            Next = Offset;
    }
    SetRelativeLocation(Next);
    if (!Replica && !Walking57 && (Managed64||ToHero.Size2D() < 470.f) && FMath::Abs(ToHero.Z) < 140.f)
        SetWorldRotation(FMath::RInterpTo(GetComponentRotation(), FRotator(0,(Managed64?WardStaging64::Focus(*Story,Id)-GetComponentLocation():ToHero).Rotation().Yaw,0), Elapsed, 5.f));
    const FString Action64=WardStaging64::PoseName(*Story,Id);
    int32 Clip = Managed64&&HumanWalking58?Walk:Managed64&&(Action64==TEXT("help")||Action64==TEXT("scan")||Action64==TEXT("panel")||Action64==TEXT("brace")||Action64==TEXT("palms")||Action64==TEXT("offer")||Action64==TEXT("tap")||Action64==TEXT("document"))?Use:Managed64&&(Action64==TEXT("wounded")||Action64==TEXT("injured")||Action64==TEXT("sit"))?Injured:bTalking ? Talk : bInjured||Pain58>0.f ? Injured : DocumentTime > 0.f ? Use
        : Walking57 || !Next.Equals(Offset,.5) ? Walk : Idle;
    const float HabitLength = HabitLength55();
    if ((Clip == Idle || Clip == Injured) && HabitLength > 0.f && ToHero.SizeSquared() < FMath::Square(680.f))
    {
        HabitDelay55 -= Elapsed;
        if (HabitDelay55 <= 0.f) Clip = Habit;
    }
    if (Clip != ActiveClip)
    {
        if (ActiveClip == Habit) HabitDelay55 = HabitPeriod55;
        ActiveClip = Clip;
        Phase = 0.f;
    }
    const float Length = (Clips[Clip].Frames.Num()-1)/Clips[Clip].Rate;
    if (Clip == Habit)
    {
        Phase = FMath::Min(Phase+Elapsed, Length);
        const float Blend = FMath::Min(FMath::SmoothStep(0.f,.30f,Phase),
            FMath::SmoothStep(0.f,.32f,Length-Phase));
        PresentPose(Habit, Phase, Blend*(bInjured ? .32f : 1.f), bInjured ? Injured : Idle);
        if (Phase >= Length)
        {
            ActiveClip = Idle;
            HabitDelay55 = HabitPeriod55;
            Phase = 0.f;
        }
        return;
    }
    const float Travel58=FVector::Dist2D(BeforeMove58,GetOwner()->GetActorLocation());
    if(Clip==Walk&&!Dead58&&!Fallen58)
    {
        StepDistance58+=Travel58;
        if(StepDistance58>=58.f)
        {
            StepDistance58=FMath::Fmod(StepDistance58,58.f);
            Mode->Cue(TEXT("step"),GetOwner()->GetActorLocation(),.14f);Story->Hear(GetOwner()->GetActorLocation()+FVector(0,0,91),State58==TEXT("flee")?550:180);
        }
    }
    const float Tempo = Clip == Talk ? TalkTempo55 : 1.f;
    const float Advance=Clip==Walk ? Travel58/105.f*Length : Elapsed*Tempo;
    Phase = FMath::Fmod(Phase+Advance, Length);
    if (DocumentTime > 0.f && !bTalking) DocumentTime = FMath::Max(0.f, DocumentTime-Elapsed);
    PresentPose(Clip, Clip == Injured ? FMath::Min(Length*.28f,.3f) : Phase);
}

bool UEWardPerson54::Ground57(const FVector& Candidate, FVector& Ground) const
{
    if (!Story) return false;
    const float Half = FMath::Max(48.f,Height55*.5f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PersonGround57),false,GetOwner());
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit,Candidate+FVector(0,0,60),
            Candidate-FVector(0,0,95),ECC_Visibility,Query) || Hit.ImpactNormal.Z < .72f) return false;
    Ground = Hit.ImpactPoint+FVector(0,0,.7f);
    if (FMath::Abs(Ground.Z-Candidate.Z)>45.f) return false;
    return !GetWorld()->OverlapBlockingTestByChannel(Ground+FVector(0,0,Half+1),
        FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(25,Half-2),Query);
}

bool UEWardPerson54::Segment57(const FVector& A, const FVector& B) const
{
    const float Half = FMath::Max(48.f,Height55*.5f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PersonWalk57),false,GetOwner());
    FHitResult Hit;
    if (GetWorld()->SweepSingleByChannel(Hit,A+FVector(0,0,Half),B+FVector(0,0,Half),
            FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(25,Half-2),Query)) return false;
    const int32 Steps = FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(A,B)/35.f));
    for (int32 Step=1; Step<=Steps; ++Step)
    {
        const FVector Probe = FMath::Lerp(A,B,float(Step)/Steps);
        if (!GetWorld()->LineTraceSingleByChannel(Hit,Probe+FVector(0,0,30),
                Probe-FVector(0,0,40),ECC_Visibility,Query) || Hit.ImpactNormal.Z < .72f) return false;
    }
    return true;
}

bool UEWardPerson54::ResetPatrol57()
{
    Patrol57.Reset();
    PatrolIndex57=0;
    PatrolWait57=0.f;
    if (!Story || Id==TEXT("PERSON_1")) return false;
    const auto Description = Story->Definitions.FindRef(Id);
    const FVector Origin = GetOwner()->GetActorLocation();
    FVector Ground;
    bool Found = Ground57(Origin,Ground);
    // Check nearby floor on the same level if an anchor sits inside furniture.
    for (int32 Ring=1; Ring<=8 && !Found; ++Ring)
        for (int32 Angle=0; Angle<12 && !Found; ++Angle)
        {
            const float ProbeAngle = Angle*PI/6.f;
            Found=Ground57(Origin+FVector(FMath::Cos(ProbeAngle),FMath::Sin(ProbeAngle),0)*float(Ring*45),Ground);
        }
    if (!Found) return false;
    GetOwner()->SetActorLocation(Ground);
    const float Extent = WardData::Number(Description,TEXT("range57"),3)*100.f;
    const float BaseAngle = WardData::Number(Description,TEXT("yaw57"))*PI/180.f;
    Patrol57.Add(Ground);
    for (float Distance : {Extent,Extent*.65f,Extent*.4f})
    {
        for (int32 Angle=0; Angle<16 && Patrol57.Num()<4; ++Angle)
        {
            const float Direction = BaseAngle+Angle*PI/8.f;
            FVector Point;
            if (Ground57(Ground+FVector(FMath::Cos(Direction),FMath::Sin(Direction),0)*Distance,Point) &&
                Segment57(Ground,Point))
            {
                bool Duplicate=false;
                for (const auto& Existing:Patrol57) Duplicate|=FVector::Dist2D(Existing,Point)<75.f;
                if (!Duplicate) { Patrol57.Add(Point); Patrol57.Add(Ground); }
            }
        }
        if (Patrol57.Num()>1) break;
    }
    PatrolIndex57=Patrol57.Num()>1 ? 1 : 0;
    UE_LOG(LogTemp,Display,TEXT("PEOPLE57_ROUTE %s points=%d at=%s"),*Id,Patrol57.Num(),*Ground.ToCompactString());
    return Patrol57.Num()>1;
}

bool UEWardPerson54::AdvancePatrol57(float Delta)
{
    if (PatrolWait57>0.f) {PatrolWait57-=Delta;return false;}
    if (Patrol57.Num()<2 && !ResetPatrol57()) {PatrolWait57=3.f;return false;}
    if (Patrol57.Num()<2 || !Story || !Story->Mode || !Story->Mode->Hero()) return false;
    const FVector At=GetOwner()->GetActorLocation();
    const FVector ToHero=Story->Mode->Hero()->GetActorLocation()-At;
    if (ToHero.Size2D()<130.f && FMath::Abs(ToHero.Z)<Height55) return false;
    const auto Description = Story->Definitions.FindRef(Id);
    const FVector Target=Patrol57[PatrolIndex57];
    const float Speed=WardData::Number(Description,TEXT("speed57"),70);
    FVector Next=FMath::VInterpConstantTo(At,Target,Delta,Speed);
    FVector Ground;
    if (!Ground57(Next,Ground) || !Segment57(At,Ground))
    {
        PatrolIndex57=(PatrolIndex57+1)%Patrol57.Num();
        PatrolWait57=1.f;
        return false;
    }
    const FVector Direction=Ground-At;
    GetOwner()->SetActorLocation(Ground);
    if (Direction.Size2D()>.1f)
        SetWorldRotation(FMath::RInterpTo(GetComponentRotation(),Direction.Rotation(),Delta,5.f));
    if (FVector::Dist2D(Ground,Target)<8.f)
    {
        PatrolIndex57=(PatrolIndex57+1)%Patrol57.Num();
        PatrolWait57=WardData::Number(Description,TEXT("wait57"),3);
    }
    return Direction.Size2D()>.1f;
}

