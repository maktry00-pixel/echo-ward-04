"""Metric, original medical and maintenance meshes; retain all previous art."""
from pathlib import Path
import json, math
import numpy as np
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard'
scope={'__file__':str(R/'tools/build_echo_supplements_v21.py')}
exec((R/'tools/build_echo_supplements_v21.py').read_text().split('def build(i):')[0],scope)
Mesh=scope['Mesh'];O=P/'RawAssets/EchoDetailV27';O.mkdir(exist_ok=True);scope['OUT']=O;scope['rows']=[]
scope['M'].update({k:'D27_'+k for k in ['ivory','alloy','dark','rubber','teal','paper','red','amber','screen','trace','linen','glass']})

def screw(m,x,y,z,r=.003):
 m.cyl((x,y+.001,z),(x,y-.002,z),r,'alloy',12)
 m.box((x,y-.0025,z),(r*1.4,.001,r*.22),'dark',.0001)

def gauge(m,x,y,z,r=.027):
 m.cyl((x,y+.007,z),(x,y,z),r,'alloy',32);m.cyl((x,y-.001,z),(x,y-.003,z),r*.85,'paper',32)
 for a in np.linspace(-.25*math.pi,1.25*math.pi,13):
  m.cyl((x+math.cos(a)*r*.67,y-.004,z+math.sin(a)*r*.67),(x+math.cos(a)*r*.8,y-.004,z+math.sin(a)*r*.8),r*.022,'dark',6)
 m.pipe([(x,y-.005,z),(x+r*.45,y-.005,z+r*.38)],r*.025,'red')

def display(m,x,y,z,w=.28,h=.16):
 m.box((x,y,z),(w+.018,.012,h+.018),'dark',.006);m.box((x,y-.007,z),(w,.002,h),'screen',.003)
 for row in range(3):
  yy=y-.0085;zz=z+h*(.3-row*.27)
  pts=[]
  for i in range(70):
   t=i/69;phase=(t*3.3+row*.21)%1;pulse=.035*math.sin(t*44)+(.68 if .39<phase<.44 else -.24 if .44<phase<.49 else .10 if .34<phase<.39 else 0)
   pts.append((x-w*.45+t*w*.69,yy,zz+pulse*h*.2))
  m.pipe(pts,.0008,'trace' if row!=1 else 'amber')
 for j in range(5):m.box((x+w*.35,y-.009,z+h*.35-j*h*.16),(w*.13,.001,h*.025),'trace',.0001)

def monitor():
 m=Mesh();m.box((0,0,.035),(.28,.17,.035),'dark',.012);m.box((0,.025,.09),(.05,.045,.11),'alloy',.012)
 m.box((0,0,.23),(.40,.115,.27),'ivory',.025);m.box((0,-.062,.23),(.37,.018,.245),'dark',.013)
 display(m,-.023,-.074,.246,.265,.177)
 for z in [.16,.205,.25,.295]:m.cyl((.146,-.074,z),(.146,-.084,z),.009,'ivory',16)
 m.cyl((.144,-.075,.12),(.144,-.09,.12),.017,'teal',24)
 for x in np.linspace(-.15,.15,18):m.box((x,.058,.26),(.004,.004,.11),'dark',.001)
 for x in [-.165,.165]:
  for z in [.125,.33]:screw(m,x,-.074,z)
 m.pipe([(-.1,.045,.34),(-.1,.045,.39),(.1,.045,.39),(.1,.045,.34)],.012,'dark')
 for i in range(3):
  x=-.11+i*.045;m.cyl((x,-.062,.11),(x,-.084,.11),.008,'teal',16)
 # The scheduled envelope includes a mobile stand (1.25 m), rather than a
 # desktop display. Raise the head and model the stand instead of stretching it.
 for i,(mat,pts,ns,uv) in enumerate(m.parts):m.parts[i]=(mat,pts+np.array([0,0,.86]),ns,uv)
 m.cyl((0,.025,.13),(0,.025,.97),.026,'alloy',28)
 for i in range(5):
  a=i*math.tau/5;x=.23*math.cos(a);y=.23*math.sin(a)
  m.pipe([(0,.025,.16),(x,y,.09)],.023,'alloy');m.cyl((x-.025,y,.045),(x+.025,y,.045),.045,'rubber',24)
 m.box((0,-.02,.80),(.27,.19,.018),'ivory',.009)
 return m

