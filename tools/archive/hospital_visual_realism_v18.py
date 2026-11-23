"""Measured native furniture, exterior joinery and overhead fixture detail.

Uses the project's already imported, attributed Poly Haven scans. No room,
story, collision or movement geometry is removed or rescaled.
"""
import collections,hashlib,json,math,shutil
from pathlib import Path
from hospital_clutter_v10 import rotate
from hospital_clutter_v10 import Space,bounds
from hospital_furnishing_v6 import Index
ROOT=Path(__file__).resolve().parents[1];PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json';OUT=ROOT/'artifacts/unreal_hospital_v18'

def apply(d):
    e=d['environment_dressing'];items=[]
    def emit(kind,zone,parts,at,turn=0,extra=None):
        ident=f'REAL18_{kind}_{len(items):05}';boxes=[]
        for q in parts:
            off=rotate(q[:3],turn);sz=q[3:6].copy()
            if turn%2:sz[0],sz[1]=sz[1],sz[0]
            boxes.append(dict(at=[round(at[i]+off[i],6) for i in range(3)],size=sz,material=q[6],kind=kind,zone=zone,solid=False,layer=18,item_id=ident))
        e['boxes'].extend(boxes);items.append(dict(id=ident,kind=kind,zone=zone,parts=len(boxes),at=at,turn=turn,**(extra or {})))
    # The source textured material instances were cooked already. Separate
    # wood from enamel and rubber; do not texture labels or light emitters.
    for key in ('wood','camp_wood','camp_crate','v12_wood'):
        if key in e['materials']:
            e['materials'][key]['source']='1246be1c8b1b3b70';e['materials'][key]['roughness']=.74
    replacements={
        '1':dict(mesh='SM_Bed',asset='old_bed_frame',yaw=90,keep_materials=['white','paper']),
        '3':dict(mesh='SM_Bed',asset='old_bed_frame',yaw=90,keep_materials=['white','paper']),
        '4':dict(mesh='SM_Nightstand',asset='painted_wooden_nightstand',yaw=0,keep_materials=[]),
        '13':dict(mesh='SM_Shelf',asset='steel_frame_shelves_01',yaw=90,keep_materials=[]),
        '25':dict(mesh='SM_Chair',asset='SchoolChair_01',yaw=0,keep_materials=[]),
    }
    allprops=d['prototype_props']+d['environment_props'];selected=[p for p in allprops if p['mount']=='floor' and str(p['model']) in replacements]
    # Weather lips and reveal trims sit outside actual exterior openings.
    sites={s['id']:s for s in d['locations']};window_count=0
    for w in d['windows']:
        if w.get('role')!='external_view':continue
        s=sites[w['site']];ox,oy,z=s['origin'];side=w['side'];ns=side in ('north','south');width=w['width'];sill=1.1;head=2.1
        xx=ox+w['at'][0]+(w.get('offset',0) if ns else 0);yy=oy+w['at'][1]+(w.get('offset',0) if not ns else 0)
        nx,ny={'north':(0,-1),'south':(0,1),'west':(-1,0),'east':(1,0)}[side]
        # Local positive Y always faces outside the facade.
        t={'south':0,'west':1,'north':2,'east':3}[side]
        p=[[0,.12,sill-.035,width+.16,.24,.07,'camp_concrete'],[0,.23,sill-.073,width+.18,.025,.025,'camp_steel'],[0,.105,head+.025,width+.12,.05,.05,'camp_steel']]
        for a in (-1,1):p.append([a*(width/2+.018),.105,(sill+head)/2,.035,.05,head-sill,'camp_steel'])
        emit('exterior_window_joinery',w['zone'],p,[xx,yy,z],t,dict(window_index=window_count,wall_side=side,site=s['id']))
        window_count+=1
    # Light housings get a shallow metal rim and real-size louvre blades,
    # instead of a bare floating luminous rectangle. All above headroom.
    lights=[b for b in e['boxes'] if b['kind']=='ceiling_lamp' and b['material']=='lamp']
    for b in lights:
        w,h,depth=b['size'];at=b['at'];parts=[]
        for sign in (-1,1):
            parts.append([0,sign*(h/2+.012),0,w+.04,.018,.035,'camp_steel'])
            parts.append([sign*(w/2+.012),0,0,.018,h+.04,.035,'camp_steel'])
        for n in (-.3,-.1,.1,.3):parts.append([w*n,0,-depth/2-.004,.008,h,.012,'camp_cream'])
        for sign in (-1,1):parts.append([sign*w*.3,0,.1325,.012,.012,.265,'camp_steel'])
        emit('luminaire_housing',b['zone'],parts,at,extra=dict(parent_at=at,parent_size=b['size']))
    # Mechanical detail on the recent floor kits, placed inside their original
    # bounds: drawer splits, label pockets and handle backplates.
    byid=collections.defaultdict(list)
    for b in e['boxes']:
        if b.get('layer')==17:byid[b['item_id']].append(b)
    refined=[]
    for q in d['finish_upgrade']['items']:
        if q.get('mount')!='floor':continue
        bodies=[b for b in byid[q['id']] if b['material']=='camp_cream' and b['size'][0]>.55 and b['size'][1]>.35 and b['size'][2]>.45]
        if not bodies:continue
        body=max(bodies,key=lambda b:math.prod(b['size']));bw,bd,bh=body['size'];t=q['turn']
        if t%2:bw,bd=bd,bw
        p=[]
        for frac in (.33,.66):p.append([0,bd/2+.001,-bh/2+bh*frac,bw-.06,.006,.009,'camp_rubber'])
        p.extend([[0,bd/2+.006,bh*.22,.15,.009,.055,'camp_steel'],[0,bd/2+.012,bh*.22,.11,.004,.032,'camp_cream']])
        emit('cabinet_hardware',q['zone'],p,body['at'],t,dict(parent=q['id'],parent_bounds=q['bounds']))
        refined.append(q['id'])
    # Reuse well tested room views without moving their cameras; add furniture
    # replacement samples where the selected item has a clear approach.
    captures=[]
    for q in list(d['furnishing_captures']):
        if 'v17_room_' in q['id']:
            q=dict(q);q['id']=q['id'].replace('v17_room_','v18_room_');d['furnishing_captures'].append(q);captures.append(q['id'])
    # Explicit native samples for each imported furniture type, selected from
    # real existing props. Their front-access routes have already been audited.
    wall_index=Index()
    for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf8')):wall_index.add(w['z'],w['rect'])
    zone_spaces={}
    zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
    def camera_for(p,model):
        zid=p['zone'];s,r=zones[zid];z=p['at'][2]
        if zid not in zone_spaces:
            space=Space()
            for prop in allprops:
                if prop['zone']!=zid:continue
                for a in d['prototype_catalog'][str(prop['model'])]['parts']:
                    off=rotate(a[:3],prop['turn']);sz=list(a[3:6])
                    if prop['turn']%2:sz[0],sz[1]=sz[1],sz[0]
                    space.add(bounds([prop['at'][j]+off[j] for j in range(3)],sz),prop['id'])
            for b in e['boxes']:
                if b['zone']==zid:space.add(bounds(b['at'],b['size']),b.get('item_id','env'))
            zone_spaces[zid]=space
        space=zone_spaces[zid];bb=d['prototype_catalog'][model]['bounds'];target=[p['at'][0],p['at'][1],z+(bb[2]+bb[5])/2]
        for dx,dy in ((1.8,3),(-1.8,3),(0,3.2),(2.2,2.2),(-2.2,2.2),(0,2.6),(1.3,2.2),(-1.3,2.2)):
            off=rotate([dx,dy,0],p['turn']);eye=[p['at'][0]+off[0],p['at'][1]+off[1],z+1.65];ox,oy,_=s['origin'];x,y,rw,rh=r['rect']
            if not(ox+x+.1<eye[0]<ox+x+rw-.1 and oy+y+.1<eye[1]<oy+y+rh-.1):continue
            if space.hits(bounds(eye,[.2]*3)) or wall_index.hits(z,[eye[0]-.1,eye[1]-.1,.2,.2]):continue
            ray=[[eye[j]+(target[j]-eye[j])*t/40 for j in range(3)] for t in range(1,40)]
            if any(space.hits(bounds(a,[.03]*3),p['id']) or wall_index.hits(z,[a[0]-.015,a[1]-.015,.03,.03]) for a in ray):continue
            return eye,target
    for model in replacements:
        candidates=[p for p in selected if str(p['model'])==model and p['zone'] not in ('DG25','DG26')]
        if not candidates:continue
        for p in candidates:
            view=camera_for(p,model)
            if not view:continue
            eye,target=view;ident='v18_scan_'+model
            d['furnishing_captures'].append(dict(id=ident,at=eye,target=target,detail_item=p['id']));captures.append(ident);break
    # Exterior views from the same verified fixed camera positions as v16.
    for q in list(d['furnishing_captures']):
        if q['id'] in ('campus_v16_plate_X01','campus_v16_plate_PS09','v16_spine_supports','campus_v15_forest','v17_guard_1'):
            q=dict(q);q['id']='v18_'+q['id'];d['furnishing_captures'].append(q);captures.append(q['id'])
    d['review_revision']=18
    counts=dict(scanned_furniture=len(selected),by_model=dict(collections.Counter(str(p['model']) for p in selected)),external_windows=window_count,luminaire_housings=len(lights),refined_cabinets=len(refined),new_assemblies=len(items),new_parts=sum(q['parts'] for q in items),new_views=len(captures))
    d['visual_realism']=dict(version=18,mesh_replacements=replacements,replacement_count=len(selected),items=items,refined_cabinets=refined,limits=['Imported visuals fit existing furniture envelopes; original collision and hide spaces retained.','Construction details are decorative and do not simulate drainage or medical equipment.','Prototype architecture remains; this is not a completed photorealistic art pass.'])
    (OUT/'inventory.json').write_text(json.dumps(counts,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(counts),flush=True)

if __name__=='__main__':
    OUT.mkdir(parents=True,exist_ok=True);base=OUT/'baseline_v17.json'
    if not base.exists():
        assert json.loads(PATH.read_text(encoding='utf8'))['review_revision']==17;shutil.copy2(PATH,base)
    d=json.loads(base.read_text(encoding='utf8'));apply(d);PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
