#include "WardMonster59.h"
#include "WardOnline80.h"
#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardEchoAssets.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/Paths.h"
using namespace WardData;
namespace WardMonster59
{
    Json Data(){static Json Root=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Monsters61/rules.json"));return Root;}
    Json Rule(const FString& Species){return Object(Object(Data(),TEXT("rules")),Species);}
    bool CameraDisabled(const AEWardHospitalMob& Mob)
    {
        if(Mob.Species!=TEXT("keeper")||!Mob.Runtime||!Mob.Runtime->Mode)return false;
        const float Z=Mob.GetActorLocation().Z;
        return (Z<-600&&Mob.Runtime->Mode->Flag(TEXT("monster_camera_B2_off59")))||
            (Z>600&&Mob.Runtime->Mode->Flag(TEXT("monster_camera_3F_off59")));
    }
    bool LinkedSignalDisabled(const AEWardHospitalMob& Mob)
    {
        if(Mob.Species!=TEXT("linked")||!Mob.Runtime||!Mob.Runtime->Mode||Mob.Runtime->Chapter()!=13)
            return false;
        // Each staff-wing group uses its nearest physical relay. Disconnecting
        // one relay leaves the other group, and the finale encounters, active.
        const TCHAR* Relay=nullptr;
        double Nearest=FMath::Square(3600.);
        for(const TCHAR* Id:{TEXT("LINK_01"),TEXT("LINK_02")})
        {
            const auto* Node=Mob.Runtime->Props.FindRef(Id);
            if(!Node)continue;
            const double Distance=FVector::DistSquared(Node->GetActorLocation(),Mob.Home);
            if(Distance<Nearest){Nearest=Distance;Relay=Id;}
        }
        return Relay&&Mob.Runtime->Mode->Completed.Contains(Relay);
    }
    bool ContactThreatAt(const AEWardHospitalMob& Mob,const FVector& At,float Margin)
    {
        // Mobile enemies can close distance. Stationary tissue cannot pursue
        // a participant who stands outside its authored contact reach.
        if(!Boolean(Mob.Rule59,TEXT("stationary")))return true;
        return FVector::DistSquared2D(Mob.GetActorLocation(),At)<=
            FMath::Square(float(Number(Mob.Rule59,TEXT("attack_reach_cm"),145))+Margin);
    }
    TArray<FVector> Path(AEWardHospitalMob& Mob,FVector From,FVector To)
    {
        auto& Story=*Mob.Runtime;
        const float Radius=Mob.GetCapsuleComponent()->GetScaledCapsuleRadius();
        if(Boolean(Mob.Rule59,TEXT("niche_pass")))return Story.FindPath(From,To,Radius);
        Story.RefreshNavigation();
        static TWeakObjectPtr<AEWardBookWorld> CachedWorld;static TSet<FIntVector> Excluded;
        if(CachedWorld.Get()!=Story.Mode->BookWorld.Get())
        {
        CachedWorld=Story.Mode->BookWorld;Excluded.Reset();
        for(const auto& Door:Story.Mode->BookWorld->Portals)if(Door.Role==TEXT("niche"))
        {
            const bool NS=Door.Side==TEXT("north")||Door.Side==TEXT("south");
            const FIntVector Center=Story.CellAt(Door.At+FVector(0,0,91));const int Along=FMath::CeilToInt((Door.Width/2+55)/50);
            for(int I=-Along;I<=Along;++I)for(int J=-1;J<=1;++J)Excluded.Add(Center+(NS?FIntVector(I,J,0):FIntVector(J,I,0)));
        }
        }
        auto Free=[&](FIntVector C){return !Excluded.Contains(C)&&Story.FloorCell(C,Radius);};
        auto Nearest=[&](FIntVector C){if(Free(C))return C;for(int R=1;R<5;++R)for(int X=-R;X<=R;++X)for(int Y=-R;Y<=R;++Y)if(Free(C+FIntVector(X,Y,0)))return C+FIntVector(X,Y,0);return C;};
        TArray<FVector> Result;FIntVector Start=Story.CellAt(From),Goal=Story.CellAt(To);if(Start.Z!=Goal.Z)return Result;
        Start=Nearest(Start);Goal=Nearest(Goal);if(!Free(Start)||!Free(Goal))return Result;
        TArray<FIntVector> Queue;TMap<FIntVector,FIntVector> Parent;Queue.Add(Start);Parent.Add(Start,Start);int Head=0;bool Found=false;
        while(Head<Queue.Num()&&Head<9000)
        {
            const FIntVector C=Queue[Head++];if(C==Goal){Found=true;break;}
            for(FIntVector D:{FIntVector(1,0,0),FIntVector(-1,0,0),FIntVector(0,1,0),FIntVector(0,-1,0)})
            {const FIntVector N=C+D;if(!Parent.Contains(N)&&FMath::Abs(N.X-Start.X)<=70&&FMath::Abs(N.Y-Start.Y)<=70&&Free(N)){Parent.Add(N,C);Queue.Add(N);}}
        }
        if(!Found)return Result;TArray<FIntVector> Reverse;for(FIntVector C=Goal;C!=Start;C=Parent[C])Reverse.Add(C);
        FIntVector Previous(0,0,0);for(int I=Reverse.Num()-1;I>=0;--I){const FIntVector Direction=I?Reverse[I-1]-Reverse[I]:FIntVector(0,0,0);if(Direction!=Previous||I==0)Result.Add(Story.WorldAt(Reverse[I]));Previous=Direction;}
        return Result;
    }
    FString ZoneState(const FString& Zone,int Chapter)
    {
        FString Result=TEXT("open");
        static TMap<FString,Json> Rooms=[](){TMap<FString,Json> Out;for(auto L:Array(WardBookMapData(),TEXT("locations")))for(auto V:Array(L->AsObject(),TEXT("rooms")))Out.Add(String(V->AsObject(),TEXT("id")),V->AsObject());return Out;}();
        for(auto S:Array(Rooms.FindRef(Zone),TEXT("states")))if(Number(S->AsObject(),TEXT("chapter"))<=Chapter)Result=String(S->AsObject(),TEXT("state"));
        if(Result==TEXT("sealed"))return Result;
        for(auto V:Array(Data(),TEXT("zone_evolution")))if(String(V->AsObject(),TEXT("zone"))==Zone)
            for(auto S:Array(V->AsObject(),TEXT("states")))if(Number(S->AsObject(),TEXT("chapter"))<=Chapter)Result=String(S->AsObject(),TEXT("state"));
        return Result;
    }
    void Refresh(AEWardHospitalGameplay& Story)
    {
        if(!Story.Mode||!Story.Mode->Hero())return;
        const auto Distance = [&Story](FVector At) { return WardOnline80::PartyDistanceSquared(*Story.Mode, At); };
        TMap<FString,AEWardHospitalMob*> Chosen;
        for(auto Mob:Story.Mobs)
        {
            if(!Mob||!Mob->Eligible59())continue;
            auto*& Candidate=Chosen.FindOrAdd(Mob->Guid59);
            if(!Candidate||Distance(Mob->Home)<Distance(Candidate->Home))Candidate=Mob;
        }
        // Keep a unique opponent at its current physical position while nearby.
        // Duplicate anchors are route options, never simultaneous named copies.
        for(auto Mob:Story.Mobs)
            if(Mob&&Mob->Active&&Mob->Eligible59()&&Distance(Mob->GetActorLocation())<FMath::Square(1800.f))Chosen.Add(Mob->Guid59,Mob);
        for(auto Mob:Story.Mobs)
        {
            if(!Mob)continue;
            const bool Was=Mob->Active;
            Mob->Active=Chosen.FindRef(Mob->Guid59)==Mob&&Mob->Eligible59();
            // Corpses remain visible only during their authored encounter.
            const bool Corpse=Mob->BodyPresent59&&Mob->Dead&&Story.Chapter()>=Mob->FirstChapter&&Story.Chapter()<=19;
            Mob->SetActorHiddenInGame(!Mob->Active&&!Corpse);
            Mob->SetActorEnableCollision(Mob->Active);
            if(!Mob->Active){Mob->GetCharacterMovement()->StopMovementImmediately();Mob->GetCharacterMovement()->DisableMovement();if(!Mob->Dead)Mob->ChangeState59(TEXT("dormant"));}
            else if(!Was){Mob->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Mob->ChangeState59(Mob->Stunned>0?TEXT("stunned"):TEXT("patrol"));}
        }
    }
    void AddControls(AEWardHospitalGameplay& Story)
    {
        for(auto V:Array(Data(),TEXT("controls")))
        {
            Json Row=V->AsObject();const FString Id=String(Row,TEXT("id")),Zone=String(Row,TEXT("zone"));
            if(Story.Props.Contains(Id))continue;
            const FWardBookRoom3D* Room=Story.Mode->BookWorld->Rooms.FindByPredicate([&](const auto& R){return R.Id==Zone;});
            if(!Room)continue;
            const FVector Point=Room->Inside+FVector(0,0,105);
            auto* Actor=Story.GetWorld()->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Actor);Actor->SetRootComponent(Root);Root->RegisterComponent();Actor->SetActorLocation(Point);
            auto* Visual=WardEchoAssets::Attach(Actor,Root,92,FBox(FVector(-20,-8,-35),FVector(20,8,35)));
            if(Visual)Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            auto* Use=NewObject<UBoxComponent>(Actor);Use->SetupAttachment(Root);Use->SetBoxExtent(FVector(22,10,37));Use->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Use->SetCollisionResponseToAllChannels(ECR_Ignore);Use->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Use->RegisterComponent();
            Json Definition=MakeShared<FJsonObject>();Definition->Values=Row->Values;Definition->SetStringField(TEXT("kind"),TEXT("monster_control59"));Definition->SetArrayField(TEXT("at"),{MakeShared<FJsonValueNumber>(Point.X/100),MakeShared<FJsonValueNumber>(Point.Y/100),MakeShared<FJsonValueNumber>(Point.Z/100)});
            Story.Props.Add(Id,Actor);Story.Definitions.Add(Id,Definition);Story.PropActors.Add(Actor);
        }
    }
    bool UseControl(AEWardHospitalGameplay& Story,const FString& Id)
    {
        const Json D=Story.Definitions.FindRef(Id);
        if(String(D,TEXT("kind"))!=TEXT("monster_control59"))return false;
        auto* A=Story.Props.FindRef(Id);
        if(!A||A->IsHidden()||Story.Chapter()<Number(D,TEXT("chapter"))||!Story.Mode->Hero()||FVector::Dist(A->GetActorLocation(),Story.Mode->Hero()->GetActorLocation())>210)return true;
        const FString Flag=String(D,TEXT("flag"));const bool Off=!Story.Mode->Flag(Flag);Story.Mode->Flag(Flag,Off);
        Story.Mode->Cue(TEXT("metal_hit"),A->GetActorLocation(),.25f);
        Story.Mode->Notify(Off?TEXT("Локальные камеры отключены. Медицинская линия работает."):TEXT("Локальный сегмент камер включён."),4);
        for(auto Mob:Story.Mobs)if(Mob&&Mob->Species==TEXT("keeper")&&
            ((Flag==TEXT("monster_camera_B2_off59")&&Mob->GetActorLocation().Z<-600)||
             (Flag==TEXT("monster_camera_3F_off59")&&Mob->GetActorLocation().Z>600)))
        {
            Mob->Memory=0;Mob->SearchLeft59=0;Mob->NPCTarget62.Empty();
            Mob->Windup59=0;Mob->AttackResolved59=true;Mob->SeenPlayer59=false;
            Mob->ChangeState59(TEXT("return"));
        }
        Story.Mode->AutoSave();
        return true;
    }
}
