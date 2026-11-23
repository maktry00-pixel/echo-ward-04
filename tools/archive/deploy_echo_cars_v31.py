"""Deploy every supplied vehicle exactly once or more, with real metric envelopes."""
from pathlib import Path
import json,math,shutil
R=Path(__file__).resolve().parents[1];C=R/'unreal/EchoWard/Content/CampaignData';D=json.loads((R/'unreal/EchoWard/RawAssets/EchoCarsV31/manifest.json').read_text());world=json.loads((C/'hospital_world.json').read_text(encoding='utf8'));models={v['slug']:v for v in D['vehicles']};assert len(models)==32
P=[]
def put(slug,at,yaw=90,replaces='',role='passenger'):
 P.append(dict(id='CAR31_'+str(len(P)+1).zfill(3),slug=slug,at=at,yaw=yaw,replaces=replaces,role=role))
props=[p for p in world['prototype_props']+world['environment_props'] if p.get('model') in [135,136,137,138]]
for p in props:
 if p['model']==135:put('shvan-92-ambulance-low-poly-model',p['at'][:2]+[.03],90+p.get('turn',0)*90,p['id'],'ambulance')
industrial={'PROP6_S07_138-01':('gmc-savana-2003',[109,-12,.03],180),'PROP6_S09_138-01':('tata-signa-cargo-truck-low-poly-game-ready-pbr',[116,-94,.03],90),'PROP6_S12_136-01':('volkswagen-crafter',[127.7,-170,.03],90)}
for k,(slug,at,yaw) in industrial.items():put(slug,at,yaw,k,'service')
put('auto-mezzanine-deck-trailer',[137,-94,.03],90,role='cargo_trailer');put('gls-van',[112,-90,.03],90,role='courier');put('iveco-daily-van-2014',[137,-170,.03],90,role='service')
hero='mercedes-e-class-w212';used={p['slug'] for p in P};parked=[s for s in models if s not in used and s!=hero]
replacements=[p['id'] for p in props if p['model']==137]+[p['id'] for p in json.loads((C/'echo_quality_v22.json').read_text())['props'] if p['id'].startswith('CAMP14_sedan_')]
assert len(replacements)==12
for i,s in enumerate(parked):
 at=[round(-57+4.8*(i//2),2),-66 if i%2==0 else -55,.03];put(s,at,90 if i%2==0 else -90,replacements[i] if i<len(replacements) else '', 'police' if 'police' in s else 'pickup' if 'amarok' in s else 'passenger')
assert len(P)==35,(len(P),len(parked));assert {p['slug'] for p in P}|{hero}==set(models)
views=[]
for slug in models:
 if slug==hero:continue
 p=next(p for p in P if p['slug']==slug);v=models[slug];lo,hi=v['bounds_cm'];length=(hi[0]-lo[0])/100;width=(hi[1]-lo[1])/100;height=(hi[2]-lo[2])/100;a=math.radians(p['yaw']);forward=(math.cos(a),math.sin(a));side=(-forward[1],forward[0]);distance=max(4,length*.72)
 if slug=='tata-signa-cargo-truck-low-poly-game-ready-pbr':side=(-side[0],-side[1])
 eye=[p['at'][0]+forward[0]*distance+side[0]*max(2.2,width),p['at'][1]+forward[1]*distance+side[1]*max(2.2,width),max(2.3,height*1.15)];views.append(dict(id='v31_'+slug,at=eye,target=p['at'][:2]+[height*.45],horror_mode=False))
next(v for v in views if v['id']=='v31_gmc-savana-2003')['at']=[113,-7,2.4]
first=next(p for p in P if p['role']=='passenger');views.append(dict(id='v31_parking_horror',at=[-49,-62,2.5],target=[-42,-70,1.1],horror_mode=True))
modeldefs={s:{k:v[k] for k in ['slug','title','body','glass','bounds_cm','point_cloud_reconstruction']} for s,v in models.items()}
for slug,v in models.items():modeldefs[slug]['body_extra']=v.get('body_extra',[])
out=dict(version=31,native_meshes=len(D['assets']),source='All 32 user-provided archives in assets/models/cars; originals preserved.',units='m except explicitly named cm fields',vehicles=modeldefs,hero=models[hero],placements=P,captures=views,controls={'F':'Enter/exit driver seat or toggle nearest other panel','Y':'Toggle nearest door, hood or trunk','mouse':'Look around while seated'},notes=['GLS source was RGB points, not a mesh; reconstructed outer surface retains supplied colors.','Static vehicles retain original configurations; only hero W212 parts are interactive.','A vehicle interior viewpoint is used; no seated human animation or free driving is supplied.'])
(C/'echo_cars_v31.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));(R/'artifacts/echo_cars_v31/placements.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));a=C/'echo_arrival_v29.json';backup=R/'artifacts/echo_cars_v31/arrival_before.json'
if not backup.exists():shutil.copy2(a,backup)
x=json.loads(a.read_text());x['car_model']=2000;x['car_size']=[(b-a)/100 for a,b in zip(*models[hero]['bounds_cm'])];x['exit']=[52.5,-60.65,.98];x['route'][0]=[52.5,-60.65,.98];x['route'][1]=[52,-57,.98];x['hero_vehicle_revision']=31;a.write_text(json.dumps(x,ensure_ascii=False,indent=2));print('Vehicles:',len(P)+1,'unique archives:',len(models),'hero parts:',len(models[hero]['hero_parts']),'captures:',len(views))
