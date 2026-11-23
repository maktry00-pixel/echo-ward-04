#include "WardPrototype.h"
#include "WardCampaign.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/Canvas.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "CreatureLayout.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "WardGraphics.h"

namespace
{
    template<class T> T* Asset(const TCHAR* Path) { return LoadObject<T>(nullptr, Path); }
    AEWardGameMode* Mode(const UObject* Context) { return Cast<AEWardGameMode>(UGameplayStatics::GetGameMode(Context)); }
    void Sound(const UObject* Context, const TCHAR* Name, FVector At, float Volume=.7f)
    {
        // Loaded once before play, instead of synchronous disk loads during combat/search.
        if (AEWardGameMode* GM=Mode(Context))
            if (const TObjectPtr<USoundBase>* S=GM->Effects.Find(FName(Name));S && *S)
                UGameplayStatics::PlaySoundAtLocation(Context,S->Get(),At,Volume);
    }
}

AEWardCharacter::AEWardCharacter()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(34,96);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
    Arm=CreateDefaultSubobject<USpringArmComponent>(TEXT("ShoulderArm")); Arm->SetupAttachment(GetRootComponent());
    Arm->TargetArmLength=300; Arm->SocketOffset=FVector(0,48,48); Arm->bUsePawnControlRotation=true;
    Arm->ProbeSize=18; Arm->bDoCollisionTest=true;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); Camera->SetupAttachment(Arm);
    Camera->SetFieldOfView(70);
    Flashlight=CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight")); Flashlight->SetupAttachment(Camera);
    Flashlight->SetRelativeLocation(FVector(25,12,-12)); Flashlight->SetIntensity(3500);
    Flashlight->SetAttenuationRadius(1800); Flashlight->SetOuterConeAngle(32); Flashlight->SetCastShadows(true);
    Pistol=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pistol")); Pistol->SetupAttachment(GetMesh());
    Pistol->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,500,0);
    GetCharacterMovement()->MaxWalkSpeed=300;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch=true;
}

void AEWardCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (USkeletalMesh* Body=Asset<USkeletalMesh>(TEXT("/Game/Prototype/Character/SK_Survivor.SK_Survivor")))
    {
        GetMesh()->SetSkeletalMesh(Body);
        const FBox Bounds=Body->GetBounds().GetBox();
        const float Scale=180/FMath::Max(1.f,Bounds.GetSize().Z);
        GetMesh()->SetRelativeScale3D(FVector(Scale));
        GetMesh()->SetRelativeLocation(FVector(0,0,-96-Bounds.Min.Z*Scale));
        GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
        GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
        Idle=Asset<UAnimSequence>(TEXT("/Game/Prototype/Character/A_Idle.A_Idle"));
        Walk=Asset<UAnimSequence>(TEXT("/Game/Prototype/Character/A_Walk.A_Walk"));
        Run=Asset<UAnimSequence>(TEXT("/Game/Prototype/Character/A_Run.A_Run"));
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        if (Idle) { GetMesh()->PlayAnimation(Idle,true); CurrentAnimation=Idle; }
        // Resolve the imported hand name rather than assuming the source FBX spelling.
        TArray<FName> Bones; GetMesh()->GetBoneNames(Bones);
        for(const FName Bone:Bones)
        {
            const FString Name=Bone.ToString().Replace(TEXT("-"),TEXT(" ")).Replace(TEXT("_"),TEXT(" "));
            if(Name.Contains(TEXT("Hand")) && (Name.Contains(TEXT(" R ")) || Name.Contains(TEXT("_R_")) || Name.Contains(TEXT("Right"))))
            {
                Pistol->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepRelativeTransform,Bone);
                UE_LOG(LogTemp,Display,TEXT("WARD_HAND %s (%d bones)"),*Name,Bones.Num()); break;
            }
        }
    }
    Pistol->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Game/Prototype/Meshes/SM_Pistol.SM_Pistol")));
    Pistol->SetRelativeLocation(FVector(5,1,-3)); Pistol->SetRelativeRotation(FRotator(0,90,0));
    ShotSound=Asset<USoundBase>(TEXT("/Game/Prototype/Audio/Shot.Shot"));
    StepSound=Asset<USoundBase>(TEXT("/Game/Prototype/Audio/Step.Step"));
    HurtSound=Asset<USoundBase>(TEXT("/Game/Prototype/Audio/Hurt.Hurt"));
    Announce(TEXT("Осмотри тумбу. Удерживай F для поиска; выстрелы привлекают внимание."),7);
}

void AEWardCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("Forward"),this,&AEWardCharacter::Forward);
    Input->BindAxis(TEXT("Right"),this,&AEWardCharacter::Right);
    Input->BindAxis(TEXT("Turn"),this,&AEWardCharacter::Turn);
    Input->BindAxis(TEXT("LookUp"),this,&AEWardCharacter::Look);
    Input->BindAction(TEXT("Aim"),IE_Pressed,this,&AEWardCharacter::AimPressed);
    Input->BindAction(TEXT("Aim"),IE_Released,this,&AEWardCharacter::AimReleased);
    Input->BindAction(TEXT("Fire"),IE_Pressed,this,&AEWardCharacter::Fire);
    Input->BindAction(TEXT("Reload"),IE_Pressed,this,&AEWardCharacter::Reload);
    Input->BindAction(TEXT("Sprint"),IE_Pressed,this,&AEWardCharacter::StartSprint);
    Input->BindAction(TEXT("Sprint"),IE_Released,this,&AEWardCharacter::StopSprint);
    Input->BindAction(TEXT("Crouch"),IE_Pressed,this,&AEWardCharacter::StartCrouch);
    Input->BindAction(TEXT("Crouch"),IE_Released,this,&AEWardCharacter::StopCrouch);
    Input->BindAction(TEXT("Jump"),IE_Pressed,this,&ACharacter::Jump);
    Input->BindAction(TEXT("Interact"),IE_Pressed,this,&AEWardCharacter::StartInteract);
    Input->BindAction(TEXT("Interact"),IE_Released,this,&AEWardCharacter::StopInteract);
    Input->BindAction(TEXT("Flashlight"),IE_Pressed,this,&AEWardCharacter::ToggleLight);
    Input->BindAction(TEXT("Pause"),IE_Pressed,this,&AEWardCharacter::Pause).bExecuteWhenPaused=true;
    Input->BindAction(TEXT("Stats"),IE_Pressed,this,&AEWardCharacter::Stats).bExecuteWhenPaused=true;
    Input->BindAction(TEXT("Graphics"),IE_Pressed,this,&AEWardCharacter::CycleGraphics).bExecuteWhenPaused=true;
    Input->BindAction(TEXT("Quit"),IE_Pressed,this,&AEWardCharacter::Quit).bExecuteWhenPaused=true;
}
void AEWardCharacter::Forward(float V) { if (!Dead && Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V); }
void AEWardCharacter::Right(float V) { if (!Dead && Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V); }
void AEWardCharacter::Turn(float V) { if (!Dead) AddControllerYawInput(V*.65f); }
void AEWardCharacter::Look(float V) { if (!Dead) AddControllerPitchInput(V*.65f); }
void AEWardCharacter::AimPressed() { Aiming=true; }
void AEWardCharacter::AimReleased() { Aiming=false; }
void AEWardCharacter::StartSprint() { SprintHeld=true; }
void AEWardCharacter::StopSprint() { SprintHeld=false; }
void AEWardCharacter::StartCrouch() { Crouch(); }
void AEWardCharacter::StopCrouch() { UnCrouch(); }
void AEWardCharacter::ToggleLight() { Flashlight->ToggleVisibility(); }
void AEWardCharacter::Announce(const FString& Text,float Seconds) { Notice=Text; NoticeTime=Seconds; }

