#include "WardMonsterVisual59.h"
#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardMonsterMotion97.h"
#include "WardDoor100.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ImageUtils.h"
#include "Engine/Texture2D.h"
#include "Misc/Compression.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"

namespace
{
struct FFoot97
{
    FVector Plant = FVector::ZeroVector;
    bool Locked = false;
};

struct FPoseMotion97
{
    FVector Position = FVector::ZeroVector;
    FVector LeapOrigin = FVector::ZeroVector;
    float LastTime = -1, LastSpeed = 0, SkidUntil = -1, SkidAmount = 0, Lead = 0;
    bool Winding = false, Recovering = false;
    FFoot97 Feet[2];
};

TMap<TWeakObjectPtr<UEWardMonsterVisual59>, FPoseMotion97> MotionStates97;

void TransformPose97(FWardPersonPose54& Pose, const FVector& Pivot, const FQuat& Rotation, const FVector& Shift)
{
    const FVector X = Rotation.RotateVector(FVector(Pose.M[0], Pose.M[4], Pose.M[8]));
    const FVector Y = Rotation.RotateVector(FVector(Pose.M[1], Pose.M[5], Pose.M[9]));
    const FVector Z = Rotation.RotateVector(FVector(Pose.M[2], Pose.M[6], Pose.M[10]));
    const FVector T = Pivot + Shift + Rotation.RotateVector(FVector(Pose.M[3], Pose.M[7], Pose.M[11]) - Pivot);
    const float Values[] = {float(X.X), float(Y.X), float(Z.X), float(T.X), float(X.Y), float(Y.Y),
        float(Z.Y), float(T.Y), float(X.Z), float(Y.Z), float(Z.Z), float(T.Z)};
    FMemory::Memcpy(Pose.M, Values, sizeof Values);
}

void AlignPose97(FWardPersonPose54& Pose, const FVector& OldStart, const FVector& OldEnd,
    const FVector& NewStart, const FVector& NewEnd)
{
    const FQuat Turn = FQuat::FindBetweenVectors(OldEnd - OldStart, NewEnd - NewStart);
    TransformPose97(Pose, OldStart, Turn, NewStart - OldStart);
}

bool Support97(UWorld& World, const AActor& Owner, const FVector& Point, FVector& Surface)
{
    FHitResult Hit;
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(MonsterFoot97), false, &Owner);
    const FCollisionObjectQueryParams Floors(ECC_WorldStatic);
    if (!World.LineTraceSingleByObjectType(Hit, Point + FVector(0, 0, 35), Point - FVector(0, 0, 42),
        Floors, Query) || Hit.ImpactNormal.Z < .64f) return false;
    Surface = Hit.ImpactPoint;
    return true;
}
}

