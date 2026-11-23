from pathlib import Path
import shutil
R=Path('unreal/EchoWard/Source/EchoWard');B=Path('artifacts/echo_cars_v31/source_before');B.mkdir(exist_ok=True)
files=['WardEchoAssets.h','WardEchoAssets.cpp','WardBookWorld.h','WardBookWorld.cpp','WardHospitalFurnishing.cpp','WardHospitalQuality.cpp','WardHospitalGameplay.h','WardHospitalGameplay.cpp','WardHospitalArrival29.cpp','WardCampaignHero.cpp','WardHospitalArrivalAssessment29.cpp','WardCampaignAssessment.cpp','WardBookWorldAssessment.cpp']
for name in files:
 if not (B/name).exists():shutil.copy2(R/name,B/name)
def edit(name,a,b):
 p=R/name;s=p.read_text(encoding='utf8');assert a in s,(name,a[:80]);p.write_text(s.replace(a,b),encoding='utf8')
edit('WardEchoAssets.h','    int ResolvedId','    WardData::Json Cars31();\n    int ResolvedId')
edit('WardEchoAssets.cpp','int WardEchoAssets::ResolvedId','Json WardEchoAssets::Cars31(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_cars_v31.json"));return D;}\nint WardEchoAssets::ResolvedId')
edit('WardBookWorld.h','    int Realism30Props=0;','    int Realism30Props=0;\n    void BuildCars31();\n    int Cars31Count=0;\n    TSet<FString> Cars31Sources,Cars31Replacements;\n    TArray<FBox> Cars31Bounds;')
edit('WardBookWorld.cpp','    BuildPrototypeFurnishings();','    BuildCars31();\n    BuildPrototypeFurnishings();')
edit('WardHospitalFurnishing.cpp','        const FString Mount=String(Prop,TEXT("mount"));','''        if(Cars31Replacements.Contains(String(Prop,TEXT("id"))))
        {PrototypeParts+=Array(Model,TEXT("parts")).Num();if(String(Prop,TEXT("id")).StartsWith(TEXT("ENV9_")))++EnvironmentProps;else ++PrototypeProps;continue;}
        const FString Mount=String(Prop,TEXT("mount"));''')
edit('WardHospitalQuality.cpp','        const auto& Corrected=','        if(Cars31Replacements.Contains(String(P,TEXT("id")))){++QualityProps;QualityReplacedItems.Add(String(P,TEXT("id")));continue;}\n        const auto& Corrected=')
