"""Place a small measured service layer, never change canonical architecture."""
import json,math,hashlib
from pathlib import Path
from collections import defaultdict
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';C=P/'Content/CampaignData';read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
w=read(C/'hospital_world.json');manifest=read(P/'RawAssets/EchoRealV30/manifest.json');models={a['id']:a for a in manifest['assets']};sites={s['id']:s for s in w['locations']};grid=defaultdict(list)
def insert(box):
 for x in range(math.floor(box[0]/4),math.floor(box[3]/4)+1):
  for y in range(math.floor(box[1]/4),math.floor(box[4]/4)+1):grid[x,y].append(box)
def free(b,pad=.25):
 for x in range(math.floor((b[0]-pad)/4),math.floor((b[3]+pad)/4)+1):
  for y in range(math.floor((b[1]-pad)/4),math.floor((b[4]+pad)/4)+1):
   for q in grid[x,y]:
    if b[0]-pad<q[3] and b[3]+pad>q[0] and b[1]-pad<q[4] and b[4]+pad>q[1] and b[2]<q[5]+.02 and b[5]>q[2]+.02:return False
 return True
for key in ['prototype_props','environment_props']:
 for q in w[key]:
  if 'rect' not in q:continue
  x,y,dx,dy=q['rect'];z=q['at'][2];insert([x,y,z,x+dx,y+dy,z+1.7])
for layer in ['echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json','echo_detail_v27.json']:
 for q in read(C/layer).get('props',[]):
  if 'world_bounds' in q:insert(q['world_bounds'])
# Reserve the entire hiding volume and its approach, not only visible furniture.
for h in w['hideouts']:
 x,y,z=h['at'];dx,dy,dz=h['size'];insert([x-.45,y-.2,z-.05,x+dx+.45,y+dy+1.15,z+dz])
for h in w['escape_niches']:
 x,y,dx,dy=h['rect'];z=h.get('z',h.get('entry',[0,0,0])[2]);insert([x-.7,y-.7,z-.05,x+dx+.7,y+dy+.7,z+2.2])
for part in w['partitions']:
 a,b=part['a'],part['b'];pad=part.get('thickness',.12)/2+.12;insert([min(a[0],b[0])-pad,min(a[1],b[1])-pad,a[2],max(a[0],b[0])+pad,max(a[1],b[1])+pad,a[2]+part.get('height',3.8)])
props=[]
def place(mid,at,turn,kind,solid=False,zone=''):
 b=models[mid]['bounds_cm'];local=[*b[0],*b[1]];local=[x*.01 for x in local];x,y,z=at
 pts=[]
 for px in [local[0],local[3]]:
  for py in [local[1],local[4]]:
   t=turn*math.pi/2;pts.append((x+px*math.cos(t)-py*math.sin(t),y+px*math.sin(t)+py*math.cos(t)))
 bb=[min(p[0] for p in pts),min(p[1] for p in pts),z,max(p[0] for p in pts),max(p[1] for p in pts),z+local[5]]
 if solid and not free(bb):return False
 q=dict(id='REAL30_%03d'%len(props),mesh=mid,at=at,turn=turn,bounds=local,world_bounds=bb,category=kind,solid=solid,zone=zone);props.append(q)
 if solid:insert(bb)
 return True
# Carts and bags fit free corners of real service subrooms, clear of the internal doorway.
for sid in ['SV','SV2','DG','DG2','HS']:
 if sid not in sites:continue
 o=sites[sid]['origin'];rooms=[q for q in w['subrooms'] if q['site']==sid and q['rect'][2]>=3.2 and q['rect'][3]>=4.2]
 placed=0
 for q in rooms:
  x,y,dx,dy=q['rect'];mid=924 if placed%3==0 else 921
  for lx,ly in [(x+.85,y+dy-.85),(x+dx-.85,y+dy-.85),(x+.85,y+.85),(x+dx-.85,y+.85)]:
   if place(mid,[o[0]+lx,o[1]+ly,o[2]+.015],placed%4,'maintenance_cart' if mid==924 else 'service_refuse',True,q['zone']):
    props[-1]['bay_rect_world']=[x+o[0],y+o[1],dx,dy];placed+=1;break
  if placed>=6:break
# Small, covered fixtures on the utility hallway walls; placed above head height.
for sid in ['SV','SV2','DG','DG2']:
 if sid not in sites:continue
 o=sites[sid]['origin']
 for ly in [27,32,49,55]:place(922,[o[0]+44.10,o[1]+ly,o[2]+2.68],3,'utility_wall_fixture',False,sid)
