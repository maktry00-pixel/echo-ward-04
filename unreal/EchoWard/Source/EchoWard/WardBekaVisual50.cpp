#include "WardBekaVisual50.h"
#include "WardBekaAnimation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/MorphTarget.h"
#include "GroomComponent.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "Materials/MaterialInterface.h"

namespace
{
float BlinkWeight(float Clock)
{
    constexpr float Cycle=25.f;
    constexpr float BlinkTimes[]={1.6f,5.1f,11.7f,17.35f,17.61f,22.2f};
    const float Phase=FMath::Fmod(Clock,Cycle);
    for(float Start:BlinkTimes)
    {
        const float Local=Phase-Start;
        if(Local>=0.f&&Local<.20f)
            return Local<.065f?FMath::SmoothStep(0.f,.065f,Local):1.f-FMath::SmoothStep(.065f,.20f,Local);
    }
    return 0.f;
}
}

AEWardBekaVisual50::AEWardBekaVisual50()
{
    PrimaryActorTick.bCanEverTick=true;
    Body=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));SetRootComponent(Body);
    Face=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Face"));Face->SetupAttachment(Body);
    Hair=CreateDefaultSubobject<UGroomComponent>(TEXT("Hair"));Hair->SetupAttachment(Face);
    Eyebrows=CreateDefaultSubobject<UGroomComponent>(TEXT("Eyebrows"));Eyebrows->SetupAttachment(Face);
    Eyelashes=CreateDefaultSubobject<UGroomComponent>(TEXT("Eyelashes"));Eyelashes->SetupAttachment(Face);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEWardBekaVisual50::BeginPlay()
{
    Super::BeginPlay();SetActorEnableCollision(false);
    Body->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/MetaHumans/BekaAccepted/Body/SK_BekaBody.SK_BekaBody")));
    Face->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/MetaHumans/BekaAccepted/Face/SK_BekaFace.SK_BekaFace")));
    Body->SetForcedLOD(1);Face->SetForcedLOD(1);
    Face->SetAnimInstanceClass(UEWardBekaFaceAnimation::StaticClass());Face->AddTickPrerequisiteComponent(Body);
    Face->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    for(auto* Groom:{Hair.Get(),Eyebrows.Get(),Eyelashes.Get()})
    {
        const FString Name=Groom->GetName(),Root=TEXT("/Game/MetaHumans/BekaAccepted/Grooms/");
        Groom->SetGroomAsset(LoadObject<UGroomAsset>(nullptr,*(Root+TEXT("GR_")+Name+TEXT(".GR_")+Name)));
        Groom->SetBindingAsset(LoadObject<UGroomBindingAsset>(nullptr,*(Root+TEXT("GB_")+Name+TEXT(".GB_")+Name)));
        Groom->SetForcedLOD(0);Groom->SetOwnerNoSee(true);Groom->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const FString Material=TEXT("/Game/MetaHumans/BekaAccepted/Materials/MI_")+Name+TEXT("_");
        for(int32 I=0;I<Groom->GetNumMaterials();++I)
            if(auto* M=LoadObject<UMaterialInterface>(nullptr,*(Material+FString::FromInt(I)+TEXT(".MI_")+Name+TEXT("_")+FString::FromInt(I))))Groom->SetMaterial(I,M);
    }
    if(auto* Mesh=Face->GetSkeletalMeshAsset())for(const auto& Entry:Mesh->GetMorphTargets())
    {
        auto* Shape=Entry.Get();if(!Shape)continue;
        const FString Name=Shape->GetName();if(Name.EndsWith(TEXT("__eye_blink_L"))||Name.EndsWith(TEXT("__eye_blink_R")))BlinkShapes.Add(Shape->GetFName());
    }
}

void AEWardBekaVisual50::Tick(float Delta)
{
    Super::Tick(Delta);
    if(BlinkShapes.IsEmpty())return;
    const float Previous=BlinkWeight(BlinkClock);
    BlinkClock=FMath::Fmod(BlinkClock+Delta,25.f);
    const float Weight=BlinkWeight(BlinkClock);
    if(Weight==0.f&&Previous==0.f)return;
    for(FName Name:BlinkShapes)Face->SetMorphTarget(Name,Weight);
}
