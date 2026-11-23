#include "WardHospitalGameplay.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "WardOnline80.h"
#include "WardCampusFinish67.h"
#include "WardCampaign.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/LocalFogVolumeComponent.h"
using namespace WardData;

void AEWardBookWorld::BuildHospitalDetails()
{
    auto* Sky=NewObject<UStaticMeshComponent>(this);Sky->SetupAttachment(RootComponent);Sky->SetMobility(EComponentMobility::Static);
    Sky->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/BookWorld/SM_HospitalSky.SM_HospitalSky")));
    auto* SkySurface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/EchoCustom/Materials/M_EchoSkyV22.M_EchoSkyV22"));
    if(!SkySurface)SkySurface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalSky.M_HospitalSky"));
    SkyMaterial=UMaterialInstanceDynamic::Create(SkySurface,this);Sky->SetMaterial(0,SkyMaterial);
    Sky->SetRelativeScale3D(FVector(4000));Sky->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sky->SetCastShadow(false);
    Sky->SetAffectDynamicIndirectLighting(false);Sky->SetAffectDistanceFieldLighting(false);Sky->RegisterComponent();AddInstanceComponent(Sky);
    // A genuine open campus ground plane, with no cell walls around parking lots.
    // Leave the main building excavation open: a ground slab through the
    // basement stair void would block a standing character on the B1 flight.
    for(int Y=-190;Y<380;++Y)
    {
        // Only actual ground-floor cells replace the campus slab. The old
        // rectangular excavation also removed the uncovered eastern apron.
        // Keep real stairs and lift voids open all the way to their stops.
        auto Ground=[&](int X){return !Cells.Contains(FIntVector(X,Y,0))&&!FloorHole(X,Y,0);};
        for(int X=-180;X<280;){if(!Ground(X)){++X;continue;}int End=X+1;while(End<280&&Ground(End))++End;Box(FVector((X+End)*50,Y*100+50,-35),FVector((End-X)*100,100,30),7);X=End;}
    }
    Box(FVector(5000,-2800,-12),FVector(1200,4200,24),0);
    // Front entrance canopy and external columns. No furniture or vehicles.
    Box(FVector(4800,-650,430),FVector(1800,1300,30),11);
    for(float X:{4000.f,5600.f})Box(FVector(X,-1100,210),FVector(35,35,420),5);
    Sign(TEXT("ECHO  /  HOSPITAL 333  /  WARD 04"),FVector(4800,-2660,380),FRotator(0,-90,0),30);
    for(const auto& S:Sites)
    {
        if(S.Outdoor||S.Id==TEXT("UT"))continue;
        bool Compact=false;for(auto V:Array(WardBookMapData(),TEXT("locations")))if(String(V->AsObject(),TEXT("id"))==S.Id)Compact=Boolean(V->AsObject(),TEXT("compact"));
        if(Compact)continue;
        const FVector O=S.Origin;
        const int Accent=S.Id==TEXT("B2")?12:S.Id==TEXT("SV")?11:2;
        // End windows and cross-hall portals make different departments readable.
        for(int Row:{19,65})
        {
            const bool WestGallery=S.Id==TEXT("1F")||S.Id==TEXT("B1")||S.Id==TEXT("B2")||S.Id==TEXT("2F")||S.Id==TEXT("3F");
            for(int Side:{0,10400})if(Side!=0||!WestGallery)
            {Box(O+FVector(Side,Row*100,200),FVector(18,570,240),6);for(int H:{90,310})Box(O+FVector(Side,Row*100,H),FVector(24,600,12),11);}
        }
        for(float X:{0.f,10400.f})
        {
            // Cross-hall centre remains a broad open entrance to each pavilion.
            for(float Y:{3650.f,4550.f})Box(O+FVector(X,Y,190),FVector(42,42,380),Accent);
            Box(O+FVector(X,4100,346),FVector(45,850,68),Accent);
        }
        for(int Y:{1800,6400})
        {
            Box(O+FVector(4800,Y,310),FVector(730,20,55),Accent);
            const FString Label=S.Id+TEXT("  /  ")+S.Name.ToUpper();
            const float LetterSize=FMath::Clamp(1100.f/FMath::Max(1,Label.Len()),15.f,24.f);
            Sign(Label,O+FVector(4800,Y-14,310),FRotator(0,-90,0),LetterSize);
        }
        // Vision panels are cut into the real boundary by WardBookWorld. Do not
        // overlay a decorative window across a new second doorway or vestibule.
        if(S.Origin.Z==0)
        {
            // Distinct masonry plinths, roof edge and empty loading door surrounds.
            for(float X:{0.f,10400.f})Box(O+FVector(X,4100,407),FVector(90,1200,30),10);
            Sign(S.Id,O+FVector(10365,4100,290),FRotator(0,0,0),55);
        }
        if(S.Id==TEXT("B1")||S.Id==TEXT("B2"))
        {
            // Ceiling utility runs and service risers; keep >3m clear headroom.
            for(float X:{4550.f,5050.f})Box(O+FVector(X,5200,350),FVector(35,4800,35),11,false);
            for(int Y=2600;Y<9000;Y+=1200)Box(O+FVector(4800,Y,355),FVector(500,60,50),11,false);
        }
    }
    // Follow the actual underground spine. Test the soffit first so pipe
    // brackets never bridge an open shaft or a missing ceiling.
    int32 UtilitySections=0;
    for(const auto& Site:Sites)
    {
        if(Site.Id!=TEXT("UT"))continue;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(Visual67TunnelSupport),false);
        if(Mode.IsValid()&&Mode->Hero())Query.AddIgnoredActor(Mode->Hero());
        for(int32 Metres=106;Metres<=325;Metres+=5)
        {
            const FVector At(5128,Metres*100,Site.Origin.Z);
            FHitResult Ceiling;
            if(!GetWorld()->LineTraceSingleByChannel(Ceiling,At+FVector(0,0,325),At+FVector(0,0,405),ECC_Pawn,Query)||
               Ceiling.ImpactNormal.Z>-.75||FMath::Abs(Ceiling.ImpactPoint.Z-At.Z-380)>4)
                continue;
            const FBox Span(FVector(-199,-23,0),FVector(199,23,65));
            if(AddCustomProp(6768,WardVisual67::FitBounds(Span),Ceiling.ImpactPoint-FVector(0,0,65),1))
                ++UtilitySections;
        }
    }
    UE_LOG(LogTemp,Display,TEXT("VISUAL67_TUNNEL sections=%d"),UtilitySections);

    // Electrical switchboards belong to the building engineering shell.
    for(float Z:{-480.f,1440.f})
    {
        Box(FVector(4445,3000,Z+135),FVector(70,130,210),11);
        Box(FVector(4483,3000,Z+135),FVector(8,110,190),5);
        for(int I=0;I<3;++I)Box(FVector(4489,2970+I*30,Z+180),FVector(4,12,12),I==0?4:12,false);
        Sign(TEXT("ELECTRICAL"),FVector(4495,3000,Z+115),FRotator(0,0,0),13);
    }
    // All three stair wells have roof guards, with a clear landing at the south.
    for(FVector O:TArray<FVector>{FVector(-800,0,1440),FVector(4400,8800,1440),FVector(-800,8600,1440)})
    {
        for(float X:{0.f,300.f})Box(O+FVector(X,340,55),FVector(10,680,110),5);
        Box(O+FVector(150,0,55),FVector(300,10,110),5);
        for(int Z:{-960,-480,0,480,960,1440})Sign(TEXT("STAIRS  /  A B C"),FVector(O.X+150,O.Y+800,Z+270),FRotator(0,90,0),17);
    }
    // Roof: continuous slab, perimeter parapet and real open-air helipad markings.
    // The shaft remains open between stops, but terminates under a real roof.
    Box(FVector(5000,700,1828),FVector(424,624,24),11);
    // Street-building stairs were marked as outdoor ground cells. Their upper
    // flights had walls, while the ground flight was exposed under the storey.
    // Enclose three sides, retaining the full-width landing approach at y+7m.
    for(auto V:Array(WardBookMapData(),TEXT("stairs")))
    {
        Json S=V->AsObject();const auto& R=Array(S,TEXT("rect"));if(R.Num()!=4||Number(S,TEXT("z_min"))>=0)continue;
        const int X=FMath::RoundToInt(R[0]->AsNumber()),Y=FMath::RoundToInt(R[1]->AsNumber());
        const auto* C=Cells.Find(FIntVector(X+1,Y+2,0));if(!C||!C->Outdoor)continue;
        for(float Side:{0.f,300.f})Box(FVector(X*100+Side,Y*100+350,240),FVector(16,700,480),10);
        Box(FVector(X*100+150,Y*100,240),FVector(300,16,480),10);
        if(Number(S,TEXT("z_max"))==0)Box(FVector(X*100+150,Y*100+350,388),FVector(316,716,16),11);
    }
    // Generated room floors cover the roof; fill the west evacuation-core apron.
    // The generated roof floor already covers the west gallery and respects
    // both stair voids. An extra apron here would seal the upper flights.
    for(float X:{-800.f,10400.f})Box(FVector(X,5000,1495),FVector(22,10000,110),1);
    for(float Y:{0.f,10000.f})Box(FVector(4800,Y,1495),FVector(11200,22,110),1);
    // H and perimeter-square marking, no helicopter (architecture-only phase).
    for(float X:{2900.f,6300.f})Box(FVector(X,4800,1441),FVector(14,3400,2),8,false);
    for(float Y:{3100.f,6500.f})Box(FVector(4600,Y,1441),FVector(3400,14,2),8,false);
    for(float X:{4250.f,4950.f})Box(FVector(X,4800,1442),FVector(90,1500,3),8,false);
    Box(FVector(4600,4800,1442),FVector(700,90,3),8,false);
    for(int I=0;I<12;++I)
    {
        const float A=I*2*PI/12;
        const FVector Marker(4600+1650*FMath::Cos(A),4800+1650*FMath::Sin(A),1450);
        Box(Marker,FVector(30,30,16),4,false);
        if(I%3==0)WarmAnchors.Add(Marker+FVector(0,0,65));
    }
    for(const auto& R:Rooms)if(R.Location==TEXT("RF")&&!R.Enclosed&&R.Id!=TEXT("T04")&&R.Id!=TEXT("T01")&&R.Id!=TEXT("T13"))
    {
        const FVector C=R.Bounds.GetCenter();
        // Roof ventilation is built infrastructure, not a furnished room.
        const FBox VentBounds(FVector(-215,-325,0),FVector(215,325,200));
        const bool Detailed=AddCustomProp(174,WardVisual67::FitBounds(VentBounds),FVector(C.X,C.Y,1440),0);
        Box(FVector(C.X,C.Y,1540),FVector(430,650,200),11,true,!Detailed);
        if(!Detailed)
            for(int I=0;I<8;++I)Box(FVector(C.X-200+I*55,C.Y-326,1550),FVector(18,8,140),5,false);
        Box(FVector(C.X+350,C.Y,1495),FVector(270,90,110),11);
    }
    // Parking markings, pavements and planted court areas on the open ground.
    const FVector ST(-7000,-19000,0);
    // Parking bay paint is created around the fitted vehicles after all supports exist.
    for(int Y=2000;Y<16500;Y+=700)Box(ST+FVector(11200,Y,1),FVector(12,320,2),8,false);
    for(int I=0;I<9;++I)Box(ST+FVector(11100+I*100,15900,1),FVector(55,600,2),8,false);
    for(const auto& R:Rooms)if(R.Location==TEXT("ST"))
    {
        const FVector C=R.Bounds.GetCenter();
        if(R.Enclosed)
        {
            // Ground buildings have a proper roof and shopfront fascia.
            Box(FVector(C.X,C.Y,400),FVector(R.Bounds.GetSize().X+36,R.Bounds.GetSize().Y+36,30),10);
            Box(R.Door+FVector(0,16,242),FVector(FMath::Min(900.,R.Bounds.GetSize().X),30,40),R.Id==TEXT("X02")?2:10);
            Sign(R.Id==TEXT("X01")?TEXT("SUPERMARKET"):R.Id==TEXT("X02")?TEXT("PHARMACY"):R.Id==TEXT("X03")?TEXT("GROCERY"):R.Id==TEXT("X04")?TEXT("SUPPLY WAREHOUSE"):R.Id==TEXT("X05")?TEXT("CAFE"):R.Id,R.Door+FVector(0,35,200),FRotator(0,90,0),30);
        }
        else if(R.Id==TEXT("S05")||R.Id==TEXT("S15"))Box(FVector(C.X,C.Y,2),FVector(R.Bounds.GetSize().X-80,R.Bounds.GetSize().Y-80,4),9,false);
    }
    for(int I=0;I<10;++I)
    {
        FVector P=ST+FVector(9000,1000+I*1600,0);Box(P+FVector(0,0,250),FVector(20,20,500),11);
        Box(P+FVector(60,0,495),FVector(150,60,15),4,false);LightAnchors.Add(P+FVector(60,0,450));
    }
    // Boundary fence belongs only at the perimeter, not around each map zone.
    for(float X:{-18000.f,28000.f})Box(FVector(X,9500,110),FVector(16,57000,220),5);
    Box(FVector(5000,38000,110),FVector(46000,16,220),5);
}

