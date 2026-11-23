"""Independent retained-art, real support triangles and reserved-volume audit."""
from pathlib import Path
import collections,hashlib,json,math
import numpy as np
from hospital_clutter_v10 import Space,bounds
from echo_support_coverage_v23 import covers,restore
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_detail_v27'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
a=read(P/'Content/CampaignData/echo_detail_v27.json');old=read(O/'before_identity.json');d=read(P/'Content/CampaignData/hospital_world.json');runtime=read(P/'Content/CampaignData/echo_custom_runtime.json');previous=read(O/'runtime.before.json');ids={p['id']:p for p in runtime['meshes']};raw=read(P/'RawAssets/EchoDetailV27/manifest.json');checks={}
checks['canonical_map_story_and_all_old_dressing_layers_unchanged']=all(sha(P/'Content/CampaignData'/n)==h for n,h in old['unchanged_data_hashes'].items() if n!='echo_custom_runtime.json') and sha(P/'Content/CampaignData/echo_detail_v26.json')==old['detail_data_sha256'] and a['map_sha256']==old['map_sha256']
all_entries={(p['id'],p.get('door_leaf',False)):p for p in runtime['meshes']}
checks['all_771_previous_native_metadata_entries_are_retained']=all(row==all_entries.get((row['id'],row.get('door_leaf',False))) for row in previous['meshes']) and len(runtime['meshes'])==len(previous['meshes'])+15
checks['fifteen_original_meshes_imported_with_metric_bounds_and_lods']=len(raw['assets'])==15 and all(sha(Path(p['file']))==p['source_sha256'] and p['id'] in ids and (P/'Content/Campaign/EchoCustom/Meshes'/('SM_'+p['name']+'.uasset')).exists() and ids[p['id']].get('lods',0)>=(4 if p['triangles']>1500 else 1) for p in raw['assets'])
checks['ten_semantic_medical_replacements_are_valid_without_chains']=len(a['mesh_aliases'])==10 and all(int(k) in ids and v in ids and str(v) not in a['mesh_aliases'] for k,v in a['mesh_aliases'].items())
checks['unique_ids_native_models_finite_positive_metric_bounds']=len({p['id'] for p in a['props']})==len(a['props']) and all(p['mesh'] in ids and all(math.isfinite(v) for v in p['at']+p['bounds']+p['world_bounds']) and all(p['bounds'][i+3]>p['bounds'][i] for i in range(3)) for p in a['props'])
space=Space();overlaps=[]
for p in a['props']:
 if space.hits(p['world_bounds']):overlaps.append(p['id'])
 space.add(p['world_bounds'],p['id'])
checks['new_belongings_do_not_overlap_one_another']=not overlaps
reserved=Space()
for s in d['locations']:
 for r in s['rooms']:
  if not r.get('enclosed',True):continue
  for door in [r.get('door')]+[x['at'] for x in r.get('extra_doors',[])]:
   if door:reserved.add(bounds([s['origin'][0]+door[0],s['origin'][1]+door[1],s['origin'][2]+1.3],[2.8,2.8,2.7]),'door_'+r['id'])
for h in d['hideouts']:
 x,y,z=h['at'];w,dep,height=h['size'];reserved.add([x-.8,y-.8,z-.05,x+w+.8,y+dep+1.3,z+height+.2],h['id'])
checks['all_door_approach_and_hide_volumes_remain_clear']=all(not reserved.hits(p['world_bounds']) for p in a['props'])
checks['decorative_objects_add_no_route_colliders']=all(not p['solid'] for p in a['props'])
planes=read(P/'RawAssets/EchoRealV23/support_planes.json');supported=[p for p in a['props'] if p['category'].startswith('supported_')]
checks['supplies_fit_exposed_triangle_union_and_shelf_clearance']=len(supported)>1000 and all(abs(p['world_bounds'][2]-p['support']['z']-.01)<.002 and covers(restore(p['support'],planes),p['world_bounds']) for p in supported)
checks['ceiling_pipes_stay_below_ceilings_above_doors']=all(p['world_bounds'][5]<p['support_ceiling']-.04 and p['world_bounds'][2]>p['support_ceiling']-.7 for p in a['props'] if p['category']=='ceiling_services')
zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};floor_ok=True
for p in [p for p in a['props'] if p['category']=='clinical_floor_equipment']:
 s,r=zones[p['zone']];x,y,w,h=r['rect'];ox,oy,z=s['origin'];b=p['world_bounds'];floor_ok&=b[0]>=x+ox+.17 and b[1]>=y+oy+.17 and b[3]<=x+ox+w-.17 and b[4]<=y+oy+h-.17 and abs(b[2]-z-.018)<.002
checks['floor_medical_equipment_fits_room_edges_and_rests_on_floor']=floor_ok
counts=collections.Counter(p['category'] for p in a['props']);checks['counts_and_categories_are_complete']=a['counts']==dict(total=len(a['props']),**counts)
checks['native_import_report_complete']=read(P/'Saved/Prototype/detail_v27_import.json')['success']
checks['v25_performance_fixes_remain']='if(LightSelectionClock<.24f)return' in (P/'Source/EchoWard/WardBookWorld.cpp').read_text(encoding='utf-8-sig') and 'if(LiftDisplay->Text.ToString()!=Caption)' in (P/'Source/EchoWard/WardHospitalDetails.cpp').read_text(encoding='utf-8-sig')
out=dict(success=all(checks.values()),checks=checks,counts=a['counts'],new_meshes=len(raw['assets']),overlaps=overlaps,map_sha256=a['map_sha256'],data_sha256=sha(P/'Content/CampaignData/echo_detail_v27.json'))
(O/'acceptance.json').write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(out,ensure_ascii=False));assert out['success'],[k for k,v in checks.items() if not v]
