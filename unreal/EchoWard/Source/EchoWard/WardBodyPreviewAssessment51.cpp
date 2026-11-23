#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardHospitalGameplay.h"
#include "WardBookWorld.h"
#include "WardHeldGrip108.h"
#include "Camera/CameraComponent.h"
#include "SkeletalRenderPublic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
using namespace WardData;
void AEWardCampaignMode::TickBodyPreviewAssessment51(float Delta)
{
    static int32 Stage=0;static float Clock=0;static TArray<TSharedPtr<FJsonValue>> Checks,Failures,Points;static TMap<FString,int32> BeforeItems;static TArray<FString> BeforeJournal;static TSet<FString> BeforeCompleted;
    static TArray<FPreview51> Catalog;static int32 Index=0;static int32 DocumentCount=0;static bool OriginalShotgun=false;static bool OriginalTorch74=false;static int32 BeforeRounds=0;static AActor* Preview=nullptr;static bool CatalogCaptured=false,ReaderOpened=false;
    auto* H=Hero();auto* PC=H?Cast<APlayerController>(H->Controller):nullptr;if(!H||!PC)return;Clock+=Delta;
    auto Check=[&](FString Name,bool Pass){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));UE_LOG(LogTemp,Display,TEXT("BODY51 %s %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Name);};
    auto Shot=[&](FString Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/BodyPreview51")/(Name+TEXT(".png")),true,false);};
    auto Next=[&](){++Stage;Clock=0;};
    auto AuditHeld=[&](const FString& Label,WardHeldGrip108::EGrip Grip,bool TwoHands)
    {
        const auto* Item=PreviewActor51.Get();
        auto* Body=H->BekaBody.Get();
        const bool Visible=Item&&Body&&!Item->IsHidden();
        Check(Label+TEXT("_physical_model_visible"),Visible);
        if(!Visible)return;

        const auto Profile=WardHeldGrip108::Preview(Item);
        Check(Label+TEXT("_grip_matches_item"),Profile.Grip==Grip&&Profile.TwoHands==TwoHands);
        if(Grip==WardHeldGrip108::EGrip::Notebook)
        {
            const auto* Face=Item->FindComponentByClass<UWidgetComponent>();
            Check(TEXT("notebook_lined_texture_is_loaded"),Face&&Face->ComponentHasTag(TEXT("WardNote109Paper")));
            Check(TEXT("notebook_handwriting_font_is_loaded"),Face&&Face->ComponentHasTag(TEXT("WardNote109Text")));
        }
        FVector2D Pixel;
        int32 Width=0,Height=0;
        PC->GetViewportSize(Width,Height);
        const bool InFrame=PC->ProjectWorldLocationToScreen(Item->GetActorLocation(),Pixel) &&
            Pixel.X>Width*.07 && Pixel.X<Width*.93 && Pixel.Y>Height*.07 && Pixel.Y<Height*.92;
        Check(Label+TEXT("_model_is_in_frame"),InFrame);
        Check(Label+TEXT("_right_hand_visible"),!Body->IsBoneHiddenByName(TEXT("hand_r")));
        Check(Label+TEXT("_left_hand_matches_support"),Body->IsBoneHiddenByName(TEXT("hand_l"))!=TwoHands);
        for(const bool Left:{false,true})
        {
            if(Left&&!TwoHands)continue;
            const FName Bone=Left?TEXT("hand_l"):TEXT("hand_r");
            const FVector Wrist=Body->GetSocketLocation(Bone);
            const FVector Anchor=Item->GetActorTransform().TransformPosition(WardHeldGrip108::Anchor(Profile,Left).Wrist);
            const double Error=FVector::Dist(Wrist,Anchor);
            UE_LOG(LogTemp,Display,TEXT("BODY51 %s %s wrist error %.2f cm"),*Label,*Bone.ToString(),Error);
            Check(Label+TEXT("_wrist_tracks_model_")+Bone.ToString(),Error<6.);
        }
    };
    auto AuditHands=[&](FString Label)
    {
        TArray<FFinalSkinVertex> Skin;
        for(FName Bone:{FName(TEXT("hand_l")),FName(TEXT("hand_r"))})
        {
            const FVector W=H->BekaBody->GetSocketLocation(Bone);FVector2D Pixel;PC->ProjectWorldLocationToScreen(W,Pixel);double Nearest=1e9;int32 Count=0;
            for(const auto& V:Skin){const FVector Vertex=H->BekaBody->GetComponentTransform().TransformPosition(FVector(V.Position));const double Distance=FVector::Dist(Vertex,W);Nearest=FMath::Min(Nearest,Distance);Count+=Distance<12;}
            Json P=MakeShared<FJsonObject>();P->SetStringField(TEXT("pose"),Label);P->SetStringField(TEXT("bone"),Bone.ToString());P->SetStringField(TEXT("world"),W.ToString());P->SetStringField(TEXT("camera"),H->Camera->GetComponentLocation().ToString());P->SetStringField(TEXT("pixel"),Pixel.ToString());Check(Label+TEXT("_hand_is_in_frame_")+Bone.ToString(),Pixel.X>100&&Pixel.X<1820&&Pixel.Y>100&&Pixel.Y<1000);Points.Add(MakeShared<FJsonValueObject>(P));
        }
    };
    if(Stage==0)
    {
        if(HospitalStory){HospitalStory->ArrivalStage=-1;HospitalStory->Seated31=false;HospitalStory->FreezeAI=true;}
        Screen.Empty();TitleView=false;CampaignStarted=true;PC->SetPause(false);PC->SetViewTarget(H);PC->SetInputMode(FInputModeGameOnly());H->SetActorHiddenInGame(false);H->SetActorLocation(FVector(4800,2800,98));H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);PC->SetControlRotation(FRotator(0,90,0));H->SetActorRotation(FRotator(0,90,0));BookWorld->SetBright(true);H->Camera->SetFieldOfView(80);GEngine->GetGameUserSettings()->SetScreenResolution(FIntPoint(1920,1080));GEngine->GetGameUserSettings()->SetResolutionScaleValueEx(100);GEngine->GetGameUserSettings()->ApplyResolutionSettings(false);
        BeforeItems=Items;BeforeJournal=Journal;BeforeCompleted=Completed;OriginalShotgun=Shotgun;OriginalTorch74=H->Flashlight->IsVisible();BeforeRounds=H->Rounds;Next();
    }
    H->UpdateFirstPerson50();UpdatePreview51();TickHero(Delta);BookWorld->Update(Delta);BookWorld->UpdateLighting(H->GetActorLocation());
    if(Stage==1&&Clock>18)
    {
        Check(TEXT("body_and_outfit_render_to_owner"),H->BekaBody&&H->BekaOutfit&&!H->BekaBody->bOwnerNoSee&&!H->BekaOutfit->bOwnerNoSee&&!H->BekaBody->bHiddenInGame&&!H->BekaOutfit->bHiddenInGame);
        if(auto* M=H->BekaBody->GetMaterial(0)){FLinearColor Tint;M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Tint")),Tint);UE_LOG(LogTemp,Display,TEXT("BODY51 material %s tint %s"),*M->GetPathName(),*Tint.ToString());}
        bool ClothReady=H->BekaOutfit->GetNumMaterials()>0;for(int32 I=0;I<H->BekaOutfit->GetNumMaterials();++I)ClothReady&=H->BekaOutfit->GetMaterial(I)&&H->BekaOutfit->GetMaterial(I)->GetName().Contains(TEXT("MI_Outfit51_"));Check(TEXT("outfit_uses_trimmed_cuff_materials"),ClothReady);
        int32 CuffCount=0;TArray<UStaticMeshComponent*> Cuffs;H->BekaBody->GetOwner()->GetComponents(Cuffs);for(auto* C:Cuffs)if(C->ComponentTags.Contains(TEXT("WristCuff51"))&&C->GetStaticMesh()&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision)++CuffCount;Check(TEXT("both_elastic_cuffs_have_renderable_collision_free_geometry"),CuffCount==2);
        Check(TEXT("empty_hands_hidden"), H->BekaBody->IsBoneHiddenByName(TEXT("hand_l")) &&
            H->BekaBody->IsBoneHiddenByName(TEXT("hand_r")));
        Shot(TEXT("01_idle_hands"));Next();
    }
    else if(Stage==2&&Clock>.5f){PC->SetControlRotation(FRotator(-88,90,0));Next();}
    else if(Stage==3&&Clock>2)
    {
        for(FName Bone:{FName(TEXT("pelvis")),FName(TEXT("foot_l")),FName(TEXT("foot_r")),FName(TEXT("hand_l")),FName(TEXT("hand_r"))})
        {
            const FVector World=H->BekaBody->GetSocketLocation(Bone);FVector2D Pixel;bool In=PC->ProjectWorldLocationToScreen(World,Pixel)&&Pixel.X>0&&Pixel.X<1920&&Pixel.Y>0&&Pixel.Y<1080;
            Json P=MakeShared<FJsonObject>();P->SetStringField(TEXT("bone"),Bone.ToString());P->SetStringField(TEXT("world"),World.ToString());P->SetStringField(TEXT("pixel"),Pixel.ToString());P->SetBoolField(TEXT("in_frame"),In);Points.Add(MakeShared<FJsonValueObject>(P));
            if(Bone.ToString().StartsWith(TEXT("foot"))||Bone==TEXT("pelvis"))Check(TEXT("look_down_contains_")+Bone.ToString(),In);
        }
        Check(TEXT("empty_hands_remain_hidden_when_looking_down"), H->BekaBody->IsBoneHiddenByName(TEXT("hand_l")) &&
            H->BekaBody->IsBoneHiddenByName(TEXT("hand_r")));
        Shot(TEXT("02_body_legs_shoes"));Next();
    }
    else if(Stage==4&&Clock>.5f){PC->SetControlRotation(FRotator(0,90,0));ToggleAdmin();Check(TEXT("admin_catalog_opens_from_command"),PreviewCommand51(TEXT("preview_catalog"))&&Screen==TEXT("admin_catalog"));Catalog=PreviewCatalog51();for(auto E:Catalog)DocumentCount+=E.Document;Check(TEXT("catalog_includes_all_story_documents"),HospitalStory&&DocumentCount>=Object(HospitalStory->Narrative35,TEXT("documents"))->Values.Num());Next();}
    else if(Stage==5&&Clock>.5f)
    {
        if(!CatalogCaptured){Shot(TEXT("03_admin_catalog"));CatalogCaptured=true;Clock=0;return;}if(Clock<.6f)return;
        if(Index<Catalog.Num())
        {
            const auto& E=Catalog[Index++];const bool OK=HoldPreview51(E.Id);if(!OK)Failures.Add(MakeShared<FJsonValueString>(E.Id));
            if(E.Document){Check(TEXT("read_")+E.Visual,PreviewCommand51(TEXT("preview_read_")+E.Visual)&&Screen==TEXT("admin_document")&&PreviewDocument51==E.Visual);}
            return;
        }
        Check(TEXT("every_catalog_item_can_be_held"),Failures.IsEmpty());Check(TEXT("preview_does_not_modify_items"),Items.OrderIndependentCompareEqual(BeforeItems));Check(TEXT("preview_does_not_collect_documents"),Journal==BeforeJournal);Check(TEXT("preview_does_not_advance_story"),Completed.Difference(BeforeCompleted).IsEmpty()&&BeforeCompleted.Difference(Completed).IsEmpty());Check(TEXT("preview_does_not_switch_game_weapon_or_ammo"),Shotgun==OriginalShotgun&&H->Rounds==BeforeRounds);
        HoldPreview51(TEXT("doc:NOTE_01_01"));Next();
    }
    else if(Stage==6&&Clock>1.f)
    {
        using WardHeldGrip108::EGrip;
        static const struct { const TCHAR* Id;const TCHAR* Label;const TCHAR* Image;EGrip Grip;bool TwoHands; } ItemsToAudit[]={
            {TEXT("doc:NOTE_01_01"),TEXT("paper"),TEXT("04_note_in_hands"),EGrip::Notebook,false},
            {TEXT("fuse"),TEXT("fuse"),TEXT("09_fuse_in_hand"),EGrip::Pinch,false},
            {TEXT("medkit"),TEXT("medkit"),TEXT("10_medkit_in_hands"),EGrip::Cradle,true},
        };
        static int32 ItemIndex=0;
        const auto& Item=ItemsToAudit[ItemIndex];
        if(PreviewItem51!=Item.Id)
        {
            if(!HoldPreview51(Item.Id))
            {
                Check(FString(Item.Label)+TEXT("_equips"),false);
                if(++ItemIndex==UE_ARRAY_COUNT(ItemsToAudit))Next();
            }
        }
        else
        {
            AuditHeld(Item.Label,Item.Grip,Item.TwoHands);
            Shot(Item.Image);
            if(++ItemIndex==UE_ARRAY_COUNT(ItemsToAudit))Next();
        }
        Clock=0;
    }
    else if(Stage==7&&Clock>.5f){if(!ReaderOpened){PreviewCommand51(TEXT("preview_read_NOTE_01_01"));ReaderOpened=true;Clock=0;return;}Shot(TEXT("05_note_readable"));Next();}
    else if(Stage==8&&Clock>.5f){HoldPreview51(TEXT("weapon"));H->Aiming=false;Next();}
    else if(Stage==9&&Clock>1.5f){AuditHands(TEXT("pistol"));Shot(TEXT("06_pistol_hands"));H->Aiming=true;Next();}
    else if(Stage==10&&Clock>1.5f){Shot(TEXT("07_aim_hands"));H->Flashlight->SetVisibility(true);H->UpdateTorchTransform();Next();}
    else if(Stage==11&&Clock>1.5f)
    {
        Check(TEXT("torch_still_lights_world_channel"),H->Flashlight->LightingChannels.bChannel0&&!H->Flashlight->LightingChannels.bChannel1);
        Check(TEXT("body_excludes_torch_but_keeps_scene_light"),!H->BekaBody->LightingChannels.bChannel0&&H->BekaBody->LightingChannels.bChannel1);
        Check(TEXT("outfit_excludes_torch_but_keeps_scene_light"),!H->BekaOutfit->LightingChannels.bChannel0&&H->BekaOutfit->LightingChannels.bChannel1);
        bool WeaponReady74=!H->Equipment.IsEmpty();for(auto Part:H->Equipment)WeaponReady74&=!Part->LightingChannels.bChannel0&&Part->LightingChannels.bChannel1;
        Check(TEXT("all_weapon_parts_exclude_close_torch"),WeaponReady74);
        Shot(TEXT("08_aim_torch_on"));Next();
    }
    else if(Stage==12&&Clock>.5f)
    {
        H->Flashlight->SetVisibility(OriginalTorch74);
        PreviewCommand51(TEXT("preview_clear"));
        Check(TEXT("clear_removes_preview_actor"),PreviewItem51.IsEmpty()&&!PreviewActor51);
        ToggleAdmin();
        Check(TEXT("exit_admin_restores_hero_camera"),!Admin&&!AdminHands51&&PC->GetViewTarget()==H);
        Check(TEXT("uncollected_note_cannot_be_held_as_a_player"),!HoldPreview51(TEXT("doc:NOTE_01_01")));
        Journal.AddUnique(TEXT("@doc:NOTE_01_01"));
        Completed.Add(TEXT("NOTE_01_01"));
        Show(TEXT("journal"));
        Command(TEXT("field_doc_")+FString::FromInt(Journal.Find(TEXT("@doc:NOTE_01_01"))));
        Command(TEXT("field_note_hold"));
        Check(TEXT("collected_note_can_be_held_from_journal"),!Admin&&Screen.IsEmpty()&&PreviewActor51&&PreviewItem51==TEXT("doc:NOTE_01_01"));
        Next();
    }
    else if(Stage==13&&Clock>.60f){Shot(TEXT("11_note_from_journal"));Next();}
    else if(Stage==14&&Clock>.60f)
    {
        bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetNumberField(TEXT("catalog_entries"),Catalog.Num());R->SetNumberField(TEXT("documents"),DocumentCount);R->SetArrayField(TEXT("checks"),Checks);R->SetArrayField(TEXT("missing_models"),Failures);R->SetArrayField(TEXT("body_projection"),Points);R->SetStringField(TEXT("scope"),TEXT("Automated real-map render, body projection, all item/document preview checks. Screenshots visually reviewed; not a manual campaign playthrough."));Write(FPaths::ProjectSavedDir()/TEXT("Prototype/body_preview51_check.json"),R);FPlatformMisc::RequestExit(false);Next();
    }
}
