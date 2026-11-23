"""A controlled medical campus, distinct departments and full-size prototype kits.

All v13 metric rooms, story actors and existing furniture are conserved. New
physical parts share their bounds with the independent navigation raster.
"""
import collections, csv, hashlib, json, math, shutil
from pathlib import Path
from hospital_clutter_v10 import Space, bounds, rotate
from hospital_furnishing_v6 import Index, overlap
from hospital_services_v13 import equipment_shape, text_rectangles
from hospital_item_types_v12 import refresh_navigation

ROOT=Path(__file__).resolve().parents[1]
PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
OUT=ROOT/'artifacts/unreal_hospital_v14'
SOURCE='hospital_campus_v14'

def shape(kind):
    p=[]
    def b(x,y,z,w,d,h,m='camp_steel'):p.append([x,y,z,w,d,h,m])
    def legs(w,d,h):
        for x in (-w/2+.04,w/2-.04):
            for y in (-d/2+.04,d/2-.04):b(x,y,h/2,.05,.05,h)
    def top(w,d,h,m='camp_wood'):legs(w,d,h);b(0,0,h,w,d,.06,m)
    def wheels(w,d):
        for x in (-w/2+.08,w/2-.08):
            for y in (-d/2+.08,d/2-.08):b(x,y,.10,.10,.16,.20,'camp_rubber')
    if kind=='sensory_station':
        top(1.30,.60,.72);b(0,-.26,1.04,1.3,.06,.65,'camp_sage')
        for x in (-.45,-.15,.15,.45):
            for z in (.89,1.10):b(x,-.21,z,.12,.06,.12,'camp_cream' if x<0 else 'camp_teal')
    elif kind=='therapy_bench':
        top(1.65,.60,.44,'camp_sage');b(0,-.26,.72,1.65,.10,.55,'camp_sage')
        b(0,0,.49,.42,.36,.06,'camp_cloth');b(.47,0,.49,.42,.36,.06,'camp_cloth')
    elif kind=='therapy_cart':
        wheels(.65,.5);legs(.65,.5,.70)
        for z in (.25,.70):b(0,0,z,.65,.50,.04,'camp_wood')
        for x in (-.20,0,.20):b(x,0,.79,.12,.15,.14,'camp_sage')
        b(0,-.22,.99,.65,.04,.38,'camp_cream')
    elif kind=='exercise_cycle':
        b(0,0,.07,.65,1.20,.14,'camp_rubber');b(0,-.15,.45,.26,.55,.65,'camp_teal')
        b(0,.25,.56,.07,.07,1.00);b(0,.25,1.05,.40,.28,.08,'camp_rubber')
        b(0,-.45,.97,.06,.06,.65);b(0,-.45,1.30,.60,.06,.06)
        for x in (-.28,.28):b(x,.05,.30,.20,.16,.06,'camp_rubber')
    elif kind=='gait_rail':
        for x in (-.55,.55):
            for y in (-1.1,1.1):b(x,y,.47,.06,.06,.94)
            b(x,0,.97,.07,2.4,.07)
        b(0,0,.025,1.4,2.5,.05,'camp_rubber')
    elif kind=='balance_course':
        for x,z in ((-.55,.06),(0,.12),(.55,.06)):b(x,0,z,.42,.65,z*2,'camp_teal')
    elif kind=='domestic_kitchen':
        b(0,0,.44,1.30,.60,.88,'camp_cream');b(0,0,.91,1.36,.66,.06,'camp_steel')
        for x in (-.4,0,.4):b(x,.31,.70,.21,.015,.035,'camp_rubber')
        b(.35,0,.95,.40,.40,.02,'camp_rubber');b(-.35,0,.95,.42,.40,.02,'camp_teal')
        b(-.35,-.18,1.07,.04,.04,.24)
    elif kind=='personal_locker':
        for x in (-.25,.25):
            b(x,0,.95,.48,.52,1.90,'camp_cream');b(x,.269,.95,.43,.018,1.80,'camp_ochre')
            b(x+.12,.29,1.06,.025,.025,.12,'camp_rubber')
            for z in (1.58,1.65,1.72):b(x,.282,z,.26,.006,.018,'camp_rubber')
    elif kind=='notice_stand':
        for x in (-.43,.43):b(x,0,.95,.055,.055,1.90);b(x,0,.03,.16,.48,.06)
        b(0,0,1.40,.96,.06,.80,'camp_sage')
        for x in (-.25,0,.25):b(x,.035,1.40,.20,.008,.34,'camp_cream')
    elif kind=='laundry_sorter':
        for x in (-.42,.42):b(x,0,.48,.70,.65,.72,'camp_cloth');b(x,0,.08,.76,.70,.08);wheels(.76,.70)
        b(0,-.30,.72,1.70,.04,.05)
    elif kind=='compressor':
        b(0,0,.30,1.50,.65,.45,'camp_teal');legs(1.5,.65,.12)
        b(-.4,0,.67,.58,.50,.35,'camp_steel');b(.42,0,.63,.42,.45,.28,'camp_rubber')
        for y in (-.25,.25):b(0,y,.81,.06,.05,.30)
        b(.70,.10,.72,.13,.12,.17,'camp_cream')
    elif kind=='duct_rack':
        legs(1.5,.65,1.8)
        for z in (.22,.90,1.65):
            b(0,0,z,1.6,.70,.06);b(-.36,0,z+.23,.55,.55,.40,'camp_steel');b(.36,0,z+.23,.55,.55,.40,'camp_cream')
    elif kind=='pump_skid':
        b(0,0,.08,1.50,.90,.16,'camp_rubber')
        for x in (-.4,.4):b(x,0,.48,.44,.60,.68,'camp_teal');b(x,-.25,.96,.12,.12,.40);b(x,.32,.56,.50,.06,.12)
        b(0,-.25,1.13,1.1,.12,.12)
    elif kind=='tool_board':
        top(1.5,.65,.83);b(0,-.28,1.35,1.5,.05,.90,'camp_sage')
        for x in (-.5,-.25,0,.25,.5):b(x,-.24,1.38,.035,.04,.34,'camp_steel');b(x,-.22,1.20,.10,.07,.10,'camp_rubber')
        b(.47,.10,.93,.25,.28,.15,'camp_steel')
    elif kind=='trolley_bay':
        wheels(.72,1.85);legs(.72,1.85,.65);b(0,0,.70,.82,1.94,.12,'camp_steel');b(0,0,.80,.76,1.85,.12,'camp_cloth')
        for x in (-.41,.41):b(x,0,.96,.035,1.80,.035);b(x,-.65,.84,.035,.035,.24);b(x,.65,.84,.035,.035,.24)
        b(0,-.96,1.01,.80,.04,.45)
    elif kind=='electric_tug':
        wheels(.80,1.50);b(0,0,.37,.90,1.7,.40,'camp_ochre');b(0,-.52,.72,.84,.50,.56,'camp_ochre')
        b(0,.38,.74,.60,.50,.20,'camp_rubber');b(0,-.3,1.03,.08,.08,.26);b(0,-.3,1.18,.46,.08,.08)
        b(0,-.78,.73,.60,.06,.13,'camp_cream')
    elif kind=='charger_station':
        b(0,0,.70,.9,.45,1.4,'camp_cream');b(0,.24,.99,.64,.03,.32,'camp_teal')
        for x in (-.27,.27):b(x,.27,.64,.08,.04,.20,'camp_rubber');b(x,.3,.28,.025,.025,.50,'camp_rubber')
    elif kind=='retired_scanner':
        b(0,-.42,.66,1.50,.70,1.32,'camp_cream')
        for x in (-.60,.60):b(x,0,1.50,.30,.70,1.40,'camp_cream')
        b(0,0,2.11,1.5,.70,.22,'camp_cream');b(0,.72,.58,.58,1.7,.15,'camp_steel');legs(.58,1.7,.5)
        b(0,-.82,1.15,.9,.04,.35,'camp_ochre')
    elif kind=='scrap_cage':
        b(0,0,.08,2.5,1.6,.16,'camp_wood')
        for x in (-1.22,1.22):
            for y in (-.77,.77):b(x,y,1.05,.06,.06,2.0)
        for z in (.3,1,1.8):
            for y in (-.77,.77):b(0,y,z,2.50,.04,.04)
            for x in (-1.22,1.22):b(x,0,z,.04,1.6,.04)
        for x in (-.8,0,.8):b(x,0,.45,.55,1,.55,'camp_cream');b(x,0,1.1,.4,.7,.45,'camp_rubber')
    elif kind=='pallet_rack':
        for x in (-1.3,1.3):
            for y in (-.55,.55):b(x,y,1.30,.08,.08,2.6,'camp_ochre')
        for z in (.18,1.3,2.45):
            b(0,0,z,2.7,1.2,.12,'camp_steel')
            if z<2: 
                for x in (-.85,0,.85):b(x,0,z+.35,.72,1,.58,'camp_crate')
    elif kind=='guard_desk':
        top(1.45,.65,.75);b(-.35,-.12,1.07,.48,.08,.32,'camp_rubber');b(-.35,-.07,1.07,.42,.01,.25,'camp_teal')
        b(-.35,-.12,.88,.08,.08,.12);b(.38,.05,.84,.20,.16,.12,'camp_rubber')
        b(.52,.05,1.02,.025,.025,.25,'camp_steel');b(0,.20,.80,.45,.16,.025,'camp_rubber')
    elif kind=='weapon_rack':
        b(0,0,.09,1.25,.55,.18,'camp_steel');b(0,-.20,.88,1.25,.08,1.58,'camp_sage')
        for x in (-.4,0,.4):
            b(x,0,.77,.065,.09,.70,'camp_rubber');b(x,0,1.30,.035,.035,.36)
            b(x,.02,.38,.13,.10,.24,'camp_wood');b(x,.08,.72,.12,.08,.06,'camp_rubber')
        b(0,.17,.15,1.22,.06,.05,'camp_ochre')
    elif kind=='security_supplies':
        b(0,0,.23,1.10,.60,.46,'camp_sage')
        for x in (-.30,.25):b(x,0,.50,.35,.36,.08,'camp_rubber');b(x,0,.54,.19,.19,.02,'camp_ochre')
        b(-.30,.31,.23,.40,.01,.12,'camp_cream')
    elif kind=='planter':
        b(0,0,.22,1.6,.85,.44,'camp_concrete');b(0,0,.45,1.46,.72,.04,'camp_soil')
        for x in (-.5,0,.5):b(x,0,.68,.48,.55,.45,'camp_leaf');b(x,.1,.92,.3,.35,.22,'camp_leaf')
    elif kind=='bench':
        legs(1.8,.52,.42)
        for y in (-.20,0,.20):b(0,y,.46,1.8,.15,.08,'camp_wood')
        for z in (.68,.90):b(0,-.24,z,1.8,.08,.16,'camp_wood')
    elif kind=='cycle_rack':
        for x in (-.8,-.4,0,.4,.8):
            for y in (-.28,.28):b(x,y,.32,.04,.04,.64)
            b(x,0,.65,.04,.60,.04)
    elif kind=='recycling':
        for x,m in ((-.65,'camp_teal'),(0,'camp_ochre'),(.65,'camp_sage')):
            b(x,0,.46,.56,.55,.92,m);b(x,0,.95,.60,.59,.06,'camp_rubber');b(x,.28,.64,.25,.01,.17,'camp_cream')
    elif kind=='sedan':
        wheels(1.72,3.70);b(0,0,.64,1.75,4.15,.65,'camp_car');b(0,-.13,1.15,1.55,2.10,.70,'camp_car')
        b(0,-1.19,1.25,1.42,.025,.44,'camp_glass');b(0,.93,1.25,1.42,.025,.42,'camp_glass')
        for x in (-.79,.79):b(x,-.13,1.29,.02,1.85,.40,'camp_glass')
        for x in (-.57,.57):b(x,-2.09,.68,.40,.04,.15,'camp_cream');b(x,2.09,.68,.35,.04,.12,'camp_red')
        b(0,-2.11,.42,1.58,.07,.11,'camp_steel');b(0,2.11,.42,1.58,.07,.11,'camp_steel')
    elif kind=='market_crates':
        top(1.5,.7,.65)
        for x in (-.48,0,.48):
            b(x,0,.80,.43,.60,.24,'camp_crate')
            for dx in (-.12,.12):
                for y in (-.18,0,.18):b(x+dx,y,.95,.15,.15,.13,'camp_leaf' if x<0 else 'camp_ochre')
    elif kind=='pharmacy_display':
        b(0,0,.9,.90,.42,1.8,'camp_cream')
        for z in (.40,.85,1.3,1.7):
            b(0,.23,z,.85,.16,.04,'camp_steel')
            for x in (-.25,0,.25):b(x,.24,z+.12,.17,.13,.20,'camp_teal');b(x,.311,z+.12,.11,.006,.11,'camp_cream')
    elif kind=='roof_unit':
        legs(1.6,1.1,.20);b(0,0,.80,1.6,1.1,1.2,'camp_steel')
        for x in (-.50,0,.50):b(x,.561,.8,.06,.015,.9,'camp_rubber')
        b(0,0,1.45,1.7,1.2,.1,'camp_cream')
    else:
        return equipment_shape(kind)
    # Placement bounds must match the geometry, including projecting handles.
    centre=[(min(a[i]-a[i+3]/2 for a in p)+max(a[i]+a[i+3]/2 for a in p))/2 for i in (0,1)]
    bottom=min(a[2]-a[5]/2 for a in p)
    return [[a[0]-centre[0],a[1]-centre[1],a[2]-bottom,*a[3:]] for a in p]

