"""Complete the hospital's service programme without shrinking its geometry.

The supplied room list is reference data. Core story locations are protected;
repeated upper-floor support rooms receive specialties and recognisable props.
"""
import collections,csv,hashlib,json,math,re,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from hospital_clutter_v10 import Space,bounds,rotate
from hospital_furnishing_v6 import Index
from hospital_item_types_v12 import refresh_navigation

ROOT=Path(__file__).resolve().parents[1]
PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
OUT=ROOT/'artifacts/unreal_hospital_v13'
INPUT=ROOT/'data/hospital_services_v13'

RENAMES={
 'DG3_02':'Биохимическая лаборатория',
 'DG3_05':'Архив функциональной диагностики',
 'DG3_06':'Серверная функциональной диагностики / PACS',
 'DG3_08':'Кабинет кардиолога',
 'DG3_09':'Хранение и подготовка эндоскопов',
 'DG3_11':'Медтехника / упаковка стерильных инструментов',
 'IF2_05':'Пост сестры гематологии и онкологии',
 'IF2_06':'Смотровая онколога',
 'IF2_09':'Палата онкологии / дневной стационар',
 'IF2_10':'Процедурная инфузионной терапии',
 'IF2_11':'Лаборатория гематологии',
 'IF2_12':'Лаборатория гистологии',
 'IF2_13':'Лаборатория цитологии / автоклавная',
 'IF2_15':'Банк крови / чистое холодильное хранение',
 'IF2_18':'Кабинет трансфузиолога',
 'IF2_19':'Архив доноров и компонентов крови',
 'IF2_20':'Контроль фильтров / чистая подготовка крови',
 'IF2_21':'Обеззараживание лабораторных отходов',
 'IF3_05':'Пост сестры детского отделения',
 'IF3_06':'Смотровая акушерства и гинекологии',
 'IF3_09':'Двухместная детская палата',
 'IF3_10':'Послеродовая палата матери и ребёнка',
 'IF3_11':'Лаборатория ПЦР перинатального блока',
 'IF3_12':'Микробиология перинатального блока',
 'IF3_15':'Чистое хранение детских расходников',
 'IF3_17':'Изолированная палата ожогового блока',
 'IF3_18':'Кабинет педиатра',
 'IF3_19':'Архив семей / социальная помощь',
 'IF3_20':'Контроль вентиляции детского блока',
 'IF3_23':'Семейная переговорная / поддержка родителей',
 'RH2_19':'Травматология и ортопедия / протезирование',
 'RH3_06':'Пост сестры нефрологии',
 'RH3_11':'Моечная подготовки диализных систем',
 'RH3_13':'Зал гемодиализа',
 'RH3_15':'Склад стерильных диализных комплектов',
 'RH3_20':'Кабинет нефролога / обучение пациентов',
 'PS2_18':'Кабинет социального работника',
 'PS05':'Палата наблюдения острых состояний',
 'PS2_05':'Палата длительного лечения / дневной стационар',
 'PS2_06':'Тихая безопасная комната',
 'PS2_12':'Сенсорная комната / лечебные упражнения',
 'HS2_14':'Гостевая преподавателей и студентов',
 'HS2_18':'Учебная аудитория медперсонала',
 'HS2_24':'Конференц-зал / клинический разбор',
 'SV2_06':'Столярная мастерская',
 'SV2_08':'Слесарная мастерская',
 'SV2_19':'Электротехническая мастерская',
 'SV3_17':'Трансформаторная / распределительные щиты',
 'SV3_20':'Отопительный узел и водоподготовка',
 'B118':'Склад реагентов / морозильники образцов',
 'B104':'Приём тел / зал прощания',
}
# B104/B118 are not scene anchors, but these additions are explicit rather
# than destructive renames: the original functions remain visible in the name.