void UEWardMonsterVisual59::Unload61()
{
    MotionStates97.Remove(this);
    ClearAllMeshSections();EmptyOverrideMaterials();Sections59.Empty();Clips59.Empty();Sample59.Empty();Bones61.Empty();Particles61.Empty();BlendFrom61.Empty();VertexCount59=0;Model61.Empty();LastClip61=-1;
}
bool UEWardMonsterVisual59::Load59(const FString& Model,float Height)
{
    if(Model61==Model&&VertexCount59>0)return true;
    Unload61();
    const FString Directory=FPaths::ProjectContentDir()/TEXT("CampaignData/Monsters61")/Model;
    TArray<uint8> Packed,Data;
    if(!FFileHelper::LoadFileToArray(Packed,*(Directory/TEXT("mesh.bin")))||Packed.Num()<8||Packed.Num()>16*1024*1024)return false;
    FMemoryReader Header(Packed);uint32 Magic=0,Size=0;Header<<Magic<<Size;
    if(Magic!=0x5A313645||Size<16||Size>16*1024*1024)return false;
    Data.SetNumUninitialized(Size);
    if(!FCompression::UncompressMemory(NAME_Zlib,Data.GetData(),Size,Packed.GetData()+8,Packed.Num()-8))return false;
    FMemoryReader Reader(Data);uint32 Bones=0,Parts=0,Clips=0;Reader<<Magic<<Bones<<Parts<<Clips;
    if(Magic!=0x4D313645||Bones<1||Bones>32||Parts!=1||Clips!=6)return false;
    Bones61.SetNum(Bones);
    for(uint32 I=0;I<Bones;++I){auto& B=Bones61[I];Reader<<B.Parent<<B.Pivot.X<<B.Pivot.Y<<B.Pivot.Z<<B.Tip.X<<B.Tip.Y<<B.Tip.Z<<B.Radius;if(B.Parent>=int32(I)||B.Parent< -1||B.Pivot.ContainsNaN()||B.Tip.ContainsNaN()||!FMath::IsFinite(B.Radius))return false;}
    Sections59.SetNum(Parts);float Maximum=1;
    for(auto& S:Sections59)
    {
        uint32 Nv=0,Ni=0,Nt=0;Reader<<Nv<<Ni<<Nt;if(Nv<3||Nv>80000||Ni>240000||Ni%3||Nt>80)return false;
        TArray<ANSICHAR> Name;Name.SetNumZeroed(Nt+1);Reader.Serialize(Name.GetData(),Nt);S.Material=ANSI_TO_TCHAR(Name.GetData());
        S.Vertices.SetNum(Nv);S.Positions.SetNum(Nv);S.Normals.SetNum(Nv);S.UV.SetNum(Nv);VertexCount59+=Nv;
        for(uint32 I=0;I<Nv;++I)
        {
            auto& V=S.Vertices[I];Reader<<V.Position.X<<V.Position.Y<<V.Position.Z<<V.Normal.X<<V.Normal.Y<<V.Normal.Z<<V.UV.X<<V.UV.Y;
            Reader.Serialize(V.Joints,4);float Sum=0;
            for(int J=0;J<4;++J){Reader<<V.Weights[J];if(V.Joints[J]>=Bones||!FMath::IsFinite(V.Weights[J])||V.Weights[J]<0)return false;Sum+=V.Weights[J];}
            if(V.Position.ContainsNaN()||V.Normal.ContainsNaN()||V.UV.ContainsNaN()||FMath::Abs(Sum-1)>.01)return false;
            Maximum=FMath::Max(Maximum,V.Position.Z);S.UV[I]=FVector2D(V.UV);
        }
        S.Indices.SetNum(Ni);Reader.Serialize(S.Indices.GetData(),Ni*4);for(int32 I:S.Indices)if(I<0||uint32(I)>=Nv)return false;
    }
    Height59=Height;Scale59=Height/Maximum;Clips59.SetNum(Clips);
    for(auto& C:Clips59)
    {
        uint32 Frames=0;Reader<<Frames<<C.Rate;if(Frames<2||Frames>120||C.Rate<=0||C.Rate>120)return false;C.Frames.SetNum(Frames);
        for(auto& F:C.Frames){F.Joints.SetNum(Bones);Reader.Serialize(F.Joints.GetData(),Bones*sizeof(FWardPersonPose54));Reader<<F.Hand.X<<F.Hand.Y<<F.Hand.Z;for(auto& J:F.Joints)for(float V:J.M)if(!FMath::IsFinite(V))return false;}
    }
    if(Reader.IsError()||Reader.Tell()!=Reader.TotalSize())return false;
    auto* Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/EchoCustom/Materials/M_Real23_OPAQUE_2S.M_Real23_OPAQUE_2S"));if(!Parent)return false;
    auto* Mat=UMaterialInstanceDynamic::Create(Parent,this);
    static TMap<FString,TWeakObjectPtr<UTexture2D>> Cache;
    auto* Image=Cache.FindRef(Model).Get();if(!Image){Image=FImageUtils::ImportFileAsTexture2D(Directory/TEXT("albedo.jpg"));if(Image)Cache.Add(Model,Image);}if(!Image)return false;
    Mat->SetTextureParameterValue(TEXT("Albedo"),Image);Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor::White);Mat->SetVectorParameterValue(TEXT("Emission"),FLinearColor::Black);Mat->SetScalarParameterValue(TEXT("Roughness"),.72f);Mat->SetScalarParameterValue(TEXT("NormalStrength"),0);
    SetMaterial(0,Mat);Model61=Model;NextPose59=0;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);SetReceivesDecals(false);SetLightingChannels(true,true,false);SetCullDistance(5000);SetAffectDistanceFieldLighting(false);SetAffectDynamicIndirectLighting(false);
    Present59(0,0,false,0);return true;
}
void UEWardMonsterVisual59::DrawPose61()
{
    for(int S=0;S<Sections59.Num();++S)
    {
        auto& Section=Sections59[S];
        for(int I=0;I<Section.Vertices.Num();++I)
        {
            const auto& V=Section.Vertices[I];FVector3f P(0),N(0);
            for(int J=0;J<4;++J)if(V.Weights[J]>0){P+=Sample59[V.Joints[J]].Apply(V.Position,true)*V.Weights[J];N+=Sample59[V.Joints[J]].Apply(V.Normal,false)*V.Weights[J];}
            Section.Positions[I]=FVector(P)*Scale59;Section.Normals[I]=FVector(N.GetSafeNormal());
        }
        if(S>=GetNumSections())CreateMeshSection(S,Section.Positions,Section.Indices,Section.Normals,Section.UV,TArray<FColor>(),TArray<FProcMeshTangent>(),false);
        else UpdateMeshSection(S,Section.Positions,Section.Normals,Section.UV,TArray<FColor>(),TArray<FProcMeshTangent>());
    }
}
void UEWardMonsterVisual59::Present59(int ClipIndex,float Time,bool Dead,float DeathTime)
{
    if(Clips59.IsEmpty()||HasFall61())return;
    const float Now=GetWorld()->GetTimeSeconds();
    float Interval=.043f;
    if(auto* M=Cast<AEWardHospitalMob>(GetOwner());M&&M->Runtime&&M->Runtime->Mode->Hero())
    {float Dist=FVector::Dist(GetComponentLocation(),M->Runtime->Mode->Hero()->GetActorLocation());if(Dist>5000)return;if(Dist>1800)Interval=.125f;}
    ClipIndex=FMath::Clamp(ClipIndex,0,Clips59.Num()-1);
    if(LastClip61==ClipIndex&&Now<NextPose59&&GetNumSections()>0)return;
    if(LastClip61!=ClipIndex)
    {
        if(LastClip61>=0)BlendFrom61=Sample59;
        else BlendFrom61.Reset();
        BlendAt61=Now;LastClip61=ClipIndex;
    }
    NextPose59=Now+Interval;
    const auto& C=Clips59[ClipIndex];const float End=C.Frames.Num()-1;
    const bool Loop=ClipIndex==0||ClipIndex==1||ClipIndex==5;
    const float Frame=Loop?FMath::Fmod(FMath::Max(0.f,Time)*C.Rate,End):FMath::Clamp(Time*C.Rate,0.f,End);
    int A=FMath::FloorToInt(Frame),B=FMath::Min(A+1,C.Frames.Num()-1);float Alpha=Frame-A,Blend=FMath::Clamp((Now-BlendAt61)/.13f,0.f,1.f);
    Sample59.SetNum(Bones61.Num());
    for(int J=0;J<Sample59.Num();++J)for(int K=0;K<12;++K){float V=FMath::Lerp(C.Frames[A].Joints[J].M[K],C.Frames[B].Joints[J].M[K],Alpha);Sample59[J].M[K]=BlendFrom61.IsValidIndex(J)?FMath::Lerp(BlendFrom61[J].M[K],V,Blend):V;}
    ApplyMotion97(Frame / End, ClipIndex, Now);
    DrawPose61();
}

