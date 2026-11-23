"""Add themed supported belongings and services; leave all old layers untouched."""
from pathlib import Path
import collections,hashlib,json,math,random
import numpy as np
from hospital_clutter_v10 import Space,rotate,bounds
from echo_support_coverage_v23 import covers
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_detail_v27';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
scope={'__file__':str(R/'tools/dress_echo_detail_v26.py')}
exec((R/'tools/dress_echo_detail_v26.py').read_text().split('def add(mesh,')[0],scope)
exec('def wall_point('+(R/'tools/dress_echo_detail_v26.py').read_text().split('def wall_point(')[1].split('for idx,(zone,')[0],scope)
d=scope['d'];occupied=scope['occupied'];reserved=scope['reserved'];zones=scope['zones'];cat=scope['cat'];world_box=scope['world_box']
old26=read(P/'Content/CampaignData/echo_detail_v26.json')
for p in old26['props']:occupied.add(p['world_bounds'],p['id'])
for i,p in enumerate(d['environment_dressing']['boxes']):
 if p.get('solid'):occupied.add(bounds(p['at'],p['size']),'environment_'+str(i))
raw=read(P/'RawAssets/EchoDetailV27/manifest.json');new={a['id']:a for a in raw['assets']};native={a['id']:a for a in read(P/'Content/CampaignData/echo_custom_runtime.json')['meshes'] if not a.get('door_leaf')};native.update(new)
supportscope={'__file__':str(R/'tools/dress_echo_real_v23.py')}
exec((R/'tools/dress_echo_real_v23.py').read_text().split('space=Space();itemspace=Space();world=Space()')[0],supportscope)
props=[];counts=collections.Counter();rng=random.Random(27052);aliases={'186':902,'196':901,'199':907,'200':900,'201':904,'203':903,'226':905,'227':913,'230':906,'231':914}

def add(mesh,at,turn,kind,zone,size=None,ignore='',support=None,**extra):
 if size is None:
  bb=np.array(native[mesh]['bounds_cm'],float).reshape(2,3)/100;size=bb[1]-bb[0]
 size=np.asarray(size,float);local=[-size[0]/2,-size[1]/2,0,size[0]/2,size[1]/2,size[2]]
 pts=np.array([rotate([x,y,z],turn) for x in [local[0],local[3]] for y in [local[1],local[4]] for z in [0,local[5]]])+at;wb=[*pts.min(0),*pts.max(0)]
 if occupied.hits(wb,ignore) or reserved.hits(wb):return None
 if support is not None and not covers(support,wb):return None
 p=dict(id=f'DETAIL27_{len(props):05}',mesh=mesh,at=[round(float(v),6) for v in at],bounds=[round(float(v),6) for v in local],world_bounds=[round(float(v),6) for v in wb],turn=turn,category=kind,zone=zone,solid=False,**extra)
 if support is not None:p['support']={k:v for k,v in support.items() if k!='triangles'}
 props.append(p);counts[kind]+=1;occupied.add(wb,p['id']);return p

def role(room):
 name=(room['name']+' '+room.get('furniture_template','')).lower()
 if any(x in name for x in ['кух','буфет','кафе','супермар','магаз','пищ']):return 'food'
 if any(x in name for x in ['склад','мастер','генера','щит','ремонт','технич','гараж','инженер']):return 'technical'
 if any(x in name for x in ['убор','белье','прач','сануз','душ']):return 'cleaning'
 if any(x in name for x in ['палат','процед','анализ','реаним','операц','диагнос','лаборат','пост мед','медсестр']):return 'clinical'
 return 'office'

