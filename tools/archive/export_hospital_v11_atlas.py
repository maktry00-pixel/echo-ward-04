"""Read-only metric atlas of the verified v11 game, including embedded registers."""
import collections,csv,hashlib,io,json,math,zipfile
from pathlib import Path
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A2,A3,landscape
from reportlab.lib.colors import HexColor
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from pypdf import PdfReader,PdfWriter

ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'output/pdf/ECHO_Map_v11';TMP=ROOT/'tmp/pdfs/atlas_v11'
OUT.mkdir(parents=True,exist_ok=True);TMP.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json';D=json.loads(SOURCE.read_text(encoding='utf-8'))
V=json.loads((ROOT/'artifacts/unreal_hospital_v11/verification.json').read_text(encoding='utf-8'))
SHA=hashlib.sha256(SOURCE.read_bytes()).hexdigest();assert V['success'] and V['map_sha256']==SHA and D['review_revision']==11
assert SOURCE.read_bytes()==(ROOT/'unreal/EchoWard/Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
pdfmetrics.registerFont(TTFont('Ward',str(ROOT/'unreal/EchoWard/Content/CampaignData/Fonts/NotoSans.ttf')))
INK='#18383c';MUTED='#576f73';ACCENT='#ad4248';TILE='#e4f1f0';VOID='#f2f0e9';ASPHALT='#b5bdc5';GRASS='#a6c88a';BLUE='#4a91aa';GOLD='#b37d36'
PDF=OUT/'ЭХО_карта_v11_все_изменения.pdf';C=canvas.Canvas(str(TMP/'body.pdf'),pageCompression=1)
C.setTitle('ЭХО - полная карта v11 со всеми изменениями');C.setAuthor('ЭХО / атлас актуальной игровой сборки')
LOC={s['id']:s for s in D['locations']};ROOMS=[dict(r,site=s['id'],origin=s['origin']) for s in D['locations'] for r in s['rooms']]
WALLS=collections.defaultdict(list)
for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):WALLS[w['z']].append(w['rect'])
FLOORS=collections.defaultdict(list)
for s in D['locations']:
    ox,oy,z=s['origin']
    for r in s['rooms']:FLOORS[z].append(([ox+r['rect'][0],oy+r['rect'][1],*r['rect'][2:]],TILE if r['enclosed'] else ASPHALT))
    for r in s['corridors']+[c['rect'] for c in s['cores']]:FLOORS[z].append(([ox+r[0],oy+r[1],*r[2:]],ASPHALT if s['id'] in ('ST','RF') else TILE))
for x,y,w,h,z in D['passages']:FLOORS[z].append(([x,y,w,h],TILE))
FURNITURE=collections.defaultdict(list);FURN_COUNT=collections.Counter();SMALL_COUNT=collections.Counter();DETAIL_COUNT=collections.Counter()
for p in D['prototype_props']+D['environment_props']:
    FURN_COUNT[p['zone']]+=1
    if p['mount']!='floor':continue
    b=D['prototype_catalog'][str(p['model'])]['bounds'];w,h=b[3]-b[0],b[4]-b[1]
    if p['turn']%2:w,h=h,w
    x,y,z=p['at'];FURNITURE[z].append(([x-w/2,y-h/2,w,h],p['model'],p['zone']))
for i in D['clutter_dressing']['items']:SMALL_COUNT[i['zone']]+=1
for i in D['microdetails_dressing']['items']:DETAIL_COUNT[i['zone']]+=1
ENV=collections.defaultdict(list)
for b in D['environment_dressing']['boxes']:
    if b.get('layer') in (10,11):continue
    ENV[b['kind']].append(b)
PAGES=[];REGISTERED=[];SITE_PAGES={}

def f(v):return str(round(v)) if abs(v-round(v))<1e-7 else f'{v:.3f}'.rstrip('0').rstrip('.')
def color(v):return HexColor(v)
def text(x,y,s,size=9,fill=INK,align='left'):
    C.setFont('Ward',size);C.setFillColor(color(fill));getattr(C,{'left':'drawString','center':'drawCentredString','right':'drawRightString'}[align])(x,y,str(s))
def wrap(s,width,size=9):
    result=[]
    for paragraph in str(s).split('\n'):
        words=paragraph.split();line=''
        for word in words:
            q=(line+' '+word).strip()
            if line and pdfmetrics.stringWidth(q,'Ward',size)>width:result.append(line);line=word
            else:line=q
        result.append(line)
    return result
