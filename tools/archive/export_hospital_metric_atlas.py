"""Metric, as-built 2D atlas of hospital_campus_v2. Never edits the game."""
from pathlib import Path
from collections import defaultdict
import csv, hashlib, json, math, subprocess, zipfile
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor, Color, white
from reportlab.lib.pagesizes import A2, A3, landscape
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from pypdf import PdfReader

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'output/pdf/ECHO_Hospital_2D'; OUT.mkdir(parents=True,exist_ok=True)
TMP=ROOT/'tmp/pdfs/hospital_metric';TMP.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
STAGED=ROOT/'unreal/EchoWard/Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/hospital_world.json'
assert SOURCE.read_bytes()==STAGED.read_bytes(), 'Source and playable package differ'
DATA=json.loads(SOURCE.read_text('utf-8')); MARKER=json.loads((ROOT/'unreal/EchoWard/Saved/PackagedWindows/package_verified.json').read_text('utf-8-sig'))
assert MARKER['success'] and MARKER['world_id']==DATA['map_id']=='hospital_campus_v2'
FONT=ROOT/'unreal/EchoWard/Content/CampaignData/Fonts/NotoSans.ttf'
pdfmetrics.registerFont(TTFont('Ward',str(FONT)))
INK=HexColor('#17373d'); MUTED=HexColor('#566d72'); RED=HexColor('#b23a44')
COLORS={'tile':'#e0eff0','asphalt':'#adb6c0','green':'#9dcc8c','roof':'#aab1be',
        'void':'#f4f0e8','wall':'#243b40','steel':'#748c9d','overhead':'#bd872c','core':'#f3c58d'}
LOC={l['id']:l for l in DATA['locations']}; ROOMS=[]; CELLS=defaultdict(dict)
FEATURES=[]; SURFACES=[]; WALLS=[]; PAGE_INDEX=[]

def fmt(v):
    if abs(v-round(v))<1e-7:return str(round(v))
    return f'{v:.4f}'.rstrip('0').rstrip('.')
def dim(w,h):return f'{fmt(w)} × {fmt(h)} м'
def put_rect(origin,rect,room=-1,outdoor=False):
    ox,oy,z=origin;x,y,w,h=rect;x=round(ox+x);y=round(oy+y)
    for a in range(x,x+round(w)):
        for b in range(y,y+round(h)):
            if (a,b) not in CELLS[z] or CELLS[z][a,b][0]==-1:CELLS[z][a,b]=(room,outdoor)

for l in DATA['locations']:
    ox,oy,z=l['origin']
    for r in l['rooms']:
        row=dict(r,location=l['id'],z=z,global_rect=[ox+r['rect'][0],oy+r['rect'][1],*r['rect'][2:]],
                 global_door=[ox+r['door'][0],oy+r['door'][1]])
        idx=len(ROOMS);ROOMS.append(row);put_rect(l['origin'],r['rect'],idx,not r['enclosed'])
    for r in l['corridors']:put_rect(l['origin'],r,-1,l['id'] in ('ST','RF'))
    for r in l['cores']:put_rect(l['origin'],r['rect'],-1,l['id'] in ('ST','RF'))
def passage(x,y,w,h):
    for a in range(x,x+w):
        for b in range(y,y+h):CELLS[0].setdefault((a,b),(-1,False))
passage(48,100,4,230);passage(44,-20,8,20)
for y in (38,158,268):passage(-46,y,94,6);passage(52,y,98,6)
def shaft(x,y):return 48<=x<52 and 4<=y<10
def stairhole(x,y):return (44<=x<47 and (0<=y<7 or 88<=y<95)) or (-8<=x<-5 and 86<=y<93)
def hole(x,y,z):return shaft(x,y) or (stairhole(x,y) and z>-9.6)
def feature(code,name,x,y,w,h,z,z_top,kind='structure',location='',note=''):
    FEATURES.append(dict(code=code,name=name,x=x,y=y,w=w,h=h,z=z,z_top=z_top,kind=kind,location=location,note=note))
def surface(code,name,rect,z,material,layer):
    SURFACES.append(dict(code=code,name=name,x=rect[0],y=rect[1],w=rect[2],h=rect[3],z=z,material=material,layer=layer))

# Exactly the four terrain strips in WardHospitalDetails.cpp (top Z=-0.20m).
for n,r in enumerate([[-180,-190,172,570],[104,-190,176,570],[-8,-190,112,190],[-8,100,112,280]],1):surface(f'G{n}','Основание территории',r,-.2,'asphalt',0)
# Reproduce runtime floor row merging and its actual stair/lift cutouts.
for z,cells in sorted(CELLS.items()):
    for (x,y),(ri,air) in sorted(cells.items(),key=lambda p:(p[0][1],p[0][0])):
        previous=cells.get((x-1,y))
        if hole(x,y,z) or (previous is not None and previous[1]==air and not hole(x-1,y,z)):continue
        end=x+1
        while (end,y) in cells and cells[end,y][1]==air and not hole(end,y,z):end+=1
        surface(f'F{len(SURFACES):05}','Пол уровня',[x,y,end-x,1],z,'asphalt' if air else 'tile',10)
