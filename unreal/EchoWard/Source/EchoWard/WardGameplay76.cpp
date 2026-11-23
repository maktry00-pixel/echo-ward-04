#include "WardGameplay76.h"
#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardHeldGrip108.h"
#include "WardBookWorld.h"
#include "WardHospitalGameplay.h"
#include "WardDialogue62.h"
#include "WardStaging64.h"
#include "WardHomeAssets53.h"
#include "WardMonsterVisual59.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
using namespace WardData;
namespace WardGameplay76
{
bool Armed(const AEWardCampaignMode& G){return G.Flag(G.Shotgun?TEXT("shotgun"):TEXT("weapon"))&&!G.Flag(TEXT("hands_empty76"))&&G.HeldSlot76.IsEmpty()&&!G.PhoneOpen76;}
void Reset(AEWardCampaignMode& G)
{
 G.PhoneOpen76=false;G.PhoneRaise76=0;G.PhonePage76=TEXT("home");G.HeldSlot76.Empty();G.PreviousObjective76.Empty();G.CompletedObjective76.Empty();G.ObjectiveFade76=0;G.LastCompleted76=G.Completed.Num();G.ScareClock76=0;G.ScareCooldown76=24;G.TutorialClock76=0;
 if(G.PhoneActor76)G.PhoneActor76->SetActorHiddenInGame(true);
 if(G.ScareActor76)G.ScareActor76->SetActorHiddenInGame(true);
}
int Door(const AEWardBookWorld& W)
{
 auto* G=WardCampaign(&W);auto* H=G?G->Hero():nullptr;if(!H||!H->Camera)return INDEX_NONE;
 const FVector Eye=H->Camera->GetComponentLocation(),Forward=H->Camera->GetForwardVector(),At=H->GetActorLocation();
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(DoorAim76),true,H);
 W.GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Forward*280,ECC_Visibility,Q);
 int Best=INDEX_NONE;double Score=-1;
 for(int I=0;I<W.Portals.Num();++I)
 {
  const auto& P=W.Portals[I];if(P.Role==TEXT("niche")||FMath::Abs(At.Z-P.At.Z-98)>120||FVector::Dist2D(At,P.At)>260+P.Width*.15)continue;
  FVector Target=P.At+FVector(0,0,125);const bool Direct=P.Leaf&&Hit.GetComponent()==P.Leaf;
  const double Dot=FVector::DotProduct(Forward,(Target-Eye).GetSafeNormal());if(!Direct&&Dot<.80)continue;
  if(Hit.bBlockingHit&&!Direct&&FVector::Dist(Eye,Hit.ImpactPoint)+30<FVector::Dist(Eye,Target))continue;
  const double Rank=(Direct?10:Dot)-FVector::Dist2D(At,P.At)*.0002;if(Rank>Score){Score=Rank;Best=I;}
 }
 return Best;
}
FString Prompt(AEWardCampaignMode& G)
{
 auto* S=G.HospitalStory.Get();
 if(S&&S->HomeStage52>=0)
 {
  const FVector At=G.Hero()->GetActorLocation();
  if(S->HomeStage52==0&&FVector::Dist2D(At,S->HomePhone52)<175)return TEXT("F — ответить на звонок · Аружан");
  if(S->HomeStage52==2&&FVector::Dist2D(At,FVector(-80000,-80350,0))<155)return TEXT("E / F — выйти из дома");
  return {};
 }
 if(G.BookWorld){const int I=Door(*G.BookWorld);if(I!=INDEX_NONE){const auto& P=G.BookWorld->Portals[I];return P.Locked?TEXT("E / F — проверить замок"):P.Open?TEXT("E / F — закрыть дверь"):TEXT("E / F — открыть дверь");}}
 if(G.Hero()->Focus)return G.Hero()->Focus->Prompt(G.Hero());
 return S?S->Prompt():FString();
}
void Skip(AEWardCampaignMode& G,bool Scene)
{
 auto* S=G.HospitalStory.Get();if(!S||!G.CampaignStarted||G.TitleView)return;
 if(S->HomeStage52==1){if(Scene)S->HomeLine52=S->HomeLines52().Num()-1;S->HomeLineTime69=0;if(S->HomeAudio53)S->HomeAudio53->Stop();return;}
 if(S->ArrivalStage==0){S->ArrivalInteract();return;}
 if(G.Screen!=TEXT("story35")&& !G.Screen.IsEmpty())return;
 if(S->DialogueSession62.IsEmpty())return;
 if(IsValid(S->DialogueAudio62))S->DialogueAudio62->Stop();
 if(Scene)
 {
  G.StringFlag(TEXT("skip76_scene"),S->ActiveScene35);
  S->DialogueIndex62=S->DialogueLines62.Num();S->DialogueKey62.Empty();S->DialogueLeft62=0;
  auto State=Object(S->NarrativeState35,TEXT("dialogue62"));State->SetNumberField(TEXT("cursor:")+S->DialogueSession62,S->DialogueIndex62);
  // Keep the authored choice and physical staging action pending.
  if(!WardDialogue62::AwaitingChoice(*S)&&WardStaging64::CanFinish(*S))S->NarrativeCommand35(TEXT("story35_next"));
 }
 else S->DialogueLeft62=0;
 if(G.Interface.IsValid()&&!G.Screen.IsEmpty())G.Interface->Refresh();
}
void Slot(AEWardCampaignMode& G,int Index)
{
 if(!G.CampaignStarted||!G.Screen.IsEmpty()||G.Hero()->Dead||G.ReloadTime>0||(G.HospitalStory&&G.HospitalStory->ArrivalCinematic()))return;
 if(G.PhoneOpen76){G.PhoneOpen76=false;G.PhoneRaise76=0;}
 G.ClearPreview51();G.HeldSlot76.Empty();G.Hero()->Aiming=false;
 if(Index<0){G.Flag(TEXT("hands_empty76"),true);return;}
 const auto Inv=G.Inventory();if(!Inv.IsValidIndex(Index)){G.Notify(TEXT("Слот пуст · 0 — свободные руки"),2);G.Flag(TEXT("hands_empty76"),true);return;}
 const FString Id=Inv[Index].Key;G.Flag(TEXT("hands_empty76"),false);
 if(Id==TEXT("weapon")||Id==TEXT("shotgun")){G.Shotgun=Id==TEXT("shotgun");G.Notify(G.ItemName(Id),2);return;}
 G.HeldSlot76=Id;G.Flag(TEXT("hands_empty76"),true);
 if(!G.HoldPreview51(Id))G.Notify(G.ItemName(Id)+TEXT(" · I — использовать предмет · 0 — убрать"),3);
}
void Phone(AEWardCampaignMode& G)
{
 if(!G.CampaignStarted||G.TitleView||!G.Hero()||G.Hero()->Dead)return;
 if(G.Screen==TEXT("phone76")){G.PhoneOpen76=false;G.Show(TEXT(""));return;}
 if(!G.Screen.IsEmpty()||(G.HospitalStory&&G.HospitalStory->ArrivalCinematic()))return;
 G.ClearPreview51();G.HeldSlot76.Empty();G.PhoneOpen76=!G.PhoneOpen76;G.PhonePage76=TEXT("home");G.Hero()->Aiming=false;G.Show(TEXT(""));
 for(auto C:G.Hero()->Equipment)C->SetVisibility(false);
 G.Cue(TEXT("cloth"),G.Hero()->GetActorLocation(),.15f);
}
bool Command(AEWardCampaignMode& G,const FString& Id)
{
 if(!Id.StartsWith(TEXT("phone76_"))&&Id!=TEXT("skip76_scene")&&Id!=TEXT("skip76_line"))return false;
 if(Id.StartsWith(TEXT("skip76_"))){Skip(G,Id==TEXT("skip76_scene"));return true;}
 if(G.Screen!=TEXT("phone76"))return true;
 const FString Page=Id.RightChop(8);
 if(Page==TEXT("close")){G.PhoneOpen76=false;G.Show(TEXT(""));return true;}
 if(Page==TEXT("send"))
 {
  if(!G.Flag(TEXT("phone76_received"))||!G.Flag(TEXT("phone76_read"))||G.Flag(TEXT("phone76_sent")))return true;
  G.Flag(TEXT("phone76_sent"),true);G.Journal.AddUnique(TEXT("Бека → Аружан: Понял. На голос не пойду. Я у КПП."));G.Cue(TEXT("UI/confirm"),G.Hero()->GetActorLocation(),.2f);

 }
 else {G.PhonePage76=Page;if(Page==TEXT("aruzhan")&&G.Flag(TEXT("phone76_received")))G.Flag(TEXT("phone76_read"),true);}
 if(G.Interface.IsValid())G.Interface->Refresh();return true;
}
namespace
{
void PhoneModel(AEWardCampaignMode& G)
{
 if(G.PhoneActor76)return;
 auto* A=G.GetWorld()->SpawnActor<AActor>();G.PhoneActor76=A;A->SetOwner(G.Hero());A->SetActorEnableCollision(false);
 auto* Root=NewObject<USceneComponent>(A);A->SetRootComponent(Root);Root->RegisterComponent();
 auto Library=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Gameplay76/phone_mesh.json"));
 auto* Mesh=WardHomeAssets::BuildMesh(A,Library,TEXT("smartphone"));
 auto* C=NewObject<UStaticMeshComponent>(A);C->SetupAttachment(Root);C->bDisallowNanite=true;
 C->SetStaticMesh(Mesh?Mesh:LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 C->SetRelativeRotation(FRotator(0,180,0));
 if(!Mesh)C->SetRelativeScale3D(FVector(.008,.075,.15));
 // Revision77PhoneTint: retain the downloaded texture with a graphite/blood tint.
 for(int I=0;I<C->GetNumMaterials();++I)if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(I))){M->SetVectorParameterValue(TEXT("Tint"),I==0?FLinearColor(.24,.11,.13):FLinearColor(.13,.005,.020));}
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetLightingChannels(false,true,false);C->RegisterComponent();A->AddInstanceComponent(C);A->SetActorHiddenInGame(true);
}
void Scare(AEWardCampaignMode& G)
{
 auto* H=G.Hero();const FVector Eye=H->Camera->GetComponentLocation(),Forward=H->Camera->GetForwardVector();
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(Scare76),false,H);
 // Never show the apparition through a wall or inside the player's capsule.
 if(G.GetWorld()->SweepSingleByChannel(Hit,Eye,Eye+Forward*210,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(28),Q))return;
 if(!G.ScareActor76)
 {
  auto* A=G.GetWorld()->SpawnActor<AActor>();G.ScareActor76=A;
  auto* V=NewObject<UEWardMonsterVisual59>(A);A->SetRootComponent(V);V->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->RegisterComponent();A->AddInstanceComponent(V);
  if(!V->Load59(TEXT("impostor_01"),185)){A->Destroy();G.ScareActor76=nullptr;return;}V->Present59(0,0,false,0);
 }
 const FVector At=Eye+Forward*190-FVector(0,0,154);G.ScareActor76->SetActorLocationAndRotation(At,FRotator(0,H->GetControlRotation().Yaw+180,0));G.ScareActor76->SetActorHiddenInGame(false);
 G.ScareClock76=1.8f;G.ScareCooldown76=65;WardHomeAssets::PlaySpatialFile61(&G,FPaths::ProjectContentDir()/TEXT("CampaignData/Gameplay76/scare.wav"),.60f*G.Setting(TEXT("effects"),.8),Eye+Forward*100);
 G.Flag(FString::Printf(TEXT("scare76_%d"),G.HospitalStory->Chapter()),true);
}
}
void Tick(AEWardCampaignMode& G,float D)
{
 if(!G.Hero()||!G.Flags.IsValid())return;
 if(G.Screen!=TEXT("phone76")&&!G.Screen.IsEmpty()&&G.PhoneOpen76){G.PhoneOpen76=false;G.PhoneRaise76=0;}
 if(G.PhoneActor76 && (!G.PhoneOpen76||G.TitleView||G.Hero()->Dead))G.PhoneActor76->SetActorHiddenInGame(true);
 if(!G.CampaignStarted||G.TitleView||G.Hero()->Dead)return;
 if(!G.Screen.IsEmpty())return;
 if(!G.HeldSlot76.IsEmpty())
 {
  bool Owned=false;for(const auto& Item:G.Inventory())Owned|=Item.Key==G.HeldSlot76;
  if(!Owned){G.ClearPreview51();G.HeldSlot76.Empty();}
 }
 G.TutorialClock76+=D;G.ObjectiveFade76=FMath::Max(0.f,G.ObjectiveFade76-D);
 const FString Goal=G.Objective();
 if(Goal!=G.PreviousObjective76)
 {
  if(!G.PreviousObjective76.IsEmpty()&&G.Completed.Num()>G.LastCompleted76){G.CompletedObjective76=G.PreviousObjective76;G.ObjectiveFade76=3.2f;G.Cue(TEXT("UI/confirm"),G.Hero()->GetActorLocation(),.11f);}
  G.PreviousObjective76=Goal;G.LastCompleted76=G.Completed.Num();
 }
 if(G.PhoneOpen76)
 {
  PhoneModel(G);G.PhoneRaise76=FMath::Min(1.f,G.PhoneRaise76+D*2.8f);
  const float Blend=FMath::SmoothStep(0.f,1.f,G.PhoneRaise76);
  const bool Visible=WardHeldGrip108::Place(*G.Hero(),*G.PhoneActor76,WardHeldGrip108::Phone(),Blend);
  G.PhoneActor76->SetActorHiddenInGame(!Visible);
  if(G.PhoneRaise76>=1)G.Show(TEXT("phone76"));return;
 }
 auto* S=G.HospitalStory.Get();if(!S)return;
 // Revision 77: the home invitation is a voice call; the warning belongs to the checkpoint.
 if(!G.Flag(TEXT("phone77_migrated")))
 {
  G.Flag(TEXT("phone76_received"),false);G.Flag(TEXT("phone76_read"),false);G.Flag(TEXT("phone76_sent"),false);G.Flag(TEXT("phone77_migrated"),true);
 }
 const FVector Checkpoint=Vector(Array(S->ArrivalData,TEXT("checkpoint")))*100;
 if(!G.Flag(TEXT("phone76_received"))&&S->HomeStage52<0&&S->ArrivalStage==2&&S->ActiveScene35.IsEmpty()&&FVector::Dist2D(G.Hero()->GetActorLocation(),Checkpoint)<950)
 {
  G.Flag(TEXT("phone76_received"),true);
  G.Journal.AddUnique(TEXT("Аружан: Бека, если услышишь мой голос — не иди на него. Это не я. Я напишу, когда будет безопасно."));
  WardHomeAssets::PlayDialogueFile(&G,FPaths::ProjectContentDir()/TEXT("CampaignData/Gameplay76/message.wav"),.38f*G.Setting(TEXT("effects"),.8));
  G.Notify(TEXT("Аружан · новое сообщение · P — телефон"),5);Phone(G);
 }
 if(G.ScareClock76>0){G.ScareClock76=FMath::Max(0.f,G.ScareClock76-D);if(G.ScareActor76)G.ScareActor76->SetActorHiddenInGame(G.ScareClock76<1.05f);}
 G.ScareCooldown76=FMath::Max(0.f,G.ScareCooldown76-D);
 if(G.Assessment||G.Admin||G.Agent||S->HomeStage52>=0||S->ArrivalCinematic()||!S->ActiveScene35.IsEmpty()||S->Chapter()<2||S->Chapter()>19||G.ScareCooldown76>0)return;
 const int Chapter=S->Chapter();if(Chapter!=2&&Chapter!=4&&Chapter!=7&&Chapter!=10&&Chapter!=14&&Chapter!=18)return;
 if(G.Flag(FString::Printf(TEXT("scare76_%d"),Chapter)))return;
 for(auto M:S->Mobs)if(M&&M->Active&&!M->Dead&&FVector::Dist2D(M->GetActorLocation(),G.Hero()->GetActorLocation())<650)return;
 if(G.Hero()->GetVelocity().Size2D()>40)Scare(G);
}

