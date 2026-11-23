#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardHospitalGameplay.h"
#include "WardBookMap.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
using namespace WardData;
void TickHospitalFacilities37(AEWardCampaignMode* M,float Delta)
{
    static TArray<FVector> Walk;static int WalkIndex=0;static float WalkSeconds=0;static int Stage=0,Index=0;static float Clock=0;static ACameraActor* Cam=nullptr;static TArray<Json> Views;static TArray<TSharedPtr<FJsonValue>> Checks;Clock+=Delta;
    auto Check=[&](const FString& Name,bool OK){Json R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetBoolField(TEXT("passed"),OK);Checks.Add(MakeShared<FJsonValueObject>(R));UE_LOG(LogTemp,Display,TEXT("FACILITIES37_CHECK %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),*Name);};
    auto* W=M->BookWorld.Get();auto* H=M->Hero();auto* PC=Cast<APlayerController>(H->Controller);auto* Story=M->HospitalStory.Get();
    auto Place=[&]()
    {
        Json V=Views[Index];const FVector P=Vector(Array(V,TEXT("at")))*100,T=Vector(Array(V,TEXT("target")))*100;
        H->SetActorLocation(P-FVector(0,0,65),false);H->GetCharacterMovement()->DisableMovement();H->SetActorHiddenInGame(true);H->Flashlight->SetVisibility(Boolean(V,TEXT("flashlight")));PC->SetControlRotation((T-P).Rotation());W->SetBright(Boolean(V,TEXT("bright")));
        W->LightSelectionClock=1;W->UpdateLighting(H->GetActorLocation());Cam->SetActorLocationAndRotation(P,(T-P).Rotation());Cam->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;PC->SetViewTarget(Cam);Clock=0;
    };
    if(Stage==0)
    {
        M->NewGame(TEXT("normal"));M->Show(TEXT(""));M->CampaignStarted=true;Story->ArrivalStage=-1;Story->FreezeAI=true;Story->SetChapter(8);W->SetStoryChapter(0);
        for(auto& P:W->Portals){P.Open=true;P.Locked=false;W->RefreshPortalVisual(P);}
        Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/hospital_environment_v37.json"));Check(TEXT("all_new_assets_and_instances_exist"),W->FacilitiesProps37==Array(D,TEXT("props")).Num()&&W->FacilitiesFailures37==0);
        Json Audit=W->AuditFacilities37();Check(TEXT("every_open_doorway_has_capsule_clearance"),Array(Audit,TEXT("blocked_open_doors")).IsEmpty());Check(TEXT("every_door_swing_has_no_detected_obstacle"),Array(Audit,TEXT("swing_obstacles")).IsEmpty());Check(TEXT("all_added_floor_furniture_has_real_support"),Array(Audit,TEXT("unsupported_props")).IsEmpty());
        Check(TEXT("indoor_door_headers_close_the_gap_to_the_soffit"),Array(Audit,TEXT("missing_indoor_headers")).IsEmpty()&&Number(Audit,TEXT("indoor_headers_tested"))>500);
        Check(TEXT("all_35_locations_preserved"),W->Sites.Num()==35&&W->Rooms.Num()==562);Check(TEXT("distant_furnishings_are_deferred_without_removing_collisions"),W->DeferredInitial37>5000);
        Check(TEXT("kazakh_names_and_scenario_preserved"),Number(Story->Narrative35,TEXT("names_revision"))==36&&Object(Story->Narrative35,TEXT("scenes"))->Values.Num()==78);
        if(FParse::Param(FCommandLine::Get(),TEXT("WardFullAudit52")))
        {
            const Json Envelope=W->AuditEnvelope();Check(TEXT("all_cell_floors_ceilings_facades_storey_bands_and_lift_roof"),Boolean(Envelope,TEXT("success")));
            const Json Doors=W->AuditEveryDoor();Write(FPaths::ProjectSavedDir()/TEXT("Prototype/every_door_check.json"),Doors);Check(TEXT("every_portal_closes_blocks_opens_and_restores_state"),Boolean(Doors,TEXT("success")));
        }
        bool ApronSupported=true;int ApronSamples=0;
        FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(ApronSupport73),false,H);
        for(int X=100;X<104;++X)for(int Y=0;Y<100;Y+=2)
        {
            const FVector P(X*100+50,Y*100+50,0);FHitResult GroundHit;
            ApronSupported &= M->GetWorld()->LineTraceSingleByChannel(GroundHit,P+FVector(0,0,18),P-FVector(0,0,60),ECC_Pawn,GroundQuery)
                && GroundHit.ImpactNormal.Z>.9f;
            ++ApronSamples;
        }
        Check(TEXT("east_main_building_apron_has_continuous_physical_ground"),ApronSamples==200&&ApronSupported);
        const Json Architecture=W->ArchitectureSnapshot();int Routes=0;TArray<TSharedPtr<FJsonValue>> Bad;FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FacilitiesRoutes37),false,H);
        for(auto V:Array(WardBookMapData(),TEXT("navigation_routes")))
        {
            Json R=V->AsObject();const auto& P=Array(R,TEXT("points"));bool Clear=true;
            for(int I=1;I<P.Num();++I){const FVector A=Vector(P[I-1]->AsArray())*100+FVector(0,0,98),B=Vector(P[I]->AsArray())*100+FVector(0,0,98);if(FMath::Abs(A.Z-B.Z)>10)continue;if(M->GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q)){Clear=false;Bad.Add(MakeShared<FJsonValueString>(String(R,TEXT("zone"))+TEXT(" / ")+GetNameSafe(Hit.GetComponent())));break;}}
            if(Clear)++Routes;
        }
        Check(TEXT("all_562_routes_remain_clear_after_new_dressing"),Routes==562);Json RoutesResult=MakeShared<FJsonObject>();RoutesResult->SetNumberField(TEXT("passed"),Routes);RoutesResult->SetArrayField(TEXT("blocked"),Bad);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/facilities37_routes.json"),RoutesResult);
        auto Add=[&](const TCHAR* Id,FVector At,FVector Target,bool Bright=false){Json V=MakeShared<FJsonObject>();auto Vec=[](FVector P){return TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)};};V->SetStringField(TEXT("id"),Id);V->SetArrayField(TEXT("at"),Vec(At));V->SetArrayField(TEXT("target"),Vec(Target));V->SetBoolField(TEXT("bright"),Bright);Views.Add(V);};
        Add(TEXT("pilot_checkpoint"),FVector(60,-62,1.7),FVector(63.3,-56.8,2.0));Add(TEXT("pilot_entry"),FVector(50,-31,1.7),FVector(50,-19,1.6));Add(TEXT("pilot_lobby"),FVector(48,-11,1.7),FVector(55,-3,1.6));Add(TEXT("pilot_first_corridor"),FVector(49.5,17.5,1.7),FVector(72,17.5,1.6));Add(TEXT("pilot_ceiling"),FVector(41,-11,1.8),FVector(52,-5,3.2));Add(TEXT("pilot_corridor_bright"),FVector(49.5,17.5,1.7),FVector(72,17.5,1.6),true);
        if(FParse::Param(FCommandLine::Get(),TEXT("WardFacilitiesAllPictures37")))
        {
            for(auto V:Array(WardBookMapData(),TEXT("furnishing_captures")))
            {
                Json Source=V->AsObject();const FString Id=String(Source,TEXT("id"));
                if(!TArray<FString>{TEXT("campus_hall_DG25"),TEXT("campus_hall_DG26"),TEXT("v15_bridge_underpass"),TEXT("v15_bridge_upper"),TEXT("environment_roof")}.Contains(Id))continue;
                Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("id"),Id);C->SetArrayField(TEXT("at"),Array(Source,TEXT("at")));C->SetArrayField(TEXT("target"),Array(Source,TEXT("target")));Views.Add(C);
            }
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("WardFacilitiesAllPictures37")))for(const auto& Site:W->Sites)
        {
            Json V=MakeShared<FJsonObject>();auto Vec=[](FVector P){return TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(P.X/100),MakeShared<FJsonValueNumber>(P.Y/100),MakeShared<FJsonValueNumber>(P.Z/100)};};V->SetStringField(TEXT("id"),TEXT("site_")+Site.Id);V->SetBoolField(TEXT("bright"),!FParse::Param(FCommandLine::Get(),TEXT("WardVisualDark67")));V->SetArrayField(TEXT("at"),Vec(Site.Capture));V->SetArrayField(TEXT("target"),Vec(Site.Capture+Site.View.Vector()*1500));Views.Add(V);
        }
        Cam=M->GetWorld()->SpawnActor<ACameraActor>();Cam->GetCameraComponent()->SetFieldOfView(78);
        Json Arrival=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_arrival_v29.json"));for(auto V:Array(Arrival,TEXT("route")))Walk.Add(Vector(V->AsArray())*100);
        H->SetActorLocation(Walk[0],false);H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);H->GetCharacterMovement()->MaxWalkSpeed=240;H->SetActorHiddenInGame(false);PC->SetViewTarget(H);Stage=4;Clock=0;return;
    }
    if(Stage!=4)H->SetActorHiddenInGame(true);W->Update(Delta);W->LightSelectionClock=1;W->UpdateLighting(H->GetActorLocation());Cam->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;if(Stage!=4)PC->SetViewTarget(Cam);
    if(Stage==4)
    {
        WalkSeconds+=Delta;
        if(WalkIndex<Walk.Num())
        {
            const FVector Diff=Walk[WalkIndex]-H->GetActorLocation();if(Diff.Size2D()<28&&FMath::Abs(Diff.Z)<40){++WalkIndex;Clock=0;}else H->AddMovementInput(Diff.GetSafeNormal2D(),1);
        }
        if(WalkIndex==Walk.Num()||Clock>18||WalkSeconds>150)
        {
            Check(TEXT("character_physically_walks_car_checkpoint_entry_route"),WalkIndex==Walk.Num());Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),WalkIndex==Walk.Num());R->SetNumberField(TEXT("waypoints_reached"),WalkIndex);R->SetNumberField(TEXT("waypoints_total"),Walk.Num());R->SetNumberField(TEXT("seconds"),WalkSeconds);R->SetStringField(TEXT("last_position_cm"),H->GetActorLocation().ToString());R->SetStringField(TEXT("method"),TEXT("Automated character movement through real collisions; not manual keyboard play."));Write(FPaths::ProjectSavedDir()/TEXT("Prototype/facilities37_walk.json"),R);H->GetCharacterMovement()->StopMovementImmediately();Place();Stage=1;
        }
    }
    else if(Stage==1)
    {
        if(W->DeferredNearRemaining37>0){Clock=0;return;}
        if(Clock<3)return;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/Facilities37/")+String(Views[Index],TEXT("id"))+TEXT(".png"),true,false);Stage=2;Clock=0;
    }
    else if(Stage==2&&Clock>.6f)
    {
        ++Index;if(Index<Views.Num()){Place();Stage=1;return;}
        Check(TEXT("no_deferred_native_mesh_failed_on_visited_views"),W->FacilitiesFailures37==0);bool OK=true;for(auto C:Checks)OK&=Boolean(C->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),OK);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);R->SetNumberField(TEXT("captures"),Views.Num());R->SetStringField(TEXT("scope"),TEXT("Automated engine sweeps and rendered observations; not a human playthrough."));Write(FPaths::ProjectSavedDir()/TEXT("Prototype/facilities37_check.json"),R);FPlatformMisc::RequestExit(false);Stage=3;
    }
}
