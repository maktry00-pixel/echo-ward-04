"""Retain metric layout; refresh exact-aperture routes and author facade views."""
import contextlib,io,json,runpy
from pathlib import Path
import hospital_gameplay_v7 as gameplay
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v8';OUT.mkdir(exist_ok=True)
SOURCE=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(SOURCE.read_text(encoding='utf-8'))
# Exact exterior walls exposed a long retreat detour at the quarantine arcade.
# Add a real 2x2 m recess; retain all existing rooms and their metric areas.
if not any(n['id']=='V8_QUARANTINE_EAST' for n in d['escape_niches']):
    d['escape_niches'].append(dict(id='V8_QUARANTINE_EAST',site='1F',rect=[119,-19,2,2],z=0,entry=[119,-18,0],side='west',opening_m=.9,large_pass=False,corridor_point=[117.5,-18,0]))
    d['passages'].append([119,-19,2,2,0])
    for i,y in enumerate((-19,-17.55)):
        d['collision_features'].append(dict(id=f'V8_NICHE_JAMB_{i}',type='niche_frame',z=0,rect=[118.89,y,.22,.55]))
SOURCE.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with contextlib.redirect_stdout(io.StringIO()):g=runpy.run_path(str(ROOT/'tools/verify_story_layout.py'))
d=g['D']
# Window rhythm follows exposed room walls, rather than a blank wall spanning
# multiple clinical bays. Glazing retains the wall's blocking collision.
d['windows']=[w for w in d['windows'] if w.get('role')!='facade_v8']
for l in d['locations']:
    if l['id'] in ('B1','B2','UT','RF'):continue
    ox,oy,z=l['origin']
    for r in l['rooms']:
        if not r['enclosed']:continue
        x,y,w,h=r['rect']
        for side,yy,outside_y in (('north',y,oy+y-.1),('south',y+h,oy+y+h+.1)):
            for cx in range(2,int(w)-1,4):
                xx=x+cx;outside=g['cells'][z].get((int(ox+xx//1),int(outside_y//1)))
                if outside is not None and not outside[1]:continue
                doors=[dict(at=r['door'],side=r['door_side'])]+r.get('extra_doors',[])
                if any(e['side']==side and abs(e['at'][1]-yy)<.01 and abs(e['at'][0]-xx)<2.2 for e in doors):continue
                if any(a['zone']==r['id'] and a['side']==side and abs(a['at'][1]-yy)<.01 and abs(a['at'][0]+a.get('offset',0)-xx)<2.2 for a in d['windows']):continue
                d['windows'].append(dict(zone=r['id'],site=l['id'],side=side,at=[xx,yy],width=2,height=1,sill=1.1,offset=0,transparent=True,role='facade_v8'))
gameplay.OUT=OUT;gameplay.build_gameplay(d,g)
d['review_revision']=8
d['shell_finish']=dict(version=8,exact_apertures=True,exterior_storey_band_m=1.0,shaft_roof=True,
    floor_wetness=.22,weathering='subtle_world_space_grime',preserve_metric_areas=True)
d['furnishing_captures']=[c for c in d['furnishing_captures'] if not c.get('envelope')]
views=[]
def view(ident,eye,target):views.append(dict(id='shell_'+ident,at=eye,target=target,envelope=True))
view('main_front',[48,-65,10],[48,-1,7])
view('main_west',[-30,48,10],[0,48,7])
for ident in ('DG','IF','PS','RH','HS','SV'):
    l=next(l for l in d['locations'] if l['id']==ident);x,y,z=l['origin']
    view(ident,[x+52,y-24,10],[x+52,y+8,6])
street=next(l for l in d['locations'] if l['id']=='ST')
for r in street['rooms']:
    if not r['enclosed']:continue
    x,y,w,h=r['rect'];ox,oy,z=street['origin'];x+=ox;y+=oy
    view(r['id'],[x+w/2,y-30,12],[x+w/2,y-.3,5.5])
view('roof',[45,42,18],[50,65,14.5])
view('stairs_X01',[-58,-111,4.0],[-59.5,-118,2.5])
view('stairs_S11',[136,-12,3.5],[134.5,-20,1.8])
d['furnishing_captures']+=views
SOURCE.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
old=json.loads((ROOT/'artifacts/unreal_hospital_v7/hospital_world_source.json').read_text(encoding='utf-8'))
metric=lambda w:{r['id']:(l['origin'],r['rect']) for l in w['locations'] for r in l['rooms']}
assert metric(d)==metric(old)
assert d['stairs']==old['stairs'] and d['prototype_props']==old['prototype_props']
report=dict(success=True,unchanged_zones=len(metric(d)),stairs_unchanged=len(d['stairs']),furnishings_unchanged=len(d['prototype_props']),
    exterior_views=len(views),facade_windows_added=sum(w.get('role')=='facade_v8' for w in d['windows']),total_views=len(d['locations'])+len(d['furnishing_captures']),
    scope='Envelope geometry is validated against actual Unreal collision; metre plans and furnished footprints are retained.')
(OUT/'metric_preservation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
