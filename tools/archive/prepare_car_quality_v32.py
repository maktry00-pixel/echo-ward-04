from pathlib import Path
import shutil,json
R=Path('D:/Projects/ECHO');U=R/'unreal/EchoWard';D=R/'artifacts/echo_car_quality_v32';(D/'before').mkdir(exist_ok=True);(D/'source_before').mkdir(exist_ok=True)
for rel in ['Source/EchoWard/WardHeroVehicle31.cpp','Source/EchoWard/WardHospitalGameplay.h','Source/EchoWard/WardCampaign.cpp','Source/EchoWard/WardHospitalArrivalAssessment29.cpp','Content/Python/import_echo_cars_v31.py','Config/DefaultEngine.ini']:
 shutil.copy2(U/rel,D/'source_before'/Path(rel).name)
for name in ['08_interior.png','09_all_panels_open.png']:shutil.copy2(R/'artifacts/echo_cars_v31/arrival'/name,D/'before'/name)
for name in ['kia-carnival','2025-toyota-camry-xv80-hybrid','parking_horror']:
 src=U/'Saved/PackageTests/CarsV31/Saved/Prototype/BookWorldCaptures'/('world_v31_'+name+'.png');shutil.copy2(src,D/'before'/src.name)
p=U/'Content/Python/import_echo_cars_v31.py';s=p.read_text(encoding='utf-8').replace("'max_texture_size',2048 if hero else 1024", "'max_texture_size',4096 if hero else 2048");p.write_text(s,encoding='utf-8')
p=U/'Source/EchoWard/WardHospitalGameplay.h';s=p.read_text(encoding='utf-8').replace('bool Seated31=false;','bool Seated31=false;\n    bool VehicleCloseQuality32=false;');p.write_text(s,encoding='utf-8')
p=U/'Source/EchoWard/WardHeroVehicle31.cpp';s=p.read_text(encoding='utf-8');needle='    const FVector At=ArrivalCar->GetActorLocation();if(!VehicleLastAt31.IsZero())';assert needle in s;s=s.replace(needle,'''    // Keep the real base geometry and full source mips around the hero vehicle.
    // Change residency only when crossing the range boundary, never every frame.
    const bool Close32=ArrivalStage>=0&&(Seated31||FVector::DistSquared(Mode->Hero()->GetActorLocation(),ArrivalCar->GetActorLocation())<FMath::Square(800.f));
    if(Close32!=VehicleCloseQuality32)
    {
        VehicleCloseQuality32=Close32;
        for(auto& Pair:VehicleParts31)if(auto* Part=Pair.Value.Get()){Part->SetForcedLodModel(Close32?1:0);Part->SetTextureForceResidentFlag(Close32);}
    }
'''+needle);p.write_text(s,encoding='utf-8')
p=U/'Source/EchoWard/WardCampaign.cpp';s=p.read_text(encoding='utf-8').replace('    const int32 AA=FMath::Clamp', '    Set(TEXT("r.Tonemapper.Sharpen"),.3f);\n    const int32 AA=FMath::Clamp');p.write_text(s,encoding='utf-8')
p=U/'Config/DefaultEngine.ini';s=p.read_text(encoding='utf-8').replace('r.Streaming.PoolSize=1500','r.Streaming.PoolSize=2048');p.write_text(s,encoding='utf-8')
# Reuse the exact v31 routes and captures. New checks cover the actual cabin state.
p=U/'Source/EchoWard/WardHospitalArrivalAssessment29.cpp';s=p.read_text(encoding='utf-8').replace('#include "UnrealClient.h"','#include "UnrealClient.h"\n#include "Engine/Engine.h"\n#include "Engine/GameViewportClient.h"\n#include "HAL/IConsoleManager.h"');needle='Check(TEXT("v31_interior_camera_can_look_around"),';s=s.replace(needle,'bool FullDetail32=VehicleCloseQuality32;for(const auto& Pair:VehicleParts31)FullDetail32&=Pair.Value&&Pair.Value->ForcedLodModel==1;Check(TEXT("v32_cabin_keeps_original_base_geometry"),FullDetail32);Check(TEXT("v32_capture_is_native_1080p"),GEngine->GameViewport->Viewport->GetSizeXY()==FIntPoint(1920,1080)&&IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"))->GetFloat()>=99);'+needle);p.write_text(s,encoding='utf-8')
print('V32 source changes ready')