FVector UEWardMonsterVisual59::MotionJoint97(int32 Index, bool Tip) const
{
    if (!Bones61.IsValidIndex(Index) || !Sample59.IsValidIndex(Index)) return GetComponentLocation();
    const auto& Bone = Bones61[Index];
    return GetComponentTransform().TransformPosition(FVector(Sample59[Index].Apply(Tip ? Bone.Tip : Bone.Pivot, true)) * Scale59);
}

FVector UEWardMonsterVisual59::MotionChestForward97() const
{
    if (!Sample59.IsValidIndex(1)) return GetForwardVector();
    return GetComponentTransform().TransformVectorNoScale(
        FVector(Sample59[1].Apply(FVector3f(1, 0, 0), false))).GetSafeNormal2D();
}

void UEWardMonsterVisual59::ApplyMotion97(float Phase, int32 Clip, float Now)
{
    auto* Mob = Cast<AEWardHospitalMob>(GetOwner());
    if (!Mob || !GetWorld() || Scale59 <= UE_SMALL_NUMBER || Sample59.Num() < 4) return;
    static double NextCleanup = 0;
    if (const double Time = FPlatformTime::Seconds(); Time > NextCleanup)
    {
        for (auto It = MotionStates97.CreateIterator(); It; ++It)
            if (!It.Key().IsValid()) It.RemoveCurrent();
        NextCleanup = Time + 10;
    }
    auto& Motion = MotionStates97.FindOrAdd(this);
    const auto& Style = WardMonsterMotion97::Style(Mob->Species);
    const bool Windup = Clip == 2 && Mob->State59 == TEXT("windup");
    const bool Recovery = Clip == 2 && Mob->State59 == TEXT("recovery");
    const bool Moving = Clip == 1 || Clip == 5;
    const FVector Position = Mob->GetActorLocation();
    const float Dt = Motion.LastTime < 0 ? .043f : FMath::Clamp(Now - Motion.LastTime, .01f, .18f);
    const float Speed = Mob->GetVelocity().Size2D();
    const float Delta = FVector::Dist2D(Position, Motion.Position);
    if (Motion.LastTime < 0 || (Delta > 185 && !Recovery) || Now - Motion.LastTime > .45f) Motion = FPoseMotion97();
    const float EstimatedSpeed = (Motion.LastTime >= 0 && Delta < 110) ? Delta / Dt : 0.f;
    const float Pace = FMath::Max(Speed, EstimatedSpeed);
    if (Style.Pounce && !Recovery && Motion.LastSpeed > 160 && Pace + 68 < Motion.LastSpeed)
    {
        Motion.SkidUntil = Now + .29f;
        Motion.SkidAmount = FMath::Clamp(Motion.LastSpeed / 280.f, .5f, 1.f);
    }
    if (Recovery && !Motion.Recovering)
        Motion.LeapOrigin = Motion.Winding ? Motion.Position : Position;
    const float Skid = FMath::Clamp((Motion.SkidUntil - Now) / .29f, 0.f, 1.f) * Motion.SkidAmount;
    const bool FirstPose = Motion.LastTime < 0;
    Motion.LastTime = Now;
    Motion.LastSpeed = Pace;
    Motion.Position = Position;
    Motion.Winding = Windup;
    Motion.Recovering = Recovery;

    const auto LocalJoint = [&](int32 I) { return FVector(Sample59[I].Apply(Bones61[I].Pivot, true)); };
    const FVector Hips = LocalJoint(0);
    const float Warmup = Windup ? FMath::Clamp(Mob->StateClock59 /
        FMath::Max(.12f, float(WardData::Number(Mob->Rule59, TEXT("windup_seconds"), .65))), 0.f, 1.f) : 0.f;
    const float Coil = WardMonsterMotion97::Preload(Warmup);
    const auto Transform = GetComponentTransform();
    if (Sample59.Num() == 4)
    {
        const float Wave = FMath::Sin(Now * (Style.Sway > .03f ? 1.8f : 2.7f));
        for (int32 I = 1; I < 4; ++I)
            TransformPose97(Sample59[I], Hips, FRotator(Wave * Style.Sway * I * 60, 0, 0).Quaternion(),
                FVector(0, 0, Wave * I * Style.Sway * Height59 / Scale59 * .28f));
        return;
    }

    FVector Desired = FVector::ZeroVector;
    if (Windup) Desired = Mob->AttackAim59 - Position;
    else if (Moving && Mob->Path.IsValidIndex(Mob->PathIndex))
    {
        const FVector To = Mob->Path[Mob->PathIndex] - Position;
        const FVector Next = Mob->Path.IsValidIndex(Mob->PathIndex + 1)
            ? Mob->Path[Mob->PathIndex + 1] - Mob->Path[Mob->PathIndex] : FVector::ZeroVector;
        Desired = WardMonsterMotion97::CornerFacing(To, Next, To.Size2D());
    }
    else if (Moving) Desired = Mob->GetVelocity();
    const float Angle = Desired.SizeSquared2D() > UE_SMALL_NUMBER
        ? float(FMath::FindDeltaAngleDegrees(Mob->GetActorRotation().Yaw, Desired.Rotation().Yaw)) : 0.f;
    const float TargetLead = FMath::Clamp(Angle * Style.ShoulderLead, -28.f, 28.f);
    Motion.Lead = FirstPose ? TargetLead * .55f : FMath::FInterpTo(Motion.Lead, TargetLead, Dt, 11.f);
    const float Swing = Moving ? FMath::Sin(Phase * 2 * PI) : 0.f;
    if (Sample59.Num() == 8)
    {
        for (int32 I = 0; I < Sample59.Num(); ++I)
            TransformPose97(Sample59[I], Hips, FRotator(-Style.GaitPitch * FMath::Abs(Swing), Motion.Lead * .48f, 0).Quaternion(),
                FVector(0, Swing * Style.Sway * Height59 / Scale59, 0));
        return;
    }
    if (Sample59.Num() != 16) return;

    const float Side = (GetTypeHash(Mob->Id) & 1) ? 1.f : -1.f;
    const float Brace = Style.Load * Coil;
    const float Compress = Style.Pounce ? Coil : 0.f;
    const float Land = Style.Pounce && Recovery ? FMath::Clamp((Mob->StateClock59 - .21f) / .17f, 0.f, 1.f) : 0.f;
    const float PlantCompression = FMath::Sin(PI * Land);
    const float Inspect = Mob->State59==TEXT("hideout_check") ? FMath::SmoothStep(.05f,.72f,Mob->StateClock59) : 0.f;
    const bool InFlight = Style.Pounce && Recovery && Mob->StateClock59 < .26f;
    const float Leap = InFlight ? WardMonsterMotion97::PounceHeight(Mob->StateClock59) : 0.f;
    const FVector HipShift = FVector(-Height59 * (.065f * Compress + .017f * Brace) + Skid * 7,
        Height59 * (Side * Brace * .072f + Swing * Style.Sway),
        -Height59 * (.16f * Compress + .034f * Brace) - Skid * 5.f - PlantCompression * 8.f - Inspect*15.f) / Scale59;
    const FVector LeapOffset = InFlight
        ? Transform.InverseTransformVectorNoScale((Motion.LeapOrigin - Position).GetClampedToMaxSize(115.f)) *
            (1.f - FMath::SmoothStep(0.f, .26f, Mob->StateClock59)) / Scale59 : FVector::ZeroVector;
    const float Lean = (Moving ? -Style.GaitPitch * (Clip == 5 ? 1.f : .55f) : 0.f)
        - Compress * 22.f - Skid * 14.f - PlantCompression * 11.f - Inspect*24.f;
    TransformPose97(Sample59[0], Hips, FQuat::Identity, HipShift);
    for (int32 I = 1; I < 10; ++I)
        TransformPose97(Sample59[I], Hips, FRotator(Lean, Motion.Lead, -Side * Brace * 7.f + Swing * Style.Sway * 180.f).Quaternion(), HipShift);

    FVector Handle;
    if (WardDoor100::HandTarget(*Mob,Handle))
    {
        const float Age = Mob->StateClock59;
        const float Contact = FMath::SmoothStep(0.f,.35f,Age)*(1.f-FMath::SmoothStep(1.08f,1.55f,Age));
        const FVector Shoulder = LocalJoint(7), Elbow = LocalJoint(8), Wrist = LocalJoint(9);
        const FVector Goal = FMath::Lerp(Wrist,Transform.InverseTransformPosition(Handle)/Scale59,Contact);
        const double Upper = FVector::Dist(Shoulder,Elbow), Lower = FVector::Dist(Elbow,Wrist);
        const FVector Axis = (Goal-Shoulder).GetSafeNormal();
        if (!Axis.IsNearlyZero() && Upper>.2 && Lower>.2)
        {
            const double Reach = FMath::Clamp(FVector::Dist(Shoulder,Goal),FMath::Abs(Upper-Lower)+.1,Upper+Lower-.1);
            const double Along = (Upper*Upper-Lower*Lower+Reach*Reach)/(2*Reach);
            FVector Bend = FVector::VectorPlaneProject(FVector(0,.65,-1),Axis).GetSafeNormal();
            if (Bend.IsNearlyZero()) Bend = FVector::VectorPlaneProject(FVector::RightVector,Axis).GetSafeNormal();
            const FVector AdjustedElbow = Shoulder+Axis*Along+Bend*FMath::Sqrt(FMath::Max(0.,Upper*Upper-Along*Along));
            const FVector AdjustedWrist = Shoulder+Axis*Reach;
            AlignPose97(Sample59[7],Shoulder,Elbow,Shoulder,AdjustedElbow);
            AlignPose97(Sample59[8],Elbow,Wrist,AdjustedElbow,AdjustedWrist);
            TransformPose97(Sample59[9],Wrist,FQuat::Identity,AdjustedWrist-Wrist);
        }
    }

    for (int32 Leg = 0; Leg < 2; ++Leg)
    {
        const int32 Thigh = 10 + Leg * 3, Shin = Thigh + 1, Foot = Shin + 1;
        const FVector Hip = LocalJoint(Thigh), Knee = LocalJoint(Shin), Ankle = LocalJoint(Foot);
        FVector Goal = Ankle;
        const FVector Sole(Bones61[Foot].Tip.X, Bones61[Foot].Tip.Y, 0);
        const FVector Predicted = Transform.TransformPosition(
            FVector(Sample59[Foot].Apply(FVector3f(Sole), true)) * Scale59);
        auto& Lock = Motion.Feet[Leg];
        const float Step = FMath::Fmod(Phase + Leg * .5f, 1.f);
        const bool Stance = !Moving || Step < .60f;
        FVector Floor;
        if (!InFlight && Clip != 3 && Support97(*GetWorld(), *Mob, Predicted, Floor))
        {
            const float Lift = Moving && !Stance
                ? FMath::Sin(PI * FMath::Clamp((Step - .60f) / .4f, 0.f, 1.f)) : 0.f;
            if (Stance && !Lock.Locked) Lock.Plant = Floor;
            if (Stance && FVector::DistSquared2D(Predicted, Lock.Plant) > FMath::Square(34.f))
                Lock.Plant = Floor;
            Lock.Locked = Stance;
            const float PlantWeight = Moving && Stance ? FMath::Min(FMath::SmoothStep(0.f, .10f, Step),
                1.f - FMath::SmoothStep(.46f, .60f, Step)) : 0.f;
            FVector DesiredSole = FMath::Lerp(Floor, Lock.Plant, PlantWeight);
            if (Lift > 0) DesiredSole.Z = FMath::Max(Predicted.Z, Floor.Z + Lift * 5.f);
            DesiredSole.Z += 1.35f;
            const FVector Correction = DesiredSole - Predicted;
            const float MaxVertical = Moving ? 27.f : 23.f;
            Goal += Transform.InverseTransformVectorNoScale(FVector(Correction.X, Correction.Y,
                FMath::Clamp(Correction.Z, -MaxVertical, MaxVertical))) / Scale59;
        }
        else Lock.Locked = false;
        if (HipShift.IsNearlyZero() && Goal.Equals(Ankle, .08)) continue;

        const FVector NewHip = Hip + HipShift;
        const float Upper = FVector::Dist(Hip, Knee), Lower = FVector::Dist(Knee, Ankle);
        if (Upper + Lower < UE_SMALL_NUMBER) continue;
        const FVector Axis = (Goal - NewHip).GetSafeNormal();
        if (Axis.IsNearlyZero()) continue;
        const float Reach = FMath::Clamp(float(FVector::Dist(NewHip, Goal)), FMath::Abs(Upper - Lower) + .1f, Upper + Lower - .1f);
        const float Along = (Upper * Upper - Lower * Lower + Reach * Reach) / (2.f * Reach);
        FVector Bend = FVector::VectorPlaneProject(Knee - Hip, Axis).GetSafeNormal();
        if (Bend.IsNearlyZero()) Bend = FVector::VectorPlaneProject(FVector::ForwardVector, Axis).GetSafeNormal();
        const FVector NewKnee = NewHip + Axis * Along + Bend * FMath::Sqrt(FMath::Max(0.f, Upper * Upper - Along * Along));
        const FVector NewAnkle = NewHip + Axis * Reach;
        AlignPose97(Sample59[Thigh], Hip, Knee, NewHip, NewKnee);
        AlignPose97(Sample59[Shin], Knee, Ankle, NewKnee, NewAnkle);
        TransformPose97(Sample59[Foot], Ankle, FQuat::Identity, NewAnkle - Ankle);
    }
    if (InFlight)
        for (auto& Bone : Sample59)
            TransformPose97(Bone, Hips, FQuat::Identity, LeapOffset + FVector(0, 0, Leap / Scale59));
}

