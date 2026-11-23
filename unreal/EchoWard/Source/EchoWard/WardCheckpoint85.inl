    static constexpr int32 SlotCount = 3;
    int32 SelectedSlot = 0;
    int32 StartChapter = 1;
    Json InitialCheckpoint;
    double NextCheckpoint = 0;
    FString ProfileId;
    Json StoredPlayers;
    FString ContinuationId;
    FVector LastSavePosition = FVector::ZeroVector;

    FString CoopDirectory() const
    {
        return FPaths::ProjectSavedDir() / TEXT("OnlineCampaignSaves");
    }

    FString SlotPath(int32 Slot) const
    {
        check(Slot >= 0 && Slot < SlotCount);
        return CoopDirectory() / FString::Printf(TEXT("team_%d.json"), Slot + 1);
    }

    const FString& LocalProfile()
    {
        if (!ProfileId.IsEmpty()) return ProfileId;
        const auto File = CoopDirectory() / TEXT("profile.json");
        ProfileId = String(Read(File), TEXT("profile"));
        if (!IsToken(ProfileId))
        {
            ProfileId = Token();
            const auto Data = MakeShared<FJsonObject>();
            Data->SetStringField(TEXT("profile"), ProfileId);
            Write(File, Data);
        }
        return ProfileId;
    }

    Json Checkpoint(int32 Slot) const
    {
        const auto Data = Read(SlotPath(Slot));
        const auto* Game = Owner.Get();
        const auto World = Object(Data, TEXT("campaign"));
        if (Number(Data, TEXT("online_format")) != 1 || !IsToken(String(Data, TEXT("run"))) ||
            !Game || !Game->ValidateSave(World)) return {};
        const int32 Chapter = Number(Object(World, TEXT("values")), TEXT("hospital_chapter"), 1);
        return Chapter >= 1 && Chapter <= 20 ? Data : Json();
    }

    bool WriteCheckpoint(bool Manual)
    {
        auto* Game = Owner.Get();
        if (!DrivePages || Role != ERole::Host || !InMap || !Game || !Game->Hero() || !Game->BookWorld ||
            !Game->HospitalStory || Game->Hero()->Dead || Game->PendingRespawn || Game->BookWorld->LiftMoving ||
            Game->BookWorld->FurnitureHiding || Game->HidingBed ||
            Game->HospitalStory->ArrivalCinematic()) return false;
        if(RescueUnsafeToSave()){if(Manual)Game->Notify(TEXT("Сначала помоги раненым напарникам."));return false;}
        const auto World = Game->Capture();
        if (!Game->ValidateSave(World)) return false;
        const auto Save = MakeShared<FJsonObject>();
        Save->SetNumberField(TEXT("online_format"), 1);
        Save->SetStringField(TEXT("run"), ContinuationId);
        Save->SetStringField(TEXT("room_name"), Room.Name);
        Save->SetNumberField(TEXT("team_size"), TeamSize);
        Save->SetObjectField(TEXT("campaign"), World);
        auto Roster = MakeShared<FJsonObject>();
        if (StoredPlayers) Roster->Values = StoredPlayers->Values;
        for (const auto& Member : Members)
        {
            if (Member.Host || !IsToken(Member.Profile)) continue;
            const auto Row = Personal(Member);
            Row->RemoveField(TEXT("spawn"));
            Row->RemoveField(TEXT("respawn"));
            Row->RemoveField(TEXT("notice"));
            Row->RemoveField(TEXT("ack"));
            if (!Roster->HasField(Member.Profile) && Roster->Values.Num() >= 32) continue;
            Roster->SetObjectField(Member.Profile, Row);
        }
        Save->SetObjectField(TEXT("members"), Roster);
        const auto Path = SlotPath(SelectedSlot);
        const auto Temporary = Path + TEXT(".tmp");
        if (!Write(Temporary, Save) || !IFileManager::Get().Move(*Path, *Temporary, true, true, false, true))
            return false;
        StoredPlayers = Roster;
        LastSavePosition = Game->Hero()->GetActorLocation();
        if (Manual) Game->Notify(TEXT("Сетевая контрольная точка сохранена."), 3);
        UE_LOG(LogTemp, Display, TEXT("ONLINE85 checkpoint slot=%d chapter=%d"), SelectedSlot + 1, Game->HospitalStory->Chapter());
        return true;
    }

    void ReadPersonalCheckpoint(FMember& Member)
    {
        if (!StoredPlayers || !IsToken(Member.Profile)) return;
        const auto Row = Object(StoredPlayers, Member.Profile);
        if (!Row->HasField(TEXT("health"))) return;
        Member.Health = FMath::Clamp(float(Number(Row, TEXT("health"))), 1.f, 100.f);
        Member.Rounds = FMath::Clamp(int32(Number(Row, TEXT("rounds"))), 0, 9);
        Member.Reserve = FMath::Clamp(int32(Number(Row, TEXT("reserve"))), 0, 72);
        Member.Shells = FMath::Clamp(int32(Number(Row, TEXT("shells"))), 0, 5);
        Member.Pistol = Boolean(Row, TEXT("pistol"));
        Member.Shotgun = Boolean(Row, TEXT("shotgun"));
        Member.LoadoutReady = Boolean(Row, TEXT("kit"));
        Member.PreferShotgun = Boolean(Row, TEXT("prefer_long"));
        Member.Items.Reset();
        for (const auto& Entry : Object(Row, TEXT("items"))->Values)
            if (Entry.Value->Type == EJson::Number && !KeyItem(FString(Entry.Key)))
                Member.Items.Add(FString(Entry.Key), FMath::Clamp(int32(Entry.Value->AsNumber()), 0, 72));
    }

    void RestoreChapterTwoBalance(AEWardCampaignMode& Game)
    {
        if (!InitialCheckpoint || !Game.HospitalStory || Game.HospitalStory->Chapter() != 2) return;
        const auto Campaign = Object(InitialCheckpoint, TEXT("campaign"));
        const auto& SavedMobs = Array(Object(Campaign, TEXT("hospital_story")), TEXT("mobs"));
        const int32 Players = Number(InitialCheckpoint, TEXT("team_size"), 2);
        const float Scale = Players <= 2 ? 1.f : Players <= 5 ? 1.4f : 1.85f;
        for (const auto& Mob : Game.HospitalStory->Mobs)
        {
            if (!Mob || Mob->FirstChapter != 2) continue;
            const auto* Before = SavedMobs.FindByPredicate([&](const auto& Value) {
                return String(Value->AsObject(), TEXT("id")) == Mob->Id;
            });
            Mob->MaxHealth = float(Number(Mob->Definition, TEXT("health"), 70)) * Scale;
            if (Before) Mob->Health = FMath::Clamp(float(Number((*Before)->AsObject(), TEXT("health"))), 0.f, Mob->MaxHealth);
        }
    }

public:
    int32 SaveSlot() const { return SelectedSlot; }
    FVector LastCheckpointPosition() const { return LastSavePosition.IsNearlyZero() ? CoopStart : LastSavePosition; }
    void SelectSaveSlot(int32 Slot) { SelectedSlot = FMath::Clamp(Slot, 0, SlotCount - 1); }
    bool ContinueSaved = false;
    bool ChapterTwoTest = false;
    FString SlotLabel(int32 Slot) const
    {
        if (const auto Saved = Checkpoint(Slot))
            return FString::Printf(TEXT("Слот %d · глава %02d"), Slot + 1,
                int32(Number(Object(Object(Saved, TEXT("campaign")), TEXT("values")), TEXT("hospital_chapter"), 1)));
        return FString::Printf(TEXT("Слот %d · пусто"), Slot + 1);
    }

    bool SaveFromMenu(bool Manual) { return WriteCheckpoint(Manual); }

private:
