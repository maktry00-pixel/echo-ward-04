namespace
{
struct FRecoveryCheck85
{
    TWeakObjectPtr<AEWardCampaignMode> Game;
    TSharedPtr<FOnline> Peer, Newcomer;
    FString Profile = Token();
    FString SavedDoor, BeforeSolo;
    Json Checks = MakeShared<FJsonObject>();
    bool Passed = true;
    int32 Phase = 0, RouteIndex = 0;
    double Deadline = 0, ResumeAt = 0;
    FVector PeerAt;

    void Check(const TCHAR* Id, bool Good)
    {
        Checks->SetBoolField(Id, Good);
        Passed &= Good;
        UE_LOG(LogTemp, Display, TEXT("ONLINE85 %s %s"), Good ? TEXT("PASS") : TEXT("FAIL"), Id);
    }

    bool Finish(AEWardCampaignMode& Mode)
    {
        Check(TEXT("solo_save_unchanged"), String(Mode.ReadSlot(TEXT("manual_1")), TEXT("saved_at")) == BeforeSolo);
        Checks->SetBoolField(TEXT("success"), Passed);
        Checks->SetNumberField(TEXT("last_phase"), Phase);
        Write(FPaths::ProjectSavedDir() / TEXT("Prototype/online85.json"), Checks);
        FPlatformMisc::RequestExit(false);
        return false;
    }

    TSharedPtr<FOnline> Join(AEWardCampaignMode& Mode, FOnline& Host, const FString& Id)
    {
        auto Connection = MakeShared<FOnline>(Mode, false);
        Connection->PlayerName = TEXT("Спутник");
        Connection->TestProfile(Id);
        Connection->Invitation = Host.InvitationCode();
        Connection->JoinCode();
        return Connection;
    }

    bool Tick()
    {
        auto* Mode = Game.Get();
        if (!Mode || !Mode->Interface || !Mode->HospitalStory || !Mode->BookWorld) return true;
        const auto Host = Session(*Mode);
        auto& Story = *Mode->HospitalStory;
        const double Now = FPlatformTime::Seconds();
        if (Deadline && Now > Deadline)
        {
            Check(TEXT("completed_test"), false);
            return Finish(*Mode);
        }
        if (Peer)
        {
            Peer->Tick();
            if (Host->InMap)
            {
                if (Array(Peer->ProbePersonal, TEXT("spawn")).Num() == 3)
                    PeerAt = Vector(Array(Peer->ProbePersonal, TEXT("spawn")));
                if (Now >= ResumeAt) Peer->ProbeTick(PeerAt, 0, false);
            }
        }
        if (Newcomer) Newcomer->Tick();
        switch (Phase)
        {
        case 0:
            BeforeSolo = String(Mode->ReadSlot(TEXT("manual_1")), TEXT("saved_at"));
            Mode->Command(TEXT("page_online"));
            Mode->Command(TEXT("online_create"));
            Host->MaxPlayers = 4;
            Host->Host();
            Check(TEXT("new_campaign_starts_at_chapter_one"), Host->Role == ERole::Host && Host->Room.Chapter == 1);
            Host->ToggleReady();
            Deadline = Now + 160;
            Phase = 20;
            break;
        case 20:
            if (Host->InternetConfigured() && !Host->InternetConnected()) break;
            Peer = Join(*Mode, *Host, Profile);
            Phase = 21;
            break;
        case 21:
            if (Peer->Role == ERole::Guest && !Peer->IsReady()) Peer->ToggleReady();
            if (!Host->CanStart()) break;
            Host->Start();
            Check(TEXT("host_enters_authored_home_prologue"), Host->InMap && Story.Chapter() == 1 &&
                Story.HomeStage52 >= 0);
            Phase = 22;
            break;
        case 22:
            if (!Peer->ProbeWorld || Number(Peer->ProbeWorld, TEXT("chapter")) != 1) break;
            Check(TEXT("guest_receives_chapter_one"), true);
            Host->Leave();
            Peer.Reset();
            Host->SelectSaveSlot(1);
            Host->ChapterTwoTest = true;
            Host->Host();
            Host->ToggleReady();
            Phase = 1;
            break;
        case 1:
            if (Host->InternetConfigured() && !Host->InternetConnected()) break;
            Peer = Join(*Mode, *Host, Profile);
            ++Phase;
            break;
        case 2:
            if (Peer->Role == ERole::Guest && !Peer->IsReady()) Peer->ToggleReady();
            if (!Host->CanStart()) break;
            Host->Start();
            Story.FreezeAI = true;
            PeerAt = Mode->Hero()->GetActorLocation();
            ++Phase;
            break;
        case 3:
            if (!Peer->ProbeWorld || Number(Peer->ProbeWorld, TEXT("chapter")) != 2 || !Host->TestingGuest()) break;
            Peer->ProbeTick(PeerAt, 0, true);
            Newcomer = Join(*Mode, *Host, Token());
            ++Phase;
            break;
        case 4:
            if (!Peer->ProbeUsedRound() || !Newcomer->Status.Contains(TEXT("уже началась"))) break;
            Check(TEXT("late_new_player_rejected"), Host->Members.Num() == 2);
            Newcomer.Reset();
            ResumeAt = Now + (Host->InternetConfigured() ? 17 : 11);
            Peer->TestDropTraffic(ResumeAt - Now);
            ++Phase;
            break;
        case 5:
            if (Now < ResumeAt) break;
            Check(TEXT("disconnected_player_kept_reserved"), Host->Members.Num() == 2 && Host->Members[1].Disconnected);
            Check(TEXT("rebind_after_connection_loss"), Peer->TestRebind());
            ++Phase;
            break;
        case 6:
            if (Peer->Reconnecting() || Host->Members[1].Disconnected) break;
            Check(TEXT("same_session_and_personal_ammo_after_reconnect"), Host->Members.Num() == 2 &&
                Peer->ProbeUsedRound() && Host->Members[1].Profile == Profile);
            if (!WardAdmin79::TestAdvanceCampaign(*Mode, 3)) { Check(TEXT("advance_to_chapter_three"), false); return Finish(*Mode); }
            {
                FVector At;
                if (Story.FindNarrativeStanding35(TEXT("SCENE_03_01"), At))
                    Mode->Hero()->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
            }
            if (Mode->BookWorld->Portals.Num() > 0)
            {
                auto& Door = Mode->BookWorld->Portals[0];
                Door.Open = false;
                Mode->BookWorld->RefreshPortalVisual(Door);
            }
            Check(TEXT("save_network_checkpoint"), Mode->Save(TEXT("manual_1")));
            SavedDoor = String(Host->CheckWorld(*Mode), TEXT("doors"));
            ++Phase;
            break;
        case 7:
            if (Number(Peer->ProbeWorld, TEXT("chapter")) != 3) break;
            Host->Leave();
            ++Phase;
            break;
        case 8:
            if (Peer->Role != ERole::Idle) break;
            Check(TEXT("host_exit_closed_room"), Peer->Status.Contains(TEXT("Хозяин закрыл")));
            Peer.Reset();
            Host->ContinueSaved = true;
            Host->Host();
            Check(TEXT("slot_advertises_saved_chapter"), Host->Role == ERole::Host && Host->Room.Chapter == 3);
            Host->ToggleReady();
            ++Phase;
            break;
        case 9:
            if (Host->InternetConfigured() && !Host->InternetConnected()) break;
            Peer = Join(*Mode, *Host, Profile);
            ++Phase;
            break;
        case 10:
            if (Peer->Role == ERole::Guest && !Peer->IsReady()) Peer->ToggleReady();
            if (!Host->CanStart()) break;
            Host->Start();
            Story.FreezeAI = true;
            PeerAt = Mode->Hero()->GetActorLocation();
            ++Phase;
            break;
        case 11:
            if (Number(Peer->ProbeWorld, TEXT("chapter")) != 3) break;
            Check(TEXT("fresh_lobby_restored_doors_objectives_and_personal_gear"),
                Story.Chapter() == 3 && Mode->Completed.Contains(TEXT("SCENE_02_04")) &&
                String(Peer->ProbeWorld, TEXT("doors")) == SavedDoor && Peer->ProbeUsedRound());
            ++Phase;
            break;
        case 12:
        {
            constexpr int32 Stops[] = {8, 14, 19, 20};
            if (RouteIndex >= UE_ARRAY_COUNT(Stops))
            {
                Check(TEXT("campaign_reached_epilogue"), Story.Chapter() == 20 && Story.NarrativeFlag35(TEXT("EPILOGUE_COMPLETE")));
                return Finish(*Mode);
            }
            const int32 Target = Stops[RouteIndex];
            if (Story.Chapter() < Target && !WardAdmin79::TestAdvanceCampaign(*Mode, Target))
            {
                Check(TEXT("scripted_campaign_advance"), false);
                return Finish(*Mode);
            }
            if (Number(Peer->ProbeWorld, TEXT("chapter")) != Target) break;
            const auto Id = FString::Printf(TEXT("same_state_at_chapter_%02d"), Target);
            Check(*Id, Array(Peer->ProbeWorld, TEXT("done")).Num() == Mode->Completed.Num());
            ++RouteIndex;
            break;
        }
        }
        return true;
    }
};
}
