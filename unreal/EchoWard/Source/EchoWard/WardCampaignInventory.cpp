#include "WardCampaign.h"
FString AEWardCampaignMode::ItemName(const FString& Id)const
{
    static const TMap<FString,FString> Names={{TEXT("weapon"),TEXT("Служебный пистолет")},{TEXT("shotgun"),TEXT("Помповый дробовик")},{TEXT("ammo"),TEXT("Патроны")},{TEXT("shells"),TEXT("Патроны дробовика")},{TEXT("medkit"),TEXT("Аптечка")},{TEXT("battery"),TEXT("Батарея")},{TEXT("bottle"),TEXT("Бутылка")},{TEXT("boards"),TEXT("Доски")},{TEXT("fuse"),TEXT("Предохранитель")},{TEXT("badge"),TEXT("Пропуск сотрудника")},{TEXT("fuel"),TEXT("Топливо")},{TEXT("coolant"),TEXT("Хладагент")},{TEXT("residence_prybar"),TEXT("Монтажный инструмент")},{TEXT("residence_core"),TEXT("Модуль лифта")},{TEXT("isolation_coil"),TEXT("Контактный блок")},{TEXT("isolation_tag"),TEXT("Браслет пациента")},{TEXT("annex_wheel"),TEXT("Маховик")}};
    return Tr(Names.Contains(Id)?Names[Id]:Id);
}
