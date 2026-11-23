"""Original detailed meshes and additive, supported world dressing.

The canonical game map is read-only. Presentation rows refer to the same
metric furniture envelopes; no route, door, save or collision is changed.
"""
from pathlib import Path
import json,math,random,collections,hashlib
import numpy as np
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard'
scope={'__file__':str(R/'tools/build_echo_supplements_v21.py')}
exec((R/'tools/build_echo_supplements_v21.py').read_text().split("titles=['Sofa'")[0],scope)
Mesh=scope['Mesh'];OUT=P/'RawAssets/EchoQualityV22';OUT.mkdir(exist_ok=True);scope['OUT']=OUT;scope['rows']=[]
M=scope['M'];M.update(carpaint='V22_CarPaint',chrome='V22_Chrome',carwindow='V22_CarGlass',tail='V22_Tail',headlamp='V22_Headlamp',bark='V22_Bark',needles='V22_Needles',leaves='V22_Leaves',stone='V22_Stone',metalclean='V22_Metal',plastic='V22_Plastic')

def sedan():
 m=Mesh()
 # Continuous rounded coachwork: cross sections, wheel recesses and shoulder.
 stations=[(-2.35,.63,.56),(-2.26,.85,.78),(-1.7,.92,.91),(-.95,.94,.96),(.95,.94,.97),(1.8,.89,.87),(2.25,.77,.7),(2.34,.62,.51)]
 rings=[]
 for y,w,h in stations:
  rings.append([(x,y,z) for x,z in [(-w*.82,.32),(-w,.47),(-w,h*.9),(-w*.87,h),(w*.87,h),(w,h*.9),(w,.47),(w*.82,.32)]])
 verts=sum(rings,[]);faces=[tuple(reversed(range(8))),tuple(range((len(rings)-1)*8,len(rings)*8))]
 for i in range(len(rings)-1):
  for j in range(8):faces.append((i*8+j,i*8+(j+1)%8,(i+1)*8+(j+1)%8,(i+1)*8+j))
 m.poly(verts,faces,'carpaint')
 # Curved cabin roof; glass is separate from the body.
 sections=[(-1.18,.79,.94),(-.58,.72,1.43),(.52,.72,1.46),(1.26,.81,.94)]
 cabin=[]
 for y,w,z in sections:cabin.extend([(-w,y,.94),(-w*.94,y,z-.04),(0,y,z+.035),(w*.94,y,z-.04),(w,y,.94)])
 faces=[]
 for i in range(3):
  for j in range(4):faces.append((i*5+j,(i+1)*5+j,(i+1)*5+j+1,i*5+j+1))
 m.poly(cabin,faces,'carwindow')
 # Roof panel excludes windscreens; chrome surrounds, door seams, handles.
 m.box((0,-.02,1.465),(1.36,1.12,.05),'carpaint',.02)
 for side in [-1,1]:
  m.pipe([(side*.79,-1.18,.96),(side*.68,-.58,1.42),(side*.68,.52,1.45),(side*.81,1.26,.96)],.018,'chrome')
  for y in [-1.15,.08,1.22]:m.pipe([(side*.925,y,.46),(side*.938,y,.86),(side*.8,y,1.01)],.004,'black')
  m.pipe([(side*.93,-1.7,.38),(side*.95,1.78,.38)],.014,'chrome')
  for y in [-.5,.75]:m.box((side*.946,y,.87),(.025,.19,.035),'chrome',.012)
  m.box((side*.93,-.7,1.02),(.24,.2,.105),'carpaint',.03)
  m.box((side*.96,-.59,1.02),(.16,.008,.07),'carwindow',.01)
  # Rubber tyres, alloy barrels, brake discs and 12 individual spokes.
  for y in [-1.52,1.49]:
   x=side*.87;z=.35
   m.cyl((x-.115,y,z),(x+.115,y,z),.355,'rubber',48)
   outer=x+side*.12;m.cyl((outer-side*.012,y,z),(outer,y,z),.265,'chrome',48)
   m.cyl((outer+side*.004,y,z),(outer+side*.009,y,z),.224,'black',40)
   m.cyl((outer+side*.01,y,z),(outer+side*.013,y,z),.17,'steel',32)
   for a in np.linspace(0,2*math.pi,12,endpoint=False):m.cyl((outer+side*.018,y+.055*math.cos(a),z+.055*math.sin(a)),(outer+side*.018,y+.245*math.cos(a+.09),z+.245*math.sin(a+.09)),.013,'chrome',8)
   m.cyl((outer+side*.015,y,z),(outer+side*.03,y,z),.055,'chrome',20)
   # Wheel arch lip and inner shadow trace.
   pts=[(side*.948,y+.385*math.cos(a),z+.385*math.sin(a)) for a in np.linspace(0,math.pi,18)]
   m.pipe(pts,.018,'carpaint')
 for side in [-1,1]:
  m.box((side*.55,-2.27,.74),(.49,.055,.115),'headlamp',.025)
  m.box((side*.55,2.27,.7),(.48,.04,.105),'tail',.025)
 m.box((0,-2.295,.51),(.79,.025,.15),'black',.02)
 for x in np.linspace(-.35,.35,15):m.box((x,-2.313,.51),(.012,.012,.13),'chrome',.003)
 for y in [-2.34,2.34]:m.box((0,y,.47),(.44,.018,.105),'white',.008)
 m.box((0,2.28,.88),(1.38,.12,.025),'carpaint',.009)
 # Seats/dashboard visible through the tinted cabin.
 for x in [-.37,.37]:
  for y in [-.2,.68]:m.box((x,y,.93),(.47,.42,.16),'black',.055);m.box((x,y+.16,1.14),(.47,.11,.43),'black',.045)
 m.box((0,-.68,1.03),(1.32,.24,.1),'black',.03)
 return m

