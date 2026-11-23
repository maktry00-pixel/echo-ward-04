#include "WardCampaign.h"
#include "WardOnline80.h"
#include "WardGameplay76.h"
#include "Camera/PlayerCameraManager.h"
#include "Serialization/JsonSerializer.h"
#include "WardAudio68.h"
#include "WardCombat63.h"
#include "WardBookWorld.h"
#include "WardHospitalGameplay.h"
#include "WardBookMap.h"
#include "WardCampaignUI.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
using namespace WardData;
namespace
{
    TArray<TSharedPtr<FJsonValue>> V3(FVector P){return {MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y),MakeShared<FJsonValueNumber>(P.Z)};}
    TArray<TSharedPtr<FJsonValue>> Strings(const TSet<FString>& Set){TArray<TSharedPtr<FJsonValue>> Out;for(const FString& S:Set)Out.Add(MakeShared<FJsonValueString>(S));return Out;}
}
FString AEWardCampaignMode::SaveDirectory()const{const bool Test=Flag(TEXT("admin_chapter_run79"));if(!SaveFolderOverride.IsEmpty())return Test?SaveFolderOverride/TEXT("AdminChapter79Saves"):SaveFolderOverride;return FPaths::ProjectSavedDir()/(Test?TEXT("AdminChapter79Saves"):BookWorldEnabled?TEXT("HospitalStoryV3Saves"):TEXT("CampaignSaves"));}
TArray<FString> AEWardCampaignMode::SaveSlots()const{return {TEXT("manual_1"),TEXT("manual_2"),TEXT("manual_3"),TEXT("manual_4"),TEXT("manual_5"),TEXT("manual_6"),TEXT("auto_1"),TEXT("auto_2"),TEXT("auto_3")};}
Json AEWardCampaignMode::ReadSlot(const FString& Slot)const{return SaveSlots().Contains(Slot)?Read(SaveDirectory()/(Slot+TEXT(".json"))):MakeShared<FJsonObject>();}
Json AEWardCampaignMode::Capture()const
{
    Json S=MakeShared<FJsonObject>();auto* H=Hero();if(!H)return S;
    if(HospitalStory)S->SetObjectField(TEXT("hospital_story"),HospitalStory->Snapshot());
    if(BookWorldEnabled){S->SetStringField(TEXT("world_id"),String(WardBookMapData(),TEXT("map_id")));if(BookWorld)S->SetObjectField(TEXT("architecture"),BookWorld->ArchitectureSnapshot());}
    S->SetNumberField(TEXT("native_version"),2);S->SetStringField(TEXT("saved_at"),FDateTime::UtcNow().ToIso8601());S->SetStringField(TEXT("difficulty"),Difficulty);S->SetStringField(TEXT("room"),RoomName());S->SetNumberField(TEXT("elapsed"),Elapsed);S->SetObjectField(TEXT("flags"),Flags);
    S->SetNumberField(TEXT("health"),H->Health);S->SetNumberField(TEXT("stamina"),H->Stamina);S->SetNumberField(TEXT("rounds"),H->Rounds);S->SetNumberField(TEXT("reserve"),H->Reserve);S->SetNumberField(TEXT("loaded"),LoadedShells);S->SetBoolField(TEXT("shotgun"),Shotgun);S->SetNumberField(TEXT("score"),Score);S->SetNumberField(TEXT("shots"),Shots);S->SetNumberField(TEXT("hits"),Hits);S->SetNumberField(TEXT("kills"),Kills);
    S->SetArrayField(TEXT("at"),V3(H->GetActorLocation()));const FRotator View=H->GetControlRotation();S->SetArrayField(TEXT("view"),V3(FVector(View.Pitch,View.Yaw,0)));
    Json Inv=MakeShared<FJsonObject>();for(const auto& E:Items)Inv->SetNumberField(E.Key,E.Value);S->SetObjectField(TEXT("items"),Inv);
    Json Stored=MakeShared<FJsonObject>();for(const auto& E:Stash)Stored->SetNumberField(E.Key,E.Value);S->SetObjectField(TEXT("stash"),Stored);
    Json Val=MakeShared<FJsonObject>();for(const auto& E:Values)Val->SetNumberField(E.Key,E.Value);S->SetObjectField(TEXT("values"),Val);
    S->SetArrayField(TEXT("completed"),Strings(Completed));S->SetArrayField(TEXT("visited"),Strings(Visited));S->SetArrayField(TEXT("auto_visited"),Strings(AutoVisited));TArray<TSharedPtr<FJsonValue>> J;for(const auto& T:Journal)J.Add(MakeShared<FJsonValueString>(T));S->SetArrayField(TEXT("journal"),J);
    Json Obj=MakeShared<FJsonObject>();for(const auto& E:Objects)if(E.Value->ObjectKind!=TEXT("dropped")||!E.Value->Used)Obj->SetObjectField(E.Key,E.Value->Snapshot());S->SetObjectField(TEXT("objects"),Obj);TArray<TSharedPtr<FJsonValue>> E;for(auto Enemy:Enemies)E.Add(MakeShared<FJsonValueObject>(Enemy->Snapshot()));S->SetArrayField(TEXT("enemies"),E);S->SetNumberField(TEXT("flash_charges"),FlashCharges);
    S->SetNumberField(TEXT("reactor_time"),ReactorTime);S->SetNumberField(TEXT("generator_time"),GeneratorTime);S->SetNumberField(TEXT("escape_time"),EscapeTime);S->SetStringField(TEXT("fault"),Fault);
    Json Snapshot;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(WardData::Serialize(S)), Snapshot);
    return Snapshot;
}
bool AEWardCampaignMode::ValidateSave(Json S)const
{
    if(!S.IsValid()||Number(S,TEXT("native_version"))!=2||Array(S,TEXT("at")).Num()!=3||Number(S,TEXT("health"))<=0||Number(S,TEXT("health"))>100||!S->HasTypedField<EJson::Object>(TEXT("objects"))||!S->HasTypedField<EJson::Object>(TEXT("flags")))return false;
    FVector P=Vector(Array(S,TEXT("at")));
    if (P.ContainsNaN() || !FMath::IsFinite(Number(S,TEXT("health")))) return false;
    for (const auto& Entry : Object(S,TEXT("items"))->Values)
        if (Entry.Value->Type != EJson::Number || !FMath::IsFinite(Entry.Value->AsNumber()) || Entry.Value->AsNumber()<0) return false;
    const Json Arrival=Object(Object(S,TEXT("hospital_story")),TEXT("arrival29"));
    const int HomeStage=int(Number(Arrival,TEXT("home_stage52"),-1));
    if(BookWorldEnabled&&HomeStage>=0&&HomeStage<=2)
        return BookWorld&&String(S,TEXT("world_id"))==String(WardBookMapData(),TEXT("map_id"))&&!P.ContainsNaN()&&FMath::Abs(P.X+80000)<390&&FMath::Abs(P.Y+80000)<340&&P.Z>50&&P.Z<140;
    if(BookWorldEnabled)return BookWorld&&String(S,TEXT("world_id"))==String(WardBookMapData(),TEXT("map_id"))&&BookWorld->ValidPosition(P);
    if(S->HasField(TEXT("world_id")))return false;
    return !P.ContainsNaN()&&FMath::Abs(P.X)<25000&&FMath::Abs(P.Y)<15000&&P.Z>-2000&&P.Z<3000;
}
bool AEWardCampaignMode::Restore(Json S)
{
    if(!ValidateSave(S))return false;
    WardGameplay76::Reset(*this);
    Json Detached;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(WardData::Serialize(S)), Detached)) return false;
    S=Detached;
    const bool WasAdmin=Admin;if(Admin)ToggleAdmin();NewGame(String(S,TEXT("difficulty"),TEXT("normal")));
    Flags=Object(S,TEXT("flags"));Completed.Reset();Visited.Reset();AutoVisited.Reset();Journal.Reset();Items.Reset();Values.Reset();
    for(const auto& V:Array(S,TEXT("completed")))Completed.Add(V->AsString());for(const auto& V:Array(S,TEXT("visited")))Visited.Add(V->AsString());for(const auto& V:Array(S,TEXT("auto_visited")))AutoVisited.Add(V->AsString());for(const auto& V:Array(S,TEXT("journal")))Journal.Add(V->AsString());
    for(const auto& E:Object(S,TEXT("items"))->Values)Items.Add(FString(E.Key),(int32)E.Value->AsNumber());for(const auto& E:Object(S,TEXT("values"))->Values)Values.Add(FString(E.Key),(int32)E.Value->AsNumber());
    for(const auto& E:Object(S,TEXT("stash"))->Values)Stash.Add(FString(E.Key),(int32)E.Value->AsNumber());
    for(const auto& E:Object(S,TEXT("objects"))->Values){Json O=E.Value->AsObject();const FString Id(E.Key);if(!Objects.Contains(Id)&&O->HasField(TEXT("definition"))){Json Definition=Object(O,TEXT("definition")),P=Object(Definition,TEXT("properties"));SpawnSupply(String(P,TEXT("supply")),Number(P,TEXT("amount"),1),Vector(Array(Object(Definition,TEXT("transform")),TEXT("position"))),Id);}if(auto* Item=Objects.Find(Id))(*Item)->Restore(O);}
    for(const auto& V:Array(S,TEXT("enemies"))){Json E=V->AsObject();for(auto Enemy:Enemies)if(Enemy->EnemyId==String(E,TEXT("id")))Enemy->Restore(E);}
    auto* H=Hero();H->Health=Number(S,TEXT("health"),100);H->Stamina=Number(S,TEXT("stamina"),100);H->Rounds=FMath::Clamp((int32)Number(S,TEXT("rounds")),0,9);H->Reserve=FMath::Clamp((int32)Number(S,TEXT("reserve")),0,72);if(BookWorld){BookWorld->RestoreLift(Vector(Array(S,TEXT("at"))));BookWorld->RestoreArchitecture(Object(S,TEXT("architecture")));}H->SetActorLocation(Vector(Array(S,TEXT("at"))),false,nullptr,ETeleportType::TeleportPhysics);
    FVector View=Vector(Array(S,TEXT("view")));if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(FRotator(View.X,View.Y,0));H->Pistol->SetVisibility(Flag(TEXT("weapon")));
    LoadedShells=FMath::Clamp(int32(Number(S,TEXT("loaded"))),0,5);FlashCharges=FMath::Clamp((int32)Number(S,TEXT("flash_charges"),1),0,3);Shotgun=Boolean(S,TEXT("shotgun"));Elapsed=Number(S,TEXT("elapsed"));Score=Number(S,TEXT("score"));Shots=Number(S,TEXT("shots"));Hits=Number(S,TEXT("hits"));Kills=Number(S,TEXT("kills"));ReactorTime=Number(S,TEXT("reactor_time"),45);GeneratorTime=Number(S,TEXT("generator_time"));EscapeTime=Number(S,TEXT("escape_time"),-1);Fault=String(S,TEXT("fault"));PowerOn=Flag(TEXT("power_on"));Invulnerable=2;SyncDoors(true);if(HospitalStory)HospitalStory->Restore(Object(S,TEXT("hospital_story")));
    // Arrival and story refresh reconstruct defaults. Explicit saved access wins afterward.
    const Json Architecture=Object(S,TEXT("architecture"));
    if(BookWorld&&Architecture->HasTypedField<EJson::Object>(TEXT("locks")))
    {
        const Json Doors=Object(Architecture,TEXT("doors")), Locks=Object(Architecture,TEXT("locks"));
        const Json Inside=Object(Architecture,TEXT("inside_only"));
        for(auto& Door:BookWorld->Portals)
        {
            // The old class-C lock was applied even after the story opened
            // the rooftop route. Retain the saved leaf position, migrate access.
            if(HospitalStory&&HospitalStory->Chapter()>=14&&Door.Zone==TEXT("T12"))
            {
                Door.Locked=false;
                Door.InsideOnly=false;
            }
            else
            {
                if(Locks->HasField(Door.Id))Door.Locked=Boolean(Locks,Door.Id);
                if(Inside->HasField(Door.Id))Door.InsideOnly=Boolean(Inside,Door.Id);
            }
            if(Doors->HasField(Door.Id)&&Door.Open!=Boolean(Doors,Door.Id))
            {
                Door.Open=Boolean(Doors,Door.Id);
                BookWorld->RefreshPortalVisual(Door);
            }
        }
    }
    // NewGame initializes the load operation; its chapter-one notice is not
    // part of the restored story and must not cover a later chapter.
    Hero()->NoticeTime=0;
    if(WasAdmin)ToggleAdmin();Show(TEXT(""));
    // A completed operation is a terminal state, not a fresh walking spawn.
    // Derive its presentation from the saved authored outcome; never replay
    // effects, kill the hero again, or modify the chosen branch.
    if(HospitalStory&&HospitalStory->Chapter()>=20&&HospitalStory->NarrativeFlag35(TEXT("EPILOGUE_COMPLETE")))
    {
        const FString Plan=String(Object(HospitalStory->NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"));
        const bool Stayed=Plan==TEXT("DESTROY")&&HospitalStory->NarrativeFlag35(TEXT("HERO_STAYS"));
        if(Stayed||Plan==TEXT("REPLACE")){H->GetCharacterMovement()->StopMovementImmediately();H->GetCharacterMovement()->DisableMovement();}
        if(Stayed)if(auto* PC=Cast<APlayerController>(H->Controller);PC&&PC->PlayerCameraManager)PC->PlayerCameraManager->SetManualCameraFade(1.f,FLinearColor::Black,false);
    }
    return true;
}
bool AEWardCampaignMode::Save(const FString& Slot,bool Automatic)
{
    if (WardOnline80::PlayingMap(*this)) return WardOnline80::SaveCheckpoint(*this, !Automatic);
    if(!PreviewItem51.IsEmpty()||ReloadTime>0||WardCombat63::Healing(*this))return false;
    if((HospitalStory&&HospitalStory->ArrivalCinematic())||(BookWorld&&(BookWorld->LiftMoving||BookWorld->FurnitureHiding))||!SaveSlots().Contains(Slot)||!CampaignStarted||!Hero()||Hero()->Dead||PendingRespawn||Admin||HidingBed||Hero()->GetCharacterMovement()->IsFalling()||Hero()->Held||EscapeTime>=0){if(!Automatic)Notify(TEXT("Сейчас нельзя сохраняться. Отойди от опасности и закончи действие."));return false;}
    Json S=Capture();if(!ValidateSave(S))return false;IFileManager::Get().MakeDirectory(*SaveDirectory(),true);const FString Path=SaveDirectory()/(Slot+TEXT(".json")),Temp=Path+TEXT(".tmp");
    if(!Write(Temp,S)||!ValidateSave(Read(Temp))||!IFileManager::Get().Move(*Path,*Temp,true,true,false,true)){Notify(TEXT("Не удалось записать сохранение."));return false;}
    if(!Automatic)WardAudio68::Play(this,TEXT("UI/save"),.12f*Setting(TEXT("effects"),.8));
    LastSave=Slot;SaveIcon=2.5f;if(!Automatic||Hero()->NoticeTime<=0)Notify(Automatic?TEXT("Контрольная точка сохранена"):TEXT("Игра сохранена"),2);return true;
}
bool AEWardCampaignMode::Load(const FString& Slot){if(Restore(ReadSlot(Slot))){LastSave=Slot;return true;}Notify(TEXT("Сохранение отсутствует или повреждено."));return false;}
bool AEWardCampaignMode::LoadLatest()
{
    TArray<TPair<FString,FString>> Sorted;for(const FString& Slot:SaveSlots()){Json S=ReadSlot(Slot);if(ValidateSave(S))Sorted.Emplace(String(S,TEXT("saved_at")),Slot);}Sorted.Sort([](const auto& A,const auto& B){return A.Key>B.Key;});for(const auto& E:Sorted)if(Load(E.Value))return true;return false;
}
void AEWardCampaignMode::AutoSave(){Flag(TEXT("autosave_pending"),true);}
void AEWardCampaignMode::TickAutoSave(float D)
{
    for(const auto& E:Objects)if(E.Value->ObjectKind==TEXT("save")&&FVector::Dist2D(Hero()->GetActorLocation(),E.Value->Stance)<170&&!AutoVisited.Contains(E.Key)){AutoVisited.Add(E.Key);AutoSave();}
    if(Flag(TEXT("autosave_pending"))&&!Hero()->Held&&!Admin&&!HidingBed&&!PendingRespawn&&EscapeTime<0){Flag(TEXT("autosave_pending"),false);FString Slot;FString Oldest;
        for(int Index=1;Index<=3;++Index)
        {
            const FString Candidate=FString::Printf(TEXT("auto_%d"),Index);
            const Json Existing=ReadSlot(Candidate);
            if(!ValidateSave(Existing)){Slot=Candidate;break;}
            const FString Date=String(Existing,TEXT("saved_at"));
            if(Slot.IsEmpty()||Date<Oldest){Slot=Candidate;Oldest=Date;}
        }
        if(Save(Slot,true))NextAuto=FCString::Atoi(*Slot.Right(1))%3+1;else Flag(TEXT("autosave_pending"),true);}
}
void AEWardCampaignMode::ImportGodotSaves()
{
    if(BookWorldEnabled){Notify(TEXT("Сохранения прежней кампании остаются в прежней версии карты."));return;}
    // Import copies into native slots. Never alter or overwrite the original Godot files.
    const FString Base=FString(FPlatformMisc::GetEnvironmentVariable(TEXT("APPDATA")))/TEXT("Godot/app_userdata/Echo Forgotten Sector/echo_saves");int32 Count=0;
    for(const FString& Slot:SaveSlots())
    {
        if(ValidateSave(ReadSlot(Slot)))continue;Json Old=Read(Base/(Slot+TEXT(".json")));if(Number(Old,TEXT("version"))!=1)continue;
        Json S=Capture();S->SetStringField(TEXT("saved_at"),String(Old,TEXT("saved_at"),FDateTime::UtcNow().ToIso8601()));S->SetStringField(TEXT("difficulty"),String(Old,TEXT("difficulty"),TEXT("normal")));S->SetNumberField(TEXT("elapsed"),Number(Old,TEXT("elapsed")));S->SetNumberField(TEXT("health"),Number(Old,TEXT("health"),100));
        Json Resume=Object(Old,TEXT("resume"));FVector G=Vector(Array(Resume,TEXT("at")));if(Array(Resume,TEXT("at")).Num()!=3)continue;S->SetArrayField(TEXT("at"),V3(FVector(-G.Z,G.X,G.Y)*100+FVector(0,0,96)));
        Json F=MakeShared<FJsonObject>();for(const TCHAR* K:{TEXT("power_on"),TEXT("badge"),TEXT("purge_complete"),TEXT("map")})F->SetBoolField(K,Boolean(Old,K));Json Combat=Object(Old,TEXT("combat"));F->SetBoolField(TEXT("weapon"),Boolean(Combat,TEXT("equipped")));F->SetBoolField(TEXT("shotgun"),Boolean(Combat,TEXT("shotgun")));S->SetObjectField(TEXT("flags"),F);
        S->SetNumberField(TEXT("rounds"),Number(Combat,TEXT("rounds"),9));S->SetNumberField(TEXT("reserve"),Number(Combat,TEXT("reserve"),12));S->SetNumberField(TEXT("loaded"),Number(Combat,TEXT("loaded")));S->SetBoolField(TEXT("shotgun"),Boolean(Combat,TEXT("selected")));
        // Only early saves with a fully understood schema are imported. Later chapter saves
        // require explicit conversion instead of silently creating an inconsistent campaign.
        if(Number(Old,TEXT("stage"))>2){continue;}
        if(ValidateSave(S)&&Write(SaveDirectory()/(Slot+TEXT(".json")),S))++Count;
    }
    Notify(FString::Printf(TEXT("Импортировано ранних сохранений: %d. Исходные файлы Godot сохранены."),Count),8);
}


#include "WardSaveAssessment70.inl"
