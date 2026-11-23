"""Create the v3 exporter from the previous reviewed vector-atlas renderer."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'tools/export_hospital_metric_atlas.py').read_text(encoding='utf-8')
s=s.replace('hospital_campus_v2','hospital_story_shell_v3').replace("ECHO_Hospital_2D'","ECHO_Hospital_v3'").replace("hospital_metric'","hospital_story_v3'")
s=s.replace('import csv, hashlib, json, math, subprocess, zipfile','import csv, hashlib, json, math, subprocess, zipfile, sys\nDRAFT="--draft" in sys.argv')
s=s.replace("assert SOURCE.read_bytes()==STAGED.read_bytes(), 'Source and playable package differ'","assert DRAFT or SOURCE.read_bytes()==STAGED.read_bytes(), 'Source and playable package differ'")
s=s.replace("ROOT/'unreal/EchoWard/Saved/PackagedWindows/package_verified.json'","ROOT/('unreal/EchoWard/Saved/PackagedWindows/package_verified.previous.json' if DRAFT else 'unreal/EchoWard/Saved/PackagedWindows/package_verified.json')")
s=s.replace("assert MARKER['success'] and MARKER['world_id']==DATA['map_id']=='hospital_story_shell_v3'","assert DRAFT or (MARKER['success'] and MARKER['world_id']==DATA['map_id']=='hospital_story_shell_v3')")
s=s.replace("FONT=ROOT/", "assert DRAFT or MARKER['build_identity'].split(':')[-1].lower()==hashlib.sha256(SOURCE.read_bytes()).hexdigest()\nFONT=ROOT/",1)
s=s.replace("manifest=dict(world_id=DATA['map_id']","manifest=dict(draft=DRAFT,world_id=DATA['map_id']")
s=s.replace("passage(48,100,4,230);passage(44,-20,8,20)\nfor y in (38,158,268):passage(-46,y,94,6);passage(52,y,98,6)","for x,y,w,h,z in DATA['passages']:put_rect([0,0,z],[x,y,w,h])")
start=s.index('def stairhole(');end=s.index('def feature(',start)
s=s[:start]+'''def stairhole(x,y):return any(r['rect'][0]<=x<r['rect'][0]+3 and r['rect'][1]<=y<r['rect'][1]+7 for r in DATA['stairs'])
def hole(x,y,z):return shaft(x,y) or any(r['z_min']<z<=r['z_max'] and r['rect'][0]<=x<r['rect'][0]+3 and r['rect'][1]<=y<r['rect'][1]+7 for r in DATA['stairs'])
'''+s[end:]
s=s.replace("surface('ROOF_APRON','Западный кровельный фартук',[-8,0,8,100],14.4,'asphalt',11)","# Roof gallery is covered by generated floor strips with both stair cutouts.")
start=s.index('for n,r in enumerate([[-180');end=s.index('# Reproduce runtime floor',start)
s=s[:start]+'''for y in range(-190,380):
    x=-180
    def ground(a):return not (-8<=a<104 and 0<=y<100) and not hole(a,y,0)
    while x<280:
        if not ground(x):x+=1;continue
        end=x+1
        while end<280 and ground(end):end+=1
        surface('G'+str(len(SURFACES)),'Основание территории',[x,y,end-x,1],-.2,'asphalt',0);x=end
'''+s[end:]
pos=s.index('# Exact wall centrelines')
s=s[:pos]+'''PORTALS=[]
for r in ROOMS:
    PORTALS.append((*r['global_door'],r['z'],r['door_side'],2))
    ox,oy,_=LOC[r['location']]['origin']
    for e in r.get('extra_doors',[]):PORTALS.append((ox+e['at'][0],oy+e['at'][1],r['z'],e['side'],e.get('width',2)))
'''+s[pos:]
old="door=rid>=0 and dy!=0 and abs(my-ROOMS[rid]['global_door'][1])<.001 and abs(mx-ROOMS[rid]['global_door'][0])<1.05"
new="door=any(abs(z-pz)<.01 and ((dy and side in ('north','south') and abs(my-py)<.001 and abs(mx-px)<pw/2+.05) or (dx and side in ('east','west') and abs(mx-px)<.001 and abs(my-py)<pw/2+.05)) for px,py,pz,side,pw in PORTALS)"
assert old in s;s=s.replace(old,new)
pos=s.index('# Engineering footprints')
s=s[:pos]+'''for p0 in DATA['partitions']:
    ax,ay,z=p0['a'];bx,by,_=p0['b'];horizontal=ay==by
    WALLS.append(dict(z=z,axis='X' if horizontal else 'Y',fixed=ay if horizontal else ax,start=min(ax,bx) if horizontal else min(ay,by),end=max(ax,bx) if horizontal else max(ay,by),thickness=p0['thickness'],kind='partition',base_z=z,height=p0['height']))
'''+s[pos:]
start=s.index("CORES=[");end=s.index("for z in (-9.6,-4.8,0,4.8,9.6,14.4):\n    feature('LIFT",start)
s=s[:start]+'''CORES=[(r['id'],r['rect'][0],r['rect'][1]) for r in DATA['stairs']]
for r0 in DATA['stairs']:
    name=r0['id'];x,y,_,_=r0['rect'];z=r0['z_min']
    while z<=r0['z_max']+.01:
        feature(f'ST-{name}-{fmt(z)}',f'Лестница {name}',x,y,3,8,z,z+3.8,'stair',name,'Марш 1.36 м; ступень 0.28 x 0.16 м')
        if z<r0['z_max']-.01:
            for i in range(15):
                rise=(i+1)*.16
                feature(f'{name}-{fmt(z)}-1-{i+1}',f'Ступень {i+1}, марш 1',x+.10,y+6.8-(i+1)*.28,1.36,.28,z,z+rise,'step',name)
                feature(f'{name}-{fmt(z)}-2-{i+1}',f'Ступень {i+1}, марш 2',x+1.54,y+2.6+i*.28,1.36,.28,z+2.4,z+2.4+rise,'step',name)
        z=round(z+4.8,2)
'''+s[end:]
s=s.replace("feature('FENCE-N','Граница забора',-180,379.92,460,.16,0,2.2,'fence','ST')","feature('FENCE-N','Граница забора',-180,379.92,460,.16,0,2.2,'fence','ST')\nfeature('FENCE-SW','Южный забор',-180,-190.08,212,.16,0,2.2,'fence','ST')\nfeature('FENCE-SE','Южный забор',62,-190.08,218,.16,0,2.2,'fence','ST')")
s=s.replace("csv_write('01_Все_333_зоны_метры.csv'","csv_write('01_Все_334_зоны_метры.csv'")
pos=s.index("(OUT/'hospital_world_source.json')")
s=s[:pos]+'''bay_rows=[]
for b in DATA['subrooms']:
    ox,oy,z=LOC[b['site']]['origin'];x,y,w,h=b['rect']
    bay_rows.append(dict(ID=b['id'],Зона=b['zone'],Корпус=b['site'],X_м=ox+x,Y_м=oy+y,Z_м=z,Ширина_м=w,Глубина_м=h,Площадь_м2=b['area_m2'],Без_стен_X_м=b['clear_width_m'],Без_стен_Y_м=b['clear_depth_m']))
csv_write('05_Внутренние_отсеки.csv',list(bay_rows[0]),bay_rows)
csv_write('06_Классы_и_главы.csv',['ID','Корпус','Класс','Главы','Убежище','Дверь','Стена'],[{'ID':r['id'],'Корпус':r['location'],'Класс':r['production_class'],'Главы':','.join(map(str,r['chapters'])),'Убежище':int(r['refuge']),'Дверь':r['door_type'],'Стена':r['wall_type']} for r in ROOMS])
'''+s[pos:]
s=s.replace("ЭХО_точная_2D_карта_333_зоны.pdf","ЭХО_карта_v3_геометрия_и_сюжет.pdf")
s=s.replace('ЭХО - точный 2D-атлас 333 зон, метры и покрытия','ЭХО - карта v3, размеры и сюжетные привязки')
s=s.replace('Точное состояние готовой Windows-сборки A45EC26E. Схема не меняет игру.','Обновлённая игровая архитектура; 333 прежние зоны и скрытая палата.')
s=s.replace('333 зоны • 13 локаций • все этажи и корпуса','334 зоны • 15 локаций • больница №333 / отделение 04')
s=s.replace('Размеры комнат заданы по линиям границ ячеек модели. Основные стены толщиной 0.16 м выступают на 0.08 м внутрь каждой стороны. Для закрытой прямоугольной комнаты ориентировочный размер в свету: (ширина - 0.16) × (глубина - 0.16). В CSV приведены оба значения.','333 прежних ID обозначают зоны, а не одну гигантскую палату. Их габариты и площади сохранены. Внутри добавлены палаты и кабинеты, проходы и ниши. Наружная стена 0.16 м; перегородка 0.12 м. Точные размеры внутренних отсеков вынесены в CSV 05. Габарит без наружных стен не равен чистой площади после перепланировки.')
s=s.replace('CSV всех 333 зон','CSV всех 334 зон')
s=s.replace('Граница южной стороны Y=-190 м открыта: забора там нет. Забор 2.20 м стоит на X=-180, X=280 и Y=380.','Южный забор: Y=-190 м; въезд X=32...62 м открыт. Остальной периметр ограждён, высота 2.20 м.')
s=s.replace("ORDER=['ST','1F','B1','B2','2F','3F','RF','DG','IF','PS','RH','HS','SV']","ORDER=['ST','1F','B1','B2','2F','3F','RF','DG','DG2','IF','PS','RH','HS','SV','UT']")
s=s.replace("denom=700 if ident=='ST' else 350","denom=700 if ident=='ST' else 1200 if ident=='UT' else 450 if ident=='1F' else 350")
s=s.replace("('SV','Хозяйственный',15)]","('SV','Хозяйственный',16),('DG2','Старый корпус 2 этаж',11),('UT','Подземные переходы',17)]")
# The extra old-pavilion sheet shifts IF and later location page numbers.
s=s.replace("('IF','Инфекционный',11),('PS','Психоневрология',12),('RH','Реабилитация',13),('HS','Персонал',14)","('IF','Инфекционный',12),('PS','Психоневрология',13),('RH','Реабилитация',14),('HS','Персонал',15)")
s=s.replace('В комплексе 450 ступеней','В комплексе 660 ступеней; добавлены лестницы шести боковых корпусов')
s=s.replace('Коридоры отделений 6 м; центральный ствол 8 м; поперечный холл 10 м; наружная галерея 4 м.','Плановые полосы сохранены; новые перегородки оставляют проходы примерно 3.38 м и 3.88 м, с нишами у дверей. Двери внутренних отсеков 1.40 м до выступа концов перегородок; наружная галерея 4 м.')
s=s.replace('«в свету» - без 0.16 м стен.','габарит без наружных стен не вычитает внутренние перегородки.')
s=s.replace("'В свету W × D'","'Без внешн. стен'")
register_prefix="    W,H=page(c,landscape(A3),'Реестр / '"
s=s.replace(register_prefix,"""    if ident=='UT':
        l=dict(l,rooms=[dict(id=a['id'],name=a['name'],rect=a['rect'],door=[a['rect'][0]+3.5,a['rect'][1]+11.4],enclosed=True) for a in DATA['infrastructure_zones']]+[dict(id='U_SPINE',name='Центральный сервисный тоннель',rect=[48,36,4,294],door=[50,36],enclosed=True)])