SERVICE_ZONES={
1:['N02 N01','R01 R11','N01 F104 F102','F101 R05','S02','R02 R03','R02 N03','R03 IF03','N13 R37','F104 R01'],
2:['R07 F202 F201','R16 F203','R26 PS18','R29 IF2_10','N04 IF22','F208 RH12','R15','RH02 R24','B110 R14','F103 N10'],
3:['R27 DG05','DG02 DG03 R31 R32','F206 DG13','DG09 F311 DG12','F205 F213','R26 DG3_08','DG06 R37','DG04','DG17 DG18'],
4:['R22 F209 E08','R19 F210','R19 F209','F211','F212','N03','R35','F210 F106','F207 F106 DG3_11','F216'],
5:['R18','R29','R21 DG3_02','F214','IF11','IF2_12 IF2_13','B118','B118 E12','F207','IF13 IF21'],
6:['DG3_08 F310','RH2_19 RH21','IF2_06 IF2_09','RH3_13','IF3_17','IF01 IF03','IF3_09 IF3_18','IF3_06 IF3_10','IF2_15 IF2_18','F313'],
7:['RH17 R43','RH03','RH05','RH18 RH17','RH04','RH20','RH19 RH21','RH09','RH07 RH11 RH15'],
8:['PS01 PS02','PS05 PS2_05','PS2_05','PS02 PS03 PS2_18','PS03 PS09','PS19','PS06 PS12','PS03 PS13 PS16','PS10 PS11','PS2_12 RH17','PS20 S05','PS04 PS08','PS2_06 PS23'],
9:['B107 B106','B106','SV10 SV11 SV12','B110 R09 X04','F105 N04','R23','SV2_06 SV2_08 SV2_19','S09 SV14 N12','SV01 SV02','N10 IF21'],
10:['R06 SV3_17','R54 SV18 B115','B117 IF16','R52 B113','SV3_20','B105 T06','B112','SV21 T05','B116 N11','T12'],
11:['N06 PS18','R04 R34','F303 F302','R39 R40','E06 DG18','F104 R37 R24','R12 IF08 RH08','HS2_18 N07 HS2_24','HS2_14'],
}

