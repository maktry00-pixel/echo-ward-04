"""Executed in the atlas renderer: layers, review sheets and embedded data."""
import shutil
from pypdf import PdfWriter

ART=ROOT/'artifacts/unreal_hospital_v4'
ESCAPE=json.loads((ART/'escape_distance_audit.json').read_text(encoding='utf-8'))
assert ESCAPE['success'] and ESCAPE['source_sha256']==VERIFIED['map_sha256']
V4_GREEN='#2d856c';V4_PURPLE='#89599e'
_old_draw_story=draw_story

def physical_review(p,z,site,tactical=False):
    # Members are drawn on their supporting floor, with beams dashed overhead.
    for s in DATA['structural_members']:
        at=s['at'];size=s['size'];base=at[2]-size[2]/2
        if s['kind']=='column' and abs(base-z)<.01:
            p.rect([at[0]-size[0]/2,at[1]-size[1]/2,*size[:2]],'#b23a44','#b23a44',.5)
        elif s['kind']=='beam' and z==0:
            p.rect([at[0]-size[0]/2,at[1]-size[1]/2,*size[:2]],None,'#bd872c',.6,True)
        elif s['kind']=='niche_wall' and abs(base-z)<.01:
            p.rect([at[0]-size[0]/2,at[1]-size[1]/2,*size[:2]],'#456d66')
    for h in DATA['hideouts']:
        if h['site']!=site:continue
        x,y,hz=h['at'];w,dep,hei=h['size']
        for r in ([x,y,w,.15],[x,y,.15,dep],[x+w-.15,y,.15,dep]):p.rect(r,'#2d856c')
        if tactical:p.label(x+w/2,y+dep+.25,'У',5,V4_GREEN)
    for n in DATA['escape_niches']:
        if n['site']!=site or n['entry'][2]!=z:continue
        x,y,_=n['entry'];ns=n['side'] in ('north','south')
        for sign in (-1,1):
            r=[x+sign*.725-.275,y-.11,.55,.22] if ns else [x-.11,y+sign*.725-.275,.22,.55]
            p.rect(r,V4_GREEN)
        if tactical:
            if ns:p.line(x-.45,y,x+.45,y,V4_GREEN,1.0)
            else:p.line(x,y-.45,x,y+.45,V4_GREEN,1.0)
    if tactical:
        for r in LOC[site]['rooms']:
            ox,oy,_=LOC[site]['origin'];x,y=r['door']
            if r['enclosed']:p.label(ox+x,oy+y-.4,'A' if r['production_class']=='A' else r['production_class'],5,'#a45d24')
            if r['refuge'] and r['enclosed']:
                p.rect([ox+x-.7,oy+y-.45,1.4,.9],None,'#ac383e',1)
            for e in r.get('extra_doors',[]):
                ex,ey=e['at'];col=V4_PURPLE if e['role']=='secret_maintenance' else '#ad4b38'
                if e['side'] in ('north','south'):p.line(ox+ex-.7,oy+ey,ox+ex+.7,oy+ey,col,1.0)
                else:p.line(ox+ex,oy+ey-.7,ox+ex,oy+ey+.7,col,1.0)
        for prop in DATA['movable_props']:
            if prop['site']!=site:continue
            x,y,z0=prop['at'];w,h,_=prop['size'];p.rect([x-w/2,y-h/2,w,h],'#dab67e','#947036')
        for dev in DATA['network_devices']:
            if dev['site']!=site:continue
            r=next(r for r in LOC[site]['rooms'] if r['id']==dev['zone']);ox,oy,_=LOC[site]['origin'];x,y=r['door'];south=r['door_side']=='south'
            dx={'phone':-1.9,'camera':-1.5,'power_switch':-1.05}[dev['type']]
            p.rect([ox+x+dx-.1,oy+y+(-.26 if south else .26)-.1,.2,.2],V4_PURPLE)

def draw_story(p,z,site):
    _old_draw_story(p,z,site)
    physical_review(p,z,site)

