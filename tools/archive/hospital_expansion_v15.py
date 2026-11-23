"""Elevated hospital galleries, tall perimeter/forest and room enrichment."""
import collections,csv,json,math,random
from pathlib import Path
from hospital_campus_v14 import Campus,shape as previous_shape
from hospital_clutter_v10 import bounds,rotate
from hospital_furnishing_v6 import overlap
from hospital_item_types_v12 import refresh_navigation
ROOT=Path(__file__).resolve().parents[1];PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json';OUT=ROOT/'artifacts/unreal_hospital_v15';SOURCE='hospital_expansion_v15'
BRIDGES=[[48,100,4,230,0],[-46,38,94,6,0],[52,38,98,6,0],[-46,158,94,6,0],[52,158,98,6,0],[-46,268,94,6,0],[52,268,98,6,0]]

def profile(kind):
    p=[]
    def b(x,y,z,w,d,h,m='camp_steel'):p.append([x,y,z,w,d,h,m])
    if kind in ('bedside_cabinet','chart_cart','cleaning_cart','mail_cart'):
        w,h=(.5,.45) if kind=='bedside_cabinet' else (.75,.5)
        for x in (-w/2+.045,w/2-.045):
            for y in (-h/2+.045,h/2-.045):b(x,y,.08,.07,.10,.16,'camp_rubber')
        b(0,0,.45,w,h,.64,'camp_cream' if kind!='cleaning_cart' else 'camp_teal')
        b(0,0,.79,w+.04,h+.04,.04)
        for z in (.34,.63):b(0,h/2+.01,z,w*.6,.02,.025,'camp_rubber')
        if kind!='bedside_cabinet':b(0,-h/2,.92,w,.04,.04)
        if kind=='chart_cart':
            for x in (-.25,-.12,0,.12,.25):b(x,0,.9,.06,.3,.18,'camp_ochre')
        if kind=='cleaning_cart':
            for x in (-.2,.2):b(x,0,.9,.16,.16,.2,'camp_red' if x<0 else 'camp_teal')
            b(.22,-.18,1.35,.035,.035,1.1,'camp_wood')
    elif kind=='privacy_screen':
        for x in (-.75,0,.75):
            b(x,0,1.03,.04,.04,2.06);b(x,0,.10,.06,.50,.06)
        for x in (-.375,.375):b(x,0,1.16,.70,.04,1.60,'camp_cloth')
    elif kind=='visitor_pair':
        for x in (-.40,.40):
            for dx in (-.20,.20):
                for y in (-.20,.20):b(x+dx,y,.23,.04,.04,.46)
            b(x,0,.48,.48,.48,.08,'camp_sage');b(x,-.20,.78,.48,.06,.55,'camp_sage')
    elif kind=='therapy_shelf':
        for x in (-.55,.55):b(x,0,.88,.05,.4,1.76,'camp_wood')
        for z in (.12,.65,1.18,1.72):
            b(0,0,z,1.15,.4,.045,'camp_wood')
            if z<1.5:
                for x in (-.36,0,.36):b(x,0,z+.18,.25,.26,.3,'camp_sage' if x else 'camp_cream')
    elif kind=='beverage_station':
        b(0,0,.42,.85,.56,.84,'camp_cream');b(0,0,.87,.9,.6,.06)
        b(-.18,0,1.1,.32,.38,.40,'camp_rubber');b(.24,0,.95,.17,.22,.12,'camp_ochre')
    elif kind=='water_dispenser':
        b(0,0,.48,.36,.37,.96,'camp_cream');b(0,0,1.13,.28,.28,.34,'camp_teal');b(0,.19,.77,.22,.04,.12,'camp_rubber')
    elif kind=='retail_chiller':
        b(0,0,1.0,.90,.60,2.0,'camp_cream')
        for z in (.35,.80,1.25,1.70):
            b(0,.31,z,.80,.04,.04)
            for x in (-.25,0,.25):b(x,.28,z+.13,.17,.10,.20,'camp_teal' if z<1 else 'camp_ochre')
        for x in (-.42,.42):b(x,.34,1.03,.04,.04,1.85,'camp_rubber')
    elif kind=='pallet_load':
        for z in (.05,.18):b(0,0,z,1.2,.85,.08,'camp_wood')
        for x in (-.3,.3):
            for y in (-.22,.22):b(x,y,.55,.53,.37,.65,'camp_crate');b(x,y,.9,.53,.04,.04,'camp_cream')
    elif kind in ('medicine_tray','desk_files','food_tray'):
        b(0,0,.015,.24,.17,.03,'camp_steel')
        if kind=='desk_files':
            b(0,0,.038,.20,.14,.016,'camp_cream');b(.05,0,.06,.08,.10,.025,'camp_ochre')
        else:
            for x in (-.07,.05):b(x,0,.075,.06,.07,.10,'camp_cream' if kind=='medicine_tray' else 'camp_leaf')
    else:return previous_shape(kind)
    mid=[(min(a[i]-a[i+3]/2 for a in p)+max(a[i]+a[i+3]/2 for a in p))/2 for i in (0,1)];bottom=min(a[2]-a[5]/2 for a in p)
    return [[a[0]-mid[0],a[1]-mid[1],a[2]-bottom,*a[3:]] for a in p]

