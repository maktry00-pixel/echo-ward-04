"""Independent checks of the additional detail layer and retained game data."""
from pathlib import Path
import collections,hashlib,json,math
from hospital_clutter_v10 import Space,rotate,bounds
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_detail_v26'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
v=read(O/'before_identity.json');d=read(P/'Content/CampaignData/hospital_world.json');a=read(P/'Content/CampaignData/echo_detail_v26.json');native={p['id'] for p in read(P/'Content/CampaignData/echo_custom_runtime.json')['meshes']};checks={}
checks['canonical_geometry_and_old_art_unchanged']=all(sha(P/'Content/CampaignData'/name)==h for name,h in v['unchanged_data_hashes'].items()) and a['map_sha256']==v['map_sha256']
checks['all_new_ids_unique_and_meshes_native']=len({p['id'] for p in a['props']})==len(a['props']) and all(p['mesh'] in native for p in a['props'])
checks['all_detail_bounds_finite_positive']=all(all(math.isfinite(x) for x in p['at']+p['bounds']+p['world_bounds']) and all(p['bounds'][i+3]>p['bounds'][i] for i in range(3)) for p in a['props'])
sp=Space();overlaps=[]
for p in a['props']:
 b=p['world_bounds']
 if sp.hits(b):overlaps.append(p['id'])
 sp.add(b,p['id'])
checks['new_details_do_not_overlap_each_other']=not overlaps
checks['new_decorative_fittings_do_not_add_route_colliders']=all(not p['solid'] for p in a['props'])
counts=collections.Counter(p['category'] for p in a['props']);checks['all_detail_categories_accounted_for']=a['counts']==dict(total=len(a['props']),**counts) and len(a['props'])>5000
checks['wall_fixture_mounts_are_below_ceiling']=all(0<=p.get('mount_height',0) and p.get('mount_height',0)+p['bounds'][5]<3.8 for p in a['props'] if 'mount_height' in p)
checks['door_closers_are_above_the_opening']=all(p['bounds'][5]<=.12 and abs((p['at'][2]+9.6)%4.8-2.73)<.002 for p in a['props'] if p['category']=='door_closer')
checks['material_assets_and_high_resolution_sources_exist']=all((P/'Content/Campaign/DetailV26'/('MI_'+name+'.uasset')).exists() for name in ['Plaster','Paint','Floor','Facade']) and read(P/'Saved/Prototype/detail_v26_materials.json')['success']
checks['v25_light_and_lift_optimizations_retained']='if(LightSelectionClock<.24f)return' in (P/'Source/EchoWard/WardBookWorld.cpp').read_text(encoding='utf-8-sig') and 'if(LiftDisplay->Text.ToString()!=Caption)' in (P/'Source/EchoWard/WardHospitalDetails.cpp').read_text(encoding='utf-8-sig')
report=dict(success=all(checks.values()),checks=checks,counts=a['counts'],overlaps=overlaps,map_sha256=a['map_sha256'],data_sha256=sha(P/'Content/CampaignData/echo_detail_v26.json'),scope='Independent placement/data audit; native fitting, movement and door clearance also require runtime checks.')
(O/'acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False));assert report['success'],[k for k,v in checks.items() if not v]