void AEWardCharacter::Tick(float Delta)
{
    Super::Tick(Delta);
    NoticeTime=FMath::Max(0.f,NoticeTime-Delta);
    Invulnerable=FMath::Max(0.f,Invulnerable-Delta);
    ShotCooldown=FMath::Max(0.f,ShotCooldown-Delta);
    if (Dead || (Mode(this) && Mode(this)->Won)) return;
    const float Speed=GetVelocity().Size2D();
    const bool Sprinting=SprintHeld && !Aiming && !Held && !bIsCrouched && Speed>10 && Stamina>1;
    if (Sprinting) { Stamina=FMath::Max(0.f,Stamina-Delta*18); RecoverClock=.8f; }
    else if ((RecoverClock-=Delta)<=0) Stamina=FMath::Min(100.f,Stamina+Delta*22);
    GetCharacterMovement()->MaxWalkSpeed=Held?0:(Aiming?165:(Sprinting?470:300));
    bUseControllerRotationYaw=Aiming;
    GetCharacterMovement()->bOrientRotationToMovement=!Aiming;
    Arm->TargetArmLength=FMath::FInterpTo(Arm->TargetArmLength,Aiming?185.f:300.f,Delta,8);
    Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,Aiming?57.f:70.f,Delta,8));
    if (ReloadTime>0 && (ReloadTime-=Delta)<=0) { const int32 N=FMath::Min(9-Rounds,Reserve); Rounds+=N; Reserve-=N; }
    UAnimSequence* Wanted=Speed<8?Idle.Get():(Speed>345?Run.Get():Walk.Get());
    if (Wanted && Wanted!=CurrentAnimation.Get()) { GetMesh()->PlayAnimation(Wanted,true); CurrentAnimation=Wanted; }
    if (Wanted && Speed>8) GetMesh()->SetPlayRate(FMath::Clamp(Speed/(Wanted==Run.Get()?310.f:140.f),.65f,1.6f));
    if (Speed>15 && GetCharacterMovement()->IsMovingOnGround() && (StepClock-=Delta)<=0)
    {
        if (StepSound) UGameplayStatics::PlaySoundAtLocation(this,StepSound,GetActorLocation(),.25f);
        StepClock=Sprinting?.31f:.49f;
        if (AEWardGameMode* GM=Mode(this)) GM->BroadcastNoise(GetActorLocation(),Sprinting?1100:240);
    }
    Focus=nullptr; float Best=240;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WardFocus),false,this);
    for (TActorIterator<AEWInteractable> It(GetWorld());It;++It)
    {
        if (It->Used) continue;
        const FVector At=It->FocusPoint(); const float Distance=FVector::Dist(GetActorLocation(),At);
        if (Distance>Best || FVector::DotProduct(Camera->GetForwardVector(),(At-Camera->GetComponentLocation()).GetSafeNormal())<.1f) continue;
        FHitResult Hit;
        const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,Camera->GetComponentLocation(),At,ECC_Visibility,Params);
        if (Blocked && Hit.GetActor()!=*It) continue;
        Best=Distance; Focus=*It;
    }
    SearchProgress=0;
    if (Held)
    {
        if (Held!=Focus || FVector::Dist(Held->FocusPoint(),GetActorLocation())>245) StopInteract();
        else { Held->Hold(this,Delta); SearchProgress=Held->Work; }
    }
}

void AEWardCharacter::StartInteract() { if (Focus && !Dead) { Held=Focus; Focus->Interact(this); } }
void AEWardCharacter::StopInteract() { if (Held) Held->Cancel(); Held=nullptr; SearchProgress=0; }
void AEWardCharacter::Reload()
{
    if (Dead || (Mode(this) && Mode(this)->Won)) { UGameplayStatics::OpenLevel(this,TEXT("Ward04")); return; }
    if (ReloadTime<=0 && Rounds<9 && Reserve>0) { ReloadTime=1.8f; Sound(this,TEXT("Reload"),GetActorLocation()); }
}
void AEWardCharacter::Fire()
{
    if (!Aiming || Dead || ShotCooldown>0 || ReloadTime>0 || (Mode(this) && Mode(this)->Won)) return;
    if (Rounds<=0) { Announce(TEXT("Нет патронов в магазине. R — перезарядка."),2); return; }
    --Rounds; ShotCooldown=.32f;
    const FVector From=Camera->GetComponentLocation(), Direction=Camera->GetForwardVector();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardShot),true,this);
    FHitResult Aim, Hit;
    GetWorld()->LineTraceSingleByChannel(Aim,From,From+Direction*5000,ECC_Visibility,Query);
    const FVector Goal=Aim.bBlockingHit?Aim.ImpactPoint:From+Direction*5000;
    const FVector Muzzle=GetActorLocation()+FVector(0,0,45)+GetActorForwardVector()*40+GetActorRightVector()*17;
    GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,Goal+Direction*5,ECC_Visibility,Query);
    if (Hit.bBlockingHit)
    {
        if (AEWardMonster* M=Cast<AEWardMonster>(Hit.GetActor()))
            UGameplayStatics::ApplyPointDamage(M,Hit.GetComponent()==M->Head?50:25,Direction,Hit,Controller,this,nullptr);
        if (UPrimitiveComponent* Component=Hit.GetComponent();Component && Component->IsSimulatingPhysics())
            Component->AddImpulseAtLocation(Direction*18000,Hit.ImpactPoint);
        Sound(this,TEXT("Impact"),Hit.ImpactPoint,.4f);
    }
    if (ShotSound) UGameplayStatics::PlaySoundAtLocation(this,ShotSound,Muzzle,.8f);
    AddControllerPitchInput(-.65f);
    if (AEWardGameMode* GM=Mode(this)) GM->BroadcastNoise(GetActorLocation(),5000);
}
float AEWardCharacter::TakeDamage(float Damage,FDamageEvent const& Event,AController* Source,AActor* Causer)
{
    if (Dead || Invulnerable>0 || (Mode(this) && Mode(this)->Won)) return 0;
    Health=FMath::Max(0.f,Health-Damage); Invulnerable=1.2f;
    if (HurtSound) UGameplayStatics::PlaySound2D(this,HurtSound,.6f);
    StopInteract(); Announce(TEXT("Отступи и сохрани дистанцию!"),2);
    if (Health<=0)
    {
        Dead=true; GetCharacterMovement()->DisableMovement(); GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetMesh()->SetCollisionProfileName(TEXT("Ragdoll")); GetMesh()->SetSimulatePhysics(true);
        Pistol->SetVisibility(false); Announce(TEXT("R — повторить комнату."),1000);
    }
    return Damage;
}
void AEWardCharacter::Pause()
{
    if (APlayerController* PC=Cast<APlayerController>(Controller)) { const bool Paused=!UGameplayStatics::IsGamePaused(this); PC->SetPause(Paused); PC->bShowMouseCursor=Paused; StopInteract(); }
}
void AEWardCharacter::Stats() { if (APlayerController* PC=Cast<APlayerController>(Controller)) PC->ConsoleCommand(TEXT("stat fps")); }
void AEWardCharacter::CycleGraphics()
{
    const int32 Next=(EWardGraphics::Current()+1)%3;if(auto* G=WardCampaign(this)){G->Options->SetNumberField(TEXT("graphics_preset"),Next);G->Options->SetNumberField(TEXT("msaa"),3);G->Options->SetNumberField(TEXT("screen_percentage"),Next==2?75:100);G->ApplySettings();}else EWardGraphics::Apply(Next,true);
    Announce(TEXT("Графика: ")+EWardGraphics::Label()+TEXT(" · F4 — сменить"),5);
}
void AEWardCharacter::Quit() { UKismetSystemLibrary::QuitGame(this,Cast<APlayerController>(Controller),EQuitPreference::Quit,false); }