def tree(seed,deciduous=False):
 rng=random.Random(seed);m=Mesh();height=12 if deciduous else 16
 m.cyl((0,0,0),(.08,.03,height),.28,'bark',14,r2=.035)
 for j in range(42 if deciduous else 65):
  z=2+j*(height-2)/65;a=j*2.399+rng.uniform(-.18,.18);reach=(1-z/height)*(3.8 if not deciduous else 4.1)+.25
  start=(0,0,z);tip=(reach*math.cos(a),reach*math.sin(a),z+(.65 if deciduous else -.55));m.cyl(start,tip,max(.014,.07*(1-z/height)),'bark',8,r2=.009)
  for k in range(7 if deciduous else 10):
   t=(k+1)/10;p=np.array(start)*(1-t)+np.array(tip)*t;aa=a+(-1 if k%2 else 1)*.9;ln=reach*.45*(1-t)+.15;end=p+np.array([ln*math.cos(aa),ln*math.sin(aa),.1]);m.cyl(p,end,.009,'bark',6,r2=.003)
   for q in range(7):
    f=q/7;mid=p*(1-f)+end*f;spread=(.18 if deciduous else .24)*(1-f)+.06
    for cross in (0,math.pi/2):
     u=np.array([math.cos(aa+cross),math.sin(aa+cross),.25])*spread;v=np.array([-math.sin(aa+cross),math.cos(aa+cross),.3])*spread*.65
     m.poly([mid-u,mid+v,mid+u,mid-v],[(0,1,2),(0,2,3),(2,1,0),(3,2,0)],'leaves' if deciduous else 'needles')
 return m

for i,title,m in [(640,'Rounded sedan with alloy wheels and separate glass',sedan()),(641,'Branch and needle conifer',tree(45)),(642,'Branch and leaf courtyard tree',tree(87,True))]:m.write(i,title)
for i in [643,644,645,646,647,648]:
 m=Mesh()
 if i==643:
  m.box((0,.015,.13),(.12,.045,.26),'metalclean');m.box((0,-.09,.19),(.22,.19,.05),'metalclean');m.box((0,-.09,.158),(.17,.14,.012),'headlamp',.003)
 elif i==644:
  m.cyl((0,0,0),(0,0,5.8),.065,'metalclean',16,r2=.04);m.box((0,0,.07),(.25,.25,.14),'metalclean');m.pipe([(0,0,5.5),(.05,0,5.8),(.65,0,5.8)],.045,'metalclean');m.box((.7,0,5.78),(.65,.24,.09),'metalclean');m.box((.7,0,5.725),(.55,.19,.012),'headlamp')
 elif i==645:
  rng=random.Random(73)
  for j in range(35):
   c=np.array([rng.uniform(-.45,.45),rng.uniform(-.3,.3),rng.uniform(.002,.02)]);a=rng.uniform(0,math.tau);u=np.array([math.cos(a),math.sin(a),.2])*.06;v=np.array([-math.sin(a),math.cos(a),.06])*.026;m.poly([c-u,c+v,c+u,c-v],[(0,1,2),(0,2,3)],'leaves')
 elif i==646:
  for j in range(6):m.ring((0,0,.014+j*.012),.16+j*.005,.009,'rubber',axis='z')
 elif i==647:
  rng=random.Random(9)
  for j in range(15):m.box((rng.uniform(-.3,.3),rng.uniform(-.2,.2),rng.uniform(.01,.08)),(rng.uniform(.04,.13),rng.uniform(.04,.12),.025),'stone',.007)
 elif i==648:
  m.box((0,0,.25),(1.25,.6,.5),'stone',.025);m.box((0,0,.51),(1.17,.52,.025),'black');rng=random.Random(95)
  for j in range(25):
   x=rng.uniform(-.52,.52);y=rng.uniform(-.2,.2);z=rng.uniform(.7,1.1);m.cyl((x,y,.5),(x,y,z),.007,'bark',6)
   for k in range(4):
    a=k*1.5+j;c=np.array([x,y,z-k*.05]);u=np.array([math.cos(a),math.sin(a),.4])*.13;v=np.array([-math.sin(a),math.cos(a),0])*.05;m.poly([c-u,c+v,c+u,c-v],[(0,1,2),(0,2,3),(2,1,0),(3,2,0)],'leaves')
 m.write(i,{643:'Shielded wall light',644:'Street lamp',645:'Scattered leaves',646:'Coiled cable',647:'Small broken plaster',648:'Stone shrub planter'}[i])
