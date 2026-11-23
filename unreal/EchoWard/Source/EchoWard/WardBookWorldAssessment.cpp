#include "WardCampaign.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardContact98.h"
#include "WardCampaignUI.h"
#include "WardSettings.h"
#include "WardEchoAssets.h"
#include "Engine/Texture.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UnrealClient.h"
using namespace WardData;
void AEWardCampaignMode::TickBookWorldAssessment(float Delta)
{
    static int Step=0,Index=0,Waypoint=0,ExtraCore=0,ExteriorStart=-1,GuardIndex=0;static bool ExteriorTailDone=false,GuardDescending=false;static float Clock=0,SampleTime=0;static int SampleFrames=0;static TArray<TSharedPtr<FJsonValue>> Checks,Performance;static TArray<FVector> Route;
    auto* H=Hero();auto* PC=Cast<APlayerController>(H->Controller);Clock+=Delta;
    auto Check=[&](FString Name,bool Pass){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));UE_LOG(LogTemp,Display,TEXT("BOOK_WORLD_CHECK %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
    auto Next=[&](int Value){Step=Value;Clock=0;};
    auto Place=[&](FVector At,FRotator View){H->SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);H->GetCharacterMovement()->StopMovementImmediately();PC->SetControlRotation(View);H->SetActorRotation(FRotator(0,View.Yaw,0));H->UpdateTorchTransform();BookWorld->UpdateLighting(At);PC->PlayerCameraManager->UpdateCamera(.016f);};
    auto HitInfo=[&](const FHitResult& Hit){FTransform T;if(auto* C=Cast<UHierarchicalInstancedStaticMeshComponent>(Hit.GetComponent()))C->GetInstanceTransform(Hit.Item,T,true);return FString::Printf(TEXT(" AT=%s SCALE=%s COMPONENT=%s"),*T.GetLocation().ToString(),*T.GetScale3D().ToString(),*GetNameSafe(Hit.GetComponent()));};
    if(Step!=21&&Step!=22&&BookWorld)BookWorld->Update(Delta);
    const bool Pictures=FParse::Param(FCommandLine::Get(),TEXT("WardBookWorldPictures"));
    const bool GeometryOnly=FParse::Param(FCommandLine::Get(),TEXT("WardGeometryOnly89"));
    const float GalleryZ=Number(Object(WardBookMapData(),TEXT("campus_upgrade")),TEXT("gallery_z_m"),0)*100;
    if(Step==0)
    {
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCaptureSupplements")))
        {
            auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::Bindings(),TEXT("review_captures")));
            WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);
        }
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCaptureQuality")))
        {
            auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::Quality(),TEXT("captures")));WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);
        }
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCaptureReal")))
        {
            auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::RealArt(),TEXT("captures")));WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);
        }
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCaptureDetail")))
        {
            auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::DetailArt(),TEXT("captures")));WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);
        }
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCaptureDetail27")))
        {
            auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::DetailArt27(),TEXT("captures")));WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);
        }
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCaptureHorror28")))
        {
            auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::HorrorArt28(),TEXT("captures")));WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);
        }
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCaptureRealism30")))
        {
            auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::Realism30(),TEXT("captures")));WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);
        }
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCarsReview31")))
        {auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::Cars31(),TEXT("captures")));WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);}
        Check(TEXT("normal_new_game_uses_complete_3d_world"),BookWorldEnabled&&BookWorld&&BookWorld->Rooms.Num()==562&&BookWorld->Sites.Num()==35);
        if(!BookWorld){FPlatformMisc::RequestExit(true);return;}if(Pictures)ApplySettings(false);Command(TEXT("new_normal"));H->NoticeTime=0;
        if(Number(WardEchoAssets::Cars31(),TEXT("version"))>=31)
        {
            Check(TEXT("v31_all_supplied_sources_placed_with_hero"),BookWorld->Cars31Count==35&&BookWorld->Cars31Sources.Num()==31&&BookWorld->Cars31Replacements.Num()==20);
            bool Assets=true,Metric=true,Lods=true,Materials=true;int Count=0;
            for(auto V:Array(WardEchoAssets::Data(),TEXT("meshes")))
            {
                const Json R=V->AsObject();const int Id=Number(R,TEXT("id"));if(!((Id>=1000&&Id<=1131)||(Id>=2000&&Id<=2011)||(Id>=3000&&Id<=3031)))continue;++Count;auto* M=WardEchoAssets::Mesh(Id);Assets&=M!=nullptr;if(!M)continue;
                const auto& B=Array(R,TEXT("bounds_cm"));Metric&=B.Num()==2&&M->GetBoundingBox().Min.Equals(Vector(B[0]->AsArray()),.5)&&M->GetBoundingBox().Max.Equals(Vector(B[1]->AsArray()),.5);
                Lods&=Number(R,TEXT("lods"))>=1;for(const auto& Mat:M->GetStaticMaterials())Materials&=Mat.MaterialInterface!=nullptr;
            }
            Check(TEXT("v31_all_vehicle_meshes_load_with_metric_bounds"),Count==Number(WardEchoAssets::Cars31(),TEXT("native_meshes"))&&Assets&&Metric);Check(TEXT("v31_vehicle_materials_and_lods_exist"),Materials&&Lods);
            bool Separate=true;for(int I=0;I<BookWorld->Cars31Bounds.Num();++I)for(int J=I+1;J<BookWorld->Cars31Bounds.Num();++J)Separate&=!BookWorld->Cars31Bounds[I].Intersect(BookWorld->Cars31Bounds[J]);Check(TEXT("v31_parked_vehicle_envelopes_do_not_intersect"),Separate);
            Check(TEXT("v33_all_35_vehicles_have_four_supported_wheel_points"),Boolean(BookWorld->VehicleAudit33,TEXT("success"))&&Array(BookWorld->VehicleAudit33,TEXT("cars")).Num()==35);
            Check(TEXT("v33_surface_fitting_rejects_walls_furniture_and_lawns"),Boolean(BookWorld->VehicleAudit33,TEXT("success")));
            Check(TEXT("v33_road_paint_is_submillimetre_and_attached_to_pavement"),Number(BookWorld->VehicleAudit33,TEXT("paint_count"))>100&&Number(BookWorld->VehicleAudit33,TEXT("paint_max_gap_cm"))<.25);

        }
        if(Number(WardBookMapData(),TEXT("review_revision"))>=19){Json Defaults=MakeShared<FJsonObject>();WardSettings::Complete(Defaults);Check(TEXT("horror_is_default_and_saved_inspection_preference_is_respected"),!Boolean(Defaults,TEXT("inspection_bright"),true)&&BookWorld->Bright==SettingBool(TEXT("inspection_bright"),false)&&Setting(TEXT("atmosphere_profile"))==19);}
        Options->SetBoolField(TEXT("inspection_bright"),false);ApplySettings(false);
        Check(TEXT("dim_mode_remains_available"),!BookWorld->Bright&&BookWorld->Daylight->Intensity<200);
        if(Number(WardEchoAssets::Cars31(),TEXT("version"))>=31)
        {
            Check(TEXT("v31_all_supplied_sources_placed_with_hero"),BookWorld->Cars31Count==35&&BookWorld->Cars31Sources.Num()==31&&BookWorld->Cars31Replacements.Num()==20);
            bool Assets=true,Metric=true,Lods=true,Materials=true;int Count=0;
            for(auto V:Array(WardEchoAssets::Data(),TEXT("meshes")))
            {
                const Json R=V->AsObject();const int Id=Number(R,TEXT("id"));if(!((Id>=1000&&Id<=1131)||(Id>=2000&&Id<=2011)||(Id>=3000&&Id<=3031)))continue;++Count;auto* M=WardEchoAssets::Mesh(Id);Assets&=M!=nullptr;if(!M)continue;
                const auto& B=Array(R,TEXT("bounds_cm"));Metric&=B.Num()==2&&M->GetBoundingBox().Min.Equals(Vector(B[0]->AsArray()),.5)&&M->GetBoundingBox().Max.Equals(Vector(B[1]->AsArray()),.5);
                Lods&=Number(R,TEXT("lods"))>=1;for(const auto& Mat:M->GetStaticMaterials())Materials&=Mat.MaterialInterface!=nullptr;
            }
            Check(TEXT("v31_all_vehicle_meshes_load_with_metric_bounds"),Count==Number(WardEchoAssets::Cars31(),TEXT("native_meshes"))&&Assets&&Metric);Check(TEXT("v31_vehicle_materials_and_lods_exist"),Materials&&Lods);
            bool Separate=true;for(int I=0;I<BookWorld->Cars31Bounds.Num();++I)for(int J=I+1;J<BookWorld->Cars31Bounds.Num();++J)Separate&=!BookWorld->Cars31Bounds[I].Intersect(BookWorld->Cars31Bounds[J]);Check(TEXT("v31_parked_vehicle_envelopes_do_not_intersect"),Separate);
        }
        if(Number(WardBookMapData(),TEXT("review_revision"))>=19)
        {
            ToggleAdmin();
            const bool TorchOn=H->Flashlight->IsVisible();bool Inherited=false;for(int I=0;I<H->InputComponent->GetNumActionBindings();++I)Inherited|=H->InputComponent->GetActionBinding(I).GetActionName()==TEXT("Flashlight");
            auto Press=[&](FKey Key){for(auto& B:H->InputComponent->KeyBindings)if(B.Chord.Key==Key&&B.KeyEvent==IE_Pressed)B.KeyDelegate.Execute(Key);};
            Press(EKeys::B);const bool BrightOn=BookWorld->Bright&&H->Flashlight->IsVisible()==TorchOn;Press(EKeys::B);Press(EKeys::N);const bool Off=!H->Flashlight->IsVisible()&&!BookWorld->Bright;Press(EKeys::N);
            Check(TEXT("inspection_and_torch_keys_are_independent_in_actual_input_bindings"),TorchOn&&!Inherited&&BrightOn&&Off&&H->Flashlight->IsVisible()&&!BookWorld->Bright&&!Interface->Root.IsValid());
            ToggleAdmin();
            H->UpdateTorchTransform();const FVector TorchOffset=H->Flashlight->GetComponentLocation()-H->GetActorLocation();
            const FVector EyeTorch=H->Flashlight->GetComponentLocation()-H->Camera->GetComponentLocation();
            FHitResult TorchHit;FCollisionQueryParams TorchQ(SCENE_QUERY_STAT(FirstPersonTorch52),false,H);
            Json TorchDiagnostic=MakeShared<FJsonObject>();TorchDiagnostic->SetBoolField(TEXT("visible"),H->Flashlight->IsVisible());TorchDiagnostic->SetNumberField(TEXT("actor_distance"),TorchOffset.Size());TorchDiagnostic->SetNumberField(TEXT("eye_distance"),EyeTorch.Size());TorchDiagnostic->SetNumberField(TEXT("forward_dot"),FVector::DotProduct(EyeTorch,PC->GetControlRotation().Vector()));TorchDiagnostic->SetNumberField(TEXT("intensity"),H->Flashlight->Intensity);TorchDiagnostic->SetNumberField(TEXT("cone"),H->Flashlight->OuterConeAngle);TorchDiagnostic->SetStringField(TEXT("actor"),H->GetActorLocation().ToString());TorchDiagnostic->SetStringField(TEXT("camera"),H->Camera->GetComponentLocation().ToString());TorchDiagnostic->SetStringField(TEXT("torch"),H->Flashlight->GetComponentLocation().ToString());TorchDiagnostic->SetBoolField(TEXT("blocked"),GetWorld()->LineTraceSingleByChannel(TorchHit,H->Camera->GetComponentLocation(),H->Flashlight->GetComponentLocation(),ECC_Visibility,TorchQ));TorchDiagnostic->SetStringField(TEXT("obstacle"),GetNameSafe(TorchHit.GetActor()));Write(FPaths::ProjectSavedDir()/TEXT("Prototype/torch_diagnostic52.json"),TorchDiagnostic);
            if(FParse::Param(FCommandLine::Get(),TEXT("WardTorchDiagnostic52"))){FPlatformMisc::RequestExit(false);return;}
            Check(TEXT("torch_starts_ahead_of_first_person_eye_with_bounded_beam"),H->Flashlight->IsVisible()&&TorchOffset.Size()<100&&EyeTorch.Size()<25&&FVector::DotProduct(EyeTorch,PC->GetControlRotation().Vector())>1&&H->Flashlight->Intensity<=4000&&H->Flashlight->OuterConeAngle<=45&&!GetWorld()->LineTraceSingleByChannel(TorchHit,H->Camera->GetComponentLocation(),H->Flashlight->GetComponentLocation(),ECC_Visibility,TorchQ));
            Place(BookWorld->Sites[0].Capture,BookWorld->Sites[0].View);BookWorld->LightSelectionClock=1;BookWorld->UpdateLighting(H->GetActorLocation());BookWorld->Update(.37f);
            Check(TEXT("horror_uses_moonlight_and_low_non_emissive_fill"),BookWorld->Daylight->Intensity<15&&H->Camera->PostProcessSettings.AmbientCubemapIntensity<.15f);
            bool Local=BookWorld->ActiveWhiteIds.Num()>0&&BookWorld->ActiveWhiteIds.Num()<=7;for(int I:BookWorld->ActiveWhiteIds)Local&=BookWorld->LightPool[I]->Intensity<=4800&&BookWorld->LightPool[I]->AttenuationRadius<=1150;
            Check(TEXT("horror_has_bounded_visible_working_lights_not_uniform_flood"),Local);
            int Mist=0;for(auto& F:BookWorld->FloorMist)if(F->IsVisible()){++Mist;const float Floor=FMath::RoundToInt((H->GetActorLocation().Z-96)/480)*480;Local&=FMath::Abs(F->GetRelativeLocation().Z-Floor+100)<1;}
            Check(TEXT("moving_floor_mist_is_bounded_and_on_current_storey"),Mist>0&&Mist<=5&&Local);
            int Failed=0;for(int I=0;I<23;++I)Failed+=BookWorld->LampPower(FVector(I*100,0,300),-1)==0;
            Check(TEXT("faulty_lamps_drop_briefly_while_other_circuits_stay_on"),Failed==3&&BookWorld->LampPower(FVector(2200,0,300),8.18f)<.1f&&BookWorld->LampPower(FVector(100,0,300),8.18f)>.9f);
            const int Before=BookWorld->AmbientEventCount;const float BeforeNoise=Noise;BookWorld->SoundRegion=BookWorld->LocationAt(H->GetActorLocation());BookWorld->SoundClock=0;BookWorld->Update(.01f);
            Check(TEXT("ambient_event_uses_distant_real_anchor_and_respects_cooldown"),BookWorld->AmbientEventCount==Before+1&&Effects.Contains(FName(*BookWorld->LastAmbientCue))&&FVector::Dist2D(BookWorld->LastAmbientAt,H->GetActorLocation())>450&&BookWorld->SoundClock>=24&&BookWorld->SoundClock<=42&&Noise==BeforeNoise);
            Place(BookWorld->Start(),FRotator(-8,90,0));
        }
        ToggleAdmin();
        Options->SetBoolField(TEXT("inspection_bright"),true);ApplySettings(false);
        Check(TEXT("bright_mode_lights_entire_map"),BookWorld->Bright&&BookWorld->Daylight->Intensity>500&&H->Camera->PostProcessSettings.AmbientCubemapIntensity>1);
        if(Number(WardEchoAssets::Cars31(),TEXT("version"))>=31)
        {
            Check(TEXT("v31_all_supplied_sources_placed_with_hero"),BookWorld->Cars31Count==35&&BookWorld->Cars31Sources.Num()==31&&BookWorld->Cars31Replacements.Num()==20);
            bool Assets=true,Metric=true,Lods=true,Materials=true;int Count=0;
            for(auto V:Array(WardEchoAssets::Data(),TEXT("meshes")))
            {
                const Json R=V->AsObject();const int Id=Number(R,TEXT("id"));if(!((Id>=1000&&Id<=1131)||(Id>=2000&&Id<=2011)||(Id>=3000&&Id<=3031)))continue;++Count;auto* M=WardEchoAssets::Mesh(Id);Assets&=M!=nullptr;if(!M)continue;
                const auto& B=Array(R,TEXT("bounds_cm"));Metric&=B.Num()==2&&M->GetBoundingBox().Min.Equals(Vector(B[0]->AsArray()),.5)&&M->GetBoundingBox().Max.Equals(Vector(B[1]->AsArray()),.5);
                Lods&=Number(R,TEXT("lods"))>=1;for(const auto& Mat:M->GetStaticMaterials())Materials&=Mat.MaterialInterface!=nullptr;
            }
            Check(TEXT("v31_all_vehicle_meshes_load_with_metric_bounds"),Count==Number(WardEchoAssets::Cars31(),TEXT("native_meshes"))&&Assets&&Metric);Check(TEXT("v31_vehicle_materials_and_lods_exist"),Materials&&Lods);
            bool Separate=true;for(int I=0;I<BookWorld->Cars31Bounds.Num();++I)for(int J=I+1;J<BookWorld->Cars31Bounds.Num();++J)Separate&=!BookWorld->Cars31Bounds[I].Intersect(BookWorld->Cars31Bounds[J]);Check(TEXT("v31_parked_vehicle_envelopes_do_not_intersect"),Separate);
        }
        if(Number(WardBookMapData(),TEXT("review_revision"))>=19){bool Hidden=true;for(auto& F:BookWorld->FloorMist)Hidden&=!F->IsVisible();Check(TEXT("bright_inspection_toggle_removes_horror_mist"),Hidden);}
        ToggleAdmin();
        Check(TEXT("inspection_world_has_no_enemies_or_pickups"),Objects.IsEmpty()&&Enemies.IsEmpty()&&Items.IsEmpty()&&H->Reserve==0&&H->Rounds==0&&!Flag(TEXT("weapon")));
        int ExpectedFlights=0;for(auto V:Array(WardBookMapData(),TEXT("stairs")))ExpectedFlights+=FMath::RoundToInt((Number(V->AsObject(),TEXT("z_max"))-Number(V->AsObject(),TEXT("z_min")))/4.8)*2;
        Check(TEXT("all_37_stair_cores_build_the_required_real_flights"),BookWorld->StairFlights==ExpectedFlights&&ExpectedFlights>100);
        bool DirectOnly=true;for(const auto& G:BookWorld->Groups)DirectOnly&=!G.Value->bAffectDynamicIndirectLighting&&!G.Value->bAffectDistanceFieldLighting;
        Check(TEXT("full_architecture_does_not_overfill_shared_lumen_cache"),DirectOnly);
        const Json Furnishing=Object(WardBookMapData(),TEXT("furnishing_audit"));
        Check(TEXT("all_10978_numbered_new_props_are_built_without_omissions"),BookWorld->PrototypeProps==10978&&BookWorld->PrototypeProps==Number(Furnishing,TEXT("scheduled_new"))&&Number(Furnishing,TEXT("unplaced"))==0&&BookWorld->PrototypeParts>40000);
        if(Boolean(WardEchoAssets::Data(),TEXT("success")))
        {
            Check(TEXT("authored_custom_models_replace_furniture_with_valid_metric_bounds"),BookWorld->CustomFurniture>3000&&BookWorld->CustomModelIds.Num()>35&&BookWorld->CustomBoundsFailures==0);
            Check(TEXT("all_real_door_leaves_use_custom_hinged_meshes"),BookWorld->CustomDoorLeaves==BookWorld->VisibleDoorLeaves&&BookWorld->CustomDoorLeaves>1000);
            UTexture* Upper=nullptr;UTexture* Lower=nullptr;
            Cast<UMaterialInstanceDynamic>(BookWorld->SurfaceMaterial(1))->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Albedo")),Upper);
            Cast<UMaterialInstanceDynamic>(BookWorld->SurfaceMaterial(2))->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Albedo")),Lower);
            Check(TEXT("former_title_grey_plaster_and_paint_replace_green_peeling_walls"),Upper&&Lower&&Upper->GetName().EndsWith(TEXT("a2f08e6cd5305c97"))&&Lower->GetName().EndsWith(TEXT("a569e90eb94884bd")));
            Check(TEXT("every_scheduled_furnishing_has_a_native_mesh_instead_of_visible_prototype_boxes"),BookWorld->UnresolvedCustomProps==0&&BookWorld->CustomFurniture>=Array(WardBookMapData(),TEXT("prototype_props")).Num()+Array(WardBookMapData(),TEXT("environment_props")).Num());
            if(Number(WardEchoAssets::Quality(),TEXT("version"))>=22)
            {
                const Json Counts=Object(WardEchoAssets::Quality(),TEXT("counts"));
                Check(TEXT("quality_layer_builds_all_refined_and_new_supported_objects"),BookWorld->QualityProps==Array(WardEchoAssets::Quality(),TEXT("props")).Num()&&BookWorld->QualityProps>25000&&BookWorld->CustomBoundsFailures==0);
                Check(TEXT("forest_sphere_crowns_replaced_by_native_branch_foliage"),BookWorld->QualityTrees==Number(Counts,TEXT("trees"))&&BookWorld->QualityTrees>3900);
                Check(TEXT("external_windows_have_relief_and_bounded_warm_light_pool"),BookWorld->QualityWindows==Number(Counts,TEXT("windows"))&&BookWorld->WarmPool.Num()==12);
            }
            if(Number(WardEchoAssets::RealArt(),TEXT("version"))>=23)
            {
                const Json Art=WardEchoAssets::RealArt(),Aliases=Object(Art,TEXT("mesh_aliases"));bool Ready=Aliases->Values.Num()>50;
                for(const auto& Pair:Aliases->Values){auto* M=WardEchoAssets::Mesh(FCString::Atoi(*Pair.Key));Ready&=M&&M->GetName()==FString::Printf(TEXT("SM_EH_%03d"),FMath::RoundToInt(Number(Object(WardEchoAssets::Realism30(),TEXT("mesh_aliases")),FString(*Pair.Key),Pair.Value->AsNumber())));}
                Check(TEXT("licensed_detailed_replacements_resolve_to_all_expected_native_assets"),Ready);
                Check(TEXT("all_additional_real_belongings_build_with_valid_metric_fitting"),BookWorld->RealProps==Array(Art,TEXT("props")).Num()&&BookWorld->RealProps>15000&&BookWorld->CustomBoundsFailures==0);
                auto* Tree=WardEchoAssets::Mesh(641);auto* Grass=WardEchoAssets::Mesh(501);bool RecordedLods=false;
                for(auto V:Array(WardEchoAssets::Data(),TEXT("meshes")))if(Number(V->AsObject(),TEXT("id"))==WardEchoAssets::ResolvedId(641))RecordedLods=Number(V->AsObject(),TEXT("lods"))>=4;
                // NullRHI does not initialise static-mesh render LOD data.
                // Require the editor-import result there and actual LODs in
                // the rendered capture, while checking the same alpha asset.
                Check(TEXT("detailed_foliage_uses_lods_and_real_alpha_cutouts"),Tree&&RecordedLods&&(!FApp::CanEverRender()||Tree->GetNumLODs()>=4)&&Grass&&Grass->GetMaterial(0)&&Grass->GetMaterial(0)->GetBlendMode()==BLEND_Masked);
                Check(TEXT("new_floor_belongings_keep_their_matching_solid_colliders"),BookWorld->RealSolidProps==Number(Object(Art,TEXT("counts")),TEXT("floor_belonging"))&&BookWorld->RealSolidProps>0);
            }
        }
        if(Number(WardEchoAssets::DetailArt(),TEXT("version"))>=26)
        {
            const Json D=WardEchoAssets::DetailArt();
            Check(TEXT("all_additional_architectural_fittings_build_at_valid_metric_bounds"),BookWorld->DetailProps==Array(D,TEXT("props")).Num()&&BookWorld->DetailProps>5000&&BookWorld->CustomBoundsFailures==0);
            Check(TEXT("medical_electrical_and_ventilation_fittings_cover_the_campus"),BookWorld->DetailWallProps>2000);
            Check(TEXT("fixed_opening_hardware_is_present_without_new_blocking_colliders"),BookWorld->DetailOpeningProps>3500&&BookWorld->DetailExteriorProps>=10);
            UTexture* FloorTexture=nullptr;auto* Surface=Cast<UMaterialInstanceDynamic>(BookWorld->SurfaceMaterial(0));if(Surface)Surface->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Albedo")),FloorTexture);
            Check(TEXT("new_world_projected_architecture_materials_are_actually_bound"),Surface&&Surface->Parent&&Surface->Parent->GetName()==TEXT("MI_Floor")&&FloorTexture&&FloorTexture->GetName()==TEXT("T_D26_5964baf57bcf1a38"));
        }
        if(Number(WardEchoAssets::DetailArt27(),TEXT("version"))>=27)
        {
            const Json D=WardEchoAssets::DetailArt27();
            Check(TEXT("v27_themed_details_have_valid_metric_bounds"),BookWorld->Detail27Props==Array(D,TEXT("props")).Num()&&BookWorld->Detail27Props>1000&&BookWorld->CustomBoundsFailures==0);
            Check(TEXT("v27_supplies_rest_on_measured_furniture_surfaces"),BookWorld->Detail27Supported>500);
            Check(TEXT("v27_overhead_services_and_hygiene_equipment_are_present"),BookWorld->Detail27Services>200&&BookWorld->Detail27Wall>50);
            bool Native=true;for(int Id=900;Id<=914;++Id)Native&=WardEchoAssets::Mesh(Id)!=nullptr;
            Check(TEXT("v27_fifteen_detailed_native_models_and_clinical_replacements_load"),Native&&WardEchoAssets::ResolvedId(200)==900&&WardEchoAssets::ResolvedId(196)==901&&WardEchoAssets::ResolvedId(230)==906&&WardEchoAssets::ResolvedId(231)==914);
        }
        if(Number(WardEchoAssets::HorrorArt28(),TEXT("version"))>=28)
        {
            const Json D=WardEchoAssets::HorrorArt28();
            Check(TEXT("v28_decay_traces_build_without_changing_route_collisions"),BookWorld->HorrorTraces28==Array(D,TEXT("traces")).Num()&&BookWorld->HorrorTraces28>500);
            Check(TEXT("v28_emergency_markers_fit_existing_architecture"),BookWorld->HorrorMarkers28==Array(D,TEXT("emergency_markers")).Num()&&BookWorld->HorrorMarkers28>0&&BookWorld->CustomBoundsFailures==0);
            Check(TEXT("v28_near_player_fill_is_soft_local_and_not_a_floodlight"),BookWorld->HorrorBounce28&&BookWorld->HorrorBounce28->Intensity<=250&&BookWorld->HorrorBounce28->AttenuationRadius<=350&&!BookWorld->HorrorBounce28->CastShadows);
            auto* Surface=Cast<UMaterialInstanceDynamic>(BookWorld->SurfaceMaterial(0));
            const Json Environment37=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/hospital_environment_v37.json"));
            const float ExpectedWetness=Number(Environment37,TEXT("version"))==37?Number(Environment37,TEXT("base_floor_wetness")):.58f;
            Check(TEXT("v28_weathered_pbr_architecture_is_actually_bound"),Surface&&Surface->Parent&&Surface->Parent->GetPathName().Contains(TEXT("HorrorV28"))&&FMath::IsNearlyEqual(Surface->K2_GetScalarParameterValue(TEXT("Wetness")),ExpectedWetness,.001f));
            if(Number(Environment37,TEXT("version"))==37)
            {
                bool Water=false;for(auto V37:Array(Environment37,TEXT("traces")))
                {Json T37=V37->AsObject();if(String(T37,TEXT("kind"))!=TEXT("Water"))continue;auto* G37=BookWorld->Groups.FindRef(String(T37,TEXT("id")));Water|=G37&&G37->GetInstanceCount()>0&&G37->GetMaterial(0)&&G37->GetMaterial(0)->GetName()==TEXT("MI_Water");}
                Check(TEXT("v37_dry_floor_base_keeps_real_local_water_patches"),Water&&ExpectedWetness<.2f&&Surface&&Surface->K2_GetScalarParameterValue(TEXT("Roughness"))>=.85f);
            }
        }
        if(Number(WardEchoAssets::Realism30(),TEXT("version"))>=30)
        {
            bool Ready=true;for(int Id=920;Id<=926;++Id)Ready&=WardEchoAssets::Mesh(Id)!=nullptr;
            Check(TEXT("v30_verified_cc0_models_and_native_sedan_load"),Ready&&WardEchoAssets::ResolvedId(640)==920&&WardEchoAssets::ResolvedId(343)==921&&WardEchoAssets::ResolvedId(126)==923);
            Check(TEXT("v30_all_measured_service_props_build_inside_their_bounds"),BookWorld->Realism30Props==Array(WardEchoAssets::Realism30(),TEXT("props")).Num()&&BookWorld->Realism30Props>30&&BookWorld->CustomBoundsFailures==0);
            auto* Road=Cast<UMaterialInstanceDynamic>(BookWorld->SurfaceMaterial(7));
            Check(TEXT("v30_scanned_asphalt_is_actually_bound"),Road&&Road->Parent&&Road->Parent->GetPathName().Contains(TEXT("RealismV30/MI_Road30")));
            auto* Car=WardEchoAssets::Mesh(640);bool Lods=false;for(auto V:Array(WardEchoAssets::Data(),TEXT("meshes")))if(Number(V->AsObject(),TEXT("id"))==920)Lods=Number(V->AsObject(),TEXT("lods"))>=4;
            Check(TEXT("v30_sedan_keeps_metric_size_and_four_native_lods"),Car&&Car->GetBoundingBox().GetSize().X>480&&Car->GetBoundingBox().GetSize().X<490&&Lods&&(!FApp::CanEverRender()||Car->GetNumLODs()>=4&&Car->GetNumSections(0)<=10));
        }
        TSet<FString> FurnishedZones;for(auto V:Array(WardBookMapData(),TEXT("prototype_props")))FurnishedZones.Add(String(V->AsObject(),TEXT("zone")));
        Check(TEXT("all_562_zones_and_17_basements_have_scheduled_set_dressing"),FurnishedZones.Num()==579);
        const Json Environment=Object(WardBookMapData(),TEXT("environment_dressing"));
        Check(TEXT("supplemental_room_kits_build_with_original_schedule_intact"),BookWorld->EnvironmentProps==Array(WardBookMapData(),TEXT("environment_props")).Num()&&BookWorld->EnvironmentProps>100);
        if(Number(WardBookMapData(),TEXT("review_revision"))>=18)
        {
            Check(TEXT("scanned_furniture_replaces_every_scheduled_placeholder"),BookWorld->DetailedFurniture==Number(Object(WardBookMapData(),TEXT("visual_realism")),TEXT("replacement_count"))&&BookWorld->DetailedFurniture>1500);
            Check(TEXT("scanned_furniture_stays_inside_original_metric_bounds"),BookWorld->DetailedFurnitureBoundsFailures==0);
        }
        const int32 RemovedShrubs=Number(Environment,TEXT("runtime_removed_shrubs"));
        const int32 RemovedForecourtParts=Number(Environment,TEXT("runtime_removed_forecourt_parts"));
        const int32 ExpectedBoxes=Number(Environment,TEXT("runtime_box_count"),Array(Environment,TEXT("boxes")).Num())-RemovedShrubs-RemovedForecourtParts;
        Check(TEXT("campus_surfaces_and_attached_fixtures_are_built"),BookWorld->EnvironmentBoxes==ExpectedBoxes&&BookWorld->EnvironmentBoxes>2000);
        {
            FCollisionQueryParams FenceQuery(SCENE_QUERY_STAT(ForecourtFenceCheck),false,H);
            FHitResult Hit;
            const FVector RoadSide(3400,-5080,165),HospitalSide(3400,-4920,165);
            const bool BlocksCharacter=GetWorld()->LineTraceSingleByChannel(Hit,RoadSide,HospitalSide,ECC_Pawn,FenceQuery);
            const FString Body=GetNameSafe(Hit.GetComponent());
            const bool OpenView=!GetWorld()->LineTraceSingleByChannel(Hit,RoadSide,HospitalSide,ECC_Visibility,FenceQuery);
            const FString SightObstruction=GetNameSafe(Hit.GetComponent());
            const bool ClearAbove=!GetWorld()->LineTraceSingleByChannel(Hit,RoadSide+FVector(0,0,240),HospitalSide+FVector(0,0,240),ECC_Pawn,FenceQuery);
            UE_LOG(LogTemp,Display,TEXT("WARD90 fence test body=%s sight=%s above=%s hit=(%s)"),
                *Body,*SightObstruction,*GetNameSafe(Hit.GetComponent()),*Hit.ImpactPoint.ToString());
            Check(TEXT("forecourt_fence_has_open_sightline_and_real_boundary_collision"),
                Number(Environment,TEXT("runtime_open_fence_sections"))>70&&BlocksCharacter&&OpenView&&ClearAbove);
            const FVector GateRoad(5000,-5120,100),GateHospital(5000,-4880,100);
            const bool ClearGate=!GetWorld()->SweepSingleByChannel(Hit,GateRoad,GateHospital,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,90),FenceQuery);
            Check(TEXT("forecourt_main_gate_remains_walkable"),ClearGate);
        }
        Check(TEXT("visible_moon_has_no_collision_and_sits_inside_sky"),BookWorld->VisibleMoon&&BookWorld->VisibleMoon->GetStaticMesh()&&BookWorld->VisibleMoon->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&BookWorld->VisibleMoon->GetRelativeLocation().Size()<195000);
        Check(TEXT("surveillance_camera_housings_cover_departments"),BookWorld->EnvironmentCameras>=100);
        Check(TEXT("outdoor_and_room_fixtures_supply_local_light_anchors"),Array(Environment,TEXT("white_lights")).Num()>500&&BookWorld->LightAnchors.Num()>1500);
        const float BeforePulse=BookWorld->EmergencyPool.Num()?BookWorld->EmergencyPool[0]->Intensity:0;BookWorld->Update(.37f);
        Check(TEXT("red_emergency_light_pulses_in_real_runtime"),BookWorld->EmergencyPool.Num()==4&&BookWorld->EmergencyAnchors.Num()>100&&BookWorld->EmergencyPool[0]->Intensity>0&&!FMath::IsNearlyEqual(BeforePulse,BookWorld->EmergencyPool[0]->Intensity));
        if(GalleryZ>0)
        {
            Check(TEXT("working_doors_have_visible_split_leaves_in_inspection"),BookWorld->VisibleDoorLeaves>1000&&BookWorld->DoorVisualGroups.Num()>3);
            bool Swing=false;
            for(auto& P:BookWorld->Portals)if(!P.VisibleParts.IsEmpty())
            {
                const auto Part=P.VisibleParts[0];FTransform Open,Closed;
                BookWorld->DoorVisualGroups[Part.Material]->GetInstanceTransform(Part.Index,Open);
                P.Open=false;BookWorld->RefreshPortalVisual(P);BookWorld->DoorVisualGroups[Part.Material]->GetInstanceTransform(Part.Index,Closed);
                Swing=!Open.GetRotation().Equals(Closed.GetRotation(),.01)&&P.Leaf->GetCollisionEnabled()!=ECollisionEnabled::NoCollision&&BookWorld->DoorVisualGroups[Part.Material]->IsVisible();
                P.Open=true;BookWorld->RefreshPortalVisual(P);break;
            }
            Check(TEXT("door_visual_panels_really_swing_and_closed_leaf_blocks"),Swing);
            Check(TEXT("tall_forest_is_built_from_real_instanced_tree_geometry"),BookWorld->ForestTrees==Number(Object(WardBookMapData(),TEXT("campus_upgrade")),TEXT("tree_count"))&&BookWorld->ForestTrees>1500);
            FCollisionQueryParams BQ(SCENE_QUERY_STAT(BridgeClearance),false,H);FHitResult BH;
            Check(TEXT("elevated_gallery_leaves_vehicle_height_clearance_below"),!GetWorld()->SweepSingleByChannel(BH,FVector(4600,11100,200),FVector(5400,11100,200),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(80,190),BQ));
        }
        Json Envelope=BookWorld->AuditEnvelope();
        if(Number(WardBookMapData(),TEXT("review_revision"))>=17)
        {
            bool Raised=true;for(auto V:Array(WardBookMapData(),TEXT("raised_access"))){Json R=V->AsObject();const auto& A=Array(R,TEXT("rect"));Raised&=BookWorld->ValidPosition(FVector((A[0]->AsNumber()+A[2]->AsNumber()*.5)*100,(A[1]->AsNumber()+A[3]->AsNumber()*.5)*100,Number(R,TEXT("z"))*100+98));}
            Check(TEXT("guard_platform_and_stair_save_positions_are_valid"),Raised&&Array(WardBookMapData(),TEXT("raised_access")).Num()==82);
            Check(TEXT("unsupported_air_above_guard_post_is_not_a_valid_save"),!BookWorld->ValidPosition(FVector(7000,-4750,1098)));
        }
        if(Number(WardBookMapData(),TEXT("review_revision"))>=16)
        {
            Json Doors=BookWorld->AuditEveryDoor();Write(FPaths::ProjectSavedDir()/TEXT("Prototype/every_door_check.json"),Doors);
            Check(TEXT("every_door_closes_blocks_opens_and_restores_state"),Boolean(Doors,TEXT("success")));
            bool Rear=true;FCollisionQueryParams RQ(SCENE_QUERY_STAT(RearReturn),false,H);FHitResult RH;
            for(auto V:Array(Object(WardBookMapData(),TEXT("realism_upgrade")),TEXT("rear_connections")))
            {
                const auto A=V->AsArray();const float X=A[0]->AsNumber()*100,Y=(A[1]->AsNumber()+A[3]->AsNumber()*.5)*100,W=A[2]->AsNumber()*100;
                Rear&=!GetWorld()->SweepSingleByChannel(RH,FVector(X+60,Y,578),FVector(X+W-60,Y,578),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),RQ);
                for(float U=0;U<=1;U+=.05f){const FVector P(X+U*W,Y,578);Rear&=GetWorld()->LineTraceSingleByChannel(RH,P,P-FVector(0,0,180),ECC_Visibility,RQ)&&FMath::Abs(RH.ImpactPoint.Z-480)<2;}
            }
            Check(TEXT("rear_return_galleries_have_real_floor_and_clear_headroom"),Rear);
            int Supported=0;for(auto V:Array(Object(WardBookMapData(),TEXT("realism_upgrade")),TEXT("support_frames")))++Supported;
            Check(TEXT("gallery_structure_has_ninety_or_more_support_frames"),Supported>=90);
        }
        for(const FString Kind:{TEXT("floors"),TEXT("ceilings"),TEXT("storey_bands"),TEXT("outer_walls"),TEXT("shaft_roof")})Check(TEXT("complete_envelope_")+Kind,Number(Object(Envelope,TEXT("tested")),Kind)>0&&Number(Object(Envelope,TEXT("failed")),Kind)==0);
        // Check collision at every room centre, through every entrance, and against
        // each solid side wall. Sweeps use the real player's standing capsule.
        int Floors=0,Doors=0,Walls=0,Enclosed=0;TArray<TSharedPtr<FJsonValue>> Bad;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(BookWorldGeometry),false,H);
        const auto Shape=FCollisionShape::MakeCapsule(H->GetCapsuleComponent()->GetScaledCapsuleRadius(),H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-2);
        for(const auto& R:BookWorld->Rooms)
        {
            if(!R.Enclosed)continue;++Enclosed;
            const FVector Centre(R.Bounds.GetCenter().X,R.Bounds.GetCenter().Y,R.Bounds.Min.Z+98);FHitResult Hit;
            if(GetWorld()->LineTraceSingleByChannel(Hit,Centre,Centre-FVector(0,0,180),ECC_Visibility,Q)&&FMath::Abs(Hit.ImpactPoint.Z-R.Bounds.Min.Z)<2)++Floors;
            const bool Clear=!GetWorld()->SweepSingleByChannel(Hit,R.Outside,R.Inside,FQuat::Identity,ECC_Pawn,Shape,Q)&&!GetWorld()->SweepSingleByChannel(Hit,R.Inside,Centre,FQuat::Identity,ECC_Pawn,Shape,Q);
            if(Clear)++Doors;else Bad.Add(MakeShared<FJsonValueString>(R.Id));
            const FVector Wall(R.Bounds.Min.X,R.Bounds.GetCenter().Y,R.Bounds.Min.Z+75);
            if(GetWorld()->LineTraceSingleByChannel(Hit,FVector(Centre.X,Centre.Y,Wall.Z),Wall-FVector(30,0,0),ECC_Visibility,Q))++Walls;
        }
        Check(TEXT("all_enclosed_rooms_have_solid_floors"),Floors==Enclosed);Check(TEXT("all_enclosed_room_entrances_fit_standing_character"),Doors==Enclosed);Check(TEXT("all_enclosed_rooms_have_blocking_side_walls"),Walls==Enclosed);
        Json Diagnostics=MakeShared<FJsonObject>();Diagnostics->SetNumberField(TEXT("floors"),Floors);Diagnostics->SetNumberField(TEXT("doors"),Doors);Diagnostics->SetNumberField(TEXT("walls"),Walls);Diagnostics->SetArrayField(TEXT("blocked_doors"),Bad);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/book_world_geometry.json"),Diagnostics);
        Check(TEXT("story_starts_at_parking_S01_on_valid_floor"),BookWorld->ValidPosition(H->GetActorLocation())&&BookWorld->Describe(H->GetActorLocation()).Contains(TEXT("S01")));
        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardEnvelopeFirst")))
        {int I=BookWorld->Sites.Num();for(auto V:Array(WardBookMapData(),TEXT("furnishing_captures"))){if(Boolean(V->AsObject(),TEXT("envelope"))){ExteriorStart=I;Index=I;break;}++I;}}
        if(GeometryOnly){Next(3);return;}
        Place(FVector(-450,1000,-862),FRotator(-8,90,0));
        for(int Z:{-960,-480,0,480,960})for(FVector P:TArray<FVector>{FVector(3150,740,98),FVector(2878,740,98),FVector(2878,650,114),FVector(2878,270,338),FVector(2878,200,338),FVector(3022,200,338),FVector(3022,290,354),FVector(3022,660,578),FVector(3022,740,578),FVector(3150,740,578)})Route.Add(P+FVector(-3600,0,Z));Next(FParse::Param(FCommandLine::Get(),TEXT("WardHospitalPreview"))?4:1);
    }
    else if(Step==1)
    {
        if(Waypoint<Route.Num())
        {
            const FVector Goal=Route[Waypoint],Diff=Goal-H->GetActorLocation();if(Diff.Size2D()<24&&FMath::Abs(Diff.Z)<38)++Waypoint;
            else H->AddMovementInput(Diff.GetSafeNormal2D(),1);
        }
        if(Waypoint==Route.Num()||Clock>180){UE_LOG(LogTemp,Display,TEXT("BOOK_STAIRS waypoint=%d position=%s"),Waypoint,*H->GetActorLocation().ToString());Check(TEXT("character_walks_all_ten_flights_from_B2_to_roof_without_teleport"),Waypoint==Route.Num()&&H->GetActorLocation().Z>1500&&H->GetCharacterMovement()->IsMovingOnGround());for(int I=0;I<Route.Num()/2;++I)Route.Swap(I,Route.Num()-1-I);Waypoint=0;Next(10);}
    }
    else if(Step==10)
    {
        if(Waypoint<Route.Num()){const FVector Diff=Route[Waypoint]-H->GetActorLocation();if(Diff.Size2D()<24&&FMath::Abs(Diff.Z)<38)++Waypoint;else H->AddMovementInput(Diff.GetSafeNormal2D(),1);}
        if(Waypoint==Route.Num()||Clock>180){UE_LOG(LogTemp,Display,TEXT("BOOK_STAIRS_DOWN waypoint=%d position=%s"),Waypoint,*H->GetActorLocation().ToString());Check(TEXT("character_walks_all_ten_flights_back_down_to_B2"),Waypoint==Route.Num()&&H->GetActorLocation().Z<-800&&H->GetCharacterMovement()->IsMovingOnGround());ExtraCore=0;Next(30);}
    }
    else if(Step==30)
    {
        Route.Reset();Waypoint=0;const Json Stair=Array(WardBookMapData(),TEXT("stairs"))[ExtraCore]->AsObject();
        const auto& R=Array(Stair,TEXT("rect"));const FVector Offset(R[0]->AsNumber()*100-2800,R[1]->AsNumber()*100,0);
        Place(FVector(3150,850,Number(Stair,TEXT("z_min"))*100+98)+Offset,FRotator(0,-90,0));
        for(int Z=FMath::RoundToInt(Number(Stair,TEXT("z_min"))*100);Z<Number(Stair,TEXT("z_max"))*100;Z+=480)
        for(FVector P:TArray<FVector>{FVector(3150,740,98),FVector(2878,740,98),FVector(2878,650,114),FVector(2878,270,338),FVector(2878,200,338),FVector(3022,200,338),FVector(3022,290,354),FVector(3022,660,578),FVector(3022,740,578),FVector(3150,740,578)})Route.Add(P+Offset+FVector(0,0,Z));
        Next(31);
    }
    else if(Step==31)
    {
        if(Waypoint<Route.Num()){const FVector Diff=Route[Waypoint]-H->GetActorLocation();if(Diff.Size2D()<24&&FMath::Abs(Diff.Z)<38)++Waypoint;else H->AddMovementInput(Diff.GetSafeNormal2D(),1);}
        if(Waypoint==Route.Num()||Clock>150)
        {
            Check(FString::Printf(TEXT("character_walks_additional_stair_%d"),ExtraCore),Waypoint==Route.Num()&&H->GetCharacterMovement()->IsMovingOnGround());
            if(++ExtraCore<Array(WardBookMapData(),TEXT("stairs")).Num())Next(30);else {Place(FVector(4850,4100,GalleryZ+98),FRotator(-8,90,0));Route={FVector(4850,9900,GalleryZ+98),FVector(5000,10500,GalleryZ+98),FVector(5000,16050,GalleryZ+98),FVector(15150,16050,GalleryZ+98),FVector(19850,16050,GalleryZ+98)};Waypoint=0;Next(2);}
        }
    }
    else if(Step==2)
    {
        if(Waypoint<Route.Num()){const FVector Diff=Route[Waypoint]-H->GetActorLocation();if(Diff.Size2D()<28)++Waypoint;else H->AddMovementInput(Diff.GetSafeNormal2D(),1);}
        if(Waypoint==Route.Num()||Clock>150){Check(TEXT("character_walks_gallery_into_rehabilitation_building"),Waypoint==Route.Num()&&BookWorld->LocationAt(H->GetActorLocation())==(GalleryZ>0?TEXT("RH2"):TEXT("RH")));Place(FVector(5000,1220,98),FRotator(0,-90,0));Route={FVector(5000,700,98)};Waypoint=0;Next(20);}
    }
    else if(Step==20)
    {
        const FVector Diff=FVector(5000,700,98)-H->GetActorLocation();
        if(Diff.Size2D()>24)H->AddMovementInput(Diff.GetSafeNormal2D(),1);
        else {H->GetCharacterMovement()->StopMovementImmediately();Check(TEXT("character_walks_through_open_lift_doors"),BookWorld->InsideLift(H->GetActorLocation()));Check(TEXT("lift_accepts_roof_request"),BookWorld->RequestLift(5));Next(21);}
        if(Clock>12){Check(TEXT("character_walks_through_open_lift_doors"),false);Next(3);}
    }
    else if(Step==21)
    {
        BookWorld->Update(Delta);
        if(!BookWorld->LiftMoving&&Clock>1){Check(TEXT("lift_physically_carries_character_to_roof"),BookWorld->LiftFloor==5&&FMath::Abs(H->GetActorLocation().Z-1538)<8);BookWorld->RequestLift(0);Next(22);}
        if(Clock>20){Check(TEXT("lift_physically_carries_character_to_roof"),false);Next(3);}
    }
    else if(Step==22)
    {
        BookWorld->Update(Delta);
        if(!BookWorld->LiftMoving&&Clock>1){Check(TEXT("lift_physically_carries_character_to_B2"),BookWorld->LiftFloor==0&&FMath::Abs(H->GetActorLocation().Z+862)<8);Next(3);}
        if(Clock>25){Check(TEXT("lift_physically_carries_character_to_B2"),false);Next(3);}
    }
    else if(Step==3&&Clock>.2)
    {
        FCollisionQueryParams Q(SCENE_QUERY_STAT(BookWorldCorridors),false,H);FHitResult Hit;bool Clear=true;bool Areas=true;
        for(const auto& R:BookWorld->Rooms)
        {
            Areas&=R.Bounds.GetSize().X*R.Bounds.GetSize().Y/10000>=R.MinimumArea;
            bool Compact=false;for(auto V:Array(WardBookMapData(),TEXT("locations")))if(String(V->AsObject(),TEXT("id"))==R.Location)Compact=Boolean(V->AsObject(),TEXT("compact"));
            if(Compact||!R.Enclosed||R.Location==TEXT("ST")||R.Location==TEXT("RF")||R.Id==TEXT("RH00"))continue;
            FVector Origin;for(const auto& Site:BookWorld->Sites)if(Site.Id==R.Location){Origin=Site.Origin;break;}
            const FVector A=R.Outside;float CentreY=A.Y;
            for(float Y:{1900.f,4100.f,6500.f})if(FMath::Abs(A.Y-Origin.Y-Y)<500)CentreY=Origin.Y+Y;
            const FVector Porch(A.X,CentreY,A.Z),B(Origin.X+4850,CentreY,A.Z);
            const bool Good=!GetWorld()->SweepSingleByChannel(Hit,A,Porch,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,90),Q)&&!GetWorld()->SweepSingleByChannel(Hit,Porch,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,90),Q);
            if(!Good)UE_LOG(LogTemp,Display,TEXT("CORRIDOR_BLOCK %s At=%s"),*R.Id,*Hit.ImpactPoint.ToString());Clear&=Good;
        }
        Check(TEXT("every_original_room_keeps_or_increases_its_area"),Areas);
        Check(TEXT("department_corridors_are_walkable"),Clear);
        bool Spokes=true;
        for(int Y:{4100,16100,27100})for(int Side:{-1,1})
        {
            const FVector A(5000,Y,GalleryZ+98),B(Side<0?-4500:15150,Y,GalleryZ+98);
            Spokes&=!GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,90),Q);
            for(int I=0;I<=20;++I){const FVector P=FMath::Lerp(A,B,I/20.f);Spokes&=GetWorld()->LineTraceSingleByChannel(Hit,P,P-FVector(0,0,180),ECC_Visibility,Q)&&FMath::Abs(Hit.ImpactPoint.Z-GalleryZ)<2;}
        }
        Check(TEXT("six_buildings_have_open_gallery_connections"),Spokes);
        bool BackHall=true;
        for(int Z:{-960,-480,0,480,960,1440})
        {
            const float Y=Z==1440?8100.f:9800.f;const FVector A(-450,Y,Z+98),B(4850,Y,Z+98);
            const bool SweepClear=!GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,90),Q);BackHall&=SweepClear;if(!SweepClear)UE_LOG(LogTemp,Display,TEXT("BACK_HALL_BLOCK Z=%d At=%s Component=%s"),Z,*Hit.ImpactPoint.ToString(),*GetNameSafe(Hit.GetComponent()));
            for(int I=0;I<=30;++I){const FVector P=FMath::Lerp(A,B,I/30.f);const bool Floor=GetWorld()->LineTraceSingleByChannel(Hit,P,P-FVector(0,0,180),ECC_Visibility,Q)&&FMath::Abs(Hit.ImpactPoint.Z-Z)<2;BackHall&=Floor;if(!Floor)UE_LOG(LogTemp,Display,TEXT("BACK_HALL_FLOOR Z=%d P=%s Hit=%s"),Z,*P.ToString(),*Hit.ImpactPoint.ToString());}
        }
        Check(TEXT("stair_C_has_a_walkable_connection_to_main_core_on_every_level"),BackHall);
        // Independent raster routes are rechecked against actual HISM collision,
        // including the new partition ends, niches and all 334 destinations.
        int Routes=0;TArray<TSharedPtr<FJsonValue>> Blocked;
        for(auto V:Array(WardBookMapData(),TEXT("navigation_routes")))
        {
            Json R=V->AsObject();const auto& P=Array(R,TEXT("points"));bool Good=true;
            for(int I=1;I<P.Num();++I)
            {
                const FVector A=Vector(P[I-1]->AsArray())*100+FVector(0,0,98),B=Vector(P[I]->AsArray())*100+FVector(0,0,98);
                if(FMath::Abs(A.Z-B.Z)>10)continue; // Stair links are physically walked above.
                if(GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q))
                {Good=false;UE_LOG(LogTemp,Display,TEXT("STORY_ROUTE_BLOCK %s A=%s B=%s HIT=%s COMPONENT=%s"),*String(R,TEXT("zone")),*A.ToString(),*B.ToString(),*Hit.ImpactPoint.ToString(),*GetNameSafe(Hit.GetComponent()));break;}
            }
            if(Good)++Routes;else Blocked.Add(MakeShared<FJsonValueString>(String(R,TEXT("zone"))));
        }
        Check(TEXT("all_562_zone_routes_fit_real_character_capsule"),Routes==BookWorld->Rooms.Num());
        bool EscapeDoors=true;TArray<TSharedPtr<FJsonValue>> BadExits;
        for(const auto& P:BookWorld->Portals)if(!P.Role.IsEmpty()&&P.Role!=TEXT("niche"))
        {
            const FVector Normal=P.Side==TEXT("north")||P.Side==TEXT("south")?FVector(0,1,0):FVector(1,0,0);
            if(GetWorld()->SweepSingleByChannel(Hit,P.At-Normal*80+FVector(0,0,98),P.At+Normal*80+FVector(0,0,98),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q))
            {EscapeDoors=false;BadExits.Add(MakeShared<FJsonValueString>(P.Id));UE_LOG(LogTemp,Display,TEXT("ESCAPE_DOOR_BLOCK %s %s %s"),*P.Id,*Hit.ImpactPoint.ToString(),*HitInfo(Hit));}
        }
        Check(TEXT("secondary_and_secret_doorways_fit_player_capsule"),EscapeDoors);
        Json ExitDiagnostics=MakeShared<FJsonObject>();ExitDiagnostics->SetArrayField(TEXT("blocked"),BadExits);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/escape_doors_check.json"),ExitDiagnostics);
        Json Nav=MakeShared<FJsonObject>();Nav->SetNumberField(TEXT("routes_passed"),Routes);Nav->SetArrayField(TEXT("blocked"),Blocked);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/story_routes_collision.json"),Nav);
        int Hides=0,LargeBlocked=0;TArray<TSharedPtr<FJsonValue>> BadHides;
        for(auto V:Array(WardBookMapData(),TEXT("hideouts")))
        {
            Json N=V->AsObject();const FVector O=Vector(Array(N,TEXT("at")))*100,S=Vector(Array(N,TEXT("size")))*100;
            const bool Bed=String(N,TEXT("kind"))==TEXT("under_bed");const float Half=Bed?48:88,Z=Bed?51:98;
            const FVector A=O+FVector(S.X*.5,S.Y+50,Z),B=O+FVector(S.X*.5,Bed?S.Y*.55:60,Z);
            if(!GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,Half),Q))++Hides;else {BadHides.Add(MakeShared<FJsonValueString>(String(N,TEXT("id"))));UE_LOG(LogTemp,Display,TEXT("HIDE_BLOCK %s %s"),*String(N,TEXT("id")),*HitInfo(Hit));}
            if(GetWorld()->SweepSingleByChannel(Hit,FVector(A.X,A.Y,O.Z+110),FVector(B.X,B.Y,O.Z+110),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(55,100),Q))++LargeBlocked;
        }
        Check(TEXT("all_architectural_hideouts_admit_player_capsule"),Hides==Array(WardBookMapData(),TEXT("hideouts")).Num());
        Check(TEXT("ninety_centimetre_hideouts_block_large_capsule"),LargeBlocked==Hides);
        Json HideDiagnostics=MakeShared<FJsonObject>();HideDiagnostics->SetArrayField(TEXT("blocked"),BadHides);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/hideout_check.json"),HideDiagnostics);
        const FVector BeforeHide=H->GetActorLocation();bool FurnitureInteractions=true;
        for(const FString Kind:{FString(TEXT("wardrobe")),FString(TEXT("under_bed"))})
        {
            bool Found=false;
            for(auto V:Array(WardBookMapData(),TEXT("hideouts")))
            {
                Json N=V->AsObject();if(String(N,TEXT("kind"))!=Kind)continue;
                const FVector O=Vector(Array(N,TEXT("at")))*100,S=Vector(Array(N,TEXT("size")))*100;
                const FVector Entry=O+FVector(S.X*.5,S.Y+65,98);H->UnCrouch();H->GetCharacterMovement()->UnCrouch(false);
                Place(Entry,FRotator(0,-90,0));H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                const bool Enter=BookWorld->UseFurnitureHide(Entry)&&BookWorld->FurnitureHiding;
                const bool NoUnsafeSave=!Save(TEXT("manual_1"));
                const bool Leave=BookWorld->UseFurnitureHide(H->GetActorLocation())&&!BookWorld->FurnitureHiding;
                FurnitureInteractions&=Enter&&NoUnsafeSave&&Leave&&FVector::Dist(H->GetActorLocation(),Entry)<8;
                Found=true;break;
            }
            FurnitureInteractions&=Found;
        }
        BookWorld->FurnitureHiding=false;H->UnCrouch();H->GetCharacterMovement()->UnCrouch(false);Place(BeforeHide,FRotator(0,90,0));
        Check(TEXT("wardrobe_and_bed_hide_enter_exit_and_refuse_unsafe_save"),FurnitureInteractions);
        int NicheEntries=0;TArray<TSharedPtr<FJsonValue>> BadNiches;
        for(auto V:Array(WardBookMapData(),TEXT("escape_niches")))
        {
            Json N=V->AsObject();const FVector Entry=Vector(Array(N,TEXT("entry")))*100,C=Vector(Array(N,TEXT("corridor_point")))*100;
            const FString Side=String(N,TEXT("side"));const FVector Toward=Side==TEXT("north")?FVector(0,1,0):Side==TEXT("south")?FVector(0,-1,0):Side==TEXT("west")?FVector(1,0,0):FVector(-1,0,0);
            const FVector A=C+FVector(0,0,98),B=Entry+Toward*(Number(N,TEXT("entry_depth_m"),1)*100)+FVector(0,0,98),Approach=Entry-Toward*70+FVector(0,0,98);
            const FVector Aligned=Side==TEXT("north")||Side==TEXT("south")?FVector(Entry.X,A.Y,A.Z):FVector(A.X,Entry.Y,A.Z);
            if(!GetWorld()->SweepSingleByChannel(Hit,A,Aligned,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q)&&!GetWorld()->SweepSingleByChannel(Hit,Aligned,Approach,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q)&&!GetWorld()->SweepSingleByChannel(Hit,Approach,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Q))++NicheEntries;
            else{BadNiches.Add(MakeShared<FJsonValueString>(String(N,TEXT("id"))));UE_LOG(LogTemp,Display,TEXT("NICHE_BLOCK %s %s %s"),*String(N,TEXT("id")),*Hit.ImpactPoint.ToString(),*HitInfo(Hit));}
        }
        Check(TEXT("all_new_corridor_niches_admit_player_capsule"),NicheEntries==Array(WardBookMapData(),TEXT("escape_niches")).Num());
        Json NicheDiagnostics=MakeShared<FJsonObject>();NicheDiagnostics->SetArrayField(TEXT("blocked"),BadNiches);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/niche_check.json"),NicheDiagnostics);
        bool RefugePass=true;
        for(const FString Zone:{TEXT("IF08"),TEXT("RH08"),TEXT("DG08"),TEXT("SV23"),TEXT("HS08")})
        {
            FVector At;for(const auto& R:BookWorld->Rooms)if(R.Id==Zone)At=R.Inside;
            RefugePass&=BookWorld->ToggleBoltForZone(Zone,At)&&BookWorld->RefugeBolts.Contains(Zone);
            Json State=BookWorld->ArchitectureSnapshot();BookWorld->RefugeBolts.Reset();BookWorld->RestoreArchitecture(State);
            RefugePass&=BookWorld->RefugeBolts.Contains(Zone);
            for(const auto& P:BookWorld->Portals)if(P.Zone==Zone)RefugePass&=!P.Open&&P.Leaf->GetCollisionEnabled()!=ECollisionEnabled::NoCollision;
            RefugePass&=!BookWorld->ToggleBoltForZone(Zone,At+FVector(0,100000,0));
            RefugePass&=BookWorld->ToggleBoltForZone(Zone,At)&&!BookWorld->RefugeBolts.Contains(Zone);
        }
        Check(TEXT("five_requested_refuges_have_inside_bolts_and_save_restore"),RefugePass);
        bool Environments=true;for(int Chapter:{0,6,15,18}){BookWorld->SetStoryChapter(Chapter);Environments&=BookWorld->EnvironmentState==(Chapter==0?TEXT("clean"):Chapter==6?TEXT("infected"):Chapter==15?TEXT("damaged"):TEXT("collapsed"));}
        Check(TEXT("B2_clean_infected_damaged_collapsed_states_change_by_chapter"),Environments);BookWorld->SetStoryChapter(0);
        bool Props=BookWorld->Movables.Num()==6;
        for(int I=0;I<BookWorld->Movables.Num();++I)
        {
            auto& P=BookWorld->Movables[I];const FVector BeforeProp=P.Body->GetRelativeLocation();Noise=0;
            const bool WardCart=P.Id==TEXT("PROP_gurney_F202");
            const FVector Direction=WardCart?FVector::ForwardVector:FVector::RightVector;
            const bool Used=BookWorld->PushMovable(I,Direction);
            for (int Frame=0; Frame<32; ++Frame) WardContact98::Advance(*BookWorld,.033f);
            const FVector AfterProp=P.Body->GetRelativeLocation();
            UE_LOG(LogTemp,Display,TEXT("CART101 %s push=%d noise=%.2f start=%s end=%s"),*P.Id,Used,Noise,*BeforeProp.ToString(),*AfterProp.ToString());
            Props&=Used&&Noise>0&&(P.Pushable?FVector::DotProduct(AfterProp-BeforeProp,Direction)>11:AfterProp.Equals(BeforeProp,1));
            Json SavedProps=BookWorld->ArchitectureSnapshot();BookWorld->ResetMovables();BookWorld->RestoreArchitecture(SavedProps);Props&=P.Body->GetRelativeLocation().Equals(AfterProp,1);
        }
        Check(TEXT("pushable_props_and_bell_emit_noise_and_restore_positions"),Props);BookWorld->ResetMovables();
        bool SafePull=true;const FVector BeforePull=H->GetActorLocation();
        for(int I=0;I<BookWorld->Movables.Num();++I)
        {
            const auto& P=BookWorld->Movables[I];for(const auto& Child:P.Body->GetAttachChildren())if(auto* Mesh=Cast<UStaticMeshComponent>(Child.Get()))SafePull&=Mesh->GetStaticMesh()!=nullptr;
            if(!P.Pushable)continue;Place(P.Body->GetComponentLocation()+FVector(0,P.Half.Y+90,98-P.Half.Z),FRotator(0,-90,0));
            BookWorld->PushMovable(I,FVector(0,1,0));
            for (int Frame=0; Frame<32; ++Frame) WardContact98::Advance(*BookWorld,.033f);
            const double Gap = H->GetActorLocation().Y - P.Body->GetComponentLocation().Y - P.Half.Y;
            SafePull &= Gap >= H->GetCapsuleComponent()->GetScaledCapsuleRadius();
        }
        Place(BeforePull,FRotator(0,90,0));Check(TEXT("movable_models_exist_and_pulling_cannot_overlap_player"),SafePull);
        bool ExtraDoors=true;for(const auto& P:BookWorld->Portals)ExtraDoors&=P.Open&&P.Leaf&&P.Leaf->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
        Check(TEXT("inspection_opens_all_typed_doors_including_class_C"),ExtraDoors&&BookWorld->Portals.Num()>400);
        BookWorld->SetStoryChapter(17);int Closed=0;for(const auto& P:BookWorld->Portals)if(!P.Open&&P.Leaf->GetCollisionEnabled()!=ECollisionEnabled::NoCollision)++Closed;
        Json Architecture=BookWorld->ArchitectureSnapshot();BookWorld->SetStoryChapter(0);BookWorld->RestoreArchitecture(Architecture);
        int RestoredClosed=0;for(const auto& P:BookWorld->Portals)if(!P.Open)++RestoredClosed;
        Check(TEXT("chapter_zone_states_create_real_closed_doors_and_restore"),Closed>=80&&Closed==RestoredClosed&&BookWorld->StoryChapter==17);BookWorld->SetStoryChapter(0);
        Json Snapshot=Capture();Check(TEXT("new_world_save_has_separate_schema_and_valid_position"),ValidateSave(Snapshot)&&String(Snapshot,TEXT("world_id"))==TEXT("hospital_story_shell_v3")&&SaveDirectory().Contains(TEXT("HospitalStoryV3Saves")));
        const FVector Before=H->GetActorLocation();const bool Wrote=Save(TEXT("manual_1"));BookWorld->RestoreLift(FVector(5000,700,98));Place(BookWorld->Start(),FRotator(0,0,0));Check(TEXT("save_restores_3d_world_position"),Wrote&&Load(TEXT("manual_1"))&&H->GetActorLocation().Equals(Before,2));
        Json Old=MakeShared<FJsonObject>();Old->Values=Snapshot->Values;Old->RemoveField(TEXT("world_id"));Check(TEXT("legacy_save_is_not_silently_loaded_into_new_geometry"),!ValidateSave(Old));
        Command(TEXT("page_map"));Check(TEXT("full_2d_plan_remains_available_for_navigation"),Screen==TEXT("map")&&Interface->BookMapVisible&&PC->IsPaused());Command(TEXT("field_close"));Show(TEXT("pause"));Command(TEXT("resume"));Check(TEXT("pause_and_resume_keep_world_and_player"),Screen.IsEmpty()&&!PC->IsPaused()&&BookWorld&&H->GetActorLocation().Equals(Before,2));
        if(GeometryOnly){Next(6);return;}
        Place(FVector(4850,9800,-382),FRotator(0,90,0));Waypoint=0;
        Route={FVector(5000,9800,-382),FVector(5000,21600,-382),FVector(4200,21600,-382),FVector(4200,23600,-382),FVector(5000,23600,-382),FVector(5000,32800,-382)};
        for(auto V:Array(WardBookMapData(),TEXT("utility_links")))if(String(V->AsObject(),TEXT("id"))==TEXT("U_LINK_SV"))for(auto P:Array(V->AsObject(),TEXT("points")))Route.Add(Vector(P->AsArray())*100+FVector(0,0,98));Next(40);
    }
    else if(Step==40)
    {
        if(Waypoint<Route.Num()){const FVector Diff=Route[Waypoint]-H->GetActorLocation();if(Diff.Size2D()<28)++Waypoint;else H->AddMovementInput(Diff.GetSafeNormal2D(),1);}
        if(Waypoint==Route.Num()||Clock>170){Check(TEXT("character_walks_B1_tunnel_and_isolation_bypass_to_SV_basement"),Waypoint==Route.Num()&&H->GetActorLocation().Z<-300&&H->GetCharacterMovement()->IsMovingOnGround());Place(BookWorld->Start(),FRotator(-8,90,0));Next(Number(WardBookMapData(),TEXT("review_revision"))>=17?50:(Pictures?4:6));}
    }
    else if(Step==50)
    {
        const float X=GuardIndex==0?7000.f:22500.f;Place(FVector(X,-2700,98),FRotator(0,-90,0));Route.Reset();Waypoint=0;GuardDescending=false;
        Route.Add(FVector(X,-2760,98));
        for(int I=0;I<40;++I)Route.Add(FVector(X,(-46+.45*(39-I)+.225)*100,15*(I+1)+98));
        Route.Add(FVector(X,-4680,703));Next(51);
    }
    else if(Step==51)
    {
        if(Waypoint<Route.Num()){const FVector Diff=Route[Waypoint]-H->GetActorLocation();if(Diff.Size2D()<16&&FMath::Abs(Diff.Z)<28)++Waypoint;else H->AddMovementInput(Diff.GetSafeNormal2D(),1);}
        if(Waypoint==Route.Num()||Clock>60)
        {
            const bool Walked=Waypoint==Route.Num()&&H->GetCharacterMovement()->IsMovingOnGround();
            if(!Walked)
            {
                UE_LOG(LogTemp,Display,TEXT("GUARD_WALK_BLOCK post=%d descending=%d waypoint=%d position=%s target=%s mode=%d"),GuardIndex,GuardDescending?1:0,Waypoint,*H->GetActorLocation().ToString(),Waypoint<Route.Num()?*Route[Waypoint].ToString():TEXT("done"),(int)H->GetCharacterMovement()->MovementMode);
                if(Waypoint<Route.Num())
                {
                    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(GuardWalk),false,H);
                    if(GetWorld()->SweepSingleByChannel(Hit,H->GetActorLocation()+FVector(0,0,45),Route[Waypoint]+FVector(0,0,45),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,48),Q))UE_LOG(LogTemp,Display,TEXT("GUARD_OBSTACLE %s %s"),*Hit.ImpactPoint.ToString(),*HitInfo(Hit));
                }
            }
            Check(FString::Printf(TEXT("guard_post_%d_walk_%s"),GuardIndex,GuardDescending?TEXT("down"):TEXT("up")),Walked);
            if(!GuardDescending)
            {
                H->GetCharacterMovement()->StopMovementImmediately();const FVector Before=H->GetActorLocation();Json State=Capture();
                Check(FString::Printf(TEXT("guard_post_%d_save_schema_valid"),GuardIndex),ValidateSave(State));
                const bool Wrote=Save(TEXT("manual_5"));Place(BookWorld->Start(),FRotator(0,0,0));
                Check(FString::Printf(TEXT("guard_post_%d_save_load_restores_platform_position"),GuardIndex),Wrote&&Load(TEXT("manual_5"))&&H->GetActorLocation().Equals(Before,2));
                if(!Wrote)Place(Before,FRotator(0,90,0));
                for(int I=0;I<Route.Num()/2;++I)Route.Swap(I,Route.Num()-1-I);Waypoint=0;GuardDescending=true;Next(51);
            }
            else if(++GuardIndex<2)Next(50);else {Place(BookWorld->Start(),FRotator(-8,90,0));Next(Pictures?4:6);}
        }
    }
    else if(Step==4&&Clock>1)
    {
        if(FParse::Param(FCommandLine::Get(),TEXT("WardRealismReview30")))
        {
            Index=FMath::Max(Index,BookWorld->Sites.Num());const auto& Views=Array(WardBookMapData(),TEXT("furnishing_captures"));
            while(Index<BookWorld->Sites.Num()+Views.Num()&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v30_")))++Index;
            if(Index>=BookWorld->Sites.Num()+Views.Num()){Next(6);return;}
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("WardCarsReview31")))
        {
            Index=FMath::Max(Index,BookWorld->Sites.Num());const auto& Views=Array(WardBookMapData(),TEXT("furnishing_captures"));
            while(Index<BookWorld->Sites.Num()+Views.Num()&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v31_")))++Index;
            if(Index>=BookWorld->Sites.Num()+Views.Num()){Next(6);return;}
        }
        FString Prefix;
        if(FParse::Value(FCommandLine::Get(),TEXT("WardCapturePrefix="),Prefix)&&!Prefix.IsEmpty())
        {
            const auto& Views=Array(WardBookMapData(),TEXT("furnishing_captures"));
            while(Index>=BookWorld->Sites.Num()&&Index<BookWorld->Sites.Num()+Views.Num()&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).Contains(Prefix)&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v21_detail_"))&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v22_"))&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v23_"))&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v26_"))&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v27_"))&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v28_")))++Index;
            if(Index>=BookWorld->Sites.Num()+Views.Num()){Next(6);return;}
        }
        Json Capture=Index<BookWorld->Sites.Num()?MakeShared<FJsonObject>():Array(WardBookMapData(),TEXT("furnishing_captures"))[Index-BookWorld->Sites.Num()]->AsObject();
        const bool WantBright=!Boolean(Capture,TEXT("horror_mode"));if(BookWorld->Bright!=WantBright){Options->SetBoolField(TEXT("inspection_bright"),WantBright);ApplySettings(false);}
        if(Index<BookWorld->Sites.Num()){const auto& Site=BookWorld->Sites[Index];H->SetActorHiddenInGame(false);PC->SetViewTarget(H);Place(Site.Capture,Site.View);}
        else if(Boolean(Capture,TEXT("gameplay_camera")))
        {
            H->SetActorHiddenInGame(false);PC->SetViewTarget(H);BookWorld->LightSelectionClock=1;Place(Vector(Array(Capture,TEXT("at")))*100,FRotator(Number(Capture,TEXT("pitch")),Number(Capture,TEXT("yaw")),0));BookWorld->Update(.01f);
        }
        else
        {
            Json C=Array(WardBookMapData(),TEXT("furnishing_captures"))[Index-BookWorld->Sites.Num()]->AsObject();
            FString Zone=String(C,TEXT("id"));Zone.RemoveFromStart(TEXT("furnished_"));
            FVector Eye=Vector(Array(C,TEXT("at")))*100,Target=C->HasField(TEXT("target"))?Vector(Array(C,TEXT("target")))*100:Eye+FVector(300,0,-50);FString Bay;double Largest=0;
            for(auto V:Array(WardBookMapData(),TEXT("prototype_props")))
            {
                Json P=V->AsObject();if(String(P,TEXT("zone"))!=Zone||String(P,TEXT("mount"))!=TEXT("floor"))continue;
                Json Model=Object(Object(WardBookMapData(),TEXT("prototype_catalog")),FString::FromInt(Number(P,TEXT("model"))));const auto& B=Array(Model,TEXT("bounds"));
                if(B.Num()!=6)continue;const double Area=(B[3]->AsNumber()-B[0]->AsNumber())*(B[4]->AsNumber()-B[1]->AsNumber());
                if(Area<=Largest)continue;Largest=Area;Target=Vector(Array(P,TEXT("at")))*100+FVector(0,0,FMath::Clamp(B[5]->AsNumber()*50,35.,130.));Bay=String(P,TEXT("subroom"));
            }
            // A separate inspection camera shows the furniture inside the bay.
            // The normal third-person camera remains in the 35 gameplay views.
            for(auto V:Array(WardBookMapData(),TEXT("bay_navigation_routes")))
            {Json R=V->AsObject();const auto& Points=Array(R,TEXT("points"));if(String(R,TEXT("bay"))==Bay&&!Points.IsEmpty()){Eye=Vector(Points.Last()->AsArray())*100+FVector(0,0,165);break;}}
            BookWorld->LightSelectionClock=1;Place(Vector(Array(C,TEXT("at")))*100,(Target-Eye).Rotation());BookWorld->Update(.01f);
            H->SetActorHiddenInGame(true);
            if(!AdminCamera)AdminCamera=GetWorld()->SpawnActor<ACameraActor>();
            AdminCamera->SetActorLocation(Eye);AdminCamera->SetActorRotation((Target-Eye).Rotation());
            AdminCamera->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;AdminCamera->GetCameraComponent()->PostProcessBlendWeight=1;
            if(Boolean(C,TEXT("envelope")))AdminCamera->GetCameraComponent()->PostProcessSettings.AutoExposureMinBrightness=AdminCamera->GetCameraComponent()->PostProcessSettings.AutoExposureMaxBrightness=9.5;
            AdminCamera->GetCameraComponent()->SetFieldOfView(85);PC->SetViewTarget(AdminCamera);PC->PlayerCameraManager->UpdateCamera(.016f);
        }
        H->NoticeTime=0;Noise=0;SaveIcon=0;SampleTime=0;SampleFrames=0;Next(5);
    }
    else if(Step==5)
    {
        SampleTime+=Delta;++SampleFrames;if(Clock>2){const FString Id=Index<BookWorld->Sites.Num()?BookWorld->Sites[Index].Id:String(Array(WardBookMapData(),TEXT("furnishing_captures"))[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id"));Json Perf=MakeShared<FJsonObject>();Perf->SetStringField(TEXT("location"),Id);Perf->SetNumberField(TEXT("fps"),SampleFrames/FMath::Max(.001f,SampleTime));Performance.Add(MakeShared<FJsonValueObject>(Perf));FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/BookWorldCaptures/world_")+Id+TEXT(".png"),true,false);++Index;const int Total=BookWorld->Sites.Num()+Array(WardBookMapData(),TEXT("furnishing_captures")).Num();if(ExteriorStart>=0&&Index==Total){Index=0;ExteriorTailDone=true;}const bool Done=ExteriorStart>=0&&ExteriorTailDone&&Index==ExteriorStart;Next(!Done&&Index<Total?4:6);}
    }
    else if(Step==6&&Clock>.5)
    {
        bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),Pass);Result->SetNumberField(TEXT("count"),Checks.Num());Result->SetNumberField(TEXT("rooms"),BookWorld->Rooms.Num());Result->SetNumberField(TEXT("locations"),BookWorld->Sites.Num());Result->SetNumberField(TEXT("instances"),BookWorld->MeshInstances);Result->SetNumberField(TEXT("custom_furniture"),BookWorld->CustomFurniture);Result->SetNumberField(TEXT("custom_model_types"),BookWorld->CustomModelIds.Num());Result->SetNumberField(TEXT("custom_door_leaves"),BookWorld->CustomDoorLeaves);Result->SetNumberField(TEXT("custom_bounds_failures"),BookWorld->CustomBoundsFailures);Result->SetNumberField(TEXT("unresolved_custom_props"),BookWorld->UnresolvedCustomProps);Result->SetNumberField(TEXT("real_props"),BookWorld->RealProps);Result->SetNumberField(TEXT("realism30_props"),BookWorld->Realism30Props);Result->SetNumberField(TEXT("cars31_count"),BookWorld->Cars31Count);Result->SetNumberField(TEXT("cars31_sources"),BookWorld->Cars31Sources.Num());Result->SetNumberField(TEXT("detail_props"),BookWorld->DetailProps);Result->SetNumberField(TEXT("detail27_props"),BookWorld->Detail27Props);Result->SetNumberField(TEXT("horror_traces28"),BookWorld->HorrorTraces28);Result->SetNumberField(TEXT("horror_markers28"),BookWorld->HorrorMarkers28);Result->SetNumberField(TEXT("detail27_supported"),BookWorld->Detail27Supported);Result->SetNumberField(TEXT("detail27_services"),BookWorld->Detail27Services);Result->SetNumberField(TEXT("detail27_wall"),BookWorld->Detail27Wall);Result->SetNumberField(TEXT("detail_wall_props"),BookWorld->DetailWallProps);Result->SetNumberField(TEXT("detail_opening_props"),BookWorld->DetailOpeningProps);Result->SetNumberField(TEXT("detail_exterior_props"),BookWorld->DetailExteriorProps);Result->SetNumberField(TEXT("real_solid_props"),BookWorld->RealSolidProps);Result->SetNumberField(TEXT("quality_props"),BookWorld->QualityProps);Result->SetNumberField(TEXT("quality_trees"),BookWorld->QualityTrees);Result->SetNumberField(TEXT("quality_windows"),BookWorld->QualityWindows);Result->SetArrayField(TEXT("checks"),Checks);Result->SetArrayField(TEXT("sampled_performance"),Performance);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/book_world_check.json"),Result);FPlatformMisc::RequestExit(false);Next(7);
    }
}