AEWInteractable::AEWInteractable()
{
    PrimaryActorTick.bCanEverTick=true;
    Pivot=CreateDefaultSubobject<USceneComponent>(TEXT("Root")); SetRootComponent(Pivot);
    Hinge=CreateDefaultSubobject<USceneComponent>(TEXT("Hinge")); Hinge->SetupAttachment(Pivot);
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body")); Body->SetupAttachment(Pivot); Body->SetCollisionProfileName(TEXT("BlockAll"));
    Moving=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Moving")); Moving->SetupAttachment(Hinge); Moving->SetCollisionProfileName(TEXT("BlockAll"));
    Loot=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Loot")); Loot->SetupAttachment(Moving); Loot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AEWInteractable::BeginPlay()
{
    Super::BeginPlay();
    // These actors tick only while a drawer or door is actually animating.
    SetActorTickEnabled(false);
    if (Kind==EWardInteraction::Drawer)
    {
        Body->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Game/Prototype/Meshes/SM_Nightstand.SM_Nightstand")));
        Moving->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Game/Prototype/Meshes/SM_Drawer.SM_Drawer")));
        Loot->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube")));
        Loot->SetRelativeLocation(FVector(-8,0,55)); Loot->SetRelativeScale3D(FVector(.12,.05,.04)); Loot->SetVisibility(false);
    }
    else
    {
        Body->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube")));
        Body->SetMaterial(0,Asset<UMaterialInterface>(TEXT("/Game/Prototype/Materials/M_Metal.M_Metal")));
        Loot->SetVisibility(false);
        if (Kind==EWardInteraction::Power) { Body->SetRelativeScale3D(FVector(.16,.65,1)); Moving->SetVisibility(false); }
        else
        {
            Body->SetVisibility(false); Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Moving->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube")));
            Moving->SetRelativeLocation(FVector(140,0,140)); Moving->SetRelativeScale3D(FVector(2.8,.15,2.8));
            Moving->SetMaterial(0,Asset<UMaterialInterface>(TEXT("/Game/Prototype/Materials/M_Door.M_Door")));
        }
    }
}
FVector AEWInteractable::FocusPoint() const
{
    return GetActorTransform().TransformPosition(Kind==EWardInteraction::Door?FVector(140,0,130):(Kind==EWardInteraction::Drawer?FVector(0,0,65):FVector::ZeroVector));
}
FString AEWInteractable::Prompt(const AEWardCharacter* Hero) const
{
    if (Kind==EWardInteraction::Drawer) return Found?TEXT("F — взять предохранитель"):(Opened?TEXT("Удерживай F — обыскать ящик"):TEXT("F — открыть тумбу"));
    if (Kind==EWardInteraction::Power) return Hero->HasFuse?TEXT("Удерживай F — установить предохранитель"):TEXT("Нужен предохранитель из тумбы");
    return Mode(this) && Mode(this)->PowerOn?TEXT("F — открыть / закрыть дверь"):TEXT("Дверь обесточена");
}
void AEWInteractable::Interact(AEWardCharacter* Hero)
{
    if (Used) return;
    if (Kind==EWardInteraction::Drawer)
    {
        if (!Opened) { Opened=true; NeedsRelease=true; SetActorTickEnabled(true); Sound(this,TEXT("Drawer"),GetActorLocation()); }
        else if (Found)
        {
            Hero->HasFuse=true; Used=true; Loot->SetVisibility(false); Hero->Announce(TEXT("Предохранитель найден. Включи щиток у стены."));
            if (AEWardGameMode* GM=Mode(this)) GM->BroadcastNoise(GetActorLocation(),1800);
        }
    }
    if (Kind==EWardInteraction::Door && Mode(this) && Mode(this)->PowerOn)
    {
        // Check the full arc against pawns before opening; do not pin a player in a leaf.
        FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_Pawn);
        FCollisionQueryParams Params(SCENE_QUERY_STAT(WardDoorArc),false,this);
        for (int32 I=0;I<=6;++I)
        {
            const FQuat R=GetActorQuat()*FQuat(FVector::UpVector,FMath::DegreesToRadians(I*15.f));
            const FVector Center=GetActorLocation()+R.RotateVector(FVector(140,0,140));
            if (GetWorld()->OverlapAnyTestByObjectType(Center,R,Objects,FCollisionShape::MakeBox(FVector(138,10,135)),Params))
            { Hero->Announce(TEXT("Отойди на шаг от створки."),2); return; }
        }
        Opened=!Opened; SetActorTickEnabled(true); Sound(this,TEXT("Door"),GetActorLocation());
    }
}
void AEWInteractable::Hold(AEWardCharacter* Hero,float Delta)
{
    if (Used || NeedsRelease || Kind==EWardInteraction::Door) return;
    if (Kind==EWardInteraction::Drawer && Opened && !Found)
    {
        Work=FMath::Min(1.f,Work+Delta/2);
        if (Work>=1) { Found=true; Loot->SetVisibility(true); Hero->Announce(TEXT("В ящике предохранитель. Отпусти F и нажми ещё раз.")); }
    }
    if (Kind==EWardInteraction::Power && Hero->HasFuse)
    {
        Work=FMath::Min(1.f,Work+Delta/3);
        if (Work>=1 && Mode(this)) { Mode(this)->PowerOn=true; Hero->HasFuse=false; Used=true; Hero->Announce(TEXT("Аварийная дверь разблокирована. Выход через коридор.")); Sound(this,TEXT("Power"),GetActorLocation()); }
    }
}
void AEWInteractable::Cancel() { NeedsRelease=false; if (!Found && !Used) Work=0; }
void AEWInteractable::Tick(float Delta)
{
    Super::Tick(Delta);
    Blend=FMath::FInterpConstantTo(Blend,Opened?1.f:0.f,Delta,1.5f);
    if (Kind==EWardInteraction::Drawer)
    {
        FHitResult Hit; Moving->SetRelativeLocation(FVector(-32*Blend,0,0),true,&Hit);
    }
    else if (Kind==EWardInteraction::Door) Hinge->SetRelativeRotation(FRotator(0,90*Blend,0));
    if (FMath::IsNearlyEqual(Blend,Opened?1.f:0.f)) SetActorTickEnabled(false);
}

