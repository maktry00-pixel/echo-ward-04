namespace
{
struct FPrologueCheck86
{
    TWeakObjectPtr<AEWardCampaignMode> Game;
    bool Host = false;
    bool Passed = true;
    int32 Step = 0;
    double Deadline = 0;
    double NextMove = 0;
    double NextInteraction = 0;
    double HoldUntil = 0;
    bool SawHolster = false;
    Json Report = MakeShared<FJsonObject>();

    void Check(const TCHAR* Key, bool Value)
    {
        Report->SetBoolField(Key, Value);
        Passed &= Value;
        UE_LOG(LogTemp, Display, TEXT("ONLINE86 %s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), Key);
    }

    bool Finish()
    {
        Report->SetBoolField(TEXT("success"), Passed);
        Report->SetNumberField(TEXT("step"), Step);
        Write(FPaths::ProjectSavedDir() / TEXT("Prototype/prologue86.json"), Report);
        FPlatformMisc::RequestExit(false);
        return false;
    }

    bool WalkTo(AEWardCampaignHero& Hero, FVector Goal, double Now)
    {
        const FVector At = Hero.GetActorLocation();
        if (FVector::Dist(At, Goal) <= 16) return true;
        if (Now >= NextMove)
        {
            NextMove = Now + .15;
            Hero.SetActorLocation(At + (Goal - At).GetClampedToMaxSize(190), false, nullptr, ETeleportType::TeleportPhysics);
        }
        return false;
    }

    void TryInteract(AEWardCampaignMode& Mode, double Now)
    {
        if (Now < NextInteraction) return;
        NextInteraction = Now + .85;
        Interact(Mode);
    }

    bool VisibleWeapon(const FOnline& Lobby, bool Long, bool Painted)
    {
        const auto Other = Lobby.Members.FindByPredicate([&](const FMember& Member) { return Member.Host != Host; });
        auto* Avatar = Other ? Lobby.TestAvatar(Other->Id) : nullptr;
        if (!Avatar || Avatar->IsHidden()) return false;
        TArray<UStaticMeshComponent*> Parts;
        Avatar->GetComponents(Parts);
        return Parts.ContainsByPredicate([&](const UStaticMeshComponent* Part) {
            const auto* Mount = Part->GetAttachParent();
            const auto* Mesh = Avatar->GetSkeletalMeshComponent();
            if (!Part->GetStaticMesh() || !Part->IsVisible() || !Mount || Mount->GetAttachParent() != Mesh ||
                !Mesh->DoesSocketExist(Mount->GetAttachSocketName()) ||
                !Part->ComponentTags.Contains(Long ? TEXT("shotgun") : TEXT("pistol")))
                return false;
            return !Painted || Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0));
        });
    }

    bool Tick()
    {
        auto* Mode = Game.Get();
        if (!Mode || !Mode->Interface || !Mode->HospitalStory || !Mode->BookWorld || !Mode->Hero()) return true;
        const auto Lobby = Session(*Mode);
        auto& Story = *Mode->HospitalStory;
        auto& Hero = *Mode->Hero();
        const double Now = FPlatformTime::Seconds();
        if (Deadline && Now > Deadline)
        {
            Check(TEXT("prologue_completed"), false);
            return Finish();
        }
        const FString Name = FString(TEXT("ECHO Prologue 86")) +
            (FParse::Param(FCommandLine::Get(), TEXT("EchoRelayOnly")) ? TEXT(" HTTPS") : TEXT(" LAN"));
        if (Step == 0)
        {
            Deadline = Now + 185;
            Mode->Command(TEXT("page_online"));
            if (Host)
            {
                Mode->Command(TEXT("online_create"));
                Lobby->RoomName = Name;
                Lobby->MaxPlayers = 2;
                Lobby->Host();
            }
            else Mode->Command(TEXT("online_browse"));
            ++Step;
        }
        if (Step == 1)
        {
            if (!Host && Lobby->Role == ERole::Idle)
            {
                if (const auto* Listing = Lobby->Available.FindByPredicate([&](const FRoom& Room) { return Room.Name == Name; }))
                {
                    Lobby->SelectedRoom = Listing->Id;
                    Lobby->JoinFound();
                }
            }
            if (Lobby->Role != (Host ? ERole::Host : ERole::Guest)) return true;
            Lobby->ToggleReady();
            ++Step;
        }
        if (Step == 2 && Host && Lobby->CanStart()) Lobby->Start();
        if (!Lobby->InMap) return true;
        const auto& Portals = Mode->BookWorld->Portals;
        const int32 DoorIndex = Portals.IndexOfByPredicate([](const auto& P) { return P.Zone == TEXT("N02") && P.Side == TEXT("north"); });
        if (!Portals.IsValidIndex(DoorIndex))
        {
            Check(TEXT("entrance_exists"), false);
            return Finish();
        }
        const auto& Door = Portals[DoorIndex];
        const FVector LobbyAt(4800, -1100, 98);
        const FVector Outside = Door.At + FVector(0, -150, 98);
        if (Step == 2)
        {
            Check(TEXT("spawned_in_home"), Story.HomeStage52 == 0 && Story.HomeContains52(Hero.GetActorLocation()));
            ++Step;
        }
        if (Host)
        {
            if (Step == 3 && Story.HomeStage52 == 1)
            {
                const auto* Guest = Lobby->TestingGuest();
                Check(TEXT("guest_answered_phone_inside_home"), Guest && Story.HomeContains52(Guest->Position));
                ++Step;
            }
            if (Step == 4 && Story.ArrivalStage == 0)
            {
                Check(TEXT("guest_started_drive"), Mode->Completed.Contains(TEXT("HOME_CALL52")));
                ++Step;
            }
            if (Step == 5 && Story.ArrivalStage == 2)
            {
                Check(TEXT("guest_exited_car"), Mode->Completed.Contains(TEXT("SCENE_01_01")));
                ++Step;
            }
            if (Step == 6 && Story.ActiveScene35 == TEXT("SCENE_01_01") && WardStaging64::CanFinish(Story))
                Mode->Command(TEXT("story35_continue64"));
            if (Step == 6 && Story.ActiveScene35 == TEXT("CHECKPOINT_69"))
            {
                Check(TEXT("guest_started_checkpoint"), true);
                ++Step;
            }
            if (Step == 7 && Story.ActiveScene35 == TEXT("CHECKPOINT_69"))
            {
                const auto& Choices = Array(Story.NarrativeScenes35.FindRef(Story.ActiveScene35), TEXT("choices"));
                if (!Choices.IsEmpty() && Story.SelectedChoice35.IsEmpty() && WardStaging64::ChoiceReady(Story))
                    Mode->Command(TEXT("story35_choice_") + String(Choices[0]->AsObject(), TEXT("id")));
                if (WardStaging64::CanFinish(Story)) Mode->Command(TEXT("story35_continue64"));
            }
            if (Step == 7 && Story.ArrivalStage == 3)
            {
                Hero.SetActorLocation(LobbyAt, false, nullptr, ETeleportType::TeleportPhysics);
                Check(TEXT("host_inside_lobby"), Story.PartyInsideEntrance29(LobbyAt));
                Check(TEXT("cannot_close_on_guest"), !OperateDoor(*Mode->BookWorld, DoorIndex, Outside, false));
                HoldUntil = Now + 2.;
                ++Step;
            }
            if (Step == 8 && Now >= HoldUntil)
            {
                Check(TEXT("entrance_remained_open_before_guest_entered"), Door.Open && !Door.Locked &&
                    Story.ArrivalStage == 3 && Story.EntranceWaiting29 == 1);
                ++Step;
            }
            if (Step == 9 && Story.ArrivalStage == 5)
            {
                Check(TEXT("shutter_closed_and_doctor_started"), !Door.Open && Door.Locked &&
                    Story.ActiveScene35 == TEXT("SCENE_01_02") && Mode->Completed.Contains(TEXT("SCENE_01_02")));
                HoldUntil = Now + 5.;
                ++Step;
            }
            if (Step == 10)
            {
                const auto* Guest = Lobby->TestingGuest();
                if (Guest && Guest->HeldWeapon == ECarriedWeapon::Shotgun && VisibleWeapon(*Lobby, true, true))
                {
                    Check(TEXT("host_has_pistol_guest_has_visible_painted_shotgun"), !Mode->Shotgun &&
                        WardGameplay76::Armed(*Mode) && Guest->Shotgun && !Guest->Pistol);
                    ++Step;
                }
            }
            if (Step == 11)
            {
                const auto* Guest = Lobby->TestingGuest();
                if (Guest && Guest->ShotsFired == 1 && Guest->LastShotLong && Guest->Shells == 4 &&
                    Guest->Items.FindRef(TEXT("shells")) == 2)
                {
                    Check(TEXT("guest_shot_and_reloaded_personal_shells"), Hero.Rounds == 9);
                    ++Step;
                }
            }
            if (Step >= 11)
            {
                const auto* Guest = Lobby->TestingGuest();
                SawHolster |= Guest && Guest->HeldWeapon == ECarriedWeapon::None && !VisibleWeapon(*Lobby, true, false);
            }
            if (Step == 12 && SawHolster && VisibleWeapon(*Lobby, true, true))
            {
                Check(TEXT("guest_holster_and_draw_visible_to_host"), true);
                HoldUntil = Now + 2.;
                ++Step;
            }
            if (Step == 13 && Now >= HoldUntil) return Finish();
        }
        else
        {
            const FVector Phone = Story.HomePhone52 + FVector(-90, 0, 15);
            if (Step == 3 && Story.HomeStage52 == 0 && WalkTo(Hero, Phone, Now))
            {
                TryInteract(*Mode, Now);
            }
            if (Step == 3 && Story.HomeStage52 == 1 && !Story.HomeSubtitle69.IsEmpty())
            {
                Check(TEXT("shared_home_subtitle"), true);
                Step = 5;
            }
            if (Step == 5 && Story.HomeStage52 == 2 && WalkTo(Hero, FVector(-80000, -80245, 98), Now))
            {
                TryInteract(*Mode, Now);
            }
            if (Step == 5 && Story.ArrivalStage == 0)
            {
                Check(TEXT("shared_car_view"), Hero.IsHidden() &&
                    Hero.GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
                Step = 7;
            }
            if (Step == 7 && Story.ArrivalStage == 1)
            {
                TryInteract(*Mode, Now);
            }
            if (Step == 7 && Story.ArrivalStage == 2 && Hero.GetCapsuleComponent()->IsCollisionEnabled() &&
                FVector::Dist(Hero.GetActorLocation(), Vector(Array(Story.ArrivalData, TEXT("exit"))) * 100) < 850)
            {
                Check(TEXT("guest_relocated_near_car"), !Hero.IsHidden());
                NextMove = Now + 5.;
                Step = 9;
            }
            if (Step == 9 && Story.ArrivalStage == 2 && Story.ActiveScene35.IsEmpty() && Now >= NextMove)
            {
                const FVector At = Vector(Array(Story.ArrivalData, TEXT("checkpoint"))) * 100;
                if (WalkTo(Hero, At, Now))
                {
                    TryInteract(*Mode, Now);
                }
            }
            if (Step == 9 && Story.ArrivalStage == 3 && Story.EntranceWaiting29 == 1)
            {
                Check(TEXT("guest_sees_wait_count_and_open_door"), Door.Open && !Door.Locked);
                HoldUntil = Now + 3.;
                Step = 11;
            }
            if (Step == 11 && Now >= HoldUntil && WalkTo(Hero, LobbyAt, Now))
            {
                Check(TEXT("guest_reached_lobby"), Story.PartyInsideEntrance29(Hero.GetActorLocation()));
                ++Step;
            }
            if (Step == 12 && Story.ArrivalStage == 5)
            {
                Check(TEXT("guest_sees_same_shutter_and_doctor"), !Door.Open && Door.Locked &&
                    Mode->Completed.Contains(TEXT("SCENE_01_02")) && Story.ActiveScene35 == TEXT("SCENE_01_02"));
                HoldUntil = Now + 2.;
                Step = 13;
            }
            if (Step == 13 && Now >= HoldUntil && Mode->Shotgun && Mode->LoadedShells == 4 &&
                VisibleWeapon(*Lobby, false, false))
            {
                Check(TEXT("guest_sees_pistol_in_hosts_hand_and_own_shotgun"), WardGameplay76::Armed(*Mode) &&
                    !Mode->Flag(TEXT("weapon")) && Hero.Equipment.ContainsByPredicate([](const auto& Part) {
                        return Part && Part->IsVisible() && Part->ComponentTags.Contains(TEXT("shotgun"));
                    }));
                Hero.Aiming = true;
                Mode->FireWeapon();
                Step = 14;
            }
            if (Step == 14 && Mode->LoadedShells == 3)
            {
                Mode->ReloadWeapon();
                Step = 15;
            }
            if (Step == 15 && Mode->LoadedShells == 4 && Mode->Items.FindRef(TEXT("shells")) == 2)
            {
                WardGameplay76::Slot(*Mode, -1);
                Check(TEXT("guest_holstered"), !WardGameplay76::Armed(*Mode));
                HoldUntil = Now + 3.;
                Step = 16;
            }
            if (Step == 16 && Now >= HoldUntil)
            {
                WardGameplay76::Slot(*Mode, 0);
                Check(TEXT("guest_redrew_shotgun"), Mode->Shotgun && WardGameplay76::Armed(*Mode));
                HoldUntil = Now + 4.;
                Step = 17;
            }
            if (Step == 17 && Now >= HoldUntil) return Finish();
        }
        return true;
    }
};
}
