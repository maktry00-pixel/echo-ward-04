#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WardBekaVisual50.generated.h"

class USkeletalMeshComponent;
class UGroomComponent;

UCLASS()
class ECHOWARD_API AEWardBekaVisual50 : public AActor
{
    GENERATED_BODY()
public:
    AEWardBekaVisual50();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Face;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UGroomComponent> Hair;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UGroomComponent> Eyebrows;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UGroomComponent> Eyelashes;
private:
    float BlinkClock=2.1f;
    TArray<FName> BlinkShapes;
};
