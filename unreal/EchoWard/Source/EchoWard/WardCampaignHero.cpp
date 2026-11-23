#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardOnline80.h"
#include "WardGameplay76.h"
#include "WardCombat63.h"
#include "WardInterface66.h"
#include "WardBookWorld.h"
#include "WardTitleScene.h"
#include "WardCampaignAnimation.h"
#include "WardSecurity103.h"
#include "WardRescue104.h"
#include "WardGait111.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/LightComponent.h"

void AEWardCampaignHero::BeginPlay()
{
    Super::BeginPlay();
    MeshRest=GetMesh()->GetRelativeTransform();
    GetMesh()->SetAnimInstanceClass(UEWardCampaignAnimation::StaticClass());
    auto* Move=GetCharacterMovement();
    Move->GravityScale=1.65f;
    Move->JumpZVelocity=460;
    Move->MaxAcceleration=WardGait111::Acceleration;
    Move->MaxWalkSpeed=WardGait111::WalkSpeed;
    Move->MaxWalkSpeedCrouched=WardGait111::CrouchSpeed;
    Move->SetCrouchedHalfHeight(58);
    Move->GroundFriction=5;
    Move->bUseSeparateBrakingFriction=true;
    Move->BrakingFriction=2;
    Move->BrakingFrictionFactor=1;
    Move->AirControl=.18f;
    Move->MaxStepHeight=28;
    Move->BrakingDecelerationWalking=WardGait111::Braking;
    Camera->bConstrainAspectRatio=false;
    Arm->ProbeSize=24;
    Arm->bEnableCameraLag=true;
    Arm->CameraLagSpeed=12;
    // The shoulder-camera torch must illuminate the corridor, not the back of
    // the player's head. Keep normal scene lighting/shadows on the character.
    GetMesh()->SetLightingChannels(false,true,false);
    GetMesh()->SetReceivesDecals(false);
    // Use the textured short-hair scalp; the legacy alpha cards form visible bands.
    GetMesh()->ShowMaterialSection(2,2,false,0);
    for(TActorIterator<ALight> It(GetWorld());It;++It)It->GetLightComponent()->SetLightingChannels(true,true,false);
    BuildEquipment();InitializeAcceptedHero50();}
