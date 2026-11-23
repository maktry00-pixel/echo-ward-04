"""Deterministic v37 dressing. Canonical architecture, story and names stay untouched."""
from pathlib import Path
import json,math,random,hashlib,sys,collections
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard/Content/CampaignData';A=R/'artifacts/echo_hospital_v37';A.mkdir(exist_ok=True)
d=json.loads((P/'hospital_world.json').read_text());story=json.loads((P/'echo_scenario_v35.json').read_text());raw=json.loads((R/'unreal/EchoWard/RawAssets/HospitalV37/manifest.json').read_text());assets={a['id']:a for a in raw['assets']};pilot='--pilot' in sys.argv
class Index:
 def __init__(self):self.cells=collections.defaultdict(list)
 def keys(self,b):
  for x in range(math.floor(b[0]/2),math.floor(b[3]/2)+1):
   for y in range(math.floor(b[1]/2),math.floor(b[4]/2)+1):
    yield x,y,round(b[2]/4.8)
 def add(self,b,id):
  for k in self.keys(b):self.cells[k].append((b,id))
 def hits(self,b):return {id for k in self.keys(b) for r,id in self.cells.get(k,[]) if b[0]<r[3] and b[3]>r[0] and b[1]<r[4] and b[4]>r[1]}
occ=Index();reserved=Index();zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
def bb(x,y,z,w,h):return [x-w/2,y-h/2,z,x+w/2,y+h/2,z+1]
for q in d['prototype_props']+d['environment_props']:
 if q['mount']=='floor' and q.get('rect'):
  x,y,w,h=q['rect'];occ.add([x-.12,y-.12,q['at'][2],x+w+.12,y+h+.12,q['at'][2]+1],q['id'])
for q in d['partitions']:
 x,y,z=q['a'];a,b,_=q['b'];t=q.get('thickness',.12)/2+.15;occ.add([min(x,a)-t,min(y,b)-t,z,max(x,a)+t,max(y,b)+t,z+1],'partition')
# Keep the whole swing fan free, not just the centreline of a doorway.
for site,room in zones.values():
 o=site['origin'];rows=[dict(at=room['door'],side=room.get('door_side','north'),width=2)]+room.get('extra_doors',[])
 for q in rows:
  x,y=q['at'];x+=o[0];y+=o[1];w=q.get('width',2);deep=max(1.65,w/2+.25)
  reserved.add(bb(x,y,o[2],w+.7,deep*2) if q['side'] in ('north','south') else bb(x,y,o[2],deep*2,w+.7),'door')
for q in d['site_entries']:
 x,y,z=q['at'];w=q.get('width',2);deep=max(1.65,w/2+.25);reserved.add(bb(x,y,z,w+.7,deep*2) if q['side'] in ('north','south') else bb(x,y,z,deep*2,w+.7),'entry')
segments=set()
for q in d['navigation_routes']+d['bay_navigation_routes']:
 for a,b in zip(q['points'],q['points'][1:]):
  if abs(a[2]-b[2])>.1:continue
  k=tuple(a+b)
  if k in segments:continue
  segments.add(k);m=.58;reserved.add([min(a[0],b[0])-m,min(a[1],b[1])-m,a[2],max(a[0],b[0])+m,max(a[1],b[1])+m,a[2]+1],'route')
arrival=json.loads((P/'echo_arrival_v29.json').read_text())
for a,b in zip(arrival['route'],arrival['route'][1:]):
 reserved.add([min(a[0],b[0])-.65,min(a[1],b[1])-.65,0,max(a[0],b[0])+.65,max(a[1],b[1])+.65,1],'arrival_route')
for q in story['gameplay']['placements']:
 x,y,z=q['at'];reserved.add(bb(x,y,z,1.9,1.9),'story_'+q['id'])
for q in d['hideouts']:
 x,y,z=q['at'];w,h,_=q['size'];reserved.add([x-.4,y-.4,z,x+w+.4,y+h+1,z+1],'hideout')