def oxygen():
 m=Mesh();m.cyl((0,0,.045),(0,0,1.12),.105,'teal',48);m.cyl((0,0,1.12),(0,0,1.2),.105,'teal',48,r2=.035)
 m.cyl((0,0,1.2),(0,0,1.28),.022,'alloy',24);m.cyl((0,0,1.265),(.11,0,1.265),.014,'alloy',16)
 m.ring((.115,0,1.265),.035,.007,'red',axis='y',n=32);gauge(m,-.065,-.025,1.29,.034)
 m.box((0,-.107,.77),(.12,.003,.19),'paper',.002)
 for z in [.32,.88]:m.ring((0,0,z),.109,.012,'dark',axis='z',n=40)
 m.pipe([(.09,0,1.24),(.19,-.04,1.18),(.21,-.1,.42),(.15,-.15,.26),(-.03,-.14,.29)],.006,'rubber')
 return m

def ivstand():
 m=Mesh();m.cyl((0,0,.12),(0,0,1.9),.015,'alloy',24)
 for i in range(5):
  a=i*math.tau/5;x=.28*math.cos(a);y=.28*math.sin(a);m.pipe([(0,0,.15),(x,y,.09)],.018,'alloy');m.cyl((x-.018,y,.045),(x+.018,y,.045),.042,'rubber',20)
 m.cyl((0,0,.85),(0,0,.91),.025,'dark',24);m.cyl((0,0,.88),(.06,0,.88),.008,'dark',16)
 m.pipe([(-.21,0,1.86),(-.23,0,1.94),(-.16,0,1.99),(.16,0,1.99),(.23,0,1.94),(.21,0,1.86)],.008,'alloy')
 m.box((-.17,0,1.68),(.115,.032,.22),'glass',.022);m.box((-.17,-.018,1.70),(.075,.002,.10),'paper',.002)
 m.pipe([(-.17,0,1.56),(-.17,0,1.50),(-.14,0,1.45),(-.10,-.04,.75),(.08,-.14,.43)],.0025,'glass');m.cyl((-.17,0,1.49),(-.17,0,1.54),.01,'glass',16)
 return m

def defibrillator():
 m=Mesh();m.box((0,0,.13),(.37,.24,.24),'ivory',.026);m.box((0,-.127,.145),(.34,.014,.19),'teal',.009);display(m,-.055,-.139,.16,.17,.105)
 for x in [.067,.111,.149]:m.cyl((x,-.14,.10),(x,-.15,.10),.012,'red' if x==.149 else 'ivory',18)
 m.pipe([(-.12,0,.24),(-.12,0,.3),(.12,0,.3),(.12,0,.24)],.015,'dark')
 for side in [-1,1]:
  x=side*.155;m.box((x,.035,.26),(.055,.13,.035),'alloy',.01);m.box((x,.035,.292),(.035,.06,.035),'dark',.009)
  pts=[(side*(.15+.022*math.sin(i*.9)),.05+.022*math.cos(i*.9),.20-i*.0025) for i in range(52)];m.pipe(pts,.0035,'rubber')
 return m

def pump():
 m=Mesh();m.box((0,0,.13),(.17,.12,.25),'ivory',.02);display(m,-.01,-.067,.18,.10,.055)
 m.box((-.012,-.067,.08),(.1,.012,.09),'teal',.006)
 for z in [.05,.08,.11]:m.box((-.015,-.075,z),(.058,.005,.004),'dark',.001)
 for z in [.04,.075,.11,.145,.18]:m.cyl((.064,-.065,z),(.064,-.073,z),.006,'amber' if z==.04 else 'dark',12)
 m.pipe([(-.07,0,.22),(-.10,-.02,.23),(-.11,-.04,.05),(-.07,-.08,.01)],.003,'glass');return m

