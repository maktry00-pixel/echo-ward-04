#include "WardCampaignAnimation.h"
#include "WardCampaign.h"
#include "WardGameplay76.h"
#include "WardMotion107.h"
#include "WardGait111.h"
#include "WardMotion107Math.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BonePose.h"
struct FEWardAnimationProxy : FAnimInstanceProxy
{
    explicit FEWardAnimationProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    UAnimSequence* Idle=nullptr;UAnimSequence* Walk=nullptr;UAnimSequence* Run=nullptr;UAnimSequence* Crouch=nullptr;
    float Speed=0,Phase=0,Aim=0,Reload=0,Clock=0,RunBlend=0,MoveBlend=0,Direction=1,LegYaw=0,LeftSupport=0;bool Armed=false,Crouching=false,LongGun=false;FVector RightTarget,LeftTarget;FQuat GunRotation;
    float LastGait=0;
    bool SawGait=false;
    virtual void PreUpdate(UAnimInstance* Instance,float D)override
    {
        FAnimInstanceProxy::PreUpdate(Instance,D);auto* A=Cast<UEWardCampaignAnimation>(Instance);Idle=A->IdleClip;Walk=A->WalkClip;Run=A->RunClip;Crouch=A->CrouchClip;
        if(auto* H=Cast<AEWardCampaignHero>(Instance->TryGetPawnOwner()))
        {
            Speed=H->GetVelocity().Size2D();Crouching=H->bIsCrouched;auto* G=WardCampaign(H);Armed=G&&WardGameplay76::Armed(*G);Aim=H->WeaponAimBlend;Reload=G?G->ReloadTime:0;
            LeftSupport=FMath::FInterpTo(LeftSupport,G&&G->Shotgun?1.f:FMath::Max(Aim,H->ReloadBlend()),D,12);
            const float Seconds=FMath::Clamp(D,0.f,.1f);
            RunBlend=WardMotion107::Follow(RunBlend,Crouching?0.f:WardGait111::RunBlend(Speed),Seconds,10.f);
            const float Moving=WardMotion107::Ease(5.f,110.f,Speed);
            MoveBlend=WardMotion107::Follow(MoveBlend,Moving,Seconds,Moving>MoveBlend?11.f:15.f);
            Direction=FVector::DotProduct(H->GetVelocity(),H->GetActorForwardVector())<-10?-1:1;
            const auto Gait=WardMotion107::Frame(*H);
            const float Travel=Gait.Gait-LastGait;
            if (SawGait&&Gait.Active&&Speed>8)
            {
                const float ForwardDelta=Travel<-.5f?Travel+1.f:Travel;
                if(ForwardDelta>=0&&ForwardDelta<=.45f)
                    Phase=FMath::Fmod(Phase+ForwardDelta*Direction+1.f,1.f);
            }
            LastGait=Gait.Gait;
            SawGait=Gait.Active;
            const FVector LocalVelocity=H->GetActorTransform().InverseTransformVectorNoScale(H->GetVelocity());float Yaw=FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y,LocalVelocity.X));if(Direction<0)Yaw=FRotator::NormalizeAxis(Yaw+180);LegYaw=FMath::FInterpTo(LegYaw,Speed>10?FMath::Clamp(Yaw,-65.f,65.f):0.f,D,10);
            const FTransform Mesh=H->GetMesh()->GetComponentTransform();RightTarget=Mesh.InverseTransformPosition(H->WeaponGrip(false));LeftTarget=Mesh.InverseTransformPosition(H->WeaponGrip(true));
            GunRotation=Mesh.InverseTransformRotation(H->WeaponPose().GetRotation());LongGun=G&&G->Shotgun;
        }
    }
    virtual void Update(float D)override{Clock+=D;}
    virtual bool Evaluate(FPoseContext& Output)override
    {
        Output.ResetToRefPose();if(!Idle)return true;
        auto Sample=[&](UAnimSequence* Clip,FPoseContext& Pose,bool Locomotion){if(!Clip)return;FAnimationPoseData Data(Pose);Clip->GetAnimationPose(Data,FAnimExtractContext(FMath::Fmod(Locomotion?Phase*Clip->GetPlayLength():Clock,Clip->GetPlayLength()),false,{},true));};
        Sample(Crouching&&Crouch?Crouch:Idle,Output,false);
        if(MoveBlend>.001f&&Walk)
        {
            FPoseContext Moving(Output);Moving.ResetToRefPose();Sample(Walk,Moving,true);
            if(Run&&RunBlend>0){FPoseContext Running(Output);Running.ResetToRefPose();Sample(Run,Running,true);for(FCompactPoseBoneIndex I:Moving.Pose.ForEachBoneIndex()){FTransform T;T.Blend(Moving.Pose[I],Running.Pose[I],RunBlend);Moving.Pose[I]=T;}}
            if(Crouching&&Crouch){FPoseContext Standing(Output);Standing.ResetToRefPose();Sample(Idle,Standing,false);for(FCompactPoseBoneIndex I:Moving.Pose.ForEachBoneIndex()){const FTransform Base=Output.Pose[I],Delta=Moving.Pose[I].GetRelativeTransform(Standing.Pose[I]);FTransform T=Delta*Base;Moving.Pose[I]=T;}}
            const float Blend=MoveBlend;
            for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex()){FTransform T;T.Blend(Output.Pose[I],Moving.Pose[I],Blend);Output.Pose[I]=T;}
        }
        const auto& Bones=Output.Pose.GetBoneContainer();TArray<FTransform> CS;CS.SetNum(Output.Pose.GetNumBones());
        auto Rebuild=[&](){for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex()){const FCompactPoseBoneIndex Parent=Output.Pose.GetParentBoneIndex(I);CS[I.GetInt()]=Parent.GetInt()<0?Output.Pose[I]:Output.Pose[I]*CS[Parent.GetInt()];}};
        Rebuild();
        auto Index=[&](const TCHAR* Name){int32 MeshIndex=Bones.GetReferenceSkeleton().FindBoneIndex(Name);if(MeshIndex==INDEX_NONE)for(int32 I=0;I<Bones.GetReferenceSkeleton().GetNum();++I)if(Bones.GetReferenceSkeleton().GetBoneName(I).ToString().Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "))==Name){MeshIndex=I;break;}return MeshIndex==INDEX_NONE?FCompactPoseBoneIndex(INDEX_NONE):Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));};
        auto Rotate=[&](FCompactPoseBoneIndex I,FVector From,FVector To)
        {
            if(I.GetInt()<0||From.IsNearlyZero()||To.IsNearlyZero())return;FTransform Desired=CS[I.GetInt()];Desired.SetRotation((FQuat::FindBetweenNormals(From.GetSafeNormal(),To.GetSafeNormal())*Desired.GetRotation()).GetNormalized());const auto Parent=Output.Pose.GetParentBoneIndex(I);Output.Pose[I]=Parent.GetInt()<0?Desired:Desired.GetRelativeTransform(CS[Parent.GetInt()]);Rebuild();
        };
        if(FMath::Abs(LegYaw)>.1f){const auto Pelvis=Index(TEXT("Bip01")),Spine=Index(TEXT("Bip01 Spine"));if(Pelvis.GetInt()>=0&&Spine.GetInt()>=0){const FTransform Upper=CS[Spine.GetInt()];FTransform Hip=CS[Pelvis.GetInt()];Hip.SetRotation(FQuat(FVector::UpVector,FMath::DegreesToRadians(LegYaw))*Hip.GetRotation());const auto Parent=Output.Pose.GetParentBoneIndex(Pelvis);Output.Pose[Pelvis]=Parent.GetInt()<0?Hip:Hip.GetRelativeTransform(CS[Parent.GetInt()]);Rebuild();Output.Pose[Spine]=Upper.GetRelativeTransform(CS[Output.Pose.GetParentBoneIndex(Spine).GetInt()]);Rebuild();}}
        auto Arm=[&](const TCHAR* UpperName,const TCHAR* LowerName,const TCHAR* HandName,FVector Target)
        {
            const auto Upper=Index(UpperName),Lower=Index(LowerName),Hand=Index(HandName);if(Upper.GetInt()<0||Lower.GetInt()<0||Hand.GetInt()<0)return;
            FVector A=CS[Upper.GetInt()].GetLocation(),B=CS[Lower.GetInt()].GetLocation(),C=CS[Hand.GetInt()].GetLocation();const double L1=FVector::Dist(A,B),L2=FVector::Dist(B,C),Distance=FMath::Clamp(FVector::Dist(A,Target),1.,L1+L2-.5);const FVector Axis=(Target-A).GetSafeNormal();FVector Bend=(FVector::DownVector-Axis*FVector::DotProduct(FVector::DownVector,Axis)).GetSafeNormal();if(Bend.IsNearlyZero())Bend=FVector::RightVector;
            const FVector Reach=A+Axis*Distance;const double Along=(L1*L1-L2*L2+Distance*Distance)/(2*Distance);const FVector Elbow=A+Axis*Along+Bend*FMath::Sqrt(FMath::Max(0.,L1*L1-Along*Along));
            Rotate(Upper,B-A,Elbow-A);B=CS[Lower.GetInt()].GetLocation();C=CS[Hand.GetInt()].GetLocation();Rotate(Lower,C-B,Reach-B);
        };
        TArray<FTransform> BeforeSupport;BeforeSupport.SetNum(Output.Pose.GetNumBones());for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())BeforeSupport[I.GetInt()]=Output.Pose[I];
        if(Armed){Arm(TEXT("Bip01 R UpperArm"),TEXT("Bip01 R Forearm"),TEXT("Bip01 R Hand"),RightTarget);if(LeftSupport>.001f)Arm(TEXT("Bip01 L UpperArm"),TEXT("Bip01 L Forearm"),TEXT("Bip01 L Hand"),LeftTarget);}
        // Solve wrist orientation from the actual finger bases, independently of
        // imported bone axes. Matching position alone left palms facing away from grips.
        FVector CurlAxes[2]={FVector::UpVector,FVector::UpVector};
        if(Armed)for(int32 Side=0;Side<2;++Side)
        {
            const bool Left=Side==1;if(Left&&LeftSupport<=.001f)continue;
            const FString Prefix=Left?TEXT("Bip01 L "):TEXT("Bip01 R ");const auto Hand=Index(*(Prefix+TEXT("Hand"))),First=Index(*(Prefix+TEXT("Finger1"))),Last=Index(*(Prefix+TEXT("Finger4")));
            if(Hand.GetInt()<0||First.GetInt()<0||Last.GetInt()<0)continue;
            const FVector At=CS[Hand.GetInt()].GetLocation(),Forward=((CS[First.GetInt()].GetLocation()+CS[Last.GetInt()].GetLocation())*.5-At).GetSafeNormal(),Across=(CS[First.GetInt()].GetLocation()-CS[Last.GetInt()].GetLocation()).GetSafeNormal();
            // Knuckles descend along the pistol grip. The old upward palm axis
            // placed fingers on top of the slide despite a zero wrist-target error.
            const FVector DesiredForward=GunRotation.RotateVector(Left&&LongGun?FVector(0,.9,.44):FVector(.35,0,-.94)).GetSafeNormal(),DesiredAcross=GunRotation.RotateVector(Left&&LongGun?FVector(1,0,0):FVector(.94,0,.35)*(Left?-1.f:1.f));
            FTransform Desired=CS[Hand.GetInt()];const FQuat Align=FRotationMatrix::MakeFromXZ(DesiredForward,DesiredAcross).ToQuat()*FRotationMatrix::MakeFromXZ(Forward,Across).ToQuat().Inverse();Desired.SetRotation((Align*Desired.GetRotation()).GetNormalized());
            Output.Pose[Hand]=Desired.GetRelativeTransform(CS[Output.Pose.GetParentBoneIndex(Hand).GetInt()]);Rebuild();CurlAxes[Side]=DesiredAcross;
            if(!Left&&!LongGun){const auto Next=Index(TEXT("Bip01 R Finger11"));if(Next.GetInt()>=0)Rotate(First,CS[Next.GetInt()].GetLocation()-CS[First.GetInt()].GetLocation(),GunRotation.RotateVector(FVector(1,-.25,.12)));}
        }
        if(Armed)for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())
        {
            const FString Name=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(I).GetInt()).ToString().Replace(TEXT("-"),TEXT(" "));
            if(!Name.Contains(TEXT("Finger"))||Name.Contains(TEXT(" L "))&&LeftSupport<=.001f)continue;
            const bool Right=Name.Contains(TEXT(" R "));const float Curl=Name.Contains(TEXT("Finger0"))?.3f:Right&&Name.Contains(TEXT("Finger1"))?.35f:.78f;
            const FVector LocalAxis=CS[I.GetInt()].InverseTransformVectorNoScale(CurlAxes[Right?0:1]).GetSafeNormal();
            FTransform At=Output.Pose[I];At.SetRotation((At.GetRotation()*FQuat(LocalAxis,!Right&&LongGun?Curl:-Curl)).GetNormalized());Output.Pose[I]=At;Rebuild();
        }
        // Fade the support arm from locomotion into the solved grip rather than
        // snapping the elbow, wrist and fingers on the first aim/reload frame.
        if(Armed&&LeftSupport<1.f)for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())
        {
            const FString Name=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(I).GetInt()).ToString().Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "));
            if(Name.Contains(TEXT(" L "))&&(Name.Contains(TEXT("UpperArm"))||Name.Contains(TEXT("Forearm"))||Name.Contains(TEXT("Hand"))||Name.Contains(TEXT("Finger")))){FTransform Blended;Blended.Blend(BeforeSupport[I.GetInt()],Output.Pose[I],LeftSupport);Output.Pose[I]=Blended;}
        }
        return true;
    }
};
void UEWardCampaignAnimation::NativeInitializeAnimation(){Super::NativeInitializeAnimation();IdleClip=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Prototype/Character/A_Idle.A_Idle"));WalkClip=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Prototype/Character/A_Walk.A_Walk"));RunClip=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Prototype/Character/A_Run.A_Run"));CrouchClip=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/Character/A_Crouch.A_Crouch"));}
FAnimInstanceProxy* UEWardCampaignAnimation::CreateAnimInstanceProxy(){return new FEWardAnimationProxy(this);}
void UEWardCampaignAnimation::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}
