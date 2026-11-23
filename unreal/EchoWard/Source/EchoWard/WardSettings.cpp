#include "WardSettings.h"
#include "WardCampaign.h"
#include "WardCampaignUI.h"
using namespace WardData;
void AEWardCampaignMode::SetNumberSetting(const FString& Key,double Value,bool Persist)
{
    if(!FMath::IsFinite(Value))return;WardSettings::Complete(Options);const auto R=WardSettings::Range(Key);
    Value=FMath::Clamp(Value,double(R.Min),double(R.Max));Options->SetNumberField(Key,Value);
    if(Key==TEXT("grain_amount"))Options->SetBoolField(TEXT("grain"),Value>0);
    ApplySettings(Persist);
}
void AEWardCampaignMode::CycleSetting(const FString& Key,int Direction)
{
    if(Key==TEXT("inspection_bright")&&!Admin)return;
    WardSettings::Complete(Options);auto V=Options->TryGetField(Key);if(!V.IsValid())return;
    if(V->Type==EJson::Boolean)Options->SetBoolField(Key,!V->AsBool());
    else if(V->Type==EJson::String)
    {
        const TArray<FString> C=Key==TEXT("resolution")?TArray<FString>{TEXT("1280x720"),TEXT("1600x900"),TEXT("1920x1080"),TEXT("2560x1440")}:Key==TEXT("language")?TArray<FString>{TEXT("ru"),TEXT("kk"),TEXT("en")}:Key==TEXT("shadow_quality")?TArray<FString>{TEXT("off"),TEXT("medium"),TEXT("high")}:TArray<FString>{TEXT("standard"),TEXT("high")};
        Options->SetStringField(Key,C[(FMath::Max(0,C.IndexOfByKey(V->AsString()))+Direction+C.Num())%C.Num()]);
    }
    else
    {
        const TArray<double> C=Key==TEXT("graphics_preset")?TArray<double>{0,1,2}:Key==TEXT("screen_percentage")?TArray<double>{50,67,75,85,100}:Key==TEXT("max_fps")?TArray<double>{30,60,90,120,0}:Key==TEXT("msaa")?TArray<double>{0,1,2,3}:Key==TEXT("fov")?TArray<double>{55,60,65,70,75,80,85}:Key==TEXT("music")||Key==TEXT("effects")?TArray<double>{0,.2,.4,.6,.8,1}:TArray<double>{.5,.75,1,1.25,1.5};
        int I=0;for(int J=0;J<C.Num();++J)if(FMath::Abs(C[J]-V->AsNumber())<.01)I=J;Options->SetNumberField(Key,C[(I+Direction+C.Num())%C.Num()]);
        if(Key==TEXT("graphics_preset")){Options->SetNumberField(TEXT("msaa"),3);Options->SetNumberField(TEXT("screen_percentage"),Setting(Key)==2?67:100);Options->SetStringField(TEXT("shadow_quality"),Setting(Key)==2?TEXT("medium"):TEXT("high"));Options->SetNumberField(TEXT("max_fps"),Setting(Key)==2?30:60);}
    }
    ApplySettings();if(Interface&&!Screen.IsEmpty())Interface->Refresh();
}

void WardSettings::AddTranslations(TMap<FString,TPair<FString,FString>>& Catalog)
{
    const TCHAR* Entries[][3]={
        {TEXT("Яркий осмотр карты · B"),TEXT("Bright map inspection · B"),TEXT("Картаны жарық қарау · B")},
        {TEXT("Качество графики"),TEXT("Graphics quality"),TEXT("Графика сапасы")},
        {TEXT("Масштаб рендера"),TEXT("Render scale"),TEXT("Рендер масштабы")},
        {TEXT("Размытие в движении"),TEXT("Motion blur"),TEXT("Қозғалыс бұлыңғырлығы")},
        {TEXT("Зернистость плёнки"),TEXT("Film grain"),TEXT("Пленка түйіршіктілігі")},
        {TEXT("Поле зрения (FOV)"),TEXT("Field of view (FOV)"),TEXT("Көру өрісі (FOV)")},
        {TEXT("Высокое"),TEXT("High"),TEXT("Жоғары")},
        {TEXT("Сбалансированное"),TEXT("Balanced"),TEXT("Теңгерімді")},
        {TEXT("Производительность"),TEXT("Performance"),TEXT("Өнімділік")},
        {TEXT("Среднее"),TEXT("Medium"),TEXT("Орташа")},
        {TEXT("Без ограничений"),TEXT("Unlimited"),TEXT("Шектеусіз")},
        {TEXT("Разрешение"),TEXT("Resolution"),TEXT("Ажыратымдылық")},
        {TEXT("Режим администратора"),TEXT("Administrator mode"),TEXT("Әкімші режимі")}};
    for(const auto& E:Entries)Catalog.Add(E[0],TPair<FString,FString>(E[1],E[2]));
}