surface('ROOF_APRON','Западный кровельный фартук',[-8,0,8,100],14.4,'asphalt',11)
surface('ENTRANCE_PATH','Плитка перед входом',[44,-49,12,42],0,'tile',15)
for r in ROOMS:
    if r['location']=='ST' and r['id'] in ('S05','S15'):
        x,y,w,h=r['global_rect'];surface('GREEN_'+r['id'],'Зелёное покрытие / '+r['name'],[x+.4,y+.4,w-.8,h-.8],.04,'green',20)

# Exact wall centrelines, including 2m doorway gaps (lintels start at Z+2.60).
series=defaultdict(list)
for z,cells in CELLS.items():
    for (x,y),(ri,air) in cells.items():
        for side,(dx,dy) in enumerate(((1,0),(0,1),(-1,0),(0,-1))):
            other=cells.get((x+dx,y+dy))
            if other is not None and side>=2:continue
            if other is not None and (other[0]==ri or (other[0]==ri==-1) or (air and other[1])):continue
            if other is None and air:continue
            mx,my=x+.5+dx*.5,y+.5+dy*.5
            rid=ri if ri>=0 else other[0] if other else -1
            door=rid>=0 and dy!=0 and abs(my-ROOMS[rid]['global_door'][1])<.001 and abs(mx-ROOMS[rid]['global_door'][0])<1.05
            axis='Y' if dx else 'X';fixed=mx if dx else my;along=y if dx else x
            series[z,axis,fixed,door].append(along)
for (z,axis,fixed,door),values in series.items():
    values=sorted(set(values));a=0
    while a<len(values):
        b=a
        while b+1<len(values) and values[b+1]==values[b]+1:b+=1
        start,end=values[a],values[b]+1
        WALLS.append(dict(z=z,axis=axis,fixed=fixed,start=start,end=end,thickness=.16,
                          kind='lintel' if door else 'wall',base_z=z+(2.6 if door else 0),height=1.2 if door else 3.8))
        a=b+1

# Engineering footprints copied from the current C++ model. Coordinates in metres.
CORES=[('A',44,0),('B',44,88),('C',-8,86)]
for name,x,y in CORES:
    for z in (-9.6,-4.8,0,4.8,9.6,14.4):feature(f'ST-{name}-{fmt(z)}',f'Лестница {name}',x,y,3,8,z,z+3.8,'stair','MAIN','Марш 1.36 м; 2 × 15 ступеней; 0.28 × 0.16 м')
    for z in (-9.6,-4.8,0,4.8,9.6):
        for i in range(15):
            rise=(i+1)*.16
            feature(f'{name}-{fmt(z)}-1-{i+1}',f'Ступень {i+1}, марш 1',x+.10,y+6.8-(i+1)*.28,1.36,.28,z,z+rise,'step','MAIN')
            feature(f'{name}-{fmt(z)}-2-{i+1}',f'Ступень {i+1}, марш 2',x+1.54,y+2.6+i*.28,1.36,.28,z+2.4,z+2.4+rise,'step','MAIN')
for z in (-9.6,-4.8,0,4.8,9.6,14.4):
    feature('LIFT-'+fmt(z),'Шахта лифта',48,4,4,6,z,z+3.8,'lift','MAIN','Кабина 3.80 × 5.80 м; в свету 3.58 × 5.68 × 3.39 м')
    feature('LIFT-CAB-'+fmt(z),'Проекция кабины на остановке',48.1,4.1,3.8,5.8,z-.2,z+3.51,'cabin','MAIN','Дверной проём при полном открытии 3.06 м')
feature('CANOPY','Козырёк главного входа',39,-13,18,13,4.15,4.45,'overhead','1F')
for i,x in enumerate((40,56),1):feature(f'COL-{i}','Колонна входа',x-.175,-11-.175,.35,.35,0,4.2,'column','1F')
for z in (-4.8,14.4):feature('EL-'+fmt(z),'Электрощит',44.10,29.35,.70,1.30,z+.3,z+2.4,'electrical','B1' if z<0 else 'RF')
for r in ROOMS:
    x,y,w,h=r['global_rect'];cx,cy=x+w/2,y+h/2;z=r['z']
    if r['enclosed']:feature('LIGHT-'+r['id'],'Потолочный светильник',cx-.65,cy-.13,1.3,.26,z+3.61,z+3.69,'light',r['location'])
    if r['location']=='RF' and not r['enclosed'] and r['id'] not in ('T04','T01','T13'):
        feature('HVAC-'+r['id'],'Вентустановка',cx-2.15,cy-3.25,4.3,6.5,14.4,16.4,'hvac','RF')
        feature('DUCT-'+r['id'],'Примыкающий воздуховод',cx+2.15,cy-.45,2.7,.9,14.4,15.5,'duct','RF')
for z in (-4.8,-9.6):
    for x in (45.5,50.5):feature(f'PIPE-{z}-{x}','Продольная коммуникация',x-.175,28,.35,48,z+3.325,z+3.675,'overhead','B1' if z==-4.8 else 'B2')
    for y in range(26,90,12):feature(f'DUCT-{z}-{y}','Поперечный воздуховод',45.5,y-.3,5,.6,z+3.3,z+3.8,'overhead','B1' if z==-4.8 else 'B2')
