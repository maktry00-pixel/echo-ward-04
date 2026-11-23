"""Check downloaded content, preserved world/registry, and staged runtime data."""
from pathlib import Path
import json,hashlib,datetime
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';C=P/'Content/CampaignData';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_realism_v30';X=R/'assets/external/quality_v30'
read=lambda f:json.loads(f.read_text(encoding='utf-8-sig'));sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest();checks=[]
def check(name,ok):
 checks.append(dict(name=name,passed=bool(ok)))
 if not ok:raise AssertionError(name)
old=read(O/'before_identity.json');before=read(O/'echo_custom_runtime.json.before');now=read(C/'echo_custom_runtime.json');a=read(C/'echo_realism_v30.json');m=read(P/'RawAssets/EchoRealV30/manifest.json')
check('canonical world and route geometry byte-identical',sha(C/'hospital_world.json')==old['map_sha256'])
new_by_id={x['id']:x for x in now['meshes']};check('all preexisting native registry entries preserved',all({q['asset']:q for q in now['meshes']}[x['asset']]==x for x in before['meshes']))
check('seven new unique native entries',len(now['meshes'])==len(before['meshes'])+7 and len({q['asset'] for q in now['meshes']})==len(now['meshes']) and now['version']>=30)
check('native LODs on every new model above 1500 triangles',all(new_by_id[x['id']]['lods']>=4 for x in m['assets'] if x['triangles']>1500))
verified=0
for q in read(X/'SOURCE_MANIFEST.json')['assets']:
 for f in q['files']:
  p=Path(f['file']);b=p.read_bytes();assert hashlib.sha256(b).hexdigest()==f['sha256'] and hashlib.md5(b).hexdigest()==f['md5'];verified+=1
for f in read(X/'ROAD_MANIFEST.json')['files'].values():
 b=Path(f['file']).read_bytes();assert hashlib.sha256(b).hexdigest()==f['sha256'] and hashlib.md5(b).hexdigest()==f['md5'];verified+=1
for f in read(X/'ALPHA_MANIFEST.json')['modular_chainlink_fence']:
 b=Path(f['file']).read_bytes();assert hashlib.sha256(b).hexdigest()==f['sha256'] and hashlib.md5(b).hexdigest()==f['md5'];verified+=1
car=read(X/'graphite_four_door_sedan/provenance.json');check('sedan source GLB retains the verified download hash',sha(X/'graphite_four_door_sedan/sedan.glb')==car['sha256'])
check('publisher license evidence retained','CC0' in (X/'graphite_four_door_sedan/source_page.html').read_text(encoding='utf8'))
check('every Poly Haven source download matches MD5 and SHA256',verified>20)
check('wire transparency has separate source alpha and masked material',any(s['name']=='modular_chainlink_fence_wire' and s['alpha']=='MASK' and 'alpha_file' in s for s in m['materials'].values()))
check('all added objects use the seven verified model IDs',{q['mesh'] for q in a['props']}|set(a['mesh_aliases'].values())==set(range(920,927)))
for f in C.glob('*.json'):assert sha(f)==sha(PK/'Windows/EchoWard/Content/CampaignData'/f.name),f.name
check('staged runtime data match source',True)
original_art=read(O/'echo_real_v23.json.before');current_art=read(C/'echo_real_v23.json');expected_art=read(O/'echo_real_v23.json.before');expected_art['bounds_overrides'].update(a['full_size_parked_cars'])
check('only the six parking-car target bounds changed in prior real-art data',sha(O/'echo_real_v23.json.before')==old['current_data_hashes']['echo_real_v23.json'] and current_art==expected_art and len(a['full_size_parked_cars'])==6 and set(a['full_size_parked_cars'])=={q['id'] for q in read(C/'echo_quality_v22.json')['props'] if q['mesh']==640})
car_dims=[(m['assets'][0]['bounds_cm'][1][i]-m['assets'][0]['bounds_cm'][0][i])*.01 for i in range(3)]
check('parked cars retain full real-world dimensions instead of shrinking to coupe slots',all(max(abs(x-y) for x,y in zip(sorted([b[3]-b[0],b[4]-b[1]]),sorted(car_dims[:2])))<1e-6 and abs(b[5]-b[2]-car_dims[2])<1e-6 for b in a['full_size_parked_cars'].values()))
for name,h in old['current_data_hashes'].items():
 if name not in ['echo_custom_runtime.json','echo_arrival_v29.json','echo_real_v23.json']:assert sha(C/name)==h,name
check('existing story, dressing and menu data preserved',True)
report=dict(success=True,checked_at=datetime.datetime.now().astimezone().isoformat(),checks=checks,count=len(checks),verified_download_files=verified,native_models=len(now['meshes']),new_models=7,new_props=len(a['props']),executable_sha256=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe'),data_sha256=sha(C/'echo_realism_v30.json'))
(O/'acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2));print(json.dumps(report,ensure_ascii=False))
