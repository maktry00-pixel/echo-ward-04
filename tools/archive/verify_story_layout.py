"""Independent 0.5m navigation raster, capsule clearance, all room destinations.

This is a static plan check. Unreal separately sweeps the real capsule along the
resulting routes and physically walks new stairs/tunnels in its assessment.
"""
from collections import deque
from pathlib import Path
import json, math
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
OUT=ROOT/'artifacts/unreal_hospital_v3'
D=json.loads(SOURCE.read_text(encoding='utf-8'))
LEVELS=[-9.6,-4.8,0,4.8,9.6,14.4]; ZINDEX={round(z,2):i for i,z in enumerate(LEVELS)}
X0,Y0=-181,-191; RES=.5; W,H=924,1144; PLANE=W*H
cells={z:{} for z in LEVELS}; rooms=[]; portals=[]
def rect(z,r,ri=-1,air=False,origin=(0,0)):
    x,y,w,h=r; x=round(x+origin[0]); y=round(y+origin[1])
    for a in range(x,x+w):
        for b in range(y,y+h):
            if (a,b) not in cells[z] or cells[z][a,b][0]<0:cells[z][a,b]=(ri,air)
            elif ri<0 and cells[z][a,b][1] and not air:cells[z][a,b]=(cells[z][a,b][0],False)
for l in D['locations']:
    ox,oy,z=l['origin']; z=round(z,2)
    for r in l['rooms']:
        ri=len(rooms); row=dict(r,site=l['id'],origin=l['origin']);rooms.append(row)
        rect(z,r['rect'],ri if r['enclosed'] else -1,not r['enclosed'],(ox,oy))
        portals.append((ox+r['door'][0],oy+r['door'][1],z,r['door_side'],2))
        for e in r.get('extra_doors',[]):portals.append((ox+e['at'][0],oy+e['at'][1],z,e['side'],e.get('width',2)))
    for r in l['corridors']:rect(z,r,-1,l['id'] in ('ST','RF'),(ox,oy))
    for r in l['cores']:rect(z,r['rect'],-1,l['id'] in ('ST','RF'),(ox,oy))
for x,y,w,h,z in D['passages']:rect(z,[x,y,w,h])
for e in D.get('site_entries',[]):
    x,y,z=e['at'];portals.append((x,y,z,e['side'],e['width']))
for e in D.get('escape_niches',[]):
    x,y,z=e['entry'];portals.append((x,y,z,e['side'],2))
mask=np.zeros((6,H,W),dtype=np.bool_)
def paint(z,x,y,w,h,value,margin=0):
    # Sample-centre inclusion, matching continuous rectangular obstacles.
    a=max(0,math.ceil((x-margin-X0)/RES-.5)); b=min(W,math.floor((x+w+margin-X0)/RES-.5)+1)
    c=max(0,math.ceil((y-margin-Y0)/RES-.5)); e=min(H,math.floor((y+h+margin-Y0)/RES-.5)+1)
    if a<b and c<e:mask[ZINDEX[round(z,2)],c:e,a:b]=value
for z,cs in cells.items():
    for x,y in cs:paint(z,x,y,1,1,True)
for r in [[-180,-190,172,570],[104,-190,176,570],[-8,-190,112,190],[-8,100,112,280]]:paint(0,*r,True)
walls=[]
portal_lines={}
for px,py,pz,s,pw in portals:
    ns=s in ('north','south')
    portal_lines.setdefault((pz,round(py if ns else px,2),ns),[]).append((px if ns else py,pw))
for z,cs in cells.items():
    for (x,y),(ri,air) in cs.items():
        for side,(dx,dy) in enumerate(((1,0),(0,1),(-1,0),(0,-1))):
            other=cs.get((x+dx,y+dy))
            if other is not None and side>=2:continue
            if other is not None and ((other[0]==ri and air==other[1]) or (air and other[1])):continue
            if other is None and air:continue
            mx,my=x+.5+dx*.5,y+.5+dy*.5
            mid=my if dx else mx;lo,hi=mid-.5,mid+.5
            cuts=sorted((max(lo,c-w/2),min(hi,c+w/2)) for c,w in portal_lines.get((z,round(mx if dx else my,2),not bool(dx)),[]) if c+w/2>lo and c-w/2<hi)
            def wall(a,b):
                if b-a<.0001:return
                r=[mx-.08,a,.16,b-a] if dx else [a,my-.08,b-a,.16]
                paint(z,*r,False,.35);walls.append(dict(z=z,rect=r))
            cursor=lo
            for a,b in cuts:wall(cursor,a);cursor=max(cursor,b)
            wall(cursor,hi)
for p in D['partitions']:
    ax,ay,z=p['a'];bx,by,_=p['b'];t=p['thickness']
    r=[min(ax,bx)-t/2,min(ay,by)-t/2,abs(ax-bx)+t,abs(ay-by)+t]
    paint(z,*r,False,.35)
    walls.append(dict(z=z,rect=r,type=p['type']))
for p in D['collision_features']:paint(p['z'],*p['rect'],False,.35)
# Exact rectangular lift and stair openings are absent at upper floor surfaces.
for z in LEVELS:
    paint(z,48,4,4,6,False)
    for s in D['stairs']:
        if s['z_min']<=z<=s['z_max']:paint(z,s['rect'][0],s['rect'][1],3,7,False,.35)