void UEWardMonsterVisual59::ResetFall61(){Particles61.Empty();FallAge61=0;LastClip61=-1;NextPose59=0;MotionStates97.Remove(this);}
void UEWardMonsterVisual59::BeginFall61(FVector Direction)
{
    if(Sample59.Num()!=Bones61.Num()||HasFall61())return;
    FallAge61=0;Direction=Direction.GetSafeNormal2D();if(Direction.IsNearlyZero())Direction=-GetForwardVector();
    for(int I=0;I<Bones61.Num();++I)for(FVector3f P:{Bones61[I].Pivot,Bones61[I].Tip})
    {
        FParticle61 V;V.Position=GetComponentTransform().TransformPosition(FVector(Sample59[I].Apply(P,true))*Scale59);V.Radius=FMath::Clamp(Bones61[I].Radius*Scale59,3.f,10.f);
        V.Previous=V.Position-Direction*(1.2f+P.Z/Height59*2.4f);Particles61.Add(V);
    }
}
void UEWardMonsterVisual59::TickFall61(float Delta)
{
    if(!HasFall61()||FallAge61>=2.5f)return;
    const float Dt=FMath::Min(Delta,.066f);const int Steps=FMath::Max(1,FMath::CeilToInt(Dt/.016f));const float H=Dt/Steps;
    auto Limit=[&](int A,int B,float Length){FVector D=Particles61[B].Position-Particles61[A].Position;float L=D.Size();if(L>UE_SMALL_NUMBER){FVector C=D*((L-Length)/L*.5);Particles61[A].Position+=C;Particles61[B].Position-=C;}};
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CreatureFall61),false,GetOwner());
    for(int S=0;S<Steps;++S)
    {
        for(auto& P:Particles61)
        {
            const FVector Before=P.Position;P.Position+=(P.Position-P.Previous)*.93f+FVector(0,0,-980)*H*H;P.Previous=Before;
        }
        for(int Pass=0;Pass<4;++Pass)
        {
            for(int I=0;I<Bones61.Num();++I)
            {
                const auto& B=Bones61[I];Limit(I*2,I*2+1,FVector(B.Tip-B.Pivot).Size()*Scale59);
                if(B.Parent>=0){const auto& Parent=Bones61[B.Parent];bool Tip=FVector::DistSquared(FVector(Parent.Tip),FVector(B.Pivot))<FVector::DistSquared(FVector(Parent.Pivot),FVector(B.Pivot));Limit(I*2,B.Parent*2+(Tip?1:0),FVector(B.Pivot-(Tip?Parent.Tip:Parent.Pivot)).Size()*Scale59);}
            }
            for(auto& P:Particles61)
            {
                FHitResult Hit;
                if(GetWorld()->SweepSingleByChannel(Hit,P.Previous,P.Position,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(P.Radius),Query))
                {P.Position=Hit.bStartPenetrating?P.Position+Hit.Normal*(Hit.PenetrationDepth+1):Hit.Location+Hit.Normal*.6;P.Previous=P.Position;}
            }
        }
    }
    FallAge61+=Dt;ApplyFall61();
}
void UEWardMonsterVisual59::ApplyFall61()
{
    if(Particles61.Num()!=Bones61.Num()*2)return;Sample59.SetNum(Bones61.Num());
    const FTransform Inv=GetComponentTransform().Inverse();
    for(int I=0;I<Bones61.Num();++I)
    {
        const auto& B=Bones61[I];FVector A=Inv.TransformPosition(Particles61[I*2].Position)/Scale59,C=Inv.TransformPosition(Particles61[I*2+1].Position)/Scale59;
        const FQuat Q=FQuat::FindBetweenVectors(FVector(B.Tip-B.Pivot),C-A);const FVector T=A-Q.RotateVector(FVector(B.Pivot));
        const FVector X=Q.RotateVector(FVector::ForwardVector),Y=Q.RotateVector(FVector::RightVector),Z=Q.RotateVector(FVector::UpVector);
        const float Values[12]={float(X.X),float(Y.X),float(Z.X),float(T.X),float(X.Y),float(Y.Y),float(Z.Y),float(T.Y),float(X.Z),float(Y.Z),float(Z.Z),float(T.Z)};
        FMemory::Memcpy(Sample59[I].M,Values,sizeof(Values));
    }
    DrawPose61();
}
TArray<FVector> UEWardMonsterVisual59::CaptureFall61()const {TArray<FVector> Out;for(auto& P:Particles61)Out.Add(P.Position);return Out;}
void UEWardMonsterVisual59::RestoreFall61(const TArray<FVector>& Points)
{
    if(Points.Num()!=Bones61.Num()*2)return;for(auto P:Points)if(P.ContainsNaN()||FVector::Dist(P,GetComponentLocation())>650)return;
    Particles61.Empty();for(auto P:Points){FParticle61 V;V.Position=V.Previous=P;V.Radius=4;Particles61.Add(V);}FallAge61=3;ApplyFall61();
}