void Draw(AEWardCampaignMode& G,UCanvas* C)
{
 if(!C||!G.Hero())return;const float W=C->SizeX,H=C->SizeY,S=FMath::Min(W/1920.f,H/1080.f);
 static UFont* F=nullptr;if(!F){F=NewObject<UFont>();F->AddToRoot();F->FontCacheType=EFontCacheType::Runtime;FTypefaceEntry E;E.Name=TEXT("Regular");E.Font=FFontData(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);F->CompositeFont.DefaultTypeface.Fonts.Add(E);F->LegacyFontSize=20;}
 const FLinearColor Teal(.76,.07,.08),White(.91,.87,.84),Back(.009,.003,.005,.90);
 auto Box=[&](float X,float Y,float Width,float Height,FLinearColor Color){FCanvasTileItem I(FVector2D(X,Y),FVector2D(Width,Height),Color);I.BlendMode=SE_BLEND_Translucent;C->DrawItem(I);};
 auto Text=[&](FString T,float X,float Y,float Scale,FLinearColor Color){FCanvasTextItem I(FVector2D(X,Y),FText::FromString(T),F,Color);I.Scale=FVector2D(Scale*S);I.EnableShadow(FLinearColor::Black);C->DrawItem(I);};
 auto Line=[&](FVector2D A,FVector2D B,FLinearColor Color,float Width=1){FCanvasLineItem I(A,B);I.SetColor(Color);I.LineThickness=Width*S;C->DrawItem(I);};
 if(G.Screen==TEXT("story35")) {Text(TEXT("Space — пропустить реплику     Backspace — пропустить сцену"),W*.29f,H-44*S,.72,White);return;}
 if(!G.Screen.IsEmpty())return;
 if(G.HospitalStory&&G.HospitalStory->HomeStage52==1){Text(TEXT("Space — пропустить реплику · Backspace — весь разговор"),W*.30,H-47*S,.7,White);}
 // Checkmark is tied to an observed completion plus a changed objective.
 if(G.ObjectiveFade76>0){Box(34*S,180*S,680*S,58*S,Back);Line({52*S,204*S},{59*S,211*S},Teal,2.2f);Line({59*S,211*S},{73*S,193*S},Teal,2.2f);Text(G.CompletedObjective76.Left(65),84*S,194*S,.72,Teal);}
 const FString Use=Prompt(G);if(!Use.IsEmpty()&&!G.Hero()->Focus){Box(W*.30,H*.70,W*.40,48*S,Back);Text(Use.Left(78),W*.30+18*S,H*.70+12*S,.78,White);}
 if(G.Flag(TEXT("phone76_received"))&&!G.Flag(TEXT("phone76_read")))Text(TEXT("[1] Аружан · P — телефон"),W-302*S,414*S,.74,Teal);
 if(G.TutorialClock76<24){Box(34*S,265*S,690*S,78*S,Back);Text(TEXT("WASD — идти · Shift — бег · Ctrl — присесть · Space — прыжок"),52*S,280*S,.7,White);Text(TEXT("E / F — взаимодействие · F2 — полное обучение · P — телефон"),52*S,308*S,.7,Teal);}
 // Inventory hotkeys remain available without a permanent hotbar.
 // North-up circular floor plan, sampled only around the current player/floor.
 if(G.BookWorld&&G.SettingBool(TEXT("minimap76"),true)&&!(G.HospitalStory&&G.HospitalStory->HomeStage52>=0))
 {
  const FVector At=G.Hero()->GetActorLocation();const FVector2D Centre(W-166*S,245*S);const float R=119*S,Cell=4.2f*S;
  for(int Y=-28;Y<=28;++Y){const float Half=FMath::Sqrt(FMath::Max(0.f,R*R-FMath::Square(Y*Cell)));Box(Centre.X-Half,Centre.Y+Y*Cell,Half*2,Cell+1,Back);}
  const int Floor=FMath::FloorToInt((At.Z+10)/480)*480,X0=FMath::FloorToInt(At.X/100),Y0=FMath::FloorToInt(At.Y/100);
  for(int Y=-27;Y<=27;++Y)for(int X=-27;X<=27;++X)
  {
   if(X*X+Y*Y>26*26)continue;const auto* CellData=G.BookWorld->Cells.Find(FIntVector(X0+X,Y0+Y,Floor));if(!CellData)continue;
   const float PX=Centre.X+X*Cell,PY=Centre.Y+Y*Cell;
   Box(PX,PY,Cell+.15f*S,Cell+.15f*S,CellData->Outdoor?FLinearColor(.040,.009,.012,.92):FLinearColor(.10,.024,.030,.96));
   const auto* East=G.BookWorld->Cells.Find(FIntVector(X0+X+1,Y0+Y,Floor));const auto* South=G.BookWorld->Cells.Find(FIntVector(X0+X,Y0+Y+1,Floor));
   if(!East||East->Room!=CellData->Room)Line({PX+Cell,PY},{PX+Cell,PY+Cell},FLinearColor(.49,.13,.15,.85),.9f);
   if(!South||South->Room!=CellData->Room)Line({PX,PY+Cell},{PX+Cell,PY+Cell},FLinearColor(.49,.13,.15,.85),.9f);
  }
  for(int I=0;I<96;++I){const float A=I*2*PI/96,B=(I+1)*2*PI/96;Line(Centre+FVector2D(FMath::Cos(A),FMath::Sin(A))*R,Centre+FVector2D(FMath::Cos(B),FMath::Sin(B))*R,Teal,1.6);}
  for(int I=0;I<12;++I){const float A=I*2.39996f;const FVector2D P=Centre+FVector2D(FMath::Cos(A),FMath::Sin(A))*(R-2*S);Line(P,P+FVector2D(1,4+(I*7)%16)*S,FLinearColor(.42,.014,.023,.80),I%3+1);}
  for(const auto& P:G.BookWorld->Portals)if(FMath::Abs(P.At.Z-Floor)<100){const FVector2D D((P.At.X-At.X)/100*Cell,(P.At.Y-At.Y)/100*Cell);if(D.Size()<R-8*S)Box(Centre.X+D.X-2*S,Centre.Y+D.Y-2*S,4*S,4*S,P.Locked?FLinearColor(.8,.22,.16):Teal);}
  const float A=FMath::DegreesToRadians(G.Hero()->GetControlRotation().Yaw);const FVector2D Direction(FMath::Cos(A),FMath::Sin(A)),Right(-Direction.Y,Direction.X);Line(Centre-Direction*5*S+Right*5*S,Centre+Direction*9*S,White,2.2);Line(Centre-Direction*5*S-Right*5*S,Centre+Direction*9*S,White,2.2);Text(TEXT("С"),Centre.X-6*S,Centre.Y-R-24*S,.7,Teal);Text(TEXT("M — план · этаж ")+FString::FromInt(Floor/480+1),Centre.X-91*S,Centre.Y+R+12*S,.62,Teal);
 }
 if(G.ScareClock76>0)
 {
  const float Shock=FMath::Clamp(G.ScareClock76/1.8f,0.f,1.f);const float Alpha=Shock*.32f;
  Box(0,0,W,32*S,FLinearColor(.18,.005,.008,Alpha));Box(0,H-32*S,W,32*S,FLinearColor(.18,.005,.008,Alpha));Box(0,0,32*S,H,FLinearColor(.18,.005,.008,Alpha));Box(W-32*S,0,32*S,H,FLinearColor(.18,.005,.008,Alpha));
 }
}

