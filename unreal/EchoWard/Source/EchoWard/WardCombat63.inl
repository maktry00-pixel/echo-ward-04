#include "WardCombat63.h"
#include "WardHomeAssets53.h"
#include "WardPhysicsProp.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/AudioComponent.h"
#include "Misc/Paths.h"
namespace WardCombat63
{
// Gameplay state is saved in campaign flags. Transient meshes/audio never own gameplay state.
struct FPresentation
{
 TWeakObjectPtr<AEWardCampaignHero> Hero;
 TArray<TWeakObjectPtr<AActor>> Stains;
 TArray<TWeakObjectPtr<UProceduralMeshComponent>> BodyStains;
 float ReloadPrevious=0,DripClock=0,BeatClock=0;
 int BreathPulse=0;
 bool ReloadWasShotgun=false;
};
static TMap<TWeakObjectPtr<AEWardCampaignMode>,FPresentation> Presentations;
static FPresentation& State(AEWardCampaignMode& G)
{
 for(auto It=Presentations.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 auto& S=Presentations.FindOrAdd(&G);
 if(S.Hero.Get()!=G.Hero()){for(auto P:S.BodyStains)if(P.IsValid())P->DestroyComponent();S=FPresentation();S.Hero=G.Hero();}
 return S;
}
bool Healing(const AEWardCampaignMode& G){return G.NumberFlag(TEXT("combat63_heal"))>0;}
float RangeScale(bool Long,float Cm){return Long?FMath::Lerp(1.f,.18f,FMath::Clamp((Cm-800.f)/2200.f,0.f,1.f)):FMath::Lerp(1.f,.7f,FMath::Clamp((Cm-3000.f)/3000.f,0.f,1.f));}
float BodyScale(float Fraction){return Fraction>.83f?2.f:Fraction<.30f?.65f:1.f;}
float Trigger(const AEWardCampaignMode& G){return G.ReloadTime<=0&&G.Hero()&&G.Hero()->Aiming?FMath::Clamp(G.ShotCooldown/(G.Shotgun?.85f:.32f),0.f,1.f):0.f;}
float OpenHand(const AEWardCampaignMode& G){return G.ReloadTime>0?FMath::Sin(FMath::Clamp(1-G.ReloadTime/(G.Shotgun?.7f:1.8f),0.f,1.f)*PI)*.7f:0;}
float ActionTravel(const AEWardCampaignMode& G)
{
 if(G.Shotgun){const float Age=.85f-G.ShotCooldown;return G.ShotCooldown>0?9*FMath::Min(FMath::SmoothStep(.14f,.31f,Age),1-FMath::SmoothStep(.40f,.62f,Age)):0;}
 if(G.ReloadTime>0)return WardPistolPose::Reload(G.ReloadTime).SlidePull;
 if(G.Hero()&&G.Hero()->Rounds==0&&G.ReloadTime<=0)return 2;
 const float Age=.32f-G.ShotCooldown;return G.ShotCooldown>0?2*FMath::Min(FMath::Clamp(Age/.018f,0.f,1.f),FMath::Clamp((.095f-Age)/.065f,0.f,1.f)):0;
}
FVector PreviewGrip(const FString& Id,bool Left)
{
 const float Side=Left?-1.f:1.f;
 if(Id.StartsWith(TEXT("doc:"))||Id==TEXT("map"))return FVector(-2,Side*10,-5);
 if(Id==TEXT("fuel")||Id==TEXT("coolant")||Id==TEXT("medkit")||Id==TEXT("boards"))return FVector(-2,Side*7,-8);
 if(Id==TEXT("badge")||Id.Contains(TEXT("key"))||Id==TEXT("fuse"))return FVector(-1,Side*3,-3);
 return FVector(-2,Side*4,-5);
}
void Sound(AEWardCampaignMode& G,const TCHAR* Clip,FVector At,float Volume)
{
 const FString Path=FPaths::ProjectContentDir()/TEXT("CampaignData/Combat63/audio")/(FString(Clip)+TEXT(".wav"));
 WardHomeAssets::PlaySpatialFile61(&G,Path,FMath::Clamp(Volume*float(G.Setting(TEXT("effects"),.8)),0.f,1.f),At);
}
static UProceduralMeshComponent* Patch(AActor* Owner,FVector Normal,float Radius,int Seed)
{
 auto* Mesh=NewObject<UProceduralMeshComponent>(Owner);Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCastShadow(false);
 FVector U,V;Normal.GetSafeNormal().FindBestAxisVectors(U,V);TArray<FVector> P{FVector::ZeroVector},N{Normal};TArray<int32> T;TArray<FVector2D> UV{FVector2D(.5,.5)};FRandomStream R(Seed);
 for(int I=0;I<16;++I){const float A=I*2*PI/16,Length=Radius*R.FRandRange(.55f,1.f);P.Add((U*FMath::Cos(A)+V*FMath::Sin(A))*Length);N.Add(Normal);UV.Add(FVector2D(.5+.5*FMath::Cos(A),.5+.5*FMath::Sin(A)));T.Append({0,I+1,(I+1)%16+1});T.Append({0,(I+1)%16+1,I+1});}
 Mesh->CreateMeshSection(0,P,T,N,UV,TArray<FColor>(),TArray<FProcMeshTangent>(),false);
 if(auto* Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_CampaignPBR.M_CampaignPBR"))){auto* M=UMaterialInstanceDynamic::Create(Parent,Owner);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.12,.002,.004));M->SetScalarParameterValue(TEXT("Roughness"),.28);Mesh->SetMaterial(0,M);}
 Mesh->RegisterComponent();return Mesh;
}
void Blood(AEWardCampaignMode& G,FVector Point,FVector Normal,float Scale)
{
 auto& S=State(G);S.Stains.RemoveAll([](auto P){return !P.IsValid();});
 if(S.Stains.Num()>=32){if(S.Stains[0].IsValid())S.Stains[0]->Destroy();S.Stains.RemoveAt(0);}
 auto* A=G.GetWorld()->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(A);A->SetRootComponent(Root);Root->RegisterComponent();A->SetActorLocation(Point+Normal*.35);
 auto* P=Patch(A,Normal,8*FMath::Clamp(Scale,.4f,2.f),S.Stains.Num()*37+G.Hits);P->AttachToComponent(Root,FAttachmentTransformRules::KeepRelativeTransform);A->SetLifeSpan(45);S.Stains.Add(A);
}
void HitBlood(AEWardCampaignMode& G,FVector Point,FVector Direction,float Scale)
{
 // Blood lands only on a traced supporting surface; never leave a floating decal at a wound.
 FHitResult Floor;FCollisionQueryParams Q(SCENE_QUERY_STAT(BloodFloor63),false,G.Hero());
 if(G.GetWorld()->LineTraceSingleByObjectType(Floor,Point+Direction*18,Point+Direction*18-FVector(0,0,210),FCollisionObjectQueryParams(ECC_WorldStatic),Q)&&Floor.ImpactNormal.Z>.5)Blood(G,Floor.ImpactPoint,Floor.ImpactNormal,Scale);
}
void BeginHeal(AEWardCampaignMode& G)
{
 auto* H=G.Hero();if(!H||H->Dead||H->Health>=100||Healing(G)||G.ReloadTime>0||G.Items.FindRef(TEXT("medkit"))<=0||!G.Screen.IsEmpty()||G.HidingBed||(G.BookWorld&&G.BookWorld->FurnitureHiding)||!G.PreviewItem51.IsEmpty()||(G.HospitalStory&&G.HospitalStory->ArrivalCinematic()))return;
 // Reserve the medkit at start; damage interrupts healing without restoring spent supplies.
 G.Consume(TEXT("medkit"));G.NumberFlag(TEXT("combat63_heal"),2.4);H->Aiming=false;Sound(G,TEXT("bandage"),H->GetActorLocation(),.35);G.Notify(TEXT("Перевязка · 2,4 с. Удар прервёт лечение."),2.4);
}
void Wound(AEWardCampaignMode& G,float Damage,AActor* Source)
{
 auto* H=G.Hero();G.NumberFlag(TEXT("combat63_heal"),0);G.ReloadTime=0;
 G.NumberFlag(TEXT("combat63_blood"),FMath::Clamp(G.NumberFlag(TEXT("combat63_blood"))+Damage/100.,0.,1.));
 G.NumberFlag(TEXT("combat63_bleed"),45);G.NumberFlag(TEXT("combat63_trauma"),FMath::Clamp(G.NumberFlag(TEXT("combat63_trauma"))+Damage/35.,0.,1.));
 Sound(G,TEXT("pain"),H->Camera->GetComponentLocation(),.25);HitBlood(G,H->GetActorLocation(),H->GetActorForwardVector(),.7);
}
void Shot(AEWardCampaignMode& G)
{
 auto* H=G.Hero();const FTransform Gun=H->WeaponPose();
 G.NumberFlag(TEXT("combat63_pump"),G.Shotgun?.28:0);
 G.NumberFlag(TEXT("combat63_kick_velocity"),G.NumberFlag(TEXT("combat63_kick_velocity"))+(G.Shotgun?100:55));
 Sound(G,G.Shotgun?TEXT("shotgun"):TEXT("pistol"),Gun.GetLocation(),.8);
 if(!G.Shotgun)G.SpawnPhysics(TEXT("case"),Gun.TransformPosition(FVector(1,5,1)),Gun.TransformVectorNoScale(FVector(25,180,130))+H->GetVelocity(),FVector(.9,.9,2.2),.007);
}
bool Observed(const AEWardCampaignMode& G)
{
 if(!G.Hero())return false;const FVector At=G.Hero()->GetActorLocation();
 for(auto E:G.Enemies)if(E&&E->Active&&!E->Dead&&E->Memory>0&&FVector::Dist(E->Observed,At)<150)return true;
 if(G.HospitalStory)for(auto M:G.HospitalStory->Mobs)if(M&&M->Active&&!M->Dead&&M->Memory>0&&FVector::Dist(M->LastSeen59,At)<180&&M->Sight59(At,true))return true;
 return false;
}
void Tick(AEWardCampaignMode& G,float Delta)
{
 auto* H=G.Hero();if(!H)return;auto& S=State(G);const float D=FMath::Clamp(Delta,0.f,.1f);
 if(Healing(G))
 {
  H->Aiming=false;const float Left=FMath::Max(0.,G.NumberFlag(TEXT("combat63_heal"))-Delta);G.NumberFlag(TEXT("combat63_heal"),Left);
  if(Left<=0&&!H->Dead){H->Health=FMath::Min(100.f,H->Health+33.34f*G.DifficultyValue(TEXT("heal")));G.NumberFlag(TEXT("combat63_bleed"),0);G.Notify(TEXT("Перевязка закончена"),1.5);}
 }
 // Stable critically damped spring, independent of frame rate (substeps <= 1/120 s).
 float X=G.NumberFlag(TEXT("combat63_kick")),V=G.NumberFlag(TEXT("combat63_kick_velocity"));
 for(float Left=D;Left>0;){const float Dt=FMath::Min(Left,1.f/120);V+=(-225*X-30*V)*Dt;X+=V*Dt;Left-=Dt;}
 G.NumberFlag(TEXT("combat63_kick"),FMath::Clamp(X,-1.f,5.f));G.NumberFlag(TEXT("combat63_kick_velocity"),V);
 const float Pump=G.NumberFlag(TEXT("combat63_pump"));
 if(Pump>0){G.NumberFlag(TEXT("combat63_pump"),FMath::Max(0.f,Pump-Delta));if(Pump<=Delta){const FTransform Gun=H->WeaponPose();G.SpawnPhysics(TEXT("case"),Gun.TransformPosition(FVector(8,5,1)),Gun.TransformVectorNoScale(FVector(20,155,120))+H->GetVelocity(),FVector(1.6,1.6,5),.015);Sound(G,TEXT("pump"),Gun.GetLocation(),.35);}}
 const float Reload=G.ReloadTime;
 if(Reload>0){const float Duration=G.Shotgun?.7f:WardPistolPose::ReloadDuration,Now=Duration-Reload,Before=S.ReloadPrevious>Reload&&S.ReloadWasShotgun==G.Shotgun?Duration-S.ReloadPrevious:0;
  for(int I=0;I<3;++I){const float At=G.Shotgun?(I==0?.08f:I==1?.35f:.59f):(I==0?WardPistolPose::ReleaseTime:I==1?WardPistolPose::SeatTime:WardPistolPose::SlideTime);if(Before<At&&Now>=At)Sound(G,G.Shotgun?TEXT("shell"):I==0?TEXT("magout"):I==1?TEXT("magin"):TEXT("slide"),H->Pistol->GetComponentLocation(),.35);}
 }S.ReloadPrevious=Reload;S.ReloadWasShotgun=G.Shotgun;
 const float Trauma=FMath::Max(0.,G.NumberFlag(TEXT("combat63_trauma"))-D*.55);G.NumberFlag(TEXT("combat63_trauma"),Trauma);
 const float Bleed=FMath::Max(0.,G.NumberFlag(TEXT("combat63_bleed"))-Delta);G.NumberFlag(TEXT("combat63_bleed"),Bleed);
 const float Load=FMath::Clamp(float(G.NumberFlag(TEXT("combat63_blood"))),0.f,1.f);
 if(Load<=0){for(auto P:S.BodyStains)if(P.IsValid())P->DestroyComponent();S.BodyStains.Reset();}
 if(Load>.05f&&S.BodyStains.IsEmpty())
 {
  // Small organic patches on visible sleeves/hands, attached to skeletal bones, never collision bodies.
  TArray<USkeletalMeshComponent*> Bodies;H->GetComponents(Bodies,true);
  for(auto* B:Bodies)if(B!=H->GetMesh()&&B->DoesSocketExist(TEXT("hand_r")))
  {for(const FString Side:{FString(TEXT("r")),FString(TEXT("l"))}){const FName Bone(*(TEXT("hand_")+Side));const FTransform At=B->GetSocketTransform(Bone);const FVector Base=At.GetLocation(),Index=B->GetSocketLocation(FName(*(TEXT("index_01_")+Side))),Pinky=B->GetSocketLocation(FName(*(TEXT("pinky_01_")+Side)));FVector Normal=FVector::CrossProduct(Index-Base,Pinky-Base).GetSafeNormal();if(FVector::DotProduct(Normal,H->Camera->GetComponentLocation()-Base)<0)Normal=-Normal;auto* P=Patch(B->GetOwner(),At.InverseTransformVectorNoScale(Normal),1.8f,12+S.BodyStains.Num());P->AttachToComponent(B,FAttachmentTransformRules::KeepRelativeTransform,Bone);P->SetRelativeLocation(At.InverseTransformPosition((Base*2+Index+Pinky)/4+Normal*1.3));S.BodyStains.Add(P);}
   const FName Chest(TEXT("spine_05"));if(B->DoesSocketExist(Chest)){const FTransform At=B->GetSocketTransform(Chest);const FVector Forward=H->GetActorForwardVector();auto* P=Patch(B->GetOwner(),At.InverseTransformVectorNoScale(Forward),5.f,36);P->AttachToComponent(B,FAttachmentTransformRules::KeepRelativeTransform,Chest);P->SetRelativeLocation(At.InverseTransformVectorNoScale(Forward*15));S.BodyStains.Add(P);}break;}
 }
 S.DripClock-=Delta;if(Bleed>0&&S.DripClock<=0&&!H->Dead){HitBlood(G,H->GetActorLocation(),H->GetActorForwardVector(),.4f+Load*.4f);S.DripClock=FMath::Lerp(2.4f,.65f,Load);}
 const auto Respiration=WardMotion107::Frame(*H);
 const bool Exhales=Respiration.Exhale>S.BreathPulse;
 S.BreathPulse=Respiration.Exhale;
 if(Exhales&&Respiration.Active&&Respiration.Effort>.12f&&!H->Dead)
     Sound(G,TEXT("breath"),H->Camera->GetComponentLocation(),.07f+Respiration.Effort*.19f);
 S.BeatClock-=Delta;if(H->Health<=25&&S.BeatClock<=0&&!H->Dead){Sound(G,TEXT("heartbeat"),H->Camera->GetComponentLocation(),.14);S.BeatClock=.9;}
 auto& PP=H->Camera->PostProcessSettings;PP.bOverride_VignetteIntensity=true;PP.VignetteIntensity=FMath::Clamp(.15f+Trauma*.3f+(100-H->Health)*.001f,0.f,.58f);
 PP.bOverride_SceneFringeIntensity=true;PP.SceneFringeIntensity=Trauma*1.1f*G.Setting(TEXT("shake"));
 PP.bOverride_DepthOfFieldFstop=PP.bOverride_DepthOfFieldFocalDistance=Trauma>.1f&&G.SettingBool(TEXT("motion_blur"),false);PP.DepthOfFieldFstop=8;PP.DepthOfFieldFocalDistance=600;
 PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=G.SettingBool(TEXT("motion_blur"),false)?Trauma*.12f:0;
}
}
