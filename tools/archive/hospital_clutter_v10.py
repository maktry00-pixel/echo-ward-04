"""Stock existing furniture with supported, non-blocking prototype belongings.

Each logical item owns one or more rendered boxes. The original furniture,
collision geometry, routes, saves and story objects are untouched.
"""
import collections,hashlib,json,math,random
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
OUT=ROOT/'artifacts/unreal_hospital_v10'

def rotate(v,t):
    x,y,z=v
    return [(x,y,z),(-y,x,z),(-x,-y,z),(y,-x,z)][t%4]

class Space:
    def __init__(self):self.cells=collections.defaultdict(list)
    def keys(self,b):
        for x in range(math.floor(b[0]),math.floor(b[3])+1):
            for y in range(math.floor(b[1]),math.floor(b[4])+1):
                for z in range(math.floor(b[2]),math.floor(b[5])+1):yield x,y,z
    def add(self,b,ident):
        for k in self.keys(b):self.cells[k].append((b,ident))
    def hits(self,b,ignore=''):
        return any(ident!=ignore and all(b[i]<r[i+3]-.002 and b[i+3]>r[i]+.002 for i in range(3)) for k in self.keys(b) for r,ident in self.cells.get(k,()))

def bounds(at,size):return [at[i]-size[i]/2 for i in range(3)]+[at[i]+size[i]/2 for i in range(3)]

