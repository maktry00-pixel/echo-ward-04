"""Contextual furnishing from the user's 1711-type catalogue.

Type IDs, geometry families and placed instances are counted separately. Existing
architecture and game interactions are retained. No catalogue prose is executed.
"""
import collections, csv, hashlib, json, math, random, re, shutil
from pathlib import Path
from hospital_clutter_v10 import Space, bounds, rotate
from hospital_furnishing_v6 import Index

ROOT=Path(__file__).resolve().parents[1]
PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
OUT=ROOT/'artifacts/unreal_hospital_v12'
INPUT=ROOT/'data/item_types_v12'

# Category profiles describe compatible room functions, not building-wide kits.
AFFINITY={
 1:'палат|бокс|стационар|реанимац|сестры|сестрин',2:'ожидани|семейн|дежур|отдых|вестибюл|приём|кабинет|терап|музык|прачеч|гладиль|кладовая игр',
 3:'склад|кладов|хранени|архив|бельев',4:'осмотр|смотров|врача|процедур|сортиров|фельдшер',
 5:'реанимац|экстренн|анестези|послеоперац',6:'операцион|предоперац|стериль',
 7:'кт-зал|мрт|рентген|узи|экг|ээг|эндоскоп|бронхоскоп|диагност|спирометр|эхокарди',
 8:'процедур|осмотр|смотров|лаборатор|сестры|сестрин|реанимац',9:'процедур|палат|сестры|фельдшер|перевяз',
 10:'стерилиз|автоклав|мойка оборуд|моечн|стериль',11:'лаборатор|пцр|посевн|микробиолог',
 12:'лаборатор|пцр|приём проб|штамм|микробиолог',13:'лаборатор|пцр|реагент|образц',
 14:'аптек|лекарств|препарат',15:'морг|секцион|патолог|приём тел|передаточная тел',
 16:'реабилитац|лфк|физиотерап|массаж|протез|ортез|эрготерап|логопед|трениров|бассейн',
 17:'терапи|психолог|психиатр|сенсор|игр|семь|семейн|тихая палат',
 18:'трудотерап|музыкаль|читаль|клуб|библиотек|комната семьи|групповой терапи',
 19:'комната ирины|палат|комната водителя|семейн|комната семь|гостевая|раздевал|гардероб',
 20:'бель|гардероб|раздевал|палат|прачеч|комната персонала',21:'сануз|душ|ванная|санитарн|санпропуск',
 22:'регист|архив|кабинет|приём|пост|диспетч|бухгалтер|секретариат',23:'кабинет|архив|диспетч|пост|секретариат|радиолюбител',
 24:'сервер|связ|радио|диспетч|мониторинг|pacs',25:'охран|пропуск|вахта|кпп|карантин',
 26:'пожар|аварийн|охран|эвакуац|инженер',27:'кухн|кафе|буфет|столов',28:'кухн|кафе|буфет|моечная посуды',
 29:'столов|кафе|буфет|кухн|комната отдыха',30:'прачеч|бель|сушка|гладиль',31:'убор|грязн|моечн|отход|санитарн',
 32:'мастер|ремонт|инструмент|механик|медтехник',33:'мастер|ремонт|инструмент|сервис|инженер',
 34:'насос|вент|водо|генератор|газов|охлажд|технич|чиллер',35:'электр|энерго|ибп|инвертор|кабел|аккумулятор',
 36:'склад|погруз|разгруз|постав|хранени|тара',37:'супермаркет|магазин|аптек|кафе',
 38:'супермаркет|магазин|продуктовая|кухн|кафе',39:'супермаркет|магазин|продуктовая|кухн',
 40:'супермаркет|магазин|продуктовая|кухн|кафе|столов',41:'супермаркет|магазин|кафе|буфет',
 42:'супермаркет|магазин|убороч|прачеч',43:'супермаркет|магазин|кафе|комната отдыха|служебн',
 44:'гараж|ремонтный бокс|колёс|механик|автопарк|водител|парковк|скорой',
 45:'сад|наружного инвентар|разгруз|парков|территор|бесед|садовник',
 46:'зимний сад|бесед|садовник|семейн|зимняя галерея',49:'гелиос|штамм|сдержив|крио|ядро|инкубатор',
 50:'гелиос|штамм|сдержив|крио|ядро|карантин|патолог',
}

# Exact correspondences to the previous numbered prototype schedule. A family
# binding is deliberately not advertised as a newly made unique mesh.
EXISTING={1:1,2:2,3:3,7:1,10:4,12:43,13:41,14:38,19:36,21:34,23:35,32:31,
 33:16,34:17,35:18,36:19,39:20,40:20,41:22,42:21,43:23,45:24,
 46:25,48:26,49:27,50:28,51:30,52:29,53:29,63:5,64:5,65:6,66:7,67:8,
 68:9,70:10,71:11,72:12,73:13,74:14,76:15,77:15,90:4,105:45,
 112:35,113:35,123:42,124:44,128:46,145:38,156:32,159:45,192:37,
 194:47,195:48,199:49,200:31,201:49,207:50,210:51,212:53,213:52,215:54,218:55,
 296:58,314:5,329:59,330:59,342:61,411:5,412:5,447:38,451:33,452:62,454:34,
 468:112,472:70,473:74,485:57,490:31,491:31,511:74,518:76,
 688:112,689:113,690:114,693:115,694:115,697:117,746:79,747:80,755:83,786:86,
 793:87,797:88,806:91,814:89,815:44,816:84,817:84,819:85,824:85,826:139,849:9,
 875:122,883:123,886:124,889:39,892:126,979:128,980:129,982:130,983:130,
 999:39,1027:120,1082:107,1131:94,1133:95,1134:95,1135:95,1141:None,
 1145:96,1146:99,1149:98,1152:100,1172:92,1173:92,1184:93,1194:177,1196:178,
 1197:178,1199:146,1200:147,1211:108,1212:108,1214:109,1215:109,1220:110,
 1240:132,1250:133,1254:125,1490:135,1491:137,1492:136,1493:138,1494:138,
 1520:141,1521:140,1522:140,1523:140,1528:145,1530:144,1531:143,1532:143,1533:120,
 1538:148,1539:148,1544:155,1545:156,1548:158,1558:154,1564:149,1572:150,
 1577:151,1585:152,1683:63,1684:186,1685:187,1686:185,
}

