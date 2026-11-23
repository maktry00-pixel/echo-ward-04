#include "WardBekaAnimation.h"
#include "WardCampaign.h"
#include "WardContact98.h"
#include "WardMotion107.h"
#include "WardHeldGrip108.h"
#include "WardGameplay76.h"
#include "WardCombat63.h"
#include "WardPistolPose.h"
#include "WardHospitalGameplay.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "BonePose.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

namespace
{
FString NormalizeBone(FName Name)
{
    return Name.ToString().Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "));
}

struct FBekaAnimationProxy : FAnimInstanceProxy
{
    explicit FBekaAnimationProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    TArray<FTransform> SourcePose,SourceReference;
    TMap<FString,int32> SourceBones;
    FVector RightGrip,LeftGrip;
    WardContact98::FPose Contact;
    WardMotion107::FFrame Locomotion;
    float ArmSwing=0;
    FVector ArmLoad=FVector::ZeroVector;
    FVector LastVelocity=FVector::ZeroVector;
    TWeakObjectPtr<AEWardCampaignHero> MotionOwner;
    bool Armed=false,LongGun=false,HoldingProp=false;
    float AimBlend=0;
    float PropWeight=0;
    float Crouch76=0,FloorZ76=8,Drop77=54;
    float SupportBlend=0,Trigger63=0,Open63=0;
    WardHeldGrip108::EGrip PropGrip=WardHeldGrip108::EGrip::Wrap;
    WardHeldGrip108::FHand PropHand[2];
    FVector NotebookThumb=FVector::ZeroVector;
    FVector PistolThumb[2]={FVector::ZeroVector,FVector::ZeroVector};
    float PistolSupportThumbWeight=0;
    FVector FootAdjust52[2]={FVector::ZeroVector,FVector::ZeroVector};
    struct FFootPlant
    {
        FVector FreeLocal=FVector::ZeroVector;
        FVector Anchor=FVector::ZeroVector;
        double SurfaceZ=0;
        bool Sampled=false,Stance=false,Locked=false;
    } FootPlant[2];
    FQuat GunRotation=FQuat::Identity,SupportRotation=FQuat::Identity;
    bool UsingPistol=false;
    float GripPressure=0;
    bool LobbyStand=false;
    double LobbyStart=0;
    float LobbyClock=0;

