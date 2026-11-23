#include "UnrealClient.h"
#include "HAL/FileManager.h"
#include "Components/TextRenderComponent.h"
using namespace WardData;
Json WardMediaAudit78(const FString& Action);
void TickCapture78(AEWardCampaignMode& G,float Delta)
{
 struct FView78 {const TCHAR* Id;FVector Eye,Target;};
 static const TArray<FView78> Views={
 {TEXT("01_gate"),{63.3,-64.5,1.7},{63.3,-55,1.6}},
 {TEXT("02_courtyard"),{-23.6,-59.6,1.7},{-32,-54,1.1}},
 {TEXT("03_east"),{69.8,-7.5,1.7},{84.8,4.5,3.3}},
 {TEXT("04_corridor"),{48,28,1.7},{48,57,1.5}},
 {TEXT("05_laboratory"),{64.84874,52.26391,1.5},{65.94874,51.46391,.78086}},
 {TEXT("06_reception"),{84.62,6.85,1.76},{84.62,5.35,.87}},
 {TEXT("07_equipment"),{29.944,4.257727,1.55},{30.52,2.765,.625375}},
 {TEXT("08_room"),{64.819675,30.912615,1.55},{65.48,32.37,.44625}},
 {TEXT("09_trace"),{63.5,8,1.65},{62.42,9.6,.026}},
 {TEXT("10_entrance"),{152.4,-115,1.7},{167.4,-103,3.3}},
 {TEXT("11_hall"),{48,73,1.7},{48,96,1.5}},
 {TEXT("12_west"),{-109.113,6.85,1.76},{-109.113,5.35,.7775}}};
 static int Index=-1,Frame=-1;static bool Shot=false,Pending=false,ReshotHall=false;static double Since=0;static ACameraActor* Cam=nullptr;
 auto* H=G.Hero();auto* S=G.HospitalStory.Get();auto* PC=G.GetWorld()->GetFirstPlayerController();if(!H||!S||!PC||!G.BookWorld)return;
 const double Now=FPlatformTime::Seconds();
 auto Place=[&](){const auto& V=Views[Index];H->SetActorLocation(V.Eye*100-FVector(0,0,65),false);H->GetCharacterMovement()->DisableMovement();Cam->SetActorLocationAndRotation(V.Eye*100,(V.Target-V.Eye).Rotation());G.BookWorld->LightSelectionClock=1;G.BookWorld->UpdateLighting(H->GetActorLocation());PC->SetViewTarget(Cam);Since=Now;Shot=false;};
 if(Index<0){G.Assessment=true;G.SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("Media78TestSaves");G.NewGame(TEXT("story"));S->SetActorTickEnabled(false);S->FreezeAI=true;S->HomeStage52=-1;S->SetHomeVisible53(false);S->ArrivalStage=5;S->ApplyArrivalState();S->ActiveScene35.Empty();G.Show(TEXT(""));G.Interface->Close();H->SetActorHiddenInGame(true);
 for(auto M:S->Mobs)if(M){M->SetActorHiddenInGame(true);M->SetActorTickEnabled(false);}for(auto E:G.Enemies)if(E){E->SetActorHiddenInGame(true);E->SetActorTickEnabled(false);}
 for(TActorIterator<AActor> It(G.GetWorld());It;++It){TInlineComponentArray<UTextRenderComponent*> Labels;It->GetComponents(Labels);for(auto* L:Labels)L->SetVisibility(false);}
 G.BookWorld->SetBright(false);Cam=G.GetWorld()->SpawnActor<ACameraActor>();auto* C=Cam->GetCameraComponent();C->SetFieldOfView(72);C->SetConstraintAspectRatio(false);C->PostProcessSettings=H->Camera->PostProcessSettings;auto& P=C->PostProcessSettings;P.bOverride_MotionBlurAmount=true;P.MotionBlurAmount=0;P.bOverride_AutoExposureBias=true;P.AutoExposureBias=.7f;P.bOverride_VignetteIntensity=true;P.VignetteIntensity=.22f;P.bOverride_FilmGrainIntensity=true;P.FilmGrainIntensity=.015f;Index=FParse::Param(FCommandLine::Get(),TEXT("WardCorridorOnly78"))?3:0;Place();return;}
 G.BookWorld->Update(Delta);G.BookWorld->UpdateLighting(H->GetActorLocation());PC->PlayerCameraManager->UpdateCamera(.016f);
 if(G.BookWorld->DeferredNearRemaining37>0){Since=Now;return;}if(Now-Since<2.5)return;
 const FString Dir=FPaths::ProjectSavedDir()/TEXT("Prototype/Media78Capture");
 if(Frame<0){if(!Shot){FScreenshotRequest::RequestScreenshot(Dir/TEXT("stills")/(FString(Views[Index].Id)+TEXT(".png")),false,false);Shot=true;Since=Now;return;}if(FParse::Param(FCommandLine::Get(),TEXT("WardCorridorOnly78"))){if(!ReshotHall){ReshotHall=true;Index=10;Place();return;}Index=3;Frame=0;Place();return;}if(++Index<Views.Num()){Place();return;}Index=3;Frame=0;Place();return;}
 // Two engine frames per request keep each captured image tied to its own camera pose.
 if(Pending){Pending=false;++Frame;return;}
 if(Frame>=72){if(FParse::Param(FCommandLine::Get(),TEXT("WardCorridorOnly78"))){FPlatformMisc::RequestExit(false);return;}Index=Index==3?4:Index==4?1:-1;if(Index<0){auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("success"),true);J->SetNumberField(TEXT("stills"),Views.Num());J->SetNumberField(TEXT("clips"),3);J->SetNumberField(TEXT("frames_per_clip"),72);J->SetStringField(TEXT("source"),TEXT("Native campaign world, horror lighting, no HUD, camera dolly"));Write(Dir/TEXT("capture.json"),J);FPlatformMisc::RequestExit(false);return;}Frame=0;Place();return;}
 const auto& V=Views[Index];const float T=Frame/71.f;const FVector Eye=(V.Eye+FVector(.65f*T,.30f*T,0))*100;Cam->SetActorLocationAndRotation(Eye,(V.Target*100-Eye).Rotation());PC->PlayerCameraManager->UpdateCamera(1.f/24);FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("clip_%02d/%04d.png"),Index,Frame),false,false);Pending=true;
}
void TickMediaCheck78(AEWardCampaignMode& G,float Delta)
{
 static int Step=0;static double Last=0;static TArray<TSharedPtr<FJsonValue>> Checks;const double Now=FPlatformTime::Seconds();if(Now-Last<1.4)return;Last=Now;
 auto Check=[&](FString Id,bool OK){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("id"),Id);J->SetBoolField(TEXT("passed"),OK);Checks.Add(MakeShared<FJsonValueObject>(J));UE_LOG(LogTemp,Display,TEXT("MEDIA78 %s %s"),*Id,OK?TEXT("PASS"):TEXT("FAIL"));};
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/Media78QA")/(FString(Name)+TEXT(".png")),true,false);};
 if(Step==0){G.Show(TEXT("screenshots"));++Step;return;}
 if(Step==1){auto J=WardMediaAudit78(TEXT(""));Check(TEXT("gallery_20_images"),Number(J,TEXT("count"))==20);Check(TEXT("gallery_preview_loaded"),Boolean(J,TEXT("image")));Shot(TEXT("gallery_all"));++Step;return;}
 if(Step==2){auto J=WardMediaAudit78(TEXT("art"));Check(TEXT("eight_illustrations_separated"),Number(J,TEXT("count"))==8);WardMediaAudit78(TEXT("next"));Shot(TEXT("gallery_art"));++Step;return;}
 if(Step==3){auto J=WardMediaAudit78(TEXT("game"));Check(TEXT("twelve_native_screenshots"),Number(J,TEXT("count"))==12);Shot(TEXT("gallery_game"));++Step;return;}
 if(Step==4){G.Show(TEXT("trailer"));++Step;return;}
 if(Step==5){auto J=WardMediaAudit78(TEXT("play"));Check(TEXT("movie_file_loaded"),Boolean(J,TEXT("video")));Check(TEXT("soundtrack_loaded"),Boolean(J,TEXT("audio")));Shot(TEXT("trailer_start"));++Step;return;}
 if(Step==6){auto J=WardMediaAudit78(TEXT("pause"));Check(TEXT("movie_advances"),Number(J,TEXT("frame"))>10);Check(TEXT("movie_pauses"),!Boolean(J,TEXT("playing")));++Step;return;}
 if(Step==7){auto J=WardMediaAudit78(TEXT("seek"));Check(TEXT("seek_halfway"),Number(J,TEXT("time"))>20);WardMediaAudit78(TEXT("mute"));J=WardMediaAudit78(TEXT(""));Check(TEXT("mute_toggle"),Boolean(J,TEXT("muted")));Shot(TEXT("trailer_middle"));++Step;return;}
 if(Step==8){auto J=WardMediaAudit78(TEXT("restart"));Check(TEXT("restart_returns_to_beginning"),Number(J,TEXT("time"))<.1);G.Show(TEXT("title"));++Step;return;}
 if(Step==9){auto J=WardMediaAudit78(TEXT(""));Check(TEXT("closing_releases_player"),!Boolean(J,TEXT("exists")));bool OK=true;for(auto C:Checks)OK&=Boolean(C->AsObject(),TEXT("passed"));auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),OK);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/media78_check.json"),R);FPlatformMisc::RequestExit(false);++Step;}
}
