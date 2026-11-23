#include "WardBookWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
using namespace WardData;

Json AEWardBookWorld::AuditEveryDoor()
{
    // Exercise every actual leaf and instance, not a representative first door.
    // Restore the full architecture afterwards, including locks and refuge bolts.
    const Json Saved=ArchitectureSnapshot();TArray<TSharedPtr<FJsonValue>> Failures;
    int Doors=0,Panels=0,ClosedHits=0,Opened=0,Restored=0;
    for(auto& P:Portals)
    {
        if(P.Role==TEXT("niche"))continue;++Doors;bool Good=P.Leaf&&!P.VisibleParts.IsEmpty();
        TArray<FTransform> Closed;
        P.Open=false;RefreshPortalVisual(P);
        const FVector Normal=P.Side==TEXT("north")||P.Side==TEXT("south")?FVector(0,1,0):FVector(1,0,0);
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(EveryDoor),false);
        bool Blocks=P.Leaf&&P.Leaf->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics&&P.Leaf->LineTraceComponent(Hit,P.At-Normal*120+FVector(0,0,130),P.At+Normal*120+FVector(0,0,130),Query);
        if(Blocks)++ClosedHits;Good&=Blocks;
        for(const auto& Part:P.VisibleParts)
        {
            FTransform T;UInstancedStaticMeshComponent* C=DoorVisualGroups.FindRef(Part.Material);
            const bool Valid=C&&C->IsVisible()&&C->GetInstanceTransform(Part.Index,T,false)&&!T.ContainsNaN()&&T.GetScale3D().GetMin()>0;
            Good&=Valid;Closed.Add(T);++Panels;
        }
        P.Open=true;RefreshPortalVisual(P);bool Moved=P.Leaf&&P.Leaf->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
        for(int I=0;I<P.VisibleParts.Num();++I)
        {
            const auto& Part=P.VisibleParts[I];FTransform T;UInstancedStaticMeshComponent* C=DoorVisualGroups.FindRef(Part.Material);
            Moved&=C&&C->GetInstanceTransform(Part.Index,T,false)&&!T.ContainsNaN()&&(P.Type==TEXT("shutter")||!T.GetRotation().Equals(Closed[I].GetRotation(),.01))&&!T.GetLocation().Equals(Closed[I].GetLocation(),.1);
        }
        if(Moved)++Opened;Good&=Moved;
        if(!Good)Failures.Add(MakeShared<FJsonValueString>(P.Id));
    }
    RestoreArchitecture(Saved);const Json States=Object(Saved,TEXT("doors"));
    for(const auto& P:Portals)if(P.Role!=TEXT("niche")&&P.Open==Boolean(States,P.Id))++Restored;
    Json Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),Failures.IsEmpty()&&Doors>500&&Restored==Doors);
    Result->SetNumberField(TEXT("doors"),Doors);Result->SetNumberField(TEXT("visible_panels"),Panels);Result->SetNumberField(TEXT("closed_collision_passed"),ClosedHits);
    Result->SetNumberField(TEXT("open_pose_passed"),Opened);Result->SetNumberField(TEXT("state_restore_passed"),Restored);Result->SetArrayField(TEXT("failed_doors"),Failures);
    return Result;
}