def apply_clutter(d):
    OUT.mkdir(parents=True,exist_ok=True)
    e=d['environment_dressing'];e['boxes']=[b for b in e['boxes'] if b.get('layer')!=10]
    catalog=d['prototype_catalog'];furniture=d['prototype_props']+d['environment_props'];space=Space()
    for p in furniture:
        if p['mount'] not in ('floor','surface','wall'):continue
        for a in catalog[str(p['model'])]['parts']:
            local=rotate(a[:3],p['turn']);at=[p['at'][i]+local[i] for i in range(3)];size=list(a[3:6])
            if p['turn']%2:size[0],size[1]=size[1],size[0]
            space.add(bounds(at,size),p['id'])
    for n,b in enumerate(e['boxes']):
        if b['kind'] in ('lawn','grass_tuft','pavement','road_dash','tree_crown'):continue
        space.add(bounds(b['at'],b['size']),'old_box_'+str(n))
    rooms={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};items=[];rng=random.Random(10041)
    e['materials'].update({
        'paper':dict(tint=[.75,.70,.56],roughness=.9,weathering=.25),
        'cardboard':dict(tint=[.42,.27,.12],roughness=.9,weathering=.35),
        'fabric':dict(tint=[.13,.27,.30],roughness=.95,weathering=.35),
        'medicine':dict(tint=[.18,.38,.46],roughness=.32,wetness=.1),
        'label':dict(tint=[.74,.77,.69],roughness=.8,weathering=.1),
    })
    # Parts are local to the item base, on the furniture's horizontal support.
    def shape(kind):
        if kind in ('supply_box','food_carton','archive_box'):
            return [[0,0,.10,.22,.18,.20,'cardboard'],[0,.091,.12,.15,.004,.08,'label'],[0,0,.202,.22,.025,.004,'paper']]
        if kind in ('medicine_bottle','chemical_bottle'):
            return [[0,0,.07,.07,.07,.14,'medicine'],[0,0,.151,.065,.065,.022,'label'],[0,.036,.07,.055,.003,.045,'paper']]
        if kind=='medical_tray':
            return [[0,0,.012,.25,.16,.024,'steel'],[-.08,0,.035,.035,.10,.025,'label'],[.07,0,.04,.055,.055,.03,'paper']]
        if kind=='mug':
            return [[0,0,.006,.09,.09,.012,'label'],[-.04,0,.06,.01,.09,.12,'label'],[.04,0,.06,.01,.09,.12,'label'],[0,-.04,.06,.07,.01,.12,'label'],[0,.04,.06,.07,.01,.12,'label'],[0,0,.09,.07,.07,.008,'wood'],[.065,0,.065,.04,.025,.055,'label']]
        if kind in ('folder','patient_chart'):
            return [[0,0,.012,.21,.28,.024,'fabric'],[0,0,.027,.18,.25,.006,'paper'],[0,-.04,.031,.12,.012,.002,'dark'],[0,0,.031,.12,.012,.002,'dark'],[0,.04,.031,.09,.012,.002,'dark']]
        if kind=='books':
            return [[-.07,0,.11,.055,.16,.22,'fabric'],[0,0,.12,.055,.16,.24,'paper'],[.07,0,.10,.055,.16,.20,'blood']]
        if kind=='tool_case':return [[0,0,.045,.26,.17,.09,'dark'],[0,0,.092,.12,.06,.004,'steel'],[0,.09,.05,.08,.025,.03,'steel']]
        if kind=='towel_stack':return [[0,0,.025,.22,.18,.05,'label'],[.01,0,.07,.20,.18,.04,'fabric']]
        if kind=='linen':return [[0,.25,.025,.65,.80,.05,'fabric'],[0,.58,.058,.65,.14,.015,'label']]
        if kind=='parcel':return [[0,0,.18,.42,.32,.36,'cardboard'],[0,0,.363,.045,.32,.006,'paper'],[0,.162,.18,.18,.004,.10,'label']]
        if kind=='floor_paper':return [[0,0,.003,.20,.28,.006,'paper'],[0,-.06,.0065,.12,.012,.001,'dark'],[0,-.02,.0065,.12,.012,.001,'dark']]
        if kind=='drain':return [[0,0,.006,.48,.28,.012,'dark']]+[[x,0,.015,.025,.27,.008,'steel'] for x in (-.18,-.12,-.06,0,.06,.12,.18)]
        raise ValueError(kind)
    def add(kind,p,local,support_height,limit):
        parts=shape(kind);t=p.get('turn',0);offset=rotate([local[0],local[1],support_height+.008],t);base=[p['at'][i]+offset[i] for i in range(3)]
        boxes=[]
        for a in parts:
            xyz=rotate(a[:3],t);at=[base[i]+xyz[i] for i in range(3)];size=list(a[3:6])
            if t%2:size[0],size[1]=size[1],size[0]
            bb=bounds(at,size)
            if space.hits(bb,p['id']):return False
            boxes.append(dict(at=[round(v,5) for v in at],size=size,material=a[6],kind=kind,zone=p['zone'],solid=False,layer=10))
        total=[min(b['at'][i]-b['size'][i]/2 for b in boxes) for i in range(3)]+[max(b['at'][i]+b['size'][i]/2 for b in boxes) for i in range(3)]
        # All parts, including labels and handles, stay inside the support.
        if not(total[0]>=limit[0]-.001 and total[1]>=limit[1]-.001 and total[3]<=limit[2]+.001 and total[4]<=limit[3]+.001):return False
        ident=f'CLUTTER10_{len(items):05}'
        for b in boxes:b['item_id']=ident;e['boxes'].append(b)
        items.append(dict(id=ident,kind=kind,zone=p['zone'],site=p.get('site','ST'),parent=p['id'],base=[round(v,5) for v in base],bounds=total,support_z=base[2]-.008,support_rect=limit,parts=len(boxes)))
        space.add(total,ident);return True
    for p in furniture:
        if p['mount']!='floor':continue
        m=p['model'];c=catalog[str(m)];b=c['bounds'];fw,fd=b[3]-b[0],b[4]-b[1];x,y,z=p['at'];turn=p['turn'];w,h=(fd,fw) if turn%2 else (fw,fd);limit=[x-w/2+.025,y-h/2+.025,x+w/2-.025,y+h/2-.025]
        if m in (13,14,15):
            s,r=rooms.get(p['zone'],({},{}));name=r.get('name','').lower()
            kind='books' if m==14 else 'food_carton' if m==15 else 'chemical_bottle' if any(k in name for k in ('лаборат','анализ','штамм')) else 'supply_box'
            for zz in (.5725,1.0025,1.4325):
                for xx in (-.33,0,.33):add(kind,p,[xx,.04],zz,limit)
        elif m in (16,17,18,19,20,21,22,23,24,37,38,39,107,130):
            choices=['medical_tray','medicine_bottle','towel_stack'] if m in (20,37,38) else ['tool_case','supply_box','folder'] if m in (21,22,107) else ['mug','patient_chart','folder'] if m in (16,23) else ['mug','food_carton','towel_stack']
            for i,xx in enumerate((-.32,0,.32)):
                for yy in (.15,-.15):
                    if add(choices[i],p,[xx,yy],c['top'],limit):break
        elif m in (4,88):
            for xx,kind in ((-.10,'medicine_bottle'),(.10,'mug')):add(kind,p,[xx,0],c['top'],limit)
        elif m in (1,2,3,31,32,33,34):add('linen',p,[0,0],c['top'],limit)
        elif m in (108,109):
            for xx in (-.25,.25):add('parcel',p,[xx,0],c['top'],limit)
    # Small sheets lie on verified room floor, with no blocking collision.
    for route in d['navigation_routes']:
        s,r=rooms[route['zone']]
        if not r['enclosed']:continue
        x,y,z=route['points'][-1];dummy=dict(id='floor_'+r['id'],zone=r['id'],site=s['id'],at=[x,y,z],turn=0)
        ox,oy,_=s['origin'];rx,ry,w,h=r['rect'];limit=[ox+rx+.12,oy+ry+.12,ox+rx+w-.12,oy+ry+h-.12]
        for xx,yy in ((0,0),(.32,.14)):
            add('floor_paper',dummy,[xx,yy],.001,limit)
    # Flat drainage grates on open roads, never a new obstacle or a cut slab.
    for i,b in enumerate([b for b in e['boxes'] if b['kind']=='road_dash'][::4]):
        x,y,z=b['at'];dummy=dict(id='road_'+str(i),zone='ST',site='ST',at=[x,y,0],turn=0)
        add('drain',dummy,[2,0],.025,[x+1.5,y-.5,x+2.5,y+.5])
    counts=collections.Counter(i['kind'] for i in items)
    d['clutter_dressing']=dict(version=10,quality='prototype',items=items,counts=dict(counts),limits=['Belongings and stock are decorative prototypes; gameplay pickups remain separate.','No new blocking collision or interactive containers.'])
    d['review_revision']=10
    # Existing proven views are retained. Close views show stocked furniture.
    d['furnishing_captures']=[c for c in d['furnishing_captures'] if not c.get('clutter')]
    for c in [c for c in d['furnishing_captures'] if c['id'].startswith('environment_furnished_')]:
        q=dict(c);q['id']=q['id'].replace('environment_','clutter_',1);q['clutter']=True;d['furnishing_captures'].append(q)
    result=dict(items=len(items),parts=sum(i['parts'] for i in items),stocked_furniture=len({i['parent'] for i in items if not i['parent'].startswith(('floor_','road_'))}),zones=len({i['zone'] for i in items}),counts=dict(counts))
    (OUT/'clutter_inventory.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(result,ensure_ascii=False));return result

if __name__=='__main__':
    d=json.loads(PATH.read_text(encoding='utf-8-sig'));apply_clutter(d)
    PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
