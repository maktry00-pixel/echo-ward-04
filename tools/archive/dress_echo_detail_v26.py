"""Purposeful architectural dressing; preserve canonical geometry and routes."""
from pathlib import Path
import collections, hashlib, json, math, random
import numpy as np
from hospital_clutter_v10 import Space, bounds, rotate

R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';OUT=R/'artifacts/echo_detail_v26';OUT.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
d=read(P/'Content/CampaignData/hospital_world.json');old=read(P/'Content/CampaignData/echo_real_v23.json');quality=read(P/'Content/CampaignData/echo_quality_v22.json');runtime=read(P/'Content/CampaignData/echo_custom_runtime.json');native={a['id']:a for a in runtime['meshes'] if not a.get('door_leaf')};cat=d['prototype_catalog'];rng=random.Random(26041)
occupied=Space();reserved=Space();props=[];counts=collections.Counter();zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};openings=[]
def world_box(p):
 b=cat[str(p['model'])]['bounds'];pts=np.array([rotate([x,y,z],p['turn']) for x in [b[0],b[3]] for y in [b[1],b[4]] for z in [b[2],b[5]]])+p['at'];return list(pts.min(0))+list(pts.max(0))
for p in d['prototype_props']+d['environment_props']:occupied.add(world_box(p),p['id'])
for p in old['props']:occupied.add(p['world_bounds'],p['id'])
for p in quality['props']:occupied.add(old['bounds_overrides'].get(p['id'],p['bounds']),p['id'])
for layer in ['clutter_dressing','microdetails_dressing','item_types_dressing','campus_plan']:
 for p in d[layer]['items']:occupied.add(p['bounds'],p['id'])
for i,p in enumerate(d['partitions']):
 a=np.array(p['a']);b=np.array(p['b']);h=p.get('thickness',.12)/2;occupied.add(list(np.minimum(a,b)-[h,h,0])+list(np.maximum(a,b)+[h,h,p.get('height',3.8)]),'partition_'+str(i))
for h in d['hideouts']:
 x,y,z=h['at'];w,dep,height=h['size'];reserved.add([x-.8,y-.8,z-.05,x+w+.8,y+dep+1.3,z+height+.2],'hide_'+h['id'])
for zone,(site,room) in zones.items():
 if not room.get('enclosed',True):continue
 ox,oy,z=site['origin']
 for door in [dict(at=room.get('door'),side=room.get('door_side','south'),type=room.get('door_type','ordinary'))]+room.get('extra_doors',[]):
  if not door.get('at'):continue
  x,y=door['at'];a=[x+ox,y+oy,z];side=door['side'];key=tuple(round(v,2) for v in a)
  if any(o['key']==key for o in openings):continue
  entry=dict(key=key,at=a,side=side,zone=zone,type=door.get('type','ordinary'));openings.append(entry);reserved.add(bounds([a[0],a[1],z+1.3],[2.8,2.8,2.7]),'door_'+zone)

def add(mesh,at,size,turn,kind,zone,allow_door=False,extra=None):
 assert mesh in native,mesh
 # These authored wall fittings expose their controls toward local -Y.
 # Match the wall normal rather than showing the flat rear mounting plate.
 if mesh in [82,102,108,136,138,139,140,165,171,172,252]:turn=(turn+2)%4
 size=np.array(size,float);local=[-size[0]/2,-size[1]/2,0,size[0]/2,size[1]/2,size[2]];pts=np.array([rotate([x,y,z],turn) for x in [local[0],local[3]] for y in [local[1],local[4]] for z in [0,size[2]]])+at;bb=[*pts.min(0),*pts.max(0)]
 if occupied.hits(bb) or (not allow_door and reserved.hits(bb)):return None
 p=dict(id=f'DETAIL26_{len(props):05}',mesh=mesh,at=[round(float(v),5) for v in at],bounds=[round(float(v),5) for v in local],turn=turn,world_bounds=[round(float(v),5) for v in bb],category=kind,zone=zone,solid=False)
 if extra:p.update(extra)
 props.append(p);counts[kind]+=1;occupied.add(bb,p['id']);return p

def wall_point(site,room,side,along,height,depth):
 ox,oy,z=site['origin'];x,y,w,h=room['rect'];offset=.12+depth/2
 if side=='north':return [ox+along,oy+y+offset,z+height],0,[0,1]
 if side=='south':return [ox+along,oy+y+h-offset,z+height],2,[0,-1]
 if side=='west':return [ox+x+offset,oy+along,z+height],3,[1,0]
 return [ox+x+w-offset,oy+along,z+height],1,[-1,0]
def on_window(zone,side,along,height,size):
 for w in d['windows']:
  if w['zone']!=zone or w['side']!=side:continue
  axis=0 if side in ['north','south'] else 1;c=w['at'][axis]+w.get('offset',0)
  if abs(along-c)<w['width']/2+size[0]/2+.06 and height<w.get('sill',1.1)+w['height']+.02 and height+size[2]>w.get('sill',1.1)-.02:return True
 return False