void AEWardCampaignHero::Forward(float V){auto* G=WardCampaign(this);if(G&&!WardOnline80::Reconnecting(*G)&&!(G->HospitalStory&&G->HospitalStory->ArrivalCinematic())&&(!G->Admin||G->AdminHands51)&&!G->HidingBed&&!(G->BookWorld&&G->BookWorld->FurnitureHiding)&&G->Screen.IsEmpty())Super::Forward(V);}
void AEWardCampaignHero::Right(float V){auto* G=WardCampaign(this);if(G&&!WardOnline80::Reconnecting(*G)&&!(G->HospitalStory&&G->HospitalStory->ArrivalCinematic())&&(!G->Admin||G->AdminHands51)&&!G->HidingBed&&!(G->BookWorld&&G->BookWorld->FurnitureHiding)&&G->Screen.IsEmpty())Super::Right(V);}
void AEWardCampaignHero::Turn(float V){if(auto* G=WardCampaign(this)){if(!G->Screen.IsEmpty())return;if(G->HospitalStory&&G->HospitalStory->ArrivalCinematic()){if(G->HospitalStory->Seated31&&G->HospitalStory->ArrivalStage!=0)G->HospitalStory->VehicleYaw31=FMath::Clamp(G->HospitalStory->VehicleYaw31+V*G->Setting(TEXT("sensitivity"),1),-110.f,110.f);return;}if(G->Admin&&!G->AdminHands51)G->AdminViewRotation.Yaw+=V*.65f*G->Setting(TEXT("sensitivity"),1);else Super::Turn(V*G->Setting(TEXT("sensitivity"),1));}}
void AEWardCampaignHero::Look(float V){if(auto* G=WardCampaign(this)){if(!G->Screen.IsEmpty())return;if(G->HospitalStory&&G->HospitalStory->ArrivalCinematic()){if(G->HospitalStory->Seated31&&G->HospitalStory->ArrivalStage!=0)G->HospitalStory->VehiclePitch31=FMath::Clamp(G->HospitalStory->VehiclePitch31-V*G->Setting(TEXT("sensitivity"),1),-50.f,45.f);return;}const float Amount=V*G->Setting(TEXT("sensitivity"),1)*(G->SettingBool(TEXT("invert_y"),false)?-1:1);if(G->Admin&&!G->AdminHands51)G->AdminViewRotation.Pitch=FMath::Clamp(G->AdminViewRotation.Pitch-Amount*.65f,-89.f,89.f);else Super::Look(Amount);}}
void AEWardCampaignHero::Fire(){if(auto* G=WardCampaign(this))G->FireWeapon();}
void AEWardCampaignHero::Reload(){if(auto* G=WardCampaign(this))G->ReloadWeapon();}
void AEWardCampaignHero::Pause(){if(auto* G=WardCampaign(this)){if(G->Screen==WardSecurity103::Page){G->Show(TEXT(""));return;}if(WardOnline80::IsPage(G->Screen)){G->Command(TEXT("online_back"));return;}if(G->Screen==TEXT("phone76")){G->PhoneOpen76=false;G->Show(TEXT(""));return;}if(G->Screen==TEXT("story35")&&G->HospitalStory){G->HospitalStory->NarrativeCommand35(TEXT("story35_close"));return;}if(WardInterface66::Back(*G))return;StopInteract();if(G->TitleScene&&G->TitleScene->Transitioning)return;if(G->TitleView||!G->CampaignStarted){G->Show(TEXT("title"));return;}G->Show(G->Screen.IsEmpty()?TEXT("pause"):TEXT(""));}}
float AEWardCampaignHero::TakeDamage(float D,const FDamageEvent& E,AController* C,AActor* A){if(auto* G=WardCampaign(this))G->DamageHero(D,A);return D;}
void AEWardCampaignHero::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    // The prototype's F10 binding exits the application; campaign F10 exits the admin camera.
    for(int32 I=Input->GetNumActionBindings()-1;I>=0;--I)if(Input->GetActionBinding(I).GetActionName()==TEXT("Jump")||Input->GetActionBinding(I).GetActionName()==TEXT("Quit")||Input->GetActionBinding(I).GetActionName()==TEXT("Flashlight")||Input->GetActionBinding(I).GetActionName()==TEXT("Interact")||Input->GetActionBinding(I).GetActionName()==TEXT("Stats")||Input->GetActionBinding(I).GetActionName()==TEXT("Graphics"))Input->RemoveActionBinding(I);
    auto Key=[&](FKey K,TFunction<void(AEWardCampaignMode*)> F,bool Paused=false){FInputKeyBinding Binding(FInputChord(K),IE_Pressed);Input->KeyBindings.Add(Binding);auto& B=Input->KeyBindings.Last();B.bExecuteWhenPaused=Paused;B.KeyDelegate.GetDelegateForManualSet().BindLambda([this,F](){if(auto* G=WardCampaign(this))F(G);});};
    auto Field=[](AEWardCampaignMode* G,const FString& Page){WardInterface66::Hotkey(*G,Page);};
    Key(EKeys::I,[Field](auto* G){Field(G,TEXT("inventory"));},true);Key(EKeys::M,[Field](auto* G){Field(G,TEXT("map"));},true);Key(EKeys::Tab,[Field](auto* G){Field(G,TEXT("map"));},true);Key(EKeys::J,[Field](auto* G){Field(G,TEXT("journal"));},true);Key(EKeys::O,[Field](auto* G){Field(G,TEXT("quests"));},true);Key(EKeys::F1,[Field](auto* G){Field(G,TEXT("controls66"));},true);Key(EKeys::L,[](auto* G){if(G->Screen.IsEmpty())G->Command(TEXT("replay"));});Key(EKeys::X,[](auto* G){if(G->CampaignStarted&&!G->TitleView&&G->Screen.IsEmpty()&&G->Items.FindRef(TEXT("badge"))>0)G->Show(TEXT("inspect"));});
    Key(EKeys::B,[this](auto* G){if(!G->Admin||!G->Screen.IsEmpty())return;if(G->BookWorld){G->CycleSetting(TEXT("inspection_bright"));G->Notify(G->BookWorld->Bright?TEXT("Яркий осмотр · B — мрачный режим · N — фонарик"):TEXT("Мрачный режим · N — фонарик · B — яркий осмотр"));}else ToggleLight();});
    Key(EKeys::N,[this](auto* G){if(G->BookWorld&&G->Screen.IsEmpty()){ToggleLight();G->Cue(TEXT("UI/settings"),GetActorLocation(),.12f);}});
    Key(EKeys::P,[](auto* G){WardGameplay76::Phone(*G);},true);
    Key(EKeys::Zero,[](auto* G){WardGameplay76::Slot(*G,-1);});
    Key(EKeys::F2,[](auto* G){if(G->CampaignStarted&&!G->TitleView&&(G->Screen.IsEmpty()||G->Screen==TEXT("training")))G->Show(G->Screen.IsEmpty()?TEXT("training"):TEXT(""));},true);
    Key(EKeys::SpaceBar,[this](auto* G){if(G->Screen==TEXT("story35")||(G->HospitalStory&&(G->HospitalStory->HomeStage52==1||G->HospitalStory->ArrivalStage==0)))WardGameplay76::Skip(*G);else if(G->Screen.IsEmpty()&&!Dead&&!G->PhoneOpen76&&!(G->BookWorld&&G->BookWorld->FurnitureHiding))Jump();},true);
    {FInputKeyBinding Release(FInputChord(EKeys::SpaceBar),IE_Released);Release.KeyDelegate.GetDelegateForManualSet().BindLambda([this](){StopJumping();});Input->KeyBindings.Add(Release);}
    Key(EKeys::BackSpace,[](auto* G){WardGameplay76::Skip(*G,true);},true);
    Key(EKeys::F,[this](auto* G){if(G->Screen==TEXT("phone76")){if(G->PhonePage76==TEXT("aruzhan"))G->Command(TEXT("phone76_send"));return;}if(!G->Screen.IsEmpty()||Dead||G->TitleView)return;if(WardRescue104::Begin(*G)||WardSecurity103::Interact(*G)||WardOnline80::Interact(*G))return;if(G->BookWorld&&(G->BookWorld->FurnitureHiding||WardGameplay76::Door(*G->BookWorld)!=INDEX_NONE))G->BookWorld->UseArchitecture();else if(Focus)StartInteract();else if(G->BookWorld)G->BookWorld->UseArchitecture();},true);
    {FInputKeyBinding Release(FInputChord(EKeys::F),IE_Released);Release.KeyDelegate.GetDelegateForManualSet().BindLambda([this](){if(auto* G=WardCampaign(this))WardRescue104::End(*G);StopInteract();});Input->KeyBindings.Add(Release);}
    Key(EKeys::F6,[](auto* G){if(G->Screen.IsEmpty())G->Flash();});
    Key(EKeys::F3,[this](auto* G){if(G->Admin)Stats();});Key(EKeys::F4,[this](auto* G){if(G->Admin)CycleGraphics();});
    Key(EKeys::Y,[](auto* G){if(G->HospitalStory&&G->Screen.IsEmpty())G->HospitalStory->ToggleVehiclePart31();});
    Key(EKeys::PageUp,[](auto* G){if(!WardOnline80::SelectLift(*G,1)&&G->BookWorld&&G->Screen.IsEmpty())G->BookWorld->SelectLift(1);});
    Key(EKeys::PageDown,[](auto* G){if(!WardOnline80::SelectLift(*G,-1)&&G->BookWorld&&G->Screen.IsEmpty())G->BookWorld->SelectLift(-1);});
    Key(EKeys::V,[](auto* G){G->Knife();});Key(EKeys::Q,[](auto* G){G->Dash();});Key(EKeys::G,[](auto* G){G->ThrowBottle();});Key(EKeys::E,[this](auto* G)
    {
        if (!G->Screen.IsEmpty()) return;
        if(Dead||G->TitleView||WardRescue104::Downed(*G))return;if(WardSecurity103::Interact(*G)||WardOnline80::Interact(*G))return;
        if(G->HospitalStory&&G->HospitalStory->HomeStage52>=0)G->HospitalStory->UseHome52(G->HospitalStory->HomeStage52==2);
        else if(G->BookWorld)G->BookWorld->UseArchitecture();
        else if(Focus)StartInteract();
    });Key(EKeys::H,[](auto* G){G->Heal();});Key(EKeys::T,[](auto* G){G->Battery();});Key(EKeys::K,[](auto* G){if(WardOnline80::Guest(*G))return;if(G->BookWorld&&G->Screen.IsEmpty())G->BookWorld->ToggleRefugeBolt();else G->Barricade();});
    Key(EKeys::Z,[](auto* G)
    {
        if (!WardOnline80::PlayingMap(*G))
        {
            if (G->Flag(TEXT("shotgun")) && G->Screen.IsEmpty() && !WardCombat63::Healing(*G) && G->ShotCooldown <= 0)
            {
                G->Shotgun = !G->Shotgun;
                G->ReloadTime = 0;
            }
            return;
        }
        if (!G->Screen.IsEmpty() || WardCombat63::Healing(*G) || G->ShotCooldown > 0 ||
            G->ReloadTime > 0 || !G->CampaignStarted || G->Hero()->Dead ||
            (G->HospitalStory && G->HospitalStory->ArrivalCinematic()))
            return;
        const bool Pistol = G->Flag(TEXT("weapon")), Shotgun = G->Flag(TEXT("shotgun"));
        if (!Pistol && !Shotgun) return;
        const bool Drawn = WardGameplay76::Armed(*G);
        G->Shotgun = Shotgun && (!Pistol || (Drawn ? !G->Shotgun : G->Shotgun));
        G->Flag(TEXT("hands_empty76"), false);
        G->HeldSlot76.Empty();
        G->ClearPreview51();
        G->Hero()->Aiming = false;
    });
    Key(EKeys::F8,[](auto* G){if(G->Admin)G->Command((G->Screen==TEXT("admin_catalog")||G->Screen==TEXT("admin_document"))?TEXT("preview_close"):TEXT("preview_catalog"));},true);
    Key(EKeys::F5,[](auto* G){if(G->CampaignStarted&&!G->TitleView&&G->Screen.IsEmpty())G->Notify(G->Save(TEXT("manual_1"))?TEXT("Игра сохранена"):TEXT("Сейчас сохранение недоступно."));});Key(EKeys::F9,[](auto* G){if(G->Admin)G->ToggleAgent();});Key(EKeys::F10,[](auto* G){if(G->Admin)G->ToggleAdmin();},true);
    Key(EKeys::C,[this](auto* G){if(auto* O=Cast<AEWardCampaignObject>(Focus);O&&O->HasStorage()&&O->Opened){O->Opened=false;O->Motion=1;O->SetActorTickEnabled(true);}});
    const FKey AnswerKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
    for (int32 N = 0; N < UE_ARRAY_COUNT(AnswerKeys); ++N)
        Key(AnswerKeys[N], [this,N](auto* G)
        {
            if (G->Screen == TEXT("story35") && G->HospitalStory)
            {
                auto* Story = G->HospitalStory.Get();
                const auto& Choices = WardData::Array(Story->NarrativeScenes35.FindRef(Story->ActiveScene35), TEXT("choices"));
                if (Choices.IsValidIndex(N))
                    G->Command(TEXT("story35_choice_") + WardData::String(Choices[N]->AsObject(), TEXT("id")));
            }
            else if(G->Screen.IsEmpty())
            {
                auto* O=Cast<AEWardCampaignObject>(Focus);
                if(N<3&&O&&O->HasStorage()&&O->Opened&&!O->Found)O->Selected=N;
                else WardGameplay76::Slot(*G,N);
            }
        });
}
void AEWardCampaignHero::Tick(float D)
{
    ACharacter::Tick(D);auto* G=WardCampaign(this);if(!G)return;if(!WardGameplay76::Armed(*G)&&G->PreviewItem51.IsEmpty())Aiming=false;if(G->PhoneOpen76){Aiming=false;SprintHeld=false;}
    UpdateLocomotion52(D);
    UpdateFirstPerson50();
    G->UpdatePreview51();
    if(G->BookWorld)UpdateTorchTransform();
    if(WardRescue104::Downed(*G))
    {
        auto* Move=GetCharacterMovement();
        Move->MaxWalkSpeed=Move->MaxWalkSpeedCrouched=WardRescue104::CrawlSpeed;
        if(!bIsCrouched){Move->SetCrouchedHalfHeight(WardRescue104::CrawlHalfHeight);Crouch();Move->Crouch(false);}
        Aiming=false;SprintHeld=false;Focus=nullptr;
        NoticeTime=FMath::Max(0.f,NoticeTime-D);
        return;
    }
    auto* Move=GetCharacterMovement();
    if(!bIsCrouched&&!G->HidingBed&&!(G->BookWorld&&G->BookWorld->FurnitureHiding))Move->SetCrouchedHalfHeight(58);
    const bool MotionAssessment=FParse::Param(FCommandLine::Get(),TEXT("WardLocomotionCheck52"));
    if(MotionAssessment&&!G->CampaignStarted){TickLocomotionAssessment52(D);return;}
    NoticeTime=FMath::Max(0.f,NoticeTime-D);if(Dead||G->Won||!G->Screen.IsEmpty()||(G->HospitalStory&&G->HospitalStory->ArrivalCinematic()))return;
    if(Stamina<=1)SprintExhausted=true;if(!SprintHeld||Stamina>=25)SprintExhausted=false;
    const float Speed=GetVelocity().Size2D();const bool Sprint=!WardCombat63::Healing(*G)&&SprintHeld&&!SprintExhausted&&!Aiming&&!Held&&!bIsCrouched&&Speed>10&&Stamina>1&&!G->HidingBed;
    if(Sprint){Stamina=FMath::Max(0.f,Stamina-D*18);RecoverClock=.8f;}else if((RecoverClock-=D)<=0)Stamina=FMath::Min(100.f,Stamina+D*22);
    const bool Blocked=Held||G->HidingBed;
    Move->MaxWalkSpeedCrouched=Blocked?0.f:WardGait111::CrouchSpeed;
    Move->MaxWalkSpeed=Blocked?0.f:(Aiming?WardGait111::AimSpeed:(Sprint?WardGait111::SprintSpeed:WardGait111::WalkSpeed));
    if(WardCombat63::Healing(*G)){Aiming=false;Move->MaxWalkSpeed=FMath::Min(Move->MaxWalkSpeed,140.f);}
    const float FOV=FMath::Clamp(G->Setting(TEXT("fov"),70),70.f,110.f);Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,Aiming?FOV*.81f:FOV,D,8));
    if(MotionAssessment){TickLocomotionAssessment52(D);return;}
    // Foot contacts and spatial sound are driven by travelled distance in UpdateLocomotion52.
    const FVector Eye=G->Admin&&!G->AdminHands51&&G->AdminCamera?G->AdminCamera->GetActorLocation():Camera->GetComponentLocation();
    const FVector Direction=G->Admin&&!G->AdminHands51&&G->AdminCamera?G->AdminCamera->GetActorForwardVector():Camera->GetForwardVector();
    const FVector Origin=G->Admin?Eye:GetActorLocation();Focus=nullptr;float Best=245;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CampaignFocus),false,this);
    for(const auto& Entry:G->Objects)
    {
        auto* O=Entry.Value.Get();if(O->Broken||O->IsHidden()||(O->Used&&!O->HasStorage()))continue;
        const FVector At=O->FocusPoint();const float Dist=FVector::Dist(Origin,At);if(Dist>Best||FVector::DotProduct(Direction,(At-Eye).GetSafeNormal())<.25f)continue;
        FHitResult Hit;if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,At,ECC_Visibility,Query)&&Hit.GetActor()!=O)continue;
        Best=Dist;Focus=O;
    }
    SearchProgress=0;if(Held){auto* O=Cast<AEWardCampaignObject>(Held);if(Held!=Focus||O&&(O->Used||O->ObjectKind==TEXT("door"))||FVector::Dist(Held->FocusPoint(),Origin)>260)StopInteract();else{Held->Hold(this,D);SearchProgress=Held?Held->Work:0;}}
}

