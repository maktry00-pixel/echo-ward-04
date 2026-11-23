#include "WardCampaign.h"
#include "WardPistolPose.h"
#include "WardHeldGrip108.h"
#include "WardGameplay76.h"
#include "WardCombat63.h"
#include "WardBekaAnimation.h"
#include "WardBekaVisual50.h"
#include "WardBookWorld.h"
#include "WardContact98.h"
#include "WardHideout102.h"
#include "WardRescue104.h"
#include "WardGait111.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"

AEWardCampaignHero::AEWardCampaignHero()
{
    BekaVisual=CreateDefaultSubobject<UChildActorComponent>(TEXT("BekaAcceptedVisual"));
    BekaVisual->SetupAttachment(GetRootComponent());
    BekaVisual->SetRelativeLocation(FVector(0,0,-96));BekaVisual->SetRelativeRotation(FRotator(0,-90,0));
}

void AEWardCampaignHero::InitializeAcceptedHero50()
{
    GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    GetMesh()->SetHiddenInGame(true);GetMesh()->SetCastShadow(false);
    BekaVisual->SetChildActorClass(AEWardBekaVisual50::StaticClass());
    AActor* Visual=BekaVisual->GetChildActor();if(!Visual)return;
    if(!Visual->HasActorBegunPlay())Visual->DispatchBeginPlay();
    Visual->SetOwner(this);Visual->SetActorEnableCollision(false);
    // Channel 0 is the shoulder torch: at a few centimetres it burns out
    // the hands and casts a weapon silhouette over the corridor. The
    // avatar keeps channel 1, shared by the normal hospital lighting.
    TArray<USkeletalMeshComponent*> Skinned;Visual->GetComponents(Skinned);
    for(auto* Part:Skinned)
    {
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetReceivesDecals(false);
        Part->SetLightingChannels(false,true,false);
        if(Part->GetName().Equals(TEXT("Body"),ESearchCase::IgnoreCase))BekaBody=Part;
        if(Part->GetName().Equals(TEXT("Face"),ESearchCase::IgnoreCase)){BekaFace=Part;Part->SetOwnerNoSee(true);}
    }
    if(!BekaBody){UE_LOG(LogTemp,Error,TEXT("BEKA50: Runtime body component missing"));return;}
    // Use an ordinary game skin material for the exposed body. The authoring
    // skin includes wardrobe cutout/shrink controls intended for editor previews.
    if(auto* Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MetaHumans/BekaAccepted/Materials/M_RuntimeSkin51.M_RuntimeSkin51")))
    {
        auto* Skin=UMaterialInstanceDynamic::Create(Parent,BekaBody);
        Skin->SetTextureParameterValue(TEXT("Albedo"),LoadObject<UTexture>(nullptr,TEXT("/Game/MetaHumans/BekaAccepted/Textures/T_BodySkin4K.T_BodySkin4K")));
        Skin->SetTextureParameterValue(TEXT("Normal"),LoadObject<UTexture>(nullptr,TEXT("/Game/MetaHumans/BekaAccepted/Textures/T_Native_1.T_Native_1")));
        // Keep texture colour and small wrinkles under the warm practicals.
        // A matte finish avoids the hard highlight across the first-person palms.
        Skin->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.49f,.35f,.28f,1));
        Skin->SetScalarParameterValue(TEXT("Roughness"),.69f);
        if(auto* Asset=BekaBody->GetSkeletalMeshAsset())
        {
            const auto& Skeleton=Asset->GetRefSkeleton();TArray<FTransform> Ref=Skeleton.GetRefBonePose();
            for(int32 I=0;I<Ref.Num();++I)if(Skeleton.GetParentIndex(I)>=0)Ref[I]=Ref[I]*Ref[Skeleton.GetParentIndex(I)];
            for(const FString Side:{FString(TEXT("l")),FString(TEXT("r"))})
            {
                const int32 Hand=Skeleton.FindBoneIndex(FName(*(TEXT("hand_")+Side))),Index=Skeleton.FindBoneIndex(FName(*(TEXT("index_01_")+Side))),Pinky=Skeleton.FindBoneIndex(FName(*(TEXT("pinky_01_")+Side)));
                if(Hand>=0&&Index>=0&&Pinky>=0){const FVector At=Ref[Hand].GetLocation(),Direction=((Ref[Index].GetLocation()+Ref[Pinky].GetLocation())*.5-At).GetSafeNormal();Skin->SetVectorParameterValue(Side==TEXT("l")?TEXT("LeftWrist"):TEXT("RightWrist"),FLinearColor(At.X,At.Y,At.Z,1));Skin->SetVectorParameterValue(Side==TEXT("l")?TEXT("LeftDirection"):TEXT("RightDirection"),FLinearColor(Direction.X,Direction.Y,Direction.Z,1));}
            }
        }
        BekaBody->SetMaterial(0,Skin);
    }
    BekaBody->bUseAttachParentBound=false;BekaBody->bComponentUseFixedSkelBounds=false;BekaBody->SetBoundsScale(2.f);BekaBody->bEnableUpdateRateOptimizations=false;
    if(auto* BodyAsset=BekaBody->GetSkeletalMeshAsset())if(auto* Render=BodyAsset->GetResourceForRendering())
    {
        for(int32 L=0;L<Render->LODRenderData.Num();++L)for(int32 I=0;I<Render->LODRenderData[L].RenderSections.Num();++I)
        {
            auto& Section=Render->LODRenderData[L].RenderSections[I];
            BekaBody->ShowMaterialSection(Section.MaterialIndex,I,true,L);
        }
    }
    BekaBody->SetAnimInstanceClass(UEWardBekaAnimation::StaticClass());
    BekaBody->AddTickPrerequisiteComponent(GetMesh());
    BekaBody->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    // Native wardrobe clothing is replaced by the already accepted tailored outfit.
    for(auto* Part:Skinned)if(Part!=BekaBody&&Part!=BekaFace){Part->SetVisibility(false,true);Part->SetHiddenInGame(true,true);}
    TArray<UPrimitiveComponent*> Primitives;Visual->GetComponents(Primitives);
    for(auto* Part:Primitives)
    {
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const FString Type=Part->GetClass()->GetName();
        if(Type.Contains(TEXT("Groom"))){Part->SetOwnerNoSee(true);Part->bCastHiddenShadow=false;}
        if(Type.Contains(TEXT("ChaosCloth"))){Part->SetVisibility(false,true);Part->SetHiddenInGame(true,true);}
    }
    auto* Outfit=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Hero/PhotoTailoredV26/SK_BekaPhotoTailored.SK_BekaPhotoTailored"));
    if(Outfit)
    {
        BekaOutfit=NewObject<USkeletalMeshComponent>(Visual,TEXT("BekaAcceptedOutfit"));
        BekaOutfit->SetupAttachment(BekaBody);BekaOutfit->SetSkeletalMesh(Outfit);BekaOutfit->SetForcedLOD(1);BekaOutfit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BekaOutfit->bUseAttachParentBound=false;BekaOutfit->SetBoundsScale(2.f);BekaOutfit->SetLightingChannels(false,true,false);BekaOutfit->SetReceivesDecals(false);BekaOutfit->SetLeaderPoseComponent(BekaBody);BekaOutfit->RegisterComponent();Visual->AddInstanceComponent(BekaOutfit);
        for(int32 I=0;I<BekaOutfit->GetNumMaterials();++I)
        {
            const FString Path=FString::Printf(TEXT("/Game/MetaHumans/BekaAccepted/Materials/FirstPerson51/MI_Outfit51_%d.MI_Outfit51_%d"),I,I);
            if(auto* Parent=LoadObject<UMaterialInterface>(nullptr,*Path))
            {
                auto* Cloth=UMaterialInstanceDynamic::Create(Parent,BekaOutfit);
                for(FName Param:{FName(TEXT("LeftWrist")),FName(TEXT("RightWrist")),FName(TEXT("LeftDirection")),FName(TEXT("RightDirection"))}){FLinearColor Value;if(BekaBody->GetMaterial(0)->GetVectorParameterValue(FMaterialParameterInfo(Param),Value))Cloth->SetVectorParameterValue(Param,Value);}
                BekaOutfit->SetMaterial(I,Cloth);
            }
        }
    }
    // Elastic cuffs close the seam between independently skinned clothing and hands.
    if(BekaOutfit&&BekaBody->GetSkeletalMeshAsset())
    {
        const auto& Skeleton=BekaBody->GetSkeletalMeshAsset()->GetRefSkeleton();TArray<FTransform> Ref=Skeleton.GetRefBonePose();
        for(int32 I=0;I<Ref.Num();++I)if(Skeleton.GetParentIndex(I)>=0)Ref[I]=Ref[I]*Ref[Skeleton.GetParentIndex(I)];
        auto* CuffMaterial=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_CampaignPBR.M_CampaignPBR")),Visual);
        CuffMaterial->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.019f,.021f,.022f,1));
        CuffMaterial->SetScalarParameterValue(TEXT("Roughness"),.89f);
        CuffMaterial->SetScalarParameterValue(TEXT("Specular"),.24f);
        if(auto* Weave=LoadObject<UTexture>(nullptr,TEXT("/Game/Outfits/techwearOutfit/techwearOutfit/Common/Materials/Textures/Clothing/Micros/micro_weave_denim_N.micro_weave_denim_N")))
        {
            CuffMaterial->SetTextureParameterValue(TEXT("Normal"),Weave);
            CuffMaterial->SetScalarParameterValue(TEXT("NormalStrength"),.45f);
            CuffMaterial->SetVectorParameterValue(TEXT("Tiling"),FLinearColor(5.f,1.6f,1.f,0.f));
        }
        for(const FString Side:{FString(TEXT("l")),FString(TEXT("r"))})
        {
            const FName Socket(*(TEXT("hand_")+Side));const int32 Hand=Skeleton.FindBoneIndex(Socket),Lower=Skeleton.FindBoneIndex(FName(*(TEXT("lowerarm_")+Side)));if(Hand<0||Lower<0)continue;
            const FVector Axis=(Ref[Lower].GetLocation()-Ref[Hand].GetLocation()).GetSafeNormal();
            auto* Cuff=NewObject<UStaticMeshComponent>(Visual);Cuff->SetupAttachment(BekaBody,Socket);Cuff->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));Cuff->bDisallowNanite=true;
            Cuff->SetRelativeLocation(Ref[Hand].InverseTransformPosition(Ref[Hand].GetLocation()+Axis*1.15));Cuff->SetRelativeRotation(FRotationMatrix::MakeFromZ(Ref[Hand].InverseTransformVectorNoScale(Axis)).Rotator());Cuff->SetRelativeScale3D(FVector(.069,.069,.025));Cuff->SetMaterial(0,CuffMaterial);
            Cuff->ComponentTags.Add(TEXT("WristCuff51"));Cuff->SetCollisionEnabled(ECollisionEnabled::NoCollision);Cuff->SetCastShadow(false);Cuff->SetReceivesDecals(false);Cuff->SetLightingChannels(false,true,false);Cuff->RegisterComponent();Visual->AddInstanceComponent(Cuff);
        }
    }
    if(auto* PC=Cast<APlayerController>(Controller))if(PC->PlayerCameraManager){PC->PlayerCameraManager->ViewPitchMin=-89;PC->PlayerCameraManager->ViewPitchMax=80;}
    UpdateFirstPerson50();
    UE_LOG(LogTemp,Display,TEXT("BEKA50: accepted model installed, body=%s face=%s outfit=%s"),*GetNameSafe(BekaBody),*GetNameSafe(BekaFace),*GetNameSafe(BekaOutfit));
}