def read_catalog():
    content=(INPUT/'EHO_full_item_types_catalog.md').read_text(encoding='utf-8-sig')
    result=[];cat=0;title=''
    for line in content.splitlines():
        if line.startswith('### '):
            cat+=1;title=line[4:].split(' — ')[0]
        m=re.match(r'- \*\*(T\d{4})\*\* — (.+)',line)
        if m:result.append(dict(id=m[1],number=int(m[1][1:]),name=m[2].rstrip('.'),category=cat,category_name=title))
    assert len(result)==1711 and len({a['id'] for a in result})==1711 and cat==50
    return result

def family(a):
    n=a['number'];s=a['name'].lower();c=a['category']
    # These construction components belong to the architectural system. Making
    # a second pile of arbitrary doors/stairs would damage the playable layout.
    if c in (47,48):return 'construction', 'reference'
    if n in EXISTING and EXISTING[n] is not None:return 'existing', 'reference'
    for pattern,fam,mount in [
        ('микроскоп','microscope','surface'),('центрифуг|пцр-амп|спектрофотометр|анализатор','analyzer','surface'),
        ('швейная машина','sewing','surface'),('шахматная доска|доска настольной игры','chess','surface'),
        ('шахматная фигура','pawn','surface'),('гитара','guitar','floor'),('пианино|ксилофон','keyboard_instrument','surface'),
        ('мольберт','easel','floor'),('гантел','hammer_not_used','surface'),
        ('стетоскоп','stethoscope','surface'),('шприц','syringe','surface'),
        ('пинцет|щипц|ножниц|зажим крово|пассатиж|клещ|кусач|иглодержател','forceps','surface'),
        ('скальпел|лезви|нож |нож$|ножов|пила |пила$','blade','surface'),
        ('молот|киянк|кувалд','hammer','surface'),('отвёрт|карандаш|ручка |кисть|мелок|стамеск|паяльник|надфил|шпател|шпильк','pen','surface'),
        ('ключ|рожок|трубный ключ','key','surface'),('шестерн|кольцо|гайка|шайба|подшипник|рулон|катушка|скотч|лента','ring','surface'),
        ('очки|бинокл','glasses','surface'),('мяч|яблоко|апельсин|персик|абрикос|слива|киви|помидор|картоф|луковиц|чеснок|яйцо|дыня|арбуз|лимон|мандари|тыква','produce','surface'),
        ('банан|морковь|огурец|кабачок|баклажан|багет|батон|колбас|сосиск','long_produce','surface'),
        ('листовой салат|брокколи|капуст|пучок зелени|виноград','leaf_produce','surface'),
        ('хлеб|булоч|круассан|лепёш|корка|пирож|пончик|кекс|котлета|печенье|пряник','bread','surface'),
        ('молок|сок|сливок|пачка |пакет |упаковка |коробка |тюбик|туба |шоколад|плитка|батончик','package','surface'),
        ('флакон|бутыл|баночка|ампула|пробирк|цилиндр|баллон|термос|банка |ингалятор','bottle','surface'),
        ('колба|воронка','flask','surface'),('чашка петри|тарел|блюдц|поднос|лоток|поддон|доска раздел','tray','surface'),
        ('кастрюл|сковород|сотейник|ковш|миска|пиала|салатник|таз|ступка','bowl','surface'),
        ('кружк|чашка|стакан|кувшин|чайник|кофемашин|кофемол','cup','surface'),
        ('лист|бланк|история болез|анкета|записк|письмо|газета|открытк|фото|чек |бирка|этикет|инструкц|банкнот|талон','paper','surface'),
        ('журнал|книга|тетрад|дневник|блокнот|папка|альбом|паспорт|скоросшив','book','surface'),
        ('карта|карточк|браслет|пропуск|удостоверен|бейдж|флешка|носитель памяти','card','surface'),
        ('монитор|телевизор|ноутбук|пульт|анализатор|осцилл|планшетный|сканер','screen','surface'),
        ('клавиатур|калькулятор|терминал оплаты','keypad','surface'),
        ('телефон|раци|диктофон|магнитофон|факс|фотоаппарат|видеорегистратор','electronics','surface'),
        ('сумк|рюкзак|чемодан|косметич|контейнер|ящик инструмента','case','surface'),
        ('простын|подуш|одеяло|полотенц|плед|матрас|салфет|ткан|пелёнк|скатерт|халат|пижама|рубашк|свитер|брюк|носк|футбол|куртк|шапк|перчат|бахил|фартук|маска|комбинезон','textile','surface'),
        ('ботинок|тапок|кроссовок|обувь|сапог','shoe','surface'),
        ('шланг|кабел|шнур|трос|верёвк|провод|эндоскоп|катетер|трубка|зонд|контур','coil','surface'),
        ('принтер|микроволн|тостер|миксер|весы|насос|аппарат|блендер|дрель|перфоратор|шлифмаш|электролоб|гайковёрт|мясорубк','appliance','surface'),
        ('кукла|игрушка|манекен|скелет','doll','surface'),('горшок|фикус|монстер|алоэ|кактус|папорот|герань|хлорофит|сансев|цвет','plant','surface'),
        ('кубик|домино|шашка|пазл|фишка|блок|конструктор|головолом','game','surface'),
    ]:
        if re.search(pattern,s):return ('dumbbell' if fam=='hammer_not_used' else fam),mount
    if c==46:return 'plant','surface'
    if c in (11,13,27,34,35,36):return 'appliance','surface'
    if c==50:return 'organic','surface'
    if c in (8,9,14,20,30):return 'package','surface'
    if c in (32,33):return 'tool','surface'
    if c in (38,40,41):return 'produce','surface'
    if c in (22,17,18):return 'card','surface'
    return 'small_device','surface'

