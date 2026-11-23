"""Export an annotated, to-scale atlas from the actual campaign coordinates."""
from pathlib import Path
import csv
import hashlib
import json
import math
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor, Color, white
from reportlab.lib.pagesizes import A2, A3, landscape
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from pypdf import PdfReader

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'unreal/EchoWard/Content/CampaignData/campaign.json'
OUT = ROOT / 'output/pdf/echo_game_atlas'
OUT.mkdir(parents=True, exist_ok=True)
DATA = json.loads(SOURCE.read_text('utf8'))
FONT = ROOT / 'assets/ui/fonts/NotoSans.ttf'
pdfmetrics.registerFont(TTFont('Ward', str(FONT)))
INK = HexColor('#15282e')
MUTED = HexColor('#536a71')
RED = HexColor('#b52637')
SECTORS = [
    ('ward', '01 / Отделение 04', '#e0edf0'),
    ('research', '02 / Исследовательское и жилое крыло', '#e2e8f3'),
    ('diag', '03 / Диагностика и эвакуация', '#e8e2f1'),
    ('iso', '04 / Изолятор', '#f1e5dc'),
    ('annex', '05 / Хозяйственный корпус', '#e3ecdd'),
    ('basement', '06 / Подвал', '#dfe9e9'),
]

def sector(r):
    i = r['id']
    if r['level'] == -1: return 'basement'
    if i.startswith('annex_'): return 'annex'
    if i.startswith('iso_'): return 'iso'
    if i.startswith('diag_') or i == 'evacuation': return 'diag'
    if i.startswith(('res_', 'morgue_', 'lab_', 'theatre_')) or i in ['research_hall', 'stairs']: return 'research'
    return 'ward'

ROOMS = sorted(DATA['rooms'], key=lambda r: ([s[0] for s in SECTORS].index(sector(r)), r['rect'][1], r['rect'][0], r['id']))
for n, r in enumerate(ROOMS, 1):
    r['code'] = f'R{n:02}'
    r['sector'] = sector(r)
ROOM_BY_ID = {r['id']: r for r in ROOMS}

def xy(position): return position[1] / 100, -position[0] / 100
def dims(r): return f"{r['rect'][2]:.2f} × {r['rect'][3]:.2f} м".replace('.00', '')
def locate(position):
    x, y = xy(position)
    floor = -1 if position[2] < 0 else 0
    found = []
    for r in ROOMS:
        a, b, w, h = r['rect']
        if r['level'] not in [floor, -2]: continue
        if a - .02 <= x <= a+w+.02 and b-.02 <= y <= b+h+.02: found.append(r)
    return min(found, key=lambda r: r['rect'][2]*r['rect'][3]) if found else None

NAMES = {
    'map':'План отделения', 'weapon':'Служебный пистолет', 'fuse':'Предохранитель',
    'badge':'Пропуск сотрудника', 'power':'Аварийный щиток', 'terminal':'Терминал выхода',
    'fuel':'Топливо', 'coolant':'Хладагент', 'reactor':'Реактор',
    'purge_valve_a':'Клапан продувки A','purge_valve_b':'Клапан продувки B',
    'residence_panel':'Панель питания жилого крыла','residence_prybar':'Монтажный инструмент',
    'residence_core':'Модуль лифта','residence_lift':'Лифт','residence_plate':'Ручная пластина лифта',
    'upgrade_shotgun':'Помповый дробовик','refuge_save':'Сохранение в убежище',
    'refuge_stash':'Шкаф припасов','refuge_records':'Записи в убежище',
    'power_gate':'Дверь с электроблокировкой','quarantine_gate':'Карантинная дверь',
    'technical_gate':'Ворота в нижний сектор','research_gate':'Вход в исследовательское крыло',
    'service_entry':'Вход в щитовую','service_shortcut':'Короткий путь: щитовая',
    'archive_shortcut':'Короткий путь: архив','supply_room':'Дверь медсклада',
    'refuge_door':'Дверь убежища','final_passage':'Проход к эвакуации',
    'residence_lock':'Разрушаемый замок','chapter_observation_glass':'Стекло наблюдательной',
    'chapter_service_sheet':'Разрушаемая перегородка','isolation_glass':'Стекло изолятора',
}
KIND = {'door':'Дверь','mission':'Задача / предмет','station':'Узел','record':'Запись','bed':'Укрытие под кроватью','save':'Сохранение','medkit':'Аптечка','battery':'Батарея','partition':'Преграда'}

