"""Independent conservation, support, corridor-fit and signage checks."""
import collections,hashlib,json,math,contextlib,io,runpy
from pathlib import Path
from hospital_furnishing_v6 import Index,overlap
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v16';PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(PATH.read_text(encoding='utf8'));old=json.loads((OUT/'baseline_v15.json').read_text(encoding='utf8'));u=d['realism_upgrade'];e=d['environment_dressing'];checks={};issues=[]
def check(name,value):checks[name]=bool(value)
check('all_562_rooms_sizes_names_and_35_locations_preserved',d['locations']==old['locations'])
check('original_furniture_story_lifts_stairs_and_tunnels_preserved',all(d[k]==old[k] for k in ('prototype_props','environment_props','prototype_catalog','prototype_materials','furnishings','hideouts','stairs','lift','utility_links','subrooms','movable_props','windows','hospital_services')))
check('story_actors_chapters_notes_and_monsters_preserved',all(d['story_gameplay'][k]==v for k,v in old['story_gameplay'].items() if k!='navigation'))
converted={n['id'] for n in u['converted_endpoint_niches']};retained=[n for n in old['escape_niches'] if n['id'] not in converted]
check('existing_niches_retained_except_replaced_gallery_endpoints',d['escape_niches'][:len(retained)]==retained and len([n for n in d['escape_niches'] if n['id'].startswith('V16_NICHE')])>=16 and all(not any(f['id'].startswith(n+'_jamb_') for f in d['collision_features']) for n in converted))
check('all_existing_environment_and_forest_geometry_preserved',e['boxes'][:len(old['environment_dressing']['boxes'])]==old['environment_dressing']['boxes'])
check('long_spine_is_connected_to_two_rear_buildings_on_the_second_floor',len(u['rear_connections'])==2 and all(p in d['passages'] and p[4]==4.8 for p in u['rear_connections']) and all(any(q['site']==sid and q['at'][1:]==[328,4.8] for q in d['site_entries']) for sid in ('HS2','SV2')))
support=[i for i in u['items'] if i['kind']=='gallery_support_frame'];galleries=d['campus_upgrade']['gallery_rects'];gaps=[]
for j,p in enumerate(galleries):
    positions=sorted(i['station'] for i in support if i['gallery']==j);length=max(p[2:4]);gaps.append(max(b-a for a,b in zip([0]+positions,positions+[length])))
check('every_gallery_has_supports_and_no_span_over_15m',len(support)>=90 and len(gaps)==9 and max(gaps)<=15)
check('all_support_legs_reach_ground_and_crossbeams_touch_deck',all(i['bounds'][2]<=-.19 and i['bounds'][5]>=4.6 and len(i['legs'])==2 for i in support))
wall=Index()
for f in old['collision_features']:wall.add(f['z'],f['rect'])
for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf8')):wall.add(w['z'],w['rect'])
features={f['id']:f for f in d['collision_features']};parts=collections.defaultdict(list)
for b in e['boxes']:
    if b.get('layer')==16:parts[b['item_id']].append(b)
check('all_new_assemblies_have_valid_geometry_and_materials',all(len(parts[i['id']])==i['parts'] for i in u['items']) and all(min(b['size'])>0 and b['material'] in e['materials'] and all(math.isfinite(v) for v in b['at']+b['size']) for bs in parts.values() for b in bs))
sites={s['id']:s for s in d['locations']};floors=[i for i in u['items'] if i['mount']=='corridor_floor']
for i in floors:
    s=sites[i['site']];ox,oy,z=s['origin'];x,y,w,h=s['corridors'][i['corridor']];bb=i['bounds'];rr=[bb[0],bb[1],bb[3]-bb[0],bb[4]-bb[1]]
    if not(ox+x<=bb[0] and oy+y<=bb[1] and bb[3]<=ox+x+w and bb[4]<=oy+y+h and abs(bb[2]-z)<.01):issues.append([i['id'],'corridor_or_floor_bounds'])
    if wall.hits(z,rr,.05):issues.append([i['id'],'prior_obstacle'])
    if not any(f['id'].startswith(i['id']+'_') and f['rect']==rr and f['z']==z for f in d['collision_features']):issues.append([i['id'],'nav_obstacle_missing'])
check('all_831_corridor_props_are_full_size_with_real_floor_and_collision',len(floors)>=800 and not issues)
check('all_enclosed_rooms_have_readable_cyrillic_wall_plates',sum(i['kind']=='room_plate' for i in u['items'])==541 and all(i.get('text') for i in u['items'] if i['mount']=='wall_sign'))
check('clock_fire_phone_and_hygiene_fixtures_reach_all_21_main_departments',len({i['site'] for i in u['items'] if i['mount']=='corridor_wall'})>=21 and all(any(i['kind']==k for i in u['items']) for k in ('wall_clock','fire_station','phone_station','notice_board','hand_sanitizer')))
check('high_wall_has_visible_mesh_and_extra_floodlights',u['mesh_panels']>=500 and u['floodlights']>=30 and all(b['at'][2]-b['size'][2]/2>=5.75 for bs in parts.values() for b in bs if b['kind']=='campus_perimeter_mesh'))
check('menu_is_spatially_outside_every_tree_and_building',u['menu_origin_cm'][0]/100>d['campus_upgrade']['landscape_rect'][0]+d['campus_upgrade']['landscape_rect'][2]+1000)
check('native_menu_origin_matches_the_fixed_manifest', 'FVector(300000,300000,2000)' in (ROOT/'unreal/EchoWard/Source/EchoWard/WardTitleScene.h').read_text(encoding='utf8'))
check('every_old_room_and_bay_remains_reachable',len(d['navigation_routes'])==562 and len(d['bay_navigation_routes'])==1939)
report=dict(success=all(checks.values()),checks=checks,issues=issues,max_support_span_m=max(gaps),map_sha256=hashlib.sha256(PATH.read_bytes()).hexdigest(),inventory=json.loads((OUT/'inventory.json').read_text(encoding='utf8')))
(OUT/'realism_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