for z,cells in CELLS.items():
    for (x,y),(ri,air) in cells.items():
        if not air and ri==-1 and x%8==0 and y-int(y/8)*8==2:feature(f'CL-{fmt(z)}-{x}-{y}','Светильник коридора',x-.15,y+.37,1.3,.26,z+3.61,z+3.69,'light','')
for i in range(10):feature(f'LP{i+1:02}','Уличный фонарь',19.9,-180+i*16-.1,.2,.2,0,5,'pole','ST')
for side,x in (('WEST',-180),('EAST',280)):feature('FENCE-'+side,'Граница забора',x-.08,-190,.16,570,0,2.2,'fence','ST')
feature('FENCE-N','Граница забора',-180,379.92,460,.16,0,2.2,'fence','ST')
for i,(x,y,w,h) in enumerate([[-8.11,0,.22,100],[103.89,0,.22,100],[-8,-.11,112,.22],[-8,99.89,112,.22]],1):feature('PARAPET-'+str(i),'Парапет кровли',x,y,w,h,14.4,15.5,'parapet','RF')
for i in range(12):
    a=i*2*math.pi/12;x=46+16.5*math.cos(a);y=48+16.5*math.sin(a)
    feature('PADLIGHT-'+str(i+1),'Огонь площадки',x-.15,y-.15,.3,.3,14.42,14.58,'padlight','RF')
feature('HELIPAD','Разметка вертолётной площадки',28.93,30.93,34.14,34.14,14.4,14.42,'helipad','RF','Осевые размеры рамки 34 × 34 м; полоса 0.14 м; зона T04 36 × 36 м')
for i,(x,w) in enumerate(((42.05,.9),(49.05,.9)),1):feature('H-STEM-'+str(i),'Буква H, стойка',x,40.5,w,15,14.4,14.435,'mark','RF')
feature('H-BAR','Буква H, перекладина',42.5,47.55,7,.9,14.4,14.435,'mark','RF')

def csv_write(name,fields,rows):
    with (OUT/name).open('w',encoding='utf-8-sig',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=fields,delimiter=';');writer.writeheader();writer.writerows({k:fmt(v) if isinstance(v,float) else v for k,v in row.items()} for row in rows)
roomrows=[]
for r in ROOMS:
    x,y,w,h=r['global_rect'];lx,ly=r['rect'][:2]
    roomrows.append(dict(ID=r['id'],Локация=r['location'],Название=r['name'],X_м=x,Y_м=y,Z_м=r['z'],
                         Локальный_X_м=lx,Локальный_Y_м=ly,Ширина_м=w,Глубина_м=h,Площадь_м2=w*h,
                         Закрытое_помещение=int(r['enclosed']),В_свету_X_м=round(w-.16,4) if r['enclosed'] else '',
                         В_свету_Y_м=round(h-.16,4) if r['enclosed'] else '',
                         Пол='Плитка' if r['enclosed'] else 'Асфальт / зелёная вставка' if r['id'] in ('S05','S15') else 'Асфальт',
                         Вход_X_м=r['global_door'][0],Вход_Y_м=r['global_door'][1]))
csv_write('01_Все_333_зоны_метры.csv',list(roomrows[0]),roomrows)
csv_write('02_Все_покрытия_координаты.csv',list(SURFACES[0]),SURFACES)
csv_write('03_Стены_и_перемычки.csv',list(WALLS[0]),WALLS)
csv_write('04_Лестницы_лифт_свет_инженерия.csv',list(FEATURES[0]),FEATURES)
(OUT/'hospital_world_source.json').write_bytes(SOURCE.read_bytes())

def txt(c,x,y,s,size=9,color=INK,anchor='left'):
    c.setFillColor(color);c.setFont('Ward',size)
    (c.drawCentredString if anchor=='center' else c.drawRightString if anchor=='right' else c.drawString)(x,y,str(s))
def wrap(s,width,size):
    lines=[];line=''
    for word in str(s).split():
        t=(line+' '+word).strip()
        if line and pdfmetrics.stringWidth(t,'Ward',size)>width:lines.append(line);line=word
        else:line=t
    return lines+[line] if line else lines
def para(c,x,y,s,width,size=10,leading=None,color=MUTED):
    for line in wrap(s,width,size):txt(c,x,y,line,size,color);y-=leading or size*1.45
    return y
def page(c,size,title,subtitle,key):
    c.setPageSize(size);W,H=size
    c.setFillColor(HexColor('#fbfcfa'));c.rect(0,0,W,H,fill=1,stroke=0)
    txt(c,45,H-32,'ЭХО / ТОЧНЫЙ 2D-АТЛАС ТЕКУЩЕЙ ИГРЫ',10,MUTED)
    txt(c,45,H-65,title,24)
    txt(c,45,H-88,subtitle,9,MUTED)
    c.setStrokeColor(HexColor('#b6c9ca'));c.line(45,H-100,W-45,H-100)
    p=c.getPageNumber();PAGE_INDEX.append(dict(page=p,key=key,title=title))
    txt(c,45,22,'hospital_campus_v2 / метры / 22.10.2026 / размеры из готовой 3D-модели',8,MUTED)
    txt(c,W-45,22,f'{p:02}',9,MUTED,'right')
    c.bookmarkPage(key);c.addOutlineEntry(title,key,0)
    return W,H