def table(c,headers,rows,widths,x=50,y=None,size=9,leading=15):
    if y is None:y=c._pagesize[1]-145
    positions=[x]
    for w in widths:positions.append(positions[-1]+w)
    c.setFillColor(HexColor('#e7efed'));c.rect(x,y-8,sum(widths),24,fill=1,stroke=0)
    for j,h in enumerate(headers):txt(c,positions[j]+5,y,h,size,INK)
    y-=29
    for row in rows:
        lines=[wrap(str(v),widths[j]-10,size) for j,v in enumerate(row)]
        height=max(map(len,lines))*leading+10
        if y-height<50:raise ValueError('Table needs another page')
        for j,ll in enumerate(lines):
            for k,s in enumerate(ll):txt(c,positions[j]+5,y-k*leading,s,size,INK)
        y-=height;c.setStrokeColor(HexColor('#d5e1df'));c.line(x,y+10,positions[-1],y+10)
    return y

def append_review_pages(c):
    # Base atlas pages 1-35 stay stable so room registers remain easy to find.
    W,H=page(c,landscape(A3),'Исправления / что включено в эту карту','Состояние проверенной игры от 22.10.2026. Экспорт PDF: 23.10.2026.','review_changes')
    rows=[('DG / DG2','Нижний зал, восемь опор и четыре балки под смещённым верхним крылом; лестница DG соединяет -4.8 / 0 / +4.8 м.'),
        ('Подземные переходы','Шесть соединительных веток к общей магистрали. SV24 доступна через лестницу SV и верхний коридор; это не отдельный тоннель на одной отметке.'),
        ('Лифт / E01','Единая шахта X48..52, Y4..10 на шести остановках. E01 - комната досмотра, а не другая шахта. N12 - холл, T12 - машинная.'),
        ('Выходы и классы','Исходные 80 A / 120 B / 133 C сохранены; RH00 дополнительно A. Закрытые A: не менее двух выходов/укрытий. B: один-два выхода и укрытие. У E20 убран третий выход.'),
        ('Укрытия и расстояния','271 комнатное укрытие, 1524 входа в боковые полосы и карманы коридоров. Это не 1524 новых комнаты. Вход 0.90 м. 88 парных соединений создают обходы.'),
        ('Убежища','IF08 / RH08 / DG08 / SV23 / HS08: физический засов изнутри по K; закрывает все входы и сохраняется. В главе 0 остальная карта открыта для осмотра.'),
        ('Проверка','Windows World 46/46 + WorldCapture 46/46; 15 свежих кадров 1920x1080. Требования данных: 23/23. Доступны 334 зоны и 1147 внутренних отсеков.')]
    y=table(c,['Изменение','Результат в карте'],rows,[160,930],size=10,leading=15)
    para(c,55,y-12,'Схемы и реестры описывают архитектурный прототип. Сюжетные привязки не означают готовые задания на 210 часов. Новые сцены и ИИ, а также физические обрушения B2 не реализованы.',1090,10,15)
    c.showPage()

    W,H=page(c,landscape(A2),'DG / DG2: опоры, пол и лестница','Два плана в общих мировых осях. Сетка 10 м. Нижний зал выделен зелёным.','review_DG_support')
    for site,box in [('DG',(60,150,720,870)),('DG2',(875,150,720,870))]:
        l=LOC[site];p=Plan(c,box,[-162,-4,118,108],450);p.clip();p.rect(p.bounds,'#f4f0e8');draw_surfaces(p,l['origin'][2])
        if site=='DG':
            for hall in ([-139,46,76,16],[-139,68,76,16]):p.rect(hall,'#dcece1')
        p.grid(10)
        draw_rooms(p,l);draw_walls(p,l['origin'][2]);draw_features(p,l['origin'][2],site);physical_review(p,l['origin'][2],site)
        p.c.restoreState();p.ruler(10)
        txt(c,box[0],H-130,site+' / Z='+fmt(l['origin'][2])+' м',15)
    para(c,65,103,'Старый корпус: верхние комнаты не перенесены и не уменьшены. Под ними устроен нижний зал. Колонны: 0.36 x 0.36 м; балки показаны пунктиром. Точная раскладка - во встроенном structural_members.csv.',1510,10,15)
    c.showPage()

    W,H=page(c,landscape(A2),'UT: все подземные связи и выход к SV24','М 1:1200. Красный - путь по тоннелю; золотой - продолжение на уровне 0 через лестницу SV.','review_UT_links')
    p=Plan(c,(50,100,1190,H-230),[-170,30,435,310],1200);p.clip();p.rect(p.bounds,'#f4f0e8');draw_surfaces(p,-4.8);p.grid(10);draw_walls(p,-4.8);physical_review(p,-4.8,'UT')
    for link in DATA['utility_links']:
        pts=link['points'];col='#bd872c' if link['target']=='SV24' else '#b23a44'
        for a,b in zip(pts,pts[1:]):p.line(a[0],a[1],b[0],b[1],col,1.4,a[2]!=b[2])
    sv=next(r for r in LOC['SV']['rooms'] if r['id']=='SV24');sx,sy,_=LOC['SV']['origin'];rx,ry,rw,rh=sv['rect']
    p.rect([sx+rx,sy+ry,rw,rh],None,'#bd872c',1.2)
    p.label(sx+rx+rw/2,sy+ry+rh/2,'SV24 / Z=0',7,'#865c12')
    upper=next(link for link in DATA['utility_links'] if link['target']=='SV24')
    ux,uy,_=upper['points'][0];p.label(ux,uy-4,'Лестница SV',7,'#865c12')
    for r in DATA['infrastructure_zones']:
        x,y,w,h=r['rect'];p.label(x+w/2,y+6,r['id'],8);p.label(x+w/2,y+10,dim(w,h),6)
    p.label(65,145,'U_SPINE 4 x 294 м',8);p.c.restoreState();p.ruler(20)
    yy=H-140
    for title,body in [('Связь с B1','Общая отметка Z=-4.8 м. Центральный тоннель начинается в главном корпусе и идёт до Y330.'),('Шесть веток','Все ветки имеют ширину 4 м. Подвалы DG/IF/PS/RH/HS/SV имеют по 60 x 18 м.'),('SV24','Подъём по SV с Z=-4.8 на Z=0, затем по коридору в SV24. Её координаты относятся к верхнему уровню.'),('Шлюз','В районе Y214..238 сохранены два обхода изоляционного шлюза. Карманы не закрывают боковые маршруты.')]:
        txt(c,1290,yy,title,13);yy=para(c,1290,yy-20,body,340,10,15)-24
    c.showPage()

    W,H=page(c,landscape(A3),'Лифт и убежища / единая вертикаль','Стопы, контрольные размеры и координаты пяти убежищ из оценки.','review_lift_refuges')
    rows=[]
    for stop in DATA['lift']['landings']:rows.append((stop['site'],fmt(stop['z']),'48..52 / 4..10',', '.join(map(fmt,stop['door']))))
    y=table(c,['Уровень','Пол Z, м','Шахта X / Y, м','Порог X, Y, Z'],rows,[100,110,220,230],size=10)
    para(c,55,y-15,'Размер шахты 4 x 6 м, кабина 3.80 x 5.80 м. Чистый размер 3.58 x 5.68 м. Просвет полностью открытой двери 3.06 м. E01 - досмотр перед доступом, а не положение шахты.',650,10,15)
    rows=[]
    for rid in ('IF08','RH08','DG08','SV23','HS08'):
        r=next(r for r in ROOMS if r['id']==rid);rows.append((rid,fmt(r['global_door'][0]),fmt(r['global_door'][1]),'K изнутри / все входы'))
    table(c,['Убежище','Вход X','Вход Y','Засов'],rows,[100,85,85,175],x=720,y=H-145,size=9)
    para(c,725,330,'При свободном осмотре все этажи доступны. В сюжетном режиме закрытая C-дверь остаётся запертой снаружи; двадцать обслуживающих проходов допускают открытие изнутри.',420,10,15)
    c.showPage()

    for site in ORDER:
        l=LOC[site];ox,oy,z=l['origin'];bx,by,bw,bh=l['bounds'];denom=700 if site=='ST' else 1200 if site=='UT' else 450 if site=='1F' else 350
        W,H=page(c,landscape(A2),site+' / выходы, укрытия и сети',f'М 1:{denom}; Z={fmt(z)} м. Показаны первичные/вторые двери, реальные карманы и комнатные укрытия.','safety_'+site)
        p=Plan(c,(55,95,1080,H-220),[ox+bx-6,oy+by-6,bw+12,bh+12],denom);p.clip();p.rect(p.bounds,COLORS['void']);draw_surfaces(p,z);p.grid(5);draw_walls(p,z);draw_rooms(p,l);draw_features(p,z,site);_old_draw_story(p,z,site);physical_review(p,z,site,True);p.c.restoreState();p.ruler(10)
        x=1180;y=H-140
        for title,body in [('Как читать','Зелёный просвет: 0.90 м в боковую нишу. Зелёная П-образная рамка с У: комнатное укрытие. Кирпично-красный: второй выход. Фиолетовый выход: секретный проход C.'),('Обозначения','A/B/C у основного входа - класс зоны. Красная рамка - убежище с засовом. Малые фиолетовые точки - камера/телефон/выключатель. Опоры DG выделены красным; балки - пунктиром.'),('Координаты','План использует мировые X/Y. Точные пороги, типы, размеры, главы и все ID приведены во вложенных CSV и JSON. Для внутренних палат используйте основной поэтажный план и реестр отсеков.')]:
            txt(c,x,y,title,13);y=para(c,x,y-22,body,445,10,15)-25
        hides=sum(h['site']==site for h in DATA['hideouts']);niches=sum(n['site']==site for n in DATA['escape_niches']);shorts=sum(s['site']==site for s in DATA['shortcuts']);nets=sum(n['site']==site for n in DATA['network_devices'])
        y=para(c,x,y,f'На листе: {len(l["rooms"])} зон; {hides} комнатных укрытий; {niches} входов в карманы; {shorts} парных связей; {nets} сетевых устройств.',445,12,19)-20
        cov=next((s for s in ESCAPE['per_site'] if s['site']==site),None)
        if cov:y=para(c,x,y,f'Плотная проверка: {cov["samples"]} стоячих точек коридоров; верхняя оценка пути до двери/ниши максимум {fmt(cov["maximum_upper_m"])} м. Глава 0; шаг сетки 0.5 м.',445,11,17)-20
        if site in ('ST','RF'):para(c,x,y,'Открытая территория/крыша: правило комнатных укрытий и коридорной плотности на наружные площадки не переносится. Наружные участки сохраняют свои ID и размеры.',445,10,15)
        c.showPage()

    W,H=page(c,landscape(A3),'Состояния B2 / повторные посещения и маршруты','Состояния и привязки данных; это не заявление о готовой сюжетной кампании.','review_states')
    table(c,['Глава','Состояние','Реализовано'],[('0-5','clean / чисто','Свободный архитектурный осмотр; исходные оттенки.'),('6-14','infected / заражено','Состояние всех 24 зон B2; зелёный/приглушённый оттенок стен.'),('15-17','damaged / повреждено','Состояние и иной оттенок материалов; полноценные обломки не добавлены.'),('18-19','collapsed / разрушено','Финальная метка состояния и тёмный оттенок. Геометрия физического обрушения ещё не реализована.')],[85,220,785],size=10)
    rows=[]
    for n in (6,7,10,15):
        ch=next(c for c in DATA['chapters'] if c['id']==n);rows.append((str(n),ch['title'],', '.join(ch['route_zones'])))
    table(c,['Глава','Название','Расширенная последовательность зон'],rows,[70,230,790],y=H-430,size=9,leading=13)
    c.showPage()

    W,H=page(c,landscape(A3),'Сети и проверка / откуда взяты размеры','Слои реальной карты, проверенные пути и ограничения результата.','review_data')
    rows=[('Двери','508 записей закрытых зон: основные и дополнительные; общая стена может быть описана со стороны обеих комнат.'),('Окна',str(len(DATA['windows']))+' записей; расположение и размеры из текущего JSON.'),('Устройства','69: телефоны, камеры и выключатели. Сегменты/связи и звуковые шины во вложенном реестре.'),('Узлы угроз','12 узлов; привязки к главам и сетям заданы. Новый ИИ ещё не реализован.'),('Энергоснабжение',' -> '.join([DATA['power_graph'][0]['source']]+[p['target'] for p in DATA['power_graph']])),('Расстояния','179396 точек на сетке 0.5 м; Manhattan-путь по свободному полу + 1 м запаса до порога. Максимум 15 м.'),('Границы проверки','Глава 0, фиксированная архитектура. Не непрерывное пространство, не будущие запирания и не передвинутые баррикады.'),('Управление','B - яркий свет; F - дверь/лифт/предмет; Shift+F - тянуть предмет; K - засов внутри убежища; M/Tab - план.')]
    table(c,['Слой','Содержание'],rows,[175,915],size=10,leading=15)
    c.showPage()

    W,H=page(c,landscape(A3),'Вложения / полный набор данных карты','Все схемы и реестры относятся к одному проверенному JSON игры.','review_attachments')
    y=para(c,55,H-140,'Этот PDF содержит векторные планы всех 15 локаций, 334 зоны с размерами, внутренние перегородки и дополнительные слои. Вложения внутри PDF позволяют изучить каждую дверь, нишу и инженерный объект без многосотстраничных таблиц.',1090,12,19)-20
    rows=[('01-06','Реестры зон, покрытий, стен/перемычек, инженерии, внутренних отсеков, классов и глав.'),('CSV слоёв','Двери, окна, сети, узлы угроз, укрытия, карманы, конструкции, тоннели, состояния, маршруты, правила монстров и предметы.'),('JSON','Полный hospital_world_source.json; manifest.json, verification.json и escape_distance_audit.json.'),('Примечания','REPORT.md и CHECKLIST.md: выполненные исправления и ограничения. CSV: UTF-8 BOM, разделитель ;, десятичная точка.'),('Открытие вложений','В настольном PDF-просмотрщике открой панель вложений/скрепку. Некоторые встроенные браузерные просмотры не показывают вложения; планы и краткие реестры читаются в любом просмотрщике.')]
    y=table(c,['Вложено','Что содержит'],rows,[150,940],y=y,size=10,leading=15)-20
    y=para(c,55,y,'SHA-256 геометрии: '+VERIFIED['map_sha256'],1090,9,14)-10
    y=para(c,55,y,'SHA-256 Windows EXE: '+VERIFIED['executable_sha256'],1090,9,14)-10
    para(c,55,y,'Пакет проверен: '+VERIFIED['checked_at']+'. Документ не меняет карту игры. Предыдущие PDF оставлены историческими снимками.',1090,10,15)
    c.showPage()

def embed_review_attachments(pdf):
    for f in ART.iterdir():
        if f.suffix=='.csv' or f.name in ('verification.json','escape_distance_audit.json','REPORT.md','CHECKLIST.md'):
            shutil.copy2(f,OUT/f.name)
    reader=PdfReader(pdf);writer=PdfWriter();writer.clone_document_from_reader(reader)
    attachments=[f for f in OUT.iterdir() if f.suffix in ('.csv','.json','.md')]
    for f in sorted(attachments):writer.add_attachment(f.name,f.read_bytes())
    temp=TMP/'atlas_attached.pdf'
    with temp.open('wb') as stream:writer.write(stream)
    temp.replace(pdf)
    check=PdfReader(pdf)
    assert len(check.pages)==57 and len(check.attachments)==len(attachments)