def binmesh():
 m=Mesh();m.box((0,0,.28),(.34,.31,.52),'ivory',.03);m.box((0,0,.555),(.36,.33,.055),'amber',.018)
 m.box((0,-.164,.31),(.15,.003,.17),'paper',.004);m.box((0,-.19,.04),(.15,.10,.025),'dark',.006)
 m.cyl((-.08,.17,.04),(.08,.17,.04),.025,'rubber',20)
 for x in [-.13,.13]:m.pipe([(x,.16,.44),(x,.2,.55),(x,.2,.63)],.009,'alloy')
 return m

def sink():
 m=Mesh()
 # Recessed bowl, rim and drain are geometry, with an open centre.
 for x in [-.28,.28]:m.box((x,-.015,.75),(.055,.38,.06),'alloy',.02)
 for y in [-.2,.17]:m.box((0,y,.75),(.51,.045,.06),'alloy',.017)
 m.box((0,-.015,.66),(.48,.32,.018),'alloy',.045)
 for x in [-.24,.24]:m.box((x,-.015,.7),(.018,.32,.09),'alloy',.006)
 for y in [-.16,.14]:m.box((0,y,.7),(.48,.018,.09),'alloy',.006)
 m.cyl((0,-.02,.669),(0,-.02,.676),.03,'dark',24)
 # The room schedule already owns a separate mixer at the rim height.
 # Keep the basin aperture open and avoid duplicating that scheduled fixture.
 m.pipe([(0,.10,.65),(0,.10,.48),(.08,.10,.44),(.08,.20,.44)],.025,'alloy')
 for x in [-.22,.22]:m.box((x,.21,.65),(.025,.05,.24),'alloy',.005)
 for x in [-.25,.25]:
  for y in [-.16,.15]:m.box((x,y,.34),(.025,.025,.68),'alloy',.004)
 m.pipe([(-.25,.15,.15),(.25,.15,.15)],.012,'alloy')
 return m

def panel():
 m=Mesh();m.box((0,0,.12),(.92,.09,.24),'ivory',.012);m.box((0,-.048,.18),(.86,.008,.035),'teal',.003)
 for i,x in enumerate([-.34,-.21,-.08]):
  m.cyl((x,-.05,.09),(x,-.065,.09),.025,'amber' if i==0 else 'teal',24);m.cyl((x,-.065,.09),(x,-.068,.09),.012,'dark',20)
 for x in [.07,.19,.31]:
  m.box((x,-.052,.09),(.075,.008,.075),'paper',.004)
  for s in [-1,1]:m.cyl((x+s*.014,-.058,.09),(x+s*.014,-.063,.09),.004,'dark',12)
 for x in [-.42,.42]:screw(m,x,-.049,.1)
 return m

def supplies():
 m=Mesh()
 for i in range(3):
  x=(i-1)*.065;m.cyl((x,0,.005),(x,0,.115+i*.012),.025,'glass' if i==0 else 'ivory',24);m.cyl((x,0,.115+i*.012),(x,0,.13+i*.012),.022,'teal',24)
  m.box((x,-.025,.065),(.034,.002,.05),'paper',.002)
 for i in range(3):m.box((.02,-.085,.008+i*.012),(.15,.065,.01),'paper',.004);m.box((.02,-.119,.01+i*.012),(.11,.001,.004),'teal',.0005)
 return m

def pipekit():
 m=Mesh()
 for i,x in enumerate([-.12,0,.12]):
  m.pipe([(x,-1.46,.16),(x,1.2,.16),(x,1.38,.25),(x,1.38,.42)],.025 if i==0 else .017,'alloy' if i!=1 else 'teal')
  for y in [-1.12,0,.95]:m.cyl((x,y-.027,.16),(x,y+.027,.16),.033,'dark',20)
 for y in [-1.05,1.05]:
  m.box((0,y,.1),(.45,.045,.025),'alloy',.004)
  for x in [-.2,.2]:m.cyl((x,y,.1),(x,y,.49),.006,'alloy',12)
 return m