    void EvaluateLobby(FPoseContext& Output) const
    {
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        const auto Index = [&Bones](const TCHAR* Name)
        {
            const int32 MeshIndex = Bones.GetReferenceSkeleton().FindBoneIndex(Name);
            return MeshIndex < 0 ? FCompactPoseBoneIndex(INDEX_NONE) : Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
        };
        TArray<FTransform> Space;
        Space.SetNum(Output.Pose.GetNumBones());
        const auto Rebuild = [&]
        {
            for (const auto Slot : Output.Pose.ForEachBoneIndex())
            {
                const int32 Parent = Output.Pose.GetParentBoneIndex(Slot).GetInt();
                Space[Slot.GetInt()] = Parent < 0 ? Output.Pose[Slot] : Output.Pose[Slot] * Space[Parent];
            }
        };
        const auto Rotate = [&](FCompactPoseBoneIndex Slot, const FQuat& Delta)
        {
            if (Slot.GetInt() < 0) return;
            const int32 Parent = Output.Pose.GetParentBoneIndex(Slot).GetInt();
            FTransform Desired = Space[Slot.GetInt()];
            Desired.SetRotation((Delta * Desired.GetRotation()).GetNormalized());
            Output.Pose[Slot] = Parent < 0 ? Desired : Desired.GetRelativeTransform(Space[Parent]);
            Rebuild();
        };
        Rebuild();
        const float Breath = FMath::Sin(LobbyClock * 1.65f);
        const float Weight = FMath::Sin(LobbyClock * .76f);
        Rotate(Index(TEXT("spine_03")), FRotator(1.35f * Breath, 2.1f * Weight, 1.2f * Weight).Quaternion());
        Rotate(Index(TEXT("head")), FRotator(2.5f * Breath, 8.0f * FMath::Sin(LobbyClock * .85f), 1.3f * Weight).Quaternion());
        for (const bool Right : {false, true})
        {
            const auto Upper = Index(Right ? TEXT("upperarm_r") : TEXT("upperarm_l"));
            const auto Lower = Index(Right ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
            if (Upper.GetInt() < 0 || Lower.GetInt() < 0) continue;
            const FVector From = (Space[Lower.GetInt()].GetLocation() - Space[Upper.GetInt()].GetLocation()).GetSafeNormal();
            const FVector Outward(From.X, From.Y, 0);
            const FVector Relaxed = (FVector::DownVector + Outward.GetSafeNormal() * (.28f + .04f * Weight)).GetSafeNormal();
            Rotate(Upper, FQuat::FindBetweenNormals(From, Relaxed));
        }
    }

    virtual void PreUpdate(UAnimInstance* Instance,float Delta) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Delta);
        USkeletalMeshComponent* Target=Instance->GetSkelMeshComponent();
        AActor* Owner=Target?Target->GetOwner():nullptr;
        LobbyStand = Owner && Owner->ActorHasTag(TEXT("OnlineLobbyBeka"));
        if (LobbyStand)
        {
            const double Time = FPlatformTime::Seconds();
            if (LobbyStart == 0) LobbyStart = Time;
            LobbyClock = float(Time - LobbyStart);
            return;
        }
        AEWardCampaignHero* Hero=Cast<AEWardCampaignHero>(Owner);
        if(!Hero&&Owner)Hero=Cast<AEWardCampaignHero>(Owner->GetOwner());
        Contact = {};
        Locomotion = {};
        if(!Hero||!Hero->GetMesh()->GetSkeletalMeshAsset())
        {
            MotionOwner.Reset();
            ArmSwing=0;
            ArmLoad=FVector::ZeroVector;
            return;
        }
        Contact = WardContact98::Pose(*Hero);
        Locomotion = WardMotion107::Frame(*Hero);
        AimBlend=FMath::Clamp(Hero->WeaponAimBlend,0.f,1.f);
        const auto* Movement=Hero->GetCharacterMovement();
        const FVector Velocity=Hero->GetVelocity();
        const bool Grounded=Locomotion.Active&&Movement->IsMovingOnGround();
        if(MotionOwner.Get()!=Hero)
        {
            MotionOwner=Hero;
            LastVelocity=Velocity;
            ArmSwing=0;
            ArmLoad=FVector::ZeroVector;
            for(int32 Side=0;Side<2;++Side)
            {
                FootPlant[Side]={};
                FootAdjust52[Side]=FVector::ZeroVector;
            }
        }
        const float Dt=FMath::Clamp(Delta,0.f,.10f);
        const float Weight=Grounded?FMath::Clamp(Hero->MotionWeight52,0.f,1.45f)*(Hero->bIsCrouched?.45f:1.f):0.f;
        ArmSwing=FMath::Lerp(ArmSwing,Weight,1.f-FMath::Exp(-Dt*9.f));
        // Derive bracing from actual acceleration, including braking. The grip anchors
        // remain attached to the item; only the elbows and the closing fingers respond.
        const FVector Accel=Grounded&&Dt>.001f?
            Hero->GetActorTransform().InverseTransformVectorNoScale((Velocity-LastVelocity)/FMath::Max(Dt,1.f/120.f)):
            FVector::ZeroVector;
        LastVelocity=Velocity;
        const FVector Load(FMath::Clamp(Accel.X/1300.,-1.,1.),FMath::Clamp(Accel.Y/1300.,-1.,1.),0);
        ArmLoad=FMath::Lerp(ArmLoad,Load,1.f-FMath::Exp(-Dt*10.f));
        if (Contact.Reach > .001f)
        {
            const auto Transform = Target->GetComponentTransform();
            Contact.Right = Transform.InverseTransformPosition(Contact.Right);
            Contact.Left = Transform.InverseTransformPosition(Contact.Left);
            Contact.Rotation = Transform.InverseTransformRotation(Contact.Rotation);
        }
        const auto& BodyTransform=Target->GetComponentTransform();
        const FVector Travel(Velocity.X,Velocity.Y,0);
        const double Speed=Travel.Size();
        const FVector TravelDirection=Travel.GetSafeNormal();
        const float CrouchTarget=Hero->bIsCrouched?1.f:0.f;
        const bool CanPlant=Grounded&&Speed>30.&&Speed<360.&&Dt>.001f&&
            FMath::Abs(Locomotion.Turn)<.45f&&FMath::Abs(Crouch76-CrouchTarget)<.12f;
        // Keep a short stance anchor in world space. Use the estimated uncorrected
        // socket to detect swing so the prior IK result cannot extend its own lock.
        for(int32 Side=0;Side<2;++Side)
        {
            auto& Plant=FootPlant[Side];
            bool Sampled=false;
            FVector Offset=FVector::ZeroVector;const FName Bone=Side?TEXT("foot_r"):TEXT("foot_l");
            if(Movement->IsMovingOnGround()&&!Hero->IsHidden()&&Target->DoesSocketExist(Bone))
            {
                const FVector Ankle=Target->GetSocketLocation(Bone);FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(BekaFoot52),false,Hero);Q.AddIgnoredActor(Owner);
                if(Hero->GetWorld()->LineTraceSingleByChannel(Hit,Ankle+FVector(0,0,28),Ankle-FVector(0,0,55),ECC_Pawn,Q)&&Hit.ImpactNormal.Z>.65f)
                {
                    const float Floor=Hero->GetActorLocation().Z-Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                    // Preserve the swing arc; only compensate local stair/slope height, bounded to 12 cm.
                    const float Swing=FMath::Clamp(1.f-float(Ankle.Z-Floor-8)/24.f,0.f,1.f);
                    FVector WorldOffset(0,0,FMath::Clamp(float(Hit.ImpactPoint.Z-Floor),-12.f,12.f)*Swing);
                    if(CanPlant&&Hit.ImpactNormal.Z>.88f)
                    {
                        const FVector Free=Ankle-BodyTransform.TransformVectorNoScale(FootAdjust52[Side]);
                        const FVector CurrentLocal=BodyTransform.InverseTransformPosition(Free);
                        const FVector RelativeMotion=BodyTransform.TransformVectorNoScale(CurrentLocal-Plant.FreeLocal);
                        const double Height=Free.Z-Hit.ImpactPoint.Z;
                        const bool Stance=Plant.Sampled&&Height>-1.&&Height<19.&&
                            FMath::Abs(RelativeMotion.Z)<Dt*100.&&
                            FVector::DotProduct(-RelativeMotion,TravelDirection)>Dt*Speed*.17;
                        if(Stance&&!Plant.Stance)
                        {
                            Plant.Anchor=Free;
                            Plant.SurfaceZ=Hit.ImpactPoint.Z;
                            Plant.Locked=true;
                        }
                        const FVector Difference=Plant.Anchor-Free;
                        const FVector Drift(Difference.X,Difference.Y,0);
                        Plant.Locked=Plant.Locked&&Stance&&Drift.SizeSquared()<64.&&
                            FMath::Abs(Plant.SurfaceZ-Hit.ImpactPoint.Z)<3.;
                        if(Plant.Locked) WorldOffset+=Drift.GetClampedToMaxSize(6.);
                        Plant.FreeLocal=CurrentLocal;
                        Plant.Stance=Stance;
                        Plant.Sampled=true;
                        Sampled=true;
                    }
                    Offset=BodyTransform.InverseTransformVectorNoScale(WorldOffset);
                }
            }
            if(!Sampled) Plant={};
            FootAdjust52[Side]=FMath::VInterpTo(FootAdjust52[Side],Offset,Dt,Plant.Locked?19.f:13.f);
        }
        Crouch76=FMath::FInterpTo(Crouch76,CrouchTarget,Dt,12);
        FloorZ76=Target->GetComponentTransform().InverseTransformPosition(Hero->GetActorLocation()-FVector(0,0,Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-8)).Z;
        Drop77=FMath::Clamp(2.f*(88.f-Hero->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight())*.9f,0.f,78.f);
        auto* Source=Hero->GetMesh();
        SourcePose=Source->GetComponentSpaceTransforms();
        const FReferenceSkeleton& Ref=Source->GetSkeletalMeshAsset()->GetRefSkeleton();
        if(SourceReference.Num()!=Ref.GetNum())
        {
            SourceReference=Ref.GetRefBonePose();SourceBones.Reset();
            for(int32 I=0;I<Ref.GetNum();++I)
            {
                if(Ref.GetParentIndex(I)>=0)SourceReference[I]=SourceReference[I]*SourceReference[Ref.GetParentIndex(I)];
                SourceBones.Add(NormalizeBone(Ref.GetBoneName(I)),I);
            }
        }
        auto* Mode=WardCampaign(Hero);
        const AActor* Prop=Mode?(Mode->PhoneOpen76?Mode->PhoneActor76.Get():Mode->PreviewActor51.Get()):nullptr;
        HoldingProp=Prop&&!Prop->IsHidden();
        const bool WeaponPreview=Mode&&!Mode->PhoneOpen76&&(Mode->PreviewItem51==TEXT("weapon")||Mode->PreviewItem51==TEXT("shotgun"));
        Trigger63=Mode?WardCombat63::Trigger(*Mode):0;
        Open63=Mode?WardCombat63::OpenHand(*Mode):0;
        Armed=Mode&&!Hero->Dead&&(HoldingProp||WeaponPreview||WardGameplay76::Armed(*Mode));
        LongGun=Armed&&!HoldingProp&&(WeaponPreview?Mode->PreviewItem51==TEXT("shotgun"):Mode->Shotgun);
        SupportBlend=Armed&&!HoldingProp?1.f:0.f;
        if(Armed&&!HoldingProp)
        {
            const auto& Mesh=Target->GetComponentTransform();
            const FTransform Gun=Hero->WeaponPose();
            RightGrip=Mesh.InverseTransformPosition(Hero->WeaponGrip(false));
            LeftGrip=Mesh.InverseTransformPosition(Hero->WeaponGrip(true));
            GunRotation=Mesh.InverseTransformRotation(Gun.GetRotation());
            if(!LongGun)
            {
                PistolThumb[0]=Mesh.InverseTransformPosition(Gun.TransformPosition(FVector(2,-2.1,-6.4)));
                PistolThumb[1]=Mesh.InverseTransformPosition(Gun.TransformPosition(FVector(4.5,-3.2,-5.2)));
            }
        }
        UsingPistol=Armed&&!HoldingProp&&!LongGun;
        GripPressure=Armed&&!HoldingProp?FMath::Clamp(.18f*float(ArmLoad.Size2D())+
            (Hero->WeaponKick+float(Mode->NumberFlag(TEXT("combat63_kick"))))*.21f,0.f,1.f):0.f;
        PropWeight=0;
        if(UsingPistol)
        {
            const auto Reload=WardPistolPose::Reload(Mode->ReloadTime);
            Open63=Reload.FingerOpen;
            PistolSupportThumbWeight=1.f-FMath::Max(Reload.GripBlend,Reload.SlideGrip);
            SupportRotation=(GunRotation*FRotator(10.f*Reload.GripBlend-12.f*Reload.SlideGrip,0,
                -36.f*Reload.GripBlend+57.f*Reload.SlideGrip).Quaternion()).GetNormalized();
            if(Hero->WeaponCloseBlend>.2f||WardPistolPose::SprintLowering(Hero->MotionWeight52)>.2f)Trigger63=0;
        }
        if(HoldingProp)
        {
            const auto Profile=Mode->PhoneOpen76?WardHeldGrip108::Phone():WardHeldGrip108::Preview(Prop);
            const auto& Item=Prop->GetActorTransform();
            const auto& Mesh=Target->GetComponentTransform();
            PropGrip=Profile.Grip;
            PropWeight=FMath::Clamp(Profile.Weight,0.f,1.f);
            SupportBlend=Profile.TwoHands?1.f:0.f;
            GunRotation=Mesh.InverseTransformRotation(Item.GetRotation());
            for(int32 Side=0;Side<2;++Side)
                PropHand[Side]=WardHeldGrip108::Anchor(Profile, Side==1);
            RightGrip=Mesh.InverseTransformPosition(Item.TransformPosition(PropHand[0].Wrist));
            LeftGrip=Mesh.InverseTransformPosition(Item.TransformPosition(PropHand[1].Wrist));
            if(PropGrip==WardHeldGrip108::EGrip::Notebook)
                NotebookThumb=Mesh.InverseTransformPosition(
                    Item.TransformPosition(FVector(-.82,Profile.Half.Y-1.1,-Profile.Half.Z*.55)));
        }
    }

    virtual bool Evaluate(FPoseContext& Output) override
    {
        Output.ResetToRefPose();
        if (LobbyStand)
        {
            EvaluateLobby(Output);
            return true;
        }
        if(SourcePose.IsEmpty()||SourceReference.IsEmpty())return true;
        const FBoneContainer& Bones=Output.Pose.GetBoneContainer();
        TArray<FTransform> CS,Reference;CS.SetNum(Output.Pose.GetNumBones());
        auto Rebuild=[&]()
        {
            for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())
            {
                const auto Parent=Output.Pose.GetParentBoneIndex(I);
                CS[I.GetInt()]=Parent.GetInt()<0?Output.Pose[I]:Output.Pose[I]*CS[Parent.GetInt()];
            }
        };
        Rebuild();Reference=CS;
        TMap<FString,FString> Names={
            {TEXT("pelvis"),TEXT("Bip01 Pelvis")},
            {TEXT("spine_01"),TEXT("Bip01 Spine")},{TEXT("spine_03"),TEXT("Bip01 Spine1")},{TEXT("spine_05"),TEXT("Bip01 Spine2")},
            {TEXT("neck_01"),TEXT("Bip01 Neck")},{TEXT("neck_02"),TEXT("Bip01 Neck")},{TEXT("head"),TEXT("Bip01 Head")}
        };
        for(const FString Side:{FString(TEXT("l")),FString(TEXT("r"))})
        {
            const FString Prefix=Side==TEXT("l")?TEXT("Bip01 L "):TEXT("Bip01 R ");
            for(const auto& Entry:TMap<FString,FString>{{TEXT("clavicle"),TEXT("Clavicle")},{TEXT("upperarm"),TEXT("UpperArm")},{TEXT("lowerarm"),TEXT("Forearm")},{TEXT("hand"),TEXT("Hand")},{TEXT("thigh"),TEXT("Thigh")},{TEXT("calf"),TEXT("Calf")},{TEXT("foot"),TEXT("Foot")},{TEXT("ball"),TEXT("Toe0")}})
                Names.Add(Entry.Key+TEXT("_")+Side,Prefix+Entry.Value);
            const TCHAR* Fingers[]={TEXT("thumb"),TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky")};
            for(int32 Finger=0;!Armed&&Finger<5;++Finger)for(int32 Segment=1;Segment<=3;++Segment)
                Names.Add(FString::Printf(TEXT("%s_0%d_%s"),Fingers[Finger],Segment,*Side),Prefix+FString::Printf(TEXT("Finger%d%s"),Finger,Segment==1?TEXT(""):Segment==2?TEXT("1"):TEXT("2")));
        }
        for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())
        {
            const int32 Parent=Output.Pose.GetParentBoneIndex(I).GetInt();
            // Compact-pose indices are parent-first, so ancestors already contain the retargeted pose.
            CS[I.GetInt()]=Parent<0?Output.Pose[I]:Output.Pose[I]*CS[Parent];
            const FString Name=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(I).GetInt()).ToString();
            const FString* SourceName=Names.Find(Name);if(!SourceName)continue;
            const int32* SourceIndex=SourceBones.Find(*SourceName);
            if(!SourceIndex&&Name==TEXT("pelvis"))SourceIndex=SourceBones.Find(TEXT("Bip01"));
            if(!SourceIndex||!SourcePose.IsValidIndex(*SourceIndex))continue;
            const FQuat Delta=SourcePose[*SourceIndex].GetRotation()*SourceReference[*SourceIndex].GetRotation().Inverse();
            FTransform Desired=CS[I.GetInt()];Desired.SetRotation((Delta*Reference[I.GetInt()].GetRotation()).GetNormalized());
            if(Name==TEXT("pelvis"))Desired.SetLocation(Reference[I.GetInt()].GetLocation()+(SourcePose[*SourceIndex].GetLocation()-SourceReference[*SourceIndex].GetLocation())*.9722f*(1-Crouch76));
            Output.Pose[I]=Parent<0?Desired:Desired.GetRelativeTransform(CS[Parent]);
            CS[I.GetInt()]=Parent<0?Output.Pose[I]:Output.Pose[I]*CS[Parent];
        }
        const bool Touching = Contact.Reach > .001f;
        if(Armed||Touching)for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())
        {
            const int32 MeshIndex=Bones.MakeMeshPoseIndex(I).GetInt();const FString Name=Bones.GetReferenceSkeleton().GetBoneName(MeshIndex).ToString();
            if(Name==TEXT("hand_l")||Name==TEXT("hand_r")||Name.StartsWith(TEXT("thumb_"))||Name.StartsWith(TEXT("index_"))||Name.StartsWith(TEXT("middle_"))||Name.StartsWith(TEXT("ring_"))||Name.StartsWith(TEXT("pinky_")))Output.Pose[I]=Bones.GetReferenceSkeleton().GetRefBonePose()[MeshIndex];
        }
        Rebuild();
        auto Index=[&](const TCHAR* Name)
        {
            const int32 MeshIndex=Bones.GetReferenceSkeleton().FindBoneIndex(Name);
            return MeshIndex==INDEX_NONE?FCompactPoseBoneIndex(INDEX_NONE):Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
        };
        auto Rotate=[&](FCompactPoseBoneIndex I,FVector From,FVector To)
        {
            if(I.GetInt()<0||From.IsNearlyZero()||To.IsNearlyZero())return;
            FTransform Desired=CS[I.GetInt()];Desired.SetRotation((FQuat::FindBetweenNormals(From.GetSafeNormal(),To.GetSafeNormal())*Desired.GetRotation()).GetNormalized());
            const auto Parent=Output.Pose.GetParentBoneIndex(I);Output.Pose[I]=Parent.GetInt()<0?Desired:Desired.GetRelativeTransform(CS[Parent.GetInt()]);Rebuild();
        };
        if(Locomotion.Active)
        {
            const float FollowAim=Armed?FMath::Lerp(.42f,.23f,AimBlend):1.f;
            const float Counter=FMath::Sin(Locomotion.Gait*2.f*PI);
            const auto Chest=Index(TEXT("spine_04"));
            if(Chest.GetInt()>=0)
            {
                auto Local=Output.Pose[Chest];
                const FQuat Delta=FRotator(Locomotion.Breath*2.5f,
                    (Counter*ArmSwing*1.5f+Locomotion.Turn*2.f)*FollowAim,
                    -Counter*ArmSwing*.85f*FollowAim).Quaternion();
                const auto Parent=Output.Pose.GetParentBoneIndex(Chest);
                const FQuat ParentRotation=Parent.GetInt()<0?FQuat::Identity:CS[Parent.GetInt()].GetRotation();
                Local.SetRotation((ParentRotation.Inverse()*Delta*ParentRotation*Local.GetRotation()).GetNormalized());
                Output.Pose[Chest]=Local;
                Rebuild();
            }
            if(!Armed&&!Touching&&ArmSwing>.025f)
            {
                for(const bool Right : {false,true})
                {
                    const auto Upper=Index(Right?TEXT("upperarm_r"):TEXT("upperarm_l"));
                    const auto Lower=Index(Right?TEXT("lowerarm_r"):TEXT("lowerarm_l"));
                    if(Upper.GetInt()<0||Lower.GetInt()<0)continue;
                    const FVector Axis=CS[Lower.GetInt()].GetLocation()-CS[Upper.GetInt()].GetLocation();
                    const float Swing=Counter*(Right?1.f:-1.f);
                    const float SwingAngle=(Swing>0?6.f:4.f)*Swing*ArmSwing;
                    const double Angle=FMath::DegreesToRadians(SwingAngle-ArmLoad.X*2.0);
                    Rotate(Upper,Axis,FQuat(FVector::RightVector,Angle).RotateVector(Axis));
                }
            }
        }
        auto Reach=[&](const TCHAR* UpperName,const TCHAR* LowerName,const TCHAR* HandName,FVector Target,FVector Pole=FVector::ZeroVector)
        {
            const auto Upper=Index(UpperName),Lower=Index(LowerName),Hand=Index(HandName);
            if(Upper.GetInt()<0||Lower.GetInt()<0||Hand.GetInt()<0)return;
            const FVector A=CS[Upper.GetInt()].GetLocation(),B=CS[Lower.GetInt()].GetLocation(),C=CS[Hand.GetInt()].GetLocation();
            const double L1=FVector::Dist(A,B),L2=FVector::Dist(B,C);
            if(FMath::Min(L1,L2)<1.)return;
            const double Distance=FMath::Clamp(FVector::Dist(A,Target),FMath::Abs(L1-L2)+.8,L1+L2-.8);
            const FVector Axis=(Target-A).GetSafeNormal(UE_SMALL_NUMBER,(C-A).GetSafeNormal(UE_SMALL_NUMBER,FVector::ForwardVector));
            const auto Project=[&](FVector V){return (V-Axis*FVector::DotProduct(V,Axis)).GetSafeNormal();};
            FVector Bend=Project(Pole);
            if(Bend.IsNearlyZero())Bend=Project(B-(A+C)*.5);
            if(Bend.IsNearlyZero())Bend=Project(FVector::DownVector);
            if(Bend.IsNearlyZero())Bend=Project(FVector::RightVector);
            const double Along=(L1*L1-L2*L2+Distance*Distance)/(2*Distance);const FVector Elbow=A+Axis*Along+Bend*FMath::Sqrt(FMath::Max(0.,L1*L1-Along*Along));
            Rotate(Upper,B-A,Elbow-A);Rotate(Lower,CS[Hand.GetInt()].GetLocation()-CS[Lower.GetInt()].GetLocation(),A+Axis*Distance-CS[Lower.GetInt()].GetLocation());
        };
        if(Crouch76>.001f)
        {
            // Lower the pelvis in mesh space while feet stay at the physical floor.
            FVector Feet[2];for(int Side=0;Side<2;++Side){const auto I=Index(Side?TEXT("foot_r"):TEXT("foot_l"));Feet[Side]=I.GetInt()>=0?CS[I.GetInt()].GetLocation():FVector::ZeroVector;Feet[Side].Z=FMath::Lerp(Feet[Side].Z,FloorZ76,Crouch76);}
            const auto Hip=Index(TEXT("pelvis"));if(Hip.GetInt()>=0){FTransform Desired=CS[Hip.GetInt()];Desired.AddToTranslation(FVector(-9,0,-Drop77)*Crouch76);const auto Parent=Output.Pose.GetParentBoneIndex(Hip);Output.Pose[Hip]=Parent.GetInt()<0?Desired:Desired.GetRelativeTransform(CS[Parent.GetInt()]);Rebuild();}
            Reach(TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l"),Feet[0]);Reach(TEXT("thigh_r"),TEXT("calf_r"),TEXT("foot_r"),Feet[1]);
        }
        auto Wrist=[&](bool Left)
        {
            const auto Hand=Index(Left?TEXT("hand_l"):TEXT("hand_r")),First=Index(Left?TEXT("index_01_l"):TEXT("index_01_r")),Last=Index(Left?TEXT("pinky_01_l"):TEXT("pinky_01_r"));
            if(Hand.GetInt()<0||First.GetInt()<0||Last.GetInt()<0)return;
            const FVector At=CS[Hand.GetInt()].GetLocation(),Forward=((CS[First.GetInt()].GetLocation()+CS[Last.GetInt()].GetLocation())*.5-At).GetSafeNormal(),Across=(CS[First.GetInt()].GetLocation()-CS[Last.GetInt()].GetLocation()).GetSafeNormal();
            const FQuat& Rotation=Touching?Contact.Rotation:Left&&UsingPistol?SupportRotation:GunRotation;
            const auto& HandPose=PropHand[Left?1:0];
            const FVector DesiredForward=Rotation.RotateVector(HoldingProp&&!Touching?HandPose.Forward:
                Left&&LongGun?FVector(1,0,.12):FVector(1,0,.2)).GetSafeNormal();
            const FVector DesiredAcross=Rotation.RotateVector(HoldingProp&&!Touching?HandPose.Across:FVector(0,0,Left?-1.f:1.f));
            FTransform Desired=CS[Hand.GetInt()];const FQuat Align=FRotationMatrix::MakeFromXZ(DesiredForward,DesiredAcross).ToQuat()*FRotationMatrix::MakeFromXZ(Forward,Across).ToQuat().Inverse();
            const FQuat Goal=(Align*Desired.GetRotation()).GetNormalized();Desired.SetRotation(Touching?FQuat::Slerp(Desired.GetRotation(),Goal,Contact.Reach):Goal);
            Output.Pose[Hand]=Desired.GetRelativeTransform(CS[Output.Pose.GetParentBoneIndex(Hand).GetInt()]);Rebuild();
        };
        auto Fingers=[&](bool Left)
        {
            const auto First=Index(Left?TEXT("index_01_l"):TEXT("index_01_r")),Last=Index(Left?TEXT("pinky_01_l"):TEXT("pinky_01_r"));
            if(First.GetInt()<0||Last.GetInt()<0)return;
            const auto Hand=Index(Left?TEXT("hand_l"):TEXT("hand_r"));
            if(Hand.GetInt()<0)return;
            const FVector Axis=(CS[First.GetInt()].GetLocation()-CS[Last.GetInt()].GetLocation()).GetSafeNormal();
            const FVector Forward=((CS[First.GetInt()].GetLocation()+CS[Last.GetInt()].GetLocation())*.5-CS[Hand.GetInt()].GetLocation()).GetSafeNormal();
            const TCHAR* Names[]={TEXT("thumb"),TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky")};
            if(!Touching&&!Left&&UsingPistol)
            {
                const auto Base=Index(TEXT("index_01_r")),Tip=Index(TEXT("index_02_r"));
                if(Base.GetInt()>=0&&Tip.GetInt()>=0)Rotate(Base,CS[Tip.GetInt()].GetLocation()-CS[Base.GetInt()].GetLocation(),GunRotation.RotateVector(FVector(1,-.2,.05)));
            }
            for(int32 F=0;F<5;++F)for(int32 Segment=1;Segment<=3;++Segment)
            {
                const auto Bone=Index(*FString::Printf(TEXT("%s_0%d_%s"),Names[F],Segment,Left?TEXT("l"):TEXT("r")));if(Bone.GetInt()<0)continue;
                const float Base=F==0?.38f:!Left&&UsingPistol&&F==1?(Segment==1?Trigger63*.25f:.08f+Trigger63*.4f):(Segment==1?.85f:Segment==2?1.05f:.65f);
                const float Pressure=!HoldingProp&&F>=2?1.f+.13f*GripPressure:1.f;
                float Amount=Base*Pressure*(Left?1-Open63*.8f:1.f);
                float Opposition=0;
                if (Touching)
                {
                    const float Curl = F==0?.42f : Segment==2?.95f:.64f;
                    const bool Pinch = Contact.Grip == WardContact98::EGrip::Key;
                    Amount=Curl*(.14f*Contact.Reach+Contact.GripAmount*.86f)*(Pinch?(F<=1?.85f:.28f):1.f);
                    Opposition=Pinch?.42f*Contact.GripAmount:.18f*Contact.GripAmount;
                }
                else if(HoldingProp)
                {
                    using enum WardHeldGrip108::EGrip;
                    switch(PropGrip)
                    {
                        case Page: Amount=F==0?.29f:Segment==1?.12f:Segment==2?.18f:.14f; Opposition=.37f; break;
                        case Notebook: Amount=F==0?.27f:Segment==1?.32f:Segment==2?.48f:.30f; Opposition=.46f; break;
                        case Pinch: Amount=F<=1?(Segment==2?.68f:.43f):(Segment==2?.62f:.43f); Opposition=.49f; break;
                        case Phone: Amount=F==0?.21f:Base*.60f; Opposition=.21f; break;
                        case Handle: Amount=F==0?.51f:Base*(.96f+.04f*F); Opposition=.33f; break;
                        case Cradle: Amount=F==0?.22f:Base*.24f; Opposition=.24f; break;
                        case Wrap: Amount=F==0?.48f:Base*(.72f+.055f*F); Opposition=.31f; break;
                    }
                }
                const auto Parent=Output.Pose.GetParentBoneIndex(Bone);
                const FTransform ParentPose=Parent.GetInt()<0?FTransform::Identity:CS[Parent.GetInt()];
                const bool LeftPalmUp=Left&&HoldingProp&&!Touching&&PropGrip==WardHeldGrip108::EGrip::Cradle;
                FQuat Curl=FQuat(Axis,LeftPalmUp?Amount:-Amount);
                if(F==0&&Segment==1&&Opposition>0)Curl=FQuat(Forward,Left?-Opposition:Opposition)*Curl;
                auto& Pose=Output.Pose[Bone];
                const FQuat ParentRotation=ParentPose.GetRotation();
                Pose.SetRotation((ParentRotation.Inverse()*Curl*ParentRotation*Pose.GetRotation()).GetNormalized());
                CS[Bone.GetInt()]=Pose*ParentPose;
            }
            Rebuild();
        };
        if (Touching)
        {
            auto Grip = [&](bool Left, FVector Target)
            {
                const TCHAR* Hand = Left ? TEXT("hand_l") : TEXT("hand_r");
                const auto IndexHand = Index(Hand);
                if (IndexHand.GetInt() < 0) return;
                Reach(Left?TEXT("upperarm_l"):TEXT("upperarm_r"), Left?TEXT("lowerarm_l"):TEXT("lowerarm_r"),
                    Hand, FMath::Lerp(CS[IndexHand.GetInt()].GetLocation(), Target, Contact.Reach), FVector(-.26,Left?-.68:.68,-1));
                Wrist(Left);
                Fingers(Left);
            };
            Grip(false, Contact.Right);
            if (Contact.Grip == WardContact98::EGrip::Cart) Grip(true, Contact.Left);
        }
        else if(Armed)
        {
            const auto ElbowPole=[&](bool Left)
            {
                const double Side=Left?-1.:1.;
                const double Brace=HoldingProp?.45+.35*PropWeight:FMath::Lerp(.65,.18,AimBlend);
                const double Width=HoldingProp?.59:LongGun?.52:FMath::Lerp(.65,.48,AimBlend);
                return FVector(-.26-ArmLoad.X*.24*Brace,
                    Side*Width-ArmLoad.Y*.32*Brace-Locomotion.Turn*.06*Brace,-1.);
            };
            const auto RestThumb=[&](bool Left,float Weight)
            {
                const TCHAR* Name=Left?TEXT("thumb_03_l"):TEXT("thumb_03_r");
                const auto Tip=Index(Name);
                if(!UsingPistol||Weight<=.001f||Tip.GetInt()<0)return;
                const FVector Target=FMath::Lerp(CS[Tip.GetInt()].GetLocation(),PistolThumb[Left?1:0],Weight);
                Reach(Left?TEXT("thumb_01_l"):TEXT("thumb_01_r"),Left?TEXT("thumb_02_l"):TEXT("thumb_02_r"),
                    Name,Target,GunRotation.RotateVector(FVector(-.2,Left?-.85:.75,-.5)));
            };
            Reach(TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r"),RightGrip,ElbowPole(false));
            Wrist(false);Fingers(false);
            RestThumb(false,1.f);
            if(HoldingProp&&PropGrip==WardHeldGrip108::EGrip::Notebook)
                Reach(TEXT("thumb_01_r"),TEXT("thumb_02_r"),TEXT("thumb_03_r"),NotebookThumb,GunRotation.RotateVector(FVector(-.5,-.2,-.8)));
            if(SupportBlend>.001f)
            {
                Reach(TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),LeftGrip,ElbowPole(true));
                Wrist(true);Fingers(true);
                RestThumb(true,PistolSupportThumbWeight);
            }
        }
        for(int32 Side=0;Side<2;++Side)
        {
            const auto Foot=Index(Side?TEXT("foot_r"):TEXT("foot_l"));
            if(Foot.GetInt()>=0&&!FootAdjust52[Side].IsNearlyZero(.05f))
            {
                const FQuat Rotation=CS[Foot.GetInt()].GetRotation();const FVector Goal=CS[Foot.GetInt()].GetLocation()+FootAdjust52[Side];
                Reach(Side?TEXT("thigh_r"):TEXT("thigh_l"),Side?TEXT("calf_r"):TEXT("calf_l"),Side?TEXT("foot_r"):TEXT("foot_l"),Goal);
                FTransform Desired=CS[Foot.GetInt()];Desired.SetRotation(Rotation);Output.Pose[Foot]=Desired.GetRelativeTransform(CS[Output.Pose.GetParentBoneIndex(Foot).GetInt()]);Rebuild();
            }
        }
        return true;
    }
};
}
FAnimInstanceProxy* UEWardBekaAnimation::CreateAnimInstanceProxy(){return new FBekaAnimationProxy(this);}
void UEWardBekaAnimation::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}

