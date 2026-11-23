TSharedRef<SWidget> FWardCampaignUI::QuestMenu66()
{
 auto* G=Mode.Get();auto* S=G->HospitalStory.Get();auto Body=SNew(SVerticalBox);const FString Font=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf");
 auto Label=[&](const FString& V,int Size,FLinearColor C=Ink){return SNew(STextBlock).Text(FText::FromString(G->Tr(V))).Font(FSlateFontInfo(Font,Size)).ColorAndOpacity(C).AutoWrapText(true);};
 auto Add=[&](const FString& Title,const FString& Copy){Body->AddSlot().AutoHeight().Padding(0,0,0,7)[Label(Title,25)];Body->AddSlot().AutoHeight().Padding(0,0,0,24)[Label(Copy,20,Muted)];};
 if(!S){Add(TEXT("Текущая задача"),G->Objective());return Body;}
 const FString Id=WardInterface66::ObjectiveId(*G);Json Scene=S->NarrativeScenes35.FindRef(Id);
 Add(TEXT("Текущая задача"),G->Objective());
 if(Scene)
 {
  FString Required;for(auto V:Array(Scene,TEXT("requires")))if(!G->Completed.Contains(V->AsString())&&!S->NarrativeFlag35(V->AsString()))Required+=TEXT("• ")+WardInterface66::PickupName(*G,V->AsString())+TEXT("\n");
  if(!Required.IsEmpty())Add(TEXT("Нужно найти"),Required);
  else Add(TEXT("Следующее действие"),TEXT("Следуй к месту текущей задачи. F — взаимодействие. Обязательные предметы этого этапа найдены."));
  Json P=S->Definitions.FindRef(Id);if(P){Body->AddSlot().AutoHeight().Padding(0,0,0,20)[Button(TEXT("Показать место задачи на карте"),TEXT("ui66_quest_map"))];}
 }
 else Add(TEXT("Посещение завершено"),TEXT("Найденные документы и принятые решения сохранены в журнале."));
 Add(TEXT("Расследование"),WardNotes65::Summary(*S));
 if(S->Chapter()>=16&&!S->NarrativeFlag35(TEXT("EVIDENCE_PACKET_READY65")))Add(TEXT("Источники для передачи"),WardNotes65::Missing(*S));
 Add(TEXT("Принятые решения"),TEXT("Ниже показаны только события, уже записанные в журнале."));
 int Lines=0;for(int I=G->Journal.Num()-1;I>=0&&Lines<10;--I)
 {
  const FString J=G->Journal[I];if(J.StartsWith(TEXT("Решение:"))||J.StartsWith(TEXT("Разговор:"))){Body->AddSlot().AutoHeight().Padding(0,0,0,12)[Label(J,18)];++Lines;}
 }
 if(!Lines)Body->AddSlot().AutoHeight().Padding(0,0,0,18)[Label(TEXT("Решения появятся после первых разговоров."),18,Muted)];
 Body->AddSlot().AutoHeight().Padding(0,20,0,12)[Label(TEXT("Дополнительные истории"),25)];
 const auto& Sides=Array(S->Narrative35,TEXT("side_stories"));
 for(int N=0;N<Sides.Num();++N)
 {
  int Found=0;for(int I=1;I<=3;++I)Found+=G->Completed.Contains(FString::Printf(TEXT("SIDE_%d_%d"),N+1,I));if(!Found)continue;
  const bool Done=S->NarrativeFlag35(TEXT("SIDE_DONE_")+FString::FromInt(N+1));Body->AddSlot().AutoHeight().Padding(0,0,0,10)[Label(String(Sides[N]->AsObject(),TEXT("title"))+FString::Printf(TEXT(" · %d / 3 · "),Found)+(Done?TEXT("завершена"):TEXT("расследуется")),18,Done?Muted:Ink)];
 }
 return SNew(SScrollBox)+SScrollBox::Slot()[Body];
}