# Deliberately different silhouettes; dimensions are full-size metres.
def equipment_shape(kind):
    p=[]
    def b(x,y,z,w,d,h,m='svc_steel'):p.append([x,y,z,w,d,h,m])
    def body(w,d,h,m='svc_ivory'):b(0,0,h/2,w,d,h,m)
    def legs(w,d,h):
        for x in (-w/2+.04,w/2-.04):
            for y in (-d/2+.04,d/2-.04):b(x,y,h/2,.04,.04,h)
    def wheels(w,d):
        for x in (-w/2+.05,w/2-.05):
            for y in (-d/2+.05,d/2-.05):b(x,y,.055,.07,.10,.11,'svc_dark')
    def screen(x,y,z,w=.27):
        b(x,y,z,w,.055,w*.66,'svc_dark');b(x,y+.031,z,w*.87,.004,w*.51,'svc_screen')
        for n in range(4):b(x+(n-1.5)*w*.16,y+.035,z,w*.09,.002,.008,'svc_ivory')
    def front_ring(x,y,z,w,h,mat='svc_steel'):
        # Stepped ring with a visibly open centre rather than a closed cube.
        for xx in (-w/2,w/2):b(x+xx,y,z,.045,.035,h*.60,mat)
        for zz in (-h/2,h/2):b(x,y,z+zz,w*.60,.035,.045,mat)
        for xx in (-w*.36,w*.36):
            for zz in (-h*.36,h*.36):b(x+xx,y,z+zz,w*.20,.035,h*.20,mat)
    if kind in ('ventilator','dialysis','defibrillator','anesthesia'):
        w,d,h=(.52,.48,.86) if kind=='dialysis' else (.45,.43,.65)
        legs(w,d,.15);wheels(w,d);b(0,0,.18+h/2,w,d,h,'svc_ivory');screen(0,.025,.18+h+.19,.34)
        for x in (-.15,.15):front_ring(x,d/2+.022,.50,.12,.12,'svc_teal' if kind=='dialysis' else 'svc_steel')
        # Visible breathing/fluid lines stay beside their own device footprint.
        for x in (-w/2-.025,w/2+.025):b(x,.07,.63,.025,.025,.80,'svc_tube')
        b(0,0,.17,w+.07,d+.05,.045)
        if kind=='anesthesia':
            for x in (-.12,.12):b(x,-.15,.46,.10,.10,.5,'svc_oxygen')
        if kind=='defibrillator':b(0,.05,.18+h,.25,.12,.04,'svc_red')
    elif kind=='infusion':
        b(0,0,.06,.45,.45,.06);b(0,0,.92,.035,.035,1.75);b(0,0,1.78,.42,.035,.035)
        b(.10,.035,1.18,.18,.14,.22,'svc_ivory');screen(.10,.11,1.20,.11)
        for x in (-.15,.15):b(x,0,1.55,.11,.07,.26,'svc_glass');b(x,.045,.85,.01,.01,1.15,'svc_tube')
    elif kind=='oxygen':
        b(0,0,.61,.20,.20,1.15,'svc_oxygen');b(0,0,1.23,.08,.08,.09);b(0,.02,1.30,.14,.04,.035)
        b(0,0,.025,.26,.26,.05,'svc_dark');front_ring(0,.12,1.23,.09,.09)
    elif kind in ('blood_fridge','sample_freezer','food_freezer'):
        w,d,h=(.68,.60,1.75);body(w,d,h)
        b(0,d/2+.012,h*.55,w-.06,.025,h-.16,'svc_glass' if kind=='blood_fridge' else 'svc_ivory')
        b(w*.35,d/2+.04,h*.55,.025,.035,.28,'svc_dark');screen(0,d/2+.04,h-.12,.18)
        for zz in (.35,.65,.95,1.25):
            b(0,d/2+.03,zz,w-.14,.02,.025)
            if kind=='blood_fridge':
                for x in (-.15,.15):b(x,d/2+.035,zz+.11,.15,.015,.16,'svc_red')
    elif kind=='pediatric_cot':
        legs(.65,1.25,.55);b(0,0,.55,.65,1.25,.09);b(0,0,.65,.60,1.20,.12,'svc_cloth')
        for x in (-.31,.31):
            b(x,0,1.10,.025,1.25,.025)
            for y in (-.56,-.28,0,.28,.56):b(x,y,.87,.02,.025,.46)
        for y in (-.61,.61):b(0,y,.87,.64,.035,.46,'svc_glass')
    elif kind=='neonatal':
        legs(.72,1.10,.70);wheels(.72,1.10);b(0,0,.71,.72,1.1,.08,'svc_ivory')
        for x in (-.34,.34):b(x,0,.95,.025,1.05,.42,'svc_glass')
        for y in (-.50,.50):b(0,y,.95,.69,.025,.42,'svc_glass')
        b(0,0,1.17,.72,1.10,.025,'svc_glass');b(0,0,.79,.60,.94,.08,'svc_cloth')
    elif kind=='gyn_chair':
        b(0,0,.31,.45,.60,.60);b(0,0,.67,.68,.70,.16,'svc_cloth');b(0,-.32,1.0,.68,.16,.65,'svc_cloth')
        for x in (-.42,.42):b(x,.36,.67,.04,.50,.04);b(x,.56,.78,.16,.18,.09,'svc_cloth')
    elif kind=='traction':
        b(0,0,.06,.48,.48,.08);b(0,0,1.10,.045,.045,2.1);b(0,0,2.1,.65,.045,.045)
        for x in (-.28,.28):b(x,.01,1.55,.014,.014,1.05,'svc_tube');b(x,.01,1.04,.11,.10,.14,'svc_dark')
    elif kind=='parallel_bars':
        for x in (-.40,.40):
            b(x,0,.92,.045,1.8,.045)
            for y in (-.78,.78):b(x,y,.46,.045,.045,.92);b(x,y,.035,.23,.35,.045)
    elif kind=='training_stairs':
        for n in range(4):b(0,-.45+n*.30,(n+1)*.075,.75,.30,(n+1)*.15,'svc_wood')
        for x in (-.43,.43):
            for n in range(4):b(x,-.45+n*.30,.55+n*.15,.035,.035,1.1);b(x,-.45+n*.30,1.1+n*.15,.04,.33,.04)
    elif kind=='scrub_sink':
        legs(.75,.47,.75);b(0,0,.78,.75,.47,.10,'svc_ivory');b(0,0,.84,.56,.33,.035,'svc_glass')
        for x in (-.20,.20):b(x,-.19,.99,.025,.025,.30);b(x,-.11,1.14,.025,.18,.025);b(x+.06,-.19,1.06,.14,.025,.025)
    elif kind=='autoclave':
        legs(.65,.60,.18);b(0,0,.65,.65,.60,1.02,'svc_ivory');front_ring(0,.315,.63,.45,.52)
        b(0,.31,.63,.32,.02,.36,'svc_dark');screen(0,.31,1.04,.20)
    elif kind=='labhood':
        legs(.95,.60,.78);b(0,0,.80,.95,.60,.06)
        for x in (-.46,.46):b(x,0,1.28,.035,.60,.94,'svc_ivory')
        b(0,-.28,1.28,.95,.04,.94,'svc_ivory');b(0,0,1.77,.95,.60,.10,'svc_ivory');b(0,.26,1.50,.88,.025,.42,'svc_glass');b(0,-.05,1.90,.30,.35,.19)
    elif kind in ('washer','dryer'):
        body(.75,.65,1.05);b(0,.33,.56,.52,.025,.52,'svc_dark');front_ring(0,.35,.56,.54,.54)
        screen(-.13,.34,.95,.19);b(.23,.35,.95,.07,.035,.07,'svc_dark')
        if kind=='dryer':
            for x in (-.18,-.06,.06,.18):b(x,.337,.15,.055,.007,.06)
    elif kind=='kitchen_range':
        legs(.85,.60,.15);b(0,0,.49,.85,.60,.70,'svc_ivory');b(0,0,.86,.88,.63,.035)
        for x in (-.21,.21):
            for y in (-.15,.15):b(x,y,.885,.23,.22,.015,'svc_dark')
        b(0,.31,.46,.64,.025,.40,'svc_dark');b(0,.34,.70,.60,.025,.025)
    elif kind=='workbench':
        legs(.90,.50,.80);b(0,0,.81,.90,.50,.06,'svc_wood');b(0,-.21,1.24,.90,.035,.80,'svc_teal')
        for x in (-.30,-.10,.10,.30):b(x,-.18,1.30,.025,.025,.27);b(x,-.18,1.43,.12,.025,.04)
        b(.26,.12,.90,.18,.18,.12,'svc_dark');b(.26,.10,1.0,.23,.15,.07)
    elif kind in ('pump','boiler','transformer'):
        if kind=='pump':
            b(0,0,.05,.65,.48,.10);b(0,0,.29,.45,.32,.38,'svc_teal');b(0,.24,.36,.12,.25,.13);b(0,.33,.56,.12,.12,.42)
        elif kind=='boiler':
            body(.65,.62,1.40,'svc_teal');b(0,.32,.78,.45,.035,.50,'svc_dark');screen(0,.35,1.21,.22)
            for x in (-.22,.22):b(x,-.32,1.55,.07,.07,.34)
        else:
            body(.70,.60,1.15);b(0,0,1.20,.60,.50,.10,'svc_dark')
            for x in (-.26,-.13,0,.13,.26):b(x,.33,.65,.045,.12,.85)
            for x in (-.22,0,.22):b(x,0,1.36,.07,.07,.23,'svc_dark')
    elif kind=='pedal_trainer':
        b(0,0,.10,.52,.90,.08);b(0,0,.48,.12,.44,.65,'svc_ivory');b(0,-.18,.85,.045,.045,.70)
        b(0,-.18,1.01,.32,.24,.08,'svc_cloth');b(0,.31,.72,.045,.045,1.1);b(0,.31,1.22,.48,.045,.045,'svc_dark')
        for x in (-.22,.22):b(x,0,.43,.16,.22,.035,'svc_dark');b(x/2,0,.43,.25,.035,.035)
    elif kind=='nurse_station':
        legs(.90,.48,1.00);b(0,0,1.02,.90,.48,.06,'svc_ivory');b(0,.20,.52,.90,.035,.95,'svc_teal');screen(-.20,0,1.25,.25)
        b(.21,.03,1.07,.24,.22,.025,'svc_ivory')
    elif kind=='lecture':
        legs(.65,.45,1.0);b(0,0,1.02,.68,.48,.05,'svc_wood');b(0,-.20,1.34,.035,.035,.64,'svc_dark');screen(0,-.18,1.40,.26)
    elif kind=='waste_station':
        for x,m in ((-.24,'svc_ivory'),(.24,'svc_yellow')):
            b(x,0,.28,.35,.36,.55,m);b(x,0,.57,.38,.39,.035,'svc_dark');b(x,.19,.34,.17,.005,.12,'svc_teal')
    else:raise ValueError(kind)
    low=min(a[2]-a[5]/2 for a in p)
    for a in p:a[2]-=low
    for i in (0,1):
        c=(min(a[i]-a[i+3]/2 for a in p)+max(a[i]+a[i+3]/2 for a in p))/2
        for a in p:a[i]-=c
    return p