# Real industrial pendants keep their authored height, with their hangers at the ceiling.
for sid in ['SV','SV2','DG','DG2']:
 if sid not in sites:continue
 o=sites[sid]['origin'];height=models[923]['bounds_cm'][1][2]*.01
 for ly in [27,55]:place(923,[o[0]+48,o[1]+ly,o[2]+3.70-height],0,'industrial_pendant',False,sid)
# Checkpoint notice and walking/vehicle approach are kept clear.
for x in [59.2,66.8]:place(922,[x,-56.88,2.65],0,'checkpoint_fixture',False,'medical_main')
# Closed maintenance shutters are surface dressing on blank utility wall bays,
# never replacements for interactive door leaves or openings.
for sid in ['SV','SV2']:
 if sid not in sites:continue
 o=sites[sid]['origin'];bay=next(q for q in w['subrooms'] if q['site']==sid and q['rect'][1]<.2 and q['rect'][2]>2.6);x,y,dx,dy=bay['rect'];place(925,[o[0]+x+dx*.5,o[1]+y+.32,o[2]+.015],0,'closed_service_shutter',False,sid);props[-1]['bay_rect_world']=[o[0]+x,o[1]+y,dx,dy]
# One wire storage enclosure in the large diagnostic support hall. Gate gap >=1.4 m.
for i in range(4):place(926,[-131.0+i*.94,60.90,.015],0,'wire_storage_partition',True,'DG25')
captures=[dict(id='v30_sedan_parking',at=[-24,-62,1.25],target=[-32,-54,1.0],horror_mode=True),dict(id='v30_checkpoint_surface',at=[45.5,-66.5,1.55],target=[50,-61,.85],horror_mode=True)]
for kind in ['maintenance_cart','wire_storage_partition','closed_service_shutter','industrial_pendant']:
 q=next((q for q in props if q['category']==kind),None)
 if q:
  if kind=='industrial_pendant':
   eye=[q['at'][0]+.4,q['at'][1]-1.4,q['at'][2]-(3.70-models[923]['bounds_cm'][1][2]*.01)+1.7]
  elif 'bay_rect_world' in q:
   x,y,dx,dy=q['bay_rect_world'];eye=[x+dx*.5,y+dy*.60,q['at'][2]+1.6]
  else:eye=[q['at'][0]+.6,q['at'][1]-2.2,q['at'][2]+1.6]
  captures.append(dict(id='v30_'+kind,at=eye,target=[q['at'][0],q['at'][1],q['at'][2]+(.3 if kind=='industrial_pendant' else 1.1)],horror_mode=kind!='wire_storage_partition'))
counts={k:sum(q['category']==k for q in props) for k in sorted({q['category'] for q in props})}
d=dict(version=30,map_sha256=hashlib.sha256((C/'hospital_world.json').read_bytes()).hexdigest(),mesh_aliases={'640':920,'343':921,'126':923},preserve_proportions={'640':True,'343':True,'126':True},props=props,captures=captures,counts=counts,scope='New sedan, source-based asphalt, service props. Architecture and existing interactive door leaves stay intact.',optimization='Four native LODs above 1500 triangles; size-based draw distance; tiny prop shadow grouping; no resolution reduction.')
# Parking bays have room for the real sedan. Do not miniaturise it to the old coupe bounds.
art=read(C/'echo_real_v23.json');quality=read(C/'echo_quality_v22.json');backup=R/'artifacts/echo_realism_v30/echo_real_v23.json.before'
if not backup.exists():backup.write_bytes((C/'echo_real_v23.json').read_bytes())
car=models[920]['bounds_cm'];dims=[(car[1][i]-car[0][i])*.01 for i in range(3)];car_bounds={}
for q in quality['props']:
 if q['mesh']!=640:continue
 b=q['bounds'];x=(b[0]+b[3])*.5;y=(b[1]+b[4])*.5;dx,dy=(dims[0],dims[1]) if b[3]-b[0]>b[4]-b[1] else (dims[1],dims[0])
 car_bounds[q['id']]=[x-dx*.5,y-dy*.5,b[2],x+dx*.5,y+dy*.5,b[2]+dims[2]]
art['bounds_overrides'].update(car_bounds);(C/'echo_real_v23.json').write_text(json.dumps(art,ensure_ascii=False,separators=(',',':')),encoding='utf8')
d['full_size_parked_cars']=car_bounds
(C/'echo_realism_v30.json').write_text(json.dumps(d,ensure_ascii=False,indent=2),encoding='utf8')
a=read(C/'echo_arrival_v29.json');b=models[920]['bounds_cm'];a['car_size']=[(b[1][i]-b[0][i])*.01 for i in range(3)];a['visual_revision']=30;(C/'echo_arrival_v29.json').write_text(json.dumps(a,ensure_ascii=False,indent=2),encoding='utf8')
print('PLACED',len(props),counts)
