#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WardPhysicsProp.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UPrimitiveComponent;

UCLASS()
class ECHOWARD_API AEWardPhysicsProp : public AActor
{
    GENERATED_BODY()
public:
    AEWardPhysicsProp();
    UPROPERTY() TObjectPtr<UBoxComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    FString Kind;
    float Age=0;bool Shattered=false;
    FVector PreviousVelocity=FVector::ZeroVector;
    void Configure(const FString& Type,FVector Size,float Mass,UMaterialInterface* Material=nullptr);
    virtual void Tick(float Delta)override;
    UFUNCTION() void Impact(UPrimitiveComponent* Self,AActor* Other,UPrimitiveComponent* OtherPart,FVector Impulse,const FHitResult& Hit);
};
