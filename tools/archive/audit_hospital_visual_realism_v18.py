"""Independent metric, collision, source-asset and fixture support audit."""
import collections,hashlib,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v18';P=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(P.read_text(encoding='utf8'));old=json.loads((OUT/'baseline_v17.json').read_text(encoding='utf8'));u=d['visual_realism'];checks={};issues=[]
def check(k,v):checks[k]=bool(v)
check('all_562_zones_metrics_buildings_and_floors_preserved',d['locations']==old['locations'])
keys=('stairs','lift','passages','site_entries','windows','escape_niches','prototype_props','environment_props','prototype_catalog','furnishings','hideouts','campus_plan','campus_upgrade','realism_upgrade','finish_upgrade','utility_links','raised_access','collision_features','navigation_routes','bay_navigation_routes','story_gameplay')
check('all_previous_collision_story_hide_spaces_and_routes_unchanged',all(d[k]==old[k] for k in keys))
check('all_previous_furniture_and_environment_parts_preserved',d['environment_dressing']['boxes'][:len(old['environment_dressing']['boxes'])]==old['environment_dressing']['boxes'])
props=[p for p in d['prototype_props']+d['environment_props'] if p['mount']=='floor' and str(p['model']) in u['mesh_replacements']]
check('every_scanned_replacement_is_registered',len(props)==u['replacement_count']==2397)
manifest=json.loads((ROOT/'unreal/EchoWard/RawAssets/manifest.json').read_text());assets={a['name']:a for a in manifest['meshes']}
check('every_scanned_mesh_and_its_three_pbr_textures_exist',all((ROOT/'unreal/EchoWard/Content/Prototype/Meshes'/f"{a['mesh']}.uasset").exists() and all((ROOT/'unreal/EchoWard/Content/Prototype/Textures'/f"T_{a['asset']}_{kind}.uasset").exists() for kind in ('color','normal','arm')) for a in u['mesh_replacements'].values()))
# Check dimensions independently of runtime scaling. The chosen scans fit the
# old envelopes without extreme axis stretching or enlargement of furniture.
ratios=[]
for model,spec in u['mesh_replacements'].items():
    a=assets[spec['mesh']];sz=[a['maximum'][i]-a['minimum'][i] for i in range(3)]
    if spec['yaw']==90:sz[0],sz[1]=sz[1],sz[0]
    b=d['prototype_catalog'][model]['bounds'];r=[(b[i+3]-b[i])*100/sz[i] for i in range(3)];ratios.append(dict(model=model,scale=r))
check('scans_keep_sensible_human_scale',all(.65<min(q['scale']) and max(q['scale'])<1.25 for q in ratios))
parts=collections.defaultdict(list)
for b in d['environment_dressing']['boxes'][len(old['environment_dressing']['boxes']):]:parts[b['item_id']].append(b)
check('all_new_parts_nonblocking_and_finite',all(not b['solid'] and min(b['size'])>0 and all(math.isfinite(v) for v in b['at']+b['size']) and b['material'] in d['environment_dressing']['materials'] for bs in parts.values() for b in bs))
check('all_new_assemblies_account_for_their_parts',all(len(parts[q['id']])==q['parts'] for q in u['items']))
w=[q for q in u['items'] if q['kind']=='exterior_window_joinery'];check('all_836_external_window_openings_have_joinery',len(w)==sum(q.get('role')=='external_view' for q in d['windows'])==836)
fixtures=[q for q in u['items'] if q['kind']=='luminaire_housing']
check('all_1939_room_luminaires_have_suspended_housings',len(fixtures)==1939)
for q in fixtures:
    bs=parts[q['id']];top=max(b['at'][2]+b['size'][2]/2 for b in bs)
    if abs(top-q['parent_at'][2]-.265)>.00001:issues.append([q['id'],'suspension_top'])
check('suspension_rods_reach_the_existing_ceiling',not issues)
report=dict(success=all(checks.values()),map_sha256=hashlib.sha256(P.read_bytes()).hexdigest(),checks=checks,issues=issues,scale_ratios=ratios,inventory=json.loads((OUT/'inventory.json').read_text(encoding='utf8')))
(OUT/'realism_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