# Every new small object uses an exposed surface from actual imported triangles.
# No additional floor clutter is placed in route or hide envelopes.
furniture=d['prototype_props']+d['environment_props']
for parent in furniture:
 if parent['mount']!='floor' or parent['model'] not in (4,13,14,15,16,17,18,19,20,21,22,23,24,37,38,89,132):continue
 if parent['zone'] not in zones:continue
 site,room=zones[parent['zone']];kind=role(room)
 pool={'clinical':[908,908,912,763,764,780,903,904],'technical':[788,789,790,792,798,800,801],'cleaning':[769,770,772,912],'food':[781,782,783,786,867,868],'office':[911,765,766,768,777,864,870]}[kind]
 for surface in supportscope['support'](parent):
  x0,y0,x1,y1=surface['rect'];higher=[p['z'] for p in supportscope['planes'].get(str(surface.get('source_mesh')),[]) if p['z']>supportscope['planes'][str(surface['source_mesh'])][surface['source_plane']]['z']+.035] if 'source_plane' in surface else []
  if higher:surface['ceiling_z']=surface['to_world'][2][2]*min(higher)+surface['to_world'][2][3]
  points=[(x,y) for x in np.linspace(x0+.07,x1-.07,8) for y in np.linspace(y0+.07,y1-.07,5)];rng.shuffle(points);placed=0
  for x,y in points:
   for mesh in rng.sample(pool,min(len(pool),4)):
    size=(np.array(native[mesh]['bounds_cm'])[1]-np.array(native[mesh]['bounds_cm'])[0])/100
    if size[2]>.45:continue
    turn=rng.randrange(4);w,h=(size[1],size[0]) if turn%2 else (size[0],size[1])
    if not x0+w/2<x<x1-w/2 or not y0+h/2<y<y1-h/2:continue
    p=add(mesh,[x,y,surface['z']+.01],turn,'supported_'+kind,parent['zone'],ignore=parent['id'],support=surface,parent=parent['id'])
    if p:placed+=1;break
   if placed>=3:break

for zone,(site,room) in zones.items():
 if not room.get('enclosed',True) or zone=='RH00' or site['id'] in ['UT','RF']:continue
 x,y,w,h=room['rect'];ox,oy,z=site['origin'];kind=role(room);name=room['name'].lower()
 if any(t in name for t in ['шахт','лестниц','лифт']):continue
 # IV stands and cylinders occupy unused room-edge pockets, outside all door,
 # hiding and existing furniture envelopes. They retain their metric sizes.
 if kind=='clinical':
  for mesh in [901,902]:
   size=(np.array(native[mesh]['bounds_cm'])[1]-np.array(native[mesh]['bounds_cm'])[0])/100;done=False
   for fx,fy in [(.08,.12),(.92,.88),(.08,.78),(.92,.22)]:
    px=min(ox+x+w-.18-size[0]/2,max(ox+x+.18+size[0]/2,ox+x+w*fx));py=min(oy+y+h-.18-size[1]/2,max(oy+y+.18+size[1]/2,oy+y+h*fy))
    if add(mesh,[px,py,z+.018],0,'clinical_floor_equipment',zone,support_z=z+.018):done=True;break
 # Suspended engineering runs sit above the normal walking and door volume.
 if kind in ['technical','clinical','cleaning'] or any(t in name for t in ['коридор','холл']):
  turn=1 if w>h else 0;length=min(2.9,max(w,h)-.7)
  if length>1.4:
   for f in [.24,.73]:
    at=[ox+x+w*f,oy+y+h*.5,z+3.24] if w>h else [ox+x+w*.5,oy+y+h*f,z+3.24]
    add(909,at,turn,'ceiling_services',zone,size=[.45,length,.49],support_ceiling=z+3.8)
 # Wall-mounted sanitary stations are visible by doors without occupying them.
 specs=[]
 if kind in ['clinical','cleaning']:specs.append((913,[.115,.20,.285],1.10,'hygiene_station'))
 if 'коридор' in name or 'холл' in name or 'вестибюль' in name:specs.append((910,[.58,.19,.70],1.05,'fire_hose_cabinet'))
 for mesh,size,height,category in specs:
  sides=['north','east','south','west'];rng.shuffle(sides);done=False
  for side in sides:
   low,span=(x,w) if side in ['north','south'] else (y,h)
   for f in [.22,.78,.43,.61]:
    along=low+span*f
    if scope['on_window'](zone,side,along,height,size):continue
    at,turn,_=scope['wall_point'](site,room,side,along,height,size[1]);p=add(mesh,at,turn,category,zone,size=size,wall_side=side,mount_height=height)
    if p:done=True;break
   if done:break

# Leaf/litter groups are confined to the existing pavement shoulder, not lawns
# or lanes. Exact old surface heights remain authoritative.
for i,surface in enumerate(d['environment_dressing']['boxes']):
 if surface['kind'] not in ['pavement','sidewalk']:continue
 at=surface['at'];sz=surface['size']
 if sz[0]<1.4 or sz[1]<1.4:continue
 for f in [.18,.48,.81]:
  x=at[0]+(f-.5)*(sz[0]-.8);y=at[1]+(rng.choice([-.5,.5]))*(sz[1]-.9);z=at[2]+sz[2]/2+.006
  add(645,[x,y,z],rng.randrange(4),'pavement_leaves','ST',size=[.6,.35,.025],ignore='environment_'+str(i),support_z=z)

