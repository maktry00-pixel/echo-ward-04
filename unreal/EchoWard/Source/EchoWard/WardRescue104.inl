    double LocalBleedUntil = 0, LastRescueTick = 0;
    float LocalRescueProgress = 0;
    FString LocalRescue, LocalRescuer;
    bool RescueTestHeld = false;

    static bool Fallen(const FMember& Member) { return Member.BleedUntil>0; }

    static void ApplyRescueStance(AEWardCampaignMode& Game,bool Down)
    {
        auto* Hero=Game.Hero();
        if(!Hero)return;
        auto* Movement=Hero->GetCharacterMovement();
        if(Down)
        {
            if(Game.BookWorld&&Game.BookWorld->FurnitureHiding) WardHideout102::Reset(*Game.BookWorld);
            Movement->SetCrouchedHalfHeight(WardRescue104::CrawlHalfHeight);
            Movement->MaxWalkSpeedCrouched=WardRescue104::CrawlSpeed;
            Hero->Aiming=false;
            Game.ReloadTime=0;
            Hero->Crouch();Movement->Crouch(false);
        }
        else
        {
            Movement->SetCrouchedHalfHeight(58);
            Movement->MaxWalkSpeedCrouched=140;
            Hero->UnCrouch();Movement->UnCrouch(false);
        }
    }

    void ApplyLocalRescue(AEWardCampaignMode& Game,double Until,float Progress,const FString& Rescuer)
    {
        if((LocalBleedUntil>0)!=(Until>0)) ApplyRescueStance(Game,Until>0);
        LocalBleedUntil=Until;LocalRescueProgress=Progress;LocalRescuer=Rescuer;
        if(Until>0)LocalRescue.Reset();
    }

    FString RescueInput(const AEWardCampaignMode& Game) const
    {
        if(LocalRescue.IsEmpty()||LocalBleedUntil>0||!Game.Screen.IsEmpty()||!Game.Hero()||Game.Hero()->Dead)return {};
        const auto* PC=Cast<APlayerController>(Game.Hero()->GetController());
        return RescueTestHeld||(PC&&PC->IsInputKeyDown(EKeys::F))?LocalRescue:FString();
    }

    void SyncHostRescue(AEWardCampaignMode& Game,double Now)
    {
        if(Members.IsEmpty())return;
        auto& Host=Members[0];
        auto* Hero=Game.Hero();
        Host.Position=Hero->GetActorLocation();Host.LastPose=Now;
        Host.Yaw=Hero->GetActorRotation().Yaw;
        Host.Health=Hero->Health;
        Host.RescueIntent=RescueInput(Game);Host.RescueLease=Now;
    }

    bool BeginDown(FMember& Member,double Now)
    {
        if(Fallen(Member))return false;
        Member.Health=0;Member.BleedUntil=Now+WardRescue104::BleedSeconds;Member.LastHit=Now;
        Member.RespawnAt=0;Member.ReloadAt=0;Member.RescueProgress=0;Member.RescueIntent.Reset();Member.Rescuer.Reset();
        Member.HeldWeapon=ECarriedWeapon::None;Member.Aiming=false;
        Member.Notice=TEXT("Ты тяжело ранен. Отползай в укрытие и жди помощи.");
        return true;
    }

    static void BleedHit(FMember& Member,double Now)
    {
        if(Fallen(Member)&&Now-Member.LastHit>.7)
        {
            Member.BleedUntil=FMath::Max(Now+.1,Member.BleedUntil-7.);
            Member.LastHit=Now;Member.RescueProgress=0;Member.Rescuer.Reset();
        }
    }

    void ExpireDown(AEWardCampaignMode& Game,FMember& Member,double Now)
    {
        Member.BleedUntil=0;Member.RescueProgress=0;Member.RescueIntent.Reset();Member.Rescuer.Reset();
        MarkDown(Member,Now);
        if(Member.Host)
        {
            auto* Hero=Game.Hero();ApplyLocalRescue(Game,0,0,{});
            Hero->Dead=true;Hero->Health=0;Hero->GetCharacterMovement()->DisableMovement();
        }
    }

    bool CanAid(AEWardCampaignMode& Game,FVector Helper,FVector Patient) const
    {
        if(FMath::Abs(Patient.Z-Helper.Z)>87||FVector::DistSquared2D(Helper,Patient)>FMath::Square(153.))return false;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(Rescue104Reach),false,Game.Hero());
        FHitResult Block;
        return !Game.GetWorld()->LineTraceSingleByChannel(Block,Helper+FVector(0,0,24),Patient+FVector(0,0,20),ECC_Visibility,Query);
    }

    bool CrawlPositionValid(AEWardCampaignMode& Game,const FMember& Member,FVector Previous,double Last,double Now) const
    {
        if(Last<=0)return false;
        const double Allowance=12.+92.*FMath::Clamp(Now-Last,.03,.85);
        if(FVector::DistSquared2D(Previous,Member.Position)>FMath::Square(Allowance)||FMath::Abs(Previous.Z-Member.Position.Z)>92)return false;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(Rescue104Crawl),false,Game.Hero());
        FHitResult Hit;
        return !Game.GetWorld()->SweepSingleByChannel(Hit,Previous+FVector(0,0,3),Member.Position+FVector(0,0,3),
            FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(27,37),Q);
    }

    void TickRescues(AEWardCampaignMode& Game,double Now)
    {
        const float Dt=FMath::Clamp(float(Now-LastRescueTick),0.f,.15f);
        LastRescueTick=Now;
        const bool CanWinHelp=Members.ContainsByPredicate([&](const auto& Entry){return Entry.Health>0&&!Entry.Disconnected&&(Entry.Host||Now-Entry.LastPose<2.3);});
        for(auto& Patient:Members)
        {
            if(!Fallen(Patient))continue;
            if(!CanWinHelp)Patient.BleedUntil=FMath::Min(Patient.BleedUntil,Now+5.);
            if(Now>=Patient.BleedUntil){ExpireDown(Game,Patient,Now);continue;}
            const auto Suitable=[&](const FMember& Helper){return Helper.Health>0&&!Helper.Disconnected&&!Patient.Disconnected&&
                Now-Helper.RescueLease<.45&&Helper.RescueIntent==Patient.Id&&CanAid(Game,Helper.Position,Patient.Position);};
            FMember* Helper=Members.FindByPredicate([&](const FMember& Entry){return Entry.Id==Patient.Rescuer&&Suitable(Entry);});
            if(Helper&&(Helper->LastHit>Patient.RescueStarted || FVector::DistSquared2D(Helper->Position,Patient.RescueStart)>FMath::Square(38.)))Helper=nullptr;
            if(!Helper)
            {
                Patient.Rescuer.Reset();Patient.RescueProgress=0;
                Helper=Members.FindByPredicate(Suitable);
                if(!Helper){if(Patient.Host)ApplyLocalRescue(Game,Patient.BleedUntil,0,{});continue;}
                Patient.Rescuer=Helper->Id;Patient.RescueStart=Helper->Position;Patient.RescueStarted=Now;
            }
            Patient.RescueProgress=FMath::Min(1.f,Patient.RescueProgress+Dt/WardRescue104::HoldSeconds);
            if(Patient.RescueProgress<1)
            {
                if(Patient.Host)ApplyLocalRescue(Game,Patient.BleedUntil,Patient.RescueProgress,Helper->Name);
                continue;
            }
            Patient.BleedUntil=0;Patient.Health=40;Patient.ProtectedUntil=Now+1.45;
            Patient.Rescuer.Reset();Patient.RescueProgress=0;Patient.Notice=TEXT("Напарник помог тебе встать.");
            if(Patient.Host)
            {
                auto* Hero=Game.Hero();Hero->Health=40;Hero->Dead=false;Game.Invulnerable=FMath::Max(Game.Invulnerable,1.45f);
                ApplyLocalRescue(Game,0,0,{});
                Game.Notify(Patient.Notice,2.6f);
            }
            if(Helper->Host)Game.Notify(TEXT("Ты поднял напарника."),2.2f);
            Game.Cue(TEXT("scrape"),Patient.Position,.16f);
        }
    }