def name(o):
    i = o['id']
    if i in NAMES: return NAMES[i]
    if i.startswith('ammo_') or 'ammo' in i: return 'Запас патронов'
    if 'shells' in i: return 'Патроны дробовика'
    if 'boards' in i: return 'Доски'
    if i.startswith('dial_'): return 'Переключатель контура '+i.rsplit('_',1)[-1]
    if i.startswith('relay_'): return 'Реле: '+{'morgue':'морг','lab':'лаборатория','theatre':'операционная'}.get(i[6:],i[6:])
    if i.startswith('upgrade_record_'): return 'Аудиозапись: '+{'shift':'смена','evidence':'улика','ceiling':'потолок','farewell':'прощание'}.get(i[15:],i[15:])
    caption = o.get('properties', {}).get('caption','')
    if caption: return caption.capitalize()
    if o['kind']=='record': return 'Запись '+i.split('_')[-1]
    if o['kind']=='save': return 'Точка сохранения'
    return KIND[o['kind']]

OBJECTS = []
for n, o in enumerate(DATA['objects'],1):
    obj = dict(o, code=f'{n:03}', label=name(o))
    room = locate(o['transform']['position'])
    obj['room_code'] = room['code'] if room else 'вне зоны'
    OBJECTS.append(obj)

def text(c, x,y,s,size=10,color=INK):
    c.setFillColor(color);c.setFont('Ward',size);c.drawString(x,y,str(s))

def wrap(s, width, size):
    lines=[];line=''
    for word in s.split():
        t=(line+' '+word).strip()
        if line and pdfmetrics.stringWidth(t,'Ward',size)>width: lines.append(line);line=word
        else:line=t
    return lines+[line] if line else lines

def paragraph(c,x,y,s,width,size=10,leading=None,color=MUTED):
    leading=leading or size*1.4
    for line in wrap(s,width,size):text(c,x,y,line,size,color);y-=leading
    return y

def header(c,size,title,subtitle,page):
    W,H=size;c.setPageSize(size);c.setFillColor(HexColor('#f7faf9'));c.rect(0,0,W,H,fill=1,stroke=0)
    text(c,40,H-45,'ЭХО / ПОЛНАЯ КАРТА КАМПАНИИ',11,MUTED)
    text(c,40,H-80,title,24)
    text(c,40,H-104,subtitle,10,MUTED)
    c.setStrokeColor(HexColor('#bacbcc'));c.line(40,H-117,W-40,H-117)
    text(c,40,23,'Источник: campaign.json / текущая версия проекта / размеры в метрах',8,MUTED)
    text(c,W-100,23,f'Лист {page:02}',8,MUTED)

def symbol(c,x,y,code,kind,small=False,locked=False):
    radius=3 if small else 5
    colors={'door': '#347e88','mission':'#bc3445','station':'#b27821','record':'#7352a3','save':'#238552','medkit':'#238552','battery':'#238552','bed':'#7b8b92','partition':'#98572b'}
    col=RED if locked else HexColor(colors[kind]);c.setStrokeColor(col);c.setFillColor(white);c.setLineWidth(.8)
    if kind=='door':c.line(x-4,y,x+4,y)
    elif kind=='bed':c.rect(x-radius,y-radius/2,radius*2,radius,fill=1,stroke=1)
    else:c.circle(x,y,radius,fill=1,stroke=1)
    if not small:
        text(c,x+7,y-2,code,6.5,col)