profiles={};props=[];signs=[];traces=[];rejected=collections.Counter();rng=random.Random(37028)
def family(site):
 for name in ['DG','IF','PS','RH','HS','SV']:
  if site.startswith(name):return name
 return site if site in ['1F','2F','3F','B1','B2','RF','UT','ST'] else 'public'
pal={'1F':([.54,.55,.53],[.19,.25,.28],.24),'2F':([.56,.55,.51],[.24,.28,.28],.24),'3F':([.52,.54,.54],[.21,.24,.30],.29),'B1':([.43,.46,.45],[.20,.23,.25],.39),'B2':([.48,.48,.46],[.27,.22,.22],.43),'DG':([.55,.55,.51],[.22,.28,.31],.23),'IF':([.54,.55,.52],[.23,.29,.29],.28),'PS':([.50,.51,.47],[.26,.25,.23],.34),'RH':([.59,.56,.50],[.29,.27,.25],.20),'HS':([.57,.54,.48],[.26,.25,.24],.19),'SV':([.46,.46,.43],[.25,.26,.27],.37),'public':([.53,.52,.48],[.28,.28,.28],.22),'RF':([.48,.49,.47],[.25,.27,.28],.33),'ST':([.51,.52,.49],[.25,.28,.30],.28),'UT':([.40,.42,.42],[.23,.25,.25],.43)}
models={'1F':[3305,3302,3309,3304],'2F':[3300,3305,3304],'3F':[3306,3311,3305],'B1':[3302,3305],'B2':[3313,3305],'DG':[3311,3305,3309],'IF':[3304,3305,3306],'PS':[3309,3304],'RH':[3311,3309],'HS':[3309,3304],'SV':[3305],'public':[3309,3304]}
def size(mesh,turn=0,tipped=False):
 a=assets[mesh]['bounds_cm'];s=[(a[1][i]-a[0][i])/100 for i in range(3)]
 if tipped:s[1],s[2]=s[2],s[1]
 if turn%2:s[0],s[1]=s[1],s[0]
 return s

def add(mesh,at,turn,zone,category,solid=False,tipped=False,fit=None):
 s=fit or size(mesh,turn,tipped);b=bb(at[0],at[1],at[2],s[0]+.28,s[1]+.28)
 if solid:
  if occ.hits(b) or reserved.hits(b):rejected['collision_or_reserved']+=1;return False
  occ.add(b,'v37_'+str(len(props)))
 props.append(dict(id=f'H37_{len(props):04}',mesh=mesh,at=[round(v,4) for v in at],yaw=turn*90,roll=90 if tipped else 0,zone=zone,category=category,solid=solid,size_override=fit or [],world_bounds=[at[0]-s[0]/2,at[1]-s[1]/2,at[2],at[0]+s[0]/2,at[1]+s[1]/2,at[2]+s[2]]));return True
pilotzones={'N01','N02','R01','R07','R11','R02'}
for site in d['locations']:
 f=family(site['id']);wall,lower,grime=pal[f];profiles[site['id']]=dict(family=f,wall=wall,lower=lower,grime=grime,wetness=.10 if f in ('B1','B2','UT') else .035)
 for room in site['rooms']:
  zone=room['id'];x,y,w,h=room['rect'];x+=site['origin'][0];y+=site['origin'][1];z=site['origin'][2]
  if not room.get('enclosed',True) or pilot and zone not in pilotzones:continue
  # New furniture is naturally scaled and only placed where measured space permits.
  if f in models and not (site['id'].startswith('X') or site['id'].startswith('S')):
   choices=models[f];mesh=choices[int(hashlib.sha256(zone.encode()).hexdigest()[:6],16)%len(choices)];tipped=(mesh==3309 and int(hashlib.sha256(zone.encode()).hexdigest()[-2:],16)%5==0)
   candidates=[(x+.8+i*.6,y+.8+j*.6) for i in range(max(1,min(24,int((w-1.6)/.6)))) for j in range(max(1,min(24,int((h-1.6)/.6))))];rng.shuffle(candidates)
   for cx,cy in candidates:
    turn=rng.randrange(4);sx,sy,sz=size(mesh,turn,tipped)
    if not (x+.25<cx-sx/2 and cx+sx/2<x+w-.25 and y+.25<cy-sy/2 and cy+sy/2<y+h-.25):continue
    if add(mesh,[cx,cy,z+.002],turn,zone,'fallen_recliner' if tipped else f'{f}_equipment',True,tipped):break
  seed=int(hashlib.sha256(zone.encode()).hexdigest()[:4],16)
  # Open ceiling structure is intermittent, never one identical damaged patch in every room.
  if (seed%7==0 or zone in ('N01','N02','R11')) and min(w,h)>4:
   s=size(3345);add(3345,[x+w*.68,y+h*.55,z+3.80-s[2]],seed%4,zone,'damaged_suspended_ceiling')
  if seed%5==0:
   # Electrical panel is attached to an existing full wall, above trolley clearance.
   add(3321,[x+.11,y+h*.76,z+1.45],1,zone,'electrical_panel')
