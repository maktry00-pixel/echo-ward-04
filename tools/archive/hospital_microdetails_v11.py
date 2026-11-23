"""Small supported belongings, wall fittings and wear over the retained v10 map."""
import collections,json,math
from pathlib import Path
from hospital_clutter_v10 import Space,bounds,rotate
from hospital_furnishing_v6 import Index
ROOT=Path(__file__).resolve().parents[1];PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json';OUT=ROOT/'artifacts/unreal_hospital_v11'

def apply_microdetails(d):
    OUT.mkdir(parents=True,exist_ok=True);e=d['environment_dressing'];e['boxes']=[b for b in e['boxes'] if b.get('layer')!=11]
    catalog=d['prototype_catalog'];furniture=d['prototype_props']+d['environment_props'];space=Space();wall=Index()
    for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):wall.add(w['z'],w['rect'])
    for p in furniture:
        if p['mount'] not in ('floor','surface','wall'):continue
        for a in catalog[str(p['model'])]['parts']:
            off=rotate(a[:3],p['turn']);at=[p['at'][i]+off[i] for i in range(3)];sz=list(a[3:6])
            if p['turn']%2:sz[0],sz[1]=sz[1],sz[0]
            space.add(bounds(at,sz),p['id'])
    for n,b in enumerate(e['boxes']):
        if b['kind'] in ('lawn','grass_tuft','pavement','road_dash','tree_crown'):continue
        space.add(bounds(b['at'],b['size']),b.get('item_id','OLD11_'+str(n)))
    rooms={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};items=[]
    e['materials'].update({'rubber':dict(tint=[.075,.10,.11],roughness=.8),'rust':dict(tint=[.23,.09,.035],weathering=.55,roughness=.85),'faded_paint':dict(tint=[.36,.42,.37],roughness=.9),'warning':dict(tint=[.68,.48,.12],roughness=.8)})
    def shape(kind):
        if kind=='syringe':return [[0,0,.008,.014,.075,.016,'label'],[0,-.051,.008,.005,.03,.007,'steel'],[0,.052,.008,.008,.035,.008,'medicine'],[0,.072,.008,.035,.006,.01,'label']]
        if kind=='gauze_packet':return [[0,0,.008,.07,.07,.016,'paper'],[0,0,.017,.038,.01,.002,'blood'],[0,0,.017,.01,.038,.002,'blood']]
        if kind=='blister_pack':return [[0,0,.003,.065,.10,.006,'steel']]+[[x,y,.009,.015,.017,.012,'label'] for x in (-.016,.016) for y in (-.032,0,.032)]
        if kind=='ampoule_rack':return [[0,0,.004,.11,.045,.008,'steel']]+[[x,0,.028,.012,.012,.048,'medicine'] for x in (-.035,0,.035)]
        if kind=='tweezers':return [[x,0,.005,.005,.095,.01,'steel'] for x in (-.012,.012)]+[[0,-.045,.005,.025,.008,.01,'steel']]
        if kind=='pen':return [[0,0,.006,.009,.135,.012,'medicine'],[0,-.074,.006,.004,.013,.007,'steel'],[0,.05,.014,.012,.035,.004,'steel']]
        if kind=='stapler':return [[0,0,.008,.03,.08,.016,'steel'],[0,-.006,.026,.03,.067,.02,'dark']]
        if kind=='calculator':return [[0,0,.008,.06,.10,.016,'dark'],[0,-.03,.018,.046,.022,.003,'medicine']]+[[x,y,.019,.01,.01,.005,'label'] for x in (-.017,0,.017) for y in (0,.018,.036)]
        if kind=='staff_badge':return [[0,0,.003,.055,.085,.006,'paper'],[-.012,-.016,.007,.018,.022,.002,'medicine'],[.012,-.014,.007,.016,.004,.002,'dark'],[0,.016,.007,.037,.004,.002,'dark']]
        if kind=='sticky_notes':return [[0,0,.003,.065,.065,.006,'warning'],[0,-.012,.007,.04,.002,.001,'dark'],[0,.004,.007,.036,.002,.001,'dark']]
        if kind=='adhesive_tape':return [[0,0,.012,.055,.04,.024,'paper'],[0,0,.025,.022,.018,.002,'dark']]
        if kind=='soap':return [[0,0,.012,.055,.075,.024,'label'],[0,0,.025,.03,.012,.002,'medicine']]
        if kind=='toothbrush':return [[0,0,.006,.009,.12,.012,'medicine'],[0,-.065,.012,.02,.026,.02,'label']]
        if kind=='tea_packet':return [[0,0,.003,.055,.065,.006,'paper'],[0,0,.007,.03,.03,.002,'fabric']]
        if kind=='napkins':return [[0,0,.008,.10,.10,.016,'paper'],[0,0,.017,.085,.085,.003,'label']]
        if kind=='cutlery':return [[x,0,.004,.007,.10,.008,'steel'] for x in (-.015,.015)]+[[-.015,-.045,.005,.021,.022,.01,'steel'],[.015,-.045,.005,.017,.032,.01,'steel']]
        if kind=='rubber_gloves':
            return [[x,0,.003,.035,.045,.006,'label'] for x in (-.024,.024)]+[[x+f*.008,-.035,.003,.006,.035,.006,'label'] for x in (-.024,.024) for f in (-1,0,1)]
        if kind=='evidence_envelope':return [[0,0,.003,.10,.15,.006,'paper'],[0,-.035,.007,.07,.005,.002,'blood'],[0,.012,.007,.065,.003,.002,'dark']]
        if kind=='vent_grille':return [[0,0,0,.36,.025,.18,'paint']]+[[0,.016,z,.31,.008,.009,'dark'] for z in (-.06,-.03,0,.03,.06)]
        if kind=='socket':return [[0,0,0,.085,.023,.075,'label'],[-.014,.014,0,.008,.004,.011,'dark'],[.014,.014,0,.008,.004,.011,'dark']]
        if kind=='sanitizer':return [[0,0,0,.09,.05,.18,'label'],[0,.028,0,.06,.008,.07,'medicine'],[0,.04,.085,.04,.035,.02,'steel']]
        if kind=='medical_notice':return [[0,0,0,.28,.018,.21,'paper'],[-.075,.012,.035,.075,.004,.025,'medicine'],[-.075,.012,.035,.025,.004,.075,'medicine']]+[[.04,.012,z,.11,.004,.008,'dark'] for z in (-.06,-.025,.01,.045)]
        if kind=='wall_scuff':return [[x,0,z,.07,.002,.009,'faded_paint'] for x,z in ((-.06,0),(0,.02),(.05,-.025))]
        if kind=='wall_crack':return [[0,0,0,.006,.002,.17,'dark'],[.022,0,.10,.048,.002,.006,'dark'],[.045,0,.14,.006,.002,.075,'dark']]
        if kind=='label_barcode':return [[-.048+i*.011,0,0,.003 if i%2 else .006,.001,.045,'dark'] for i in range(9)]
        if kind=='bin_wrapper':return [[0,0,.003,.06,.085,.006,'paper'],[.016,.008,.009,.032,.04,.006,'warning']]
        if kind=='floor_scuff':return [[x,0,.001,.035,.14,.002,'faded_paint'] for x in (-.035,.035)]
        if kind=='fallen_leaves':return [[x,y,.002,.035,.06,.004,'rust'] for x,y in ((-.05,0),(0,.02),(.045,-.02))]
        if kind=='pole_bolts':return [[x,y,.018,.025,.025,.036,'steel'] for x in (-.10,.10) for y in (-.10,.10)]
        raise ValueError(kind)
    def put(kind,parent,base,turn=0,mount='surface',support=None,wall_samples=None):
        rendered=[]
        for a in shape(kind):
            off=rotate(a[:3],turn);at=[base[i]+off[i] for i in range(3)];sz=list(a[3:6])
            if turn%2:sz[0],sz[1]=sz[1],sz[0]
            if space.hits(bounds(at,sz),parent['id']):return False
            rendered.append(dict(at=[round(v,6) for v in at],size=sz,material=a[6],kind=kind,zone=parent['zone'],solid=False,layer=11))
        bb=[min(b['at'][i]-b['size'][i]/2 for b in rendered) for i in range(3)]+[max(b['at'][i]+b['size'][i]/2 for b in rendered) for i in range(3)]
        if support:
            x,y,w,h,z=support
            if bb[0]<x-.0001 or bb[1]<y-.0001 or bb[3]>x+w+.0001 or bb[4]>y+h+.0001 or abs(bb[2]-z)>.012:return False
        ident=f'DETAIL11_{len(items):05}'
        for b in rendered:b['item_id']=ident;e['boxes'].append(b)
        items.append(dict(id=ident,kind=kind,parent=parent['id'],zone=parent['zone'],mount=mount,bounds=bb,support=support,wall_samples=wall_samples,parts=len(rendered)))
        space.add(bb,ident);return True

    # Small work items on genuinely free tabletop space. Clinical supplies are
    # kept in clinical areas; offices, catering and workshops get their own kit.
    for p in furniture:
        if p['mount']!='floor':continue
        m=p['model'];c=catalog[str(m)]
        if m not in (4,16,17,18,19,20,21,22,23,24,37,38,39,107,112,130):continue
        _,r=rooms.get(p['zone'],({},{}));name=r.get('name','').lower();template=r.get('furniture_template','')
        if m==112:kinds=['soap','toothbrush','adhesive_tape']
        elif m in (17,18,19,24) or any(k in name for k in ('кафе','кухн','столов')):kinds=['tea_packet','napkins','cutlery','pen']
        elif m in (21,22,107,130) or template in ('industrial','utility','service'):kinds=['pen','adhesive_tape','calculator','rubber_gloves','evidence_envelope']
        elif m in (16,23) or template in ('office','registration'):kinds=['pen','stapler','staff_badge','calculator','sticky_notes','evidence_envelope']
        else:kinds=['syringe','gauze_packet','blister_pack','ampoule_rack','tweezers','rubber_gloves','staff_badge']
        b=c['bounds'];fw,fd=b[3]-b[0],b[4]-b[1];w,h=(fd,fw) if p['turn']%2 else (fw,fd);x,y,z=p['at'];support=[x-w/2+.03,y-h/2+.03,w-.06,h-.06,z+c['top']]
        slots=[(xx,yy) for xx in (-.36,-.18,0,.18,.36) for yy in (-.20,0,.20)]
        for kind in kinds:
            for xx,yy in slots:
                off=rotate([xx,yy,c['top']+.003],p['turn']);base=[p['at'][i]+off[i] for i in range(3)]
                if put(kind,p,base,p['turn'],support=support):break

    # Use the actual wall-facing direction from the existing camera assembly.
    byzone=collections.defaultdict(dict)
    for b in list(e['boxes']):
        if b.get('layer')!=11 and b['kind'] in ('radiator','camera_housing'):byzone[b['zone']][b['kind']]=b
    for zid,pair in byzone.items():
        if len(pair)<2:continue
        rad,cam=pair['radiator'],pair['camera_housing'];dx,dy=cam['at'][0]-rad['at'][0],cam['at'][1]-rad['at'][1]
        t=0 if abs(dy)>abs(dx) and dy>0 else 2 if abs(dy)>abs(dx) else 3 if dx>0 else 1
        nx,ny,_=rotate([0,1,0],t);x,y,rz=rad['at'];z=round(rz-.7,3);parent=dict(id='wall_'+zid,zone=zid)
        for kind,lx,height,width in [('vent_grille',0,3.48,.36),('socket',.12,1.10,.085),('sanitizer',-.08,1.33,.09),('medical_notice',0,1.98,.28),('wall_scuff',0,.20,.20),('wall_crack',-.08,.40,.12)]:
            # Test backing along the full fixture width against real wall spans,
            # including doorway gaps. Nothing is attached to an imagined wall.
            samples=[];backed=True
            for u in (-width/2,0,width/2):
                along=rotate([lx+u,0,0],t);xx,yy=x+along[0],y+along[1]
                found=None
                for depth in (.11,.15,.20,.25):
                    px,py=xx-nx*depth,yy-ny*depth
                    if wall.hits(z,[px-.008,py-.008,.016,.016]):found=[px,py,z];break
                if found is None:backed=False;break
                samples.append(found)
            if not backed:continue
            off=rotate([lx,-.077,height],t);put(kind,parent,[x+off[0],y+off[1],z+height],t,'wall',wall_samples=samples)

    # Barcodes refine existing labels, rather than adding more large boxes.
    for b in list(e['boxes']):
        if b.get('layer')!=10 or b['material']!='label' or b['kind'] not in ('supply_box','parcel','food_carton'):continue
        if int(b['item_id'].split('_')[-1])%4:continue
        p=next((i for i in d['clutter_dressing']['items'] if i['id']==b['item_id']),None)
        if not p:continue
        dx,dy=b['at'][0]-p['base'][0],b['at'][1]-p['base'][1]
        t=0 if abs(dy)>abs(dx) and dy>0 else 2 if abs(dy)>abs(dx) else 3 if dx>0 else 1
        normal=rotate([0,1,0],t);depth=min(b['size'][:2])/2+.001
        put('label_barcode',dict(id=p['id'],zone=p['zone']),[b['at'][0]+normal[0]*depth,b['at'][1]+normal[1]*depth,b['at'][2]],t,'label')

    for p in furniture:
        if p['mount']!='floor':continue
        x,y,z=p['at']
        if p['model']==120:
            put('bin_wrapper',p,[x+.26,y,z+.004],mount='floor',support=[x+.20,y-.10,.2,.2,z])
        if p['model'] in (1,3,25,29) and sum(map(ord,p['id']))%4==0:
            b=catalog[str(p['model'])]['bounds'];w,h=b[3]-b[0],b[4]-b[1]
            off=rotate([w/2+.13,0,.004],p['turn']);put('floor_scuff',p,[x+off[0],y+off[1],z+off[2]],p['turn'],'floor',support=[x-3,y-3,6,6,z])
    for n,b in enumerate([b for b in e['boxes'] if b['kind']=='grass_tuft'][::14]):
        x,y,_=b['at'];put('fallen_leaves',dict(id='lawn_'+str(n),zone='ST'),[x+.20,y,.03],mount='ground',support=[x-.2,y-.2,.8,.8,.025])
    for n,b in enumerate([b for b in e['boxes'] if b['kind']=='lamp_post']):
        x,y,_=b['at'];put('pole_bolts',dict(id='pole_'+str(n),zone='ST'),[x,y,.005],mount='ground',support=[x-.2,y-.2,.4,.4,0])
    counts=dict(collections.Counter(i['kind'] for i in items))
    d['microdetails_dressing']=dict(version=11,items=items,counts=counts,quality='prototype',limits=['Small fittings, labels and supplies are decorative; they do not add inventory or electrical gameplay.','Wall wear and labels use simple geometry; final textures and models remain future art.'])
    d['review_revision']=11;d['furnishing_captures']=[c for c in d['furnishing_captures'] if not c.get('microdetails')]
    # Explicit close viewpoints show new details instead of repeating a wide
    # room photograph in which a syringe would be a single pixel.
    for name,kind,preferred in [('clinical','syringe','R03'),('office','calculator','R01'),('cafe','tea_packet','PS13'),('storage','label_barcode','SV'),('wall','medical_notice','R03')]:
        candidates=[i for i in items if i['kind']==kind]
        candidates.sort(key=lambda i:(i['zone']!=preferred,not i['zone'].startswith(preferred),i['id']))
        chosen=None
        for item in candidates:
            if item['zone'] not in rooms:continue
            s,r=rooms[item['zone']];ox,oy,z=s['origin'];rx,ry,w,h=r['rect'];b=item['bounds'];target=[(b[a]+b[a+3])/2 for a in range(3)]
            if kind=='medical_notice':
                pair=byzone[item['zone']];a=pair['radiator']['at'];c=pair['camera_housing']['at'];v=[c[0]-a[0],c[1]-a[1]];length=math.hypot(*v)
                eyes=[[target[0]+v[0]/length*1.2,target[1]+v[1]/length*1.2,target[2]+.18]]
            else:
                eyes=[[target[0]+dx,target[1]+dy,max(z+1.45,target[2]+.55)] for dx,dy in ((0,1.0),(0,-1.0),(1.0,0),(-1.0,0),(.7,.7),(-.7,-.7))]
            for eye in eyes:
                if not(ox+rx+.15<eye[0]<ox+rx+w-.15 and oy+ry+.15<eye[1]<oy+ry+h-.15):continue
                if wall.hits(z,[eye[0]-.05,eye[1]-.05,.1,.1]) or space.hits(bounds(eye,[.08,.08,.08])):continue
                chosen=(eye,target,item['id']);break
            if chosen:break
        if chosen:
            eye,target,ident=chosen;d['furnishing_captures'].append(dict(id='details_'+name,at=eye,target=target,microdetails=True,envelope=False,detail_item=ident))
        else:raise ValueError(('No clear detail camera',name))
    result=dict(items=len(items),parts=sum(i['parts'] for i in items),types=len(counts),zones=len({i['zone'] for i in items}),counts=counts)
    (OUT/'details_inventory.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(result,ensure_ascii=False));return result

if __name__=='__main__':
    d=json.loads(PATH.read_text(encoding='utf-8'));apply_microdetails(d);PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
