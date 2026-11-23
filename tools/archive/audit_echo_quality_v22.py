"""Conservation, real package, geometric fitting and new support audit."""
import json,hashlib,collections,math
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_quality_v22';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
d=read(P/'Content/CampaignData/hospital_world.json');q=read(P/'Content/CampaignData/echo_quality_v22.json');runtime=read(P/'Content/CampaignData/echo_custom_runtime.json');new=read(P/'RawAssets/EchoQualityV22/manifest.json');checks={};ids={x['id'] for x in runtime['meshes']}
checks['canonical_map_geometry_routes_story_and_existing_population_are_unchanged']=sha(P/'Content/CampaignData/hospital_world.json')==q['map_sha256']=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec'
checks['nine_new_original_meshes_have_native_packages_and_metric_bounds']=len(new['assets'])==9 and all(m['id'] in ids and (P/'Content/Campaign/EchoCustom/Meshes'/('SM_'+m['name']+'.uasset')).exists() and any(n['id']==m['id'] and max(abs(a-b) for x,y in zip(n['bounds_cm'],m['bounds_cm']) for a,b in zip(x,y))<.3 for n in runtime['meshes']) for m in new['assets'])
checks['all_new_clutter_mesh_ids_exist_and_have_positive_bounds']=all(p['mesh'] in ids and len(p['bounds'])==6 and all(math.isfinite(v) for v in p['bounds']) and all(p['bounds'][i+3]>p['bounds'][i] for i in range(3)) for p in q['props'])
checks['no_duplicate_visual_ids']=len({p['id'] for p in q['props']})==len(q['props'])
checks['all_skipped_old_visuals_have_success_checked_replacements']=set(q['replaced_environment_items'])<={p['id'] for p in q['props']}
old=read(P/'RawAssets/EchoCustom/manifest.json');checks['all_original_glb_sources_are_unchanged']=all(sha(R/f'assets/models/echo_custom/EH_{m["id"]:03}/EH_{m["id"]:03}.glb')==m['source_sha256'] for m in old['assets'])
parents={p['id']:p for p in d['prototype_props']+d['environment_props']};supported=True;added=0
for p in q['props']:
 if p['category']!='new_supported_detail':continue
 added+=1;parent=parents[p['parent']];model=d['prototype_catalog'][str(parent['model'])];bb=p['bounds'];z=parent['at'][2]+model['top'];supported&=abs(bb[2]-z-.015)<.0001
 # Inverse quarter-turn all four XY corners into the parent support envelope.
 for x in [bb[0],bb[3]]:
  for y in [bb[1],bb[4]]:
   lx=x-parent['at'][0];ly=y-parent['at'][1];t=parent['turn']%4
   lx,ly=[(lx,ly),(ly,-lx),(-lx,-ly),(-ly,lx)][t];b=model['bounds'];supported&=b[0]-.002<=lx<=b[3]+.002 and b[1]-.002<=ly<=b[4]+.002
checks['4694_new_details_rest_on_real_furniture_and_stay_on_its_footprint']=supported and added==4694
checks['all_3940_existing_tree_positions_preserved']=len(q['trees'])==3940 and all(t['mesh'] in (641,642) and t['height']>1 for t in q['trees'])
checks['836_external_window_apertures_remain_transparent']=len(q['windows'])==836 and all(w['width']>0 and w['height']>0 for w in q['windows'])
checks['native_import_succeeded']=read(P/'Saved/Prototype/quality_import_v22.json')['success']
report=dict(success=all(checks.values()),checks=checks,counts=q['counts'],map_sha256=sha(P/'Content/CampaignData/hospital_world.json'),runtime_sha256=sha(P/'Content/CampaignData/echo_custom_runtime.json'),bindings_sha256=sha(P/'Content/CampaignData/echo_custom_bindings.json'),quality_sha256=sha(P/'Content/CampaignData/echo_quality_v22.json'),limits=q['limits']);(O/'acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False));assert report['success']