def legend(c,x,y,width=390):
    for key,caption in [('tile','Плитка / пол корпуса'),('asphalt','Асфальт / открытая кровля'),('green','Зелёное покрытие (плоский участок)'),('void','Нет пола на данной отметке'),('core','Лестница / шахта / площадка')]:
        c.setFillColor(HexColor(COLORS[key]));c.setStrokeColor(HexColor('#789194'));c.rect(x,y-3,16,10,fill=1,stroke=1)
        txt(c,x+24,y,caption,9);y-=19
    c.setStrokeColor(INK);c.setLineWidth(2);c.line(x,y+2,x+16,y+2);txt(c,x+24,y,'Стена, толщина 0.16 м',9);y-=19
    c.setStrokeColor(HexColor('#bd872c'));c.setDash(3,2);c.line(x,y+2,x+16,y+2);c.setDash();txt(c,x+24,y,'Потолочная / навесная конструкция',9);y-=19
    return y
class Plan:
    def __init__(self,c,box,bounds,denom):
        self.c=c;self.box=box;self.bounds=bounds;self.s=2834.645669/denom
        x,y,w,h=box;bx,by,bw,bh=bounds
        self.ox=x+(w-bw*self.s)/2;self.oy=y+(h-bh*self.s)/2
    def p(self,x,y):
        bx,by,bw,bh=self.bounds
        return self.ox+(x-bx)*self.s,self.oy+(by+bh-y)*self.s
    def rect(self,r,fill,stroke=None,lw=.3,dash=False):
        x,y,w,h=r;px,py=self.p(x,y+h);c=self.c
        c.setFillColor(HexColor(fill) if isinstance(fill,str) else fill or white)
        c.setStrokeColor(HexColor(stroke) if isinstance(stroke,str) else stroke or white)
        c.setLineWidth(lw);c.setDash(3,2) if dash else c.setDash()
        c.rect(px,py,w*self.s,h*self.s,fill=int(fill is not None),stroke=int(stroke is not None));c.setDash()
    def line(self,x1,y1,x2,y2,color=INK,width=.5,dash=False):
        c=self.c;c.setStrokeColor(HexColor(color) if isinstance(color,str) else color);c.setLineWidth(width)
        c.setDash(3,2) if dash else c.setDash();c.line(*self.p(x1,y1),*self.p(x2,y2));c.setDash()
    def label(self,x,y,s,size=8,color=INK,anchor='center'):txt(self.c,*self.p(x,y),s,size,color,anchor)
    def clip(self):
        c=self.c;c.saveState();bx,by,bw,bh=self.bounds;x,y=self.p(bx,by+bh);p=c.beginPath();p.rect(x,y,bw*self.s,bh*self.s);c.clipPath(p,stroke=0)
    def grid(self,step):
        bx,by,bw,bh=self.bounds
        for x in range(math.ceil(bx/step)*step,math.floor((bx+bw)/step)*step+1,step):self.line(x,by,x,by+bh,'#dbe1df',.23)
        for y in range(math.ceil(by/step)*step,math.floor((by+bh)/step)*step+1,step):self.line(bx,y,bx+bw,y,'#dbe1df',.23)
    def ruler(self,length):
        x,y,w,h=self.box;px=x+12;py=y+12;c=self.c
        c.setStrokeColor(INK);c.setLineWidth(2);c.line(px,py,px+length*self.s,py)
        for k in (0,length/2,length):c.line(px+k*self.s,py-4,px+k*self.s,py+4);txt(c,px+k*self.s,py+8,fmt(k),8,INK,'center')
        txt(c,px+length*self.s+12,py+8,'м',8)
    def axes(self):
        bx,by,bw,bh=self.bounds;x,y=self.p(bx+3,by+3)
        self.c.setStrokeColor(INK);self.c.setLineWidth(.8)
        self.c.line(x,y,x+38,y);self.c.line(x,y,x,y-38)
        txt(self.c,x+42,y-3,'+X',8);txt(self.c,x-5,y-50,'+Y',8)
    def dimension(self,x1,y1,x2,y2,label,offset=0):
        if y1==y2:
            y=y1+offset;self.line(x1,y,x2,y,width=.45)
            for x in (x1,x2):self.line(x,y-.5,x,y+.5,width=.7)
            self.label((x1+x2)/2,y-.7,label,8)
        else:
            x=x1+offset;self.line(x,y1,x,y2,width=.45)
            for y in (y1,y2):self.line(x-.5,y,x+.5,y,width=.7)
            px,py=self.p(x,(y1+y2)/2);self.c.saveState();self.c.translate(px-4,py);self.c.rotate(90);txt(self.c,0,0,label,8,INK,'center');self.c.restoreState()