# Secure psychiatric windows: frames follow existing opening size and sill, not a new wall.
for win in d['windows']:
 zone=win['zone'];site,room=zones.get(zone,({},{}))
 if not site or family(site['id'])!='PS' or win.get('role')!='external_view' or pilot:continue
 x,y=win['at'];x+=site['origin'][0];y+=site['origin'][1];z=site['origin'][2]+win.get('sill',1.1)
 side=win['side'];turn=0 if side in ('north','south') else 1;dx,dy={'north':(0,-.16),'south':(0,.16),'west':(-.16,0),'east':(.16,0)}[side];fit=[win['width'],.10,win['height']]
 if turn%2:fit[0],fit[1]=fit[1],fit[0]
 add(3317,[x+dx,y+dy,z],turn,zone,'psychiatric_window_guard',False,fit=fit)
# A small, curated corridor arrangement; the central route stays empty.
for mesh,at,turn in [(3302,[51.15,7.3,.002],1),(3305,[51.18,11.0,.002],0),(3303,[38.0,-8.0,.002],0),(3306,[57.8,-5.3,.002],1)]:add(mesh,at,turn,'N01' if at[1]<0 else 'R07','pilot_corridor_equipment',True)
add(3309,[58.3,-11.2,.002],0,'N01','fallen_recliner',True,True)
add(3345,[62,19,3.025],0,'R11','damaged_suspended_ceiling')
# Real light fixtures anchor illumination to the checkpoint canopy and entry.
for at in [[63.3,-57.3,2.8],[45.1,-25.7,2.8],[54.5,-13.8,3.2]]:
 props.append(dict(id=f'H37_{len(props):04}',mesh=643,at=at,yaw=0,roll=0,zone='N02',category='pilot_canopy_lamp',solid=False,size_override=[.66,.30,.09],world_bounds=bb(at[0],at[1],at[2],.66,.30)))
# Additional blood is constrained to authored injury/death locations; water sits at leaks.
for zone,kind,frac in [('R11','Drag',(.32,.56)),('R37','Blood',(.62,.70)),('E10','Drag',(.70,.35)),('N01','Water',(.22,.42))]:
 if zone not in zones or pilot and zone not in pilotzones:continue
 s,r=zones[zone];x,y,w,h=r['rect'];at=[s['origin'][0]+x+w*frac[0],s['origin'][1]+y+h*frac[1],s['origin'][2]+.001]
 traces.append(dict(id='H37_trace_'+zone,zone=zone,kind=kind,at=at,size=[1.1,1.8,.001],story_reason='authored_death_or_injury' if kind!='Water' else 'local_ceiling_leak'))
# Small intentional holes in non-structural partitions. Floors, structural slabs and room bounds stay intact.
wall_damage=[]
for zone,(site,room) in zones.items():
 if pilot and zone not in ('R11','N01','R07'):continue
 seed=int(hashlib.sha256(zone.encode()).hexdigest()[:4],16)
 if not pilot and zone not in ('R11','R07','E10','PS08') and seed%31:continue
 if site['id'] in ('ST','RF'):continue
 x,y,w,h=room['rect'];x+=site['origin'][0];y+=site['origin'][1]
 for i,q in enumerate(d['partitions']):
  if q['site']!=site['id'] or q.get('type')=='niche':continue
  a,b=q['a'],q['b'];cx=(a[0]+b[0])/2;cy=(a[1]+b[1])/2;length=abs(a[0]-b[0])+abs(a[1]-b[1])
  if x+.15<cx<x+w-.15 and y+.15<cy<y+h-.15 and 2.5<length<6:
   wall_damage.append(dict(index=i,zone=zone,site=site['id'],a=a,b=b,width=.95,bottom=1.60,top=2.45,reason='damaged_nonstructural_partition'));break
