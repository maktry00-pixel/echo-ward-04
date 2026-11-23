#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WardNurseAnimation.generated.h"
class UAnimSequence;
UCLASS(Transient)
class ECHOWARD_API UEWardNurseAnimation : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UAnimSequence> Idle;
    UPROPERTY() TObjectPtr<UAnimSequence> Walk;
    virtual void NativeInitializeAnimation()override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy()override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy)override;
};
