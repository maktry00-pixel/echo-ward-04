#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WardCampaignAnimation.generated.h"
class UAnimSequence;
UCLASS(Transient)
class ECHOWARD_API UEWardCampaignAnimation : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UAnimSequence> IdleClip;
    UPROPERTY() TObjectPtr<UAnimSequence> WalkClip;
    UPROPERTY() TObjectPtr<UAnimSequence> RunClip;
    UPROPERTY() TObjectPtr<UAnimSequence> CrouchClip;
    virtual void NativeInitializeAnimation()override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy()override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy)override;
};