(OUT/'manifest.json').write_text(json.dumps(dict(version=22,assets=scope['rows']),ensure_ascii=False,indent=2),encoding='utf8')

d=json.loads((P/'Content/CampaignData/hospital_world.json').read_text());bindings=json.loads((P/'Content/CampaignData/echo_custom_bindings.json').read_text());bindings['prototype_models']['137']=640;bindings['version']=22
# Convert complete logical clutter items; their old component boxes are hidden
# only when the fitted native mesh succeeds, preserving all solid collisions.
props=[];skip=[]
semantic={'mug':267,'patient_chart':259,'folder':256,'medicine_bottle':290,'tool_case':347,'supply_box':356,'towel_stack':178,'books':257,'food_carton':356,'linen':179,'medical_tray':212,'parcel':358,'chemical_bottle':290,'floor_paper':262,'drain':483,'pen':264,'syringe':217,'rubber_gloves':222,'blister_pack':221,'staff_badge':550,'evidence_envelope':263,'adhesive_tape':220,'tweezers':214,'gauze_packet':221,'ampoule_rack':284,'stapler':266,'calculator':260,'sticky_notes':261,'soap':341,'toothbrush':264,'fallen_leaves':645}
for layer in ['clutter_dressing','microdetails_dressing']:
 for q in d[layer]['items']:
  if q['kind'] not in semantic:continue
  bb=q['bounds'];props.append(dict(id=q['id'],mesh=semantic[q['kind']],at=[0,0,0],bounds=bb,turn=0,category='refined_clutter'));skip.append(q['id'])
campus={'sedan':640,'bench':466,'planter':648,'cycle_rack':469,'recycling':468,'treatment_trolley':192,'crash_cart':192,'mail_cart':362,'cleaning_cart':335,'personal_locker':242,'water_dispenser':272,'linen_hamper':194,'bedside_cabinet':181,'phone_station':252,'stock_cabinet':244,'retail_chiller':280,'tool_board':347,'kitchen_trolley':195,'corridor_bin':270}
for q in d['campus_plan']['items']:
 if q['kind'] in campus:
  props.append(dict(id=q['id'],mesh=campus[q['kind']],at=[0,0,0],bounds=q['bounds'],turn=0,category='refined_campus'));skip.append(q['id'])
# Add useful small objects to previously bare portions of support surfaces.
from hospital_clutter_v10 import Space,bounds,rotate
space=Space()
for q in props:
 space.add(q['bounds'],q['id'])
for q in d['item_types_dressing']['items']:space.add(q['bounds'],q['id'])
rng=random.Random(22478);add=0
for p in d['prototype_props']+d['environment_props']:
 model=d['prototype_catalog'][str(p['model'])];top=model.get('top',0)
 if not top or p['mount']!='floor' or p['model'] not in (4,13,16,17,19,20,21,22,23,24,37,38,89,108,109,114,132):continue
 bb=model['bounds'];w=bb[3]-bb[0];dep=bb[4]-bb[1]
 for j,(x,y) in enumerate([(-.27,-.25),(.26,-.26),(-.25,.25),(.25,.24)]):
  ident=f'Q22_{p["id"]}_{j}';off=rotate([x*w,y*dep,top+.015],p['turn']);at=[p['at'][k]+off[k] for k in range(3)];mesh,size=rng.choice([(261,[.18,.24,.026]),(269,[.065,.065,.19]),(267,[.08,.09,.1]),(263,[.16,.1,.008]),(264,[.13,.012,.012]),(258,[.16,.21,.025]),(219,[.065,.065,.05])]);b=bounds([at[0],at[1],at[2]+size[2]/2],size)
  if space.hits(b):continue
  # Stay within the actual support footprint after its quarter-turn.
  if size[0]>.4*w or size[1]>.4*dep:continue
  props.append(dict(id=ident,mesh=mesh,at=[0,0,0],bounds=b,turn=0,category='new_supported_detail',parent=p['id']));space.add(b,ident);add+=1