def para(x,y,s,width,size=10,leading=15):
    for line in wrap(s,width,size):text(x,y,line,size);y-=leading
    return y
def page(title,sub='',size=None,key=None):
    size=size or landscape(A3);C.setPageSize(size);w,h=size;num=len(PAGES)+1;key=key or 'p'+str(num)
    C.setFillColor(color('#ffffff'));C.rect(0,0,w,h,fill=1,stroke=0)
    C.setFillColor(color(INK));C.rect(0,h-88,w,88,fill=1,stroke=0)
    C.setFillColor(color(ACCENT));C.rect(35,h-69,4,44,fill=1,stroke=0)
    text(53,h-43,title,23,'#ffffff');text(53,h-67,sub,9,'#c8dfdc')
    C.setStrokeColor(color('#cbd9d6'));C.setLineWidth(.5);C.line(35,40,w-35,40)
    text(35,23,'ЭХО / v11 / 23.10.2026 / метры / снимок проверенной игры',8,MUTED)
    text(w-35,23,str(num).zfill(2),9,INK,'right')
    C.bookmarkPage(key);C.addOutlineEntry(title,key,level=0,closed=False);PAGES.append(dict(page=num,title=title,key=key));return w,h
def end():C.showPage()
def table(headers,rows,widths,x=45,y=None,size=8.2):
    if y is None:y=C._pagesize[1]-125
    total=sum(widths);C.setFillColor(color('#dfedeb'));C.rect(x,y-7,total,23,fill=1,stroke=0)
    xx=x
    for head,w in zip(headers,widths):text(xx+5,y,head,size);xx+=w
    y-=29
    for row in rows:
        lines=[wrap(str(v),w-10,size) for v,w in zip(row,widths)];height=max(len(a) for a in lines)*(size+3)+10
        assert y-height>55,('table_overflow',row)
        xx=x
        for ll,w in zip(lines,widths):
            for k,line in enumerate(ll):text(xx+5,y-k*(size+3),line,size)
            xx+=w
        y-=height;C.setStrokeColor(color('#dce5e2'));C.setLineWidth(.3);C.line(x,y+size+6,x+total,y+size+6)
    return y

class Plan:
    def __init__(self,box,extent,scale=None):
        self.box=box;self.extent=extent
        x,y,w,h=box;bx,by,bw,bh=extent
        if scale is None:scale=max(50,math.ceil(max(bw*2834.645669/w,bh*2834.645669/h)/50)*50)
        self.denom=scale;self.s=2834.645669/scale;self.ox=x+(w-bw*self.s)/2;self.oy=y+(h-bh*self.s)/2
    def at(self,x,y):bx,by,bw,bh=self.extent;return self.ox+(x-bx)*self.s,self.oy+(by+bh-y)*self.s
    def visible(self,r):a,b,w,h=self.extent;x,y,ww,hh=r;return x+ww>=a and x<=a+w and y+hh>=b and y<=b+h
    def rect(self,r,fill=None,stroke=None,lw=.4):
        if not self.visible(r):return
        x,y,w,h=r;px,py=self.at(x,y+h);C.setFillColor(color(fill or '#ffffff'));C.setStrokeColor(color(stroke or '#ffffff'));C.setLineWidth(lw)
        C.rect(px,py,w*self.s,h*self.s,fill=bool(fill),stroke=bool(stroke))
    def line(self,a,b,stroke=INK,lw=.5):C.setStrokeColor(color(stroke));C.setLineWidth(lw);C.line(*self.at(*a),*self.at(*b))
    def label(self,x,y,s,size=7,fill=INK):text(*self.at(x,y),s,size,fill,'center')
    def start(self):
        C.saveState();a,b,w,h=self.extent;px,py=self.at(a,b+h);path=C.beginPath();path.rect(px,py,w*self.s,h*self.s);C.clipPath(path,stroke=0)
        self.rect(self.extent,VOID)
    def finish(self,step=10):
        C.restoreState();x,y,w,h=self.box;a,b,bw,bh=self.extent
        text(x,y-20,f'М 1:{self.denom} при печати 100%.  +X вправо, +Y вниз.',8,MUTED)
        px=x+w-140;py=y-16;length=step*self.s
        if length>130:length/=2;step/=2
        C.setStrokeColor(color(INK));C.setLineWidth(1.3);C.line(px,py,px+length,py)
        for k in (0,step/2,step):C.line(px+k*self.s,py-3,px+k*self.s,py+3);text(px+k*self.s,py+6,f(k),7,INK,'center')
        text(px+length+8,py-2,'м',7)
    def grid(self,step):
        x,y,w,h=self.extent
        for xx in range(math.ceil(x/step)*step,math.floor((x+w)/step)*step+1,step):self.line((xx,y),(xx,y+h),'#d2ded9',.2)
        for yy in range(math.ceil(y/step)*step,math.floor((y+h)/step)*step+1,step):self.line((x,yy),(x+w,yy),'#d2ded9',.2)