public:
    bool IsLocalDown() const {return InMap&&LocalBleedUntil>0;}

    void RegisterRescueHostHit(){if(InMap&&Role==ERole::Host&&!Members.IsEmpty())Members[0].LastHit=FPlatformTime::Seconds();}

    bool RescueUnsafeToSave() const
    {return InMap&&Members.ContainsByPredicate([](const auto& Entry){return Fallen(Entry)||Entry.RespawnAt>0;});}

    bool HurtDownHost(AEWardCampaignMode& Game,bool FellOut)
    {
        if(!InMap||Role!=ERole::Host||Members.Num()<2||!Game.Hero())return false;
        auto& Host=Members[0];const double Now=FPlatformTime::Seconds();
        if(FellOut){ExpireDown(Game,Host,Now);return true;}
        if(Fallen(Host)){BleedHit(Host,Now);return true;}
        if(BeginDown(Host,Now))
        {
            ApplyLocalRescue(Game,Host.BleedUntil,0,{});
            Game.Notify(Host.Notice,3.f);
        }
        return true;
    }

    WardRescue104::FPrompt RescuePrompt(AEWardCampaignMode& Game)
    {
        WardRescue104::FPrompt Result;
        auto* Hero=Game.Hero();
        if(!InMap||!Hero||Hero->Dead)return Result;
        const double Now=FPlatformTime::Seconds();
        if(IsLocalDown())
        {
            Result.LocalDown=true;Result.Remaining=FMath::Max(0.,LocalBleedUntil-Now);Result.Progress=LocalRescueProgress;
            Result.Name=LocalRescuer;Result.Helping=LocalRescueProgress>0;return Result;
        }
        const FMember* Patient=Members.FindByPredicate([&](const FMember& Entry){return Entry.Id==LocalRescue&&Fallen(Entry)&&!Entry.Disconnected;});
        if(!Patient)Patient=AidTarget(Game,true);
        if(!Patient)Patient=AidTarget(Game,false);
        if(!Patient)return Result;
        Result.Name=Patient->Name;Result.Remaining=FMath::Max(0.,Patient->BleedUntil-Now);
        Result.Distance=FVector::Dist2D(Hero->GetActorLocation(),Patient->Position)/100;
        Result.CanHelp=CanAimAid(Game,*Patient);
        Result.Helping=Patient->Rescuer==Self;
        Result.Progress=Patient->RescueProgress;
        return Result;
    }

    bool BeginRescue(AEWardCampaignMode& Game)
    {
        if(IsLocalDown())return true;
        const auto* Patient=AidTarget(Game,true);
        if(!Patient)return false;
        LocalRescue=Patient->Id;
        return true;
    }

    void EndRescue(){LocalRescue.Reset();RescueTestHeld=false;}

    void TestRescueHold(FString Patient){LocalRescue=MoveTemp(Patient);RescueTestHeld=!LocalRescue.IsEmpty();}
    bool TestCanAid(AEWardCampaignMode& Game,FVector Helper,FVector Patient) const {return CanAid(Game,Helper,Patient);}