def hosecabinet():
 m=Mesh();m.box((0,.03,.35),(.58,.14,.7),'red',.015);m.box((0,-.048,.35),(.52,.012,.64),'dark',.01)
 for r in np.linspace(.07,.235,8):m.ring((-.02,-.065,.35),float(r),.012,'linen',axis='y',n=44)
 m.pipe([(.18,-.065,.2),(.21,-.065,.1),(.13,-.065,.06)],.018,'linen');m.cyl((.13,-.065,.06),(.01,-.065,.06),.023,'alloy',20)
 m.box((.23,-.073,.35),(.025,.025,.14),'alloy',.007);return m

def files():
 m=Mesh()
 for j in range(5):
  x=(j-2)*.045;m.box((x,0,.145),(.041,.24,.29),'teal' if j%2 else 'linen',.004);m.box((x,-.124,.19),(.026,.003,.07),'paper',.001)
  m.cyl((x,-.123,.055),(x,-.13,.055),.007,'dark',16)
 return m

def linen():
 m=Mesh()
 for i in range(4):
  m.box((0,0,.025+i*.046),(.37-i*.008,.28-i*.006,.043),'linen',.018)
  for y in [-.122,.122]:m.pipe([(-.16,y,.028+i*.046),(.16,y,.028+i*.046)],.0014,'paper')
 return m

def dispenser():
 m=Mesh();m.box((0,0,.13),(.115,.08,.25),'ivory',.025);m.box((0,-.044,.14),(.07,.008,.095),'glass',.013);m.box((0,-.052,.055),(.085,.025,.055),'dark',.008)
 m.box((0,-.064,.04),(.065,.003,.027),'teal',.006);m.pipe([(0,0,.245),(0,0,.285),(0,-.12,.285)],.009,'alloy');return m

def mixer():
 m=Mesh();m.cyl((0,-.07,0),(0,-.07,.025),.017,'alloy',36)
 pts=[(0,-.07,.02),(0,-.07,.245)]
 for a in np.linspace(math.pi,math.pi/2,14)[1:]:pts.append((0,.0+.07*math.cos(a),.245+.07*math.sin(a)))
 pts.extend([(0,.09,.315),(0,.09,.28)]);m.pipe(pts,.012,'alloy');m.cyl((0,.09,.27),(0,.09,.285),.013,'dark',24)
 m.pipe([(0,-.07,.08),(0,-.098,.095),(0,-.098,.17)],.005,'alloy');m.box((0,-.10,.17),(.015,.006,.018),'red',.002);return m

models=[(900,'Patient monitor with controls and ECG lines',monitor()),(901,'Oxygen cylinder with regulator and hose',oxygen()),(902,'Five-leg IV stand with bag and drip line',ivstand()),(903,'Defibrillator with coiled leads and paddles',defibrillator()),(904,'Infusion pump with tube and buttons',pump()),(905,'Clinical pedal bin',binmesh()),(906,'Recessed stainless washbasin and mixer',sink()),(907,'Bedhead medical gas and electrical panel',panel()),(908,'Medical bottle and sterile packet set',supplies()),(909,'Ceiling services with collars and hangers',pipekit()),(910,'Fire hose cabinet',hosecabinet()),(911,'Five medical archive binders',files()),(912,'Folded linen with edge seams',linen()),(913,'Wall sanitiser dispenser',dispenser())]
models.append((914,'Curved separate medical mixer and lever',mixer()))
for ident,title,m in models:
 assert all(np.isfinite(pts).all() and np.isfinite(ns).all() for mat,pts,ns,uv in m.parts),ident
 m.write(ident,title)
(O/'manifest.json').write_text(json.dumps(dict(version=27,assets=scope['rows']),ensure_ascii=False,indent=2,allow_nan=False),encoding='utf8')
print(json.dumps([dict(id=a['id'],triangles=a['triangles'],title=a['title']) for a in scope['rows']]))
