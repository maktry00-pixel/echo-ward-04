from pathlib import Path
R=Path('unreal/EchoWard/Source/EchoWard')
def edit(n,a,b):
 p=R/n;s=p.read_text();assert a in s,(n,a[:80]);p.write_text(s.replace(a,b))
edit('WardHospitalGameplay.h','    bool ArrivalCinematic()const {return ArrivalStage==0||ArrivalStage==1;}','''    bool ArrivalCinematic()const {return ArrivalStage==0||ArrivalStage==1||Seated31;}
    void BuildHeroVehicle31();
    void TickVehicle31(float Delta);
    bool VehicleInteract31();
    bool VehicleExitClear31()const;
    void ToggleVehiclePart31();
    FString VehiclePrompt31()const;
    FString NearestVehiclePart31()const;
    bool Seated31=false;
    float VehicleYaw31=0,VehiclePitch31=0,VehicleAutoClose31=0,WheelRoll31=0;
    FVector VehicleLastAt31=FVector::ZeroVector;
    UPROPERTY() TMap<FString,TObjectPtr<UStaticMeshComponent>> VehicleParts31;
    TMap<FString,float> VehicleOpen31,VehicleTarget31;''')
edit('WardHospitalArrival29.cpp','    WardEchoAssets::Attach(ArrivalCar,ArrivalCar->GetRootComponent(),Number(ArrivalData,TEXT("car_model"),640),FBox(FVector(-CarSize.X*.5f,-CarSize.Y*.5f,0),FVector(CarSize.X*.5f,CarSize.Y*.5f,CarSize.Z)));','    BuildHeroVehicle31();')
edit('WardHospitalArrival29.cpp','    ArrivalStage=-1;ArrivalClock=0;EntryClock=0;GateBlend=0;','''    ArrivalStage=-1;ArrivalClock=0;EntryClock=0;GateBlend=0;Seated31=false;VehicleYaw31=0;VehiclePitch31=0;VehicleAutoClose31=0;WheelRoll31=0;VehicleLastAt31=FVector::ZeroVector;
    for(auto& P:VehicleOpen31){P.Value=0;VehicleTarget31.Add(P.Key,0);if(auto* C=VehicleParts31.FindRef(P.Key))C->SetRelativeRotation(FRotator::ZeroRotator);}''')
edit('WardHospitalArrival29.cpp','    if(ArrivalStage==0){ArrivalClock=','    if(ArrivalStage>=2&&VehicleInteract31())return true;\n    if(ArrivalStage==0){ArrivalClock=')
edit('WardHospitalArrival29.cpp','        ArrivalStage=2;auto* H=Mode->Hero();','''        if(!VehicleExitClear31()){Mode->Notify(TEXT("Выход из машины перекрыт."));return true;}
        Seated31=false;VehicleTarget31.Add(TEXT("DoorFL"),1);VehicleAutoClose31=1.5f;
        ArrivalStage=2;auto* H=Mode->Hero();''')
edit('WardHospitalArrival29.cpp','    auto* H=Mode->Hero();\n    const float Goal=','    auto* H=Mode->Hero();TickVehicle31(D);\n    const float Goal=')
edit('WardHospitalArrival29.cpp','        if(ArrivalStage==0)ArrivalClock+=D;','''        if(Seated31&&ArrivalStage>=2)return;
        if(ArrivalStage==0)ArrivalClock+=D;''')
edit('WardHospitalArrival29.cpp','        const FVector Eye=At+FVector(-350,-540,215);ArrivalCamera->SetActorLocationAndRotation(Eye,(At+FVector(0,130,70)-Eye).Rotation());ArrivalCamera->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;','''        if(ArrivalStage==1){Seated31=true;TickVehicle31(0);}
        else {const FVector Eye=At+FVector(-350,-540,215);ArrivalCamera->SetActorLocationAndRotation(Eye,(At+FVector(0,130,70)-Eye).Rotation());}
        ArrivalCamera->GetCameraComponent()->PostProcessSettings=H->Camera->PostProcessSettings;''')
edit('WardHospitalArrival29.cpp','S->SetNumberField(TEXT("version"),29);','S->SetNumberField(TEXT("version"),31);')
edit('WardHospitalArrival29.cpp','S->SetNumberField(TEXT("gate_blend"),GateBlend);return S;','''S->SetNumberField(TEXT("gate_blend"),GateBlend);Json Doors=MakeShared<FJsonObject>();for(const auto& P:VehicleTarget31)Doors->SetNumberField(P.Key,P.Value);S->SetObjectField(TEXT("vehicle_doors31"),Doors);return S;''')
edit('WardHospitalArrival29.cpp','    ArrivalClock=FMath::Clamp','''    Seated31=false;VehicleAutoClose31=0;
    for(auto& P:VehicleOpen31){P.Value=FMath::Clamp(float(Number(Object(S,TEXT("vehicle_doors31")),P.Key)),0.f,1.f);VehicleTarget31.Add(P.Key,P.Value);}
    ArrivalClock=FMath::Clamp''')
edit('WardHospitalGameplay.cpp','    if(!Mode||!Mode->Hero())return FString();const FVector H=','    if(!Mode||!Mode->Hero())return FString();const FString CarPrompt=VehiclePrompt31();if(!CarPrompt.IsEmpty())return CarPrompt;const FVector H=')
edit('WardCampaignHero.cpp','if(G->HospitalStory&&G->HospitalStory->ArrivalCinematic())return;if(G->Admin)G->AdminViewRotation.Yaw','if(G->HospitalStory&&G->HospitalStory->ArrivalCinematic()){if(G->HospitalStory->Seated31)G->HospitalStory->VehicleYaw31=FMath::Clamp(G->HospitalStory->VehicleYaw31+V*G->Setting(TEXT("sensitivity"),1),-110.f,110.f);return;}if(G->Admin)G->AdminViewRotation.Yaw')
edit('WardCampaignHero.cpp','if(G->HospitalStory&&G->HospitalStory->ArrivalCinematic())return;const float Amount=','if(G->HospitalStory&&G->HospitalStory->ArrivalCinematic()){if(G->HospitalStory->Seated31)G->HospitalStory->VehiclePitch31=FMath::Clamp(G->HospitalStory->VehiclePitch31-V*G->Setting(TEXT("sensitivity"),1),-50.f,45.f);return;}const float Amount=')
edit('WardCampaignHero.cpp','    Key(EKeys::PageUp','    Key(EKeys::Y,[](auto* G){if(G->HospitalStory&&G->Screen.IsEmpty())G->HospitalStory->ToggleVehiclePart31();});\n    Key(EKeys::PageUp')