void AEWardCampaignHero::UpdateFirstPerson50()
{
    bUseControllerRotationYaw=true;GetCharacterMovement()->bOrientRotationToMovement=false;
    Arm->TargetArmLength=0;Arm->SocketOffset=FVector::ZeroVector;Arm->TargetOffset=FVector::ZeroVector;
    const float DownLean=FMath::Clamp((-FRotator::NormalizeAxis(GetControlRotation().Pitch)-35.f)/50.f,0.f,1.f)*10.f;
    const float Eye77=bIsCrouched?FMath::Min(44.f,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-10.f):68.f;
    // Follow the crouched head's forward lean, rather than looking through the collar.
    FVector EyeLocal77(8+DownLean,0,Eye77);
    if(bIsCrouched&&BekaBody)
    {
        const FVector HeadLocal=GetActorTransform().InverseTransformPosition(BekaBody->GetSocketLocation(TEXT("head")));
        EyeLocal77.X+=FMath::Clamp(float(HeadLocal.X),0.f,48.f);EyeLocal77.Y=FMath::Clamp(float(HeadLocal.Y),-14.f,14.f);
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(CrouchEye77),false,this);if(BekaVisual&&BekaVisual->GetChildActor())Q.AddIgnoredActor(BekaVisual->GetChildActor());
        const FVector From=GetActorTransform().TransformPosition(FVector(0,0,Eye77)),To=GetActorTransform().TransformPosition(EyeLocal77);
        if(GetWorld()->SweepSingleByChannel(Hit,From,To,FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(8),Q))EyeLocal77=GetActorTransform().InverseTransformPosition(Hit.Location);
    }
    FVector HideEye;FRotator HideCentre;float HideYaw=0;
    const auto* Game=WardCampaign(this);
    const bool Wounded=Game&&WardRescue104::Downed(*Game);
    if(Wounded)EyeLocal77=FVector(15,0,27-GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const bool HiddenInFurniture = WardHideout102::View(*this,HideEye,HideCentre,HideYaw);
    if (HiddenInFurniture)
    {
        EyeLocal77=GetActorTransform().InverseTransformPosition(HideEye);
        if (Controller)
        {
            const auto View=GetControlRotation();
            const float Yaw=HideCentre.Yaw+FMath::Clamp(FMath::FindDeltaAngleDegrees(HideCentre.Yaw,View.Yaw),-HideYaw,HideYaw);
            const float Pitch=FMath::Clamp(FRotator::NormalizeAxis(View.Pitch),HideYaw>20?-20.f:-10.f,HideYaw>20?28.f:10.f);
            Controller->SetControlRotation(FRotator(Pitch,Yaw,0));
        }
    }
    if (!HiddenInFurniture&&!Wounded)
    {
        const float Correction=WardGait111::EyeHeightCorrection(*this);
        if (FMath::Abs(Correction)>.15f)
        {
            const FVector From=GetActorTransform().TransformPosition(EyeLocal77);
            const FVector To=From+FVector(0,0,Correction);
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(WalkingEye),false,this);
            if (BekaVisual&&BekaVisual->GetChildActor()) Query.AddIgnoredActor(BekaVisual->GetChildActor());
            if (!GetWorld()->SweepSingleByChannel(Hit,From,To,FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(8),Query))
                EyeLocal77.Z+=Correction;
            else if (!Hit.bStartPenetrating)
                EyeLocal77=GetActorTransform().InverseTransformPosition(Hit.Location);
        }
    }
    Arm->SetRelativeLocation(EyeLocal77+(HiddenInFurniture?FVector::ZeroVector:ViewMotion52));Arm->bEnableCameraLag=false;Arm->bEnableCameraRotationLag=false;Arm->bDoCollisionTest=false;Arm->bUsePawnControlRotation=true;
    Camera->SetRelativeLocation(FVector::ZeroVector);Camera->SetRelativeRotation(FRotator::ZeroRotator);
    // Flush the former third-person socket before first-frame torch/camera queries.
    if(Arm->IsRegistered()&&FVector::DistSquared(Camera->GetComponentLocation(),GetActorLocation())>10000.f)Arm->TickComponent(0,LEVELTICK_All,nullptr);
    GetMesh()->SetHiddenInGame(true);
    if (BekaBody)
    {
        const auto* Mode = WardCampaign(this);
        const auto Contact = WardContact98::Pose(*this);
        const AActor* Prop = Mode ? (Mode->PhoneOpen76 ? Mode->PhoneActor76.Get() : Mode->PreviewActor51.Get()) : nullptr;
        const bool Gun = Mode && !Mode->PhoneOpen76 && !Prop && (WardGameplay76::Armed(*Mode) ||
            Mode->PreviewItem51==TEXT("weapon") || Mode->PreviewItem51==TEXT("shotgun"));
        const bool HasProp=Prop&&!Prop->IsHidden();
        const bool Support=HasProp&&!Mode->PhoneOpen76&&WardHeldGrip108::Preview(Prop).TwoHands;
        for (const FName Bone : {FName(TEXT("hand_l")), FName(TEXT("hand_r"))})
        {
            const bool Right = Bone == FName(TEXT("hand_r"));
            const bool Holding = Gun || (HasProp && (Right || Support));
            const bool Visible = !Dead && !Wounded && !HiddenInFurniture && (Contact.Reach > .035f ? Right || Contact.Grip == WardContact98::EGrip::Cart : Holding);
            if (BekaBody->IsBoneHiddenByName(Bone) == !Visible) continue;
            if (Visible) BekaBody->UnHideBoneByName(Bone);
            else BekaBody->HideBoneByName(Bone, PBO_None);
        }
    }
    if (Pistol) Pistol->SetHiddenInGame(Wounded || HiddenInFurniture || WardContact98::Pose(*this).Reach > .035f);
    if(BekaBody){BekaBody->SetOwnerNoSee(false);BekaBody->SetOnlyOwnerSee(false);BekaBody->SetVisibility(true,false);BekaBody->SetHiddenInGame(false,false);}
    if(BekaOutfit){BekaOutfit->SetOwnerNoSee(false);BekaOutfit->SetOnlyOwnerSee(false);BekaOutfit->SetVisibility(true,false);BekaOutfit->SetHiddenInGame(false,false);}
    if(BekaVisual)
    {
        BekaVisual->SetRelativeLocation(FVector(-4,0,-GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
        if(auto* Visual=BekaVisual->GetChildActor())
        {
            auto* Mode=WardCampaign(this);const bool Hidden=IsHidden()||(Mode&&(Mode->HidingBed||(Mode->BookWorld&&Mode->BookWorld->FurnitureHiding)));
            Visual->SetActorHiddenInGame(Wounded||Hidden);
        }
    }
}

void AEWardCampaignHero::SetActorHiddenInGame(bool Hidden)
{
    Super::SetActorHiddenInGame(Hidden);
    if(BekaVisual)if(auto* Visual=BekaVisual->GetChildActor())Visual->SetActorHiddenInGame(Hidden);
}

FTransform AEWardCampaignHero::FirstPersonWeaponPose50(float Aim,float Close) const
{
    auto* Mode=WardCampaign(this);const bool Long=Mode&&(Mode->PreviewItem51.IsEmpty()?Mode->Shotgun:Mode->PreviewItem51==TEXT("shotgun"));
    const FRotator View=GetControlRotation();const FQuat Rotation=View.Quaternion();
    FVector Local=WardPistolPose::Mount(Long,Aim);
    const float Reload=ReloadBlend();Local+=FVector(2,4,6)*Reload;
    const float Spring=Mode?float(Mode->NumberFlag(TEXT("combat63_kick"))):0;
    const float Trauma=Mode?float(Mode->NumberFlag(TEXT("combat63_trauma")))*Mode->Setting(TEXT("shake")):0;
    if(Mode&&!Long&&Mode->PreviewItem51.IsEmpty())
    {
        const auto Phase=WardPistolPose::Reload(Mode->ReloadTime);
        const float Sprint=WardPistolPose::SprintLowering(MotionWeight52);
        const float ClearSprint=Sprint*(1-Close);
        Local+=FVector(-5,5,-13)*ClearSprint+FVector(-12,5,-13)*Close;
        Local+=FVector(3,-4,3)*Phase.WristTurn;
        Local+=WeaponMotion52*(1-Aim*.85f)*(1-Reload*.8f)*(1+ClearSprint*1.3f);
        const float Kick=WeaponKick+Spring;
        Local+=FVector(-1.05f,0,-.24f)*Kick+FVector(0,0,-.7f)*Trauma;
        const float Tilt=-29.f*Close-24.f*ClearSprint+9.f*Phase.WristTurn;
        const FRotator Articulation(Tilt+Kick*1.7f, -9.f*Close+9.f*ClearSprint-5.f*Phase.WristTurn,
            6.f*Close+10.f*ClearSprint-19.f*Phase.WristTurn+Kick*.65f);
        const FQuat Pose=(Articulation+WeaponSway52*(1-Aim*.85f)).Quaternion();
        return FTransform((Rotation*Pose).GetNormalized(),Camera->GetComponentLocation()+Rotation.RotateVector(Local),FVector::OneVector);
    }
    Local.X-=Close*15+WeaponKick*.7f+Spring;Local.Z-=Trauma*.7f;Local.Z-=Close*11;
    const float Stride=WardPistolPose::SprintLowering(MotionWeight52)*(1-Close);
    if (Long) Local+=FVector(-3,4,-10)*Stride;
    Local+=WeaponMotion52*(1-Aim*.85f)*(1-Reload*.8f);
    const FQuat Recoil=(FRotator(WeaponKick*1.1f+Spring*.8f-Close*32+Reload*3-17*Stride,Trauma*.4f-Reload*12,Reload*-14+Trauma*.7f+5*Stride)+WeaponSway52*(1-Aim*.85f)).Quaternion();
    return FTransform((Rotation*Recoil).GetNormalized(),Camera->GetComponentLocation()+Rotation.RotateVector(Local),FVector::OneVector);
}
