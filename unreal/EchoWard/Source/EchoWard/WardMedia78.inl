// Native gallery and streamed cinematic player; no external browser or codec process.
#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Brushes/SlateColorBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Engine/Texture2D.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/Compression.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
using WardData::Json;
namespace WardMedia78
{
 const FString Base(){return FPaths::ProjectContentDir()/TEXT("CampaignData/Media78");}
 const FString StudioBase(){return FPaths::ProjectDir()/TEXT("Qaisar114");}
 const FString MovieBase()
 {
     const FString Folder = StudioBase();
     return IFileManager::Get().FileExists(*(Folder/TEXT("trailer.echo"))) &&
         IFileManager::Get().FileExists(*(Folder/TEXT("trailer.wav"))) ? Folder : Base();
 }
 const FLinearColor Ink(.006,.003,.005),Cream(.84,.79,.71),Red(.72,.025,.035),Dim(.38,.32,.31);
 FSlateColorBrush White78(FLinearColor::White);
 FButtonStyle Button78=[](){FButtonStyle S;S.SetNormal(FSlateColorBrush(FLinearColor(.025,.008,.012)));S.SetHovered(FSlateColorBrush(FLinearColor(.19,.018,.025)));S.SetPressed(FSlateColorBrush(FLinearColor(.33,.018,.026)));return S;}();
 struct FImage78 {TStrongObjectPtr<UTexture2D> Texture;FSlateBrush Brush;explicit FImage78(UTexture2D* T):Texture(T){Brush.SetResourceObject(T);Brush.ImageSize=FVector2D(T->GetSizeX(),T->GetSizeY());}};
 struct FEntry78 {FString Id,Title;bool Art=false;};
 const FSlateBrush* Cover78(bool Trailer){static TUniquePtr<FImage78> Covers[2];const int I=Trailer?0:1;if(!Covers[I])if(auto* T=FImageUtils::ImportFileAsTexture2D(Base()/TEXT("gallery")/(Trailer?TEXT("04_aruzhan.jpg"):TEXT("08_search.jpg"))))Covers[I]=MakeUnique<FImage78>(T);return Covers[I]?&Covers[I]->Brush:nullptr;}
 class SMedia78;
 TWeakPtr<SMedia78> Active78;
 class SMedia78:public SCompoundWidget
 {
 public:
 SLATE_BEGIN_ARGS(SMedia78){}SLATE_ARGUMENT(AEWardCampaignMode*,Mode)SLATE_ARGUMENT(bool,Trailer)SLATE_END_ARGS()
 TWeakObjectPtr<AEWardCampaignMode> Mode;bool Trailer=false,Playing=false,Muted=false,Focused=false;int Selected=0,Filter=0,Frame=-1;double Time=0;int W=0,H=0,FPS=24,Count=0;FSlateBrush VideoBrush;TStrongObjectPtr<UTexture2D> VideoTexture;TStrongObjectPtr<USoundWaveProcedural> Wave;TStrongObjectPtr<UAudioComponent> Audio;TUniquePtr<FArchive> Movie;TArray<uint64> Offsets;TArray<uint32> Sizes;TArray<uint8> PCM;TArray<FEntry78> Entries;TArray<int> Visible;TMap<FString,TUniquePtr<FImage78>> Images;FString Error;
 virtual ~SMedia78(){if(Audio.IsValid()){Audio->Stop();Audio->DestroyComponent();}}
 TSharedRef<SWidget> Label(FString T,int Size=18,FLinearColor C=Cream,bool Serif=false){return SNew(STextBlock).Text(FText::FromString(T)).Font(FSlateFontInfo(FPaths::ProjectContentDir()/(Serif?TEXT("CampaignData/Fonts/Literata.ttf"):TEXT("CampaignData/Fonts/NotoSans.ttf")),Size)).ColorAndOpacity(C).AutoWrapText(true);}
 TSharedRef<SWidget> Button(FString Text,TFunction<void()> Fn,bool Chosen=false){return SNew(SButton).ButtonStyle(&Button78).ContentPadding(FMargin(18,10)).OnClicked_Lambda([Fn](){Fn();return FReply::Handled();})[Label(Text,16,Chosen?Red:Cream)];}
 const FSlateBrush* Picture(const FString& Id,bool Thumb=false){const FString Key=Id+(Thumb?TEXT("_thumb"):TEXT(""));if(auto* V=Images.Find(Key))return &(*V)->Brush;const FString Path=Base()/(Thumb?TEXT("thumbs"):TEXT("gallery"))/(Id+TEXT(".jpg"));auto* T=FImageUtils::ImportFileAsTexture2D(Path);if(!T)return nullptr;auto P=MakeUnique<FImage78>(T);const auto* B=&P->Brush;Images.Add(Key,MoveTemp(P));return B;}
 void Construct(const FArguments& A)
 {
 Mode=A._Mode;Trailer=A._Trailer;
 const TArray<FString> GameTitles={TEXT("КПП · у последней границы"),TEXT("Двор отделения"),TEXT("Восточный корпус"),TEXT("Тихий коридор"),TEXT("Лаборатория"),TEXT("Пост дежурной"),TEXT("Старое оборудование"),TEXT("Комната наблюдения"),TEXT("Следы в тишине"),TEXT("Территория больницы"),TEXT("Пустой холл"),TEXT("Западное крыло")};
 const TArray<FString> GameIds={TEXT("01_gate"),TEXT("02_courtyard"),TEXT("03_east"),TEXT("04_corridor"),TEXT("05_laboratory"),TEXT("06_reception"),TEXT("07_equipment"),TEXT("08_room"),TEXT("09_trace"),TEXT("10_entrance"),TEXT("11_hall"),TEXT("12_west")};
 for(int I=0;I<GameIds.Num();++I)Entries.Add({GameIds[I],GameTitles[I],false});
 const TArray<FString> ArtIds={TEXT("01_together"),TEXT("02_missing"),TEXT("03_hospital"),TEXT("04_aruzhan"),TEXT("05_phone"),TEXT("06_glass"),TEXT("07_theatre"),TEXT("08_search")};
 const TArray<FString> ArtTitles={TEXT("Когда она была рядом"),TEXT("Один пропущенный звонок"),TEXT("Отделение 04"),TEXT("Аружан · до исчезновения"),TEXT("Не иди на мой голос"),TEXT("Кто за стеклом?"),TEXT("Операционная"),TEXT("Найти её до рассвета")};
 for(int I=0;I<ArtIds.Num();++I)Entries.Add({ArtIds[I],ArtTitles[I],true});
 if(Trailer)OpenMovie();ApplyFilter(0);Rebuild();
 }
 void ApplyFilter(int F){Filter=F;Visible.Reset();for(int I=0;I<Entries.Num();++I)if(F==0||(F==1&&!Entries[I].Art)||(F==2&&Entries[I].Art))Visible.Add(I);Selected=0;}
 void Navigate(int D){if(Visible.Num()){Selected=(Selected+D+Visible.Num())%Visible.Num();Rebuild();}}
 void OpenMovie()
 {
 const FString MovieFolder=MovieBase();
 Movie.Reset(IFileManager::Get().CreateFileReader(*(MovieFolder/TEXT("trailer.echo"))));if(!Movie){Error=TEXT("Не удалось открыть видеозапись.");return;}uint8 Magic[8];Movie->Serialize(Magic,8);uint32 Header[4];Movie->Serialize(Header,16);W=Header[0];H=Header[1];FPS=Header[2];Count=Header[3];
 if(FMemory::Memcmp(Magic,"ECHOJ078",8)||W!=1280||H!=720||FPS!=24||Count<24||Count>3600){Error=TEXT("Видеозапись повреждена.");Movie.Reset();return;}
 Offsets.SetNum(Count);Sizes.SetNum(Count);for(int I=0;I<Count;++I){*Movie<<Offsets[I];*Movie<<Sizes[I];if(Offsets[I]+Sizes[I]>uint64(Movie->TotalSize())){Movie.Reset();Error=TEXT("Видеозапись неполная.");return;}}
 VideoTexture.Reset(UTexture2D::CreateTransient(W,H,PF_B8G8R8A8));VideoTexture->SRGB=true;VideoTexture->Filter=TF_Bilinear;VideoTexture->UpdateResource();VideoBrush.SetResourceObject(VideoTexture.Get());VideoBrush.ImageSize=FVector2D(W,H);Upload(0);
 TArray<uint8> Bytes;if(FFileHelper::LoadFileToArray(Bytes,*(MovieFolder/TEXT("trailer.wav")))&&Bytes.Num()>44){int Pos=12;while(Pos+8<=Bytes.Num()){uint32 Size=0;FMemory::Memcpy(&Size,Bytes.GetData()+Pos+4,4);if(!FMemory::Memcmp(Bytes.GetData()+Pos,"data",4)&&Size<=uint32(Bytes.Num()-Pos-8)){PCM.Append(Bytes.GetData()+Pos+8,Size);break;}Pos+=8+Size+(Size&1);}}
 if(PCM.Num()&&Mode.IsValid()){Wave.Reset(NewObject<USoundWaveProcedural>());Wave->SetSampleRate(48000);Wave->NumChannels=1;Wave->Duration=INDEFINITELY_LOOPING_DURATION;Wave->bLooping=false;Audio.Reset(NewObject<UAudioComponent>(Mode.Get()));Audio->bIsUISound=true;Audio->bAllowSpatialization=false;Audio->bAutoDestroy=false;Audio->SetSound(Wave.Get());Audio->RegisterComponent();}
 }
 void Upload(int I)
 {
 if(!Movie||!VideoTexture.IsValid()||I==Frame||!Offsets.IsValidIndex(I))return;TArray<uint8> Packed;Packed.SetNumUninitialized(Sizes[I]);Movie->Seek(Offsets[I]);Movie->Serialize(Packed.GetData(),Packed.Num());FImage Decoded;if(!FImageUtils::DecompressImage(Packed.GetData(),Packed.Num(),Decoded)||Decoded.SizeX!=W||Decoded.SizeY!=H||Decoded.Format!=ERawImageFormat::BGRA8){Playing=false;Error=TEXT("Ошибка чтения кадра.");return;}const int N=W*H*4;auto* Raw=new uint8[N];FMemory::Memcpy(Raw,Decoded.RawData.GetData(),N);auto* Region=new FUpdateTextureRegion2D(0,0,0,0,W,H);VideoTexture->UpdateTextureRegions(0,1,Region,W*4,4,Raw,[](uint8* Data,const FUpdateTextureRegion2D* R){delete[] Data;delete R;});Frame=I;
 }
 void StartAudio(){if(!Audio.IsValid())return;Audio->Stop();Wave->ResetAudio();const int Byte=FMath::Clamp(int(Time*48000)*2,0,PCM.Num());if(Byte<PCM.Num())Wave->QueueAudio(PCM.GetData()+Byte,PCM.Num()-Byte);Audio->SetVolumeMultiplier(Muted?0:1);Audio->Play();}
 void Toggle(){if(!Movie)return;Playing=!Playing;if(Playing){if(Time>=Duration()-.05)Time=0;StartAudio();}else if(Audio.IsValid())Audio->Stop();}
 double Duration()const{return Count/double(FPS);}
 void Seek(double T){Time=FMath::Clamp(T,0.,FMath::Max(0.,Duration()-.05));Upload(FMath::Clamp(int(Time*FPS),0,Count-1));if(Playing)StartAudio();}
 void Restart(){Seek(0);}
 FString Clock()const{return FString::Printf(TEXT("%02d:%02d  /  %02d:%02d"),int(Time)/60,int(Time)%60,int(Duration())/60,int(Duration())%60);}
 virtual bool SupportsKeyboardFocus()const override{return true;}
 virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& E)override{auto K=E.GetKey();if(K==EKeys::Escape){if(Mode.IsValid())Mode->Show(TEXT("title"));}else if(K==EKeys::SpaceBar&&Trailer)Toggle();else if(K==EKeys::Left){if(Trailer)Seek(Time-5);else Navigate(-1);}else if(K==EKeys::Right){if(Trailer)Seek(Time+5);else Navigate(1);}else return FReply::Unhandled();return FReply::Handled();}
 virtual void Tick(const FGeometry& Geom,double Now,float Delta)override{SCompoundWidget::Tick(Geom,Now,Delta);if(!Focused){FSlateApplication::Get().SetKeyboardFocus(AsShared(),EFocusCause::SetDirectly);Focused=true;}if(Trailer&&Playing&&Movie){Time=FMath::Min(Time+double(Delta),Duration());Upload(FMath::Min(int(Time*FPS),Count-1));if(Time>=Duration()){Playing=false;if(Audio.IsValid())Audio->Stop();}}}
 void Rebuild()
 {
 auto Body=SNew(SVerticalBox);auto Header=SNew(SHorizontalBox);Header->AddSlot().AutoWidth()[Button(TEXT("<  ГЛАВНОЕ МЕНЮ"),[this](){if(Mode.IsValid())Mode->Show(TEXT("title"));})];Header->AddSlot().FillWidth(1).HAlign(HAlign_Right).VAlign(VAlign_Center)[Label(TEXT("Э Х О   /   О Т Д Е Л Е Н И Е  0 4"),18,Dim,true)];Body->AddSlot().AutoHeight()[Header];
 Body->AddSlot().AutoHeight().Padding(0,28,0,8)[Label(Trailer?TEXT("Т Р Е Й Л Е Р"):TEXT("К А Д Р Ы  И З  Т И Ш И Н Ы"),34,Cream,true)];Body->AddSlot().AutoHeight().Padding(0,0,0,22)[Label(Trailer?TEXT("Она позвала его. Но чей голос ждёт внутри?"):TEXT("Место, в котором каждый кадр хранит чужую историю."),16,Dim)];
 auto Row=SNew(SHorizontalBox);auto Left=SNew(SVerticalBox);
 const auto& E=Entries[Visible.IsValidIndex(Selected)?Visible[Selected]:0];const FSlateBrush* MainImage=Trailer?(Movie?&VideoBrush:Picture(TEXT("03_hospital"))):Picture(E.Id);
 Left->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(&White78).BorderBackgroundColor(FLinearColor(.20,.025,.03)).Padding(1)[SNew(SBox).WidthOverride(1184).HeightOverride(666)[SNew(SImage).Image_Lambda([this,MainImage](){return Trailer&&Time==0&&!Playing?Picture(TEXT("04_aruzhan")):MainImage;})]]];
 if(Trailer)
 {
 Left->AddSlot().AutoHeight().Padding(0,16,0,7)[SNew(SSlider).Value_Lambda([this](){return Duration()>0?float(Time/Duration()):0.f;}).OnValueChanged_Lambda([this](float V){Seek(V*Duration());}).SliderBarColor(FLinearColor(.15,.025,.035)).SliderHandleColor(Red)];
 auto Controls=SNew(SHorizontalBox);Controls->AddSlot().AutoWidth()[SNew(SButton).ButtonStyle(&Button78).ContentPadding(FMargin(20,10)).OnClicked_Lambda([this](){Toggle();return FReply::Handled();})[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Playing?TEXT("Пауза"):TEXT("Смотреть"));}).Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),16)).ColorAndOpacity(Cream)]];Controls->AddSlot().AutoWidth().Padding(8,0)[Button(TEXT("Сначала"),[this](){Restart();})];Controls->AddSlot().AutoWidth()[SNew(SButton).ButtonStyle(&Button78).ContentPadding(FMargin(18,10)).OnClicked_Lambda([this](){Muted=!Muted;if(Audio.IsValid())Audio->SetVolumeMultiplier(Muted?0:1);return FReply::Handled();})[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Muted?TEXT("Звук: выкл."):TEXT("Звук: вкл."));}).Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),16)).ColorAndOpacity(Cream)]];Controls->AddSlot().FillWidth(1).HAlign(HAlign_Right).VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Clock());}).Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),16)).ColorAndOpacity(Dim)];Left->AddSlot().AutoHeight()[Controls];
 }
 else{auto Nav=SNew(SHorizontalBox);Nav->AddSlot().AutoWidth()[Button(TEXT("<"),[this](){Navigate(-1);})];Nav->AddSlot().FillWidth(1).Padding(18,8)[Label(E.Title,22,Cream,true)];Nav->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10,0)[Label(FString::Printf(TEXT("%02d / %02d"),Selected+1,Visible.Num()),16,Dim)];Nav->AddSlot().AutoWidth()[Button(TEXT(">"),[this](){Navigate(1);})];Left->AddSlot().AutoHeight().Padding(0,14)[Nav];Left->AddSlot().AutoHeight()[Label(E.Art?TEXT("КИНОИЛЛЮСТРАЦИЯ  /  создано с помощью ИИ"):TEXT("КАДР ИЗ ИГРЫ  /  Unreal Engine · игровое освещение · без HUD"),13,Red)];}
 Row->AddSlot().AutoWidth()[Left];auto Side=SNew(SVerticalBox);
 if(Trailer){Side->AddSlot().AutoHeight()[Label(TEXT("ПОСЛЕДНИЙ ЗВОНОК"),23,Red,true)];Side->AddSlot().AutoHeight().Padding(0,24)[Label(TEXT("Вчера они были вместе.\n\nСегодня Аружан не отвечает.\n\nЕё след ведёт туда, где давно не должно быть никого живого."),21,Cream,true)];Side->AddSlot().AutoHeight().Padding(0,15)[SNew(SBox).HeightOverride(210)[SNew(SImage).Image(Picture(TEXT("01_together"),true))]];Side->AddSlot().AutoHeight().Padding(0,18)[Label(TEXT("Киноиллюстрации, игровые съёмки и оригинальный звук EXO."),14,Dim)];Side->AddSlot().AutoHeight()[Label(Error.IsEmpty()?TEXT("Пробел — пауза\nСтрелки — перемотка на 5 секунд\nEsc — главное меню"):Error,14,Error.IsEmpty()?Dim:Red)];}
 else{auto Filters=SNew(SHorizontalBox);for(int I=0;I<3;++I)Filters->AddSlot().FillWidth(1).Padding(2,0)[Button(I==0?TEXT("Все"):I==1?TEXT("Игра"):TEXT("Арт"),[this,I](){ApplyFilter(I);Rebuild();},Filter==I)];Side->AddSlot().AutoHeight()[Filters];Side->AddSlot().AutoHeight().Padding(2,12)[Label(TEXT("ИГРА / 12     КИНОИЛЛЮСТРАЦИИ / 8"),12,Dim)];auto Scroll=SNew(SScrollBox);for(int I=0;I<Visible.Num();I+=2){auto Pair=SNew(SHorizontalBox);for(int J=I;J<FMath::Min(I+2,Visible.Num());++J){const auto& V=Entries[Visible[J]];Pair->AddSlot().FillWidth(1).Padding(3)[SNew(SButton).ButtonStyle(&Button78).ContentPadding(2).OnClicked_Lambda([this,J](){Selected=J;Rebuild();return FReply::Handled();})[SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(100)[SNew(SImage).Image(Picture(V.Id,true))]]+SVerticalBox::Slot().AutoHeight().Padding(4,6)[Label(V.Title,12,J==Selected?Red:Cream)]]];}Scroll->AddSlot()[Pair];}Side->AddSlot().FillHeight(1)[Scroll];}
 Row->AddSlot().FillWidth(1).Padding(35,0,0,0)[Side];Body->AddSlot().FillHeight(1)[Row];Body->AddSlot().AutoHeight().Padding(0,14,0,0)[Label(TEXT("E C H O     /     ОСТАНЬСЯ В ТИШИНЕ."),12,Dim,true)];
 ChildSlot[SNew(SBorder).BorderImage(&White78).BorderBackgroundColor(Ink).Padding(0)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[SNew(SBorder).BorderImage(&White78).BorderBackgroundColor(Ink).Padding(FMargin(75,42))[Body]]]]];
 }
 Json Audit(const FString& A){if(A==TEXT("art")){ApplyFilter(2);Rebuild();}else if(A==TEXT("game")){ApplyFilter(1);Rebuild();}else if(A==TEXT("next"))Navigate(1);else if(A==TEXT("play")){if(!Playing)Toggle();}else if(A==TEXT("pause")){if(Playing)Toggle();}else if(A==TEXT("seek"))Seek(Duration()*.5);else if(A==TEXT("restart"))Restart();else if(A==TEXT("mute")){Muted=!Muted;if(Audio.IsValid())Audio->SetVolumeMultiplier(Muted?0:1);}auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("exists"),true);J->SetNumberField(TEXT("count"),Visible.Num());J->SetBoolField(TEXT("image"),!Trailer&&Picture(Entries[Visible[Selected]].Id)!=nullptr);J->SetBoolField(TEXT("video"),Movie.IsValid());J->SetBoolField(TEXT("audio"),Audio.IsValid());J->SetBoolField(TEXT("playing"),Playing);J->SetBoolField(TEXT("muted"),Muted);J->SetNumberField(TEXT("frame"),Frame);J->SetNumberField(TEXT("time"),Time);return J;}
 };
}
TSharedRef<SWidget> MakeWardMedia78(AEWardCampaignMode* G,const FString& Kind){auto W=SNew(WardMedia78::SMedia78).Mode(G).Trailer(Kind==TEXT("trailer"));WardMedia78::Active78=W;return W;}
Json WardMediaAudit78(const FString& Action){if(auto W=WardMedia78::Active78.Pin())return W->Audit(Action);auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("exists"),false);return J;}