class Campus:
    def __init__(self,d):
        self.layer=14;self.source=SOURCE;self.prefix='CAMP14'
        self.d=d;self.e=d['environment_dressing'];self.items=[];self.requests=[];self.space=Space();self.floor=Index();self.reserved=Index();self.walls=Index()
        self.zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
        self.sites={s['id']:s for s in d['locations']}
        for k,tint in dict(steel=[.26,.30,.31],rubber=[.025,.03,.035],wood=[.32,.20,.10],sage=[.22,.33,.25],teal=[.07,.29,.33],cream=[.67,.70,.65],ochre=[.56,.36,.09],cloth=[.30,.36,.35],crate=[.37,.23,.10],concrete=[.27,.29,.28],soil=[.10,.065,.035],leaf=[.075,.18,.06],car=[.16,.22,.25],glass=[.09,.18,.21],red=[.5,.035,.025],purple=[.31,.22,.36],ink=[.025,.055,.06]).items():
            self.e['materials']['camp_'+k]=dict(tint=tint,roughness=.38 if k in ('glass','car') else .8,weathering=.30,metallic=.5 if k=='steel' else 0)
        for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):
            self.floor.add(w['z'],w['rect']);self.walls.add(w['z'],w['rect'])
        for f in d['collision_features']:self.floor.add(f['z'],f['rect'])
        for p in d['prototype_props']+d['environment_props']:
            for a in d['prototype_catalog'][str(p['model'])]['parts']:
                off=rotate(a[:3],p['turn']);at=[p['at'][i]+off[i] for i in range(3)];sz=list(a[3:6])
                if p['turn']%2:sz[0],sz[1]=sz[1],sz[0]
                self.space.add(bounds(at,sz),p['id'])
        for n,p in enumerate(self.e['boxes']):
            if p['kind'] not in ('lawn','grass_tuft','pavement','road_dash','tree_crown'):self.space.add(bounds(p['at'],p['size']),'old_'+str(n))
        def reserve(z,a,b,r=.62):self.reserved.add(z,[min(a[0],b[0])-r,min(a[1],b[1])-r,abs(a[0]-b[0])+2*r,abs(a[1]-b[1])+2*r])
        for r in d['navigation_routes']+d['bay_navigation_routes']:
            for a,b in zip(r['points'],r['points'][1:]):
                if a[2]==b[2]:reserve(a[2],a,b)
        for s,r in self.zones.values():
            ox,oy,z=s['origin'];x,y,w,h=r['rect']
            for q in [dict(at=r['door'],side=r['door_side'])]+r.get('extra_doors',[]):
                xx,yy=ox+q['at'][0],oy+q['at'][1];ns=q['side'] in ('north','south')
                reserve(z,[xx if ns else xx-1.8,yy-1.8 if ns else yy],[xx if ns else xx+1.8,yy+1.8 if ns else yy],.7)
            if r['enclosed']:reserve(z,[ox+r['door'][0],oy+r['door'][1]],[ox+x+w/2,oy+y+h/2],.68)
        for q in d['hideouts']:
            x,y,z=q['at'];w,h,_=q['size'];self.reserved.add(z,[x-.2,y-.2,w+.4,h+1.8])
        for q in d['escape_niches']:
            x,y,z=q['entry'];self.reserved.add(z,q['rect'])
            nx,ny={'north':(0,1),'south':(0,-1),'west':(1,0),'east':(-1,0)}[q['side']]
            dep=q.get('entry_depth_m',1)
            reserve(z,q.get('corridor_point',[x,y,z]),[x+nx*(dep+.12),y+ny*(dep+.12)],.7)
        for q in d['story_gameplay']['placements']+d['story_gameplay']['mobs']:
            x,y,z=q['at'];rad=1.1 if q.get('kind') in ('person','scene') or 'species' in q else .7
            self.reserved.add(z,[x-rad,y-rad,rad*2,rad*2])
        for q in d['stairs']:
            x,y,w,h=q['rect'];z=q['z_min']
            while z<q['z_max']+.01:self.reserved.add(round(z,2),[x-.6,y-.6,w+1.2,h+1.2]);z+=4.8
        for z in (-9.6,-4.8,0,4.8,9.6,14.4):self.reserved.add(z,[47.2,3.2,5.6,9.6])

    def emit(self,kind,zone,parts,at,turn=0,solid=True,strict=True,partial=False,extra=None):
        group=[]
        for a in parts:
            off=rotate(a[:3],turn);pos=[round(at[i]+off[i],6) for i in range(3)];sz=list(a[3:6])
            if turn%2:sz[0],sz[1]=sz[1],sz[0]
            if strict and self.space.hits(bounds(pos,sz)):return None
            group.append(dict(at=pos,size=sz,material=a[6],kind='campus_'+kind,zone=zone,solid=solid,layer=self.layer))
        ident=f'{self.prefix}_{kind}_{len(self.items):05}'
        bb=[min(p['at'][i]-p['size'][i]/2 for p in group) for i in range(3)]+[max(p['at'][i]+p['size'][i]/2 for p in group) for i in range(3)]
        for p in group:p['item_id']=ident;self.e['boxes'].append(p)
        item=dict(id=ident,kind=kind,zone=zone,at=at,turn=turn,bounds=bb,solid=solid,parts=len(group),**(extra or {}));self.items.append(item)
        self.space.add(bb,ident)
        if solid:
            rects=[]
            if partial:
                for p in group:
                    q=bounds(p['at'],p['size'])
                    if q[2]<at[2]+1.95 and q[5]>at[2]+.25:rects.append([q[0],q[1],q[3]-q[0],q[4]-q[1]])
            else:rects=[[bb[0],bb[1],bb[3]-bb[0],bb[4]-bb[1]]]
            floor_z=min((-9.6,-4.8,0,4.8,9.6,14.4),key=lambda z:abs(z-at[2]))
            for n,rr in enumerate(rects):
                self.d['collision_features'].append(dict(id=ident+f'_{n}',rect=rr,z=floor_z,source=self.source));self.floor.add(floor_z,rr)
            item['nav_rects']=rects
        return item

    def relocate_registration_terminal(self):
        """Keep the scheduled model/ID, but give its screen a usable approach."""
        prop=next(p for p in self.d['prototype_props'] if p['id']=='PROP6_R01_082-01')
        assert not any(q.get('parent')==prop['id'] for k in ('clutter_dressing','microdetails_dressing','item_types_dressing') for q in self.d[k]['items'])
        feature=next(f for f in self.d['collision_features'] if f['id']==prop['id']);oldrect=feature['rect']
        for k,v in self.floor.grid.items():self.floor.grid[k]=[(r,t) for r,t in v if r!=oldrect]
        for k,v in self.space.cells.items():self.space.cells[k]=[(b,i) for b,i in v if i!=prop['id']]
        parts=self.d['prototype_catalog'][str(prop['model'])]['parts'];poses=[(8.7,8.3,3)]
        poses.extend((8.65+ix*.25,1.0+iy*.25,t) for t in range(4) for ix in range(23) for iy in range(57))
        for xx,yy,t in poses:
            group=[]
            for a in parts:
                off=rotate(a[:3],t);sz=list(a[3:6])
                if t%2:sz[0],sz[1]=sz[1],sz[0]
                group.append(([xx+off[0],yy+off[1],off[2]],sz))
            bb=[min(q[0][i]-q[1][i]/2 for q in group) for i in range(3)]+[max(q[0][i]+q[1][i]/2 for q in group) for i in range(3)]
            rr=[bb[0],bb[1],bb[3]-bb[0],bb[4]-bb[1]];nx,ny,_=rotate([0,1,0],t);extent=(rr[2] if nx else rr[3])/2
            front=[xx+nx*(extent+.65),yy+ny*(extent+.65),0];access=[front[0]-.4,front[1]-.4,.8,.8]
            if not(8.2<=bb[0] and bb[3]<=14.8 and .2<=bb[1] and bb[4]<=15.8 and 8.15<=access[0] and access[0]+.8<=14.85 and .15<=access[1] and access[1]+.8<=15.85):continue
            if self.floor.hits(0,rr,.12) or self.floor.hits(0,access,.04) or self.reserved.hits(0,rr,.08) or any(self.space.hits(bounds(at,sz)) for at,sz in group):continue
            before={k:prop[k] for k in ('at','turn','rect')};prop['at']=[xx,yy,0];prop['turn']=t;prop['rect']=rr;feature['rect']=rr
            for at,sz in group:self.space.add(bounds(at,sz),prop['id'])
            self.floor.add(0,rr);self.reserved.add(0,access)
            item=dict(id=prop['id'],kind='patient_terminal',at=prop['at'],turn=t,bounds=bb,front_access=front)
            self.capture(item,'R01_terminal')
            return dict(id=prop['id'],before=before,after={k:prop[k] for k in ('at','turn','rect')},front_access=front,reason='free screen approach and all original door swing porches')
        raise RuntimeError('No full-size accessible registration terminal position')

    def fit(self,kind,zid,near=None,parts=None):
        s,r=self.zones[zid];ox,oy,z=s['origin'];x,y,w,h=r['rect'];parts=parts or shape(kind)
        pb=[min(a[i]-a[i+3]/2 for a in parts) for i in range(3)]+[max(a[i]+a[i+3]/2 for a in parts) for i in range(3)];opts=[]
        for t in range(4):
            fw,fd=pb[3]-pb[0],pb[4]-pb[1]
            if t%2:fw,fd=fd,fw
            for ix in range(max(0,math.floor((w-fw-.6)/.50)+1)):
                xx=ox+x+fw/2+.3+ix*.5
                for iy in range(max(0,math.floor((h-fd-.6)/.50)+1)):
                    yy=oy+y+fd/2+.3+iy*.5;rr=[xx-fw/2,yy-fd/2,fw,fd]
                    if self.floor.hits(z,rr,.12) or self.reserved.hits(z,rr,.08):continue
                    nx,ny,_=rotate([0,1,0],t);extent=(fw if nx else fd)/2;front=[xx+nx*(extent+.65),yy+ny*(extent+.65),z];access=[front[0]-.4,front[1]-.4,.8,.8]
                    if not(ox+x+.05<=access[0] and oy+y+.05<=access[1] and access[0]+.8<=ox+x+w-.05 and access[1]+.8<=oy+y+h-.05) or self.floor.hits(z,access,.04):continue
                    edge=min(xx-ox-x,ox+x+w-xx,yy-oy-y,oy+y+h-yy)
                    score=(xx-near[0])**2+(yy-near[1])**2 if near else edge
                    opts.append((score,xx,yy,t,front,access))
        for _,xx,yy,t,front,access in sorted(opts):
            item=self.emit(kind,zid,parts,[xx,yy,z],t,extra=dict(front_access=front,mount='floor'))
            if item:self.reserved.add(z,access);return item
        return None

    def outside(self,kind,at,turn=0,zone='ST'):
        p=shape(kind);bb=[min(a[i]-a[i+3]/2 for a in p) for i in range(2)]+[max(a[i]+a[i+3]/2 for a in p) for i in range(2)]
        w,h=bb[2]-bb[0],bb[3]-bb[1]
        if turn%2:w,h=h,w
        rr=[at[0]-w/2,at[1]-h/2,w,h]
        if self.floor.hits(0,rr,.25) or self.reserved.hits(0,rr,.3):return None
        # Do not plant props inside a building, passage, loading door or street.
        for s in self.d['locations']:
            ox,oy,z=s['origin']
            if z!=0:continue
            for r in s['rooms']:
                x,y,rw,rh=r['rect']
                if r['enclosed'] and overlap(rr,[ox+x-.7,oy+y-.7,rw+1.4,rh+1.4]):return None
            if s['id']!='ST':
                for x,y,rw,rh in s['corridors']+[q['rect'] for q in s['cores']]:
                    if overlap(rr,[ox+x-.6,oy+y-.6,rw+1.2,rh+1.2]):return None
        for x,y,rw,rh,z in self.d['passages']:
            if z==0 and overlap(rr,[x-.6,y-.6,rw+1.2,rh+1.2]):return None
        return self.emit(kind,zone,p,at,turn,extra=dict(mount='outside'))

    def sign(self,text,at,turn=0,kind='wayfinding',zone='ST'):
        p=[[0,0,0,2.5,.06,.80,'camp_cream']]
        if kind!='gate_sign':
            for x in (-1.13,1.13):p.append([x,0,-.80,.06,.06,1.60,'camp_steel'])
        for x,y,w,h in text_rectangles(text):p.append([(x+w/2-128)*.009,.037,(38-y-h/2)*.009,w*.009,.008,h*.009,'camp_ink'])
        return self.emit(kind,zone,p,at,turn,solid=False,strict=False,extra=dict(text=text,mount='sign'))

    def capture(self,item,label):
        if not item:return
        bb=item['bounds'];target=[(bb[a]+bb[a+3])/2 for a in range(3)];z=item['at'][2]
        for dx,dy in ((0,3.2),(2,3),(-2,3),(0,2.2),(1,2),(-1,2),(0,1.5),(.5,1.3),(-.5,1.3),(0,1.1)):
            vx,vy,_=rotate([dx,dy,0],item['turn']);eye=[target[0]+vx,target[1]+vy,max(z+1.65,target[2]+.4)]
            if item.get('zone') in self.zones:
                s,r=self.zones[item['zone']];ox,oy,_=s['origin'];x,y,w,h=r['rect']
                if not(ox+x+.1<eye[0]<ox+x+w-.1 and oy+y+.1<eye[1]<oy+y+h-.1):continue
            if self.walls.hits(z,[eye[0]-.08,eye[1]-.08,.16,.16]):continue
            if self.space.hits(bounds(eye,[.15]*3)):continue
            if any(self.space.hits(bounds([eye[a]+(target[a]-eye[a])*t for a in range(3)],[.025]*3),item['id']) or self.walls.hits(z,[eye[0]+(target[0]-eye[0])*t-.015,eye[1]+(target[1]-eye[1])*t-.015,.03,.03]) for t in [j/50 for j in range(1,49)]):continue
            self.d['furnishing_captures'].append(dict(id='campus_'+label,at=eye,target=target,campus=True,detail_item=item['id']));return

