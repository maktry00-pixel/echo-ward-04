#include "WardHospitalGameplay.h"
#include "WardStaging64.h"
#include "WardOnline80.h"
#include "WardNotes65.h"
#include "WardDialogue62.h"
#include "WardPeople54.h"
#include "WardPeople58.h"
#include "WardMonster59.h"
#include "Components/AudioComponent.h"
#include "WardCampaign.h"
#include "WardAudio68.h"
#include "WardCampaignUI.h"
#include "WardBookWorld.h"
#include "WardBookMap.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
using namespace WardData;
namespace {
Json Clone35(Json Value){Json R;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Serialize(Value)),R);return R.IsValid()?R:MakeShared<FJsonObject>();}
TArray<FString> Pages35(const FString& Text)
{
    TArray<FString> Paragraphs,Out;Text.ParseIntoArray(Paragraphs,TEXT("\n\n"),true);FString Page;
    for(const FString& P:Paragraphs)
    {
        if(Page.Len()+P.Len()>1050&&!Page.IsEmpty()){Out.Add(Page);Page.Empty();}
        if(P.Len()>1300){TArray<FString> Lines;P.ParseIntoArrayLines(Lines);for(const auto& Line:Lines){if(Page.Len()+Line.Len()>1100&&!Page.IsEmpty()){Out.Add(Page);Page.Empty();}Page+=Line+TEXT("\n");}}
        else Page+=P+TEXT("\n\n");
    }
    if(!Page.IsEmpty())Out.Add(Page);if(Out.IsEmpty())Out.Add(TEXT("Продолжить."));return Out;
}
}
void AEWardHospitalGameplay::BuildNarrative35()
{
    Narrative35=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/echo_scenario_v35.json"));
    NarrativeEnabled35=Number(Narrative35,TEXT("version"))==35;
    // Historical assessments retain their original data contract. Scenario35 has
    // its own assessment, including actual shipped narrative data and save state.
    if(FParse::Param(FCommandLine::Get(),TEXT("WardStoryCheck"))||FParse::Param(FCommandLine::Get(),TEXT("WardArrivalCheck")))NarrativeEnabled35=false;
    if(!NarrativeEnabled35)return;
    WardNotes65::Apply(*this);
    Data=Object(Narrative35,TEXT("gameplay"));for(const auto& E:Object(Narrative35,TEXT("scenes"))->Values)NarrativeScenes35.Add(FString(E.Key),E.Value->AsObject());
    auto Checkpoint = MakeShared<FJsonObject>();
    Checkpoint->SetStringField(TEXT("title"), TEXT("КПП"));
    Checkpoint->SetStringField(TEXT("text"), TEXT("Әсел: Фамилия человека, которого вы ищете?"));
    TArray<TSharedPtr<FJsonValue>> Answers;
    auto Answer = [&Answers](const TCHAR* Id, const TCHAR* Label, const TCHAR* Text)
    {
        auto Option = MakeShared<FJsonObject>();
        Option->SetStringField(TEXT("id"), Id);
        Option->SetStringField(TEXT("label"), Label);
        Option->SetStringField(TEXT("text"), Text);
        Answers.Add(MakeShared<FJsonValueObject>(Option));
    };
    Answer(TEXT("SISTER"), TEXT("Ищу Құрбанәлі Аружан."),
        TEXT("Бека: Құрбанәлі Аружан. Сотрудница. Она только что прислала сообщение.\n")
        TEXT("Әсел: Оставайтесь у автомобиля. Внутренняя группа проверит.\n")
        TEXT("Бека: Сколько ждать?\nӘсел: Мы не можем назвать срок."));
    Answer(TEXT("ACCIDENT"), TEXT("Что случилось в лаборатории?"),
        TEXT("Бека: Ищу Құрбанәлі Аружан. Что случилось в лаборатории?\n")
        TEXT("Әсел: Авария была давно. Лабораторный блок до сих пор закрыт.\n")
        TEXT("Әсел: Оставайтесь у автомобиля. Внутренняя группа проверит."));
    Checkpoint->SetArrayField(TEXT("choices"), Answers);
    NarrativeScenes35.Add(TEXT("CHECKPOINT_69"), Checkpoint);
    ResetNarrative35();
    UE_LOG(LogTemp,Display,TEXT("SCENARIO35_LOADED chapters=%d scene_ids=%d documents=%d placements=%d"),Array(Data,TEXT("chapters")).Num(),NarrativeScenes35.Num(),Object(Narrative35,TEXT("documents"))->Values.Num(),Array(Data,TEXT("placements")).Num());
}
void AEWardHospitalGameplay::ResetNarrative35()
{
    WardStaging64::Reset(*this);WardDialogue62::Stop(*this,true);
    NarrativeState35=MakeShared<FJsonObject>();NarrativeState35->SetNumberField(TEXT("version"),35);
    NarrativeState35->SetObjectField(TEXT("facts"),MakeShared<FJsonObject>());NarrativeState35->SetObjectField(TEXT("choices"),MakeShared<FJsonObject>());NarrativeState35->SetObjectField(TEXT("resolved"),MakeShared<FJsonObject>());NarrativeState35->SetObjectField(TEXT("npc_positions"),MakeShared<FJsonObject>());
    Mode->StringFlag(TEXT("skip76_scene"),TEXT(""));
    ActiveScene35.Empty();SelectedChoice35.Empty();DialoguePage35=0;NarrativeReplay35=false;
    SetNarrativeFlag35(TEXT("IRINA_ALIVE"),true);SetNarrativeFlag35(TEXT("RUDIN_ALIVE"),true);SetNarrativeFlag35(TEXT("NIKITA_ALIVE"),true);SetNarrativeFlag35(TEXT("MARINA_TRUST"),true);SetNarrativeFlag35(TEXT("BORIS_TRUST"),true);SetNarrativeFlag35(TEXT("GLASS_IS_IMPOSTOR"),true);SetNarrativeFlag35(TEXT("RADIO_IS_LIVE_IRINA"),true);WardNotes65::Sync(*this);
}
bool AEWardHospitalGameplay::NarrativeFlag35(const FString& Key)const{return Boolean(Object(NarrativeState35,TEXT("facts")),Key);}
void AEWardHospitalGameplay::SetNarrativeFlag35(const FString& Key,bool Value){Object(NarrativeState35,TEXT("facts"))->SetBoolField(Key,Value);}
bool AEWardHospitalGameplay::NarrativeRequirements35(Json Row,FString* Missing)const
{
    for(auto V:Array(Row,TEXT("requires")))
    {
        const FString Key=V->AsString();if(!Mode->Completed.Contains(Key)&&!NarrativeFlag35(Key))
        {if(Missing){if(Key==TEXT("EVIDENCE_PACKET_READY65")){*Missing=WardNotes65::Missing(*this);return false;}Json P=Definitions.FindRef(Key);*Missing=P?String(P,TEXT("label"))+TEXT(" [")+String(P,TEXT("zone"))+TEXT("]"):Key.StartsWith(TEXT("PLAN_READY_"))?TEXT("питание, медицинская проверка и подтверждение участников"):Key;}return false;}
    }
    return true;
}
void AEWardHospitalGameplay::NarrativeEffects35(Json Effects){for(const auto& E:Effects->Values)if(E.Value->Type==EJson::Boolean)SetNarrativeFlag35(FString(E.Key),E.Value->AsBool());}
Json AEWardHospitalGameplay::NarrativeOptions35()const
{
    Json Result=MakeShared<FJsonObject>();Json Scene=NarrativeScenes35.FindRef(ActiveScene35);if(Scene)Result->SetArrayField(TEXT("options"),Array(Scene,TEXT("choices")));return Result;
}
FString AEWardHospitalGameplay::NarrativeTitle35()const
{
    if(Json Scene=NarrativeScenes35.FindRef(ActiveScene35))return String(Scene,TEXT("title"));
    return String(NarrativeState35,TEXT("aux_title"),TEXT("Разговор"));
}
FString AEWardHospitalGameplay::NarrativeVoice35()const
{
    const FString Body=NarrativeBody35();
    if(ActiveScene35.StartsWith(TEXT("SCENE_10")))return TEXT("Источник у стекла и автономная рация проверяются отдельно");
    if(Body.Contains(TEXT("Мусалимова")))return TEXT("Внутренняя связь");
    if(Body.Contains(TEXT("Аружан")))return TEXT("Аружан / местный разговор");return TEXT("Местный разговор / найденная запись");
}
FString AEWardHospitalGameplay::NarrativeBody35(bool FullText)const
{
    Json Scene=NarrativeScenes35.FindRef(ActiveScene35);FString Text=Scene?String(Scene,TEXT("text")):String(NarrativeState35,TEXT("aux_text"));
    if(Scene&&!SelectedChoice35.IsEmpty())for(auto V:Array(Scene,TEXT("choices")))if(String(V->AsObject(),TEXT("id"))==SelectedChoice35)Text=String(V->AsObject(),TEXT("text"));
    const FString Plan=String(Object(NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"));
    for(auto V:Array(Narrative35,TEXT("reactions")))
    {
        Json R=V->AsObject();if(String(R,TEXT("scene"))!=ActiveScene35)continue;
        const FString Key=String(R,TEXT("choice_key"));const bool Match=!Key.IsEmpty()?String(Object(NarrativeState35,TEXT("choices")),Key)==String(R,TEXT("choice_value")):NarrativeFlag35(String(R,TEXT("flag")));
        if(Match){if(Boolean(R,TEXT("replace")))Text=String(R,TEXT("text"));else Text+=TEXT("\n\n")+String(R,TEXT("text"));}
    }
    if(ActiveScene35.StartsWith(TEXT("SCENE_19")))
    {
        FString Variant;
        if(NarrativeFlag35(TEXT("OUTBREAK"))||(ActiveScene35==TEXT("SCENE_19_02")&&SelectedChoice35==TEXT("BREACH")))Variant=TEXT("OUTBREAK");
        else if(Plan!=TEXT("CURE")&&!Plan.IsEmpty())Variant=(NarrativeFlag35(TEXT("HERO_STAYS"))||(ActiveScene35==TEXT("SCENE_19_01")&&SelectedChoice35==TEXT("STAY")))&&Plan==TEXT("DESTROY")?TEXT("DESTROY_STAY"):Plan;
        else if(!NarrativeFlag35(TEXT("MEMORY_SPARED")))Variant=TEXT("CURE_LOSS");
        if(!Variant.IsEmpty())
        {
            const int Index=FCString::Atoi(*ActiveScene35.Right(2))-1;const auto& Sections=Array(Object(Narrative35,TEXT("final_scenes")),Variant);
            Text=Sections.IsValidIndex(Index)?Sections[Index]->AsString():String(Object(Narrative35,TEXT("final_texts")),Variant);
            if(Text.IsEmpty())Text=TEXT("Продолжи подтверждённый маршрут. Условия выбранной операции остаются в силе.");
        }
        if(ActiveScene35==TEXT("SCENE_19_05")){if(!NarrativeFlag35(TEXT("TRUTH_READY")))Text+=TEXT("\n\n")+String(Object(Narrative35,TEXT("final_texts")),TEXT("NO_TRUTH"));Text+=TEXT("\n\n")+NarrativeEnding35();}
    }
    if (ActiveScene35 == TEXT("SCENE_01_01") && ArrivalStage >= 0)
    {
        // The recorded exchange with the duty officer belongs to the checkpoint.
        int Post = Text.Find(TEXT("\nӘсел:"));
        if (Post != INDEX_NONE) Text.LeftInline(Post);
    }
    if (FullText) return Text;
    auto Pages=Pages35(Text);return Pages[FMath::Clamp(DialoguePage35,0,Pages.Num()-1)];
}
bool AEWardHospitalGameplay::BeginNarrative35(const FString& Id,bool Replay)
{
    if(!NarrativeEnabled35)return false;
    if(!ActiveScene35.IsEmpty()&&ActiveScene35!=Id){if(!NarrativeTesting35)Mode->Show(TEXT("story35"));return true;}
    Json Scene=NarrativeScenes35.FindRef(Id);if(!Scene)return false;
    if(!Replay)
    {
        if(Chapter()>19||Number(Scene,TEXT("chapter"))!=Chapter())return false;
        const auto& Steps=Array(Array(Data,TEXT("chapters"))[Chapter()-1]->AsObject(),TEXT("required"));int Step=Mode->Values.FindRef(TEXT("hospital_step"));
        if(!Steps.IsValidIndex(Step)||Steps[Step]->AsString()!=Id){Mode->Notify(Objective());return true;}
        FString Missing;if(!NarrativeRequirements35(Scene,&Missing)){Mode->Notify(TEXT("Сначала найди: ")+Missing,6);return true;}
    }
    if(ActiveScene35.IsEmpty()){SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),false);ActiveScene35=Id;SelectedChoice35.Empty();DialoguePage35=0;NarrativeReplay35=Replay;}
    WardStaging64::Begin(*this,Id);if(!NarrativeTesting35)Mode->Show(TEXT("story35"));return true;
}
bool AEWardHospitalGameplay::NarrativeUse35(const FString& Id)
{
    if(!NarrativeEnabled35)return false;
    if(!ActiveScene35.IsEmpty()&&!WardOnline80::ParallelPickup(*Mode,Id)){if(!NarrativeTesting35)Mode->Show(TEXT("story35"));return true;}
    Json P=Definitions.FindRef(Id);if(!P)return false;const FString Kind=String(P,TEXT("kind"));
    if(Id==TEXT("staff_pass")&&!Mode->Completed.Contains(TEXT("SCENE_01_04"))){Mode->Notify(TEXT("Служебный пропуск ещё у Файзулаева."));return true;}
    if(Kind==TEXT("scene"))
    {
        if(ArrivalStage>=0&&ArrivalStage<5){Mode->Notify(ArrivalObjective());return true;}
        if(Mode->Completed.Contains(Id)){Mode->Notify(TEXT("Событие уже завершено. Запись сохранена в журнале."));return true;}
        return BeginNarrative35(Id);
    }
    if(Chapter()<Number(P,TEXT("chapter"),1)&&!Boolean(P,TEXT("memory_active57")))return true;
    if(!WardNotes65::Allowed(*this,Id))return true;
    if(Kind==TEXT("key")&&WardNotes65::UseKey(*this,Id))return true;
    if(Kind==TEXT("note"))
    {
        if(Mode->Completed.Contains(Id))return true;Mode->Cue(TEXT("UI/paper_pick"),Props.Contains(Id)?Props[Id]->GetActorLocation():Mode->Hero()->GetActorLocation(),.2f);Mode->Journal.AddUnique(TEXT("@doc:")+Id);Mode->Completed.Add(Id);WardNotes65::Collected(*this,Id);Mode->Notify(TEXT("Документ найден: ")+String(P,TEXT("label"))+TEXT(" · J — читать"),5);
        if(Id.StartsWith(TEXT("SIDE_")))
        {
            TArray<FString> Parts;Id.ParseIntoArray(Parts,TEXT("_"));if(Parts.Num()==3){bool Complete=true;for(int I=1;I<=3;++I)Complete&=Mode->Completed.Contains(TEXT("SIDE_")+Parts[1]+TEXT("_")+FString::FromInt(I));
                if(Complete&&!NarrativeFlag35(TEXT("SIDE_DONE_")+Parts[1])){const int Index=FCString::Atoi(*Parts[1])-1;const auto& Sides=Array(Narrative35,TEXT("side_stories"));if(Sides.IsValidIndex(Index)){Json Side=Sides[Index]->AsObject();ActiveScene35=TEXT("SIDE_")+Parts[1];NarrativeState35->SetStringField(TEXT("aux_title"),String(Side,TEXT("title")));NarrativeState35->SetStringField(TEXT("aux_text"),String(Side,TEXT("text")));SelectedChoice35.Empty();DialoguePage35=0;if(!NarrativeTesting35)Mode->Show(TEXT("story35"));}}
            }
        }
        Refresh();Mode->AutoSave();return true;
    }
    if(Kind==TEXT("empty_container")){Mode->Completed.Add(Id);Mode->Notify(TEXT("Контейнер пуст. Припасов нет."));Refresh();return true;}
    if(Kind==TEXT("story_object"))
    {
        if(Mode->Completed.Contains(Id))return true;Mode->Completed.Add(Id);SetNarrativeFlag35(Id,true);
        if(Id==TEXT("NEW_RADIO_01"))SetNarrativeFlag35(TEXT("AUTONOMOUS_RADIO"),true);
        if(Id==TEXT("NEW_STROBE_01"))Mode->FlashCharges=FMath::Min(3,Mode->FlashCharges+1);
        Mode->Journal.AddUnique(String(P,TEXT("label")));Mode->Notify(String(P,TEXT("label")),5);Refresh();Mode->AutoSave();return true;
    }
    if(Kind==TEXT("person"))
    {
        if (Id == TEXT("PERSON_1"))
        {
            Mode->Notify(TEXT("За стеклом стоит манекен."));
            return true;
        }
        if(auto* Actor=Props.FindRef(Id))
            if(auto* Model=Actor->FindComponentByClass<UEWardPerson54>();Model&&Model->IsDead58())
            {Mode->Notify(String(P,TEXT("label"))+TEXT(" · погиб"));return true;}
        ActiveScene35=TEXT("NPC_")+Id;SelectedChoice35.Empty();DialoguePage35=0;NarrativeReplay35=true;
        const FString Name=String(P,TEXT("label"));
        FString Opening = UEWardPerson54::OpeningLine55(Id);
        NarrativeState35->RemoveField(TEXT("aux_voice57"));
        if (Id.StartsWith(TEXT("FILE_")))
        {
            const auto Card=Object(Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People56/archive.json")),Id);
            const auto& Lines=Array(Card,TEXT("lines"));
            const int32 Index=int32(Number(P,TEXT("spoken57"))) % FMath::Max(1,Lines.Num());
            if (Lines.IsValidIndex(Index)) Opening=Lines[Index]->AsString();
            P->SetNumberField(TEXT("spoken57"),Index+1);
            NarrativeState35->SetStringField(TEXT("aux_voice57"),FPaths::ProjectContentDir()/
                TEXT("CampaignData/People56")/(Id+TEXT("_")+FString::FromInt(Index+1)+TEXT(".wav")));
        }
        FString Body = Opening.IsEmpty() ? FString() : Name+TEXT(": ")+Opening+TEXT("\n\n");
        if(Boolean(P,TEXT("past57")))Body=TEXT("[СЦЕНА ПРОШЛОГО · ДО АВАРИИ]\n")+Body;
        for(auto V:Array(Narrative35,TEXT("questions")))
        {const auto& A=V->AsArray();if(A.Num()!=4)continue;const FString Speaker=A[0]->AsString();if(!Name.Contains(Speaker)&&!Speaker.Contains(Name))continue;
            const FString Context=A[1]->AsString();int Required=0;FString Digits;for(TCHAR Ch:Context){if(FChar::IsDigit(Ch))Digits+=Ch;else if(!Digits.IsEmpty())break;}Required=FCString::Atoi(*Digits);if(Required>Chapter())continue;
            Body+=TEXT("Бека: ")+A[2]->AsString()+TEXT("\n")+Speaker+TEXT(": ")+A[3]->AsString()+TEXT("\n\n");}
        if(Body.IsEmpty())Body=Name+TEXT(": Сначала закончим текущую задачу. Не оставляй проход открытым.\n\n")+Objective();
        NarrativeState35->SetStringField(TEXT("aux_title"),Name);NarrativeState35->SetStringField(TEXT("aux_text"),Body);if(!NarrativeTesting35)Mode->Show(TEXT("story35"));return true;
    }
    return false;
}
void AEWardHospitalGameplay::PreparePlans35()
{
    WardNotes65::Sync(*this);
    const bool Prep=NarrativeFlag35(TEXT("COMPARISON_READY"))&&NarrativeFlag35(TEXT("MANUAL_POWER_READY"))&&NarrativeFlag35(TEXT("CONTAINMENT_READY"));
    SetNarrativeFlag35(TEXT("PLAN_READY_CURE"),Prep&&NarrativeFlag35(TEXT("IRINA_CONSENT"))&&NarrativeFlag35(TEXT("KOVALEV_RESCUED"))&&Mode->Completed.Contains(TEXT("NEW_BASELINE_001")));
    SetNarrativeFlag35(TEXT("PLAN_READY_DESTROY"),NarrativeFlag35(TEXT("MANUAL_POWER_READY"))&&NarrativeFlag35(TEXT("CONTAINMENT_READY")));
    SetNarrativeFlag35(TEXT("PLAN_READY_EVAC"),NarrativeFlag35(TEXT("EVAC_LIST_CHECKED"))&&NarrativeFlag35(TEXT("ORLOVA_ALLY")));
    SetNarrativeFlag35(TEXT("PLAN_READY_REPLACE"),NarrativeFlag35(TEXT("MANUAL_POWER_READY"))&&NarrativeFlag35(TEXT("KOVALEV_RESCUED")));
    SetNarrativeFlag35(TEXT("PLAN_READY_CONTINUE"),Mode->Completed.Contains(TEXT("SCENE_18_02")));
    SetNarrativeFlag35(TEXT("MEMORY_SPARED"),NarrativeFlag35(TEXT("PATIENT_RESCUED"))&&NarrativeFlag35(TEXT("LOG_PRESERVED"))&&NarrativeFlag35(TEXT("COMPARISON_READY")));
    SetNarrativeFlag35(TEXT("TRUTH_READY"),NarrativeFlag35(TEXT("TRUTH_SENT"))&&NarrativeFlag35(TEXT("SEMYON_HELPED"))&&NarrativeFlag35(TEXT("FULL_PROTOCOL_FRAGMENT"))&&NarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65")));
}
void AEWardHospitalGameplay::FinishNarrative35()
{
    if(!WardStaging64::CanFinish(*this)){Mode->Notify(WardStaging64::Status(*this));return;}
    if (ActiveScene35 == TEXT("CHECKPOINT_69"))
    {
        if (SelectedChoice35.IsEmpty()) return;
        NarrativeState35->SetStringField(TEXT("checkpoint_answer69"), SelectedChoice35);
        ActiveScene35.Reset();
        SelectedChoice35.Reset();
        DialoguePage35 = 0;
        NarrativeReplay35 = false;
        Mode->Show(TEXT(""));
        FinishCheckpoint69();
        return;
    }
    WardStaging64::Finish(*this);const FString Id=ActiveScene35;Json Scene=NarrativeScenes35.FindRef(Id);
    if(Scene&&!NarrativeReplay35)
    {
        Json Selected;for(auto V:Array(Scene,TEXT("choices")))if(String(V->AsObject(),TEXT("id"))==SelectedChoice35)Selected=V->AsObject();
        if(Array(Scene,TEXT("choices")).Num()&&!Selected)return;
        if(Selected){Object(NarrativeState35,TEXT("choices"))->SetStringField(String(Scene,TEXT("choice_key")),SelectedChoice35);NarrativeEffects35(Object(Selected,TEXT("effects")));Mode->Journal.AddUnique(TEXT("Решение: ")+String(Scene,TEXT("title"))+TEXT(" / ")+String(Selected,TEXT("label")));}
        NarrativeEffects35(Object(Scene,TEXT("effects")));Object(NarrativeState35,TEXT("resolved"))->SetBoolField(Id,true);Mode->Completed.Add(Id);
        if(Id==TEXT("SCENE_01_04")){Mode->Flag(TEXT("badge"),true);Mode->Journal.AddUnique(TEXT("Файзулаев Талғат погиб у двери. Служебный пропуск доступен после его предупреждения."));}
        if(Id==TEXT("SCENE_10_02"))
        {
            for(const TCHAR* Alt:{TEXT("SCENE_10_03"),TEXT("SCENE_10_04")})Object(NarrativeState35,TEXT("resolved"))->SetStringField(Alt,SelectedChoice35==TEXT("OPEN")&&FString(Alt).EndsWith(TEXT("03"))||SelectedChoice35==TEXT("RADIO")&&FString(Alt).EndsWith(TEXT("04"))?TEXT("RESOLVED_BY_BRANCH"):TEXT("SKIPPED_BY_BRANCH"));
        }
        if(Id==TEXT("SCENE_19_02")&&NarrativeFlag35(TEXT("RECHECKED")))SetNarrativeFlag35(TEXT("CLEAN_EXIT"),String(Object(NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"))==TEXT("CURE"));
        const auto& Steps=Array(Array(Data,TEXT("chapters"))[Chapter()-1]->AsObject(),TEXT("required"));int Step=Mode->Values.FindRef(TEXT("hospital_step"))+1;
        if(Step>=Steps.Num())
        {
            const int Ch=Chapter();bool Primary=true;for(const auto& Pair:NarrativeScenes35)if(Number(Pair.Value,TEXT("chapter"))==Ch&&Array(Pair.Value,TEXT("choices")).Num()){const FString Choice=String(Object(NarrativeState35,TEXT("choices")),String(Pair.Value,TEXT("choice_key")));if(!Choice.IsEmpty()&&Choice!=String(Array(Pair.Value,TEXT("choices"))[0]->AsObject(),TEXT("id")))Primary=false;}if(Primary&&Array(Narrative35,TEXT("journal")).IsValidIndex(Ch-1)){const auto& J=Array(Narrative35,TEXT("journal"))[Ch-1]->AsArray();if(J.Num()==2)Mode->Journal.AddUnique(J[1]->AsString());}
            SetChapter(Ch+1);
        }
        else Mode->Values.Add(TEXT("hospital_step"),Step);
        if(Id==TEXT("SCENE_19_05")){SetNarrativeFlag35(TEXT("EPILOGUE_COMPLETE"),true);Mode->Journal.AddUnique(NarrativeEnding35());}
        Mode->Journal.AddUnique(TEXT("Разговор: ")+String(Scene,TEXT("title")));PreparePlans35();
    }
    else if(Id.StartsWith(TEXT("SIDE_")))
    {
        const FString Num=Id.RightChop(5);SetNarrativeFlag35(TEXT("SIDE_DONE_")+Num,true);SetNarrativeFlag35(String(Object(Narrative35,TEXT("side_rewards")),Num),true);Mode->Journal.AddUnique(String(NarrativeState35,TEXT("aux_title"))+TEXT(" — история завершена."));
    }
    ActiveScene35.Empty();SelectedChoice35.Empty();DialoguePage35=0;NarrativeReplay35=false;Refresh();if(!NarrativeTesting35)Mode->Show(TEXT(""));Mode->AutoSave();
}
bool AEWardHospitalGameplay::NarrativeCommand35(const FString& Action)
{
    if(!Action.StartsWith(TEXT("story35_")))return false;if(!NarrativeEnabled35)return true;
    if(WardStaging64::Command(*this,Action))return true;
    if(Action==TEXT("story35_close")){SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),true);Mode->Show(TEXT(""));return true;}
    if(Action==TEXT("story35_save")){Mode->Save(TEXT("manual_6"));return true;}
    if(Action.StartsWith(TEXT("story35_choice_")))
    {
        if(!SelectedChoice35.IsEmpty()||!WardStaging64::ChoiceReady(*this)||(!NarrativeTesting35&&!WardDialogue62::AwaitingChoice(*this)))return true;const FString Key=Action.RightChop(15);Json Scene=NarrativeScenes35.FindRef(ActiveScene35);if(!Scene)return true;
        for(auto V:Array(Scene,TEXT("choices")))
        {
            Json Opt=V->AsObject();if(String(Opt,TEXT("id"))!=Key)continue;FString Missing;if(!NarrativeRequirements35(Opt,&Missing)){Mode->Notify(TEXT("План ещё не подготовлен: ")+Missing,5);return true;}
            if(Key==TEXT("STAY")&&ActiveScene35==TEXT("SCENE_19_01")&&String(Object(NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"))!=TEXT("DESTROY"))return true;
            SelectedChoice35=Key;DialoguePage35=0;WardDialogue62::Stop(*this,true);break;
        }
    }
    else if(Action==TEXT("story35_back"))DialoguePage35=FMath::Max(0,DialoguePage35-1);
    else if(Action==TEXT("story35_next"))
    {
        Json Scene=NarrativeScenes35.FindRef(ActiveScene35);if(Scene&&Array(Scene,TEXT("choices")).Num()&&SelectedChoice35.IsEmpty())return true;
        if (DialoguePage35 == 0 && ActiveScene35.StartsWith(TEXT("NPC_")) && IsValid(Mode->Voice))
            Mode->Voice->Stop();
        if (Scene && !Array(Scene, TEXT("choices")).IsEmpty()) { FinishNarrative35(); return true; }
        const FString Current=NarrativeBody35();++DialoguePage35;const FString Next=NarrativeBody35();if(Current==Next){--DialoguePage35;FinishNarrative35();return true;}
    }
    if(!NarrativeTesting35&&Mode->Interface.IsValid())Mode->Interface->Refresh();return true;
}
FString AEWardHospitalGameplay::NarrativeEnding35()const
{
    const FString Plan=String(Object(NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"),TEXT("EVAC"));FString Result=TEXT("ИТОГ ПОСЕЩЕНИЯ\n");
    Result+=NarrativeFlag35(TEXT("OUTBREAK"))?TEXT("Активная связь выпущена наружу. Карантин нарушен.\n"):NarrativeFlag35(TEXT("CLEAN_EXIT"))?TEXT("Бека и Аружан прошли повторную проверку.\n"):TEXT("Носители остаются в карантине до независимой проверки.\n");
    if(Plan==TEXT("REPLACE"))Result+=TEXT("Бека остаётся носителем в центре. Аружан получает отдельный медицинский выход.\n");
    else if(Plan==TEXT("CONTINUE"))Result+=TEXT("Проект продолжен по ведомственной сделке.\n");
    else if(Plan==TEXT("EVAC"))Result+=TEXT("Гражданские эвакуированы. Бека и Аружан остаются под карантинным наблюдением.\n");
    else if(Plan==TEXT("DESTROY"))Result+=NarrativeFlag35(TEXT("HERO_STAYS"))?
        TEXT("Бека остался у ручного контура. Бека и Аружан погибли внутри центра.\n"):
        TEXT("Центр уничтожен. Аружан погибла у контура. Бека вышел наземным путём.\n");
    else if(Plan==TEXT("CURE")&&!NarrativeFlag35(TEXT("MEMORY_SPARED")))Result+=TEXT("Лечение завершено с потерей части памяти.\n");
    Result+=TEXT("Выбранная операция: ")+Plan+TEXT("\n");Result+=NarrativeFlag35(TEXT("TRUTH_READY"))?TEXT("Доказательства доставлены независимым получателям.\n"):TEXT("Внешняя доставка доказательств не подтверждена.\n");
    Result+=TEXT("Файзулаев Талғат: погиб.\nЖарылқасын Маржан: ")+(NarrativeFlag35(TEXT("NIKITA_ALIVE"))?FString(TEXT("спасена.\n")):FString(TEXT("не спасена.\n")));
    Result+=TEXT("Кәрім Батырхан: ")+(NarrativeFlag35(TEXT("PATIENT_RESCUED"))?FString(TEXT("помощь оказана.\n")):FString(TEXT("его вывела Аружан; таблица не получена.\n")));
    Result+=TEXT("Жуманбаев Адылбек: ")+(NarrativeFlag35(TEXT("SEMYON_HELPED"))?FString(TEXT("выведен добровольно.\n")):FString(TEXT("вывод не подтверждён.\n")));Result+=TEXT("\n")+WardNotes65::Summary(*this);return Result;
}
void AEWardHospitalGameplay::RefreshNarrative35()
{
    if(!NarrativeEnabled35)return;
    WardPeople58::Sync(*this);
    WardMonster59::Refresh(*this);
    const FString Plan=String(Object(NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"));
    if(!Plan.IsEmpty()&&Plan!=TEXT("EVAC"))
    {
        if(auto A=Props.FindRef(TEXT("SCENE_19_03"))){Json P=Definitions.FindRef(TEXT("SCENE_19_03"));const FVector At(50,-49,0);A->SetActorLocation(At*100);}
    }
    for(const auto& E:NarrativeGates35)
    {
        const int Required=FCString::Atoi(*E.Key.Left(2));const bool Closed=Chapter()<Required;
        E.Value->SetCollisionEnabled(Closed?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
        for(auto Child:E.Value->GetAttachChildren())Child->SetVisibility(Closed,true);
    }
    if((Plan.IsEmpty()||Plan==TEXT("EVAC"))&&Props.Contains(TEXT("SCENE_19_03")))
    {
        Json Original;for(auto V:Array(Object(Narrative35,TEXT("gameplay")),TEXT("placements")))if(String(V->AsObject(),TEXT("id"))==TEXT("SCENE_19_03")){Original=V->AsObject();break;}
        if(Original)Props[TEXT("SCENE_19_03")]->SetActorLocation(Vector(Array(Original,TEXT("at")))*100);
    }
    if(auto A=Props.FindRef(TEXT("staff_pass")))if(!Mode->Completed.Contains(TEXT("SCENE_01_04"))){A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}
    if(Chapter()>1&&!Mode->Completed.Contains(TEXT("staff_pass"))&&Mode->Completed.Contains(TEXT("SCENE_01_04")))SetNarrativeFlag35(TEXT("CH1_DONE"),true);
    WardStaging64::RefreshAnchors(*this);WardNotes65::Refresh(*this);
}
void AEWardHospitalGameplay::TickNarrative35(float Delta)
{
    WardStaging64::Tick(*this,Delta);
    if(!NarrativeEnabled35)return;
    if (!NarrativeTesting35 && Mode->Screen == TEXT("story35") && WardDialogue62::PageFinished(*this)
        && !WardDialogue62::AwaitingChoice(*this) && WardStaging64::CanFinish(*this))
    {
        NarrativeCommand35(TEXT("story35_next"));
        return;
    }
    if(!ActiveScene35.IsEmpty()&&Mode->Screen.IsEmpty()&&!NarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"))){if(!NarrativeTesting35)Mode->Show(TEXT("story35"));return;}
    NarrativeClock35-=Delta;if(NarrativeClock35<=0&&Mode->Screen.IsEmpty()){NarrativeClock35=1.5f;RefreshNarrative35();}
}
Json AEWardHospitalGameplay::NarrativeSnapshot35()const
{
    Json S=Clone35(NarrativeState35);S->SetStringField(TEXT("active_scene"),ActiveScene35);S->SetStringField(TEXT("selected_choice"),SelectedChoice35);S->SetNumberField(TEXT("page"),DialoguePage35);S->SetBoolField(TEXT("replay"),NarrativeReplay35);
    Json Actors=MakeShared<FJsonObject>();for(const auto& E:Props)if(String(Definitions.FindRef(E.Key),TEXT("kind"))==TEXT("person")){const FVector At=E.Value->GetActorLocation();Actors->SetArrayField(E.Key,{MakeShared<FJsonValueNumber>(At.X),MakeShared<FJsonValueNumber>(At.Y),MakeShared<FJsonValueNumber>(At.Z)});}S->SetObjectField(TEXT("npc_positions"),Actors);S->SetObjectField(TEXT("humans58"),WardPeople58::Snapshot(*const_cast<AEWardHospitalGameplay*>(this)));return S;
}
void AEWardHospitalGameplay::RestoreNarrative35(Json S)
{
    WardStaging64::Reset(*this);WardDialogue62::Stop(*this,true);
    if(!NarrativeEnabled35)return;
    if(Number(S,TEXT("version"))!=35)
    {
        ResetNarrative35();SetNarrativeFlag35(TEXT("LEGACY_SAVE"),true);
        // Preserve progress without silently choosing a branch for the player.
        SetNarrativeFlag35(TEXT("RUDIN_ALIVE"),!Mode->Completed.Contains(TEXT("SCENE_01_04")));
        // Preserve the next unresolved legacy scene when chapter 10 no longer
        // requires both OPEN and RADIO; avoid indexing into a different event.
        const int Ch=Chapter();if(Ch<=19){const auto& Steps=Array(Array(Data,TEXT("chapters"))[Ch-1]->AsObject(),TEXT("required"));int Step=0;while(Steps.IsValidIndex(Step)&&Mode->Completed.Contains(Steps[Step]->AsString()))++Step;Mode->Values.Add(TEXT("hospital_step"),Step);}
        return;
    }
    NarrativeState35=Clone35(S);ActiveScene35=String(S,TEXT("active_scene"));SelectedChoice35=String(S,TEXT("selected_choice"));DialoguePage35=FMath::Max(0,int(Number(S,TEXT("page"))));NarrativeReplay35=Boolean(S,TEXT("replay"));
    if(!ActiveScene35.IsEmpty()&&!NarrativeScenes35.Contains(ActiveScene35)&&!ActiveScene35.StartsWith(TEXT("SIDE_"))&&!ActiveScene35.StartsWith(TEXT("NPC_")))ActiveScene35.Empty();
    for(const auto& E:Object(S,TEXT("npc_positions"))->Values)if(auto* A=Props.Find(FString(E.Key))){if(E.Value->Type==EJson::Array)(*A)->SetActorLocation(Vector(E.Value->AsArray()),false);}
    // These are authorial facts, never random outcomes of loading.
    SetNarrativeFlag35(TEXT("GLASS_IS_IMPOSTOR"),true);SetNarrativeFlag35(TEXT("RADIO_IS_LIVE_IRINA"),true);
}
void AEWardHospitalGameplay::BuildNarrativeGates35()
{
    // Quarantine blocks only the three main vertical cores at future stops.
    // Existing architecture and authored door leaves are left intact.
    for(const auto& Stop:TArray<TPair<int,float>>{{2,480.f},{5,-480.f},{6,-960.f},{7,960.f}})
    for(const auto& Core:TArray<FVector2D>{{44,0},{44,88},{-8,86}})
    {
        const FString Id=FString::Printf(TEXT("%02d_%.0f_%.0f"),Stop.Key,Core.X,Core.Y);
        auto* Gate=NewObject<UBoxComponent>(this);Gate->SetupAttachment(RootComponent);Gate->SetRelativeLocation(FVector(Core.X*100+150,Core.Y*100+400,Stop.Value+190));Gate->SetBoxExtent(FVector(147,395,185));Gate->SetCollisionProfileName(TEXT("BlockAll"));Gate->SetGenerateOverlapEvents(false);Gate->RegisterComponent();AddInstanceComponent(Gate);NarrativeGates35.Add(Id,Gate);
        auto* Mesh=NewObject<UStaticMeshComponent>(this);Mesh->SetupAttachment(Gate);Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->SetRelativeScale3D(FVector(2.94,7.9,3.7));Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetLightingChannels(true,true,false);
        auto* Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/BookWorld/M_HospitalSurface.M_HospitalSurface")),this);Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.12,.10,.10));Mat->SetScalarParameterValue(TEXT("Roughness"),.85);Mesh->SetMaterial(0,Mat);Mesh->RegisterComponent();AddInstanceComponent(Mesh);
    }
}
bool AEWardHospitalGameplay::FindNarrativeStanding35(const FString& Id,FVector& Standing)const
{
    auto A=Props.FindRef(Id);if(!A||!Mode||!Mode->Hero())return false;
    const FVector Target=A->GetActorLocation()+FVector(0,0,15);Json Def=Definitions.FindRef(Id);const float Base=FMath::FloorToFloat((A->GetActorLocation().Z+10)/480)*480;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ScenarioStanding35),false,Mode->Hero());for(const auto& E:Props)Q.AddIgnoredActor(E.Value);for(auto M:Mobs)Q.AddIgnoredActor(M.Get());
    const auto* Capsule=Mode->Hero()->GetCapsuleComponent();
    const float Half=Capsule->GetScaledCapsuleHalfHeight();
    const float Width=Capsule->GetScaledCapsuleRadius();
    for(float Radius:{85.f,115.f,145.f,175.f})for(int I=0;I<12;++I)
    {
        const float Angle=I*PI/6;const FVector Candidate=FVector(Target.X+FMath::Cos(Angle)*Radius,Target.Y+FMath::Sin(Angle)*Radius,Base+Half+3);
        FHitResult Floor;if(!GetWorld()->LineTraceSingleByChannel(Floor,Candidate,Candidate-FVector(0,0,250),ECC_WorldStatic,Q)||FMath::Abs(Floor.ImpactNormal.Z)<.8f)continue;
        // The player must stand on this floor, not on the desk or cupboard
        // supporting the pickup. Use the real pawn's body clearance.
        if(FMath::Abs(Floor.ImpactPoint.Z-Base)>30)continue;
        const FVector Feet=Floor.ImpactPoint+FVector(0,0,Half+3);
        if(GetWorld()->OverlapBlockingTestByChannel(Feet,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Width,Half-2),Q))continue;
        FHitResult Sight;if(GetWorld()->LineTraceSingleByChannel(Sight,Feet+FVector(0,0,25),Target,ECC_Visibility,Q))continue;
        Standing=Feet;return true;
    }
    return false;
}


#include "WardStaging64.inl"
#include "WardStagingAssessment64.inl"
#include "WardNotes65.inl"
#include "WardNotesAssessment65.inl"