def terrain(p):
    p.rect([-180,-190,460,570],ASPHALT)
    for r in D['environment_dressing']['lawns']:p.rect(r,GRASS)
    for b in ENV['pavement']:
        x,y,_=b['at'];w,h,_=b['size'];p.rect([x-w/2,y-h/2,w,h],'#e1e0d5')
    for r in ROOMS:
        if r['site']=='ST' and r['id'] in ('S05','S15'):
            x,y,w,h=r['rect'];ox,oy,_=r['origin'];p.rect([ox+x+.4,oy+y+.4,w-.8,h-.8],GRASS)
    for g in D['garden_paths']:p.rect(g['rect'],'#e1e0d5')

def architecture(p,z,site=None,furnished=True,overview=False):
    if z==0:terrain(p)
    # Indoor corridors cover the campus substrate; enclosed room floors retain
    # priority over broad outdoor ST rectangles, as in the game's cell map.
    for r,col in FLOORS[z]:
        if col==ASPHALT:p.rect(r,col)
    for r,col in FLOORS[z]:
        if col==TILE:p.rect(r,col)
    # Green paint is above the street floor in the actual runtime.
    if z==0:
        for r in D['environment_dressing']['lawns']:p.rect(r,GRASS)
        for g in D['garden_paths']:p.rect(g['rect'],'#e1e0d5')
        for r in ROOMS:
            if r['site']=='ST' and r['id'] in ('S05','S15'):
                x,y,w,h=r['rect'];ox,oy,_=r['origin'];p.rect([ox+x+.4,oy+y+.4,w-.8,h-.8],GRASS)
    p.grid(25 if overview else 10 if site=='UT' else 5)
    for r in WALLS[z]:p.rect(r,INK)
    if furnished and not overview:
        for r,model,zone in FURNITURE[z]:
            col='#b8d6df' if model in (1,2,3,31,32,33,34) else '#dcc8a4' if model in (16,17,18,19,23,24,25,26,27,28,29) else '#afbcc4'
            p.rect(r,col,'#667e86',.25)
        for h in D['hideouts']:
            if abs(h['at'][2]-z)>.01:continue
            x,y,_=h['at'];w,dep,_=h['size'];p.rect([x,y,w,dep],None,'#348268' if h['kind']!='under_bed' else GOLD,.65)
        for f0 in D['furnishings']:
            if abs(f0['at'][2]-z)>.01:continue
            x,y,_=f0['at'];w,h,_=f0['size'];p.rect([x-w/2,y-h/2,w,h],'#aebac2')
        for n in D['escape_niches']:
            if abs(n['entry'][2]-z)<.01:p.rect(n['rect'],None,'#348268',.45)
    for s in D['stairs']:
        if not s['z_min']<=z<=s['z_max']:continue
        r=s['rect'];p.rect(r,'#f4d7a0','#ac7a34',.6)
        if not overview:
            x,y,w,h=r
            for i in range(14):p.line((x+.12,y+2.6+i*.28),(x+2.88,y+2.6+i*.28),'#ad8545',.25)
            p.label(x+1.5,y+7.5,s['id'],5.5)
    if z in (-9.6,-4.8,0,4.8,9.6,14.4):
        p.rect([48,4,4,6],'#adcfdc','#36778e',.8)
        if not overview:p.label(50,7,'ЛИФТ',5.5)
    for s in D['locations']:
        if s['origin'][2]!=z:continue
        ox,oy,_=s['origin']
        for r in s['rooms']:
            for door in [dict(at=r['door'],side=r['door_side'],type=r['door_type'])]+r['extra_doors']:
                x,y=ox+door['at'][0],oy+door['at'][1];ns=door['side'] in ('north','south');width=door.get('width',2)
                col='#9563a9' if door.get('role')=='secret_maintenance' else '#b7544e' if door.get('role') in ('shortcut','loop') else BLUE
                p.line((x-width/2,y) if ns else (x,y-width/2),(x+width/2,y) if ns else (x,y+width/2),col,.75 if not overview else .3)
    if z==0:
        for b in ENV['lamp_post']:
            x,y,_=b['at']
            if p.visible([x,y,0,0]):p.rect([x-.2,y-.2,.4,.4],GOLD)
        for b in ENV['road_dash']:
            x,y,_=b['at'];w,h,_=b['size'];p.rect([x-w/2,y-h/2,w,h],'#f9f5df')
    for win in D['windows']:
        s=LOC[win['site']]
        if s['origin'][2]!=z:continue
        x,y=s['origin'][0]+win['at'][0],s['origin'][1]+win['at'][1];ns=win['side'] in ('north','south');off=win.get('offset',0);width=win['width']
        if ns:x+=off
        else:y+=off
        if p.visible([x-width/2,y-width/2,width,width]):p.line((x-width/2,y) if ns else (x,y-width/2),(x+width/2,y) if ns else (x,y+width/2),'#66b9c3',.5)
    if site=='RF':
        p.rect([29,31,34,34],None,'#ffffff',1.4);p.rect([42.5,40.5,.9,15],'#ffffff');p.rect([49.5,40.5,.9,15],'#ffffff');p.rect([42.5,47.55,7.9,.9],'#ffffff')
    if site=='UT':
        for u in D['utility_links']:
            for a,b in zip(u['points'],u['points'][1:]):
                if abs(a[2]-z)<.01 and abs(b[2]-z)<.01:p.line(a[:2],b[:2],ACCENT,.65)
        for r in D['infrastructure_zones']:
            x,y,w,h=r['rect'];p.label(x+w/2,y+h/2,r['id'],6)

