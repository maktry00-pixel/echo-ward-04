#include "WardAudio68.h"
#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardBookWorld.h"
#include "WardHospitalGameplay.h"
#include "WardPeople54.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundBase.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMemory.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "UnrealClient.h"
using namespace WardData;
namespace WardAudio68
{
namespace
{
Json Manifest(){static Json J=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Audio68/manifest.json"));return J;}
struct FPCM{TArray<uint8> Data;int32 Rate=48000,Channels=1;float Duration=0;double Access=0;};
TMap<FString,TSharedPtr<FPCM,ESPMode::ThreadSafe>> Cache;
TMap<FString,int32> Takes;
int64 CacheSize=0;constexpr int64 CacheBudget=24*1024*1024;
struct FVoice{TWeakObjectPtr<UAudioComponent> Audio;double End=0;bool Loop=false,UI=false;};
TArray<FVoice> Voices;
TMap<FString,double> Limits;
int32 Played=0,Rejected=0;
TAtomic<int32> LoopRefills{0};
TSharedPtr<FPCM,ESPMode::ThreadSafe> PCM(const FString& File)
{
 if(auto* P=Cache.Find(File)){(*P)->Access=FPlatformTime::Seconds();return *P;}
 TArray<uint8> Bytes;FWaveModInfo W;
 if(!FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectContentDir()/TEXT("CampaignData/Audio68")/File))||!W.ReadWaveInfo(Bytes.GetData(),Bytes.Num())||*W.pBitsPerSample!=16||(*W.pChannels!=1&&*W.pChannels!=2)||W.SampleDataSize==0||W.SampleDataSize>6*1024*1024)return nullptr;
 auto P=MakeShared<FPCM,ESPMode::ThreadSafe>();P->Rate=*W.pSamplesPerSec;P->Channels=*W.pChannels;P->Data.Append(W.SampleDataStart,W.SampleDataSize);P->Duration=float(W.SampleDataSize)/(2.f*P->Rate*P->Channels);P->Access=FPlatformTime::Seconds();
 while(CacheSize+P->Data.Num()>CacheBudget&&!Cache.IsEmpty())
 {
  FString Old;double Time=DBL_MAX;for(auto E:Cache)if(E.Value->Access<Time){Time=E.Value->Access;Old=E.Key;}
  CacheSize-=Cache[Old]->Data.Num();Cache.Remove(Old);
 }
 CacheSize+=P->Data.Num();Cache.Add(File,P);return P;
}
void Prune()
{
 const double Now=FPlatformTime::Seconds();
 for(int I=Voices.Num()-1;I>=0;--I)if(!Voices[I].Audio.IsValid()||!Voices[I].Audio->IsPlaying()||(!Voices[I].Loop&&Now>=Voices[I].End))
 {if(Voices[I].Audio.IsValid()){Voices[I].Audio->Stop();Voices[I].Audio->DestroyComponent();}Voices.RemoveAtSwap(I);}
}
struct FScene
{
 TWeakObjectPtr<UAudioComponent> Bed,OldBed,Window,LiftBed;
 FString Region,Screen,Scene,Goal;
 int32 Document=-1,Journal=0,Completed=0;
 float Clock=0,Breath=2,Cloth=0,RegionAge=0;
 bool Aim=false,Home=false,Lift=false,Hide=false,Dead=false;
 TMap<TWeakObjectPtr<AActor>,FString> HumanStates;
 TMap<TWeakObjectPtr<AActor>,double> HumanNext;
};
TMap<TWeakObjectPtr<AEWardCampaignMode>,FScene> Scenes;
FString Ambience(AEWardCampaignMode& G)
{
 if(G.HospitalStory&&G.HospitalStory->HomeStage52>=0)return TEXT("INTRO/home_room");
 const FString R=G.BookWorld&&G.Hero()?G.BookWorld->LocationAt(G.Hero()->GetActorLocation()):TEXT("1F");
 if(R==TEXT("ST"))return TEXT("AMB/outdoor");if(R==TEXT("RF"))return TEXT("AMB/roof");if(R==TEXT("UT"))return TEXT("AMB/tunnel");
 if(R==TEXT("B2"))return TEXT("AMB/helios");if(R==TEXT("B1"))return TEXT("AMB/morgue");
 if(R.StartsWith(TEXT("PS")))return TEXT("AMB/psy");if(R.StartsWith(TEXT("IF")))return TEXT("AMB/infection");
 if(R.StartsWith(TEXT("RH")))return TEXT("AMB/rehab");if(R.StartsWith(TEXT("DG")))return TEXT("AMB/diagnostic");
 if(R.StartsWith(TEXT("HS")))return TEXT("AMB/staff");if(R.StartsWith(TEXT("SV")))return TEXT("AMB/service");
 if(R.StartsWith(TEXT("X"))||R==TEXT("SHOP"))return TEXT("AMB/commercial");return TEXT("AMB/wards");
}
}
UAudioComponent* Play(AActor* Owner,const FString& Key,float Volume,const FVector* At,bool Loop)
{
 if(!Owner||!Owner->GetWorld()||Volume<=0||!Owner->GetWorld()->GetAudioDevice())return nullptr;
 Json Group=Object(Object(Manifest(),TEXT("groups")),Key);const auto& Files=Array(Group,TEXT("files"));if(Files.IsEmpty())return nullptr;
 Prune();const bool UI=At==nullptr;const double Now=FPlatformTime::Seconds();
 const FString Limit=FString::Printf(TEXT("%p|%s"),Owner,*Key);
 const float Interval=Number(Group,TEXT("interval"),UI?.06:.025);
 if(Now-Limits.FindRef(Limit)<Interval||Voices.Num()>=28){++Rejected;return nullptr;}Limits.Add(Limit,Now);
 if(Limits.Num()>2048)Limits.Reset();
 int32& Take=Takes.FindOrAdd(Key);const FString File=Files[Take++%Files.Num()]->AsString();auto P=PCM(File);if(!P)return nullptr;
 // Positional sources are exported mono. Stereo beds remain nonspatial.
 if(At&&P->Channels!=1)return nullptr;
 auto* Wave=NewObject<USoundWaveProcedural>(Owner);Wave->SetSampleRate(P->Rate);Wave->NumChannels=P->Channels;
 Wave->Duration=P->Duration;Wave->bLooping=Loop;Wave->QueueAudio(P->Data.GetData(),P->Data.Num());
 if(Loop)Wave->OnSoundWaveProceduralUnderflow=FOnSoundWaveProceduralUnderflow::CreateLambda([P](USoundWaveProcedural* W,int32){if(W->GetAvailableAudioByteCount()<4096){W->QueueAudio(P->Data.GetData(),P->Data.Num());++LoopRefills;}});
 auto* A=NewObject<UAudioComponent>(Owner);A->bAutoDestroy=false;A->bAllowSpatialization=!UI;A->SetUISound(UI);A->SetSound(Wave);
 A->SetVolumeMultiplier(FMath::Clamp(Volume*float(Number(Group,TEXT("gain"),1)),0.f,1.f));
 const float Pitch=Number(Group,TEXT("pitch_jitter"),.015);A->SetPitchMultiplier(1+FMath::FRandRange(-Pitch,Pitch));
 if(At){A->bOverrideAttenuation=true;auto& S=A->AttenuationOverrides;S.bAttenuate=true;S.bSpatialize=true;S.AttenuationShape=EAttenuationShape::Sphere;S.AttenuationShapeExtents=FVector(100);S.FalloffDistance=Number(Group,TEXT("range_cm"),2200);S.bEnableOcclusion=true;S.OcclusionTraceChannel=ECC_Visibility;S.OcclusionVolumeAttenuation=.28;S.OcclusionLowPassFilterFrequency=1800;}
 A->RegisterComponent();if(At)A->SetWorldLocation(*At);A->Play();
 Voices.Add({A,Now+P->Duration/.94+.12,Loop,UI});++Played;
 UE_LOG(LogTemp,Verbose,TEXT("AUDIO68_PLAY %s %s %.2fs %s"),*Key,*File,P->Duration,UI?TEXT("2D"):TEXT("3D"));return A;
}
FString FileKey(const FString& Path)
{
 FString P=Path;P.ReplaceInline(TEXT("\\"),TEXT("/"));int32 Start=P.Find(TEXT("CampaignData/"));if(Start<0)return FString();
 return String(Object(Manifest(),TEXT("file_aliases")),P.Mid(Start+13));
}
bool Cue(AEWardCampaignMode& G,const FString& Id,FVector At,float Volume)
{
 FString Key=String(Object(Manifest(),TEXT("aliases")),Id);if(Key.IsEmpty()&&Object(Manifest(),TEXT("groups"))->HasField(Id))Key=Id;
 if(Key.IsEmpty()||Id.StartsWith(TEXT("story_")))return false;
 if(Id==TEXT("step")&&G.BookWorld){const FString Region=G.BookWorld->LocationAt(At);if(Region!=TEXT("ST"))Key=Region==TEXT("RF")||Region==TEXT("UT")?TEXT("STEP/metal_plate"):Region.StartsWith(TEXT("PS"))||Region.StartsWith(TEXT("RH"))?TEXT("STEP/vinyl"):TEXT("STEP/tile_dry");}
 Play(&G,Key,Volume*G.Setting(TEXT("effects"),.8),&At);return true;
}
void Page(AEWardCampaignMode& G,const FString& From,const FString& To)
{
 if(From==To||From.IsEmpty()&&To==TEXT("title"))return;
 const FString Key=To==TEXT("journal")?TEXT("UI/paper_open"):From==TEXT("journal")?TEXT("UI/paper_close"):To==TEXT("map")?TEXT("UI/map"):To.IsEmpty()?TEXT("UI/back"):TEXT("UI/page");
 Play(&G,Key,.16f*G.Setting(TEXT("effects"),.8));
}
bool Command(AEWardCampaignMode& G,const FString& Id)
{
 static TWeakObjectPtr<UAudioComponent> Preview;
 if(Id.StartsWith(TEXT("audio68_")))
 {
  if(!G.Admin)return true;
  if(Preview.IsValid()){Preview->Stop();Preview->DestroyComponent();Preview.Reset();}
  if(Id==TEXT("audio68_catalog"))G.Show(TEXT("admin_audio"));
  else if(Id==TEXT("audio68_back"))G.Show(TEXT("admin_catalog"));
  else if(Id.StartsWith(TEXT("audio68_play:")))Preview=Play(&G,Id.RightChop(13),.35f*G.Setting(TEXT("effects"),.8));
  return true;
 }
 FString Key;
 if(Id.StartsWith(TEXT("setting_"))||Id==TEXT("settings_reset"))Key=TEXT("UI/settings");
 else if(Id.StartsWith(TEXT("field_"))||Id.StartsWith(TEXT("inv_")))Key=TEXT("UI/slot");
 else if(Id.StartsWith(TEXT("story35_choice_")))Key=TEXT("UI/confirm");
 if(!Key.IsEmpty())Play(&G,Key,.12f*G.Setting(TEXT("effects"),.8));
 return false;
}
void Human(AEWardCampaignMode& G,AActor* Person,const FString& Identity,const FString& Event,bool Female)
{
 if(!Person||Person->IsHidden()||!G.Hero()||FVector::DistSquared(Person->GetActorLocation(),G.Hero()->GetActorLocation())>FMath::Square(2000.f))return;
 const double Now=FPlatformTime::Seconds();const FString Limit=FString::Printf(TEXT("human:%p:%s"),Person,*Event);
 if(Now<Limits.FindRef(Limit))return;Limits.Add(Limit,Now+(Event==TEXT("flee")?12:Event==TEXT("hurt")?1:4));
 const FString Suffix=Female?TEXT("_f"):TEXT("_m");
 const FString Key=Event==TEXT("death")?TEXT("HUMAN/scream")+Suffix:Event==TEXT("flee")?TEXT("HUMAN/fear")+Suffix:Event==TEXT("hurt")?TEXT("HUMAN/pain")+Suffix:Event==TEXT("cough")?TEXT("HUMAN/cough"):Event==TEXT("smoking")?TEXT("BODY/npc_smoke"):TEXT("HUMAN/breath")+Suffix;
 const FVector At=Person->GetActorLocation()+FVector(0,0,140);Play(Person,Key,(Event==TEXT("death")?.38f:Event==TEXT("flee")?.27f:.14f)*G.Setting(TEXT("effects"),.8),&At);
}
void Tick(AEWardCampaignMode& G,float D)
{
 Prune();for(auto I=Scenes.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();
 if(!G.Hero())return;auto& S=Scenes.FindOrAdd(&G);if(G.Screen==TEXT("journal")&&G.Interface.IsValid()){const int32 Document=G.Interface->DocumentIndex;if(S.Document>=0&&S.Document!=Document)Play(&G,TEXT("UI/paper_open"),.14f*G.Setting(TEXT("effects"),.8));S.Document=Document;}else S.Document=-1;const bool Live=G.CampaignStarted&&!G.TitleView&&(G.Screen.IsEmpty()||G.Screen==TEXT("story35"))&&!G.Won&&!G.Hero()->Dead;
 if(!Live){for(auto W:{S.Bed,S.OldBed,S.Window,S.LiftBed})if(W.IsValid())W->SetVolumeMultiplier(0);return;}
 const bool Speech=(G.Voice&&G.Voice->IsPlaying())||(G.HospitalStory&&G.HospitalStory->DialogueAudio62&&G.HospitalStory->DialogueAudio62->IsPlaying());
 const float FX=G.Setting(TEXT("effects"),.8),BedVolume=.16f*G.Setting(TEXT("music"),.7)*(Speech?.35f:1.f);
 const FString R=Ambience(G);
 if(S.Region!=R||!S.Bed.IsValid())
 {
  if(S.OldBed.IsValid()){S.OldBed->Stop();S.OldBed->DestroyComponent();}S.OldBed=S.Bed;S.Region=R;S.RegionAge=0;
  S.Bed=Play(&G,R,BedVolume,nullptr,true);
 }
 S.RegionAge=FMath::Min(1.5f,S.RegionAge+D);const float Mix=S.RegionAge/1.5;
 if(S.Bed.IsValid())S.Bed->SetVolumeMultiplier(BedVolume*Mix);
 if(S.OldBed.IsValid()){S.OldBed->SetVolumeMultiplier(BedVolume*(1-Mix));if(Mix>=1){S.OldBed->Stop();S.OldBed->DestroyComponent();S.OldBed.Reset();}}
 // The new region bed replaces the old generic loop rather than doubling it.
 if(S.Bed.IsValid()&&G.Ambience)G.Ambience->Stop();
 const bool Home=G.HospitalStory&&G.HospitalStory->HomeStage52>=0;
 if(Home&&!S.Window.IsValid())S.Window=Play(&G,TEXT("INTRO/window"),.04f*FX,nullptr,true);
 if(S.Window.IsValid()){S.Window->SetVolumeMultiplier(Home?.04f*FX:0);if(!Home){S.Window->Stop();S.Window->DestroyComponent();S.Window.Reset();}}
 const FVector At=G.Hero()->GetActorLocation();
 if(S.Aim!=G.Hero()->Aiming){S.Aim=G.Hero()->Aiming;Play(&G,TEXT("FIGHT/aim"),.12f*FX);}
 const bool Lift=G.BookWorld&&G.BookWorld->LiftMoving,Hide=G.HidingBed||(G.BookWorld&&G.BookWorld->FurnitureHiding);
 if(Lift!=S.Lift){S.Lift=Lift;Play(&G,TEXT("SYSTEM/elevator"),.22f*FX,&At);}
 if(Lift&&!S.LiftBed.IsValid())S.LiftBed=Play(&G,TEXT("SYSTEM/lift_motor"),.11f*FX,nullptr,true);
 if(S.LiftBed.IsValid()){S.LiftBed->SetVolumeMultiplier(Lift?.11f*FX:0);if(!Lift){S.LiftBed->Stop();S.LiftBed->DestroyComponent();S.LiftBed.Reset();}}
 if(Hide!=S.Hide){S.Hide=Hide;Play(&G,TEXT("DOOR/hide"),.18f*FX,&At);}
 S.Breath-=D;const float Speed=G.Hero()->GetVelocity().Size2D();
 if(S.Breath<=0){S.Breath=Speed>380?3.4f:7.5f;if(G.Hero()->Stamina>=65&&G.Hero()->Health>=35)Play(&G,Speed>380?TEXT("BODY/exertion"):TEXT("BODY/idle"),Speed>380?.16f*FX:.045f*FX);}
 S.Cloth-=D;if(Speed>15&&S.Cloth<=0){S.Cloth=Speed>380?1.6f:3.1f;Play(&G,TEXT("BODY/cloth"),.075f*FX);}
 S.Clock-=D;if(S.Clock>0)return;S.Clock=.6f;
 if(G.Journal.Num()>S.Journal&&S.Journal>0)Play(&G,TEXT("UI/evidence"),.12f*FX);S.Journal=G.Journal.Num();
 if(G.Completed.Num()>S.Completed&&S.Completed>0)Play(&G,TEXT("UI/quest"),.11f*FX);S.Completed=G.Completed.Num();
 if(!G.HospitalStory)return;
 for(auto E:G.HospitalStory->Props)
 {
  auto* Person=E.Value;if(!Person||Person->IsHidden()||FVector::DistSquared(Person->GetActorLocation(),At)>FMath::Square(1800.f))continue;
  auto* Body=Person->FindComponentByClass<UEWardPerson54>();if(!Body)continue;
  const FString State=Body->HumanState58();FString& Last=S.HumanStates.FindOrAdd(Person);
  static Json Profiles=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People55/profiles.json"));
  const bool Female=String(Object(Profiles,E.Key),TEXT("motion")).StartsWith(TEXT("f_"));
  if(State!=Last){if(State==TEXT("flee")||State==TEXT("smoke"))Human(G,Person,E.Key,State==TEXT("smoke")?TEXT("smoking"):State,Female);Last=State;}
  double& Next=S.HumanNext.FindOrAdd(Person);const double Now=FPlatformTime::Seconds();
  if(Now>=Next&&!Body->IsDead58()&&State!=TEXT("talk")&&State!=TEXT("flee")){Next=Now+13+GetTypeHash(E.Key)%13;Human(G,Person,E.Key,Body->Health62()<65?TEXT("cough"):TEXT("breath"),Female);}
 }
}
void Stop(AEWardCampaignMode& G)
{
 if(auto* S=Scenes.Find(&G))for(auto A:{S->Bed,S->OldBed,S->Window,S->LiftBed})if(A.IsValid()){A->Stop();A->DestroyComponent();}Scenes.Remove(&G);Prune();
}
void Assess(AEWardCampaignMode& G,float Delta)
{
 static float Age=0;Age+=Delta;static bool Started=false,Picture=false,Unpaused=false;static Json Result=MakeShared<FJsonObject>();static TArray<TSharedPtr<FJsonValue>> Checks;static int Step=0;static TWeakObjectPtr<UAudioComponent> TestLoop;
 auto Check=[&](FString Name,bool Pass){Json R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));};
 if(!Started)
 {
  Started=true;G.Assessment=true;G.TitleView=false;G.CampaignStarted=true;G.Admin=true;G.Show(TEXT("admin_audio"));
  const auto Groups=Object(Manifest(),TEXT("groups"));Check(TEXT("manifest_revision"),Number(Manifest(),TEXT("revision"))==68);Check(TEXT("library_has_all_catalogue_groups"),Groups->Values.Num()>=161);
  for(auto E:Groups->Values)for(auto V:Array(E.Value->AsObject(),TEXT("files"))){auto P=PCM(V->AsString());Check(FString(TEXT("pcm:"))+FString(E.Key)+TEXT(":")+V->AsString(),P.IsValid()&&P->Duration>.03f&&P->Rate==48000);}
  for(auto E:Object(Manifest(),TEXT("aliases"))->Values)Check(FString(TEXT("cue:"))+FString(E.Key),Groups->HasField(E.Value->AsString()));
  for(auto E:Object(Manifest(),TEXT("file_aliases"))->Values){Check(FString(TEXT("path:"))+FString(E.Key),Groups->HasField(E.Value->AsString())&&!FString(E.Key).Contains(TEXT("speech"))&&!FString(E.Key).Contains(TEXT("voice"))&&!FString(E.Key).Contains(TEXT("line")));}
  Check(TEXT("cache_budget_24MiB"),CacheSize<=CacheBudget);Check(TEXT("spatial_device_present"),!!G.GetWorld()->GetAudioDevice());TestLoop=Play(&G,TEXT("MENU/electric"),.025,nullptr,true);Age=0;
 }
 // Tick still runs while the administrator panel pauses the world.
 if(!Picture){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/audio68_catalog.png"),true,false);Picture=true;return;}
 if(!Unpaused){G.Admin=false;G.Show(TEXT(""));Unpaused=true;}
 const TCHAR* Keys[]={TEXT("UI/paper_pick"),TEXT("STEP/tile_dry"),TEXT("DOOR/steel"),TEXT("FIGHT/pistol"),TEXT("HUMAN/scream_m"),TEXT("HUMAN/scream_f"),TEXT("MON/hound/notice"),TEXT("INTRO/car_drive")};
 if(Step<UE_ARRAY_COUNT(Keys)&&Age>=Step*1.5f)
 {
  const FVector At=G.Hero()->GetActorLocation()+FVector(150,0,100);auto* A=Play(&G,Keys[Step],.12,&At);Check(FString(TEXT("device_play:"))+Keys[Step],A&&A->Sound&&A->IsPlaying());++Step;
 }
 if(Age<14)return;
 auto* Loop=Play(&G,TEXT("AMB/tunnel"),.02,nullptr,true);Check(TEXT("loop_spawn"),Loop&&Loop->Sound);const int BeforeStop=Voices.Num();if(Loop){Loop->Stop();Loop->DestroyComponent();}Prune();Check(TEXT("stopped_preview_releases_source_immediately"),Voices.Num()<BeforeStop);
 Check(TEXT("muted_does_not_spawn"),!Play(&G,TEXT("UI/confirm"),0));Check(TEXT("unknown_key_safe"),!Play(&G,TEXT("missing"),.2));Check(TEXT("voices_bounded"),Voices.Num()<=28);
 Check(TEXT("loop_crossed_underflow_and_still_playing"),TestLoop.IsValid()&&TestLoop->IsPlaying()&&LoopRefills.Load()>0);if(TestLoop.IsValid()){TestLoop->Stop();TestLoop->DestroyComponent();}
 Result->SetNumberField(TEXT("loop_refills"),LoopRefills.Load());
 Result->SetArrayField(TEXT("checks"),Checks);Result->SetNumberField(TEXT("count"),Checks.Num());Result->SetNumberField(TEXT("played"),Played);Result->SetNumberField(TEXT("rejected"),Rejected);Result->SetNumberField(TEXT("cache_bytes"),CacheSize);Result->SetNumberField(TEXT("memory_used_mb"),FPlatformMemory::GetStats().UsedPhysical/1048576.);
 bool Success=true;for(auto V:Checks)Success&=Boolean(V->AsObject(),TEXT("passed"));Result->SetBoolField(TEXT("success"),Success);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/audio68_check.json"),Result);Stop(G);G.GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
}
}
