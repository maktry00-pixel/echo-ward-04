#include "WardTitleScene.h"
#include "WardCampaign.h"
#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Misc/Paths.h"
void AEWardTitleScene::BuildIdleEvents()
{
    const FString Base=TEXT("/Game/Campaign/MenuEvents/");
    auto Mat=[&](const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*(Base+Name));};
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Campaign/Creatures/SK_Stalker"));
    FVector HeadRef(0,0,167),HandRef(80,0,145);
    if(Mesh)
    {
        const auto& Ref=Mesh->GetRefSkeleton();
        for(int32 I=0;I<Ref.GetNum();++I)
        {
            FString N=Ref.GetBoneName(I).ToString().ToLower();FTransform Rest=Ref.GetRefBonePose()[I];for(int32 P=Ref.GetParentIndex(I);P>=0;P=Ref.GetParentIndex(P))Rest*=Ref.GetRefBonePose()[P];
            if(N==TEXT("jaw")||N.EndsWith(TEXT("head"))){PeekBone=Ref.GetBoneName(I);HeadRef=Rest.GetLocation();}
            // HorrorGameMaker's authored wrist is named joint1211, rather than RightHand.
            if(N==TEXT("joint1211")||N.EndsWith(TEXT("righthand"))||N.EndsWith(TEXT("r hand"))||N.EndsWith(TEXT("r_hand"))){ReachBone=Ref.GetBoneName(I);HandRef=Rest.GetLocation();}
        }
    }
    HandDirection=HandRef.X>=0?1.f:-1.f;
    EventRigReady=!PeekBone.IsNone()&&!ReachBone.IsNone();
    auto Rig=MakeShared<FJsonObject>();Rig->SetStringField(TEXT("head_bone"),PeekBone.ToString());Rig->SetStringField(TEXT("hand_bone"),ReachBone.ToString());Rig->SetStringField(TEXT("head_reference"),HeadRef.ToString());Rig->SetStringField(TEXT("hand_reference"),HandRef.ToString());TArray<TSharedPtr<FJsonValue>> Names;if(Mesh)for(const auto& Info:Mesh->GetRefSkeleton().GetRefBoneInfo())Names.Add(MakeShared<FJsonValueString>(Info.Name.ToString()));Rig->SetArrayField(TEXT("bones"),Names);WardData::Write(FPaths::ProjectSavedDir()/TEXT("Prototype/menu_event_rig.json"),Rig);
    auto Cropped=[&](bool Hand)
    {
        auto* C=NewObject<USkeletalMeshComponent>(this);C->SetupAttachment(RootComponent);C->SetSkeletalMeshAsset(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetReceivesDecals(false);C->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;C->bEnableUpdateRateOptimizations=false;C->RegisterComponent();AddInstanceComponent(C);C->SetComponentTickEnabled(false);
        auto* M=UMaterialInstanceDynamic::Create(Mat(TEXT("M_MenuZombieCrop")),this);M->SetScalarParameterValue(TEXT("HandMode"),Hand?1:0);M->SetScalarParameterValue(TEXT("CutZ"),HeadRef.Z-4);M->SetScalarParameterValue(TEXT("CutX"),FMath::Abs(HandRef.X)-16);M->SetScalarParameterValue(TEXT("HandSign"),HandDirection);
        if(Mesh)for(int32 I=0;I<Mesh->GetMaterials().Num();++I)C->SetMaterial(I,M);
        C->SetRelativeScale3D(FVector(Hand?1.35f:1.25f));
        if(!Hand)C->PlayAnimation(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Campaign/Creatures/A_StalkerIdle")),true);
        C->RefreshBoneTransforms();C->SetVisibility(false);return C;
    };
    PeekingHead=Cropped(false);ReachingHand=Cropped(true);
    RollingBall=Part(TEXT("/Engine/BasicShapes/Sphere"),FVector(1335,360,12),FVector(.24),Mat(TEXT("M_MenuBall")));
    Forearm=Part(Base+TEXT("SM_MenuForearm"),FVector::ZeroVector,FVector(1),Mat(TEXT("M_MenuArm")));
    for(const TCHAR* Id:{TEXT("menu_roll"),TEXT("menu_drag")})if(auto* S=LoadObject<USoundBase>(nullptr,*(Base+TEXT("A_")+Id)))Mode->Effects.Add(FName(Id),S);
    auto* FaceLight=NewObject<UPointLightComponent>(this);FaceLight->SetupAttachment(RootComponent);FaceLight->SetIntensityUnits(ELightUnits::Lumens);FaceLight->SetRelativeLocation(FVector(1215,140,180));FaceLight->SetLightColor(FLinearColor(.38,.55,.49));FaceLight->SetIntensity(65);FaceLight->SetAttenuationRadius(380);FaceLight->SetSourceRadius(7);FaceLight->RegisterComponent();AddInstanceComponent(FaceLight);LightBase.Add(FaceLight,65);
    // Decals belong to the distant title set only. Campaign materials are untouched.
    auto Blood=[&](FVector At,FVector Size,FRotator R,bool Wall,int32 Seed)
    {
        auto* M=UMaterialInstanceDynamic::Create(Mat(TEXT("M_MenuBlood")),this);M->SetScalarParameterValue(TEXT("Seed"),Seed);M->SetScalarParameterValue(TEXT("Wall"),Wall?1:0);
        auto* D=NewObject<UDecalComponent>(this);D->SetupAttachment(RootComponent);D->SetDecalMaterial(M);D->DecalSize=Size;D->SetRelativeLocation(At);D->SetRelativeRotation(R);D->RegisterComponent();AddInstanceComponent(D);++BloodDecals;
    };
    for(int32 I=0;I<6;++I)Blood(FVector(650+I*275,35+FMath::Sin(I*2.1f)*105,1),FVector(7,65+I%3*10,115),FRotator(90,21+I*31,0),false,9+I*7);
    for(int32 I=0;I<4;++I)Blood(FVector(890+I*390,276,105+I%2*25),FVector(8,72,100),FRotator(0,-90,0),true,8+I*13);
    Blood(FVector(1520,-276,104),FVector(8,65,90),FRotator(0,90,0),true,31);
    HideIdleEvents();
}
void AEWardTitleScene::HideIdleEvents()
{
    for(auto* C:{static_cast<USceneComponent*>(PeekingHead.Get()),static_cast<USceneComponent*>(ReachingHand.Get()),static_cast<USceneComponent*>(RollingBall.Get()),static_cast<USceneComponent*>(Forearm.Get())})if(C)C->SetVisibility(false,true);
}
void AEWardTitleScene::AnimateIdleEvents(float Delta)
{
    IdleCycle=FMath::Fmod(Age,60.f);IdleLoopCount=FMath::FloorToInt(Age/60.f);
    if(Transitioning){HideIdleEvents();IdleBeat=TEXT("transition");PreviousIdleCycle=IdleCycle;return;}
    auto Ease=[](float X){return FMath::SmoothStep(0.f,1.f,FMath::Clamp(X,0.f,1.f));};
    auto Open=[&](float Start,float Opened,float Closing,float End){return Ease((IdleCycle-Start)/(Opened-Start))*(1-Ease((IdleCycle-Closing)/(End-Closing)));};
    const float HeadDoor=Open(1.8f,3.f,6.2f,7.3f),BallDoor=Open(9.3f,10.5f,21.6f,22.8f),NurseDoor=Open(24.f,25.2f,45.8f,47.f);
    IdleDoorOpen=FMath::Max(HeadDoor*.72f,FMath::Max(BallDoor,NurseDoor));if(SideHinge)SideHinge->SetRelativeRotation(FRotator(0,IdleDoorOpen*86,0));
    IdleEntranceOpen=Open(48.f,49.f,52.f,53.f);FarDoorOpen=0;
    for(int32 I=0;I<FarHinges.Num();++I)FarHinges[I]->SetRelativeRotation(FRotator(0,I==1?IdleEntranceOpen*17:0,0));
    IdleBeat=IdleCycle>=1.8f&&IdleCycle<7.3f?TEXT("head"):IdleCycle>=10.4f&&IdleCycle<13.4f?TEXT("ball_roll"):IdleCycle>=13.4f&&IdleCycle<18.4f?TEXT("ball_wait"):IdleCycle>=18.4f&&IdleCycle<19.8f?TEXT("hand_reach"):IdleCycle>=19.8f&&IdleCycle<21.6f?TEXT("ball_pull"):IdleCycle>=24&&IdleCycle<47?TEXT("nurse"):IdleCycle>=48&&IdleCycle<53?TEXT("entrance"):TEXT("quiet");
    const bool HeadVisible=IdleCycle>=2.6f&&IdleCycle<6.2f;PeekingHead->SetVisibility(HeadVisible);
    if(HeadVisible)
    {
        PeekingHead->TickAnimation(Delta,false);PeekingHead->RefreshBoneTransforms();const float Lean=Ease((IdleCycle-2.6f)/.9f)*(1-Ease((IdleCycle-5.5f)/.7f));
        const FVector Target(1340,FMath::Lerp(335.f,283.f,Lean),166);const float Heading=(Camera->GetActorLocation()-Origin-Target).Rotation().Yaw+FMath::Sin(IdleCycle*2)*9;
        const FQuat Q=FRotator(0,Heading-90,FMath::Sin(IdleCycle)*4).Quaternion();const FVector Bone=PeekingHead->GetSocketTransform(PeekBone,RTS_Component).GetLocation();PeekingHead->SetRelativeLocationAndRotation(Target-Q.RotateVector(Bone*1.25),Q);
    }
    const bool BallVisible=IdleCycle>=10.4f&&IdleCycle<21.6f;RollingBall->SetVisibility(BallVisible);
    const FVector Inside(1335,360,12),Rest(1345,110,12);FVector Ball=Inside;
    if(IdleCycle<13.4f){const float P=FMath::Clamp((IdleCycle-10.4f)/3.f,0.f,1.f),Roll=1-FMath::Pow(1-P,2);Ball=FMath::Lerp(Inside,Rest,Roll);Ball.X+=FMath::Sin(P*PI*2)*2;Ball.Z+=FMath::Abs(FMath::Sin(P*PI*6))*(1-P)*1.3f;}
    else if(IdleCycle<19.8f)Ball=Rest;
    else Ball=FMath::Lerp(Rest,Inside,Ease((IdleCycle-19.8f)/1.8f));
    RollingBall->SetRelativeLocation(Ball);RollingBall->SetRelativeRotation(FRotator(0,0,(Inside.Y-Ball.Y)/12*180/PI));BallStillSeconds=FMath::Clamp(IdleCycle-13.4f,0.f,5.f);
    const bool HandVisible=IdleCycle>=18.4f&&IdleCycle<21.6f;ReachingHand->SetVisibility(HandVisible);Forearm->SetVisibility(HandVisible);
    if(HandVisible)
    {
        const FVector Root(1348,335,25),Grab=Ball+FVector(0,14,11);const FVector Wrist=IdleCycle<19.8f?FMath::Lerp(Root,Grab,Ease((IdleCycle-18.4f)/1.4f)):Grab;
        const FQuat Q=FQuat::FindBetweenNormals(FVector(HandDirection,0,0),FVector(0,-1,-.25).GetSafeNormal());const FVector Bone=ReachingHand->GetSocketTransform(ReachBone,RTS_Component).GetLocation();ReachingHand->SetRelativeLocationAndRotation(Wrist-Q.RotateVector(Bone*1.35),Q);
        const FVector Direction=Wrist-Root;Forearm->SetRelativeLocationAndRotation(Root,Direction.Rotation());Forearm->SetRelativeScale3D(FVector(FMath::Max(.01,Direction.Size()/100),1,1));
    }
    // Crossings, rather than frame equality, keep one-shots stable at different frame rates.
    if(IdleCycle<PreviousIdleCycle)PreviousIdleCycle=0;
    auto Cue=[&](float At,const TCHAR* Id,float Volume){if(PreviousIdleCycle<=At&&IdleCycle>At)Sound(Id,Volume);};
    Cue(1.8f,TEXT("creak"),.12f);Cue(6.2f,TEXT("menu_click"),.12f);Cue(9.3f,TEXT("creak"),.12f);Cue(10.4f,TEXT("menu_roll"),.22f);Cue(18.4f,TEXT("creak"),.065f);Cue(19.8f,TEXT("menu_drag"),.22f);Cue(21.6f,TEXT("menu_click"),.12f);Cue(24.f,TEXT("creak"),.1f);Cue(45.8f,TEXT("menu_click"),.1f);Cue(48.f,TEXT("creak"),.12f);Cue(52.f,TEXT("menu_click"),.12f);Cue(2.5f,TEXT("MENU/peek"),.045f);Cue(24.4f,TEXT("MENU/nurse_breath"),.055f);PreviousIdleCycle=IdleCycle;
}
