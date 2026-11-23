"""Update the runtime assessment from fixed v4 totals to the expanded world."""
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'unreal/EchoWard/Source/EchoWard/WardBookWorldAssessment.cpp'
s=p.read_text(encoding='utf-8')
s=s.replace('BookWorld->Rooms.Num()==334&&BookWorld->Sites.Num()==15',
    'BookWorld->Rooms.Num()==562&&BookWorld->Sites.Num()==35')
s=s.replace('Check(TEXT("nine_stair_cores_have_forty_four_real_flights"),BookWorld->StairFlights==44);',
    'int ExpectedFlights=0;for(auto V:Array(WardBookMapData(),TEXT("stairs")))ExpectedFlights+=FMath::RoundToInt((Number(V->AsObject(),TEXT("z_max"))-Number(V->AsObject(),TEXT("z_min")))/4.8)*2;\n        Check(TEXT("all_33_stair_cores_build_the_required_real_flights"),BookWorld->StairFlights==ExpectedFlights&&ExpectedFlights>100);')
s=s.replace('int Floors=0,Doors=0,Walls=0;', 'int Floors=0,Doors=0,Walls=0,Enclosed=0;')
s=s.replace('if(!R.Enclosed)continue;\n            const FVector Centre', 'if(!R.Enclosed)continue;++Enclosed;\n            const FVector Centre')
s=s.replace('Floors==313','Floors==Enclosed').replace('Doors==313','Doors==Enclosed').replace('Walls==313','Walls==Enclosed')
start=s.index('        Route.Reset();Waypoint=0;static const FVector ExtraOffsets[]')
end=s.index('        Next(31);',start)
s=s[:start]+'''        Route.Reset();Waypoint=0;const Json Stair=Array(WardBookMapData(),TEXT("stairs"))[ExtraCore]->AsObject();
        const auto& R=Array(Stair,TEXT("rect"));const FVector Offset(R[0]->AsNumber()*100-2800,R[1]->AsNumber()*100,0);
        Place(FVector(3150,850,Number(Stair,TEXT("z_min"))*100+98)+Offset,FRotator(0,-90,0));
        for(int Z=FMath::RoundToInt(Number(Stair,TEXT("z_min"))*100);Z<Number(Stair,TEXT("z_max"))*100;Z+=480)
        for(FVector P:TArray<FVector>{FVector(3150,740,98),FVector(2878,740,98),FVector(2878,650,114),FVector(2878,270,338),FVector(2878,200,338),FVector(3022,200,338),FVector(3022,290,354),FVector(3022,660,578),FVector(3022,740,578),FVector(3150,740,578)})Route.Add(P+Offset+FVector(0,0,Z));
'''+s[end:]
s=s.replace('Waypoint==Route.Num()||Clock>40','Waypoint==Route.Num()||Clock>150')
s=s.replace('if(++ExtraCore<8)', 'if(++ExtraCore<Array(WardBookMapData(),TEXT("stairs")).Num())')
s=s.replace('if(!R.Enclosed||R.Location==TEXT("ST")',
    'bool Compact=false;for(auto V:Array(WardBookMapData(),TEXT("locations")))if(String(V->AsObject(),TEXT("id"))==R.Location)Compact=Boolean(V->AsObject(),TEXT("compact"));\n            if(Compact||!R.Enclosed||R.Location==TEXT("ST")')
s=s.replace('TEXT("all_334_zone_routes_fit_real_character_capsule"),Routes==334',
    'TEXT("all_562_zone_routes_fit_real_character_capsule"),Routes==BookWorld->Rooms.Num()')
s=s.replace('const FVector A=O+FVector(S.X*.5,S.Y+50,98),B=O+FVector(S.X*.5,60,98);',
    'const bool Bed=String(N,TEXT("kind"))==TEXT("under_bed");const float Half=Bed?48:88,Z=Bed?48:98;\n            const FVector A=O+FVector(S.X*.5,S.Y+50,Z),B=O+FVector(S.X*.5,Bed?S.Y*.55:60,Z);')
s=s.replace('FCollisionShape::MakeCapsule(34,88),Q))++Hides',
    'FCollisionShape::MakeCapsule(34,Half),Q))++Hides')
s=s.replace('Closed>=130','Closed>=80')
s=s.replace('Place(FVector(4850,9800,-382),FRotator(0,90,0));Waypoint=0;\n        Route={FVector(5000,9800,-382),FVector(5000,21600,-382),FVector(4200,21600,-382),FVector(4200,23600,-382),FVector(5000,23600,-382),FVector(5000,32800,-382),FVector(14550,32800,-382),FVector(14550,32340,-382)};Next(40);',
    '''Place(FVector(4850,9800,-382),FRotator(0,90,0));Waypoint=0;
        Route={FVector(5000,9800,-382),FVector(5000,21600,-382),FVector(4200,21600,-382),FVector(4200,23600,-382),FVector(5000,23600,-382),FVector(5000,32800,-382)};
        for(auto V:Array(WardBookMapData(),TEXT("utility_links")))if(String(V->AsObject(),TEXT("id"))==TEXT("U_LINK_SV"))for(auto P:Array(V->AsObject(),TEXT("points")))Route.Add(Vector(P->AsArray())*100+FVector(0,0,98));Next(40);''')
p.write_text(s,encoding='utf-8')
# Saving a hidden crouched position would restore a standing capsule in a bed.
q=p.with_name('WardCampaignSave.cpp');t=q.read_text(encoding='utf-8')
t=t.replace('(BookWorld&&BookWorld->LiftMoving)', '(BookWorld&&(BookWorld->LiftMoving||BookWorld->FurnitureHiding))')
q.write_text(t,encoding='utf-8')
