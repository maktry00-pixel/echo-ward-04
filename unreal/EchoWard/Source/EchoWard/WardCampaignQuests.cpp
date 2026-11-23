#include "WardCampaign.h"
#include "WardHospitalGameplay.h"
#include "WardCampaignUI.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

using namespace WardData;
void AEWardCampaignMode::Wake(int32 Index){if(Enemies.IsValidIndex(Index))Enemies[Index]->Wake();}
void AEWardCampaignMode::Open(const FString& Id,bool IsOpen,bool IsLocked){if(auto* Item=Objects.Find(Id))(*Item)->SetDoor(IsOpen,IsLocked);}
void AEWardCampaignMode::Finish(const FString& Id)
{
    if(Completed.Contains(Id))return;Completed.Add(Id);Score+=100;
    if(auto* Item=Objects.Find(Id))
    {
        (*Item)->Used=true;(*Item)->Work=0;(*Item)->Duration=0;(*Item)->SetContents(false);
        if((*Item)->ObjectKind==TEXT("record")||(*Item)->ObjectKind==TEXT("medkit")||(*Item)->ObjectKind==TEXT("battery")){(*Item)->SetActorHiddenInGame(true);(*Item)->SetActorEnableCollision(false);}
    }
    if(Hero())Cue(TEXT("core"),Hero()->GetActorLocation(),.7f);SyncDoors();
}
float AEWardCampaignMode::WorkDuration(const FString& Id)const
{
    if(Id==TEXT("power")||Id==TEXT("residence_lift"))return 4;
    if(Id==TEXT("residence_plate")||Id==TEXT("chapter_final_reset")||Id==TEXT("chapter_exit_crank"))return 5;
    if(Id==TEXT("reactor")||Id==TEXT("residence_core")||Id.StartsWith(TEXT("purge_valve")))return 2.5f;
    if(Id==TEXT("isolation_generator")||Id==TEXT("annex_radio"))return 6;
    if(Id.StartsWith(TEXT("isolation_vent")))return 3;
    if(Id==TEXT("annex_files"))return 5;
    if(Id==TEXT("annex_pump")||Id==TEXT("annex_antenna"))return 4;
    if(TArray<FString>{TEXT("annex_wheel"),TEXT("annex_manual"),TEXT("annex_key"),TEXT("annex_register"),TEXT("annex_intake"),TEXT("annex_drain")}.Contains(Id))return 2.5f;
    return 0;
}
FString AEWardCampaignMode::Prompt(AEWardCampaignObject* O)const
{
    if(!O)return FString();const FString& Id=O->ObjectId;
    if(O->ObjectKind==TEXT("bed"))return Tr(HidingBed==O?TEXT("F — выбраться · Shift — задержать дыхание"):TEXT("F — спрятаться под кроватью"));
    if(O->ObjectKind==TEXT("door"))
    {
        if(O->Barricade>0)return Tr(TEXT("Укреплено · удерживай K — разобрать"));
        if(O->Locked)return Tr(Id==TEXT("quarantine_gate")?TEXT("F — кодовый замок карантина"):TEXT("F — проверить запертую дверь"));
        return Tr(O->Opened?TEXT("F — закрыть дверь"):TEXT("F — открыть дверь"));
    }
    if(O->ObjectKind==TEXT("partition"))return Tr(TEXT("V / выстрел — разбить преграду"));
    if(O->ObjectKind==TEXT("save")||Id==TEXT("refuge_save"))return Tr(TEXT("F — журнал сохранений"));
    if(O->HasStorage()&&!O->Opened)return Tr(TEXT("F — открыть старую тумбу"));
    if(O->Definition->HasField(TEXT("storage"))&&!O->Found)return Tr(FString::Printf(TEXT("1 / 2 / 3 — отделение [%d] · удерживай F — осмотреть · C — закрыть"),O->Selected+1));
    if(O->Used)return Tr(TEXT("Осмотрено · C — закрыть ящик"));
    if(Id.StartsWith(TEXT("dial_"))||Id.StartsWith(TEXT("annex_dial_")))return Tr(FString::Printf(TEXT("F — повернуть контур / %d / 0–3"),O->Value));
    if(Id==TEXT("annex_frequency")){const TCHAR* Frequencies[]={TEXT("90.6"),TEXT("101.2"),TEXT("104.8"),TEXT("119.1")};return Tr(TEXT("F — настроить волну / "))+Frequencies[O->Value%4]+TEXT(" МГц");}
    FString Name=String(Object(O->Definition,TEXT("properties")),TEXT("caption"));
    if(Name.IsEmpty())Name=Id==TEXT("weapon")?TEXT("Служебный пистолет"):Id==TEXT("fuse")?TEXT("Предохранитель"):Id==TEXT("badge")?TEXT("Пропуск сотрудника"):Id==TEXT("map")?TEXT("План отделения"):Id==TEXT("power")?TEXT("Аварийный щиток"):Id==TEXT("terminal")?TEXT("Управление выходом"):TEXT("Осмотреть / забрать");
    return Tr(WorkDuration(Id)>0?TEXT("Удерживай F — "):TEXT("F — "))+Tr(Name);
}
void AEWardCampaignMode::Use(AEWardCampaignObject* O,bool FinishedWork)
{
    if(!O||!Hero()||Won||PendingRespawn)return;const FString Id=O->ObjectId;
    if(O->ObjectKind==TEXT("bed")){ToggleHide(O);return;}
    if(O->ObjectKind==TEXT("save")||Id==TEXT("refuge_save")){Show(TEXT("saves"));return;}
    if(Id==TEXT("refuge_stash")){Show(TEXT("stash"));return;}
    if(O->ObjectKind==TEXT("partition")){Notify(TEXT("V или выстрел — разбить тонкую преграду."));return;}
    if(O->ObjectKind==TEXT("door"))
    {
        if(O->Broken)return;
        if(O->Locked)
        {
            const FString Lock=String(Object(O->Definition,TEXT("properties")),TEXT("lock_kind"));
            if(Id==TEXT("quarantine_gate")&&Flag(TEXT("badge"))&&!Flag(TEXT("terminal"))){Interface->PendingCode.Empty();Show(TEXT("keypad"));return;}
            if(Lock==TEXT("power")&&Flag(TEXT("power_on")))O->Locked=false;
            else if(Lock==TEXT("badge")&&Flag(TEXT("badge")))O->Locked=false;
            else if(Lock==TEXT("inside"))
            {
                const double X=Hero()->GetActorLocation().Y/100;
                if((Id==TEXT("service_shortcut")&&X<-15)||(Id==TEXT("archive_shortcut")&&X<-3))O->Locked=false;
            }
            else if(Lock==TEXT("res_service")&&Flag(TEXT("res_prybar"))&&Hero()->GetActorLocation().Y<7900)
            {Flag(TEXT("res_service"),true);O->Locked=false;AutoSave();}
            if(O->Locked){Notify(Objective());return;}
        }
        O->ToggleDoor();UpdateBlocked();return;
    }
    if(O->Used)return;
    if(O->ObjectKind==TEXT("dropped")){Json P=Object(O->Definition,TEXT("properties"));if(Take(String(P,TEXT("supply")),Number(P,TEXT("amount"),1))){O->Used=true;O->SetActorHiddenInGame(true);O->SetActorEnableCollision(false);}return;}
    auto Require=[&](bool OK,const FString& Message){if(!OK)Notify(Message);return OK;};
    if(Id.StartsWith(TEXT("dial_")))
    {
        if(!Flag(TEXT("badge")))return;TArray<FString> P;Id.ParseIntoArray(P,TEXT("_"));if(P.Num()==3&&!Completed.Contains(TEXT("relay_")+P[1])){O->Value=(O->Value+1)%4;Values.Add(Id,O->Value);Cue(TEXT("reload"),O->GetActorLocation(),.5f);}return;
    }
    if(Id.StartsWith(TEXT("relay_")))
    {
        if(!Require(Flag(TEXT("badge"))&&Flag(TEXT("power_on")),TEXT("Нужны питание и пропуск сотрудника.")))return;
        const FString Department=Id.RightChop(6);const auto& Circuit=Array(Object(Data,TEXT("circuits")),Department);
        for(int32 I=0;I<3;++I)if(Circuit.Num()!=3||Values.FindRef(FString::Printf(TEXT("dial_%s_%d"),*Department,I))!=Circuit[I]->AsNumber()){Notify(TEXT("Схема не замкнута. Настрой три цепи по надписи на стене рядом с узлом."));return;}
        if(Department==TEXT("theatre")&&!Enemies[5]->Dead){Notify(TEXT("Охранный узел заблокирован. Уничтожь хранителя операционной."));return;}
        Finish(Id);BroadcastNoise(O->GetActorLocation(),2000);Wake(Department==TEXT("morgue")?6:7);AutoSave();return;
    }
    if(Id.StartsWith(TEXT("ammo_")))
    {
        const int32 Amount=DifficultyValue(TEXT("ammo"));if(Take(TEXT("ammo"),FMath::Min(Amount,72-Hero()->Reserve)))Finish(Id);return;
    }
    if(O->ObjectKind==TEXT("medkit")||O->ObjectKind==TEXT("battery")){if(Take(O->ObjectKind))Finish(Id);return;}
    if(O->ObjectKind==TEXT("record"))
    {
        const int32 Index=FCString::Atoi(*Id.RightChop(7));const auto& Stories=Array(Data,TEXT("stories"));if(Stories.IsValidIndex(Index)){Journal.AddUnique(Stories[Index]->AsString());Notify(Stories[Index]->AsString(),12);}Finish(Id);return;
    }
    if(Id.StartsWith(TEXT("purge_valve")))
    {
        if(!Require(Fault==Id,TEXT("Давление в норме. Этот вентиль пока не нужен.")))return;
        if(!FinishedWork){O->Duration=WorkDuration(Id);return;}Fault.Empty();Finish(Id);return;
    }
    if(Id==TEXT("power")&&!Require(Items.FindRef(TEXT("fuse"))>0,TEXT("Запасной предохранитель оставили в палате 05 справа.")))return;
    if(Id==TEXT("badge")&&!Require(Flag(TEXT("power_on")),TEXT("Сначала нужно восстановить питание отделения.")))return;
    if((Id==TEXT("coolant")||Id==TEXT("fuel"))&&!Require(Completed.Contains(TEXT("relay_theatre"))&&Completed.Contains(TEXT("relay_lab"))&&Completed.Contains(TEXT("relay_morgue")),TEXT("Сначала включи три аварийных узла.")))return;
    if(Id==TEXT("reactor"))
    {
        if(!Require(Items.FindRef(TEXT("fuel"))>0&&Items.FindRef(TEXT("coolant"))>0,TEXT("Нужны хладагент из насосной и топливо из ремонтной зоны нижнего сектора."))||Flag(TEXT("reactor_started")))return;
    }
    if(Id.StartsWith(TEXT("residence_")))
    {
        if(!Require(Flag(TEXT("purge_complete")),TEXT("Сначала очисти реактор.")))return;
        if(Id==TEXT("residence_panel"))
        {
            if(Flag(TEXT("res_complete")))return;Values.Add(TEXT("res_mode"),Values.FindRef(TEXT("res_mode"))==2?1:2);SyncDoors();Cue(TEXT("door"),O->GetActorLocation());BroadcastNoise(O->GetActorLocation(),1800);return;
        }
        if(Id==TEXT("residence_core"))
        {
            if(!Require(Values.FindRef(TEXT("res_mode"))!=1&&Flag(TEXT("res_prybar"))&&Flag(TEXT("res_service")),TEXT("Обесточь запад и вернись через служебный обход с монтажным инструментом.")))return;
            if(auto* Lock=Objects.Find(TEXT("residence_lock"));Lock&&!(*Lock)->Broken){Notify(TEXT("Сначала разбей крепление выстрелом или ударом ножа V."));return;}
        }
        if(Id==TEXT("residence_lift")||Id==TEXT("residence_plate"))
        {
            if(!Require(Flag(TEXT("res_core"))&&Values.FindRef(TEXT("res_mode"))==2,TEXT("Нужны модуль из западной ветки и питание востока.")))return;
            if(Id==TEXT("residence_lift")&&!Require(Flag(TEXT("res_plate")),TEXT("Освободи ручной стопор у восточной стены дальней спальни.")))return;
        }
    }
    if(Id.StartsWith(TEXT("chapter_")))
    {
        const FString Part=Id.RightChop(8);
        if(Part==TEXT("xray")){if(Flag(TEXT("badge"))){Interface->FilmTurn=1;Interface->FilmOffset=FVector2D(72,-48);Show(TEXT("xray"));}else Notify(TEXT("Нужен допуск сотрудника."));return;}
        if(Part==TEXT("observe"))
        {
            if(!Require(Flag(TEXT("xray")),TEXT("Карта закрыта. Восстанови экспозиции в рентгене.")))return;
            Flag(TEXT("observed"),true);Finish(Id);Open(TEXT("diag_loop"));Wake(15);Journal.AddUnique(TEXT("Пациент 04: охлаждение → седативное → разблокировка."));Notify(Journal.Last(),12);return;
        }
        if(Part==TEXT("treatment")){if(Require(Flag(TEXT("observed")),TEXT("Сначала прочти карту в наблюдательной."))){Interface->Treatment.Empty();Show(TEXT("treatment"));}return;}
        if(Part==TEXT("memo")||Part==TEXT("log")||Part==TEXT("office"))
        {
            Finish(Id);FString Text=Part==TEXT("memo")?TEXT("Ирина: «Мою смену стёрли. В журнале переводов осталась строка — ищи в наблюдательной»."):Part==TEXT("log")?TEXT("Журнал: Ирина не покидала отделение. Приказ подписан врачом."):TEXT("Врач знал о побеге пациента. Ирина вывела людей через служебный вход. Ты забираешь доказательства для её семьи.");
            Journal.AddUnique(Text);Notify(Text,12);if(Completed.Contains(TEXT("chapter_memo"))&&Completed.Contains(TEXT("chapter_log")))Open(TEXT("diag_office"));if(Part==TEXT("office"))Take(TEXT("medkit"));return;
        }
        if(Part==TEXT("supplies")){if(Take(TEXT("ammo"),8))Finish(Id);return;}
        if(Part==TEXT("trap")){Flag(TEXT("trap_armed"),true);Finish(Id);Notify(TEXT("Мокрый участок под током. Примани противника бутылкой; сам обойди воду."));return;}
        if(Part==TEXT("final_reset")||Part==TEXT("exit_crank"))
        {
            if(!Require(Flag(TEXT("terminal"))&&(Part!=TEXT("exit_crank")||Flag(TEXT("breaker"))),TEXT("Сначала команда терминала карантина, затем резервный ввод.")))return;
        }
    }
    if(Id.StartsWith(TEXT("isolation_")))
    {
        if(!Require(Flag(TEXT("diagnostics_complete")),TEXT("Сначала закончи медицинскую диагностику.")))return;
        const FString Part=Id.RightChop(10);
        if(Part==TEXT("light")||Part==TEXT("dark"))
        {
            if(!Require(Completed.Contains(TEXT("isolation_case")),TEXT("Сначала прочитай дело на посту охраны.")))return;
            if(!StringFlag(TEXT("circuit")).IsEmpty()){Notify(TEXT("Линия уже выбрана. Резерва на вторую нет."));return;}
            StringFlag(TEXT("circuit"),Part);Finish(Id);SyncDoors();AutoSave();return;
        }
        if(Part.StartsWith(TEXT("coil_"))&&!Require(StringFlag(TEXT("circuit"))==Part.RightChop(5),TEXT("Этот шкаф остался за отключённой линией.")))return;
        if(Part==TEXT("tag")&&StringFlag(TEXT("circuit")).IsEmpty())return;
        if(Part==TEXT("generator")&&!Require(Flag(TEXT("iso_coil"))&&Flag(TEXT("iso_tag")),TEXT("Нужны контактный блок выбранной ветки и браслет из душевых.")))return;
        if(Part.StartsWith(TEXT("vent_"))&&!Require(Fault==Id,TEXT("Этот сброс сейчас не требуется.")))return;
        if(Part==TEXT("log")&&!Require(Flag(TEXT("generator_ready")),TEXT("Регистратору нужно питание генератора.")))return;
        if(Part==TEXT("proof")&&!Require(Completed.Contains(TEXT("isolation_diary"))&&Completed.Contains(TEXT("isolation_report")),TEXT("Нужны дневник санитара и отчёт охраны.")))return;
        if(Part==TEXT("release")&&!Require(Completed.Contains(TEXT("isolation_log")),TEXT("Сначала выгрузи журнал из регистратора.")))return;
        if(Part==TEXT("ammo")){if(Take(TEXT("ammo"),DifficultyValue(TEXT("ammo"))))Finish(Id);return;}
        if(Part==TEXT("diary")||Part==TEXT("report")||Part==TEXT("proof")){Journal.AddUnique(Part==TEXT("diary")?TEXT("Пациент 17 пытался вывести людей через душевые. Его подпись есть в журнале."):Part==TEXT("report")?TEXT("Приказ о закрытии камер поступил до аварии. Дневник и отчёт открывают архив."):TEXT("Архив 17: людей оставили внутри намеренно. Копия приказа сохранена."));Notify(Journal.Last(),12);}
    }
    if(Id.StartsWith(TEXT("annex_")))
    {
        if(!Require(Flag(TEXT("isolation_complete")),TEXT("Хозяйственный корпус открывается после изолятора 17.")))return;
        const FString Part=Id.RightChop(6);
        if(Part.StartsWith(TEXT("dial_")))
        {
            if(!Require(Completed.Contains(TEXT("annex_pump"))&&Completed.Contains(TEXT("annex_manual")),TEXT("Сначала восстанови насос и прочти журнал слесаря.")))return;
            O->Value=(O->Value+1)%4;Values.Add(Id,O->Value);Cue(TEXT("reload"),O->GetActorLocation(),.5f);return;
        }
        if(Part==TEXT("frequency")){if(!Completed.Contains(TEXT("annex_radio"))){O->Value=(O->Value+1)%4;Values.Add(Id,O->Value);}return;}
        for(const auto& Requirement:Array(Object(Data,TEXT("annex_requirements")),Part))if(!Require(Completed.Contains(TEXT("annex_")+Requirement->AsString()),Objective()))return;
        if(Part==TEXT("boiler")&&!Require(Values.FindRef(TEXT("annex_dial_a"))==2&&Values.FindRef(TEXT("annex_dial_b"))==1&&Values.FindRef(TEXT("annex_dial_c"))==3,TEXT("Журнал слесаря: контуры А — 2, Б — 1, В — 3.")))return;
        if(Part==TEXT("radio")&&!Require(Values.FindRef(TEXT("annex_frequency"))==2,TEXT("Дежурная волна — 104.8 МГц.")))return;
        if(Part.StartsWith(TEXT("ammo_"))||Part.StartsWith(TEXT("aid_"))){if(Take(Part.StartsWith(TEXT("ammo_"))?TEXT("ammo"):TEXT("medkit"),Part.StartsWith(TEXT("ammo_"))?12:1))Finish(Id);return;}
    }
    if(Id.StartsWith(TEXT("upgrade_")))
    {
        const FString Part=Id.RightChop(8);
        if(Part==TEXT("shotgun")){if(Take(TEXT("shotgun"))){Finish(Id);Notify(TEXT("Помповый дробовик найден. Z — сменить оружие; R — зарядить по одному патрону."));}return;}
        if(Part.StartsWith(TEXT("shells_"))){if(Take(TEXT("shells"),DifficultyValue(TEXT("shells"))))Finish(Id);return;}
        if(Part.StartsWith(TEXT("boards_"))){if(Take(TEXT("boards"),1))Finish(Id);return;}
        if(Part.StartsWith(TEXT("record_"))){Finish(Id);Journal.AddUnique(String(Object(Object(Data,TEXT("upgrade_recordings")),Part.RightChop(7)),TEXT("text"),TEXT("Ночная смена: запись сохранена.")));Cue(TEXT("story_")+Part.RightChop(7),O->GetActorLocation());Notify(Journal.Last(),12);return;}
    }
    if(Id==TEXT("terminal"))
    {
        if(!Require(Flag(TEXT("power_on"))&&Flag(TEXT("badge"))&&Flag(TEXT("purge_complete"))&&Flag(TEXT("res_complete"))&&Flag(TEXT("diagnostics_complete"))&&Flag(TEXT("isolation_complete"))&&Flag(TEXT("annex_complete")),Objective()))return;
    }
    if(!FinishedWork&&WorkDuration(Id)>0){O->Duration=WorkDuration(Id);return;}
    if(Id==TEXT("fuse")){if(!Take(TEXT("fuse")))return;Wake(0);Wake(1);}
    else if(Id==TEXT("weapon")){if(!Take(TEXT("weapon")))return;Hero()->Pistol->SetVisibility(true);}
    else if(Id==TEXT("map"))Flag(TEXT("map"),true);
    else if(Id==TEXT("power")){Items.Remove(TEXT("fuse"));Flag(TEXT("power_on"),true);PowerOn=true;AutoSave();}
    else if(Id==TEXT("badge")){if(!Take(TEXT("badge")))return;Flag(TEXT("badge"),true);AutoSave();Notify(TEXT("Пропуск найден. X — осмотреть; проверь оборот."),8);}
    else if(Id==TEXT("fuel")||Id==TEXT("coolant")){if(!Take(Id))return;}
    else if(Id==TEXT("reactor")){Items.Remove(TEXT("fuel"));Items.Remove(TEXT("coolant"));Flag(TEXT("reactor_started"),true);for(int32 I:{8,9,10})Wake(I);BroadcastNoise(O->GetActorLocation(),5500);return;}
    else if(Id==TEXT("residence_prybar")){if(!Take(TEXT("residence_prybar")))return;Flag(TEXT("res_prybar"),true);Wake(11);AutoSave();}
    else if(Id==TEXT("residence_core")){if(!Take(TEXT("residence_core")))return;Flag(TEXT("res_core"),true);Wake(13);Open(TEXT("res_cell"));AutoSave();}
    else if(Id==TEXT("residence_plate"))Flag(TEXT("res_plate"),true);
    else if(Id==TEXT("residence_lift")){Flag(TEXT("res_complete"),true);Items.Remove(TEXT("residence_core"));Items.Remove(TEXT("residence_prybar"));Values.Add(TEXT("res_mode"),0);Wake(13);AutoSave();}
    else if(Id==TEXT("isolation_coil_light")||Id==TEXT("isolation_coil_dark")){if(!Take(TEXT("isolation_coil")))return;Flag(TEXT("iso_coil"),true);}
    else if(Id==TEXT("isolation_tag")){if(!Take(TEXT("isolation_tag")))return;Flag(TEXT("iso_tag"),true);Flag(TEXT("iso_released"),true);Wake(20);Open(TEXT("iso_cell"));AutoSave();}
    else if(Id==TEXT("isolation_generator")){Flag(TEXT("generator_started"),true);Items.Remove(TEXT("isolation_coil"));Wake(18);Wake(19);return;}
    else if(Id.StartsWith(TEXT("isolation_vent_"))){Fault.Empty();Finish(Id);return;}
    else if(Id==TEXT("isolation_log"))Items.Remove(TEXT("isolation_tag"));
    else if(Id==TEXT("isolation_release")){Flag(TEXT("isolation_complete"),true);AutoSave();}
    else if(Id==TEXT("annex_wheel")){if(!Take(TEXT("annex_wheel")))return;Wake(21);}
    else if(Id==TEXT("annex_manual"))Journal.AddUnique(TEXT("Журнал слесаря: контуры котла А — 2, Б — 1, В — 3."));
    else if(Id==TEXT("annex_pump")){Items.Remove(TEXT("annex_wheel"));Wake(22);}
    else if(Id==TEXT("annex_files")){Wake(23);Journal.AddUnique(TEXT("Маршрут 12-Б: санитарный выход. Дежурная волна 104.8 МГц."));}
    else if(Id==TEXT("annex_radio"))Wake(24);
    else if(Id==TEXT("annex_release")){Flag(TEXT("annex_complete"),true);AutoSave();}
    else if(Id==TEXT("chapter_final_reset"))Flag(TEXT("breaker"),true);
    else if(Id==TEXT("chapter_exit_crank"))Flag(TEXT("exit_released"),true);
    else if(Id==TEXT("terminal")){Flag(TEXT("terminal"),true);EscapeTime=DifficultyValue(TEXT("escape"));Wake(16);Open(TEXT("quarantine_gate"),false,true);Open(TEXT("wing_-9_-9"),false,true);Open(TEXT("wing_9_-9"),false,true);Notify(TEXT("Главная заслонка упала! Обойди через диагностику, сбрось ввод в щитовой и открой рукоять у главного входа."),12);}
    Finish(Id);BroadcastNoise(O->GetActorLocation(),900);
}
void AEWardCampaignMode::SyncDoors(bool Restore)
{
    if(Restore){UpdateBlocked();return;}
    for(auto& Entry:Objects)
    {
        auto* O=Entry.Value.Get();if(O->ObjectKind!=TEXT("door"))continue;
        const FString Lock=String(Object(O->Definition,TEXT("properties")),TEXT("lock_kind"));
        if((Lock==TEXT("power")&&Flag(TEXT("power_on")))||(Lock==TEXT("badge")&&Flag(TEXT("badge"))&&(O->ObjectId!=TEXT("quarantine_gate")||Flag(TEXT("quarantine_unlocked")))))O->Locked=false;
    }
    if(Completed.Contains(TEXT("relay_morgue"))&&Completed.Contains(TEXT("relay_lab"))&&Completed.Contains(TEXT("relay_theatre")))Open(TEXT("technical_gate"),Restore?Objects[TEXT("technical_gate")]->Opened:true,false);
    if(Flag(TEXT("purge_complete"))&&!Flag(TEXT("res_complete")))Open(TEXT("res_entry"),Restore?Objects[TEXT("res_entry")]->Opened:false,false);
    if(Flag(TEXT("res_service")))Open(TEXT("res_west_service"),Restore?Objects[TEXT("res_west_service")]->Opened:true,false);
    for(auto& Entry:Objects)
    {
        auto* O=Entry.Value.Get();const FString Lock=String(Object(O->Definition,TEXT("properties")),TEXT("lock_kind"));
        if(Lock==TEXT("res_west")||Lock==TEXT("res_east"))O->SetDoor((Restore?O->Opened:true)&&Values.FindRef(TEXT("res_mode"))==(Lock==TEXT("res_west")?1:2),Values.FindRef(TEXT("res_mode"))!=(Lock==TEXT("res_west")?1:2));
    }
    if(Flag(TEXT("res_complete"))){Open(TEXT("res_entry"),false,true);Open(TEXT("res_return"));Open(TEXT("res_cell"));}
    if(Flag(TEXT("badge")))Open(TEXT("diag_entry"),Restore?Objects[TEXT("diag_entry")]->Opened:false,false);
    if(Flag(TEXT("xray"))){Open(TEXT("diag_37"));Open(TEXT("diag_cross"));}
    if(Flag(TEXT("observed")))Open(TEXT("diag_loop"));
    if(Completed.Contains(TEXT("chapter_memo"))&&Completed.Contains(TEXT("chapter_log")))Open(TEXT("diag_office"));
    if(Flag(TEXT("diagnostics_complete"))){Open(TEXT("diag_52"));Open(TEXT("iso_entry"),Restore?Objects[TEXT("iso_entry")]->Opened:false,false);}
    const FString Circuit=StringFlag(TEXT("circuit"));if(!Circuit.IsEmpty()){Open(TEXT("iso_west"),Circuit==TEXT("light"),Circuit!=TEXT("light"));Open(TEXT("iso_east"),Circuit==TEXT("dark"),Circuit!=TEXT("dark"));}
    if(Completed.Contains(TEXT("isolation_diary"))&&Completed.Contains(TEXT("isolation_report")))for(const TCHAR* Id:{TEXT("iso_archive"),TEXT("iso_archive_inner"),TEXT("iso_north")})Open(Id);
    if(Completed.Contains(TEXT("isolation_proof")))Open(TEXT("iso_shortcut"));
    if(Flag(TEXT("iso_released")))Open(TEXT("iso_cell"));
    if(Flag(TEXT("isolation_complete")))Open(TEXT("annex_entry"));
    if(Completed.Contains(TEXT("annex_brief")))Open(TEXT("annex_stores"));
    if(Completed.Contains(TEXT("annex_pump")))Open(TEXT("annex_steam"));
    if(Completed.Contains(TEXT("annex_key")))Open(TEXT("annex_east162"));
    if(Completed.Contains(TEXT("annex_files")))Open(TEXT("annex_w183"));
    if(Flag(TEXT("terminal"))){Open(TEXT("quarantine_gate"),false,true);Open(TEXT("final_passage"));Open(TEXT("diag_entry"));Open(TEXT("final_research_return"));}
    UpdateBlocked();
}
void AEWardCampaignMode::TickQuests(float Delta)
{
    if(Flag(TEXT("reactor_started"))&&!Flag(TEXT("purge_complete")))
    {
        if(Fault.IsEmpty()&&!HidingBed&&Objects.Contains(TEXT("reactor"))&&FVector::Dist(Hero()->GetActorLocation(),Objects[TEXT("reactor")]->GetActorLocation())<1200)ReactorTime=FMath::Max(0.f,ReactorTime-Delta);
        if(ReactorTime<=30&&!Completed.Contains(TEXT("purge_valve_a"))&&Fault.IsEmpty())Fault=TEXT("purge_valve_a");
        if(ReactorTime<=15&&!Completed.Contains(TEXT("purge_valve_b"))&&Fault.IsEmpty())Fault=TEXT("purge_valve_b");
        if(ReactorTime<=0&&Fault.IsEmpty()){Flag(TEXT("purge_complete"),true);Finish(TEXT("reactor"));AutoSave();}
    }
    if(Flag(TEXT("generator_started"))&&!Flag(TEXT("generator_ready")))
    {
        if(Fault.IsEmpty())GeneratorTime=FMath::Min(25.f,GeneratorTime+Delta);
        if(GeneratorTime>=7&&!Completed.Contains(TEXT("isolation_vent_a")))Fault=TEXT("isolation_vent_a");
        else if(GeneratorTime>=16&&!Completed.Contains(TEXT("isolation_vent_b")))Fault=TEXT("isolation_vent_b");
        if(GeneratorTime>=25){Flag(TEXT("generator_ready"),true);Finish(TEXT("isolation_generator"));AutoSave();}
    }
    const FVector P=Hero()->GetActorLocation();const double X=P.Y/100,Z=-P.X/100;
    if(Flag(TEXT("badge"))&&X>15&&X<60&&Z>9&&Z<32&&P.Z>0)
    {const int32 I=X<30?2:X<44?3:4;const FString Event=FString::Printf(TEXT("encounter_%d"),I);if(!Flag(Event)){Flag(Event,true);Wake(I);BroadcastNoise(P,2200);Cue(TEXT("sting"),P);}}
    if(Flag(TEXT("res_core"))&&X>86&&Z>12&&!Flag(TEXT("east_seen"))){Flag(TEXT("east_seen"),true);Wake(12);BroadcastNoise(P,1800);}
    if(Flag(TEXT("badge"))&&X>44.4&&X<59.6&&Z>-5&&Z<30&&!Flag(TEXT("guardian_started")))
    {Flag(TEXT("guardian_started"),true);NumberFlag(TEXT("guardian_intro"),2.6);Cue(TEXT("growl"),Objects[TEXT("sterile_52")]->GetActorLocation());Notify(TEXT("За дверью тяжёлое дыхание. Отойди от проёма. После рывка броня хранителя раскрывается."),7);}
    if(Flag(TEXT("guardian_started"))&&!Flag(TEXT("guardian_released")))
    {NumberFlag(TEXT("guardian_intro"),FMath::Max(0.,NumberFlag(TEXT("guardian_intro"))-Delta));if(NumberFlag(TEXT("guardian_intro"))<=0){Flag(TEXT("guardian_released"),true);Objects[TEXT("sterile_52")]->Break();Wake(5);Enemies[5]->Memory=12;Enemies[5]->Observed=P;}}
    if(Flag(TEXT("trap_armed"))&&!Flag(TEXT("trap_spent")))
    {
        if(X>=46.5&&X<=50&&Z>=-36&&Z<=-33){Flag(TEXT("trap_spent"),true);DamageHero(33.34f,nullptr);}
        for(auto E:Enemies)if(E->Active&&!E->Dead){const FVector Q=E->GetActorLocation();if(Q.Y>=4650&&Q.Y<=5000&&Q.X>=3300&&Q.X<=3600){Flag(TEXT("trap_spent"),true);UGameplayStatics::ApplyDamage(E,75,nullptr,Hero(),nullptr);E->Stunned=4;}}
    }
    if(StringFlag(TEXT("circuit"))==TEXT("dark")&&!Flag(TEXT("generator_ready"))&&FMath::Fmod(Elapsed,3.4f)<.85f&&X>=48&&X<=56&&Z>=-83&&Z<=-80)DamageHero(33.34f,nullptr);
    if(Z<-62&&!Flag(TEXT("iso_seen"))){Flag(TEXT("iso_seen"),true);Wake(20);}
}
FString AEWardCampaignMode::Goal()const
{
    if(BookWorldEnabled)return FString();
    if(Flag(TEXT("terminal")))return !Flag(TEXT("breaker"))?TEXT("chapter_final_reset"):!Flag(TEXT("exit_released"))?TEXT("chapter_exit_crank"):TEXT("exit");
    if(!Flag(TEXT("power_on")))return Items.FindRef(TEXT("fuse"))>0?TEXT("power"):TEXT("fuse");
    if(!Flag(TEXT("badge")))return TEXT("badge");
    for(const TCHAR* Department:{TEXT("morgue"),TEXT("lab"),TEXT("theatre")})if(!Completed.Contains(FString(TEXT("relay_"))+Department))return FString(TEXT("relay_"))+Department;
    if(!Flag(TEXT("purge_complete")))return Flag(TEXT("reactor_started"))?(Fault.IsEmpty()?TEXT("reactor"):Fault):Items.FindRef(TEXT("coolant"))==0?TEXT("coolant"):Items.FindRef(TEXT("fuel"))==0?TEXT("fuel"):TEXT("reactor");
    if(!Flag(TEXT("res_complete")))return !Flag(TEXT("res_prybar"))?TEXT("residence_prybar"):!Flag(TEXT("res_service"))?TEXT("res_west_service"):!Flag(TEXT("res_core"))?TEXT("residence_core"):!Flag(TEXT("res_plate"))?TEXT("residence_plate"):TEXT("residence_lift");
    if(!Flag(TEXT("diagnostics_complete")))return !Flag(TEXT("xray"))?TEXT("chapter_xray"):!Flag(TEXT("observed"))?TEXT("chapter_observe"):TEXT("chapter_treatment");
    if(!Flag(TEXT("isolation_complete")))
    {
        if(!Completed.Contains(TEXT("isolation_case")))return TEXT("isolation_case");if(StringFlag(TEXT("circuit")).IsEmpty())return TEXT("isolation_light");
        if(!Flag(TEXT("iso_coil")))return TEXT("isolation_coil_")+StringFlag(TEXT("circuit"));if(!Flag(TEXT("iso_tag")))return TEXT("isolation_tag");
        if(!Flag(TEXT("generator_ready")))return Fault.IsEmpty()?TEXT("isolation_generator"):Fault;return !Completed.Contains(TEXT("isolation_log"))?TEXT("isolation_log"):TEXT("isolation_release");
    }
    if(!Flag(TEXT("annex_complete")))
    {
        for(const TCHAR* Id:{TEXT("brief"),TEXT("wheel"),TEXT("manual"),TEXT("intake"),TEXT("drain"),TEXT("pump")})if(!Completed.Contains(FString(TEXT("annex_"))+Id))return FString(TEXT("annex_"))+Id;
        const TCHAR* Dials[]={TEXT("annex_dial_a"),TEXT("annex_dial_b"),TEXT("annex_dial_c")};int32 Wanted[]={2,1,3};
        if(!Completed.Contains(TEXT("annex_boiler")))for(int32 I=0;I<3;++I)if(Values.FindRef(Dials[I])!=Wanted[I])return Dials[I];
        for(const TCHAR* Id:{TEXT("boiler"),TEXT("key"),TEXT("register"),TEXT("files"),TEXT("antenna")})if(!Completed.Contains(FString(TEXT("annex_"))+Id))return FString(TEXT("annex_"))+Id;
        if(!Completed.Contains(TEXT("annex_radio")))return Values.FindRef(TEXT("annex_frequency"))!=2?TEXT("annex_frequency"):TEXT("annex_radio");return TEXT("annex_release");
    }
    return TEXT("terminal");
}
FString AEWardCampaignMode::Objective()const
{
    if(BookWorldEnabled)return HospitalStory?HospitalStory->Objective():Tr(TEXT("Исследуй весь больничный комплекс · M — полный план"));
    const FString Id=Goal();
    if(!Fault.IsEmpty())return Tr(TEXT("АВАРИЙНОЕ ДАВЛЕНИЕ · найди красный вентиль · удерживай F"));
    if(Id==TEXT("fuse"))return Tr(TEXT("Найди предохранитель · палата 05 справа"));
    if(Id==TEXT("power"))return Tr(TEXT("Восстанови питание · служебное крыло слева"));
    if(Id==TEXT("badge"))return Tr(TEXT("Найди пропуск сотрудника · архив справа"));
    if(Id.StartsWith(TEXT("relay_")))return Tr(TEXT("Включи три аварийных узла · исследовательское крыло"));
    if(Id==TEXT("coolant")||Id==TEXT("fuel")||Id==TEXT("reactor"))return Tr(TEXT("Очистка реактора · лестница за операционной → нижний сектор"));
    if(Id.StartsWith(TEXT("res")))return Tr(TEXT("Жилой корпус · переключай питание, найди инструмент и освободи лифт"));
    if(Id.StartsWith(TEXT("chapter_")))return Tr(Flag(TEXT("terminal"))?TEXT("Аварийный ввод → рукоять главной заслонки"):TEXT("Диагностика · рентген → наблюдение → процедурная"));
    if(Id.StartsWith(TEXT("isolation_")))return Tr(TEXT("Изолятор 17 · браслет → генератор → журнал → блокировка"));
    if(Id.StartsWith(TEXT("annex_")))return Tr(TEXT("Хозяйственный корпус · насос → котельная → архив → связь снаружи"));
    if(Id==TEXT("exit"))return Tr(TEXT("Заслонка открыта · вернись к главному входу"));
    return Tr(TEXT("Вернись к терминалу карантина"));
}