# Each requested new service has a visible signature at its actual address.
KITS={
 'ventilator':['N03','F212','R35','IF3_17'],
 'dialysis':['RH3_13'], 'defibrillator':['R02','DG3_08'],
 'anesthesia':['F211','F209'],
 'infusion':['IF2_10','IF2_09','RH3_13','F212'],
 'oxygen':['N03','F211','IF3_17'],
 'blood_fridge':['IF2_15'], 'sample_freezer':['B118','IF2_11'],
 'food_freezer':['B106'],
 'pediatric_cot':['IF3_09','IF3_10'], 'neonatal':['IF3_10'],
 'gyn_chair':['IF3_06'], 'traction':['RH2_19'],
 'parallel_bars':['RH17'], 'training_stairs':['RH18'],
 'scrub_sink':['R19','F209','F207'], 'autoclave':['F106','DG3_11','IF13'],
 'labhood':['DG3_02','IF2_12','IF2_13'],
 'washer':['SV10'], 'dryer':['SV11'],
 'kitchen_range':['B107'],
 'workbench':['SV2_06','SV2_08','SV2_19'],
 'pump':['SV3_20'], 'boiler':['SV3_20'], 'transformer':['SV3_17'],
 'lecture':['HS2_18','HS2_24'],
 'nurse_station':['R16','F203','IF3_05','RH3_06'],
 'pedal_trainer':['PS2_12'],
 'waste_station':['IF2_21','N10','SV09'],
}

