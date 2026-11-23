"""Independent room programme, conservation and equipment fitting audit."""
import collections,hashlib,json,math
from pathlib import Path
from hospital_furnishing_v6 import Index,overlap
from hospital_clutter_v10 import Space,bounds,rotate

ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v13'
source=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(source.read_text(encoding='utf-8'));old=json.loads((OUT/'baseline_v12.json').read_text(encoding='utf-8'))
data=d['hospital_services'];checks={};issues=[]
def check(k,v):checks[k]=bool(v)
zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};oldzones={r['id']:(s,r) for s in old['locations'] for r in s['rooms']}
changes={r['zone']:r for r in data['room_changes']};protected={r['zone'] for r in old['story_gameplay']['placements']+old['story_gameplay']['mobs']}
check('all_562_zones_35_locations_and_metric_geometry_retained',set(zones)==set(oldzones) and len(zones)==562 and len(d['locations'])==35 and all(zones[k][0]['origin']==oldzones[k][0]['origin'] and all(zones[k][1].get(f)==v for f,v in oldzones[k][1].items() if f!='name') for k in zones))
check('only_authorized_support_room_names_changed',not(set(changes)&protected) and all((r['name']==oldzones[z][1]['name'] if z not in changes else r['name']==changes[z]['after'] and changes[z]['before']==oldzones[z][1]['name']) for z,(_,r) in zones.items()))
skip={'review_revision','locations','environment_dressing','collision_features','furnishing_captures','story_gameplay','navigation_routes','bay_navigation_routes'}
check('all_old_furniture_items_catalogue_and_architectural_layers_retained',all(d[k]==v for k,v in old.items() if k not in skip))
check('all_old_story_content_and_mobs_retained',all(d['story_gameplay'][k]==v for k,v in old['story_gameplay'].items() if k!='navigation'))
e=d['environment_dressing'];oe=old['environment_dressing']
check('old_surfaces_lighting_parts_and_materials_retained',[p for p in e['boxes'] if p.get('layer')!=13]==oe['boxes'] and all(e[k]==v for k,v in oe.items() if k not in ('boxes','materials')) and all(e['materials'][k]==v for k,v in oe['materials'].items()))
check('old_collisions_and_views_retained',[f for f in d['collision_features'] if f.get('source')!='hospital_services_v13']==old['collision_features'] and d['furnishing_captures'][:len(old['furnishing_captures'])]==old['furnishing_captures'])
requirements=data['requirements'];routes={r['zone'] for r in d['navigation_routes']};sites={s['id']:s for s in d['locations']}
check('all_123_reference_requirements_have_real_reachable_addresses',len(requirements)==123 and {r['section'] for r in requirements}==set(range(1,13)) and all(set(r['zones'])<=routes|set(sites) and (r['existing_models'] or r['new_equipment'] or r['sites'] or r['section']==12) for r in requirements))
check('all_requested_apparatus_fitted_without_scaling_or_omission',not data['unplaced_equipment'] and collections.Counter((i['kind'],i['zone']) for i in data['items'] if i['mount']=='floor')==collections.Counter((r['kind'],r['zone']) for r in data['equipment_requests']))
check('all_target_rooms_have_cyrillic_sign_panels',not data['unsigned_rooms'] and all(any(i['zone']==z and i['kind']=='sign' and i['text']==zones[z][1]['name'] for i in data['items']) for z in changes))
parts=collections.defaultdict(list);space=Space();newspace=Space();wall=Index();solid=Index()
for p in e['boxes']:
    if p.get('layer')==13:parts[p['item_id']].append(p)
for p in old['prototype_props']+old['environment_props']:
    for a in old['prototype_catalog'][str(p['model'])]['parts']:
        off=rotate(a[:3],p['turn']);at=[p['at'][j]+off[j] for j in range(3)];sz=list(a[3:6])
        if p['turn']%2:sz[0],sz[1]=sz[1],sz[0]
        space.add(bounds(at,sz),p['id'])
for n,p in enumerate(oe['boxes']):
    if p['kind'] not in ('lawn','grass_tuft','pavement','road_dash','tree_crown'):space.add(bounds(p['at'],p['size']),'old_'+str(n))
