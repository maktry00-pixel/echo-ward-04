"""Read-only metric atlas of the verified v11 game, including embedded registers."""
import collections,csv,hashlib,io,json,math,zipfile
from pathlib import Path
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A2,A3,landscape
from reportlab.lib.colors import HexColor
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from pypdf import PdfReader,PdfWriter

ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'output/pdf/ECHO_Full_Map_2026_10_27';TMP=ROOT/'tmp/pdfs/atlas_current_20261027'
OUT.mkdir(parents=True,exist_ok=True);TMP.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json';D=json.loads(SOURCE.read_text(encoding='utf-8'))
V=json.loads((ROOT/'unreal/EchoWard/Saved/PackagedWindows/package_verified.json').read_text(encoding='utf-8'))
SHA=hashlib.sha256(SOURCE.read_bytes()).hexdigest();assert V['success'] and V['map_sha256']==SHA and D['review_revision']==19 and V['revision']==33
assert SOURCE.read_bytes()==(ROOT/'unreal/EchoWard/Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
pdfmetrics.registerFont(TTFont('Ward',str(ROOT/'unreal/EchoWard/Content/CampaignData/Fonts/NotoSans.ttf')))
INK='#18383c';MUTED='#576f73';ACCENT='#ad4248';TILE='#e4f1f0';VOID='#f2f0e9';ASPHALT='#b5bdc5';GRASS='#a6c88a';BLUE='#4a91aa';GOLD='#b37d36'
PDF=OUT/'ЭХО_полная_карта_27-10-2026.pdf';C=canvas.Canvas(str(TMP/'body.pdf'),pageCompression=1)
C.setTitle('ЭХО - полная карта текущей игры / 27.10.2026');C.setAuthor('ЭХО / атлас актуальной игровой сборки')
LOC={s['id']:s for s in D['locations']};ROOMS=[dict(r,site=s['id'],origin=s['origin']) for s in D['locations'] for r in s['rooms']]
WALLS=collections.defaultdict(list)
# Current walls are reconstructed below from the current runtime cell algorithm.
FLOORS=collections.defaultdict(list)
for s in D['locations']:
    ox,oy,z=s['origin']
    for r in s['rooms']:FLOORS[z].append(([ox+r['rect'][0],oy+r['rect'][1],*r['rect'][2:]],TILE if r['enclosed'] else ASPHALT))
    for r in s['corridors']+[c['rect'] for c in s['cores']]:FLOORS[z].append(([ox+r[0],oy+r[1],*r[2:]],ASPHALT if s['id'] in ('ST','RF') else TILE))
for x,y,w,h,z in D['passages']:FLOORS[z].append(([x,y,w,h],TILE))
CARS=json.loads((SOURCE.parent/'echo_cars_v31.json').read_text(encoding='utf-8'))
REPLACED={p.get('replaces') for p in CARS['placements']}
FURNITURE=collections.defaultdict(list);FURN_COUNT=collections.Counter();SMALL_COUNT=collections.Counter();DETAIL_COUNT=collections.Counter()
for p in D['prototype_props']+D['environment_props']:
    FURN_COUNT[p['zone']]+=1
    if p['id'] in REPLACED or p['mount']!='floor':continue
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
    text(35,23,'ЭХО / сборка 33 / 27.10.2026 / координаты в метрах',8,MUTED)
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
    for q in D['partitions']:
        if abs(q['a'][2]-z)<.01:p.line(q['a'][:2],q['b'][:2],INK,.5 if overview else .7)
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
        for b in []: # v33 road paint is exported separately; do not draw the old hovering strips
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

# Reconstruct the same one-metre occupancy and aperture clipping as WardBookWorld.cpp.
def rnd(v):return math.floor(v+.5)
CELLS=collections.defaultdict(dict);PORTALS=[]
def fill_cell_rect(origin,rect,owner=-1,outdoor=False,passage=False):
    ox,oy,z=origin;x,y,w,h=rect;x=rnd(ox+x);y=rnd(oy+y)
    for xx in range(x,x+math.ceil(w)):
        for yy in range(y,y+math.ceil(h)):
            old=CELLS[z].get((xx,yy))
            if old is None or old[0]==-1:CELLS[z][xx,yy]=(owner,outdoor)
            elif passage and old[1]:CELLS[z][xx,yy]=(old[0],False)
for i,r in enumerate(ROOMS):
    fill_cell_rect(r['origin'],r['rect'],i if r['enclosed'] else -1,not r['enclosed'])
# Preserve native per-location processing precedence (rooms before corridors).
CELLS.clear();idx=0
for site in D['locations']:
    ox,oy,z=site['origin']
    for r in site['rooms']:
        fill_cell_rect(site['origin'],r['rect'],idx if r['enclosed'] else -1,not r['enclosed']);idx+=1
        doors=([dict(at=r['door'],side=r['door_side'],type=r.get('door_type','ordinary'),width=4 if r['id']=='S06' else 2)] if r['enclosed'] or r['id']=='S06' else [])+r['extra_doors']
        for e in doors:PORTALS.append(dict(zone=r['id'],site=site['id'],at=[ox+e['at'][0],oy+e['at'][1],z],side=e['side'],width=e.get('width',2),type=e.get('type','ordinary'),role=e.get('role','primary')))
    for r in site['corridors']+[c['rect'] for c in site['cores']]:fill_cell_rect(site['origin'],r,-1,site['id'] in ('ST','RF'))
for r in D['site_entries']:PORTALS.append(dict(r,role='site_entry'))
for r in D['escape_niches']:PORTALS.append(dict(at=r['entry'],side=r['side'],width=2,role='niche',zone=r.get('zone',''),type='niche'))
air=D['isolation_airlock']
if 'at' in air:PORTALS.append(dict(at=air['at'],side='south',width=4,role='isolation',type='shutter',zone='E10'))
for x,y,w,h,z in D['passages']:fill_cell_rect([0,0,z],[x,y,w,h],passage=True)
PLINES=collections.defaultdict(list)
for q in PORTALS:
    x,y,z=q['at'];ns=q['side'] in ('north','south');PLINES[z,ns,rnd(y if ns else x)].append(((x if ns else y)-q.get('width',2)/2,(x if ns else y)+q.get('width',2)/2))
series=collections.defaultdict(list)
for z,cells in CELLS.items():
    for (x,y),(owner,outdoor) in cells.items():
        for side,(dx,dy) in enumerate(((1,0),(0,1),(-1,0),(0,-1))):
            other=cells.get((x+dx,y+dy))
            if other and side>=2:continue
            if other and ((other[0]==owner and other[1]==outdoor) or (outdoor and other[1])):continue
            if other is None and outdoor:continue
            ns=bool(dy);fixed=y+.5+dy*.5 if ns else x+.5+dx*.5;lo=x if ns else y;hi=lo+1
            cur=lo
            for a,b in sorted(PLINES.get((z,ns,rnd(fixed)),[])):
                a=max(a,lo);b=min(b,hi)
                if b<=a:continue
                if a>cur:series[z,ns,fixed].append((cur,a))
                cur=max(cur,b)
            if cur<hi:series[z,ns,fixed].append((cur,hi))
for (z,ns,fixed),segs in series.items():
    merged=[]
    for a,b in sorted(segs):
        if merged and a<=merged[-1][1]+1e-6:merged[-1]=(merged[-1][0],max(b,merged[-1][1]))
        else:merged.append((a,b))
    for a,b in merged:WALLS[z].append([a,fixed-.08,b-a,.16] if ns else [fixed-.08,a,.16,b-a])
print('Geometry ready:',sum(len(v) for v in CELLS.values()),'cells;',sum(len(v) for v in WALLS.values()),'wall segments',flush=True)

ZONE={r['id']:r for r in ROOMS};BAYS=collections.defaultdict(list)
for b in D['subrooms']:BAYS[b['zone']].append(b)
ITEMS=collections.defaultdict(list);MOBS=collections.defaultdict(list)
for q in D['story_gameplay']['placements']:ITEMS[q['zone']].append(q)
for q in D['story_gameplay']['mobs']:MOBS[q['zone']].append(q)
MODEL_COUNT=collections.defaultdict(collections.Counter)
for q in D['prototype_props']+D['environment_props']:
    if q['id'] not in REPLACED:MODEL_COUNT[q['zone']][str(q['model'])]+=1
ARRIVAL=json.loads((SOURCE.parent/'echo_arrival_v29.json').read_text(encoding='utf-8'))
RAW={p.name:json.loads(p.read_text(encoding='utf-8-sig')) for p in SOURCE.parent.glob('echo_*.json') if p.name in V['current_data_hashes']}

def circle(p,x,y,r,fill,stroke=None):
    if not p.visible([x-r,y-r,2*r,2*r]):return
    px,py=p.at(x,y);C.setFillColor(color(fill));C.setStrokeColor(color(stroke or fill));C.setLineWidth(.3);C.circle(px,py,r*p.s,stroke=bool(stroke),fill=1)

def more_features(p,z,overview=False):
    if z==0:
        for t in D['campus_upgrade']['trees']:circle(p,*t['at'][:2],min(t['crown_width_m']/2,2.8),'#73916c')
        # Physical upper-level security wall at the clinical boundary, plus outer fence.
        for layer in ('campus_plan','campus_upgrade','finish_upgrade','visual_realism'):
            for q in D[layer].get('items',[]):
                kind=q.get('kind','').lower()
                if any(a in kind for a in ('wall','fence','barrier','gatepost')) and 'bounds' in q:
                    b=q['bounds']
                    if b[2]<3:p.rect([b[0],b[1],b[3]-b[0],b[4]-b[1]],'#766b67')
        x,y,w,h=D['boundary']['rect'];gate=D['boundary']['south_gate']
        p.line((x,y),(gate[0],y),'#62594e',1.4);p.line((sum(gate),y),(x+w,y),'#62594e',1.4)
        p.line((x,y),(x,y+h),'#62594e',1.4);p.line((x+w,y),(x+w,y+h),'#62594e',1.4);p.line((x,y+h),(x+w,y+h),'#62594e',1.4)
        for q in CARS['placements']:
            x,y,zz=q['at'];bb=CARS['vehicles'][q['slug']]['bounds_cm'];ww=(bb[1][0]-bb[0][0])/100;hh=(bb[1][1]-bb[0][1])/100
            angle=math.radians(q['yaw']);pts=[]
            for a,b in [(-ww/2,-hh/2),(ww/2,-hh/2),(ww/2,hh/2),(-ww/2,hh/2)]:pts.append((x+a*math.cos(angle)-b*math.sin(angle),y+a*math.sin(angle)+b*math.cos(angle)))
            if p.visible([x-ww,y-hh,2*ww,2*hh]):
                path=C.beginPath();path.moveTo(*p.at(*pts[0]))
                for pt in pts[1:]:path.lineTo(*p.at(*pt))
                path.close();C.setFillColor(color('#465465'));C.drawPath(path,fill=1,stroke=0)
                if not overview:p.label(x,y,q['id'].split('_')[-1],4.5,'#ffffff')
        # Hero car and initial approach are distinct from the 35 parked vehicles.
        p.rect([47.5,-62.1,5,2.2],'#8e343e');p.label(50,-65,'АВТО ГЕРОЯ',5.5,ACCENT)
    for q in D['site_entries']:
        x,y,zz=q['at']
        if abs(zz-z)<.01:
            ns=q['side'] in ('north','south');w=q.get('width',2)
            p.line((x-w/2,y) if ns else (x,y-w/2),(x+w/2,y) if ns else (x,y+w/2),'#ffffff',2)
            p.line((x-w/2,y) if ns else (x,y-w/2),(x+w/2,y) if ns else (x,y+w/2),BLUE,1)
    if not overview:
        for q in D['story_gameplay']['placements']:
            x,y,zz=q['at']
            if abs(zz-z)<.05:circle(p,x,y,.24,'#bb4b41')
        for q in D['story_gameplay']['mobs']:
            x,y,zz=q['at']
            if abs(zz-z)<.05 and p.visible([x,y,0,0]):p.label(x,y,'М',7,'#9b3273')
        for q in D['network_devices']:
            at=q.get('at',[])
            if len(at)==3 and abs(at[2]-z)<4 and z<=at[2]<z+4:circle(p,at[0],at[1],.13,'#446f9a')

BASE_ARCH=architecture
def architecture(p,z,site=None,furnished=True,overview=False):
    BASE_ARCH(p,z,site,furnished,overview);more_features(p,z,overview)

def pages_table(title,sub,headers,rows,widths,size=8.2):
    rows=list(rows);i=0;part=1
    while i<len(rows):
        w,h=page(title+(f' / {part}' if part>1 else ''),sub);yy=h-125;batch=[]
        while i<len(rows):
            height=max(len(wrap(str(v),ww-10,size)) for v,ww in zip(rows[i],widths))*(size+3)+10
            if yy-29-height<75 and batch:break
            batch.append(rows[i]);yy-=height+(29 if len(batch)==1 else 0);i+=1
        table(headers,batch,widths,size=size);end();part+=1

def short_name(model):return D['prototype_catalog'][str(model)]['name'].split(' - ')[0].split(';')[0].rstrip('.')
def atstr(at):return '; '.join(f(v) for v in at)

def plan_page(title,sub,extent,z,site=None,overview=False,legend=True):
    w,h=page(title,sub,landscape(A2));p=Plan((45,115,1100,h-235),extent);p.start();architecture(p,z,site,overview=overview)
    if site and site in LOC:room_labels(p,LOC[site])
    if overview:
        for s in D['locations']:
            if s['origin'][2]!=z or s['id']=='UT':continue
            ox,oy,_=s['origin'];x,y,ww,hh=s['bounds']
            if s['id']=='ST':
                for r in s['rooms']:
                    a,b,rw,rh=r['rect'];p.label(ox+a+rw/2,oy+b+rh/2,r['id'],6)
            else:p.label(ox+x+ww/2,oy+y+hh/2,s['id'],10)
    p.finish(20 if overview or site=='UT' else 10)
    yy=h-120
    if site and site in LOC:
        s=LOC[site];text(1195,yy,'ЗОНЫ И РАЗМЕРЫ',12);yy-=24
        for r in s['rooms']:
            suffix=f' / {f(r["rect"][2])} x {f(r["rect"][3])} м'
            name=r['name'];line=r['id']+' / '+name+suffix
            while pdfmetrics.stringWidth(line,'Ward',7.6)>w-1245:
                name=name[:-1];line=r['id']+' / '+name+'...'+suffix
            text(1195,yy,line,7.6);yy-=20
    else:
        text(1195,yy,'Ориентиры',12);yy-=24
        for s in D['locations']:
            if s['origin'][2]!=z or s['id']=='UT':continue
            for ln in wrap(s['id']+' / '+s['name'],w-1245,8.5):text(1195,yy,ln,8.5);yy-=13
            yy-=5
    yy=min(yy-20,330)
    for heading,body in [('Покрытия','Зелёный - трава и кроны. Серый - асфальт или кровля. Светло-голубой - пол помещений и крытых переходов. Бежевый - дорожки.'),('Обозначения','Тёмный - стены. Голубой штрих - дверь; красноватый - дополнительный выход; фиолетовый - секрет. Бирюзовый - окно. Охра - лестница, синий - лифт.'),('Предметы и размеры','Контуры - мебель; точка - предмет, М - моб. XYZ и названия - в реестрах. X вправо, Y вниз. Чистые размеры секций - в отдельном разделе. Контуры мебели условные.')]:
        text(1195,yy,heading,10);yy=para(1195,yy-17,body,w-1245,8,12)-16
    assert yy>45,(title,yy)
    end()

w,h=page('ЭХО / полный атлас игровой карты','Текущая Windows-сборка 33 | архитектура 19 | 27 октября 2026','' or landscape(A3),'cover')
text(48,h-145,'562 зоны  /  35 локаций  /  1 939 внутренних секций',23)
y=para(48,h-186,'Вся больница: территория 460 x 570 м, отдельные корпуса, магазины, подвалы, лаборатория, тоннели, верхние галереи и крыша.',1090,13,21)
y=para(48,y-20,'Масштабные планы, размеры в метрах, названия и координаты помещений; реестры мебели, предметов, транспорта, монстров и сюжетных привязок.',1090,12,20)
y=table(['Как читать','Что включено'],[
('Схемы','Общий план шести отметок высоты, отдельный план каждой локации, увеличенные участки улицы и тоннелей.'),
('Реестры','Все 562 зоны и 1 939 секций; размеры, площадь, координаты. Наполнение каждой зоны по типам, сюжетные вещи и точки размещения мобов.'),
('Полные данные','Вложенный ZIP содержит исходные JSON текущей сборки и CSV с точными координатами каждой записи. Это дополнение к читаемым страницам атласа.'),
('Версия','Данные сверены с файлами запускаемой игры по SHA-256. Новая модельная библиотека и отдельный пробный коридор v34 сюда не включены: они ещё не размещены в игре.')],[155,935],y=y-30,size=10)
para(48,115,'Габариты зоны не равны свободной площади: внутри находятся перегородки, мебель и секции. Планы описывают геометрию и размещения; доступность зависит от главы и состояния дверей. Напечатанный масштаб верен при 100%.',1090,10,15)
end()

pages_table('Указатель всех корпусов и этажей','Найди ID локации на общем плане; далее есть отдельный увеличенный лист.', ['ID','Локация','X; Y; Z начала','Габариты, м','Зон'],[(s['id'],s['name'],atstr(s['origin']),f'{f(s["bounds"][2])} x {f(s["bounds"][3])}',len(s['rooms'])) for s in D['locations']],[90,440,220,220,120],9)
for z,title in [(0,'Генплан / первый этаж и территория'),(-9.6,'B2 / центр Гелиос и лаборатории'),(-4.8,'B1 / подвалы и подземная сеть'),(4.8,'Второй этаж / надземные галереи'),(9.6,'Третий этаж'),(14.4,'Крыша / вертолётная площадка')]:
    plan_page(title,f'Полная горизонтальная проекция Z={f(z)} м. Светлое пустое поле - нет пола на этой высоте.',[-195,-205,490,600],z,overview=True)
for name,extent in [('Вход, КПП и общественная зона',[-82,-198,252,174]),('Западная половина комплекса',[-178,-16,168,365]),('Главный корпус и двор',[-38,-50,180,410]),('Восточная половина комплекса',[116,-16,162,365])]:
    plan_page('Улица / '+name,'Здания, асфальт, трава, деревья, граница, дороги и актуальные машины.',extent,0,overview=True)
for s in D['locations']:
    ident=s['id'];ox,oy,z=s['origin'];x,y,ww,hh=s['bounds'];SITE_PAGES[ident]=len(PAGES)+1
    plan_page(ident+' / '+s['name'],f'Начало локации: X={f(ox)}, Y={f(oy)}, Z={f(z)} м. Сетка 5 м (UT: 10 м).',[ox+x-4,oy+y-4,ww+8,hh+8],z,ident)
for name,extent in [('Юг / магазины и КПП',[-172,-192,440,192]),('Запад / подвалы боковых корпусов',[-170,-2,230,352]),('Восток / сеть и выход SV24',[40,-2,230,352])]:
    plan_page('Тоннели / '+name,'Z=-4,8 м. Красные линии - оси связей; оболочка и лестницы показаны по текущей геометрии.',extent,-4.8,'UT')

for s in D['locations']:
    ox,oy,z=s['origin']
    pages_table('Все зоны / '+s['id']+' / '+s['name'],f'План локации: лист {SITE_PAGES[s["id"]]}. XYZ - мировой левый верхний угол; площадь - W x D.', ['ID','Название','X; Y; Z','W x D, м','м²','Класс / главы'],[(r['id'],r['name'],atstr([ox+r['rect'][0],oy+r['rect'][1],z]),f'{f(r["rect"][2])} x {f(r["rect"][3])}',f(r['rect'][2]*r['rect'][3]),r.get('production_class','')+' / '+','.join(map(str,r.get('chapters',[])))) for r in s['rooms']],[90,350,210,135,85,220],8.4)
    REGISTERED.extend(r['id'] for r in s['rooms'])

pages_table('Все внутренние секции','Чистый размер учитывает перегородки. XYZ - мировое начало секции; это подразделение зоны, не дополнительные 1 939 сюжетных зон.', ['Секция','Зона / локация','X; Y; Z','W x D, м','Чистый размер, м','м²'],[(b['id'],b['zone']+' / '+b['site'],atstr([LOC[b['site']]['origin'][0]+b['rect'][0],LOC[b['site']]['origin'][1]+b['rect'][1],LOC[b['site']]['origin'][2]]),f'{f(b["rect"][2])} x {f(b["rect"][3])}',f'{f(b["clear_width_m"])} x {f(b["clear_depth_m"])}',f(b['area_m2'])) for b in D['subrooms']],[140,160,235,185,245,125],8.1)

pages_table('Что находится в каждой зоне','Каталожная мебель и оборудование по типам; координаты каждого экземпляра - в приложении CSV. Мелкие вещи и детали показаны отдельным числом.', ['Зона / место','Мебель и оборудование','Вещи / детали / игровые предметы'],[(r['id']+' / '+r['name'],'; '.join(short_name(m)+' x'+str(n) for m,n in sorted(MODEL_COUNT[r['id']].items(),key=lambda v:int(v[0]))) or 'Нет каталожной мебели',f'{SMALL_COUNT[r["id"]]} / {DETAIL_COUNT[r["id"]]} / {len(ITEMS[r["id"]])}') for r in ROOMS],[230,655,205],8)

pages_table('Игровые предметы / оружие / записки','Все 395 размещений исходного сюжетного слоя. Записки пока заготовки; наличие записи не означает завершённую постановку главы.', ['ID','Предмет','Зона / локация','X; Y; Z','Глава'],[(q['id'],q['label'],q['zone']+' / '+q['site'],atstr(q['at']),q.get('chapter','')) for q in D['story_gameplay']['placements']],[260,320,160,250,100],8.3)
pages_table('Монстры / исходные точки размещения','Точки из текущего игрового слоя. Движение, появление и доступность зависят от правил и главы.', ['ID','Название / вид','Зона / локация','X; Y; Z','Глава'],[(q['id'],q['name']+' / '+q['species'],q['zone']+' / '+q['site'],atstr(q['at']),q['chapter']) for q in D['story_gameplay']['mobs']],[275,310,160,245,100],8.3)
pages_table('Сюжет / пространственные опорные точки','Привязки 19 глав из текущей игры. Полный текст сценария в этот технический атлас не переносится.', ['Глава','Название','Опорные зоны'],[(q['id'],q['title'],', '.join(q['anchors'])) for q in D['story_gameplay']['chapters']],[100,440,550],10)
pages_table('Инфраструктура / отдельные технические залы','17 инфраструктурных зон вне реестра 562 именованных зон.', ['ID','Название','X; Y; Z','W x D, м'],[(r['id'],r['name'],atstr([r['rect'][0],r['rect'][1],r['z']]),f'{f(r["rect"][2])} x {f(r["rect"][3])}') for r in D['infrastructure_zones']],[150,420,290,230],9)
pages_table('Лестницы / все 37 узлов','Каждый прямоугольник - габарит лестничного узла, Z - нижняя и верхняя остановка.', ['ID','X; Y','W x D, м','Низ Z','Верх Z'],[(q['id'],atstr(q['rect'][:2]),atstr(q['rect'][2:]),f(q['z_min']),f(q['z_max'])) for q in D['stairs']],[240,270,230,175,175],10)
pages_table('Лифт / шесть остановок','Единая шахта X=48..52; Y=4..10 м. Положение одинаково на всех уровнях.', ['Этаж','Отметка Z','Дверь X; Y; Z','Холл X; Y; Z'],[(q['site'],f(q['z']),atstr(q['door']),atstr(q['lobby'])) for q in D['lift']['landings']],[150,200,370,370],11)
pages_table('Подземные связи','19 соединений. Все вершины оси маршрута в порядке следования; XYZ в метрах.', ['ID / откуда -> куда','Ширина','Координаты оси'],[(q['id']+' / '+q.get('source','')+' -> '+q.get('target',''),f(q['width_m']), ' / '.join(atstr(a) for a in q['points'])) for q in D['utility_links']],[300,100,690],8.2)
pages_table('Транспорт / актуальные размещения','35 стоящих машин и отдельная машина героя. Это текущие положения после исправления пересечений.', ['ID','Автомобиль','Назначение / зона','X; Y; Z','Поворот'],[(q['id'],CARS['vehicles'][q['slug']]['title'],q['role']+' / '+q.get('zone33','улица'),atstr(q['at']),str(q['yaw'])+'°') for q in CARS['placements']]+[('HERO','Mercedes-Benz W212','Машина героя / прибытие',atstr([50,-61,.0005]),'по маршруту')],[140,340,250,250,110],8.2)
pages_table('Территория / дороги и газоны','Асфальтовые полосы. Полный реестр 443 газонов и деревьев с размерами сохранён во вложении.', ['Тип','X; Y','W x D, м','Площадь, м²'],[('Дорога '+str(i+1),atstr(r[:2]),atstr(r[2:]),f(r[2]*r[3])) for i,r in enumerate(D['environment_dressing']['roads'])]+[('Граница комплекса',atstr(D['boundary']['rect'][:2]),atstr(D['boundary']['rect'][2:]),f(460*570))],[220,290,290,290],10)

# Complete machine-readable attachments keep all layers, rather than discarding tiny placements.
def flat_csv(name,rows):
    if isinstance(rows,dict):rows=[dict(key=k,value=v) for k,v in rows.items()]
    else:rows=list(rows)
    if not rows:return
    if not isinstance(rows[0],dict):rows=[dict(index=i,value=v) for i,v in enumerate(rows)]
    fields=list(dict.fromkeys(k for r in rows for k in r))
    rows=[{k:json.dumps(v,ensure_ascii=False,separators=(',',':')) if isinstance(v,(list,dict)) else v for k,v in r.items()} for r in rows]
    csv_file(name+'.csv',fields,rows)
flat_csv('01_zones',[dict(id=r['id'],site=r['site'],name=r['name'],global_xyz=[r['origin'][0]+r['rect'][0],r['origin'][1]+r['rect'][1],r['origin'][2]],width_m=r['rect'][2],depth_m=r['rect'][3],area_m2=r['rect'][2]*r['rect'][3],**{k:v for k,v in r.items() if k not in ('id','site','name','origin')}) for r in ROOMS])
flat_csv('02_subrooms',[dict(b,global_xyz=[LOC[b['site']]['origin'][0]+b['rect'][0],LOC[b['site']]['origin'][1]+b['rect'][1],LOC[b['site']]['origin'][2]]) for b in D['subrooms']])
for k in ['prototype_catalog','prototype_props','environment_props','furnishings','hideouts','stairs','utility_links','windows','network_devices','threat_nodes','secret_passages','zone_evolution','site_entries','partitions','structural_members','structural_support','raised_access','power_graph','acoustic_graph','monster_rules','navigation_routes']:
    flat_csv('map_'+k,D[k])
for k in ['clutter_dressing','microdetails_dressing','item_types_dressing','hospital_services','campus_plan','campus_upgrade','finish_upgrade','visual_realism']:
    if 'items' in D[k]:flat_csv(k+'_items',D[k]['items'])
flat_csv('trees',D['campus_upgrade']['trees']);flat_csv('lawns',[dict(x=r[0],y=r[1],width=r[2],depth=r[3],area=r[2]*r[3]) for r in D['environment_dressing']['lawns']])
flat_csv('roads',[dict(x=r[0],y=r[1],width=r[2],depth=r[3]) for r in D['environment_dressing']['roads']])
flat_csv('registered_portals',PORTALS)
for k in ['placements','mobs','chapters','scenes','side_stories']:flat_csv('story_'+k,D['story_gameplay'][k])
for fn,obj in RAW.items():
    if 'props' in obj:flat_csv(fn.removesuffix('.json')+'_props',obj['props'])
flat_csv('cars_actual',CARS['placements'])

w,h=page('Полнота атласа / данные внутри PDF','Приложение ECHO_current_map_data.zip: исходные файлы и полный набор координатных реестров.')
y=table(['Слой','Содержание и способ чтения'],[
('Планы','Все 35 локаций и шесть высот. Стены пересчитаны по текущим ячейкам игры с дверными проёмами; перегородки, окна, входы, лестницы и шахта нанесены из актуальных данных.'),
('Реестры страниц','562 зоны, 1 939 секций, содержимое каждой зоны, 395 сюжетных размещений, 46 мобов, 19 глав, 37 лестниц, 6 остановок лифта, 19 подземных соединений, 36 машин.'),
('Мелкие объекты','Все точные записи сохранены в CSV и JSON: мебель, детали, инженерия, деревья, газоны, сети, секреты, сюжетные состояния и слои более позднего улучшения качества.'),
('Как открыть','В настольном PDF-просмотрщике открой панель вложений / значок скрепки и сохрани ZIP. Некоторые браузеры не показывают вложения. Основные планы и реестры доступны прямо на страницах.'),
('Статус','Документ описывает существующие данные. Отдельные монстры, предметы и мебель ещё прототипные. Привязки глав не означают, что вся длительная кампания уже закончена.')],[180,910],size=10)
para(48,y-28,'Источник: hospital_world.json, совпадает с текущей установленной сборкой.\nSHA-256 карты: '+SHA+'\nСборка 33 проверена: 159 проверок карты; 1 012 дверей по журналу проверки. Атлас не запускает новую проверку игры и не меняет сохранения.',1090,9,15);end()
index=list(PAGES)
for start in range(0,len(index),29):
    w,h=page('Содержание / '+str(start//29+1),'Кликабельный список. В PDF также доступны закладки всех разделов.');yy=h-125
    for entry in index[start:start+29]:
        text(48,yy,str(entry['page']).zfill(3),9,ACCENT);text(95,yy,entry['title'],9)
        C.linkRect('',entry['key'],(43,yy-4,w-43,yy+13),relative=0,thickness=0);yy-=22
    end()
C.save();assert len(REGISTERED)==562 and len(set(REGISTERED))==562
manifest=dict(date='2026-10-27',game_build=33,architecture_revision=19,pages=len(PAGES),zones=len(ROOMS),locations=len(LOC),subrooms=len(D['subrooms']),source_sha256=SHA,page_index=PAGES,csv_files=len(list(OUT.glob('*.csv'))),source_files=list(V['current_data_hashes']))
(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
readme='ЭХО - данные карты текущей сборки 33, 27.10.2026.\nCSV: UTF-8 BOM, разделитель ;. Вложенные поля содержат JSON.\nМетры, глобальные координаты если в имени поля global_xyz или at; локальные rect/door требуют прибавления origin локации.\nsubrooms.rect локальный, partitions.a/b глобальный. Автомобили - echo_cars_v31.json revision33.\nПрямоугольники логической мебели не повторяют точный силуэт всех детальных мешей.\nКлассы, состояния дверей и сюжетные привязки - часть исходных JSON.\nСодержимое ещё не интегрированной библиотеки v34 не включено.\n'
archive=TMP/'ECHO_current_map_data.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as zf:
    zf.writestr('ПРОЧТИ.txt',readme.encode('utf-8'))
    for fn,expected in V['current_data_hashes'].items():
        p=SOURCE.parent/fn;assert hashlib.sha256(p.read_bytes()).hexdigest()==expected,fn;zf.write(p,'game_data/'+fn)
    for p in sorted(OUT.glob('*.csv')):zf.write(p,'registers/'+p.name)
    zf.write(OUT/'manifest.json','manifest.json');zf.write(ROOT/'unreal/EchoWard/Saved/PackagedWindows/package_verified.json','package_verified.json')
reader=PdfReader(TMP/'body.pdf');writer=PdfWriter();writer.clone_document_from_reader(reader);writer.add_attachment(archive.name,archive.read_bytes())
with PDF.open('wb') as stream:writer.write(stream)
reader=PdfReader(PDF);alltext='\n'.join(p.extract_text() or '' for p in reader.pages)
for r in ROOMS:assert r['id'] in alltext and r['name'] in alltext,r['id']
for b in D['subrooms']:assert b['id'] in alltext,b['id']
assert len(reader.pages)==len(PAGES) and len(reader.attachments)==1
with zipfile.ZipFile(io.BytesIO(reader.attachments[archive.name][0])) as zf:
    assert hashlib.sha256(zf.read('game_data/hospital_world.json')).hexdigest()==SHA
result=dict(success=True,pdf=str(PDF),pages=len(reader.pages),mb=round(PDF.stat().st_size/1e6,2),zones=len(ROOMS),locations=len(LOC),subrooms=len(D['subrooms']),source_sha256=SHA,embedded_archive=archive.name,csv_files=manifest['csv_files'])
(OUT/'pdf_check.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(result,ensure_ascii=False),flush=True)
