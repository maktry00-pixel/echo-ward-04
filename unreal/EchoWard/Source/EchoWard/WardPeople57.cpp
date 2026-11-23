#include "WardPeople57.h"
#include "WardPeople54.h"
#include "WardHospitalGameplay.h"
#include "WardCampaign.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Misc/Paths.h"

using namespace WardData;
namespace WardPeople57
{
    Json Info(const FString& Id)
    {
        static const Json Data = Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People57/placements.json"));
        for (auto Entry : Array(Data,TEXT("people")))
            if (String(Entry->AsObject(),TEXT("id")) == Id) return Entry->AsObject();
        return nullptr;
    }

    void Append(AEWardHospitalGameplay& Story)
    {
        static const Json Data = Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People57/placements.json"));
        auto Rows = Array(Story.Data,TEXT("placements"));
        TSet<FString> Known;
        for (auto Row : Rows) Known.Add(String(Row->AsObject(),TEXT("id")));
        for (auto Row : Array(Data,TEXT("people")))
            if (!Known.Contains(String(Row->AsObject(),TEXT("id")))) Rows.Add(Row);
        Story.Data->SetArrayField(TEXT("placements"),Rows);
    }

    bool Replay(AEWardCampaignMode& Mode, const FString& Id)
    {
        auto* Story = Mode.HospitalStory.Get();
        const Json Description = Info(Id);
        if (!Story || !Mode.Hero() || !Boolean(Description,TEXT("past57"))) return false;
        auto* Actor = Story->Props.FindRef(Id);
        auto* Model = Actor ? Actor->FindComponentByClass<UEWardPerson54>() : nullptr;
        if (!Model) return false;
        // A recording is rendered in the current space. It cannot change plot flags.
        const FRotator Yaw(0,Mode.Hero()->GetControlRotation().Yaw,0);
        const FVector Origin = Mode.Hero()->GetActorLocation()-FVector(0,0,91);
        Actor->SetActorLocation(Origin+Yaw.Vector()*260.f);
        if (!Model->ResetPatrol57())
        {
            Actor->SetActorHiddenInGame(true);
            Mode.Notify(TEXT("Для сцены прошлого нужен свободный участок пола."),5);
            return true;
        }
        Story->Definitions[Id]->SetBoolField(TEXT("memory_active57"),true);
        Model->BeginMemory57();
        Actor->SetActorHiddenInGame(false);
        Actor->SetActorEnableCollision(true);
        // Keep the original chronology visible while the performance is playing.
        auto* Caption = Actor->FindComponentByClass<UTextRenderComponent>();
        if (!Caption)
        {
            Caption = NewObject<UTextRenderComponent>(Actor);
            Caption->SetupAttachment(Actor->GetRootComponent());
            Caption->SetRelativeLocation(FVector(0,0,Model->TargetHeight55()+25));
            Caption->SetHorizontalAlignment(EHTA_Center);
            Caption->SetWorldSize(12);
            Caption->SetTextRenderColor(FColor(130,205,211));
            Caption->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Actor->AddInstanceComponent(Caption);
            Caption->RegisterComponent();
        }
        Caption->SetWorldRotation((Mode.Hero()->GetActorLocation()-Actor->GetActorLocation()).Rotation());
        Caption->SetText(FText::FromString(TEXT("ДО АВАРИИ · ")+String(Description,TEXT("label"))));
        Mode.Show(TEXT(""));
        Mode.Notify(TEXT("СЦЕНА ПРОШЛОГО · ")+String(Description,TEXT("label"))+
            TEXT(" · F — слушать · запись длится минуту"),10);
        return true;
    }
}
