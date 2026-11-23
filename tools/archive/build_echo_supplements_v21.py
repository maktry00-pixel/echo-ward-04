"""Author missing furnishings as real bevelled/curved OBJ meshes, in metres.

No downloaded assets and no changes to the authored EH_001..575 sources.
The native OBJ importer counter-reflects Y; reverse winding accordingly.
"""
import math,json,hashlib,collections
from pathlib import Path
import numpy as np
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';OUT=P/'RawAssets/EchoSupplementsV21';OUT.mkdir(exist_ok=True)
base=json.loads((P/'RawAssets/EchoCustom/manifest.json').read_text())
M={v['name']:k for k,v in base['materials'].items()};rows=[]
class Mesh:
 def __init__(self):self.parts=[]
 def poly(self,verts,faces,mat):
  verts=np.asarray(verts,float)
  for f in faces:
   pts=verts[list(f)];n=np.cross(pts[1]-pts[0],pts[2]-pts[0]);ln=np.linalg.norm(n)
   if ln<1e-10:continue
   n/=ln
   # Face-local metric UVs rather than one colour per primitive.
   ax=np.argmax(abs(n));uv=np.delete(pts,ax,axis=1)
   self.parts.append((M[mat],pts,np.tile(n,(len(pts),1)),uv))
 def box(self,c,s,mat='white',b=.015):
  x,y,z=c;w,d,h=np.array(s)/2;b=min(b,w*.45,d*.45,h*.45)
  # Four octagonal rings bevel all vertical and horizontal edges.
  v=[]
  for zz,ww,dd in [(-h,w-b,d-b),(-h+b,w,d),(h-b,w,d),(h,w-b,d-b)]:
   bb=min(b,ww*.45,dd*.45)
   v.extend([(x+xx,y+yy,z+zz) for xx,yy in [(-ww+bb,-dd),(ww-bb,-dd),(ww,-dd+bb),(ww,dd-bb),(ww-bb,dd),(-ww+bb,dd),(-ww,dd-bb),(-ww,-dd+bb)]])
  faces=[tuple(reversed(range(8))),tuple(range(24,32))]
  for k in range(3):
   for i in range(8):faces.append((k*8+i,k*8+(i+1)%8,(k+1)*8+(i+1)%8,(k+1)*8+i))
  self.poly(v,faces,mat)
 def cyl(self,a,b,r,mat='steel',n=20,r2=None):
  a=np.array(a,float);b=np.array(b,float);axis=b-a;axis/=np.linalg.norm(axis);ref=np.array([0,0,1]) if abs(axis[2])<.9 else np.array([0,1,0]);u=np.cross(ref,axis);u/=np.linalg.norm(u);v=np.cross(axis,u);r2=r if r2 is None else r2
  pts=[p+rr*(math.cos(i*2*math.pi/n)*u+math.sin(i*2*math.pi/n)*v) for p,rr in [(a,r),(b,r2)] for i in range(n)]
  self.poly(pts,[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)],mat)
 def ring(self,c,r,t,mat='steel',axis='y',n=40):
  v=[]
  for i in range(n):
   a=i*2*math.pi/n
   for j in range(10):
    b=j*2*math.pi/10;p=((r+t*math.cos(b))*math.cos(a),t*math.sin(b),(r+t*math.cos(b))*math.sin(a))
    if axis=='z':p=(p[0],p[2],p[1])
    v.append(np.array(c)+p)
  self.poly(v,[(i*10+j,i*10+(j+1)%10,((i+1)%n)*10+(j+1)%10,((i+1)%n)*10+j) for i in range(n) for j in range(10)],mat)
 def pipe(self,pts,r=.012,mat='rubber'):
  for a,b in zip(pts,pts[1:]):self.cyl(a,b,r,mat,12)
 def wheels(self,w=.55,d=.45):
  for x in [-w/2,w/2]:
   for y in [-d/2,d/2]:
    self.cyl((x-.035,y,.055),(x+.035,y,.055),.055,'rubber');self.box((x,y,.12),(.04,.04,.12),'steel')
 def legs(self,w,d,h=.7):
  for x in [-w/2,w/2]:
   for y in [-d/2,d/2]:self.cyl((x,y,.03),(x,y,h),.026,'steel')
 def handle(self,x,y,z,w=.18):
  self.pipe([(x-w/2,y,z),(x-w/2,y-.045,z),(x+w/2,y-.045,z),(x+w/2,y,z)],.009,'steel')
 def vents(self,x,y,z,w=.35,count=7):
  for i in range(count):self.box((x,y,z+i*.022),(w,.009,.009),'black',.002)
 def controls(self,x,y,z,count=5):
  for i in range(count):self.cyl((x+i*.045,y,z),(x+i*.045,y-.012,z),.012,'red' if i==0 else 'black',12)
 def monitor(self,z=1.3,w=.45):
  self.box((0,.03,z-.2),(.08,.08,.32),'steel');self.box((0,0,z),(w,.07,w*.68),'black');self.box((0,-.041,z),(w*.87,.008,w*.52),'monitor_trace',.002)
 def trolley(self):
  self.wheels();self.box((0,0,.22),(.65,.56,.08),'steel');self.box((0,0,.6),(.55,.46,.65));self.handle(0,-.24,.68);self.vents(0,-.237,.3)
 def write(self,ident,title):
  name=f'EH_{ident:03}';f=OUT/(name+'.obj');lines=['mtllib '+name+'.mtl','g '+name];off=1;v_all=[]
  for mat,pts,ns,uv in self.parts:
   lines.append('usemtl '+mat)
   for v in pts:lines.append('v %.6f %.6f %.6f'%(v[0]*100,-v[1]*100,v[2]*100));v_all.append(v*100)
   for u in uv:lines.append('vt %.6f %.6f'%tuple(u))
   for n in ns:lines.append('vn %.6f %.6f %.6f'%(n[0],-n[1],n[2]))
   for j in range(1,len(pts)-1):lines.append('f '+' '.join(f'{i}/{i}/{i}' for i in (off,off+j+1,off+j)))
   off+=len(pts)
  f.write_text('\n'.join(lines)+'\n',encoding='utf8');v=np.array(v_all)
  (OUT/(name+'.mtl')).write_text(''.join('newmtl '+key+'\nKd .5 .5 .5\n\n' for key in dict.fromkeys(q[0] for q in self.parts)),encoding='utf8')
  row=dict(id=ident,name=name,title=title,file=str(f.resolve()),bounds_cm=[v.min(0).tolist(),v.max(0).tolist()],materials=list(dict.fromkeys(q[0] for q in self.parts)),triangles=sum(len(q[1])-2 for q in self.parts),source_sha256=hashlib.sha256(f.read_bytes()).hexdigest(),door_leaf=False)
  rows.append(row)

