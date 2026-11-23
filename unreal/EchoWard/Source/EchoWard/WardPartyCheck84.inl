namespace
{
class FPartyCheck84
{
public:
    TWeakObjectPtr<AEWardCampaignMode> Game;
    TArray<TSharedPtr<FOnline>> Guests;
    TArray<FVector> Locations;
    TArray<TSharedPtr<FJsonValue>> Results;
    Json Checks;
    int32 Group = 0;
    int32 Phase = 0;
    double Deadline = 0;
    double NextScene = 0;
    bool ParallelChecked = false;
    double ParallelUntil = 0;
    bool Passed = true;

    void Check(const TCHAR* Name, bool Good)
    {
        Checks->SetBoolField(Name, Good);
        Passed &= Good;
        UE_LOG(LogTemp, Display, TEXT("ONLINE84 group=%d %s %s"), Size(), Good ? TEXT("PASS") : TEXT("FAIL"), Name);
    }

    int32 Size() const
    {
        constexpr int32 Sizes[] = {2, 5, 10};
        return Sizes[FMath::Clamp(Group, 0, 2)];
    }

    bool AdvanceScene(AEWardCampaignMode& Mode)
    {
        auto& Story = *Mode.HospitalStory;
        if (Story.Chapter() != 2) return Story.Chapter() == 3;
        const TGuardValue<bool> Testing(Story.NarrativeTesting35, true);
        if (Story.ActiveScene35.IsEmpty())
        {
            constexpr const TCHAR* Scenes[] = {TEXT("SCENE_02_01"), TEXT("SCENE_02_02"), TEXT("SCENE_02_03"), TEXT("SCENE_02_04")};
            const int32 Step = Mode.Values.FindRef(TEXT("hospital_step"));
            if (Step < 0 || Step >= UE_ARRAY_COUNT(Scenes)) return false;
            const FString Id(Scenes[Step]);
            FVector Standing;
            if (Story.FindNarrativeStanding35(Id, Standing))
                Mode.Hero()->SetActorLocation(Standing, false, nullptr, ETeleportType::TeleportPhysics);
            return Story.UseId(Id);
        }
        const auto& Choices = Array(Story.NarrativeScenes35.FindRef(Story.ActiveScene35), TEXT("choices"));
        if (!Choices.IsEmpty() && Story.SelectedChoice35.IsEmpty() && WardStaging64::ChoiceReady(Story))
            Mode.Command(TEXT("story35_choice_") + String(Choices[0]->AsObject(), TEXT("id")));
        if (WardStaging64::CanFinish(Story)) Mode.Command(TEXT("story35_continue64"));
        return true;
    }

    bool NarrowPass(AEWardCampaignMode& Mode, FOnline& Host)
    {
        auto& Book = *Mode.BookWorld;
        auto* Hero = Mode.Hero();
        const auto* Capsule = Hero->GetCapsuleComponent();
        const auto Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight() - 2);
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(PartyPassage), false, Hero);
        const auto Profile = Capsule->GetCollisionProfileName();
        for (const auto& Portal : Book.Portals)
        {
            const float Width = Portal.Role == TEXT("niche") ? 90 : Portal.Width;
            if (!Portal.Open || Width > 120 || Width < 2 * Capsule->GetScaledCapsuleRadius()) continue;
            FVector Mid = Portal.At + FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight() + 4);
            const bool NorthSouth = Portal.Side == TEXT("north") || Portal.Side == TEXT("south");
            const FVector Cross = NorthSouth ? FVector(0, 85, 0) : FVector(85, 0, 0);
            FHitResult Hit;
            if (Mode.GetWorld()->SweepSingleByProfile(Hit, Mid - Cross, Mid + Cross, FQuat::Identity, Profile, Shape, Query))
                continue;
            TArray<TPair<TWeakObjectPtr<ASkeletalMeshActor>, FVector>> Before;
            for (const auto& Member : Host.Members)
            {
                if (auto* Avatar = Host.TestAvatar(Member.Id))
                {
                    Before.Emplace(Avatar, Avatar->GetActorLocation());
                    Avatar->SetActorLocation(Mid);
                }
            }
            const bool Clear = !Mode.GetWorld()->SweepSingleByProfile(Hit, Mid - Cross, Mid + Cross, FQuat::Identity,
                Profile, Shape, Query);
            for (const auto& Pair : Before)
                if (auto* Avatar = Pair.Key.Get()) Avatar->SetActorLocation(Pair.Value);
            Checks->SetNumberField(TEXT("passage_cm"), Width);
            Checks->SetStringField(TEXT("passage"), Portal.Id);
            return Clear && Before.Num() == Size() - 1;
        }
        return false;
    }

    void SaveResult(FOnline& Host)
    {
        auto Row = MakeShared<FJsonObject>();
        Row->SetNumberField(TEXT("players"), Size());
        Row->SetObjectField(TEXT("checks"), Checks);
        Row->SetNumberField(TEXT("phase"), Phase);
        Results.Add(MakeShared<FJsonValueObject>(Row));
        for (const auto& Guest : Guests) Guest->Close();
        Guests.Reset();
        Locations.Reset();
        Host.Leave();
        Phase = 0;
        ParallelChecked = false;
        ParallelUntil = 0;
        NextScene = 0;
        ++Group;
    }

    bool Tick()
    {
        auto* Mode = Game.Get();
        if (!Mode || !Mode->Interface || !Mode->HospitalStory || !Mode->BookWorld) return true;
        auto Host = Session(*Mode);
        const double Now = FPlatformTime::Seconds();
        if (Group >= 3)
        {
            auto Root = MakeShared<FJsonObject>();
            Root->SetBoolField(TEXT("success"), Passed);
            Root->SetArrayField(TEXT("groups"), Results);
            Write(FPaths::ProjectSavedDir() / TEXT("Prototype/party84.json"), Root);
            FPlatformMisc::RequestExit(false);
            return false;
        }
        if (Phase && Now > Deadline)
        {
            Check(TEXT("completed_in_time"), false);
            SaveResult(*Host);
            return true;
        }

        for (int32 Index = 0; Index < Guests.Num(); ++Index)
        {
            auto& Guest = Guests[Index];
            Guest->Tick();
            if (Locations.IsValidIndex(Index))
            {
                if (Index == 0 && Phase == 7 && Number(Guest->ProbePersonal, TEXT("respawn")) > 0 &&
                    Array(Guest->ProbePersonal, TEXT("spawn")).Num() == 3)
                    Locations[Index] = Vector(Array(Guest->ProbePersonal, TEXT("spawn")));
                Guest->ProbeTick(Locations[Index], 130, false);
            }
        }

        auto& Story = *Mode->HospitalStory;
        switch (Phase)
        {
        case 0:
            Checks = MakeShared<FJsonObject>();
            Mode->Command(TEXT("page_online"));
            Mode->Command(TEXT("online_create"));
            Host->RoomName = FString::Printf(TEXT("ECHO 84 test %d"), Size());
            Host->MaxPlayers = Size();
            Host->Host();
            if (Host->Role != ERole::Host)
            {
                Check(TEXT("host_started"), false);
                SaveResult(*Host);
                break;
            }
            Host->ToggleReady();
            Deadline = Now + 105;
            ++Phase;
            break;
        case 1:
            if (!Host->InternetConfigured() || Host->InternetConnected())
            {
                for (int32 Index = 1; Index < Size(); ++Index)
                {
                    auto Peer = MakeShared<FOnline>(*Mode, false);
                    Peer->PlayerName = FString::Printf(TEXT("Probe %d"), Index);
                    Peer->Invitation = Host->InvitationCode();
                    Peer->JoinCode();
                    Guests.Add(Peer);
                }
                ++Phase;
            }
            break;
        case 2:
            for (const auto& Guest : Guests)
                if (Guest->Role == ERole::Guest && !Guest->IsReady()) Guest->ToggleReady();
            if (Host->CanStart() && Host->Members.Num() == Size())
            {
                Host->Start();
                Check(TEXT("all_players_started"), Host->InMap && Story.Chapter() == 2);
                Story.FreezeAI = true;
                Check(TEXT("separate_start_positions"), Host->SafeSeatCount(*Mode) == Size());
                const FVector Upper = Mode->Hero()->GetActorLocation();
                const auto* Room = Mode->BookWorld->Rooms.FindByPredicate([](const auto& R) { return R.Id == TEXT("N01"); });
                const FVector Lower = Room ? Room->Inside : Upper - FVector(0, 0, 480);
                for (int32 I = 0; I < Guests.Num(); ++I)
                    Locations.Add(I % 2 == 0 ? Lower : Upper);
                ++Phase;
            }
            break;
        case 3:
            if (Guests.ContainsByPredicate([&](const auto& Peer) { return Peer->ProbePoses != Size() || !Peer->ProbeWorld; })) break;
            if (Host->VisibleAvatars() != Size() - 1) break;
            Check(TEXT("narrow_passage_clear_with_group"), NarrowPass(*Mode, *Host));
            Check(TEXT("split_floors_keep_remote_in_simulation"),
                FMath::Abs(Locations[0].Z - Mode->Hero()->GetActorLocation().Z) >= 400 &&
                PartyDistanceSquared(*Mode, Locations[0]) < 1);
            Check(TEXT("scaled_supplies"), SupplyAmount(*Mode, TEXT("SUPPLY_2F_0_MED"), 1) == (Size() == 2 ? 1 : Size() == 5 ? 2 : 3) &&
                SupplyAmount(*Mode, TEXT("SUPPLY_2F_0_AMMO"), 5) == (Size() == 2 ? 5 : Size() == 5 ? 10 : 15) &&
                Mode->Hero()->Reserve == (Size() == 2 ? 9 : Size() == 5 ? 7 : 5));
            Check(TEXT("scaled_monster_pressure"), RecoveryScale(*Mode) == (Size() == 2 ? 1.f : Size() == 5 ? .83f : .68f) &&
                Story.Mobs.ContainsByPredicate([&](const auto& Mob) {
                    return Mob && Mob->FirstChapter == 2 && FMath::IsNearlyEqual(Mob->MaxHealth,
                        float(Number(Mob->Definition, TEXT("health"), 70)) * (Size() == 2 ? 1.f : Size() == 5 ? 1.4f : 1.85f));
                }));
            for (int32 I = 0; I < Guests.Num(); ++I) Guests[I]->ProbeTick(Locations[I], 130, true);
            // The local weapon trace is covered by the two-window test. Record its
            // announcement here to exercise all emitters within one batch.
            Host->LocalShot(Mode->Hero()->GetActorLocation());
            ++Phase;
            break;
        case 4:
            if (Guests.ContainsByPredicate([&](const auto& Peer) {
                return !Peer->ProbeUsedRound() || Peer->ProbeHeardShots != Size();
            })) break;
            Check(TEXT("concurrent_shots_each_acked_once"), true);
            ++Phase;
            break;
        case 5:
            if (Mode->Values.FindRef(TEXT("hospital_step")) < 1)
            {
                if (!Story.ActiveScene35.IsEmpty() && !ParallelChecked)
                {
                    const TCHAR* Items[] = {TEXT("NOTE_02_02"), TEXT("NOTE_02_03")};
                    const int32 Count = FMath::Min(2, Guests.Num());
                    if (ParallelUntil == 0)
                    {
                        ParallelUntil = Now + 10;
                        for (int32 I = 0; I < Count; ++I)
                            if (auto* Prop = Story.Props.FindRef(Items[I]))
                                Prop->SetActorLocation(Locations[I] + FVector(0, 0, 38));
                    }
                    bool Collected = true;
                    for (int32 I = 0; I < Count; ++I)
                    {
                        if (Mode->Completed.Contains(Items[I])) continue;
                        Collected = false;
                        Guests[I]->ProbeTick(Locations[I], 130, false, Items[I]);
                    }
                    if (!Collected && Now < ParallelUntil) break;
                    Check(TEXT("parallel_notes_during_host_scene"), Collected && Mode->Values.FindRef(TEXT("hospital_step")) == 0);
                    ParallelChecked = true;
                }
                if (Now >= NextScene) { AdvanceScene(*Mode); NextScene = Now + .45; }
                break;
            }
            Locations[0].Z = -2700;
            ++Phase;
            break;
        case 6:
            if (Number(Guests[0]->ProbePersonal, TEXT("health"), 100) > 0) break;
            Check(TEXT("one_player_down_others_continue"), Guests.Num() + 1 == Host->Members.Num() &&
                Mode->Values.FindRef(TEXT("hospital_step")) == 1 &&
                Mode->Hero()->Health > 0 && !Guests.ContainsByPredicate([&](const auto& Peer) {
                    return Peer != Guests[0] && Number(Peer->ProbePersonal, TEXT("health"), 100) <= 0;
                }));
            ++Phase;
            break;
        case 7:
            if (Number(Guests[0]->ProbePersonal, TEXT("respawn")) < 1 ||
                Number(Guests[0]->ProbePersonal, TEXT("health")) <= 0) break;
            if (!Host->Members.ContainsByPredicate([](const auto& M) { return !M.Host && M.ConfirmedRespawn == 1 && M.LastPose > 0; }))
                break;
            Check(TEXT("respawn_without_valid_pose_preserves_ammo_and_progress"),
                Guests[0]->ProbeUsedRound() &&
                Number(Guests[0]->ProbePersonal, TEXT("health")) == 60 &&
                Mode->Values.FindRef(TEXT("hospital_step")) == 1);
            ++Phase;
            break;
        case 8:
            if (Story.Chapter() < 3)
            {
                if (Now >= NextScene) { AdvanceScene(*Mode); NextScene = Now + .45; }
                break;
            }
            if (Guests.ContainsByPredicate([](const auto& Peer) { return Number(Peer->ProbeWorld, TEXT("chapter")) != 3; }))
                break;
            Check(TEXT("whole_group_received_chapter_3"), true);
            const auto End = Host->CheckWorld(*Mode);
            Check(TEXT("same_doors_tasks_and_monsters"), !Guests.ContainsByPredicate([&](const auto& Peer) {
                const auto& Mirror = Peer->ProbeWorld;
                if (String(End, TEXT("doors")) != String(Mirror, TEXT("doors"))) return true;
                const auto& Done = Array(Mirror, TEXT("done"));
                if (Array(End, TEXT("done")).Num() != Done.Num()) return true;
                for (const auto& Goal : Array(End, TEXT("done")))
                    if (!Done.ContainsByPredicate([&](const auto& Entry) { return Entry->AsString() == Goal->AsString(); }))
                        return true;
                const auto& Mobs = Array(Mirror, TEXT("mobs"));
                if (Array(End, TEXT("mobs")).Num() != Mobs.Num()) return true;
                for (const auto& Value : Array(End, TEXT("mobs")))
                {
                    const auto Mob = Value->AsObject();
                    const auto Id = String(Mob, TEXT("id"));
                    const auto* Other = Mobs.FindByPredicate([&](const auto& Entry) {
                        return String(Entry->AsObject(), TEXT("id")) == Id;
                    });
                    if (!Other || Number((*Other)->AsObject(), TEXT("hp")) != Number(Mob, TEXT("hp")) ||
                        Boolean((*Other)->AsObject(), TEXT("active")) != Boolean(Mob, TEXT("active")) ||
                        String((*Other)->AsObject(), TEXT("phase")) != String(Mob, TEXT("phase")))
                        return true;
                }
                return !ParallelChecked;
            }));
            SaveResult(*Host);
            break;
        }
        return true;
    }
};
}