def room_labels(p,s,full=False):
    ox,oy,z=s['origin']
    for r in s['rooms']:
        x,y,w,h=r['rect'];x+=ox;y+=oy
        if not p.visible([x,y,w,h]):continue
        if not r['enclosed']:p.rect([x,y,w,h],None,'#718489',.45)
        label=r['id'];size=7.5 if w*p.s>60 else 6
        px,py=p.at(x+w/2,y+h/2)
        lines=[label,f'{f(w)} x {f(h)} м']
        tw=max(pdfmetrics.stringWidth(a,'Ward',size if j==0 else 5.6) for j,a in enumerate(lines))+6
        C.setFillColor(color('#ffffff'));C.rect(px-tw/2,py-11,tw,24,fill=1,stroke=0)
        text(px,py+3,lines[0],size,INK,'center');text(px,py-7,lines[1],5.6,MUTED,'center')

def csv_file(name,fields,rows):
    p=OUT/name
    with p.open('w',encoding='utf-8-sig',newline='') as stream:
        writer=csv.DictWriter(stream,fieldnames=fields,delimiter=';',extrasaction='ignore');writer.writeheader();writer.writerows(rows)
    return p

# Cover and reading contract.
w,h=page('ЭХО / полная карта со всеми изменениями','Атлас актуальной версии 11. Данные совпадают с запускаемой игровой сборкой.','' or landscape(A3),'cover')
text(48,h-145,'562 зоны  /  35 локаций  /  37 лестничных узлов',23)
y=para(48,h-182,'Территория 460 x 570 м. Все этажи от B2 до крыши, отдельные корпуса, магазины, склады, улица и соединяющие подземные тоннели.',1090,13,20)
y=para(48,y-20,'Включены мебель и оборудование, газоны и дороги, свет, камеры, укрытия, сюжетные привязки, 15 727 вещей и ещё 15 082 мелкие детали последних обновлений.',1090,12,19)
table(['Раздел','Что находится внутри'],[
 ('Планы','Генплан, общие планы пяти других высот и отдельные планы всех 35 локаций. Векторные чертежи можно увеличивать.'),
 ('Размеры','ID и габариты на схемах; полный реестр 562 зон с мировыми координатами, площадями и количеством наполнения.'),
 ('Мелкие помещения','1 939 внутренних секций и их точные размеры приведены во встроенном CSV, отдельно от крупных сюжетных зон.'),
 ('Полный снимок','Во вложении PDF находится ZIP с исходной картой JSON и реестрами мебели, вещей, инженерии, маршрутов, оружия, записок и мобов.')], [150,940],y=y-25,size=10)
para(48,150,'Единицы: метры и м². X направлен вправо, Y вниз; это оси игры, не географический север. Размер W x D относится к границам зоны и не равен чистой площади после вычета перегородок. Класс A/B/C - производственная категория, не сложность.',1090,10,15)
para(48,85,'Численный масштаб действует при печати 100%. При подгонке листа пользуйся масштабной линейкой. Мебель и мелкие детали пока прототипные; весь сюжет не заявлен завершённым.',1090,9,14);end()