def specialty(number):
    if 564<=number<=571:return 'музыкаль|клуб'
    if 545<=number<=563:return 'трудотерап|клуб|групповой терапи'
    if 572<=number<=577:return 'читаль|библиотек|семь|семейн'
    if 198<=number<=198:return 'мрт'
    if 203<=number<=206:return 'рентген|описания|изображений'
    if 208<=number<=209:return 'узи|эхокарди'
    if 211<=number<=212:return 'экг|холтер'
    if number==214:return 'ээг|невролог'
    if 219<=number<=220:return 'эндоскоп|бронхоскоп'
    if 500<=number<=504:return 'протез|ортез'
    if 505<=number<=507:return 'логопед|реабилитац'
    if 508<=number<=511:return 'бассейн|инвентаря'
    if 979<=number<=1007:return 'прачеч|бель|сушка|гладиль'
    return None

def shape(fam,n):
    p=[]
    def b(x,y,z,w,d,h,m='v12_steel'):p.append([x,y,z,w,d,h,m])
    def base(w,d,h,m='v12_ivory'):b(0,0,h/2,w,d,h,m)
    def rim(w,d,z,h=.025,m='v12_steel'):
        for x in (-w/2,w/2):b(x,0,z,.012,d,h,m)
        for y in (-d/2,d/2):b(0,y,z,w,.012,h,m)
    def digits(x,y,z,w):
        for a in range(3):b(x+(a-1)*w/4,y,z,w/6,.003,.006,'v12_ink')
    def face(w,d,z):
        b(0,d/2+.002,z,w*.7,.004,w*.32,'v12_teal');digits(0,d/2+.005,z,w)
    if fam=='stand':
        for x in (-.31,.31):
            for y in (-.21,.21):b(x,y,.345,.035,.035,.69)
        b(0,0,.7025,.70,.50,.035,'v12_wood')
    elif fam=='microscope':
        base(.17,.23,.025);b(0,-.055,.14,.035,.04,.25);b(0,0,.115,.16,.12,.015);b(0,.01,.22,.05,.11,.07,'v12_ink');b(0,.045,.18,.024,.024,.06);b(0,-.06,.285,.045,.045,.07,'v12_ink');b(.055,-.05,.15,.045,.035,.035,'v12_ink')
    elif fam=='analyzer':
        base(.34,.28,.18);b(0,-.04,.20,.28,.16,.035,'v12_teal');b(0,.105,.19,.14,.07,.03,'v12_ink');face(.34,.28,.09)
        for x in (-.10,-.03,.04,.11):b(x,-.06,.227,.025,.025,.018,'v12_cap_blue')
    elif fam=='sewing':
        base(.36,.19,.025);b(-.12,0,.13,.07,.14,.22);b(0,0,.23,.29,.12,.065);b(.12,0,.17,.035,.07,.09);b(.12,.01,.095,.006,.006,.07);b(-.12,-.04,.30,.01,.01,.06);b(-.12,-.04,.28,.045,.045,.035,'v12_thread')
    elif fam=='chess':
        base(.32,.32,.013,'v12_wood')
        for x in range(8):
            for y in range(8):b((x-3.5)*.037,(y-3.5)*.037,.014,.036,.036,.004,'v12_ivory' if (x+y)%2 else 'v12_ink')
        for x,y in ((1,1),(3,2),(5,6),(6,4),(4,5)):
            b((x-3.5)*.037,(y-3.5)*.037,.026,.018,.018,.023,'v12_wood' if y<4 else 'v12_ivory')
    elif fam=='guitar':
        b(0,0,.33,.31,.13,.38,'v12_wood');b(0,0,.57,.23,.12,.16,'v12_wood');b(0,0,.89,.065,.085,.60,'v12_wood');b(0,.07,.42,.09,.007,.09,'v12_ink');b(0,0,1.21,.095,.08,.13,'v12_wood')
        for x in (-.016,0,.016):b(x,.075,.80,.002,.002,.90,'v12_steel')
    elif fam=='easel':
        for x in (-.25,.25):b(x,0,.65,.04,.10,1.3,'v12_wood')
        b(0,0,.65,.60,.15,.05,'v12_wood');b(0,0,1.03,.52,.04,.65,'v12_canvas');b(0,.028,1.03,.13,.006,.30,'v12_ochre');b(.10,.028,1.19,.16,.006,.07,'v12_teal')
    elif fam=='keyboard_instrument':
        base(.55,.20,.05,'v12_wood')
        for x in range(16):b((x-7.5)*.032,0,.065,.03,.16,.03,'v12_ivory')
        for x in range(10):b((x-4.5)*.045,-.035,.09,.018,.09,.025,'v12_ink')
    elif fam in ('pen','tool','blade','key','hammer','forceps','dumbbell'):
        base(.018,.14,.016,'v12_ink' if fam in ('pen','hammer') else 'v12_steel')
        if fam=='pen':b(0,-.09,.008,.005,.04,.006,'v12_steel');b(0,.035,.02,.016,.045,.008,'v12_steel')
        elif fam=='hammer':b(0,-.072,.018,.095,.035,.035)
        elif fam=='dumbbell':
            for y in (-.09,.09):b(0,y,.026,.085,.045,.052,'v12_ink')
        elif fam=='blade':b(0,-.12,.008,.038,.12,.007)
        elif fam=='key':rim(.05,.04,.012,.02);b(.02,.095,.01,.03,.012,.02)
        elif fam=='forceps':
            for x in (-.023,.023):b(x,-.03,.007,.008,.15,.012)
    elif fam in ('bottle','flask','cup','bowl','ring'):
        w,d,h=(.055,.055,.13) if fam=='bottle' else (.09,.09,.12) if fam=='flask' else (.085,.085,.10) if fam=='cup' else (.18,.18,.08) if fam=='bowl' else (.065,.065,.025)
        base(w,d,.009,'v12_glass' if fam in ('bottle','flask') else 'v12_ivory')
        rim(w-.01,d-.01,h/2,h,'v12_glass' if fam in ('bottle','flask') else 'v12_ivory')
        if fam in ('bottle','flask'):
            b(0,0,h,w*.65,d*.65,.025,'v12_cap_red' if n%3 else 'v12_cap_blue');b(0,d/2+.002,h*.45,w*.65,.003,h*.30,'v12_label')
        elif fam=='cup':b(w*.7,0,h*.6,.04,.02,h*.55,'v12_ivory')
    elif fam in ('package','case','book','paper','card','textile','tray','game','pawn','small_device'):
        w,d,h={'package':(.10,.075,.15),'case':(.26,.18,.12),'book':(.14,.20,.023),'paper':(.15,.21,.003),'card':(.07,.095,.003),'textile':(.19,.23,.035),'tray':(.23,.17,.012),'game':(.055,.055,.035),'pawn':(.025,.025,.05),'small_device':(.07,.10,.035)}[fam]
        mat={'case':'v12_canvas','book':'v12_book','paper':'v12_label','card':'v12_label','textile':'v12_thread','tray':'v12_steel','pawn':'v12_ivory','game':'v12_wood'}.get(fam,'v12_package_'+str(n%6))
        base(w,d,h,mat)
        if fam in ('paper','card','book'):
            for y in (-d*.23,0,d*.23):b(0,y,h+.001,w*.65,.003,.002,'v12_ink')
            if n%3==0:b(-w*.22,-d*.25,h+.003,w*.20,d*.20,.002,'v12_teal')
        elif fam=='case':rim(w*.4,d*.45,h+.008,.012,'v12_ink')
        elif fam=='package':
            b(0,d/2+.003,h*.5,w*.83,.004,h*.48,'v12_label');digits(0,d/2+.006,h*.5,w)
        elif fam=='tray':rim(w,d,.025)
        elif fam=='textile':
            for x in (-.065,.065):b(x,0,h+.002,.008,d*.95,.004,'v12_ivory')
        elif fam=='game':
            for x in (-.012,.012):b(x,-.012,h+.002,.007,.007,.004,'v12_ink')
        elif fam=='small_device':face(w,d,h*.5)
    elif fam in ('screen','electronics','keypad','appliance'):
        w,d,h={'screen':(.25,.12,.24),'electronics':(.14,.17,.055),'keypad':(.20,.10,.018),'appliance':(.24,.22,.18)}[fam]
        if fam=='screen':
            base(w,.11,.02);b(0,0,.10,.035,.035,.18);b(0,0,.24,w,.04,.17,'v12_ink');b(0,.023,.24,w*.86,.006,.12,'v12_teal')
        else:
            base(w,d,h,'v12_ink' if fam in ('electronics','keypad') else 'v12_ivory');face(w,d,h*.55)
            if fam=='keypad':
                for x in range(6):
                    for y in range(3):b((x-2.5)*.027,(y-1)*.025,.022,.018,.016,.006,'v12_ivory')
            if fam=='appliance':
                for x in (-.07,0,.07):b(x,-.112,.07,.025,.003,.07,'v12_ink')
    elif fam in ('produce','long_produce','leaf_produce','bread'):
        w,d,h=(.08,.075,.075) if fam=='produce' else (.055,.18,.05) if fam=='long_produce' else (.12,.12,.07) if fam=='leaf_produce' else (.11,.15,.05)
        base(w*.8,d*.8,h,'v12_food_'+str(n%5) if fam!='bread' else 'v12_bread');b(0,0,h*.5,w,d,h*.45,'v12_food_'+str(n%5) if fam!='bread' else 'v12_bread')
        if fam=='produce':b(0,0,h+.008,.009,.009,.016,'v12_wood')
        if fam=='leaf_produce':
            for x,y in ((-.05,0),(.05,0),(0,-.05),(0,.05)):b(x,y,h*.55,.045,.045,.025,'v12_leaf')
    elif fam in ('coil','stethoscope'):
        rim(.13,.15,.008,.014,'v12_ink');b(.09,0,.008,.06,.012,.014,'v12_ink');b(.12,0,.012,.035,.035,.02,'v12_steel')
    elif fam=='syringe':
        base(.015,.08,.015,'v12_ivory');b(0,-.06,.008,.004,.04,.005);b(0,.06,.008,.008,.035,.008,'v12_teal');b(0,.08,.008,.033,.006,.01)
    elif fam=='glasses':
        for x in (-.038,.038):
            b(x,0,.008,.056,.045,.004,'v12_glass');b(x,.035,.012,.005,.06,.01,'v12_ink')
        b(0,0,.009,.03,.008,.008,'v12_ink')
    elif fam=='shoe':base(.07,.16,.025,'v12_ink');b(0,-.025,.055,.065,.10,.06,'v12_canvas')
    elif fam=='plant':
        base(.095,.095,.10,'v12_terracotta');b(0,0,.20,.012,.012,.22,'v12_leaf')
        for x,y,z in ((-.065,0,.20),(.065,0,.24),(0,-.055,.27),(0,.055,.29)):
            b(x,y,z,.09,.055,.022,'v12_leaf');b(x*.6,y*.6,z-.015,.012,.012,.07,'v12_leaf')
    elif fam=='doll':
        b(0,0,.12,.065,.045,.12,'v12_canvas');b(0,0,.21,.06,.05,.06,'v12_ivory')
        for x in (-.06,.06):b(x,0,.14,.055,.035,.04,'v12_canvas')
        for x in (-.02,.02):b(x,0,.045,.025,.04,.07,'v12_wood')
    elif fam=='organic':
        base(.16,.14,.10,'v12_organic')
        for x in (-.06,0,.06):b(x,0,.10,.04,.12,.07,'v12_organic')
    else:raise ValueError(fam)
    # Keep every mesh resting exactly on its support (including low rings).
    low=min(a[2]-a[5]/2 for a in p)
    for a in p:a[2]-=low
    return p

