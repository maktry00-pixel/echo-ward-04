from pathlib import Path
R=Path('unreal/EchoWard/Source/EchoWard')
p=R/'WardHospitalFurnishing.cpp';s=p.read_text().replace('{PrototypeParts+=Array(Model,TEXT("parts")).Num();','{if(String(Prop,TEXT("mount"))==TEXT("floor")&&!String(Object(Detailed,FString::FromInt(Number(Prop,TEXT("model")))),TEXT("mesh")).IsEmpty())++DetailedFurniture;PrototypeParts+=Array(Model,TEXT("parts")).Num();');p.write_text(s)
p=R/'WardBookWorldAssessment.cpp';s=p.read_text();a='        Check(TEXT("normal_new_game_uses_complete_3d_world")';insert='''        if(Pictures&&FParse::Param(FCommandLine::Get(),TEXT("WardCarsReview31")))
        {auto Views=Array(WardBookMapData(),TEXT("furnishing_captures"));Views.Append(Array(WardEchoAssets::Cars31(),TEXT("captures")));WardBookMapData()->SetArrayField(TEXT("furnishing_captures"),Views);}
''';assert a in s;s=s.replace(a,insert+a)
a='        FString Prefix;';insert='''        if(FParse::Param(FCommandLine::Get(),TEXT("WardCarsReview31")))
        {
            Index=FMath::Max(Index,BookWorld->Sites.Num());const auto& Views=Array(WardBookMapData(),TEXT("furnishing_captures"));
            while(Index<BookWorld->Sites.Num()+Views.Num()&&!String(Views[Index-BookWorld->Sites.Num()]->AsObject(),TEXT("id")).StartsWith(TEXT("v31_")))++Index;
            if(Index>=BookWorld->Sites.Num()+Views.Num()){Next(6);return;}
        }
''';s=s.replace(a,insert+a)
a='        if(Number(WardBookMapData(),TEXT("review_revision"))>=19)';insert='''        if(Number(WardEchoAssets::Cars31(),TEXT("version"))>=31)
        {
            Check(TEXT("v31_all_supplied_sources_placed_with_hero"),BookWorld->Cars31Count==35&&BookWorld->Cars31Sources.Num()==31&&BookWorld->Cars31Replacements.Num()==20);
            bool Assets=true,Metric=true,Lods=true,Materials=true;int Count=0;
            for(auto V:Array(WardEchoAssets::Data(),TEXT("meshes")))
            {
                const Json R=V->AsObject();const int Id=Number(R,TEXT("id"));if(!((Id>=1000&&Id<=1131)||(Id>=2000&&Id<=2011)))continue;++Count;auto* M=WardEchoAssets::Mesh(Id);Assets&=M!=nullptr;if(!M)continue;
                const auto& B=Array(R,TEXT("bounds_cm"));Metric&=B.Num()==2&&M->GetBoundingBox().Min.Equals(Vector(B[0]->AsArray()),.5)&&M->GetBoundingBox().Max.Equals(Vector(B[1]->AsArray()),.5);
                Lods&=Number(R,TEXT("lods"))>=1;for(const auto& Mat:M->GetStaticMaterials())Materials&=Mat.MaterialInterface!=nullptr;
            }
            Check(TEXT("v31_all_68_vehicle_meshes_load_with_metric_bounds"),Count==68&&Assets&&Metric);Check(TEXT("v31_vehicle_materials_and_lods_exist"),Materials&&Lods);
            bool Separate=true;for(int I=0;I<BookWorld->Cars31Bounds.Num();++I)for(int J=I+1;J<BookWorld->Cars31Bounds.Num();++J)Separate&=!BookWorld->Cars31Bounds[I].Intersect(BookWorld->Cars31Bounds[J]);Check(TEXT("v31_parked_vehicle_envelopes_do_not_intersect"),Separate);
        }
''';assert a in s;s=s.replace(a,insert+a);s=s.replace('Result->SetNumberField(TEXT("realism30_props"),BookWorld->Realism30Props);','Result->SetNumberField(TEXT("realism30_props"),BookWorld->Realism30Props);Result->SetNumberField(TEXT("cars31_count"),BookWorld->Cars31Count);Result->SetNumberField(TEXT("cars31_sources"),BookWorld->Cars31Sources.Num());');p.write_text(s)
p=R/'WardHospitalArrivalAssessment29.cpp';s=p.read_text();s=s.replace('Check(TEXT("cannot_save_while_seated_in_car")','Check(TEXT("v31_hero_has_body_four_doors_hood_trunk_wheels_and_steering"),VehicleParts31.Num()==12&&VehicleTarget31.Num()==6);\n        Check(TEXT("cannot_save_while_seated_in_car")')
a='        H->SetActorLocation(Vector(Array(ArrivalData,TEXT("checkpoint")))*100,false);PC->SetControlRotation(FRotator(-5,90,0));Stage=4;Clock=0;'
insert='''        VehicleAutoClose31=0;for(auto& P:VehicleOpen31){P.Value=0;VehicleTarget31.Add(P.Key,0);}TickVehicle31(0);
        const auto* Door=VehicleParts31.FindRef(TEXT("DoorFL"));const FVector DoorAt=ArrivalCar->GetActorTransform().TransformPosition(Door->GetRelativeLocation()+Door->GetStaticMesh()->GetBoundingBox().GetCenter()+FVector(0,-150,0));H->SetActorLocation(FVector(DoorAt.X,DoorAt.Y,98),false);Check(TEXT("v31_driver_door_is_selected_from_outside"),NearestVehiclePart31()==TEXT("DoorFL"));ToggleVehiclePart31();TickVehicle31(.7f);Check(TEXT("v31_door_rotates_about_its_own_hinge"),VehicleOpen31.FindRef(TEXT("DoorFL"))>.99f&&FMath::Abs(Door->GetRelativeRotation().Yaw-65)<.1f);ToggleVehiclePart31();TickVehicle31(.7f);Check(TEXT("v31_driver_door_closes"),FMath::Abs(Door->GetRelativeRotation().Yaw)<.1f);
        Check(TEXT("v31_F_enters_the_driver_seat"),VehicleInteract31()&&Seated31&&H->IsHidden()&&PC->GetViewTarget()==ArrivalCamera&&H->GetCharacterMovement()->MovementMode==MOVE_None);
        const FVector Eye=ArrivalCar->GetActorTransform().TransformPosition(Vector(Array(Object(WardEchoAssets::Cars31(),TEXT("hero")),TEXT("driver_eye_cm"))));Check(TEXT("v31_camera_is_inside_the_real_interior"),ArrivalCamera->GetActorLocation().Equals(Eye,.5));Check(TEXT("v31_reentry_blocks_saving"),!Mode->Save(TEXT("manual_6")));const float Stamina=H->Stamina;Mode->Dash();Check(TEXT("v31_seat_blocks_dash"),H->Stamina==Stamina);
        VehicleYaw31=35;VehiclePitch31=-10;TickVehicle31(0);Check(TEXT("v31_interior_camera_can_look_around"),FMath::Abs(ArrivalCamera->GetActorRotation().Yaw-125)<.1f);Mode->Show(TEXT("pause"));Mode->Show(TEXT(""));TickVehicle31(0);Check(TEXT("v31_pause_resume_keeps_seated_camera"),Seated31&&H->IsHidden()&&PC->GetViewTarget()==ArrivalCamera);Shot(TEXT("08_interior"));Stage=31;Clock=0;
''';assert a in s;s=s.replace(a,insert)
a='    else if(Stage==4&&Clock>1)';insert='''    else if(Stage==31&&Clock>.6f)
    {
        Check(TEXT("v31_F_exits_after_reentry"),VehicleInteract31()&&!Seated31&&!H->IsHidden()&&H->GetCharacterMovement()->MovementMode==MOVE_Walking);VehicleAutoClose31=0;
        for(const FString Name:{TEXT("DoorFL"),TEXT("DoorFR"),TEXT("DoorRL"),TEXT("DoorRR"),TEXT("Hood"),TEXT("Trunk")})
        {
            auto* C=VehicleParts31.FindRef(Name);const FVector Local=C->GetRelativeLocation()+C->GetStaticMesh()->GetBoundingBox().GetCenter();const FVector Out=Name==TEXT("Hood")?FVector(130,0,0):Name==TEXT("Trunk")?FVector(-130,0,0):Name.EndsWith(TEXT("L"))?FVector(0,-150,0):FVector(0,150,0);FVector At=ArrivalCar->GetActorTransform().TransformPosition(Local+Out);At.Z=98;H->SetActorLocation(At,false);VehicleTarget31.Add(Name,0);VehicleOpen31.Add(Name,0);ToggleVehiclePart31();TickVehicle31(.8f);Check(TEXT("v31_interactive_panel_opens_")+Name,VehicleOpen31.FindRef(Name)>.99f);
        }
        Check(TEXT("v31_open_panels_save"),Mode->Save(TEXT("manual_3")));for(auto& P:VehicleTarget31)P.Value=0;TickVehicle31(1);Check(TEXT("v31_panel_states_restore_from_save"),Mode->Load(TEXT("manual_3"))&&VehicleTarget31.FindRef(TEXT("Hood"))==1&&VehicleTarget31.FindRef(TEXT("Trunk"))==1);TickVehicle31(0);
        const FVector At=ArrivalCar->GetActorLocation(),Eye=At+FVector(430,520,290);ArrivalCamera->SetActorLocationAndRotation(Eye,(At+FVector(0,0,70)-Eye).Rotation());PC->SetViewTarget(ArrivalCamera);Stage=32;Clock=0;
    }
    else if(Stage==32&&Clock>1)
    {
        Shot(TEXT("09_all_panels_open"));for(auto& P:VehicleTarget31)P.Value=0;TickVehicle31(1);H->SetActorLocation(Vector(Array(ArrivalData,TEXT("checkpoint")))*100,false);PC->SetViewTarget(H);PC->SetControlRotation(FRotator(-5,90,0));Stage=4;Clock=0;
    }
''';assert a in s;s=s.replace(a,insert+a);s=s.replace('if(Clock>35&&Stage<11)','if(Clock>35&&Stage!=11)');s=s.replace('#include "WardEchoAssets.h"','#include "WardEchoAssets.h"\n#include "Engine/StaticMesh.h"');p.write_text(s)
