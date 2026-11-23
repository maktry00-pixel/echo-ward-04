"""Independent data contracts and escape paths with real chapter door states."""
import contextlib,hashlib,io,json,math,runpy
from collections import Counter,deque
from pathlib import Path
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
revision=json.loads((ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json').read_text(encoding='utf-8')).get('review_revision',5)
OUT=ROOT/f'artifacts/unreal_hospital_v{revision}';OUT.mkdir(exist_ok=True)
with contextlib.redirect_stdout(io.StringIO()):g=runpy.run_path(str(ROOT/'tools/verify_story_layout.py'))
d=g['D'];source=g['SOURCE'];base_mask=g['mask'].copy();W=g['W'];H=g['H'];PLANE=g['PLANE'];RES=g['RES'];node=g['node'];point=g['point']
zones={r['id']:(l,r) for l in d['locations'] for r in l['rooms']};sites={l['id']:l for l in d['locations']}
checks={}
def check(k,v):checks[k]=bool(v)
old=json.loads((ROOT/'artifacts/unreal_hospital_v4/hospital_world_source.json').read_text(encoding='utf-8'))
oldzones={r['id']:r for l in old['locations'] for r in l['rooms']}
check('all_334_existing_ids_and_areas_preserved',set(oldzones)<=set(zones) and all(zones[k][1]['rect'][2]*zones[k][1]['rect'][3]>=r['rect'][2]*r['rect'][3] for k,r in oldzones.items()))
check('562_zones_35_locations_37_stairs',len(zones)==562 and len(sites)==35 and len(d['stairs'])==37)
listener=next(m for m in d['monster_rules'] if m['id']=='listener')
check('listener_B_finale_only',listener['combat_class']=='B' and listener['kill_condition']=='finale_only')
distribution=Counter(s['site'] for s in d['secret_passages'])
check('20_secrets_across_9_departments',len(d['secret_passages'])==20 and len(distribution)==9 and max(distribution.values())<=3 and all(zones[s['zone']][1]['production_class']=='C' for s in d['secret_passages']))
check('PS_RH_at_least_10_playable_side_story_rooms',all(sum(r['production_class']!='C' and bool(r['side_story']) for r in sites[s]['rooms'])>=10 for s in ('PS','RH')))
check('three_distinct_room_pair_loops_per_hospital_floor',all(a['independent_room_pairs']>=3 for a in d['loop_audit']))
hidecount=Counter(h['zone'] for h in d['hideouts'])
check('A_two_exits_two_hides_B_one_two_exits_one_hide',all((1+len(r['extra_doors'])>=2 and hidecount[r['id']]>=2) if r['production_class']=='A' else (1<=1+len(r['extra_doors'])<=2 and hidecount[r['id']]>=1) for l,r in zones.values() if r['enclosed'] and r['production_class']!='C'))
check('wardrobes_and_beds_exist',all(any(h['kind']==k for h in d['hideouts']) for k in ('wardrobe','under_bed','architectural_recess')))
furnished={f['zone'] for f in d['furnishings']}
check('all_enclosed_A_B_rooms_have_physical_furniture',all(r['id'] in furnished for l,r in zones.values() if r['enclosed'] and r['production_class']!='C'))
check('DG_halls_registered_with_classes_and_states',all(k in zones and zones[k][1]['production_class']=='A' and zones[k][1]['environment_states'] for k in ('DG25','DG26')))
check('dogleg_connections_for_all_six_pavilions',all(len(next(u for u in d['utility_links'] if u['id']=='U_LINK_'+s)['points'])>=6 for s in ('DG','IF','PS','RH','HS','SV')))
check('all_11_street_buildings_connected_below_ground',all(any(u['target']=='U_'+s for u in d['utility_links']) for s in ('X01','X02','X03','X04','X05','S03','S12','S13','S14','S11','S16')))
check('20_new_stacked_floors_have_two_stairs',len(d['floor_expansion'])==20 and all(len([s for s in d['stairs'] if s['id'] in (f['building'],f['building']+'_EAST',f['building']+'_W',f['building']+'_E') and s['z_max']>=f['z']])>=2 for f in d['floor_expansion']))
# Sample every point along the upper SV service route in actual indoor floor
# cells. Asphalt does not satisfy this test.
sv=next(u for u in d['utility_links'] if u['target']=='SV24');covered=True
for a,b in zip(sv['points'][1:],sv['points'][2:]):
    for t in np.linspace(0,1,max(2,int(math.dist(a,b)/.5)+1)):
        x,y,z=[a[i]+(b[i]-a[i])*float(t) for i in range(3)];cell=g['cells'][z].get((math.floor(x),math.floor(y)))
        covered &= cell is not None and not cell[1]
check('SV24_upper_route_has_indoor_floor_not_bare_ground',covered)

# Corridor sample ownership is independent of future chapter access. Report
# all floors and covered galleries. Outdoor campus/roof are sampled separately.
owned={};outside={}
for l in d['locations']:
    ox,oy,z=l['origin'];indices=set();rects=[[ox+x,oy+y,w,h] for x,y,w,h in l['corridors']]
    if l['id']=='UT':rects += [p[:4] for p in d['passages'] if p[4]<0]
    if l['id'] in ('1F','2F','3F'):rects += [p[:4] for p in d['passages'] if p[4]==z]
    for x,y,w,h in rects:
        step=2 if l['id'] in ('ST','RF') else .5
        for yy in np.arange(y+.25,y+h,step):
            for xx in np.arange(x+.25,x+w,step):
                n=node(xx,yy,z);cell=g['cells'][z].get((math.floor(xx),math.floor(yy)))
                if base_mask.reshape(-1)[n] and g['parent'][n]>=0 and cell and (cell[0]<0 or not g['rooms'][cell[0]]['enclosed']):indices.add(n)
    (outside if l['id'] in ('ST','RF') else owned)[l['id']]=np.array(sorted(indices),dtype=np.int32)

def state_at(r,ch):
    state='open'
    for s in r['states']:
        if s['chapter']<=ch:state=s['state']
    return state

reports=[];failures=[]
for chapter in (0,1,6,12,17,19):
    # Paint exact shut leaves with capsule clearance into a fresh floor mask.
    g['mask'][:]=base_mask
    sources=[n['entry'] for n in d['escape_niches']]+[e['at'] for e in d['site_entries']]
    closed=0
    for l,r in zones.values():
        if not r['enclosed']:continue
        ox,oy,z=l['origin'];state=state_at(r,chapter);opened=state in ('open','destroyed')
        for e in [dict(at=r['door'],side=r['door_side'],width=2)]+r.get('extra_doors',[]):
            x,y=ox+e['at'][0],oy+e['at'][1];width=e.get('width',2)-.12
            if opened:sources.append([x,y,z])
            else:
                closed+=1
                rr=[x-width/2,y-.045,width,.09] if e['side'] in ('north','south') else [x-.045,y-width/2,.09,width]
                g['paint'](z,*rr,False,.35)
    mask=g['mask'];flat=mask.reshape(-1);dist=np.full(len(flat),-1,dtype=np.int16);q=deque()
    for x,y,z in sources:
        for dx in (-1,-.5,0,.5,1):
            for dy in (-1,-.5,0,.5,1):
                n=node(x+dx,y+dy,z)
                if flat[n] and dist[n]<0 and math.dist(point(n)[:2],[x,y])<=.8:dist[n]=0;q.append(n)
    while q:
        n=q.popleft()
        if dist[n]>=60:continue
        v=n%PLANE;ix=v%W;iy=v//W;ns=[]
        if ix:ns.append(n-1)
        if ix<W-1:ns.append(n+1)
        if iy:ns.append(n-W)
        if iy<H-1:ns.append(n+W)
        for k in ns:
            if flat[k] and dist[k]<0:dist[k]=dist[n]+1;q.append(k)
    per=[]
    for site,arr in owned.items():
        valid=arr[flat[arr]];values=dist[valid];bad=valid[(values<0)|(values>28)]
        maxval=float(values.max()*.5+1) if len(values) else 0
        if np.any(values<0):maxval=None
        per.append(dict(site=site,samples=len(valid),over_15m=len(bad),maximum_upper_m=maxval))
        failures += [dict(chapter=chapter,site=site,at=point(int(n)),distance_upper_m=float(dist[n]*.5+1) if dist[n]>=0 else None) for n in bad]
    ext=[]
    for site,arr in outside.items():
        valid=arr[flat[arr]];values=dist[valid]
        ext.append(dict(site=site,grid_m=2,samples=len(valid),over_15m=int(((values<0)|(values>28)).sum()),
            beyond_30m=int((values<0).sum()),rule='outdoor exposure report; no claim of continuous indoor refuge coverage'))
    reports.append(dict(chapter=chapter,closed_leaves=closed,corridors=per,exterior=ext))
    print(json.dumps(dict(chapter=chapter,failed_corridor_samples=sum(p['over_15m'] for p in per),closed_leaves=closed)),flush=True)

check('all_six_chapter_corridor_states_within_15m',not failures)
sha=hashlib.sha256(source.read_bytes()).hexdigest()
result=dict(success=all(checks.values()),checks=checks,source_sha256=sha,counts=dict(zones=len(zones),locations=len(sites),furnishings=len(d['furnishings']),
    hideouts=dict(Counter(h['kind'] for h in d['hideouts'])),secrets=dict(distribution)),chapter_reports=reports,failures=failures,
    limits=['0.5m standing-capsule collision raster, Manhattan path +1m threshold allowance; not a continuous-space proof.',
            'Closed chapter doors are physical obstacles and are never escape targets. Movable props remain at authored positions.',
            'Campus and roof sampled separately on 2m grid; long open exposure distances are reported, not passed off as indoor 15m coverage.',
            'This static audit verifies map geometry; prototype monster behaviour is verified separately by native story tests.'])
(OUT/'acceptance.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(success=result['success'],checks=checks,failed_samples=len(failures)),ensure_ascii=False,indent=2))
if not result['success']:raise SystemExit(1)