views=[]
for category in ['supported_clinical','supported_office','ceiling_services','fire_hose_cabinet','hygiene_station','clinical_floor_equipment']:
 for p in [p for p in props if p['category']==category]:
  site,room=zones[p['zone']];b=p['world_bounds'];target=[(b[i]+b[i+3])/2 for i in range(3)];x,y,w,h=room['rect'];ox,oy,z=site['origin'];direction=np.array([ox+x+w/2,oy+y+h/2])-target[:2];direction/=max(np.linalg.norm(direction),.01)
  eye=[target[0]+direction[0]*1.6,target[1]+direction[1]*1.6,z+1.65]
  if not ox+x+.2<eye[0]<ox+x+w-.2 or not oy+y+.2<eye[1]<oy+y+h-.2 or occupied.hits(bounds(eye,[.3,.3,.3])):continue
  for horror in [False,True]:views.append(dict(id='v27_'+category+('_horror' if horror else '_bright'),at=eye,target=target,horror_mode=horror))
  break
# Pick installed original monitors/oxygen/etc for honest close-up review.
binding=read(P/'Content/CampaignData/echo_custom_bindings.json')
for old in [200,196,186,230,199,227,226]:
 for p in furniture:
  if binding['prototype_models'].get(str(p['model']))!=old or p['zone'] not in zones:continue
  s,r=zones[p['zone']];b=world_box(p);target=[(b[i]+b[i+3])/2 for i in range(3)];x,y,w,h=r['rect'];ox,oy,z=s['origin'];c=np.array([ox+x+w/2,oy+y+h/2]);delta=c-target[:2];delta/=max(np.linalg.norm(delta),.01);eye=[target[0]+delta[0]*1.6,target[1]+delta[1]*1.6,z+1.55]
  if not ox+x+.2<eye[0]<ox+x+w-.2 or not oy+y+.2<eye[1]<oy+y+h-.2 or occupied.hits(bounds(eye,[.28,.28,.28])):continue
  if any(occupied.hits(bounds(np.array(eye)*(1-t)+np.array(target)*t,[.05,.05,.05]),p['id']) for t in np.linspace(.05,.85,12)):continue
  views.append(dict(id=f'v27_replacement_{old}',at=eye,target=target));break
for mesh in [902,903,904,908,911,912]:
 found=False
 for p in [p for p in props if p['mesh']==mesh]:
  site,room=zones[p['zone']];x,y,w,h=room['rect'];ox,oy,z=site['origin'];b=p['world_bounds'];target=np.array([(b[i]+b[i+3])/2 for i in range(3)])
  for dx,dy in [(0,1.1),(1.1,0),(0,-1.1),(-1.1,0),(.85,.85),(-.85,-.85)]:
   eye=np.array([target[0]+dx,target[1]+dy,max(z+1.35,target[2]+.4)])
   if not ox+x+.2<eye[0]<ox+x+w-.2 or not oy+y+.2<eye[1]<oy+y+h-.2 or occupied.hits(bounds(eye,[.2,.2,.2])):continue
   if any(occupied.hits(bounds(eye*(1-t)+target*t,[.025,.025,.025]),p.get('parent',p['id'])) for t in np.linspace(.05,.7,12)):continue
   views.append(dict(id=f'v27_model_{mesh}',at=eye.tolist(),target=target.tolist()));found=True;break
  if found:break
result=dict(version=27,map_sha256=hashlib.sha256((P/'Content/CampaignData/hospital_world.json').read_bytes()).hexdigest(),mesh_aliases=aliases,props=props,captures=views,counts=dict(total=len(props),**counts),limits=['Decorative, non-interactive supply objects; canonical routes and old collisions retained.','Fourteen original authored models; opaque IV bag material approximates plastic rather than fluid simulation.'])
(P/'Content/CampaignData/echo_detail_v27.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8');(O/'placement_summary.json').write_text(json.dumps(dict(counts=result['counts'],views=views),ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(result['counts']));print('captures',len(views))