for idx,(zone,(site,room)) in enumerate(zones.items()):
 if not room.get('enclosed',True) or zone=='RH00' or site['id'] in ['RF','UT']:continue
 name=(room['name']+' '+room.get('furniture_template','')).lower();x,y,w,h=room['rect'];z=site['origin'][2]
 if any(s in name for s in ['шахт','лестниц','туннел','тоннел']):continue
 clinical=any(s in name for s in ['палат','процед','реаним','лаборат','операц','диагнос'])
 technical=any(s in name for s in ['мастер','генера','щит','инженер','ремонт','технич','склад'])
 specs=[(136,[.082,.03,.086],1.22,'switch'),(139 if clinical else 138,[.328,.03,.086] if clinical else [.082,.03,.086],.88 if clinical else .35,'medical_power' if clinical else 'socket'),(171 if not technical else 172,[.46,.07,.46],2.88,'ventilation'),(140,[.16,.10,.16],2.45,'junction_box')]
 if idx%4==0:specs.append((102,[.48,.045,.61],1.36,'evacuation_frame'))
 if technical and idx%3==0:specs.append((165,[.18,.065,.19],1.48,'pressure_gauge'))
 if idx%11==0:specs.append((252,[.24,.18,.15],1.3,'wall_telephone'))
 for mesh,size,height,kind in specs:
  sides=['north','east','south','west'];rng.shuffle(sides);done=False
  for side in sides:
   low,span=(x,w) if side in ['north','south'] else (y,h)
   for frac in [.17,.83,.36,.64,.50]:
    along=low+span*frac
    if on_window(zone,side,along,height,size):continue
    at,turn,normal=wall_point(site,room,side,along,height,size[1]);p=add(mesh,at,size,turn,kind,zone,extra=dict(wall_side=side,mount_height=height))
    if p:done=True;break
   if done:break

# Hardware is fixed to the frame, above the moving leaf, or flush to the floor.
# No new collision is placed inside a door's swing or the player's approach.
for door in openings:
 x,y,z=door['at'];side=door['side'];normal={'north':[0,1],'south':[0,-1],'west':[1,0],'east':[-1,0]}[side];turn={'north':0,'east':1,'south':2,'west':3}[side]
 add(82,[x+normal[0]*.18,y+normal[1]*.18,z+2.73],[.575,.16,.113],turn,'door_closer',door['zone'],True)
 add(46,[x,y,z+.012],[1.94,.14,.018],turn,'metal_threshold',door['zone'],True)
 for sign in [-1,1]:
  along=[-normal[1]*sign*1.15,normal[0]*sign*1.15]
  add(22,[x+along[0]+normal[0]*.13,y+along[1]+normal[1]*.13,z+.16],[.058,.065,1.95],turn,'corner_protector',door['zone'],True)

for i,win in enumerate(quality['windows']):
 side=win['side'];x,y,z=win['at'];w=win['width'];sill=win['sill'];height=win['height'];normal={'north':[0,1],'south':[0,-1],'west':[1,0],'east':[-1,0]}[side];turn={'north':0,'east':1,'south':2,'west':3}[side];along=[normal[1],-normal[0]]
 at=[x+normal[0]*.17+along[0]*(w/2-.13),y+normal[1]*.17+along[1]*(w/2-.13),z+sill+.34]
 add(108,at,[.10,.06,.16],turn,'window_handle',win.get('zone',''),True)
 if i%5==0:
  add(118,[x+normal[0]*.15,y+normal[1]*.15,z+sill+height-.60],[max(.45,w-.16),.055,.58],turn,'partly_lowered_blind',win.get('zone',''),True)

# Exterior fixtures sit on solid facade above the window head, not on paths.
for site in d['locations']:
 if site['id'] in ['B1','B2','RF','UT','ST'] or site['origin'][2]!=0:continue
 rooms=[r for r in site['rooms'] if r.get('enclosed',True)]
 if not rooms:continue
 room=rooms[0];x,y,w,h=room['rect'];ox,oy,z=site['origin']
 for k,frac in enumerate([.28,.72]):
  add(753,[ox+x+w*frac,oy+y-.46,z+2.62],[.88,.58,.65],2,'exterior_air_conditioner',room['id'],True)

views=[]
for kind in ['medical_power','ventilation','evacuation_frame','door_closer','partly_lowered_blind','exterior_air_conditioner']:
 candidates=[p for p in props if p['category']==kind and p['zone'] in zones]
 if not candidates:continue
 for p in candidates:
  site,room=zones[p['zone']];b=p['world_bounds'];target=[(b[0]+b[3])/2,(b[1]+b[4])/2,(b[2]+b[5])/2];rx,ry,w,h=room['rect'];ox,oy,z=site['origin'];centre=[ox+rx+w/2,oy+ry+h/2]
  delta=np.array(centre)-target[:2];delta/=max(np.linalg.norm(delta),.01);eye=[target[0]+delta[0]*1.9,target[1]+delta[1]*1.9,z+1.65]
  if occupied.hits(bounds(eye,[.25,.25,.25])):continue
  if kind=='exterior_air_conditioner':eye=[target[0],target[1]-3.5,z+1.6]
  for horror in [False,True]:views.append(dict(id='v26_'+kind+('_horror' if horror else '_bright'),at=eye,target=target,horror_mode=horror))
  break
result=dict(version=26,map_sha256=hashlib.sha256((P/'Content/CampaignData/hospital_world.json').read_bytes()).hexdigest(),props=props,captures=views,counts=dict(total=len(props),**counts),limits=['Architectural fittings are decorative and do not add obstacles to existing routes.','Furniture, story objects, original mesh sources and canonical geometry are retained.'])
(P/'Content/CampaignData/echo_detail_v26.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
(OUT/'placement_summary.json').write_text(json.dumps(dict(counts=result['counts'],views=views),ensure_ascii=False,indent=2),encoding='utf8')
print(json.dumps(result['counts'],ensure_ascii=False));print('captures',len(views))