links={}
def node(x,y,z):
    i=math.floor((x-X0)/RES);j=math.floor((y-Y0)/RES)
    assert 0<=i<W and 0<=j<H,(x,y,z)
    return ZINDEX[round(z,2)]*PLANE+j*W+i
flat=mask.reshape(-1)
def nearest(x,y,z,radius=3):
    n=node(x,y,z)
    if flat[n]:return n
    opts=[]
    for yy in range(-radius,radius+1):
        for xx in range(-radius,radius+1):
            k=n+yy*W+xx
            if 0<=k<len(flat) and flat[k]:opts.append((xx*xx+yy*yy,k))
    return min(opts)[1] if opts else None
for s in D['stairs']:
    x,y,_,_=s['rect']
    z=s['z_min']
    while z<s['z_max']-.01:
        a=nearest(x+3.5,y+7.4,z);b=nearest(x+3.5,y+7.4,round(z+4.8,2))
        assert a is not None and b is not None,('stair approach',s['id'],z)
        links.setdefault(a,[]).append(b);links.setdefault(b,[]).append(a);z=round(z+4.8,2)
start=nearest(*D['start'][:2],0)
parent=np.full(len(flat),-1,dtype=np.int32);parent[start]=start;q=deque([start])
while q:
    n=q.popleft(); within=n%PLANE;ix=within%W;iy=within//W
    ns=[]
    if ix:ns.append(n-1)
    if ix<W-1:ns.append(n+1)
    if iy:ns.append(n-W)
    if iy<H-1:ns.append(n+W)
    ns.extend(links.get(n,()))
    for k in ns:
        if flat[k] and parent[k]<0:parent[k]=n;q.append(k)
def point(n):
    lev=n//PLANE;v=n%PLANE
    return [round(X0+(v%W+.5)*RES,3),round(Y0+(v//W+.5)*RES,3),LEVELS[lev]]
unreachable=[];routes=[]
for r in rooms:
    ox,oy,z=r['origin'];x,y=r['door'];south=r['door_side']=='south'
    target=nearest(ox+x,oy+y+(-1.4 if south else 1.4),z,1)
    if target is None or parent[target]<0:unreachable.append(r['id']);continue
    path=[];p=target
    while parent[p]!=p:path.append(p);p=int(parent[p])
    path.append(p);path.reverse()
    compressed=[path[0]];direction=None
    for i in range(1,len(path)):
        delta=path[i]-path[i-1]
        if direction is not None and delta!=direction:compressed.append(path[i-1])
        direction=delta
    compressed.append(path[-1])
    routes.append(dict(zone=r['id'],points=[point(n) for n in dict.fromkeys(compressed)],
        distance_m=round(sum(RES if abs(path[i]-path[i-1]) in (1,W) else 4.8 for i in range(1,len(path))),2)))
site_origins={l['id']:l['origin'] for l in D['locations']}
bad_bays=[];bay_routes=[]
for bay in D['subrooms']:
    ox,oy,z=site_origins[bay['site']];x,y,w,h=bay['rect']
    # Recesses may occupy a mathematical centre. Require an actually reachable
    # interior standing position, at least 0.4m inside every bay boundary.
    reachable=False
    for yy in np.arange(oy+y+.4,oy+y+h-.39,.5):
        for xx in np.arange(ox+x+.4,ox+x+w-.39,.5):
            t=node(xx,yy,z)
            if flat[t] and parent[t]>=0:reachable=True;break
        if reachable:break
    if not reachable:bad_bays.append(bay['id'])
    else:
        path=[];p=t
        while parent[p]!=p:path.append(p);p=int(parent[p])
        path.append(p);path.reverse();compressed=[path[0]];direction=None
        for i in range(1,len(path)):
            delta=path[i]-path[i-1]
            if direction is not None and delta!=direction:compressed.append(path[i-1])
            direction=delta
        compressed.append(path[-1])
        bay_routes.append(dict(bay=bay['id'],zone=bay['zone'],points=[point(n) for n in dict.fromkeys(compressed)]))
report=dict(success=not unreachable and not bad_bays,grid_m=RES,capsule_radius_m=.35,reachable=len(routes),total=len(rooms),
    unreachable=unreachable,walkable_samples=int(flat.sum()),visited_samples=int((parent>=0).sum()),
    reachable_subrooms=len(D['subrooms'])-len(bad_bays),unreachable_subrooms=bad_bays,
    per_building_shortcuts={s:sum(p['site']==s for p in D['shortcuts']) for s in {r['site'] for r in rooms}},
    note='Static navigation plus explicit stair graph; bays require reachable interior standing space, not an empty mathematical centre. Real UE capsule tests are separate.')
(OUT/'navigation_audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'navigation_routes.json').write_text(json.dumps(routes,ensure_ascii=False,separators=(',',':')),encoding='utf-8')
(OUT/'raster_walls.json').write_text(json.dumps(walls,separators=(',',':')),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
if unreachable or bad_bays:raise SystemExit(1)
# The same independently generated routes are exercised by native sweeps.
D['navigation_routes']=routes
D['bay_navigation_routes']=bay_routes
(OUT/'bay_navigation_routes.json').write_text(json.dumps(bay_routes,separators=(',',':')),encoding='utf-8')
SOURCE.write_text(json.dumps(D,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