def read_services():
    text=(INPUT/'room_reference.txt').read_text(encoding='utf-8-sig');section=0;seen=collections.Counter();rows=[]
    for line in text.splitlines():
        m=re.match(r'(\d+)\. ',line)
        if m:section=int(m.group(1))
        if line.startswith('- '):
            i=seen[section];seen[section]+=1;rows.append(dict(id=f'{section:02}.{i+1:02}',section=section,requirement=line[2:],zones=SERVICE_ZONES[section][i].split()))
    assert len(rows)==110 and all(seen[k]==len(v) for k,v in SERVICE_ZONES.items())
    for n,(name,zones) in enumerate([
       ('Парковки','S01'),('КПП','S03'),('Сад','S05'),('Навесы','S02 S15 S09'),
       ('Служебные дворы','S04 S09 SV24'),('Отдельные корпуса и крытые переходы','DG IF PS RH HS SV'),
       ('Вертолётная площадка','T04'),('Приём тел','B104'),('Холодильное хранение тел','N09'),
       ('Секционная','B103'),('Лаборатории патологии','R20 IF2_12'),('Служебный въезд патологии','S07 N16'),
       ('Помещение прощания','B104')],1):
        rows.append(dict(id=f'12.{n:02}',section=12,requirement=name,zones=zones.split()))
    return rows

def text_rectangles(text):
    """Readable Cyrillic using merged pixel runs on a thin world-space panel."""
    image=Image.new('1',(256,76),0);draw=ImageDraw.Draw(image)
    font=ImageFont.truetype(str(ROOT/'unreal/EchoWard/Content/CampaignData/Fonts/NotoSans.ttf'),18)
    words=text.upper().replace(' / ',' ').split();lines=['']
    for word in words:
        candidate=(lines[-1]+' '+word).strip()
        if draw.textlength(candidate,font=font)>242 and lines[-1]:lines.append(word)
        else:lines[-1]=candidate
    if len(lines)>3:
        font=ImageFont.truetype(str(ROOT/'unreal/EchoWard/Content/CampaignData/Fonts/NotoSans.ttf'),14)
        lines=[''];
        for word in words:
            candidate=(lines[-1]+' '+word).strip()
            if draw.textlength(candidate,font=font)>242 and lines[-1]:lines.append(word)
            else:lines[-1]=candidate
    assert len(lines)<=3,('sign too long',text)
    lineheight=22 if len(lines)<=2 else 20;top=(76-len(lines)*lineheight)/2
    for i,line in enumerate(lines):draw.text((128,top+i*lineheight),line,font=font,fill=1,anchor='mt')
    active={};rects=[];px=image.load()
    for y in range(77):
        runs=[];start=None
        for x in range(257):
            ink=x<256 and y<76 and px[x,y]
            if ink and start is None:start=x
            if not ink and start is not None:runs.append((start,x));start=None
        current={}
        for r in runs:
            if r in active:current[r]=active.pop(r)
            else:current[r]=y
        for (x1,x2),yy in active.items():rects.append([x1,yy,x2-x1,y-yy])
        active=current
    return rects