private:
    bool CanAimAid(AEWardCampaignMode& Game,const FMember& Patient) const
    {
        auto* Hero=Game.Hero();
        if(!CanAid(Game,Hero->GetActorLocation(),Patient.Position))return false;
        const FVector Direction=(Patient.Position+FVector(0,0,22)-Hero->Camera->GetComponentLocation()).GetSafeNormal();
        return FVector::DotProduct(Direction,Hero->GetControlRotation().Vector())>.43;
    }

    const FMember* AidTarget(AEWardCampaignMode& Game,bool OnlyReachable) const
    {
        const auto* Hero=Game.Hero();
        if(!InMap||!Hero||Hero->Dead||!Game.Screen.IsEmpty())return nullptr;
        const double Now=FPlatformTime::Seconds();
        const FMember* Closest=nullptr;
        double Best=FMath::Square(1700.);
        for(const auto& Entry:Members)
        {
            if(!Fallen(Entry)||Entry.Id==Self||Entry.Disconnected||Now-Entry.LastPose>2.3||
                FMath::Abs(Entry.Position.Z-Hero->GetActorLocation().Z)>190) continue;
            const double Dist=FVector::DistSquared2D(Entry.Position,Hero->GetActorLocation());
            if(Dist>=Best||(OnlyReachable&&!CanAimAid(Game,Entry)))continue;
            Best=Dist;Closest=&Entry;
        }
        return Closest;
    }
