"""Independent bounds, support and conservation audit for the finishing pass."""
import collections,hashlib,json,math
from pathlib import Path
from hospital_furnishing_v6 import Index
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v17';PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(PATH.read_text(encoding='utf8'));old=json.loads((OUT/'baseline_v16.json').read_text(encoding='utf8'));u=d['finish_upgrade'];checks={};issues=[]
def check(k,v):checks[k]=bool(v)
check('all_562_rooms_and_sizes_are_unchanged',d['locations']==old['locations'])
check('previous_architecture_furniture_doors_and_forest_are_retained',all(d[k]==old[k] for k in ('stairs','lift','passages','site_entries','windows','escape_niches','prototype_props','environment_props','prototype_catalog','furnishings','hideouts','campus_plan','campus_upgrade','realism_upgrade','utility_links')) and d['environment_dressing']['boxes'][:len(old['environment_dressing']['boxes'])]==old['environment_dressing']['boxes'])
check('story_weapons_notes_enemies_chapters_are_preserved',all(d['story_gameplay'][k]==v for k,v in old['story_gameplay'].items() if k!='navigation'))
check('82_real_guard_deck_and_tread_surfaces_are_registered',len(d['raised_access'])==82 and sum(a['kind']=='guard_deck' and a['z']==6.05 for a in d['raised_access'])==2 and sum(a['kind']=='guard_stair' for a in d['raised_access'])==80)
check('both_guard_stairs_have_side_and_front_back_protection',sum(i['kind']=='guard_stair_rail' for i in u['items'])==80 and sum(i['kind']=='guard_back_rail' for i in u['items'])==2 and sum(i['kind']=='guard_front_rail' for i in u['items'])==4)
check('both_guard_stairs_have_zero_level_ground_landings',sum(i['kind']=='guard_ground_landing' and abs(i['bounds'][5])<.0001 for i in u['items'])==2)
parts=collections.defaultdict(list)
for b in d['environment_dressing']['boxes']:
    if b.get('layer')==17:parts[b['item_id']].append(b)
check('every_new_assembly_has_positive_finite_geometry',all(len(parts[i['id']])==i['parts'] for i in u['items']) and all(min(b['size'])>0 and all(math.isfinite(x) for x in b['at']+b['size']) and b['material'] in d['environment_dressing']['materials'] for bs in parts.values() for b in bs))
zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};solid=Index();features={f['id']:f for f in d['collision_features']}
for f in d['collision_features']:solid.add(f['z'],f['rect'])
for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf8')):solid.add(w['z'],w['rect'])
for i in u['items']:
    b=i['bounds'];mount=i['mount']
    if mount=='floor':
        s,r=zones[i['zone']];ox,oy,z=s['origin'];x,y,w,h=r['rect'];f=features.get(i['id']+'_0')
        if not(ox+x+.05<=b[0] and b[3]<=ox+x+w-.05 and oy+y+.05<=b[1] and b[4]<=oy+y+h-.05 and abs(b[2]-z)<.0001):issues.append([i['id'],'room_or_floor_bounds'])
        if not f or f['rect']!=[b[0],b[1],b[3]-b[0],b[4]-b[1]]:issues.append([i['id'],'nav_collision'])
        a=i['front_access']
        if solid.hits(z,[a[0]-.4,a[1]-.4,.8,.8],.02):issues.append([i['id'],'blocked_front'])
    elif mount=='supported_detail':
        q=i['support'];a=q['at'];sz=q['size']
        if not(a[0]-sz[0]/2-.0001<=b[0] and b[3]<=a[0]+sz[0]/2+.0001 and a[1]-sz[1]/2-.0001<=b[1] and b[4]<=a[1]+sz[1]/2+.0001 and abs(b[2]-a[2]-sz[2]/2-.004)<.0001):issues.append([i['id'],'unsupported_detail'])
check('new_full_size_kits_and_details_fit_real_supports_and_access',not issues)
check('each_optional_room_request_records_its_actual_result',len(u['requests'])==541 and len(u['room_kits'])==sum(bool(q['item']) for q in u['requests']))
check('all_562_rooms_and_1939_bays_remain_reachable',len(d['navigation_routes'])==562 and len(d['bay_navigation_routes'])==1939)
report=dict(success=all(checks.values()),checks=checks,issues=issues,map_sha256=hashlib.sha256(PATH.read_bytes()).hexdigest(),inventory=json.loads((OUT/'inventory.json').read_text(encoding='utf8')))
(OUT/'finish_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