AEWardMonster::AEWardMonster()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(40,96);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
    GetCharacterMovement()->bRunPhysicsWithNoController=true;
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,220,0); GetCharacterMovement()->MaxWalkSpeed=150;
    bUseControllerRotationYaw=false;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body")); Body->SetupAttachment(GetRootComponent()); Body->SetRelativeLocation(FVector(0,0,-96));
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Body->SetCollisionResponseToAllChannels(ECR_Ignore); Body->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Head=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head")); Head->SetupAttachment(Body);
    Head->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Head->SetCollisionResponseToAllChannels(ECR_Ignore); Head->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    for (int32 I=0;I<6;++I)
    {
        UStaticMeshComponent* Leg=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Leg%d"),I));
        Leg->SetupAttachment(Body); Leg->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Leg->SetCollisionResponseToAllChannels(ECR_Ignore); Leg->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block); Legs.Add(Leg);
    }
}
void AEWardMonster::BeginPlay()
{
    Super::BeginPlay();
    Body->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Game/Prototype/Monster/SM_Body.SM_Body")));
    Head->SetStaticMesh(Asset<UStaticMesh>(TEXT("/Game/Prototype/Monster/SM_Head.SM_Head"))); Head->SetRelativeLocation(EWardCreature::HeadPivot);
    for (int32 I=0;I<6;++I)
    {
        const FString PathName=FString::Printf(TEXT("/Game/Prototype/Monster/SM_Leg%d.SM_Leg%d"),I,I);
        Legs[I]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*PathName)); Legs[I]->SetRelativeLocation(EWardCreature::LegPivots[I]); LegPositions.Add(EWardCreature::LegPivots[I]);
    }
    Growl=Asset<USoundBase>(TEXT("/Game/Prototype/Audio/Growl.Growl"));
}
void AEWardMonster::HearNoise(const FVector& At,float Radius)
{
    if (Dead || FVector::Dist(At,GetActorLocation())>Radius) return;
    if (!Awake && Growl) UGameplayStatics::PlaySoundAtLocation(this,Growl,GetActorLocation(),.6f);
    Awake=true; LastKnown=At; Memory=8; Repath=0;
}
TArray<FVector> AEWardMonster::FindPath(const FVector& Goal) const
{
    constexpr float Step=50;
    auto Cell=[=](const FVector& P) { return FIntPoint(FMath::RoundToInt(P.X/Step),FMath::RoundToInt(P.Y/Step)); };
    const FIntPoint Start=Cell(GetActorLocation()), End=Cell(Goal);
    TArray<FIntPoint> Open{Start}; TSet<FIntPoint> Closed; TMap<FIntPoint,FIntPoint> Previous; TMap<FIntPoint,float> Cost; Cost.Add(Start,0);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardAStar),false,this);
    if (APawn* P=UGameplayStatics::GetPlayerPawn(this,0)) Query.AddIgnoredActor(P);
    auto Heuristic=[End](FIntPoint P) { return FMath::Abs(P.X-End.X)+FMath::Abs(P.Y-End.Y); };
    auto Free=[&](FIntPoint P)
    {
        if (P.X<-11 || P.X>11 || P.Y<-8 || P.Y>21) return false;
        const FVector At(P.X*Step,P.Y*Step,96);
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor,At,At-FVector(0,0,120),ECC_Visibility,Query) || Floor.ImpactNormal.Z<.7f || FMath::Abs(Floor.ImpactPoint.Z)>6) return false;
        return !GetWorld()->OverlapBlockingTestByChannel(At,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,94),Query);
    };
    int32 Expanded=0;
    while (!Open.IsEmpty() && Expanded++<1600)
    {
        Open.Sort([&](FIntPoint A,FIntPoint B) { return Cost[A]+Heuristic(A)>Cost[B]+Heuristic(B); });
        FIntPoint Current=Open.Pop();
        if (Current==End)
        {
            TArray<FVector> Result; FIntPoint At=End;
            while (At!=Start && Previous.Contains(At)) { Result.Insert(FVector(At.X*Step,At.Y*Step,96),0); At=Previous[At]; }
            return Result;
        }
        if (Closed.Contains(Current)) continue;
        Closed.Add(Current);
        const FIntPoint Offsets[]={{1,0},{-1,0},{0,1},{0,-1}};
        for (FIntPoint Offset:Offsets)
        {
            const FIntPoint Next=Current+Offset;
            if (Closed.Contains(Next) || !Free(Next)) continue;
            const float NewCost=Cost[Current]+1;
            if (!Cost.Contains(Next) || NewCost<Cost[Next]) { Cost.Add(Next,NewCost); Previous.Add(Next,Current); Open.AddUnique(Next); }
        }
    }
    return {};
}
void AEWardMonster::Tick(float Delta)
{
    Super::Tick(Delta); if (Dead) return;
    AEWardCharacter* Hero=Cast<AEWardCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    if (!Hero || Hero->Dead || (Mode(this) && Mode(this)->Won)) return;
    const float Distance=FVector::Dist2D(Hero->GetActorLocation(),GetActorLocation());
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WardVision),false,this); FHitResult Hit;
    const bool Obstructed=GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()+FVector(0,0,40),Hero->GetActorLocation()+FVector(0,0,40),ECC_Visibility,Query);
    if (Awake && Distance<1600 && (!Obstructed || Hit.GetActor()==Hero)) { LastKnown=Hero->GetActorLocation(); Memory=6; }
    Memory=FMath::Max(0.f,Memory-Delta); Stun=FMath::Max(0.f,Stun-Delta); AttackCooldown=FMath::Max(0.f,AttackCooldown-Delta);
    if (Awake && Stun<=0)
    {
        if (Winding)
        {
            Windup-=Delta;
            if (Windup<=0)
            {
                Winding=false; AttackCooldown=2;
                if (Distance<160 && (!Obstructed || Hit.GetActor()==Hero)) UGameplayStatics::ApplyDamage(Hero,25,nullptr,this,nullptr);
            }
        }
        else if (Distance<135 && (!Obstructed || Hit.GetActor()==Hero) && AttackCooldown<=0) { Winding=true; Windup=.8f; Sound(this,TEXT("Growl"),GetActorLocation(),.35f); }
        else if (Memory>0)
        {
            Repath-=Delta;
            if (Repath<=0) { Path=FindPath(LastKnown); Repath=.65f; }
            while (!Path.IsEmpty() && FVector::Dist2D(GetActorLocation(),Path[0])<30) Path.RemoveAt(0);
            if (!Path.IsEmpty()) AddMovementInput((Path[0]-GetActorLocation()).GetSafeNormal2D(),1,true);
        }
    }
    Phase+=GetVelocity().Size2D()*Delta*.055f;
    Body->SetRelativeLocation(FVector(0,0,-96+FMath::Sin(Phase*2)*1.8f));
    Head->SetRelativeRotation(FRotator(Winding?-12:0, FMath::Sin(GetWorld()->TimeSeconds)*4,0));
    for (int32 I=0;I<Legs.Num();++I)
    {
        const float P=Phase+(I%2)*PI+(I/2)*.65f;
        Legs[I]->SetRelativeRotation(FRotator(FMath::Sin(P)*12,0,FMath::Cos(P)*5));
    }
}
float AEWardMonster::TakeDamage(float Damage,FDamageEvent const& Event,AController* Source,AActor* Causer)
{
    if (Dead) return 0;
    Health-=Damage; Stun=.35f; Winding=false;
    if (Causer) HearNoise(Causer->GetActorLocation(),5000);
    if (Health<=0)
    {
        Dead=true; GetCharacterMovement()->DisableMovement(); GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Body->SetCollisionResponseToAllChannels(ECR_Block);
        Body->SetSimulatePhysics(true); Body->AddImpulse(FVector(5000,0,2000));
        SetLifeSpan(60);
    }
    return Damage;
}