def draw_surfaces(p,z,base=False):
    for s in SURFACES:
        if (base and s['layer']==0) or (s['layer']!=0 and abs(s['z']-z)<.06):p.rect([s[k] for k in ('x','y','w','h')],COLORS[s['material']])
def draw_walls(p,z):
    for w in WALLS:
        if abs(w['z']-z)>.001:continue
        f,a,b=w['fixed'],w['start'],w['end'];isdoor=w['kind']=='lintel'
        if w['axis']=='X':p.line(a,f,b,f,'#4e9bad' if isdoor else INK,.6 if isdoor else max(.65,.16*p.s),isdoor)
        else:p.line(f,a,f,b,'#4e9bad' if isdoor else INK,.6 if isdoor else max(.65,.16*p.s),isdoor)
def draw_features(p,z,loc='',overview=False):
    for f in FEATURES:
        if f['kind']=='step' or f['kind']=='cabin':continue
        if f['kind']=='fence':
            if z==0:p.rect([f[k] for k in ('x','y','w','h')],None,'#293d42',1.4)
            continue
        if abs(f['z']-z)>.01 and not (f['kind'] in ('light','overhead','electrical','hvac','duct') and z<=f['z']<z+3.8):continue
        kind=f['kind'];r=[f[k] for k in ('x','y','w','h')]
        if kind in ('light','overhead'):
            if not overview:p.rect(r,None,COLORS['overhead'],.35,True)
        elif kind=='stair':
            if overview:continue
            p.rect(r,COLORS['core'],'#a86728',.7)
            x,y=f['x'],f['y']
            for i in range(16):
                p.line(x+.1,y+2.6+i*.28,x+1.46,y+2.6+i*.28,'#a86728',.35)
                p.line(x+1.54,y+2.6+i*.28,x+2.90,y+2.6+i*.28,'#a86728',.35)
            p.label(x+1.5,y+7.5,f['code'].split('-')[1],8)
        elif kind=='lift':
            if overview:continue
            p.rect(r,'#bad7e4','#44798e',.7);p.label(f['x']+2,f['y']+3,'ЛИФТ',6.7)
        elif kind=='helipad':p.rect(r,None,'#eeeedd',max(.6,.14*p.s))
        else:
            p.rect(r,'#f9faf7' if kind=='mark' else COLORS['steel'],'#4d6570',.35)
            if kind in ('hvac','electrical') and not overview:p.label(f['x']+f['w']/2,f['y']+f['h']/2,'ВУ' if kind=='hvac' else 'ЭЩ',6,white)
def markings(p):
    for row in range(2):
        for i in range(18):
            x,y=-60+i*3.3,-178+row*18
            p.rect([x-.045,y-2.6,.09,5.2],'#fafcf7')
            p.rect([x+1.6-1.65,y+2.6-.045,3.3,.09],'#fafcf7')
    for y in range(20,165,7):p.rect([41.94,-190+y-1.6,.12,3.2],'#fafcf7')
    for i in range(9):p.rect([41+i-.275,-28,.55,6],'#fafcf7')
def draw_rooms(p,l,global_coords=False,overview=False):
    ox,oy,z=l['origin']
    for r in l['rooms']:
        x,y,w,h=r['rect'];x+=ox;y+=oy
        if l['id'] in ('ST','RF') and not r['enclosed']:p.rect([x,y,w,h],None,'#65747b',.5,True)
        if overview:continue
        # IDs and exact dimensions on plan; full unabridged names in the side key.
        if l['id'] in ('ST','RF'):
            lines=wrap(r['id']+' / '+r['name'],w*p.s-10,8.5)
            for j,line in enumerate(lines):p.label(x+w/2,y+2+j*11/p.s,line,8.5)
            p.label(x+w/2,y+2+(len(lines)*11+5)/p.s,dim(w,h)+f' / {fmt(w*h)} м²',7,MUTED)
        else:
            px,py=p.p(x+w/2,y+h/2);rw=w*p.s-9
            lines=wrap(r['name'],rw,6.5)
            lines=lines[:4]
            txt(p.c,px,py+15,r['id'],10,INK,'center')
            txt(p.c,px,py+3,dim(w,h),7.1,MUTED,'center')
            for j,line in enumerate(lines):txt(p.c,px,py-9-j*8,line,6.5,INK,'center')

