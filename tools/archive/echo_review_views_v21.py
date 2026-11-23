"""Select actual placed objects and unobstructed room cameras; do not edit map."""
import json,math
from pathlib import Path
from hospital_clutter_v10 import Space,bounds,rotate
from hospital_furnishing_v6 import Index
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard/Content/CampaignData'
d=json.loads((P/'hospital_world.json').read_text());b=json.loads((P/'echo_custom_bindings.json').read_text());props=d['prototype_props']+d['environment_props'];zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};cache={};views=[]
walls=Index()
for w in json.loads((R/'artifacts/unreal_hospital_v3/raster_walls.json').read_text()):walls.add(w['z'],w['rect'])
def camera(p):
 zid=p['zone']
 if zid not in zones:return
 s,r=zones[zid];z=p['at'][2]
 if zid not in cache:
  space=Space()
  for q in props:
   if q['zone']!=zid:continue
   for a in d['prototype_catalog'][str(q['model'])]['parts']:
    off=rotate(a[:3],q['turn']);sz=list(a[3:6])
    if q['turn']%2:sz[0],sz[1]=sz[1],sz[0]
    space.add(bounds([q['at'][j]+off[j] for j in range(3)],sz),q['id'])
  for q in d['environment_dressing']['boxes']:
   if q['zone']==zid:space.add(bounds(q['at'],q['size']),q.get('item_id','env'))
  cache[zid]=space
 space=cache[zid];bb=d['prototype_catalog'][str(p['model'])]['bounds'];target=[p['at'][0],p['at'][1],z+(bb[2]+bb[5])/2]
 for dist in [1.5,2,2.6,3.2,4,5]:
  for ang in [90,65,115,45,135,0,180,225,270,315]:
   a=math.radians(ang);off=rotate([dist*math.cos(a),dist*math.sin(a),0],p['turn']);eye=[p['at'][0]+off[0],p['at'][1]+off[1],z+min(1.8,max(1,target[2]-z+.5))];ox,oy,_=s['origin'];x,y,w,h=r['rect']
   if not(ox+x+.1<eye[0]<ox+x+w-.1 and oy+y+.1<eye[1]<oy+y+h-.1):continue
   if space.hits(bounds(eye,[.2]*3)) or walls.hits(z,[eye[0]-.1,eye[1]-.1,.2,.2]):continue
   ray=[[eye[j]+(target[j]-eye[j])*t/40 for j in range(3)] for t in range(1,40)]
   if any(space.hits(bounds(a,[.03]*3),p['id']) or walls.hits(z,[a[0]-.015,a[1]-.015,.03,.03]) for a in ray):continue
   return eye,target
for model in [28,126,86,128,50,47,48,185,186,187,77,96,49,55,124,152,81,113]:
 for p in props:
  if p['model']!=model:continue
  result=camera(p)
  if result:
   eye,target=result;views.append(dict(id=f'v21_detail_{model}',at=eye,target=target,detail_item=p['id'],native_model=b['prototype_models'][str(model)]));break
b['review_captures']=views;(P/'echo_custom_bindings.json').write_text(json.dumps(b,ensure_ascii=False,indent=2),encoding='utf8');print('Unobstructed detail views:',len(views))
