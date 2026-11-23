#pragma once
#include "CoreMinimal.h"
class AEWardCampaignMode;
class AEWardCampaignHero;
class AActor;
namespace WardCombat63
{
 void Tick(AEWardCampaignMode& G,float Delta);
 void BeginHeal(AEWardCampaignMode& G);
 bool Healing(const AEWardCampaignMode& G);
 void Wound(AEWardCampaignMode& G,float Damage,AActor* Source);
 void Shot(AEWardCampaignMode& G);
 bool Observed(const AEWardCampaignMode& G);
 float RangeScale(bool Shotgun,float Centimeters);
 float BodyScale(float Fraction);
 float ActionTravel(const AEWardCampaignMode& G);
 float Trigger(const AEWardCampaignMode& G);
 float OpenHand(const AEWardCampaignMode& G);
 FVector PreviewGrip(const FString& Item,bool Left);
 void HitBlood(AEWardCampaignMode& G,FVector Point,FVector Direction,float Scale);
 void Blood(AEWardCampaignMode& G,FVector Point,FVector Normal,float Scale=1);
 void Sound(AEWardCampaignMode& G,const TCHAR* Clip,FVector Point,float Volume=1);
 void Assess(AEWardCampaignMode& G,float Delta);
}