# Complete horizontal slices of the campus, before the local detailed sheets.
for z,title in [(0,'Генплан / территория и первый этаж'),(-9.6,'Общий план B2 / экспериментальная зона'),(-4.8,'Общий план B1 / морг, инженерия и тоннели'),(4.8,'Общий план второго уровня'),(9.6,'Общий план третьего уровня'),(14.4,'Общий план крыши')]:
    w,h=page(title,f'Отметка пола Z={f(z)} м. Незакрашенные участки: на этой высоте нет показанного пола.',A2,'global_'+str(z))
    p=Plan((45,120,w-90,h-240),[-184,-194,468,578]);p.start();architecture(p,z,overview=True)
    for s in D['locations']:
        if s['origin'][2]!=z:continue
        ox,oy,_=s['origin'];bx,by,bw,bh=s['bounds']
        if s['id']=='ST':
            for r in s['rooms']:
                x,y,rw,rh=r['rect'];p.label(ox+x+rw/2,oy+y+rh/2,r['id'],6)
        elif s['id']!='UT':p.label(ox+bx+bw/2,oy+by+bh/2,s['id'],12)
    p.finish(50)
    text(48,70,'Зелёный: газон. Серый: асфальт/открытая кровля. Голубой: пол корпуса и крытая связь. Охра: лестница. Синий: лифт.',9)
    end()

# Local plans: exact existing wall intervals, physical furniture, exits.
for s in D['locations']:
    ident=s['id'];ox,oy,z=s['origin'];bx,by,bw,bh=s['bounds'];extent=[ox+bx-4,oy+by-4,bw+8,bh+8]
    w,h=page(ident+' / '+s['name'],f'План из игровой геометрии. Z={f(z)} м; начало локации X={f(ox)}, Y={f(oy)} м.',landscape(A2),'site_'+ident)
    SITE_PAGES[ident]=len(PAGES);p=Plan((45,100,1100,h-220),extent);p.start();architecture(p,z,ident);room_labels(p,s);p.finish(20 if ident=='UT' else 10)
    yy=h-122;text(1195,yy,'Зоны / название / габариты',13);yy-=26
    for r in s['rooms']:
        rw,rh=r['rect'][2:];name=r['name'];label=f'{r["id"]} / {name} / {f(rw)} x {f(rh)} м'
        while pdfmetrics.stringWidth(label,'Ward',8)>w-1240:
            name=name[:-1];label=f'{r["id"]} / {name}... / {f(rw)} x {f(rh)} м'
        text(1195,yy,label,8);yy-=19
    yy=min(yy-20,450)
    for title,body in [
        ('Наполнение','Контуры: кровати и медтехника - голубой; столы и места ожидания - бежевый; шкафы и стеллажи - серо-синий. Зелёная рамка - укрытие или ниша.'),
        ('Двери и окна','Голубой штрих - основной проход. Красноватый - дополнительный выход. Фиолетовый - секретный служебный проход. Бирюзовый - окно.'),
        ('Мелкие детали','Вещи на столах и полках не увеличены до размеров мебели. Их точные координаты, типы и принадлежность опоре - во встроенных реестрах; крупные планы есть в конце.'),
        ('Координаты','Габариты зон подписаны на схеме и в реестре. Сетка - 5 м, для UT - 10 м. Показаны также фрагменты соседних переходов в границах листа.')]:
        text(1195,yy,title,11);yy=para(1195,yy-18,body,w-1240,9,13)-19
    end()

# All registered zones, including infrastructure halls in a separate section.
for s in D['locations']:
    rows=s['rooms'];ox,oy,z=s['origin']
    for offset in range(0,len(rows),18):
        w,h=page('Реестр зон / '+s['id']+f' / {offset//18+1}',f'План локации: лист {SITE_PAGES[s["id"]]}. Координаты мировые, метры; площадь W x D.',key='register_'+s['id']+'_'+str(offset))
        rr=[]
        for r in rows[offset:offset+18]:
            x,y,rw,rh=r['rect'];REGISTERED.append(r['id']);rr.append((r['id'],r['name'],f'{f(ox+x)}; {f(oy+y)}; {f(z)}',f'{f(rw)} x {f(rh)}',f(rw*rh),r['production_class'],f'{FURN_COUNT[r["id"]]} / {SMALL_COUNT[r["id"]]} / {DETAIL_COUNT[r["id"]]}'))
        table(['ID','Название','X; Y; Z','W x D, м','м²','Класс','М / В / Д'],rr,[90,370,175,110,75,60,210],size=8.1)
        para(48,85,'М / В / Д: экземпляры каталожной мебели и оборудования / декоративные вещи этапа 10 / мелкие детали этапа 11. Это три разных слоя, не три оценки площади. Старые укрытия и инженерные детали учтены в отдельных реестрах.',1090,9,14);end()

