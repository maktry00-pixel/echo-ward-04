#include "WardHospitalGameplay.h"
#include "WardNotes65.h"
#include "WardInterface66.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "WardBookWorld.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "WardPeople54.h"
#include "WardPeople57.h"
#include "WardHomeAssets53.h"
#include "Components/AudioComponent.h"
#include "WardCampaignUI.h"
#include "WardCampaign.h"
#include "WardCampaignMap.h"
#include "WardBookMap.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/SLeafWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Rendering/DrawElements.h"
#include "Misc/Paths.h"
using namespace WardData;
void AddWardFieldTranslations(TMap<FString,TPair<FString,FString>>& Catalog)
{
    struct FEntry {const TCHAR* Ru;const TCHAR* En;const TCHAR* Kk;};
    const FEntry Entries[]={
        {TEXT("ЗАДАНИЯ"),TEXT("OBJECTIVES"),TEXT("ТАПСЫРМАЛАР")},
        {TEXT("УПРАВЛЕНИЕ"),TEXT("CONTROLS"),TEXT("БАСҚАРУ")},
        {TEXT("Задания  [O]"),TEXT("Objectives  [O]"),TEXT("Тапсырмалар  [O]")},
        {TEXT("Управление  [F1]"),TEXT("Controls  [F1]"),TEXT("Басқару  [F1]")},
        {TEXT("Моё положение"),TEXT("My location"),TEXT("Менің орным")},
        {TEXT("Текущая цель"),TEXT("Current objective"),TEXT("Ағымдағы мақсат")},
        {TEXT("Непрочитанные"),TEXT("Unread"),TEXT("Оқылмағандар")},
        {TEXT("Все найденные"),TEXT("All collected"),TEXT("Барлық табылғандар")},
        {TEXT("Сбросить поиск"),TEXT("Clear search"),TEXT("Іздеуді тазарту")},
        {TEXT("Текущая задача"),TEXT("Current objective"),TEXT("Ағымдағы тапсырма")},
        {TEXT("Нужно найти"),TEXT("Find these items"),TEXT("Табу керек")},
        {TEXT("Следующее действие"),TEXT("Next action"),TEXT("Келесі әрекет")},
        {TEXT("Расследование"),TEXT("Investigation"),TEXT("Тергеу")},
        {TEXT("Источники для передачи"),TEXT("Evidence to transmit"),TEXT("Жіберілетін деректер")},
        {TEXT("Принятые решения"),TEXT("Decisions made"),TEXT("Қабылданған шешімдер")},
        {TEXT("Дополнительные истории"),TEXT("Side stories"),TEXT("Қосымша оқиғалар")},
        {TEXT("Посещение завершено"),TEXT("Visit complete"),TEXT("Сапар аяқталды")},
        {TEXT("Показать место задачи на карте"),TEXT("Show objective on the map"),TEXT("Мақсатты картадан көрсету")},
        {TEXT("Ниже показаны только события, уже записанные в журнале."),TEXT("Only events already recorded in the journal appear below."),TEXT("Төменде журналға жазылған оқиғалар ғана көрсетіледі.")},
        {TEXT("Решения появятся после первых разговоров."),TEXT("Decisions appear after your first conversations."),TEXT("Шешімдер алғашқы әңгімелерден кейін пайда болады.")},
        {TEXT("Найденные документы и принятые решения сохранены в журнале."),TEXT("Collected documents and decisions remain in the journal."),TEXT("Табылған құжаттар мен шешімдер журналда сақталады.")},
        {TEXT("Следуй к месту текущей задачи. F — взаимодействие. Обязательные предметы этого этапа найдены."),TEXT("Go to the current objective. F — interact. Required items for this stage have been collected."),TEXT("Ағымдағы мақсатқа бар. F — әрекеттесу. Осы кезеңнің қажетті заттары табылды.")},
        {TEXT("Служебный пистолет. ПКМ — прицелиться, ЛКМ — выстрел, R — перезарядка."),TEXT("Service pistol. RMB — aim, LMB — fire, R — reload."),TEXT("Қызметтік тапанша. Тінтуірдің оң жақ батырмасы — көздеу, сол жақ — ату, R — оқтау.")},
        {TEXT("Помповый дробовик, 5 патронов. Z — сменить оружие, R — поочерёдная перезарядка."),TEXT("Pump shotgun, 5 shells. Z — switch weapon, R — load shells."),TEXT("Помпалы мылтық, 5 оқ. Z — қаруды ауыстыру, R — оқтау.")},
        {TEXT("Патроны пистолета. Используй или объедини с пистолетом для зарядки магазина."),TEXT("Pistol ammunition. Use or combine with the pistol to load the magazine."),TEXT("Тапанша оғы. Оқтау үшін қолдан немесе тапаншамен біріктір.")},
        {TEXT("Патроны дробовика. Используй или объедини с дробовиком для зарядки."),TEXT("Shotgun shells. Use or combine with the shotgun to load it."),TEXT("Мылтық оғы. Оқтау үшін қолдан немесе мылтықпен біріктір.")},
        {TEXT("Перевязка занимает 2,4 секунды. Удар прерывает лечение. Использование закроет инвентарь."),TEXT("Bandaging takes 2.4 seconds. Damage interrupts healing. Use closes the inventory."),TEXT("Таңу 2,4 секундқа созылады. Соққы емдеуді тоқтатады. Қолдану жабдық экранын жабады.")},
        {TEXT("Запасной заряд ремонтного ослепителя. T — зарядить, E — вспышка."),TEXT("Spare charge for the flash unit. T — charge, E — flash."),TEXT("Жарқыл құрылғысының қосымша қуаты. T — қуаттау, E — жарқыл.")},
        {TEXT("Служебный пропуск. Осмотри оборот. Сюжетные допуски не расходуются."),TEXT("Staff pass. Inspect the reverse. Story clearances are not consumed."),TEXT("Қызметтік рұқсатнама. Артқы жағын қара. Оқиғалық рұқсаттар жұмсалмайды.")},
        {TEXT("G — бросить бутылку и отвлечь противника шумом."),TEXT("G — throw a bottle to distract enemies with noise."),TEXT("G — бөтелкені лақтырып, жауды дыбыспен алдау.")},
        {TEXT("Материал для укрепления двери. K — засов убежища или укрепление подходящей двери."),TEXT("Door reinforcement material. K — refuge bolt or reinforce a suitable door."),TEXT("Есікті бекіту материалы. K — паналау ілмегі немесе жарамды есікті бекіту.")},
        {TEXT("Запасной предохранитель. Нужен для местного питания; не выбрасывается."),TEXT("Spare fuse for local power. Cannot be discarded."),TEXT("Жергілікті қуатқа қажетті қосалқы сақтандырғыш. Тастауға болмайды.")},
        {TEXT("Монтажный инструмент для технических механизмов."),TEXT("Prying tool for service mechanisms."),TEXT("Техникалық тетіктерге арналған монтаж құралы.")},
        {TEXT("Браслет медицинского допуска. Не заменяет служебный пропуск."),TEXT("Medical clearance bracelet. Does not replace a staff pass."),TEXT("Медициналық рұқсат білезігі. Қызметтік рұқсатнаманы алмастырмайды.")},
        {TEXT("Сюжетный предмет. Используется у связанного механизма; выбросить его нельзя."),TEXT("Story item for its associated mechanism. Cannot be discarded."),TEXT("Байланысты тетікке арналған оқиғалық зат. Тастауға болмайды.")},
        {TEXT("Полный план"),TEXT("Full plan"),TEXT("Толық жоспар")},
        {TEXT("Текущая игра"),TEXT("Current game"),TEXT("Ағымдағы ойын")},
        {TEXT("Все планы"),TEXT("All plans"),TEXT("Барлық жоспарлар")},
        {TEXT("Масштаб +"),TEXT("Zoom +"),TEXT("Үлкейту +")},
        {TEXT("Масштаб -"),TEXT("Zoom -"),TEXT("Кішірейту -")},
        {TEXT("Сбросить вид"),TEXT("Reset view"),TEXT("Көріністі қалпына келтіру")},
        {TEXT("Полный план из книги · 328 помещений · 13 локаций"),TEXT("Full book plan · 328 rooms · 13 locations"),TEXT("Кітаптағы толық жоспар · 328 бөлме · 13 орын")},
        {TEXT("Треугольник — вы · ромб — текущая цель · колесо — масштаб · перетаскивание — перемещение"),TEXT("Triangle — you · diamond — objective · wheel — zoom · drag — pan"),TEXT("Үшбұрыш — сіз · ромб — мақсат · дөңгелек — масштаб · сүйреу — жылжыту")},
        {TEXT("ИНВЕНТАРЬ"),TEXT("INVENTORY"),TEXT("ЖАБДЫҚ")},
        {TEXT("ДОКУМЕНТЫ"),TEXT("DOCUMENTS"),TEXT("ҚҰЖАТТАР")},
        {TEXT("КАРТА"),TEXT("MAP"),TEXT("КАРТА")},
        {TEXT("АРХИВ ГОЛОСОВ"),TEXT("VOICE ARCHIVE"),TEXT("ДАУЫС МҰРАҒАТЫ")},
        {TEXT("Архив голосов"),TEXT("Voice archive"),TEXT("Дауыс мұрағаты")},
        {TEXT("Лицо / в полный рост"),TEXT("Face / full body"),TEXT("Бет / толық бой")},
        {TEXT("Прослушать реплику"),TEXT("Play line"),TEXT("Репликаны тыңдау")},
        {TEXT("Показать жест"),TEXT("Show gesture"),TEXT("Қимылды көрсету")},
        {TEXT("Модель недоступна"),TEXT("Model unavailable"),TEXT("Модель қолжетімсіз")},
        {TEXT("Инвентарь  [I]"),TEXT("Inventory  [I]"),TEXT("Жабдық  [I]")},
        {TEXT("Документы  [J]"),TEXT("Documents  [J]"),TEXT("Құжаттар  [J]")},
        {TEXT("Карта  [M]"),TEXT("Map  [M]"),TEXT("Карта  [M]")},
        {TEXT("Закрыть  [Esc]"),TEXT("Close  [Esc]"),TEXT("Жабу  [Esc]")},
        {TEXT("Использовать"),TEXT("Use"),TEXT("Қолдану")},
        {TEXT("Объединить"),TEXT("Combine"),TEXT("Біріктіру")},
        {TEXT("Выбросить"),TEXT("Discard"),TEXT("Тастау")},
        {TEXT("Отменить"),TEXT("Cancel"),TEXT("Бас тарту")},
        {TEXT("Инвентарь пуст"),TEXT("Inventory is empty"),TEXT("Жабдық бос")},
        {TEXT("Оружие заряжено."),TEXT("Weapon loaded."),TEXT("Қару оқталды.")},
        {TEXT("Эти предметы нельзя объединить."),TEXT("These items cannot be combined."),TEXT("Бұл заттарды біріктіру мүмкін емес.")},
        {TEXT("Выбери второй предмет. Повторное нажатие отменяет выбор."),TEXT("Select a second item. Press again to cancel."),TEXT("Екінші затты таңда. Бас тарту үшін қайта бас.")},
        {TEXT("Обычный служебный пистолет. Надёжный, но не очень мощный."),TEXT("A standard service pistol. Reliable, but not very powerful."),TEXT("Кәдімгі қызметтік тапанша. Сенімді, бірақ қуаты аса жоғары емес.")},
        {TEXT("Помповый дробовик. Мощный на близкой дистанции. Магазин на 5 патронов."),TEXT("A pump shotgun. Powerful at close range. Holds 5 shells."),TEXT("Помпалы мылтық. Жақын қашықтықта қуатты. Оқжатарға 5 оқ сыяды.")},
        {TEXT("Патроны для служебного пистолета. Объедини с пистолетом, чтобы зарядить магазин."),TEXT("Service pistol ammunition. Combine with the pistol to load its magazine."),TEXT("Қызметтік тапаншаның оқтары. Оқтау үшін тапаншамен біріктір.")},
        {TEXT("Патроны дробовика. Объедини с дробовиком, чтобы зарядить его."),TEXT("Shotgun shells. Combine with the shotgun to load it."),TEXT("Мылтық оқтары. Оқтау үшін мылтықпен біріктір.")},
        {TEXT("Медицинские принадлежности. Восстанавливают здоровье."),TEXT("Medical supplies. Restore health."),TEXT("Медициналық жабдықтар. Денсаулықты қалпына келтіреді.")},
        {TEXT("Запасная батарея. Восстанавливает заряд вспышки."),TEXT("A spare battery. Restores a flash charge."),TEXT("Қосалқы батарея. Жарқыл зарядын қалпына келтіреді.")},
        {TEXT("Пропуск сотрудника. Осмотри оборот: на нём сохранилась надпись."),TEXT("A staff pass. Examine the back: a faded inscription remains."),TEXT("Қызметкер рұқсатнамасы. Артқы бетін қара: онда жазу сақталған.")},
        {TEXT("Пустая бутылка. Брось её клавишей G, чтобы отвлечь противника."),TEXT("An empty bottle. Press G to throw it and distract an enemy."),TEXT("Бос бөтелке. Жаудың назарын бұру үшін G пернесімен лақтыр.")},
        {TEXT("Доски для укрепления дверей. Подойди к двери и нажми K."),TEXT("Boards for barricading doors. Approach a door and press K."),TEXT("Есікті бекітуге арналған тақтайлар. Есікке жақындап, K пернесін бас.")},
        {TEXT("Предмет для исследования отделения. Используется у подходящего оборудования."),TEXT("An item for exploring the ward. Use it at the appropriate equipment."),TEXT("Бөлімді зерттеуге арналған зат. Тиісті жабдықтың жанында қолданылады.")},
        {TEXT("Записка %d"),TEXT("Note %d"),TEXT("Жазба %d")},
        {TEXT("Журнал дежурств"),TEXT("Duty log"),TEXT("Кезекшілік журналы")},
        {TEXT("Карта пациента"),TEXT("Patient record"),TEXT("Науқас картасы")},
        {TEXT("Личный дневник"),TEXT("Personal diary"),TEXT("Жеке күнделік")},
        {TEXT("План отделения"),TEXT("Ward plan"),TEXT("Бөлім жоспары")},
        {TEXT("Неизвестный документ"),TEXT("Unknown document"),TEXT("Белгісіз құжат")},
        {TEXT("Прослушать запись"),TEXT("Play recording"),TEXT("Жазбаны тыңдау")},
        {TEXT("Документов по этому фильтру нет. Найденные записки появятся здесь после подбора."),TEXT("Collected documents will appear here."),TEXT("Табылған құжаттар осында көрсетіледі.")},
        {TEXT("ОТДЕЛЕНИЕ 04    /    %02d"),TEXT("WARD 04    /    %02d"),TEXT("04-БӨЛІМ    /    %02d")},
        {TEXT("Отделение 04"),TEXT("Ward 04"),TEXT("04-бөлім")},
        {TEXT("Подвал"),TEXT("Basement"),TEXT("Жертөле")},
        {TEXT("Архив"),TEXT("Archive"),TEXT("Мұрағат")},
        {TEXT("Игрок"),TEXT("Player"),TEXT("Ойыншы")},
        {TEXT("Цель"),TEXT("Objective"),TEXT("Мақсат")},
        {TEXT("Дверь"),TEXT("Door"),TEXT("Есік")},
        {TEXT("Закрыто"),TEXT("Locked"),TEXT("Құлыптаулы")},
        {TEXT("Событие"),TEXT("Event"),TEXT("Оқиға")},
        {TEXT("Колесо мыши — масштаб · перетаскивание — перемещение"),TEXT("Mouse wheel — zoom · drag — pan"),TEXT("Тінтуір дөңгелегі — масштаб · сүйреу — жылжыту")},
        {TEXT("Найди план в приёмной."),TEXT("Find the plan in reception."),TEXT("Қабылдау бөлмесінен жоспарды тап.")}
    };
    for(const auto& E:Entries)Catalog.Add(E.Ru,TPair<FString,FString>(E.En,E.Kk));
    Catalog.Add(WardInterface66::Controls(false),TPair<FString,FString>(TEXT("MOVEMENT\nW A S D - move · mouse - look\nShift - sprint · Ctrl - crouch · Space - jump\n\nINTERACTION\nF - collect, talk, open; hold F - search\nN - flashlight · E - flash · Y - vehicle parts\nPage Up / Page Down - select lift stop · F - call / travel\nK - refuge bolt / reinforce a suitable door\n\nCOMBAT AND SUPPLIES\nRMB - aim · LMB - fire · R - reload\nZ - switch weapon · V - knife · G - throw bottle\nH - bandage · T - flash charge · Q - dash\n\nMENUS\nI - inventory · J - documents · M or Tab - map\nO - objectives · F1 - controls · Esc - back / pause\nPress the current menu key again to close it.\nF5 - quick save · B - bright inspection\n\nPROLOGUE\nAt home: F - telephone; E - leave after the conversation."),TEXT("ҚОЗҒАЛЫС\nW A S D - қозғалу · тінтуір - айналаны қарау\nShift - жүгіру · Ctrl - отыру · Бос орын - секіру\n\nӘРЕКЕТТЕСУ\nF - алу, сөйлесу, ашу; F ұстап тұру - тінту\nN - шам · E - жарқыл · Y - көлік бөліктері\nPage Up / Page Down - лифт қабатын таңдау · F - шақыру / бару\nK - паналау ілмегі / жарамды есікті бекіту\n\nШАЙҚАС ЖӘНЕ ҚОРЛАР\nТінтуірдің оң жағы - көздеу · сол жағы - ату · R - оқтау\nZ - қаруды ауыстыру · V - пышақ · G - бөтелке лақтыру\nH - таңу · T - жарқыл қуаты · Q - жұлқыну\n\nЭКРАНДАР\nI - жабдық · J - құжаттар · M немесе Tab - карта\nO - тапсырмалар · F1 - басқару · Esc - артқа / үзіліс\nАшық экранның пернесін қайта басу оны жабады.\nF5 - жылдам сақтау · B - жарық тексеру\n\nКІРІСПЕ\nҮйде: F - телефон; E - әңгімеден кейін шығу.")));
    Catalog.Add(WardInterface66::Controls(true),TPair<FString,FString>(TEXT("MOVEMENT\nW A S D - move · mouse - look\nShift - sprint · Ctrl - crouch · Space - jump\n\nINTERACTION\nF - collect, talk, open; hold F - search\nN - flashlight · E - flash · Y - vehicle parts\nPage Up / Page Down - select lift stop · F - call / travel\nK - refuge bolt / reinforce a suitable door\n\nCOMBAT AND SUPPLIES\nRMB - aim · LMB - fire · R - reload\nZ - switch weapon · V - knife · G - throw bottle\nH - bandage · T - flash charge · Q - dash\n\nMENUS\nI - inventory · J - documents · M or Tab - map\nO - objectives · F1 - controls · Esc - back / pause\nPress the current menu key again to close it.\nF5 - quick save · B - bright inspection\n\nPROLOGUE\nAt home: F - telephone; E - leave after the conversation.\n\nADMINISTRATOR\nF8 - item catalogue · F9 - agent · F10 - character\nF3 - frame counter · F4 - graphics profile"),TEXT("ҚОЗҒАЛЫС\nW A S D - қозғалу · тінтуір - айналаны қарау\nShift - жүгіру · Ctrl - отыру · Бос орын - секіру\n\nӘРЕКЕТТЕСУ\nF - алу, сөйлесу, ашу; F ұстап тұру - тінту\nN - шам · E - жарқыл · Y - көлік бөліктері\nPage Up / Page Down - лифт қабатын таңдау · F - шақыру / бару\nK - паналау ілмегі / жарамды есікті бекіту\n\nШАЙҚАС ЖӘНЕ ҚОРЛАР\nТінтуірдің оң жағы - көздеу · сол жағы - ату · R - оқтау\nZ - қаруды ауыстыру · V - пышақ · G - бөтелке лақтыру\nH - таңу · T - жарқыл қуаты · Q - жұлқыну\n\nЭКРАНДАР\nI - жабдық · J - құжаттар · M немесе Tab - карта\nO - тапсырмалар · F1 - басқару · Esc - артқа / үзіліс\nАшық экранның пернесін қайта басу оны жабады.\nF5 - жылдам сақтау · B - жарық тексеру\n\nКІРІСПЕ\nҮйде: F - телефон; E - әңгімеден кейін шығу.\n\nӘКІМШІ\nF8 - заттар тізімі · F9 - агент · F10 - кейіпкер\nF3 - кадр есептегіші · F4 - графика профилі")));
}
namespace
{
    FSlateColorBrush Black(FLinearColor(.009,.002,.003,1)),Line(FLinearColor(.25,.055,.052)),Paper(FLinearColor(.52,.43,.29));
    FButtonStyle FieldStyle=[](){FButtonStyle S;S.SetNormal(FSlateColorBrush(FLinearColor(.003,.004,.004)));S.SetHovered(FSlateColorBrush(FLinearColor(.22,.008,.012)));S.SetPressed(FSlateColorBrush(FLinearColor(.35,.012,.015)));S.SetDisabled(FSlateColorBrush(FLinearColor(.002,.003,.003)));return S;}();
    FLinearColor Ink(.82,.74,.69),Muted(.54,.33,.29);
    static int32 PreviewBrightPixels66=0;
    // Each preview has an isolated, transient stage; no objects are added to the playable rooms.
    class SWardItemImage : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SWardItemImage){}SLATE_ARGUMENT(AEWardCampaignMode*,Mode)SLATE_ARGUMENT(FString,Item)SLATE_END_ARGS()
        void Construct(const FArguments& A)
        {
            auto* G=A._Mode;FString Key=A._Item==TEXT("weapon")?TEXT("equipped_pistol"):A._Item==TEXT("shotgun")?TEXT("equipped_shotgun"):TEXT("loose_")+A._Item;
            const auto* Rows=G->Visuals.Find(Key);if(!Rows)Rows=G->Visuals.Find(A._Item);if(!Rows||Rows->IsEmpty()){ChildSlot[SNew(STextBlock).Text(FText::FromString(G->ItemName(A._Item))).AutoWrapText(true).Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),14)).ColorAndOpacity(Ink)];return;}
            FString PreviewPath;if(Key.StartsWith(TEXT("loose_"))&&!G->Visuals.Contains(Key))for(auto V:Array(G->Data,TEXT("objects"))){Json O=V->AsObject();if(String(O,TEXT("id"))==A._Item)PreviewPath=String(Object(Object(O,TEXT("storage")),TEXT("preview")),TEXT("node"));}
            Stage=G->GetWorld()->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Stage.Get());Stage->SetRootComponent(Root);Root->RegisterComponent();
            static int Serial=0;const FVector Origin(0,0,100000+(Serial++%50)*1000);Stage->SetActorLocation(Origin);
            TArray<UStaticMeshComponent*> Parts;FBox Bounds(ForceInit);
            for(Json R:*Rows)
            {
                if(!PreviewPath.IsEmpty()?!String(R,TEXT("path")).StartsWith(PreviewPath):!Boolean(R,TEXT("visible"),true))continue;auto* Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath(String(R,TEXT("mesh"))));if(!Mesh)continue;
                auto* C=NewObject<UStaticMeshComponent>(Stage.Get());C->bDisallowNanite=true;C->SetupAttachment(Root);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetVisibleInSceneCaptureOnly(true);
                int I=0;for(auto V:Array(R,TEXT("materials"))){auto* Parent=LoadObject<UMaterialInterface>(nullptr,*MaterialPath(V->AsString()));if(Parent){auto* M=UMaterialInstanceDynamic::Create(Parent,Stage.Get());M->SetVectorParameterValue(TEXT("Emission"),FLinearColor::Black);float Metal=0;FLinearColor Tint;Parent->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Metallic")),Metal);if(Metal<.2f&&Parent->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Tint")),Tint)){const float Alpha=Tint.A;Tint*=.85f;Tint.A=Alpha;M->SetVectorParameterValue(TEXT("Tint"),Tint);}C->SetMaterial(I++,M);}}
                const FTransform At=Transform(Object(R,TEXT("transform")));C->SetRelativeTransform(At);C->RegisterComponent();Bounds+=Mesh->GetBoundingBox().TransformBy(At);Parts.Add(C);
            }
            if(!Bounds.IsValid||Parts.IsEmpty())return;
            const FVector Center=Bounds.GetCenter();for(auto* C:Parts)C->SetRelativeLocation(C->GetRelativeLocation()-Center);
            const float Radius=FMath::Max(1.,Bounds.GetExtent().Size());
            for(int I=0;I<2;++I){auto* L=NewObject<UPointLightComponent>(Stage.Get());L->SetupAttachment(Root);L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity((I?20:45)*FMath::Square(Radius/30.f));L->SetAttenuationRadius(Radius*12);L->SetLightColor(I?FLinearColor(.35,.5,.62):FLinearColor(.85,.85,.78));L->SetCastShadows(false);L->SetRelativeLocation(FVector(I?-Radius:Radius,-Radius*2,Radius*2));L->RegisterComponent();}
            Target.Reset(NewObject<UTextureRenderTarget2D>(Stage.Get()));Target->ClearColor=FLinearColor(.006,.009,.012);Target->InitCustomFormat(640,480,PF_B8G8R8A8,false);Target->UpdateResourceImmediate(true);
            Capture=NewObject<USceneCaptureComponent2D>(Stage.Get());Capture->SetupAttachment(Root);Capture->TextureTarget=Target.Get();Capture->CaptureSource=SCS_FinalColorLDR;Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;Capture->FOVAngle=35;
            for(auto* C:Parts)Capture->ShowOnlyComponent(C);
            Capture->ShowFlags.SetLighting(true);Capture->ShowFlags.SetAtmosphere(false);Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetMotionBlur(false);Capture->ShowFlags.SetTemporalAA(false);Capture->ShowFlags.SetBloom(false);
            Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;Capture->PostProcessSettings.AutoExposureMethod=AEM_Manual;Capture->PostProcessSettings.bOverride_AutoExposureBias=true;Capture->PostProcessSettings.AutoExposureBias=0;Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
            const FVector Camera=FVector(Radius*.7,-Radius*3.8,Radius*.65);Capture->SetRelativeLocation(Camera);Capture->SetRelativeRotation((-Camera).Rotation());Capture->RegisterComponent();
            Brush.SetResourceObject(Target.Get());Brush.ImageSize=FVector2D(640,480);ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(&Brush)]];
        }
        virtual void Tick(const FGeometry& G,double T,float D)override{SCompoundWidget::Tick(G,T,D);if(Capture.IsValid()){++Frames;if(Frames==4||Frames==12||Frames==30)Capture->CaptureScene();if(Frames==14&&FParse::Param(FCommandLine::Get(),TEXT("WardInterfaceCheck66"))){TArray<FColor> Pixels;int Bright=0;if(Target.IsValid()&&Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels))for(auto P:Pixels)Bright+=!Pixels.IsEmpty()&&(FMath::Abs(int(P.R)-int(Pixels[0].R))+FMath::Abs(int(P.G)-int(Pixels[0].G))+FMath::Abs(int(P.B)-int(Pixels[0].B)))>35;PreviewBrightPixels66=FMath::Max(PreviewBrightPixels66,Bright);UE_LOG(LogTemp,Display,TEXT("UI66_PREVIEW pixels=%d camera=%s"),Bright,*Capture->GetComponentLocation().ToString());}}}
        virtual ~SWardItemImage(){if(Stage.IsValid())Stage->Destroy();}
    private:TWeakObjectPtr<AActor> Stage;TWeakObjectPtr<USceneCaptureComponent2D> Capture;TStrongObjectPtr<UTextureRenderTarget2D> Target;FSlateBrush Brush;int Frames=0;
    };
    class SWardPersonArchive56 : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SWardPersonArchive56) {}
            SLATE_ARGUMENT(AEWardCampaignMode*, Mode)
            SLATE_ARGUMENT(FString, Id)
            SLATE_ARGUMENT(Json, Card)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args)
        {
            auto* Mode = Args._Mode;
            auto Card = Args._Card;
            const FString Id = Args._Id;
            auto Body = SNew(SVerticalBox);
            Body->AddSlot().AutoHeight()[Label(String(Card, TEXT("name")), 35, Ink)];
            Body->AddSlot().AutoHeight().Padding(0,5,0,18)[Label(String(Card, TEXT("medium")), 19, Muted)];

            auto Columns = SNew(SHorizontalBox);
            auto Portrait = SNew(SVerticalBox);
            auto Image = SNew(SImage).Image(&Brush);
            if (SetupPortrait(Mode, Id))
            {
                Portrait->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(440)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[Image]]];
                Portrait->AddSlot().AutoHeight().Padding(0,10,0,0)[
                    SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(FMargin(16,10))
                    .OnClicked_Lambda([this]{ bCloseUp = !bCloseUp; Frame(); return FReply::Handled(); })
                    [Label(Mode->Tr(TEXT("Лицо / в полный рост")), 18, Ink)]];
            }
            else Portrait->AddSlot().AutoHeight()[Label(Mode->Tr(TEXT("Модель недоступна")), 20, Muted)];
            if (Person.IsValid() && Person->HabitLength55() > 0.f)
                Portrait->AddSlot().AutoHeight().Padding(0,8,0,0)[
                    SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(FMargin(16,10))
                    .OnClicked_Lambda([this]{ bCloseUp = false; Frame(); GestureTime = 0.f; return FReply::Handled(); })
                    [Label(Mode->Tr(TEXT("Показать жест")), 18, Ink)]];
            Columns->AddSlot().FillWidth(.43)[Portrait];

            auto Lines = SNew(SVerticalBox);
            Lines->AddSlot().AutoHeight().Padding(0,0,0,12)[Label(String(Card, TEXT("habit")), 19, Ink)];
            int32 Number = 0;
            for (auto Reply : Array(Card, TEXT("lines")))
            {
                ++Number;
                const FString Clip = FPaths::ProjectContentDir() / TEXT("CampaignData/People56") /
                    (Id + TEXT("_") + FString::FromInt(Number) + TEXT(".wav"));
                Lines->AddSlot().AutoHeight().Padding(0,10,0,6)[Label(Reply->AsString(), 23, Ink)];
                Lines->AddSlot().AutoHeight().Padding(0,0,0,22)[
                    SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(FMargin(16,10))
                    .IsEnabled(Stage.IsValid() && FPaths::FileExists(Clip))
                    .OnClicked_Lambda([this, Clip, Weak=TWeakObjectPtr<AEWardCampaignMode>(Mode)]
                    {
                        if (Voice.IsValid()) Voice->Stop();
                        if (Stage.IsValid() && Weak.IsValid())
                            Voice = WardHomeAssets::PlayDialogueFile(Stage.Get(), Clip, .88f*Weak->Setting(TEXT("effects"), .8f));
                        return FReply::Handled();
                    })
                    [Label(Mode->Tr(TEXT("Прослушать реплику")) + FString::Printf(TEXT("  %02d"), Number), 18, Ink)]];
            }
            Columns->AddSlot().FillWidth(.57).Padding(35,0,0,0)[Lines];
            Body->AddSlot().AutoHeight()[Columns];
            if (Boolean(WardPeople57::Info(Id),TEXT("past57")))
                Body->AddSlot().AutoHeight().Padding(0,18,0,0)[
                    SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(FMargin(16,12))
                    .OnClicked_Lambda([Weak=TWeakObjectPtr<AEWardCampaignMode>(Mode),Id]{
                        if(Weak.IsValid()) Weak->Command(TEXT("people57_replay_")+Id);
                        return FReply::Handled();
                    })[Label(TEXT("Воспроизвести сцену прошлого рядом"),20,Ink)]];
            ChildSlot[SNew(SScrollBox)+SScrollBox::Slot()[Body]];
        }

        void Tick(const FGeometry& Geo, double Time, float Delta) override
        {
            SCompoundWidget::Tick(Geo, Time, Delta);
            if (Capture.IsValid() && Frames++ == 3) Capture->CaptureScene();
            if (GestureTime < 0.f || !Person.IsValid() || !Capture.IsValid()) return;
            GestureTime += Delta;
            GestureStep += Delta;
            if (GestureStep < .067f) return;
            GestureStep = 0.f;
            const float Length = Person->HabitLength55();
            if (GestureTime >= Length)
            {
                GestureTime = -1.f;
                Person->PresentPose(0,0.f);
            }
            else
            {
                const float Blend = FMath::Min(FMath::SmoothStep(0.f,.25f,GestureTime),
                    FMath::SmoothStep(0.f,.28f,Length-GestureTime));
                Person->PresentPose(5,GestureTime,Blend);
            }
            Capture->CaptureScene();
        }

        ~SWardPersonArchive56() override
        {
            if (Voice.IsValid()) Voice->Stop();
            if (Stage.IsValid()) Stage->Destroy();
        }

    private:
        static TSharedRef<SWidget> Label(const FString& Text, int32 Size, FLinearColor Color)
        {
            return SNew(STextBlock).Text(FText::FromString(Text)).AutoWrapText(true).ColorAndOpacity(Color)
                .Font(FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"), Size));
        }

        bool SetupPortrait(AEWardCampaignMode* Mode, const FString& Id)
        {
            Stage = Mode->GetWorld()->SpawnActor<AActor>();
            if (!Stage.IsValid()) return false;
            auto* Root = NewObject<USceneComponent>(Stage.Get());
            Stage->SetRootComponent(Root);
            Root->RegisterComponent();
            Stage->SetActorLocation(FVector(0,0,130000));
            auto* Model = NewObject<UEWardPerson54>(Stage.Get());
            Model->SetIdentity(nullptr, Id, FString());
            Model->SetupAttachment(Root);
            Model->SetVisibleInSceneCaptureOnly(true);
            Model->SetCastShadow(false);
            Model->SetLightingChannels(false,true,false);
            Model->RegisterComponent();
            Model->SetComponentTickEnabled(false);
            if (!Model->LoadAppearance()) return false;
        Person = Model;
            Height = Model->TargetHeight55();
            if (Height <= 0.f) return false;
            // The head crop uses the reference pose to keep the face centered.
            Model->PresentPose(0, 0.f);
            for (int32 Index=0; Index<3; ++Index)
            {
                auto* Light = NewObject<UPointLightComponent>(Stage.Get());
                Light->SetupAttachment(Root);
                Light->SetIntensityUnits(ELightUnits::Lumens);
                Light->SetIntensity(Index==0 ? 360 : Index==1 ? 175 : 100);
                Light->SetLightingChannels(false,true,false);
                Light->SetAttenuationRadius(720);
                Light->SetCastShadows(false);
                Light->SetLightColor(Index==0 ? FLinearColor(.97f,.90f,.82f) :
                    Index==1 ? FLinearColor(.61f,.79f,.91f) : FLinearColor(.36f,.61f,.79f));
                Light->SetRelativeLocation(FVector(Index==2 ? -100 : 160, Index==1 ? 110 : -85, Height+25));
                Light->RegisterComponent();
            }
            Target.Reset(NewObject<UTextureRenderTarget2D>(Stage.Get()));
            Target->InitCustomFormat(512, 608, PF_B8G8R8A8, false);
            Target->UpdateResourceImmediate(true);
            auto* Camera = NewObject<USceneCaptureComponent2D>(Stage.Get());
            Capture = Camera;
            Camera->SetupAttachment(Root);
            Camera->TextureTarget = Target.Get();
            Camera->CaptureSource = SCS_FinalColorLDR;
            Camera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
            Camera->ShowOnlyComponent(Model);
            Camera->bCaptureEveryFrame = false;
            Camera->bCaptureOnMovement = false;
            Camera->FOVAngle = 35;
            Camera->ShowFlags.SetFog(false);
            Camera->ShowFlags.SetAtmosphere(false);
            Camera->ShowFlags.SetMotionBlur(false);
            Camera->ShowFlags.SetTemporalAA(false);
            Camera->ShowFlags.SetBloom(false);
            Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
            Camera->PostProcessSettings.AutoExposureMethod = AEM_Manual;
            Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
            Camera->PostProcessSettings.AutoExposureBias = .65f;
            Camera->RegisterComponent();
            Brush.SetResourceObject(Target.Get());
            Brush.ImageSize = FVector2D(512,608);
            Frame();
            return true;
        }

        void Frame()
        {
            if (!Capture.IsValid()) return;
            const FVector Aim(0,0,Height*(bCloseUp ? .91f : .51f));
            const float Distance = Height*(bCloseUp ? .28f : 1.62f);
            const FVector Camera = Aim + FVector(Distance, Distance*.12f, 2);
            Capture->SetRelativeLocationAndRotation(Camera, (Aim-Camera).Rotation());
            Frames = 0;
        }

        TWeakObjectPtr<AActor> Stage;
        TWeakObjectPtr<USceneCaptureComponent2D> Capture;
        TWeakObjectPtr<UAudioComponent> Voice;
        TWeakObjectPtr<UEWardPerson54> Person;
        float GestureTime = -1.f;
        float GestureStep = 0.f;
        TStrongObjectPtr<UTextureRenderTarget2D> Target;
        FSlateBrush Brush;
        float Height = 0.f;
        int32 Frames = 0;
        bool bCloseUp = true;
    };

    class SWardMapSymbol : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SWardMapSymbol){}SLATE_ARGUMENT(int,Kind)SLATE_END_ARGS()
        void Construct(const FArguments& A){Kind=A._Kind;}
        virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(30,28);}
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& D,int32 L,const FWidgetStyle&,bool)const override
        {
            TArray<FVector2f> P;if(Kind==0)P={{15,5},{6,23},{24,23},{15,5}};else if(Kind==1||Kind==4)P={{15,5},{6,14},{15,23},{24,14},{15,5}};else P={{5,14},{25,14}};
            FSlateDrawElement::MakeLines(D,L,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Kind==1||Kind==3?FLinearColor(.7,.025,.035):Ink,true,2);return L;
        }
    private:int Kind=0;
    };
    class SWardPaper : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SWardPaper){}SLATE_DEFAULT_SLOT(FArguments,Content)SLATE_END_ARGS()
        void Construct(const FArguments& A)
        {
            Texture.Reset(UTexture2D::CreateTransient(512,768,PF_B8G8R8A8));Texture->SRGB=true;Texture->NeverStream=true;auto* Pixels=static_cast<FColor*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));FRandomStream Random(417);
            for(int Y=0;Y<768;++Y)for(int X=0;X<512;++X){const float Edge=FMath::Min(FMath::Min(X,511-X),FMath::Min(Y,767-Y));const float Grain=Random.FRandRange(-8,8),Noise=FMath::PerlinNoise2D(FVector2D(X*.023,Y*.023));const float Stain=FMath::Max(0.f,FMath::PerlinNoise2D(FVector2D(X*.008+12,Y*.008+4))-.1f)*85;const float Wear=FMath::Max(0.f,1-Edge/65)*FMath::Max(.2f,.65f+Noise)*65;const float V=Grain+Noise*12-Stain-Wear;const bool Torn=Edge<2+FMath::Abs(FMath::PerlinNoise2D(FVector2D(X*.12,Y*.12)))*5;Pixels[Y*512+X]=Torn?FColor(7,8,8):FColor(FMath::Clamp(176+V,0.f,255.f),FMath::Clamp(157+V,0.f,255.f),FMath::Clamp(124+V,0.f,255.f));}
            Texture->GetPlatformData()->Mips[0].BulkData.Unlock();Texture->UpdateResource();PaperImage.SetResourceObject(Texture.Get());PaperImage.ImageSize=FVector2D(512,768);ChildSlot.Padding(FMargin(65,65))[A._Content.Widget];
        }
        virtual int32 OnPaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& C,FSlateWindowElementList& D,int32 L,const FWidgetStyle& S,bool E)const override
        {
            
            FSlateDrawElement::MakeBox(D,L,G.ToPaintGeometry(),&PaperImage);
            return SCompoundWidget::OnPaint(A,G,C,D,L+2,S,E);
        }
    private:TStrongObjectPtr<UTexture2D> Texture;FSlateBrush PaperImage;
    };
    bool HasItem(AEWardCampaignMode* G,const FString& Id){for(const auto& E:G->Inventory())if(E.Key==Id)return true;return false;}
    bool LoadPair(AEWardCampaignMode* G,const FString& A,const FString& B)
    {
        if(!HasItem(G,A)||!HasItem(G,B)||!G->Hero())return false;
        if((A==TEXT("weapon")&&B==TEXT("ammo"))||(B==TEXT("weapon")&&A==TEXT("ammo"))){const int N=FMath::Min(9-G->Hero()->Rounds,G->Hero()->Reserve);if(N<=0)return false;G->Hero()->Rounds+=N;G->Consume(TEXT("ammo"),N);G->ReloadTime=0;return true;}
        if((A==TEXT("shotgun")&&B==TEXT("shells"))||(B==TEXT("shotgun")&&A==TEXT("shells"))){const int N=FMath::Min(5-G->LoadedShells,G->Items.FindRef(TEXT("shells")));if(N<=0)return false;G->LoadedShells+=N;G->Consume(TEXT("shells"),N);G->ReloadTime=0;return true;}return false;
    }
}
bool FWardCampaignUI::FieldCommand(const FString& Action)
{
    auto* G=Mode.Get();if(!G)return false;
    const bool Field=G->Screen==TEXT("inventory")||G->Screen==TEXT("journal")||G->Screen==TEXT("map")||G->Screen==TEXT("people56")||G->Screen==TEXT("quests")||G->Screen==TEXT("controls66");
    if(!Field)return false;
    if(Action==TEXT("field_map_book")||Action==TEXT("field_map_current")){BookMapVisible=G->BookWorldEnabled||Action==TEXT("field_map_book");Refresh();return true;}
    if(Action.StartsWith(TEXT("field_atlas_")))
    {
        if(!BookMapState)BookMapState=MakeShared<FWardBookMapState>();const FString Id=Action.RightChop(12);
        if(Id==TEXT("zoom_in"))BookMapState->Zoom=FMath::Clamp(BookMapState->Zoom*1.25,1.,24.);
        else if(Id==TEXT("zoom_out")){BookMapState->Zoom=FMath::Clamp(BookMapState->Zoom*.8,1.,24.);if(BookMapState->Zoom==1)BookMapState->Pan=FVector2D::ZeroVector;}
        else if(Id==TEXT("reset"))BookMapState->Reset();
        else {bool Valid=Id==TEXT("all");for(auto L:Array(WardBookMapData(),TEXT("locations")))Valid|=String(L->AsObject(),TEXT("id"))==Id;if(!Valid)return true;BookMapState->Location=Id;BookMapState->Reset();}
        BookMapVisible=true;Refresh();return true;
    }
    if(Action==TEXT("ui66_quest_map")){G->Show(TEXT("map"));G->Command(TEXT("ui66_map_goal"));return true;}
    if(Action==TEXT("field_close")){CombineItem.Empty();FieldFeedback.Empty();G->Show(TEXT(""));return true;}
    if(Action.StartsWith(TEXT("field_page_"))){CombineItem.Empty();FieldFeedback.Empty();G->Show(Action.RightChop(11));return true;}
    if(Action.StartsWith(TEXT("field_map_"))){BookMapVisible=false;const FString Area=Action.RightChop(10);G->Values.Add(TEXT("map_area"),Area==TEXT("archive")?2:0);G->Values.Add(TEXT("map_floor"),Area==TEXT("basement")?-1:0);Refresh();return true;}
    if(Action==TEXT("field_note_hold")&&G->Screen==TEXT("journal"))
    {
        if(!G->Journal.IsValidIndex(DocumentIndex))return true;
        const FString& Note=G->Journal[DocumentIndex];
        if(!Note.StartsWith(TEXT("@doc:"))&&!Note.StartsWith(TEXT("@blank:")))return true;
        G->HoldPreview51(TEXT("doc:")+Note.RightChop(Note.StartsWith(TEXT("@doc:"))?5:7));
        return true;
    }
    if(Action.StartsWith(TEXT("field_doc_"))){DocumentIndex=FCString::Atoi(*Action.RightChop(10));Refresh();return true;}
    if(G->Screen!=TEXT("inventory"))return false;
    const auto Items=G->Inventory();InventorySlot=FMath::Clamp(InventorySlot,0,FMath::Max(0,Items.Num()-1));const FString Item=Items.IsValidIndex(InventorySlot)?Items[InventorySlot].Key:TEXT("");
    if(Action.StartsWith(TEXT("field_slot_"))){const int Slot=FCString::Atoi(*Action.RightChop(11));if(Items.IsValidIndex(Slot)){InventorySlot=Slot;SelectedItem=Items[Slot].Key;if(!CombineItem.IsEmpty()){const bool Success=LoadPair(G,CombineItem,SelectedItem);FieldFeedback=Success?TEXT("Оружие заряжено."):TEXT("Эти предметы нельзя объединить.");CombineItem.Empty();if(Success)G->Cue(TEXT("reload"),G->Hero()->GetActorLocation());}}Refresh();return true;}
    if(Action==TEXT("field_combine")){if(!Item.IsEmpty()){if(CombineItem.IsEmpty()){CombineItem=Item;FieldFeedback=TEXT("Выбери второй предмет. Повторное нажатие отменяет выбор.");}else {CombineItem.Empty();FieldFeedback.Empty();}}Refresh();return true;}
    if(Action==TEXT("field_use")){if(WardInterface66::CanUse(*G,Item)){if(Item==TEXT("medkit")){G->Show(TEXT(""));G->Heal();return true;}if(Item==TEXT("badge")){SelectedItem=Item;G->Show(TEXT("inspect"));}else if(Item==TEXT("ammo")||Item==TEXT("shells")){LoadPair(G,Item,Item==TEXT("ammo")?TEXT("weapon"):TEXT("shotgun"));Refresh();}else G->Command(TEXT("use_")+Item);}return true;}
    if(Action==TEXT("field_drop")){if(HasItem(G,Item))G->Drop(Item);CombineItem.Empty();Refresh();return true;}
    return false;
}
TSharedRef<SWidget> FWardCampaignUI::FieldMenu()
{
    auto* G=Mode.Get();TitleActions.Reset();TitleMotion.Reset();const FString Font=FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf");
    auto Label=[&](const FString& S,int N=22,FLinearColor C=Ink,bool Wrap=true)->TSharedRef<SWidget>{return SNew(STextBlock).Text(FText::FromString(G->Tr(S))).Font(FSlateFontInfo(Font,N)).ColorAndOpacity(C).AutoWrapText(Wrap);};
    auto Control=[&](const FString& Caption,const FString& Id,bool Enabled=true)->TSharedRef<SWidget>{auto B=SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(FMargin(22,15)).IsEnabled(Enabled).OnClicked_Lambda([Weak=Mode,Id](){if(Weak.IsValid())Weak->Command(Id);return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(G->Tr(Caption))).Font(FSlateFontInfo(Font,22)).ColorAndOpacity(Enabled?Ink:Muted).WrapTextAt(Id.StartsWith(TEXT("field_doc_"))?350.f:0.f)];TitleActions.Add(Id,B);return B;};
    auto Rule=[&]()->TSharedRef<SWidget>{return SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&Line).Padding(0)];};
    auto Body=SNew(SVerticalBox);const bool Inv=G->Screen==TEXT("inventory"),Docs=G->Screen==TEXT("journal"),Archive=G->Screen==TEXT("people56"),Tasks=G->Screen==TEXT("quests"),Help=G->Screen==TEXT("controls66");
    Body->AddSlot().AutoHeight()[Label(Inv?TEXT("ИНВЕНТАРЬ"):Docs?TEXT("ДОКУМЕНТЫ"):Tasks?TEXT("ЗАДАНИЯ"):Help?TEXT("УПРАВЛЕНИЕ"):Archive?TEXT("АРХИВ ГОЛОСОВ"):TEXT("КАРТА"),32,FLinearColor(.82,.83,.79))];Body->AddSlot().AutoHeight().Padding(0,18,0,25)[Rule()];
    auto Nav=SNew(SHorizontalBox);Nav->AddSlot().AutoWidth()[Control(TEXT("Инвентарь  [I]"),TEXT("field_page_inventory"))];Nav->AddSlot().AutoWidth().Padding(8,0)[Control(TEXT("Документы  [J]"),TEXT("field_page_journal"))];Nav->AddSlot().AutoWidth()[Control(TEXT("Карта  [M]"),TEXT("field_page_map"))];Nav->AddSlot().AutoWidth().Padding(8,0)[Control(TEXT("Задания  [O]"),TEXT("field_page_quests"))];Nav->AddSlot().FillWidth(1)[SNullWidget::NullWidget];Nav->AddSlot().AutoWidth()[Control(TEXT("Закрыть  [Esc]"),TEXT("field_close"))];Body->AddSlot().AutoHeight().Padding(0,0,0,Inv||Docs?35:14)[Nav];
    if(Tasks)Body->AddSlot().FillHeight(1)[QuestMenu66()];
    else if(Help)Body->AddSlot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[Label(WardInterface66::Controls(G->Admin),22)]];
    else if(Archive)
    {
        static const Json Cards = Read(FPaths::ProjectContentDir()/TEXT("CampaignData/People56/archive.json"));
        auto Detail = SNew(SBox);
        auto List = SNew(SScrollBox);
        if (Cards)
        {
            TArray<FString> Ids;
            for (const auto& Entry : Cards->Values) Ids.Emplace(*Entry.Key);
            Ids.Sort();
            for (const FString& Id : Ids)
            {
                const Json Card = Object(Cards, Id);
                const FString Caption = String(Card, TEXT("name"));
                List->AddSlot().Padding(0,3)[
                    SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(FMargin(16,12))
                    .OnClicked_Lambda([Weak=Mode, Id, Card, Detail]
                    {
                        if (Weak.IsValid()) Detail->SetContent(SNew(SWardPersonArchive56).Mode(Weak.Get()).Id(Id).Card(Card));
                        return FReply::Handled();
                    })[Label(Caption, 23)]];
            }
            if (!Ids.IsEmpty()) Detail->SetContent(SNew(SWardPersonArchive56).Mode(G).Id(Ids[0]).Card(Object(Cards, Ids[0])));
        }
        Body->AddSlot().FillHeight(1)[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(375)[List]]
            + SHorizontalBox::Slot().FillWidth(1).Padding(35,0,0,0)[Detail]];
    }
    else if(Inv)
    {
        const auto Items=G->Inventory();InventorySlot=FMath::Clamp(InventorySlot,0,FMath::Max(0,Items.Num()-1));SelectedItem=Items.IsValidIndex(InventorySlot)?Items[InventorySlot].Key:TEXT("");
        auto Columns=SNew(SHorizontalBox);auto Grid=SNew(SUniformGridPanel).SlotPadding(FMargin(3));
        for(int I=0;I<12;++I)
        {
            auto Cell=SNew(SOverlay);if(Items.IsValidIndex(I)){Cell->AddSlot().Padding(12)[SNew(SWardItemImage).Mode(G).Item(Items[I].Key)];Cell->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(10)[Label(FString::FromInt(Items[I].Value),20)];}
            auto B=SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(1).OnClicked_Lambda([Weak=Mode,I](){if(Weak.IsValid())Weak->Command(TEXT("field_slot_")+FString::FromInt(I));return FReply::Handled();})[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor_Lambda([this,I](){auto W=TitleActions.FindRef(TEXT("field_slot_")+FString::FromInt(I)).Pin();return W&&W->IsHovered()?FLinearColor(.65,.015,.025):I==InventorySlot?FLinearColor(.25,.28,.26):FLinearColor(.045,.055,.05);}).Padding(1)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor_Lambda([this,I](){auto W=TitleActions.FindRef(TEXT("field_slot_")+FString::FromInt(I)).Pin();return W&&W->IsHovered()?FLinearColor(.06,.003,.005):FLinearColor(.002,.003,.003);}).Padding(0)[Cell]]];TitleActions.Add(TEXT("field_slot_")+FString::FromInt(I),B);Grid->AddSlot(I%4,I/4)[SNew(SBox).WidthOverride(170).HeightOverride(142)[B]];
        }
        auto Left=SNew(SVerticalBox);Left->AddSlot().AutoHeight()[Grid];Left->AddSlot().AutoHeight().Padding(4,18)[Label(FString::Printf(TEXT("%02d / 12"),Items.Num()),18,Muted)];Columns->AddSlot().AutoWidth()[Left];
        auto Detail=SNew(SVerticalBox);if(!SelectedItem.IsEmpty())
        {
            Detail->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(245)[SNew(SWardItemImage).Mode(G).Item(SelectedItem)]];Detail->AddSlot().AutoHeight().Padding(0,20,0,12)[Label(G->ItemName(SelectedItem),30)];Detail->AddSlot().AutoHeight()[Rule()];
            const TMap<FString,FString> Descriptions={{TEXT("weapon"),TEXT("Обычный служебный пистолет. Надёжный, но не очень мощный.")},{TEXT("shotgun"),TEXT("Помповый дробовик. Мощный на близкой дистанции. Магазин на 5 патронов.")},{TEXT("ammo"),TEXT("Патроны для служебного пистолета. Объедини с пистолетом, чтобы зарядить магазин.")},{TEXT("shells"),TEXT("Патроны дробовика. Объедини с дробовиком, чтобы зарядить его.")},{TEXT("medkit"),TEXT("Медицинские принадлежности. Восстанавливают здоровье.")},{TEXT("battery"),TEXT("Запасная батарея. Восстанавливает заряд вспышки.")},{TEXT("badge"),TEXT("Пропуск сотрудника. Осмотри оборот: на нём сохранилась надпись.")},{TEXT("bottle"),TEXT("Пустая бутылка. Брось её клавишей G, чтобы отвлечь противника.")},{TEXT("boards"),TEXT("Доски для укрепления дверей. Подойди к двери и нажми K.")}};
            Detail->AddSlot().AutoHeight().Padding(0,15)[SNew(STextBlock).Text(FText::FromString(G->Tr(WardInterface66::Description(SelectedItem)))).Font(FSlateFontInfo(Font,21)).ColorAndOpacity(Muted).WrapTextAt(850)];
            if(SelectedItem==TEXT("weapon")||SelectedItem==TEXT("shotgun"))Detail->AddSlot().AutoHeight()[Label(FString::Printf(TEXT("%02d / %02d"),SelectedItem==TEXT("weapon")?G->Hero()->Rounds:G->LoadedShells,SelectedItem==TEXT("weapon")?G->Hero()->Reserve:G->Items.FindRef(TEXT("shells"))),26)];
        }else Detail->AddSlot().AutoHeight().Padding(30)[Label(TEXT("Инвентарь пуст"),28)];Columns->AddSlot().FillWidth(1).Padding(45,0,0,0)[SNew(SScrollBox)+SScrollBox::Slot()[Detail]];Body->AddSlot().FillHeight(1)[Columns];
        auto Actions=SNew(SHorizontalBox);const bool Use=WardInterface66::CanUse(*G,SelectedItem);
        Actions->AddSlot().FillWidth(1).Padding(3)[Control(TEXT("Использовать"),TEXT("field_use"),Use)];Actions->AddSlot().FillWidth(1).Padding(3)[Control(CombineItem.IsEmpty()?TEXT("Объединить"):TEXT("Отменить"),TEXT("field_combine"),TArray<FString>{TEXT("weapon"),TEXT("shotgun"),TEXT("ammo"),TEXT("shells")}.Contains(SelectedItem))];Actions->AddSlot().FillWidth(1).Padding(3)[Control(TEXT("Выбросить"),TEXT("field_drop"),TArray<FString>{TEXT("ammo"),TEXT("shells"),TEXT("medkit"),TEXT("battery"),TEXT("bottle"),TEXT("boards")}.Contains(SelectedItem))];Body->AddSlot().AutoHeight()[Actions];
        Body->AddSlot().AutoHeight().Padding(4,12)[Label(FieldFeedback,17,Ink)];
        if(G->HospitalStory){FString Permits;for(const FString Id:{TEXT("visitor_pass"),TEXT("staff_pass"),TEXT("medical_clearance"),TEXT("quarantine_code"),TEXT("evidence_case"),TEXT("NEW_RADIO_01"),TEXT("NEW_LIFT_BELT"),TEXT("NEW_BASELINE_001")})if(G->Completed.Contains(Id))Permits+=(Permits.IsEmpty()?TEXT(""):TEXT(" · "))+String(G->HospitalStory->Definitions.FindRef(Id),TEXT("label"));if(!Permits.IsEmpty())Body->AddSlot().AutoHeight()[Label(TEXT("При себе: ")+Permits,16,Muted)];}
    }
    else if(Docs)
    {
        if(G->HospitalStory){WardNotes65::Sync(*G->HospitalStory);Body->AddSlot().AutoHeight().Padding(0,0,0,12)[Label(WardNotes65::Summary(*G->HospitalStory),17,Muted)];}
        auto Filters=SNew(SHorizontalBox);auto Search=SNew(SEditableTextBox).Text(FText::FromString(DocumentQuery66)).HintText(FText::FromString(TEXT("Название или код документа · Enter"))).OnTextChanged_Lambda([this](const FText& T){DocumentQuery66=T.ToString();}).OnTextCommitted_Lambda([this](const FText&,ETextCommit::Type How){if(How==ETextCommit::OnEnter)Refresh();});TitleActions.Add(TEXT("ui66_doc_search"),Search);Filters->AddSlot().FillWidth(1)[Search];Filters->AddSlot().AutoWidth().Padding(12,0)[Control(UnreadOnly66?TEXT("Непрочитанные"):TEXT("Все найденные"),TEXT("ui66_unread"))];Filters->AddSlot().AutoWidth()[Control(TEXT("Сбросить поиск"),TEXT("ui66_search_clear"))];Body->AddSlot().AutoHeight().Padding(0,0,0,16)[Filters];
        const auto DocIds=WardInterface66::Documents(*G,DocumentQuery66,UnreadOnly66);const int Count=DocIds.Num();if(Count&&!DocIds.Contains(DocumentIndex))DocumentIndex=DocIds[0];auto Columns=SNew(SHorizontalBox);auto List=SNew(SVerticalBox);
        for(int I:DocIds){FString Name=G->Tr(FString::Printf(TEXT("Записка %d"),I+1));const FString& J=G->Journal[I];if(J.Contains(TEXT("Журнал")))Name=G->Tr(TEXT("Журнал дежурств"));else if(J.Contains(TEXT("Пациент")))Name=G->Tr(TEXT("Карта пациента"));else if(J.Contains(TEXT("Ирина")))Name=G->Tr(TEXT("Личный дневник"));if(G->HospitalStory&&(J.StartsWith(TEXT("@blank:"))||J.StartsWith(TEXT("@doc:"))))Name=G->HospitalStory->DocumentTitle(J);List->AddSlot().AutoHeight().Padding(0,3)[Control((I==DocumentIndex?TEXT("|  "):TEXT("   "))+Name,TEXT("field_doc_")+FString::FromInt(I))];}
        List->AddSlot().AutoHeight().Padding(0,12)[Control(TEXT("Архив голосов"),TEXT("field_page_people56"))];
        if(Count&&G->Journal.IsValidIndex(DocumentIndex))
        {
            const FString& Note=G->Journal[DocumentIndex];
            if(Note.StartsWith(TEXT("@doc:"))||Note.StartsWith(TEXT("@blank:")))
                List->AddSlot().AutoHeight().Padding(0,3)[Control(TEXT("Взять записку в руку"),TEXT("field_note_hold"))];
        }
        if(G->Flag(TEXT("map")))List->AddSlot().AutoHeight().Padding(0,3)[Control(TEXT("План отделения"),TEXT("field_page_map"))];
        List->AddSlot().AutoHeight().Padding(0,22)[Control(TEXT("Прослушать запись"),TEXT("replay"),!G->StringFlag(TEXT("last_story")).IsEmpty())];Columns->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(450)[SNew(SScrollBox)+SScrollBox::Slot()[List]]];
        if(Count&&G->HospitalStory)WardNotes65::Read(*G->HospitalStory,G->Journal[DocumentIndex]);
        auto Page=SNew(SVerticalBox);Page->AddSlot().AutoHeight()[Label(FString::Printf(TEXT("ОТДЕЛЕНИЕ 04    /    %02d"),DocumentIndex+1),17,FLinearColor(.025,.017,.009))];if(Count&&G->HospitalStory)Page->AddSlot().AutoHeight().Padding(0,16,0,0)[Label(G->HospitalStory->DocumentTitle(G->Journal[DocumentIndex]),23,FLinearColor(.025,.017,.009))];Page->AddSlot().AutoHeight().Padding(0,20)[SNew(STextBlock).Text(FText::FromString(G->Tr(Count?(G->HospitalStory?G->HospitalStory->DocumentBody(G->Journal[DocumentIndex]):G->Journal[DocumentIndex]):TEXT("Нет документов по выбранному фильтру. Найденные записки появятся здесь после подбора.")))).Font(FSlateFontInfo(Font,21)).ColorAndOpacity(FLinearColor(.016,.009,.004)).WrapTextAt(510)];
        Columns->AddSlot().FillWidth(1).HAlign(HAlign_Center).Padding(80,0,0,15)[SNew(SBox).WidthOverride(650)[SNew(SWardPaper)[SNew(SScrollBox)+SScrollBox::Slot()[Page]]]];Body->AddSlot().FillHeight(1)[Columns];
    }
    else
    {
        auto ModeRow=SNew(SHorizontalBox);ModeRow->AddSlot().AutoWidth()[Control(TEXT("Полный план"),TEXT("field_map_book"))];if(!G->BookWorldEnabled)ModeRow->AddSlot().AutoWidth().Padding(10,0)[Control(TEXT("Текущая игра"),TEXT("field_map_current"))];Body->AddSlot().AutoHeight().Padding(0,0,0,12)[ModeRow];
        if(BookMapVisible)
        {
            if(!BookMapState)BookMapState=MakeShared<FWardBookMapState>();const auto& Locations=Array(WardBookMapData(),TEXT("locations"));
            auto AtlasButton=[&](const FString& Caption,const FString& Id)->TSharedRef<SWidget>{auto B=SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(FMargin(10,7)).OnClicked_Lambda([Weak=Mode,Id](){if(Weak.IsValid())Weak->Command(Id);return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(G->Tr(Caption))).Font(FSlateFontInfo(Font,14)).ColorAndOpacity(Ink).AutoWrapText(false)];TitleActions.Add(Id,B);return B;};
            auto FloorOptions=MakeShared<TArray<TSharedPtr<FString>>>();FloorOptions->Add(MakeShared<FString>(TEXT("all")));for(auto V:Locations)FloorOptions->Add(MakeShared<FString>(String(V->AsObject(),TEXT("id"))));
            auto Caption=[Weak=Mode](const FString& Id){if(Id==TEXT("all"))return Weak.IsValid()?Weak->Tr(TEXT("Все планы")):FString();for(auto V:Array(WardBookMapData(),TEXT("locations")))if(String(V->AsObject(),TEXT("id"))==Id)return Id+TEXT(" / ")+String(V->AsObject(),Weak.IsValid()&&Weak->Language==TEXT("en")?TEXT("name_en"):Weak.IsValid()&&Weak->Language==TEXT("kk")?TEXT("name_kk"):TEXT("name"));return Id;};
            auto Floor=SNew(SComboBox<TSharedPtr<FString>>).OptionsSource(&FloorOptions.Get()).MaxListHeight(340)
                .OnGenerateWidget_Lambda([FloorOptions,Caption,Font](TSharedPtr<FString> Id){return SNew(STextBlock).Text(FText::FromString(Caption(*Id))).Font(FSlateFontInfo(Font,16));})
                .OnSelectionChanged_Lambda([Weak=Mode,FloorOptions](TSharedPtr<FString> Id,ESelectInfo::Type){if(Weak.IsValid()&&Id)Weak->Command(TEXT("field_atlas_")+*Id);})
                [SNew(STextBlock).Text(FText::FromString(Caption(BookMapState->Location))).Font(FSlateFontInfo(Font,18)).ColorAndOpacity(Ink)];
            TitleActions.Add(TEXT("ui66_floor_list"),Floor);Body->AddSlot().AutoHeight().Padding(0,0,0,8)[SNew(SBox).HeightOverride(42)[Floor]];
            Body->AddSlot().AutoHeight().Padding(3,8)[Label(WardInterface66::MapSummary(*G),17,Muted)];
            auto View=MakeWardBookMap(G,BookMapState.ToSharedRef(),[Weak=Mode](const FString& Id){if(Weak.IsValid())Weak->Command(TEXT("field_atlas_")+Id);});TitleActions.Add(TEXT("atlas_view"),View);Body->AddSlot().FillHeight(1)[View];
            auto Tools=SNew(SHorizontalBox);Tools->AddSlot().AutoWidth().Padding(0,0,8,0)[AtlasButton(TEXT("Моё положение"),TEXT("ui66_map_here"))];Tools->AddSlot().AutoWidth().Padding(0,0,8,0)[AtlasButton(TEXT("Текущая цель"),TEXT("ui66_map_goal"))];Tools->AddSlot().AutoWidth()[AtlasButton(TEXT("Масштаб +"),TEXT("field_atlas_zoom_in"))];Tools->AddSlot().AutoWidth().Padding(8,0)[AtlasButton(TEXT("Масштаб -"),TEXT("field_atlas_zoom_out"))];Tools->AddSlot().AutoWidth()[AtlasButton(TEXT("Сбросить вид"),TEXT("field_atlas_reset"))];Body->AddSlot().AutoHeight().Padding(0,8)[Tools];Body->AddSlot().AutoHeight()[Label(TEXT("Треугольник — вы · ромб — текущая цель · колесо — масштаб · перетаскивание — перемещение"),16,Muted)];
        }
        else
        {
        auto Tabs=SNew(SHorizontalBox);Tabs->AddSlot().AutoWidth()[Control(TEXT("Отделение 04"),TEXT("field_map_ward"))];Tabs->AddSlot().AutoWidth().Padding(20,0)[Control(TEXT("Подвал"),TEXT("field_map_basement"))];Tabs->AddSlot().AutoWidth()[Control(TEXT("Архив"),TEXT("field_map_archive"))];Body->AddSlot().AutoHeight()[Tabs];
        if(G->Flag(TEXT("map"))){Body->AddSlot().FillHeight(1).Padding(10,30)[MakeWardCampaignMap(G)];auto Legend=SNew(SHorizontalBox);const TArray<FString> Captions={TEXT("Игрок"),TEXT("Цель"),TEXT("Дверь"),TEXT("Закрыто"),TEXT("Событие")};for(int N=0;N<5;++N){Legend->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SWardMapSymbol).Kind(N)];Legend->AddSlot().AutoWidth().Padding(6,0,45,0)[Label(Captions[N],23)];}Body->AddSlot().AutoHeight().Padding(0,15)[Legend];Body->AddSlot().AutoHeight()[Label(TEXT("Колесо мыши — масштаб · перетаскивание — перемещение"),17,Muted)];}
        else Body->AddSlot().FillHeight(1).VAlign(VAlign_Center)[Label(TEXT("Найди план в приёмной."),28)];
        }
    }
    auto HelpButton=SNew(SButton).ButtonStyle(&FieldStyle).ContentPadding(FMargin(14,5)).OnClicked_Lambda([Weak=Mode](){if(Weak.IsValid())Weak->Command(TEXT("ui66_help"));return FReply::Handled();})[Label(TEXT("Управление  [F1]"),17)];TitleActions.Add(TEXT("ui66_help"),HelpButton);Body->AddSlot().AutoHeight().HAlign(HAlign_Right).Padding(0,12,0,0)[HelpButton];
    return SNew(SBorder).BorderImage(&Black).Padding(0)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080)[SNew(SBorder).BorderImage(&Black).Padding(FMargin(115,65))[Body]]]];
}




#include "WardInterface66.inl"
#include "QuestMenu66.inl"
#include "WardInterfaceAssessment66.inl"
