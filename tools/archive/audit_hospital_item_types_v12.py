"""Independent conservation, support, spacing and room-function checks."""
import collections,hashlib,json,math,re
from pathlib import Path
from hospital_clutter_v10 import Space,bounds,rotate
from hospital_furnishing_v6 import Index
from hospital_item_types_v12 import AFFINITY

ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v12'
source=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(source.read_text(encoding='utf-8'))
old=json.loads((OUT/'baseline_v11.json').read_text(encoding='utf-8'))
data=d['item_types_dressing'];items=data['items'];e=d['environment_dressing']
checks={};issues=[]
def check(k,v):checks[k]=bool(v)
skip={'review_revision','environment_dressing','collision_features','furnishing_captures','story_gameplay','navigation_routes','bay_navigation_routes'}
check('previous_geometry_furniture_dimensions_and_all_old_layers_retained',all(d[k]==v for k,v in old.items() if k not in skip))
check('old_environment_and_materials_retained',[b for b in e['boxes'] if b.get('layer')!=12]==old['environment_dressing']['boxes'] and all(e[k]==v for k,v in old['environment_dressing'].items() if k not in ('boxes','materials')) and all(e['materials'][k]==v for k,v in old['environment_dressing']['materials'].items()))
check('old_collision_retained',[f for f in d['collision_features'] if f.get('source')!='item_catalog_v12']==old['collision_features'])
check('story_placements_and_rules_retained',all(d['story_gameplay'][k]==v for k,v in old['story_gameplay'].items() if k!='navigation'))
check('previous_views_retained',d['furnishing_captures'][:len(old['furnishing_captures'])]==old['furnishing_captures'])
check('all_room_and_bay_routes_retained',len(d['navigation_routes'])==len(old['navigation_routes']) and len(d['bay_navigation_routes'])==len(old['bay_navigation_routes']))
types={t['id']:t for t in data['catalog']};parts=collections.defaultdict(list)
check('1711_types_50_categories_and_honest_statuses',len(types)==1711 and len({t['category'] for t in types.values()})==50 and all((t['status']=='new_shared_geometry_prototype')==bool(t['instances']) for t in types.values()))
for b in e['boxes']:
    if b.get('layer')==12:parts[b['item_id']].append(b)
check('complete_unique_instance_assemblies',len(items)==len({i['id'] for i in items})==len(parts) and all(len(parts[i['id']])==i['parts'] for i in items))
check('all_new_categories_have_explicit_room_rules',all(i['category'] in AFFINITY for i in items))
zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};oldspace=Space();newspace=Space();walls=Index()
for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):walls.add(w['z'],w['rect'])
for p in old['prototype_props']+old['environment_props']:
    for a in old['prototype_catalog'][str(p['model'])]['parts']:
        off=rotate(a[:3],p['turn']);at=[p['at'][j]+off[j] for j in range(3)];sz=list(a[3:6])
        if p['turn']%2:sz[0],sz[1]=sz[1],sz[0]
        oldspace.add(bounds(at,sz),p['id'])
for j,b in enumerate(old['environment_dressing']['boxes']):
    if b['kind'] not in ('lawn','grass_tuft','pavement','road_dash','tree_crown'):oldspace.add(bounds(b['at'],b['size']),'old_'+str(j))
features={f['id']:f for f in d['collision_features'] if f.get('source')=='item_catalog_v12'}
furniture={p['id']:p for p in old['prototype_props']+old['environment_props']}
newitems={i['id']:i for i in items}
for i in items:
    group=parts[i['id']];b=i['bounds'];s,r=zones[i['zone']]
    if not re.search(AFFINITY[i['category']],r['name'].lower()):issues.append([i['id'],'room_function'])
    specialty=types[i['type']].get('specialty_pattern')
    if specialty and not re.search(specialty,r['name'].lower()):issues.append([i['id'],'specialty'])
    actual=[min(p['at'][a]-p['size'][a]/2 for p in group) for a in range(3)]+[max(p['at'][a]+p['size'][a]/2 for p in group) for a in range(3)]
    if max(abs(x-y) for x,y in zip(b,actual))>2e-6:issues.append([i['id'],'bounds'])
    if i['mount']=='surface':
        q=i['support'];x,y,w,h=q['rect']
        if i['parent'] in furniture:
            p=furniture[i['parent']];c=old['prototype_catalog'][str(p['model'])]
            heights=[.5725,1.0025,1.4325] if p['model'] in (13,14,15) else [c['top']]
            if min(abs(q['z']-p['at'][2]-zz) for zz in heights)>.00001:issues.append([i['id'],'false_support_height'])
        else:
            parent=newitems[i['parent']]
            if parent['family']!='stand' or abs(parent['bounds'][5]-q['z'])>1e-6:issues.append([i['id'],'false_new_support'])
        if not(b[0]>=x-1e-6 and b[1]>=y-1e-6 and b[3]<=x+w+1e-6 and b[4]<=y+h+1e-6 and abs(b[2]-q['z']-.004)<1e-6):issues.append([i['id'],'support'])
        if any(p['solid'] for p in group):issues.append([i['id'],'surface_collision'])
    else:
        f=features.get(i['id'])
        if not f or not all(p['solid'] for p in group) or abs(b[2]-s['origin'][2])>.00001:issues.append([i['id'],'floor_collision'])
        elif max(abs(a-c) for a,c in zip(f['rect'],[b[0],b[1],b[3]-b[0],b[4]-b[1]]))>1e-5:issues.append([i['id'],'collider_bounds'])
    for p in group:
        bb=bounds(p['at'],p['size'])
        if oldspace.hits(bb):issues.append([i['id'],'old_mesh_overlap']);break
        if newspace.hits(bb):issues.append([i['id'],'new_mesh_overlap']);break
    for p in group:newspace.add(bounds(p['at'],p['size']),i['id'])
    if i['mount']=='floor' and walls.hits(s['origin'][2],[b[0],b[1],b[3]-b[0],b[4]-b[1]],.04):issues.append([i['id'],'wall_overlap'])
check('supported_attached_and_nonintersecting_new_items',not issues)
check('all_materials_and_coordinates_valid',all(p['material'] in e['materials'] and min(p['size'])>0 and all(math.isfinite(x) for x in p['at']+p['size']) for pp in parts.values() for p in pp))
check('type_to_instance_index_is_exact',all(t['instances']==[i['id'] for i in items if i['type']==t['id']] for t in types.values()))
check('different_departments_have_distinct_contents',len({i['category'] for i in items})>=45 and all(any(i['family']==fam for i in items) for fam in ('microscope','sewing','chess','glasses','bread','hammer')))
report=dict(success=all(checks.values()),checks=checks,issues=issues,map_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),counts=json.loads((OUT/'catalog_inventory.json').read_text(encoding='utf-8')),limits=data['limits'])
(OUT/'catalog_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
