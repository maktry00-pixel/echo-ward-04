#include "WardCampaign.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
using namespace WardData;
void AEWardCampaignMode::StashTransfer(const FString& Id,bool Deposit)
{
    if(!TArray<FString>{TEXT("ammo"),TEXT("shells"),TEXT("medkit"),TEXT("battery"),TEXT("bottle"),TEXT("boards")}.Contains(Id))return;
    if(Deposit){if(Id==TEXT("ammo")){if(Hero()->Reserve<=0)return;--Hero()->Reserve;}else{if(Items.FindRef(Id)<=0)return;Consume(Id);}++Stash.FindOrAdd(Id);}
    else{if(Stash.FindRef(Id)<=0||!Take(Id))return;--Stash[Id];}
}
void AEWardCampaignMode::TickAtmosphere(float D)
{
    if(!Hero())return;const FVector P=Hero()->GetActorLocation();const double X=P.Y/100,Z=-P.X/100;const bool Refuge=X>=4.4&&X<=13.2&&Z>=16.15&&Z<=23.1;
    Flag(TEXT("in_refuge"),Refuge);bool Threat=false;for(auto E:Enemies)if(E->Active&&!E->Dead&&E->Memory>0&&FVector::Dist(E->GetActorLocation(),P)<1800){Threat=true;break;}
    const float Mix=FMath::FInterpTo(NumberFlag(TEXT("danger_mix")),Threat&&!Refuge?1.f:0.f,D,1.5f);NumberFlag(TEXT("danger_mix"),Mix);
    const float Volume=Setting(TEXT("music"),.7)*(Voice&&Voice->IsPlaying()?.25f:1.f);if(Music)Music->SetVolumeMultiplier((Refuge?.03f:.20f)*(1-Mix*.7f)*Volume);if(Ambience)Ambience->SetVolumeMultiplier((Refuge?.06f:.18f)*Volume);if(Pursuit)Pursuit->SetVolumeMultiplier(Mix*.38f*Volume);if(RefugeMusic)RefugeMusic->SetVolumeMultiplier(Refuge?.24f*Volume:0);
    if(Refuge&&!Flag(TEXT("refuge_discovered"))){Flag(TEXT("refuge_discovered"),true);Notify(TEXT("Дежурная · тихая зона. Здесь есть журнал сохранений и личный шкаф."),8);AutoSave();}
    if(Threat){WorldCueTime=12;return;}if(Refuge||EscapeTime>=0)return;WorldCueTime=FMath::Max(0.f,WorldCueTime-D);if(WorldCueTime>0)return;
    if(Completed.Contains(TEXT("fuse"))&&!Flag(TEXT("beat_ward"))&&FMath::Abs(X)<14&&Z>0&&Z<8){Flag(TEXT("beat_ward"),true);Cue(TEXT("scrape"),FVector(-340,-590,120),.9f);Notify(TEXT("Из пустой палаты донёсся скрежет. Не задерживайся у ширмы."),5);WorldCueTime=12;}
    else if(Flag(TEXT("power_on"))&&!Flag(TEXT("badge"))&&!Flag(TEXT("beat_archive"))&&X>3&&X<14&&Z>-8&&Z<-1){Flag(TEXT("beat_archive"),true);Cue(TEXT("phone"),FVector(640,800,100),.9f);Notify(TEXT("На старом посту звонит телефон. В комнате никого нет."),5);WorldCueTime=12;}
    else if(Completed.Contains(TEXT("relay_morgue"))&&!Flag(TEXT("beat_lab"))&&X>30&&X<44&&Z>-8&&Z<7){Flag(TEXT("beat_lab"),true);Cue(TEXT("vent"),FVector(-580,3700,150),.9f);Notify(TEXT("Из вентиляции вырвался воздух. Дальше — операционная."),5);WorldCueTime=12;}
    else if(Elapsed>40){const FVector Anchor=P+Hero()->GetActorRightVector()*FMath::FRandRange(-800.f,800.f)+FVector(500,0,20);Cue(FMath::RandBool()?TEXT("whisper"):TEXT("creak"),Anchor,.18f);WorldCueTime=FMath::FRandRange(24.f,43.f);}
}



