"""Conservation and metric/route checks for the elevated campus and forest."""
import collections,hashlib,json,math,runpy,contextlib,io
from pathlib import Path
from hospital_furnishing_v6 import Index
from hospital_clutter_v10 import bounds,rotate,Space
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v15';PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
with contextlib.redirect_stdout(io.StringIO()):g=runpy.run_path(str(ROOT/'tools/verify_story_layout.py'))
d=g['D'];old=json.loads((OUT/'baseline_v14.json').read_text(encoding='utf-8'));u=d['campus_upgrade'];e=d['environment_dressing'];checks={};issues=[]
def check(k,v):checks[k]=bool(v)
check('all_562_original_room_ids_names_and_metric_geometry_retained',d['locations']==old['locations'])
check('all_prior_furniture_models_and_10978_scheduled_positions_retained',all(d[k]==old[k] for k in ('prototype_props','environment_props','prototype_catalog','prototype_materials','furnishings','hideouts','stairs','lift','movable_props','subrooms')))
check('all_story_actors_notes_species_and_chapter_content_retained',all(d['story_gameplay'][k]==v for k,v in old['story_gameplay'].items() if k!='navigation'))
check('all_123_hospital_services_and_underground_connections_retained',d['hospital_services']==old['hospital_services'] and d['utility_links']==old['utility_links'])
check('old_room_detail_layers_9_to_13_retained',[b for b in e['boxes'] if b.get('layer') not in (14,15)]==[b for b in old['environment_dressing']['boxes'] if b.get('layer')!=14])
check('seven_connected_galleries_are_on_second_floor_not_ground',len(u['gallery_rects'])==7 and all(b in d['passages'] and b[4]==4.8 and [*b[:4],0] not in d['passages'] for b in u['gallery_rects']))
check('six_bridge_endpoints_refer_to_real_upper_floor_locations',len(d['campus_plan']['connections'])==6 and all(q['from_site']=='2F' and q['to_site'] in {s['id'] for s in d['locations']} and q['passage'] in d['passages'] for q in d['campus_plan']['connections']))
check('all_relocated_escape_niches_have_upper_floor_entries_and_jamb_collision',len(u['moved_niches'])>40 and all(n['entry'][2]==4.8 and n.get('corridor_point',n['entry'])[2]==4.8 and all(f['z']==4.8 for f in d['collision_features'] if f['id'].startswith(n['id']+'_jamb')) for n in d['escape_niches'] if n['id'] in u['moved_niches']))
walk=[]
for x,y,w,h,z in u['gallery_rects']:
    for k in range(4,int(max(w,h)-3),5):
        xx=x+k if w>h else x+w/2;yy=y+h/2 if w>h else y+k;n=g['nearest'](xx,yy,z)
        walk.append(n is not None and g['parent'][n]>=0)
