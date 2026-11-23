#include "WardCampaign.h"
#include "Kismet/GameplayStatics.h"
using namespace WardData;
namespace {TArray<TSharedPtr<FJsonValue>> SupplyVector(FVector P){return {MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)};}}
AEWardCampaignObject* AEWardCampaignMode::SpawnSupply(const FString& Id,int32 Amount,FVector At,const FString& Name)
{
    const FString Key=Name.IsEmpty()?FString::Printf(TEXT("dropped_%d"),++Values.FindOrAdd(TEXT("drop_serial"))):Name;
    Json D=MakeShared<FJsonObject>(),T=MakeShared<FJsonObject>(),P=MakeShared<FJsonObject>();T->SetArrayField(TEXT("position"),SupplyVector(At));T->SetArrayField(TEXT("scale"),SupplyVector(FVector::OneVector));T->SetArrayField(TEXT("rotation"),{MakeShared<FJsonValueNumber>(0),MakeShared<FJsonValueNumber>(0),MakeShared<FJsonValueNumber>(0),MakeShared<FJsonValueNumber>(1)});
    D->SetStringField(TEXT("id"),Key);D->SetStringField(TEXT("kind"),TEXT("dropped"));D->SetObjectField(TEXT("transform"),T);D->SetArrayField(TEXT("stance"),SupplyVector(At+FVector(-75,0,10)));P->SetStringField(TEXT("supply"),Id);P->SetStringField(TEXT("caption"),ItemName(Id));P->SetNumberField(TEXT("amount"),Amount);D->SetObjectField(TEXT("properties"),P);
    TArray<Json> Rows;if(auto* Template=Visuals.Find(TEXT("loose_")+Id))for(Json R:*Template){Json Copy=MakeShared<FJsonObject>();Copy->Values=R->Values;Json Position=MakeShared<FJsonObject>();Position->Values=Object(R,TEXT("transform"))->Values;Position->SetArrayField(TEXT("position"),SupplyVector(Vector(Array(Position,TEXT("position")))+At));Copy->SetObjectField(TEXT("transform"),Position);Rows.Add(Copy);}Visuals.Add(Key,Rows);
    const FTransform TransformAt(At);auto* O=GetWorld()->SpawnActorDeferred<AEWardCampaignObject>(AEWardCampaignObject::StaticClass(),TransformAt);O->ObjectId=Key;O->ObjectKind=TEXT("dropped");O->Descriptor=WardData::Serialize(D);Objects.Add(Key,O);UGameplayStatics::FinishSpawningActor(O,TransformAt);return O;
}
void AEWardCampaignMode::Drop(const FString& Id)
{
    if(!Hero()||!TArray<FString>{TEXT("ammo"),TEXT("shells"),TEXT("medkit"),TEXT("battery"),TEXT("bottle"),TEXT("boards")}.Contains(Id))return;
    const int32 Have=Id==TEXT("ammo")?Hero()->Reserve:Items.FindRef(Id);if(Have<=0)return;
    const int32 Amount=Id==TEXT("ammo")?FMath::Min(24,Have):Id==TEXT("shells")?FMath::Min(8,Have):Id==TEXT("bottle")||Id==TEXT("boards")?Have:1;
    Consume(Id,Amount);const FVector At=Hero()->GetActorLocation();FHitResult Floor;FCollisionQueryParams Query(SCENE_QUERY_STAT(DroppedSupplyFloor),false,Hero());GetWorld()->LineTraceSingleByChannel(Floor,At,At-FVector(0,0,300),ECC_WorldStatic,Query);
    SpawnSupply(Id,Amount,Floor.bBlockingHit?Floor.ImpactPoint+FVector(0,0,3):At-FVector(0,0,90));Notify(TEXT("Припасы оставлены у ног. Их можно подобрать снова клавишей F."),4);
}