def apply_campus(d):
    OUT.mkdir(parents=True,exist_ok=True)
    if not (OUT/'baseline_v13.json').exists():
        assert d['review_revision']==13
        (OUT/'baseline_v13.json').write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    c=Campus(d);relocations=[c.relocate_registration_terminal()];gates=[];segments=[]
    # One internal clinical boundary. Public retail is entirely north of -50m.
    # The original exterior fence and all old site IDs are retained.
    gate_spans=[(-176,-160,'west_service'),(-59,-49,'retail_pedestrian'),(-42,-16,'visitors'),(40,60,'medical_main'),(116,142,'supplies'),(260,278,'east_emergency')]
    cursor=-178
    for a,b,name in gate_spans:
        if a>cursor:segments.append(([cursor,-50],[a,-50]))
        gates.append(dict(id=name,centre=[(a+b)/2,-50,0],width_m=b-a,always_passable=True));cursor=b
    if cursor<278:segments.append(([cursor,-50],[278,-50]))
    segments.extend([([-178,-50],[-178,366]),([278,-50],[278,366]),([-178,366],[278,366])])
    for a,b in segments:
        along_x=a[1]==b[1];length=abs(b[0]-a[0])+abs(b[1]-a[1]);n=math.ceil(length/4)
        for j in range(n):
            xx=a[0]+(b[0]-a[0])*(j+.5)/n;yy=a[1]+(b[1]-a[1])*(j+.5)/n;w=length/n
            p=[[0,0,1.45,w,.34,2.9,'camp_concrete'],[0,0,2.95,w+.04,.48,.10,'camp_steel'],[-w/2,0,1.60,.24,.54,3.20,'camp_concrete']]
            c.emit('perimeter_wall','ST',p,[xx,yy,0],0 if along_x else 1,strict=False,extra=dict(mount='wall',boundary=True))
    for g in gates:
        xx,yy,_=g['centre'];w=g['width_m']
        # Raised barrier leaves leave at least 4.5m overhead and full clear width.
        for side in (-1,1):
            p=[[0,0,.55,.50,.50,1.1,'camp_ochre'],[0,0,3.1,.12,.12,4.0,'camp_cream'],[0,0,5.15,.25,.25,.12,'camp_red']]
            c.emit('gate_barrier','ST',p,[xx+side*(w/2-.65),yy,0],strict=False)
        c.emit('gate_header','ST',[[0,0,5.20,w,.18,.18,'camp_steel']],[xx,yy,0],solid=False,strict=False)
        c.sign('БОЛЬНИЦА 333 / КОНТРОЛЬ ДОСТУПА' if g['id']=='medical_main' else 'СЛУЖЕБНЫЙ ВЪЕЗД / КПП',[xx,yy,4.8],2,'gate_sign')
    # Two wall-mounted sentry platforms, reached by proper 15cm / 45cm steps.
    platforms=[]
    for xx in (70,225):
        at=[xx,-47.5,0];p=[[0,0,2.95,4,3,.20,'camp_steel']]
        for x in (-1.8,1.8):
            for y in (-1.3,1.3):p.append([x,y,1.45,.18,.18,2.9,'camp_steel'])
        for x in (-1.95,1.95):
            p.extend([[x,0,3.55,.07,3,1.1,'camp_steel'],[x,0,4.75,.10,.10,3.0,'camp_steel']])
        p.append([0,0,5.95,4.3,3.3,.12,'camp_steel'])
        c.emit('sentry_platform','ST',p,at,strict=False,partial=True,extra=dict(mount='structure'))
        for j in range(20):
            # Last tread ends flush at platform floor, no jump required.
            cy=-46+(.45*(19-j)+.225);h=.15*(j+1)
            c.emit('sentry_stair','ST',[[0,0,h/2,1.2,.45,h,'camp_concrete']],[xx,cy,0],strict=False)
        c.emit('wall_weapon_rack','ST',shape('weapon_rack'),[xx,-48,3.05],strict=False,solid=False,extra=dict(mount='platform',decorative_weapon=True))
        c.emit('wall_security_supplies','ST',shape('security_supplies'),[xx+1,-47,3.05],strict=False,solid=False,extra=dict(mount='platform'))
        platforms.append(dict(at=at,floor_z=3.05,tread_m=.45,riser_m=.15,steps=20,width_m=1.2))
    # New guard shelters with genuinely open fronts, desk/rack/first-aid stores.
    for xx,yy in ((64,-55),(145,-55)):
        p=[[0,0,.04,3.6,3,.08,'camp_concrete'],[0,-1.45,1.30,3.6,.10,2.6,'camp_sage'],[-1.75,0,1.30,.10,3,2.6,'camp_sage'],[1.75,0,1.30,.10,3,2.6,'camp_sage'],[0,0,2.65,3.9,3.3,.10,'camp_steel']]
        c.emit('checkpoint_shelter','ST',p,[xx,yy,0],strict=False,partial=True,extra=dict(mount='structure',clear_door_m=3.4))
        for kind,dx,dy in (('guard_desk',-.8,-.55),('weapon_rack',.70,-.70),('security_supplies',.80,.45)):
            c.emit(kind,'ST',shape(kind),[xx+dx,yy+dy,.08],strict=False,extra=dict(mount='checkpoint',decorative_weapon=kind=='weapon_rack'))
    # Full-size distinctive room landmarks, not a common prop repeated by ID.
    landmark_requests={
      'PS09':['therapy_bench','therapy_cart'],'PS10':['sensory_station'],'PS11':['therapy_bench'],
      'PS12':['sensory_station'],'PS16':['notice_stand'],'PS20':['planter'],
      'PS2_12':['sensory_station'],'PS2_18':['notice_stand'],
      'RH03':['exercise_cycle'],'RH04':['domestic_kitchen'],'RH17':['gait_rail','exercise_cycle'],
      'RH18':['balance_course'],'RH19':['tool_board'],'RH23':['planter'],
      'HS02':['domestic_kitchen'],'HS06':['laundry_sorter'],'HS11':['guard_desk'],
      'HS18':['notice_stand'],'HS21':['tool_board'],'HS24':['therapy_bench'],
      'SV03':['compressor'],'SV06':['tool_board'],'SV10':['laundry_sorter'],
      'SV18':['pump_skid'],'SV20':['pump_skid'],'SV22':['duct_rack'],
      'SV2_06':['tool_board'],'SV2_08':['compressor'],'SV2_19':['tool_board'],
      'X01':['market_crates','market_crates'],'X02':['pharmacy_display'],'X03':['market_crates'],
      'X04':['pallet_rack'],'X05':['domestic_kitchen'],'S03':['guard_desk','weapon_rack'],
    }
    for zid,kinds in landmark_requests.items():
        for k in kinds:
            item=c.fit(k,zid);c.requests.append(dict(zone=zid,kind=k,placed=item['id'] if item else None))
            if item and not any(q['id']=='campus_'+zid for q in d['furnishing_captures']):c.capture(item,zid)
    # The 76x16m halls stay at their original area. Equipment islands create
    # side loops around a protected centre aisle and both original doors.
    hall_sections=[]
    for zid,kinds in [('DG25',['trolley_bay','trolley_bay','electric_tug','charger_station','trolley_bay','guard_desk','trolley_bay','waste_station','trolley_bay','trolley_bay','oxygen','notice_stand']),('DG26',['retired_scanner','pallet_rack','scrap_cage','pallet_rack','scrap_cage','tool_board','duct_rack','scrap_cage','retired_scanner','pallet_rack','compressor','notice_stand'])]:
        s,r=c.zones[zid];ox,oy,z=s['origin'];x,y,w,h=r['rect'];fitted=[]
        for j,k in enumerate(kinds):
            near=[ox+x+5+(j%6)*12,oy+y+(3 if j<6 else h-3)]
            item=c.fit(k,zid,near);c.requests.append(dict(zone=zid,kind=k,placed=item['id'] if item else None))
            if item:fitted.append(item)
        hall_sections.append(dict(zone=zid,area_m2=w*h,role='diagnostic_transport_hub' if zid=='DG25' else 'retired_equipment_depot',items=[i['id'] for i in fitted],clear_centre_aisle_m=2.5,sections=['dispatch','patient_transport','charging','cleaning'] if zid=='DG25' else ['scanner_salvage','parts_cages','pallet_storage','repair']))
        if fitted:c.capture(fitted[2],'hall_'+zid)
    # Additional service kit per suitable room across every building/floor.
    # All requested services were mapped in v13; this pass adds context.
    for zid,(s,r) in sorted(c.zones.items()):
        if zid in landmark_requests or zid in ('DG25','DG26') or not r['enclosed']:continue
        name=r['name'].lower();kind=None
        for terms,k in [(('кухн','буфет','столов','раздаточ'),'domestic_kitchen'),(('гардероб','раздевал','комната водител','гостевая','общежит'),'personal_locker'),(('моечная','прачеч','бельевая','белья'),'laundry_sorter'),(('мастерск','ремонт','инструмент','протез'),'tool_board'),(('насос','водоочист','водоподготов'),'pump_skid'),(('вент','фильтр'),'duct_rack'),(('физиотерап','лфк','трениров'),'exercise_cycle'),(('психолог','терапии','сенсорн'),'sensory_station'),(('охраны','охранник','вахта'),'guard_desk'),(('лаборатор','пцр','микробиол'),'labhood'),(('склад','кладовая','хранение'),'pallet_rack'),(('аптек',),'pharmacy_display')]:
            if any(t in name for t in terms):kind=k;break
        if kind:
            item=c.fit(kind,zid);c.requests.append(dict(zone=zid,kind=kind,placed=item['id'] if item else None))
    print('Interior requests',len(c.requests),'placed',sum(bool(r['placed']) for r in c.requests),flush=True)
    # Colour-coded public/clinical/service orientation, as on hospital campus
    # plans. Thin strips are decorative and never replace walls or floors.
    identities={}
    for s in d['locations']:
        sid=s['id'];base='PS' if sid.startswith('PS') else 'RH' if sid.startswith('RH') else 'HS' if sid.startswith('HS') else 'SV' if sid.startswith('SV') else 'DG' if sid.startswith('DG') else 'IF' if sid.startswith('IF') else 'CORE'
        mat=dict(PS='sage',RH='teal',HS='ochre',SV='steel',DG='purple',IF='teal',CORE='cream')[base]
        identities[sid]=dict(colour=mat,theme=dict(PS='терапия / сенсорные занятия / зимний сад',RH='обучение ходьбе / бытовая реабилитация',HS='быт смены / личные шкафчики / клуб',SV='транспорт / насосы / промышленная стирка',DG='диагностика / транспорт / списанная техника',IF='изоляция / чистые и грязные потоки',CORE='приём / лечение / вертикальные связи')[base])
        ox,oy,z=s['origin']
        if sid in ('ST','UT'):continue
        for x,y,w,h in s['corridors']:
            if w>=h:at=[ox+x+w/2,oy+y+.28,z+.025];sz=[max(.1,w-.6),.08,.018]
            else:at=[ox+x+.28,oy+y+h/2,z+.025];sz=[.08,max(.1,h-.6),.018]
            c.emit('department_route',sid,[[0,0,0,*sz,'camp_'+mat]],at,solid=False,strict=False,extra=dict(mount='floor_marking'))
    # Public forecourt, medical arrival, quiet courts and a separate service
    # yard. Items use free sites only; road lanes and tested routes stay open.
    districts=[dict(id='public_retail',rect=[-70,-188,220,104],role='shops_cafe_pharmacy_outside_controlled_campus'),dict(id='arrival',rect=[-70,-81,175,79],role='visitors_and_ambulance'),dict(id='west_clinical_court',rect=[-44,50,36,44],role='quiet_patient_court'),dict(id='east_clinical_court',rect=[108,170,38,38],role='rehabilitation_landscape'),dict(id='staff_court',rect=[-44,236,84,28],role='staff_break_and_bicycles'),dict(id='service_court',rect=[108,238,38,28],role='laundry_delivery_and_maintenance'),dict(id='south_service',rect=[-150,334,396,28],role='emergency_and_service_access')]
    outdoor_counts=collections.Counter()
    for district in districts:
        x,y,w,h=district['rect']
        kinds=['planter','bench','cycle_rack','recycling'] if 'service' not in district['id'] else ['recycling','duct_rack','security_supplies','planter']
        for j,xx in enumerate(range(math.ceil(x+3),int(x+w-2),7)):
            for k,yy in enumerate(range(math.ceil(y+2),int(y+h-1),8)):
                # Street centres remain free even where the terrain looks empty.
                if any(overlap([xx-1.5,yy-1.5,3,3],road) for road in c.e['roads']):continue
                kind=kinds[(j+k)%len(kinds)];item=c.outside(kind,[xx,yy,0],turn=(j+k)%4)
                if item:item['district']=district['id'];outdoor_counts[district['id']]+=1
    # Parking groups are aligned and actual car dimensions are 1.75x4.22m.
    for xx in (-57,-51,-45,-39,-33,-12,-6):
        for yy in (-75,-65,-55):
            item=c.outside('sedan',[xx,yy,0],zone='S01')
            if item:outdoor_counts['parking_cars']+=1
    # Curbs are split at actual pedestrian paths, never continuous barriers.
    for district in districts:
        x,y,w,h=district['rect']
        for xx in range(math.ceil(x),int(x+w),3):
            yy=y+.4;rr=[xx,yy,2,.18]
            if c.floor.hits(0,rr,.1) or c.reserved.hits(0,rr,.6) or any(overlap(rr,road) for road in c.e['roads']):continue
            if any(overlap(rr,[q[0]-.3,q[1]-.3,q[2]+.6,q[3]+.6]) for s in d['locations'] if s['origin'][2]==0 for r in s['rooms'] if r['enclosed'] for q in [[s['origin'][0]+r['rect'][0],s['origin'][1]+r['rect'][1],*r['rect'][2:]]]):continue
            c.emit('curb','ST',[[0,0,.08,2,.18,.16,'camp_concrete']],[xx+1,yy+.09,0],strict=False,extra=dict(mount='outside',district=district['id']))
    # A few new genuine sheltered stops; roof parts stay above head height.
    canopies=[]
    for xx,yy in ((15,-69),(84,-39),(-78,109),(233,219)):
        rr=[xx-3,yy-1.7,6,3.4]
        if c.floor.hits(0,rr,.2) or c.reserved.hits(0,rr,.1):continue
        p=[[0,0,2.95,6,3.4,.12,'camp_steel']]
        for dx in (-2.8,2.8):
            for dy in (-1.5,1.5):p.append([dx,dy,1.45,.10,.10,2.9,'camp_steel'])
        item=c.emit('court_canopy','ST',p,[xx,yy,0],partial=True,strict=True,extra=dict(mount='structure'))
        if item:canopies.append(item['id'])
    # Place readable signs at real enclosed corridor junctions (high enough to
    # avoid old furniture), and separate visitor/service entries outside.
    for text,at,t in [('МАГАЗИНЫ / АПТЕКА / КАФЕ',[15,-110,2.5],1),('ПРИЁМНОЕ / СКОРАЯ ПОМОЩЬ',[59,-31,2.5],2),('ПСИХОНЕВРОЛОГИЯ / ТИХИЙ ДВОР',[-60,117,2.5],2),('РЕАБИЛИТАЦИЯ / ЛЕЧЕБНАЯ ДОРОЖКА',[158,117,2.5],2),('ОБЩЕЖИТИЕ / КОРПУС ПЕРСОНАЛА',[-60,227,2.5],2),('ХОЗЯЙСТВЕННЫЙ БЛОК / ДОСТАВКА',[158,227,2.5],2)]:c.sign(text,at,t)
    # Existing passages are already enclosed; register their continuous
    # above-ground links and the retained dogleg tunnels, rather than drawing
    # a misleading additional disconnected tube.
    connections=[dict(from_site=a,to_site=b,passage=p,kind='enclosed_gallery') for a,b,p in [('CORE','DG',[-46,38,94,6,0]),('CORE','IF',[52,38,98,6,0]),('CORE','PS',[-46,158,94,6,0]),('CORE','RH',[52,158,98,6,0]),('CORE','HS',[-46,268,94,6,0]),('CORE','SV',[52,268,98,6,0])]]
    # No new universal hiding immunity: record the actual native behaviours.
    counterplay=dict(niche='physical sight break; 0.9m opening excludes large capsules, not small patients',wardrobe='F enters; suppresses visual acquisition, does not erase remembered target or sound',under_bed='F crouches and enters; visual concealment with physically checked exit',refuge='F operates the real inside bolt; ordinary patients cannot bypass it',exceptions=dict(listener='hears noise; no visual sensor',blind_surgeon='hears noise; no visual sensor',hound='tracks blood within 10m below 65 health even when furniture-hidden',remembered_pursuit='can still attack within reach on a clear trace; hiding during pursuit is not invulnerability'))
    for name,at,target in [('perimeter',[59,-72,10],[60,-48,2]),('sentry',[76,-40,5.5],[70,-47.5,3.8]),('public',[11,-116,9],[-25,-139,2]),('campus_court',[112,123,12],[166,112,1]),('DG25_overview',[-102,60,3.1],[-116,51,1]),('DG26_overview',[-100,69,3.1],[-116,78,1])]:
        d['furnishing_captures'].append(dict(id='campus_'+name,at=at,target=target,campus=True))
    d['campus_plan']=dict(version=14,clinical_boundary=dict(rect=[-178,-50,456,416],height_m=2.9,segments=segments,gates=gates),public_retail=['X01','X02','X03','X05'],external_supply='X04',clinical_sites=['1F','B1','B2','2F','3F','RF','DG','DG2','DG3','IF','IF2','IF3','PS','PS2','RH','RH2','RH3','HS','HS2','SV','SV2','SV3'],connections=connections,underground_links=[q['id'] for q in d['utility_links']],districts=districts,department_identity=identities,halls=hall_sections,sentry_platforms=platforms,canopies=canopies,items=c.items,requests=c.requests,hide_counterplay=counterplay,limits=['Metric playable prototype, not final environment art.','Wall weapons are static guard-position props; existing story weapons and ammunition remain unchanged.','Existing underground links and indoor galleries remain walkable; no new disconnected schematic tunnels.','Optional extra room kits that do not fit are explicitly listed, never shrunk into doorways.'])
    d['campus_plan']['furniture_relocations']=relocations
    d['review_revision']=14
    stats=dict(items=len(c.items),parts=sum(i['parts'] for i in c.items),kinds=dict(collections.Counter(i['kind'] for i in c.items)),new_floor_kits=sum(i.get('mount')=='floor' for i in c.items),new_zones=len({i['zone'] for i in c.items if i.get('mount')=='floor'}),unfitted_optional=[r for r in c.requests if not r['placed']],district_items=dict(outdoor_counts),new_views=sum(q.get('campus',False) for q in d['furnishing_captures']))
    (OUT/'campus_inventory.json').write_text(json.dumps(stats,ensure_ascii=False,indent=2),encoding='utf-8')
    with (OUT/'campus_items.csv').open('w',encoding='utf-8-sig',newline='') as f:
        keys=list(dict.fromkeys(k for row in c.items for k in row));w=csv.DictWriter(f,keys);w.writeheader();w.writerows({k:json.dumps(v,ensure_ascii=False) if isinstance(v,(list,dict)) else v for k,v in row.items()} for row in c.items)
    print(json.dumps({k:v for k,v in stats.items() if k not in ('kinds','unfitted_optional')},ensure_ascii=False),flush=True)
    return stats

if __name__=='__main__':
    baseline=OUT/'baseline_v13.json';d=json.loads((baseline if baseline.exists() else PATH).read_text(encoding='utf-8'))
    apply_campus(d);PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');refresh_navigation()
