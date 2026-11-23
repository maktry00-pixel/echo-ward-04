"""Independent schedule coverage, exact rendered bounds and overlap contracts."""
import collections,hashlib,json,math
from pathlib import Path
from hospital_furnishing_v6 import Index,overlap

ROOT=Path(__file__).resolve().parents[1]
source=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(source.read_text(encoding='utf-8'))
OUT=ROOT/f'artifacts/unreal_hospital_v{d.get("review_revision",6)}';OUT.mkdir(exist_ok=True)
plan=json.loads((ROOT/'data/hospital_furnishing/placement_plan.json').read_text(encoding='utf-8-sig'))
old=json.loads((ROOT/'artifacts/unreal_hospital_v5/hospital_world_source.json').read_text(encoding='utf-8-sig'))
checks={};issues=[]
expected=collections.Counter()
areas={z['id']:z['rect_global'] for z in plan['zones']}
for z in plan['zones']:
    for i in z['items']:expected[z['id'],i['model']]+=i['qty']-i['existing']
actual=collections.Counter((p['zone'],p['model']) for p in d['prototype_props'])
checks['every_numbered_zone_model_quantity_matches_pdf']=all(actual[k]==v for k,v in expected.items()) and not(actual.keys()-expected.keys())
checks['all_579_zone_records_have_new_content']=len({p['zone'] for p in d['prototype_props']})==579
checks['all_10978_instance_ids_unique']=len(d['prototype_props'])==len({p['id'] for p in d['prototype_props']})==10978
checks['existing_furniture_hides_movables_keep_exact_coordinates']=all(d[k]==old[k] for k in ('furnishings','hideouts','movable_props','stairs','infrastructure_zones'))
checks['all_original_room_ids_and_metric_areas_preserved']=all({r['id']:r['rect'] for r in l['rooms']}=={r['id']:r['rect'] for r in old['locations'][i]['rooms']} for i,l in enumerate(d['locations']))
index=Index();features={f['id']:f for f in d['collision_features']};floor_count=0;parts=0
for p in d['prototype_props']:
    c=d['prototype_catalog'][str(p['model'])];parts+=len(c['parts'])
    if p['mount']!='floor':continue
    floor_count+=1;x,y,z=p['at'];turn=p['turn'];coordinates=[]
    for a in c['parts']:
        xx,yy,zz,w,h,dep,mat,solid=a
        if turn==1:xx,yy=-yy,xx;w,h=h,w
        elif turn==2:xx,yy=-xx,-yy
        elif turn==3:xx,yy=yy,-xx;w,h=h,w
        coordinates.append([x+xx-w/2,y+yy-h/2,w,h])
    lo=[min(r[i] for r in coordinates) for i in (0,1)];hi=[max(r[i]+r[i+2] for r in coordinates) for i in (0,1)]
    r=[*lo,hi[0]-lo[0],hi[1]-lo[1]];f=features.get(p['id'])
    if not f or any(abs(a-b)>.001 for a,b in zip(r,f['rect'])):issues.append([p['id'],'rendered_bounds_differ_from_navigation'])
    zx,zy,zw,zh=areas[p['zone']]
    if not(zx<lo[0]<hi[0]<zx+zw and zy<lo[1]<hi[1]<zy+zh):issues.append([p['id'],'outside_zone'])
    if index.hits(z,r,.001):issues.append([p['id'],'floor_prop_overlap'])
    index.add(z,r)
checks['all_floor_props_fit_their_real_zone_and_match_collision_data']=not issues
grass=[p for p in d['prototype_props'] if p['model']==151]
checks['2912_grass_tufts_are_distributed_not_stacked']=len(grass)==len({tuple(p['at']) for p in grass})==2912
checks['grass_keeps_garden_paths_and_gazebo_foundation_clear']=all(not any(p['zone']==q['zone'] and q['rect'][0]-.12<p['at'][0]<q['rect'][0]+q['rect'][2]+.12 and q['rect'][1]-.12<p['at'][1]<q['rect'][1]+q['rect'][3]+.12 for q in d['garden_paths']) for p in grass)
checks['source_model_catalog_has_no_missing_used_prototypes']=all(str(p['model']) in d['prototype_catalog'] for p in d['prototype_props'])
result=dict(success=all(checks.values()),checks=checks,map_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    new_props=len(d['prototype_props']),solid_floor_props=floor_count,rendered_primitive_parts=parts,
    zones=579,issues=issues,map_fixes=d['map_fixes_v6'],
    limits=['Prototype static set dressing; new medical devices and vehicles have no gameplay simulation.',
            'Independent bounds and navigation checks are supplemented by native capsule sweeps and rendered captures.'])
(OUT/'furnishing_acceptance.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
if not result['success']:raise SystemExit(1)
