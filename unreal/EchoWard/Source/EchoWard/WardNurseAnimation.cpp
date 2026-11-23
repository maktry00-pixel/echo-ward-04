#include "WardNurseAnimation.h"
#include "WardTitleScene.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraActor.h"
struct FWardNurseProxy : FAnimInstanceProxy
{
    explicit FWardNurseProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    UAnimSequence* Idle=nullptr;UAnimSequence* Walk=nullptr;
    float Clock=0,Moving=0,Peek=0,Scare=0,Speed=0;FVector RightTarget,LeftTarget;
    virtual void PreUpdate(UAnimInstance* Instance,float Delta)override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Delta);auto* Anim=Cast<UEWardNurseAnimation>(Instance);Idle=Anim->Idle;Walk=Anim->Walk;
        if(auto* Scene=Cast<AEWardTitleScene>(Instance->GetOwningActor()))
        {
            Moving=FMath::FInterpTo(Moving,Scene->NurseMoving,Delta,7);Peek=FMath::FInterpTo(Peek,Scene->NursePeek,Delta,6);Scare=Scene->NurseScare;Speed=Scene->NurseSpeed;
            const FTransform Mesh=Instance->GetSkelMeshComponent()->GetComponentTransform();const FVector At=Scene->Camera->GetActorLocation();RightTarget=Mesh.InverseTransformPosition(At+FVector(8,-31,-18));LeftTarget=Mesh.InverseTransformPosition(At+FVector(8,31,-18));
        }
    }
    virtual void Update(float Delta)override{Clock+=Delta*FMath::Lerp(.55f,FMath::Clamp(Speed/130.f,.3f,.9f),Moving);}
    virtual bool Evaluate(FPoseContext& Output)override
    {
        Output.ResetToRefPose();if(!Idle)return true;
        auto Sample=[&](UAnimSequence* Clip,FPoseContext& Pose){FAnimationPoseData Data(Pose);Clip->GetAnimationPose(Data,FAnimExtractContext(FMath::Fmod(Clock,Clip->GetPlayLength()),false,{},true));};
        Sample(Idle,Output);
        if(Walk&&Moving>.001f){FPoseContext MovingPose(Output);MovingPose.ResetToRefPose();Sample(Walk,MovingPose);for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex()){FTransform T;T.Blend(Output.Pose[I],MovingPose.Pose[I],Moving);Output.Pose[I]=T;}}
        const auto& Bones=Output.Pose.GetBoneContainer();
        for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())
        {
            const FString N=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(I).GetInt()).ToString().Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "));
            FRotator R=FRotator::ZeroRotator;
            if(N==TEXT("Bip01 Head"))R=FRotator(3+Scare*5,Peek*14,FMath::Sin(Clock*2.1f)*2+8+Scare*4);
            else if(N==TEXT("Bip01 Spine1"))R=FRotator(0,0,5+Scare*13);
            else if(N.Contains(TEXT("Finger")))R=FRotator(0,0,(N.Contains(TEXT(" L "))?-1:1)*(12+Scare*18));
            if(!R.IsNearlyZero()){FTransform T=Output.Pose[I];T.SetRotation((T.GetRotation()*R.Quaternion()).GetNormalized());Output.Pose[I]=T;}
        }
        if(Scare>.01f)
        {
            TArray<FTransform> CS;CS.SetNum(Output.Pose.GetNumBones());
            auto Rebuild=[&](){for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex()){const auto Parent=Output.Pose.GetParentBoneIndex(I);CS[I.GetInt()]=Parent.GetInt()<0?Output.Pose[I]:Output.Pose[I]*CS[Parent.GetInt()];}};Rebuild();
            auto Index=[&](const FString& Name){for(FCompactPoseBoneIndex I:Output.Pose.ForEachBoneIndex())if(Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(I).GetInt()).ToString().Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "))==Name)return I;return FCompactPoseBoneIndex(INDEX_NONE);};
            auto Rotate=[&](FCompactPoseBoneIndex I,FVector From,FVector To){FTransform Desired=CS[I.GetInt()];Desired.SetRotation((FQuat::FindBetweenNormals(From.GetSafeNormal(),To.GetSafeNormal())*Desired.GetRotation()).GetNormalized());const auto Parent=Output.Pose.GetParentBoneIndex(I);Output.Pose[I]=Desired.GetRelativeTransform(CS[Parent.GetInt()]);Rebuild();};
            for(int32 Side=0;Side<2;++Side)
            {
                const FString Prefix=Side?TEXT("Bip01 L "):TEXT("Bip01 R ");const auto Upper=Index(Prefix+TEXT("UpperArm")),Lower=Index(Prefix+TEXT("Forearm")),Hand=Index(Prefix+TEXT("Hand"));if(Upper.GetInt()<0||Lower.GetInt()<0||Hand.GetInt()<0)continue;
                const FVector A=CS[Upper.GetInt()].GetLocation(),B=CS[Lower.GetInt()].GetLocation(),C=CS[Hand.GetInt()].GetLocation(),Target=Side?LeftTarget:RightTarget;
                const double L1=FVector::Dist(A,B),L2=FVector::Dist(B,C),Distance=FMath::Clamp(FVector::Dist(A,Target),1.,L1+L2-.5);const FVector Axis=(Target-A).GetSafeNormal();const FVector Bend=(FVector::DownVector-Axis*FVector::DotProduct(FVector::DownVector,Axis)).GetSafeNormal();const double Along=(L1*L1-L2*L2+Distance*Distance)/(2*Distance);const FVector Elbow=A+Axis*Along+Bend*FMath::Sqrt(FMath::Max(0.,L1*L1-Along*Along));Rotate(Upper,B-A,Elbow-A);Rotate(Lower,CS[Hand.GetInt()].GetLocation()-CS[Lower.GetInt()].GetLocation(),A+Axis*Distance-CS[Lower.GetInt()].GetLocation());
            }
        }
        return true;
    }
};
void UEWardNurseAnimation::NativeInitializeAnimation(){Super::NativeInitializeAnimation();Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/MenuNurse/A_NurseIdle"));Walk=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/MenuNurse/A_NurseWalk"));}
FAnimInstanceProxy* UEWardNurseAnimation::CreateAnimInstanceProxy(){return new FWardNurseProxy(this);}
void UEWardNurseAnimation::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}
