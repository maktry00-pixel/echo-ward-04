    struct FAvatarWeapon
    {
        TWeakObjectPtr<USceneComponent> Mount;
        ECarriedWeapon Type = ECarriedWeapon::None;
        int32 Finish = INDEX_NONE;
    };
    TMap<FString, FAvatarWeapon> AvatarWeapons;
    TArray<TArray<TWeakObjectPtr<UMaterialInterface>>> OriginalWeaponMaterials;
    bool ReceivedLoadout = false;

    static ECarriedWeapon ReadCarriedWeapon(const Json& Packet)
    {
        const double Value = Number(Packet, TEXT("held_weapon"));
        return Value == int32(ECarriedWeapon::Pistol) ? ECarriedWeapon::Pistol :
            Value == int32(ECarriedWeapon::Shotgun) ? ECarriedWeapon::Shotgun : ECarriedWeapon::None;
    }

    static ECarriedWeapon LocalCarriedWeapon(const AEWardCampaignMode& Game)
    {
        if (Game.Hero()->Dead || Game.HidingBed || Game.BookWorld->FurnitureHiding ||
            Game.HospitalStory->ArrivalCinematic() || !WardGameplay76::Armed(Game))
            return ECarriedWeapon::None;
        return Game.Shotgun ? ECarriedWeapon::Shotgun : ECarriedWeapon::Pistol;
    }

    void EnsurePartyLoadouts(AEWardCampaignMode& Game)
    {
        const auto& Story = *Game.HospitalStory;
        if (Story.HomeStage52 >= 0 || (Story.ArrivalStage >= 0 && Story.ArrivalStage < 4)) return;
        const bool FirstGrant = !Game.Flag(TEXT("online_loadout87"));
        bool Changed = FirstGrant;

        auto& Hero = *Game.Hero();
        if (FirstGrant && !Game.Flag(TEXT("weapon")) && !Game.Flag(TEXT("shotgun")))
        {
            Game.Flag(TEXT("weapon"), true);
            Hero.Rounds = 9;
            Hero.Reserve = ReservePerPlayer();
            Game.Shotgun = false;
        }
        if (FirstGrant) Game.Flag(TEXT("hands_empty76"), false);
        for (FMember& Member : Members)
        {
            if (Member.Host || Member.LoadoutReady) continue;
            Member.PreferShotgun = Member.Finish % 2 == 1;
            if (Member.PreferShotgun && !Member.Shotgun)
            {
                Member.Shotgun = true;
                Member.Shells = TeamSize <= 2 ? 4 : 3;
                Member.Items.FindOrAdd(TEXT("shells")) = FMath::Max(TeamSize <= 5 ? 3 : 2,
                    Member.Items.FindRef(TEXT("shells")));
            }
            else if (!Member.PreferShotgun && !Member.Pistol)
            {
                Member.Pistol = true;
                Member.Rounds = 9;
                Member.Reserve = FMath::Max(ReservePerPlayer(), Member.Reserve);
            }
            Member.LoadoutReady = true;
            Member.Items.FindOrAdd(TEXT("bottle")) = FMath::Max(1,Member.Items.FindRef(TEXT("bottle")));
            Changed = true;
        }
        if (Changed)
        {
            Game.Flag(TEXT("online_loadout87"), true);
            Game.AutoSave();
        }
    }

    static void TintWeapon(UStaticMeshComponent& Part, int32 SlotIndex, UMaterialInterface* Base, int32 Finish)
    {
        if (!Base || Finish == 0) return;
        static const FLinearColor Tints[] = {
            FLinearColor::White, {1.20,.79,.48}, {.61,.81,1.34}, {.61,1.12,.72}, {1.24,.56,.48},
            {.96,.72,1.20}, {1.18,1.05,.62}, {.53,1.09,1.10}, {1.29,1.24,1.17}, {.72,.68,1.08}
        };
        FLinearColor Color;
        const FName Parameter(TEXT("Tint"));
        if (!Base->GetVectorParameterValue(FMaterialParameterInfo(Parameter), Color)) return;
        if (auto* Paint = Part.CreateDynamicMaterialInstance(SlotIndex, Base))
        {
            const auto Shade = Tints[FMath::Clamp(Finish, 0, UE_ARRAY_COUNT(Tints) - 1)];
            Paint->SetVectorParameterValue(Parameter, FLinearColor(Color.R * Shade.R, Color.G * Shade.G,
                Color.B * Shade.B, Color.A));
        }
    }

    void ApplyLocalWeaponFinish(AEWardCampaignMode& Game, int32 Finish)
    {
        ClearLocalWeaponFinish(Game);
        if (Finish == 0) return;
        const auto& Parts = Game.Hero()->Equipment;
        OriginalWeaponMaterials.SetNum(Parts.Num());
        for (int32 Index = 0; Index < Parts.Num(); ++Index)
        {
            auto* Part = Parts[Index].Get();
            if (!Part || !Part->GetStaticMesh()) continue;
            const auto Materials = Part->GetMaterials();
            for (int32 SlotIndex = 0; SlotIndex < Materials.Num(); ++SlotIndex)
            {
                OriginalWeaponMaterials[Index].Add(Materials[SlotIndex]);
                TintWeapon(*Part, SlotIndex, Materials[SlotIndex], Finish);
            }
        }
    }

    void ClearLocalWeaponFinish(AEWardCampaignMode& Game)
    {
        if (auto* Hero = Game.Hero())
            for (int32 Index = 0; Index < FMath::Min(Hero->Equipment.Num(), OriginalWeaponMaterials.Num()); ++Index)
                if (auto* Part = Hero->Equipment[Index].Get())
                    for (int32 SlotIndex = 0; SlotIndex < OriginalWeaponMaterials[Index].Num(); ++SlotIndex)
                        Part->SetMaterial(SlotIndex, OriginalWeaponMaterials[Index][SlotIndex].Get());
        OriginalWeaponMaterials.Reset();
    }

    void UpdateAvatarWeapon(AEWardCampaignMode& Game, const FMember& Member, ASkeletalMeshActor& Avatar)
    {
        auto& State = AvatarWeapons.FindOrAdd(Member.Id);
        auto* Mount = State.Mount.Get();
        auto* Body = Avatar.GetSkeletalMeshComponent();
        if (Member.HeldWeapon == ECarriedWeapon::None)
        {
            if (Mount) Mount->SetVisibility(false, true);
            return;
        }
        const bool Long = Member.HeldWeapon == ECarriedWeapon::Shotgun;
        if (!Mount || State.Type != Member.HeldWeapon || State.Finish != Member.Finish)
        {
            FName Hand;
            for (const FName Candidate : {FName(TEXT("Bip01 R Hand")), FName(TEXT("Bip01-R-Hand")), FName(TEXT("hand_r"))})
                if (Body->DoesSocketExist(Candidate)) { Hand = Candidate; break; }
            if (Hand.IsNone()) return;
            if (Mount)
            {
                TArray<USceneComponent*> Parts;
                Mount->GetChildrenComponents(false, Parts);
                for (auto* Part : Parts) Part->DestroyComponent();
                Mount->DestroyComponent();
            }
            Mount = NewObject<USceneComponent>(&Avatar);
            Mount->SetupAttachment(Body, Hand);
            Mount->SetAbsolute(false, true, true);
            Mount->SetMobility(EComponentMobility::Movable);
            Mount->RegisterComponent();
            Avatar.AddInstanceComponent(Mount);
            State = {Mount, Member.HeldWeapon, Member.Finish};
            const auto& Hero = *Game.Hero();
            const FName WeaponTag(Long ? TEXT("shotgun") : TEXT("pistol"));
            for (int32 Index = 0; Index < Hero.Equipment.Num(); ++Index)
            {
                const auto* Source = Hero.Equipment[Index].Get();
                if (!Source || !Source->GetStaticMesh() || !Source->ComponentTags.Contains(WeaponTag) ||
                    !Source->ComponentTags.Contains(TEXT("authored_visible")))
                    continue;
                auto* Part = NewObject<UStaticMeshComponent>(&Avatar);
                Part->SetupAttachment(Mount);
                Part->SetMobility(EComponentMobility::Movable);
                Part->SetStaticMesh(Source->GetStaticMesh());
                Part->SetRelativeTransform(Hero.EquipmentRest[Index]);
                Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Part->SetGenerateOverlapEvents(false);
                Part->bDisallowNanite = Source->bDisallowNanite;
                Part->SetReceivesDecals(false);
                Part->SetCastShadow(false);
                Part->ComponentTags.Add(WeaponTag);
                for (int32 SlotIndex = 0; SlotIndex < Source->GetNumMaterials(); ++SlotIndex)
                {
                    UMaterialInterface* Base = OriginalWeaponMaterials.IsValidIndex(Index) &&
                        OriginalWeaponMaterials[Index].IsValidIndex(SlotIndex) ?
                        OriginalWeaponMaterials[Index][SlotIndex].Get() : Source->GetMaterial(SlotIndex);
                    Part->SetMaterial(SlotIndex, Base);
                    TintWeapon(*Part, SlotIndex, Base, Member.Finish);
                }
                Part->RegisterComponent();
                Avatar.AddInstanceComponent(Part);
            }
        }
        const FQuat Rotation = FRotator(Member.Aiming ? FMath::Clamp(Member.Pitch, -45.f, 45.f) : (Long ? -18.f : -40.f),
            Member.Yaw + (Long ? 5.f : 10.f), 0).Quaternion();
        const FVector Wrist = Body->GetSocketLocation(Mount->GetAttachSocketName());
        const FVector Grip = Long ? FVector(-6, 0, -10) : FVector(-5, 3, -10);
        Mount->SetWorldLocationAndRotation(Wrist - Rotation.RotateVector(Grip), Rotation);
        if (!Mount->IsVisible()) Mount->SetVisibility(true, true);
    }
