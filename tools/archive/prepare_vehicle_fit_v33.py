from pathlib import Path
import json,shutil,collections
R=Path('D:/Projects/ECHO');U=R/'unreal/EchoWard';D=R/'artifacts/echo_vehicle_fit_v33';C=U/'Content/CampaignData'
for rel in ['Source/EchoWard/WardCars31.cpp','Source/EchoWard/WardBookWorld.cpp','Source/EchoWard/WardBookWorld.h','Source/EchoWard/WardHospitalDetails.cpp','Source/EchoWard/WardHospitalEnvironment.cpp','Source/EchoWard/WardBookWorldAssessment.cpp','Content/CampaignData/echo_cars_v31.json','Content/CampaignData/echo_arrival_v29.json','Config/DefaultEngine.ini','Content/Python/upgrade_car_quality_v32.py']:
 shutil.copy2(U/rel,D/'source_before'/Path(rel).name)
shutil.copy2(U/'Saved/PackagedWindows/package_verified.json',D/'before_package_verified.json')
w=json.loads((C/'hospital_world.json').read_text());d=json.loads((C/'echo_cars_v31.json').read_text());rooms={r['id']:([l['origin'][0]+r['rect'][0],l['origin'][1]+r['rect'][1],r['rect'][2],r['rect'][3]],r,l) for l in w['locations'] for r in l['rooms']};props={p['id']:p for p in w['prototype_props']+w['environment_props']};i=0
for p in d['placements']:
 role=p['role'];zone='S01' if role in ['passenger','pickup','police'] else props[p['replaces']]['zone'] if p.get('replaces') in props else 'S09' if role in ['cargo_trailer','courier'] else 'S12'
 if zone=='S01':p['at']=[round(-58+4.8*(i//2),2),-73 if i%2==0 else -61,0];i+=1
 elif zone=='S09':p['at']=[116 if 'tata' in p['slug'] else 135,-98,0]
 if p['slug']=='gls-van':p['at']=[125.5,-98,0]
 p['region33']=rooms[zone][0];p['zone33']=zone;p['garage33']=rooms[zone][1].get('enclosed',False);p['at'][2]=0
 p['preferred_yaw33']=p['yaw']
# Classify visible source cabin structure from mesh and material names; keep full hero glass.
manifest=json.loads((U/'RawAssets/EchoCarsV31/manifest.json').read_text());by={a['id']:a for a in manifest['assets']};decisions={}
for slug,v in d['vehicles'].items():
 scene=json.loads((R/'artifacts/echo_cars_v31'/(slug+'_scene.json')).read_text());texts=[]
 for ob in scene.get('objects',[]):
  if ob.get('type')=='MESH':texts.append(ob['name'].lower())
 keys={k for mid in [v['body'],v['glass']]+v.get('body_extra',[]) if mid for k in by[mid]['materials']};texts += [manifest['materials'][k]['name'].lower() for k in keys]
 categories={n:any(t in s for s in texts for t in tokens) for n,tokens in {'seats':['seat','salon','interior','int_'],'dashboard':['dashboard','dash','inside_panel','console','cockpit'],'steering':['steering','steer','volant']}.items()}
 detailed=sum(categories.values())>=2;v['tint33']=not detailed;v['interior_evidence33']=categories;decisions[slug]=dict(tinted=not detailed,evidence=categories)
d['revision']=33;d['surface_fit33']=True;d['glass_policy33']='Opaque black exterior glazing for incomplete cabins; hero retains transparent original glazing.';d['quality_cap33']=4096
# Refresh views from the final native placement during capture, using the same local offset.
for capture in d['captures']:
 slug=capture['id'].removeprefix('v31_');p=next((p for p in d['placements'] if p['slug']==slug),None)
 if not p:continue
 lo,hi=d['vehicles'][slug]['bounds_cm'];L=(hi[0]-lo[0])/100;W=(hi[1]-lo[1])/100;H=(hi[2]-lo[2])/100
 import math
 a=math.radians(p['yaw']);f=[math.cos(a),math.sin(a)];s=[-f[1],f[0]]
 capture['at']=[p['at'][0]+f[0]*max(4,L*.72)+s[0]*max(2.2,W),p['at'][1]+f[1]*max(4,L*.72)+s[1]*max(2.2,W),max(2.3,H*1.15)];capture['target']=p['at'][:2]+[H*.45];capture['car33_id']=p['id'];capture['car33_initial']=p['at']
(C/'echo_cars_v31.json').write_text(json.dumps(d,ensure_ascii=False,indent=2));(D/'placements_planned.json').write_text(json.dumps(d,ensure_ascii=False,indent=2));(D/'interior_decisions.json').write_text(json.dumps(decisions,ensure_ascii=False,indent=2));print('planned cars',len(d['placements']),'tint',sum(v['tinted'] for v in decisions.values()))
# All existing vehicle textures may retain source detail up to 4096; no synthetic resizing.
p=U/'Content/Python/upgrade_car_quality_v32.py';s=p.read_text(encoding='utf-8');s=s.replace("echo_car_quality_v32';src", "echo_vehicle_fit_v33';src").replace("(report/'source_textures.json')", "(R/'artifacts/echo_car_quality_v32/source_textures.json')").replace("limit=4096 if row['hero'] else 2048",'limit=4096').replace("if row['hero'] and row['kind']=='color':", "if row['kind']=='color':").replace('revision=32,hero_budget=4096,parked_budget=2048','revision=33,hero_budget=4096,parked_budget=4096').replace('CAR_QUALITY32_PASS','CAR_QUALITY33_PASS').replace('quality32_failed.txt','quality33_failed.txt');(U/'Content/Python/upgrade_vehicle_quality_v33.py').write_text(s,encoding='utf-8')
