#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
using namespace WardData;
void AEWardCampaignMode::TickHUDMenuAssessment(float Delta)
{
    static int Step=0;static double Since=FPlatformTime::Seconds();static TArray<TSharedPtr<FJsonValue>> Checks,Frames;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);auto* HUD=Cast<AEWardCampaignHUD>(PC->GetHUD());if(!HUD)return;
    const double T=FPlatformTime::Seconds()-Since;auto Next=[&](){++Step;Since=FPlatformTime::Seconds();};
    auto Check=[&](FString N,bool B){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),N);J->SetBoolField(TEXT("passed"),B);Checks.Add(MakeShared<FJsonValueObject>(J));};
    auto Shot=[&](const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/HUDCaptures/")+N+TEXT(".png"),true,false);if(HUD->LastFrame){Json J=MakeShared<FJsonObject>();J->SetStringField(TEXT("image"),N);J->SetObjectField(TEXT("frame"),HUD->LastFrame);Frames.Add(MakeShared<FJsonValueObject>(J));}};
    auto Has=[&](const TCHAR* Id){return HUD->LastFrame&&Object(HUD->LastFrame,TEXT("regions"))->HasField(Id);};
    if(Step==0){NewGame(TEXT("normal"));Show(TEXT(""));Language=TEXT("ru");Options->SetBoolField(TEXT("show_noise"),true);Options->SetBoolField(TEXT("crosshair"),true);Options->SetBoolField(TEXT("subtitles"),true);Options->SetNumberField(TEXT("hud_opacity"),1);Flag(TEXT("weapon"),true);Noise=.5f;Hero()->Health=66;Hero()->Stamina=100;Hero()->Rounds=6;Hero()->Reserve=12;Hero()->NoticeTime=0;Hero()->Focus=nullptr;Hero()->Aiming=false;Hero()->SetActorLocation(FVector(-1200,0,106));PC->SetControlRotation(FRotator(-8,5,0));Hero()->GetCharacterMovement()->StopMovementImmediately();for(auto E:Enemies){E->SetActorTickEnabled(false);E->SetActorHiddenInGame(true);}TickHero(.016f);Hero()->Focus=nullptr;PC->SetPause(true);GetWorld()->bIsCameraMoveableWhenPaused=true;Next();}
    else if(Step==1&&T>3){Check(TEXT("hud_visible_and_all_text_in_bounds"),Boolean(HUD->LastFrame,TEXT("visible"))&&Boolean(HUD->LastFrame,TEXT("bounded")));Check(TEXT("four_corner_regions_present"),Has(TEXT("objective"))&&Has(TEXT("noise"))&&Has(TEXT("health"))&&Has(TEXT("breath"))&&Has(TEXT("weapon")));Check(TEXT("live_health_ammo_values"),Number(HUD->LastFrame,TEXT("health"))==66&&Number(HUD->LastFrame,TEXT("rounds"))==6&&Number(HUD->LastFrame,TEXT("reserve"))==12);Check(TEXT("no_interaction_without_target"),!Has(TEXT("interaction")));Shot(TEXT("hud_1080"));Next();}
    else if(Step==2&&T>1){Hero()->Aiming=true;TickHero(.016f);Hero()->Focus=Objects[TEXT("refuge_save")];Hero()->SearchProgress=.5f;Hero()->Notice=TEXT("Тише… кто-то ходит по коридору.");Hero()->NoticeTime=99;Next();}
    else if(Step==3&&T>1){Check(TEXT("context_interaction_and_subtitle_visible"),Has(TEXT("interaction"))&&Has(TEXT("subtitle")));Check(TEXT("aim_crosshair_visible"),Has(TEXT("crosshair")));Check(TEXT("context_text_stays_in_bounds"),Boolean(HUD->LastFrame,TEXT("bounded")));const Json Regions=Object(HUD->LastFrame,TEXT("regions"));const auto& Prompt=Array(Regions,TEXT("interaction"));const auto& Subtitle=Array(Regions,TEXT("subtitle"));Check(TEXT("interaction_clear_of_subtitles"),Prompt.Num()==4&&Subtitle.Num()==4&&Prompt[1]->AsNumber()+Prompt[3]->AsNumber()+8<=Subtitle[1]->AsNumber());Shot(TEXT("hud_interaction_1080"));Next();}
    else if(Step==4&&T>1){Options->SetBoolField(TEXT("show_noise"),false);Options->SetBoolField(TEXT("crosshair"),false);Options->SetBoolField(TEXT("subtitles"),false);Next();}
    else if(Step==5&&T>1){Check(TEXT("hud_preferences_respected"),!Has(TEXT("noise"))&&!Has(TEXT("crosshair"))&&!Has(TEXT("subtitle")));Options->SetBoolField(TEXT("show_noise"),true);Options->SetBoolField(TEXT("crosshair"),true);Options->SetBoolField(TEXT("subtitles"),true);Hero()->Focus=nullptr;Hero()->SearchProgress=0;Hero()->NoticeTime=0;Hero()->Aiming=false;Shotgun=true;LoadedShells=3;Items.Add(TEXT("shells"),8);Hero()->Health=19;Next();}
    else if(Step==6&&T>1){Check(TEXT("shotgun_uses_shell_counts"),String(HUD->LastFrame,TEXT("weapon"))==TEXT("shotgun")&&Number(HUD->LastFrame,TEXT("rounds"))==3&&Number(HUD->LastFrame,TEXT("reserve"))==8);Check(TEXT("low_health_keeps_layout_bounded"),Boolean(HUD->LastFrame,TEXT("bounded"))&&Number(HUD->LastFrame,TEXT("health"))==19);Shot(TEXT("hud_shotgun_1080"));Next();}
    else if(Step==7&&T>1){Language=TEXT("kk");Shotgun=false;Hero()->Health=66;Fault=TEXT("pressure");Next();}
    else if(Step==8&&T>1){Check(TEXT("kazakh_long_objective_fits"),Boolean(HUD->LastFrame,TEXT("bounded")));Shot(TEXT("hud_kk_1080"));Next();}
    else if(Step==9&&T>1){Language=TEXT("en");Next();}
    else if(Step==10&&T>1){Check(TEXT("english_long_objective_fits"),Boolean(HUD->LastFrame,TEXT("bounded")));Shot(TEXT("hud_en_1080"));Next();}
    else if(Step==11&&T>1){Language=TEXT("ru");Fault.Empty();FSystemResolution::RequestResolutionChange(1280,720,EWindowMode::Windowed);Next();}
    else if(Step==12&&T>2){Check(TEXT("720p_layout_fits"),Number(HUD->LastFrame,TEXT("width"))==1280&&Number(HUD->LastFrame,TEXT("height"))==720&&Boolean(HUD->LastFrame,TEXT("bounded")));Shot(TEXT("hud_720"));Next();}
    else if(Step==13&&T>1){FSystemResolution::RequestResolutionChange(1920,1200,EWindowMode::Windowed);Next();}
    else if(Step==14&&T>2){Check(TEXT("16_10_layout_fits"),Number(HUD->LastFrame,TEXT("width"))==1920&&Number(HUD->LastFrame,TEXT("height"))==1200&&Boolean(HUD->LastFrame,TEXT("bounded")));Shot(TEXT("hud_1200"));Next();}
    else if(Step==15&&T>1){Show(TEXT("pause"));Next();}
    else if(Step==16&&T>1){Check(TEXT("hud_hidden_in_pause"),!Boolean(HUD->LastFrame,TEXT("visible")));Command(TEXT("resume"));PC->SetPause(true);Next();}
    else if(Step==17&&T>1){Check(TEXT("hud_returns_on_resume"),Boolean(HUD->LastFrame,TEXT("visible"))&&Has(TEXT("health"))&&Has(TEXT("weapon")));Flag(TEXT("weapon"),false);Next();}
    else if(Step==18&&T>1){Check(TEXT("weapon_panel_hidden_until_found"),!Has(TEXT("weapon")));bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);R->SetArrayField(TEXT("captures"),Frames);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/hud_check.json"),R);FPlatformMisc::RequestExit(false);Next();}
    if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(.016f);
}