def map_draw(c,rooms,box,markers=True,overview=False,floor=0):
    bx,by,bw,bh=box
    minx=min(r['rect'][0] for r in rooms)-2;miny=min(r['rect'][1] for r in rooms)-2
    maxx=max(r['rect'][0]+r['rect'][2] for r in rooms)+2;maxy=max(r['rect'][1]+r['rect'][3] for r in rooms)+2
    scale=min(bw/(maxx-minx),bh/(maxy-miny));ox=bx+(bw-(maxx-minx)*scale)/2;oy=by+(bh-(maxy-miny)*scale)/2
    def point(x,y):return ox+(x-minx)*scale,oy+(maxy-y)*scale
    c.saveState();path=c.beginPath();path.rect(bx,by,bw,bh);c.clipPath(path,stroke=0)
    # Metric grid. Drawing orientation follows the in-game plan, not geographic north.
    c.setLineWidth(.25);c.setStrokeColor(HexColor('#dce5e4'))
    for x in range(math.ceil(minx/10)*10,math.ceil(maxx/10)*10,10):
        xx,_=point(x,miny);c.line(xx,by,xx,by+bh)
    for y in range(math.ceil(miny/10)*10,math.ceil(maxy/10)*10,10):
        _,yy=point(minx,y);c.line(bx,yy,bx+bw,yy)
    # Small embedded areas (e.g. the doctor's office) remain visible above parent zones.
    for r in sorted(rooms,key=lambda r:r['rect'][2]*r['rect'][3],reverse=True):
        a,b,w,h=r['rect'];x,y=point(a,b+h)
        color=next(s[2] for s in SECTORS if s[0]==r['sector'])
        c.setFillColor(HexColor(color));c.setStrokeColor(HexColor('#526d73'));c.setLineWidth(.9);c.rect(x,y,w*scale,h*scale,fill=1,stroke=1)
    occupied=[]
    for r in rooms:
        a,b,w,h=r['rect'];x,y=point(a,b+h);rw=w*scale;rh=h*scale
        label=f"{r['code']}  {r['name']}";size=7 if overview else 10
        rotate=rw<52 and rh>rw*2
        c.saveState()
        if rotate:c.translate(x+rw/2,y+rh/2);c.rotate(90);tx=-rh/2+4;ty=1;available=rh-8;maxh=rw-3
        else:
            inset=8*scale if r['id']=='diag_xray' else 0
            c.translate(x+3,y+rh-12-inset);tx=0;ty=0;available=rw-6;maxh=rh-4
        while size>5 and (len(wrap(label,available,size))*size*1.25+size*1.4>maxh):size-=.5
        lines=wrap(label,available,size)
        height=(len(lines)+1)*size*1.2
        labeltop=height/2 if rotate else 0
        for j,line in enumerate(lines):text(c,tx,ty+labeltop-j*size*1.2,line,size)
        text(c,tx,ty+labeltop-len(lines)*size*1.2,dims(r),max(4.5,size-1),MUTED)
        if not rotate:occupied.append((x+2,y+rh-12-height,rw-4,height+13))
        c.restoreState()
    if markers:
        x0,y0=xy(DATA['start']);ex,ey=xy(DATA['exit'])
        valid_ids={r['id'] for r in rooms}
        for o in OBJECTS:
            if overview and o['kind']!='door':continue
            p=o['transform']['position'];x,y=xy(p)
            if (-1 if p[2]<0 else 0)!=floor or not (minx<=x<=maxx and miny<=y<=maxy):continue
            room=locate(p)
            if room and room['id'] not in valid_ids:continue
            px,py=point(x,y)
            def overlaps(a,b):return a[0]<b[0]+b[2] and a[0]+a[2]>b[0] and a[1]<b[1]+b[3] and a[1]+a[3]>b[1]
            if not overview and o['kind']!='door' and any(overlaps((px-5,py-5,10,10),r) for r in occupied):
                original=(px,py)
                for dx,dy in [(0,-18),(16,-16),(-16,-16),(0,-30),(24,-30),(-24,-30)]:
                    rect=(px+dx-5,py+dy-5,10,10)
                    if not any(overlaps(rect,r) for r in occupied):
                        px+=dx;py+=dy;c.setLineWidth(.4);c.setStrokeColor(MUTED);c.line(*original,px,py);break
            symbol(c,px,py,'',o['kind'],overview,o.get('properties',{}).get('initial_locked',False))
            if not overview:
                width=pdfmetrics.stringWidth(o['code'],'Ward',6.5)
                options=[(7,-2),(7,7),(-width-7,-2),(7,-10),(-width-7,7),(12,15),(-width-12,-15)]
                choice=options[0]
                for dx,dy in options:
                    rect=(px+dx-1,py+dy-1,width+2,9)
                    if bx<=rect[0] and rect[0]+rect[2]<=bx+bw and by<=rect[1] and rect[1]+rect[3]<=by+bh and not any(overlaps(rect,r) for r in occupied):choice=(dx,dy);break
                dx,dy=choice;occupied.append((px+dx-1,py+dy-1,width+2,9));text(c,px+dx,py+dy,o['code'],6.5,MUTED)
        if floor==0:
            for caption,p,color in [('СТАРТ',DATA['start'],HexColor('#116b3b')),('ВЫХОД',DATA['exit'],RED)]:
                x,y=xy(p)
                if minx<=x<=maxx and miny<=y<=maxy:
                    px,py=point(x,y);c.setFillColor(color);c.circle(px,py,4,fill=1,stroke=0);text(c,px+7,py+6,caption,7,color)
    c.restoreState()
    # Scale bar is measured in the current panel, so it remains valid on every sheet.
    length=5 if maxx-minx<50 else 10
    xx,yy=bx+8,by+8;c.setLineWidth(2);c.setStrokeColor(INK);c.line(xx,yy,xx+length*scale,yy);c.line(xx,yy-3,xx,yy+3);c.line(xx+length*scale,yy-3,xx+length*scale,yy+3);text(c,xx,yy+6,f'{length} м',7)

