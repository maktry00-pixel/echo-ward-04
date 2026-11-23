#include "WardStaging64.h"
#include "WardPeople54.h"
#include "WardMonster59.h"
#include "WardDialogue62.h"
#include "WardBekaVisual50.h"
#include "WardHomeAssets53.h"
#include "WardEchoAssets.h"
#include "WardRescueAircraft67.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Misc/App.h"
namespace WardStaging64
{
using namespace WardData;
struct FStageWorld
{
 FString Token;
 TArray<TWeakObjectPtr<AActor>> Props;
 TWeakObjectPtr<AActor> Bed,Vehicle;
 TWeakObjectPtr<USceneComponent> Lever,Rotor,TailRotor;
 TWeakObjectPtr<UAudioComponent> EngineAudio;
 TWeakObjectPtr<UPointLightComponent> Lamp;
 struct FApproach
 {
  FVector Position=FVector::ZeroVector;
  float Stalled=0;
 };
 TMap<FString,FApproach> Approaches;
 int Portal=-1;
};
static TMap<TWeakObjectPtr<AEWardHospitalGameplay>,FStageWorld> Worlds;
Json Data(){static Json D=Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Staging64/scenes.json"));return D;}
static Json Root(AEWardHospitalGameplay& S){if(!S.NarrativeState35->HasField(TEXT("staging64")))S.NarrativeState35->SetObjectField(TEXT("staging64"),MakeShared<FJsonObject>());return Object(S.NarrativeState35,TEXT("staging64"));}
static Json Child(Json P,const TCHAR* Key){if(!P->HasField(Key))P->SetObjectField(Key,MakeShared<FJsonObject>());return Object(P,Key);}
static Json Current(AEWardHospitalGameplay& S){return Child(Root(S),TEXT("active"));}
static FStageWorld& World(AEWardHospitalGameplay& S){for(auto I=Worlds.CreateIterator();I;++I)if(!I.Key().IsValid())I.RemoveCurrent();return Worlds.FindOrAdd(&S);}
static UEWardPerson54* Person(AEWardHospitalGameplay& S,const FString& Id){auto A=S.Props.FindRef(Id);return A?A->FindComponentByClass<UEWardPerson54>():nullptr;}
static bool Alive(AEWardHospitalGameplay& S,const FString& Id)
{
 auto* P=Person(S,Id);if(!P||P->IsDead58()||Boolean(S.Definitions.FindRef(Id),TEXT("past57")))return false;
 if(Id==TEXT("PERSON_2")&&!S.NarrativeFlag35(TEXT("RUDIN_ALIVE")))return false;
 if(Id==TEXT("PERSON_4")&&!S.NarrativeFlag35(TEXT("NIKITA_ALIVE")))return false;
 if(Boolean(Child(Root(S),TEXT("evacuated")),Id))return false;
 return true;
}
bool Enabled(const AEWardHospitalGameplay& S)
{
 if(!S.NarrativeEnabled35||S.NarrativeTesting35||Number(Data(),TEXT("version"))!=64)return false;
 const TCHAR* C=FCommandLine::Get();
 if(FParse::Param(C,TEXT("WardStagingCheck64")))return true;
 // Existing regression fixtures retain their contract; this stage has a dedicated integration suite.
 for(const TCHAR* F:{TEXT("WardScenarioCheck"),TEXT("WardDialogueCheck62"),TEXT("WardMonstersCheck59"),TEXT("WardHumansCheck58"),TEXT("WardPeopleCheck54"),TEXT("WardBodyPreviewCheck51"),TEXT("WardCombatCheck63"),TEXT("WardStoryCheck"),TEXT("WardArrivalCheck")})if(FParse::Param(C,F))return false;
 return S.Mode&&S.Mode->CampaignStarted&&!S.Mode->TitleView;
}
bool Active(const AEWardHospitalGameplay& S){return Enabled(S)&&(!S.NarrativeReplay35||((S.ActiveScene35==TEXT("SCENE_01_01")||S.ActiveScene35==TEXT("SCENE_01_02"))&&!Boolean(Object(Object(S.NarrativeState35,TEXT("staging64")),TEXT("history")),S.ActiveScene35)))&&S.ActiveScene35.StartsWith(TEXT("SCENE_"))&&Object(Data(),TEXT("scenes"))->HasField(S.ActiveScene35);}
static Json Scene(AEWardHospitalGameplay& S){return Object(Object(Data(),TEXT("scenes")),S.ActiveScene35);}
static TArray<Json> Actions(AEWardHospitalGameplay& S)
{
 TArray<Json> Out;const Json Def=Scene(S);for(auto V:Array(Def,TEXT("lead")))Out.Add(V->AsObject());
 if(!S.SelectedChoice35.IsEmpty()){for(auto V:Array(Object(Def,TEXT("branches")),S.SelectedChoice35))Out.Add(V->AsObject());for(auto V:Array(Def,TEXT("outro")))Out.Add(V->AsObject());}
 else if(Array(Def,TEXT("choices")).IsEmpty())for(auto V:Array(Def,TEXT("outro")))Out.Add(V->AsObject());
 return Out;
}
static FString Plan(AEWardHospitalGameplay& S){if(S.NarrativeFlag35(TEXT("OUTBREAK")))return TEXT("OUTBREAK");return String(Object(S.NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"),TEXT("CURE"));}
static void ClearProps(AEWardHospitalGameplay& S){auto& W=World(S);for(auto A:W.Props)if(A.IsValid())A->Destroy();W.Props.Reset();W.Bed=nullptr;W.Vehicle=nullptr;W.Lever=nullptr;W.Rotor=nullptr;W.TailRotor=nullptr;W.Lamp=nullptr;W.Portal=-1;}
void Reset(AEWardHospitalGameplay& S){ClearProps(S);World(S).Token.Empty();World(S).Approaches.Reset();}
static FVector Anchor(AEWardHospitalGameplay& S){if(S.ActiveScene35==TEXT("SCENE_01_01")&&S.ArrivalCar)return S.ArrivalCar->GetActorLocation();if(auto A=S.Props.FindRef(S.ActiveScene35))return A->GetActorLocation();return S.Mode->Hero()->GetActorLocation()-FVector(0,0,91);}
static bool UnsafeStationaryMark(AEWardHospitalGameplay& S,const FVector& Feet)
{
 for(auto M:S.Mobs)if(M&&M->Active&&!M->Dead&&!M->IsHidden()&&
  Boolean(M->Rule59,TEXT("stationary"))&&FMath::Abs(M->GetActorLocation().Z-Feet.Z)<250&&
  WardMonster59::ContactThreatAt(*M,Feet,120.f))return true;
 return false;
}
static bool Spot(AEWardHospitalGameplay& S,FVector Centre,int Slot,FVector& Out,Json Reserved={})
{
 FCollisionQueryParams Q(SCENE_QUERY_STAT(StageSpot64),false,S.Mode->Hero());for(auto E:S.Props)Q.AddIgnoredActor(E.Value);for(auto M:S.Mobs)Q.AddIgnoredActor(M.Get());for(auto A:World(S).Props)if(A.IsValid())Q.AddIgnoredActor(A.Get());
 const float Floor=FMath::RoundToFloat(Centre.Z/480)*480;
 for(float Radius:{100.f,145.f,190.f,65.f,230.f,280.f})for(int I=0;I<16;++I)
 {
  const float Angle=(I+Slot*3)*PI/8;const FVector P(Centre.X+FMath::Cos(Angle)*Radius,Centre.Y+FMath::Sin(Angle)*Radius,Floor+150);
  FHitResult Hit;if(!S.GetWorld()->LineTraceSingleByChannel(Hit,P,P-FVector(0,0,220),ECC_Visibility,Q)||Hit.ImpactNormal.Z<.75||FMath::Abs(Hit.ImpactPoint.Z-Floor)>35)continue;
  const FVector Feet=Hit.ImpactPoint+FVector(0,0,.7);if(UnsafeStationaryMark(S,Feet))continue;if(FVector::Dist2D(Feet,S.Mode->Hero()->GetActorLocation())<125)continue;if(S.GetWorld()->OverlapBlockingTestByChannel(Feet+FVector(0,0,87),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(29,84),Q))continue;
  const Json Goals=Reserved?Reserved:Child(Current(S),TEXT("goals"));
  bool Taken=false;for(auto E:Goals->Values)if(FVector::Dist2D(Vector(Array(E.Value->AsObject(),TEXT("at"))),Feet)<132)Taken=true;if(Taken)continue;Out=Feet;return true;
 }return false;
}
// Scene marks are authored story locations. Set up remote participants
// before the scene is in view so that an unrelated patrol route cannot hold
// a mandatory scene on another wing or floor.
static bool InView(AEWardHospitalGameplay& S,FVector Feet,const AActor* Participant)
{
 auto* Hero=S.Mode->Hero();
 if(!Hero||!Hero->Camera)return true;
 const FVector Eye=Hero->Camera->GetComponentLocation();
 const FVector Chest=Feet+FVector(0,0,105);
 const FVector Offset=Chest-Eye;
 if(Offset.SizeSquared()>FMath::Square(1600.))return false;
 if(FVector::DotProduct(Offset.GetSafeNormal(),Hero->Camera->GetForwardVector())<.45)return false;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(WardStageVisibility73),false,Hero);
 if(Participant)Query.AddIgnoredActor(Participant);
 FHitResult Hit;
 return !S.GetWorld()->LineTraceSingleByChannel(Hit,Eye,Chest,ECC_Visibility,Query);
}
static bool StageRemoteParticipant(AEWardHospitalGameplay& S,const FString& Id,FVector Mark,bool SceneOpening)
{
 auto* Actor=S.Props.FindRef(Id);
 auto* Model=Person(S,Id);
 if(!Actor||!Model||Model->IsFallen58()||Model->HumanState58()==TEXT("flee"))return false;
 const FVector From=Actor->GetActorLocation();
 if(FVector::DistSquared2D(From,Mark)<FMath::Square(SceneOpening?600.:2400.)&&FMath::Abs(From.Z-Mark.Z)<180)return false;
 if(!Actor->IsHidden()&&InView(S,From,Actor))return false;
 if(InView(S,Mark,Actor))
 {
  if(!SceneOpening)return false;
  // This is the opening cut of an explicitly entered scene.
  auto* PC=S.GetWorld()->GetFirstPlayerController();
  if(!PC||!PC->PlayerCameraManager)return false;
  PC->PlayerCameraManager->StartCameraFade(1.f,0.f,.3f,FLinearColor::Black,false);
 }
 if(!Actor->SetActorLocation(Mark,false,nullptr,ETeleportType::TeleportPhysics))return false;
 const FVector Face=S.Mode->Hero()->GetActorLocation()-Mark;
 Model->SetWorldRotation(FRotator(0,Face.Rotation().Yaw,0));
 UE_LOG(LogTemp,Display,TEXT("STAGE73 %s %s at %s"),*S.ActiveScene35,*Id,*Mark.ToString());
 return true;
}
static void SetGoal(AEWardHospitalGameplay& S,const FString& Id,FVector At,const FString& Pose,float Speed=95)
{
 if(!Alive(S,Id))return;World(S).Approaches.Remove(Id);Json G=MakeShared<FJsonObject>();G->SetArrayField(TEXT("at"),TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(At.X),MakeShared<FJsonValueNumber>(At.Y),MakeShared<FJsonValueNumber>(At.Z)});G->SetStringField(TEXT("pose"),Pose);G->SetNumberField(TEXT("speed"),Speed);Child(Current(S),TEXT("goals"))->SetObjectField(Id,G);Person(S,Id)->SetTask58(TEXT("scene"),At);S.Props[Id]->SetActorHiddenInGame(false);
}
bool Controls(AEWardHospitalGameplay& S,const FString& Id){return Active(S)&&Child(Current(S),TEXT("goals"))->HasField(Id);}
bool Goal(AEWardHospitalGameplay& S,const FString& Id,FVector& At,FString& Pose,float& Speed)
{
 if(!Controls(S,Id))return false;Json G=Object(Child(Current(S),TEXT("goals")),Id);At=Vector(Array(G,TEXT("at")));Pose=String(G,TEXT("pose"),TEXT("idle"));Speed=Number(G,TEXT("speed"),95);
 if(S.NarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"))){At=S.Props[Id]->GetActorLocation();Speed=0;}
 return true;
}
FString PoseName(AEWardHospitalGameplay& S,const FString& Id){FVector A;FString P;float V;return Goal(S,Id,A,P,V)?P:FString();}
static UStaticMeshComponent* Mesh(AEWardHospitalGameplay& S,AActor* A,USceneComponent* Parent,const TCHAR* Path,FVector Position,FVector Scale,FLinearColor Color=FLinearColor(.12,.16,.17))
{
 auto* C=NewObject<UStaticMeshComponent>(A);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));C->SetRelativeLocation(Position);C->SetRelativeScale3D(Scale);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetLightingChannels(true,true,false);C->SetCullDistance(6000);
 if(FString(Path).StartsWith(TEXT("/Engine/")))if(auto* P=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Campaign/Materials/M_CampaignPBR.M_CampaignPBR"))){auto* M=UMaterialInstanceDynamic::Create(P,A);M->SetVectorParameterValue(TEXT("Tint"),Color);M->SetScalarParameterValue(TEXT("Roughness"),.65);C->SetMaterial(0,M);}C->RegisterComponent();A->AddInstanceComponent(C);return C;
}
static AActor* Prop(AEWardHospitalGameplay& S,FVector At)
{
 auto* A=S.GetWorld()->SpawnActor<AActor>();auto* C=NewObject<USceneComponent>(A);A->SetRootComponent(C);C->SetMobility(EComponentMobility::Movable);C->RegisterComponent();A->SetActorLocation(At);World(S).Props.Add(A);return A;
}
static void Panel(AEWardHospitalGameplay& S)
{
 auto& W=World(S);if(W.Lever.IsValid())return;auto* A=Prop(S,Anchor(S));
 Mesh(S,A,A->GetRootComponent(),TEXT("/Game/Campaign/HospitalV37/Meshes/SM_H37_3321.SM_H37_3321"),FVector(0,0,85),FVector(.7));
 auto* Pivot=NewObject<USceneComponent>(A);Pivot->SetupAttachment(A->GetRootComponent());Pivot->SetRelativeLocation(FVector(0,-18,120));Pivot->RegisterComponent();W.Lever=Pivot;
 Mesh(S,A,Pivot,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(0,0,12),FVector(.035,.035,.25),FLinearColor(.7,.06,.025));
 auto* L=NewObject<UPointLightComponent>(A);L->SetupAttachment(A->GetRootComponent());L->SetRelativeLocation(FVector(0,-20,145));L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(30);L->SetAttenuationRadius(230);L->SetCastShadows(false);L->RegisterComponent();W.Lamp=L;
}
static void Bed(AEWardHospitalGameplay& S,const FString& Id)
{
 if(World(S).Bed.IsValid()||!Alive(S,Id))return;const FVector At=S.Props[Id]->GetActorLocation();auto* A=Prop(S,At);A->SetActorRotation(FRotator(0,90,0));Mesh(S,A,A->GetRootComponent(),TEXT("/Game/Campaign/HospitalV37/Meshes/SM_H37_3302.SM_H37_3302"),FVector::ZeroVector,FVector::OneVector);World(S).Bed=A;
}
static void Vehicle(AEWardHospitalGameplay& S,bool Air)
{
 auto& W=World(S);if(W.Vehicle.IsValid())return;auto* A=Prop(S,Anchor(S)+FVector(Air?380:450,0,0));W.Vehicle=A;
 if(Air)
 {
  auto Add=[&](UStaticMesh* Asset,FVector Position,FRotator Rotation)
  {
   if(!Asset)return static_cast<UStaticMeshComponent*>(nullptr);
   auto* Component=NewObject<UStaticMeshComponent>(A);
   Component->SetupAttachment(A->GetRootComponent());
   Component->SetMobility(EComponentMobility::Movable);
   Component->SetStaticMesh(Asset);
   Component->SetRelativeLocation(Position);
   Component->SetRelativeRotation(Rotation);
   Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Component->SetLightingChannels(true,true,false);
   Component->SetCullDistance(15000);
   Component->RegisterComponent();
   A->AddInstanceComponent(Component);
   return Component;
  };
  const auto* Body=Add(WardVisual67::RescueAircraft(A),FVector(0,0,-11),FRotator::ZeroRotator);
  W.Rotor=Add(WardVisual67::Rotor(A,false),FVector(-6,0,278),FRotator::ZeroRotator);
  W.TailRotor=Add(WardVisual67::Rotor(A,true),FVector(-553,21,193),FRotator(0,0,90));
  UE_LOG(LogTemp,Display,TEXT("VISUAL67_AIRCRAFT body=%d main=%d tail=%d"),Body!=nullptr,W.Rotor.IsValid(),W.TailRotor.IsValid());
 }
 else
 {
  for(int Id:{1018,1118})if(auto* Native=WardEchoAssets::Mesh(Id)){auto* C=NewObject<UStaticMeshComponent>(A);C->SetupAttachment(A->GetRootComponent());C->SetStaticMesh(Native);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetLightingChannels(true,true,false);C->SetCullDistance(15000);C->RegisterComponent();}
 }
}
static bool Door(AEWardHospitalGameplay& S,bool Open,float Alpha);
static void FinalVisual(AEWardHospitalGameplay& S,Json Action,float T)
{
 const FString K=String(Action,TEXT("kind")),P=Plan(S);auto C=Current(S);auto& W=World(S);const int Cycle=Number(C,TEXT("cycle"));
 if(K==TEXT("final_operation"))
 {
  if(P==TEXT("CURE")&&Alive(S,TEXT("PERSON_3"))){SetGoal(S,TEXT("PERSON_3"),S.Props[TEXT("PERSON_3")]->GetActorLocation(),TEXT("lie"));Bed(S,TEXT("PERSON_3"));}
  if(P==TEXT("REPLACE")){S.Mode->Hero()->GetCharacterMovement()->StopMovementImmediately();S.Mode->Hero()->GetCharacterMovement()->DisableMovement();}
  if(W.Lamp.IsValid())W.Lamp->SetLightColor(P==TEXT("DESTROY")?FLinearColor(1,.025,.008):P==TEXT("CONTINUE")?FLinearColor(.3,.5,1):FLinearColor(.1,.8,.6));
  if(W.Lever.IsValid())W.Lever->SetRelativeRotation(FRotator(T*65,0,Cycle*15));
 }
 if(K==TEXT("final_check"))
 {
  if(P==TEXT("DESTROY")){if(Alive(S,TEXT("PERSON_3")))SetGoal(S,TEXT("PERSON_3"),S.Props[TEXT("PERSON_3")]->GetActorLocation(),TEXT("brace"));Door(S,false,T);}
  else if(Alive(S,TEXT("NEW_NPC_ORLOVA")))SetGoal(S,TEXT("NEW_NPC_ORLOVA"),S.Props[TEXT("NEW_NPC_ORLOVA")]->GetActorLocation(),TEXT("scan"));
 }
 if(K==TEXT("final_exit"))
 {
  if(P==TEXT("EVAC"))
  {
   Vehicle(S,true);const FVector DoorAt=Anchor(S)+FVector(330,130,0);
   auto Board=Child(C,TEXT("boarding"));
   if(!Boolean(C,TEXT("boarding_started"))){C->SetBoolField(TEXT("boarding_started"),true);for(auto V:Array(Scene(S),TEXT("cast"))){const FString Id=V->AsString();if(Id==TEXT("PERSON_3")||Id==TEXT("NEW_NPC_ORLOVA")||!Alive(S,Id))continue;SetGoal(S,Id,DoorAt,TEXT("walk"),Id==TEXT("PERSON_4")?60:120);Board->SetBoolField(Id,false);}}
   TArray<FString> BoardingIds;for(auto E:Board->Values)BoardingIds.Add(FString(E.Key));
   for(const FString& Id:BoardingIds)
   {
    if(Boolean(Board,Id))continue;
    if(!Alive(S,Id)){Board->SetBoolField(Id,true);continue;}
    if(FVector::Dist2D(S.Props[Id]->GetActorLocation(),DoorAt)>=105)continue;
    S.Props[Id]->SetActorHiddenInGame(true);S.Props[Id]->SetActorEnableCollision(false);
    Board->SetBoolField(Id,true);Child(Root(S),TEXT("evacuated"))->SetBoolField(Id,true);
   }
   bool All=true;for(auto E:Board->Values)All&=E.Value->AsBool();
   if(!All){C->SetNumberField(TEXT("elapsed"),0);C->SetStringField(TEXT("status"),TEXT("Посадка гражданских · Бека и Аружан остаются"));}
   else if(W.Vehicle.IsValid())W.Vehicle->SetActorLocation(Anchor(S)+FVector(380+T*T*2200,0,T*T*2700));
   const double Spin=S.GetWorld()->GetTimeSeconds()*1500;
   if(W.Rotor.IsValid())W.Rotor->SetRelativeRotation(FRotator(0,Spin,0));
   if(W.TailRotor.IsValid())
       W.TailRotor->SetRelativeRotation(FQuat(FVector::ForwardVector,PI*.5)*FQuat(FVector::UpVector,-Spin*2.7*PI/180));
   if(!W.EngineAudio.IsValid()||!W.EngineAudio->IsPlaying())W.EngineAudio=WardHomeAssets::PlaySpatialFile61(&S,FPaths::ProjectContentDir()/TEXT("CampaignData/Staging64/rotor.wav"),.2f*S.Mode->Setting(TEXT("effects"),.8),W.Vehicle->GetActorLocation());
   if(W.EngineAudio.IsValid()){W.EngineAudio->SetVolumeMultiplier(.2f*S.Mode->Setting(TEXT("effects"),.8));W.EngineAudio->SetWorldLocation(W.Vehicle->GetActorLocation());}
  }
  else if(P==TEXT("DESTROY")&&S.NarrativeFlag35(TEXT("HERO_STAYS"))){Door(S,false,T);}
  else if(P==TEXT("REPLACE")||P==TEXT("CONTINUE")){Panel(S);Door(S,false,T);}
  else {Door(S,true,T);if(S.NarrativeFlag35(TEXT("OUTBREAK"))&&W.Lamp.IsValid())W.Lamp->SetLightColor(FLinearColor(1,.01,.005));}
 }
 if(K==TEXT("final_arrival"))
 {
  if(P==TEXT("CURE")||(P==TEXT("DESTROY")&&!S.NarrativeFlag35(TEXT("HERO_STAYS")))||P==TEXT("OUTBREAK")){S.VehicleTarget31.Add(TEXT("DoorFR"),1);S.TickVehicle31(.016f);}
  else if(P==TEXT("EVAC")&&Alive(S,TEXT("PERSON_3"))){Bed(S,TEXT("PERSON_3"));SetGoal(S,TEXT("PERSON_3"),S.Props[TEXT("PERSON_3")]->GetActorLocation(),TEXT("lie"));}
  else {Panel(S);if(P==TEXT("DESTROY")&&S.NarrativeFlag35(TEXT("HERO_STAYS"))&&T>.65){S.Mode->Hero()->GetCharacterMovement()->DisableMovement();if(auto* PC=S.GetWorld()->GetFirstPlayerController())PC->PlayerCameraManager->StartCameraFade(0,1,2,FLinearColor::Black,false,true);}}
 }
}
static int NearestDoor(AEWardHospitalGameplay& S)
{
 if(!S.Mode->BookWorld)return -1;int Found=-1;double Best=FMath::Square(650.f);const FString Zone=String(Scene(S),TEXT("zone"));
 for(int I=0;I<S.Mode->BookWorld->Portals.Num();++I){auto& P=S.Mode->BookWorld->Portals[I];if(P.Role==TEXT("niche")||P.Locked||FMath::Abs(P.At.Z-Anchor(S).Z)>60)continue;const double D=FVector::DistSquared2D(P.At,Anchor(S));if(D<Best){Best=D;Found=I;}}return Found;
}
static bool Door(AEWardHospitalGameplay& S,bool Open,float Alpha)
{
 auto& W=World(S);if(W.Portal<0)W.Portal=NearestDoor(S);if(W.Portal<0)return true;auto& P=S.Mode->BookWorld->Portals[W.Portal];
 if(!Open)
 {
  const FVector H=S.Mode->Hero()->GetActorLocation();if(FVector::Dist2D(H,P.At)<100&&FMath::Abs(H.Z-P.At.Z)<190)return false;
  for(auto E:Child(Current(S),TEXT("goals"))->Values){const FString Id=FString(E.Key);if(auto A=S.Props.FindRef(Id))if(FVector::Dist2D(A->GetActorLocation(),P.At)<80&&FMath::Abs(A->GetActorLocation().Z-P.At.Z)<100)return false;}
 }
 const float T=Open?Alpha:1-Alpha;const bool NS=P.Side==TEXT("north")||P.Side==TEXT("south");const FVector Along=NS?FVector(1,0,0):FVector(0,1,0);const float Inward=(P.Side==TEXT("north")||P.Side==TEXT("east"))?P.OpenSign37:-P.OpenSign37;
 for(const auto& Part:P.VisibleParts){const bool Right=Part.Leaf==1;const FQuat R=FRotator(0,(NS?0:90)+(Right?180:0)+(P.Type==TEXT("shutter")?0:Inward*(Right?-90:90)*T),0).Quaternion();FVector Centre=P.At+Along*((Right?1:-1)*(P.Width*.5-6))+R.RotateVector(Part.Centre),Scale=Part.Size/100;if(P.Type==TEXT("shutter")){Centre.Z=FMath::Lerp(Centre.Z,double(P.At.Z+260+(Part.Centre.Z-4)*.07f),double(T));Scale.Z*=1-T*.93f;}S.Mode->BookWorld->DoorVisualGroups[Part.Material]->UpdateInstanceTransform(Part.Index,FTransform(R,Centre,Scale),false,false,true);}
 for(auto& Group:S.Mode->BookWorld->DoorVisualGroups)Group.Value->MarkRenderStateDirty();
 if(Alpha>=1){P.Open=Open;S.Mode->BookWorld->RefreshPortalVisual(P);}return true;
}
static bool Event(AEWardHospitalGameplay& S,const FString& Key){auto E=Child(Root(S),TEXT("events"));if(Boolean(E,Key))return false;E->SetBoolField(Key,true);return true;}
static void Visual(AEWardHospitalGameplay& S,Json A,float Progress)
{
 const FString K=String(A,TEXT("kind"));auto& W=World(S);
 if(K==TEXT("panel")||K==TEXT("power")||K==TEXT("alarm")||K==TEXT("screen")||K.StartsWith(TEXT("final_"))){Panel(S);if(W.Lever.IsValid())W.Lever->SetRelativeRotation(FRotator(Progress*65,0,0));if(W.Lamp.IsValid()){W.Lamp->SetLightColor(K==TEXT("alarm")?FLinearColor(1,.015,.008):Progress>=.95?FLinearColor(.1,.7,.8):FLinearColor(.7,.08,.025));W.Lamp->SetIntensity(25+20*FMath::Sin(Progress*PI));}}
 if(K==TEXT("door"))Door(S,Boolean(A,TEXT("open")),Progress);
 if(K==TEXT("lie")||K==TEXT("transport"))Bed(S,String(A,TEXT("actor")));
 if(K==TEXT("help")&&!String(A,TEXT("target")).IsEmpty()&&PoseName(S,String(A,TEXT("target")))==TEXT("lie"))Bed(S,String(A,TEXT("target")));
 if(K.StartsWith(TEXT("final_")))FinalVisual(S,A,Progress);
 if(K==TEXT("vehicle")) {Vehicle(S,false);if(W.Vehicle.IsValid())W.Vehicle->SetActorLocation(Anchor(S)+FVector(450+FMath::Lerp(-1700.f,1700.f,Progress),0,0));}
 if(K==TEXT("tap")){if(Event(S,S.ActiveScene35+TEXT("/tap/")+FString::FromInt(FMath::Min(2,FMath::FloorToInt(Progress*3)))))S.Mode->Cue(TEXT("metal_hit"),Anchor(S),.15);}
}
void Begin(AEWardHospitalGameplay& S,const FString& Id)
{
 if(!Active(S))return;auto C=Current(S);if(String(C,TEXT("id"))==Id)return;Reset(S);C=MakeShared<FJsonObject>();Root(S)->SetObjectField(TEXT("active"),C);C->SetStringField(TEXT("id"),Id);C->SetNumberField(TEXT("index"),0);C->SetNumberField(TEXT("elapsed"),0);C->SetNumberField(TEXT("cycle"),0);C->SetStringField(TEXT("status"),TEXT("Участники занимают места"));
 // The script opens this conversation after Aruzhan has secured the room.
 // Close only clear doorways, retaining normal manual opening afterwards.
 if(Id==TEXT("SCENE_03_02")&&S.Mode->BookWorld)
 {
  const FVector Player=S.Mode->Hero()->GetActorLocation();
  for(auto& Portal:S.Mode->BookWorld->Portals)
  {
   if(Portal.Zone!=TEXT("F211")||!Portal.Open||Portal.Role==TEXT("niche"))continue;
   if(FVector::DistSquared2D(Player,Portal.At)<FMath::Square(100.))continue;
   bool Occupied=false;
   for(const auto& PersonEntry:S.Props)
    if(Person(S,PersonEntry.Key)&&!PersonEntry.Value->IsHidden()&&
     FMath::Abs(PersonEntry.Value->GetActorLocation().Z-Portal.At.Z)<190&&
     FVector::DistSquared2D(PersonEntry.Value->GetActorLocation(),Portal.At)<FMath::Square(80.))
     Occupied=true;
   if(Occupied)continue;
   Portal.Open=false;
   S.Mode->BookWorld->RefreshPortalVisual(Portal);
  }
 }
 // In the playable arrival, the external officer speaks through the post intercom later.
 if (S.ActiveScene35 == TEXT("SCENE_01_01") && S.ArrivalStage >= 0) return;
 int Slot=0;for(auto V:Array(Scene(S),TEXT("cast"))){const FString Who=V->AsString();if(!Alive(S,Who))continue;if(Who==TEXT("PERSON_3")&&S.ActiveScene35>=TEXT("SCENE_19_03")&&Plan(S)==TEXT("DESTROY")&&!S.NarrativeFlag35(TEXT("HERO_STAYS")))continue;FVector At;if(Person(S,Who)->IsFallen58()){SetGoal(S,Who,S.Props[Who]->GetActorLocation(),TEXT("wounded"),0);continue;}if(Spot(S,Anchor(S),Slot++,At)){StageRemoteParticipant(S,Who,At,true);SetGoal(S,Who,At,TEXT("idle"));}else C->SetStringField(TEXT("blocked"),Who+TEXT(": нет свободного места"));}
}
static bool AtPlaces(AEWardHospitalGameplay& S)
{
 for(auto E:Child(Current(S),TEXT("goals"))->Values){if(!Alive(S,FString(E.Key)))continue;const FVector At=S.Props[FString(E.Key)]->GetActorLocation(),To=Vector(Array(E.Value->AsObject(),TEXT("at")));if(FVector::Dist2D(At,To)>32||FMath::Abs(At.Z-To.Z)>55)return false;}return true;
}
static bool RecoveryMark(AEWardHospitalGameplay& S,AActor& Actor,FVector Mark)
{
 const FVector Player=S.Mode->Hero()->GetActorLocation();
 if(FMath::Abs(Player.Z-Mark.Z)<190&&FVector::DistSquared2D(Player,Mark)<FMath::Square(90.))return false;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(WardStageRecovery73),false,&Actor);
 FHitResult Hit;
 if(!S.GetWorld()->LineTraceSingleByChannel(Hit,Mark+FVector(0,0,32),Mark-FVector(0,0,30),ECC_Visibility,Query)
  ||Hit.ImpactNormal.Z<.75||FMath::Abs(Mark.Z-Hit.ImpactPoint.Z)>10)return false;
 return !S.GetWorld()->OverlapBlockingTestByChannel(Mark+FVector(0,0,87),FQuat::Identity,ECC_Pawn,
  FCollisionShape::MakeCapsule(29,84),Query);
}
static void RecoverStalledApproaches(AEWardHospitalGameplay& S,float Delta)
{
 const Json Goals=Child(Current(S),TEXT("goals"));
 auto& Approaches=World(S).Approaches;
 bool Cut=false;
 for(const auto& Entry:Goals->Values)
 {
  const FString Id(Entry.Key);
  if(!Alive(S,Id))continue;
  auto* Model=Person(S,Id);
  auto* Actor=S.Props.FindRef(Id);
  const Json Goal=Entry.Value->AsObject();
  const FVector From=Actor->GetActorLocation();
  FVector Mark=Vector(Array(Goal,TEXT("at")));
  if(Number(Goal,TEXT("speed"))<=0||Model->IsFallen58()||Model->HumanState58()==TEXT("flee")||
   (FVector::DistSquared2D(From,Mark)<=FMath::Square(32.)&&FMath::Abs(From.Z-Mark.Z)<=55))
  {
   Approaches.Remove(Id);
   continue;
  }
  auto& Approach=Approaches.FindOrAdd(Id);
  if(!Approach.Position.Equals(From,14))
  {
   Approach.Position=From;
   Approach.Stalled=0;
   continue;
  }
  Approach.Stalled+=FMath::Min(Delta,.1f);
  if(Approach.Stalled<6)continue;
  Approach.Stalled=0;
  if(!RecoveryMark(S,*Actor,Mark)&&
   (!Spot(S,Mark,Approaches.Num(),Mark)||!RecoveryMark(S,*Actor,Mark)))continue;
  // Recover only within an active scene. Mask a visible reposition with
  // the same brief cut used when distant participants enter the scene.
  if(!Cut&&(InView(S,From,Actor)||InView(S,Mark,Actor)))
  {
   auto* PC=S.GetWorld()->GetFirstPlayerController();
   if(!PC||!PC->PlayerCameraManager)continue;
   PC->PlayerCameraManager->StartCameraFade(1.f,0.f,.3f,FLinearColor::Black,false);
   Cut=true;
  }
  if(!Actor->SetActorLocation(Mark,false,nullptr,ETeleportType::TeleportPhysics))continue;
  Goal->SetArrayField(TEXT("at"),{MakeShared<FJsonValueNumber>(Mark.X),
   MakeShared<FJsonValueNumber>(Mark.Y),MakeShared<FJsonValueNumber>(Mark.Z)});
  Model->SetTask58(TEXT("scene"),Mark);
  UE_LOG(LogTemp,Display,TEXT("STAGE73_RECOVER %s %s from %s to %s"),*S.ActiveScene35,*Id,*From.ToString(),*Mark.ToString());
 }
}
bool ChoiceReady(AEWardHospitalGameplay& S){return !Active(S)||int(Number(Current(S),TEXT("index")))>=Array(Scene(S),TEXT("lead")).Num();}
bool WaitingAction(AEWardHospitalGameplay& S){if(!Active(S))return false;const auto List=Actions(S);const int I=Number(Current(S),TEXT("index"));return List.IsValidIndex(I)&&String(List[I],TEXT("kind"))==TEXT("final_operation")&&Boolean(Current(S),TEXT("entered"))&&!Boolean(Current(S),TEXT("confirmed"));}
bool CanFinish(AEWardHospitalGameplay& S)
{
 if(!Active(S))return true;if(!Array(Scene(S),TEXT("choices")).IsEmpty()&&S.SelectedChoice35.IsEmpty())return false;
 return Number(Current(S),TEXT("index"))>=Actions(S).Num()&&AtPlaces(S)&&String(Current(S),TEXT("blocked")).IsEmpty();
}
FString Status(AEWardHospitalGameplay& S){return Active(S)?String(Current(S),TEXT("status")):FString();}
FVector Focus(AEWardHospitalGameplay& S,const FString& Id)
{
 auto List=Actions(S);const int I=Number(Current(S),TEXT("index"));if(List.IsValidIndex(I)&&String(List[I],TEXT("actor"))==Id){const FString To=String(List[I],TEXT("target"));if(Alive(S,To))return S.Props[To]->GetActorLocation()+FVector(0,0,105);if(String(List[I],TEXT("kind"))==TEXT("panel")||String(List[I],TEXT("kind"))==TEXT("brace"))return Anchor(S)+FVector(0,0,110);}
 return S.Mode->Hero()->GetActorLocation();
}
static void HandProp(AEWardHospitalGameplay& S,const FString& Id,bool Paper)
{
 if(!Alive(S,Id))return;auto* A=Prop(S,FVector::ZeroVector);auto* P=Person(S,Id);A->GetRootComponent()->AttachToComponent(P,FAttachmentTransformRules::KeepRelativeTransform);A->GetRootComponent()->ComponentTags.Add(TEXT("stage64_hand"));
 auto* M=Mesh(S,A,A->GetRootComponent(),Paper?TEXT("/Engine/BasicShapes/Cube.Cube"):TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector::ZeroVector,Paper?FVector(.15,.22,.002):FVector(.055,.055,.03),FLinearColor(.75,.73,.65));M->SetRelativeRotation(FRotator(Paper?65:90,0,0));
}
static void Enter(AEWardHospitalGameplay& S,Json A,const FString& Token)
{
 auto C=Current(S);const FString K=String(A,TEXT("kind")),Id=String(A,TEXT("actor")),Target=String(A,TEXT("target"));
 if(!Boolean(C,TEXT("entered")))
 {
  C->SetBoolField(TEXT("entered"),true);C->SetBoolField(TEXT("confirmed"),K!=TEXT("final_operation"));
  if(Alive(S,Id))
  {
   FVector At=S.Props[Id]->GetActorLocation();
   if(K==TEXT("walk")||K==TEXT("escort")||K==TEXT("transport")){FVector GoalAt;if(Spot(S,Anchor(S)+FVector(230,0,0),3,GoalAt))At=GoalAt;}
   if(!Target.IsEmpty()&&Alive(S,Target)){const FVector Other=S.Props[Target]->GetActorLocation();FVector GoalAt;if(Spot(S,Other,0,GoalAt))At=Other+(GoalAt-Other).GetSafeNormal2D()*68;}
   SetGoal(S,Id,At,K,K==TEXT("escort")?75:95);
  }
  if(K==TEXT("evacuate"))for(auto V:Array(Scene(S),TEXT("cast"))){const FString Who=V->AsString();if(!Alive(S,Who)||Who==TEXT("PERSON_3")||Who==Id)continue;FVector To;if(Spot(S,Anchor(S)+FVector(330,0,0),int(GetTypeHash(Who)%4),To))SetGoal(S,Who,To,TEXT("walk"),Who==TEXT("PERSON_4")?60:115);}
  if(K==TEXT("help")||K==TEXT("document")||K==TEXT("offer"))HandProp(S,Id,K==TEXT("document"));
  if(K==TEXT("fall"))if(auto P=Person(S,Id);P&&!P->IsDead58()&&!P->IsFallen58())if(Event(S,Token+TEXT("/fall")))P->Impact58(35,FVector(-70,0,0));
  if(Event(S,Token+TEXT("/sound")))S.Mode->Cue(K==TEXT("phone")?TEXT("INTRO/phone_handset"):K==TEXT("door")?TEXT("DOOR/steel"):K==TEXT("help")?TEXT("SCENE/rescue"):K==TEXT("alarm")?TEXT("SYSTEM/alarm"):K==TEXT("power")?TEXT("SYSTEM/breaker"):K==TEXT("panel")?TEXT("SYSTEM/terminal"):K==TEXT("screen")?TEXT("SYSTEM/cctv"):K.StartsWith(TEXT("final_"))?TEXT("SCENE/final_choices"):TEXT("cloth"),Anchor(S),.23f);
  if(S.Mode->Interface.IsValid())S.Mode->Interface->Refresh();
 }
}
static void CompleteBeat(AEWardHospitalGameplay& S,Json A,const FString& Token)
{
 const FString K=String(A,TEXT("kind")),Id=String(A,TEXT("actor")),Target=String(A,TEXT("target"));if(!Event(S,Token+TEXT("/effect")))return;
 if(K==TEXT("death")){if(auto P=Person(S,Id))P->Impact58(200,FVector(-65,0,10),true);}
 if(K==TEXT("help")&&!Target.IsEmpty()){if(auto P=Person(S,Target);P&&!P->IsDead58())P->GetUp58();if(Alive(S,Target))SetGoal(S,Target,S.Props[Target]->GetActorLocation(),TEXT("injured"));}
 if(K==TEXT("power")||K==TEXT("panel"))Child(Root(S),TEXT("mechanisms"))->SetStringField(S.ActiveScene35,String(A,TEXT("mode"),TEXT("operated")));
 if(K==TEXT("transport")&&World(S).Bed.IsValid())World(S).Bed->SetActorLocation(S.Props[Id]->GetActorLocation());
 if(K==TEXT("final_operation")){const int Cycle=Number(Current(S),TEXT("cycle"));Child(Root(S),TEXT("circuits"))->SetBoolField(Plan(S)+TEXT("/")+FString::FromInt(Cycle),true);if(Cycle==2){Root(S)->SetStringField(TEXT("operation_done"),Plan(S));if(Plan(S)==TEXT("DESTROY")){Root(S)->SetNumberField(TEXT("escape_seconds"),180);if(auto P=Person(S,TEXT("NEW_NPC_001")))P->Impact58(200,FVector(0,0,-20),true);}}}
 if(K==TEXT("final_exit")){Root(S)->SetBoolField(TEXT("exit_done"),true);Root(S)->SetNumberField(TEXT("escape_seconds"),-1);}
 if(K==TEXT("final_arrival")&&Plan(S)==TEXT("DESTROY")){if(auto P=Person(S,TEXT("PERSON_3")))P->Impact58(200,FVector(0,0,-20),true);S.SetNarrativeFlag35(TEXT("IRINA_ALIVE"),false);if(S.NarrativeFlag35(TEXT("HERO_STAYS")))Root(S)->SetBoolField(TEXT("hero_canonical_death"),true);}
}
static void PrepareNext(AEWardHospitalGameplay& S,float Delta)
{
 if(S.Chapter()>19||!S.Mode->Screen.IsEmpty()||!S.ActiveScene35.IsEmpty()||S.ArrivalStage>=0&&S.ArrivalStage<5||S.HomeStage52>=0)return;
 const auto& Chapters=Array(S.Data,TEXT("chapters"));if(!Chapters.IsValidIndex(S.Chapter()-1))return;const auto& Steps=Array(Chapters[S.Chapter()-1]->AsObject(),TEXT("required"));const int Step=S.Mode->Values.FindRef(TEXT("hospital_step"));if(!Steps.IsValidIndex(Step))return;
 const FString Id=Steps[Step]->AsString();Json Def=Object(Object(Data(),TEXT("scenes")),Id);if(!S.Props.Contains(Id)||!Def->HasField(TEXT("cast")))return;
 auto Prep=Child(Root(S),TEXT("preparation"));
 if(String(Prep,TEXT("id"))!=Id)
 {
  for(auto E:Child(Prep,TEXT("goals"))->Values)if(Alive(S,FString(E.Key)))Person(S,FString(E.Key))->SetTask58(TEXT("routine"));
  Prep=MakeShared<FJsonObject>();Root(S)->SetObjectField(TEXT("preparation"),Prep);Prep->SetStringField(TEXT("id"),Id);
  int Slot=0;for(auto V:Array(Def,TEXT("cast"))){const FString Who=V->AsString();if(!Alive(S,Who))continue;FVector At;if(Spot(S,S.Props[Id]->GetActorLocation(),Slot++,At,Child(Prep,TEXT("goals")))){StageRemoteParticipant(S,Who,At,false);Json Goal=MakeShared<FJsonObject>();Goal->SetArrayField(TEXT("at"),TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(At.X),MakeShared<FJsonValueNumber>(At.Y),MakeShared<FJsonValueNumber>(At.Z)});Child(Prep,TEXT("goals"))->SetObjectField(Who,Goal);Person(S,Who)->SetTask58(TEXT("scene"),At);}}
 }
 TArray<FString> Ids;for(auto E:Child(Prep,TEXT("goals"))->Values)Ids.Add(FString(E.Key));for(const FString& Who:Ids)if(Alive(S,Who)&&FVector::Dist2D(S.Props[Who]->GetActorLocation(),S.Mode->Hero()->GetActorLocation())>1500)Person(S,Who)->MoveTo58(Vector(Array(Object(Child(Prep,TEXT("goals")),Who),TEXT("at"))),FMath::Min(Delta,.1f),180);
}
void Tick(AEWardHospitalGameplay& S,float Delta)
{
 if(!Enabled(S))return;auto R=Root(S);const float Escape=Number(R,TEXT("escape_seconds"),-1);if(Escape>0&&S.Mode->Screen.IsEmpty()){R->SetNumberField(TEXT("escape_seconds"),FMath::Max(0.f,Escape-Delta));if(Escape<=Delta)S.Mode->DamageHero(1000,nullptr);}if(!Active(S)){PrepareNext(S,Delta);return;}Begin(S,S.ActiveScene35);auto C=Current(S);
 // Revalidate saved marks too: a resumed conversation must not send its
 // participants back into the same stationary contact hazard every time.
 const Json Goals=Child(C,TEXT("goals"));
 TArray<FString> Unsafe;
 for(const auto& E:Goals->Values)if(Alive(S,FString(E.Key))&&UnsafeStationaryMark(S,Vector(Array(E.Value->AsObject(),TEXT("at")))))Unsafe.Add(FString(E.Key));
 for(const FString& Id:Unsafe)
 {
  FVector Safe;
  if(!Spot(S,Anchor(S),Unsafe.IndexOfByKey(Id),Safe))continue;
  const Json Old=Object(Goals,Id);
  const FString Pose=String(Old,TEXT("pose"),TEXT("idle"));
  const float Speed=Number(Old,TEXT("speed"),95);
  SetGoal(S,Id,Safe,Pose,Speed);
  UE_LOG(LogTemp,Display,TEXT("STAGE73_SAFE_MARK %s %s at %s"),*S.ActiveScene35,*Id,*Safe.ToString());
 }
 // A fleeing participant may cross the presentation distance while the
 // conversation is suspended. Their fear and recovery still need to tick.
 for(const auto& Entry:Child(C,TEXT("goals"))->Values)
 {
  const FString Id(Entry.Key);
  if(!Alive(S,Id))continue;
  auto* Participant=Person(S,Id);
  if(Participant->HumanState58()==TEXT("flee")&&
   FVector::Dist2D(S.Props[Id]->GetActorLocation(),S.Mode->Hero()->GetActorLocation())>1500)
   Participant->TickHuman58(FMath::Min(Delta,.1f),false);
 }
 if(S.Mode->Screen!=TEXT("story35")||S.NarrativeFlag35(TEXT("DIALOGUE_SUSPENDED")))return;
 if(!String(C,TEXT("blocked")).IsEmpty()){C->SetStringField(TEXT("status"),TEXT("Не удалось поставить сцену: ")+String(C,TEXT("blocked")));return;}
 // Far actors travel through validated navigation without loading their heavy presentation.
 for(auto E:Child(C,TEXT("goals"))->Values)if(Alive(S,FString(E.Key))){auto* P=Person(S,FString(E.Key));if(P->HumanState58()==TEXT("flee"))
 {
  C->SetStringField(TEXT("status"),TEXT("Опасность прервала действие"));
  S.SetNarrativeFlag35(TEXT("DIALOGUE_SUSPENDED"),true);
  S.Mode->Show(TEXT(""));
  return;
 }if(FVector::Dist2D(S.Props[FString(E.Key)]->GetActorLocation(),S.Mode->Hero()->GetActorLocation())>1500)P->MoveTo58(Vector(Array(E.Value->AsObject(),TEXT("at"))),FMath::Min(Delta,.1f),155);}
 const auto Pending=Actions(S);const int PendingIndex=Number(C,TEXT("index"));if(Pending.IsValidIndex(PendingIndex)&&String(Pending[PendingIndex],TEXT("kind"))==TEXT("final_exit")&&Plan(S)==TEXT("EVAC")&&Boolean(C,TEXT("boarding_started")))FinalVisual(S,Pending[PendingIndex],float(Number(C,TEXT("elapsed")))/FMath::Max(1.f,float(Number(Pending[PendingIndex],TEXT("duration")))));
 if(!AtPlaces(S)){RecoverStalledApproaches(S,Delta);if(!AtPlaces(S)){C->SetStringField(TEXT("status"),TEXT("Участники подходят по коридору · можно продолжить осмотр"));return;}}
 auto List=Actions(S);const int I=Number(C,TEXT("index"));if(!List.IsValidIndex(I)){C->SetStringField(TEXT("status"),S.SelectedChoice35.IsEmpty()&&!Array(Scene(S),TEXT("choices")).IsEmpty()?TEXT("Выберите действие"):TEXT("Сцена завершена"));return;}
 Json A=List[I];const FString K=String(A,TEXT("kind"));const int Cycle=Number(C,TEXT("cycle"));const FString Token=S.ActiveScene35+TEXT("/")+FString::FromInt(I)+TEXT("/")+FString::FromInt(Cycle);auto& W=World(S);if(W.Token!=Token){W.Token=Token;W.Portal=-1;}
 Enter(S,A,Token);if(WaitingAction(S)){C->SetStringField(TEXT("status"),FString::Printf(TEXT("Подтвердите этап %d из 3 · %s"),Cycle+1,*Plan(S)));Visual(S,A,0);return;}
 const float Duration=FMath::Max(1.f,float(Number(A,TEXT("duration"),3)));const float Clock=Number(C,TEXT("elapsed"))+FMath::Clamp(Delta,0.f,.1f);const float Progress=FMath::Clamp(Clock/Duration,0.f,1.f);Visual(S,A,Progress);
 if(K==TEXT("door")&&!Door(S,Boolean(A,TEXT("open")),Progress)){C->SetStringField(TEXT("status"),TEXT("Отойдите из дверного проёма · створка ждёт"));return;}
 C->SetNumberField(TEXT("elapsed"),Clock);C->SetStringField(TEXT("status"),K==TEXT("help")?TEXT("Помощь раненому"):K==TEXT("escort")?TEXT("Персонаж проходит в безопасную зону"):K==TEXT("brace")?TEXT("Удержание механизма"):K==TEXT("final_operation")?FString::Printf(TEXT("Этап %d из 3"),Cycle+1):TEXT("Действие происходит в комнате"));
 if(K==TEXT("final_exit")&&Plan(S)==TEXT("EVAC")){for(auto E:Child(C,TEXT("boarding"))->Values)if(!E.Value->AsBool())return;}
 if(Progress<1)return;CompleteBeat(S,A,Token);
 if(K==TEXT("final_operation")&&Cycle<2){C->SetNumberField(TEXT("cycle"),Cycle+1);C->SetNumberField(TEXT("elapsed"),0);C->SetBoolField(TEXT("entered"),false);return;}
 C->SetNumberField(TEXT("index"),I+1);C->SetNumberField(TEXT("cycle"),0);C->SetNumberField(TEXT("elapsed"),0);C->SetBoolField(TEXT("entered"),false);if(S.Mode->Interface.IsValid())S.Mode->Interface->Refresh();
}
void Finish(AEWardHospitalGameplay& S)
{
 if(!Active(S))return;Child(Root(S),TEXT("history"))->SetBoolField(S.ActiveScene35,true);for(auto E:Child(Current(S),TEXT("goals"))->Values)if(Alive(S,FString(E.Key)))Person(S,FString(E.Key))->SetTask58(TEXT("routine"));Root(S)->SetObjectField(TEXT("active"),MakeShared<FJsonObject>());Reset(S);
}
bool Command(AEWardHospitalGameplay& S,const FString& Action)
{
 if(Action==TEXT("story35_action64")){if(WaitingAction(S))Current(S)->SetBoolField(TEXT("confirmed"),true);return true;}
 if(Action==TEXT("story35_continue64")){if(CanFinish(S))S.FinishNarrative35();else S.Mode->Notify(Status(S));return true;}
 if(Action==TEXT("story35_script64")){Root(S)->SetBoolField(TEXT("expanded"),!Boolean(Root(S),TEXT("expanded")));if(S.Mode->Interface.IsValid())S.Mode->Interface->Refresh();return true;}
 return false;
}
void RefreshAnchors(AEWardHospitalGameplay& S)
{
 if(!Enabled(S))return;const FString P=Plan(S);FVector At;bool Change=false;
 if(P==TEXT("REPLACE")||P==TEXT("CONTINUE")||(P==TEXT("DESTROY")&&S.NarrativeFlag35(TEXT("HERO_STAYS")))){if(auto A=S.Props.FindRef(TEXT("SCENE_19_01")))At=A->GetActorLocation();Change=true;}
 else if(P==TEXT("EVAC")){if(auto A=S.Props.FindRef(TEXT("SCENE_16_01")))At=A->GetActorLocation();for(const TCHAR* Id:{TEXT("SCENE_19_04"),TEXT("SCENE_19_05")})if(auto A=S.Props.FindRef(Id))A->SetActorLocation(At);}
 if(Change)for(const TCHAR* Id:{TEXT("SCENE_19_03"),TEXT("SCENE_19_04"),TEXT("SCENE_19_05")})if(auto A=S.Props.FindRef(Id))A->SetActorLocation(At);
 auto RestoreAnchor=[&S](const TCHAR* Id)
 {
  const Json Definition=S.Definitions.FindRef(Id);
  if(auto A=S.Props.FindRef(Id);A&&Array(Definition,TEXT("at")).Num()==3)
   A->SetActorLocation(Vector(Array(Definition,TEXT("at")))*100);
 };
 if(P==TEXT("REPLACE")){if(auto A=S.Props.FindRef(TEXT("SCENE_19_02")))if(auto B=S.Props.FindRef(TEXT("SCENE_19_01")))A->SetActorLocation(B->GetActorLocation());}
 else RestoreAnchor(TEXT("SCENE_19_02"));
 if(!Change&&P!=TEXT("EVAC")&&P!=TEXT("OUTBREAK"))RestoreAnchor(TEXT("SCENE_19_04"));
 if(!Change&&P!=TEXT("EVAC"))RestoreAnchor(TEXT("SCENE_19_05"));
 if(P==TEXT("OUTBREAK")){if(auto A=S.Props.FindRef(TEXT("SCENE_19_03")))A->SetActorLocation(FVector(5000,-4900,0));if(auto A=S.Props.FindRef(TEXT("SCENE_19_04")))A->SetActorLocation(FVector(5200,-6000,0));}
 for(auto E:Child(Root(S),TEXT("evacuated"))->Values)if(E.Value->AsBool())if(auto A=S.Props.FindRef(FString(E.Key))){A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);}
}
void Pose(AEWardHospitalGameplay& S,const FString& Id,TArray<FWardPersonPose54>& Skin)
{
 const FString K=PoseName(S,Id);if(K!=TEXT("lie")&&K!=TEXT("transport"))return;const FQuat R=FRotator(0,0,90).Quaternion();const FVector Shift(0,75,92);
 for(auto& J:Skin){const FVector X=R.RotateVector(FVector(J.M[0],J.M[4],J.M[8])),Y=R.RotateVector(FVector(J.M[1],J.M[5],J.M[9])),Z=R.RotateVector(FVector(J.M[2],J.M[6],J.M[10])),T=R.RotateVector(FVector(J.M[3],J.M[7],J.M[11]))+Shift;J.M[0]=X.X;J.M[4]=X.Y;J.M[8]=X.Z;J.M[1]=Y.X;J.M[5]=Y.Y;J.M[9]=Y.Z;J.M[2]=Z.X;J.M[6]=Z.Y;J.M[10]=Z.Z;J.M[3]=T.X;J.M[7]=T.Y;J.M[11]=T.Z;}
}
}
