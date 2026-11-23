"""Current as-built atlas, using the shared metric drawing primitives."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
base=(root/'tools/export_story_hospital_atlas.py').read_text(encoding='utf-8').split("PDF=OUT/")[0]
base=base.replace('output/pdf/ECHO_Hospital_v3','output/pdf/ECHO_Hospital_v5').replace('tmp/pdfs/hospital_story_v3','tmp/pdfs/hospital_expansion_v5')
start=base.index("DATA=json.loads(SOURCE.read_text('utf-8')); MARKER=")
end=base.index("FONT=ROOT/",start)
base=base[:start]+'''DATA=json.loads(SOURCE.read_text('utf-8'))
VERIFIED=json.loads((ROOT/'artifacts/unreal_hospital_v5/verification.json').read_text(encoding='utf-8'))
assert VERIFIED['success'] and VERIFIED['map_sha256']==hashlib.sha256(SOURCE.read_bytes()).hexdigest()
assert DATA['review_revision']==5
MARKER=dict(success=True,world_id=DATA['map_id'],build_identity=VERIFIED['build_identity'])
'''+base[end:]
base=base.replace("idx,not r['enclosed'])","idx if r['enclosed'] else -1,not r['enclosed'])")
base=base.replace("CELLS[z][a,b]=(room,outdoor)","CELLS[z][a,b]=(room,outdoor)\n            elif room<0 and CELLS[z][a,b][1] and not outdoor:CELLS[z][a,b]=(-1,False)")
needle='# Exact wall centrelines'
pos=base.index(needle)
base=base[:pos]+'''for n in DATA['escape_niches']:PORTALS.append((*n['entry'][:2],n['entry'][2],n['side'],2))
for e in DATA['site_entries']:PORTALS.append((*e['at'][:2],e['at'][2],e['side'],e['width']))
PORTAL_BINS=defaultdict(list)
for px,py,pz,side,pw in PORTALS:PORTAL_BINS[(pz,'NS' if side in ('north','south') else 'EW',py if side in ('north','south') else px)].append((px,py,pz,side,pw))
'''+base[pos:]
base=base.replace('for px,py,pz,side,pw in PORTALS)',"for px,py,pz,side,pw in PORTAL_BINS.get((z,'NS' if dy else 'EW',my if dy else mx),[]))")
base=base.replace('01_Все_334_зоны_метры.csv','01_Все_562_зоны_метры.csv')
base=base.replace('hospital_story_shell_v3 / метры / 22.10.2026','revision 5 / метры / 23.10.2026')
base=base.replace("for f in FEATURES:\n        if f['kind']=='step'", "for f in FEATURES:\n        bx,by,bw,bh=p.bounds\n        if f['x']+f['w']<bx or f['x']>bx+bw or f['y']+f['h']<by or f['y']>by+bh:continue\n        if f['kind']=='step'")
exec(compile(base,__file__,'exec'),globals())

import shutil
from collections import Counter
from pypdf import PdfWriter
ART=ROOT/'artifacts/unreal_hospital_v5'
AUDIT=json.loads((ART/'acceptance.json').read_text(encoding='utf-8'))
assert AUDIT['success'] and AUDIT['source_sha256']==VERIFIED['map_sha256']
PDF=OUT/'ЭХО_карта_v5_новые_этажи_тоннели_укрытия.pdf'
c=canvas.Canvas(str(PDF),pagesize=landscape(A3),pageCompression=1)
c.setTitle('ЭХО / Полная карта v5 / 562 зоны / 35 локаций')
c.setAuthor('ЭХО / карта из проверенной игры')

def rowtable(headers,rows,widths,x=48,y=None,size=9):
    if y is None:y=c._pagesize[1]-145
    cols=[x]
    for w in widths:cols.append(cols[-1]+w)
    c.setFillColor(HexColor('#e0eeeb'));c.rect(x,y-8,sum(widths),25,fill=1,stroke=0)
    for j,h in enumerate(headers):txt(c,cols[j]+5,y,h,size)
    y-=30
    for rr in rows:
        lines=[wrap(str(t),widths[j]-12,size) for j,t in enumerate(rr)];height=max(map(len,lines))*14+13
        assert y-height>45,('table_overflow',rr)
        for j,ll in enumerate(lines):
            for k,line in enumerate(ll):txt(c,cols[j]+5,y-k*14,line,size)
        y-=height;c.setStrokeColor(HexColor('#cfdfdb'));c.line(x,y+11,cols[-1],y+11)
    return y

def physical(p,z,site):
    for s in DATA['structural_members']:
        at=s['at'];w,h,hei=s['size'];bottom=at[2]-hei/2
        if abs(bottom-z)<.01:p.rect([at[0]-w/2,at[1]-h/2,w,h],'#6f8b7e')
    for n in DATA['escape_niches']:
        if n['entry'][2]!=z:continue
        ex,ey,_=n['entry'];ns=n['side'] in ('north','south')
        for sign in (-1,1):p.rect([ex+sign*.725-.275,ey-.11,.55,.22] if ns else [ex-.11,ey+sign*.725-.275,.22,.55],'#31856f')
    for h in DATA['hideouts']:
        if h['site']!=site:continue
        x,y,_=h['at'];w,dep,_=h['size'];col='#a47e45' if h['kind']=='under_bed' else '#428b75'
        p.rect([x,y,w,dep],None,col,.7)
    for f in DATA['furnishings']:
        if f['site']!=site:continue
        x,y,_=f['at'];w,dep,_=f['size'];p.rect([x-w/2,y-dep/2,w,dep],'#9caaaf')
    ox,oy,_=LOC[site]['origin']
    for r in LOC[site]['rooms']:
        for e in r['extra_doors']:
            x,y=e['at'];x+=ox;y+=oy;col='#9464a3' if e['role']=='secret_maintenance' else '#bb5c4d'
            if e['side'] in ('north','south'):p.line(x-1,y,x+1,y,col,.8)
            else:p.line(x,y-1,x,y+1,col,.8)

W,H=page(c,landscape(A3),'Полная карта / все исправления в игре','Версия 5. Геометрия, этажи, предметы и проходы из текущей Windows-сборки.','guide')
y=para(c,50,H-145,'562 зоны / 35 локаций / 37 лестничных ядер / 20 новых этажей',1080,20,28,INK)-20
y=para(c,50,y,'Все 334 прежние зоны и их площади сохранены. Дополнительно зарегистрированы два нижних зала DG и новые этажи корпусов, складов и уличных зданий. PDF содержит поэтажные векторные планы, размеры всех зон и встроенные реестры CSV/JSON.',1080,11,17)-22
rowtable(['Листы','Содержание'],[
    ('Начало','Общий план территории, затем все 35 локаций: полы, стены, двери, инженерия, мебель и укрытия.'),
    ('Реестры','Все 562 ID, название, мировые X/Y/Z, ширина, глубина, площадь и класс A/B/C.'),
    ('В конце','Новые этажи, секретные проходы, подсчёт петель, состояния глав и проверки проходимости.'),
    ('Вложения PDF','Полный исходный JSON игры, CSV дверей/мебели/укрытий/тоннелей/классов/монстров, отчёт и результаты проверки.')],[150,940],y=y,size=10)
para(c,50,120,'Масштаб рассчитан для печати без подгонки листа. При просмотре увеличивай векторный PDF. Все размеры в метрах. Это архитектурный прототип с физической мебелью; новые сюжетные сцены и ИИ не заявлены готовыми.',1090,10,15)
c.showPage()

W,H=page(c,A2,'Весь комплекс / территория и первый этаж','Основание 460 x 570 м. Асфальт, зелёные вставки и реальные крытые связи.','campus')
p=Plan(c,(60,140,W-120,H-255),[-190,-200,480,590],1250);p.clip();p.rect(p.bounds,COLORS['void']);draw_surfaces(p,0,True);p.grid(25);draw_walls(p,0);markings(p);draw_features(p,0,overview=True)
for l in DATA['locations']:
    if l['origin'][2]!=0:continue
    ox,oy,_=l['origin'];bx,by,bw,bh=l['bounds'];p.label(ox+bx+bw/2,oy+by+bh/2,l['id'],9)
p.c.restoreState();p.ruler(50)
txt(c,65,100,'Голубой: плиточный пол и крытая галерея. Серый: асфальт/кровля. Зелёный: вставки S05/S15.',10)
txt(c,65,78,'Верхние этажи и подземные связи приведены на отдельных листах.',10)
c.showPage()

ORDER=[l['id'] for l in DATA['locations']]
for site in ORDER:
    l=LOC[site];ox,oy,z=l['origin'];bx,by,bw,bh=l['bounds']
    if site=='UT':bx,by,bw,bh=-164,-184,432,528
    bounds=[ox+bx-5,oy+by-5,bw+10,bh+10]
    # Fit the entire site in a consistent plan panel and round up the scale.
    denom=math.ceil(max((bw+10)*2834.645669/1080,(bh+10)*2834.645669/920)/50)*50
    denom=max(200,denom)
    W,H=page(c,landscape(A2),site+' / '+l['name'],f'М 1:{denom} / Z={fmt(z)} м / начало X={fmt(ox)}, Y={fmt(oy)} м','plan_'+site)
    p=Plan(c,(55,100,1080,H-220),bounds,denom);p.clip();p.rect(bounds,COLORS['void']);draw_surfaces(p,z);p.grid(10 if site=='UT' else 5)
    draw_walls(p,z);draw_features(p,z,site);physical(p,z,site);draw_rooms(p,l)
    if site=='ST':markings(p)
    if site=='UT':
        for u in DATA['utility_links']:
            if u['target']=='SV24':continue
            for a,b in zip(u['points'],u['points'][1:]):p.line(a[0],a[1],b[0],b[1],'#b23a44',1)
        for a in DATA['infrastructure_zones']:
            x,y,w,h=a['rect'];p.label(x+w/2,y+h/2,a['id'],6)
    p.c.restoreState();p.ruler(20 if site=='UT' else 10)
    sy=H-135
    for r in l['rooms']:
        w,h=r['rect'][2:];label=f'{r["id"]}  / {r["production_class"]} / {fmt(w)} x {fmt(h)} м / {fmt(w*h)} м²'
        txt(c,1180,sy,label,9);sy-=18
    sy=min(sy-16,390)
    for title,body in [('Обозначения','Зелёный - ниша/шкаф; охра - койка; кирпичный - второй выход; фиолетовый - секретный проход C; серые детали - мебель.'),
                       ('Покрытия','Голубое: пол корпуса и крытые галереи. Серое: асфальт/кровля. Бежевое пустое поле: на этой высоте пола нет.'),
                       ('Размеры','Площадь зоны включает внутренние перегородки и оборудование. Чистые размеры отдельных палат и кабинетов - во вложенном CSV 05.')]:
        txt(c,1180,sy,title,11);sy=para(c,1180,sy-18,body,440,9,13)-20
    c.showPage()

for site in ORDER:
    l=LOC[site];ox,oy,z=l['origin'];rows=l['rooms']
    if site=='UT':rows=[dict(id=r['id'],name=r['name'],rect=r['rect'],production_class='инфр.') for r in DATA['infrastructure_zones']]
    for offset in range(0,len(rows),22):
        W,H=page(c,landscape(A3),'Реестр / '+site+f' / часть {offset//22+1}','Глобальные координаты, метры. Габариты и площади сохранены из JSON игры.','register_'+site+'_'+str(offset))
        rr=[]
        for r in rows[offset:offset+22]:
            x,y,w,h=r['rect'];rr.append((r['id'],r['name'],f'{fmt(ox+x)}; {fmt(oy+y)}; {fmt(z)}',f'{fmt(w)} x {fmt(h)}',fmt(w*h),r['production_class']))
        rowtable(['ID','Название','X; Y; Z','W x D, м','Площадь, м²','Класс'],rr,[100,420,200,130,130,110],size=8.5)
        c.showPage()

W,H=page(c,landscape(A3),'Исправления / что изменилось','Всё перечисленное относится к физической карте и текущим данным, кроме явно указанного ИИ.','changes')
rowtable(['Пункт','Результат'],[
('Слушатель','B / finale_only. Это классификация правил; новый ИИ и финальная схватка ещё не реализованы.'),
('Секретные проходы','20 проходов по 9 локациям, максимум 3 в одной. Все относятся к закрытым комнатам C.'),
('PS и RH','В PS 15 комнат B и 1 A; в RH 14 B и 2 A. Побочные истории имеют доступное пространство.'),
('Петли','Отдельно считаются уникальные пары смежных комнат с общей стеной и возвратом через коридор. Две двери одного фасада за отдельную петлю не выдаются.'),
('Мебель','451 медицинский шкафчик, 226 шкафов-укрытий и 219 коек с проходом для пригнувшегося персонажа. Сохранены архитектурные ниши и пять перемещаемых прототипов.'),
('Нижний DG','DG25 и DG26 зарегистрированы как A, имеют несколько выходов, укрытия и состояния. Старые 334 ID сохранены.'),
('Тоннели','Изломы шести веток; общий южный контур и подвалы всех 11 уличных зданий, включая электрощитовую S11 и пост S16. Маршрут SV24 на Z=0 идёт по крытому коридору.'),
('Новые этажи','DG / IF / RH / SV - по три этажа; PS / HS - по два. Супермаркет X01 и склад X04 - по три; остальные выбранные уличные здания - по два. Два независимых лестничных направления.')],[140,950],size=9)
c.showPage()

W,H=page(c,landscape(A3),'Новые этажи / координаты и связи','Лестницы находятся на одной оси по высоте; переходы сделаны без телепортации.','floors')
rowtable(['Этаж','Здание','Z, м','Зон'],[(f['site'],f['building'],fmt(f['z']),str(len(LOC[f['site']]['rooms']))) for f in DATA['floor_expansion']],[240,300,200,350],size=10)
c.showPage()

W,H=page(c,landscape(A3),'Секреты и петли / проверяемый подсчёт','Секретные проходы остаются внутренними служебными дверями класса C.','loops')
dist=Counter(s['site'] for s in DATA['secret_passages'])
y=rowtable(['Локация','Секретов','ID'],[(s,str(n),', '.join(p['zone'] for p in DATA['secret_passages'] if p['site']==s)) for s,n in sorted(dist.items())],[150,120,820],size=10)
para(c,52,y-20,'Петли по этажам: '+', '.join(f'{a["site"]}: {a["independent_room_pairs"]}' for a in DATA['loop_audit'])+'. Полные пары и способ подсчёта - в loop_audit.csv.',1090,10,15)
c.showPage()

W,H=page(c,landscape(A3),'Проходимость / проверка в игре и по главам','Физические испытания и расчёт расстояний разделены, чтобы не подменять одно другим.','verification')
y=rowtable(['Проверка','Результат'],[
('Windows',f'{VERIFIED["world_checks"]} физических проверок: все 562 маршрута, все лестничные ядра, входы/выходы, укрытия, лифт, сохранения.'),
('Изображения',f'35 свежих кадров 1920x1080; {VERIFIED["visual_initial_checks"]} начальных проверок геометрии с отрисовкой.'),
('Главы 0 / 1 / 6 / 12 / 17 / 19','В пределах 15 м по свободному полу от всех проверенных стоячих точек коридоров до открытой двери/ниши; закрытые двери не считаются укрытием.'),
('Метод','Сетка 0.5 м, радиус капсулы 0.35 м, путь Manhattan +1 м запаса до порога. Физические лестницы проверены отдельно в Unreal.'),
('Улица / крыша','Отдельный отчёт на сетке 2 м: показаны участки открытой экспозиции. Правило внутренних коридоров не выдаётся за сплошное покрытие всей территории убежищами.'),
('Ограничения','Мебель пока состоит из физических прототипов. Нет нового ИИ, полной сюжетной кампании и физических обрушений B2; состояния B2 меняют материалы.')],[155,935],size=10)
para(c,52,y-18,'SHA-256 карты: '+VERIFIED['map_sha256'],1090,9,14)
c.showPage()

W,H=page(c,landscape(A3),'Вложения / полный набор данных','Все вложения относятся к той же геометрии и проверенной сборке.','attachments')
para(c,52,H-145,'Открой панель вложений (значок скрепки) в настольном PDF-просмотрщике. В PDF вложены исходная карта JSON, все реестры CSV, отчёт и результаты проверок. Браузерный просмотрщик может показывать только страницы, без панели вложений.',1090,12,19)
para(c,52,H-250,'Во вложениях есть полные координаты дверей, секрeтных проходов, мебели, ниш, лестниц, тоннелей, сетей, сюжетных классов и правил монстров. Основные планы остаются векторными и читаются при увеличении.',1090,11,18)
para(c,52,160,'Карта: '+VERIFIED['map_sha256']+'\nWindows EXE: '+VERIFIED['executable_sha256'],1090,9,15)
c.showPage();c.save()

shutil.copy2(SOURCE,OUT/'hospital_world_source.json')
for f in ART.iterdir():
    if f.suffix=='.csv' or f.name in ('REPORT.md','CHECKLIST.md','verification.json','acceptance.json'):
        shutil.copy2(f,OUT/f.name)
manifest=dict(revision=5,rooms=len(ROOMS),locations=len(LOC),pages=len(PAGE_INDEX),source_sha256=VERIFIED['map_sha256'],page_index=PAGE_INDEX)
(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
reader=PdfReader(PDF);alltext='\n'.join(p.extract_text() or '' for p in reader.pages)
for r in ROOMS:assert r['id'] in alltext and r['name'] in alltext,r['id']
writer=PdfWriter();writer.clone_document_from_reader(reader)
for f in sorted(OUT.iterdir()):
    if f.suffix in ('.csv','.json','.md'):writer.add_attachment(f.name,f.read_bytes())
temp=TMP/'attached.pdf'
with temp.open('wb') as stream:writer.write(stream)
temp.replace(PDF)
print(json.dumps(dict(pdf=str(PDF),pages=len(PAGE_INDEX),rooms=len(ROOMS),attachments=len(PdfReader(PDF).attachments)),ensure_ascii=False))