PDF=OUT/'ЭХО_полная_карта_игры.pdf'
c=canvas.Canvas(str(PDF));c.setTitle('ЭХО - полная 2D-карта игры');c.setAuthor('Карта из данных кампании ЭХО')
page=1;W,H=A2
header(c,A2,'Все 54 помещения / общий план','Вид сверху. Цветом выделены игровые секторы. Полные размеры и содержимое - на следующих листах.',page)
upper=[r for r in ROOMS if r['level'] in [0,-2]]
map_draw(c,upper,(42,130,W-315,H-265),overview=True)
sx=W-245;y=H-155
for key,title,color in SECTORS:
    c.setFillColor(HexColor(color));c.rect(sx,y-8,13,13,fill=1,stroke=0);y=paragraph(c,sx+22,y,title,175,10)-16
y=paragraph(c,sx,y-15,'R01-R54: номера помещений. На увеличенных листах 001-199 обозначают игровые объекты.',205,10)
y=paragraph(c,sx,y-18,'Главный уровень: отметка 0 м. Подвал: объекты на отметке -5 м. Лестница связывает уровни; её зона показана на обоих планах.',205,10)
y=paragraph(c,sx,y-18,'Размеры взяты из прямоугольников игровых зон. Это не обмер внутренней поверхности стен. Кабинет врача вложен в зону рентгена.',205,10)
y=paragraph(c,sx,y-18,'Монстры перемещаются: их текущее положение не нанесено. Предметы и замки показаны по исходным данным, до прохождения.',205,10)
text(c,sx,y-30,'КЛЮЧЕВЫЕ МЕСТА',11)
y-=55
for key in ['map','weapon','fuse','badge','upgrade_shotgun','refuge_save','coolant','fuel']:
    o=next(o for o in OBJECTS if o['id']==key)
    y=paragraph(c,sx,y,f"{o['room_code']} / {o['label']}",205,9)-7
paragraph(c,sx,y-15,'Красный штрих - изначально запертая дверь; бирюзовый - дверь без замка. Номера объектов читаются на увеличенных листах.',205,9)
text(c,sx,500,'ПОДВАЛ / отдельный уровень',11)
map_draw(c,[r for r in ROOMS if r['level'] in [-1,-2]],(sx,160,205,315),overview=True,floor=-1)
text(c,45,100,'Ориентация сохранена как на игровом плане. Координаты и привязка всех объектов - в приложениях.',10,MUTED)
c.showPage()

for key,title,color in SECTORS:
    page+=1;size=landscape(A3);W,H=size
    header(c,size,title,'Номера R - помещения; 001-199 - объекты. Двери и проёмы показаны маркерами, а не строительным чертежом.',page)
    rooms=[r for r in ROOMS if r['sector']==key]
    if key=='basement':rooms=rooms+[ROOM_BY_ID['stairs']]
    map_draw(c,rooms,(40,105,760,H-245),overview=False,floor=-1 if key=='basement' else 0)
    x=830;y=H-150;text(c,x,y,'ПОМЕЩЕНИЯ / РАЗМЕР ЗОНЫ',11);y-=25
    for r in rooms:
        y=paragraph(c,x,y,f"{r['code']}  {r['name']}",310,10,14,color=INK)
        text(c,x,y,dims(r),9,MUTED);y-=24
    paragraph(c,x,126,'Найди номер объекта в реестре: там указаны название, помещение и координаты. Положение предметов может зависеть от уже выполненных задач.',310,9)
    x=42;y=70
    for k,label in [('mission','Предмет / задача'),('station','Узел'),('record','Запись'),('save','Сохранение'),('bed','Укрытие'),('door','Дверь')]:
        symbol(c,x+3,y+3,'',k,True);text(c,x+13,y,label,8);x+=145
    c.showPage()

