#include "WardCampaign.h"
#include "WardSecurity103.h"
#include "WardAdmin79.h"
#include "WardOnline80.h"
#include "WardGameplay76.h"
#include "Camera/PlayerCameraManager.h"
#include "WardAudio68.h"
#include "WardStaging64.h"
#include "WardNotes65.h"
#include "WardInterface66.h"
#include "WardCombat63.h"
#include "WardDialogue62.h"
#include "WardPeople58.h"
#include "WardMonster59.h"
#include "WardPeople54.h"
#include "WardHomeAssets53.h"
#include "WardPeopleAssessment54.h"
#include "WardBookWorld.h"
#include "WardHospitalGameplay.h"
#include "Engine/TextureCube.h"
#include "WardCampaignScene.h"
#include "WardCampaignUI.h"
#include "WardTitleScene.h"
#include "WardGraphics.h"
#include "WardSettings.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Internationalization/Regex.h"

using namespace WardData;
void TickHospitalFacilities37(AEWardCampaignMode* Mode,float Delta);
void AddWardFieldTranslations(TMap<FString,TPair<FString,FString>>& Catalog);
void TickRevision77(AEWardCampaignMode& G,float Delta);
AEWardCampaignMode* WardCampaign(const UObject* Context){return Cast<AEWardCampaignMode>(UGameplayStatics::GetGameMode(Context));}
AEWardCampaignMode::AEWardCampaignMode(){DefaultPawnClass=AEWardCampaignHero::StaticClass();HUDClass=AEWardCampaignHUD::StaticClass();PrimaryActorTick.bTickEvenWhenPaused=true;}
AEWardCampaignHero* AEWardCampaignMode::Hero()const{return Cast<AEWardCampaignHero>(UGameplayStatics::GetPlayerCharacter(this,0));}
bool AEWardCampaignMode::Flag(const FString& Key)const{return Boolean(Flags,Key);}
void AEWardCampaignMode::Flag(const FString& Key,bool Value){Flags->SetBoolField(Key,Value);}
double AEWardCampaignMode::NumberFlag(const FString& Key,double Default)const{return Number(Flags,Key,Default);}
void AEWardCampaignMode::NumberFlag(const FString& Key,double Value){Flags->SetNumberField(Key,Value);}
FString AEWardCampaignMode::StringFlag(const FString& Key)const{return String(Flags,Key);}
void AEWardCampaignMode::StringFlag(const FString& Key,const FString& Value){Flags->SetStringField(Key,Value);}
double AEWardCampaignMode::Setting(const FString& Key,double Default)const{return Number(Options,Key,Default);}
bool AEWardCampaignMode::SettingBool(const FString& Key,bool Default)const{return Key==TEXT("inspection_bright")?Admin&&Boolean(Options,Key,Default):Boolean(Options,Key,Default);}
float AEWardCampaignMode::DifficultyValue(const FString& Key)const{return Number(Object(Object(Data,TEXT("difficulty")),Difficulty),Key,1);}
FString AEWardCampaignMode::Tr(const FString& Russian)const
{
    if(Language==TEXT("ru")||Russian.IsEmpty())return DisplayNames36(Russian);
    // Cache each locale separately, including direct language changes by assessments.
    const FString CacheKey=Language+TEXT("|")+Russian;
    if(const FString* Cached=TranslationCache.Find(CacheKey))return *Cached;
    FString Result=Russian;
    if(const auto* Match=Translations.Find(Russian))Result=Language==TEXT("en")?Match->Key:Match->Value;
    else
    {
        // Match formatted catalog entries after values have been inserted into prompts.
        for(const auto& Entry:Translations)
        {
            const FString& Format=Entry.Key;if(!Format.Contains(TEXT("%")))continue;
            FString Pattern=TEXT("^");int32 Groups=0;
            for(int32 I=0;I<Format.Len();++I)
            {
                if(Format[I]=='%'&&I+1<Format.Len())
                {
                    if(Format[I+1]=='%'){Pattern+=TEXT("%");++I;continue;}
                    int32 End=I+1;while(End<Format.Len()&&(FChar::IsDigit(Format[End])||Format[End]=='.'||Format[End]=='-'||Format[End]=='+'))++End;
                    if(End<Format.Len()&&FString(TEXT("dsfg")).Contains(FString::Chr(Format[End]))){Pattern+=Format[End]=='s'?TEXT("(.+?)"):TEXT("(-?[0-9]+(?:\\.[0-9]+)?)");++Groups;I=End;continue;}
                }
                const TCHAR C=Format[I];if(FString(TEXT(".^$|()[]{}*+?\\")).Contains(FString::Chr(C)))Pattern+=TEXT("\\");Pattern+=FString::Chr(C);
            }
            Pattern+=TEXT("$");FRegexMatcher Matcher(FRegexPattern(Pattern),Russian);if(Groups==0||!Matcher.FindNext())continue;
            const FString Target=Language==TEXT("en")?Entry.Value.Key:Entry.Value.Value;FString Translated;int32 Group=1;
            for(int32 I=0;I<Target.Len();++I){if(Target[I]=='%'&&I+1<Target.Len()){if(Target[I+1]=='%'){Translated+=TEXT("%");++I;continue;}int32 End=I+1;while(End<Target.Len()&&(FChar::IsDigit(Target[End])||Target[End]=='.'||Target[End]=='-'||Target[End]=='+'))++End;if(End<Target.Len()&&FString(TEXT("dsfg")).Contains(FString::Chr(Target[End]))){Translated+=Matcher.GetCaptureGroup(Group++);I=End;continue;}}Translated+=FString::Chr(Target[I]);}
            Translated=DisplayNames36(Translated);TranslationCache.Add(CacheKey,Translated);return Translated;
        }
        // Only static fragments are replaced; cache avoids scanning the catalog per frame.
        TArray<FString> Keys;Translations.GetKeys(Keys);Keys.Sort([](const FString& A,const FString& B){return A.Len()>B.Len();});
        for(const FString& Key:Keys)if(Key.Len()>=5 && !Key.Contains(TEXT("%")) && Result.Contains(Key))
            Result.ReplaceInline(*Key,*(Language==TEXT("en")?Translations[Key].Key:Translations[Key].Value),ESearchCase::CaseSensitive);
    }
    Result=DisplayNames36(Result);if(TranslationCache.Num()>2048)TranslationCache.Reset();TranslationCache.Add(CacheKey,Result);return Result;
}
void AEWardCampaignMode::BeginPlay()
{
    AGameModeBase::BeginPlay();BookWorldEnabled=!FParse::Param(FCommandLine::Get(),TEXT("WardLegacyCampaign"));Data=Campaign();Flags=MakeShared<FJsonObject>();Options=MakeShared<FJsonObject>();Options->Values=Object(Data,TEXT("settings"))->Values;
    WardSettings::Complete(Options);
    const auto Training76=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Gameplay76/training.json"));if(!Array(Training76,TEXT("pages")).IsEmpty())Data->SetArrayField(TEXT("training"),Array(Training76,TEXT("pages")));
    Json SavedOptions=Read(FPaths::ProjectSavedDir()/TEXT("Config/CampaignOptions.json"));
    for(const auto& Entry:SavedOptions->Values)if(Options->HasField(Entry.Key))Options->SetField(Entry.Key,Entry.Value);
    // Migrate the old always-bright map-inspection default once. Future B
    // toggles/settings are still saved and respected.
    if(BookWorldEnabled&&Number(SavedOptions,TEXT("atmosphere_profile"))<19){Options->SetBoolField(TEXT("inspection_bright"),false);Options->SetNumberField(TEXT("atmosphere_profile"),19);}
    if(BookWorldEnabled&&Number(SavedOptions,TEXT("quality_revision"))<22){Options->SetNumberField(TEXT("msaa"),3);Options->SetNumberField(TEXT("screen_percentage"),100);Options->SetNumberField(TEXT("quality_revision"),22);}
    // A clean beta install starts with a conservative 1080p-output / 4 GB GPU profile.
    // Existing players retain their own graphics and frame-limit choices.
    if(!SavedOptions->HasField(TEXT("graphics_preset"))){Options->SetNumberField(TEXT("graphics_preset"),2);Options->SetNumberField(TEXT("screen_percentage"),67);Options->SetStringField(TEXT("shadow_quality"),TEXT("medium"));Options->SetNumberField(TEXT("msaa"),3);}
    if(!SavedOptions->HasField(TEXT("max_fps")))Options->SetNumberField(TEXT("max_fps"),Setting(TEXT("graphics_preset"),2)==2?30:60);
    Language=String(Options,TEXT("language"),TEXT("ru"));if(!TArray<FString>{TEXT("ru"),TEXT("kk"),TEXT("en")}.Contains(Language))Language=TEXT("ru");
    TArray<FString> Lines;String(Data,TEXT("locale_tsv")).ParseIntoArrayLines(Lines);
    FString NativeLocale;TArray<FString> Extra;if(FFileHelper::LoadFileToString(NativeLocale,*(FPaths::ProjectContentDir()/TEXT("CampaignData/native_locale.tsv")))){NativeLocale.ParseIntoArrayLines(Extra);Lines.Append(Extra);}
    for(const FString& Line:Lines){TArray<FString> Columns;Line.ParseIntoArray(Columns,TEXT("\t"),false);if(Columns.Num()>=3)Translations.Add(Columns[0],TPair<FString,FString>(Columns[1],Columns[2]));}
    WardSettings::AddTranslations(Translations);
    AddWardFieldTranslations(Translations);
    for(const auto& V:Array(Data,TEXT("instances"))){Json Row=V->AsObject();const FString OwnerId=String(Row,TEXT("owner"));if(!OwnerId.IsEmpty())Visuals.FindOrAdd(OwnerId).Add(Row);}
    for(const auto& V:Array(Data,TEXT("collisions"))){Json Row=V->AsObject();const FString OwnerId=String(Row,TEXT("owner"));if(!OwnerId.IsEmpty())Colliders.FindOrAdd(OwnerId).Add(Row);}
    Grid.Load(Data);
    SpatialSound=NewObject<USoundAttenuation>(this);SpatialSound->Attenuation.bAttenuate=true;SpatialSound->Attenuation.bSpatialize=true;
    SpatialSound->Attenuation.AttenuationShapeExtents=FVector(180,0,0);SpatialSound->Attenuation.FalloffDistance=2200;
    SpatialSound->Attenuation.bEnableOcclusion=true;SpatialSound->Attenuation.OcclusionTraceChannel=ECC_Visibility;
    for(const auto& V:Array(Data,TEXT("audio")))
    {
        const FString Id=String(V->AsObject(),TEXT("id"));const FString Path=TEXT("/Game/Campaign/Audio/A_")+Id+TEXT(".A_")+Id;
        if(USoundBase* S=LoadObject<USoundBase>(nullptr,*Path))Effects.Add(FName(*Id),S);
    }
    if(!BookWorldEnabled)for(const auto& V:Array(Data,TEXT("objects")))
    {
        Json Row=V->AsObject();FTransform At=Transform(Object(Row,TEXT("transform")));
        auto* Item=GetWorld()->SpawnActorDeferred<AEWardCampaignObject>(AEWardCampaignObject::StaticClass(),At);
        Item->ObjectId=String(Row,TEXT("id"));Item->ObjectKind=String(Row,TEXT("kind"));Item->Descriptor=WardData::Serialize(Row);
        Objects.Add(Item->ObjectId,Item);UGameplayStatics::FinishSpawningActor(Item,At);
    }
    if(!BookWorldEnabled)for(const auto& V:Array(Data,TEXT("enemies")))
    {
        Json Row=V->AsObject();FTransform At=Transform(Object(Row,TEXT("transform")));At.AddToTranslation(FVector(0,0,96));
        auto* Enemy=GetWorld()->SpawnActorDeferred<AEWardCampaignEnemy>(AEWardCampaignEnemy::StaticClass(),At);
        Enemy->EnemyId=String(Row,TEXT("id"));Enemy->Definition=Row;Enemies.Add(Enemy);UGameplayStatics::FinishSpawningActor(Enemy,At);
    }
    if(BookWorldEnabled)
    {
        for(TActorIterator<AEWardCampaignScene> It(GetWorld());It;++It)It->Destroy();
        for(TActorIterator<ALight> It(GetWorld());It;++It)It->Destroy();
        BookWorld=GetWorld()->SpawnActor<AEWardBookWorld>();BookWorld->Build(this);
        if(!FParse::Param(FCommandLine::Get(),TEXT("WardBookWorldCheck"))&&!FParse::Param(FCommandLine::Get(),TEXT("WardInspection")))
        {HospitalStory=GetWorld()->SpawnActor<AEWardHospitalGameplay>();HospitalStory->Build(this);}
    }
    Interface=MakeShared<FWardCampaignUI>(this);WardAdmin79::Install(*this);
    Assessment=FParse::Param(FCommandLine::Get(),TEXT("WardMenuCheck"))||FParse::Param(FCommandLine::Get(),TEXT("WardCampaignBenchmark"))||FParse::Param(FCommandLine::Get(),TEXT("WardCampaignSmoke"))||FParse::Param(FCommandLine::Get(),TEXT("WardCampaignAgent"))||FParse::Param(FCommandLine::Get(),TEXT("WardCampaignCapture"))||FParse::Param(FCommandLine::Get(),TEXT("WardBenchmark"))||FParse::Param(FCommandLine::Get(),TEXT("WardPhysicsCheck"));
    Assessment|=FParse::Param(FCommandLine::Get(),TEXT("WardLocomotionCheck52"))||FParse::Param(FCommandLine::Get(),TEXT("WardBodyPreviewCheck51"))||FParse::Param(FCommandLine::Get(),TEXT("WardFacilitiesCheck37"))||FParse::Param(FCommandLine::Get(),TEXT("WardFirstPersonCheck50"));
    Assessment|=FParse::Param(FCommandLine::Get(),TEXT("WardArrivalCheck"))||FParse::Param(FCommandLine::Get(),TEXT("WardScenarioCheck"));
    Assessment|=FParse::Param(FCommandLine::Get(),TEXT("WardAudioCheck68"))||FParse::Param(FCommandLine::Get(),TEXT("WardAdminCheck79"));
    Assessment|=FParse::Param(FCommandLine::Get(),TEXT("WardBookWorldCheck"))||FParse::Param(FCommandLine::Get(),TEXT("WardStoryCheck"))||FParse::Param(FCommandLine::Get(),TEXT("WardHospitalBenchmark"));NewGame(TEXT("normal"));CampaignStarted=false;
    if(Assessment){CampaignStarted=true;Screen.Empty();}else{EWardGraphics::Load();ApplySettings(false);Show(TEXT("title"));}
    if(auto* S=Effects.Find(TEXT("score")))Music=UGameplayStatics::SpawnSound2D(this,S->Get(),TitleView?.001f:.25f,1,0,nullptr,false,false);
    if(auto* S=Effects.Find(TEXT("ambience")))Ambience=UGameplayStatics::SpawnSound2D(this,S->Get(),TitleView?.001f:.15f,1,0,nullptr,false,false);
    if(auto* S=Effects.Find(TEXT("pursuit")))Pursuit=UGameplayStatics::SpawnSound2D(this,S->Get(),.001f,1,0,nullptr,false,false);
    if(auto* S=Effects.Find(TEXT("refuge_music")))RefugeMusic=UGameplayStatics::SpawnSound2D(this,S->Get(),.001f,1,0,nullptr,false,false);
    WardOnline80::StartAssessment(*this);
    UE_LOG(LogTemp,Display,TEXT("WARD_CAMPAIGN_READY %d objects %d enemies %d navigation cells"),Objects.Num(),Enemies.Num(),Grid.Ground.Num());
}
void AEWardCampaignMode::Notify(const FString& Text,float Seconds){if(Hero())Hero()->Announce(Tr(Text),Seconds);}
void AEWardCampaignMode::EndPlay(const EEndPlayReason::Type Reason){WardOnline80::Shutdown(*this);WardAdmin79::Uninstall(*this);WardAudio68::Stop(*this);if(Interface)Interface->Close();for(auto C:{Music,Ambience,Pursuit,Voice,RefugeMusic})if(C)C->Stop();Super::EndPlay(Reason);}
void AEWardCampaignMode::Cue(const FString& Id,FVector At,float Volume)
{
    if(WardAudio68::Cue(*this,Id,At,Volume))return;
    static const TMap<FString,FString> Aliases={{TEXT("heal"),TEXT("breath")},{TEXT("hit"),TEXT("flesh_hit")},{TEXT("knife"),TEXT("claw")},{TEXT("bottle"),TEXT("glass_tinkle")},{TEXT("barricade"),TEXT("wood_hit")},{TEXT("cloth"),TEXT("scrape")},{TEXT("creature_hit"),TEXT("bone_hit")},{TEXT("creature_death"),TEXT("growl")}};
    const FString Key=Aliases.Contains(Id)?Aliases[Id]:Id;if(auto* Sound=Effects.Find(FName(*Key))){if(Id.StartsWith(TEXT("story_"))){if(Voice)Voice->Stop();Voice=UGameplayStatics::SpawnSound2D(this,Sound->Get(),Volume*Setting(TEXT("effects"),.8));StringFlag(TEXT("last_story"),Id);}else UGameplayStatics::PlaySoundAtLocation(this,Sound->Get(),At,Volume*Setting(TEXT("effects"),.8),1,0,SpatialSound);}
}
void AEWardCampaignMode::BroadcastNoise(const FVector& At,float Radius)
{
    Noise=FMath::Max(Noise,FMath::Clamp(Radius/1400.f,0.f,1.f));
    if(Admin&&!Agent)return;
    for(auto Enemy:Enemies)if(Enemy->Active&&!Enemy->Dead)Enemy->HearNoise(At,Radius);
    if(HospitalStory)HospitalStory->Hear(At,Radius);
}
void AEWardCampaignMode::Show(const FString& Page)
{
    if(!Interface)return;
    WardAudio68::Page(*this,Screen,Page);
    if(Page==TEXT("settings")&&Screen!=Page)Interface->SettingsReturn66=Screen;
    if(HospitalStory&&!WardOnline80::Guest(*this)&&((!Page.IsEmpty()&&Page!=TEXT("story35"))||(Page.IsEmpty()&&Screen==TEXT("story35"))))WardDialogue62::Stop(*HospitalStory,true);
    const bool ReturnToTitle=Page==TEXT("title")&&Screen!=Page;
    const bool OpenPerson = Page == TEXT("story35") && Screen != Page && HospitalStory &&
        HospitalStory->ActiveScene35.StartsWith(TEXT("NPC_")) && HospitalStory->DialoguePage35 == 0;
    if (Screen == TEXT("story35") && Page != Screen && IsValid(Voice)) Voice->Stop();
    if(Page!=Screen&&(Screen==TEXT("inventory")||Screen==TEXT("journal")||Screen==TEXT("map"))){Interface->CombineItem.Empty();Interface->FieldFeedback.Empty();}
    if(Screen==TEXT("settings")&&Page!=Screen)ApplySettings();if(Page==TEXT("saves")&&Screen!=Page)Interface->SaveFeedback.Empty();Screen=Page;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC)return;
    if(Page==TEXT("title"))TitleView=true;else if(Page.IsEmpty()||Page==TEXT("pause"))TitleView=false;
    // The terminal death fade belongs to the game view, not the pause/title
    // backdrop or document reader. Restore it only when leaving those pages.
    if(HospitalStory&&HospitalStory->Chapter()>=20&&HospitalStory->NarrativeFlag35(TEXT("EPILOGUE_COMPLETE"))&&
       HospitalStory->NarrativeFlag35(TEXT("HERO_STAYS"))&&
       String(Object(HospitalStory->NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"))==TEXT("DESTROY")&&PC->PlayerCameraManager)
    {
        if(!Page.IsEmpty())PC->PlayerCameraManager->StopCameraFade();
        else if(HospitalStory->ActiveScene35.IsEmpty())PC->PlayerCameraManager->SetManualCameraFade(1.f,FLinearColor::Black,false);
    }
    const bool Presentation=!WardOnline80::PlayingMap(*this)&&(TitleView||Page==TEXT("pause")||Page==TEXT("game")||Page==TEXT("difficulty")||Page==TEXT("saves")||Page==TEXT("about")||Page==TEXT("training")||Page==TEXT("settings"));
    if(Presentation&&!TitleScene){TitleScene=GetWorld()->SpawnActor<AEWardTitleScene>();TitleScene->Build(this);}
    if(TitleScene){if(ReturnToTitle&&TitleScene->Active)TitleScene->Activate(false);TitleScene->Activate(Presentation);TitleScene->SetGameBackdrop(TitleView&&Page==TEXT("game"));TitleScene->SetSaveBackdrop(Page==TEXT("saves")||Page==TEXT("difficulty"));TitleScene->SetAboutBackdrop(Page==TEXT("about")||Page==TEXT("pause")||(!TitleView&&Page==TEXT("game")));TitleScene->SetTrainingBackdrop(Page==TEXT("training")||Page==TEXT("settings"));}
    if(Hero()){Hero()->SetActorHiddenInGame(Presentation);}
    if(Page.IsEmpty()&&Admin&&AdminCamera)PC->SetViewTarget(AdminHands51?static_cast<AActor*>(Hero()):static_cast<AActor*>(AdminCamera.Get()));
    if(Presentation)for(auto C:{Music,Ambience,Pursuit,Voice,RefugeMusic})if(C)C->SetVolumeMultiplier(0);
    const bool Pause=!Screen.IsEmpty()&&Screen!=TEXT("story35")&&Screen!=WardSecurity103::Page&&!WardOnline80::PlayingMap(*this);PC->SetPause(Pause);PC->bShowMouseCursor=!Screen.IsEmpty();
    if(!Screen.IsEmpty()){PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));Interface->Open();}
    else{Interface->Close();PC->SetInputMode(FInputModeGameOnly());}
    if(Hero()){Hero()->StopInteract();Hero()->Aiming=false;Hero()->SprintHeld=false;Hero()->GetCharacterMovement()->StopMovementImmediately();}
    if (OpenPerson && !HospitalStory->NarrativeEnabled35)
    {
        if (IsValid(Voice)) Voice->Stop();
        const FString VoiceFile = String(HospitalStory->NarrativeState35,TEXT("aux_voice57"),
            UEWardPerson54::VoicePath55(HospitalStory->ActiveScene35.RightChop(4)));
        Voice = WardHomeAssets::PlayDialogueFile(this, VoiceFile,
            .88f*Setting(TEXT("effects"),.8f));
    }
}
void AEWardCampaignMode::NewGame(const FString& Id)
{
    WardSecurity103::Reset(*this);
    WardAdmin79::ResetWorld(*this);
    WardGameplay76::Reset(*this);
    WardAudio68::Stop(*this);
    // A new campaign (also used as the base of Load) must clear the
    // held fade from the previous terminal scene.
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0);PC&&PC->PlayerCameraManager)PC->PlayerCameraManager->StopCameraFade();
    ClearPreview51();AdminHands51=false;Noise=0;Breath=3;DashTime=0;
    if(Hero()){Hero()->MotionInitialized52=false;Hero()->StrideTravel52=0;Hero()->ViewMotion52=FVector::ZeroVector;Hero()->WeaponMotion52=FVector::ZeroVector;Hero()->WeaponSway52=FRotator::ZeroRotator;}
    ClearPhysics();if(Hero()){Hero()->WeaponKick=0;Hero()->WeaponAimBlend=0;Hero()->WeaponCloseBlend=0;Hero()->WeaponObstructed=false;Hero()->MuzzleRemaining=0;}
    Difficulty=TArray<FString>{TEXT("story"),TEXT("normal"),TEXT("nightmare"),TEXT("hardcore")}.Contains(Id)?Id:TEXT("normal");
    if(Interface){Interface->Treatment.Reset();Interface->PendingCode.Empty();Interface->FilmTurn=1;Interface->FilmOffset=FVector2D(72,-48);Interface->SelectedItem=TEXT("badge");Interface->InventorySlot=0;Interface->DocumentIndex=0;Interface->CombineItem.Empty();Interface->FieldFeedback.Empty();}
    Flags=MakeShared<FJsonObject>();Completed.Reset();Visited.Reset();AutoVisited.Reset();Values.Reset();Items.Reset();Stash.Reset();Journal.Reset();
    Items.Add(TEXT("bottle"),3);Score=0;Shots=0;Hits=0;Kills=0;LoadedShells=0;Shotgun=false;ReloadTime=0;ShotCooldown=0;EscapeTime=-1;ReactorTime=45;GeneratorTime=0;Elapsed=0;Fault.Empty();HidingBed=nullptr;Won=false;PowerOn=false;PendingRespawn=false;Invulnerable=2;AgentIndex=0;AgentPath.Empty();
    TArray<FString> Loose;for(auto& Entry:Objects)if(Entry.Value->ObjectKind==TEXT("dropped"))Loose.Add(Entry.Key);for(const FString& Id:Loose){Objects[Id]->Destroy();Objects.Remove(Id);Visuals.Remove(Id);}
    for(auto& Entry:Objects)Entry.Value->ResetState();for(auto Enemy:Enemies)Enemy->ResetEnemy();FlashCharges=1;
    if(auto* H=Hero())
    {
        H->StopInteract();H->Focus=nullptr;H->SprintHeld=false;H->SprintExhausted=false;H->UnCrouch();H->Dead=false;H->Health=100;H->Stamina=100;H->Rounds=9;H->Reserve=12;H->Aiming=false;H->HasFuse=false;
        H->GetMesh()->SetSimulatePhysics(false);H->GetMesh()->AttachToComponent(H->GetCapsuleComponent(),FAttachmentTransformRules::KeepRelativeTransform);H->GetMesh()->SetRelativeTransform(H->MeshRest);
        H->GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));H->GetMesh()->SetHiddenInGame(true);H->UpdateFirstPerson50();
        H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        H->SetActorLocation(Vector(Array(Data,TEXT("start")))+FVector(0,0,96),false,nullptr,ETeleportType::TeleportPhysics);H->GetCharacterMovement()->StopMovementImmediately();
        H->Pistol->SetVisibility(false);if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetControlRotation(FRotator(-8,0,0));
    }
    if(BookWorld)
    {
          Items.Reset();FlashCharges=0;Flag(TEXT("map"),true);CampaignStarted=true;
        if(auto* H=Hero()){H->Rounds=0;H->Reserve=0;H->Flashlight->SetVisibility(true);H->SetActorLocation(BookWorld->Start(),false,nullptr,ETeleportType::TeleportPhysics);if(auto* PC=Cast<APlayerController>(H->Controller))PC->SetControlRotation(FRotator(-8,90,0));}
        BookWorld->ResetMovables();BookWorld->SetStoryChapter(0);CurrentRoom=BookWorld->Describe(BookWorld->Start());BookWorld->UpdateLighting(BookWorld->Start());
        if(HospitalStory){HospitalStory->ResetStory();Notify(TEXT("Глава 1. Больница № 333. F — взаимодействие · J — документы · M — карта · N — фонарь · B — яркий осмотр"),8);}
        else Notify(TEXT("Весь комплекс открыт. WASD — идти · Shift — бег · M — план · B — яркий свет · F5 — сохранить"),7);return;
    }
    AllocateResources();SyncDoors();CampaignStarted=true;Notify(TEXT("Ты заперт в отделении 04. Найди план и оружие в приёмной."),7);
}
void AEWardCampaignMode::ApplySettings(bool Persist)
{
    WardSettings::Complete(Options);
    Language=String(Options,TEXT("language"),TEXT("ru"));TranslationCache.Reset();
    EWardGraphics::Apply(FMath::Clamp(int32(Setting(TEXT("graphics_preset"),1)),0,2),false);
    if(Hero()){Hero()->Camera->SetFieldOfView(FMath::Clamp((float)Setting(TEXT("fov"),70),55.f,100.f));Hero()->Flashlight->SetIntensity(3500*Setting(TEXT("flashlight_strength"),1));if(BookWorldEnabled){auto& P=Hero()->Camera->PostProcessSettings;P.bOverride_AutoExposureBias=true;P.AutoExposureBias=(SettingBool(TEXT("inspection_bright"),false)?1.f:.25f)+FMath::Log2(FMath::Max(.1,double(Setting(TEXT("brightness")))));P.bOverride_DynamicGlobalIlluminationMethod=true;P.DynamicGlobalIlluminationMethod=EDynamicGlobalIlluminationMethod::None;P.bOverride_ReflectionMethod=true;P.ReflectionMethod=EReflectionMethod::ScreenSpace;P.bOverride_AmbientCubemapIntensity=true;P.AmbientCubemapIntensity=SettingBool(TEXT("inspection_bright"),false)?1.4f:.13f;P.AmbientCubemap=LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"));}}
    if(BookWorld){
        if(auto* H=Hero()){H->Flashlight->SetIntensityUnits(ELightUnits::Lumens);H->Flashlight->SetIntensity(2200*Setting(TEXT("flashlight_strength"),1));H->Flashlight->SetInnerConeAngle(19);H->Flashlight->SetOuterConeAngle(42);H->Flashlight->SetAttenuationRadius(2000);H->Flashlight->SetLightingChannels(true,false,false);H->UpdateTorchTransform();}
        BookWorld->SetBright(SettingBool(TEXT("inspection_bright"),false));
    }
    if(Hero())Hero()->Arm->bEnableCameraLag=SettingBool(TEXT("camera_motion"));
    for(TActorIterator<ALight> It(GetWorld());It;++It)if(auto* Light=It->GetLightComponent()){if(!LightLevels.Contains(Light))LightLevels.Add(Light,Light->Intensity);Light->SetIntensity(LightLevels[Light]*Setting(TEXT("ambient")));}
    for(TActorIterator<ASkyLight> It(GetWorld());It;++It)It->GetLightComponent()->SetIntensity(.12f*Setting(TEXT("ambient")));
    if(auto* Settings=GEngine->GetGameUserSettings())
    {
        Settings->SetVSyncEnabled(SettingBool(TEXT("vsync"),true));Settings->SetFrameRateLimit(Setting(TEXT("max_fps"),60));
        FString Width,Height;String(Options,TEXT("resolution"),TEXT("1920x1080")).Split(TEXT("x"),&Width,&Height);const FIntPoint R(FCString::Atoi(*Width),FCString::Atoi(*Height));
        if(R.X>=640&&R.Y>=480)Settings->SetScreenResolution(R);Settings->SetResolutionScaleValueEx(FMath::Clamp(float(Setting(TEXT("screen_percentage"),100)),50.f,100.f));
        // Borderless keeps the desktop display mode stable for task switching and capture overlays.
        if(auto* Fullscreen=IConsoleManager::Get().FindConsoleVariable(TEXT("r.FullScreenMode")))Fullscreen->Set(1,ECVF_SetByCode);
        Settings->SetFullscreenMode(SettingBool(TEXT("fullscreen"))?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
        // Borderless uses the desktop's physical size; windowed mode uses the requested size.
        const bool ChangeWindow=Settings->IsFullscreenModeDirty()||(!SettingBool(TEXT("fullscreen"))&&Settings->IsScreenResolutionDirty());
        if(FParse::Param(FCommandLine::Get(),TEXT("RenderOffscreen"))||!ChangeWindow)Settings->ApplyNonResolutionSettings();else {Settings->ApplySettings(false);Settings->ConfirmVideoMode();}if(Persist)Settings->SaveSettings();
    }
    auto Set=[](const TCHAR* Id,float Value){if(auto* C=IConsoleManager::Get().FindConsoleVariable(Id))C->Set(Value,ECVF_SetByCode);};
    const FString Shadow=String(Options,TEXT("shadow_quality"),TEXT("high"));Set(TEXT("sg.ShadowQuality"),Shadow==TEXT("off")?0:Shadow==TEXT("medium")?1:EWardGraphics::Current()==0?3:2);
    Set(TEXT("r.MaxAnisotropy"),16);Set(TEXT("r.Streaming.MipBias"),String(Options,TEXT("texture_quality"))==TEXT("standard")?1:0);
    Set(TEXT("r.Tonemapper.Sharpen"),EWardGraphics::Current()==2?.3f:.18f);
    const int32 AA=FMath::Clamp((int32)Setting(TEXT("msaa"),3),0,3);Set(TEXT("r.AntiAliasingMethod"),AA==3?4:AA);
    for(TActorIterator<APostProcessVolume> It(GetWorld());It;++It)
    {
        auto& S=It->Settings;S.bOverride_AutoExposureBias=true;S.AutoExposureBias=FMath::Log2(FMath::Max(.1,(double)Setting(TEXT("brightness"))));
        S.bOverride_AmbientCubemapIntensity=true;S.AmbientCubemapIntensity=.36f*Setting(TEXT("ambient"));
        S.bOverride_ColorContrast=true;S.ColorContrast=FVector4(1.04*Setting(TEXT("contrast")),1.04*Setting(TEXT("contrast")),1.04*Setting(TEXT("contrast")),1);
        S.bOverride_VignetteIntensity=true;S.VignetteIntensity=SettingBool(TEXT("vignette"))?.25f:0;
        S.bOverride_FilmGrainIntensity=true;S.FilmGrainIntensity=SettingBool(TEXT("grain"))?Setting(TEXT("grain_amount"),.035):0;
        S.bOverride_MotionBlurAmount=true;S.MotionBlurAmount=SettingBool(TEXT("motion_blur"),false)?.2f:0;
    }
    if(TitleScene)for(auto View:{TitleScene->Camera,TitleScene->GameCamera,TitleScene->SaveCamera,TitleScene->AboutCamera,TitleScene->TrainingCamera})if(View){auto& S=View->GetCameraComponent()->PostProcessSettings;S.bOverride_AutoExposureBias=true;S.AutoExposureBias=FMath::Log2(FMath::Max(.1,double(Setting(TEXT("brightness")))));S.bOverride_ColorContrast=true;const float C=Setting(TEXT("contrast"));S.ColorContrast=FVector4(C,C,C,1);S.bOverride_FilmGrainIntensity=true;S.FilmGrainIntensity=SettingBool(TEXT("grain"))?Setting(TEXT("grain_amount"),.035):0;S.bOverride_VignetteIntensity=true;S.VignetteIntensity=SettingBool(TEXT("vignette"))?.25f:0;}
    for(TActorIterator<AExponentialHeightFog> It(GetWorld());It;++It)It->GetComponent()->SetFogDensity(.006f*Setting(TEXT("fog")));
    if(Music)Music->SetVolumeMultiplier(TitleView?0:(BookWorldEnabled?.025f:.3f)*Setting(TEXT("music"),.7));if(Ambience)Ambience->SetVolumeMultiplier(TitleView?0:.18f*Setting(TEXT("music"),.7));
    if(Persist)Write(FPaths::ProjectSavedDir()/TEXT("Config/CampaignOptions.json"),Options);
}
#include "WardPlaythrough71.inl"
void WardSaveCheck70(AEWardCampaignMode& Mode,float Delta);
void TickCapture78(AEWardCampaignMode&,float);
void TickMediaCheck78(AEWardCampaignMode&,float);
void AEWardCampaignMode::Tick(float Delta)
{
    AGameModeBase::Tick(Delta);
    static const bool SecurityTest=FParse::Param(FCommandLine::Get(),TEXT("WardSecurityCheck103"));
    if(SecurityTest){WardSecurity103::Assess(*this,Delta);return;}
    if(!Assessment)WardSecurity103::Tick(*this,Delta);
    if (WardOnline80::PlayingMap(*this))
    {
        WardAudio68::Tick(*this, Delta);
        if (!WardOnline80::Guest(*this)) WardGameplay76::Tick(*this, Delta);
        if (BookWorld && Hero())
        {
            BookWorld->Update(Delta);
            TickHero(Delta);
            Invulnerable = FMath::Max(0.f, Invulnerable - Delta);
            Noise = FMath::Max(0.f, Noise - Delta * .32f);
            CurrentRoom = BookWorld->Describe(Hero()->GetActorLocation());
            BookWorld->UpdateLighting(Hero()->GetActorLocation());
            if (PendingRespawn && (RespawnTime -= Delta) <= 0)
            {
                auto* H = Hero();
                PendingRespawn = false;
                H->Dead = false;
                H->Health = 60;
                H->GetMesh()->SetSimulatePhysics(false);
                H->GetMesh()->AttachToComponent(H->GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
                H->GetMesh()->SetRelativeTransform(H->MeshRest);
                H->GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
                H->GetMesh()->SetHiddenInGame(true);
                H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
                const auto At = WardOnline80::RespawnPoint(*this);
                H->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
                Invulnerable = 2.f;
            }
        }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WardAdminCheck79"))){WardAdmin79::Assess(*this);return;}
    WardAdmin79::Tick(*this,Delta);
    if(FParse::Param(FCommandLine::Get(),TEXT("WardCinematicCapture78"))){TickCapture78(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardMediaCheck78"))){TickMediaCheck78(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardRevisionCheck77"))){TickRevision77(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardGameplayCheck76"))){WardGameplay76::Assess(*this,Delta);return;}
    WardGameplay76::Tick(*this,Delta);
    static const bool Playthrough71=FParse::Param(FCommandLine::Get(),TEXT("WardPlaythrough71"));
    if(Playthrough71)WardPlaythrough71::Tick(*this);
    if(FParse::Param(FCommandLine::Get(),TEXT("WardSaveCheck70"))){WardSaveCheck70(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardAudioCheck68"))){WardAudio68::Assess(*this,Delta);return;}
    WardAudio68::Tick(*this,Delta);
    if(FParse::Param(FCommandLine::Get(),TEXT("WardLocomotionCheck52")))return;
    if(FParse::Param(FCommandLine::Get(),TEXT("WardBodyPreviewCheck51"))){TickBodyPreviewAssessment51(Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardFirstPersonCheck50"))){TickFirstPersonAssessment50(Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardFacilitiesCheck37"))){TickHospitalFacilities37(this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardInterfaceCheck66"))){WardInterface66::Assess(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardNotesCheck65"))){WardNotes65::Assess(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardStagingCheck64"))){WardStaging64::Assess(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardCombatCheck63"))){WardCombat63::Assess(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardDialogueCheck62"))){WardDialogue62::Assess(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardMonstersCheck59"))){WardMonster59::Assess(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardHumansCheck58"))){AssessHumans58(*this,Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardPeopleCheck54"))){AssessPeople54(*this, Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardScenarioCheck"))){if(HospitalStory)HospitalStory->AssessNarrative35(Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardArrivalCheck"))){if(HospitalStory)HospitalStory->AssessArrival(Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardHospitalBenchmark"))){TickHospitalBenchmark(Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardStoryCheck"))){if(HospitalStory)HospitalStory->Assess(Delta);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("WardBookWorldCheck"))){TickBookWorldAssessment(Delta);return;}
    if(Assessment){if(FParse::Param(FCommandLine::Get(),TEXT("WardPhysicsCheck"))){TickPhysicsAssessment(Delta);return;}if(FParse::Param(FCommandLine::Get(),TEXT("WardCampaignBenchmark")))TickCampaignBenchmark(Delta);else if(FParse::Param(FCommandLine::Get(),TEXT("WardBenchmark")))TickAssessment(Delta);else TickCampaignAssessment(Delta);if(FParse::Param(FCommandLine::Get(),TEXT("WardBookMapCheck"))||FParse::Param(FCommandLine::Get(),TEXT("WardFieldMenuCheck"))||FParse::Param(FCommandLine::Get(),TEXT("WardCampaignCapture"))||FParse::Param(FCommandLine::Get(),TEXT("WardCampaignBenchmark"))||FParse::Param(FCommandLine::Get(),TEXT("WardHUDCheck")))return;}
    if(!Screen.IsEmpty())
    {
        const bool Menu=Screen==TEXT("pause")||Screen==TEXT("title")||Screen==TEXT("game")||Screen==TEXT("difficulty")||Screen==TEXT("settings")||Screen==TEXT("saves")||Screen==TEXT("training")||Screen==TEXT("about")||Screen==TEXT("inventory")||Screen==TEXT("journal")||Screen==TEXT("map");
        if(Agent&&!Menu)TickAgent(Delta);return;
    }
    if(PendingRespawn){RespawnTime-=Delta;if(RespawnTime<=0){const bool WasAgent=Agent;if(!LoadLatest())NewGame(Difficulty);PendingRespawn=false;Invulnerable=2;Agent=WasAgent;AgentPath.Reset();AgentIndex=0;Notify(TEXT("Последнее сохранение загружено."));}return;}
    if(Won)return;
    Elapsed+=Delta;SaveIcon=FMath::Max(0.f,SaveIcon-Delta);Invulnerable=FMath::Max(0.f,Invulnerable-Delta);Noise=FMath::Max(0.f,Noise-Delta*.32f);
    if(BookWorld)
    {
        BookWorld->Update(Delta);TickHero(Delta);TickAutoSave(Delta);if(Admin)TickAdmin(Delta);
        if(Hero()){CurrentRoom=BookWorld->Describe(Hero()->GetActorLocation());BookWorld->UpdateLighting(Hero()->GetActorLocation());}
        return;
    }
    TickHero(Delta);TickQuests(Delta);TickAtmosphere(Delta);TickAutoSave(Delta);
    if(Admin)TickAdmin(Delta);if(Agent)TickAgent(Delta);
    if(Hero())
    {
        CurrentRoom.Empty();const FVector P=Hero()->GetActorLocation();const FVector G(P.Y/100,P.Z/100,-P.X/100);
        for(const auto& V:Array(Data,TEXT("rooms")))
        {
            Json R=V->AsObject();auto& A=Array(R,TEXT("rect"));const int32 Level=Number(R,TEXT("level"));
            if(A.Num()!=4||(Level!=-2&&Level!=(P.Z<0?-1:0)))continue;
            if(G.X>=A[0]->AsNumber()&&G.X<A[0]->AsNumber()+A[2]->AsNumber()&&G.Z>=A[1]->AsNumber()&&G.Z<A[1]->AsNumber()+A[3]->AsNumber()){CurrentRoom=String(R,TEXT("id"));Visited.Add(CurrentRoom);break;}
        }
        if(EscapeTime>=0){EscapeTime=FMath::Max(0.f,EscapeTime-Delta);if(EscapeTime<=0)DamageHero(1000,nullptr);}
        if(Flag(TEXT("exit_released"))&&FVector::Dist2D(P,Vector(Array(Data,TEXT("exit"))))<160)
        {Won=true;Agent=false;Score+=FMath::RoundToInt(Hero()->Health*6+EscapeTime*5);Ending=Completed.Contains(TEXT("chapter_office"))?2:1;Show(TEXT("ending"));Cue(TEXT("win"),P);}
    }
}
FString AEWardCampaignMode::RoomName()const
{
    if(HospitalStory&&HospitalStory->HomeStage52>=0)return TEXT("Дом Беки · перед поездкой");
    if(BookWorld&&Hero())return BookWorld->Describe(Hero()->GetActorLocation());
    for(const auto& V:Array(Data,TEXT("rooms")))if(String(V->AsObject(),TEXT("id"))==CurrentRoom)return Tr(String(V->AsObject(),TEXT("name")));
    return FString();
}




#include "WardRevision77.inl"
#include "WardCapture78.inl"