def build(i):
 m=Mesh()
 if i==600: # Sofa: actual cushions, back, armrests, legs, seams.
  m.legs(1.4,.5,.25);m.box((0,0,.3),(1.65,.65,.24),'wood');m.box((0,.29,.67),(1.65,.15,.7),'fabric_dirty',.06)
  for x in [-.54,0,.54]:m.box((x,-.03,.47),(.52,.55,.19),'fabric_blue',.065);m.box((x,.2,.75),(.52,.15,.45),'fabric_dirty',.06)
  for x in [-.79,.79]:m.box((x,0,.57),(.16,.73,.45),'fabric_dirty',.06)
 elif i==601:
  m.cyl((0,0,.03),(0,0,.2),.075,'steel',28,r2=.065);m.cyl((0,0,.2),(0,0,.21),.068,'black');m.cyl((0,0,.21),(0,0,.23),.018,'black');m.pipe([(.05,0,.06),(.13,0,.07),(.13,0,.19),(.05,0,.2)],.015);m.cyl((-.055,0,.14),(-.13,0,.22),.025,'steel',16,r2=.017)
 elif i in (602,615,610):
  w,h=(.8,1.85) if i==602 else (.65,1.1) if i==615 else (.5,.7)
  m.box((0,0,h/2),(w,.55,h),'black');m.box((0,-.285,h/2),(w-.04,.03,h-.04),'steel');m.handle(w*.36,-.31,h*.55)
  for z in np.arange(.13,h-.08,.12):m.box((0,-.309,z),(w-.1,.025,.085),'black');m.controls(-w*.3,-.327,z,3);m.vents(w*.13,-.328,z-.025,w*.3,2)
  if i==602:
   for x in [-.34,.34]:
    for z in np.arange(.06,h,.06):m.cyl((x,-.32,z),(x,-.33,z),.004,'steel',8)
 elif i==603:
  m.cyl((0,0,0),(0,0,.3),.18,'wood',28,r2=.26);m.cyl((0,0,.29),(0,0,.31),.24,'black');m.cyl((0,0,.3),(0,0,.9),.025,'wood')
  for j in range(16):
   a=j*2.399;z=.42+j*.035;tip=np.array([.28*math.cos(a),.28*math.sin(a),z+.18]);m.cyl((0,0,z),tip,.009,'foliage',8,r2=.004);mid=(np.array([0,0,z])+tip)/2;side=np.array([-.055*math.sin(a),.055*math.cos(a),0]);m.poly([mid+side,tip,mid-side,[0,0,z]],[(0,1,2),(0,2,3),(2,1,0),(3,2,0)],'foliage')
 elif i==604:
  m.box((0,0,.9),(.06,.9,1.8),'white');m.cyl((-.04,-.38,.05),(-.04,-.38,1.76),.018,'steel');m.box((-.055,.34,.95),(.03,.12,.05),'steel')
 elif i==605:
  m.box((-.03,0,.015),(.34,.15,.03),'black',.01)
  for x in np.linspace(-.18,.12,13):
   for y in np.linspace(-.055,.05,5):m.box((x,y,.035),(.018,.018,.01),'white',.003)
  m.box((.21,0,.02),(.07,.1,.04),'black',.015);m.box((.21,-.02,.042),(.004,.04,.003),'steel')
 elif i==606:
  m.cyl((0,0,.72),(0,0,.77),.65,'wood',48);m.cyl((0,0,.08),(0,0,.72),.065,'steel');m.cyl((0,0,.04),(0,0,.08),.37,'steel',32)
 elif i in (607,625):
  m.box((0,0,.2),(.5,.12,.4),'white');m.box((0,-.067,.23),(.3,.012,.16),'black')
  for x in [-.13,0,.13]:m.cyl((x,-.065,.09),(x,-.086,.09),.027,'red' if x==0 else 'phosphor')
  m.pipe([(.17,-.08,.12),(.19,-.15,-.06),(.12,-.16,-.12)],.007)
 elif i==608:
  m.cyl((0,0,.03),(0,0,.1),.28,'steel');m.box((0,0,.6),(.15,.2,1.05),'white');m.monitor(1.15,.52);m.box((0,-.13,.85),(.52,.34,.05),'steel');m.controls(-.15,-.31,.87)
 elif i in (609,616):
  m.legs(1.4,.65,.2)
  for x in [-.48,0,.48]:m.cyl((x,0,.22),(x,0,1.25),.16,'steel',28);m.cyl((x,0,.2),(x,0,.25),.17,'black');m.pipe([(x,0,1.25),(x,0,1.36),(x,-.36,1.36),(x,-.36,.38)],.022,'steel')
  m.pipe([(-.7,-.36,.38),(.7,-.36,.38)],.025,'steel');m.box((0,-.4,.8),(.28,.08,.32));m.controls(-.08,-.45,.8,4)
 elif i in (611,612,628):
  m.box((0,0,.65),(1.05,.72,1.3),'white',.045);m.box((0,-.37,1.13),(.93,.035,.19),'steel');m.controls(-.32,-.4,1.14,5);m.box((.28,-.4,1.14),(.2,.02,.08),'monitor_trace')
  if i==628:
   m.box((0,-.39,.58),(.91,.05,.87),'steel');m.handle(0,-.43,.99,.65)
  else:
   m.ring((0,-.4,.6),.35,.038);m.cyl((0,-.375,.6),(0,-.39,.6),.3,'black',40);m.cyl((0,-.395,.6),(0,-.405,.6),.25,'frosted_glass',40);m.handle(.32,-.43,.6,.07)
  m.vents(0,-.373,.08,.8,3)
 elif i==613:
  for y in np.arange(-1,1.01,.07):m.box((0,y,.03),(1.1,.018,.055),'steel',.003)
  for x in [-.55,.55]:m.box((x,0,.03),(.035,2,.06),'steel')
 elif i==614:m.box((0,0,.2),(.55,.55,.4),'fabric_blue',.08)
 elif i==617:
  m.trolley();m.monitor(1.25,.47);m.box((0,-.12,.95),(.57,.45,.08),'white');m.controls(-.18,-.35,.96,7);m.pipe([(.26,-.15,.91),(.36,-.3,.75),(.36,-.3,.35),(.2,-.15,.45)],.012);m.box((.3,-.15,.65),(.06,.05,.15),'white')
 elif i==618:
  for x in [-.07,.07]:m.cyl((x,0,.15),(x,0,1.15),.018,'wood');m.box((x,0,.12),(.1,.025,.23),'steel')
 elif i==619:
  m.trolley();m.monitor(1.14,.38)
  for x in [-.2,.2]:m.cyl((x,.24,.25),(x,.24,1),.075,'steel');m.cyl((x,.24,1),(x,.24,1.08),.022,'black')
  m.pipe([(.25,0,1),(.4,-.1,1.05),(.42,-.2,.55),(.28,-.26,.5)],.035);m.cyl((-.2,-.27,.72),(-.2,-.31,.72),.09,'frosted_glass')
 elif i==620:
  m.legs(.8,.5,.2);m.box((0,.1,.85),(.9,.15,1.1),'steel')
  for x in [-.32,-.1,.1,.32]:m.pipe([(x,0,.3),(x,0,1.25)],.024,'steel');m.ring((x,-.04,.8),.07,.012,'red');m.cyl((x,-.04,1.1),(x,-.09,1.1),.052,'white')
 elif i==621:
  m.box((0,.12,.07),(.85,.7,.14),'steel');m.cyl((0,.2,.14),(0,.2,1.8),.055);m.cyl((0,.2,1.5),(0,-.45,1.5),.045);m.cyl((0,-.45,1.4),(0,-.45,1.65),.16,'white',32);m.box((0,-.45,1.33),(.26,.26,.12),'black');m.box((0,-.3,.8),(.65,.09,.72),'white')
 elif i==622:
  m.wheels();m.legs(.5,.4,1.2)
  for z in [.2,.55,.9]:m.box((0,0,z),(.6,.5,.06),'steel');m.box((0,-.04,z+.1),(.48,.32,.15),'white');m.controls(-.16,-.21,z+.1,6)
  m.monitor(1.3,.45);m.pipe([(.25,0,1.1),(.4,-.1,.9),(.4,-.2,.3),(.2,-.2,.2)],.018)
 elif i==623:
  m.box((0,0,.05),(.3,.2,.1),'steel');m.pipe([(-.12,0,.1),(-.25,.06,.15),(-.25,-.1,.15),(.12,-.08,.1)],.012);m.cyl((.12,-.08,.1),(.19,-.08,.1),.028,'black')
 elif i==624:
  m.legs(1,.4,.6);m.box((0,0,.69),(1.1,.6,.18),'wood');
  for x in np.linspace(-.5,.5,28):m.box((x,-.13,.8),(.032,.3,.025),'white',.002)
  for x in np.linspace(-.46,.46,17):m.box((x,-.05,.83),(.015,.13,.045),'black',.002)
 elif i==626:
  m.cyl((-.3,-.25,.12),(.3,-.25,.12),.12,'rubber');m.box((0,0,.45),(.64,.65,.35),'steel',.06);m.pipe([(-.28,.5,.8),(-.28,.2,.35),(-.28,-.25,.12),(.28,-.25,.12),(.28,.2,.35),(.28,.5,.8)],.023,'steel')
 elif i==627:
  m.box((0,0,.055),(.13,.07,.11),'white',.015);m.box((0,-.04,.07),(.08,.008,.04),'monitor_trace');m.pipe([(0,0,.1),(.1,0,.2),(.18,0,.15),(.14,.04,.06)],.005);m.cyl((.14,.04,.05),(.14,.04,.06),.035,'white')
 elif i in (629,631):
  # Hollow gantry with separate inner wall; no opaque box across the bore.
  z=1.1;m.ring((0,0,z),.73,.28,'white',n=64);m.ring((0,-.16,z),.52,.035,'steel',n=64)
  m.box((0,.28,.3),(1.85,.85,.55),'white',.08);m.box((0,-.65,.45),(.5,1.5,.75),'steel',.06);m.box((0,-.75,.86),(.55,2.1,.08),'white',.035);m.box((0,-1.32,.92),(.5,.8,.08),'fabric_blue',.025)
  for x in [-.72,.72]:m.box((x,-.29,1.35),(.15,.025,.22),'black');m.controls(x-.045,-.32,1.35,3)
  if i==631:m.ring((0,.4,z),.73,.28,'white',n=64)
 elif i==630:
  m.box((0,0,.13),(.3,.2,.26),'white',.025);m.box((0,-.11,.17),(.18,.01,.1),'monitor_trace');m.pipe([(.14,0,.15),(.26,0,.2),(.28,-.1,.06),(.15,-.18,.05)],.013);m.cyl((.15,-.18,.05),(.15,-.26,.05),.022,'white')
 elif i==632:
  for x in [-1,1]:m.box((x,0,.25),(.12,2.5,.5),'white')
  for y in [-1.2,1.2]:m.box((0,y,.25),(2,.12,.5),'white')
  m.box((0,0,.03),(2,2.5,.06),'fabric_blue');m.box((0,0,.12),(1.85,2.32,.025),'water')
 elif i==633:
  m.legs(2.6,2.6,2.5)
  for x in [-1.35,1.35]:m.box((x,0,1),( .08,2.7,.08),'wood')
  for y in [-1.35,1.35]:m.box((0,y,1),(2.7,.08,.08),'wood')
  m.poly([[-1.65,-1.65,2.5],[1.65,-1.65,2.5],[1.65,1.65,2.5],[-1.65,1.65,2.5],[0,0,3.2]],[(0,1,4),(1,2,4),(2,3,4),(3,0,4)],'steel')
 elif i==634:
  m.cyl((0,0,0),(0,0,2.5),.035);m.ring((.3,0,2.5),.15,.01,'steel',axis='y');m.cyl((.3,-.01,2.5),(.3,1,2.4),.15,'red',24,r2=.045)
 elif i==638:m.pipe([(-.05,0,.02),(0,0,.07),(0,0,.6),(.035,0,.66),(.11,0,.66)],.018,'steel')
 elif i==639:
  m.cyl((0,0,.005),(0,0,.025),.12,'rubber',32);m.cyl((0,0,.025),(0,0,.055),.115,'steel',32,r2=.10);m.cyl((0,0,.055),(0,0,.13),.10,'steel',32,r2=.045);m.cyl((0,0,.13),(0,0,.15),.03,'black',24)
 elif i in (635,636,637):
  m.cyl((0,0,0),(0,0,.22),.82,'steel',48);m.cyl((0,0,.22),(0,0,2.2),.33,'frosted_glass',48);m.cyl((0,0,2.2),(0,0,2.4),.7,'steel',48)
  for z in [.3,.65,1,1.35,1.7,2.05]:m.ring((0,0,z),.36,.025,'red_lens' if i!=636 else 'steel',axis='z')
  for a in np.arange(0,2*math.pi,math.pi/3):
   x,y=.6*math.cos(a),.6*math.sin(a);m.cyl((x,y,.22),(x,y,2.3),.055,'steel');m.pipe([(x,y,.3),(x*1.25,y*1.25,.6),(x*1.25,y*1.25,2),(x,y,2.2)],.023)
  m.box((0,-.72,1),(.38,.18,.45),'black');m.box((0,-.82,1.04),(.28,.015,.21),'monitor_trace');m.controls(-.1,-.83,.89,5)
 else:raise ValueError(i)
 return m