PDF=OUT/'ЭХО_точная_2D_карта_333_зоны.pdf'
c=canvas.Canvas(str(PDF),pageCompression=1)
c.setTitle('ЭХО - точный 2D-атлас 333 зон, метры и покрытия');c.setAuthor('Экспорт из hospital_campus_v2 / Unreal Engine')
W,H=page(c,landscape(A3),'Как читать этот атлас','Точное состояние готовой Windows-сборки A45EC26E. Схема не меняет игру.','readme')
left=50;right=640;y=H-135
y=para(c,left,y,'333 зоны • 13 локаций • все этажи и корпуса',550,19,26,INK)-15
for title,body in [
 ('Единицы и оси','Все координаты и размеры указаны в метрах. X направлен вправо, Y вниз на плане; это оси игры, а не географический север. Начало (0; 0; 0) - главный корпус, первый этаж.'),
 ('Точность размеров','Размеры комнат заданы по линиям границ ячеек модели. Основные стены толщиной 0.16 м выступают на 0.08 м внутрь каждой стороны. Для закрытой прямоугольной комнаты ориентировочный размер в свету: (ширина - 0.16) × (глубина - 0.16). В CSV приведены оба значения.'),
 ('Покрытия без выдуманных участков','Зелёное покрытие существует только в двух вставках S05 и S15. В игре это зелёные плоскости, без отдельных травинок. Остальная открытая территория покрыта асфальтом, кроме показанных плиточных проходов и полов зданий. Незаполненные места на поэтажном плане не следует считать газоном.'),
 ('Масштаб и печать','Чертежи векторные: увеличивай PDF без потери резкости. Указанный численный масштаб верен при печати исходного листа в 100%; при подгонке под бумагу пользуйся графической линейкой. Сетка и размерные подписи измеряются в метрах.'),
 ('Что входит в архив','PDF, отдельные PNG всех схем, CSV всех 333 зон, покрытий, стен, лестничных ступеней, лифта, фонарей и инженерии, а также исходный JSON карты. Идентификаторы на плане совпадают с игрой.')]:
    txt(c,left,y,title,12,INK);y=para(c,left,y-20,body,550,10,14)-19
y=legend(c,right,H-155)
levels=[('B2','Экспериментальный подвал',-9.6),('B1','Подвал / морг / инженерия',-4.8),('1F','Приёмный корпус',0),('2F','Лечебный этаж',4.8),('3F','Администрация',9.6),('RF','Эксплуатируемая крыша',14.4)]
txt(c,right,y-10,'Высотные отметки пола',13);yy=y-35
for ident,name,z in reversed(levels):txt(c,right,yy,f'{ident:>2}   {fmt(z):>5} м   {name}',10);yy-=24
yy=para(c,right,yy-8,'Шаг этажей 4.80 м. Стены 3.80 м; низ потолочного светильника 3.61 м. Толщина основного пола 0.20 м. Асфальтовое основание вне корпусов: верх -0.20 м; локальная территория ST: 0.00 м.',480,10,15)-18
para(c,right,yy,'Проверенный источник: hospital_world.json + WardBookWorld.cpp + WardHospitalDetails.cpp. 1 единица Unreal = 0.01 м. Геометрия снимка: 22.10.2026; SHA-256 источника указан в manifest.json.',480,9,14)
txt(c,50,195,'Где искать',13)
for j,(ident,caption,pg) in enumerate([('ST','Территория',3),('1F','Приёмный корпус',4),('B1','Подвал / морг',5),('B2','Экспериментальный',6),('2F','Лечебный этаж',7),('3F','Администрация',8),('RF','Крыша',9),('DG','Диагностика',10),('IF','Инфекционный',11),('PS','Психоневрология',12),('RH','Реабилитация',13),('HS','Персонал',14),('SV','Хозяйственный',15)]):
    col=j//5;row=j%5;txt(c,50+col*360,170-row*21,f'{ident} / {caption}  -  лист {pg:02}',9)
c.showPage()

# Whole campus, with the true ground and all six separately located pavilions.
W,H=page(c,A2,'Весь комплекс / первый этаж и территория','М 1:1250 / сетка 25 м / X: -180…280 м; Y: -190…380 м / граница основания 460 × 570 м','campus')
p=Plan(c,(65,145,W-130,H-270),[-190,-200,480,590],1250);p.clip();p.rect(p.bounds,COLORS['void']);draw_surfaces(p,0,True);p.grid(25);draw_walls(p,0);markings(p);draw_features(p,0,overview=True)
for ident in ('1F','DG','IF','PS','RH','HS','SV'):
    l=LOC[ident];ox,oy,_=l['origin'];p.label(ox+48,oy+41,ident+' / '+l['name'],9)
for r in LOC['ST']['rooms']:
    x,y,w,h=r['rect'];p.label(x-70+w/2,y-190+h/2,r['id'],7.5)
p.label(50,215,'ГАЛЕРЕЯ 4 м',7)
p.label(50,-8,'ГЛАВНЫЙ ВХОД',7)
p.dimension(-180,-190,280,-190,'460 м',-4);p.dimension(280,-190,280,380,'570 м',4)
p.c.restoreState();p.ruler(50);p.axes()
txt(c,65,92,'Основное поле - асфальт. Голубым показаны реальные плиточные полы и галереи. Зелёным - только S05 и S15.',10)
txt(c,65,72,'Граница южной стороны Y=-190 м открыта: забора там нет. Забор 2.20 м стоит на X=-180, X=280 и Y=380.',10,MUTED)
c.showPage()

