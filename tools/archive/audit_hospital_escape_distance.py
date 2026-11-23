"""Conservative path distance on the independent half-metre capsule raster.

Unlike authoring centreline samples, this covers every reachable standing sample
inside corridor cells. Distances follow walkable floor, not a line through walls.
"""
import contextlib,hashlib,io,json,math,runpy
from collections import deque
from pathlib import Path
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
with contextlib.redirect_stdout(io.StringIO()):
    g=runpy.run_path(str(ROOT/'tools/verify_story_layout.py'))
d=g['D'];mask=g['mask'];flat=mask.reshape(-1);parent=g['parent'];W=g['W'];PLANE=g['PLANE'];node=g['node'];point=g['point'];RES=g['RES']
dist=np.full(len(flat),-1,dtype=np.int16);q=deque()
sources=[]
for l in d['locations']:
    ox,oy,z=l['origin']
    for r in l['rooms']:
        if not r['enclosed']:continue
        sources.append([ox+r['door'][0],oy+r['door'][1],z])
        sources += [[ox+e['at'][0],oy+e['at'][1],z] for e in r.get('extra_doors',[])]
sources += [n['entry'] for n in d['escape_niches']]
# Seed standing cells within 0.8m of a portal. Add 1m to every result as a
# conservative allowance for reaching the exact threshold from the seed cell.
for x,y,z in sources:
    for dx in (-1,-.5,0,.5,1):
        for dy in (-1,-.5,0,.5,1):
            n=node(x+dx,y+dy,z)
            if flat[n] and dist[n]<0 and math.dist(point(n)[:2],[x,y])<=.8:
                dist[n]=0;q.append(n)
while q:
    n=q.popleft()
    if dist[n]>=60:continue
    within=n%PLANE;ix=within%W;iy=within//W;ns=[]
    if ix:ns.append(n-1)
    if ix<W-1:ns.append(n+1)
    if iy:ns.append(n-W)
    if iy<g['H']-1:ns.append(n+W)
    for k in ns:
        if flat[k] and dist[k]<0:dist[k]=dist[n]+1;q.append(k)
sites={l['id']:l for l in d['locations']};report=[];bad=[];samples=[]
for site,l in sites.items():
    if site in ('ST','RF'):continue
    ox,oy,z=l['origin'];owned=set()
    rects=[[ox+x,oy+y,w,h] for x,y,w,h in l['corridors']]
    if site=='DG':rects += [p[:4] for p in d['passages'] if p[4]==0 and p[0]==-139 and p[1] in (46,68)]
    if site=='UT':rects += [p[:4] for p in d['passages'] if p[4]<0]
    if site=='1F':rects += [p[:4] for p in d['passages'] if p[4]==0 and p[0]!=-139]
    for x,y,w,h in rects:
        for yy in np.arange(y+.25,y+h,.5):
            for xx in np.arange(x+.25,x+w,.5):
                n=node(xx,yy,z)
                cell=g['cells'][z].get((math.floor(xx),math.floor(yy)))
                if flat[n] and parent[n]>=0 and cell and cell[0]<0:owned.add(n)
    failures=[];worst=0
    for n in owned:
        distance=float(dist[n]*RES+1) if dist[n]>=0 else 999
        worst=max(worst,distance)
        if distance>15:failures.append(n)
    if failures:
        failures.sort(key=lambda n:int(dist[n]) if dist[n]>=0 else 1000,reverse=True)
        bad += [dict(site=site,at=point(n),distance_upper_m=float(dist[n]*RES+1) if dist[n]>=0 else None) for n in failures]
    report.append(dict(site=site,samples=len(owned),over_15m=len(failures),maximum_upper_m=worst,
        worst_points=[point(n) for n in failures[:8]]))
result=dict(success=not bad,grid_m=RES,source_sha256=hashlib.sha256(g['SOURCE'].read_bytes()).hexdigest(),metric='Manhattan walkable-floor path upper bound + 1m threshold allowance',
    scope='Chapter 0; reachable corridor standing cells, fixed architectural obstacles, no movable barricades. Stair floors excluded.',
    per_site=report,failed_samples=len(bad),failures=bad)
out=ROOT/'artifacts/unreal_hospital_v4';out.mkdir(exist_ok=True)
(out/'escape_distance_audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='failures'},ensure_ascii=False,indent=2))
