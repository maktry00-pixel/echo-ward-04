#include "WardTitleScene.h"
#include "WardNurseAnimation.h"
#include "WardCampaign.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
using namespace WardData;
void AEWardTitleScene::BuildNurse()
{
    Nurse=NewObject<USkeletalMeshComponent>(this,TEXT("MenuNurse"));Nurse->SetupAttachment(RootComponent);
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Campaign/MenuNurse/SK_MenuNurseFace"));
    Nurse->SetSkeletalMesh(Mesh);Nurse->SetCollisionEnabled(ECollisionEnabled::NoCollision);Nurse->SetReceivesDecals(false);
    Nurse->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;Nurse->bEnableUpdateRateOptimizations=false;
    Nurse->RegisterComponent();AddInstanceComponent(Nurse);Nurse->SetAnimInstanceClass(UEWardNurseAnimation::StaticClass());Nurse->SetComponentTickEnabled(false);
    if(Mesh)
    {
        const auto& Ref=Mesh->GetRefSkeleton();
        for(int32 I=0;I<Ref.GetNum();++I)
        {
            const FString Name=Ref.GetBoneName(I).ToString().Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "));
            if(Name==TEXT("Bip01 Head")){NurseHeadBone=Ref.GetBoneName(I);FTransform Rest=Ref.GetRefBonePose()[I];for(int32 P=Ref.GetParentIndex(I);P>=0;P=Ref.GetParentIndex(P))Rest*=Ref.GetRefBonePose()[P];NurseHeadRest=Rest.GetRotation();}
            if(Name==TEXT("Bip01 R Hand")){NurseHandBone=Ref.GetBoneName(I);FTransform Rest=Ref.GetRefBonePose()[I];for(int32 P=Ref.GetParentIndex(I);P>=0;P=Ref.GetParentIndex(P))Rest*=Ref.GetRefBonePose()[P];NurseHandRest=Rest.GetRotation();}
        }
    }
    auto* Cloth=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/MenuNurse/M_NurseCloth"));
    NurseHeadParts.Add(Part(TEXT("/Game/Campaign/MenuNurse/SM_nurse_cap"),FVector::ZeroVector,FVector(1,1,.45),Cloth));
    NurseHeadParts.Add(Part(TEXT("/Game/Campaign/MenuNurse/SM_nurse_wrap"),FVector::ZeroVector,FVector(1),Cloth));
    auto* Dark=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,*MaterialPath(TEXT("4ae6673fcc8b19e7"))),this);Dark->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.006,.002,.001));Dark->SetScalarParameterValue(TEXT("Roughness"),.96);
    KnifeGrip=NewObject<USceneComponent>(this);KnifeGrip->SetupAttachment(RootComponent);KnifeGrip->RegisterComponent();AddInstanceComponent(KnifeGrip);
    FVector Anchor=FVector::ZeroVector,Tip=FVector(0,1,0);
    for(const auto& V:Array(Mode->Data,TEXT("instances"))){const Json Row=V->AsObject();const FString Path=String(Row,TEXT("path"));if(!Path.Contains(TEXT("fish_knife")))continue;const FTransform T=Transform(Object(Row,TEXT("transform")));if(auto* M=LoadObject<UStaticMesh>(nullptr,*MeshPath(String(Row,TEXT("mesh"))))){const FVector Center=T.TransformPosition(M->GetBounds().Origin);if(Path.Contains(TEXT("handle")))Anchor=Center;else Tip=Center;}}
    const FQuat Align=FQuat::FindBetweenNormals((Tip-Anchor).GetSafeNormal(),FVector(0,0,-1));
    for(const auto& V:Array(Mode->Data,TEXT("instances")))
    {
        const Json Row=V->AsObject();if(!String(Row,TEXT("path")).Contains(TEXT("fish_knife")))continue;const FTransform T=Transform(Object(Row,TEXT("transform")));const auto& Mats=Array(Row,TEXT("materials"));
        auto* P=Part(MeshPath(String(Row,TEXT("mesh"))),FVector::ZeroVector,T.GetScale3D(),Mats.Num()?LoadObject<UMaterialInterface>(nullptr,*MaterialPath(Mats[0]->AsString())):Dark);
        P->AttachToComponent(KnifeGrip,FAttachmentTransformRules::KeepRelativeTransform);P->SetRelativeLocation(Align.RotateVector(T.GetLocation()-Anchor));P->SetRelativeRotation(Align*T.GetRotation());if(!NurseKnife)NurseKnife=P;
    }
    ScareLight=NewObject<USpotLightComponent>(this);ScareLight->SetupAttachment(RootComponent);ScareLight->SetIntensityUnits(ELightUnits::Lumens);ScareLight->SetLightColor(FLinearColor(.78,.9,.8));ScareLight->SetIntensity(0);ScareLight->SetAttenuationRadius(350);ScareLight->SetInnerConeAngle(30);ScareLight->SetOuterConeAngle(65);ScareLight->SetCastShadows(true);ScareLight->RegisterComponent();AddInstanceComponent(ScareLight);
}
void AEWardTitleScene::AnimateNurse(float Delta)
{
    if(!Nurse)return;
    const bool Entrance=Transitioning&&Pending==TEXT("page_game");const bool Scare=Entrance&&TransitionAge>=5.65f&&TransitionAge<7.1f;
    const float Cycle=FMath::Fmod(Age,60.f)-22.8f;
    FVector Local(1345,405,0);float Heading=-90;bool Visible=!Transitioning&&Cycle>=2&&Cycle<23;
    NurseMoving=0;NursePeek=0;NurseScare=Scare?1:0;
    auto Move=[&](FVector A,FVector B,float P){Local=FMath::Lerp(A,B,FMath::Clamp(P,0.f,1.f));Heading=(B-A).Rotation().Yaw;NurseMoving=1;};
    if(Cycle<6)Move(FVector(1345,405,0),FVector(1345,60,0),(Cycle-2)/4);
    else if(Cycle<10){Local=FVector(1345,60,0);Heading=-150;NursePeek=FMath::Sin((Cycle-7)*.8f);}
    else if(Cycle<14.5)Move(FVector(1345,60,0),FVector(1610,20,0),(Cycle-10)/4.5f);
    else if(Cycle<18.5)Move(FVector(1610,20,0),FVector(1345,60,0),(Cycle-14.5f)/4);
    else Move(FVector(1345,60,0),FVector(1345,405,0),(Cycle-18.5f)/4.5f);
    NurseInCorridor=Visible&&Local.Y<285;
    if(Scare)
    {
        const float Lunge=FMath::Clamp((TransitionAge-5.65f)/.18f,0.f,1.f);const FVector Cam=Camera->GetActorLocation()-Origin;
        Local=Cam+FVector(FMath::Lerp(125.f,61.f,Lunge),0,-161);Heading=180;Visible=true;NurseMoving=0;NursePeek=.4;
        const float Shake=FMath::Exp(-(TransitionAge-5.65f)*3);Camera->AddActorWorldOffset(FVector(0,FMath::Sin(TransitionAge*87)*1.1f*Shake,FMath::Sin(TransitionAge*63)*.8f*Shake));
        ScareLight->SetWorldLocationAndRotation(Camera->GetActorLocation()+FVector(0,22,-11),FRotator(2,0,0));ScareLight->SetIntensity(24*(1-Fade));
    }
    else ScareLight->SetIntensity(0);
    Nurse->SetVisibility(Visible,true);for(auto P:NurseHeadParts)P->SetVisibility(Visible);if(KnifeGrip)KnifeGrip->SetVisibility(Visible&&!Scare,true);
    NurseSpeed=NurseMoving>.5f?FMath::Clamp(float(FVector::Dist2D(Nurse->GetRelativeLocation(),Local)/FMath::Max(.001f,Delta)),0.f,120.f):0;
    Nurse->SetRelativeLocation(Local);Nurse->SetRelativeRotation(FRotator(0,Heading-90,0));
    if(!Visible)return;
    Nurse->SetMorphTarget(TEXT("AU_26_JawDrop"),.10f+NurseScare*.80f);Nurse->SetMorphTarget(TEXT("AU_25_LipsPart"),.12f+NurseScare*.53f);Nurse->SetMorphTarget(TEXT("AU_27_MouthStretch"),NurseScare*.38f);Nurse->SetMorphTarget(TEXT("AU_09_NoseWrinkler"),.18f+NurseScare*.45f);Nurse->SetMorphTarget(TEXT("AU_04_BrowLowerer"),.4f+NurseScare*.25f);
    // The world is paused while menus are open: sample and refresh explicitly once per real frame.
    Nurse->TickAnimation(Delta,false);Nurse->RefreshBoneTransforms();Nurse->UpdateComponentToWorld();
    const FTransform Head=Nurse->GetSocketTransform(NurseHeadBone);const FQuat Basis=(Head.GetRotation()*NurseHeadRest.Inverse()*FQuat(FVector::UpVector,PI*.5f)).GetNormalized();
    // Fit to the animated eye line, not the imported head pivot (which is at the jaw).
    const FVector Eyes=(Nurse->GetSocketLocation(TEXT("Bip01-REye"))+Nurse->GetSocketLocation(TEXT("Bip01-LEye")))*.5;
    const FVector Center=Eyes-Basis.RotateVector(FVector(7.4,0,0));
    for(int32 I=0;I<NurseHeadParts.Num();++I)
    {
        const FVector Offset=I==0?FVector(0,0,5.3):FVector(0,0,-3.5);
        NurseHeadParts[I]->SetWorldLocationAndRotation(Center+Basis.RotateVector(Offset),Basis);
    }
    if(KnifeGrip){const FTransform Hand=Nurse->GetSocketTransform(NurseHandBone);KnifeGrip->SetWorldLocationAndRotation(Hand.GetLocation(),(Hand.GetRotation()*NurseHandRest.Inverse()*FQuat(FVector::UpVector,PI*.5f)).GetNormalized());}
    LastFootstep-=Delta;if(NurseMoving>.5f&&LastFootstep<=0&&!Entrance){Sound(TEXT("menu_footstep"),.08f);LastFootstep=.78f;}
}
