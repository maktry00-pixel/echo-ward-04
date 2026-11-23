"""Reflow crowded and tall details onto free exposed surfaces in the same room."""
import collections,json,math,sys
from pathlib import Path
import numpy as np
from hospital_clutter_v10 import Space,bounds,rotate
from echo_support_coverage_v23 import covers,restore
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_real_v23'
def read(p):return json.loads(p.read_text(encoding='utf8'))
art=read(P/'Content/CampaignData/echo_real_v23.json');world=read(P/'Content/CampaignData/hospital_world.json');q22=read(P/'Content/CampaignData/echo_quality_v22.json');native=read(P/'RawAssets/EchoRealV23/support_planes.json');assign=read(O/'support_assignments.json')
parents={p['id']:p for p in world['prototype_props']+world['environment_props']};legacy={p['id']:p for layer in ['clutter_dressing','microdetails_dressing'] for p in world[layer]['items']};old={p['id']:p for p in q22['props']};surfaces=collections.defaultdict(dict);jobs=[];invalid=set(art['unresolved_support'])
def metadata(s):return {k:v for k,v in s.items() if k!='triangles'}
def register(zone,parent,plane):
 s=restore(plane,native);key=(parent,s.get('source_plane',-1),round(s['z'],5));surfaces[zone][key]=(parent,s)
 return s
for ident,plane in assign.items():
 p=old[ident];parent=parents.get(legacy.get(ident,p).get('parent'))
 if not parent:continue
 s=register(parent['zone'],parent['id'],plane);assign[ident]=metadata(s)
 b=art['bounds_overrides'].get(ident,p['bounds'])
 if b[5]>s.get('ceiling_z',float('inf'))-.018:invalid.add(ident)
for p in art['props']:
 if p['category']!='supported_detail':continue
 s=register(p['zone'],p['parent'],p['support']);p['support']=metadata(s)
 if p['world_bounds'][5]>s.get('ceiling_z',float('inf'))-.018:invalid.add(p['id'])
new={p['id']:p for p in art['props']}
def apply(ident,b,c,parent,s):
 if ident in new:
  p=new[ident];p['at']=[float(p['at'][i]+c[i]-b[i]) for i in range(3)];p['world_bounds']=c;p['parent']=parent;p['support']=metadata(s)
 else:art['bounds_overrides'][ident]=c;assign[ident]=metadata(s)
replayed=[]
if '--resume' in sys.argv:
 for m in read(O/'support_reflow.json')['moved']:
  candidates=[s for parent,s in surfaces[m['zone']].values() if parent==m['parent'] and abs(s['z']+.009-m['after'][2])<.002 and covers(s,m['after'])]
  assert candidates,m['id'];apply(m['id'],m['before'],m['after'],m['parent'],candidates[0]);invalid.discard(m['id']);replayed.append(m)
for ident in invalid:
 p=new.get(ident,old.get(ident));ref=legacy.get(ident,p);parent=parents.get(ref.get('parent'))
 if ident in new:zone=p['zone'];b=p['world_bounds']
 else:zone=parent['zone'];b=art['bounds_overrides'].get(ident,p['bounds'])
 jobs.append((ident,zone,list(b)))
space=Space()
for ident,p in old.items():
 if ident not in invalid:space.add(art['bounds_overrides'].get(ident,p['bounds']),ident)
for ident,p in new.items():
 if ident not in invalid:space.add(p['world_bounds'],ident)
# Existing fixed tabletop equipment must also remain free of new overlaps.
catalog=world['prototype_catalog']
for p in parents.values():
 if p['mount']=='floor':continue
 bb=catalog[str(p['model'])]['bounds'];pts=np.array([rotate([x,y,z],p['turn']) for x in [bb[0],bb[3]] for y in [bb[1],bb[4]] for z in [bb[2],bb[5]]])+p['at'];space.add(list(pts.min(0))+list(pts.max(0)),p['id'])
