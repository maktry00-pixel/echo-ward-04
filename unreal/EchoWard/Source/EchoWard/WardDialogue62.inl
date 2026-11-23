#include "WardDialogue62.h"
#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardStaging64.h"
#include "GameFramework/PlayerController.h"
#include "WardPeople54.h"
#include "WardMonster59.h"
#include "WardHomeAssets53.h"
#include "Components/AudioComponent.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
using namespace WardData;
namespace WardDialogue62
{
Json Data(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Dialogue62/dialogue.json"));return D;}
namespace
{
 Json State(AEWardHospitalGameplay& S){if(!S.NarrativeState35->HasTypedField<EJson::Object>(TEXT("dialogue62")))S.NarrativeState35->SetObjectField(TEXT("dialogue62"),MakeShared<FJsonObject>());return Object(S.NarrativeState35,TEXT("dialogue62"));}
 UEWardPerson54* Person(AEWardHospitalGameplay& S,const FString& Id){auto* A=S.Props.FindRef(Id);return A?A->FindComponentByClass<UEWardPerson54>():nullptr;}
 bool Available(AEWardHospitalGameplay& S,const FString& Id){auto* A=S.Props.FindRef(Id);auto* P=Person(S,Id);return A&&!A->IsHidden()&&P&&!P->IsDead58()&&!P->IsFallen58()&&!Boolean(S.Definitions.FindRef(Id),TEXT("past57"));}
 bool Threat(AEWardHospitalGameplay& S,const FVector& At,float Range)
 {
  for(auto M:S.Mobs)if(M&&M->Active&&!M->Dead&&!M->IsHidden()&&M->Stunned<=0&&FMath::Abs(M->GetActorLocation().Z-At.Z-85)<200&&FVector::Dist2D(At,M->GetActorLocation())<Range&&WardMonster59::ContactThreatAt(*M,At)&&M->Sight59(At+FVector(0,0,85),true))return true;
  return false;
 }
 bool PairReady(AEWardHospitalGameplay& S,Json G)
 {
  if(S.Chapter()<Number(G,TEXT("min_chapter"))||S.Chapter()>Number(G,TEXT("max_chapter"),19))return false;
  FVector Last=FVector::ZeroVector;
  for(auto V:Array(G,TEXT("actors")))
  {
   const FString Id=V->AsString();if(!Available(S,Id))return false;const FVector At=S.Props[Id]->GetActorLocation();
   if(FVector::Dist2D(At,S.Mode->Hero()->GetActorLocation())>1100||FMath::Abs(At.Z-S.Mode->Hero()->GetActorLocation().Z+91)>120||Threat(S,At,1000))return false;
   if(!Last.IsZero())
   {
    if(FVector::Dist2D(Last,At)>650||FMath::Abs(Last.Z-At.Z)>80)return false;
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(WorldConversation62),false);
    for(auto PersonId:Array(G,TEXT("actors")))if(auto* A=S.Props.FindRef(PersonId->AsString()))Q.AddIgnoredActor(A);
    if(S.GetWorld()->LineTraceSingleByChannel(Hit,Last+FVector(0,0,140),At+FVector(0,0,140),ECC_Visibility,Q))return false;
   }
   Last=At;
  }
  return true;
 }
 TArray<FString> SpokenLines(const FString& Script)
 {
  TArray<FString> Raw, Lines;
  Script.ParseIntoArrayLines(Raw);
  const Json Spoken = Object(Data(), TEXT("spoken"));
  for (FString& Line : Raw)
  {
   Line.TrimStartAndEndInline();
   FString Speaker, Speech;
   const bool HasSpeaker = Line.Split(TEXT(": "), &Speaker, &Speech);
   const bool Direction = HasSpeaker &&
       (Speaker.Contains(TEXT("разворачивает")) || Speaker.Contains(TEXT("выполняет")) ||
        Speaker.Contains(TEXT("читает")) || Speaker.Contains(TEXT("показывает")) ||
        Speaker.Contains(TEXT("подключает")) || Speaker.Contains(TEXT("открывает")) ||
        Speaker.Contains(TEXT("перечитывает")));
   if (Direction)
   {
    // Stage directions stay in the script, never in a character's voice.
    // Preserve the actual quoted question and the authored vent response.
    if (Speaker.Contains(TEXT("спрашивает")))
    {
     int Open = INDEX_NONE, Close = INDEX_NONE;
     if (Speech.FindChar(TCHAR(0x00ab), Open) && Speech.FindChar(TCHAR(0x00bb), Close) && Close > Open)
     {
      FString Name, Rest;
      Speaker.Split(TEXT(" "), &Name, &Rest);
      const FString Question = Name + TEXT(": ") + Speech.Mid(Open + 1, Close - Open - 1);
      if (Spoken->HasField(Question)) Lines.Add(Question);
     }
     if (Speech.Contains(TEXT("Из вентиляции"))) Lines.Add(TEXT("Голос из вентиляции: Не помню."));
    }
    continue;
   }
   // Unknown direct speech can still be displayed with a short speaker label.
   if (Spoken->HasField(Line) ||
       (Line.Split(TEXT(": "), &Speaker, &Speech) && Speaker.Len() <= 32 &&
        !Speaker.Contains(TEXT(".")) && Speaker != TEXT("Окончательная операция") && !Speech.IsEmpty()))
    Lines.Add(MoveTemp(Line));
  }
  return Lines;
 }
 TArray<FString> ChoiceLines(AEWardHospitalGameplay& S, const TArray<TSharedPtr<FJsonValue>>& Options)
 {
  auto Body = SpokenLines(S.NarrativeBody35(true));
  if (S.ActiveScene35 == TEXT("CHECKPOINT_69")) return Body;
  const FString Selected = S.SelectedChoice35;
  S.SelectedChoice35.Reset();
  auto Setup = SpokenLines(S.NarrativeBody35(true));
  int Shared = Setup.Num();
  for (const auto& Value : Options)
  {
   S.SelectedChoice35 = String(Value->AsObject(), TEXT("id"));
   const auto Branch = SpokenLines(S.NarrativeBody35(true));
   int Match = 0;
   while (Match < Shared && Match < Branch.Num() && Branch[Match] == Setup[Match]) ++Match;
   Shared = Match;
  }
  S.SelectedChoice35 = Selected;
  if (Selected.IsEmpty())
  {
   Setup.SetNum(Shared);
   return Setup;
  }
  Body.RemoveAt(0, Shared);
  return Body;
 }
 void Play(AEWardHospitalGameplay& S,const FString& Key)
 {
  const Json Row=Object(Object(Data(),TEXT("spoken")),Key);
  S.DialogueKey62=Key;S.DialogueActor62=String(Row,TEXT("actor"));S.DialogueTime62=0;
  const bool CheckpointIntercom = S.ActiveScene35 == TEXT("CHECKPOINT_69") && Key.StartsWith(TEXT("Әсел:"));
  if (CheckpointIntercom) S.DialogueActor62 = TEXT("remote");
  FVector At=S.Mode->Hero()->GetActorLocation();
  if(auto* Actor=S.Props.FindRef(S.DialogueActor62);Actor&&!Actor->IsHidden()&&FVector::Dist2D(At,Actor->GetActorLocation())<1500&&FMath::Abs(At.Z-Actor->GetActorLocation().Z-91)<200)At=Actor->GetActorLocation()+FVector(0,0,145);
  const FString VoiceId = String(Row, TEXT("id"));
  if (!VoiceId.IsEmpty() && !Key.Contains(TEXT(", сообщение:")))
  {
   const FString Path = FPaths::ProjectContentDir() / TEXT("CampaignData/Dialogue62/voice") / (VoiceId + TEXT(".wav"));
   if (CheckpointIntercom)
    At = Vector(Array(S.ArrivalData, TEXT("panel"))) * 100 + FVector(0,-30,0);
   S.DialogueAudio62 = WardHomeAssets::PlaySpatialFile61(&S, Path, .82f*S.Mode->Setting(TEXT("effects"),.8), At);
  }
  const float ReadTime = FMath::Clamp(Key.Len() / 17.f, 1.25f, 30.f);
  S.DialogueLeft62 = FMath::Max(ReadTime, float(Number(Row, TEXT("duration")))) + .35f;
 }
 void Begin(AEWardHospitalGameplay& S,const FString& Session,const TArray<TSharedPtr<FJsonValue>>& Lines,bool Main)
 {
  Stop(S);S.DialogueSession62=Session;S.DialogueMain62=Main;S.DialogueLines62.Reset();
  for(auto V:Lines)S.DialogueLines62.Add(V->AsString());
  S.DialogueIndex62=Main?FMath::Clamp(int(Number(State(S),TEXT("cursor:")+Session)),0,S.DialogueLines62.Num()):0;
  S.DialogueLeft62=.35f;
 }
}
void Stop(AEWardHospitalGameplay& S,bool Suspend)
{
 if(IsValid(S.DialogueAudio62))S.DialogueAudio62->Stop();S.DialogueAudio62=nullptr;S.DialogueKey62.Empty();S.DialogueActor62.Empty();S.DialogueLeft62=0;
 if(Suspend){S.DialogueSession62.Empty();S.DialogueLines62.Empty();}
}
bool PageFinished(const AEWardHospitalGameplay& S)
{
 return S.DialogueMain62 && !S.ActiveScene35.IsEmpty()
  && S.DialogueSession62.StartsWith(S.ActiveScene35 + TEXT("/"))
  && S.DialogueIndex62 >= S.DialogueLines62.Num() && S.DialogueLeft62 <= 0;
}
bool AwaitingChoice(AEWardHospitalGameplay& S)
{
 return S.SelectedChoice35.IsEmpty() && PageFinished(S) && WardStaging64::ChoiceReady(S)
  && !Array(S.NarrativeScenes35.FindRef(S.ActiveScene35), TEXT("choices")).IsEmpty();
}
bool HasControls(AEWardHospitalGameplay& S)
{
 if (AwaitingChoice(S) || WardStaging64::WaitingAction(S)) return true;
 if (!S.ActiveScene35.StartsWith(TEXT("NPC_"))) return false;
 const auto* P = Person(S, S.ActiveScene35.RightChop(4));
 return P && !P->IsDead58() && (P->CanEscort58() || P->IsFallen58());
}
bool Speaking(const AEWardHospitalGameplay& S,const FString& Actor){return S.DialogueActor62==Actor&&IsValid(S.DialogueAudio62)&&S.DialogueAudio62->IsPlaying();}
float Mouth(const AEWardHospitalGameplay& S,const FString& Actor)
{
 if(!Speaking(S,Actor))return 0;const auto& Env=Array(Object(Object(Data(),TEXT("spoken")),S.DialogueKey62),TEXT("envelope"));int I=FMath::FloorToInt(S.DialogueTime62*30);return Env.IsValidIndex(I)?float(Env[I]->AsNumber()):FMath::Abs(FMath::Sin(S.DialogueTime62*15))*.4f;
}
void Event(AEWardHospitalGameplay& S,const FString& Actor,const FString& Kind)
{
 // The final aftermath is over; new incidental death/danger reactions
 // must not intrude on the completed ending. Explicit replay still works.
 if(S.Chapter()>19&&S.NarrativeFlag35(TEXT("EPILOGUE_COMPLETE")))return;
 if(!S.Mode||!S.Mode->Hero()||!S.Mode->Screen.IsEmpty()&&S.Mode->Screen!=TEXT("story35")||!Available(S,Actor))return;
 if(FVector::Dist(S.Props[Actor]->GetActorLocation(),S.Mode->Hero()->GetActorLocation())>1500)return;
 const FString Flag=TEXT("event:")+Actor+TEXT(":")+Kind;const double Now=S.Mode->Elapsed;
 if(Kind==TEXT("death_seen")&&State(S)->HasField(Flag))return;
 if(State(S)->HasField(Flag)&&Now-Number(State(S),Flag)<(Kind==TEXT("danger")?18:35))return;
 const auto& Lines=Array(Object(Object(Data(),TEXT("events")),Actor),Kind);if(Lines.IsEmpty())return;
 // Danger interrupts speech without advancing the interrupted line or plot.
 if(Kind==TEXT("danger")||Kind==TEXT("hurt"))
 {
  Stop(S,true);if(S.Mode->Screen==TEXT("story35")){S.SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),true);S.Mode->Show(TEXT(""));}
 }
 else if(!S.DialogueSession62.IsEmpty())return;
 const int Take=int(Number(State(S),Flag+TEXT(":take")))%Lines.Num();State(S)->SetNumberField(Flag,Now);State(S)->SetNumberField(Flag+TEXT(":take"),Take+1);
 Begin(S,TEXT("event/")+Flag,{Lines[Take]},false);
}
void Tick(AEWardHospitalGameplay& S,float D)
{
 if(!S.Mode||!S.Mode->Hero()||!S.Mode->CampaignStarted||S.Mode->TitleView||S.HomeStage52>=0||S.ArrivalCinematic())return;
 if(S.Chapter()>19&&S.NarrativeFlag35(TEXT("EPILOGUE_COMPLETE"))&&S.ActiveScene35.IsEmpty()){Stop(S,true);return;}
 if (S.Mode->Hero()->Dead)
 {
  Stop(S, true);
  if (S.Mode->Screen == TEXT("story35"))
  {
   S.SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"), true);
   S.Mode->Show(TEXT(""));
  }
  return;
 }
 if(!S.Mode->Screen.IsEmpty()&&S.Mode->Screen!=TEXT("story35")){Stop(S,true);return;}
 const bool Main=S.Mode->Screen==TEXT("story35")&&!S.ActiveScene35.IsEmpty();
 if(Main&&S.ActiveScene35.StartsWith(TEXT("NPC_")))
 {
  auto* P=Person(S,S.ActiveScene35.RightChop(4));
  if(P&&P->IsDead58()){Stop(S,true);S.ActiveScene35.Empty();S.Mode->Show(TEXT(""));return;}
 }
 const auto& Options = Array(S.NarrativeScenes35.FindRef(S.ActiveScene35), TEXT("choices"));
 const bool HasChoices = !Options.IsEmpty();
 FString Session=S.ActiveScene35+TEXT("/")+(HasChoices ? FString(TEXT("choices69")) : FString::FromInt(S.DialoguePage35))+TEXT("/")+S.SelectedChoice35;
 // Archive NPCs cycle different personal lines. Their second line must not
 // inherit the completed cursor of a different text on the same UI page.
 if(S.ActiveScene35.StartsWith(TEXT("NPC_")))Session+=TEXT("/")+FString::FromInt(int32(GetTypeHash(S.NarrativeBody35())));
 if(Main&&Threat(S,S.Mode->Hero()->GetActorLocation()-FVector(0,0,91),700))
 {Stop(S,true);S.SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),true);S.Mode->Show(TEXT(""));return;}
 if(Main&&Session!=S.DialogueSession62)
 {
  const auto Text = HasChoices ? ChoiceLines(S, Options) : SpokenLines(S.NarrativeBody35());
  TArray<TSharedPtr<FJsonValue>> Lines;
  for (const auto& Line : Text) Lines.Add(MakeShared<FJsonValueString>(Line));
  Begin(S,Session,Lines,true);
  if(S.Mode->StringFlag(TEXT("skip76_scene"))==S.ActiveScene35)
  {
   S.DialogueIndex62=S.DialogueLines62.Num();S.DialogueLeft62=0;S.DialogueKey62.Empty();
   State(S)->SetNumberField(TEXT("cursor:")+Session,S.DialogueIndex62);
  }
 }
 else if(!Main&&S.DialogueMain62&&!S.DialogueSession62.IsEmpty())Stop(S,true);
 if(!S.DialogueSession62.IsEmpty())
 {
  if(!S.DialogueMain62&&!S.DialogueActor62.IsEmpty()&&S.DialogueActor62!=TEXT("hero")&&S.DialogueActor62!=TEXT("remote"))
  {auto* P=Person(S,S.DialogueActor62);if(!P||P->IsDead58()||P->IsFallen58()){Stop(S,true);return;}}
  if(IsValid(S.DialogueAudio62))
  {
   FVector At=S.Mode->Hero()->GetActorLocation();
   if(auto* A=S.Props.FindRef(S.DialogueActor62);A&&!A->IsHidden()&&FVector::Dist2D(At,A->GetActorLocation())<1500&&FMath::Abs(At.Z-A->GetActorLocation().Z-91)<200)At=A->GetActorLocation()+FVector(0,0,145);
   if (S.ActiveScene35 == TEXT("CHECKPOINT_69") && S.DialogueKey62.StartsWith(TEXT("Әсел:")))
    At = Vector(Array(S.ArrivalData, TEXT("panel"))) * 100 + FVector(0,-30,0);
   S.DialogueAudio62->SetWorldLocation(At);
  }
  S.DialogueLeft62-=D;S.DialogueTime62+=D;
  if(S.DialogueLeft62>0)return;
  if(!S.DialogueKey62.IsEmpty()){++S.DialogueIndex62;Stop(S);if(S.DialogueMain62)State(S)->SetNumberField(TEXT("cursor:")+S.DialogueSession62,S.DialogueIndex62);}
  if(S.DialogueLines62.IsValidIndex(S.DialogueIndex62))
  {
   const FString Key=S.DialogueLines62[S.DialogueIndex62];const FString Speaker=String(Object(Object(Data(),TEXT("spoken")),Key),TEXT("actor"));
   if(!S.DialogueMain62&&Speaker!=TEXT("hero")&&Speaker!=TEXT("remote")&&!Available(S,Speaker)){Stop(S,true);return;}
   Play(S,Key);return;
  }
  if(S.DialogueMain62)
  {
   // Rebuild the overlay only when control passes to the player.
   if (auto* PC = S.GetWorld()->GetFirstPlayerController(); PC && PC->bShowMouseCursor != HasControls(S))
    if (S.Mode->Interface.IsValid()) S.Mode->Interface->Refresh();
   return;
  }
  State(S)->SetBoolField(TEXT("done:")+S.DialogueSession62,true);S.DialogueSession62.Empty();S.DialogueLines62.Empty();S.DialogueCooldown62=20;
 }
 if(Main||!S.ActiveScene35.IsEmpty()||(S.DialogueCooldown62-=D)>0)return;
 S.DialogueCooldown62=3;
 for(auto V:Array(Data(),TEXT("groups")))
 {Json G=V->AsObject();const FString Id=String(G,TEXT("id"));if(!Boolean(State(S),TEXT("done:")+Id)&&PairReady(S,G)){Begin(S,Id,Array(G,TEXT("lines")),false);break;}}
}
}

