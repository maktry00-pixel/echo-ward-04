#include "WardPeople58.h"
#include "WardPeople54.h"
#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"
#include "Misc/FileHelper.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UnrealClient.h"
using namespace WardData;
namespace
{
    TArray<FVector> Points58(UEWardPerson54* Model)
    {
        TArray<FVector> Points;
        for(int32 Part=0;Part<Model->GetNumSections();++Part)
            for(const auto& Vertex:Model->GetProcMeshSection(Part)->ProcVertexBuffer)Points.Add(Vertex.Position);
        return Points;
    }
    float Change58(const TArray<FVector>& Before,const TArray<FVector>& After)
    {
        if(Before.Num()!=After.Num())return -1;
        float Max=0;
        for(int32 I=0;I<Before.Num();++I)Max=FMath::Max(Max,float(FVector::Dist(Before[I],After[I])));
        return Max;
    }
    void Capture58(AEWardCampaignMode& Mode,UEWardPerson54* Model,const FString& Name,bool Face=false,bool Fallen=false)
    {
        auto* Stage=Mode.GetWorld()->SpawnActor<AActor>();
        auto* Root=NewObject<USceneComponent>(Stage);Stage->SetRootComponent(Root);Root->RegisterComponent();
        const FVector At=Model->GetOwner()->GetActorLocation();
        Stage->SetActorLocation(At);
        const FVector Aim=Fallen?FVector(0,0,30):Face?FVector(0,0,Model->TargetHeight55()*.92f):FVector(0,0,Model->TargetHeight55()*.62f);
        const FVector From=Fallen?FVector(230,-170,230):Face?Aim+FVector(110,25,0):FVector(300,-90,130);
        for(int32 I=0;I<2;++I)
        {
            auto* Light=NewObject<UPointLightComponent>(Stage);Light->SetupAttachment(Root);
            Light->SetRelativeLocation(FVector(180,I?150:-140,200));
            Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(I?450:900);
            Light->SetAttenuationRadius(900);Light->SetCastShadows(false);Light->RegisterComponent();
        }
        auto* Target=NewObject<UTextureRenderTarget2D>(Stage);Target->InitCustomFormat(1280,960,PF_B8G8R8A8,false);Target->UpdateResourceImmediate(true);
        auto* Capture=NewObject<USceneCaptureComponent2D>(Stage);Capture->SetupAttachment(Root);
        Capture->TextureTarget=Target;Capture->CaptureSource=SCS_FinalColorLDR;
        Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        Capture->ShowOnlyComponent(Model);
        TInlineComponentArray<UStaticMeshComponent*> Parts(Model->GetOwner());
        for(auto* Part:Parts)if(Part->ComponentTags.Contains(TEXT("npc_smoking58")))Capture->ShowOnlyComponent(Part);
        Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;Capture->FOVAngle=Face?30:55;
        Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetAtmosphere(false);Capture->ShowFlags.SetBloom(false);Capture->ShowFlags.SetMotionBlur(false);
        Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;Capture->PostProcessSettings.AutoExposureMethod=AEM_Manual;
        Capture->PostProcessSettings.bOverride_AutoExposureBias=true;Capture->PostProcessSettings.AutoExposureBias=-3.5f;
        Capture->SetRelativeLocationAndRotation(From,(Aim-From).Rotation());Capture->RegisterComponent();Capture->CaptureScene();
        FBufferArchive Buffer;
        if(FImageUtils::ExportRenderTarget2DAsPNG(Target,Buffer))
        {
            const FString Path=FPaths::ProjectSavedDir()/TEXT("Prototype/People58")/(Name+TEXT(".png"));
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
            FFileHelper::SaveArrayToFile(Buffer,*Path);
        }
        Stage->Destroy();
    }
    FBox Bounds58(UEWardPerson54* Model)
    {
        FBox Bounds(ForceInit);
        for(const FVector& Point:Points58(Model))Bounds+=Point;
        return Bounds;
    }
}
void AssessHumans58(AEWardCampaignMode& Mode,float Delta)
{
    static int32 Stage=0;
    static float Time=0,MaxBlink=0,MaxMouth=0;
    static TArray<TSharedPtr<FJsonValue>> Checks;
    static TWeakObjectPtr<UEWardPerson54> Speaker;
    static TWeakObjectPtr<ACameraActor> Camera;
    static FVector Stand;
    Time+=FApp::GetDeltaTime();
    auto Check=[&](const FString& Name,bool Pass)
    {
        Json Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Name);Row->SetBoolField(TEXT("passed"),Pass);
        Checks.Add(MakeShared<FJsonValueObject>(Row));
        UE_LOG(LogTemp,Display,TEXT("HUMAN58 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);
    };
    auto* Story=Mode.HospitalStory.Get();
    auto Get=[&](const FString& Id)->UEWardPerson54*
    {
        auto* Actor=Story?Story->Props.FindRef(Id):nullptr;
        return Actor?Actor->FindComponentByClass<UEWardPerson54>():nullptr;
    };
    if(Stage==0&&Story&&Mode.Hero())
    {
        Mode.CampaignStarted=true;Mode.TitleView=false;Mode.Show(TEXT(""));
        Story->FreezeAI=true;Story->HomeStage52=-1;Story->ArrivalStage=-1;Story->SetChapter(19);
        for(auto Mob:Story->Mobs){Mob->Active=false;Mob->SetActorHiddenInGame(true);Mob->SetActorEnableCollision(false);}
        for(const auto& Pair:Story->Props)
        {
            auto* Model=Pair.Value->FindComponentByClass<UEWardPerson54>();
            if(!Model||Pair.Key==TEXT("PERSON_1"))continue;
            Check(Pair.Key+TEXT("_human_rig_loads"),Model->LoadAppearance());
            Model->SetComponentTickEnabled(false);
            Model->DebugFace58(0,0);Model->PresentPose(0,0);
            const auto Base=Points58(Model);
            Model->DebugFace58(1,0);Model->PresentPose(0,0);
            Check(Pair.Key+TEXT("_eyelids_move"),Change58(Base,Points58(Model))>.08f);
            Model->DebugFace58(0,1);Model->PresentPose(0,0);
            Check(Pair.Key+TEXT("_mouth_moves"),Change58(Base,Points58(Model))>.08f);
            Model->DebugFace58(0,0);Model->PresentPose(0,0);
        }
        auto* Walker=Get(TEXT("FILE_33"));
        Stand=Walker->GetOwner()->GetActorLocation();
        FHitResult Hit;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(HumanTest58),false,Walker->GetOwner());
        Mode.GetWorld()->LineTraceSingleByChannel(Hit,Stand+FVector(0,0,100),Stand-FVector(0,0,200),ECC_Visibility,Q);
        if(Hit.bBlockingHit)Stand.Z=Hit.ImpactPoint.Z+.7f;
        Walker->GetOwner()->SetActorHiddenInGame(false);
        Mode.Hero()->SetActorLocation(Stand+FVector(650,0,91));
        Walker->SetTask58(TEXT("follow"));
        const double Before=FVector::Dist2D(Stand,Mode.Hero()->GetActorLocation());
        for(int32 I=0;I<100;++I)Walker->TickHuman58(.05f,false);
        Check(TEXT("escort_moves_toward_player_on_real_floor"),FVector::Dist2D(Walker->GetOwner()->GetActorLocation(),Stand)>100 &&
            FVector::Dist2D(Walker->GetOwner()->GetActorLocation(),Mode.Hero()->GetActorLocation())<Before-100);
        Walker->SetTask58(TEXT("wait"));
        const FVector Wait=Walker->GetOwner()->GetActorLocation();
        for(int32 I=0;I<20;++I)Walker->TickHuman58(.05f,false);
        Check(TEXT("explicit_wait_does_not_move"),Wait.Equals(Walker->GetOwner()->GetActorLocation(),.1));
        const Json Waiting=Walker->HumanSnapshot58();
        Walker->SetTask58(TEXT("follow"));Walker->RestoreHuman58(Waiting);
        Check(TEXT("wait_order_survives_snapshot_restore"),String(Walker->HumanSnapshot58(),TEXT("task"))==TEXT("wait"));

        Walker->GetOwner()->SetActorLocation(Stand);
        auto* Door=Mode.GetWorld()->SpawnActor<AActor>();
        auto* Box=NewObject<UBoxComponent>(Door);Door->SetRootComponent(Box);Box->SetBoxExtent(FVector(20,260,160));
        Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Door->SetActorLocation(Stand+FVector(160,0,160));
        Mode.Hero()->SetActorLocation(Stand+FVector(700,0,91));
        Walker->SetTask58(TEXT("scene"),Stand+FVector(450,0,0));
        for(int32 I=0;I<80;++I)Walker->TickHuman58(.05f,false);
        Check(TEXT("closed_door_blocks_escort"),Walker->GetOwner()->GetActorLocation().X<Stand.X+140);
        Door->Destroy();
        for(int32 I=0;I<140;++I)Walker->TickHuman58(.05f,false);
        Check(TEXT("escort_continues_after_door_opens"),Walker->GetOwner()->GetActorLocation().X>Stand.X+200);

        const auto& Stairs=Array(WardBookMapData(),TEXT("stairs"));
        if(!Stairs.IsEmpty())
        {
            const auto& Rect=Array(Stairs[0]->AsObject(),TEXT("rect"));
            const FVector Lower(Rect[0]->AsNumber()*100+150,Rect[1]->AsNumber()*100+740,.7);
            const FVector Upper=Lower+FVector(0,0,480);
            Walker->GetOwner()->SetActorLocation(Lower);
            Mode.Hero()->SetActorLocation(Lower+FVector(1500,1000,91));
            Walker->SetTask58(TEXT("scene"),Upper);
            float LargestStep=0;
            for(int32 I=0;I<2000;++I)
            {
                const auto BeforeStep=Walker->GetOwner()->GetActorLocation();
                Walker->TickHuman58(.04f,false);
                LargestStep=FMath::Max(LargestStep,float(FMath::Abs(Walker->GetOwner()->GetActorLocation().Z-BeforeStep.Z)));
                if(FVector::Dist(Walker->GetOwner()->GetActorLocation(),Upper)<20)break;
            }
            UE_LOG(LogTemp,Display,TEXT("HUMAN58 stair up %s target %s"),*Walker->GetOwner()->GetActorLocation().ToString(),*Upper.ToString());
            Check(TEXT("npc_climbs_real_two_flight_stair"),FVector::Dist(Walker->GetOwner()->GetActorLocation(),Upper)<25);
            Check(TEXT("stair_movement_has_no_vertical_teleport"),LargestStep<33);
            Walker->SetTask58(TEXT("scene"),Lower);
            for(int32 I=0;I<2000;++I)
            {
                Walker->TickHuman58(.04f,false);
                if(FVector::Dist(Walker->GetOwner()->GetActorLocation(),Lower)<20)break;
            }
            UE_LOG(LogTemp,Display,TEXT("HUMAN58 stair down %s target %s"),*Walker->GetOwner()->GetActorLocation().ToString(),*Lower.ToString());
            Check(TEXT("npc_descends_real_two_flight_stair"),FVector::Dist(Walker->GetOwner()->GetActorLocation(),Lower)<25);
        }
        Walker->GetOwner()->SetActorLocation(Stand);
        Walker->RestoreHuman58(MakeShared<FJsonObject>());
        Walker->Impact58(45,FVector(-70,25,10));
        for(int32 I=0;I<100;++I)Walker->TickHuman58(.04f,false);
        Walker->PresentPose(0,0);
        FBox Body=Bounds58(Walker);
        Check(TEXT("injured_actor_falls_and_body_settles"),Walker->IsFallen58()&&!Walker->IsDead58()&&Body.IsValid&&
            !Body.GetSize().ContainsNaN()&&Body.Min.Z>-30&&Body.Max.Z<130&&Body.GetSize().GetMax()<350);
        const Json Fallen=Walker->HumanSnapshot58();
        const auto BeforeLoad=Points58(Walker);
        Walker->RestoreHuman58(Fallen);Walker->PresentPose(0,0);
        Check(TEXT("fallen_body_pose_survives_save_restore"),!String(Fallen,TEXT("body58")).IsEmpty()&&
            Change58(BeforeLoad,Points58(Walker))<12.f);
        Walker->GetUp58();
        for(int32 I=0;I<35;++I)Walker->TickHuman58(.04f,false);
        Walker->PresentPose(0,0);Body=Bounds58(Walker);
        Check(TEXT("help_returns_injured_actor_to_standing"),!Walker->IsFallen58()&&!Walker->IsDead58()&&Body.Max.Z>Walker->TargetHeight55()*.8f);

        Walker->RestoreHuman58(MakeShared<FJsonObject>());
        Walker->GetOwner()->SetActorLocation(Stand);
        for(int32 I=0;I<39;++I)Walker->TickHuman58(1.f,false);
        Walker->PresentPose(0,0);
        int32 Props=0;TInlineComponentArray<UStaticMeshComponent*> Parts(Walker->GetOwner());
        for(auto* Part:Parts)if(Part->ComponentTags.Contains(TEXT("npc_smoking58"))&&Part->IsVisible())++Props;
        Check(TEXT("adult_outdoor_smoking_uses_pose_and_two_3d_parts"),Walker->HumanState58()==TEXT("smoke")&&Props==2);
        Walker->SetTask58(TEXT("wait"));Walker->PresentPose(0,0);
        bool Hidden=true;for(auto* Part:Parts)if(Part->ComponentTags.Contains(TEXT("npc_smoking58")))Hidden&=!Part->IsVisible();
        Check(TEXT("smoking_stops_when_player_gives_order"),Hidden);

        Walker->RestoreHuman58(MakeShared<FJsonObject>());
        if(!Story->Mobs.IsEmpty())
        {
            auto* Threat=Story->Mobs[0].Get();
            Threat->Active=true;Threat->Dead=false;Threat->AttackClock=10;
            Threat->SetActorHiddenInGame(false);Threat->SetActorEnableCollision(false);
            const FVector ThreatPosition=Stand+FVector(-250,0,80);
            Threat->SetActorLocation(ThreatPosition);
            const double BeforeDanger=FVector::Dist2D(Walker->GetOwner()->GetActorLocation(),ThreatPosition);
            for(int32 I=0;I<35;++I)Walker->TickHuman58(.05f,false);
            Check(TEXT("visible_monster_makes_npc_retreat"),FVector::Dist2D(Walker->GetOwner()->GetActorLocation(),ThreatPosition)>BeforeDanger+45);
            Threat->Active=false;Threat->SetActorHiddenInGame(true);
        }
        Walker->RestoreHuman58(MakeShared<FJsonObject>());
        Walker->GetOwner()->SetActorLocation(Stand);
        Walker->Impact58(100,FVector(-65,0,30),true);
        for(int32 I=0;I<90;++I)Walker->TickHuman58(.04f,false);
        Walker->PresentPose(0,0);Body=Bounds58(Walker);
        const Json Dead=Walker->HumanSnapshot58();
        Walker->RestoreHuman58(Dead);
        Check(TEXT("death_and_ragdoll_survive_save_restore"),Walker->IsDead58()&&Walker->IsFallen58()&&Boolean(Dead,TEXT("dead"))&&Body.Max.Z<130);
        Walker->GetUp58();Check(TEXT("dead_actor_cannot_be_revived_by_help"),Walker->IsDead58());

        Speaker=Get(TEXT("FILE_30"));
        auto* Actor=Speaker->GetOwner();Actor->SetActorHiddenInGame(false);Speaker->SetComponentTickEnabled(true);
        Mode.Hero()->SetActorLocation(Actor->GetActorLocation()+FVector(300,-200,91));
        Story->ActiveScene35.Empty();Story->NarrativeUse35(TEXT("FILE_30"));
        Camera=Mode.GetWorld()->SpawnActor<ACameraActor>();
        const FVector Eye=Actor->GetActorLocation()+FVector(140,10,155);
        Camera->SetActorLocationAndRotation(Eye,(Actor->GetActorLocation()+FVector(0,0,155)-Eye).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(45);
        auto* Light=NewObject<UPointLightComponent>(Camera.Get());Light->SetupAttachment(Camera->GetRootComponent());
        Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(500);Light->SetAttenuationRadius(800);Light->SetCastShadows(false);Light->RegisterComponent();
        if(auto* PC=Cast<APlayerController>(Mode.Hero()->GetController())){PC->SetViewTarget(Camera.Get());if(auto* HUD=PC->GetHUD())HUD->bShowHUD=false;}
        Time=0;Stage=1;
    }
    else if(Stage==1)
    {
        MaxBlink=FMath::Max(MaxBlink,Speaker->BlinkWeight58());MaxMouth=FMath::Max(MaxMouth,Speaker->MouthWeight58());
        if(Time>15.f)
        {
            Check(TEXT("actual_game_ticks_blink_eyelids"),MaxBlink>.3f);
            Check(TEXT("actual_voiced_dialogue_moves_mouth"),MaxMouth>.1f);
            Mode.Show(TEXT(""));Story->ActiveScene35.Empty();Speaker->SetTask58(TEXT("wait"));
            Speaker->DebugFace58(0,.25f);Speaker->PresentPose(0,0);
            Capture58(Mode,Speaker.Get(),TEXT("face"),true,false);
            Time=0;Stage=2;
        }
    }
    else if(Stage==2&&Time>.6f)
    {
        auto* Walker=Get(TEXT("FILE_33"));Walker->RestoreHuman58(MakeShared<FJsonObject>());
        Walker->GetOwner()->SetActorLocation(Stand);Walker->SetTask58(TEXT("routine"));
        for(int32 I=0;I<39;++I)Walker->TickHuman58(1.f,false);
        Walker->PresentPose(0,0);
        const FVector Eye=Stand+FVector(240,-110,130);
        Camera->SetActorLocationAndRotation(Eye,(Stand+FVector(0,0,130)-Eye).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(60);
        Capture58(Mode,Walker,TEXT("smoking"));
        Time=0;Stage=3;
    }
    else if(Stage==3&&Time>.6f)
    {
        auto* Walker=Get(TEXT("FILE_33"));Walker->Impact58(100,FVector(-70,0,20),true);
        for(int32 I=0;I<100;++I)Walker->TickHuman58(.04f,false);
        Walker->PresentPose(0,0);
        const FVector Eye=Stand+FVector(240,-160,240);
        Camera->SetActorLocationAndRotation(Eye,(Stand+FVector(0,0,30)-Eye).Rotation());
        Capture58(Mode,Walker,TEXT("fallen"),false,true);
        Time=0;Stage=4;
    }
    else if(Stage==4&&Time>.6f)
    {
        bool Pass=true;for(auto Row:Checks)Pass&=Boolean(Row->AsObject(),TEXT("passed"));
        Json Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),Pass);
        Result->SetNumberField(TEXT("count"),Checks.Num());Result->SetArrayField(TEXT("checks"),Checks);
        Write(FPaths::ProjectSavedDir()/TEXT("Prototype/humans58_check.json"),Result);
        Stage=5;FPlatformMisc::RequestExit(false);
    }
}