def apply_item_types(d):
    OUT.mkdir(parents=True,exist_ok=True)
    if not (OUT/'baseline_v11.json').exists():
        assert d['review_revision']==11,'A v11 baseline is required for conservation checks'
        (OUT/'baseline_v11.json').write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    rows=read_catalog();e=d['environment_dressing'];e['boxes']=[b for b in e['boxes'] if b.get('layer')!=12]
    d['collision_features']=[f for f in d['collision_features'] if f.get('source')!='item_catalog_v12']
    space=Space();wall=Index();reserved=Index();floor=Index()
    furniture=d['prototype_props']+d['environment_props'];catalog=d['prototype_catalog']
    zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
    for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):
        wall.add(w['z'],w['rect']);floor.add(w['z'],w['rect'])
    # Preserve all existing physical routes, objects and meshes.
    for f in d['collision_features']:floor.add(f['z'],f['rect'])
    for p in furniture:
        for a in catalog[str(p['model'])]['parts']:
            off=rotate(a[:3],p['turn']);at=[p['at'][i]+off[i] for i in range(3)];sz=list(a[3:6])
            if p['turn']%2:sz[0],sz[1]=sz[1],sz[0]
            space.add(bounds(at,sz),p['id'])
    for j,b in enumerate(e['boxes']):
        if b['kind'] not in ('lawn','grass_tuft','pavement','road_dash','tree_crown'):
            space.add(bounds(b['at'],b['size']),b.get('item_id','OLD12_'+str(j)))
    # Reserve full player-width sweeps, rather than just path centre points.
    def reserve(z,a,b,r=.62):reserved.add(z,[min(a[0],b[0])-r,min(a[1],b[1])-r,abs(a[0]-b[0])+2*r,abs(a[1]-b[1])+2*r])
    for route in d['navigation_routes']+d['bay_navigation_routes']:
        for a,b in zip(route['points'],route['points'][1:]):
            if a[2]==b[2]:reserve(a[2],a,b)
    for s,r in zones.values():
        ox,oy,z=s['origin'];x,y,w,h=r['rect']
        for door in [dict(at=r['door'],side=r['door_side'])]+r.get('extra_doors',[]):
            xx,yy=ox+door['at'][0],oy+door['at'][1];ns=door['side'] in ('north','south')
            reserve(z,[xx if ns else xx-1.8,yy-1.8 if ns else yy],[xx if ns else xx+1.8,yy+1.8 if ns else yy],.65)
        if r['enclosed']:reserve(z,[ox+r['door'][0],oy+r['door'][1]],[ox+x+w/2,oy+y+h/2],.68)
    for q in d['hideouts']:
        x,y,z=q['at'];w,dep,_=q['size'];reserved.add(z,[x-.2,y-.2,w+.4,dep+1.8])
    for q in d['escape_niches']:
        x,y,z=q['entry'];reserved.add(z,q['rect']);reserve(z,q.get('corridor_point',[x,y,z]),[x,y],.7)
    for q in d['story_gameplay']['placements']+d['story_gameplay']['mobs']:
        x,y,z=q['at'];radius=1.1 if q.get('kind') in ('person','scene') or 'species' in q else .7
        reserved.add(z,[x-radius,y-radius,radius*2,radius*2])
    for q in d['stairs']:
        x,y,w,h=q['rect'];z=q['z_min']
        while z<q['z_max']+.01:reserved.add(round(z,2),[x-.6,y-.6,w+1.2,h+1.2]);z+=4.8
    for z in (-9.6,-4.8,0,4.8,9.6,14.4):reserved.add(z,[47.2,3.2,5.6,9.6])

    materials={'ivory':[.66,.69,.62],'ink':[.023,.031,.035],'steel':[.27,.34,.35],'glass':[.12,.30,.31],
      'teal':[.07,.29,.29],'wood':[.28,.14,.06],'canvas':[.17,.23,.21],'thread':[.36,.42,.35],
      'label':[.73,.70,.59],'book':[.16,.22,.25],'ochre':[.54,.31,.08],'cap_red':[.44,.04,.025],
      'cap_blue':[.06,.18,.34],'terracotta':[.35,.16,.08],'leaf':[.08,.20,.06],'bread':[.49,.29,.10],
      'organic':[.22,.035,.028]}
    for i,c in enumerate(([.50,.34,.14],[.22,.36,.33],[.61,.54,.32],[.37,.15,.10],[.18,.24,.37],[.57,.57,.50])):materials['package_'+str(i)]=c
    for i,c in enumerate(([.51,.09,.035],[.12,.27,.055],[.59,.39,.055],[.39,.20,.075],[.45,.36,.20])):materials['food_'+str(i)]=c
    for k,tint in materials.items():e['materials']['v12_'+k]=dict(tint=tint,roughness=.48 if k=='glass' else .82,weathering=.24,metallic=.5 if k=='steel' else 0)

    # A surface includes its actual thickness and orientation. Shelves are not
    # treated as a single invisible tabletop at an arbitrary cabinet height.
    supports=[]
    top_models={4,16,17,18,19,20,21,22,23,24,37,38,39,107,112,130,132}
    for p in furniture:
        if p['mount']!='floor' or p['zone'] not in zones:continue
        c=catalog[str(p['model'])];bb=c['bounds'];w,h=bb[3]-bb[0],bb[4]-bb[1]
        if p['turn']%2:w,h=h,w
        x,y,z=p['at'];heights=[.5725,1.0025,1.4325] if p['model'] in (13,14,15) else [c['top']] if p['model'] in top_models else []
        for zz in heights:
            supports.append(dict(parent=p['id'],zone=p['zone'],site=p['site'],turn=p['turn'],model=p['model'],rect=[x-w/2+.035,y-h/2+.035,w-.07,h-.07],z=z+zz,used=0))
    rng=random.Random(120023);rng.shuffle(supports)
    eligible={}
    for category,pattern in AFFINITY.items():
        eligible[category]=[q for q in supports if re.search(pattern,zones[q['zone']][1]['name'].lower())]
    items=[];type_data={};newfloors=[]
    counts=collections.Counter();zonecounts=collections.Counter();capture_candidates=[]
    def put(a,parts,q,base,turn,mount):
        boxes=[]
        for part in parts:
            off=rotate(part[:3],turn);at=[base[i]+off[i] for i in range(3)];sz=list(part[3:6])
            if turn%2:sz[0],sz[1]=sz[1],sz[0]
            bb=bounds(at,sz)
            if space.hits(bb):return False
            boxes.append(dict(at=[round(v,6) for v in at],size=[round(v,6) for v in sz],material=part[6],kind='catalog_'+a['id'],zone=q['zone'],solid=False,layer=12))
        bb=[min(b['at'][i]-b['size'][i]/2 for b in boxes) for i in range(3)]+[max(b['at'][i]+b['size'][i]/2 for b in boxes) for i in range(3)]
        if mount=='surface':
            x,y,w,h=q['rect']
            if bb[0]<x or bb[1]<y or bb[3]>x+w or bb[4]>y+h or abs(bb[2]-q['z']-.004)>.00001:return False
        ident=f'CAT12_{a["id"]}_{len(items):05}'
        for b in boxes:b['item_id']=ident;e['boxes'].append(b)
        item=dict(id=ident,type=a['id'],family=type_data[a['id']]['family'],category=a['category'],zone=q['zone'],site=q['site'],parent=q.get('parent'),mount=mount,base=base,turn=turn,bounds=bb,parts=len(boxes),support=dict(rect=q['rect'],z=q['z']) if mount=='surface' else None)
        items.append(item);space.add(bb,ident);counts[a['id']]+=1;zonecounts[q['zone']]+=1
        if mount=='surface':q['used']+=1
        capture_candidates.append(item);return True

    bymodel=collections.defaultdict(list)
    for p in furniture:bymodel[p['model']].append(p['id'])
    # Six catalog pages' worth of construction are tracked honestly as pending
    # art, rather than claimed as new playable or destructible mechanics.
    for a in rows:
        fam,mount=family(a);n=a['number'];spec=dict(a,family=fam,mount=mount,status='not_placed',instances=[],existing_instances=[])
        type_data[a['id']]=spec
        if fam=='construction':spec['status']='architectural_art_backlog';continue
        if fam=='existing':
            spec['base_model']=EXISTING[n];spec['existing_instances']=bymodel.get(EXISTING[n],[])
            spec['status']='existing_family_prototype' if spec['existing_instances'] else 'not_placed';continue
        # Keep furniture-size proposals out of the small-item pass. A future
        # full-size mesh is preferable to shrinking a gantry, mattress or desk.
        oversized=(a['category'] in (1,2,3) and n not in (30,60,62,81,84,89)) or n in {
            98,99,110,111,114,115,125,134,135,147,148,149,150,151,152,153,155,
            157,158,160,164,198,202,222,225,297,298,299,310,317,326,327,328,
            331,341,343,344,345,347,416,417,453,455,457,466,469,470,
            474,475,480,486,488,489,492,508,509,510,520,521,522,538,539,
            558,570,613,695,696,759,760,784,787,794,800,807,808,809,825,833,837,839,
            853,864,866,868,869,871,873,874,876,877,878,879,880,884,885,887,890,891,
            900,901,904,905,906,907,908,981,985,997,998,1000,1005,1007,1008,1012,
            1013,1015,1017,1019,1023,1024,1025,1026,1029,1036,1039,1132,1137,1138,
            1139,1140,1142,1143,1144,1147,1148,1150,1153,1171,1174,1198,1209,1210,
            1217,1218,1219,1221,1222,1223,1224,1225,1233,1234,1239,
            1251,1253,1255,1256,1257,1258,1261,1269,1270,1495,1496,1502,1503,
            1524,1525,1526,1527,1529,1534,1535,1536,1537,1540,1543,1547,1557,
            1559,1564,1565,1566,1567,1568,1569,1570,1571,1572,1573,1574,1575,1576,
            1577,1578,1579,1596,1597,1677,1678,1680,1685,1691,1694,1697,1699,1700,
            1701,1702,1703,1704,1705,1706,1707,1709,1710,1711}
        if oversized:spec['status']='full_size_art_backlog';continue
        parts=shape(fam,n);spec['parts']=parts
        # Oversize equipment is handled by existing bounded furniture, not tiny
        # toy versions masquerading as full-size appliances. Small accessories
        # are appropriate reusable prototypes from the catalogue.
        if any(w in a['name'].lower() for w in ('промышленн','камера продуктов','погрузчик','автомобильный подъёмник','реанимационная доска','спинальный щит','носилки','транспортная тележка','напольная','котёл отопления','чиллер','вентиляционная установка','главный распределительный','трансформатор','холодильная','генератор','штабелёр','навес','постовая будка','бака','люк','модуль стены')):
            spec['status']='full_size_art_backlog';continue
        if fam in ('guitar','easel'):
            # Floor accents have actual colliders. Reserve routes and hideout
            # sweeps; add a matching data obstacle for navigation checks.
            matched=[(s,r) for s,r in zones.values() if re.search('музыкаль' if fam=='guitar' else 'трудотерап',r['name'].lower())]
            for s,r in matched:
                ox,oy,z=s['origin'];x,y,w,h=r['rect'];placed=False
                for t in (0,1):
                    pb=[min(v[i]-v[i+3]/2 for v in parts) for i in range(3)]+[max(v[i]+v[i+3]/2 for v in parts) for i in range(3)]
                    fw,fd=pb[3]-pb[0],pb[4]-pb[1]
                    if t%2:fw,fd=fd,fw
                    for xx in (ox+x+fw/2+.25,ox+x+w-fw/2-.25):
                        for yy in (oy+y+fd/2+.25,oy+y+h-fd/2-.25):
                            rr=[xx-fw/2,yy-fd/2,fw,fd]
                            if floor.hits(z,rr,.08) or reserved.hits(z,rr,.05):continue
                            q=dict(zone=r['id'],site=s['id'])
                            if put(a,parts,q,[xx,yy,z],t,'floor'):
                                item=items[-1];bb=item['bounds'];feature=dict(id=item['id'],rect=[bb[0],bb[1],bb[3]-bb[0],bb[4]-bb[1]],z=z,source='item_catalog_v12')
                                d['collision_features'].append(feature);newfloors.append(feature);floor.add(z,feature['rect'])
                                # All visible floor parts share the footprint;
                                # physical boxes exactly match the render.
                                for b in e['boxes'][-item['parts']:]:b['solid']=True
                                placed=True;break
                        if placed:break
                    if placed:break
            continue
        pool=eligible.get(a['category'],[])
        restriction=specialty(n)
        if restriction:pool=[q for q in pool if re.search(restriction,zones[q['zone']][1]['name'].lower())]
        spec['specialty_pattern']=restriction
        if not pool:spec['status']='no_compatible_support';continue
        target=3 if a['category'] in (17,18,19,20,27,28,29,30,38,39,40,41) else 2
        # Restrict clinical instruments, food, clothes and workshop tools to
        # suitable support types within their already compatible rooms.
        if a['category'] in (38,39,40,41):pool=sorted(pool,key=lambda q:(q['model']!=15,q['used']))
        else:pool=sorted(pool,key=lambda q:q['used'])
        visited=set()
        for q in pool:
            if q['zone'] in visited or q['used']>=7:continue
            x,y,w,h=q['rect'];done=False
            for px,py in ((.12,.15),(.50,.15),(.88,.15),(.12,.50),(.50,.50),(.88,.50),(.12,.85),(.50,.85),(.88,.85)):
                for turn in (q['turn'],(q['turn']+1)%4):
                    if put(a,parts,q,[x+w*px,y+h*py,q['z']+.004],turn,'surface'):done=True;visited.add(q['zone']);break
                if done:break
            if counts[a['id']]>=target:break

    # Focal arrangements need an uncluttered support. Fit a small full-size
    # folding table before adding the sewing machine, board game or instrument;
    # don't shrink these objects to squeeze between old stock on a full shelf.
    stand_parts=shape('stand',37)
    stand_type=type_data['T0037'];stand_type.update(family='stand',mount='floor',parts=stand_parts)
    for ident,pattern in [('T0523','групповой терапи|кладовая игр'),('T0986','гладиль|прачеч'),('T0566','музыкаль')]:
        a=type_data[ident]
        if a.get('instances') or counts[ident]:continue
        parts=a.get('parts')
        if not parts:continue
        placed_count=0
        for s,r in zones.values():
            if not r['enclosed'] or not re.search(pattern,r['name'].lower()):continue
            ox,oy,z=s['origin'];rx,ry,w,h=r['rect'];opts=[]
            for turn in (0,1):
                fw,fd=(.50,.70) if turn else (.70,.50)
                xx=ox+rx+fw/2+.20
                while xx<ox+rx+w-fw/2-.19:
                    yy=oy+ry+fd/2+.20
                    while yy<oy+ry+h-fd/2-.19:
                        rr=[xx-fw/2,yy-fd/2,fw,fd]
                        edge=min(xx-(ox+rx),ox+rx+w-xx,yy-(oy+ry),oy+ry+h-yy)
                        opts.append((edge,xx,yy,turn,rr));yy+=.36
                    xx+=.36
            done=False
            for _,xx,yy,turn,rr in sorted(opts):
                if floor.hits(z,rr,.10) or reserved.hits(z,rr,.05):continue
                q=dict(zone=r['id'],site=s['id'])
                if not put(stand_type,stand_parts,q,[xx,yy,z],turn,'floor'):continue
                stand=items[-1];feature=dict(id=stand['id'],rect=rr,z=z,source='item_catalog_v12')
                d['collision_features'].append(feature);newfloors.append(feature);floor.add(z,rr)
                for b in e['boxes'][-stand['parts']:]:b['solid']=True
                fw,fd=rr[2:];q.update(parent=stand['id'],rect=[xx-fw/2+.025,yy-fd/2+.025,fw-.05,fd-.05],z=z+.72,used=0)
                if not put(a,parts,q,[xx,yy,z+.724],turn,'surface'):raise ValueError(('hero support did not fit',ident))
                done=True;placed_count+=1;break
            if placed_count>=2:break
    for i in items:type_data[i['type']]['instances'].append(i['id'])
    for a in type_data.values():
        if a['instances']:a['status']='new_shared_geometry_prototype'
    d['item_types_dressing']=dict(version=12,source_file='EHO_full_item_types_catalog.md',
        source_sha256=hashlib.sha256((INPUT/'EHO_full_item_types_catalog.md').read_bytes()).hexdigest(),
        room_reference_sha256=hashlib.sha256((INPUT/'room_reference.txt').read_bytes()).hexdigest(),
        catalog=list(type_data.values()),items=items,room_profiles=[dict(zone=z,site=s['id'],name=r['name'],categories=[c for c,p in AFFINITY.items() if re.search(p,r['name'].lower())],added_items=zonecounts[z]) for z,(s,r) in zones.items()],
        limits=['The catalogue counts semantic types, not unique meshes. Geometry is deliberately shared between related objects.',
                'New small props are decorative, not new usable inventory, medical simulations or written story content.',
                'Unplaced types and architectural/full-size art backlog are recorded explicitly; not all 1711 types are claimed as finished.',
                'Old floor furniture and routes are retained; easels/guitars have matching physical and data collision.'])
    d['review_revision']=12
    d['furnishing_captures']=[c for c in d['furnishing_captures'] if not c.get('item_catalog')]
    # Explicit viewpoints inside the room and a clear line to the new object.
    for name,fam,category in [('microscope','microscope',11),('sewing','sewing',30),('therapy','chess',17),('music','keyboard_instrument',18),('personal','glasses',19),('groceries','produce',38),('kitchen','bowl',28),('workshop','hammer',32),('guitar','guitar',18),('easel','easel',18)]:
        candidates=sorted([i for i in items if i['family']==fam and i['category']==category],key=lambda i:i['zone'])
        chosen=None
        for i in candidates:
            s,r=zones[i['zone']];ox,oy,z=s['origin'];rx,ry,w,h=r['rect'];b=i['bounds'];target=[(b[a]+b[a+3])/2 for a in range(3)]
            for dx,dy in ((0,.95),(0,-.95),(.95,0),(-.95,0),(.7,.7),(-.7,-.7)):
                eye=[target[0]+dx,target[1]+dy,max(z+1.45,target[2]+.50)]
                if not(ox+rx+.1<eye[0]<ox+rx+w-.1 and oy+ry+.1<eye[1]<oy+ry+h-.1):continue
                if space.hits(bounds(eye,[.12,.12,.12])):continue
                if any(wall.hits(z,[eye[0]+(target[0]-eye[0])*t-.018,eye[1]+(target[1]-eye[1])*t-.018,.036,.036]) for t in (.1,.2,.3,.4,.5,.6,.7,.8,.9)):continue
                chosen=(eye,target,i['id']);break
            if chosen:break
        if chosen:
            eye,target,ident=chosen;d['furnishing_captures'].append(dict(id='catalog_'+name,at=eye,target=target,item_catalog=True,detail_item=ident))
    stats=dict(types=1711,categories=50,new_items=len(items),new_parts=sum(i['parts'] for i in items),new_types=len(counts),geometry_families=len({i['family'] for i in items}),zones=len(zonecounts),new_floor_accents=len(newfloors),status_counts=dict(collections.Counter(a['status'] for a in type_data.values())),category_counts=dict(collections.Counter(i['category'] for i in items)))
    (OUT/'catalog_inventory.json').write_text(json.dumps(stats,ensure_ascii=False,indent=2),encoding='utf-8')
    for filename,records,fields in [('item_types.csv',list(type_data.values()),['id','name','category','category_name','family','mount','status','base_model','existing_instances','instances']),('placements.csv',items,['id','type','family','category','zone','site','mount','parent','base','bounds','parts'])]:
        with (OUT/filename).open('w',encoding='utf-8-sig',newline='') as f:
            w=csv.DictWriter(f,fields,extrasaction='ignore');w.writeheader();w.writerows({k:json.dumps(v,ensure_ascii=False) if isinstance(v,(list,dict)) else v for k,v in a.items()} for a in records)
    print(json.dumps(stats,ensure_ascii=False),flush=True);return stats

