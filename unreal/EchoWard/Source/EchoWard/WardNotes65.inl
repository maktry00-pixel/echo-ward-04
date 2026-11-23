namespace WardNotes65
{
Json Data(){static Json D=WardData::Read(FPaths::ProjectContentDir()/TEXT("CampaignData/Notes65/notes.json"));return D;}
static Json State(AEWardHospitalGameplay& S)
{
 if(!S.NarrativeState35->HasField(TEXT("notes65")))S.NarrativeState35->SetObjectField(TEXT("notes65"),MakeShared<FJsonObject>());
 return Object(S.NarrativeState35,TEXT("notes65"));
}
static Json Field(Json D,const FString& Key)
{
 if(!D->HasField(Key))D->SetObjectField(Key,MakeShared<FJsonObject>());return Object(D,Key);
}
void Apply(AEWardHospitalGameplay& S)
{
 if(Number(Data(),TEXT("version"))!=65)return;
 S.Narrative35->SetObjectField(TEXT("documents"),Clone35(Object(Data(),TEXT("documents"))));
 for(auto V:Array(Object(S.Narrative35,TEXT("gameplay")),TEXT("placements")))
 {
  auto P=V->AsObject();auto New=Object(Object(Data(),TEXT("placements")),String(P,TEXT("id")));
  for(auto E:New->Values)P->SetField(FString(E.Key),E.Value);
 }
 auto Scene=Object(Object(S.Narrative35,TEXT("scenes")),TEXT("SCENE_16_03"));
 for(auto V:Array(Scene,TEXT("choices")))if(String(V->AsObject(),TEXT("id"))==TEXT("SEND"))
 {
  TArray<TSharedPtr<FJsonValue>> R=Array(V->AsObject(),TEXT("requires"));R.Add(MakeShared<FJsonValueString>(TEXT("EVIDENCE_PACKET_READY65")));V->AsObject()->SetArrayField(TEXT("requires"),R);
 }
}
bool Allowed(const AEWardHospitalGameplay& S,const FString& Id)
{
 // Chapter rules here apply to revised pickups only. Historical NPC replay uses
 // its existing memory_active57 exception and must remain independently usable.
 if(!Object(Data(),TEXT("placements"))->HasField(Id))return true;
 Json P=S.Definitions.FindRef(Id);if(!P||S.Chapter()<Number(P,TEXT("chapter"),1))return false;
 Json R=Object(P,TEXT("rule65"));
 for(auto V:Array(R,TEXT("requires")))if(!S.Mode->Completed.Contains(V->AsString()))return false;
 for(auto V:Array(R,TEXT("flags")))if(!S.NarrativeFlag35(V->AsString()))return false;
 for(auto V:Array(R,TEXT("not_flags")))if(S.NarrativeFlag35(V->AsString()))return false;
 if(!Array(R,TEXT("any_flags")).IsEmpty()){bool Any=false;for(auto V:Array(R,TEXT("any_flags")))Any|=S.NarrativeFlag35(V->AsString());if(!Any)return false;}
 const FString Plan=String(Object(S.NarrativeState35,TEXT("choices")),TEXT("FINAL_PLAN"));
 if(!Array(R,TEXT("plans")).IsEmpty()){bool Any=false;for(auto V:Array(R,TEXT("plans")))Any|=V->AsString()==Plan;if(!Any)return false;}
 if(Boolean(R,TEXT("operation_done"))&&(Plan.IsEmpty()||String(Object(S.NarrativeState35,TEXT("staging64")),TEXT("operation_done"))!=Plan))return false;
 return true;
}
static bool Group(const AEWardHospitalGameplay& S,Json R)
{
 for(auto V:Array(R,TEXT("all")))if(!S.Mode->Completed.Contains(V->AsString()))return false;
 if(!Array(R,TEXT("any")).IsEmpty()){bool Any=false;for(auto V:Array(R,TEXT("any")))Any|=S.Mode->Completed.Contains(V->AsString());if(!Any)return false;}
 return true;
}
void Sync(AEWardHospitalGameplay& S)
{
 if(!S.NarrativeEnabled35||!S.Mode||Number(Data(),TEXT("version"))!=65)return;
 bool All=true;for(auto E:Object(Data(),TEXT("evidence_groups"))->Values){const bool Ready=Group(S,E.Value->AsObject());S.SetNarrativeFlag35(TEXT("EVIDENCE_")+FString(E.Key)+TEXT("65"),Ready);All&=Ready;}
 S.SetNarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65"),All);
 S.SetNarrativeFlag35(TEXT("TRUTH_READY"),All&&S.NarrativeFlag35(TEXT("TRUTH_SENT"))&&S.NarrativeFlag35(TEXT("SEMYON_HELPED"))&&S.NarrativeFlag35(TEXT("FULL_PROTOCOL_FRAGMENT")));
}
void Refresh(AEWardHospitalGameplay& S)
{
 Sync(S);
 // This side-story source was inside a class-C room with an inside-only secret
 // exit. Release its ordinary entrance at the authored chapter, not all doors.
 if(S.Chapter()>=11&&S.Mode->BookWorld&&!S.Mode->BookWorld->RefugeBolts.Contains(TEXT("PS11")))
  for(auto& P:S.Mode->BookWorld->Portals)if(P.Zone==TEXT("PS11")&&P.Type==TEXT("ordinary")&&!P.InsideOnly&&P.Locked){P.Locked=false;S.Mode->BookWorld->RefreshPortalVisual(P);}
 for(auto E:Object(Data(),TEXT("placements"))->Values)if(auto* A=S.Props.FindRef(FString(E.Key)))
 {
  bool Visible=Allowed(S,FString(E.Key))&&!S.Mode->Completed.Contains(FString(E.Key));
  A->SetActorHiddenInGame(!Visible);A->SetActorEnableCollision(Visible);
 }
}
void Collected(AEWardHospitalGameplay& S,const FString& Id)
{
 if(!Object(Data(),TEXT("documents"))->HasField(Id))return;
 Field(State(S),TEXT("collected"))->SetBoolField(Id,true);Sync(S);
}
void Read(AEWardHospitalGameplay& S,const FString& Entry)
{
 if(!Entry.StartsWith(TEXT("@doc:"))&&!Entry.StartsWith(TEXT("@blank:")))return;
 const FString Id=Entry.RightChop(Entry.StartsWith(TEXT("@doc:"))?5:7);
 // The administrative catalogue renders bodies without collecting or reading them.
 if(!S.Mode->Journal.Contains(Entry)||!S.Mode->Completed.Contains(Id)||!Object(Data(),TEXT("documents"))->HasField(Id))return;
 auto R=Field(State(S),TEXT("read"));if(Boolean(R,Id))return;R->SetBoolField(Id,true);S.Mode->AutoSave();
}
bool UseKey(AEWardHospitalGameplay& S,const FString& Id)
{
 if(!TArray<FString>{TEXT("visitor_pass"),TEXT("staff_pass"),TEXT("medical_clearance"),TEXT("quarantine_code"),TEXT("evidence_case")}.Contains(Id))return false;
 if(S.Mode->Completed.Contains(Id))return true;
 S.Mode->Completed.Add(Id);S.Mode->Flag(TEXT("hospital_key_")+Id,true);
 if(Id==TEXT("staff_pass"))S.Mode->Flag(TEXT("badge"),true);
 S.Mode->Journal.AddUnique(TEXT("Предмет: ")+String(S.Definitions.FindRef(Id),TEXT("label")));
 S.Mode->Notify(String(S.Definitions.FindRef(Id),TEXT("label")));S.Refresh();S.Mode->AutoSave();return true;
}
FString Summary(const AEWardHospitalGameplay& S)
{
 int Ready=0;for(auto E:Object(Data(),TEXT("evidence_groups"))->Values)Ready+=Group(S,E.Value->AsObject());
 return FString::Printf(TEXT("Досье: %d / 3 группы доказательств. "),Ready)+(S.NarrativeFlag35(TEXT("TRUTH_READY"))?TEXT("Полная передача подтверждена."):S.NarrativeFlag35(TEXT("TRUTH_SENT"))?TEXT("Передача записана; полнота досье требует проверки."):TEXT("Внешняя передача пока не подтверждена."));
}
FString Missing(const AEWardHospitalGameplay& S)
{
 FString Out;for(auto E:Object(Data(),TEXT("evidence_groups"))->Values)if(!Group(S,E.Value->AsObject()))
 {
  Json R=E.Value->AsObject();Out+=String(R,TEXT("title"))+TEXT(": ");TArray<FString> IDs;
  for(auto V:Array(R,TEXT("all")))if(!S.Mode->Completed.Contains(V->AsString()))IDs.Add(V->AsString());
  bool Any=Array(R,TEXT("any")).IsEmpty();for(auto V:Array(R,TEXT("any")))Any|=S.Mode->Completed.Contains(V->AsString());if(!Any)IDs.Add(Array(R,TEXT("any"))[0]->AsString());
  for(const auto& Id:IDs){Json D=Object(Object(Data(),TEXT("documents")),Id);Out+=String(D,TEXT("title"))+TEXT(" [")+String(D,TEXT("zone"))+TEXT("]; ");}
 }
 return Out;
}
void Fit(AEWardHospitalGameplay& S)
{
 // A deterministic local fit. It does not assert a complete chapter route.
 auto Ordered=Object(Data(),TEXT("placements"));TArray<FString> IDs;for(auto E:Ordered->Values)IDs.Add(FString(E.Key));IDs.Sort();TArray<FVector> Used;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(NotesSupport65),false,S.Mode->Hero());for(auto E:S.Props)Q.AddIgnoredActor(E.Value);
 for(const auto& Id:IDs)
 {
  auto* A=S.Props.FindRef(Id);Json P=S.Definitions.FindRef(Id);if(!A||!P)continue;
  const FVector Original=A->GetActorLocation();bool Chosen=false;
  for(auto V:Array(P,TEXT("candidates65")))
  {
   Json C=V->AsObject();FVector At=Vector(Array(C,TEXT("at")))*100;const bool Table=String(C,TEXT("support")).StartsWith(TEXT("PROP6_"));
   // Fixed mechanism supports (camera/lever) must keep their authored mounting.
   const bool Paper=String(P,TEXT("kind"))==TEXT("note")||String(P,TEXT("kind"))==TEXT("key")||String(P,TEXT("kind"))==TEXT("map");
   if(Table||Paper)
   {
    FHitResult Hit;const FVector High=At+FVector(0,0,Table?28:18),Low=At-FVector(0,0,Table?35:25);
    if(!S.GetWorld()->LineTraceSingleByChannel(Hit,High,Low,ECC_Visibility,Q)||Hit.ImpactNormal.Z<.8)continue;
    At.Z=Hit.ImpactPoint.Z+.3-2.;
   }
   bool Overlap=false;for(auto U:Used)Overlap|=FVector::Dist2D(U,At)<31&&FMath::Abs(U.Z-At.Z)<35;if(Overlap)continue;
   A->SetActorLocation(At);FVector Standing;if(!S.FindNarrativeStanding35(Id,Standing))continue;
   A->SetActorLocation(At);P->SetArrayField(TEXT("at"),{MakeShared<FJsonValueNumber>(At.X/100),MakeShared<FJsonValueNumber>(At.Y/100),MakeShared<FJsonValueNumber>(At.Z/100)});P->SetStringField(TEXT("support65"),String(C,TEXT("support")));Used.Add(At);Chosen=true;break;
  }
  if(!Chosen){A->SetActorLocation(Original);P->SetStringField(TEXT("support65"),TEXT("AUTHORED_FALLBACK_REQUIRES_AUDIT"));}
 }
}
}
