namespace WardInterface66
{
bool IsField(const FString& Page){return TArray<FString>{TEXT("inventory"),TEXT("journal"),TEXT("map"),TEXT("quests"),TEXT("people56"),TEXT("controls66")}.Contains(Page);}
bool Hotkey(AEWardCampaignMode& G,const FString& Page)
{
 if(!G.CampaignStarted||G.TitleView||!G.Hero()||G.Hero()->Dead||(G.HospitalStory&&(G.HospitalStory->HomeStage52>=0||G.HospitalStory->ArrivalCinematic())))return false;
 if(!G.Screen.IsEmpty()&&!IsField(G.Screen))return false;
 if(G.Screen==Page){G.Command(TEXT("field_close"));return true;}
 if(Page==TEXT("controls66"))G.Interface->HelpReturn66=G.Screen;
 G.Command(Page==TEXT("map")?TEXT("page_map"):TEXT("page_")+Page);return true;
}
bool Back(AEWardCampaignMode& G)
{
 if(G.Screen==TEXT("inspect")){G.Show(TEXT("inventory"));return true;}
 if(G.Screen==TEXT("people56")){G.Show(TEXT("journal"));return true;}
 if(G.Screen==TEXT("controls66")){G.Show(G.Interface->HelpReturn66);return true;}
 if(IsField(G.Screen)){G.Command(TEXT("field_close"));return true;}
 if(G.Screen==TEXT("settings")&&!G.Interface->SettingsReturn66.IsEmpty()){G.Show(G.Interface->SettingsReturn66);return true;}
 return false;
}
TArray<int32> Documents(const AEWardCampaignMode& G,const FString& Query,bool Unread)
{
 TArray<int32> Out;auto* S=G.HospitalStory.Get();
 for(int I=0;I<G.Journal.Num();++I)
 {
  const FString J=G.Journal[I];const bool Ref=J.StartsWith(TEXT("@doc:"))||J.StartsWith(TEXT("@blank:"));
  if(S&&S->NarrativeEnabled35&&!Ref)continue;
  const FString Id=Ref?J.RightChop(J.StartsWith(TEXT("@doc:"))?5:7):J;
  if(Unread&&S&&Boolean(Object(Object(S->NarrativeState35,TEXT("notes65")),TEXT("read")),Id))continue;
  const FString Name=S?S->DocumentTitle(J):J;
  if(!Query.IsEmpty()&&!Name.Contains(Query,ESearchCase::IgnoreCase)&&!Id.Contains(Query,ESearchCase::IgnoreCase))continue;
  Out.Add(I);
 }
 return Out;
}
FString ObjectiveId(const AEWardCampaignMode& G)
{
 auto* S=G.HospitalStory.Get();if(!S||S->Chapter()>19)return FString();
 const auto& C=Array(S->Data,TEXT("chapters"));if(!C.IsValidIndex(S->Chapter()-1))return FString();
 const auto& Steps=Array(C[S->Chapter()-1]->AsObject(),TEXT("required"));const int I=G.Values.FindRef(TEXT("hospital_step"));return Steps.IsValidIndex(I)?Steps[I]->AsString():FString();
}
FString Site(const AEWardCampaignMode& G){return G.BookWorld&&G.Hero()?G.BookWorld->LocationAt(G.Hero()->GetActorLocation()):FString();}
FString MapSummary(const AEWardCampaignMode& G)
{
 int Rooms=0,Locations=0;for(auto V:Array(WardBookMapData(),TEXT("locations"))){++Locations;for(auto R:Array(V->AsObject(),TEXT("rooms")))if(!Boolean(R->AsObject(),TEXT("hidden"))||G.Flag(TEXT("discovered_")+String(R->AsObject(),TEXT("id"))))++Rooms;}
 return FString::Printf(TEXT("Больница №333 · помещений на плане: %d · планов: %d"),Rooms,Locations);
}
FString Controls(bool Admin)
{
 FString T=TEXT("ПЕРЕМЕЩЕНИЕ\nW A S D — движение · мышь — осмотр\nShift — бег · Ctrl — присесть · Пробел — прыжок\n\nВЗАИМОДЕЙСТВИЕ\nF — подобрать, поговорить, открыть дверь; удерживать F — обыск\nN — фонарик · F6 — вспышка · Y — детали автомобиля\nPage Up / Page Down — выбрать остановку лифта · F — вызвать / ехать\nK — засов убежища / укрепление двери\n\nБОЙ И ПРИПАСЫ\nПКМ — прицелиться · ЛКМ — выстрел · R — перезарядка\nZ — сменить оружие · V — нож · G — бросить бутылку\nH — перевязка · T — заряд ослепителя · Q — рывок\n\nЭКРАНЫ\nI — инвентарь · J — документы · M или Tab — карта\nO — задания · F1 — управление · Esc — назад / пауза\nПовторная клавиша открытого экрана закрывает его.\nF5 — быстрое сохранение · B — яркий осмотр (только администратор)\n\nВСТУПЛЕНИЕ\nДома: F — ответить на звонок у окна; E / F — выйти после разговора.\n\nБЫСТРЫЙ ДОСТУП\n1–5 — первые пять слотов · 0 — пустые руки\nP — телефон · F — отправить ответ в переписке\nSpace — пропустить реплику · Backspace — пропустить речь сцены\nF2 — полное обучение.");
 if(Admin)T+=TEXT("\n\nАДМИНИСТРАТОР\nF8 — каталог предметов · F9 — агент · F10 — персонаж\nF3 — счётчик кадров · F4 — профиль графики");return T;
}
FString Description(const FString& Id)
{
 const TMap<FString,FString> D={{TEXT("weapon"),TEXT("Служебный пистолет. ПКМ — прицелиться, ЛКМ — выстрел, R — перезарядка.")},{TEXT("shotgun"),TEXT("Помповый дробовик, 5 патронов. Z — сменить оружие, R — поочерёдная перезарядка.")},{TEXT("ammo"),TEXT("Патроны пистолета. Используй или объедини с пистолетом для зарядки магазина.")},{TEXT("shells"),TEXT("Патроны дробовика. Используй или объедини с дробовиком для зарядки.")},{TEXT("medkit"),TEXT("Перевязка занимает 2,4 секунды. Удар прерывает лечение. Использование закроет инвентарь.")},{TEXT("battery"),TEXT("Запасной заряд ремонтного ослепителя. T — зарядить, F6 — вспышка.")},{TEXT("badge"),TEXT("Служебный пропуск. Осмотри оборот. Сюжетные допуски не расходуются.")},{TEXT("bottle"),TEXT("G — бросить бутылку и отвлечь противника шумом.")},{TEXT("boards"),TEXT("Материал для укрепления двери. K — засов убежища или укрепление подходящей двери.")},{TEXT("fuse"),TEXT("Запасной предохранитель. Нужен для местного питания; не выбрасывается.")},{TEXT("residence_prybar"),TEXT("Монтажный инструмент для технических механизмов.")},{TEXT("isolation_tag"),TEXT("Браслет медицинского допуска. Не заменяет служебный пропуск.")}};
 return D.Contains(Id)?D[Id]:TEXT("Сюжетный предмет. Используется у связанного механизма; выбросить его нельзя.");
}
FString PickupName(AEWardCampaignMode& G,const FString& Id)
{
 if(!G.HospitalStory)return Id;Json P=G.HospitalStory->Definitions.FindRef(Id);return P?String(P,TEXT("label"))+TEXT(" [")+String(P,TEXT("zone"))+TEXT("]"):Id;
}
bool CanUse(AEWardCampaignMode& G,const FString& Id)
{
 if(!G.Hero()||!HasItem(&G,Id))return false;
 if(Id==TEXT("weapon")||Id==TEXT("shotgun")||Id==TEXT("badge"))return true;
 if(Id==TEXT("medkit"))return G.Hero()->Health<100&&!G.Hero()->Dead&&G.NumberFlag(TEXT("combat63_heal"))<=0&&G.ReloadTime<=0;
 if(Id==TEXT("battery"))return G.FlashCharges<3;
 if(Id==TEXT("ammo"))return G.Flag(TEXT("weapon"))&&G.Hero()->Rounds<9&&G.Hero()->Reserve>0;
 if(Id==TEXT("shells"))return G.Flag(TEXT("shotgun"))&&G.LoadedShells<5&&G.Items.FindRef(TEXT("shells"))>0;
 return false;
}
bool ShowNotice(const AEWardCampaignMode& G)
{
 if(!G.Hero()||G.Hero()->NoticeTime<=0)return false;
 auto* S=G.HospitalStory.Get();const bool Speech=S&&S->DialogueLeft62>0&&!S->DialogueKey62.IsEmpty()&&G.Hero()->Notice==G.Tr(S->DialogueKey62);
 return !Speech||G.SettingBool(TEXT("subtitles"));
}
bool Command(AEWardCampaignMode& G,const FString& A)
{
 if(A==TEXT("ui66_back")){if(!Back(G))G.Hero()->Pause();return true;}
 if(A==TEXT("ui66_help")){if(G.Screen!=TEXT("controls66")){G.Interface->HelpReturn66=G.Screen;G.Show(TEXT("controls66"));}return true;}
 if(A==TEXT("ui66_unread")){G.Interface->UnreadOnly66=!G.Interface->UnreadOnly66;G.Interface->Refresh();return true;}
 if(A==TEXT("ui66_search_clear")){G.Interface->DocumentQuery66.Empty();G.Interface->Refresh();return true;}
 if(A==TEXT("ui66_map_here")||A==TEXT("ui66_map_goal"))
 {
  FString Where=Site(G);if(A==TEXT("ui66_map_goal")){const FString Id=ObjectiveId(G);if(G.HospitalStory&&G.HospitalStory->Definitions.Contains(Id)){Where=String(G.HospitalStory->Definitions[Id],TEXT("site"));if(G.BookWorld)if(auto* P=G.HospitalStory->Props.FindRef(Id)){const FString At=G.BookWorld->LocationAt(P->GetActorLocation()+FVector(0,0,96));if(At!=TEXT("gallery"))Where=At;}}}
  bool Valid=false;for(auto L:Array(WardBookMapData(),TEXT("locations")))Valid|=String(L->AsObject(),TEXT("id"))==Where;
  if(Valid)G.Command(TEXT("field_atlas_")+Where);else G.Notify(TEXT("Для этой точки отдельный план недоступен."));return true;
 }
 return false;
}
}