def transform_architecture(d):
    medical=[s for s in d['locations'] if s['origin'][2]==0 and s['id']!='ST']
    def inside(p,r):return r[0]-.01<=p[0]<=r[0]+r[2]+.01 and r[1]-.01<=p[1]<=r[1]+r[3]+.01
    def external(p):return not any(inside(p,[s['origin'][0]+r[0],s['origin'][1]+r[1],r[2],r[3]]) for s in medical for r in [r['rect'] for r in s['rooms']]+s['corridors']+[r['rect'] for r in s['cores']])
    moved=[];oldrects=set();ids=set()
    for n in d['escape_niches']:
        p=n.get('corridor_point',n['entry'])
        if p[2]==0 and external(p) and any(inside(p,b[:4]) for b in BRIDGES):
            moved.append(n['id']);ids.add(n['id']);oldrects.add(tuple(n['rect']));n['entry'][2]=4.8
            if 'corridor_point' in n:n['corridor_point'][2]=4.8
            if 'z' in n:n['z']=4.8
            n['site']='2F'
    count=0
    for p in d['passages']:
        if p in BRIDGES or (p[4]==0 and tuple(p[:4]) in oldrects):p[4]=4.8;count+=1
    for f in d['collision_features']:
        if f['z']==0 and f['id'].rsplit('_jamb_',1)[0] in ids:f['z']=4.8
    for i,n in enumerate(d['campus_plan']['connections']):
        n['passage'][4]=4.8;n['from_site']='2F';n['to_site']+='2';n['kind']='elevated_enclosed_gallery'
    for site,x,y in [('DG2',-46,41),('IF2',150,41),('PS2',-46,161),('RH2',150,161),('HS2',-46,271),('SV2',150,271)]:
        d['site_entries'].append(dict(id='V15_BRIDGE_'+site,site=site,at=[x,y,4.8],side='east' if x<0 else 'west',width=3))
    # The wall and sentry positions grow together, preserving the same footprint.
    remove={i['id'] for i in d['campus_plan']['items'] if i['kind']=='sentry_stair'}
    d['environment_dressing']['boxes']=[b for b in d['environment_dressing']['boxes'] if b.get('item_id') not in remove]
    d['collision_features']=[f for f in d['collision_features'] if not any(f['id'].startswith(i+'_') for i in remove)]
    d['campus_plan']['items']=[i for i in d['campus_plan']['items'] if i['id'] not in remove]
    byid=collections.defaultdict(list)
    for b in d['environment_dressing']['boxes']:
        if b.get('layer')!=14:continue
        kind=b['kind'].removeprefix('campus_');h=b['size'][2]
        if kind=='perimeter_wall':
            if abs(h-2.9)<.001:b['size'][2]=5.8;b['at'][2]=2.9
            elif abs(h-3.2)<.001:b['size'][2]=6.1;b['at'][2]=3.05
            else:b['at'][2]+=2.9
        elif kind=='sentry_platform':
            if abs(h-2.9)<.001:b['size'][2]=5.9;b['at'][2]=2.95
            else:b['at'][2]+=3
        elif kind in ('wall_weapon_rack','wall_security_supplies'):b['at'][2]+=3
        elif kind in ('gate_header','gate_sign'):b['at'][2]+=2
        elif kind=='gate_barrier':
            if abs(h-4)<.001:b['size'][2]=6;b['at'][2]+=1
            elif b['at'][2]>5:b['at'][2]+=2
        byid[b.get('item_id')].append(b)
    for i in d['campus_plan']['items']:
        if i['id'] not in byid:continue
        ps=byid[i['id']];i['bounds']=[min(b['at'][j]-b['size'][j]/2 for b in ps) for j in range(3)]+[max(b['at'][j]+b['size'][j]/2 for b in ps) for j in range(3)]
        if i['kind'] in ('wall_weapon_rack','wall_security_supplies'):i['at'][2]+=3
    d['campus_plan']['clinical_boundary']['height_m']=5.8
    for q in d['campus_plan']['sentry_platforms']:q['floor_z']=6.05;q['steps']=40
    # More true glazed apertures in every exterior public building/floor.
    newwindows=[]
    for s in d['locations']:
        for r in s['rooms']:
            if not r['enclosed']:continue
            psy=s['id'].startswith('PS')
            for w in d['windows']:
                if w['zone']==r['id'] and psy:w['protection']='security_mesh';w['restricted_opening_m']=.08
            if s['id']!='ST' and not s['id'].startswith(('X','S03','S12','S13','S14')):continue
            x,y,w,h=r['rect']
            for side in ('north','south','west','east'):
                length=w if side in ('north','south') else h
                for t in range(3,int(length-2),5):
                    at=[x+t,y if side=='north' else y+h] if side in ('north','south') else [x if side=='west' else x+w,y+t]
                    if abs(at[0]-r['door'][0])+abs(at[1]-r['door'][1])<2.5:continue
                    if any(q['zone']==r['id'] and q['side']==side and math.dist(at,q['at'])<2.5 for q in d['windows']):continue
                    newwindows.append(dict(zone=r['id'],site=s['id'],side=side,at=at,width=1.5,height=1,sill=1.1,offset=0,transparent=True,role='external_view'))
    d['windows']+=newwindows
    d['review_revision']=15
    d['campus_upgrade']=dict(version=15,gallery_z_m=4.8,gallery_rects=[[*b[:4],4.8] for b in BRIDGES],moved_niches=moved,moved_passage_rects=count,new_windows=len(newwindows),wall_height_m=5.8,landscape_rect=[-320,-330,740,850],tree_count=0,forest_scope='Accessible outer forest and surrounding scenery; original hospital room areas retained.')