# Exact room register. Bounds use game-plan axes in metres.
for offset in range(0,len(ROOMS),27):
    page+=1;size=landscape(A3);W,H=size
    header(c,size,'Размеры всех помещений',f'R{offset+1:02}-R{min(offset+27,len(ROOMS)):02} / ширина × глубина, площадь зоны и положение на плане',page)
    cols=[40,100,390,560,680,820];y=H-145
    for x,label in zip(cols,['Код','Название','Размер, м','Площадь, м²','Левый верхний угол, м','Игровой ID']):text(c,x,y,label,9)
    y-=22
    for i,r in enumerate(ROOMS[offset:offset+27]):
        if i%2==0:c.setFillColor(HexColor('#eaf0ef'));c.rect(40,y-6,W-80,21,fill=1,stroke=0)
        a,b,w,h=r['rect'];values=[r['code'],r['name'],dims(r),f'{w*h:.2f}',f'x {a:.2f}; y {b:.2f}',r['id']]
        for x,value in zip(cols,values):text(c,x,y,value,8.5)
        y-=22
    paragraph(c,40,63,'Площадь = ширина × глубина прямоугольной зоны. Вложенные / пересекающиеся зоны нельзя суммировать как полезную площадь здания. Для лестницы указан общий прямоугольник обоих уровней.',W-80,9)
    c.showPage()

for offset in range(0,len(OBJECTS),37):
    page+=1;size=landscape(A3);W,H=size
    header(c,size,'Что где находится / реестр объектов',f'Объекты {offset+1:03}-{min(offset+37,len(OBJECTS)):03} из {len(OBJECTS)}. Координаты в метрах относительно игрового плана.',page)
    cols=[40,82,400,500,690,835];y=H-142
    for x,label in zip(cols,['№','Название','Зона','Координаты x / y / z','Тип / состояние','Игровой ID']):text(c,x,y,label,9)
    y-=20
    for i,o in enumerate(OBJECTS[offset:offset+37]):
        if i%2==0:c.setFillColor(HexColor('#eaf0ef'));c.rect(40,y-5,W-80,17,fill=1,stroke=0)
        p=o['transform']['position'];x,ycoord=xy(p);state=KIND[o['kind']]
        if o['kind']=='door':state='Замок' if o.get('properties',{}).get('initial_locked',False) else 'Без замка'
        values=[o['code'],o['label'],o['room_code'],f'{x:.2f} / {ycoord:.2f} / {p[2]/100:.2f}',state,o['id']]
        widths=[35,312,92,182,137,315]
        for xx,v,width in zip(cols,values,widths):
            fs=8
            while pdfmetrics.stringWidth(v,'Ward',fs)>width and fs>6:fs-=.25
            text(c,xx,y,v,fs)
        y-=17
    paragraph(c,40,57,'«Вне зоны» означает, что точка объекта не попала ни в один экспортированный прямоугольник. У дверей на границе указана одна из прилегающих зон. Доступность меняется по сюжету.',W-80,8)
    c.showPage()
c.save()

with (OUT/'rooms.csv').open('w',encoding='utf-8-sig',newline='') as f:
    writer=csv.writer(f,delimiter=';');writer.writerow(['Код','Название','ID','Уровень','X, м','Y, м','Ширина, м','Глубина, м','Площадь, м²'])
    for r in ROOMS:writer.writerow([r['code'],r['name'],r['id'],r['level'],*r['rect'],r['rect'][2]*r['rect'][3]])
with (OUT/'objects.csv').open('w',encoding='utf-8-sig',newline='') as f:
    writer=csv.writer(f,delimiter=';');writer.writerow(['Номер','Название','ID','Тип','Зона','X, м','Y, м','Z, м'])
    for o in OBJECTS:
        p=o['transform']['position'];writer.writerow([o['code'],o['label'],o['id'],o['kind'],o['room_code'],*xy(p),p[2]/100])
reader=PdfReader(PDF)
assert len(ROOMS)==54 and len(OBJECTS)==199
assert len({r['code'] for r in ROOMS})==54
alltext='\n'.join(p.extract_text() for p in reader.pages)
assert all(r['id'] in alltext for r in ROOMS)
assert all(o['id'] in alltext for o in OBJECTS)
report={'success':True,'rooms':len(ROOMS),'objects':len(OBJECTS),'pdf_pages':len(reader.pages),'source_sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),'source':str(SOURCE),'pdf':str(PDF),'unassigned_objects':[o['id'] for o in OBJECTS if o['room_code']=='вне зоны'],'note':'Dimensions are exported rectangular gameplay zones; object positions are authored initial positions.'}
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),'utf8')
print(json.dumps(report,ensure_ascii=False))