titles=['Sofa','Electric kettle','Server rack','Potted plant','Sanitary partition','Keyboard and mouse','Round table','Nurse call panel','Patient terminal','Water treatment','UPS battery','Industrial washer','Industrial dryer','Service grating','Upholstered pouf','Network cabinet','Filter bank','Ultrasound cart','Garden tools','Anaesthesia machine','Medical gas manifold','X ray unit','Endoscopy tower','Endoscope','Therapy keyboard','Sensory panel','Garden barrow','Holter recorder','Dishwasher','CT scanner','Spirometer','MRI scanner','Therapy pool','Gazebo','Windsock','Helios transport core','Containment chamber','Helios reactor']
titles+=['Service prybar','Desk bell']
for i,title in enumerate(titles,600):build(i).write(i,title)
(OUT/'manifest.json').write_text(json.dumps(dict(version=21,assets=rows),ensure_ascii=False,indent=2),encoding='utf8')
# Complete coverage of existing scheduled types, retaining physical envelopes.
bindings=json.loads((P/'Content/CampaignData/echo_custom_bindings.json').read_text());extra={181:94,176:138,113:231,131:178,177:121,28:600,119:229,104:143,126:601,86:602,163:85,101:156,152:603,158:483,116:604,81:605,89:320,18:606,182:318,44:607,82:608,96:609,88:610,128:611,129:612,173:613,30:614,87:615,99:174,148:478,149:493,97:616,50:617,142:481,141:448,146:134,153:618,46:619,143:470,100:620,49:621,55:622,56:623,77:624,76:625,52:204,74:314,147:134,154:626,53:627,124:628,47:629,54:630,48:631,73:632,75:244,139:452,140:450,144:633,155:634,187:635,186:636,185:637}
extra[141]=447
bindings['prototype_models'].update({str(k):v for k,v in extra.items()});bindings['version']=21
bindings['limits']=['Every scheduled prototype type has a fitted native mesh; original collision stays unchanged.','Additional meshes are first-pass original art, not final high-detail production assets.','Rigged characters keep existing gameplay and await complete art/animation.']
(P/'Content/CampaignData/echo_custom_bindings.json').write_text(json.dumps(bindings,ensure_ascii=False,indent=2),encoding='utf8')
d=json.loads((P/'Content/CampaignData/hospital_world.json').read_text());counts=collections.Counter(str(x['model']) for x in d['prototype_props']+d['environment_props']);assert not set(counts)-set(bindings['prototype_models'])
print(json.dumps(dict(meshes=len(rows),triangles=sum(x['triangles'] for x in rows),scheduled_types=len(counts),scheduled_instances=sum(counts.values()),unresolved=0)))
