#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WardBekaAnimation.generated.h"

// The accepted MetaHuman follows the existing campaign motion/weapon driver.
// The source skeleton stays invisible and is never used as the player visual.
UCLASS(Transient)
class ECHOWARD_API UEWardBekaAnimation : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};

UCLASS(Transient)
class ECHOWARD_API UEWardBekaFaceAnimation : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