w,h=page('Технические подвалы и связь корпусов','17 дополнительных инфраструктурных залов, отдельно от 562 именованных зон.');table(['ID','Назначение','X; Y; Z','Габариты, м'],[(r['id'],r['name'],f'{f(r["rect"][0])}; {f(r["rect"][1])}; {f(r["z"])}',f'{f(r["rect"][2])} x {f(r["rect"][3])}') for r in D['infrastructure_zones']],[150,420,270,250],size=9);end()

for title,rows in [('Вертикальные связи / все лестницы',D['stairs'])]:
    for start in range(0,len(rows),18):
        w,h=page(title+f' / {start//18+1}','Реальные марши в игре. Положение шахты лифта постоянно: X=48..52, Y=4..10 м.');table(['Узел','X; Y','W x D, м','Низ Z','Верх Z'],[(r['id'],f'{f(r["rect"][0])}; {f(r["rect"][1])}',f'{f(r["rect"][2])} x {f(r["rect"][3])}',f(r['z_min']),f(r['z_max'])) for r in rows[start:start+18]],[230,260,240,180,180],size=10);end()

w,h=page('Что изменено / актуальный состав v11','Все этапы включены в один снимок карты. Геометрия не уменьшена.')
y=table(['Слой','Включено в актуальную карту'],[
 ('Геометрия','562 зоны, 35 локаций, 20 добавленных этажей, 37 лестничных узлов, лифт B2 - крыша. Закрыты найденные щели оболочек, добавлены окна, рамы и отделка.'),
 ('Планировка','Подвалы у всех выбранных корпусов и уличных зданий; разветвлённые тоннели; несколько входов; дополнительные и секретные проходы; шкафы, койки и ниши для укрытия.'),
 ('Мебель','10 978 экземпляров исходного плана и ещё 2 696 предметов следующего этапа. Сохранены старые физические шкафы, мебельные укрытия и перемещаемые прототипы.'),
 ('Улица',f'{sum(r[2]*r[3] for r in D["environment_dressing"]["lawns"]):,.0f} м² новых газонов, трава, кусты, деревья, асфальт, разметка, дорожки, фонари, луна.'),
 ('Интерьер','1 939 дополнительных потолочных светильников; 541 комплект радиаторов, камер, маяков, огнетушителей и настенных деталей. Красный свет пульсирует; яркий режим доступен по B.'),
 ('Вещи','15 727 декоративных вещей на столах, полках и других опорах: документы, книги, лекарства, коробки, бельё, посуда, инструменты.'),
 ('Микродетали','Ещё 15 082 детали 29 видов: медицинские мелочи, канцелярия, розетки, вентиляция, дозаторы, таблички, штрихкоды, износ, упаковки и листья.'),
 ('Сюжет','395 игровых размещений и 46 мобов 17 видов; 19 глав и шесть побочных историй привязаны к пространству. Записки намеренно пустые; сцены, диалоги и финальная постановка остаются заготовками.')],[145,945],size=9)
end()

TRANSLATE={'pen':'Ручка','stapler':'Степлер','staff_badge':'Бейдж','calculator':'Калькулятор','sticky_notes':'Стикеры','evidence_envelope':'Конверт','soap':'Мыло','toothbrush':'Зубная щётка','adhesive_tape':'Скотч','rubber_gloves':'Перчатки','tea_packet':'Пакет чая','napkins':'Салфетки','cutlery':'Столовые приборы','syringe':'Шприц','gauze_packet':'Перевязочный пакет','tweezers':'Пинцет','blister_pack':'Блистер','ampoule_rack':'Держатель ампул','vent_grille':'Вентрешётка','socket':'Розетка','sanitizer':'Дозатор','medical_notice':'Медицинская табличка','wall_scuff':'Потёртость стены','wall_crack':'Трещина','label_barcode':'Штрихкод','bin_wrapper':'Упаковка у урны','floor_scuff':'Следы у мебели','fallen_leaves':'Опавшие листья','pole_bolts':'Крепёж фонаря'}
types=list(D['microdetails_dressing']['counts'].items())
for start in range(0,len(types),17):
    w,h=page('Реестр новых мелких деталей'+f' / {start//17+1}','Точное число логических декоративных деталей. Одна деталь может состоять из нескольких простых частей.')
    table(['Тип','ID типа в игре','Количество'],[(TRANSLATE[k],k,str(v)) for k,v in types[start:start+17]],[450,420,220],size=10);end()