for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):wall.add(w['z'],w['rect']);solid.add(w['z'],w['rect'])
for f in old['collision_features']:solid.add(f['z'],f['rect'])
features={f['id']:f for f in d['collision_features'] if f.get('source')=='hospital_services_v13'}
items=data['items'];check('unique_complete_item_assemblies',len(items)==len({i['id'] for i in items})==len(parts) and all(len(parts[i['id']])==i['parts'] for i in items))
for i in items:
    group=parts[i['id']];b=i['bounds'];s,r=zones[i['zone']];z=s['origin'][2]
    actual=[min(p['at'][a]-p['size'][a]/2 for p in group) for a in range(3)]+[max(p['at'][a]+p['size'][a]/2 for p in group) for a in range(3)]
    if max(abs(a-c) for a,c in zip(actual,b))>2e-6:issues.append([i['id'],'bounds'])
    if i['mount']=='floor':
        rr=[b[0],b[1],b[3]-b[0],b[4]-b[1]];f=features.get(i['id']);ox,oy,_=s['origin'];x,y,w,h=r['rect']
        if not f or max(abs(a-c) for a,c in zip(f['rect'],rr))>2e-6 or not all(p['solid'] for p in group):issues.append([i['id'],'collision'])
        if abs(b[2]-z)>1e-5 or not(ox+x+.1<=b[0]<b[3]<=ox+x+w-.1 and oy+y+.1<=b[1]<b[4]<=oy+y+h-.1):issues.append([i['id'],'outside_room_floor'])
        if solid.hits(z,rr,.05):issues.append([i['id'],'wall_or_furniture_overlap'])
        solid.add(z,rr)
        if not(.20<=max(rr[2:])<=2.6 and .50<=b[5]-b[2]<=2.4):issues.append([i['id'],'not_full_size'])
    else:
        if any(p['solid'] for p in group) or i['text']!=r['name'] or len(i['wall_samples'])!=3 or not all(wall.hits(q[2],[q[0]-.008,q[1]-.008,.016,.016]) for q in i['wall_samples']):issues.append([i['id'],'sign_backing_or_text'])
    for p in group:
        bb=bounds(p['at'],p['size'])
        if space.hits(bb):issues.append([i['id'],'old_mesh_overlap']);break
        if newspace.hits(bb):issues.append([i['id'],'new_mesh_overlap']);break
    for p in group:newspace.add(bounds(p['at'],p['size']),i['id'])
check('full_size_supported_apparatus_sign_backing_and_mesh_spacing_valid',not issues)
check('all_parts_have_finite_sizes_and_existing_materials',all(min(p['size'])>0 and all(math.isfinite(v) for v in p['at']+p['size']) and p['material'] in e['materials'] for ps in parts.values() for p in ps))
check('food_storage_is_separate_from_morgue_plant_room',all(i['zone']=='B106' for i in items if i['kind']=='food_freezer') and zones['B105'][1]['name']==oldzones['B105'][1]['name'])
check('new_medicine_and_support_services_are_distinct',all(any(i['kind']==k and i['mount']=='floor' for i in items) for k in ('dialysis','ventilator','pediatric_cot','gyn_chair','blood_fridge','labhood','washer','kitchen_range','boiler','transformer','nurse_station')))
check('every_apparatus_faces_a_free_player_sized_approach',all('front_access' in i and not solid.hits(i['front_access'][2],[i['front_access'][0]-.35,i['front_access'][1]-.35,.70,.70],.01) for i in items if i['mount']=='floor'))
byid={i['id']:i for i in items};frontviews=[]
for c in d['furnishing_captures']:
    if not c.get('hospital_services'):continue
    i=byid[c['detail_item']]
    if i['mount']!='floor':continue
    nx,ny,_=rotate([0,1,0],i['turn']);delta=[c['at'][a]-c['target'][a] for a in (0,1)]
    frontviews.append(delta[0]*nx+delta[1]*ny>.5)
check('new_equipment_inspection_views_show_front_not_blank_back',len(frontviews)>=10 and all(frontviews))
report=dict(success=all(checks.values()),checks=checks,issues=issues,map_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),counts=json.loads((OUT/'services_inventory.json').read_text(encoding='utf-8')),limits=data['limits'])
(OUT/'services_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
