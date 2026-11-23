#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Components/ShapeComponent.h"
using namespace WardData;

void AEWardCampaignMode::UpdateBlocked()
{
    Grid.Blocked.Reset();for(const auto& E:Objects)
    {
        auto* O=E.Value.Get();if(O->ObjectKind!=TEXT("door")||O->Opened||O->Broken||!O->Locked&&O->Barricade<=0)continue;
        // The quarantine shutter spans six metres. Block its actual closed leaf,
        // not a small circle at the hinge, or A* routes through the shutter edges.
        for(auto Shape:O->MovingCollision)
        {
            const FBox Bounds=Shape->Bounds.GetBox();const FIntPoint Min=Grid.Cell(Bounds.Min-FVector(55,55,0)),Max=Grid.Cell(Bounds.Max+FVector(55,55,0));
            for(int32 X=Min.X;X<=Max.X;++X)for(int32 Y=Min.Y;Y<=Max.Y;++Y)
            {
                const FIntPoint C(X,Y);const float* Ground=Grid.Ground.Find(C);if(!Ground)continue;
                if(X*100>=Bounds.Min.X-50&&X*100<=Bounds.Max.X+50&&Y*100>=Bounds.Min.Y-50&&Y*100<=Bounds.Max.Y+50&&*Ground+170>Bounds.Min.Z&&*Ground+10<Bounds.Max.Z)Grid.Blocked.Add(C);
            }
        }
    }
    for(const FString& Id:TArray<FString>{TEXT("ammo_morgue"),TEXT("ammo_lab"),TEXT("ammo_theatre"),TEXT("field_medkit_1"),TEXT("field_medkit_2")})
    {
        auto* Item=Objects.Find(Id);if(!Item)continue;TArray<UShapeComponent*> Shapes;(*Item)->GetComponents(Shapes);
        for(auto* Shape:Shapes)if(Shape->GetCollisionEnabled()!=ECollisionEnabled::NoCollision&&Shape->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Block)
        {
            const FBox Bounds=Shape->Bounds.GetBox();const FIntPoint Min=Grid.Cell(Bounds.Min-FVector(70,70,0)),Max=Grid.Cell(Bounds.Max+FVector(70,70,0));
            for(int32 X=Min.X;X<=Max.X;++X)for(int32 Y=Min.Y;Y<=Max.Y;++Y){FIntPoint C(X,Y);const float* Ground=Grid.Ground.Find(C);if(!Ground)continue;const FVector P(X*100,Y*100,*Ground+70);if(P.X>=Bounds.Min.X-52&&P.X<=Bounds.Max.X+52&&P.Y>=Bounds.Min.Y-52&&P.Y<=Bounds.Max.Y+52&&*Ground+170>Bounds.Min.Z&&*Ground+10<Bounds.Max.Z)Grid.Blocked.Add(C);}
        }
    }
}
void AEWardCampaignMode::AllocateResources()
{
    // Match the original safety/path/separation constraints, solving all five locations together.
    Grid.Blocked.Reset();TMap<FString,TArray<FVector>> Domains;FRandomStream Random(1907);
    for(const FString Department:{FString(TEXT("morgue")),FString(TEXT("lab")),FString(TEXT("theatre"))})
    {
        const double Center=Department==TEXT("morgue")?23:Department==TEXT("lab")?37:52;const FVector Entry(600,Center*100,106);TArray<FVector> Candidates;
        for(double Side:{-4.,4.})for(double Z:{-3.,3.,11.,18.})
        {
            const FVector P(-Z*100,(Center+Side)*100,0);if(!Grid.Ground.Contains(Grid.Cell(P)))continue;
            auto Path=Grid.FindPath(Entry,P+FVector(0,0,96));if(Path.Num()<6||Path.Num()>36)continue;
            bool Safe=true;for(auto Enemy:Enemies)if(FVector::Dist(P,Enemy->GetActorLocation()-FVector(0,0,96))<500){Safe=false;break;}if(Safe)Candidates.Add(P);
        }
        for(int32 I=Candidates.Num()-1;I>0;--I)Candidates.Swap(I,Random.RandRange(0,I));
        Domains.Add(TEXT("ammo_")+Department,Candidates);if(Department!=TEXT("lab"))Domains.Add(Department==TEXT("morgue")?TEXT("field_medkit_1"):TEXT("field_medkit_2"),Candidates);
    }
    TMap<FString,FVector> Result;if(Grid.Allocate(Domains,Result,300))
    {
        for(const auto& E:Result)if(auto* P=Objects.Find(E.Key)){const FVector Offset=E.Value-(*P)->GetActorLocation();(*P)->AddActorWorldOffset(Offset,false);(*P)->Stance+=Offset;}
        NumberFlag(TEXT("csp_assignments"),Grid.Assignments);Flag(TEXT("csp_solved"),true);
    }
    UpdateBlocked();
}
void AEWardCampaignMode::ToggleAdmin()
{
    auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC||!Hero())return;
    if(Admin){ClearPreview51();AdminHands51=false;}
    Admin=!Admin;if(Admin){Options->SetBoolField(TEXT("inspection_bright"),false);AdminViewRotation=Hero()->Camera->GetComponentRotation();AdminCamera=GetWorld()->SpawnActor<ACameraActor>(Hero()->Camera->GetComponentLocation(),AdminViewRotation);AdminCamera->GetCameraComponent()->SetFieldOfView(75);PC->SetViewTarget(AdminCamera);Hero()->StopInteract();}
    else {Agent=false;PC->SetViewTarget(Hero());if(AdminCamera)AdminCamera->Destroy();AdminCamera=nullptr;}
    if(!Admin){Options->SetBoolField(TEXT("inspection_bright"),false);ApplySettings(false);}
    Show(TEXT(""));Notify(Admin?TEXT("Камера администратора · F9 — агент · F10 — вернуться"):TEXT("Управление персонажем восстановлено"));
}
void AEWardCampaignMode::TickAdmin(float D)
{
    auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC||!AdminCamera||AdminHands51)return;const FRotator Rot=AdminViewRotation;AdminCamera->SetActorRotation(Rot);
    auto Axis=[&](FKey Positive,FKey Negative){return float(PC->IsInputKeyDown(Positive))-float(PC->IsInputKeyDown(Negative));};
    const FVector Move=Rot.Vector()*Axis(EKeys::W,EKeys::S)+FRotationMatrix(Rot).GetUnitAxis(EAxis::Y)*Axis(EKeys::D,EKeys::A)+FVector::UpVector*Axis(EKeys::SpaceBar,EKeys::LeftControl);
    AdminCamera->AddActorWorldOffset(Move*(PC->IsInputKeyDown(EKeys::LeftShift)?1800:650)*D,false);
}
void AEWardCampaignMode::ToggleAgent()
{
    if(BookWorldEnabled){Notify(TEXT("Это пустая карта для исследования. Управление — WASD."));return;}
    if(!Admin&&!Assessment)return;Agent=!Agent;if(Agent){AgentRoute.Reset();for(const auto& V:Array(Object(Data,TEXT("agent_routes")),StringFlag(TEXT("circuit"))==TEXT("dark")?TEXT("dark"):TEXT("light")))AgentRoute.Add(V->AsString());AgentIndex=0;AgentPath.Reset();AgentClock=0;}
    else if(Hero()){Hero()->GetCharacterMovement()->StopMovementImmediately();Hero()->Aiming=false;Hero()->StopInteract();}
    Notify(Agent?TEXT("Агент начал прохождение. Персонаж уязвим; при гибели загрузится последнее сохранение."):TEXT("Агент остановлен."));
}
void AEWardCampaignMode::TickAgent(float D)
{
    auto* H=Hero();if(!H||H->Dead||PendingRespawn||Won)return;const float Step=FMath::Min(.2f,D);
    if(!Screen.IsEmpty())
    {
        if(Screen==TEXT("keypad")){Interface->PendingCode=TEXT("417");Command(TEXT("keypad_submit"));}
        else if(Screen==TEXT("xray")){Interface->FilmOffset=FVector2D::ZeroVector;Interface->FilmTurn=0;Command(TEXT("xray_submit"));}
        else if(Screen==TEXT("treatment"))Command(FString(TEXT("treatment_"))+(Interface->Treatment.Num()==0?TEXT("cool"):Interface->Treatment.Num()==1?TEXT("dose"):TEXT("release")));
        else Show(TEXT(""));return;
    }
    if(H->Health<99.9f&&Items.FindRef(TEXT("medkit"))>0)Heal();
    AEWardCampaignEnemy* Threat=nullptr;float Nearest=1300;for(auto E:Enemies)if(E->Active&&!E->Dead){const float Distance=FVector::Dist(H->GetActorLocation(),E->GetActorLocation());if(Distance<Nearest){FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(AgentSight),false,H);if(!GetWorld()->LineTraceSingleByChannel(Hit,H->GetActorLocation()+FVector(0,0,35),E->GetActorLocation(),ECC_Visibility,Q)||Hit.GetActor()==E){Nearest=Distance;Threat=E;}}}
    if(Threat&&Flag(TEXT("weapon"))&&H->Rounds+H->Reserve+LoadedShells+Items.FindRef(TEXT("shells"))>0)
    {
        if(Threat->Guardian&&Threat->Exposed<=0&&Flag(TEXT("guardian_started")))
        {
            const FVector Away=(H->GetActorLocation()-Threat->GetActorLocation()).GetSafeNormal2D();FVector Dodge(-Away.Y,Away.X,0);if(H->GetActorLocation().Y>5200&&Dodge.Y>0||H->GetActorLocation().Y<5200&&Dodge.Y<0)Dodge=-Dodge;
            H->Aiming=false;H->SprintHeld=Threat->AttackTime>0||Threat->ChargeTime>0;
            if(H->SprintHeld){H->AddMovementInput(Dodge);if(DashCooldown<=0)Dash();}else if(Nearest<500)H->AddMovementInput(Away);
            if(H->Rounds<3&&H->Reserve>0){Shotgun=false;ReloadWeapon();}return;
        }
        H->SprintHeld=false;
        if(Nearest<350&&Threat->Stunned<=0&&FlashCharges>0)Flash();
        if(Flag(TEXT("shotgun"))){if(Nearest>600&&LoadedShells<2&&Items.FindRef(TEXT("shells"))>0){Shotgun=true;ReloadWeapon();return;}Shotgun=Nearest<550&&LoadedShells>0&&!Threat->Guardian;}
        H->Aiming=true;const FRotator Aim=(Threat->GetActorLocation()+FVector(0,0,Threat->Crawler?-35:30)-H->Camera->GetComponentLocation()).Rotation();if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(Aim);H->SetActorRotation(FRotator(0,Aim.Yaw,0));
        if(Shotgun&&LoadedShells==0||!Shotgun&&H->Rounds==0)ReloadWeapon();else FireWeapon();if(Nearest<200){H->AddMovementInput((H->GetActorLocation()-Threat->GetActorLocation()).GetSafeNormal2D());Knife();}return;
    }
    if(Threat&&Nearest<300&&H->Rounds+H->Reserve+LoadedShells+Items.FindRef(TEXT("shells"))==0)
    {
        // The agent uses the same last-resort knife and flash as a player.
        const FRotator Aim=(Threat->GetActorLocation()+FVector(0,0,Threat->Crawler?-35:20)-H->Camera->GetComponentLocation()).Rotation();if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(Aim);H->SetActorRotation(FRotator(0,Aim.Yaw,0));H->Aiming=false;
        if(Items.FindRef(TEXT("battery"))>0&&FlashCharges==0)Battery();if(FlashCharges>0&&Threat->Stunned<=0)Flash();
        if(Nearest<195)Knife();const FVector Toward=(Threat->GetActorLocation()-H->GetActorLocation()).GetSafeNormal2D();H->AddMovementInput(Nearest<125?-Toward:Toward);return;
    }
    H->Aiming=false;H->SprintHeld=false;if(AgentIndex>=AgentRoute.Num()){AgentStatus=TEXT("Маршрут завершён");return;}
    FString Id=AgentRoute[AgentIndex];AgentStatus=Id;if(Id==TEXT("chapter_glass"))Id=TEXT("chapter_observation_glass");
    auto Next=[&](){++AgentIndex;AgentPath.Reset();AgentWait=0;H->StopInteract();};
    if(Id.StartsWith(TEXT("dial_"))){TArray<FString> Parts;Id.ParseIntoArray(Parts,TEXT("_"));if(Parts.Num()==3&&Completed.Contains(TEXT("relay_")+Parts[1])){Next();return;}}
    if(Flag(TEXT("res_complete"))&&(Id.StartsWith(TEXT("residence_"))||Id.StartsWith(TEXT("res_")))){Next();return;}
    if(Id.StartsWith(TEXT("field_medkit_"))&&H->Health>=99.9f){Next();return;}
    if(Id==TEXT("upgrade_close")||Id==TEXT("upgrade_build")||Id==TEXT("upgrade_remove")||Id==TEXT("upgrade_open")||Id==TEXT("refuge_stash")){Next();return;}
    if(Id==TEXT("residence_east")||Id==TEXT("residence_west")){if(Values.FindRef(TEXT("res_mode"))==(Id.EndsWith(TEXT("east"))?2:1)){Next();return;}Id=TEXT("residence_panel");}
    FVector Target;if(Id==TEXT("exit"))Target=Vector(Array(Data,TEXT("exit")))+FVector(0,0,96);
    else if(auto* P=Objects.Find(Id))
    {
        auto* O=P->Get();if(O->Used||O->Broken){Next();return;}
        if(O->ObjectKind==TEXT("door")&&O->Opened){Next();return;}
        if(Id==TEXT("reactor")&&Flag(TEXT("reactor_started"))){if(Flag(TEXT("purge_complete"))){Next();return;}if(!Fault.IsEmpty()&&Objects.Contains(Fault))O=Objects[Fault];}
        if(Id==TEXT("isolation_generator")&&Flag(TEXT("generator_started"))){if(Flag(TEXT("generator_ready"))){Next();return;}if(!Fault.IsEmpty()&&Objects.Contains(Fault))O=Objects[Fault];}
        Target=O->Stance+FVector(0,0,96);
        if(Id==TEXT("residence_lock")){Target=O->GetActorLocation()+FVector(-100,0,0);Target.Z=Grid.Point(Grid.Nearest(Target)).Z;}
        if(Id==TEXT("chapter_observation_glass"))Target=FVector(3300,4280,106);
        if(Id==TEXT("isolation_glass"))Target=FVector(7500,4520,106);
        if(O->ObjectKind==TEXT("door"))
        {
            FVector Offset(0,-200,96);
            if(Id==TEXT("quarantine_gate")||Id==TEXT("diag_entry")||Id==TEXT("diag_23")||Id==TEXT("diag_37")||Id==TEXT("diag_52"))Offset=FVector(-200,0,96);
            if(Id==TEXT("technical_gate"))Offset=FVector(200,0,96);
            if(Id==TEXT("diag_office"))Offset=FVector(0,200,96);
            if(Id.StartsWith(TEXT("iso_"))||Id.StartsWith(TEXT("annex_")))Offset=O->GetActorTransform().TransformVectorNoScale(FVector(-200,0,0))+FVector(0,0,96);
            Target=O->GetActorLocation()+Offset;
        }
        if(FVector::Dist2D(H->GetActorLocation(),Target)<(O->HasStorage()?12:45)&&FMath::Abs(H->GetActorLocation().Z-Target.Z)<180)
        {
            FString Supply;int32 Amount=1;
            if(Id.Contains(TEXT("ammo"))){Supply=TEXT("ammo");Amount=Id.StartsWith(TEXT("annex_"))?12:DifficultyValue(TEXT("ammo"));}
            else if(Id.Contains(TEXT("shells"))){Supply=TEXT("shells");Amount=DifficultyValue(TEXT("shells"));}
            else if(O->ObjectKind==TEXT("medkit")||Id.Contains(TEXT("aid_")))Supply=TEXT("medkit");
            else if(O->ObjectKind==TEXT("battery"))Supply=TEXT("battery");
            if(!Supply.IsEmpty()&&!CanTake(Supply,Amount)&&Inventory().Num()<InventoryCapacity){Next();return;}
            if(Inventory().Num()>=InventoryCapacity&&!O->Used&&O->ObjectKind!=TEXT("door")&&O->ObjectKind!=TEXT("partition"))
            {
                if(Items.FindRef(TEXT("boards"))>0){Drop(TEXT("boards"));return;}if(Items.FindRef(TEXT("bottle"))>0){Drop(TEXT("bottle"));return;}
                if(Id.Contains(TEXT("ammo"))||Id.Contains(TEXT("shells"))||Id.Contains(TEXT("medkit"))||Id.Contains(TEXT("battery"))||Id.Contains(TEXT("aid_"))){Next();return;}
                if(H->Reserve>24){Drop(TEXT("ammo"));return;}
                if(Items.FindRef(TEXT("shells"))>0){Drop(TEXT("shells"));return;}
                if(Items.FindRef(TEXT("medkit"))>1){Drop(TEXT("medkit"));return;}
                if(Items.FindRef(TEXT("battery"))>0){Drop(TEXT("battery"));return;}
                if(H->Reserve>0){Drop(TEXT("ammo"));return;}
            }
            const FRotator Face=(O->FocusPoint()-H->Camera->GetComponentLocation()).Rotation();if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(Face);H->Focus=O;
            if(O->ObjectKind==TEXT("partition")){Knife();return;}
            if(O->ObjectId.StartsWith(TEXT("dial_"))){TArray<FString> A;O->ObjectId.ParseIntoArray(A,TEXT("_"));const auto& C=Array(Object(Data,TEXT("circuits")),A[1]);if(C.Num()==3&&O->Value==(int32)C[FCString::Atoi(*A[2])]->AsNumber()){Next();return;}}
            if(O->ObjectId.StartsWith(TEXT("annex_dial_"))){const int32 Desired=O->ObjectId.EndsWith(TEXT("a"))?2:O->ObjectId.EndsWith(TEXT("b"))?1:3;if(O->Value==Desired){Next();return;}}
            if(O->ObjectId==TEXT("annex_frequency")&&O->Value==2){Next();return;}
            if(O->Motion>0)return;
            if(O->HasStorage()&&!O->Opened){O->Interact(H);O->Cancel();return;}
            if(O->Definition->HasField(TEXT("storage"))&&!O->Found){O->Selected=O->Reward;O->Hold(H,Step);if(O->NeedsRelease)O->Cancel();return;}
            if(O->Duration>0){O->Hold(H,Step);if(O->NeedsRelease)O->Cancel();return;}
            if(O->ObjectId==TEXT("quarantine_gate")&&!Flag(TEXT("badge_examined")))Flag(TEXT("badge_examined"),true);
            O->Interact(H);if(O->NeedsRelease)O->Cancel();if(O->Used||O->Opened&&O->ObjectKind==TEXT("door"))Next();return;
        }
    }else {Next();return;}
    if((AgentWait-=Step)<=0||AgentPath.IsEmpty()){AgentWait=1;AgentPath=Grid.FindPath(H->GetActorLocation(),Target);}
    while(AgentPath.Num()>0&&FVector::Dist2D(H->GetActorLocation(),AgentPath[0])<40)AgentPath.RemoveAt(0);
    const FVector Heading=((AgentPath.Num()>0?AgentPath[0]:Target)-H->GetActorLocation()).GetSafeNormal2D();H->AddMovementInput(Heading);
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(AgentDoor),false,H);if(GetWorld()->LineTraceSingleByChannel(Hit,H->GetActorLocation(),H->GetActorLocation()+Heading*150,ECC_Visibility,Q))if(auto* Door=Cast<AEWardCampaignObject>(Hit.GetActor());Door&&Door->ObjectKind==TEXT("door")&&!Door->Opened&&!Door->Locked)Door->ToggleDoor();
    if(!Admin)if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(FRotator(-8,Heading.Rotation().Yaw,0));
}
