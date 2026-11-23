"""Import the user's embedded numbered PDF schedule into the playable world.

Semantic diagram points are fitted to real model bounds, not copied as colliding
transforms. Existing objects retain their source coordinates. Every correction
is recorded, and failure to fit a scheduled instance is an authoring error.
"""
import collections, copy, csv, hashlib, json, math, random
from pathlib import Path
from hospital_prop_catalog import MATERIALS, make_catalog

ROOT=Path(__file__).resolve().parents[1]
PLAN=ROOT/'data/hospital_furnishing/placement_plan.json'
OUT=ROOT/'artifacts/unreal_hospital_v6'

def overlap(a,b,gap=0):
    return a[0]<b[0]+b[2]+gap and a[0]+a[2]>b[0]-gap and a[1]<b[1]+b[3]+gap and a[1]+a[3]>b[1]-gap

class Index:
    def __init__(self):self.grid=collections.defaultdict(list)
    def keys(self,z,r):
        for x in range(math.floor(r[0]/4),math.floor((r[0]+r[2])/4)+1):
            for y in range(math.floor(r[1]/4),math.floor((r[1]+r[3])/4)+1):yield (round(z,2),x,y)
    def add(self,z,r,kind='solid'):
        for k in self.keys(z,r):self.grid[k].append((r,kind))
    def hits(self,z,r,gap=0):
        q=[r[0]-gap,r[1]-gap,r[2]+2*gap,r[3]+2*gap]
        return any(overlap(r,b,gap) for k in self.keys(z,q) for b,_ in self.grid.get(k,()))

