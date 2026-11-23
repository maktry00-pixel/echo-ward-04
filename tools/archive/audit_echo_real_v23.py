"""Independent source, support, route-clearance and native-art checks."""
import collections,hashlib,json,math
from pathlib import Path
import numpy as np
from echo_support_coverage_v23 import covers,restore
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';OUT=R/'artifacts/echo_real_v23';OUT.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
d=read(P/'Content/CampaignData/hospital_world.json');art=read(P/'Content/CampaignData/echo_real_v23.json');raw=read(P/'RawAssets/EchoRealV23/manifest.json');runtime=read(P/'Content/CampaignData/echo_custom_runtime.json');ids={a['id']:a for a in runtime['meshes']};checks={};source=read(R/'assets/external/quality_v23/SOURCE_MANIFEST.json')['assets']+read(R/'assets/external/quality_v23/SAMPLES_MANIFEST.json')['assets']+read(R/'assets/external/quality_v23/EXTRA_MANIFEST.json')['assets']
checks['canonical_562_zone_map_is_byte_for_byte_unchanged']=sha(P/'Content/CampaignData/hospital_world.json')==art['map_sha256']=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec'
checks['all_149_prepared_real_assets_have_verified_native_metric_bounds']=len(raw['assets'])==149 and all(a['id'] in ids and (P/'Content/Campaign/EchoCustom/Meshes'/('SM_'+a['name']+'.uasset')).exists() and max(abs(x-y) for u,v in zip(a['bounds_cm'],ids[a['id']]['bounds_cm']) for x,y in zip(u,v))<.5 for a in raw['assets'])
files_ok=True;file_count=0
for a in source:
 if 'publisher_files' in a:
  entry=a['publisher_files'];path=Path(a['file']);jobs=[(path,entry)]+[(path.parent/name,spec) for name,spec in entry['include'].items()]
  for p,spec in jobs:file_count+=1;files_ok&=p.exists() and hashlib.md5(p.read_bytes()).hexdigest()==spec['md5']
 for f in a.get('files',[]):
  p=Path(f['file']);file_count+=1;files_ok&=p.exists() and sha(p)==f['sha256']
for masks in read(R/'assets/external/quality_v23/ALPHA_MANIFEST.json').values():
 for m in masks:file_count+=1;files_ok&=hashlib.md5(Path(m['file']).read_bytes()).hexdigest()==m['publisher']['md5']
