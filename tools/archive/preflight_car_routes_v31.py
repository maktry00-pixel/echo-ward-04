from pathlib import Path
import json,math
R=Path('D:/Projects/ECHO');d=json.loads((R/'unreal/EchoWard/Content/CampaignData/echo_cars_v31.json').read_text());w=json.loads((R/'unreal/EchoWard/Content/CampaignData/hospital_world.json').read_text());boxes=[]
for p in d['placements']:
 v=d['vehicles'][p['slug']];lo,hi=v['bounds_cm'];a=math.radians(p['yaw']);cs,sn=round(math.cos(a)),round(math.sin(a));corners=[(p['at'][0]+(x*cs-y*sn)/100,p['at'][1]+(x*sn+y*cs)/100,p['at'][2]+z/100) for x in [lo[0],hi[0]] for y in [lo[1],hi[1]] for z in [lo[2],hi[2]]];low=[min(q[i] for q in corners) for i in range(3)];high=[max(q[i] for q in corners) for i in range(3)];center=[(a+b)/2 for a,b in zip(low,high)];half=[(b-a)/2*s for a,b,s in zip(low,high,[.96,.96,.92])];boxes.append((p,[c-h for c,h in zip(center,half)],[c+h for c,h in zip(center,half)]))
def hit(a,b,lo,hi):
 t0,t1=0,1
 for j in range(3):
  q=b[j]-a[j]
  if abs(q)<1e-8:
   if a[j]<lo[j] or a[j]>hi[j]:return False
  else:
   aa=(lo[j]-a[j])/q;bb=(hi[j]-a[j])/q
   if aa>bb:aa,bb=bb,aa
   t0=max(t0,aa);t1=min(t1,bb)
   if t0>t1:return False
 return True
bad=[];way=[(31.5,7.4,.98),(28.78,7.4,.98),(28.78,6.5,1.14),(28.78,2.7,3.38),(28.78,2,3.38),(30.22,2,3.38),(30.22,2.9,3.54),(30.22,6.6,5.78),(30.22,7.4,5.78),(31.5,7.4,5.78)]
for st in w['stairs']:
 off=[st['rect'][0]-28,st['rect'][1]];route=[[31.5+off[0],8.5+off[1],st['z_min']+.98]]
 for z in range(round(st['z_min']*100),round(st['z_max']*100),480):route.extend([[x+off[0],y+off[1],zz+z/100] for x,y,zz in way])
 for p,lo,hi in boxes:
  l=[v-r for v,r in zip(lo,[.34,.34,.88])];h=[v+r for v,r in zip(hi,[.34,.34,.88])]
  if any(hit(a,b,l,h) for a,b in zip(route,route[1:])):bad.append(dict(stair=st['id'],car=p['id'],slug=p['slug']))
out=dict(success=not bad,parked=len(boxes),distinct_sources=len({p['slug'] for p,_,_ in boxes}),protected_stair_routes=len(w['stairs']),blocked=bad,method='Conservative metric AABB sweep, player radius .34m and half-height .88m; final native physics/render checks remain authoritative.')
(R/'artifacts/echo_cars_v31/metric_preflight.json').write_text(json.dumps(out,indent=2));print(out)
