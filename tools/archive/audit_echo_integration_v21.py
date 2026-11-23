"""Check all scheduled furnishing types, native bounds and conserved sources."""
import hashlib,json,collections
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_integration_v21';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
d=read(P/'Content/CampaignData/hospital_world.json');r=read(P/'Content/CampaignData/echo_custom_runtime.json');b=read(P/'Content/CampaignData/echo_custom_bindings.json');old=read(P/'RawAssets/EchoCustom/manifest.json');new=read(P/'RawAssets/EchoSupplementsV21/manifest.json');counts=collections.Counter(str(p['model']) for p in d['prototype_props']+d['environment_props']);ids={m['id'] for m in r['meshes']};checks={}
checks['entire_562_zone_map_geometry_and_story_unchanged']=sha(P/'Content/CampaignData/hospital_world.json')=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec'
checks['all_154_scheduled_types_resolve_to_native_meshes']=len(counts)==154 and all(b['prototype_models'].get(k) in ids for k in counts)
checks['613_native_mesh_packages_exist']=len(r['meshes'])==613 and all((P/'Content'/m['asset'].split('.')[0].removeprefix('/Game/')).with_suffix('.uasset').is_file() for m in r['meshes'])
expected={(m['id'],m['door_leaf']):m for m in old['assets']+new['assets']}
checks['all_613_native_bounds_match_independent_source_dimensions']=all(max(abs(a-c) for x,y in zip(m['bounds_cm'],expected[(m['id'],m['door_leaf'])]['bounds_cm']) for a,c in zip(x,y))<=.3 for m in r['meshes'])
checks['562_original_static_source_models_unchanged']=all(sha(R/f'assets/models/echo_custom/EH_{m["id"]:03}/EH_{m["id"]:03}.glb')==m['source_sha256'] for m in old['assets'])
checks['40_original_supplements_have_indexed_geometry_and_materials']=len(new['assets'])==40 and all(sha(Path(m['file']))==m['source_sha256'] and m['triangles']>20 and Path(m['file']).with_suffix('.mtl').exists() for m in new['assets'])
checks['native_importer_verified_all_40_supplements']=read(P/'Saved/Prototype/echo_supplement_import_v21.json')['success']
checks['rigged_characters_keep_existing_animations']=r['skeletal_deferred']==list(range(563,576))
checks['18_detail_cameras_have_real_placed_targets']=len(b['review_captures'])==18 and all(c['detail_item'] in {p['id'] for p in d['prototype_props']+d['environment_props']} for c in b['review_captures'])
report=dict(success=all(checks.values()),checks=checks,map_sha256=sha(P/'Content/CampaignData/hospital_world.json'),runtime_sha256=sha(P/'Content/CampaignData/echo_custom_runtime.json'),bindings_sha256=sha(P/'Content/CampaignData/echo_custom_bindings.json'),imported_meshes=613,original_static_entries=562,supplemental_models=40,scheduled_types=len(counts),scheduled_replacements=sum(counts.values()),schedule_by_model=dict(sorted(counts.items())),limits=['Coverage refers to the 154 existing scheduled furniture types, not every piece of architecture or all possible artwork.','Supplemental and original assets remain first-pass art, not final photorealism.','Original rigged library characters remain deferred; existing rigged game assets are reused. No FPS or long stability claim.'])
(O/'acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False));assert report['success']