ORDER=['ST','1F','B1','B2','2F','3F','RF','DG','IF','PS','RH','HS','SV']
for ident in ORDER:
    l=LOC[ident];ox,oy,z=l['origin'];bx,by,bw,bh=l['bounds'];denom=700 if ident=='ST' else 350
    W,H=page(c,landscape(A2),ident+' / '+l['name'],f'М 1:{denom} / сетка 5 м / Z пола {fmt(z)} м / начало локального листа: мировой X={fmt(ox)}, Y={fmt(oy)} м','plan_'+ident)
    bounds=[ox+bx-6,oy+by-6,bw+12,bh+12]
    p=Plan(c,(55,95,1080,H-220),bounds,denom);p.clip();p.rect(bounds,COLORS['void'])
    # Hatch is deliberate: these areas have no generated floor at this level.
    for x in range(math.floor(bounds[0]-bounds[3]),math.ceil(bounds[0]+bounds[2])+1,4):p.line(x,bounds[1],x+bounds[3],bounds[1]+bounds[3],'#ded8cc',.28)
    draw_surfaces(p,z);p.grid(5)
    draw_rooms(p,l);draw_walls(p,z)
    if ident=='ST':markings(p)
    draw_features(p,z,ident)
    # Label areas again where overhead light symbols sit, keeping IDs legible.
    if ident not in ('ST','RF'):draw_rooms(p,l)
    p.dimension(ox+bx,oy+by,ox+bx+bw,oy+by,fmt(bw)+' м',-3)
    p.dimension(ox+bx+bw,oy+by,ox+bx+bw,oy+by+bh,fmt(bh)+' м',3)
    p.c.restoreState();p.ruler(20 if ident=='ST' else 10);p.axes()
    # Metric axis ticks in world coordinates; local coordinates in the register.
    for x in range(math.ceil((ox+bx)/10)*10,math.floor((ox+bx+bw)/10)*10+1,10):
        px,py=p.p(x,oy+by);txt(c,px,py+10,fmt(x),7,MUTED,'center')
    sx=1180;sy=H-135
    txt(c,sx,sy,'Экспликация зон',15);sy-=25
    for r in l['rooms']:
        w,h=r['rect'][2:];text=f"{r['id']}  {r['name']}"
        lines=wrap(text,445,9)
        txt(c,sx,sy,lines[0],9)
        for line in lines[1:]:sy-=12;txt(c,sx+35,sy,line,9)
        sy-=13;txt(c,sx+35,sy,dim(w,h)+f' / {fmt(w*h)} м²',8,MUTED);sy-=13
    if ident=='ST':
        sy-=6
        for s in SURFACES:
            if s['material']=='green':
                sy=para(c,sx,sy,s['code']+': '+dim(s['w'],s['h'])+f" = {fmt(s['w']*s['h'])} м²; X={fmt(s['x'])}, Y={fmt(s['y'])}.",445,9,12)-10
        sy=para(c,sx,sy,'В S05/S15 зелёная вставка отступает на 0.40 м от границы зоны. По краям остаётся асфальт. Разметка парковки: два ряда по 18 позиций, шаг 3.30 м, длина линии 5.20 м.',445,9,13)-10
    elif ident=='RF':
        sy=para(c,sx,sy-8,'ВУ - 8 вентустановок по 4.30 × 6.50 м; воздуховод 2.70 × 0.90 м. ЭЩ - электрощит. Рамка посадочной площадки: 34 × 34 м по осям, ширина полосы 0.14 м. Зона T04: 36 × 36 м.',445,9,13)-10
    sy=legend(c,sx,min(sy-15,350))
    para(c,sx,sy-8,'Подписи X/Y на краю схемы - мировые координаты. Все числовые размеры помещений и координаты входов приведены далее и в CSV. Схема показана без верхних этажей и крыши.',445,9,13)
    c.showPage()

# Vertical section and useful engineering dimensions.
W,H=page(c,landscape(A3),'Лестницы, лифт и покрытия / точные размеры','Схема связей по высоте; горизонтальное расположение см. на поэтажных планах.','engineering')
x0=70;y0=140;ystep=72
for i,(ident,name,z) in enumerate(levels):
    yy=y0+i*ystep;c.setStrokeColor(HexColor('#b8c9cb'));c.line(x0,yy,600,yy)
    txt(c,x0,yy+9,f'{ident}  {fmt(z)} м',11);txt(c,x0+120,yy+9,name,10,MUTED)
for x,name in ((410,'A'),(465,'B'),(520,'C')):
    c.setStrokeColor(HexColor('#b98540'));c.setLineWidth(2);c.line(x,y0,x,y0+5*ystep)
    for i in range(5):c.line(x-8,y0+i*ystep+10,x+8,y0+(i+1)*ystep-10)
    txt(c,x,y0+5*ystep+32,name,11,INK,'center')
