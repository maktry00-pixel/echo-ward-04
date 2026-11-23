"""Prototype dressing, fitted around retained navigation and story anchors.

This is a separate layer: the user's numbered PDF schedule stays unchanged.
All dimensions are metres. Attached details and ground paint have no collision;
additional floor furniture has the same bounds in the navigation data and game.
"""
import collections, hashlib, json, math, random
from pathlib import Path
from hospital_furnishing_v6 import Index, overlap

ROOT=Path(__file__).resolve().parents[1]
PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
OUT=ROOT/'artifacts/unreal_hospital_v9'

def apply_environment(d):
    OUT.mkdir(parents=True,exist_ok=True)
    d['collision_features']=[f for f in d['collision_features'] if not f['id'].startswith('ENV9_')]
    sites={s['id']:s for s in d['locations']}
    rooms={r['id']:(s,r) for s in d['locations'] for r in s['rooms']}
    catalog=d['prototype_catalog'];solid=Index();reserved=Index()
    walls=collections.defaultdict(list)
    for q in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):
        solid.add(q['z'],q['rect']);walls[round(q['z'],2)].append(q['rect'])
    for q in d['collision_features']:solid.add(q['z'],q['rect'])
    def reserve(z,a,b,r=.52):reserved.add(z,[min(a[0],b[0])-r,min(a[1],b[1])-r,abs(a[0]-b[0])+2*r,abs(a[1]-b[1])+2*r])
    seen=set()
    for route in d['navigation_routes']+d['bay_navigation_routes']:
        for a,b in zip(route['points'],route['points'][1:]):
            k=tuple(a+b)
            if a[2]==b[2] and k not in seen:reserve(a[2],a,b);seen.add(k)
    for s,r in rooms.values():
        ox,oy,z=s['origin'];x,y,w,h=r['rect'];x+=ox;y+=oy
        if r['enclosed']:reserve(z,[ox+r['door'][0],oy+r['door'][1]],[x+w/2,y+h/2],.67)
        for e in [dict(at=r['door'],side=r['door_side'])]+r.get('extra_doors',[]):
            xx,yy=ox+e['at'][0],oy+e['at'][1];ns=e['side'] in ('north','south')
            reserve(z,[xx if ns else xx-1.8,yy-1.8 if ns else yy],[xx if ns else xx+1.8,yy+1.8 if ns else yy],.65)
    for h in d['hideouts']:
        x,y,z=h['at'];w,dep,_=h['size'];reserved.add(z,[x-.2,y-.2,w+.4,dep+1.8])
    for n in d['escape_niches']:
        x,y,z=n['entry'];reserved.add(z,n['rect']);reserve(z,n.get('corridor_point',[x,y,z]),[x,y],.7)
    for s in d['stairs']:
        x,y,_,_=s['rect'];z=s['z_min']
        while z<s['z_max']+.01:reserved.add(round(z,2),[x-.6,y-.6,4.8,10]);z+=4.8
    for z in (-9.6,-4.8,0,4.8,9.6,14.4):reserved.add(z,[47.2,3.2,5.6,9.6])
    for q in d['story_gameplay']['placements']+d['story_gameplay']['mobs']:
        x,y,z=q['at'];r=1.1 if q.get('kind') in ('person','scene') or 'species' in q else .7
        reserved.add(z,[x-r,y-r,r*2,r*2])

    props=[];boxes=[];white=[];red=[];coverage=[];camera_count=0;fixture_count=collections.Counter()
    def box(at,size,mat,kind,zone='',solid_box=False):
        boxes.append(dict(at=[round(v,4) for v in at],size=[round(v,4) for v in size],material=mat,kind=kind,zone=zone,solid=solid_box))
        fixture_count[kind]+=1
    def dims(model,turn):
        b=catalog[str(model)]['bounds'];w,h=b[3]-b[0],b[4]-b[1]
        return (h,w) if turn%2 else (w,h)
    def place(model,bay,area,z,site,zone):
        x,y,w,h=area;opts=[]
        for turn in (0,1):
            fw,fh=dims(model,turn)
            if fw+.4>w or fh+.4>h:continue
            xs=[x+fw/2+.2,x+w-fw/2-.2];ys=[y+fh/2+.2,y+h-fh/2-.2]
            xs += [x+fw/2+.2+i*.4 for i in range(int((w-fw-.4)/.4)+1)]
            ys += [y+fh/2+.2+i*.4 for i in range(int((h-fh-.4)/.4)+1)]
            for xx in xs:
                for yy in ys:
                    rr=[xx-fw/2,yy-fh/2,fw,fh]
                    edge=min(rr[0]-x,rr[1]-y,x+w-rr[0]-fw,y+h-rr[1]-fh)
                    opts.append((edge,xx,yy,turn,rr))
        opts.sort()
        for _,xx,yy,t,rr in opts:
            if solid.hits(z,rr,.10) or reserved.hits(z,rr,.04):continue
            ident=f'ENV9_{bay}_{model}_{len(props):04}'
            p=dict(id=ident,model=model,at=[round(xx,4),round(yy,4),z],turn=t,rect=[round(v,4) for v in rr],mount='floor',subroom=bay,zone=zone,site=site)
            props.append(p);solid.add(z,rr);d['collision_features'].append(dict(id=ident,rect=p['rect'],z=z,source='environment_dressing_v9'));return True
        return False

    bybay=collections.Counter(p.get('subroom') for p in d['prototype_props'] if p.get('mount')=='floor')
    def kit(name,template):
        name=name.lower()
        for words,models in [(['сануз','душ','туалет'],[114,112,120]),(['палат','бокс','спаль'],[1,4,25]),(['склад','кладов','архив','хран'],[13,109,108]),(['кафе','столов','кухн','буфет'],[17,25,123]),(['операц','перевяз','процед'],[31,37,5]),(['лаборат','анализ'],[20,5,25]),(['гараж','ремонт','мастер'],[21,11,107]),(['регист','приём','вахта','пост','диспетч'],[16,26,9]),(['магаз','супермар','аптека'],[15,125,132])]:
            if any(w in name for w in words):return models
        return {'ward':[1,4,25],'old_ward':[1,4,25],'isolation':[1,5,37],'rehab':[31,25,4],
            'therapy':[27,19,25],'office':[16,26,9],'residential':[3,7,25],'industrial':[13,21,109],
            'utility':[13,109,120],'laboratory':[20,5,25],'registration':[16,25,9],'service':[13,109,38],
            'diagnostic':[31,37,25]}.get(template,[25,120,4])
    for b in d['subrooms']:
        s,r=rooms[b['zone']];ox,oy,z=s['origin'];x,y,w,h=b['rect'];area=[ox+x,oy+y,w,h]
        if not r['enclosed']:continue
        models=kit(r['name'],r.get('furniture_template',''))
        count=0
        for m in models[:max(1,3-min(bybay[b['id']],2))]:count+=place(m,b['id'],area,z,s['id'],r['id'])
        if not bybay[b['id']] and not count:count+=place(120,b['id'],area,z,s['id'],r['id'])
        # Every logical bay gets a visible ceiling fitting, even where the
        # escape sweep leaves insufficient space for another solid object.
        cx,cy=area[0]+w/2,area[1]+h/2
        box([cx,cy,z+3.58],[.95,.22,.07],'steel','lamp_housing',r['id'])
        box([cx,cy,z+3.535],[.8,.16,.015],'lamp','ceiling_lamp',r['id']);white.append([cx,cy,z+3.15])
        coverage.append(dict(bay=b['id'],zone=r['id'],existing_floor=bybay[b['id']],added_floor=count,ceiling_fixture=True))

    # A wall fixture is fitted to a real solid wall interval, never across a
    # door aperture. It does not replace the independent physical door actors.
    def wall_anchor(s,r):
        ox,oy,z=s['origin'];x,y,w,h=r['rect'];x+=ox;y+=oy;cx=x+w/2;cy=y+h/2;opts=[]
        for a in walls[round(z,2)]:
            wx,wy,ww,hh=a
            if ww>=.6 and hh<.3:
                px=max(wx+.3,min(cx,wx+ww-.3));py=wy+hh/2
                for side in (-1,1):
                    yy=py+side*(hh/2+.10)
                    if x+.15<px<x+w-.15 and y+.15<yy<y+h-.15:opts.append((abs(px-cx)+abs(yy-cy),px,yy,0 if side==1 else 2))
            elif hh>=.6 and ww<.3:
                py=max(wy+.3,min(cy,wy+hh-.3));px=wx+ww/2
                for side in (-1,1):
                    xx=px+side*(ww/2+.10)
                    if x+.15<xx<x+w-.15 and y+.15<py<y+h-.15:opts.append((abs(xx-cx)+abs(py-cy),xx,py,3 if side==1 else 1))
        return min(opts)[1:] if opts else None
    for zid,(s,r) in rooms.items():
        if not r['enclosed']:continue
        a=wall_anchor(s,r)
        if not a:continue
        x,y,turn=a;z=s['origin'][2]
        def mounted(dx,dy,dz,w,dep,h,mat,kind):
            q=[(dx,dy),(-dy,dx),(-dx,-dy),(dy,-dx)][turn]
            box([x+q[0],y+q[1],z+dz],[dep,w,h] if turn%2 else [w,dep,h],mat,kind,zid)
        mounted(0,0,.7,.60,.12,.55,'paint','radiator')
        for dx in (-.21,-.07,.07,.21):mounted(dx,.07,.7,.035,.035,.46,'steel','radiator_fin')
        mounted(0,0,2.85,.28,.13,.18,'steel','beacon_backplate')
        mounted(0,.10,2.85,.16,.12,.15,'emergency','red_beacon');red.append([x,y,z+2.8])
        mounted(.30,0,1.7,.12,.05,.18,'steel','switch')
        mounted(.30,.035,1.7,.045,.02,.06,'dark','switch_handle')
        # CCTV prototypes: bracket, hood, dark lens and a tiny red status LED.
        mounted(0,.04,3.25,.055,.28,.055,'steel','camera_bracket')
        mounted(0,.22,3.24,.20,.34,.15,'paint','camera_housing')
        mounted(0,.40,3.24,.10,.015,.085,'dark','camera_lens')
        mounted(.075,.40,3.28,.02,.02,.02,'emergency','camera_led');camera_count+=1
        # Safety information and a dark red extinguisher wall silhouette.
        mounted(-.35,0,1.5,.12,.12,.45,'blood','extinguisher')
        mounted(-.35,.03,1.77,.10,.06,.06,'steel','extinguisher_handle')
        mounted(0,0,2.45,.40,.025,.16,'exit','exit_sign')
        mounted(0,.017,2.45,.20,.015,.025,'lamp','exit_arrow')
        # Sparse stains with broken edges, attached to solid walls at ankle /
        # hand height, rather than covering every room with a uniform red sheet.
        if sum(map(ord,zid))%3==0:
            for i in range(4):mounted(-.15+i*.08,.073,.25+i*.1,.13,.006,.38-i*.04,'blood','wall_stain')
        cx=s['origin'][0]+r['rect'][0]+r['rect'][2]*.32;cy=s['origin'][1]+r['rect'][1]+r['rect'][3]*.64
        if sum(map(ord,zid))%4==0:box([cx,cy,z+.014],[.70,.45,.012],'blood','floor_stain',zid)

    # Explicit surface plan. Existing building / parking / service footprints
    # and covered passage slabs are retained; new lawn occupies the spare land.
    roads=[[-174,-186,12,556],[262,-186,12,556],[-174,346,448,12],[-174,106,448,10],[-174,216,448,10],[-32,-186,12,544],[122,-186,12,544],[44,-186,12,556],[-174,-58,448,12]]
    protected=[]
    for s in d['locations']:
        if s['origin'][2]!=0:continue
        ox,oy,_=s['origin']
        if s['id']=='ST':
            for r in s['rooms']:
                x,y,w,h=r['rect'];protected.append([ox+x-.8,oy+y-.8,w+1.6,h+1.6])
        else:
            x,y,w,h=s['bounds'];protected.append([ox+x-1,oy+y-1,w+2,h+2])
    for p in d['passages']:
        if p[4]==0:protected.append([p[0]-.5,p[1]-.5,p[2]+1,p[3]+1])
    for s in d['stairs']:
        if s['z_min']<=0<=s['z_max']:
            x,y,w,h=s['rect'];protected.append([x-1,y-1,w+2,h+2])
    lawns=[];grass_rects=[]
    for y in range(-186,374,4):
        start=None
        for x in range(-176,280,4):
            r=[x,y,4,4];ok=x<276 and not any(overlap(r,q) for q in protected+roads)
            if ok and start is None:start=x
            if not ok and start is not None:grass_rects.append([start,y,x-start,4]);start=None
        if start is not None:grass_rects.append([start,y,276-start,4])
    for r in grass_rects:
        x,y,w,h=r;box([x+w/2,y+h/2,.015],[w,h,.02],'grass','lawn');lawns.append(r)
    rng=random.Random(903)
    # Low tufts, shrubs and tree silhouettes stay exclusively on the new lawn.
    for i,r in enumerate(lawns):
        x,y,w,h=r
        for j in range(max(1,int(w*h/12))):
            xx=x+.3+rng.random()*(w-.6);yy=y+.3+rng.random()*(h-.6)
            box([xx,yy,.13],[.035,.24,.23],'foliage','grass_tuft')
            box([xx,yy,.16],[.22,.035,.29],'grass','grass_tuft')
        if i%4==0 and w>6:
            xx=x+w*.5;yy=y+2
            box([xx,yy,.42],[1.3,.9,.8],'foliage','shrub')
            if i%12==0:
                box([xx,yy,1.75],[.20,.20,3.5],'wood','tree_trunk')
                box([xx,yy,3.25],[2.1,1.9,1.6],'foliage','tree_crown')
    # Road paint is fitted only where a road is exposed, outside structures.
    for r in roads:
        x,y,w,h=r;vertical=h>w
        for t in range(0,int(max(w,h)),8):
            xx=x+w/2 if vertical else x+t+2;yy=y+t+2 if vertical else y+h/2
            rr=[xx-.1,yy-1.5,.2,3] if vertical else [xx-1.5,yy-.1,3,.2]
            if not any(overlap(rr,q) for q in protected):box([xx,yy,.026],[rr[2],rr[3],.012],'road_paint','road_dash')
        # Low, non-blocking edge paint marks an actual pavement alongside road.
        for t in range(0,int(max(w,h)),4):
            xx=x-1.2 if vertical else x+t+2;yy=y+t+2 if vertical else y-1.2
            rr=[xx-1,yy-2,2,4] if vertical else [xx-2,yy-1,4,2]
            if not any(overlap(rr,q) for q in protected):box([xx,yy,.03],[rr[2],rr[3],.035],'pavement','pavement')
    # Lit posts are placed on reserved road verges, not in the traffic lane.
    for road_index,r in enumerate(roads):
        x,y,w,h=r;vertical=h>w
        for t in range(14,int(max(w,h))-10,28):
            xx=x+w+1.5 if vertical else x+t;yy=y+t if vertical else y+h+1.5
            rr=[xx-.2,yy-.2,.4,.4]
            if any(overlap([xx-1,yy-1,2,2],q) for q in protected) or reserved.hits(0,rr,.2) or solid.hits(0,rr,.2):continue
            ident=f'ENV9_POLE_{road_index}_{t}'
            box([xx,yy,2.6],[.12,.12,5.2],'steel','lamp_post',solid_box=True)
            box([xx,yy,5.15],[.8,.4,.12],'steel','lamp_post_hood')
            box([xx,yy,5.075],[.65,.30,.02],'lamp','street_lamp');white.append([xx,yy,4.8])
            d['collision_features'].append(dict(id=ident,rect=[xx-.06,yy-.06,.12,.12],z=0,source='environment_dressing_v9'));solid.add(0,rr)
    # The asphalt substrate is already a continuous collision slab. A wet
    # road-edge wash makes its role clear without layering a slab over stairs.
    materials={
        'steel':dict(tint=[.24,.30,.32],metallic=.65,roughness=.4),
        'paint':dict(tint=[.65,.73,.69],weathering=.4),
        'lamp':dict(tint=[.8,.92,.89],roughness=.3),
        'emergency':dict(tint=[.7,.025,.008],roughness=.3),
        'dark':dict(tint=[.018,.025,.029]),'blood':dict(tint=[.22,.012,.009],roughness=.35,wetness=.4),
        'grass':dict(tint=[.12,.22,.055],weathering=.6,roughness=.95),
        'foliage':dict(tint=[.065,.14,.04],weathering=.5,roughness=.95),
        'wood':dict(tint=[.18,.10,.045]),'exit':dict(tint=[.025,.22,.13]),
        'road_paint':dict(tint=[.65,.66,.53],roughness=.7),
        'pavement':dict(tint=[.4,.43,.41],weathering=.55,roughness=.8)}
    d['environment_props']=props
    d['environment_dressing']=dict(version=9,quality='prototype',materials=materials,boxes=boxes,white_lights=white,red_lights=red,
        camera_count=camera_count,camera_scope='visible static housings and lenses; surveillance gameplay is not added',
        roads=roads,lawns=lawns,protected_surface_rects=protected,bay_coverage=coverage,fixture_counts=dict(fixture_count),
        moon=dict(at_cm=[100000,-120000,110000],scale=46),
        limits=['Simple box prototypes, not final art.','New floor kits supplement, never replace, the numbered furniture schedule.',
                'Attached radiators, cameras and wall switches are set dressing; existing interactive systems retain their functions.'])
    d['review_revision']=9
    d['furnishing_captures']=[c for c in d['furnishing_captures'] if not c.get('environment')]
    for name,at,target in [
        ('environment_campus',[124,-10,5],[170,30,3]),('environment_garden',[116,188,3],[86,208,.2]),
        ('environment_moon',[60,-78,4],[1000,-1200,1100]),('environment_road',[-24,110,3],[25,110,2]),
        ('environment_roof',[58,28,16.2],[88,50,15.5]),
    ]:d['furnishing_captures'].append(dict(id=name,at=at,target=target,envelope=True,environment=True))
    # Choose an actual clear viewpoint inside the bay and aim at its furniture.
    # A renamed capture cannot rely on the old implicit ID-to-room convention.
    for c in d['furnishing_captures'][:5]:
        q=dict(c);q['id']='environment_'+c['id'];q['environment']=True;q['envelope']=False
        zid=c['id'].removeprefix('furnished_');s,r=rooms[zid];ox,oy,z=s['origin'];options=[]
        room_bays=[b for b in d['subrooms'] if b['zone']==zid] or [dict(id=zid,rect=r['rect'])]
        for b in room_bays:
            x,y,w,h=b['rect'];x+=ox;y+=oy
            objects=[p for p in d['prototype_props']+props if p.get('subroom')==b['id'] and p['mount']=='floor']
            for p in objects:
                tx,ty,_=p['at'];model=catalog[str(p['model'])];height=min(1.2,max(.45,model['bounds'][5]*.5))
                for xx in (x+.7,x+w/2,x+w-.7):
                    for yy in (y+.7,y+h/2,y+h-.7):
                        distance=math.dist((xx,yy),(tx,ty))
                        if not 1.3<distance<5 or solid.hits(z,[xx-.2,yy-.2,.4,.4]):continue
                        score=min(distance,3)*2+len(objects)+(2 if p['model'] in (1,3,31,17,16,13) else 0)
                        options.append((score,[xx,yy,z+1.65],[tx,ty,z+height]))
        if options:_,q['at'],q['target']=max(options,key=lambda o:o[0])
        d['furnishing_captures'].append(q)
    result=dict(added_floor_props=len(props),logical_bays=len(coverage),bays_with_floor=sum(c['existing_floor']+c['added_floor']>0 for c in coverage),
        boxes=len(boxes),camera_count=camera_count,white_lights=len(white),red_lights=len(red),lawn_area_m2=sum(q[2]*q[3] for q in lawns),
        fixtures=dict(fixture_count),original_numbered_props=len(d['prototype_props']))
    (OUT/'environment_inventory.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(result,ensure_ascii=False));return result

if __name__=='__main__':
    d=json.loads(PATH.read_text(encoding='utf-8-sig'));apply_environment(d)
    PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