AEWardGameMode::AEWardGameMode()
{
    DefaultPawnClass=AEWardCharacter::StaticClass(); HUDClass=AEWardHUD::StaticClass(); PrimaryActorTick.bCanEverTick=true;
}
void AEWardGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (!FParse::Param(FCommandLine::Get(),TEXT("WardBenchmark")) && !FParse::Param(FCommandLine::Get(),TEXT("WardSmoke"))) EWardGraphics::Load();
    for (const TCHAR* Name:{TEXT("Shot"),TEXT("Step"),TEXT("Hurt"),TEXT("Impact"),TEXT("Reload"),TEXT("Drawer"),TEXT("Door"),TEXT("Power"),TEXT("Growl"),TEXT("Ambience")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Prototype/Audio/%s.%s"),Name,Name);
        if(USoundBase* Effect=LoadObject<USoundBase>(nullptr,*Path)) Effects.Add(FName(Name),Effect);
        else UE_LOG(LogTemp,Error,TEXT("WARD_MISSING_AUDIO %s"),Name);
    }
    auto Spawn=[this](EWardInteraction Kind,FVector At,FRotator Rotation)
    {
        const FTransform Transform(Rotation,At);
        AEWInteractable* Item=GetWorld()->SpawnActorDeferred<AEWInteractable>(AEWInteractable::StaticClass(),Transform);
        Item->Kind=Kind; UGameplayStatics::FinishSpawningActor(Item,Transform);
    };
    Spawn(EWardInteraction::Drawer,FVector(-430,170,0),FRotator(0,180,0));
    Spawn(EWardInteraction::Power,FVector(578,-270,95),FRotator::ZeroRotator);
    Spawn(EWardInteraction::Door,FVector(-140,450,0),FRotator::ZeroRotator);
    Monster=GetWorld()->SpawnActor<AEWardMonster>(AEWardMonster::StaticClass(),FVector(350,230,100),FRotator(0,-130,0));
    if (USoundBase* Ambience=Asset<USoundBase>(TEXT("/Game/Prototype/Audio/Ambience.Ambience"))) UGameplayStatics::SpawnSound2D(this,Ambience,.2f);
}
void AEWardGameMode::BroadcastNoise(const FVector& At,float Radius) { if (Monster) Monster->HearNoise(At,Radius); }
void AEWardGameMode::Tick(float Delta)
{
    Super::Tick(Delta);
    if (FParse::Param(FCommandLine::Get(),TEXT("WardBenchmark")) || FParse::Param(FCommandLine::Get(),TEXT("WardSmoke"))) TickAssessment(Delta);
    AEWardCharacter* Hero=Cast<AEWardCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    if (!Hero || Hero->Dead || Won) return;
    Elapsed+=Delta;
    if (PowerOn && Hero->GetActorLocation().Y>1000 && FMath::Abs(Hero->GetActorLocation().X)<120)
    { Won=true; Hero->GetCharacterMovement()->StopMovementImmediately(); Hero->Announce(TEXT("Ты выбрался. R — повторить комнату; F3 — показать FPS."),1000); }
}
void AEWardHUD::DrawHUD()
{
    Super::DrawHUD(); if (!Canvas) return;
    const AEWardCharacter* Hero=Cast<AEWardCharacter>(GetOwningPawn()); if (!Hero) return;
    const AEWardGameMode* GM=Mode(this);
    const float Scale=FMath::Min(Canvas->SizeX/1920.f,Canvas->SizeY/1080.f), Margin=48*Scale;
    auto Text=[&](const FString& Value,float X,float Y,FLinearColor Color=FLinearColor(.85,.9,.88),float Size=1.2f) { DrawText(Value,Color,X,Y,GEngine->GetMediumFont(),Size*Scale); };
    const FLinearColor Teal(.2,.72,.65), Gold(.85,.62,.29);
    Text(TEXT("EXO · ОТДЕЛЕНИЕ 04"),Margin,Margin,Teal,1.4f);
    const FString Task=GM && GM->PowerOn?TEXT("Найди выход через коридор"):(Hero->HasFuse?TEXT("Восстанови питание щитка"):TEXT("Найди предохранитель в тумбе"));
    Text(Task,Margin,Margin+42*Scale,FLinearColor::White,1.35f);
    DrawRect(FLinearColor(.02,.04,.04,.8),Margin,Canvas->SizeY-130*Scale,320*Scale,80*Scale);
    Text(FString::Printf(TEXT("ЗДОРОВЬЕ  %.0f"),Hero->Health),Margin+12*Scale,Canvas->SizeY-120*Scale);
    DrawRect(FLinearColor(.7,.8,.77),Margin+12*Scale,Canvas->SizeY-90*Scale,280*Scale*Hero->Health/100,7*Scale);
    DrawRect(Gold,Margin+12*Scale,Canvas->SizeY-70*Scale,280*Scale*Hero->Stamina/100,5*Scale);
    Text(FString::Printf(TEXT("9 мм   %d / %d"),Hero->Rounds,Hero->Reserve),Canvas->SizeX-300*Scale,Canvas->SizeY-105*Scale,Gold,1.5f);
    if (Hero->Aiming && !Hero->Dead)
    {
        DrawLine(Canvas->SizeX/2-9*Scale,Canvas->SizeY/2,Canvas->SizeX/2-3*Scale,Canvas->SizeY/2,FLinearColor::White,1.5f);
        DrawLine(Canvas->SizeX/2+3*Scale,Canvas->SizeY/2,Canvas->SizeX/2+9*Scale,Canvas->SizeY/2,FLinearColor::White,1.5f);
    }
    if (Hero->Focus && !Hero->Dead)
    {
        const float X=Canvas->SizeX/2-260*Scale,Y=Canvas->SizeY*.68f;
        DrawRect(FLinearColor(.02,.045,.045,.92),X,Y,520*Scale,68*Scale);
        Text(Hero->Focus->Prompt(Hero),X+15*Scale,Y+18*Scale,Gold,1.05f);
        if (Hero->SearchProgress>0) DrawRect(Teal,X,Y+64*Scale,520*Scale*Hero->SearchProgress,4*Scale);
    }
    if (Hero->NoticeTime>0) Text(Hero->Notice,Margin,Canvas->SizeY-200*Scale,FLinearColor::White,.95f);
    if (UGameplayStatics::IsGamePaused(this) || Hero->Dead || (GM && GM->Won))
    {
        DrawRect(FLinearColor(0,0,0,.65),0,0,Canvas->SizeX,Canvas->SizeY);
        Text(Hero->Dead?TEXT("ТЕБЯ НАШЛИ · R — повторить"):(GM && GM->Won?TEXT("ТЫ ВЫБРАЛСЯ · R — повторить"):TEXT("ПАУЗА · Esc — продолжить · F10 — выход")),Canvas->SizeX*.23f,Canvas->SizeY*.45f,Teal,1.8f);
        Text(TEXT("ГРАФИКА: ")+EWardGraphics::Label(),Canvas->SizeX*.23f,Canvas->SizeY*.54f,FLinearColor::White,1.1f);
        Text(TEXT("F4 — выбрать профиль · F3 — счётчик FPS · лимит 60 кадров/с"),Canvas->SizeX*.23f,Canvas->SizeY*.59f,Gold,1.f);
    }
}