"""+register_prefix)
s=s.replace("assert sum(len(v) for v in CELLS.values())==128848","assert sum(len(v) for v in CELLS.values())>128848")
s=s.replace("len(ROOMS)==333 and len({r['id'] for r in ROOMS})==333","len(ROOMS)==334 and len({r['id'] for r in ROOMS})==334")
s=s.replace("f['kind']=='step'])==450","f['kind']=='step'])==660")
s=s.replace("assert len(reader.pages)==len(PAGE_INDEX)==29","assert len(reader.pages)==len(PAGE_INDEX)==35")
s=s.replace('rooms=333','rooms=334').replace('steps=450','steps=660').replace("'rooms':333","'rooms':334")
# Class and chapter overview adds two pages after complete room registers.
pos=s.index('c.save()')
s=s[:pos]+'''for block in (DATA['chapters'][:10],DATA['chapters'][10:]):
    W,H=page(c,landscape(A3),'Сюжет / главы и места','Пространственная привязка сценария. Сцены, диалоги и ИИ этих глав ещё не реализованы.','chapters_'+str(block[0]['id']))
    yy=H-140
    for chapter in block:
        txt(c,48,yy,f"{chapter['id']:02}  {chapter['title']}",12)
        yy=para(c,48,yy-19,', '.join(chapter['anchors']),W-110,9,12)-22
    para(c,48,70,'Сохранены 333 исходные зоны: 80 A / 120 B / 133 C. Скрытая RH00 добавлена отдельно. Глава 0 - свободный осмотр: все двери открыты. В данных есть типы дверей, окон, стен, питания, камер, телефонов и 12 узлов угроз.',W-100,9,13)
    c.showPage()
'''+s[pos:]
# Add window glyphs, short-cut connections and interior dimensions as vector art.
pos=s.index('def draw_features(')
s=s[:pos]+'''def draw_story(p,z,site):
    for w in DATA['windows']:
        if w['site']!=site:continue
        ox,oy,oz=LOC[site]['origin'];x=ox+w['at'][0]+w.get('offset',0);y=oy+w['at'][1]
        p.line(x-w['width']/2,y,x+w['width']/2,y,'#65acc0',1.0)
    for bay in DATA['subrooms']:
        if bay['site']!=site:continue
        ox,oy,_=LOC[site]['origin'];x,y,w,h=bay['rect']
        if w*p.s>36 and h*p.s>36:p.label(ox+x+w/2,oy+y+h/2,f"{w:.1f}x{h:.1f}",4.5,'#547b7d')
    if site=='UT':
        p.label(50,212,'Шлюз / главы 9-10',6)
        p.line(48,226,52,226,'#b23a44',1.2)
        for a in DATA['infrastructure_zones']:
            x,y,w,h=a['rect'];p.label(x+w/2,y+h/2,a['name'],6)
'''+s[pos:]
s=s.replace('if ident not in (\'ST\',\'RF\'):draw_rooms(p,l)',"if ident not in ('ST','RF'):draw_rooms(p,l)\n    draw_story(p,z,ident)")
s=s.replace('Где искать','Где искать / номера листов')
s=s.replace("p.label(50,-8,'ГЛАВНЫЙ ВХОД',7)","p.label(50,-26,'ГЛАВНЫЙ ВХОД',7)")
s=s.replace("['WardBookWorld.cpp','WardHospitalDetails.cpp']","['WardBookWorld.cpp','WardHospitalDetails.cpp','WardHospitalStory.cpp']")
# Coordinates of parking markings follow the relocated parking zone.
s=s.replace("x,y=-60+i*3.3,-178+row*18","x,y=-60+i*3.3,-76+row*18")
s=s.replace("p.rect([41+i-.275,-28,.55,6]","p.rect([41+i-.275,-34,.55,6]")
(ROOT/'tools/export_story_hospital_atlas.py').write_text(s,encoding='utf-8')
print('Prepared tools/export_story_hospital_atlas.py')
