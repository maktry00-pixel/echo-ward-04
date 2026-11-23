"""Independent bounds/coverage contracts for the supplemental environment."""
import collections, hashlib, json, math
from pathlib import Path
from hospital_furnishing_v6 import Index,overlap
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v9'
path=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(path.read_text(encoding='utf-8-sig'));old=json.loads((ROOT/'artifacts/unreal_hospital_v8/hospital_world_source.json').read_text(encoding='utf-8-sig'))
e=d['environment_dressing'];props=d['environment_props'];checks={};issues=[]
def check(name,ok):checks[name]=bool(ok)
check('all_562_zone_coordinates_areas_and_37_stairs_unchanged',d['locations']==old['locations'] and d['stairs']==old['stairs'])
check('original_10978_numbered_furniture_placements_unchanged',d['prototype_props']==old['prototype_props'])
check('all_395_story_items_and_46_mobs_keep_their_positions',d['story_gameplay']['placements']==old['story_gameplay']['placements'] and d['story_gameplay']['mobs']==old['story_gameplay']['mobs'])
check('every_logical_bay_has_supplemental_lighting',len(e['bay_coverage'])==len(d['subrooms'])==1939 and all(c['ceiling_fixture'] for c in e['bay_coverage']))
check('new_room_kits_have_unique_ids_and_real_floor_collision',len(props)==len({p['id'] for p in props}) and {p['id'] for p in props}<={f['id'] for f in d['collision_features']})
index=Index()
for f in d['collision_features']:
    if not f['id'].startswith('ENV9_'):index.add(f['z'],f['rect'])
for q in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):index.add(q['z'],q['rect'])
bybay={b['id']:b for b in d['subrooms']};sites={l['id']:l for l in d['locations']}
for p in props:
    b=d['prototype_catalog'][str(p['model'])]['bounds'];w,h=b[3]-b[0],b[4]-b[1]
    if p['turn']%2:w,h=h,w
    x,y,z=p['at'];expected=[x-w/2,y-h/2,w,h];r=p['rect'];bay=bybay[p['subroom']];ox,oy,zz=sites[p['site']]['origin'];bx,by,bw,bh=bay['rect']
    if max(abs(a-b) for a,b in zip(expected,r))>.0002 or not(ox+bx<=r[0] and oy+by<=r[1] and r[0]+r[2]<=ox+bx+bw+.001 and r[1]+r[3]<=oy+by+bh+.001 and z==zz):issues.append([p['id'],'bounds'])
    if index.hits(z,r,.04):issues.append([p['id'],'overlap'])
    index.add(z,r)
check('all_added_floor_models_fit_their_bays_without_solid_overlaps',not issues)
check('new_lawns_preserve_buildings_service_areas_and_roads',all(not overlap(r,q) for r in e['lawns'] for q in e['protected_surface_rects']+e['roads']))
check('no_decoration_creates_a_new_floor_or_ceiling_blocker',all(not b['solid'] or b['kind']=='lamp_post' for b in e['boxes']))
check('all_wall_and_floor_stains_are_thin_attached_dressing',all(not b['solid'] and min(b['size'])<=.0121 for b in e['boxes'] if b['kind'] in ('wall_stain','floor_stain')))
check('departments_have_cameras_radiators_switches_and_red_beacons',e['camera_count']>=500 and all(e['fixture_counts'][k]>=500 for k in ('camera_housing','radiator','switch','red_beacon','extinguisher')))
check('new_light_anchors_are_finite_and_at_fixture_height',all(len(p)==3 and all(math.isfinite(v) for v in p) for p in e['white_lights']+e['red_lights']) and len(e['white_lights'])>=2000 and len(e['red_lights'])>=500)
check('moon_is_inside_existing_sky_and_above_campus',math.dist(e['moon']['at_cm'],[0,0,0])+e['moon']['scale']*50<200000 and e['moon']['at_cm'][2]>50000)
report=dict(success=all(checks.values()),checks=checks,issues=issues,map_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),counts=dict(floor_props=len(props),boxes=len(e['boxes']),cameras=e['camera_count'],lawns_m2=sum(r[2]*r[3] for r in e['lawns'])),limits=e['limits'])
(OUT/'environment_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