def apply_furnishing(d):
    plan=json.loads(PLAN.read_text(encoding='utf-8-sig'));catalog=make_catalog(plan['model_catalog'])
    proposed={str(i['model']) for z in plan['zones'] for i in z['items'] if i['qty']>i['existing']}
    assert proposed<=set(catalog),('missing prototypes',proposed-set(catalog))
    OUT.mkdir(parents=True,exist_ok=True)
    d['review_revision']=6;d['prototype_catalog']=catalog;d['prototype_materials']=MATERIALS
    d['prototype_props']=[];d['furnishing_source']=dict(file='ЭХО_план_наполнения_всех_зон (1).pdf',
        schedule_sha256=hashlib.sha256(PLAN.read_bytes()).hexdigest(),quality='prototype',
        placement_note=plan['placement_note'])
    # Repeatable: remove only this pass's generated obstacles before rebuilding.
    d['collision_features']=[f for f in d['collision_features'] if not f['id'].startswith('PROP6_')]
    sites={l['id']:l for l in d['locations']};zones={r['id']:(l,r) for l in d['locations'] for r in l['rooms']}
    open_rooms=[];removed_partitions=[]
    open_names={'Комната групповой терапии','Столовая','Мойка машин','Комната Ирины',
                'Продуктовая кладовая','Регистрация','Профсоюзный архив','Мастерская садовника',
                'Комната инженера','Медицинский склад','Отдельный приёмный покой','Четырёхместная палата','Паллиативная палата'}
    for zid in [key for key,(_,r) in zones.items() if r['name'] in open_names]:
        l,r=zones[zid];ox,oy,z=l['origin'];x,y,w,h=r['rect'];open_rooms.append((z,[ox+x,oy+y,w,h],zid))
        r['layout']='open_functional_room'
    def inside_open(z,r):
        return any(z==zz and ar[0]+.05<r[0]+r[2]/2<ar[0]+ar[2]-.05 and ar[1]+.05<r[1]+r[3]/2<ar[1]+ar[3]-.05 for zz,ar,_ in open_rooms)
    kept=[]
    for p in d['partitions']:
        ax,ay,z=p['a'];bx,by,_=p['b'];rr=[min(ax,bx),min(ay,by),abs(bx-ax),abs(by-ay)]
        if p['type']=='partition' and inside_open(z,rr):removed_partitions.append(p)
        else:kept.append(p)
    d['partitions']=kept
    d['map_fixes_v6']=[dict(id='open_shared_rooms',zones=[q[2] for q in open_rooms],removed_partitions=len(removed_partitions),reason='Dining and group therapy must be usable shared spaces, not repeated cubicles'),
        dict(id='garden_paths',zones=['S05','S15'],reason='Restore two-metre paths through green slabs and level gazebo base')]
    bays=collections.defaultdict(list)
    for b in d['subrooms']:
        ox,oy,_=sites[b['site']]['origin'];x,y,w,h=b['rect'];bays[b['zone']].append((b['id'],[ox+x,oy+y,w,h]))
    solid=Index();reserved=Index();decor=Index();wallsegs=collections.defaultdict(list)
    # Walls are exported by the independent shell raster. Derive partitions and
    # external boundaries from source geometry when a shell export is unavailable.
    wallfile=ROOT/'artifacts/unreal_hospital_v3/raster_walls.json'
    assert wallfile.exists(),'Run the shell navigation audit before furnishing'
    for wall in json.loads(wallfile.read_text(encoding='utf-8')):
        z=wall['z'];r=wall['rect']
        if wall.get('type')=='partition' and inside_open(z,r):continue
        solid.add(z,r);wallsegs[z].append(r)
    for f in d['collision_features']:solid.add(f['z'],f['rect'])
    def reserve(z,a,b,r=.47):
        reserved.add(z,[min(a[0],b[0])-r,min(a[1],b[1])-r,abs(a[0]-b[0])+r*2,abs(a[1]-b[1])+r*2])
    # Preserve actual tested routes, every door porch, escape recess approaches,
    # and furniture hiding entry/exit sweeps. These spaces cannot be stocked.
    seen=set()
    baypaths=d.get('bay_navigation_routes') or json.loads((ROOT/'artifacts/unreal_hospital_v3/bay_navigation_routes.json').read_text(encoding='utf-8'))
    for route in d['navigation_routes']+baypaths:
        for a,b in zip(route['points'],route['points'][1:]):
            if a[2]!=b[2]:continue
            k=tuple(a+b)
            if k not in seen:reserve(a[2],a,b);seen.add(k)
    for l in d['locations']:
        ox,oy,z=l['origin']
        for r in l['rooms']:
            x,y,w,h=r['rect'];cx,cy=ox+x+w/2,oy+y+h/2
            # The clear central aisle is intentional, not an unfurnished room.
            if r['enclosed']:
                px,py=ox+r['door'][0],oy+r['door'][1];reserve(z,[px,py],[cx,cy],.62)
            for e in [dict(at=r['door'],side=r['door_side'])]+r.get('extra_doors',[]):
                px,py=ox+e['at'][0],oy+e['at'][1];ns=e['side'] in ('north','south')
                reserve(z,[px if ns else px-1.6,py-1.6 if ns else py],[px if ns else px+1.6,py+1.6 if ns else py],.62)
    for h in d['hideouts']:
        x,y,z=h['at'];w,dep,_=h['size'];reserved.add(z,[x-.12,y-.12,w+.24,dep+1.6])
    for s in d['stairs']:
        x,y,_,_=s['rect'];z=s['z_min']
        while z<=s['z_max']+.01:
            reserved.add(z,[x-.55,y-.55,4.6,9.8]);z=round(z+4.8,2)
    for n in d['escape_niches']:
        ex,ey,z=n['entry'];a=n.get('corridor_point',[ex,ey,z]);reserve(z,a,[ex,ey],.6)
        reserved.add(z,n['rect'])
    for z in (-9.6,-4.8,0,4.8,9.6,14.4):reserved.add(z,[47.3,3.3,5.4,9.4])
    # Infrastructure halls have a through aisle even though they are not rooms
    # in the story registry, and so have no room-centre navigation destination.
    for q in d['infrastructure_zones']:
        x,y,w,h=q['rect'];reserve(q['z'],[x,y+h/2],[x+w,y+h/2],.7)
    placed_by_zone=collections.defaultdict(list);instances=[];unplaced=[];corrections=[]
    existing=[]
    for zone in plan['zones']:
        zid=zone['id'];fz=zone['floor_z'];area=zone['rect_global']
        for item in zone['items']:
            for anchor in item['instances']:
                p=dict(id=f"PROP6_{zid}_{anchor['instance']}",zone=zid,site=zone['site'],model=item['model'],
                       source_xy=anchor['xy'],source_subroom=anchor.get('subroom'),floor_z=fz,
                       source_position=anchor.get('position',''),source_reference=anchor.get('reference',''),area=area)
                if anchor['source']:existing.append(p);continue
                p['anchor']=anchor;instances.append(p)
    def dimensions(c,turn):
        b=c['bounds'];w=b[3]-b[0];h=b[4]-b[1]
        return (h,w) if turn%2 else (w,h)
    def footprint(c,x,y,turn):
        w,h=dimensions(c,turn);return [x-w/2,y-h/2,w,h]
    def floorspaces(p):
        if p['zone'] in [q[2] for q in open_rooms]:return [(p['zone'],p['area'])]
        bs=bays[p['zone']]
        if bs:
            target=p['source_subroom'];return sorted(bs,key=lambda b:(b[0]!=target,math.dist([b[1][0]+b[1][2]/2,b[1][1]+b[1][3]/2],p['source_xy'])))
        return [(p['zone'],p['area'])]
    def fit_floor(p):
        c=catalog[str(p['model'])];fz=p['floor_z'];anchor=p['source_xy'];best=None
        for bid,r in floorspaces(p):
            x,y,w,h=r
            for turn in (0,1):
                fw,fh=dimensions(c,turn);mx=fw/2+.18;my=fh/2+.18
                if w<mx*2 or h<my*2:continue
                # Fine enough for clinical bays, bounded work on large halls.
                step=.16 if w*h<70 and fw*fh<.5 else .28 if w*h<70 else .55 if w*h<700 else 1.1
                xs=[anchor[0]]+[x+mx+i*step for i in range(int((w-2*mx)/step)+1)]+[x+w-mx]
                ys=[anchor[1]]+[y+my+i*step for i in range(int((h-2*my)/step)+1)]+[y+h-my]
                opts=sorted(((math.dist((xx,yy),anchor),xx,yy) for xx in xs for yy in ys
                             if x+mx-.001<=xx<=x+w-mx+.001 and y+my-.001<=yy<=y+h-my+.001))
                for dist,xx,yy in opts:
                    rr=footprint(c,xx,yy,turn)
                    if solid.hits(fz,rr,.08) or reserved.hits(fz,rr):continue
                    score=dist+(0 if bid==p['source_subroom'] else 3)
                    if best is None or score<best[0]:best=(score,xx,yy,turn,rr,bid)
                    break
            if best is not None and best[0]<3:break
        if best is None:return False
        _,x,y,t,r,bid=best;p.update(at=[round(x,3),round(y,3),fz],turn=t,rect=[round(v,4) for v in r],subroom=bid,mount='floor')
        solid.add(fz,r,'prop');d['collision_features'].append(dict(id=p['id'],rect=p['rect'],z=fz,source='prototype_furnishing'))
        return True
    # Large apparatus first, then tables/storage, then chairs and small floor
    # pieces. Surface items and wall fixtures are supported, never floor piles.
    floor=[p for p in instances if catalog[str(p['model'])]['mount']=='floor' and p['model'] not in (151,156,158,173,182)]
    floor.sort(key=lambda p:(-math.prod(dimensions(catalog[str(p['model'])],0)),p['id']))
    for i,p in enumerate(floor):
        if fit_floor(p):placed_by_zone[p['zone']].append(p)
        else:unplaced.append(p)
    supports=collections.defaultdict(list)
    for zone in plan['zones']:
        for item in zone['items']:
            for src in item.get('source_objects',[]):
                if item['model']==1:
                    x,y,z=src['at'];w,h,hh=src['size'];at=[x+w/2,y+h/2,z+1.42]
                elif item['model']==5:
                    at=list(src['at']);at[2]+=src['size'][2]
                else:continue
                supports[zone['id']].append(dict(model=item['model'],at=at,size=src['size'][:2],used=0,id=src['id']))
    for p in floor:
        if 'at' not in p:continue
        c=catalog[str(p['model'])]
        if c['top']:
            supports[p['zone']].append(dict(model=p['model'],at=[*p['at'][:2],p['at'][2]+c['top']],size=dimensions(c,p['turn']),used=0,id=p['id']))
    grass_points={}
    for zid in ('S05','S15'):
        sample=next(p for p in instances if p['zone']==zid);x,y,w,h=sample['area'];z=sample['floor_z']
        pts=[];rng=random.Random(600+len(zid))
        for i in range(int((w-.9)/.5)):
            for j in range(int((h-.9)/.5)):
                xx=x+.6+i*.5;yy=y+.6+j*.5
                if abs(yy-(y+h/2))<1.25:continue
                if zid=='S15' and abs(xx-(x+w/2))<3.2 and abs(yy-(y+h/2))<3.2:continue
                if solid.hits(z,[xx-.08,yy-.08,.16,.16]):continue
                pts.append([round(xx+rng.uniform(-.1,.1),3),round(yy+rng.uniform(-.1,.1),3),z+.04])
        rng.shuffle(pts);grass_points[zid]=pts
    for p in instances:
        if p in floor:continue
        c=catalog[str(p['model'])];mount=c['mount'];ax,ay=p['source_xy'];z=p['floor_z'];x,y,w,h=p['area']
        if p['model']==151:
            assert grass_points[p['zone']],('not enough lawn for scheduled grass',p['zone'])
            p.update(at=grass_points[p['zone']].pop(),turn=0,mount='ground_detail')
        elif mount=='surface':
            opts=[s for s in supports[p['zone']] if s['model'] in c['support']]
            if not opts:
                # Document ledges / a tray on top of an existing cabinet are a
                # visible support, rather than floating papers and bottles.
                opts=list(supports[p['zone']])
            if not opts:
                # A small, wall-fixed shelf is the explicit support of last resort.
                xx=x+.5;yy=y+.5
                opts=[dict(model=0,at=[xx,yy,z+.95],size=[.65,.4],used=0,id=p['id']+'_ledge')]
                p['support_ledge']=True
            s=min(opts,key=lambda s:(s['used'],math.dist(s['at'][:2],[ax,ay])))
            col=s['used']%3;row=s['used']//3;s['used']+=1
            dx=(col-1)*min(.18,s['size'][0]/4);dy=(row%2-.5)*min(.18,s['size'][1]/3)
            p.update(at=[round(s['at'][0]+dx,3),round(s['at'][1]+dy,3),round(s['at'][2]+(row//2)*.25,3)],turn=0,mount=mount,support_id=s['id'])
        elif mount=='wall':
            # Place against an actual wall, never an imaginary edge of a plan bay.
            choices=[]
            fw,dep=dimensions(c,0)
            for r in wallsegs[z]:
                wx,wy,ww,hh=r
                if wx>x+w+.1 or wy>y+h+.1 or wx+ww<x-.1 or wy+hh<y-.1:continue
                horizontal=ww>hh
                if horizontal:
                    xx=min(max(ax,wx+fw/2+.08),wx+ww-fw/2-.08);yy=wy+hh/2
                    for side in (-1,1):
                        py=yy+side*(hh/2+dep/2+.025)
                        if x+.1<=xx<=x+w-.1 and y+.1<=py<=y+h-.1:choices.append((math.dist((xx,py),(ax,ay)),xx,py,0 if side==1 else 2))
                else:
                    yy=min(max(ay,wy+fw/2+.08),wy+hh-fw/2-.08);xx=wx+ww/2
                    for side in (-1,1):
                        px=xx+side*(ww/2+dep/2+.025)
                        if x+.1<=px<=x+w-.1 and y+.1<=yy<=y+h-.1:choices.append((math.dist((px,yy),(ax,ay)),px,yy,3 if side==1 else 1))
            # Tiny boundary wall segments may be narrower than a fixture, but
            # neighbouring collinear segments form a continuous supporting wall.
            choices.sort();chosen=None
            for _,xx,yy,t in choices:
                height={180:2.1,181:2.35,176:.35,117:1.35,70:.1,87:.8,92:.8,163:1.1}.get(p['model'],1.25)
                rr=footprint(c,xx,yy,t)
                keyz=round(z+height,2)
                if not decor.hits(keyz,rr,.035):chosen=(xx,yy,t,height,rr,keyz);break
            if chosen:
                xx,yy,t,height,rr,keyz=chosen;decor.add(keyz,rr)
                p.update(at=[round(xx,3),round(yy,3),round(z+height,3)],turn=t,mount=mount)
            else:
                # Exterior signs/bolts can belong to a fence or an existing
                # doorway; use a thin backing, with no new blocking floor box.
                p.update(at=[round(ax,3),round(ay,3),z+1.4],turn=0,mount='post_fixture',support_ledge=True)
        elif mount=='ceiling':
            p.update(at=[ax,ay,z+3.5],turn=0,mount=mount)
        else:
            # Low mats, gratings and grass are nonblocking surface details.
            p.update(at=[ax,ay,z+.035],turn=0,mount='ground_detail')
        placed_by_zone[p['zone']].append(p)
    (OUT/'unplaced.json').write_text(json.dumps(unplaced,ensure_ascii=False,indent=2),encoding='utf-8')
    if unplaced:
        print('UNPLACED',len(unplaced),collections.Counter(p['model'] for p in unplaced))
    for p in instances:
        if 'at' not in p:continue
        delta=round(math.dist(p['at'][:2],p['source_xy']),3)
        if delta>.05:corrections.append(dict(id=p['id'],zone=p['zone'],model=p['model'],moved_m=delta,reason='fit real bounds, preserve doors, aisles and hiding entries'))
        p.pop('anchor',None);p.pop('area',None)
        d['prototype_props'].append(p)
    # Outdoor route correction: green slabs previously covered every path in
    # S05/S15. Add the documented two-metre path and a level gazebo foundation.
    d['garden_paths']=[]
    for zid in ('S05','S15'):
        l,r=zones[zid];ox,oy,z=l['origin'];x,y,w,h=r['rect']
        d['garden_paths'].append(dict(zone=zid,rect=[ox+x+.4,oy+y+h/2-1,w-.8,2],z=z))
        if zid=='S15':d['garden_paths'].append(dict(zone=zid,rect=[ox+x+w/2-3,oy+y+h/2-3,6,6],z=z,kind='gazebo_base'))
    d['furnishing_audit']=dict(zones=len(plan['zones']),scheduled_new=sum(s['add'] for s in plan['summary']),
        placed_new=len(d['prototype_props']),preserved_existing=len(existing),unplaced=len(unplaced),
        adjusted_anchors=len(corrections),catalog_models=len(proposed),quality='simple geometric prototypes',
        interactive_scope='existing doors, hideouts and five movable props remain interactive; new set dressing is static')
    d['furnishing_captures']=[]
    for zid in ('R03','PS13','PS09','RH09','DG02','R31','E09','U_DG','S01','S05','S15','X01','S13'):
        zone=next(q for q in plan['zones'] if q['id']==zid);x,y,w,h=zone['rect_global'];z=zone['floor_z']
        # Clinical room centres are intentionally kept as a walking aisle.
        camera=[x+w/2,y+h/2,z+1.7];targets=[p for p in placed_by_zone[zid] if p['mount']=='floor']
        if targets:
            target=max(targets,key=lambda p:dimensions(catalog[str(p['model'])],0)[0]*dimensions(catalog[str(p['model'])],0)[1])['at']
            yaw=math.degrees(math.atan2(target[1]-camera[1],target[0]-camera[0]))
        else:yaw=0
        d['furnishing_captures'].append(dict(id='furnished_'+zid,at=camera,view=[-12,yaw]))
    (OUT/'placement_corrections.json').write_text(json.dumps(corrections,ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/'furnishing_audit.json').write_text(json.dumps(d['furnishing_audit'],ensure_ascii=False,indent=2),encoding='utf-8')
    with (OUT/'placed_instances.csv').open('w',encoding='utf-8-sig',newline='') as f:
        wr=csv.writer(f,delimiter=';');wr.writerow(['ID','Zone','Model','X','Y','Z','Turn','Mount','Source X','Source Y'])
        for p in d['prototype_props']:wr.writerow([p['id'],p['zone'],p['model'],*p['at'],p['turn'],p['mount'],*p['source_xy']])
    print(json.dumps(d['furnishing_audit'],ensure_ascii=False))
    return not unplaced

if __name__=='__main__':
    source=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
    # Use immutable PDF source, so repeated fitting never reserves old new props.
    d=json.loads((ROOT/'artifacts/unreal_hospital_v5/hospital_world_source.json').read_text(encoding='utf-8-sig'))
    ok=apply_furnishing(d);source.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    raise SystemExit(0 if ok else 1)