checks['downloaded_models_textures_and_alpha_match_publisher_hashes']=files_ok
checks['sources_are_individually_recorded_cc0_or_cc_by_with_credit']=all(a['license'] in ('CC0-1.0','CC-BY-4.0') and a['source'] and a['authors'] for a in source) and (P/'Content/CampaignData/THIRD_PARTY_ART.md').exists()
checks['all_aliases_and_added_items_resolve_to_imported_meshes']=all(int(x) in ids and y in ids for x,y in art['mesh_aliases'].items()) and all(p['mesh'] in ids for p in art['props'])
checks['no_duplicate_added_ids_and_all_bounds_are_finite_positive']=len({p['id'] for p in art['props']})==len(art['props']) and all(all(math.isfinite(n) for n in p['bounds']+p['world_bounds']+p['at']) and all(p['bounds'][i+3]>p['bounds'][i] for i in range(3)) for p in art['props'])
supported=True;floor=True;plants=True;wall=True;floor_count=0
native_support=read(P/'RawAssets/EchoRealV23/support_planes.json')
rooms={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
door_boxes=[]
for s,r in rooms.values():
 if not r.get('enclosed',True):continue
 for door in [r.get('door')]+[x['at'] for x in r.get('extra_doors',[])]:
  if not door:continue
  x=s['origin'][0]+door[0];y=s['origin'][1]+door[1];z=s['origin'][2];door_boxes.append([x-1.4,y-1.4,z-.05,x+1.4,y+1.4,z+2.65])
def overlap(a,b):return all(a[i]<b[i+3]-.002 and a[i+3]>b[i]+.002 for i in range(3))
for p in art['props']:
 b=p['world_bounds'];cat=p['category']
 if cat=='supported_detail':
  plane=p['support'];x0,y0,x1,y1=plane['rect'];supported&=b[0]>=x0-.002 and b[1]>=y0-.002 and b[3]<=x1+.002 and b[4]<=y1+.002 and abs(b[2]-plane['z']-.009)<.002 and covers(restore(plane,native_support),b)
 elif cat=='floor_belonging':
  floor_count+=1;rect=p['room_rect'];floor&=b[0]>=rect[0]+.17 and b[1]>=rect[1]+.17 and b[3]<=rect[2]-.17 and b[4]<=rect[3]-.17 and abs(b[2]-p['support_z']-.018)<.002 and not any(overlap(b,d) for d in door_boxes)
 elif cat=='lawn_plant':
  lawn=p['lawn_bounds'];plants&=b[0]>=lawn[0] and b[1]>=lawn[1] and b[3]<=lawn[3] and b[4]<=lawn[4] and abs(b[2]-p['support_z'])<.002
 elif cat=='wall_equipment':wall&=not any(overlap(b,d) for d in door_boxes) and b[2]>rooms[p['zone']][0]['origin'][2]+1
checks['12500_new_details_rest_on_measured_support_planes']=supported and sum(p['category']=='supported_detail' for p in art['props'])==12500
assignments=read(OUT/'support_assignments.json');q22=read(P/'Content/CampaignData/echo_quality_v22.json');old={p['id']:p for p in q22['props']}
checks['corrected_legacy_details_use_exposed_triangle_surfaces_not_drawer_interiors']=not art['unresolved_support'] and all(ident in assignments and covers(restore(assignments[ident],native_support),bounds) and abs(bounds[2]-assignments[ident]['z']-.009)<.002 for ident,bounds in art['bounds_overrides'].items())
checks['all_new_floor_belongings_fit_rooms_and_clear_door_envelopes']=floor and floor_count==art['counts']['floor_belonging']
hide_clear=True
for p in art['props']:
 if p['category'] not in ('floor_belonging','wall_equipment'):continue
 for h in d['hideouts']:
  x,y,z=h['at'];w,depth,height=h['size'];hide_clear&=not overlap(p['world_bounds'],[x-.8,y-.8,z-.05,x+w+.8,y+depth+1.3,z+height+.2])
checks['hide_volume_and_player_approach_remain_free_of_new_floor_and_wall_objects']=hide_clear
checks['new_plants_stay_on_lawn_surfaces']=plants
checks['wall_equipment_is_above_floor_and_clear_of_doors']=wall
partitions=[]
for p in d['partitions']:
 a=np.array(p['a']);b=np.array(p['b']);half=p.get('thickness',.12)*.5;lo=np.minimum(a,b)-[half,half,0];hi=np.maximum(a,b)+[half,half,p.get('height',3.8)];partitions.append(list(lo)+list(hi))
partitions=np.array(partitions);clear=True
for p in art['props']:
 if p['category'] not in ('floor_belonging','wall_equipment'):continue
 b=np.array(p['world_bounds']);clear&=not bool((np.all(b[:3]<partitions[:,3:]-.002,axis=1)&np.all(b[3:]>partitions[:,:3]+.002,axis=1)).any())
checks['new_floor_and_wall_objects_do_not_intersect_existing_partitions']=clear
old=read(P/'RawAssets/EchoCustom/manifest.json');checks['all_original_authored_static_glb_files_unchanged']=all(sha(R/f'assets/models/echo_custom/EH_{a["id"]:03}/EH_{a["id"]:03}.glb')==a['source_sha256'] for a in old['assets'])
checks['batched_native_import_is_complete']=read(P/'Saved/Prototype/real_import_v23.json')['success']
checks['native_lod_evidence_exists_for_all_heavy_real_models']=read(P/'Saved/Prototype/real_lods_v23.json')['success'] and all(ids[a['id']].get('lods',0)>=(4 if a['triangles']>1500 else 1) for a in raw['assets'])
report=dict(success=all(checks.values()),checks=checks,counts=art['counts'],verified_source_files=file_count,native_meshes=len(runtime['meshes']),map_sha256=sha(P/'Content/CampaignData/hospital_world.json'),data_hashes={n:sha(P/'Content/CampaignData'/n) for n in ('echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json')},limits=art['limits'])
(OUT/'acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False));assert report['success'],[k for k,v in checks.items() if not v]