def populate(d):
    c=Campus(d);c.layer=15;c.source=SOURCE;c.prefix='EXP15';requests=[];rng=random.Random(150333)
    # Correct the short visibility rays used by new inspection cameras.
    # Existing captures are retained as historical camera positions.
    for xx in (70,225):
        for j in range(40):
            cy=-46+(.45*(39-j)+.225);h=.15*(j+1)
            c.emit('sentry_stair_v15','ST',[[0,0,h/2,1.2,.45,h,'camp_concrete']],[xx,cy,0],strict=False,extra=dict(mount='structure'))
    # Columns support the elevated structures without fencing off the streets.
    supports=[]
    for x,y,w,h,_ in BRIDGES:
        along=w>h;length=w if along else h
        for t in range(10,int(length-4),18):
            for side in (-1,1):
                xx=x+t if along else x+w/2+side*(w/2-.3);yy=y+h/2+side*(h/2-.3) if along else y+t
                rr=[xx-.22,yy-.22,.44,.44]
                if any(overlap([xx-.8,yy-.8,1.6,1.6],r) for r in c.e['roads']) or c.floor.hits(0,rr,.1) or c.reserved.hits(0,rr,.1):continue
                if any(s['origin'][2]==0 and s['id']!='ST' and any(overlap(rr,[s['origin'][0]+r[0],s['origin'][1]+r[1],r[2],r[3]]) for r in [r['rect'] for r in s['rooms']]+s['corridors']) for s in d['locations']):continue
                i=c.emit('bridge_column','ST',[[0,0,2.3,.44,.44,4.6,'camp_concrete']],[xx,yy,0],extra=dict(mount='structure'))
                if i:supports.append(i['id'])
    # Functional kits in all locations, including the floors outside the КПП.
    for zid,(s,r) in sorted(c.zones.items()):
        if not r['enclosed']:continue
        name=r['name'].lower();sid=s['id'];kinds=[]
        if sid.startswith(('X01','X03')) or zid in ('X01','X03'):kinds=['retail_chiller','market_crates','pallet_load','retail_chiller']
        elif sid.startswith('X02') or zid=='X02':kinds=['pharmacy_display','chart_cart','visitor_pair']
        elif sid.startswith('X04') or zid=='X04':kinds=['pallet_load','pallet_rack','mail_cart','pallet_load']
        elif sid.startswith('X05') or zid=='X05':kinds=['beverage_station','visitor_pair','water_dispenser']
        elif any(t in name for t in ('палат','пациент','паллиат','ожог','нефрол')):kinds=['bedside_cabinet','privacy_screen','visitor_pair']
        elif any(t in name for t in ('сануз','душ','мойка','моеч','белья','убор','прачеч')):kinds=['cleaning_cart','laundry_sorter']
        elif any(t in name for t in ('лаборат','исслед','диагност','процед','осмотр','операц','перевяз')):kinds=['chart_cart','privacy_screen','oxygen']
        elif any(t in name for t in ('склад','кладов','запас','погруз','хранени')):kinds=['pallet_load','mail_cart']
        elif sid.startswith('PS'):kinds=['therapy_shelf','visitor_pair','water_dispenser']
        elif sid.startswith('RH'):kinds=['bedside_cabinet','therapy_shelf','exercise_cycle']
        elif sid.startswith('HS') or any(t in name for t in ('отдых','столов','кухн')):kinds=['beverage_station','personal_locker','visitor_pair']
        elif sid.startswith('SV'):kinds=['tool_board','mail_cart','pallet_load']
        else:kinds=['chart_cart','visitor_pair','water_dispenser']
        count=4 if sid.startswith('X') or zid.startswith('X') else 2 if r['rect'][2]*r['rect'][3]>=100 else 1
        placed=[]
        for kind in kinds[:count]:
            i=c.fit(kind,zid,parts=profile(kind));requests.append(dict(zone=zid,kind=kind,item=i['id'] if i else None))
            if i:placed.append(i)
        if placed and (sid.startswith(('X','S03','S12')) or zid in ('X01','X02','X03','X04','X05','PS05','PS09','RH17','HS24','SV03','R07')):
            before=len(d['furnishing_captures']);c.capture(placed[0],'v15_'+zid)
            for q in d['furnishing_captures'][before:]:q['expansion_v15']=True
    print('Room kits',sum(bool(q['item']) for q in requests),'/',len(requests),flush=True)
    # Stock free, real furniture supports; these are non-interactive props.
    surfaces=0
    for p in d['prototype_props']+d['environment_props']:
        cat=d['prototype_catalog'][str(p['model'])]
        if p['mount']!='floor' or p['model'] not in (16,17,18,19,20,21,22,23,24,37,38,39) or cat.get('top',0)<.4:continue
        k='medicine_tray' if p['model'] in (20,37,38) else 'desk_files' if p['model'] in (16,21,22,23) else 'food_tray';parts=profile(k)
        for lx,ly in ((-.2,.10),(.2,.1),(-.2,-.12),(.2,-.12),(0,0)):
            off=rotate([lx,ly,cat['top']+.016],p['turn']);at=[p['at'][j]+off[j] for j in range(3)];bb=cat['bounds'];fw,fd=bb[3]-bb[0],bb[4]-bb[1]
            if p['turn']%2:fw,fd=fd,fw
            if abs(lx)+.12>fw/2-.03 or abs(ly)+.09>fd/2-.03:continue
            i=c.emit(k,p['zone'],parts,at,p['turn'],False,extra=dict(mount='surface',parent=p['id']))
            if i:surfaces+=1;break
    # Rounded, individually varied tree crowns use an already-cooked sphere.
    c.e['materials'].update({
      'forest_bark':dict(tint=[.15,.09,.045],roughness=.98,weathering=.6),
      'forest_pine':dict(tint=[.025,.105,.035],roughness=.98,weathering=.25),
      'forest_leaf':dict(tint=[.055,.18,.045],roughness=.98,weathering=.2),
      'forest_leaf2':dict(tint=[.10,.20,.045],roughness=.98,weathering=.3),
      'forest_soil':dict(tint=[.10,.13,.07],roughness=1,weathering=.7)})
    def raw(kind,at,size,mat,solid=False,mesh=None,ident=None):
        b=dict(at=at,size=size,material=mat,kind=kind,solid=solid,layer=15,zone='ST')
        if mesh:b['mesh']=mesh
        if ident:b['tree_id']=ident
        c.e['boxes'].append(b)
    bridge_lights=0
    for x,y,w,h,_ in BRIDGES:
        horizontal=w>h;length=w if horizontal else h
        for t in range(8,int(length-2),14):
            xx=x+t if horizontal else x+w/2;yy=y+h/2 if horizontal else y+t
            raw('bridge_lamp',[xx,yy,8.42],[1.1,.24,.08] if horizontal else [.24,1.1,.08],'lamp')
            c.e['white_lights'].append([xx,yy,7.9]);bridge_lights+=1
    for xx in (64,145):
        raw('checkpoint_wall_light',[xx,-49.7,4.0],[.6,.20,.12],'lamp');c.e['white_lights'].append([xx,-49.2,3.9])
    for r in [[-320,-330,140,850],[280,-330,140,850],[-180,-330,460,140],[-180,380,460,140]]:
        x,y,w,h=r;raw('forest_ground',[x+w/2,y+h/2,-.15],[w,h,.30],'forest_soil',True)
    trees=[]
    def tree(x,y,height,inside=False):
        radius=.22+height*.008;rr=[x-radius,y-radius,2*radius,2*radius]
        if -180<x<280 and -190<y<380:
            if c.floor.hits(0,rr,.6) or c.reserved.hits(0,[x-1.1,y-1.1,2.2,2.2],.1):return
            if any(overlap([x-2.5,y-2.5,5,5],r) for r in c.e['roads']):return
            for s in d['locations']:
                if s['origin'][2]!=0:continue
                for r in s['rooms']:
                    if r['enclosed'] and overlap([x-5,y-5,10,10],[s['origin'][0]+r['rect'][0],s['origin'][1]+r['rect'][1],*r['rect'][2:]]):return
            if any(p[4]==0 and overlap([x-3,y-3,6,6],p[:4]) for p in d['passages']):return
            if any(p[4]==4.8 and overlap([x-6,y-6,12,12],p[:4]) for p in d['campus_upgrade']['gallery_rects']):return
        tid=f'TREE15_{len(trees):04}';pine=rng.random()<.65;width=height*(.28 if pine else .38)
        raw('forest_trunk',[x,y,height*.43],[radius*2,radius*2,height*.86],'forest_bark',True,ident=tid)
        if pine:
            for j in range(4):raw('forest_crown',[x+.1*j,y,height*(.48+.12*j)],[width*(1-j*.16),width*(1-j*.16),height*.38],'forest_pine',mesh='sphere',ident=tid)
        else:
            for dx,dy,zz in ((-.9,0,.70),(.9,.6,.76),(0,-.7,.88)):raw('forest_crown',[x+dx,y+dy,height*zz],[width,width*.9,height*.30],'forest_leaf' if rng.random()<.6 else 'forest_leaf2',mesh='sphere',ident=tid)
        d['collision_features'].append(dict(id=tid,rect=rr,z=0,source=SOURCE));c.floor.add(0,rr)
        trees.append(dict(id=tid,at=[x,y,0],height_m=height,crown_width_m=width,inside=inside,pine=pine))
    # Sparse deliberate planting inside; dense woods beyond the clinical wall.
    for x,y in [(-15,55),(-15,78),(-15,135),(-15,187),(-15,240),(-15,303),(113,55),(113,78),(113,135),(113,187),(113,240),(113,303),(-80,338),(10,338),(90,338),(170,338),(230,338),(-30,65),(113,180),(-20,245),(115,245)]:tree(x,y,rng.uniform(11,16),True)
    for x in range(-306,414,9):
        for y in range(-315,511,9):
            xx=x+rng.uniform(-2.5,2.5);yy=y+rng.uniform(-2.5,2.5)
            if -174<xx<274 and -44<yy<372:continue
            if -80<xx<160 and -190<yy<-44:continue  # clear retail/parking forecourt
            if rng.random()<.13:continue
            tree(xx,yy,rng.uniform(19,30))
    # Clear views are selected inside their physical room before tracing.
    for name,at,target in [('bridge_underpass',[60,114,1.8],[50,128,6]),('bridge_upper',[50,130,6.5],[50,175,6.2]),('forest_gate',[42,-186,1.8],[30,-260,14]),('forest_wall',[-170,50,1.8],[-220,65,16]),('high_wall',[76,-30,10],[70,-47.5,7]),('psy_window',[-128,119,1.8],[-128,124,1.6])]:
        d['furnishing_captures'].append(dict(id='v15_'+name,at=at,target=target,expansion_v15=True))
    d['campus_upgrade'].update(tree_count=len(trees),trees=trees,inside_trees=sum(t['inside'] for t in trees),items=c.items,requests=requests,surface_details=surfaces,bridge_supports=supports,bridge_lights=bridge_lights,quality='playable prototype geometry; guard weapons remain decorative')
    stats=dict(new_items=len(c.items),new_parts=sum(i['parts'] for i in c.items),room_kits=sum(i.get('mount')=='floor' for i in c.items),rooms_enriched=len({i['zone'] for i in c.items if i.get('mount')=='floor'}),outside_building_kits=sum(i.get('mount')=='floor' and (i['zone'].startswith('X') or i['zone'] in ('S03','S12','S13','S14','S11','S16')) for i in c.items),surface_details=surfaces,trees=len(trees),inside_trees=sum(t['inside'] for t in trees),added_windows=d['campus_upgrade']['new_windows'],secure_windows=sum(q.get('protection')=='security_mesh' for q in d['windows']),bridge_supports=len(supports),new_views=sum(q.get('expansion_v15',False) for q in d['furnishing_captures']),unfitted=sum(not q['item'] for q in requests))
    (OUT/'expansion_inventory.json').write_text(json.dumps(stats,ensure_ascii=False,indent=2),encoding='utf-8')
    with (OUT/'new_items.csv').open('w',encoding='utf-8-sig',newline='') as f:
        keys=list(dict.fromkeys(k for row in c.items for k in row));w=csv.DictWriter(f,keys);w.writeheader();w.writerows({k:json.dumps(v,ensure_ascii=False) if isinstance(v,(list,dict)) else v for k,v in row.items()} for row in c.items)
    print(json.dumps(stats,ensure_ascii=False),flush=True)

if __name__=='__main__':
    OUT.mkdir(parents=True,exist_ok=True);baseline=OUT/'baseline_v14.json'
    if not baseline.exists():baseline.write_bytes(PATH.read_bytes())
    d=json.loads(baseline.read_text(encoding='utf-8'));transform_architecture(d)
    PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');refresh_navigation()
    d=json.loads(PATH.read_text(encoding='utf-8'));populate(d)
    PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');refresh_navigation()
