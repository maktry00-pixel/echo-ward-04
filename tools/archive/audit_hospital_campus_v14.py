"""Independent conservation, geometry, public/clinical separation and kit QA."""
import collections, hashlib, json, math
from pathlib import Path
from hospital_clutter_v10 import Space, bounds, rotate
from hospital_furnishing_v6 import Index, overlap
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v14'
path=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(path.read_text(encoding='utf-8'));old=json.loads((OUT/'baseline_v13.json').read_text(encoding='utf-8'))
p=d['campus_plan'];checks={};issues=[]
def check(k,v):checks[k]=bool(v)
skip={'review_revision','environment_dressing','collision_features','furnishing_captures','story_gameplay','navigation_routes','bay_navigation_routes','prototype_props'}
check('all_562_metric_rooms_floors_doors_lifts_hides_and_prior_content_retained',all(d[k]==v for k,v in old.items() if k not in skip))
check('all_original_story_positions_mobs_notes_and_progression_retained',all(d['story_gameplay'][k]==v for k,v in old['story_gameplay'].items() if k!='navigation'))
check('all_v13_hospital_services_and_address_coverage_retained',d['hospital_services']==old['hospital_services'] and len(d['hospital_services']['requirements'])==123)
e=d['environment_dressing'];oe=old['environment_dressing']
check('old_lighting_surfaces_and_202895_parts_retained',[b for b in e['boxes'] if b.get('layer')!=14]==oe['boxes'] and all(e[k]==v for k,v in oe.items() if k not in ('boxes','materials')) and all(e['materials'][k]==v for k,v in oe['materials'].items()))
rel=p['furniture_relocations'];moved={r['id'] for r in rel}
check('only_R01_terminal_moved_other_10977_scheduled_props_retained',moved=={'PROP6_R01_082-01'} and len(d['prototype_props'])==10978 and all(a==b for a,b in zip(d['prototype_props'],old['prototype_props']) if a['id'] not in moved) and all({k:v for k,v in a.items() if k not in ('at','turn','rect')}=={k:v for k,v in b.items() if k not in ('at','turn','rect')} for a,b in zip(d['prototype_props'],old['prototype_props']) if a['id'] in moved))
check('old_collisions_and_views_retained_except_recorded_terminal_pose',[f for f in d['collision_features'] if f.get('source')!='hospital_campus_v14' and f['id'] not in moved]==[f for f in old['collision_features'] if f['id'] not in moved] and d['furnishing_captures'][:len(old['furnishing_captures'])]==old['furnishing_captures'])
zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
check('shops_and_cafe_entirely_outside_clinical_boundary',all(zones[z][0]['origin'][1]+zones[z][1]['rect'][1]+zones[z][1]['rect'][3]<=-84 for z in p['public_retail']))
rect=p['clinical_boundary']['rect'];x,y,w,h=rect
check('all_medical_buildings_labs_and_staff_service_blocks_inside_wall',all(x<=s['origin'][0]+r['rect'][0] and s['origin'][0]+r['rect'][0]+r['rect'][2]<=x+w and y<=s['origin'][1]+r['rect'][1] and s['origin'][1]+r['rect'][1]+r['rect'][3]<=y+h for s in d['locations'] if s['id'] in p['clinical_sites'] for r in s['rooms']))
check('six_genuine_enclosed_links_and_all_previous_dogleg_tunnels_retained',len(p['connections'])==6 and all(q['passage'] in d['passages'] for q in p['connections']) and p['underground_links']==[q['id'] for q in d['utility_links']] and d['utility_links']==old['utility_links'])
wall=[b for b in e['boxes'] if b.get('layer')==14 and b['kind']=='campus_perimeter_wall' and b['solid']]
check('physical_continuous_wall_with_six_clear_controlled_gates',len(p['clinical_boundary']['gates'])==6 and len(wall)>1000 and all(g['width_m']>=10 and g['always_passable'] for g in p['clinical_boundary']['gates']))
parts=collections.defaultdict(list)
for b in e['boxes']:
    if b.get('layer')==14:parts[b['item_id']].append(b)
items=p['items'];features=collections.defaultdict(list)
for f in d['collision_features']:
    if f.get('source')=='hospital_campus_v14':features[f['id'].rsplit('_',1)[0]].append(f)