txt(c,465,y0+5*ystep+54,'Лестницы',10,INK,'center')
xx=580;c.setFillColor(HexColor('#d4e7ee'));c.rect(xx-13,y0-10,26,5*ystep+20,fill=1,stroke=0);txt(c,xx,y0+5*ystep+32,'Лифт',9,INK,'center')
yy=H-145
for title,body in [
 ('Лестницы A / B / C','Ядро на плане 3 × 8 м. Ширина марша 1.36 м. На один этаж: два марша по 15 ступеней; всего 30 подъёмов по 0.16 м = 4.80 м. Проступь 0.28 м. В комплексе 450 ступеней; каждая записана в инженерный CSV.'),
 ('Лифт на шесть остановок','Шахта 4 × 6 м, кабина по полу 3.80 × 5.80 м. Внутренний световой размер между панелями: 3.58 × 5.68 м; высота 3.39 м. Полностью открытый дверной просвет 3.06 м. Пол кабины 0.20 м толщиной.'),
 ('Стены и обычные проходы','Основная стена 0.16 м, высота 3.80 м. Обычный открытый вход имеет ширину 2.00 м; низ перемычки 2.60 м. Нижняя окрашенная часть стены 1.30 м. Коридоры отделений 6 м; центральный ствол 8 м; поперечный холл 10 м; наружная галерея 4 м.'),
 ('Основания и ограждения','Обычный пол: верх на отметке этажа, низ на 0.20 м ниже. Наружное основание: от -0.50 до -0.20 м. Зелёные вставки: от 0 до +0.04 м. Парапет кровли 0.22 м толщиной и 1.10 м высотой. Наружный забор 0.16 м толщиной, 2.20 м высотой.')]:
    txt(c,665,yy,title,12);yy=para(c,665,yy-21,body,470,10,15)-25
c.showPage()

# Complete unabridged room register, one table per location.
for ident in ORDER:
    l=LOC[ident]
    W,H=page(c,landscape(A3),'Реестр / '+ident+' / '+l['name'],'Глобальные координаты в метрах. W × D - границы модели; «в свету» - без 0.16 м стен.','register_'+ident)
    columns=[45,105,430,545,660,740,850,955,1085]
    heads=['ID / тип','Название зоны','X; Y','W × D, м','S, м²','В свету W × D','Вход X; Y','Z, м','Пол']
    y=H-128
    c.setFillColor(HexColor('#dcebed'));c.rect(40,y-9,W-80,27,fill=1,stroke=0)
    for x,h in zip(columns,heads):txt(c,x,y,h,8.5)
    y-=31
    for i,r in enumerate(l['rooms']):
        xg=l['origin'][0]+r['rect'][0];yg=l['origin'][1]+r['rect'][1];w,h=r['rect'][2:]
        name_lines=wrap(r['name'],310,8.2);rh=max(19,len(name_lines)*11+5)
        if i%2==0:c.setFillColor(HexColor('#eff3f1'));c.rect(40,y-rh+6,W-80,rh,fill=1,stroke=0)
        values=[r['id']+(' / П' if r['enclosed'] else ' / О'),' ',f'{fmt(xg)}; {fmt(yg)}',f'{fmt(w)} × {fmt(h)}',fmt(w*h),f'{fmt(w-.16)} × {fmt(h-.16)}' if r['enclosed'] else '-',f"{fmt(l['origin'][0]+r['door'][0])}; {fmt(l['origin'][1]+r['door'][1])}",fmt(l['origin'][2]),'плитка' if r['enclosed'] else 'асфальт*' if r['id'] in ('S05','S15') else 'асфальт']
        for x,v in zip(columns,values):txt(c,x,y,v,8.0)
        for j,line in enumerate(name_lines):txt(c,columns[1],y-j*11,line,8.2)
        y-=rh
    para(c,45,65,'П - закрытое помещение; О - открытая зона. *S05/S15: асфальт с зелёной вставкой, размеры вставки на листе ST. Вход открытой зоны - опорная точка из данных, не отдельная дверь. Площадь S не исключает стены и оборудование.',W-90,8.3,12)
    c.showPage()
c.save()

# Data QA: no lost zones, unchanged executable/map, grass equals runtime footprints.
reader=PdfReader(PDF)
assert sum(len(v) for v in CELLS.values())==128848
assert len(ROOMS)==333 and len({r['id'] for r in ROOMS})==333
assert len([f for f in FEATURES if f['kind']=='step'])==450
assert len([f for f in FEATURES if f['kind']=='hvac'])==8
assert len(reader.pages)==len(PAGE_INDEX)==29
alltext='\n'.join(p.extract_text() or '' for p in reader.pages)
for r in ROOMS:assert r['id'] in alltext and r['name'] in alltext, r['id']
assert all((p.mediabox.width>0 and p.mediabox.height>0) for p in reader.pages)
manifest=dict(world_id=DATA['map_id'],source_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
              executable_sha256=MARKER['build_identity'].split(':')[0],rooms=333,pdf_pages=len(reader.pages),
              page_index=PAGE_INDEX,steps=450,hvac=8,
              green_patches=[s for s in SURFACES if s['material']=='green'],
              green_area_m2=sum(s['w']*s['h'] for s in SURFACES if s['material']=='green'),
              runtime_cells=sum(len(v) for v in CELLS.values()),floor_strips=len(SURFACES),walls_and_lintels=len(WALLS),engineering_components=len(FEATURES))
manifest['geometry_sources']={name:hashlib.sha256((ROOT/'unreal/EchoWard/Source/EchoWard'/name).read_bytes()).hexdigest() for name in ['WardBookWorld.cpp','WardHospitalDetails.cpp']}
(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'pdf':str(PDF),'pages':len(reader.pages),'rooms':333,'green_m2':manifest['green_area_m2']},ensure_ascii=False))