void Assess(AEWardCampaignMode& G,float D)
{
 static int Step=0;static double Last=0;static Json Result=MakeShared<FJsonObject>();static TArray<TSharedPtr<FJsonValue>> Checks;static float StandingPelvis=0,StandingHeight=0;static int JournalCount=0;static bool SavedAI=false;
 const double Now=FPlatformTime::Seconds();if(Now-Last<.9)return;Last=Now;auto* H=G.Hero();auto* S=G.HospitalStory.Get();if(!H||!S||!G.BookWorld)return;
 auto Check=[&](const FString& Id,bool Pass){auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("id"),Id);R->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(R));UE_LOG(LogTemp,Display,TEXT("GAMEPLAY76 %s %s"),*Id,Pass?TEXT("PASS"):TEXT("FAIL"));};
 auto Picture=[&](const FString& Id){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/Gameplay76")/(Id+TEXT(".png")),true,false);};
 if(Step==0)
 {
  G.Assessment=true;G.SaveFolderOverride=FPaths::ProjectSavedDir()/TEXT("Gameplay76TestSaves");G.NewGame(TEXT("normal"));G.Show(TEXT(""));S->SetActorTickEnabled(false);SavedAI=S->FreezeAI;S->FreezeAI=true;
  Check(TEXT("eight_training_chapters"),Array(G.Data,TEXT("training")).Num()==8);Step++;return;
 }
 if(Step==1)
 {
  S->HomeStage52=-1;S->SetHomeVisible53(false);S->ArrivalStage=5;S->Seated31=false;S->ApplyArrivalState();S->SetChapter(2);S->ActiveScene35.Empty();G.Show(TEXT(""));H->SetActorHiddenInGame(false);H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
  for(const auto& R:G.BookWorld->Rooms)if(R.Id==TEXT("N02"))H->SetActorLocation(R.Inside+FVector(0,0,98),false);
  H->Controller->SetControlRotation(FRotator(-18,90,0));G.GetWorld()->GetFirstPlayerController()->SetViewTarget(H);H->UpdateFirstPerson50();
  G.Flag(TEXT("weapon"),true);G.Flag(TEXT("shotgun"),true);G.Items.Add(TEXT("medkit"),1);G.Items.Add(TEXT("battery"),1);G.Items.Add(TEXT("bottle"),1);H->Reserve=24;
  Check(TEXT("real_five_inventory_slots"),G.Inventory().Num()>=5);Slot(G,-1);Check(TEXT("empty_hands_disables_weapon"),!Armed(G));const int Rounds=H->Rounds;H->Aiming=true;G.FireWeapon();Check(TEXT("empty_hands_cannot_fire"),H->Rounds==Rounds);Slot(G,0);Check(TEXT("slot_one_equips_pistol"),Armed(G)&&!G.Shotgun);Slot(G,1);Check(TEXT("slot_two_equips_shotgun"),Armed(G)&&G.Shotgun);Slot(G,4);Check(TEXT("slot_five_matches_inventory"),G.HeldSlot76==G.Inventory()[4].Key);Slot(G,-1);
  StandingHeight=H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();StandingPelvis=H->BekaBody?H->BekaBody->GetSocketLocation(TEXT("pelvis")).Z:0;H->Crouch();H->GetCharacterMovement()->Crouch(false);Step++;return;
 }
 if(Step==2)
 {
  Check(TEXT("ctrl_changes_collision_capsule"),H->bIsCrouched&&H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()<StandingHeight-15);
  Check(TEXT("accepted_avatar_pelvis_crouches"),H->BekaBody&&H->BekaBody->GetSocketLocation(TEXT("pelvis")).Z<StandingPelvis-15);
  Picture(TEXT("crouch"));H->UnCrouch();H->GetCharacterMovement()->UnCrouch(false);Step++;return;
 }
 if(Step==3)
 {
  Check(TEXT("standing_capsule_restored"),!H->bIsCrouched&&FMath::IsNearlyEqual(H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),StandingHeight,1.f));
  G.Flag(TEXT("phone76_received"),true);G.Flag(TEXT("phone76_read"),false);G.Flag(TEXT("phone76_sent"),false);Phone(G);Tick(G,.5f);Tick(G,0);Check(TEXT("phone_model_loaded"),G.PhoneActor76&&G.PhoneActor76->FindComponentByClass<UStaticMeshComponent>()->GetStaticMesh()!=nullptr);Check(TEXT("phone_opens_with_mouse"),G.Screen==TEXT("phone76")&&G.GetWorld()->GetFirstPlayerController()->bShowMouseCursor);Picture(TEXT("phone_home"));Step++;return;
 }
 if(Step==4)
 {
  Command(G,TEXT("phone76_messages"));Check(TEXT("messages_app_opens"),G.PhonePage76==TEXT("messages"));Command(G,TEXT("phone76_aruzhan"));Check(TEXT("opening_aruzhan_marks_read"),G.Flag(TEXT("phone76_read")));JournalCount=G.Journal.Num();Command(G,TEXT("phone76_send"));Command(G,TEXT("phone76_send"));Check(TEXT("reply_sent_once"),G.Flag(TEXT("phone76_sent"))&&G.Journal.Num()==JournalCount+1);Picture(TEXT("phone_chat"));Step++;return;
 }
 if(Step==5)
 {
  Command(G,TEXT("phone76_insta"));Check(TEXT("offline_insta_opens"),G.PhonePage76==TEXT("insta"));Command(G,TEXT("phone76_weather"));Check(TEXT("weather_app_opens"),G.PhonePage76==TEXT("weather"));Command(G,TEXT("phone76_close"));Check(TEXT("phone_closes_and_restores_input"),G.Screen.IsEmpty()&&!G.PhoneOpen76&&!G.GetWorld()->GetFirstPlayerController()->bShowMouseCursor);
  const auto Save=G.Capture();const bool Loaded=G.Restore(Save);Check(TEXT("phone_reply_survives_save_restore"),Loaded&&G.Flag(TEXT("phone76_sent"))&&G.Flag(TEXT("phone76_read")));G.Show(TEXT(""));S->SetActorTickEnabled(false);S->ActiveScene35.Empty();
  G.PreviousObjective76=TEXT("Взять пистолет");G.LastCompleted76=G.Completed.Num();G.Completed.Add(TEXT("GAMEPLAY76_COMPLETED"));Tick(G,.01f);Check(TEXT("completed_objective_gets_checkmark"),G.ObjectiveFade76>0&&G.CompletedObjective76==TEXT("Взять пистолет"));Tick(G,4);Check(TEXT("completed_objective_disappears"),G.ObjectiveFade76<=0);Picture(TEXT("hud"));Step++;return;
 }
 if(Step==6)
 {
  int Count=0;for(int I=0;I<G.BookWorld->Portals.Num();++I)
  {
   auto& P=G.BookWorld->Portals[I];if(P.Zone!=TEXT("N02")||P.Side!=TEXT("north"))continue;
   const FVector Stand=P.At+FVector(0,-180,98);H->SetActorLocation(Stand,false);H->Controller->SetControlRotation((P.At+FVector(0,0,125)-(Stand+FVector(8,0,68))).Rotation());H->UpdateFirstPerson50();H->Camera->UpdateComponentToWorld();
   const bool Was=P.Open;P.Open=false;G.BookWorld->RefreshPortalVisual(P);Check(TEXT("entrance_closed_leaf_blocks"),P.Leaf&&P.Leaf->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
   const FRotator Aim=(P.At+FVector(0,0,125)-(Stand+FVector(8,0,68))).Rotation();H->Camera->SetWorldLocationAndRotation(Stand+FVector(8,0,68),Aim);
   const int Target=Door(*G.BookWorld);Check(TEXT("entrance_under_crosshair_is_selected"),Target!=INDEX_NONE&&G.BookWorld->Portals[Target].At.Equals(P.At,1));
   Check(TEXT("entrance_has_context_action_prompt"),Prompt(G).Contains(TEXT("E / F")));
   P.Open=true;G.BookWorld->RefreshPortalVisual(P);Check(TEXT("entrance_open_leaf_passable"),P.Leaf->GetCollisionEnabled()==ECollisionEnabled::NoCollision);P.Open=Was;G.BookWorld->RefreshPortalVisual(P);Count++;
  }
  Check(TEXT("entrance_doors_present"),Count>0);G.Show(TEXT("training"));Picture(TEXT("training"));Step++;return;
 }
 if(Step==7)
 {
  G.Show(TEXT(""));S->HomeStage52=1;S->HomeLine52=0;S->HomeLineTime69=20;Skip(G);Check(TEXT("space_skips_home_line"),S->HomeLineTime69==0);Skip(G,true);Check(TEXT("backspace_skips_home_conversation"),S->HomeLine52==S->HomeLines52().Num()-1);S->HomeStage52=-1;
  // Exercise the real main-dialogue cursor without selecting a branch.
  S->BeginNarrative35(TEXT("CHECKPOINT_69"),true);WardDialogue62::Tick(*S,.1f);Skip(G,true);Check(TEXT("scene_skip_keeps_unselected_choice"),S->SelectedChoice35.IsEmpty());G.Show(TEXT(""));S->ActiveScene35.Empty();
  G.ScareCooldown76=0;H->SetActorLocation(FVector(5000,-2500,98),false);H->Controller->SetControlRotation(FRotator(0,0,0));H->UpdateFirstPerson50();Scare(G);Check(TEXT("scare_has_visible_monster"),G.ScareActor76&&G.ScareClock76>0);Check(TEXT("scare_does_not_damage_player"),H->Health>0);Picture(TEXT("scare"));Step++;return;
 }
 if(Step==8)
 {
  Tick(G,2);Check(TEXT("scare_clears_after_short_duration"),G.ScareActor76&&G.ScareActor76->IsHidden());Check(TEXT("scare_has_cooldown"),G.ScareCooldown76>50);
  const auto Doors=G.BookWorld->AuditEveryDoor();Write(FPaths::ProjectSavedDir()/TEXT("Prototype/doors76_check.json"),Doors);Check(TEXT("all_hospital_doors_open_close_and_restore"),Boolean(Doors,TEXT("success")));
  bool Success=true;for(auto V:Checks)Success&=Boolean(V->AsObject(),TEXT("passed"));Result->SetBoolField(TEXT("success"),Success);Result->SetNumberField(TEXT("count"),Checks.Num());Result->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/gameplay76_check.json"),Result);S->FreezeAI=SavedAI;UKismetSystemLibrary::QuitGame(&G,G.GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false);Step++;
 }
}
}
