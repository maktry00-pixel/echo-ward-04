"""Independent import, source conservation, native binding and material audit."""
import hashlib,json,collections
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';OUT=R/'artifacts/echo_integration_v20';OUT.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
d=read(P/'Content/CampaignData/hospital_world.json');runtime=read(P/'Content/CampaignData/echo_custom_runtime.json');prepared=read(P/'RawAssets/EchoCustom/manifest.json');bindings=read(P/'Content/CampaignData/echo_custom_bindings.json');checks={}
def check(k,v):checks[k]=bool(v)
check('map_geometry_furniture_story_and_all_562_zones_are_byte_identical_to_v19',sha(P/'Content/CampaignData/hospital_world.json')=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec')
check('all_562_static_library_entries_and_11_door_leaves_are_imported',runtime['success'] and len(runtime['meshes'])==573 and {m['id'] for m in runtime['meshes']}==set(range(1,563)) and sum(m['door_leaf'] for m in runtime['meshes'])==11)
check('every_imported_mesh_has_a_real_native_package',all((P/'Content'/m['asset'].split('.')[0].removeprefix('/Game/') ).with_suffix('.uasset').is_file() for m in runtime['meshes']))
check('all_imported_metric_bounds_match_independent_gltf_conversion',all(max(abs(x-y) for a,b in zip(m['bounds_cm'],q['bounds_cm']) for x,y in zip(a,b))<=.3 for m,q in zip(runtime['meshes'],prepared['assets'])))
check('import_sources_match_unmodified_authored_glbs',all(sha(R/f'assets/models/echo_custom/EH_{m["id"]:03}/EH_{m["id"]:03}.glb')==m['source_sha256'] for m in runtime['meshes']))
check('all_96_semantic_replacements_resolve_to_imported_models',len(bindings['prototype_models'])==96 and all(int(k) in {int(i) for i in d['prototype_catalog']} and v in set(range(1,563)) for k,v in bindings['prototype_models'].items()))
check('wardrobe_hideout_and_unsupported_specialty_equipment_not_replaced',all(str(i) not in bindings['prototype_models'] for i in (6,47,48,49,50,185,186,187)))
check('all_surface_pbr_packages_exist',len(runtime['surfaces'])==16 and all((P/'Content'/s.split('.')[0].removeprefix('/Game/')).with_suffix('.uasset').is_file() for s in runtime['surfaces'].values()))
check('rigged_characters_not_silently_replaced_with_unanimated_meshes',runtime['skeletal_deferred']==list(range(563,576)))
check('glass_and_metal_finish_completed',read(P/'Saved/Prototype/echo_material_finish.json')['success'])
counts=collections.Counter(p['model'] for p in d['prototype_props']+d['environment_props'] if str(p['model']) in bindings['prototype_models'])
report=dict(success=all(checks.values()),checks=checks,map_sha256=sha(P/'Content/CampaignData/hospital_world.json'),runtime_sha256=sha(P/'Content/CampaignData/echo_custom_runtime.json'),bindings_sha256=sha(P/'Content/CampaignData/echo_custom_bindings.json'),imported_meshes=573,imported_static_entries=562,semantic_bindings=96,scheduled_replacements=sum(counts.values()),schedule_by_model=dict(sorted(counts.items())),limits=['Library import is distinct from active placement. Not all 562 static entries have an existing placement.','Skeletal models 563–575 remain deferred pending complete animation and art.','Native collision, working doors and selected captured appearances require the separate runtime checks.'])
(OUT/'acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False,indent=2));assert report['success']
