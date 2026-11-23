#include "WardCampaign.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Serialization/JsonSerializer.h"

using namespace WardData;
namespace
{
    FString NodePath(Json D,const FString& Key){return String(Object(D,Key),TEXT("node"));}
}
bool AEWardCampaignObject::MovingPart(const FString& Path)const
{
    if(!MovingPath.IsEmpty()&&Path.StartsWith(MovingPath))return true;
    Json Storage=Object(Definition,TEXT("storage"));
    for(const TCHAR* Key:{TEXT("drawer_body"),TEXT("tray"),TEXT("preview")})
    {const FString Root=NodePath(Storage,Key);if(!Root.IsEmpty()&&Path.StartsWith(Root))return true;}
    return false;
}
bool AEWardCampaignObject::HasStorage()const{return Definition && (Definition->HasField(TEXT("storage"))||!NodePath(Object(Definition,TEXT("properties")),TEXT("drawer")).IsEmpty());}
void AEWardCampaignObject::BeginPlay()
{
    AActor::BeginPlay();SetActorTickEnabled(false);
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Descriptor),Definition);
    AEWardCampaignMode* GM=WardCampaign(this);if(!GM||!Definition)return;
    Json Props=Object(Definition,TEXT("properties")),Storage=Object(Definition,TEXT("storage"));
    const FTransform Author=Transform(Object(Definition,TEXT("transform")));
    MovingPath=NodePath(Props,ObjectKind==TEXT("door")?TEXT("hinge"):TEXT("drawer"));
    if(MovingPath.IsEmpty())MovingPath=NodePath(Storage,TEXT("drawer"));
    ContentsPath=NodePath(Props,TEXT("contents"));
    if(ContentsPath.IsEmpty())ContentsPath=NodePath(Storage,TEXT("preview"));
    if(ContentsPath.IsEmpty())ContentsPath=NodePath(Props,TEXT("content_visual"));
    const FString MovingKey=ObjectKind==TEXT("door")?TEXT("hinge_transform"):TEXT("drawer_transform");
    if(Definition->HasField(MovingKey))RestHinge=Transform(Object(Definition,MovingKey)).GetRelativeTransform(Author);
    else
    {
        RestHinge=FTransform::Identity;
        // Search-storage drawer is a mesh nested below imported furniture.
        if(const auto* Rows=GM->Visuals.Find(ObjectId))for(Json Row:*Rows)if(String(Row,TEXT("path"))==MovingPath){RestHinge=Transform(Object(Row,TEXT("transform"))).GetRelativeTransform(Author);break;}
    }
    Hinge->SetRelativeTransform(RestHinge);
    Stance=GetActorTransform().TransformPosition(Author.InverseTransformPosition(Vector(Array(Definition,TEXT("stance")))));
    if(const auto* Rows=GM->Visuals.Find(ObjectId))for(Json Row:*Rows)
    {
        UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath(String(Row,TEXT("mesh"))));if(!Mesh)continue;
        const FString Path=String(Row,TEXT("path"));
        UStaticMeshComponent* Part=NewObject<UStaticMeshComponent>(this);Part->SetupAttachment(MovingPart(Path)?Hinge.Get():Pivot.Get());
        // Interactive pieces are small and may use glass. Choose the legacy
        // renderer before assigning a shared Nanite-enabled room-shell mesh.
        Part->bDisallowNanite=true;
        Part->SetStaticMesh(Mesh);Part->SetMobility(EComponentMobility::Movable);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        int32 Index=0;for(const auto& M:Array(Row,TEXT("materials"))){auto* Material=LoadObject<UMaterialInterface>(nullptr,*MaterialPath(M->AsString()));Part->SetMaterial(Index++,Material);if(Material&&Material->GetBlendMode()!=BLEND_Opaque&&Material->GetBlendMode()!=BLEND_Masked)Part->bDisallowNanite=true;}
        FTransform At=Transform(Object(Row,TEXT("transform"))).GetRelativeTransform(Author)*GetActorTransform();
        Part->SetWorldTransform(At);Part->SetCastShadow(Boolean(Row,TEXT("shadow"),true));
        const bool Visible=Boolean(Row,TEXT("visible"),true);Part->SetVisibility(Visible);Part->RegisterComponent();
        Parts.Add(Part);PartPaths.Add(Path);InitialVisibility.Add(Visible);
    }
    if(const auto* Rows=GM->Colliders.Find(ObjectId))for(Json Row:*Rows)
    {
        // Interaction Areas are sensors, not walls.
        if((((int32)Number(Row,TEXT("layer")))&1)==0 && ObjectKind!=TEXT("door")&&ObjectKind!=TEXT("partition"))continue;
        UShapeComponent* Shape=nullptr;const FString Type=String(Row,TEXT("type")),Path=String(Row,TEXT("path"));
        if(Type==TEXT("BoxShape3D")){auto* B=NewObject<UBoxComponent>(this);FVector S=Vector(Array(Row,TEXT("size")));B->InitBoxExtent(FVector(S.Z,S.X,S.Y)*50);Shape=B;}
        else if(Type==TEXT("SphereShape3D")){auto* S=NewObject<USphereComponent>(this);S->InitSphereRadius(Number(Row,TEXT("radius"))*100);Shape=S;}
        else if(Type==TEXT("CapsuleShape3D")||Type==TEXT("CylinderShape3D")){auto* C=NewObject<UCapsuleComponent>(this);C->InitCapsuleSize(Number(Row,TEXT("radius"))*100,Number(Row,TEXT("height"))*50);Shape=C;}
        if(!Shape)continue;
        Shape->SetupAttachment(MovingPart(Path)?Hinge.Get():Pivot.Get());Shape->SetMobility(EComponentMobility::Movable);
        Shape->SetWorldTransform(Transform(Object(Row,TEXT("transform"))).GetRelativeTransform(Author)*GetActorTransform());
        Shape->SetCollisionProfileName(TEXT("BlockAll"));Shape->SetGenerateOverlapEvents(false);Shape->SetHiddenInGame(true);Shape->RegisterComponent();
        if(ObjectKind==TEXT("partition")&&(((int32)Number(Row,TEXT("layer")))&1)==0){Shape->SetCollisionResponseToAllChannels(ECR_Ignore);Shape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);}
        if(Boolean(Row,TEXT("disabled")))Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CollisionParts.Add(Shape);InitialCollision.Add(Shape->GetCollisionEnabled());
        if(MovingPart(Path))MovingCollision.Add(Shape);
    }
    // The snapshot exports initially open leaves in their open world pose.
    // Children were attached against that pose above; normalize only the closed
    // hinge basis now, otherwise SetDoor adds a second 90-degree rotation.
    if(ObjectKind==TEXT("door")&&Boolean(Props,TEXT("is_open"),Boolean(Props,TEXT("initial_open"),true)))RestHinge.SetRotation((RestHinge.GetRotation()*FQuat(FVector::UpVector,FMath::DegreesToRadians(-90.f))).GetNormalized());
    ResetState();
}
void AEWardCampaignObject::ResetState()
{
    SetActorTransform(Transform(Object(Definition,TEXT("transform"))),false,nullptr,ETeleportType::TeleportPhysics);
    Stance=Vector(Array(Definition,TEXT("stance")));SetActorHiddenInGame(false);SetActorEnableCollision(true);
    for(int32 I=0;I<CollisionParts.Num();++I)CollisionParts[I]->SetCollisionEnabled(InitialCollision[I]);
    Json Props=Object(Definition,TEXT("properties")),Storage=Object(Definition,TEXT("storage"));
    Opened=ObjectKind==TEXT("door")?Boolean(Props,TEXT("initial_open"),true):false;
    Locked=Boolean(Props,TEXT("initial_locked"));Broken=false;Used=false;Found=false;Work=0;Value=0;Searched=0;Selected=0;Barricade=0;NeedsRelease=false;Motion=0;
    Reward=FMath::Clamp((int32)Number(Storage,TEXT("reward"),2),0,2);
    Hinge->SetRelativeTransform(RestHinge);
    if(ObjectKind==TEXT("door"))SetDoor(Opened,Locked);else RefreshVisuals();
    SetActorTickEnabled(false);
}
void AEWardCampaignObject::RefreshVisuals()
{
    for(int32 I=0;I<Parts.Num();++I)
    {
        const bool Content=!ContentsPath.IsEmpty()&&PartPaths[I].StartsWith(ContentsPath);
        const bool Visible=Content?(HasStorage()?Found&&!Used:InitialVisibility[I]&&!Used):(InitialVisibility[I]||Opened&&HasStorage());
        Parts[I]->SetVisibility(!Broken&&Visible);
    }
    for(int32 I=0;I<CollisionParts.Num();++I){auto C=CollisionParts[I];C->SetCollisionEnabled(Broken||ObjectKind==TEXT("door")&&Opened&&MovingCollision.Contains(C)?ECollisionEnabled::NoCollision:InitialCollision[I].GetValue());}
}
void AEWardCampaignObject::SetContents(bool Visible)
{
    if(ContentsPath.IsEmpty())return;
    for(int32 I=0;I<Parts.Num();++I)if(PartPaths[I].StartsWith(ContentsPath))Parts[I]->SetVisibility(Visible&&!Used&&!Broken);
}
FVector AEWardCampaignObject::FocusPoint()const{return GetActorLocation()+FVector(0,0,ObjectKind==TEXT("partition")?0:ObjectKind==TEXT("door")?130:75);}
FString AEWardCampaignObject::Prompt(const AEWardCharacter*)const{const AEWardCampaignMode* GM=WardCampaign(this);return GM?GM->Prompt(const_cast<AEWardCampaignObject*>(this)):FString();}
void AEWardCampaignObject::Interact(AEWardCharacter*)
{
    AEWardCampaignMode* GM=WardCampaign(this);if(!GM)return;
    if(ObjectKind==TEXT("door")||ObjectKind==TEXT("bed")||ObjectKind==TEXT("save")||ObjectKind==TEXT("partition")){GM->Use(this);return;}
    if(Used){if(Opened){Opened=false;Motion=.4f;SetActorTickEnabled(true);}return;}
    if(HasStorage()&&!Opened)
    {
        FCollisionObjectQueryParams Query;Query.AddObjectTypesToQuery(ECC_Pawn);FCollisionQueryParams Params(SCENE_QUERY_STAT(CampaignDrawer),false,this);
        bool Blocked=false;const FVector Travel=Hinge->GetComponentTransform().TransformVectorNoScale(FVector(-37,0,0));
        for(auto C:MovingCollision)if(auto* Box=Cast<UBoxComponent>(C)){const FVector Extra=Box->GetComponentQuat().UnrotateVector(Travel*.5f).GetAbs();Blocked|=GetWorld()->OverlapAnyTestByObjectType(Box->GetComponentLocation()+Travel*.5f,Box->GetComponentQuat(),Query,FCollisionShape::MakeBox(Box->GetScaledBoxExtent()+Extra),Params);}
        if(Blocked){GM->Notify(TEXT("Отойди на шаг от ящика, чтобы открыть его."));return;}
        Opened=true;Motion=.45f;NeedsRelease=true;SetActorTickEnabled(true);RefreshVisuals();GM->Cue(TEXT("creak"),GetActorLocation());return;
    }
    if(Motion>0)return;
    if(Definition->HasField(TEXT("storage"))&&!Found){Duration=1.2f;return;}
    GM->Use(this);
}
void AEWardCampaignObject::Hold(AEWardCharacter*,float Delta)
{
    AEWardCampaignMode* GM=WardCampaign(this);if(!GM||Used||NeedsRelease||Motion>0)return;
    if(Definition->HasField(TEXT("storage"))&&!Found)
    {
        if(!Opened||(Searched&(1<<Selected)))return;Work=FMath::Min(1.f,Work+Delta/1.2f);
        if(Work>=1){Searched|=1<<Selected;Work=0;NeedsRelease=true;if(Selected==Reward){Found=true;SetContents(true);GM->Notify(TEXT("В глубине ящика что-то есть. Отпусти F, затем забери находку."));}else{Selected=(Selected+1)%3;GM->Notify(TEXT("Только пыль и старые бумаги. Проверь другое отделение: 1 / 2 / 3."));}}
        return;
    }
    if(Duration<=0)return;
    Work=FMath::Min(1.f,Work+Delta/Duration);
    if(Work>=1){Work=0;Duration=0;NeedsRelease=true;GM->Use(this,true);}
}
void AEWardCampaignObject::Cancel(){NeedsRelease=false;Work=0;Duration=0;}
bool AEWardCampaignObject::HingeBlocked()const
{
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_Pawn);FCollisionQueryParams Params(SCENE_QUERY_STAT(CampaignMovingLeaf),false,this);
    for(auto C:MovingCollision)
        if(const UBoxComponent* Box=Cast<UBoxComponent>(C))
            if(GetWorld()->OverlapAnyTestByObjectType(Box->GetComponentLocation(),Box->GetComponentQuat(),Objects,FCollisionShape::MakeBox(Box->GetScaledBoxExtent()),Params))return true;
    return false;
}
void AEWardCampaignObject::ToggleDoor()
{
    if(Locked||Broken||Barricade>0||Motion>0)return;
    if(Opened&&FVector::Dist2D(GetActorLocation(),WardCampaign(this)->Hero()->GetActorLocation())<145){WardCampaign(this)->Notify(TEXT("Отойди от створки, чтобы закрыть дверь."));return;}
    Opened=!Opened;Motion=.45f;SetActorTickEnabled(true);
    for(auto C:MovingCollision)C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WardCampaign(this)->Cue(TEXT("door"),GetActorLocation());
}
void AEWardCampaignObject::SetDoor(bool Open,bool Lock)
{
    Opened=Open;Locked=Lock;Motion=0;
    // Source doors use a +90 degree Unreal yaw when open.
    FTransform At=RestHinge;if(ObjectKind==TEXT("door"))At.SetRotation(RestHinge.GetRotation()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Opened?90.f:0.f)));else At.AddToTranslation(FVector(Opened?-37:0,0,0));
    Hinge->SetRelativeTransform(At);RefreshVisuals();
    if(HingeBlocked()){for(auto C:MovingCollision)C->SetCollisionEnabled(ECollisionEnabled::NoCollision);SetActorTickEnabled(true);}
}
void AEWardCampaignObject::Tick(float Delta)
{
    AActor::Tick(Delta);
    if(Motion>0)
    {
        Motion=FMath::Max(0.f,Motion-Delta);
        if(ObjectKind==TEXT("door")){const FQuat Target=RestHinge.GetRotation()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Opened?90.f:0.f));Hinge->SetRelativeRotation(FMath::RInterpConstantTo(Hinge->GetRelativeRotation(),Target.Rotator(),Delta,220));}
        else Hinge->SetRelativeLocation(FMath::VInterpConstantTo(Hinge->GetRelativeLocation(),RestHinge.GetLocation()+FVector(Opened?-37:0,0,0),Delta,100));
    }
    if(Motion<=0)
    {
        if(ObjectKind==TEXT("door")&&Opened){for(auto C:MovingCollision)C->SetCollisionEnabled(ECollisionEnabled::NoCollision);SetActorTickEnabled(false);return;}
        if(HingeBlocked())return;
        RefreshVisuals();
        SetActorTickEnabled(false);
    }
}
void AEWardCampaignObject::Break()
{
    if(Broken)return;Broken=true;Used=true;RefreshVisuals();
    TArray<UPrimitiveComponent*> Components;GetComponents(Components);for(auto* C:Components)C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(auto* G=WardCampaign(this))for(int32 I=0;I<8;++I)G->SpawnPhysics(TEXT("chip"),GetActorLocation()+FVector(0,(I-4)*12,100+(I%3)*18),-GetActorForwardVector()*150+FMath::VRand()*100,FVector(2,5,.4),.006f,Parts.Num()?Parts[0]->GetMaterial(0):nullptr);
    if(auto* GM=WardCampaign(this)){GM->Cue(TEXT("glass_tinkle"),GetActorLocation());GM->BroadcastNoise(GetActorLocation(),2200);if(ObjectId==TEXT("chapter_observation_glass"))GM->Wake(15);GM->UpdateBlocked();}
}
Json AEWardCampaignObject::Snapshot()const
{
    Json D=MakeShared<FJsonObject>();D->SetBoolField(TEXT("open"),Opened);D->SetBoolField(TEXT("locked"),Locked);D->SetBoolField(TEXT("used"),Used);D->SetBoolField(TEXT("found"),Found);D->SetBoolField(TEXT("broken"),Broken);
    D->SetNumberField(TEXT("value"),Value);D->SetNumberField(TEXT("selected"),Selected);D->SetNumberField(TEXT("searched"),Searched);D->SetNumberField(TEXT("barricade"),Barricade);
    if(ObjectKind==TEXT("dropped"))D->SetObjectField(TEXT("definition"),Definition);
    TArray<TSharedPtr<FJsonValue>> At;for(double V:{GetActorLocation().X,GetActorLocation().Y,GetActorLocation().Z})At.Add(MakeShared<FJsonValueNumber>(V));D->SetArrayField(TEXT("at"),At);return D;
}
void AEWardCampaignObject::Restore(Json D)
{
    Used=Boolean(D,TEXT("used"));Found=Boolean(D,TEXT("found"));Broken=Boolean(D,TEXT("broken"));Value=Number(D,TEXT("value"));Selected=FMath::Clamp((int32)Number(D,TEXT("selected")),0,2);Searched=FMath::Clamp((int32)Number(D,TEXT("searched")),0,7);Barricade=FMath::Clamp((int32)Number(D,TEXT("barricade")),0,3);
    if(Array(D,TEXT("at")).Num()==3){FVector Old=GetActorLocation(),At=Vector(Array(D,TEXT("at")));Stance+=At-Old;SetActorLocation(At);}
    SetDoor(Boolean(D,TEXT("open")),Boolean(D,TEXT("locked")));
    if(Broken){TArray<UPrimitiveComponent*> Components;GetComponents(Components);for(auto* C:Components)C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
}