check('all_elevated_bridge_spines_have_reachable_capsule_floor',all(walk) and len(walk)>80)
check('the_vehicle_underpass_is_clear_on_the_ground',all(g['flat'][g['node'](x,111,0)] for x in (46,47,48,49,50,51,52,53,54)))
check('clinical_wall_is_5_8m_and_guard_platforms_have_matching_tall_stairs',d['campus_plan']['clinical_boundary']['height_m']==5.8 and all(q['floor_z']==6.05 and q['steps']==40 and q['riser_m']==.15 for q in d['campus_plan']['sentry_platforms']) and sum(i['kind']=='sentry_stair_v15' for i in u['items'])==80)
trees=u['trees'];trunks=[b for b in e['boxes'] if b['kind']=='forest_trunk'];crowns=[b for b in e['boxes'] if b['kind']=='forest_crown']
check('dense_forest_has_thousands_of_real_separate_tall_tree_instances',len(trees)==u['tree_count']==len(trunks)>3000 and len({t['id'] for t in trees})==len(trees) and len(crowns)>len(trees)*3)
check('inside_planting_is_sparse_outside_woods_are_19_to_30m',8<=sum(t['inside'] for t in trees)<=35 and all(19<=t['height_m']<=30 for t in trees if not t['inside']) and all(11<=t['height_m']<=16 for t in trees if t['inside']))
check('tree_crowns_are_rounded_nonblocking_meshes_and_trunks_are_solid',all(b.get('mesh')=='sphere' and not b['solid'] for b in crowns) and all(b['solid'] for b in trunks))
check('forest_has_its_own_ground_outside_original_hospital_excavations',sum(b['kind']=='forest_ground' and b['solid'] for b in e['boxes'])==4 and u['landscape_rect']==[-320,-330,740,850])
before=old['windows'];after=d['windows'][:len(before)]
check('old_windows_keep_geometry_with_added_protection_metadata',all(all(a.get(k)==v for k,v in b.items()) for a,b in zip(after,before)))
check('psych_buildings_have_secure_glazing_and_public_buildings_more_true_windows',sum(q.get('protection')=='security_mesh' for q in d['windows'])>=100 and u['new_windows']>=300 and all(q['restricted_opening_m']==.08 for q in d['windows'] if q.get('protection')=='security_mesh'))
newwindow=d['windows'][len(before):];zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
check('new_window_apertures_are_on_actual_room_wall_planes',all(abs(q['at'][1]-(zones[q['zone']][1]['rect'][1]+(zones[q['zone']][1]['rect'][3] if q['side']=='south' else 0)))<.001 if q['side'] in ('north','south') else abs(q['at'][0]-(zones[q['zone']][1]['rect'][0]+(zones[q['zone']][1]['rect'][2] if q['side']=='east' else 0)))<.001 for q in newwindow))
check('new_kits_reach_all_541_enclosed_zones_including_82_external_building_kits',sum(i.get('mount')=='floor' for i in u['items'])==1030 and len({i['zone'] for i in u['items'] if i.get('mount')=='floor'})==541 and sum(i.get('mount')=='floor' and (i['zone'].startswith('X') or i['zone'] in ('S03','S12','S13','S14','S11','S16')) for i in u['items'])>=82)
check('every_requested_full_size_kit_fits_without_omission_or_scaling',len(u['requests'])==1030 and all(q['item'] for q in u['requests']))
parts=collections.defaultdict(list)
for b in e['boxes']:
    if b.get('layer')==15 and b.get('item_id'):parts[b['item_id']].append(b)
check('complete_item_assemblies_and_all_valid_positive_geometry',len(parts)==len(u['items']) and all(len(parts[i['id']])==i['parts'] for i in u['items']) and all(min(b['size'])>0 and all(math.isfinite(x) for x in b['at']+b['size']) and b['material'] in e['materials'] for b in e['boxes']))
solid=Index();features={f['id']:f for f in d['collision_features']}
for f in d['collision_features']:solid.add(f['z'],f['rect'])
for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):solid.add(w['z'],w['rect'])
for i in u['items']:
    if i.get('mount')!='floor':continue
    s,r=zones[i['zone']];ox,oy,z=s['origin'];x,y,w,h=r['rect'];b=i['bounds'];f=features.get(i['id']+'_0')
    if not f or f['z']!=z or any(abs(a-c)>.00001 for a,c in zip(f['rect'],[b[0],b[1],b[3]-b[0],b[4]-b[1]])):issues.append([i['id'],'nav_bounds'])
    if not(ox+x+.05<=b[0] and b[3]<=ox+x+w-.05 and oy+y+.05<=b[1] and b[4]<=oy+y+h-.05 and abs(b[2]-z)<.0001):issues.append([i['id'],'room_bounds_or_floor'])
    front=i['front_access']
    if solid.hits(z,[front[0]-.4,front[1]-.4,.8,.8],.02):issues.append([i['id'],'blocked_front'])
check('all_new_room_kit_bounds_and_80cm_front_approaches_are_valid',not issues)
check('all_original_room_and_bay_destinations_still_reachable',len(d['navigation_routes'])==562 and len(d['bay_navigation_routes'])==1939)
check('new_galleries_receive_regular_ceiling_lamps',u['bridge_lights']>40 and e['white_lights'][:len(old['environment_dressing']['white_lights'])]==old['environment_dressing']['white_lights'])
report=dict(success=all(checks.values()),checks=checks,issues=issues,map_sha256=hashlib.sha256(PATH.read_bytes()).hexdigest(),counts=json.loads((OUT/'expansion_inventory.json').read_text(encoding='utf-8')),limits=['Prototype art; new weapons at sentry posts remain decorative.','Doors and new protected glazing are rendered and tested by the rebuilt native executable.','Static route checks cover the original hospital campus; scenery terrain is tested separately in native capture views.'])
(OUT/'expansion_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(dict(success=report['success'],checks=checks,issues=issues[:20]),ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
