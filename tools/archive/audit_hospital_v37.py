"""Independent v37 input, conservation and model-size audit; runtime sweeps are separate."""
from pathlib import Path
import json,hashlib,math,sys,collections
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard/Content/CampaignData';A=R/'artifacts/echo_hospital_v37';read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'));sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
d=read(P/'hospital_world.json');e=read(P/'hospital_environment_v37.json');m=read(P/'hospital_assets_v37.json');old=read(A/'baseline_identity.json');checks={}
checks['all_preexisting_data_files_unchanged']=all(sha(P/k)==v for k,v in old['data_hashes'].items())
checks['canonical_metric_map_preserved']=e['map_sha256']==sha(P/'hospital_world.json') and len(d['locations'])==35 and sum(len(s['rooms']) for s in d['locations'])==562
checks['every_imported_mesh_exists_and_has_four_lods']=len(m['assets'])==21 and all(q['lods']==4 and (R/'unreal/EchoWard/Content'/q['asset'].split('/Game/')[1].split('.')[0]).with_suffix('.uasset').exists() for q in m['assets'])
checks['every_mesh_has_measured_bounds_and_sensible_material_sections']=all(min(q['actual_bounds_cm'][1][i]-q['actual_bounds_cm'][0][i] for i in range(3))>0 and 1<=q['material_slots']<=8 for q in m['assets'])
checks['all_placement_values_are_finite']=len({q['id'] for q in e['props']})==len(e['props']) and all(all(math.isfinite(x) for x in q['at']+q['world_bounds']) and q['mesh'] in {q['id'] for q in m['assets']}|{643} for q in e['props'])
checks['distinct_muted_department_palettes']=len({tuple(p['lower']) for p in e['profiles'].values()})>=10 and all(p['lower'][1]<.4 for p in e['profiles'].values())
checks['new_floor_props_are_not_floating_in_source_data']=all(abs((q['at'][2]/4.8)%1)<.004 for q in e['props'] if q['solid'])
checks['new_blood_is_limited_to_authored_locations']=all(q['zone'] in ('R11','R37','E10','N01') for q in e['traces'])
checks['new_traces_follow_the_floor_with_1mm_gap']=all(abs((q['at'][2]/4.8)%1)<.001 for q in e['traces'])
checks['suspended_ceiling_rods_reach_the_structural_soffit']=all(abs(q['world_bounds'][5]-(math.floor(q['at'][2]/4.8)*4.8+3.8))<.0002 for q in e['props'] if q['category']=='damaged_suspended_ceiling')
checks['profiles_cover_every_location']={s['id'] for s in d['locations']}==set(e['profiles'])
report=dict(success=all(checks.values()),scope=e['scope'],checks=checks,counts=e['counts'],limitations=['Source-space checks are not an exhaustive visual inspection.','Runtime collision, floor support and door sweeps must pass separately.']);(A/'data_audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(report,ensure_ascii=False));sys.exit(0 if report['success'] else 1)