# Preserve the standing approach to every large room's centre as well as authored routes.
clearance_repairs=[]
for zone in ['R22','B101','N03','IF17','RH19','E01','E12','IF2_09','RH2_10']:
 if zone not in zones:continue
 site,room=zones[zone];o=site['origin'];rx,ry,rw,rh=room['rect'];rx+=o[0];ry+=o[1];cx=rx+rw/2;cy=ry+rh/2
 dx=o[0]+room['door'][0];dy=o[1]+room['door'][1];inside=[dx,dy+(-1.4 if room.get('door_side')=='south' else 1.4)]
 safe=[min(inside[0],cx)-.65,min(inside[1],cy)-.65,o[2],max(inside[0],cx)+.65,max(inside[1],cy)+.65,o[2]+1]
 reserved.add(safe,'room_centre_approach')
 for q in list(props):
  if q['zone']!=zone or not q['solid']:continue
  old=q['at'].copy();sx,sy,sz=size(q['mesh'],int(q['yaw']/90),bool(q['roll']));candidates=[(rx+.5+i*.3,ry+.5+j*.3) for i in range(max(1,int((rw-1)/.3))) for j in range(max(1,int((rh-1)/.3)))];candidates.sort(key=lambda c:(c[0]-old[0])**2+(c[1]-old[1])**2)
  found=False
  for x,y in candidates:
   if not(rx+.2<x-sx/2 and x+sx/2<rx+rw-.2 and ry+.2<y-sy/2 and y+sy/2<ry+rh-.2):continue
   b=bb(x,y,old[2],sx+.28,sy+.28)
   if occ.hits(b) or reserved.hits(b):continue
   q['at']=[round(x,4),round(y,4),old[2]];q['world_bounds']=[x-sx/2,y-sy/2,old[2],x+sx/2,y+sy/2,old[2]+sz];occ.add(b,q['id']);found=True;break
  if not found:props.remove(q)
  clearance_repairs.append(dict(zone=zone,id=q['id'],before=old,after=q['at'] if found else None,action='relocated' if found else 'rejected_additional_prop'))
blood_zones=['R11','R37','R38','F202','B112','B115','PS04','PS13','PS19','DG25','DG26','IF08','SV24']+[f'E{i:02d}' for i in range(1,25)]
D=dict(version=37,base_floor_wetness=.12,clearance_repairs=clearance_repairs,blood_zones=blood_zones,wall_damage=wall_damage,scope='pilot' if pilot else 'all',map_sha256=hashlib.sha256((P/'hospital_world.json').read_bytes()).hexdigest(),profiles=profiles,props=props,traces=traces,counts=dict(props=len(props),floor_solid=sum(p['solid'] for p in props),native_models=len(assets),guards=sum(p['category']=='psychiatric_window_guard' for p in props),damaged_ceilings=sum(p['category']=='damaged_suspended_ceiling' for p in props),damaged_partitions=len(wall_damage)),cold_light_anchors=[[60,17.5,3.1]],light_anchors=[[63.3,-57.3,2.8],[45.1,-25.7,2.8],[54.5,-13.8,3.2]],limits=['Broken suspended ceiling retains the structural slab above it.','Added dressing does not claim a full art replacement for every old prop.'])
(P/'hospital_environment_v37.json').write_text(json.dumps(D,ensure_ascii=False,separators=(',',':')),encoding='utf-8')
(A/('pilot_manifest.json' if pilot else 'all_manifest.json')).write_text(json.dumps(D,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(scope=D['scope'],**D['counts'],placement_rejections=dict(rejected)),ensure_ascii=False))
