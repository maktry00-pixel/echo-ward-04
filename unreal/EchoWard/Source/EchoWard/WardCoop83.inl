    FVector CoopStart = FVector::ZeroVector;
    TMap<FString, FString> MobGuests;
    TArray<TSharedPtr<FJsonValue>> PendingActions;
    int32 NextAction = 0, LastWorldSequence = 0, WorldSequence = 0;
    int32 LocalPoseFrame = 0, LastReceivedPose = 0;
    int32 IncomingWorldSequence = 0, IncomingWorldPart = 0;
    FString IncomingWorld;
    double WorldDeadline = 0;
    static constexpr int32 WorldChunkSize = 3600;
    static constexpr int32 MaxWorldChunks = 48;
    double NextWorld = 0;
    FString WorldWire;
    Json LastWorld;
    int32 TeamSize = 0, ReceivedRespawn = 0;
    int32 TrackedArrivalStage = -1;
    double NextPartyRefresh = 0;
    TMap<FString, int32> HeardShots;

    int32 ReservePerPlayer() const { return TeamSize <= 2 ? 9 : TeamSize <= 5 ? 7 : 5; }

    bool ValidPartyPosition(AEWardCampaignMode& Game, FVector At) const
    {
        const auto& Story = *Game.HospitalStory;
        return Story.HomeStage52 >= 0 ? Story.HomeContains52(At) : Game.BookWorld->ValidPosition(At);
    }

    static void PlaceMember(FMember& Member, FVector At)
    {
        Member.SeatPosition = Member.Position = At;
        Member.LastPose = 0;
        Member.GroupSpawn = true;
        ++Member.RespawnSerial;
    }

    void UpdatePrologueSeats(AEWardCampaignMode& Game)
    {
        const int32 Stage = Game.HospitalStory->ArrivalStage;
        if (Stage == 2 && TrackedArrivalStage < 2)
        {
            CoopStart = Game.Hero()->GetActorLocation();
            const auto Seats = FindSeats(Game, CoopStart);
            for (int32 Index = 1; Index < Members.Num(); ++Index)
                PlaceMember(Members[Index], Seats.IsValidIndex(Index) ? Seats[Index] : CoopStart);
        }
        TrackedArrivalStage = Stage;
    }

    int32 OutsideEntrance(AEWardCampaignMode& Game, double Now) const
    {
        const auto& Story = *Game.HospitalStory;
        int32 Outside = Game.Hero()->Dead || !Story.PartyInsideEntrance29(Game.Hero()->GetActorLocation()) ? 1 : 0;
        for (const auto& Member : Members)
            if (!Member.Host && (Member.Disconnected || Member.Health <= 0 || Member.LastPose <= 0 ||
                Now - Member.LastPose > 2.3 || !Story.PartyInsideEntrance29(Member.Position)))
                ++Outside;
        return Outside;
    }

    TArray<FVector> FindSeats(AEWardCampaignMode& Game, FVector Origin) const
    {
        TArray<FVector> Seats{Origin};
        auto* Hero = Game.Hero();
        auto* Capsule = Hero->GetCapsuleComponent();
        const auto Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius() + 2,
            Capsule->GetScaledCapsuleHalfHeight() - 2);
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(CoopSeats), false, Hero);
        const FName Profile = Capsule->GetCollisionProfileName();
        for (const float Radius : {100.f, 180.f, 260.f, 360.f, 500.f, 680.f})
        {
            for (int32 Direction = 0; Direction < 24 && Seats.Num() < TeamSize; ++Direction)
            {
                const float Angle = Direction * PI / 12;
                const FVector Candidate = Origin + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0);
                if (Seats.ContainsByPredicate([&](FVector At) { return FVector::DistSquared2D(At, Candidate) < FMath::Square(90.); }))
                    continue;
                FHitResult Floor, Obstacle;
                if (!Game.GetWorld()->LineTraceSingleByChannel(Floor, Candidate, Candidate - FVector(0, 0, 150),
                    ECC_WorldStatic, Query) || Floor.ImpactNormal.Z < .85 ||
                    FMath::Abs(Floor.ImpactPoint.Z + Capsule->GetScaledCapsuleHalfHeight() - Origin.Z) > 30)
                    continue;
                const FVector Standing = Floor.ImpactPoint + FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight() + 3);
                if (!ValidPartyPosition(Game, Standing) ||
                    Game.GetWorld()->OverlapBlockingTestByProfile(Standing, FQuat::Identity, Profile, Shape, Query))
                    continue;
                const bool Connected = Seats.ContainsByPredicate([&](FVector At) {
                    return FVector::DistSquared2D(At, Standing) < FMath::Square(260.) &&
                        !Game.GetWorld()->SweepSingleByProfile(Obstacle, At, Standing, FQuat::Identity, Profile, Shape, Query);
                });
                if (Connected) Seats.Add(Standing);
            }
        }
        return Seats;
    }

    void BalanceOpponents(AEWardCampaignMode& Game) const
    {
        if (!Game.HospitalStory) return;
        const float Scale = TeamSize <= 2 ? 1.f : TeamSize <= 5 ? 1.4f : 1.85f;
        for (const auto& Mob : Game.HospitalStory->Mobs)
        {
            if (!Mob || Mob->FirstChapter != 2) continue;
            Mob->MaxHealth = float(Number(Mob->Definition, TEXT("health"), 70)) * Scale;
            if (!Mob->Dead) Mob->Health = Mob->MaxHealth;
        }
    }

    static void RestoreOpponents(AEWardCampaignMode& Game)
    {
        if (!Game.HospitalStory) return;
        for (const auto& Mob : Game.HospitalStory->Mobs)
            if (Mob && Mob->FirstChapter == 2)
                Mob->MaxHealth = Number(Mob->Definition, TEXT("health"), 70);
    }

    static void MarkDown(FMember& Member, double Now)
    {
        Member.Health = 0;
        Member.LastHit = Now;
        Member.RespawnAt = Now + 3;
        Member.Moving = false;
    }

    void UpdateParty(AEWardCampaignMode& Game, double Now)
    {
        TickRescues(Game,Now);
        for (auto& Member : Members)
        {
            if (!Member.Host && Member.ReloadAt > 0 && Now >= Member.ReloadAt)
            {
                const auto Loaded = FMath::Min(9 - Member.Rounds, Member.Reserve);
                Member.Rounds += Loaded;
                Member.Reserve -= Loaded;
                Member.ReloadAt = 0;
            }
            if (Member.RespawnAt <= 0 || Now < Member.RespawnAt) continue;
            Member.RespawnAt = 0;
            Member.Health = 60;
            Member.Position = Member.SeatPosition;
            Member.LastPose = 0;
            Member.LastHit = Now;
            ++Member.RespawnSerial;
            Member.GroupSpawn = false;
            Member.Notice = TEXT("Ты вернулся к началу главы.");
            if(Member.Host)
            {
                auto* Hero=Game.Hero();Hero->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Hero->Dead=false;Hero->Health=60;
                Hero->SetActorLocation(Member.SeatPosition,false,nullptr,ETeleportType::TeleportPhysics);
                Hero->MotionInitialized52=false;Game.Invulnerable=2;Game.Notify(Member.Notice,2.7f);
            }
        }
        if (Game.HospitalStory && Now >= NextPartyRefresh)
        {
            NextPartyRefresh = Now + 1.1;
            WardMonster59::Refresh(*Game.HospitalStory);
        }
    }

    static void PlayRemoteShot(AEWardCampaignMode& Game, FVector At, bool Long)
    {
        const auto File = FPaths::ProjectContentDir() / (Long ?
            TEXT("CampaignData/Combat63/audio/shotgun.wav") : TEXT("CampaignData/Combat63/audio/pistol.wav"));
        WardHomeAssets::PlaySpatialFile61(&Game, File, .48f * Game.Setting(TEXT("effects"), .8), At);
    }

    static bool IsPersonalFlag(const FString& Key)
    {
        return Key.StartsWith(TEXT("combat63_")) || Key == TEXT("weapon") || Key == TEXT("shotgun") ||
            Key == TEXT("flash_cooldown") || Key == TEXT("hands_empty76");
    }

    static bool KeyItem(const FString& Id)
    {
        return Id == TEXT("fuse") || Id == TEXT("badge") || Id == TEXT("residence_prybar") ||
            Id == TEXT("isolation_tag") || Id == TEXT("fuel") || Id == TEXT("coolant");
    }

    static TArray<TSharedPtr<FJsonValue>> Position(const FVector& Point)
    {
        return {MakeShared<FJsonValueNumber>(FMath::RoundToInt(Point.X)),
            MakeShared<FJsonValueNumber>(FMath::RoundToInt(Point.Y)),
            MakeShared<FJsonValueNumber>(FMath::RoundToInt(Point.Z))};
    }

    static FString Pack(const Json& Data)
    {
        FString Wire;
        if (!WriteWire(Data, Wire)) return {};
        const FTCHARToUTF8 Raw(*Wire);
        if (Raw.Length() <= 0 || Raw.Length() > 128 * 1024) return {};
        TArray<uint8> Bytes;
        int32 Size = FCompression::CompressMemoryBound(NAME_Zlib, Raw.Length());
        Bytes.SetNumUninitialized(Size + 4);
        const uint32 SizeLE = Raw.Length();
        FMemory::Memcpy(Bytes.GetData(), &SizeLE, 4);
        if (!FCompression::CompressMemory(NAME_Zlib, Bytes.GetData() + 4, Size, Raw.Get(), Raw.Length())) return {};
        Bytes.SetNum(Size + 4);
        return FBase64::Encode(Bytes);
    }

    static Json Unpack(const FString& Wire)
    {
        TArray<uint8> Packed;
        if (Wire.Len() > WorldChunkSize * MaxWorldChunks || !FBase64::Decode(Wire, Packed) || Packed.Num() <= 4) return {};
        uint32 Size = 0;
        FMemory::Memcpy(&Size, Packed.GetData(), 4);
        if (!Size || Size > 128 * 1024) return {};
        TArray<uint8> Raw;
        Raw.SetNumUninitialized(Size);
        if (!FCompression::UncompressMemory(NAME_Zlib, Raw.GetData(), Raw.Num(), Packed.GetData() + 4, Packed.Num() - 4)) return {};
        const FUTF8ToTCHAR Text(reinterpret_cast<const char*>(Raw.GetData()), Raw.Num());
        return ReadWire(FString(Text.Length(), Text.Get()));
    }

    Json CaptureWorld(AEWardCampaignMode& Game)
    {
        auto& Story = *Game.HospitalStory;
        auto& Book = *Game.BookWorld;
        const auto Data = MakeShared<FJsonObject>();
        Data->SetNumberField(TEXT("chapter"), Story.Chapter());
        Data->SetNumberField(TEXT("step"), Game.Values.FindRef(TEXT("hospital_step")));
        Data->SetStringField(TEXT("scene"), Story.ActiveScene35);
        Data->SetStringField(TEXT("choice"), Story.SelectedChoice35);
        Data->SetNumberField(TEXT("page"), Story.DialoguePage35);
        Data->SetStringField(TEXT("subtitle"), Story.DialogueKey62);
        Data->SetNumberField(TEXT("arrival"), Story.ArrivalStage);
        Data->SetNumberField(TEXT("drive_clock"), Story.ArrivalStage == 0 ? Story.ArrivalClock : 0.f);
        Data->SetNumberField(TEXT("home"), Story.HomeStage52);
        Data->SetStringField(TEXT("home_subtitle"), Story.HomeSubtitle69);
        Data->SetNumberField(TEXT("outside"), Story.EntranceWaiting29);
        auto Lift = MakeShared<FJsonObject>();
        Lift->SetNumberField(TEXT("z"), Book.LiftZ);
        Lift->SetNumberField(TEXT("floor"), Book.LiftFloor);
        Lift->SetNumberField(TEXT("target"), Book.LiftTarget);
        Lift->SetNumberField(TEXT("door"), Book.LiftDoor);
        Lift->SetBoolField(TEXT("moving"), Book.LiftMoving);
        Data->SetObjectField(TEXT("lift"), Lift);
        Data->SetObjectField(TEXT("story"), Story.NarrativeState35);

        auto Flags = MakeShared<FJsonObject>();
        for (const auto& Entry : Game.Flags->Values)
            if (!IsPersonalFlag(FString(Entry.Key))) Flags->SetField(Entry.Key, Entry.Value);
        Data->SetObjectField(TEXT("flags"), Flags);
        TArray<TSharedPtr<FJsonValue>> Done, Journal;
        for (const auto& Id : Game.Completed) Done.Add(MakeShared<FJsonValueString>(Id));
        for (const auto& Note : Game.Journal) Journal.Add(MakeShared<FJsonValueString>(Note));
        Data->SetArrayField(TEXT("done"), Done);
        Data->SetArrayField(TEXT("journal"), Journal);
        const auto Keys = MakeShared<FJsonObject>();
        for (const auto& Item : Game.Items)
            if (KeyItem(Item.Key)) Keys->SetNumberField(Item.Key, Item.Value);
        Data->SetObjectField(TEXT("keys"), Keys);

        TArray<uint8> Doors;
        const auto Handles = MakeShared<FJsonObject>();
        Doors.Reserve(Book.Portals.Num());
        for (int32 Index=0; Index<Book.Portals.Num(); ++Index)
        {
            const auto& Door = Book.Portals[Index];
            Doors.Add((Door.Open ? 1 : 0) | (Door.Locked ? 2 : 0) |
                (Door.InsideOnly ? 4 : 0) | (Book.RefugeBolts.Contains(Door.Zone) ? 8 : 0));
            const float Age = WardDoor100::CheckAge(Book,Door.At);
            if (Age>=0 && WardDoor100::HasLever(Door) && !Door.Open) Handles->SetNumberField(FString::FromInt(Index),Age);
        }
        Data->SetStringField(TEXT("doors"), FBase64::Encode(Doors));
        if (!Handles->Values.IsEmpty()) Data->SetObjectField(TEXT("handle_checks"),Handles);
        const auto Carts = MakeShared<FJsonObject>();
        for (const auto& Prop : Book.Movables)
        {
            const FVector Offset = Prop.Body->GetRelativeLocation() - Prop.Home;
            if (!Offset.IsNearlyZero(.5f)) Carts->SetArrayField(Prop.Id, Position(Offset));
        }
        Data->SetObjectField(TEXT("moved_props"), Carts);

        TArray<TSharedPtr<FJsonValue>> Monsters;
        for (const auto& Mob : Story.Mobs)
        {
            if (!IsValid(Mob) || (!Mob->Active && !Mob->BodyPresent59 && !Mob->Dead)) continue;
            const auto Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("id"), Mob->Id);
            Row->SetArrayField(TEXT("at"), Position(Mob->GetActorLocation()));
            Row->SetNumberField(TEXT("yaw"), FMath::RoundToInt(Mob->GetActorRotation().Yaw));
            Row->SetNumberField(TEXT("hp"), FMath::RoundToInt(Mob->Health));
            Row->SetStringField(TEXT("phase"), Mob->State59);
            Row->SetNumberField(TEXT("speed"), FMath::RoundToInt(Mob->GetVelocity().Size2D()));
            Row->SetNumberField(TEXT("clock"), Mob->StateClock59);
            Row->SetNumberField(TEXT("stun"), Mob->Stunned);
            Row->SetBoolField(TEXT("dead"), Mob->Dead);
            Row->SetBoolField(TEXT("active"), Mob->Active);
            Row->SetBoolField(TEXT("visible"), !Mob->IsHidden());
            Row->SetNumberField(TEXT("strike"), Mob->StrikeVisual);
            if (Mob->Id.StartsWith(TEXT("CHILD_"))) Row->SetObjectField(TEXT("definition"), Mob->Definition);
            Monsters.Add(MakeShared<FJsonValueObject>(Row));
        }
        Data->SetArrayField(TEXT("mobs"), Monsters);
        const auto People = MakeShared<FJsonObject>();
        for (const auto& Entry : Story.Props)
        {
            auto* Actor = Entry.Value;
            if (!IsValid(Actor) || String(Story.Definitions.FindRef(Entry.Key), TEXT("kind")) != TEXT("person")) continue;
            const auto At = Actor->GetActorLocation();
            const auto* Human = Actor->FindComponentByClass<UEWardPerson54>();
            if (!Human) continue;
            const int32 PoseFlags = (Actor->IsHidden() ? 1 : 0) | (Human->IsDead58() ? 2 : 0) |
                (Human->IsFallen58() ? 4 : 0);
            auto Row = Position(At);
            Row.Add(MakeShared<FJsonValueNumber>(FMath::RoundToInt(Human->GetComponentRotation().Yaw)));
            Row.Add(MakeShared<FJsonValueNumber>(PoseFlags));
            People->SetArrayField(Entry.Key, Row);
        }
        Data->SetObjectField(TEXT("people"), People);
        return Data;
    }

    int32 WorldPartCount() const { return FMath::DivideAndRoundUp(WorldWire.Len(), WorldChunkSize); }

    void AdvanceWorldSnapshot(AEWardCampaignMode& Game)
    {
        if (!Game.HospitalStory || !Game.BookWorld) return;
        const double Now = FPlatformTime::Seconds();
        if (Now < NextWorld) return;
        if (WorldPartCount() > 1 && Now < WorldDeadline && Members.ContainsByPredicate([&](const auto& Member) {
            return !Member.Host && !Member.Disconnected && Now - Member.LastSeen < 4 &&
                (Member.WorldAck != WorldSequence || Member.WorldPart < WorldPartCount());
        })) return;
        NextWorld = Now + (TeamSize >= 8 ? .28 : .18);
        const FString Wire = Pack(CaptureWorld(Game));
        if (Wire.IsEmpty() || Wire.Len() > WorldChunkSize * MaxWorldChunks) return;
        WorldWire = Wire;
        WorldDeadline = Now + 20;
        ++WorldSequence;
    }

    void AppendWorld(const Json& Packet, const FMember& Member) const
    {
        const int32 Count = WorldPartCount();
        const int32 Part = Member.WorldAck == WorldSequence ? Member.WorldPart : 0;
        Packet->SetNumberField(TEXT("world_seq"), WorldSequence);
        Packet->RemoveField(TEXT("world"));
        if (Part >= Count) return;
        Packet->SetNumberField(TEXT("part"), Part);
        Packet->SetNumberField(TEXT("parts"), Count);
        Packet->SetStringField(TEXT("world"), WorldWire.Mid(Part * WorldChunkSize, WorldChunkSize));
    }

    void WriteWorldAck(const Json& Packet) const
    {
        Packet->SetNumberField(TEXT("world_ack"), IncomingWorldSequence);
        Packet->SetNumberField(TEXT("world_next"), IncomingWorldPart);
    }

    void ReadWorldAck(FMember& Member, const Json& Packet)
    {
        const double Sequence = Number(Packet, TEXT("world_ack"));
        const double Next = Number(Packet, TEXT("world_next"));
        if (Sequence != WorldSequence || !FMath::IsFinite(Next) || Next < 0 || Next > WorldPartCount()) return;
        Member.WorldPart = Member.WorldAck == WorldSequence ? FMath::Max(Member.WorldPart, int32(Next)) : int32(Next);
        Member.WorldAck = WorldSequence;
    }

    void ReceiveWorldPiece(const Json& Packet)
    {
        const auto Piece = String(Packet, TEXT("world"));
        Packet->RemoveField(TEXT("world"));
        const int32 Part = Number(Packet, TEXT("part"), -1), Count = Number(Packet, TEXT("parts"));
        const int32 Sequence = Number(Packet, TEXT("world_seq"));
        if (Piece.IsEmpty() || Piece.Len() > WorldChunkSize || Count < 1 || Count > MaxWorldChunks ||
            Part < 0 || Part >= Count || Sequence < IncomingWorldSequence) return;
        if (Sequence > IncomingWorldSequence)
        {
            if (Part != 0) return;
            IncomingWorldSequence = Sequence;
            IncomingWorldPart = 0;
            IncomingWorld.Reset();
        }
        if (Part != IncomingWorldPart) return;
        IncomingWorld += Piece;
        if (++IncomingWorldPart == Count) Packet->SetStringField(TEXT("world"), IncomingWorld);
    }

    static Json Personal(const FMember& Member)
    {
        const auto Data = MakeShared<FJsonObject>();
        Data->SetNumberField(TEXT("health"), Member.Health);
        if(Fallen(Member))
        {
            Data->SetNumberField(TEXT("bleed"),FMath::Max(.01,Member.BleedUntil-FPlatformTime::Seconds()));
            Data->SetNumberField(TEXT("revive"),Member.RescueProgress);
            Data->SetStringField(TEXT("rescuer"),Member.Rescuer);
        }
        Data->SetNumberField(TEXT("rounds"), Member.Rounds);
        Data->SetNumberField(TEXT("reserve"), Member.Reserve);
        Data->SetNumberField(TEXT("shells"), Member.Shells);
        Data->SetBoolField(TEXT("pistol"), Member.Pistol);
        Data->SetBoolField(TEXT("shotgun"), Member.Shotgun);
        Data->SetBoolField(TEXT("kit"), Member.LoadoutReady);
        Data->SetBoolField(TEXT("prefer_long"), Member.PreferShotgun);
        Data->SetNumberField(TEXT("ack"), Member.Action);
        if (Member.RespawnSerial > 0)
        {
            Data->SetNumberField(TEXT("respawn"), Member.RespawnSerial);
            if (Member.ConfirmedRespawn < Member.RespawnSerial)
            {
                Data->SetArrayField(TEXT("spawn"), Position(Member.SeatPosition));
                Data->SetBoolField(TEXT("group_spawn"), Member.GroupSpawn);
            }
        }
        Data->SetStringField(TEXT("notice"), Member.Notice);
        const auto Items = MakeShared<FJsonObject>();
        for (const auto& Item : Member.Items) Items->SetNumberField(Item.Key, Item.Value);
        Data->SetObjectField(TEXT("items"), Items);
        return Data;
    }

    void ApplyPersonal(AEWardCampaignMode& Game, const Json& Data)
    {
        auto* Hero = Game.Hero();
        if (!Hero || !Data->HasField(TEXT("health"))) return;
        const auto Ack = static_cast<int32>(Number(Data, TEXT("ack")));
        if (Ack < LastAck) return;
        if (Ack > LastAck)
        {
            const auto Notice = String(Data, TEXT("notice"));
            if (!Notice.IsEmpty()) Game.Notify(Notice, 2.2f);
        }
        LastAck = Ack;
        PendingActions.RemoveAll([Ack](const auto& Value) {
            return Number(Value->AsObject(), TEXT("seq")) <= Ack;
        });
        const float Health = FMath::Clamp(float(Number(Data, TEXT("health"), 100)), 0.f, 100.f);
        if (Health < Hero->Health) WardCombat63::Wound(Game, Hero->Health - Health, nullptr);
        const bool WasDead = Hero->Dead;
        const bool WasDown = IsLocalDown();
        const float Bleed=FMath::Clamp(float(Number(Data,TEXT("bleed"))),0.f,WardRescue104::BleedSeconds);
        Hero->Health = Health;
        Hero->Dead = Health <= 0&&Bleed<=0;
        if (Hero->Dead != WasDead)
        {
            Hero->GetCharacterMovement()->SetMovementMode(Hero->Dead ? MOVE_None : MOVE_Walking);
        }
        const FString RescuerId=String(Data,TEXT("rescuer"));
        const auto* Rescuer=Members.FindByPredicate([&](const auto& Entry){return Entry.Id==RescuerId;});
        ApplyLocalRescue(Game,Bleed>0?FPlatformTime::Seconds()+Bleed:0,FMath::Clamp(float(Number(Data,TEXT("revive"))),0.f,1.f),Rescuer?Rescuer->Name:FString());
        if(WasDown&&Bleed<=0&&Health>0)Game.Notify(TEXT("Напарник помог тебе встать."),2.6f);
        const int32 Respawn = static_cast<int32>(Number(Data, TEXT("respawn")));
        if (Respawn > ReceivedRespawn && Array(Data, TEXT("spawn")).Num() == 3)
        {
            const FVector Spawn = Vector(Array(Data, TEXT("spawn")), CoopStart);
            const auto& Story = *Game.HospitalStory;
            const bool Group = Boolean(Data, TEXT("group_spawn"));
            if (!Group || ((Story.HomeStage52 >= 0) == Story.HomeContains52(Spawn) && !Story.ArrivalCinematic()))
            {
                ReceivedRespawn = Respawn;
                Hero->GetCharacterMovement()->StopMovementImmediately();
                Hero->SetActorLocation(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
                Hero->MotionInitialized52 = false;
                if (!Group) Game.Notify(TEXT("Ты вернулся к началу главы."), 3);
            }
        }
        Hero->Rounds = static_cast<int32>(Number(Data, TEXT("rounds")));
        Hero->Reserve = static_cast<int32>(Number(Data, TEXT("reserve")));
        Game.LoadedShells = static_cast<int32>(Number(Data, TEXT("shells")));
        const bool Pistol = Boolean(Data, TEXT("pistol")), Shotgun = Boolean(Data, TEXT("shotgun"));
        Game.Flag(TEXT("weapon"), Pistol);
        Game.Flag(TEXT("shotgun"), Shotgun);
        if (!ReceivedLoadout && Boolean(Data, TEXT("kit")))
        {
            ReceivedLoadout = true;
            Game.Shotgun = Shotgun && (!Pistol || Boolean(Data, TEXT("prefer_long")));
            Game.Flag(TEXT("hands_empty76"), false);
            Game.HeldSlot76.Empty();
            Game.ClearPreview51();
        }
        if (Game.Shotgun && !Shotgun) Game.Shotgun = false;
        if (!Pistol && Shotgun) Game.Shotgun = true;
        for (auto It = Game.Items.CreateIterator(); It; ++It)
            if (!KeyItem(It.Key())) It.RemoveCurrent();
        for (const auto& Item : Object(Data, TEXT("items"))->Values)
            Game.Items.Add(FString(Item.Key), int32(Item.Value->AsNumber()));
    }

    void ApplyMobs(AEWardCampaignMode& Game, const Json& Data)
    {
        auto& Story = *Game.HospitalStory;
        TSet<FString> Seen;
        for (const auto& Value : Array(Data, TEXT("mobs")))
        {
            const auto Row = Value->AsObject();
            const auto Id = String(Row, TEXT("id"));
            auto* Found = Story.Mobs.FindByPredicate([&](const auto& Mob) { return Mob && Mob->Id == Id; });
            auto* Mob = Found ? Found->Get() : (Id.StartsWith(TEXT("CHILD_")) && Row->HasField(TEXT("definition")) ?
                Story.SpawnMob(Object(Row, TEXT("definition"))) : nullptr);
            if (!Mob) continue;
            Seen.Add(Id);
            const bool PreviouslyDead = Mob->Dead;
            Mob->Active = Boolean(Row, TEXT("active"));
            Mob->Dead = Boolean(Row, TEXT("dead"));
            Mob->BodyPresent59 = Mob->Dead && Boolean(Row, TEXT("visible"));
            const auto State = String(Row, TEXT("phase"));
            if (Mob->State59 != State && (State == TEXT("windup") || State == TEXT("recovery") || State == TEXT("door_check")))
                Mob->Sound61(State == TEXT("windup") ? TEXT("attack") : TEXT("impact"), .4f);
            Mob->State59 = State;
            Mob->Health = Number(Row, TEXT("hp"));
            Mob->Definition->SetNumberField(TEXT("_coop_speed"), Number(Row, TEXT("speed")));
            Mob->StateClock59 = Number(Row, TEXT("clock"));
            Mob->Stunned = Number(Row, TEXT("stun"));
            Mob->StrikeVisual = Number(Row, TEXT("strike"));
            Mob->SetActorLocationAndRotation(Vector(Array(Row, TEXT("at"))),
                FRotator(0, Number(Row, TEXT("yaw")), 0), false, nullptr, ETeleportType::TeleportPhysics);
            Mob->SetActorHiddenInGame(!Boolean(Row, TEXT("visible")));
            Mob->SetActorEnableCollision(Mob->Active && !Mob->Dead);
            if (Mob->Dead && !PreviouslyDead)
            {
                Mob->EnsureVisual61();
                if (Mob->Visual59) Mob->Visual59->BeginFall61(-Mob->GetActorForwardVector());
            }
        }
        for (const auto& Mob : Story.Mobs)
            if (Mob && !Seen.Contains(Mob->Id))
            {
                Mob->Active = false;
                Mob->SetActorHiddenInGame(true);
                Mob->SetActorEnableCollision(false);
            }
    }

    void ApplyWorld(AEWardCampaignMode& Game, const Json& Packet)
    {
        if (!Game.HospitalStory || !Game.BookWorld) return;
        ApplyPersonal(Game, Object(Packet, TEXT("personal")));
        const int32 Sequence = static_cast<int32>(Number(Packet, TEXT("world_seq")));
        if (Sequence <= LastWorldSequence) return;
        const auto Data = Unpack(String(Packet, TEXT("world")));
        if (!Data) return;
        TArray<uint8> Doors;
        auto& Book = *Game.BookWorld;
        if (!FBase64::Decode(String(Data, TEXT("doors")), Doors) || Doors.Num() != Book.Portals.Num()) return;
        auto& Story = *Game.HospitalStory;
        const int32 Chapter = static_cast<int32>(Number(Data, TEXT("chapter")));
        if (Chapter < 1 || Chapter > 20) return;
        LastWorldSequence = Sequence;
        const bool ChangedChapter = Story.Chapter() != Chapter;
        const int32 Step = static_cast<int32>(Number(Data, TEXT("step")));
        const bool ChangedProgress = !LastWorld || ChangedChapter || Game.Values.FindRef(TEXT("hospital_step")) != Step ||
            Game.Completed.Num() != Array(Data, TEXT("done")).Num();
        for (const auto& Entry : Object(Data, TEXT("flags"))->Values)
            if (!IsPersonalFlag(FString(Entry.Key))) Game.Flags->SetField(Entry.Key, Entry.Value);
        Game.Completed.Reset();
        for (const auto& Value : Array(Data, TEXT("done"))) Game.Completed.Add(Value->AsString());
        if (!ContactKey98.IsEmpty())
        {
            if (FPlatformTime::Seconds() > ContactKeyExpiry98 || ChangedChapter) ContactKey98.Empty();
            else if (Game.Completed.Contains(ContactKey98))
            {
                if (auto* Item = Story.Props.FindRef(ContactKey98); IsValid(Item) && !Item->IsHidden())
                    WardContact98::KeyPickup(*Game.Hero(), *Item);
                ContactKey98.Empty();
            }
        }
        Game.Journal.Reset();
        for (const auto& Value : Array(Data, TEXT("journal"))) Game.Journal.Add(Value->AsString());
        for (auto It = Game.Items.CreateIterator(); It; ++It)
            if (KeyItem(It.Key())) It.RemoveCurrent();
        for (const auto& Entry : Object(Data, TEXT("keys"))->Values)
            Game.Items.Add(FString(Entry.Key), int32(Entry.Value->AsNumber()));
        Story.NarrativeState35 = Object(Data, TEXT("story"));
        Story.ActiveScene35 = String(Data, TEXT("scene"));
        Story.SelectedChoice35 = String(Data, TEXT("choice"));
        Story.DialoguePage35 = static_cast<int32>(Number(Data, TEXT("page")));
        ApplyDialogue(Story, String(Data, TEXT("subtitle")));
        if (ChangedChapter) Story.SetChapter(Chapter);
        Story.ApplyCoopHome52(FMath::Clamp(int32(Number(Data, TEXT("home"), -1)), -1, 2),
            String(Data, TEXT("home_subtitle")));
        Story.ApplyCoopArrival29(FMath::Clamp(int32(Number(Data, TEXT("arrival"), -1)), -1, 5),
            float(Number(Data, TEXT("drive_clock"))));
        Story.EntranceWaiting29 = FMath::Clamp(int32(Number(Data, TEXT("outside"))), 0, 10);
        ApplyPersonal(Game, Object(Packet, TEXT("personal")));
        Game.Values.Add(TEXT("hospital_step"), Step);
        if (ChangedProgress) Story.Refresh();
        const auto Lift = Object(Data, TEXT("lift"));
        if (Lift->HasField(TEXT("z")) && Book.LiftCabin)
        {
            const bool Riding = Book.InsideLift(Game.Hero()->GetActorLocation());
            const float Z = FMath::Clamp(float(Number(Lift, TEXT("z"))), -960.f, 1440.f);
            const float Raise = Z - Book.LiftZ;
            Book.LiftZ = Z;
            Book.LiftFloor = FMath::Clamp(int32(Number(Lift, TEXT("floor"))), 0, 5);
            Book.LiftTarget = FMath::Clamp(int32(Number(Lift, TEXT("target"))), 0, 5);
            Book.LiftMoving = Boolean(Lift, TEXT("moving"));
            Book.LiftDoor = FMath::Clamp(float(Number(Lift, TEXT("door"))), 0.f, 1.f);
            Book.LiftCabin->SetWorldLocation(FVector(5000, 700, Z));
            if (Riding && FMath::Abs(Raise) > .01)
                Game.Hero()->SetActorLocation(Game.Hero()->GetActorLocation() + FVector(0, 0, Raise));
        }
        Book.RefugeBolts.Reset();
        const int32 FocusedDoor = LastWorld && !ChangedChapter && Game.Screen.IsEmpty() ? WardGameplay76::Door(Book) : INDEX_NONE;
        for (int32 Index = 0; Index < Doors.Num(); ++Index)
        {
            auto& Door = Book.Portals[Index];
            const uint8 State = Doors[Index];
            Door.Locked = (State & 2) != 0;
            Door.InsideOnly = (State & 4) != 0;
            if (State & 8) Book.RefugeBolts.Add(Door.Zone);
            const bool Open = (State & 1) != 0;
            if (Open != Door.Open)
            {
                if (LastWorld && !ChangedChapter && Game.Screen.IsEmpty())
                {
                    const auto At = Game.Hero()->GetActorLocation();
                    WardContact98::BeginDoor(Book, Index, At, FocusedDoor == Index, Open);
                }
                Door.Open = Open;
                Book.RefreshPortalVisual(Door);
            }
        }
        const auto Handles = Object(Data,TEXT("handle_checks"));
        const auto PreviousHandles = Object(LastWorld,TEXT("handle_checks"));
        for (const auto& Entry : Handles->Values)
        {
            int32 Index = INDEX_NONE;
            if (!LexTryParseString(Index,*FString(Entry.Key)) || !Book.Portals.IsValidIndex(Index) || Entry.Value->Type!=EJson::Number) continue;
            const float Age = Entry.Value->AsNumber();
            if (!FMath::IsFinite(Age) || Age<0 || Age>=WardDoor100::Duration || PreviousHandles->HasField(Entry.Key)) continue;
            WardDoor100::Begin(Book,Book.Portals[Index],!ChangedChapter,Age);
        }
        if (Data->HasField(TEXT("moved_props")))
        {
            const auto Carts = Object(Data, TEXT("moved_props"));
            for (auto& Prop : Book.Movables)
            {
                const auto& Row = Array(Carts, Prop.Id);
                const FVector Offset = Row.Num() == 3 ? Vector(Row) : FVector::ZeroVector;
                if (Offset.ContainsNaN() || Offset.SizeSquared2D() > FMath::Square(40000.) || FMath::Abs(Offset.Z) > 6) continue;
                const auto At = Prop.Home + Offset;
                if (Prop.Body->GetRelativeLocation().Equals(At, .5f)) continue;
                if (LastWorld && !ChangedChapter) Game.Cue(TEXT("scrape"), At, .13f);
                Prop.Body->SetRelativeLocation(At);
            }
        }
        ApplyMobs(Game, Data);
        for (const auto& Entry : Object(Data, TEXT("people"))->Values)
        {
            const FString Id(Entry.Key);
            auto* Actor = Story.Props.FindRef(Id);
            if (!IsValid(Actor) || Entry.Value->Type != EJson::Array) continue;
            const auto& Row = Entry.Value->AsArray();
            auto* Human = Actor->FindComponentByClass<UEWardPerson54>();
            if (Row.Num() != 5 || !Human) continue;
            const auto At = Vector(Row);
            const int32 Flags = int32(Row[4]->AsNumber());
            const bool Dead = (Flags & 2) != 0, Fallen = (Flags & 4) != 0;
            auto Definition = Story.Definitions.FindRef(Id);
            if (Definition)
            {
                const double Distance = FVector::Dist(Actor->GetActorLocation(), At);
                Definition->SetBoolField(TEXT("_coop_walk"), Distance > 4 && Distance < 240);
            }
            Actor->SetActorLocation(At);
            Human->SetWorldRotation(FRotator(0, Row[3]->AsNumber(), 0));
            if (Human->IsDead58() != Dead || Human->IsFallen58() != Fallen)
            {
                const auto State = MakeShared<FJsonObject>();
                State->SetNumberField(TEXT("health"), Dead ? 0 : 100);
                State->SetBoolField(TEXT("dead"), Dead);
                State->SetBoolField(TEXT("fallen"), Fallen);
                Human->RestoreHuman58(State);
            }
            Actor->SetActorHiddenInGame((Flags & 1) != 0);
            Actor->SetActorEnableCollision((Flags & 1) == 0 && !Dead);
        }
        if (ChangedChapter) Game.Notify(Chapter < 20 ? FString::Printf(TEXT("Открыта глава %d"), Chapter) :
            TEXT("Кампания завершена"), 4);
        LastWorld = Data;
    }

    int32 LastAck = 0;
    FString ContactKey98;
    double ContactKeyExpiry98 = 0;

    bool Reach(AEWardCampaignMode& Game, const FMember& Member, FVector At, AActor* Prop, UPrimitiveComponent* Leaf)
    {
        if (FMath::Abs(At.Z - Member.Position.Z) > 150 || FVector::DistSquared2D(At, Member.Position) > FMath::Square(310.))
            return false;
        FHitResult Block;
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(CoopReach), false, Game.Hero());
        const FVector Eye = Member.Position + FVector(0, 0, 62);
        return !Game.GetWorld()->LineTraceSingleByChannel(Block, Eye, At, ECC_Visibility, Query) ||
            (Prop && Block.GetActor() == Prop) || (Leaf && Block.GetComponent() == Leaf);
    }

    void UseStory(AEWardCampaignMode& Game, FMember& Member, const FString& Id)
    {
        auto& Story = *Game.HospitalStory;
        auto* Prop = Story.Props.FindRef(Id);
        const auto Definition = Story.Definitions.FindRef(Id);
        if (!Prop || !Definition || Prop->IsHidden() || Game.Completed.Contains(Id) ||
            !Reach(Game, Member, Prop->GetActorLocation() + FVector(0, 0, 30), Prop, nullptr)) return;
        const auto Kind = String(Definition, TEXT("kind"));
        if (!Story.ActiveScene35.IsEmpty() && !ParallelPickup(Game, Id)) return;
        if (Kind == TEXT("scene") || Kind == TEXT("key") || Kind == TEXT("note") || Kind == TEXT("node") ||
            Kind == TEXT("person") || Kind == TEXT("story_object") || Kind == TEXT("monster_control59"))
        {
            Story.UseId(Id);
            return;
        }

        auto* Hero = Game.Hero();
        if (!Hero) return;
        {
            const TGuardValue<TMap<FString, int32>> Inventory(Game.Items, Member.Items);
            const TGuardValue<int32> Rounds(Hero->Rounds, Member.Rounds), Reserve(Hero->Reserve, Member.Reserve),
                Shells(Game.LoadedShells, Member.Shells);
            const bool HostPistol = Game.Flag(TEXT("weapon")), HostShotgun = Game.Flag(TEXT("shotgun"));
            Game.Flag(TEXT("weapon"), Member.Pistol);
            Game.Flag(TEXT("shotgun"), Member.Shotgun);
            Story.UseId(Id);
            Member.Items = Game.Items;
            Member.Rounds = Hero->Rounds;
            Member.Reserve = Hero->Reserve;
            Member.Shells = Game.LoadedShells;
            Member.Pistol = Game.Flag(TEXT("weapon"));
            Member.Shotgun = Game.Flag(TEXT("shotgun"));
            Game.Flag(TEXT("weapon"), HostPistol);
            Game.Flag(TEXT("shotgun"), HostShotgun);
        }
        if (Game.Completed.Contains(Id)) Member.Notice = String(Definition, TEXT("label"));
        else Member.Notice = TEXT("Не удалось подобрать предмет.");
    }

    void ResolveShot(AEWardCampaignMode& Game, FMember& Member, double Now, bool Knife, bool Long)
    {
        if (Now < Member.NextShot || Member.ReloadAt > 0) return;
        if (!Knife && (Long ? (!Member.Shotgun || Member.Shells <= 0) : (!Member.Pistol || Member.Rounds <= 0)))
        { Member.Notice = TEXT("Нужна перезарядка."); return; }
        Member.NextShot = Now + (Knife ? .65 : Long ? .85 : .32);
        const FVector At = Member.Position + FVector(0, 0, Knife ? 20 : 62);
        if (!Knife)
        {
            if (Long) --Member.Shells; else --Member.Rounds;
            ++Member.ShotsFired;
            Member.LastShotLong = Long;
            Member.ShotOrigin = At;
            PlayRemoteShot(Game, At, Long);
        }
        const FVector Direction = FRotator(Member.Pitch, Member.Yaw, 0).Vector();
        FHitResult Hit;
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(CoopShot), true);
        if (Knife) Game.Cue(TEXT("knife"), At, .5f);
        Game.BroadcastNoise(At, Knife ? 500 : Long ? 6500 : 5000);
        if (!Game.GetWorld()->LineTraceSingleByChannel(Hit, At, At + Direction * (Knife ? 170 : 6000), ECC_Visibility, Query))
            return;
        if (auto* Mob = Cast<AEWardHospitalMob>(Hit.GetActor()); Mob && Mob->Active && !Mob->Dead)
        {
            const float Half = Mob->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            const float Height = (Hit.ImpactPoint.Z - Mob->GetActorLocation().Z + Half) / (Half * 2);
            const float Damage = Knife ? 22.f : (Long ? 90.f : 34.f) *
                WardCombat63::RangeScale(Long, FVector::Dist(At, Hit.ImpactPoint)) * WardCombat63::BodyScale(Height);
            FPointDamageEvent Event(Damage, Hit, Direction, nullptr);
            Mob->TakeDamage(Event.Damage, Event, nullptr, Avatars.FindRef(Member.Id).Get());
            WardCombat63::HitBlood(Game, Hit.ImpactPoint, Direction, Knife ? .65f : 1.f);
        }
        else if (!Knife) Game.SurfaceImpact(Hit.ImpactPoint, Hit.ImpactNormal);
    }

    void ProcessActions(AEWardCampaignMode& Game, FMember& Member, const Json& Packet, double Now)
    {
        if (!Game.HospitalStory || !Game.BookWorld) return;
        const auto& Actions = Array(Packet, TEXT("actions"));
        for (int32 Index = 0; Index < FMath::Min(8, Actions.Num()); ++Index)
        {
            const auto Action = Actions[Index]->AsObject();
            const double WireSequence = Number(Action, TEXT("seq"));
            if (!FMath::IsFinite(WireSequence) || WireSequence <= Member.Action || WireSequence > Member.Action + 32.) continue;
            const int32 Sequence = static_cast<int32>(WireSequence);
            Member.Action = Sequence;
            Member.Notice.Reset();
            if (Member.Health <= 0 || Game.HospitalStory->Chapter() >= 20) continue;
            const auto Type = String(Action, TEXT("type"));
            auto& Story = *Game.HospitalStory;
            if (Story.HomeStage52 >= 0)
            {
                if (Type == TEXT("home")) Story.UseHomeAt52(Member.Position, false);
                continue;
            }
            if (Story.ArrivalStage == 0 || Story.ArrivalStage == 1)
            {
                if (Type == TEXT("car_exit") && Story.ArrivalStage == 1) Story.ArrivalInteract();
                continue;
            }
            if (Type == TEXT("checkpoint")) Story.UseCheckpoint29(Member.Position);
            else if (Type == TEXT("door"))
            {
                const double WireIndex = Number(Action, TEXT("index"), -1);
                if (!FMath::IsFinite(WireIndex) || WireIndex < 0 || WireIndex >= Game.BookWorld->Portals.Num()) continue;
                const int32 DoorIndex = static_cast<int32>(WireIndex);
                const auto& Door = Game.BookWorld->Portals[DoorIndex];
                if (Reach(Game, Member, Door.At + FVector(0, 0, 112), nullptr, Door.Leaf))
                    Member.Notice = OperateDoor(*Game.BookWorld, DoorIndex, Member.Position, Boolean(Action, TEXT("open"))) ?
                        (Door.Open ? TEXT("Дверь открыта") : TEXT("Дверь закрыта")) : TEXT("Дверь недоступна.");
            }
            else if (Type == TEXT("cart"))
            {
                const double CartIndex = Number(Action, TEXT("index"), -1);
                if (!FMath::IsFinite(CartIndex) || CartIndex < 0 || CartIndex >= Game.BookWorld->Movables.Num() || CartIndex != int32(CartIndex)) continue;
                auto& Prop = Game.BookWorld->Movables[int32(CartIndex)];
                if (Reach(Game, Member, Prop.Body->GetComponentLocation(), nullptr, Prop.Body))
                    WardContact98::PushCart(*Game.BookWorld, int32(CartIndex), Member.Position, Boolean(Action, TEXT("open")), false);
            }
            else if (Type == TEXT("use")) UseStory(Game, Member, String(Action, TEXT("id")));
            else if (Type == TEXT("lift"))
            {
                auto& Book = *Game.BookWorld;
                const auto At = Member.Position;
                const double Floor = Number(Action, TEXT("index"), -1);
                const bool Inside = Book.InsideLift(At);
                const bool AtCall = FMath::Abs(At.X - 5000) < 250 && At.Y >= 1000 && At.Y < 1350 &&
                    Floor == FMath::RoundToInt((At.Z - 98 + 960) / 480);
                if (Floor >= 0 && Floor <= 5 && (Inside || AtCall)) Book.RequestLift(int32(Floor));
            }
            else if (Type == TEXT("fire") || Type == TEXT("fire_long") || Type == TEXT("knife"))
                ResolveShot(Game, Member, Now, Type == TEXT("knife"), Type == TEXT("fire_long"));
            else if (Type == TEXT("reload") && Member.Pistol && Member.ReloadAt <= 0 && Member.Rounds < 9 && Member.Reserve > 0)
                Member.ReloadAt = Now + 1.8;
            else if (Type == TEXT("reload_long") && Member.Shotgun && Member.ReloadAt <= 0 &&
                Member.Shells < 5 && Member.Items.FindRef(TEXT("shells")) > 0)
            {
                if (--Member.Items[TEXT("shells")] == 0) Member.Items.Remove(TEXT("shells"));
                ++Member.Shells;
                Member.NextShot = FMath::Max(Member.NextShot, Now + .7);
            }
            else if (Type == TEXT("heal") && Member.Health < 100 && Member.Items.FindRef(TEXT("medkit")) > 0)
            {
                if (--Member.Items[TEXT("medkit")] == 0) Member.Items.Remove(TEXT("medkit"));
                Member.Health = FMath::Min(100.f, Member.Health + 35);
            }
            else if (Type == TEXT("bottle") && Member.Items.FindRef(TEXT("bottle")) > 0)
            {
                const auto Direction = FRotator(Member.Pitch, Member.Yaw, 0).Vector();
                const auto At = Member.Position + FVector(0, 0, 45);
                FHitResult Obstacle;
                const FCollisionQueryParams Query(SCENE_QUERY_STAT(CoopBottle), false);
                if (!Game.GetWorld()->SweepSingleByChannel(Obstacle, At, At + Direction * 75, FQuat::Identity,
                    ECC_Visibility, FCollisionShape::MakeSphere(12), Query))
                {
                    if (--Member.Items[TEXT("bottle")] == 0) Member.Items.Remove(TEXT("bottle"));
                    Game.SpawnPhysics(TEXT("bottle"), At + Direction * 75, Direction * 850 + FVector(0, 0, 260),
                        FVector(6, 6, 18), .45f);
                }
            }
        }
    }