namespace
{
struct FBekaFaceProxy : FAnimInstanceProxy
{
    explicit FBekaFaceProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    TArray<FTransform> BodyPose;
    TMap<FName,int32> BodyBones;
    virtual void PreUpdate(UAnimInstance* Instance,float Delta) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Delta);
        auto* Face=Instance->GetSkelMeshComponent();auto* Body=Face?Cast<USkeletalMeshComponent>(Face->GetAttachParent()):nullptr;
        if(!Body||!Body->GetSkeletalMeshAsset())return;
        BodyPose=Body->GetComponentSpaceTransforms();
        if(BodyBones.IsEmpty())
        {
            const auto& Ref=Body->GetSkeletalMeshAsset()->GetRefSkeleton();for(int32 I=0;I<Ref.GetNum();++I)BodyBones.Add(Ref.GetBoneName(I),I);
        }
    }
    virtual bool Evaluate(FPoseContext& Output) override
    {
        Output.ResetToRefPose();const auto& Bones=Output.Pose.GetBoneContainer();TArray<FTransform> CS;CS.SetNum(Output.Pose.GetNumBones());
        for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())
        {
            const auto Parent=Output.Pose.GetParentBoneIndex(I);
            const FName Name=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(I).GetInt());
            const int32* Source=BodyBones.Find(Name);
            if(Source&&BodyPose.IsValidIndex(*Source))
            {
                CS[I.GetInt()]=BodyPose[*Source];Output.Pose[I]=Parent.GetInt()<0?CS[I.GetInt()]:CS[I.GetInt()].GetRelativeTransform(CS[Parent.GetInt()]);
            }
            else CS[I.GetInt()]=Parent.GetInt()<0?Output.Pose[I]:Output.Pose[I]*CS[Parent.GetInt()];
        }
        return true;
    }
};
}
FAnimInstanceProxy* UEWardBekaFaceAnimation::CreateAnimInstanceProxy(){return new FBekaFaceProxy(this);}
void UEWardBekaFaceAnimation::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}
