"""Localised decay and emergency markers; retain the canonical map and props."""
from pathlib import Path
import collections,copy,hashlib,json,math,random
from hospital_clutter_v10 import Space,bounds
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_horror_v28';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
scope={'__file__':str(R/'tools/dress_echo_detail_v26.py')}
source=(R/'tools/dress_echo_detail_v26.py').read_text();exec(source.split('def add(mesh,')[0],scope);exec('def wall_point('+source.split('def wall_point(')[1].split('for idx,(zone,')[0],scope)
d=scope['d'];space=scope['occupied'];reserved=scope['reserved'];zones=scope['zones'];rng=random.Random(280419);traces=[];lights=[];counts=collections.Counter()
for n in ['echo_detail_v26.json','echo_detail_v27.json']:
 for p in read(P/'Content/CampaignData'/n)['props']:space.add(p['world_bounds'],p['id'])
placed=Space()
def add(kind,at,size,zone,wall='',yaw=0):
 sx,sy,sz=size;bb=bounds(at,size)
 if space.hits(bb) or reserved.hits(bb) or placed.hits(bb):return False
 traces.append(dict(id=f'HORROR28_{len(traces):04}',kind=kind,at=[round(v,5) for v in at],size=size,world_bounds=bb,zone=zone,wall=wall,yaw=yaw,solid=False));counts[kind]+=1;placed.add(bb,traces[-1]['id']);return True

for idx,(zone,(site,room)) in enumerate(zones.items()):
 if not room.get('enclosed',True) or zone=='RH00' or site['id']=='UT':continue
 name=room['name'].lower();safe=any(t in name for t in ['убежищ','точка отдыха','комната дежурного'])
 if any(t in name for t in ['лестниц','шахт','лифт']):continue
 x,y,w,h=room['rect'];ox,oy,z=site['origin'];technical=any(t in name for t in ['щит','технич','генера','мастер','склад','подвал'])
 clinical=any(t in name for t in ['палат','морг','лаборат','процед','операц','изолят','реаним'])
 corridor=any(t in name for t in ['коридор','холл','вестибюль'])
 if not safe:
  specs=[('Drag',[1.15,.38,.004])]
  if technical or clinical or idx%4==0:specs.append(('Water',[1.35,.76,.004]))
  if clinical and idx%5==0:specs.append(('Blood',[.62,.43,.004]))
  for kind,size in specs:
   candidates=[(.20,.27),(.77,.70),(.38,.60),(.66,.34),(.30,.81),(.82,.23)];rng.shuffle(candidates)
   for fx,fy in candidates:
    px=ox+x+min(w-size[0]/2-.25,max(size[0]/2+.25,w*fx));py=oy+y+min(h-size[1]/2-.25,max(size[1]/2+.25,h*fy))
    if add(kind,[px,py,z+.026],size,zone,yaw=0):break
 # Vertical leaks originate below the wall/ceiling joint. The soft-edged
 # plane lies just inside the painted surface and clear of windows/controls.
 if idx%3==0 or technical:
  for side in ['north','east','south','west']:
   low,span=(x,w) if side in ['north','south'] else (y,h)
   size=[.66,.006,1.55]
   for f in [.13,.86,.35,.65]:
    along=low+span*f
    if scope['on_window'](zone,side,along,1.85,size):continue
    at,turn,_=scope['wall_point'](site,room,side,along,1.85,.006);at[2]+=.775
    actual=[.66,.006,1.55] if side in ['north','south'] else [.006,.66,1.55]
    if add('Damp',at,actual,zone,wall=side):break
   else:continue
   break
 if clinical and idx%9==0 and not safe:
  side='north';along=x+w*.70;size=[.36,.006,.48]
  if not scope['on_window'](zone,side,along,1.05,size):
   at,_,_=scope['wall_point'](site,room,side,along,1.05,.006);at[2]+=.24;add('Hand',at,size,zone,wall=side)

# One additional, caged emergency marker in an occupied corridor/hall per site.
seen=set()
for zone,(site,room) in zones.items():
 if site['id'] in seen or site['id'] in ['UT','RF','ST'] or not room.get('enclosed',True):continue
 if not any(t in room['name'].lower() for t in ['коридор','холл','вестибюль']):continue
 x,y,w,h=room['rect'];ox,oy,z=site['origin'];side='east';along=y+h*.76;at,turn,normal=scope['wall_point'](site,room,side,along,2.48,.12);bb=bounds([at[0],at[1],at[2]+.15],[.14,.28,.30])
 if space.hits(bb) or scope['on_window'](zone,side,along,2.48,[.28,.12,.30]):continue
 lights.append(dict(zone=zone,site=site['id'],at=at,turn=(turn+2)%4,anchor=[at[0]+normal[0]*.22,at[1]+normal[1]*.22,at[2]+.14],bounds=[-.14,-.06,0,.14,.06,.30]));seen.add(site['id'])

profiles=copy.deepcopy(d['horror_atmosphere']['profiles'])
for key,p in profiles.items():p['white_lumens']=round(p['white_lumens']*.89);p['floor_mist']=round(p['floor_mist']*.84,3)
views=[]
for n,ident in [('echo_real_v23.json','v23_lab_clutter'),('echo_real_v23.json','v23_detail_126'),('echo_real_v23.json','v23_court_S01'),('echo_detail_v27.json','v27_replacement_200'),('echo_detail_v27.json','v27_replacement_230'),('echo_detail_v27.json','v27_fire_hose_cabinet_bright')]:
 for v in read(P/'Content/CampaignData'/n)['captures']:
  if v['id']!=ident:continue
  for dark in [True,False]:
   c=copy.deepcopy(v);c['id']='v28_'+ident+('_horror' if dark else '_bright');c['horror_mode']=dark;views.append(c)
  break
# Document a decay patch from a clear eye at room centre; use horizontal views
# only when the line does not cross furniture or a partition.
for kind in ['Blood','Damp','Water','Hand']:
 for t in traces:
  if t['kind']!=kind:continue
  site,room=zones[t['zone']];x,y,w,h=room['rect'];ox,oy,z=site['origin'];target=t['at'];eye=[ox+x+w*.5,oy+y+h*.5,z+1.65]
  if math.dist(eye,target)<.7 or math.dist(eye,target)>5:continue
  if space.hits(bounds(eye,[.25,.25,.25])):continue
  if any(space.hits(bounds([eye[j]*(1-f)+target[j]*f for j in range(3)],[.05,.05,.05])) for f in [.1,.25,.4,.55,.7,.85]):continue
  for dark in [True,False]:views.append(dict(id='v28_trace_'+kind+('_horror' if dark else '_bright'),at=eye,target=target,horror_mode=dark))
  break
out=dict(version=28,map_sha256=hashlib.sha256((P/'Content/CampaignData/hospital_world.json').read_bytes()).hexdigest(),traces=traces,emergency_markers=lights,profiles=profiles,captures=views,counts=dict(traces=len(traces),markers=len(lights),**counts))
(P/'Content/CampaignData/echo_horror_v28.json').write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf8');(O/'placement_summary.json').write_text(json.dumps(out['counts'],ensure_ascii=False,indent=2),encoding='utf8');print(out['counts']);print('views',len(views))