def refresh_navigation():
    import contextlib,io,runpy
    import numpy as np
    with contextlib.redirect_stdout(io.StringIO()):g=runpy.run_path(str(ROOT/'tools/verify_story_layout.py'))
    d=g['D'];runs=[]
    for zi,z in enumerate(g['LEVELS']):
        for yi,row in enumerate(g['mask'][zi]):
            edges=np.diff(np.r_[False,row,False].astype(np.int8));starts=np.flatnonzero(edges==1);ends=np.flatnonzero(edges==-1)
            runs.extend([[round(z*100),yi,int(a),int(b-a)] for a,b in zip(starts,ends)])
    d['story_gameplay']['navigation']['rows']=runs
    PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':
    INPUT.mkdir(parents=True,exist_ok=True)
    if not (INPUT/'EHO_full_item_types_catalog.md').exists():shutil.copy2(Path('C:/Users/YOUR_WINDOWS_USER/Downloads/EHO_full_item_types_catalog.md'),INPUT/'EHO_full_item_types_catalog.md')
    if not (INPUT/'room_reference.txt').exists():shutil.copy2(Path('C:/Users/YOUR_WINDOWS_USER/.codex/attachments/a4b8fa04-a79b-4ec1-9600-301e80c8e6fc/Вставленный текст.txt'),INPUT/'room_reference.txt')
    d=json.loads(PATH.read_text(encoding='utf-8'));d['collision_features']=[f for f in d['collision_features'] if f.get('source')!='item_catalog_v12']
    apply_item_types(d);PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    refresh_navigation()