void AEWardCampaignHero::UpdateTorchTransform()
{
    // A camera-mounted light can sit behind the hero or on the other side of
    // a doorway. Keep the source at the forward shoulder, inside the capsule's
    // local space, while aiming with the player's view.
    const FRotator View=Controller?Controller->GetControlRotation():GetActorRotation();
    const FVector Right=FRotationMatrix(FRotator(0,View.Yaw,0)).GetUnitAxis(EAxis::Y);
    const FVector Eye=Camera->GetComponentLocation();
    FVector At=Eye+Right*10+View.Vector()*6;
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(TorchShoulder),false,this);
    if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,At,ECC_Visibility,Q))At=Hit.Location+Hit.Normal*5;
    const FRotator Beam(View.Pitch-5,View.Yaw,0);
    Flashlight->SetWorldLocationAndRotation(At,Beam);
    const auto* Campaign=WardCampaign(this);
    if(!Flashlight->IsVisible()||!Campaign||!Campaign->BookWorld)return;

    // Limit the hotspot on nearby walls, without shortening the long beam.
    float Output=2200.f*Campaign->Setting(TEXT("flashlight_strength"),1);
    if(GetWorld()->LineTraceSingleByChannel(Hit,At,At+Beam.Vector()*285,ECC_Visibility,Q))
        Output*=FMath::SmoothStep(0.f,1.f,FMath::Clamp((Hit.Distance-40.f)/235.f,0.f,1.f))*.85f+.15f;
    const float Elapsed=FMath::Clamp(GetWorld()->GetDeltaSeconds(),0.f,.10f);
    const float Response=Output<Flashlight->Intensity?18.f:7.f;
    const float Updated=FMath::Lerp(Flashlight->Intensity,Output,1.f-FMath::Exp(-Response*Elapsed));
    if(FMath::Abs(Updated-Flashlight->Intensity)>.75f)Flashlight->SetIntensity(Updated);
}