for j,group in enumerate([['details_clinical','details_office','details_storage','details_wall'],['environment_road','environment_garden','environment_moon','environment_furnished_PS13']]):
    w,h=page('Кадры из актуальной игры'+f' / {j+1}','Реальные прототипы из проверенной сборки, 1920 x 1080. Для изучения расположения используй планы.')
    for k,name in enumerate(group):
        x=45+(k%2)*550;y=h-420-(k//2)*320
        image=ROOT/f'artifacts/unreal_hospital_v11/world_{name}.png';assert image.exists()
        C.drawImage(str(image),x,y,width=530,height=298.125,preserveAspectRatio=True,anchor='c')
        text(x,y-13,name.replace('details_','').replace('environment_',''),8,MUTED)
    end()

w,h=page('Проверка и ограничения снимка','PDF построен из той же карты, что загружается текущей Windows-сборкой.')
y=table(['Проверка','Результат'],[
 ('Реальная игра','89/89 проверок карты: оболочки, проходы, лестницы, лифт, тоннели, двери, укрытия, сохранения и окружение.'),
 ('Сюжетный прототип','23/23 проверок игровых взаимодействий, оружия, мобов и сохранений.'),
 ('Данные','Карта 15/15, исходное наполнение 9/9, сюжет 11/11, микродетали 9/9. Сохранение старых габаритов и слоёв проверено.'),
 ('Оболочки','899 569 выборочных проверок коллизии: найденных пропусков нет. Это выборка по сетке, не доказательство каждого пикселя.'),
 ('Кадры','90 свежих снимков 1920 x 1080 и 23 начальные проверки с отрисовкой. Некоторые камеры показывают близкие стены; это не полный визуальный обход каждой комнаты.'),
 ('Готовность','Модели мебели, вещей, растений и мобов пока простые. Новые камеры, розетки, дозаторы и таблички декоративные. 210-часовая кампания не объявляется завершённой.')],[170,920],size=10)
para(48,y-20,'SHA-256 карты: '+SHA+'\nSHA-256 Windows EXE: '+V['executable_sha256'],1090,8.5,15);end()

# Embedded complete registers retain exact coordinates of every small thing.
roomrows=[]
for r in ROOMS:
    x,y,w,h=r['rect'];ox,oy,z=r['origin'];roomrows.append(dict(id=r['id'],site=r['site'],name=r['name'],x=ox+x,y=oy+y,z=z,width=w,depth=h,area=w*h,production_class=r['production_class'],furniture=FURN_COUNT[r['id']],things=SMALL_COUNT[r['id']],details=DETAIL_COUNT[r['id']]))
csv_file('01_zones_562.csv',list(roomrows[0]),roomrows)
bays=[]
for b in D['subrooms']:
    s=LOC[b['site']];x,y,w,h=b['rect'];ox,oy,z=s['origin'];bays.append(dict(id=b['id'],zone=b['zone'],site=b['site'],x=ox+x,y=oy+y,z=z,width=w,depth=h,area=w*h,clear_width=b['clear_width_m'],clear_depth=b['clear_depth_m']))
csv_file('02_internal_bays_1939.csv',list(bays[0]),bays)
csv_file('03_furniture_catalog.csv',['model','name','mount','bounds'],[dict(model=k,**v) for k,v in D['prototype_catalog'].items()])
for name,key in [('04_furniture_instances','prototype_props'),('05_supplemental_furniture','environment_props'),('06_hideouts','hideouts'),('07_stairs','stairs'),('08_tunnels','utility_links'),('09_windows','windows'),('10_network_devices','network_devices'),('11_threat_nodes','threat_nodes'),('12_secret_passages','secret_passages'),('13_zone_states','zone_evolution')]:
    rows=D[key];fields=list(dict.fromkeys(k for r in rows for k in r));csv_file(name+'.csv',fields,rows)
for filename,key in [('14_things_v10','clutter_dressing'),('15_microdetails_v11','microdetails_dressing')]:
    rows=D[key]['items'];csv_file(filename+'.csv',list(dict.fromkeys(k for r in rows for k in r)),rows)
for filename,key in [('16_story_items','placements'),('17_mobs','mobs'),('18_chapters','chapters'),('19_scenes','scenes')]:
    rows=D['story_gameplay'][key];csv_file(filename+'.csv',list(dict.fromkeys(k for r in rows for k in r)),rows)
csv_file('20_environment_geometry.csv',list(dict.fromkeys(k for r in D['environment_dressing']['boxes'] for k in r)),D['environment_dressing']['boxes'])
csv_file('21_lawns.csv',['x','y','width','depth','area'],[dict(x=x,y=y,width=w,depth=h,area=w*h) for x,y,w,h in D['environment_dressing']['lawns']])
csv_file('22_typed_doors.csv',['zone','site','at_global','side','type','role','width'],[dict(zone=r['id'],site=r['site'],at_global=[r['origin'][0]+e['at'][0],r['origin'][1]+e['at'][1],r['origin'][2]],side=e['side'],type=e.get('type','ordinary'),role=e.get('role','primary'),width=e.get('width',2)) for r in ROOMS for e in [dict(at=r['door'],side=r['door_side'],type=r['door_type'])]+r['extra_doors']])

w,h=page('Полные данные внутри PDF','Приложение ECHO_v11_data.zip содержит точные координаты, включая всю мебель и каждую мелкую деталь.')
y=para(48,h-140,'Открой панель вложений в настольном PDF-просмотрщике (значок скрепки). Браузерный просмотрщик может не показывать вложения. Основные планы и реестр зон видны на страницах без распаковки.',1090,12,19)
table(['Файлы во вложении','Содержание'],[
 ('hospital_world.json','Полный исходный снимок карты из игры, включая все слои, типы дверей, состояния, сюжетные связи и маршруты.'),
 ('01-03 CSV','Все 562 зоны; 1 939 внутренних секций с чистыми размерами; каталог 154 типов прототипов.'),
 ('04-13 CSV','Экземпляры мебели, укрытия, лестницы, тоннели, окна, сети, узлы, секреты и состояния зон.'),
 ('14-15 CSV','15 727 вещей и 15 082 микродетали: ID, родительская опора, зона, координаты и точные границы.'),
 ('16-19 CSV','Игровые предметы, мобы, главы и пространственные сцены. Пустые записки и незавершённые сцены отмечены в исходных данных.'),
 ('20-22 CSV','Геометрия окружения, газоны и все типизированные двери с глобальными координатами.'),
 ('verification.json','Проверка именно этой карты и EXE, контрольные суммы и границы проведённых испытаний.')],[230,860],y=y-25,size=10);end()

index=list(PAGES)
for start in range(0,len(index),29):
    w,h=page('Навигация по атласу'+f' / {start//29+1}','В PDF также доступны закладки по локациям и разделам.')
    yy=h-125
    for entry in index[start:start+29]:
        text(48,yy,str(entry['page']).zfill(2),10,ACCENT);text(90,yy,entry['title'],10)
        C.linkRect('',entry['key'],(43,yy-4,w-43,yy+13),relative=0,thickness=0);yy-=22
    end()
C.save();assert len(REGISTERED)==562 and len(set(REGISTERED))==562
manifest=dict(revision=11,pages=len(PAGES),rooms=len(ROOMS),locations=len(LOC),source_sha256=SHA,page_index=PAGES,embedded_tables=22)
(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
archive=TMP/'ECHO_v11_data.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    z.write(SOURCE,'hospital_world.json')
    for p in sorted(OUT.glob('*.csv')):z.write(p,p.name)
    z.write(OUT/'manifest.json','manifest.json');z.write(ROOT/'artifacts/unreal_hospital_v11/verification.json','verification.json')
reader=PdfReader(TMP/'body.pdf');writer=PdfWriter();writer.clone_document_from_reader(reader);writer.add_attachment(archive.name,archive.read_bytes())
with PDF.open('wb') as stream:writer.write(stream)
reader=PdfReader(PDF);full='\n'.join(p.extract_text() or '' for p in reader.pages)
for r in ROOMS:assert r['id'] in full and r['name'] in full,r['id']
assert len(reader.pages)==len(PAGES) and len(reader.attachments)==1
result=dict(success=True,pdf=str(PDF),pages=len(reader.pages),mb=round(PDF.stat().st_size/1e6,2),rooms=562,locations=35,source_sha256=SHA,embedded_archive=archive.name)
(OUT/'pdf_check.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(result,ensure_ascii=False))