void AEWardBookWorld::BuildLift()
{
    auto Part=[&](USceneComponent* Parent,FVector At,FVector Size,int Mat,bool Collision=true)
    {
        auto* C=NewObject<UStaticMeshComponent>(this);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Campaign/BookWorld/SM_BookCube.SM_BookCube")));
        C->SetRelativeLocation(At);C->SetRelativeScale3D(Size/100);C->SetMaterial(0,Palette[Mat]);C->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));
        C->SetAffectDynamicIndirectLighting(false);C->SetAffectDistanceFieldLighting(false);C->SetLightingChannels(true,true,false);C->RegisterComponent();AddInstanceComponent(C);return C;
    };
    // Independent movable root allows the complete bed-sized cabin to travel.
    LiftCabin=NewObject<USceneComponent>(this);LiftCabin->SetMobility(EComponentMobility::Movable);LiftCabin->RegisterComponent();AddInstanceComponent(LiftCabin);LiftCabin->SetWorldLocation(FVector(5000,700,0));
    LiftPlatform=Part(LiftCabin,FVector(0,0,-10),FVector(380,580,20),11);
    for(float X:{-185.f,185.f})Part(LiftCabin,FVector(X,0,170),FVector(12,570,340),11);
    Part(LiftCabin,FVector(0,-280,170),FVector(380,12,340),11);
    Part(LiftCabin,FVector(0,0,345),FVector(380,570,12),11);
    Part(LiftCabin,FVector(0,0,334),FVector(140,30,8),4,false);
    LiftDisplay=NewObject<UTextRenderComponent>(this);LiftDisplay->SetupAttachment(LiftCabin);LiftDisplay->SetRelativeLocation(FVector(0,-265,250));LiftDisplay->SetRelativeRotation(FRotator(0,90,0));LiftDisplay->SetWorldSize(19);LiftDisplay->SetHorizontalAlignment(EHTA_Center);LiftDisplay->RegisterComponent();AddInstanceComponent(LiftDisplay);
    for(int I=0;I<6;++I)
    {
        const float Z=-960+I*480;
        Box(FVector(4800,700,Z+190),FVector(16,600,380),11);Box(FVector(5200,700,Z+190),FVector(16,600,380),11);
        Box(FVector(5000,400,Z+190),FVector(400,16,380),11);
        Box(FVector(5000,1000,Z+340),FVector(400,20,80),11);
        for(int Side:{-1,1})LiftDoors.Add(Part(RootComponent,FVector(5000+Side*74,1000,Z+140),FVector(148,12,280),11));
        Sign(TEXT("BED LIFT   [F] CALL"),FVector(5000,1020,Z+300),FRotator(0,90,0),19);
    }
}
bool AEWardBookWorld::InsideLift(FVector P)const{return FMath::Abs(P.X-5000)<165&&P.Y>430&&P.Y<930&&FMath::Abs(P.Z-LiftZ-98)<80;}
void AEWardBookWorld::RestoreLift(FVector P)
{
    if(FMath::Abs(P.X-5000)>=190||P.Y<=400||P.Y>=1000)return;
    LiftFloor=FMath::Clamp(FMath::RoundToInt((P.Z-98+960)/480),0,5);LiftTarget=LiftFloor;LiftZ=-960+LiftFloor*480;
    LiftMoving=false;LiftDoor=1;LiftCabin->SetWorldLocation(FVector(5000,700,LiftZ));Update(0);
}
bool AEWardBookWorld::RequestLift(int Floor)
{
    if(LiftMoving||Floor<0||Floor>5||Floor==LiftFloor)return false;
    if(Mode.IsValid()&&Mode->HospitalStory&&Mode->HospitalStory->NarrativeEnabled35)
    {
        static const int RequiredChapter[]={6,5,1,2,7,14};
        if(Mode->HospitalStory->Chapter()<RequiredChapter[Floor]){Mode->Notify(TEXT("Карантин: этот этаж пока закрыт. Сначала закончи текущую задачу."),5);return false;}
    }
    LiftTarget=Floor;LiftMoving=true;LiftWait=.7f;return true;
}
void AEWardBookWorld::SelectLift(int Direction)
{
    if(!Mode.IsValid()||!Mode->Hero()||!InsideLift(Mode->Hero()->GetActorLocation())||LiftMoving)return;
    LiftSelection=FMath::Clamp(LiftSelection+Direction,0,5);
    static const TCHAR* Names[]={TEXT("B2"),TEXT("B1"),TEXT("1F"),TEXT("2F"),TEXT("3F"),TEXT("RF")};
    Mode->Notify(FString::Printf(TEXT("Лифт: %s · PgUp / PgDn — этаж · F — ехать"),Names[LiftSelection]));
}
void AEWardBookWorld::UseLift()
{
    if(!Mode.IsValid()||!Mode->Hero()||LiftMoving)return;
    const FVector P=Mode->Hero()->GetActorLocation();
    if(InsideLift(P))RequestLift(LiftSelection);
    else if(FMath::Abs(P.X-5000)<250&&P.Y>=1000&&P.Y<1350)
    {
        const int Floor=FMath::RoundToInt((P.Z-98+960)/480);
        if(Floor==LiftFloor)Mode->Notify(TEXT("Войди в лифт · PgUp / PgDn — этаж · F — ехать"));else RequestLift(Floor);
    }
}
void AEWardBookWorld::Update(float Delta)
{
    UpdateDeferred37(Delta);
    UpdateCars33(Delta);
    EmergencyClock+=Delta;
    UpdateHorrorAtmosphere(Delta);
    // A quiet pulse with a short, irregular-looking electrical dropout. Four
    // local lights are reused rather than spawning a light for every beacon.
    const float Pulse=.65f+.35f*FMath::Sin(EmergencyClock*2.1f);
    const float Flicker=FMath::Fmod(EmergencyClock,11.7f)<.11f?.08f:Pulse;
    if(EmergencyMaterial)EmergencyMaterial->SetVectorParameterValue(TEXT("Emission"),FLinearColor(40,.22f,.035f)*Flicker);
    for(int I=0;I<EmergencyPool.Num();++I)EmergencyPool[I]->SetIntensity((Bright?2200.f:1950.f)*Flicker*(.88f+.12f*FMath::Sin(EmergencyClock*5+I)));
    auto* H=Mode.IsValid()?Mode->Hero():nullptr;
    const bool Replica = Mode.IsValid() && WardOnline80::Guest(*Mode);
    if (!Replica)
    {
        const float Want=LiftMoving?0:1;
        LiftDoor=FMath::FInterpConstantTo(LiftDoor,Want,Delta,2);
    }
    for(int I=0;I<LiftDoors.Num();++I)
    {
        const int Floor=I/2,Side=I%2?1:-1;const float Open=Floor==LiftFloor?LiftDoor:0;
        LiftDoors[I]->SetRelativeLocation(FVector(5000+Side*(74+Open*153),1000,-960+Floor*480+140));
        LiftDoors[I]->SetVisibility(Open<.99f);LiftDoors[I]->SetCollisionProfileName(Open<.99f?TEXT("BlockAll"):TEXT("NoCollision"));
    }
    if(!Replica&&LiftMoving&&LiftDoor<=.001f&&(LiftWait-=Delta)<=0)
    {
        const bool Rider=H&&InsideLift(H->GetActorLocation());const float Old=LiftZ;
        LiftZ=FMath::FInterpConstantTo(LiftZ,-960+LiftTarget*480,Delta,180);LiftCabin->SetWorldLocation(FVector(5000,700,LiftZ));
        if(Rider){H->GetCharacterMovement()->DisableMovement();H->SetActorLocation(H->GetActorLocation()+FVector(0,0,LiftZ-Old),false);}
        if(FMath::Abs(LiftZ-(-960+LiftTarget*480))<.1f)
        {
            LiftFloor=LiftTarget;LiftMoving=false;LiftSelection=FMath::Clamp(LiftFloor+1,0,5);
            if(Rider)H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
    static const TCHAR* Names[]={TEXT("B2"),TEXT("B1"),TEXT("1F"),TEXT("2F"),TEXT("3F"),TEXT("RF")};
    const FString Caption=FString::Printf(TEXT("%s  >  %s\nPgUp / PgDn    [F]"),Names[LiftFloor],Names[LiftSelection]);
    // UTextRenderComponent::SetText always destroys/recreates its render proxy,
    // even for equal text. A stationary lift should not invalidate the scene.
    if(LiftDisplay->Text.ToString()!=Caption)LiftDisplay->SetText(FText::FromString(Caption));
}

void AEWardBookWorld::SetBright(bool Enabled)
{
    Enabled=Enabled&&Mode.IsValid()&&Mode->Admin;
    Bright=Enabled;
    LightSelectionClock=1;
    if(Daylight){Daylight->SetIntensity(Enabled?600.f:8.f);Daylight->SetLightColor(Enabled?FLinearColor(.97f,.95f,.91f):FLinearColor(.63f,.39f,.33f));}
    if(SkyMaterial)SkyMaterial->SetScalarParameterValue(TEXT("Bright"),Enabled?1:0);
    for(int I=0;I<Palette.Num();++I)if(I!=4)
    {
        auto* M=Cast<UMaterialInstanceDynamic>(Palette[I]);if(!M)continue;const FLinearColor Tint=M->K2_GetVectorParameterValue(TEXT("Tint"));
        M->SetVectorParameterValue(TEXT("Emission"),Tint*(Enabled?.22f:.005f));
    }
    if(EmergencyMaterial)EmergencyMaterial->SetVectorParameterValue(TEXT("Emission"),FLinearColor(40,.22f,.035f));
    if(DormantLampMaterial!=INDEX_NONE)Cast<UMaterialInstanceDynamic>(Palette[DormantLampMaterial])->SetVectorParameterValue(TEXT("Emission"),FLinearColor::Black);
    for(auto& Fog:FloorMist)if(Enabled)Fog->SetVisibility(false);
    if(Mode.IsValid()&&Mode->Hero())UpdateLighting(Mode->Hero()->GetActorLocation());
}