def apply_services(d):
    OUT.mkdir(parents=True,exist_ok=True);INPUT.mkdir(parents=True,exist_ok=True)
    if not (INPUT/'room_reference.txt').exists():shutil.copy2(ROOT/'data/item_types_v12/room_reference.txt',INPUT/'room_reference.txt')
    if not (OUT/'baseline_v12.json').exists():
        assert d['review_revision']==12
        (OUT/'baseline_v12.json').write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    e=d['environment_dressing'];e['boxes']=[b for b in e['boxes'] if b.get('layer')!=13]
    d['collision_features']=[f for f in d['collision_features'] if f.get('source')!='hospital_services_v13']
    d['furnishing_captures']=[c for c in d['furnishing_captures'] if not c.get('hospital_services')]
    zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};protected={q['zone'] for q in d['story_gameplay']['placements']+d['story_gameplay']['mobs']}
    assert not(set(RENAMES)&protected),('protected story names',set(RENAMES)&protected)
    changes=[]
    for zid,name in RENAMES.items():
        s,r=zones[zid];oldname=r.get('pre_service_name',r['name']);r['pre_service_name']=oldname;r['name']=name
        changes.append(dict(zone=zid,before=oldname,after=name))
    # Restore the machinery-only title if upgrading an intermediate v13 draft;
    # food storage belongs to the kitchen store, never the morgue plant room.
    if 'B105' not in RENAMES:
        room=zones['B105'][1]
        if 'pre_service_name' in room:room['name']=room.pop('pre_service_name')
    # Functional names are visible in the game's map and location HUD; IDs,
    # rectangles, door states, refuge bolts and chapter links remain intact.
    mats={'ivory':[.69,.72,.67],'dark':[.025,.035,.038],'steel':[.30,.37,.38],
      'teal':[.055,.25,.25],'red':[.43,.04,.025],'yellow':[.62,.43,.09],
      'screen':[.08,.30,.28],'tube':[.18,.23,.23],'oxygen':[.12,.31,.38],
      'glass':[.19,.39,.40],'cloth':[.25,.40,.38],'wood':[.29,.16,.08],
      'sign':[.78,.80,.74],'ink':[.045,.10,.11]}
    for k,tint in mats.items():e['materials']['svc_'+k]=dict(tint=tint,roughness=.43 if k=='glass' else .74,weathering=.16,metallic=.52 if k=='steel' else 0)
    floor=Index();reserved=Index();walls=Index();space=Space()
    for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):
        floor.add(w['z'],w['rect']);walls.add(w['z'],w['rect'])
    for f in d['collision_features']:floor.add(f['z'],f['rect'])
    for p in d['prototype_props']+d['environment_props']:
        for a in d['prototype_catalog'][str(p['model'])]['parts']:
            off=rotate(a[:3],p['turn']);at=[p['at'][i]+off[i] for i in range(3)];sz=list(a[3:6])
            if p['turn']%2:sz[0],sz[1]=sz[1],sz[0]
            space.add(bounds(at,sz),p['id'])
    for j,b in enumerate(e['boxes']):
        if b['kind'] not in ('lawn','grass_tuft','pavement','road_dash','tree_crown'):space.add(bounds(b['at'],b['size']),b.get('item_id','OLD13_'+str(j)))
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
        x,y,z=q['at'];w,h,_=q['size'];reserved.add(z,[x-.2,y-.2,w+.4,h+1.8])
    for q in d['escape_niches']:
        x,y,z=q['entry'];reserved.add(z,q['rect']);reserve(z,q.get('corridor_point',[x,y,z]),[x,y],.7)
    for q in d['story_gameplay']['placements']+d['story_gameplay']['mobs']:
        x,y,z=q['at'];radius=1.1 if q.get('kind') in ('person','scene') or 'species' in q else .7
        reserved.add(z,[x-radius,y-radius,radius*2,radius*2])
    for q in d['stairs']:
        x,y,w,h=q['rect'];z=q['z_min']
        while z<q['z_max']+.01:reserved.add(round(z,2),[x-.6,y-.6,w+1.2,h+1.2]);z+=4.8
    for z in (-9.6,-4.8,0,4.8,9.6,14.4):reserved.add(z,[47.2,3.2,5.6,9.6])
    items=[];missing=[]
    def render(kind,zid,parts,base,turn,solid,mount,extra=None):
        boxes=[]
        for a in parts:
            off=rotate(a[:3],turn);at=[base[i]+off[i] for i in range(3)];sz=list(a[3:6])
            if turn%2:sz[0],sz[1]=sz[1],sz[0]
            if space.hits(bounds(at,sz)):return False
            boxes.append(dict(at=[round(v,6) for v in at],size=[round(v,6) for v in sz],material=a[6],kind='service_'+kind,zone=zid,solid=solid,layer=13))
        ident=f'SVC13_{kind}_{zid}_{len(items):04}'
        for b in boxes:b['item_id']=ident;e['boxes'].append(b)
        bb=[min(b['at'][i]-b['size'][i]/2 for b in boxes) for i in range(3)]+[max(b['at'][i]+b['size'][i]/2 for b in boxes) for i in range(3)]
        item=dict(id=ident,kind=kind,zone=zid,site=zones[zid][0]['id'],mount=mount,base=base,turn=turn,bounds=bb,parts=len(boxes),**(extra or {}));items.append(item);space.add(bb,ident)
        if solid:
            rr=[bb[0],bb[1],bb[3]-bb[0],bb[4]-bb[1]];floor.add(base[2],rr);d['collision_features'].append(dict(id=ident,rect=rr,z=base[2],source='hospital_services_v13'))
        return True
    def fit(kind,zid):
        s,r=zones[zid];ox,oy,z=s['origin'];x,y,w,h=r['rect'];parts=equipment_shape(kind)
        pb=[min(a[i]-a[i+3]/2 for a in parts) for i in range(3)]+[max(a[i]+a[i+3]/2 for a in parts) for i in range(3)];opts=[]
        for t in (0,1,2,3):
            fw,fd=pb[3]-pb[0],pb[4]-pb[1]
            if t%2:fw,fd=fd,fw
            if fw+.4>w or fd+.4>h:continue
            for ix in range(math.floor((w-fw-.4)/.30)+1):
                xx=ox+x+fw/2+.20+ix*.30
                for iy in range(math.floor((h-fd-.4)/.30)+1):
                    yy=oy+y+fd/2+.20+iy*.30;rr=[xx-fw/2,yy-fd/2,fw,fd]
                    if floor.hits(z,rr,.10) or reserved.hits(z,rr,.05):continue
                    nx,ny,_=rotate([0,1,0],t);extent=(fw if nx else fd)/2
                    front=[xx+nx*(extent+.55),yy+ny*(extent+.55),z]
                    access=[front[0]-.35,front[1]-.35,.70,.70]
                    if not(ox+x+.05<=access[0] and oy+y+.05<=access[1] and access[0]+.70<=ox+x+w-.05 and access[1]+.70<=oy+y+h-.05):continue
                    if floor.hits(z,access,.03):continue
                    edge=min(xx-(ox+x),ox+x+w-xx,yy-(oy+y),oy+y+h-yy)
                    opts.append((edge,xx,yy,t,front,access))
        for _,xx,yy,t,front,access in sorted(opts):
            if render(kind,zid,parts,[xx,yy,z],t,True,'floor',dict(front_access=front)):
                reserved.add(z,access);return True
        return False
    # Fit the larger apparatus first. No apparatus is scaled down to make it fit.
    pairs=[(kind,zid) for kind,zids in KITS.items() for zid in zids]
    pairs.sort(key=lambda p:-max(a[3]*a[4] for a in equipment_shape(p[0])))
    for kind,zid in pairs:
        if not fit(kind,zid):missing.append(dict(kind=kind,zone=zid))
    # Sign panels are genuinely attached to verified wall spans. The character
    # geometry uses the same already-cooked cube as the rest of the world.
    fixtures=collections.defaultdict(dict)
    for b in e['boxes']:
        if b['kind'] in ('radiator','camera_housing'):fixtures[b['zone']][b['kind']]=b
    matrix=read_services()
    signzones=set(RENAMES)|{zid for ids in KITS.values() for zid in ids}|{zid for row in matrix for zid in row['zones'] if zid in zones and zones[zid][1]['enclosed']}
    unsigned=[]
    for zid in sorted(signzones):
        s,r=zones[zid];ox,oy,z=s['origin'];name=r['name'];candidates=[]
        # Read the destination before entering the room: first try the wall
        # beside the doorway on its corridor-facing side.
        t={'north':2,'south':0,'west':1,'east':3}[r['door_side']];nx,ny,_=rotate([0,1,0],t)
        for along in (1.55,-1.55,2,-2):
            off=rotate([along,0,0],t);xx,yy=ox+r['door'][0]+off[0]+nx*.13,oy+r['door'][1]+off[1]+ny*.13
            for height in (2.15,2.40,1.90):candidates.append((xx,yy,z+height,t,'corridor'))
        pair=fixtures[zid]
        if len(pair)==2:
            rad,cam=pair['radiator'],pair['camera_housing'];dx,dy=cam['at'][0]-rad['at'][0],cam['at'][1]-rad['at'][1]
            t=0 if abs(dy)>abs(dx) and dy>0 else 2 if abs(dy)>abs(dx) else 3 if dx>0 else 1
            x,y,rz=rad['at']
            for lx in (0,-.5,.5,-1,1):
                for height in (2.40,2.15,2.55):
                    off=rotate([lx,-.073,height],t);candidates.append((x+off[0],y+off[1],z+height,t,'room'))
        baseparts=[[0,0,0,.86,.018,.30,'svc_sign'],[0,.012,.132,.86,.004,.025,'svc_teal']]
        for px,py,w,h in text_rectangles(name):baseparts.append([(px+w/2-128)*.0032,.013,(38-py-h/2)*.0032,w*.0032,.003,h*.0032,'svc_ink'])
        placed=False
        for x,y,zz,t,placement in candidates:
            nx,ny,_=rotate([0,1,0],t);samples=[]
            for u in (-.44,0,.44):
                off=rotate([u,0,0],t);xx,yy=x+off[0],y+off[1];found=None
                for dep in (.11,.15,.20,.25):
                    xx2,yy2=xx-nx*dep,yy-ny*dep
                    if walls.hits(z,[xx2-.008,yy2-.008,.016,.016]):found=[xx2,yy2,z];break
                if found is None:break
                samples.append(found)
            if len(samples)!=3:continue
            if render('sign',zid,baseparts,[x,y,zz],t,False,'wall',dict(text=name,placement=placement,wall_samples=samples)):
                placed=True;break
        if not placed:unsigned.append(zid)
    for row in matrix:
        row['rooms']=[dict(zone=z,name=zones[z][1]['name']) for z in row['zones'] if z in zones]
        row['sites']=[z for z in row['zones'] if z not in zones and any(s['id']==z for s in d['locations'])]
        assert len(row['rooms'])+len(row['sites'])==len(row['zones']),row
        row['status']='mapped_to_reachable_prototype_spaces'
        row['existing_models']=sorted({p['model'] for p in d['prototype_props']+d['environment_props'] if p['zone'] in row['zones']})
        row['new_equipment']=[i['id'] for i in items if i['mount']=='floor' and i['zone'] in row['zones']]
        row['signs']=[i['id'] for i in items if i['kind']=='sign' and i['zone'] in row['zones']]
    d['hospital_services']=dict(version=13,source_sha256=hashlib.sha256((INPUT/'room_reference.txt').read_bytes()).hexdigest(),
        requirements=matrix,room_changes=changes,items=items,equipment_requests=[dict(kind=k,zone=z) for k,z in pairs],unplaced_equipment=missing,unsigned_rooms=unsigned,
        limits=['The programme is a fictional hospital layout, not certification of a real clinical building.',
          'Equipment and signs are prototype geometry; no medical-device simulation is added.',
          'Existing architecture, rooms, chapter states and story items retain their IDs, dimensions and positions.',
          'The former generic furniture is retained. Individual final room dressing and finished art remain later work.'])
    d['review_revision']=13
    # Clear inspection cameras test all height samples against actual meshes,
    # not only the 2D wall raster, so a shelf cannot hide the featured apparatus.
    for kind in ('dialysis','pediatric_cot','gyn_chair','blood_fridge','labhood','washer','kitchen_range','boiler','transformer','lecture','nurse_station','sign'):
        chosen=None
        for item in [i for i in items if i['kind']==kind]:
            s,r=zones[item['zone']];ox,oy,z=s['origin'];rx,ry,w,h=r['rect'];bb=item['bounds'];target=[(bb[a]+bb[a+3])/2 for a in range(3)]
            if kind=='sign':
                n=rotate([0,1,0],item['turn']);vectors=[(n[0]*1.6,n[1]*1.6)]
            else:
                vectors=[]
                for vx,vy in ((0,1.5),(.7,1.3),(-.7,1.3),(0,1.15),(.5,1.0),(-.5,1.0)):
                    q=rotate([vx,vy,0],item['turn']);vectors.append(q[:2])
            for dx,dy in vectors:
                eye=[target[0]+dx,target[1]+dy,max(z+1.55,target[2]+.45)]
                if kind!='sign' and not(ox+rx+.12<eye[0]<ox+rx+w-.12 and oy+ry+.12<eye[1]<oy+ry+h-.12):continue
                if space.hits(bounds(eye,[.12,.12,.12])):continue
                if any(space.hits(bounds([eye[a]+(target[a]-eye[a])*t for a in range(3)],[.025,.025,.025]),item['id']) or walls.hits(z,[eye[0]+(target[0]-eye[0])*t-.015,eye[1]+(target[1]-eye[1])*t-.015,.03,.03]) for t in (.05,.1,.15,.2,.3,.4,.5,.6,.7,.8,.9)):
                    continue
                chosen=eye,target,item['id'];break
            if chosen:break
        if chosen:
            eye,target,ident=chosen;d['furnishing_captures'].append(dict(id='services_'+kind,at=eye,target=target,hospital_services=True,detail_item=ident))
    stats=dict(services=len(matrix),sections=12,renamed_rooms=len(changes),equipment=sum(i['mount']=='floor' for i in items),equipment_types=len({i['kind'] for i in items if i['mount']=='floor'}),signs=sum(i['kind']=='sign' for i in items),corridor_signs=sum(i.get('placement')=='corridor' for i in items),parts=sum(i['parts'] for i in items),unplaced_equipment=missing,unsigned_rooms=unsigned,new_views=sum(c.get('hospital_services',False) for c in d['furnishing_captures']))
    (OUT/'services_inventory.json').write_text(json.dumps(stats,ensure_ascii=False,indent=2),encoding='utf-8')
    for filename,records in [('requirements.csv',matrix),('room_changes.csv',changes),('service_items.csv',items)]:
        fields=list(dict.fromkeys(k for row in records for k in row))
        with (OUT/filename).open('w',encoding='utf-8-sig',newline='') as f:
            writer=csv.DictWriter(f,fields);writer.writeheader();writer.writerows({k:json.dumps(v,ensure_ascii=False) if isinstance(v,(list,dict)) else v for k,v in row.items()} for row in records)
    print(json.dumps(stats,ensure_ascii=False),flush=True);return stats

if __name__=='__main__':
    d=json.loads(PATH.read_text(encoding='utf-8'));apply_services(d)
    PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');refresh_navigation()