public:
    int32 StartingTeamSize() const { return TeamSize; }
    Json ProbeWorld, ProbePersonal;
    int32 ProbePoses = 0, ProbeHeardShots = 0;

    void ProbeTick(FVector At, float Yaw, bool Shoot, const FString& Item = {})
    {
        if (DrivePages || Role != ERole::Guest || !Room.Committed) return;
        const double Now = FPlatformTime::Seconds();
        if (Now < NextPose && !Shoot && Item.IsEmpty()) return;
        NextPose = Now + .085;
        auto Packet = MemberPacket(TEXT("pose"));
        WritePose(Packet, At, Yaw, false);
        Packet->SetNumberField(TEXT("frame"), ++LocalPoseFrame);
        Packet->SetNumberField(TEXT("respawn_ack"), Number(ProbePersonal, TEXT("respawn")));
        WriteWorldAck(Packet);
        if (Shoot)
        {
            auto Shot = MakeShared<FJsonObject>();
            Shot->SetStringField(TEXT("type"), Boolean(ProbePersonal, TEXT("prefer_long")) ? TEXT("fire_long") : TEXT("fire"));
            Shot->SetNumberField(TEXT("seq"), ++NextAction);
            PendingActions.Add(MakeShared<FJsonValueObject>(Shot));
        }
        if (!Item.IsEmpty() && PendingActions.IsEmpty())
        {
            auto Use = MakeShared<FJsonObject>();
            Use->SetStringField(TEXT("type"), TEXT("use"));
            Use->SetStringField(TEXT("id"), Item);
            Use->SetNumberField(TEXT("seq"), ++NextAction);
            PendingActions.Add(MakeShared<FJsonValueObject>(Use));
        }
        Packet->SetArrayField(TEXT("actions"), PendingActions);
        Send(Packet, Room.Address);
    }

    bool ProbeUsedRound() const
    {
        return Boolean(ProbePersonal, TEXT("prefer_long")) ?
            Number(ProbePersonal, TEXT("shells")) == (Room.Players <= 2 ? 3 : 2) :
            Number(ProbePersonal, TEXT("rounds")) == 8;
    }

    void ProbeReceive(const Json& Packet)
    {
        ProbePersonal = Object(Packet, TEXT("personal"));
        ProbePoses = Array(Packet, TEXT("poses")).Num();
        ProbeHeardShots = 0;
        for (const auto& Row : Array(Packet, TEXT("poses")))
            ProbeHeardShots += int32(Number(Row->AsObject(), TEXT("shots")));
        const FString Wire = String(Packet, TEXT("world"));
        if (!Wire.IsEmpty())
            if (const auto Data = Unpack(Wire)) ProbeWorld = Data;
        const int32 Ack = int32(Number(ProbePersonal, TEXT("ack")));
        PendingActions.RemoveAll([Ack](const auto& Action) { return Number(Action->AsObject(), TEXT("seq")) <= Ack; });
    }

    int32 VisibleAvatars() const
    {
        int32 Count = 0;
        for (const auto& Avatar : Avatars)
            if (Avatar.Value.IsValid() && !Avatar.Value->IsHidden() && !Avatar.Value->GetActorEnableCollision()) ++Count;
        return Count;
    }

    int32 SafeSeatCount(AEWardCampaignMode& Game) const { return FindSeats(Game, CoopStart).Num(); }
    int32 EntranceWait(AEWardCampaignMode& Game) const { return OutsideEntrance(Game, FPlatformTime::Seconds()); }
    ASkeletalMeshActor* TestAvatar(const FString& Id) const { return Avatars.FindRef(Id).Get(); }

    double DistanceToParty(AEWardCampaignMode& Game, FVector At) const
    {
        const auto Score = [At](FVector Point) {
            return FVector::DistSquared(At, Point) +
                (FMath::Abs(At.Z - Point.Z) > 210 ? FMath::Square(2200.) : 0.);
        };
        double Best = Score(Game.Hero()->GetActorLocation());
        if (!InMap || Role != ERole::Host) return Best;
        if (Game.Hero()->Dead) Best = MAX_dbl;
        const double Now = FPlatformTime::Seconds();
        for (const auto& Member : Members)
            if (!Member.Host && Member.Health > 0 && Member.LastPose > 0 && Now - Member.LastPose < 2.3)
                Best = FMath::Min(Best, Score(Member.Position));
        return Best;
    }

    void LocalShot(FVector At, bool Long = false)
    {
        if (Role != ERole::Host || !InMap || Members.IsEmpty()) return;
        ++Members[0].ShotsFired;
        Members[0].LastShotLong = Long;
        Members[0].ShotOrigin = At;
    }

    bool HitRemote(AEWardHospitalMob& Mob, float Damage)
    {
        const auto* Id = MobGuests.Find(Mob.Id);
        if (!Id) return false;
        auto* Member = Members.FindByPredicate([&](const auto& Candidate) { return Candidate.Id == *Id; });
        const auto Now = FPlatformTime::Seconds();
        if (Member && Fallen(*Member) && Now-Member->LastPose<2.3)BleedHit(*Member,Now);
        else if (Member && Member->Health > 0 && Now>=Member->ProtectedUntil && Now - Member->LastHit > .55 && Now - Member->LastPose < 2.3)
        {
            Member->Health = FMath::Max(0.f, Member->Health - Damage);
            Member->LastHit = Now;
            if (Member->Health <= 0) BeginDown(*Member,Now);
        }
        // A windup against a departed guest cannot hit a different player.
        return true;
    }

    Json CheckWorld(AEWardCampaignMode& Game)
    {
        return CaptureWorld(Game);
    }

    FMember* TestingGuest()
    {
        return Members.FindByPredicate([](const FMember& Member) { return !Member.Host && Member.LastPose > 0; });
    }

    void TestingAimAtGuest(const AEWardHospitalMob& Mob, const FMember& Member)
    {
        MobGuests.Add(Mob.Id, Member.Id);
    }

    bool QueueAction(const TCHAR* Type, const FString& Id = {}, int32 Index = -1, bool Open = false)
    {
        if (!InMap || Role != ERole::Guest) return false;
        if (PendingActions.Num() >= 8) return true;
        auto Action = MakeShared<FJsonObject>();
        Action->SetStringField(TEXT("type"), Type);
        Action->SetNumberField(TEXT("seq"), ++NextAction);
        if (!Id.IsEmpty()) Action->SetStringField(TEXT("id"), Id);
        if (Index >= 0) { Action->SetNumberField(TEXT("index"), Index); Action->SetBoolField(TEXT("open"), Open); }
        PendingActions.Add(MakeShared<FJsonValueObject>(Action));
        return true;
    }

    void QueuePickup(const FString& Id, bool Key)
    {
        if (PendingActions.Num() >= 8 || !QueueAction(TEXT("use"), Id) || !Key) return;
        ContactKey98 = Id;
        ContactKeyExpiry98 = FPlatformTime::Seconds() + 1.4;
    }

    FMember* Threat(AEWardHospitalMob& Mob, bool Select)
    {
        if (!InMap || Role != ERole::Host || !Mob.Runtime || !Mob.Runtime->Mode->Hero()) return nullptr;
        if (Select && Mob.State59 != TEXT("windup"))
        {
            FMember* Best = nullptr;
            const FVector At = Mob.GetActorLocation();
            const double Now = FPlatformTime::Seconds();
            const auto* Hero = Mob.Runtime->Mode->Hero();
            const auto HostAt = Hero->GetActorLocation();
            const bool HostSameFloor = !Hero->Dead && FMath::Abs(HostAt.Z - At.Z) < 210;
            const bool HostVisible = HostSameFloor && !IsLocalDown() && Mob.Sight59(HostAt);
            double Range = HostVisible ? FVector::DistSquared2D(At, HostAt) : FMath::Square(1900.);
            double Fallback = HostSameFloor ? FVector::DistSquared2D(At, HostAt) : FMath::Square(1900.);
            FMember* Nearby = nullptr;
            FMember* NearbyFallen = nullptr;
            double NearbyFallenDistance=FMath::Square(400.);
            for (auto& Member : Members)
            {
                if (Member.Host || (Member.Health<=0&&!Fallen(Member)) || Now - Member.LastPose > 1.9 ||
                    FMath::Abs(Member.Position.Z - At.Z) > 205) continue;
                const double Distance = FVector::DistSquared2D(At, Member.Position);
                if(Fallen(Member))
                {
                    if(Distance<NearbyFallenDistance&&Mob.Sight59(Member.Position,true)){NearbyFallen=&Member;NearbyFallenDistance=Distance;}
                    continue;
                }
                if (Distance < Fallback) { Nearby = &Member; Fallback = Distance; }
                if (Distance >= Range || !Mob.Sight59(Member.Position, Distance < FMath::Square(100.))) continue;
                Range = Distance;
                Best = &Member;
            }
            if (!Best && !HostVisible) Best = Nearby?Nearby:NearbyFallen;
            if (!Best) { MobGuests.Remove(Mob.Id); return nullptr; }
            MobGuests.Add(Mob.Id, Best->Id);
        }
        const auto* Selected = MobGuests.Find(Mob.Id);
        return Selected ? Members.FindByPredicate([&](const auto& Member) { return Member.Id == *Selected; }) : nullptr;
    }

    bool Occupied(FVector At, bool NS, float Width) const
    {
        if (!InMap || Role != ERole::Host) return false;
        const auto Check = [&](FVector Player) {
            const FVector Offset = Player - At;
            return FMath::Abs(Offset.Z - 96) < 95 && FMath::Abs(NS ? Offset.Y : Offset.X) < 65 &&
                FMath::Abs(NS ? Offset.X : Offset.Y) <= Width * .5 + 45;
        };
        const auto* Game = Owner.Get();
        if (Game && Game->Hero() && Check(Game->Hero()->GetActorLocation())) return true;
        const double Now = FPlatformTime::Seconds();
        return Members.ContainsByPredicate([&](const auto& Member) {
            return !Member.Host && (Member.Health>0||Fallen(Member)) && Member.LastPose > 0 && Now - Member.LastPose < 2.3 &&
                Check(Member.Position);
        });
    }

private:
