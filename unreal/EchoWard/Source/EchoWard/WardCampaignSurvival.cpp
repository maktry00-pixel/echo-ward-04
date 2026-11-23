#include "WardCampaign.h"
#include "WardRescue104.h"
#include "WardPistolPose.h"
#include "WardGameplay76.h"
#include "WardOnline80.h"
#include "WardCombat63.h"
#include "WardMotion107.h"
#include "WardHospitalGameplay.h"
#include "WardPeople54.h"
#include "WardBookWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
using namespace WardData;

TArray<TPair<FString,int32>> AEWardCampaignMode::Inventory()const
{
    TArray<TPair<FString,int32>> Out;
    if(Flag(TEXT("weapon")))Out.Emplace(TEXT("weapon"),1);if(Flag(TEXT("shotgun")))Out.Emplace(TEXT("shotgun"),1);
    for(int32 N=Items.FindRef(TEXT("shells"));N>0;N-=8)Out.Emplace(TEXT("shells"),FMath::Min(8,N));
    if(Items.FindRef(TEXT("boards"))>0)Out.Emplace(TEXT("boards"),Items.FindRef(TEXT("boards")));
    for(int32 N=Hero()?Hero()->Reserve:0;N>0;N-=24)Out.Emplace(TEXT("ammo"),FMath::Min(24,N));
    if(Items.FindRef(TEXT("bottle"))>0)Out.Emplace(TEXT("bottle"),Items.FindRef(TEXT("bottle")));
    for(const FString& Id:TArray<FString>{TEXT("medkit"),TEXT("battery")})for(int32 N=0;N<Items.FindRef(Id);++N)Out.Emplace(Id,1);
    for(const FString& Id:TArray<FString>{TEXT("fuse"),TEXT("badge"),TEXT("fuel"),TEXT("coolant"),TEXT("residence_prybar"),TEXT("residence_core"),TEXT("isolation_coil"),TEXT("isolation_tag"),TEXT("annex_wheel")})if(Items.FindRef(Id)>0)Out.Emplace(Id,1);
    return Out;
}
bool AEWardCampaignMode::CanTake(const FString& Id,int32 Amount)const
{
    if(Amount<=0)return false;
    if(Id==TEXT("ammo")&&(!Hero()||Hero()->Reserve+Amount>72)||Id==TEXT("shells")&&Items.FindRef(Id)+Amount>18||(Id==TEXT("bottle")||Id==TEXT("boards"))&&Items.FindRef(Id)+Amount>3)return false;
    int32 Slots=Inventory().Num();
    if(Id==TEXT("ammo"))Slots+=FMath::DivideAndRoundUp(Hero()->Reserve+Amount,24)-FMath::DivideAndRoundUp(Hero()->Reserve,24);
    else if(Id==TEXT("shells"))Slots+=FMath::DivideAndRoundUp(Items.FindRef(Id)+Amount,8)-FMath::DivideAndRoundUp(Items.FindRef(Id),8);
    else if(Id==TEXT("medkit")||Id==TEXT("battery"))Slots+=Amount;
    else if(!Items.Contains(Id)||Items.FindRef(Id)<=0)++Slots;
    return Slots<=InventoryCapacity;
}
bool AEWardCampaignMode::Take(const FString& Id,int32 Amount)
{
    if(!CanTake(Id,Amount)){Notify(TEXT("Не хватает места или достигнут предел запаса. I — инвентарь."));return false;}
    if(Id==TEXT("ammo"))Hero()->Reserve+=Amount;else Items.FindOrAdd(Id)+=Amount;
    if(Id==TEXT("weapon")||Id==TEXT("shotgun"))Flag(Id,true);const FString PickupSound=Id==TEXT("weapon")||Id==TEXT("shotgun")?TEXT("PROP/weapon"):Id==TEXT("ammo")||Id==TEXT("shells")?TEXT("PROP/ammo"):Id==TEXT("medkit")?TEXT("PROP/medical"):Id==TEXT("battery")||Id==TEXT("fuse")?TEXT("PROP/battery"):Id.Contains(TEXT("key"))||Id==TEXT("badge")?TEXT("PROP/key"):TEXT("PROP/loot");Cue(PickupSound,Hero()->GetActorLocation(),.2f);return true;
}
void AEWardCampaignMode::Consume(const FString& Id,int32 Amount){if(Id==TEXT("ammo")){Hero()->Reserve=FMath::Max(0,Hero()->Reserve-Amount);return;}Items.FindOrAdd(Id)=FMath::Max(0,Items.FindRef(Id)-Amount);if(Items[Id]==0)Items.Remove(Id);}
void AEWardCampaignMode::Heal(){if(!WardOnline80::PersonalAction(*this,TEXT("heal")))WardCombat63::BeginHeal(*this);}
void AEWardCampaignMode::Battery(){if(Items.FindRef(TEXT("battery"))>0&&FlashCharges<3){Consume(TEXT("battery"));++FlashCharges;Cue(TEXT("PROP/battery"),Hero()->GetActorLocation(),.2f);}}
void AEWardCampaignMode::ReloadWeapon()
{
    if (WardOnline80::PersonalAction(*this, TEXT("reload"))) return;
    if(!WardGameplay76::Armed(*this)||!Hero()||Hero()->Dead||ReloadTime>0||WardCombat63::Healing(*this)||!Screen.IsEmpty()||!PreviewItem51.IsEmpty()||HidingBed||(BookWorld&&BookWorld->FurnitureHiding)||Admin&&!Agent)return;
    if(Shotgun){if(LoadedShells<5&&Items.FindRef(TEXT("shells"))>0){ReloadTime=.7f;Hero()->Aiming=false;ShotgunReload=true;Cue(TEXT("reload"),Hero()->GetActorLocation());}}
    else if(Hero()->Rounds<9&&Hero()->Reserve>0){ReloadTime=WardPistolPose::ReloadDuration;Hero()->Aiming=false;ShotgunReload=false;Cue(TEXT("reload"),Hero()->GetActorLocation());}
}
void AEWardCampaignMode::FireWeapon()
{
    if (WardOnline80::PersonalAction(*this, TEXT("fire"))) return;
    if(!WardGameplay76::Armed(*this)||!PreviewItem51.IsEmpty()||WardCombat63::Healing(*this))return;
    if(HospitalStory&&HospitalStory->ArrivalCinematic())return;
    auto* H=Hero();if(!H||H->Dead||!H->Aiming||ShotCooldown>0||HidingBed||(BookWorld&&BookWorld->FurnitureHiding)||Admin&&!Agent||!Screen.IsEmpty())return;
    if(!H->WeaponHasClearance()){H->WeaponObstructed=true;return;}
    if(ReloadTime>0){if(Shotgun&&LoadedShells>0)ReloadTime=0;else return;}
    if((Shotgun?LoadedShells:H->Rounds)<=0){ShotCooldown=.4f;Notify(TEXT("Магазин пуст · R — перезарядка"),2);Cue(TEXT("empty"),H->GetActorLocation());return;}
    if(Shotgun)--LoadedShells;else --H->Rounds;ShotCooldown=Shotgun?.85f:.32f;++Shots;
    H->MuzzleRemaining=.055f;
    H->WeaponKick=FMath::Min(7.f,H->WeaponKick+(Shotgun?4.f:1.8f));
    const FTransform Gun=H->WeaponPose();WardCombat63::Shot(*this);
    const FVector Eye=H->Camera->GetComponentLocation(),Forward=H->Camera->GetForwardVector();
    FHitResult Aim;FCollisionQueryParams Q(SCENE_QUERY_STAT(CampaignAim),true,H);GetWorld()->LineTraceSingleByChannel(Aim,Eye,Eye+Forward*6000,ECC_Visibility,Q);
    FVector End=Aim.bBlockingHit?Aim.ImpactPoint:Eye+Forward*6000;
    const FVector Muzzle=Gun.TransformPosition(FVector(Shotgun?76:22,0,0));
    WardOnline80::RecordShot(*this, Muzzle);
    const float Assist=DifficultyValue(TEXT("assist"));if(Assist>0)
    {
        float Best=FMath::Cos(Assist);for(auto E:Enemies)if(E->Active&&!E->Dead)
        {const FVector Point=E->GetActorLocation()+FVector(0,0,E->Crawler?-35:30);const float Dot=FVector::DotProduct((Point-Eye).GetSafeNormal(),Forward);if(Dot<=Best||FVector::Dist(Eye,Point)>2000)continue;FHitResult Visible;if(!GetWorld()->LineTraceSingleByChannel(Visible,Eye,Point,ECC_Visibility,Q)||Visible.GetActor()==E){Best=Dot;End=Point;}}
    }
    const int32 PreviousHits=Hits;for(int32 I=0;I<(Shotgun?7:1);++I){const FVector Direction=Shotgun?FMath::VRandCone((End-Muzzle).GetSafeNormal(),.052f):(End-Muzzle).GetSafeNormal();DamageAt(Muzzle,Direction,Shotgun?16:34,Shotgun);}if(Hits>PreviousHits)Hits=PreviousHits+1;
    BroadcastNoise(Muzzle,Shotgun?6500:5000);
    H->AddControllerPitchInput((Shotgun?-1.1f:-.42f)*Setting(TEXT("shake")));H->AddControllerYawInput(FMath::FRandRange(-.12f,.12f)*Setting(TEXT("shake")));
}
void AEWardCampaignMode::DamageAt(FVector From,FVector Direction,float Power,bool Pellet)
{
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(CampaignShot),true,Hero());
    if(!FMath::IsFinite(Power)||Power<=0||!Hero()||!GetWorld()->LineTraceSingleByChannel(Hit,From,From+Direction.GetSafeNormal()*6000,ECC_Visibility,Q))return;
    Power*=WardCombat63::RangeScale(Pellet,FVector::Dist(From,Hit.ImpactPoint));
    bool Flesh=false;
    if(auto* E=Cast<AEWardCampaignEnemy>(Hit.GetActor()))
    {
        Flesh=true;const float Height=Hit.ImpactPoint.Z-(E->GetActorLocation().Z-96);const bool Head=Height>(E->Crawler?60:120);
        UGameplayStatics::ApplyPointDamage(E,Power*(Head?2.f:Height<40?.65f:1.f),Direction,Hit,Hero()->GetController(),Hero(),nullptr);++Hits;if(Height<40)++E->Injuries;
        Cue(TEXT("hit"),Hit.ImpactPoint,.6f);
    }
    else if(auto* M=Cast<AEWardHospitalMob>(Hit.GetActor())){Flesh=true;const float Half=M->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();const float Fraction=(Hit.ImpactPoint.Z-M->GetActorLocation().Z+Half)/(Half*2);UGameplayStatics::ApplyPointDamage(M,Power*WardCombat63::BodyScale(Fraction),Direction,Hit,Hero()->GetController(),Hero(),nullptr);++Hits;Cue(TEXT("hit"),Hit.ImpactPoint,.6f);}
    else if(auto* Person=Hit.GetActor()?Hit.GetActor()->FindComponentByClass<UEWardPerson54>():nullptr)
    {
        Flesh=true;Person->Impact58(Power,Direction*Power*7);
        Cue(TEXT("FIGHT/flesh_hit"),Hit.ImpactPoint,.3f);
    }
    else if(auto* O=Cast<AEWardCampaignObject>(Hit.GetActor());O&&O->ObjectKind==TEXT("partition"))O->Break();
    else {Cue(TEXT("impact"),Hit.ImpactPoint,.5f);if(Hit.GetComponent()&&Hit.GetComponent()->IsSimulatingPhysics())Hit.GetComponent()->AddImpulseAtLocation(Direction*18000,Hit.ImpactPoint);}
    if(Flesh)WardCombat63::HitBlood(*this,Hit.ImpactPoint,Direction,Pellet?.7f:1.f);else SurfaceImpact(Hit.ImpactPoint,Hit.ImpactNormal);
}
void AEWardCampaignMode::DamageHero(float D,AActor* Source)
{
    if (WardOnline80::Guest(*this)) return;
    auto* H=Hero();if(!FMath::IsFinite(D)||D<=0||!H||H->Dead||Invulnerable>0||Won||Admin&&!Agent)return;
    const bool FellOut=H->GetActorLocation().Z<-2500;
    if(WardRescue104::Downed(*this)&&WardRescue104::DownHost(*this,FellOut))return;
    WardRescue104::HostHit(*this);
    H->Health=FMath::Max(0.f,H->Health-D);WardCombat63::Wound(*this,D,Source);Invulnerable=DifficultyValue(TEXT("invulnerability"));if(H->Health<=0)Cue(TEXT("HUMAN/scream_m"),H->GetActorLocation(),.4f);H->StopInteract();
    if(H->Health<=0){if(WardRescue104::DownHost(*this,FellOut))return;H->Dead=true;HidingBed=nullptr;H->GetCharacterMovement()->DisableMovement();H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);H->GetMesh()->SetHiddenInGame(false);H->GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));H->GetMesh()->SetSimulatePhysics(true);PendingRespawn=true;RespawnTime=1.7f;Notify(TEXT("Последняя смена не окончена… Загрузка сохранения."),3);}
}
void AEWardCampaignMode::Knife()
{
    if (WardOnline80::PersonalAction(*this, TEXT("knife"))) return;
    if(HospitalStory&&HospitalStory->ArrivalCinematic())return;
    if(!Hero()||Hero()->Dead||ShotCooldown>0||HidingBed||(BookWorld&&BookWorld->FurnitureHiding)||!Screen.IsEmpty()||ReloadTime>0||WardCombat63::Healing(*this)||!PreviewItem51.IsEmpty()||Admin&&!Agent||Hero()->Stamina<20)return;Hero()->Stamina-=20;ShotCooldown=.65f;Cue(TEXT("knife"),Hero()->GetActorLocation());BroadcastNoise(Hero()->GetActorLocation(),500);
    FHitResult Hit;const FVector From=Hero()->GetActorLocation()+FVector(0,0,20),Dir=Hero()->Camera->GetForwardVector();FCollisionQueryParams Q(SCENE_QUERY_STAT(CampaignKnife),false,Hero());
    if(GetWorld()->SweepSingleByChannel(Hit,From,From+Dir*170,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(28),Q))
    {if(auto* O=Cast<AEWardCampaignObject>(Hit.GetActor());O&&O->ObjectKind==TEXT("partition"))O->Break();else if(auto* E=Cast<AEWardCampaignEnemy>(Hit.GetActor())){UGameplayStatics::ApplyDamage(E,22,Hero()->GetController(),Hero(),nullptr);E->Stunned=.35f;}else if(auto* M=Cast<AEWardHospitalMob>(Hit.GetActor()))UGameplayStatics::ApplyDamage(M,22,Hero()->GetController(),Hero(),nullptr);if(Cast<AEWardCampaignEnemy>(Hit.GetActor())||Cast<AEWardHospitalMob>(Hit.GetActor()))WardCombat63::HitBlood(*this,Hit.ImpactPoint,Dir,.65f);}
}
void AEWardCampaignMode::Dash(){if(!Screen.IsEmpty()||WardRescue104::Downed(*this)||WardCombat63::Healing(*this)||(BookWorld&&BookWorld->FurnitureHiding))return;if(HospitalStory&&HospitalStory->ArrivalCinematic())return;if(!Hero()||DashCooldown>0||Hero()->Stamina<22||HidingBed)return;DashTime=.22f;DashCooldown=2.8f;Hero()->Stamina-=22;DashHeading=Hero()->GetLastMovementInputVector().IsNearlyZero()?Hero()->GetActorForwardVector():Hero()->GetLastMovementInputVector();Hero()->LaunchCharacter(DashHeading*1050+FVector(0,0,35),true,false);Invulnerable=.15f;}
void AEWardCampaignMode::Flash(){if(HospitalStory&&HospitalStory->ArrivalCinematic())return;if(!Hero()||FlashCharges<=0||NumberFlag(TEXT("flash_cooldown"))>0)return;--FlashCharges;NumberFlag(TEXT("flash_cooldown"),6);Cue(TEXT("flash"),Hero()->GetActorLocation());BroadcastNoise(Hero()->GetActorLocation(),1800);for(auto E:Enemies)if(E->Active&&!E->Dead&&FVector::Dist(E->GetActorLocation(),Hero()->GetActorLocation())<850){FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(CampaignFlash),false,Hero());if(!GetWorld()->LineTraceSingleByChannel(Hit,Hero()->GetActorLocation(),E->GetActorLocation(),ECC_Visibility,Q)||Hit.GetActor()==E)E->Stunned=E->Blind?0:3;}
    if(HospitalStory)for(auto M:HospitalStory->Mobs)if(M&&M->Active&&!M->Dead&&FVector::Dist(M->GetActorLocation(),Hero()->GetActorLocation())<850)
    {
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(MonsterFlash59),false,Hero());
        if(!GetWorld()->LineTraceSingleByChannel(Hit,Hero()->GetActorLocation(),M->GetActorLocation(),ECC_Visibility,Q)||Hit.GetActor()==M)M->Stun59(3,true);
    }
}

void AEWardCampaignMode::ThrowBottle()
{
    if (WardOnline80::PersonalAction(*this, TEXT("bottle"))) return;
    if(HospitalStory&&HospitalStory->ArrivalCinematic())return;
    if(!Hero()||Hero()->Dead||WardCombat63::Healing(*this)||ReloadTime>0||Items.FindRef(TEXT("bottle"))<=0||HidingBed||(BookWorld&&BookWorld->FurnitureHiding)||!Screen.IsEmpty())return;
    const FVector Start=Hero()->GetActorLocation()+FVector(0,0,45),Dir=Hero()->Camera->GetForwardVector();
    FHitResult Obstacle;FCollisionQueryParams Query(SCENE_QUERY_STAT(BottleRelease),false,Hero());
    if(GetWorld()->SweepSingleByChannel(Obstacle,Start,Start+Dir*75,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(12),Query))return;
    Consume(TEXT("bottle"));SpawnPhysics(TEXT("bottle"),Start+Dir*75,Dir*850+FVector(0,0,260)+Hero()->GetVelocity(),FVector(6,6,18),.45f);
}
void AEWardCampaignMode::ToggleHide(AEWardCampaignObject* Bed)
{
    if(!Hero())return;if(HidingBed){HidingBed=nullptr;Hero()->SetActorLocation(HideExit);Hero()->GetMesh()->SetHiddenInGame(false);Hero()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Hero()->GetCharacterMovement()->SetMovementMode(MOVE_Walking);return;}
    if(WardCombat63::Observed(*this)){Notify(TEXT("Сначала разорви видимость. Противник видел, как ты прячешься."));return;}
    HideExit=Hero()->GetActorLocation();HidingBed=Bed;Breath=3;Hero()->GetCharacterMovement()->StopMovementImmediately();Hero()->GetCharacterMovement()->DisableMovement();Hero()->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);Hero()->SetActorLocation(Bed->GetActorLocation()+FVector(0,0,55));Hero()->GetMesh()->SetHiddenInGame(true);Cue(TEXT("cloth"),HideExit);
}
void AEWardCampaignMode::Barricade(){if(auto* O=Cast<AEWardCampaignObject>(Hero()->Focus);O&&O->ObjectKind==TEXT("door")){if(O->Barricade>0){O->Barricade=0;Take(TEXT("boards"));}else if(Items.FindRef(TEXT("boards"))>0&&!O->Opened){Consume(TEXT("boards"));O->Barricade=3;Cue(TEXT("barricade"),O->GetActorLocation());BroadcastNoise(O->GetActorLocation(),1400);}}}
void AEWardCampaignMode::TickHero(float D)
{
    auto* H=Hero();if(!H)return;WardCombat63::Tick(*this,D);ShotCooldown=FMath::Max(0.f,ShotCooldown-D);DashCooldown=FMath::Max(0.f,DashCooldown-D);DashTime=FMath::Max(0.f,DashTime-D);
    if(ReloadTime>0&&(ReloadTime-=D)<=0){if(ShotgunReload){if(Items.FindRef(TEXT("shells"))>0){--Items[TEXT("shells")];++LoadedShells;ReloadWeapon();}}else{int32 N=FMath::Min(9-H->Rounds,H->Reserve);H->Rounds+=N;H->Reserve-=N;}}
    H->MuzzleRemaining=FMath::Max(0.f,H->MuzzleRemaining-D);
    H->WeaponKick=FMath::FInterpTo(H->WeaponKick,0.f,D,13);
    H->WeaponAimBlend=FMath::FInterpTo(H->WeaponAimBlend,H->Aiming?1.f:0.f,D,10);
    H->WeaponObstructed=WardGameplay76::Armed(*this)&&!H->WeaponHasClearance();
    const float WallTarget=H->WeaponObstructed?1.f:0.f;
    H->WeaponCloseBlend=FMath::Lerp(H->WeaponCloseBlend,WallTarget,1.f-FMath::Exp(-FMath::Max(0.f,D)*(H->WeaponObstructed?17.f:10.f)));
    if(H->MuzzleLight){H->MuzzleLight->SetRelativeLocation(FVector(Shotgun?80:23,0,0));H->MuzzleLight->SetIntensity(H->MuzzleRemaining>0?1200:0);}if(H->MuzzleFlame){H->MuzzleFlame->SetRelativeLocation(FVector(Shotgun?88:30,0,0));H->MuzzleFlame->SetVisibility(H->MuzzleRemaining>0);}
    if(H->Equipment.Num()>0)
    {
        H->Pistol->SetWorldTransform(H->WeaponPose());
        Json Rig=Object(Object(Data,TEXT("equipment")),Shotgun?TEXT("shotgun"):TEXT("pistol"));const FString Pump=String(Object(Rig,Shotgun?TEXT("pump"):TEXT("slide")),TEXT("node")),Magazine=String(Object(Rig,TEXT("magazine")),TEXT("node"));
        const float Action=WardCombat63::ActionTravel(*this),MagazineDrop=H->MagazineTravel();
        const bool HasItem=PreviewItem51.IsEmpty()?WardGameplay76::Armed(*this):PreviewItem51==TEXT("weapon")||PreviewItem51==TEXT("shotgun");
        const FName Equipped=(PreviewItem51.IsEmpty()?Shotgun:PreviewItem51==TEXT("shotgun"))?TEXT("shotgun"):TEXT("pistol");
        for(int32 I=0;I<H->Equipment.Num();++I)
        {
            auto* Part=H->Equipment[I].Get();
            Part->SetVisibility(HasItem&&!H->Dead&&!HidingBed&&Part->ComponentTags.Contains(TEXT("authored_visible"))&&Part->ComponentTags.Contains(Equipped));
            const FString& Path=H->EquipmentPaths[I];
            FTransform Rest=H->EquipmentRest[I];
            if(Part->ComponentTags.Contains(Equipped)&&!Pump.IsEmpty()&&Path.StartsWith(Pump))Rest.AddToTranslation(FVector(-Action,0,0));
            if(!Magazine.IsEmpty()&&Part->ComponentTags.Contains(TEXT("pistol"))&&Path.StartsWith(Magazine))Rest.AddToTranslation(FVector(0,0,-MagazineDrop));
            Part->SetRelativeTransform(Rest);
        }
    }
    NumberFlag(TEXT("flash_cooldown"),FMath::Max(0.,NumberFlag(TEXT("flash_cooldown"))-D));
    if(HidingBed){auto* PC=UGameplayStatics::GetPlayerController(this,0);const bool Holding=PC&&PC->IsInputKeyDown(EKeys::LeftShift);Breath=FMath::Clamp(Breath+D*(Holding?-1.f:.6f),0.f,3.f);if(!Holding||Breath<=0)BroadcastNoise(HideExit,180);}
    if(H->GetActorLocation().Z<-2500)DamageHero(1000,nullptr);
}



#include "WardCombat63.inl"
#include "WardCombatAssessment63.inl"
