#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WardCampaignScene.generated.h"

UCLASS()
class ECHOWARD_API AEWardCampaignScene : public AActor
{
    GENERATED_BODY()
public:
    AEWardCampaignScene();
    UFUNCTION(BlueprintCallable,CallInEditor) void RebuildScene();
    UPROPERTY(VisibleAnywhere) int32 VisualInstances=0;
    UPROPERTY(VisibleAnywhere) int32 CollisionShapes=0;
};
