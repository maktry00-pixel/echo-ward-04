#include "WardHospitalGameplay.h"
#include "WardPeople57.h"
#include "WardPeople58.h"
#include "WardCampaign.h"
#include "WardAdmin79.h"
#include "WardOnline80.h"
#include "WardGameplay76.h"
#include "WardAudio68.h"
#include "WardCampaignUI.h"
#include "WardInterface66.h"
#include "WardTitleScene.h"
#include "WardSettings.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/SpringArmComponent.h"
using namespace WardData;
void AEWardCampaignMode::Command(const FString& Id)
{
    if(WardOnline80::Command(*this,Id))return;
    if(WardAdmin79::Command(*this,Id))return;
    if(WardGameplay76::Command(*this,Id))return;
    if(WardAudio68::Command(*this,Id))return;
    if(WardInterface66::Command(*this,Id))return;
    if(Id==TEXT("back")&&WardInterface66::Back(*this))return;
    if(WardPeople58::Command(*this,Id))return;
    if(Id.StartsWith(TEXT("people57_replay_")))
    {
        if(!WardPeople57::Replay(*this,Id.RightChop(16))) Notify(TEXT("Сцена прошлого недоступна."));
        return;
    }
    if(PreviewCommand51(Id))return;
    if(HospitalStory&&HospitalStory->NarrativeCommand35(Id))return;
    if(Interface.IsValid()&&Interface->FieldCommand(Id))return;
    if(TitleScene&&TitleScene->Active&&(!Assessment||FParse::Param(FCommandLine::Get(),TEXT("WardMenuCheck")))&&!TitleActionExecuting)
    {
        if(TitleScene->Transitioning)return;
        TitleScene->Click();
        if((TitleView&&Id==TEXT("page_game"))||Id.StartsWith(TEXT("new_"))||Id.StartsWith(TEXT("load_"))||Id==TEXT("latest"))
        {
            bool Valid=true;if(Id==TEXT("latest")){Valid=false;for(const auto& Slot:SaveSlots())if(ValidateSave(ReadSlot(Slot))){Valid=true;break;}}
            else if(Id.StartsWith(TEXT("load_")))Valid=ValidateSave(ReadSlot(Id.RightChop(5)));
            if(Valid){TitleScene->BeginTransition(Id);return;}
        }
    }
    if(Id==TEXT("page_map")&&Hero()){Interface->BookMapVisible=true;Values.Add(TEXT("map_area"),0);Values.Add(TEXT("map_floor"),Hero()->GetActorLocation().Z<0?-1:0);Show(TEXT("map"));return;}
    if(Id.StartsWith(TEXT("page_"))&&Id!=TEXT("page_quit")){Interface->GuidePage=0;Show(Id.RightChop(5));return;}
    if(Id==TEXT("resume")){Show(TEXT(""));return;}
    if(Id==TEXT("replay")){const FString Last=StringFlag(TEXT("last_story"));if(!Last.IsEmpty())Cue(Last,Hero()->GetActorLocation());return;}
    if(Id==TEXT("map_upper")||Id==TEXT("map_lower")){Values.Add(TEXT("map_floor"),Id==TEXT("map_lower")?-1:0);Interface->Refresh();return;}
    if(Id==TEXT("agent_toggle")){if(Admin){ToggleAgent();Show(TEXT(""));}return;}
    if(Id==TEXT("admin_exit")){if(Admin)ToggleAdmin();return;}
    if(Id.StartsWith(TEXT("deposit_"))||Id.StartsWith(TEXT("withdraw_"))){const bool Deposit=Id.StartsWith(TEXT("deposit_"));StashTransfer(Id.RightChop(Deposit?8:9),Deposit);Interface->Refresh();return;}
    if(Id==TEXT("back")){Show(Screen==TEXT("difficulty")||(Screen==TEXT("saves")&&TitleView)?TEXT("game"):TitleView?TEXT("title"):CampaignStarted?TEXT("pause"):TEXT("title"));return;}
    if(Id==TEXT("quit")||Id==TEXT("page_quit")){UKismetSystemLibrary::QuitGame(this,UGameplayStatics::GetPlayerController(this,0),EQuitPreference::Quit,false);return;}
    if(Id==TEXT("latest")){if(!LoadLatest()){Notify(TEXT("Сохранений пока нет. Начни новую игру."));Interface->Refresh();}return;}
    if(Id.StartsWith(TEXT("new_"))){if(Admin)ToggleAdmin();NewGame(Id.RightChop(4));Show(TEXT(""));if(HospitalStory)HospitalStory->StartArrival();AutoSave();return;}
    if(Id.StartsWith(TEXT("load_"))){Load(Id.RightChop(5));return;}
    if(Id.StartsWith(TEXT("save_"))){const bool Saved=Save(Id.RightChop(5));Interface->SaveFeedback=Saved?Tr(TEXT("Игра сохранена")):Tr(TEXT("Не удалось записать сохранение."));Interface->Refresh();return;}
    if(Id.StartsWith(TEXT("section_"))){Interface->Section=Id.RightChop(8);Interface->GuidePage=0;Interface->Refresh();return;}
    if(Id.StartsWith(TEXT("guide_"))){Interface->GuidePage=FCString::Atoi(*Id.RightChop(6));Interface->Refresh();return;}
    if(Id==TEXT("admin_enter")){if(Interface->PendingCode==TEXT("0890")){if(!Admin)ToggleAdmin();if(Admin)Show(TEXT("admin_tools79"));}else Notify(TEXT("Неверный пароль"));return;}
    if(Id==TEXT("inspect_turn")){if(Items.FindRef(TEXT("badge"))>0)Flag(TEXT("badge_examined"),true);Interface->Refresh();return;}
    if(Id.StartsWith(TEXT("inspect_"))){Interface->SelectedItem=Id.RightChop(8);Show(TEXT("inspect"));return;}
    if(Id.StartsWith(TEXT("drop_"))){Drop(Id.RightChop(5));Interface->Refresh();return;}
    if(Id.StartsWith(TEXT("use_"))){if(Id.EndsWith(TEXT("medkit")))Heal();else if(Id.EndsWith(TEXT("battery")))Battery();else if(Id==TEXT("use_weapon")){Shotgun=false;ReloadTime=0;}else if(Id==TEXT("use_shotgun")&&Flag(TEXT("shotgun"))){Shotgun=true;ReloadTime=0;}Interface->Refresh();return;}
    if(Id==TEXT("keypad_submit"))
    {
        if(Interface->PendingCode==TEXT("417")&&Flag(TEXT("badge"))){Flag(TEXT("quarantine_unlocked"),true);Open(TEXT("quarantine_gate"));Show(TEXT(""));AutoSave();}else {Notify(TEXT("Доступ отклонён. Проверь оборот пропуска."));Cue(TEXT("alarm"),Hero()->GetActorLocation());}return;
    }
    if(Id.StartsWith(TEXT("film_")))
    {
        if(Id.EndsWith(TEXT("left")))Interface->FilmOffset.X-=24;else if(Id.EndsWith(TEXT("right")))Interface->FilmOffset.X+=24;
        else if(Id.EndsWith(TEXT("up")))Interface->FilmOffset.Y-=24;else if(Id.EndsWith(TEXT("down")))Interface->FilmOffset.Y+=24;else Interface->FilmTurn=(Interface->FilmTurn+1)%4;
        Interface->Refresh();return;
    }
    if(Id==TEXT("xray_submit"))
    {
        if(Interface->FilmOffset.Size()<12&&Interface->FilmTurn==0){Flag(TEXT("xray"),true);Finish(TEXT("chapter_xray"));Wake(14);Show(TEXT(""));AutoSave();}else Notify(TEXT("Контуры не совпадают: совмести снимок и поверни плёнку."));return;
    }
    if(Id.StartsWith(TEXT("treatment_")))
    {
        const TArray<FString> Required={TEXT("cool"),TEXT("dose"),TEXT("release")};const FString Step=Id.RightChop(10);
        if(Interface->Treatment.Num()<3&&Step==Required[Interface->Treatment.Num()]){Interface->Treatment.Add(Step);Cue(TEXT("core"),Hero()->GetActorLocation());}
        else{Interface->Treatment.Reset();BroadcastNoise(Hero()->GetActorLocation(),2400);Notify(TEXT("Протокол нарушен. Посмотри порядок в карте пациента."));}
        if(Interface->Treatment.Num()==3){Flag(TEXT("diagnostics_complete"),true);Finish(TEXT("chapter_treatment"));Show(TEXT(""));AutoSave();}else Interface->Refresh();return;
    }
    if(Id==TEXT("settings_reset")){Options=MakeShared<FJsonObject>();Options->Values=Object(Data,TEXT("settings"))->Values;Options->SetNumberField(TEXT("max_fps"),60);WardSettings::Complete(Options);ApplySettings();Interface->Refresh();return;}
    if(Id.StartsWith(TEXT("setting_prev_"))){CycleSetting(Id.RightChop(13),-1);return;}
    if(Id.StartsWith(TEXT("setting_"))){CycleSetting(Id.RightChop(8));return;}
}