failed=[];moved=replayed
def choose(ident,zone,b):
 w,depth,height=[b[i+3]-b[i] for i in range(3)];cx=(b[0]+b[3])/2;cy=(b[1]+b[4])/2;choices=[]
 for parent,s in surfaces[zone].values():
  x0,y0,x1,y1=s['rect']
  if w+.006>x1-x0 or depth+.006>y1-y0:continue
  if s['z']+.009+height>s.get('ceiling_z',float('inf'))-.018:continue
  positions=[(min(x1-w/2,max(x0+w/2,cx)),min(y1-depth/2,max(y0+depth/2,cy)))]+[(x,y) for x in np.linspace(x0+w/2+.002,x1-w/2-.002,11) for y in np.linspace(y0+depth/2+.002,y1-depth/2-.002,11)]
  for x,y in positions:
   z=s['z']+.009;c=[float(x-w/2),float(y-depth/2),z,float(x+w/2),float(y+depth/2),z+height]
   if covers(s,c) and not space.hits(c,ident):choices.append((math.hypot(x-cx,y-cy)+abs(z-b[2]),c,parent,s))
 return min(choices,key=lambda q:q[0]) if choices else None
raw=read(P/'RawAssets/EchoRealV23/manifest.json');by_slug=collections.defaultdict(list)
for a in raw['assets']:by_slug[a['slug']].append(a)
for ident,zone,b in sorted(jobs):
 result=choose(ident,zone,b)
 replacement=None
 if not result and ident in new:
  # Use a smaller item of the same purpose, rather than distorting a full-size
  # thermos or letting a tall bottle intersect the shelf above it.
  role=new[ident]['role'];slugs=['medical_tape','medical_box'] if role=='clinical' else ['office_notepads','stationery_supplies']
  options=[a for slug in slugs for a in by_slug[slug]];options.sort(key=lambda a:np.prod(np.array(a['bounds_cm'])[1]-np.array(a['bounds_cm'])[0]))
  for a in options:
   bb=np.array(a['bounds_cm'])/100;points=np.array([rotate([x,y,z],new[ident]['turn']) for x in bb[:,0] for y in bb[:,1] for z in bb[:,2]]);sz=points.max(0)-points.min(0);cx=(b[0]+b[3])/2;cy=(b[1]+b[4])/2
   test=[cx-sz[0]/2,cy-sz[1]/2,b[2],cx+sz[0]/2,cy+sz[1]/2,b[2]+sz[2]];result=choose(ident,zone,test)
   if result:replacement=(a,bb,points);break
 if not result:failed.append(ident);continue
 score,c,parent,s=result;space.add(c,ident);moved.append(dict(id=ident,zone=zone,before=b,after=c,parent=parent,replacement_mesh=replacement[0]['id'] if replacement else None))
 if replacement:
  a,bb,points=replacement;p=new[ident];p['mesh']=a['id'];p['bounds']=list(bb[0])+list(bb[1]);p['at']=[float(c[i]-points.min(0)[i]) for i in range(3)];p['world_bounds']=c;p['parent']=parent;p['support']=metadata(s)
 else:apply(ident,b,c,parent,s)
art['unresolved_support']=failed;art['counts']['corrected_supports']=len(art['bounds_overrides'])
(O/'support_reflow.json').write_text(json.dumps(dict(success=not failed,relocated=len(moved),failed=failed,moved=moved),ensure_ascii=False,indent=2),encoding='utf8')
assert not failed,failed
(P/'Content/CampaignData/echo_real_v23.json').write_text(json.dumps(art,ensure_ascii=False,separators=(',',':')),encoding='utf8');(O/'support_assignments.json').write_text(json.dumps(assign,ensure_ascii=False,separators=(',',':')),encoding='utf8')
print('EXPOSED_SUPPORT_REFLOW_OK',len(moved),art['counts'])