# Native trees replace aggregate sphere crowns; existing trunk collision stays.
trees=[]
for q in d['environment_dressing']['boxes']:
 if q['kind'] not in ('forest_trunk','tree_trunk'):continue
 h=q['size'][2];base=[q['at'][0],q['at'][1],q['at'][2]-h*.5];trees.append(dict(id=q.get('tree_id',f'TREE_{len(trees)}'),at=base,height=h+1.5,mesh=642 if len(trees)%5==0 else 641,yaw=(len(trees)*137)%360))

windows=[]
sites={s['id']:s for s in d['locations']}
for q in d['windows']:
 if q.get('role')!='external_view':continue
 s=sites[q['site']];o=s['origin'];x,y=q['at'];x+=o[0];y+=o[1];side=q['side'];offset=q.get('offset',0)
 if side in ('north','south'):x+=offset
 else:y+=offset
 windows.append(dict(at=[x,y,o[2]],side=side,width=q['width'],sill=q.get('sill',1.1),height=q.get('height',1),lit=len(windows)%5 not in (0,4),zone=q['zone']))

# Map-independent capture points taken from real outdoor rooms.
views=[]
rooms={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
enclosed=[]
for site in d['locations']:
 if abs(site['origin'][2])>.01 or site['id']=='UT':continue
 for room in site['rooms']:
  if not room.get('enclosed',True):continue
  x,y,w,h=room['rect'];ox,oy,_=site['origin'];enclosed.append([ox+x,oy+y,ox+x+w,oy+y+h])
solid=[bounds(b['at'],b['size']) for b in d['environment_dressing']['boxes'] if b.get('solid') and b['at'][2]-b['size'][2]/2<2 and b['at'][2]+b['size'][2]/2>.2]
def outdoor_free(x,y,margin=.6):
 return not any(b[0]-margin<x<b[2]+margin and b[1]-margin<y<b[3]+margin for b in enclosed) and not any(b[0]-margin<x<b[3]+margin and b[1]-margin<y<b[4]+margin for b in solid)
lamps=[]
for zid in ['S01','S05','S15','S02']:
 site,room=rooms[zid];ox,oy,_=site['origin'];x,y,w,h=room['rect']
 for lx in np.arange(x+2,x+w-1,10):
  for ly in [y+2,y+h-2]:
   px,py=ox+lx,oy+ly
   if outdoor_free(px,py,1):lamps.append(dict(at=[float(px),float(py),0],yaw=0))
for x in [-75,-2,106,140]:
 for y in np.arange(-155,235,18):
  if outdoor_free(x,y,1):lamps.append(dict(at=[x,float(y),0],yaw=90))
for zid in ['S01','S02','S05','S15','S06']:
 s,r=rooms[zid];x,y,w,h=r['rect'];o=s['origin'];candidates=[]
 for fx,fy in [(.35,.75),(.6,.6),(.45,.45),(.8,.2),(.15,.7),(.85,.8)]:
  px,py=o[0]+x+w*fx,o[1]+y+h*fy
  if outdoor_free(px,py,1):candidates.append([px,py,1.7])
 eye=candidates[0] if candidates else [o[0]+x+w*.5,o[1]+y-3,1.7]
 target=[eye[0]+15,eye[1]+12,3.3]
 if zid=='S01':target=[-32,-54,1.1]
 views.append(dict(id='v22_court_'+zid,at=eye,target=target,horror_mode=True))
for c in bindings['review_captures'][:6]:c=dict(c);c['id']=c['id'].replace('v21_','v22_');views.append(c)
quality=dict(version=22,map_sha256=hashlib.sha256((P/'Content/CampaignData/hospital_world.json').read_bytes()).hexdigest(),props=props,replaced_environment_items=skip,trees=trees,windows=windows,outdoor_lamps=lamps,captures=views,counts=dict(refined_items=len(skip),new_supported_details=add,trees=len(trees),windows=len(windows),new_lamps=len(lamps)),limits=['Additive visual dressing; existing collisions and story remain unchanged.','Original first-pass vehicle and foliage art is more detailed but does not equal a photographic reference.'])
(P/'Content/CampaignData/echo_quality_v22.json').write_text(json.dumps(quality,ensure_ascii=False,indent=2),encoding='utf8');(P/'Content/CampaignData/echo_custom_bindings.json').write_text(json.dumps(bindings,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(dict(meshes=len(scope['rows']),**quality['counts'])))