check('every_new_item_has_complete_unique_finite_render_geometry',len(items)==len({i['id'] for i in items})==len(parts) and all(len(parts[i['id']])==i['parts'] for i in items) and all(min(b['size'])>0 and all(math.isfinite(n) for n in b['at']+b['size']) and b['material'] in e['materials'] for bs in parts.values() for b in bs))
check('all_new_physical_items_match_navigation_obstacles',all(sorted(tuple(f['rect']) for f in features[i['id']])==sorted(tuple(r) for r in i.get('nav_rects',[])) for i in items if i['solid']))
old_solid=Index()
for f in old['collision_features']:old_solid.add(f['z'],f['rect'])
structure_conflicts=[f['id'] for f in d['collision_features'] if f.get('source')=='hospital_campus_v14' and old_solid.hits(f['z'],f['rect'])]
check('new_walls_checkpoint_parts_and_outdoor_obstacles_do_not_engulf_old_furniture',not structure_conflicts)
floor=Index();space=Space();new=Space()
for f in old['collision_features']:floor.add(f['z'],f['rect'])
for a in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):floor.add(a['z'],a['rect'])
for prop in old['prototype_props']+old['environment_props']:
    for a in old['prototype_catalog'][str(prop['model'])]['parts']:
        off=rotate(a[:3],prop['turn']);at=[prop['at'][j]+off[j] for j in range(3)];sz=list(a[3:6])
        if prop['turn']%2:sz[0],sz[1]=sz[1],sz[0]
        space.add(bounds(at,sz),prop['id'])
for b in oe['boxes']:
    if b['kind'] not in ('lawn','grass_tuft','pavement','road_dash','tree_crown'):space.add(bounds(b['at'],b['size']),'old')
for item in items:
    if item.get('mount')!='floor':continue
    bb=item['bounds'];s,r=zones[item['zone']];ox,oy,z=s['origin'];rx,ry,rw,rh=r['rect'];rr=[bb[0],bb[1],bb[3]-bb[0],bb[4]-bb[1]]
    if not(ox+rx+.05<=bb[0]<bb[3]<=ox+rx+rw-.05 and oy+ry+.05<=bb[1]<bb[4]<=oy+ry+rh-.05 and abs(bb[2]-z)<1e-5):issues.append([item['id'],'outside_room_or_floor'])
    if floor.hits(z,rr,.08):issues.append([item['id'],'wall_or_old_solid_overlap'])
    if any(space.hits(bounds(b['at'],b['size'])) or new.hits(bounds(b['at'],b['size'])) for b in parts[item['id']]):issues.append([item['id'],'old_or_new_mesh_overlap'])
    for b in parts[item['id']]:new.add(bounds(b['at'],b['size']),item['id'])
    floor.add(z,rr)
check('all_new_full_size_room_kits_fit_without_wall_or_furniture_intersection',not issues)
solid=Index()
for f in d['collision_features']:solid.add(f['z'],f['rect'])
for a in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):solid.add(a['z'],a['rect'])
check('every_new_room_kit_has_80cm_clear_front_approach',all(not solid.hits(i['front_access'][2],[i['front_access'][0]-.4,i['front_access'][1]-.4,.8,.8],.025) for i in items if i.get('mount')=='floor'))
check('R01_patient_screen_has_an_actual_80cm_standing_approach',all(not solid.hits(0,[q['front_access'][0]-.4,q['front_access'][1]-.4,.8,.8],.025) for q in rel))
check('DG25_DG26_keep_1216sqm_and_have_distinct_four_section_roles',len(p['halls'])==2 and all(h['area_m2']==1216 and len(h['items'])>=12 and len(h['sections'])==4 for h in p['halls']) and p['halls'][0]['role']!=p['halls'][1]['role'])
check('all_four_requested_departments_have_individual_signature_kits',all(any(i['zone'].startswith(site) and i.get('mount')=='floor' and i['kind'] in kinds for i in items) for site,kinds in [('PS',{'therapy_bench','sensory_station'}),('RH',{'gait_rail','exercise_cycle'}),('HS',{'domestic_kitchen','personal_locker','notice_stand'}),('SV',{'compressor','pump_skid','duct_rack'})]))
check('outdoors_has_seven_different_usable_districts_and_nonblocking_canopy',len(p['districts'])==7 and all(any(i.get('district')==a['id'] for i in items) for a in p['districts']) and len(p['canopies'])>=1)
check('two_real_checkpoints_and_wall_positions_with_weapons_not_new_npcs',sum(i['kind']=='checkpoint_shelter' for i in items)==2 and sum(i['kind']=='wall_weapon_rack' for i in items)==2 and len(p['sentry_platforms'])==2 and all(q['tread_m']>=.3 and q['riser_m']<=.17 and q['width_m']>=1.2 for q in p['sentry_platforms']))
check('existing_hiding_methods_and_species_specific_counterplay_preserved',d['hideouts']==old['hideouts'] and all(collections.Counter(q['kind'] for q in d['hideouts'])[k]>100 for k in ('wardrobe','under_bed','architectural_recess')) and set(p['hide_counterplay']['exceptions'])=={'listener','blind_surgeon','hound','remembered_pursuit'})
check('all_rooms_and_bays_reachable_after_obstacles',len(d['navigation_routes'])==562 and len(d['bay_navigation_routes'])==len(d['subrooms'])==1939)
report=dict(success=all(checks.values()),checks=checks,issues=issues,map_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),inventory=json.loads((OUT/'campus_inventory.json').read_text(encoding='utf-8')),limits=p['limits'])
(OUT/'campus_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(success=report['success'],checks=checks,issues=issues[:30]),ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
