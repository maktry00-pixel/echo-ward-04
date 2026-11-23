#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "Engine/StaticMesh.h"
#include "WardEchoAssets.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
using namespace WardData;
void AEWardHospitalGameplay::BuildHeroVehicle31()
{
    VehicleCabin31=NewObject<UPointLightComponent>(ArrivalCar);VehicleCabin31->SetupAttachment(ArrivalCar->GetRootComponent());VehicleCabin31->SetRelativeLocation(FVector(-15,0,125));VehicleCabin31->SetIntensityUnits(ELightUnits::Lumens);VehicleCabin31->SetIntensity(12);VehicleCabin31->SetAttenuationRadius(155);VehicleCabin31->SetLightColor(FLinearColor(.73f,.78f,.67f));VehicleCabin31->SetCastShadows(false);VehicleCabin31->SetVisibility(false);VehicleCabin31->RegisterComponent();ArrivalCar->AddInstanceComponent(VehicleCabin31);
    const Json Hero=Object(WardEchoAssets::Cars31(),TEXT("hero"));
    for(auto V:Array(Hero,TEXT("hero_parts")))
    {
        const Json P=V->AsObject();auto* Mesh=WardEchoAssets::Mesh(Number(P,TEXT("model")));if(!Mesh)continue;
        auto* C=NewObject<UStaticMeshComponent>(ArrivalCar);C->SetupAttachment(ArrivalCar->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetRelativeLocation(Vector(Array(P,TEXT("pivot_cm"))));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);C->SetAffectDynamicIndirectLighting(false);C->SetAffectDistanceFieldLighting(false);C->SetLightingChannels(true,true,false);C->SetCullDistance(16000);C->RegisterComponent();ArrivalCar->AddInstanceComponent(C);const FString Name=String(P,TEXT("name"));VehicleParts31.Add(Name,C);if(Name.StartsWith(TEXT("Door"))||Name==TEXT("Hood")||Name==TEXT("Trunk")){VehicleOpen31.Add(Name,0);VehicleTarget31.Add(Name,0);}
    }
}
bool AEWardHospitalGameplay::VehicleExitClear31()const
{
    if(!Mode||!Mode->Hero())return false;auto* H=Mode->Hero();FCollisionQueryParams Q(SCENE_QUERY_STAT(Vehicle31Exit),false,H);Q.AddIgnoredActor(ArrivalCar);
    return !GetWorld()->OverlapBlockingTestByChannel(Vector(Array(ArrivalData,TEXT("exit")))*100,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(H->GetCapsuleComponent()->GetScaledCapsuleRadius(),H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),Q);
}
FString AEWardHospitalGameplay::NearestVehiclePart31()const
{
    if(!ArrivalCar||ArrivalStage<2||!Mode||!Mode->Hero()||Mode->Admin)return FString();const FVector At=Mode->Hero()->GetActorLocation();float Best=170;FString Name;
    for(const auto& P:VehicleTarget31)
    {
        const auto* C=VehicleParts31.FindRef(P.Key).Get();if(!C)continue;const float D=FVector::Dist(At,ArrivalCar->GetActorTransform().TransformPosition(C->GetRelativeLocation()+C->GetStaticMesh()->GetBoundingBox().GetCenter())+FVector(0,0,15));
        if(D<Best)
        {
            FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(Vehicle31PanelSight),false,Mode->Hero());Q.AddIgnoredActor(ArrivalCar);const FVector Target=ArrivalCar->GetActorTransform().TransformPosition(C->GetRelativeLocation()+C->GetStaticMesh()->GetBoundingBox().GetCenter())+FVector(0,0,15);
            if(GetWorld()->LineTraceSingleByChannel(Hit,At+FVector(0,0,35),Target,ECC_Visibility,Q))continue;
            Best=D;Name=P.Key;
        }
    }
    return Name;
}
FString AEWardHospitalGameplay::VehiclePrompt31()const
{
    if(Seated31)return TEXT("F — выйти из машины · мышь — осмотреть салон");const FString Part=NearestVehiclePart31();
    if(Part==TEXT("DoorFL"))return TEXT("F — сесть за руль · Y — открыть / закрыть дверь");
    if(Part==TEXT("Hood"))return TEXT("F / Y — открыть / закрыть капот");
    if(Part==TEXT("Trunk"))return TEXT("F / Y — открыть / закрыть багажник");
    return Part.IsEmpty()?FString():TEXT("F / Y — открыть / закрыть дверь");
}
void AEWardHospitalGameplay::ToggleVehiclePart31()
{
    if(Seated31||ArrivalStage<2||!Mode||!Mode->Screen.IsEmpty())return;const FString Name=NearestVehiclePart31();if(Name.IsEmpty())return;
    VehicleTarget31.Add(Name,VehicleTarget31.FindRef(Name)>.5f?0.f:1.f);VehicleAutoClose31=0;Mode->Cue(TEXT("INTRO/car_entry"),VehicleParts31[Name]->GetComponentLocation(),.18f);
}
bool AEWardHospitalGameplay::VehicleInteract31()
{
    auto* H=Mode->Hero();auto* PC=Cast<APlayerController>(H->Controller);
    if(Seated31)
    {
        if(!VehicleExitClear31()){Mode->Notify(TEXT("Выход из машины перекрыт."));return true;}
        Seated31=false;H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);VehicleTarget31.Add(TEXT("DoorFL"),1);VehicleAutoClose31=1.5f;H->SetActorHiddenInGame(false);H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);H->SetActorLocation(Vector(Array(ArrivalData,TEXT("exit")))*100,false,nullptr,ETeleportType::TeleportPhysics);if(PC)PC->SetViewTargetWithBlend(H,.45f);Mode->Cue(TEXT("INTRO/car_entry"),H->GetActorLocation(),.2f);return true;
    }
    const FString Part=NearestVehiclePart31();if(Part.IsEmpty())return false;
    if(Part!=TEXT("DoorFL")){ToggleVehiclePart31();return true;}
    Seated31=true;VehicleYaw31=0;VehiclePitch31=0;VehicleTarget31.Add(TEXT("DoorFL"),1);VehicleAutoClose31=1.1f;H->SetActorHiddenInGame(true);H->GetCharacterMovement()->StopMovementImmediately();H->GetCharacterMovement()->DisableMovement();if(PC)PC->SetViewTargetWithBlend(ArrivalCamera,.4f);TickVehicle31(0);
    // Commit the seat transition atomically before any input/audio callbacks.
    const FVector SeatEye=ArrivalCar->GetActorTransform().TransformPosition(Vector(Array(Object(WardEchoAssets::Cars31(),TEXT("hero")),TEXT("driver_eye_cm"))));
    H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    H->SetActorLocation(SeatEye-FVector(0,0,14),false,nullptr,ETeleportType::TeleportPhysics);
    UE_LOG(LogTemp,Display,TEXT("V31_SEAT_COMMITTED hero=%s at=%s collision=%d"),*H->GetName(),*H->GetActorLocation().ToString(),(int32)H->GetCapsuleComponent()->GetCollisionEnabled());
    Mode->Cue(TEXT("INTRO/car_entry"),H->GetActorLocation(),.2f);Mode->Notify(TEXT("F — выйти из машины. Мышь — осмотреть салон."),4);return true;
}
void AEWardHospitalGameplay::TickVehicle31(float D)
{
    if(!ArrivalCar||!Mode||!Mode->Hero())return;
    // Keep the real base geometry and full source mips around the hero vehicle.
    // Change residency only when crossing the range boundary, never every frame.
    const bool Close32=ArrivalStage>=0&&(Seated31||FVector::DistSquared(Mode->Hero()->GetActorLocation(),ArrivalCar->GetActorLocation())<FMath::Square(800.f));
    if(Close32!=VehicleCloseQuality32)
    {
        VehicleCloseQuality32=Close32;
        for(auto& Pair:VehicleParts31)if(auto* Part=Pair.Value.Get()){Part->SetForcedLodModel(Close32?1:0);Part->SetTextureForceResidentFlag(Close32);}
    }
    const FVector At=ArrivalCar->GetActorLocation();if(!VehicleLastAt31.IsZero())WheelRoll31-=FVector::Dist(At,VehicleLastAt31)/32.94f*180.f/PI;VehicleLastAt31=At;WheelRoll31=FMath::Fmod(WheelRoll31,360.f);
    if(VehicleAutoClose31>0){VehicleAutoClose31=FMath::Max(0.f,VehicleAutoClose31-D);if(VehicleAutoClose31==0)VehicleTarget31.Add(TEXT("DoorFL"),0);}
    for(auto V:Array(Object(WardEchoAssets::Cars31(),TEXT("hero")),TEXT("hero_parts")))
    {
        const Json P=V->AsObject();const FString Name=String(P,TEXT("name"));auto* C=VehicleParts31.FindRef(Name).Get();if(!C)continue;
        if(VehicleOpen31.Contains(Name))
        {float& Blend=VehicleOpen31[Name];Blend=FMath::FInterpConstantTo(Blend,VehicleTarget31.FindRef(Name),D,1.6f);const float Angle=Number(P,TEXT("open_degrees"))*Blend;C->SetRelativeRotation(String(P,TEXT("axis"))==TEXT("pitch")?FRotator(Angle,0,0):FRotator(0,Angle,0));}
        else if(Name.StartsWith(TEXT("Wheel")))C->SetRelativeRotation(FRotator(WheelRoll31,0,0));
        else if(Name==TEXT("SteeringWheel"))C->SetRelativeRotation(FRotator::ZeroRotator);
    }
    if(VehicleCabin31)VehicleCabin31->SetVisibility(Seated31||VehicleOpen31.FindRef(TEXT("DoorFL"))>.1f);
    if(Seated31&&ArrivalCamera)
    {
        auto* H=Mode->Hero();H->SetActorHiddenInGame(true);H->GetCharacterMovement()->DisableMovement();H->GetCharacterMovement()->StopMovementImmediately();
        const FVector Eye=ArrivalCar->GetActorTransform().TransformPosition(Vector(Array(Object(WardEchoAssets::Cars31(),TEXT("hero")),TEXT("driver_eye_cm"))));H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);H->SetActorLocation(Eye-FVector(0,0,14),false,nullptr,ETeleportType::TeleportPhysics);ArrivalCamera->SetActorLocationAndRotation(Eye,FRotator(VehiclePitch31,ArrivalCar->GetActorRotation().Yaw+VehicleYaw31,0));ArrivalCamera->GetCameraComponent()->SetFieldOfView(82);ArrivalCamera->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetViewTarget(ArrivalCamera);
    }
}
