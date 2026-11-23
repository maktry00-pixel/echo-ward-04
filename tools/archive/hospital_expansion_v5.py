"""Playable expansion requested after the complete v4 atlas.

Keep every existing footprint. Additional floors, routes and furnishings are
physical runtime data, rather than extra labels on the exported plan.
"""
import copy
import csv
import json
import math
from collections import Counter
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'artifacts/unreal_hospital_v5'

def apply_expansion(d):
    d['review_revision']=5
    d['review_source']='User review of the complete v4 atlas, 23 October 2026'
    sites={l['id']:l for l in d['locations']}
    old_ids={r['id'] for l in d['locations'] for r in l['rooms']}
    d['site_entries']=[]
    d['furnishings']=[]
    d['floor_expansion']=[]
    # A maintenance shortcut must not leave its former C-room masquerading as
    # another escape exit after promotion to a playable side-story room.
    for l in d['locations']:
        for r in l['rooms']:
            r['extra_doors']=[e for e in r.get('extra_doors',[]) if e.get('role')!='secret_maintenance']
    d['secret_passages']=[]
    for key in ('PS','RH'):
        candidates=[r for r in sites[key]['rooms'] if r['production_class']=='C']
        for r in candidates[:10]:
            r['production_class']='B';r['states']=[dict(chapter=0,state='open'),dict(chapter=1,state='open')]
            r['side_story']='Последняя дежурная' if key=='PS' else 'Палата без номера'
            r['chapters']=sorted(set(r['chapters']+([2,13] if key=='PS' else [11,16])))
    for m in d['monster_rules']:
        if m['id']=='listener':
            m.update(combat_class='B',kill_condition='finale_only',counterplay=['escape','cover','finale_combat'])

    def room(rid,name,rect,side='south',cls='B',chapters=None):
        x,y,w,h=rect
        return dict(id=rid,name=name,rect=rect,minimum_area_m2=w*h,enclosed=True,
            door_side=side,door=[x+w//2,y+h if side=='south' else y],production_class=cls,
            chapters=chapters or [1,14,19],side_story='',refuge=False,door_type='ordinary',wall_type='main',
            furniture_template='service',extra_doors=[],states=[dict(chapter=0,state='open'),
            dict(chapter=1,state='closed' if cls=='C' else 'open')])

    # Register the two independently accessible structural halls as real zones.
    for rid,y,side in (('DG25',46,'south'),('DG26',68,'north')):
        r=room(rid,'Нижний опорный зал '+('север' if y==46 else 'юг'),[11,y,76,16],side,'A',[12,16,18])
        r['door'][0]=48
        r['extra_doors']=[dict(at=[70,r['door'][1]],side=side,type='ordinary',role='hall_escape'),
            dict(at=[48,y if side=='south' else y+16],side='north' if side=='south' else 'south',type='ordinary',role='hall_through')]
        r['environment_states']=[dict(chapter=0,state='clean'),dict(chapter=16,state='infected'),dict(chapter=18,state='damaged')]
        sites['DG']['rooms'].append(r)
        d['zone_evolution'].append(dict(zone=rid,site='DG',states=r['environment_states']))

    # Separate building wings receive stacked floors. They inherit circulation
    # and clinical bays, with new IDs and their own chapter/class records.
    for base,count in (('DG',3),('IF',3),('PS',2),('RH',3),('HS',2),('SV',3)):
        template=sites['DG2'] if base=='DG' else sites[base]
        first=3 if base=='DG' else 2
        for level in range(first,count+1):
            ident=base+str(level);l=copy.deepcopy(template);ox,oy,_=l['origin'];z=(level-1)*4.8
            l.update(id=ident,name=sites[base]['name']+f' / этаж {level}',origin=[ox,oy,z],
                     parent_building=base,floor_number=level)
            l['rooms']=[r for r in l['rooms'] if not r.get('hidden')]
            mapping={r['id']:f'{ident}_{i+1:02}' for i,r in enumerate(l['rooms'])}
            for i,r in enumerate(l['rooms']):
                r['id']=mapping[r['id']];r['refuge']=i==7;r['production_class']='A' if r['refuge'] else 'C' if i%5==4 else 'B'
                r['states']=[dict(chapter=0,state='open'),dict(chapter=1,state='closed' if r['production_class']=='C' else 'open')]
                r['door_type']='bolt' if r['refuge'] else 'ordinary';r['chapters']=[12,16] if base=='DG' else [11,14,16]
                r['extra_doors']=[];r.pop('environment_states',None)
            l['refuge']=l['rooms'][min(7,len(l['rooms'])-1)]['id'];l['identity']['landmark']=l['rooms'][0]['id']
            d['locations'].append(l);sites[ident]=l
            for kind in ('partitions','subrooms','windows'):
                copies=[]
                for rec in d[kind]:
                    if rec.get('site')!=template['id']:continue
                    q=copy.deepcopy(rec);q['site']=ident
                    if kind=='partitions':q['a'][2]=q['b'][2]=z
                    else:
                        if q['zone'] not in mapping:continue
                        before=q['zone'];q['zone']=mapping[before]
                        if 'id' in q:q['id']=q['id'].replace(before,q['zone'],1)
                    copies.append(q)
                d[kind]+=copies
            # Preserve the real side-strip openings in the copied partitions.
            for n in list(d['escape_niches']):
                if n['site']!=template['id']:continue
                q=copy.deepcopy(n);q['id']=ident+'_'+n['id'];q['site']=ident;q['entry'][2]=z;q['corridor_point'][2]=z
                d['escape_niches'].append(q)
                if not n.get('slot'):d['passages'].append([*n['rect'],z])
                for f in list(d['collision_features']):
                    if f['id'].startswith(n['id']+'_'):
                        a=copy.deepcopy(f);a['id']=ident+'_'+f['id'];a['z']=z;d['collision_features'].append(a)
            d['floor_expansion'].append(dict(site=ident,building=base,floor=level,z=z))
        next(s for s in d['stairs'] if s['id']==base)['z_max']=(count-1)*4.8
        # A second independent staircase and a rear cross connection on every
        # floor, including the shared basement, give both evacuation directions.
        ox,oy,_=sites[base]['origin'];sid=base+'_EAST'
        d['stairs'].append(dict(id=sid,rect=[ox+108,oy+86,3,8],z_min=-4.8,z_max=(count-1)*4.8))
        floors=[l for l in d['locations'] if l['id']==base or l.get('parent_building')==base or (base=='DG' and l['id']=='DG2')]
        for l in floors:
            l['corridors'] += [[104,86,8,14],[50,96,62,4],[102,40,6,60]]
            l['bounds']=[min(-8,l['bounds'][0]),min(0,l['bounds'][1]),120,100]
            l['cores'].append(dict(name='STAIR '+sid,rect=[108,86,3,8]))
        d['passages'] += [[ox+50,oy+96,62,4,-4.8],[ox+104,oy+86,8,14,-4.8]]
        for x,side in ((ox,'west'),(ox+104,'east')):
            d['site_entries'].append(dict(id=base+'_ENTRY_'+side,site=base,at=[x,oy+41,0],side=side,width=3))

    # Compact street buildings: supermarket/warehouse get three floors, the
    # pharmacy, shop, cafe and service buildings get two, all above their own XY.
    street=sites['ST'];compact=[]
    store_ids=('X01','X02','X03','X04','X05','S03','S12','S13','S14','S11','S16')
    for rid in store_ids:
        r=next(r for r in street['rooms'] if r['id']==rid);x,y,w,h=r['rect'];ox=street['origin'][0]+x;oy=street['origin'][1]+y
        levels=1 if rid in ('S11','S16') else 3 if rid in ('X01','X04') else 2
        # Stair shafts are outside the retained ground-floor room footprint.
        side_stairs=rid=='S12'
        for suffix,sx in (('W',ox-4 if side_stairs else ox+1),('E',ox+w+1 if side_stairs else ox+w-4)):
            d['stairs'].append(dict(id=rid+'_'+suffix,rect=[sx,oy+1 if side_stairs else oy+h+5,3,8],z_min=-4.8,z_max=(levels-1)*4.8))
        entry_y=10 if side_stairs else 5
        r['extra_doors'] += [dict(at=[x,y+entry_y],side='west',type='ordinary',role='street_exit'),
                            dict(at=[x+w,y+entry_y],side='east',type='ordinary',role='street_exit')]
        r['extra_doors'] += [dict(at=[x+xx,y+h],side='south',type='ordinary',role='rear_exit') for xx in range(4,w-2,8)]
        d['site_entries'] += [dict(id=rid+'_GALLERY_FRONT_'+str(xx),site='1F',at=[xx,oy,0],side='north',width=2) for xx in (ox-2,ox+w+2)]
        # Covered external galleries around the two independent stairs.
        for z in [i*4.8 for i in range(levels)]+[-4.8]:
            depth=4 if side_stairs else 14
            d['passages'] += [[ox-4,oy,4,h+depth,z],[ox+w,oy,5,h+depth,z],[ox-4,oy+h, w+9,depth,z]]
        # Both side-stair basements connect to the same inspection/service hall.
        hall=[ox,oy,w,h];sites['UT']['corridors'].append(hall)
        d['infrastructure_zones'].append(dict(id='U_'+rid,name='Подвал '+r['name'],rect=hall,z=-4.8))
        d['site_entries'] += [dict(id=rid+'_BASE_'+side,site='UT',at=[xx,oy+entry_y,-4.8],side=side,width=2)
                             for xx,side in ((ox,'west'),(ox+w,'east'))]
        for level in range(2,levels+1):
            key=rid+'F'+str(level);z=(level-1)*4.8;cw=4;half=(w-cw)/2
            # Integer room borders match the runtime one-metre floor cells.
            split=int(half);rr=[room(key+'A','Служебное помещение / '+r['name'],[0,0,w,5],'south'),
                room(key+'B','Основной зал / '+r['name'],[0,9,w,h-9],'north')]
            l=dict(id=key,name=r['name']+f' / этаж {level}',name_en=key,name_kk=key,origin=[ox,oy,z],
                   bounds=[-4,0,w+9,h+depth],rooms=rr,corridors=[[0,5,w,4],[-4,0,4,h+depth],[w,0,5,h+depth],[-4,h,w+9,depth]],
                   cores=[dict(name='STAIR '+rid+'_W',rect=[-4,1,3,8] if side_stairs else [1,h+5,3,8]),dict(name='STAIR '+rid+'_E',rect=[w+1,1,3,8] if side_stairs else [w-4,h+5,3,8])],
                   capture=[w/2,7,.98,-5,90],identity=copy.deepcopy(sites['SV']['identity']),refuge=rr[0]['id'],
                   compact=True,parent_building=rid,floor_number=level,style='commercial')
            l['identity']['landmark']=rr[0]['id'];rr[0]['refuge']=True;rr[0]['door_type']='bolt'
            rr[0]['production_class']='A';rr[0]['extra_doors']=[dict(at=[w-2,5],side='south',type='ordinary',role='escape_exit')]
            rr[1]['extra_doors']=[dict(at=[w-2,9],side='north',type='ordinary',role='escape_exit')]
            rr[1]['extra_doors'] += [dict(at=[xx,h],side='south',type='ordinary',role='rear_exit') for xx in range(4,w-2,8)]
            d['locations'].append(l);sites[key]=l;compact.append(key)
            d['floor_expansion'].append(dict(site=key,building=rid,floor=level,z=z))
            for xx,side in ((ox,'west'),(ox+w,'east')):
                d['site_entries'].append(dict(id=key+'_ENTRY_'+side,site=key,at=[xx,oy+7,z],side=side,width=2))

    # Rebuild the six straight basement branches with alternating doglegs.
    old_ut_niches=[n for n in d['escape_niches'] if n['site']=='UT']
    old_nids={n['id'] for n in old_ut_niches}
    pocket_rects={tuple([*n['rect'],n['entry'][2]]) for n in old_ut_niches if not n.get('slot')}
    d['passages']=[p for p in d['passages'] if tuple(p) not in pocket_rects]
    d['escape_niches']=[n for n in d['escape_niches'] if n['site']!='UT']
    d['collision_features']=[f for f in d['collision_features'] if not any(f['id'].startswith(k+'_') for k in old_nids)]
    d['structural_members']=[f for f in d['structural_members'] if not any(f['id'].startswith(k+'_') for k in old_nids)]
    ut=sites['UT'];d['utility_links']=[]
    def path_rects(points,width=4):
        result=[]
        for a,b in zip(points,points[1:]):
            assert a[0]==b[0] or a[1]==b[1]
            result.append([int(min(a[0],b[0])-width/2),int(min(a[1],b[1])-width/2),
                           int(abs(a[0]-b[0])+width),int(abs(a[1]-b[1])+width)])
        return result
    def tunnel(key,pts,target):
        ut['corridors']+=path_rects(pts)
        d['utility_links'].append(dict(id=key,source='U_SPINE',target=target,width_m=4,points=[[x,y,-4.8] for x,y in pts]))
    for key in ('DG','IF','PS','RH','HS','SV'):
        ox,oy,_=sites[key]['origin'];a,b=sorted((ox-8,50));straight=[a,oy+96,b-a+4,4]
        ut['corridors']=[r for r in ut['corridors'] if r!=straight]
        end=ox-4;direction=1 if end>50 else -1;yy=oy+98;pts=[[50,yy]];x=50
        phase=0
        while abs(end-x)>35:
            x+=direction*28;pts.append([x,pts[-1][1]]);phase+=1
            pts.append([x,yy+(8 if phase%2 else -8)])
        pts += [[end,pts[-1][1]],[end,yy],[end,oy+93]]
        pts=[p for i,p in enumerate(pts) if i==0 or p!=pts[i-1]]
        tunnel('U_LINK_'+key,pts,'U_'+key)
    # A southern loop serves all street shops/warehouses. Each building has two
    # stairs and its own basement connection, not a teleporting destination.
    ring=[[50,40],[50,-176],[-72,-176],[-72,-12],[50,-12]]
    tunnel('U_STREET_RING',ring,'U_STREET')
    for rid in store_ids:
        r=next(r for r in street['rooms'] if r['id']==rid);x,y,w,h=r['rect'];ox=street['origin'][0]+x;oy=street['origin'][1]+y
        midx=ox+w/2;cy=oy+h+2
        # South-facing rear gallery meets an L-shaped branch to the ring.
        pts=[[50,cy],[midx,cy],[midx,oy+h-1]]
        tunnel('U_LINK_'+rid,pts,'U_'+rid)
    d['utility_links'].append(dict(id='U_SV_TO_SV24',source='U_SV',target='SV24',width_m=3.4,
        points=[[145.5,323.4,-4.8],[145.5,323.4,0],[145.5,328,0],[198.5,328,0],
                [198.5,295,0],[230,295,0],[230,299.4,0]],stair='SV'))
    # This upper route has real floor, walls and roof generated from indoor
    # passages; it runs outside the main pavilion footprint deliberately.
    d['passages'] += [[144,326,56,4,0],[197,293,4,37,0],[197,293,35,4,0],[228,293,4,7,0]]
    ut['bounds']=[-164,-184,432,528]

    # Deterministic per-department distribution, rather than the first twenty
    # IDs alphabetically. Twenty old C passages retained as a numeric budget.
    for key,quota in {'1F':3,'2F':3,'3F':3,'B1':2,'PS':2,'RH':2,'SV':2,'RF':1,'ST':2}.items():
        choices=[r for r in sites[key]['rooms'] if r['enclosed'] and r['production_class']=='C' and not r.get('extra_doors')]
        if len(choices)<quota:
            choices += [r for r in sites[key]['rooms'] if r['enclosed'] and r['production_class']=='C' and r not in choices]
        for r in choices[:quota]:
            x,y,w,h=r['rect'];at=[r['door'][0]-3 if r['door'][0]-3>x+1 else r['door'][0]+3,r['door'][1]]
            e=dict(at=at,side=r['door_side'],type='manual',role='secret_maintenance',inside_only=True)
            r.setdefault('extra_doors',[]).append(e)
            d['secret_passages'].append(dict(zone=r['id'],site=key,at=at,side=r['door_side'],inside_only=True))

    # Count a loop only when a second portal actually links two different rooms
    # already served by the common corridor. Two doors on one facade do not count.
    d['loop_audit']=[]
    for l in d['locations']:
        if l['id'] in ('ST','RF','UT') or l.get('compact'):continue
        existing={tuple(sorted((s['a'],s['b']))) for s in d['shortcuts'] if s['site']==l['id']}
        candidates=[]
        for i,r in enumerate(l['rooms']):
            x,y,w,h=r['rect']
            for q in l['rooms'][i+1:]:
                bx,by,bw,bh=q['rect']
                if abs(x+w-bx)>.01 or by!=y or bh!=h or r.get('hidden') or q.get('hidden'):continue
                pair=tuple(sorted((r['id'],q['id'])))
                if pair not in existing:candidates.append((r,q,[bx,y+h-1.4 if r['door_side']=='south' else y+1.4]))
        for r,q,at in candidates:
            if len(existing)>=3:break
            # Explicit story loops stay available through these rooms. Promote
            # an involved C room instead of counting its locked front as a loop.
            for rr in (r,q):
                if rr['production_class']=='C':rr['production_class']='B';rr['states']=[dict(chapter=0,state='open'),dict(chapter=1,state='open')]
            for rr,side in ((r,'east'),(q,'west')):
                rr.setdefault('extra_doors',[]).append(dict(at=at,side=side,type='ordinary',role='loop'))
            d['shortcuts'].append(dict(site=l['id'],a=r['id'],b=q['id'],at=at,kind='loop'))
            existing.add(tuple(sorted((r['id'],q['id']))))
        # Existing loops may be C-C; designate their paired circulation rooms
        # playable so the counted route exists beyond inspection chapter zero.
        byid={r['id']:r for r in l['rooms']}
        for s in [s for s in d['shortcuts'] if s['site']==l['id']]:
            for rid in (s['a'],s['b']):
                rr=byid[rid]
                if rr['production_class']=='C':rr['production_class']='B';rr['states']=[dict(chapter=0,state='open'),dict(chapter=1,state='open')]
        d['loop_audit'].append(dict(site=l['id'],independent_room_pairs=len(existing),pairs=[list(p) for p in sorted(existing)],
            method='unique shared-wall room pair + both primary doors + common corridor; facade double doors excluded'))

    # Junction rooms with three exits are deliberately class A, not a silent
    # exception to the one/two-exit class B contract.
    for l in d['locations']:
        for r in l['rooms']:
            if r['production_class']=='B' and len(r.get('extra_doors',[]))>1:r['production_class']='A'
            if r['enclosed'] and r['production_class']=='A' and not r.get('extra_doors'):
                x,y,w,h=r['rect'];dx=3 if r['door'][0]+3<x+w-1 else -3
                r['extra_doors']=[dict(at=[r['door'][0]+dx,r['door'][1]],side=r['door_side'],type='ordinary',role='escape_exit')]

    # Fill new A/B rooms with architectural hideouts at the same bay-safe
    # positions as the existing shell. Furniture supplements rather than erases
    # the narrow refuges already verified in the released prototype.
    hides=Counter(h['zone'] for h in d['hideouts'])
    for l in d['locations']:
        ox,oy,z=l['origin']
        for r in l['rooms']:
            if not r['enclosed'] or r['production_class']=='C':continue
            need=2 if r['production_class']=='A' else 1
            bays=[b for b in d['subrooms'] if b['zone']==r['id']]
            x,y,w,h=r['rect'];candidates=[]
            for b in bays:
                bx,by,bw,bh=b['rect'];xx=bx+.28 if bx<r['door'][0] else bx+bw-1.48
                candidates.append([ox+xx,oy+by+.12,z])
            if not candidates:candidates=[[ox+x+.4,oy+y+(.4 if r['door_side']=='south' else h-2.2),z],
                [ox+x+w-1.8,oy+y+(.4 if r['door_side']=='south' else h-2.2),z]]
            for i in range(hides[r['id']],need):
                at=candidates[i%len(candidates)];item=dict(id=f'HIDE_{r["id"]}_{i+1}',zone=r['id'],site=l['id'],at=at,
                    size=[1.2,1,2.1],opening_m=.9,small_pass=True,large_pass=False,chapters=r['chapters'],kind='architectural_recess')
                d['hideouts'].append(item)
                for suffix,rect in (('back',[at[0],at[1],1.2,.15]),('left',[at[0],at[1],.15,1]),('right',[at[0]+1.05,at[1],.15,1])):
                    d['collision_features'].append(dict(id=item['id']+'_'+suffix,rect=rect,z=z))
            # Convert selected standing recesses into an actual wardrobe model.
            hs=[v for v in d['hideouts'] if v['zone']==r['id']]
            if hs and len(d['furnishings'])%2==0:hs[0]['kind']='wardrobe'
            # Place bedside furniture inside a real bay, clear of its centre-aisle
            # doorway. Every A/B room now has physical furniture, not 5 global props.
            if bays:
                b=bays[-1];bx,by,bw,bh=b['rect'];fx=ox+bx+(.7 if bx<r['door'][0] else bw-.7);fy=oy+by+bh-.8
            else:fx=ox+x+w-1.1;fy=oy+y+h-1.2
            avoid=[]
            for hh in d['hideouts']:
                if hh['zone']==r['id']:
                    hx,hy,_=hh['at'];hw,hd,_=hh['size'];avoid.append([hx-.4,hy-.4,hw+.8,hd+1.5])
            for xx,yy in [r['door']]+[e['at'] for e in r.get('extra_doors',[])]:avoid.append([ox+xx-1.4,oy+yy-1.8,2.8,3.6])
            def overlaps(a,b):return min(a[0]+a[2],b[0]+b[2])>max(a[0],b[0]) and min(a[1]+a[3],b[1]+b[3])>max(a[1],b[1])
            options=[(fx,fy)]
            for b in bays:
                bx,by,bw,bh=b['rect']
                options += [(ox+bx+(.7 if bx<r['door'][0] else bw-.7),oy+by+v) for v in (.65,bh-.65)]
            options += [(ox+x+w-.7,oy+y+h-.7),(ox+x+.7,oy+y+h-.7),(ox+x+.7,oy+y+.7)]
            safe=next(((a,b) for a,b in options if not any(overlaps([a-.7,b-.6,1.4,1.2],q) for q in avoid)),None)
            mounted=safe is None
            if safe:fx,fy=safe
            d['furnishings'].append(dict(id='CABINET_'+r['id'],zone=r['id'],site=l['id'],type='medical_cabinet',
                at=[fx,fy,z+(2.4 if mounted else 0)],size=[.65,.55,1.0 if mounted else 1.5],wall_mounted=mounted))
            if not mounted:d['collision_features'].append(dict(id='CABINET_'+r['id'],rect=[fx-.325,fy-.275,.65,.55],z=z))
            # A bed with real clearance for a crouched character. Reserve an
            # additional bay where available so it cannot cover an existing hide.
            if len(bays)>=3:
                b=bays[2];bx,by,bw,bh=b['rect']
                if bw>=2.8 and bh>=3.7:
                    at=[ox+bx+(.3 if bx<r['door'][0] else bw-1.8),oy+by+.3,z]
                    item=dict(id='BED_HIDE_'+r['id'],zone=r['id'],site=l['id'],kind='under_bed',at=at,
                        size=[1.5,2.2,1.5],opening_m=1.2,opening_height_m=1.05,small_pass=True,large_pass=False,
                        chapters=r['chapters'],requires_crouch=True)
                    d['hideouts'].append(item)
                    # Standing navigation treats the bed as an obstacle. The
                    # crouched clearance is tested separately against its legs.
                    d['collision_features'].append(dict(id=item['id'],rect=[*at[:2],1.5,2.2],z=z))

    # New tunnel pockets on both sides of every circulation strip, including
    # commercial connections. Create doors, floor and jambs from one record.
    occupied=[]
    for l in d['locations']:
        if l['origin'][2]==-4.8:
            ox,oy,_=l['origin'];occupied += [[ox+x,oy+y,w,h] for x,y,w,h in l['corridors']]
            occupied += [[ox+r['rect'][0],oy+r['rect'][1],*r['rect'][2:]] for r in l['rooms']]
    def overlap(a,b):return min(a[0]+a[2],b[0]+b[2])>max(a[0],b[0])+.01 and min(a[1]+a[3],b[1]+b[3])>max(a[1],b[1])+.01
    underground_rects=list(ut['corridors'])+[p[:4] for p in d['passages'] if p[4]==-4.8 and min(p[2:4])>=3 and max(p[2:4])>=8]
    occupied += [p[:4] for p in d['passages'] if p[4]==-4.8]
    for x,y,w,h in underground_rects:
        horizontal=w>=h;length=w if horizontal else h
        for t in range(3,max(4,int(length)-1),8):
            for sign in (-1,1):
                if horizontal:
                    ex,ey=x+t,y if sign<0 else y+h;rr=[ex-1,ey-2 if sign<0 else ey,2,2];side='south' if sign<0 else 'north';approach=[ex,ey-sign*1.5,-4.8]
                else:
                    ex,ey=x if sign<0 else x+w,y+t;rr=[ex-2 if sign<0 else ex,ey-1,2,2];side='east' if sign<0 else 'west';approach=[ex-sign*1.5,ey,-4.8]
                if any(overlap(rr,b) for b in occupied):continue
                if any(n['entry'][2]==-4.8 and math.dist(n['entry'][:2],[ex,ey])<3 for n in d['escape_niches']):continue
                if any(overlap(rr,s['rect']) for s in d['stairs']):continue
                approach_box=[min(ex,approach[0])-.5,min(ey,approach[1])-.5,abs(ex-approach[0])+1,abs(ey-approach[1])+1]
                if any(s['z_min']<=-4.8<=s['z_max'] and overlap(approach_box,[s['rect'][0]-.4,s['rect'][1]-.4,3.8,8.8]) for s in d['stairs']):continue
                n=dict(id=f'UT_V5_N{len(d["escape_niches"])}',site='UT',rect=rr,z=-4.8,entry=[ex,ey,-4.8],
                    side=side,opening_m=.9,large_pass=False,corridor_point=approach)
                d['escape_niches'].append(n);d['passages'].append([*rr,-4.8]);occupied.append(rr)
                for sign2 in (-1,1):
                    box=[ex+sign2*.725-.275,ey-.11,.55,.22] if horizontal else [ex-.11,ey+sign2*.725-.275,.22,.55]
                    d['collision_features'].append(dict(id=n['id']+'_jamb_'+str(sign2),rect=box,z=-4.8))

    # The added rear galleries and the covered SV24 route need the same real
    # escape opportunities as the old hospital corridors, including on upper floors.
    for l in d['locations']:
        if l['id'] in ('ST','RF','UT'):continue
        ox,oy,z=l['origin'];rects=[[ox+x,oy+y,w,h] for x,y,w,h in l['corridors']]
        if l['id']=='1F':rects += [p[:4] for p in d['passages'] if p[4]==0 and min(p[2:4])>=3 and max(p[2:4])>=8]
        obstacles=[]
        for ll in d['locations']:
            if ll['origin'][2]!=z:continue
            xx,yy,_=ll['origin'];obstacles += [[xx+r['rect'][0],yy+r['rect'][1],*r['rect'][2:]] for r in ll['rooms'] if r['enclosed']]
            if ll['id']!='ST':obstacles += [[xx+x,yy+y,w,h] for x,y,w,h in ll['corridors']]
        obstacles += [p[:4] for p in d['passages'] if p[4]==z]
        for x,y,w,h in rects:
            horiz=w>=h;length=w if horiz else h
            for t in range(3,max(4,int(length)-1),8):
                for sign in (-1,1):
                    if horiz:
                        ex,ey=x+t,y if sign<0 else y+h;rr=[ex-1,ey-2 if sign<0 else ey,2,2];side='south' if sign<0 else 'north';approach=[ex,ey-sign*1.5,z]
                    else:
                        ex,ey=x if sign<0 else x+w,y+t;rr=[ex-2 if sign<0 else ex,ey-1,2,2];side='east' if sign<0 else 'west';approach=[ex-sign*1.5,ey,z]
                    if any(overlap(rr,b) for b in obstacles):continue
                    if any(n['entry'][2]==z and math.dist(n['entry'][:2],[ex,ey])<3 for n in d['escape_niches']):continue
                    if any(s['z_min']<=z<=s['z_max'] and overlap(rr,s['rect']) for s in d['stairs']):continue
                    approach_box=[min(ex,approach[0])-.5,min(ey,approach[1])-.5,abs(ex-approach[0])+1,abs(ey-approach[1])+1]
                    if any(s['z_min']<=z<=s['z_max'] and overlap(approach_box,[s['rect'][0]-.4,s['rect'][1]-.4,3.8,8.8]) for s in d['stairs']):continue
                    n=dict(id=f'V5_N{len(d["escape_niches"])}',site=l['id'],rect=rr,z=z,entry=[ex,ey,z],side=side,
                           opening_m=.9,large_pass=False,corridor_point=approach)
                    d['escape_niches'].append(n);d['passages'].append([*rr,z]);obstacles.append(rr)
                    for sign2 in (-1,1):
                        box=[ex+sign2*.725-.275,ey-.11,.55,.22] if horiz else [ex-.11,ey+sign2*.725-.275,.22,.55]
                        d['collision_features'].append(dict(id=n['id']+'_jamb_'+str(sign2),rect=box,z=z))

    # Broad basement halls and stair foyers also need reachable internal cover.
    # These low-scale service niches leave the narrow through corridors clear.
    large=[('UT',r,-4.8) for r in ut['corridors'] if min(r[2:])>=12]
    large.append(('UT',[-22,-132,10,10],-4.8))
    large += [('1F',[-22,-132,10,10],0),('1F',[-34,-32,10,10],0)]
    large.append(('1F',[42,10,10,10],0))
    large += [('1F',p[:4],0) for p in d['passages'] if p[4]==0 and min(p[2:4])>=12 and max(p[2:4])>=18]
    for l in d['locations']:
        if l.get('compact'):
            ox,oy,z=l['origin'];large += [(l['id'],[ox+x,oy+y,w,h],z) for x,y,w,h in l['corridors'] if min(w,h)>=12]
    for site,(x,y,w,h),z in large:
        for xx in range(math.ceil(x+4),int(x+w-3),8):
            for yy in range(math.ceil(y+3),int(y+h-3),6):
                rr=[xx-1,yy-1,2,2];clear=[xx-1.5,yy-1.5,3,4]
                if any(s['z_min']<=z<=s['z_max'] and overlap(clear,[s['rect'][0]-.5,s['rect'][1]-.5,4,9]) for s in d['stairs']):continue
                if any(f['z']==z and overlap(clear,f['rect']) for f in d['collision_features']):continue
                if any(n['entry'][2]==z and overlap(clear,n['rect']) for n in d['escape_niches']):continue
                if any(l['origin'][2]==z and r['enclosed'] and overlap(clear,[l['origin'][0]+r['rect'][0],l['origin'][1]+r['rect'][1],*r['rect'][2:]]) for l in d['locations'] for r in l['rooms']):continue
                ex,ey=xx,yy+1;n=dict(id=f'V5_INTERIOR_{len(d["escape_niches"])}',site=site,rect=rr,z=z,
                    entry=[ex,ey,z],side='south',opening_m=.9,large_pass=False,internal=True,corridor_point=[ex,ey+1.5,z])
                d['escape_niches'].append(n)
                boxes=[([xx,yy-.925,z+1.3],[2,.15,2.6]),([xx-.925,yy,z+1.3],[.15,2,2.6]),([xx+.925,yy,z+1.3],[.15,2,2.6])]
                for i,(at,size) in enumerate(boxes):
                    fid=n['id']+'_wall_'+str(i);d['structural_members'].append(dict(id=fid,kind='niche_wall',at=at,size=size))
                    d['collision_features'].append(dict(id=fid,rect=[at[0]-size[0]/2,at[1]-size[1]/2,*size[:2]],z=z))
                for sign in (-1,1):
                    d['collision_features'].append(dict(id=n['id']+'_jamb_'+str(sign),rect=[ex+sign*.725-.275,ey-.11,.55,.22],z=z))

    # Protect the blind ends behind the six basement stair landings too.
    for key in ('DG','IF','PS','RH','HS','SV'):
        ox,oy,_=sites[key]['origin'];ex,ey=ox-8,oy+98
        n=dict(id='V5_BASEMENT_END_'+key,site='UT',rect=[ex-2,ey-1,2,2],z=-4.8,
            entry=[ex,ey,-4.8],side='east',opening_m=.9,large_pass=False,corridor_point=[ex+1.5,ey,-4.8])
        if any(p['entry'][2]==-4.8 and math.dist(p['entry'][:2],[ex,ey])<2.5 for p in d['escape_niches']):continue
        d['escape_niches'].append(n);d['passages'].append([*n['rect'],-4.8])
        for sign in (-1,1):d['collision_features'].append(dict(id=n['id']+'_jamb_'+str(sign),rect=[ex-.11,ey+sign*.725-.275,.22,.55],z=-4.8))

    # A former DG corridor pocket overlaps the newly registered hall threshold.
    hall_doors=[]
    for r in sites['DG']['rooms'][-2:]:hall_doors += [[-150+x,y] for x,y in [r['door']]+[e['at'] for e in r['extra_doors']]]
    removed={n['id'] for n in d['escape_niches'] if n['id']=='NICHE_380' or (n['entry'][2]==0 and any(math.dist(n['entry'][:2],p)<2.5 for p in hall_doors))}
    d['escape_niches']=[n for n in d['escape_niches'] if n['id'] not in removed]
    d['collision_features']=[f for f in d['collision_features'] if not any(f['id'].startswith(k+'_') for k in removed)]
    d['structural_members']=[f for f in d['structural_members'] if not any(f['id'].startswith(k+'_') for k in removed)]
    # New/relocated doors must cut the inner narrowing walls too, not only the
    # outer shell. A small clear porch fits the real standing capsule.
    porches=[]
    for l in d['locations']:
        ox,oy,z=l['origin']
        for r in l['rooms']:
            for e in [dict(at=r['door'],side=r['door_side'])]+r.get('extra_doors',[]):
                porches.append((ox+e['at'][0],oy+e['at'][1],z,e['side']))
    porches += [(*n['entry'],n['side']) for n in d['escape_niches'] if n['id'].startswith(('UT_V5_','V5_'))]
    pieces=[]
    for pp in d['partitions']:
        ax,ay,z=pp['a'];bx,by,_=pp['b'];horizontal=ay==by;lo,hi=(ax,bx) if horizontal else (ay,by);cuts=[]
        for px,py,pz,side in porches:
            if pz!=z:continue
            if horizontal and side in ('north','south') and abs(ay-py)<1.7:cuts.append((px-1.3,px+1.3))
            elif not horizontal and side in ('east','west') and abs(ax-px)<1.7:cuts.append((py-1.3,py+1.3))
        cursor=lo
        for a,b in sorted(cuts):
            if b<=cursor or a>=hi:continue
            if a>cursor:
                q=copy.deepcopy(pp);q['a']=[cursor,ay,z] if horizontal else [ax,cursor,z];q['b']=[min(a,hi),by,z] if horizontal else [bx,min(a,hi),z];pieces.append(q)
            cursor=max(cursor,b)
        if cursor<hi:
            q=copy.deepcopy(pp);q['a']=[cursor,ay,z] if horizontal else [ax,cursor,z];pieces.append(q)
    d['partitions']=pieces
    # All stair railings must be represented in the independent collision grid,
    # including the new cores. The flights themselves are tested by walking.
    for s in d['stairs']:
        x,y,_,_=s['rect'];z=s['z_min']
        while z<s['z_max']-.01:
            for xx in (x,x+3):d['collision_features'].append(dict(id=f'V5_RAIL_{s["id"]}_{xx}_{z}',rect=[xx-.04,y,.08,6.8],z=z))
            z=round(z+4.8,2)
    d['expansion_summary']=dict(original_zones=334,additional_zones=sum(len(l['rooms']) for l in d['locations'])-334,
        floors=d['floor_expansion'],loop_definition='unique room-pair cycle, not raw exit count',
        escape_audit_scope='story chapters 0/1/6/12/17/19, closed doors excluded as targets; exterior/roof reported separately',
        furnishing_status='physical prototype models with wardrobe and crouch-bed clearance, not final art')
    # Export current registers only after every promotion and new floor exists.
    OUT.mkdir(parents=True,exist_ok=True)
    def export(name,rows):
        if not rows:return
        fields=list(dict.fromkeys(k for r in rows for k in r))
        with (OUT/name).open('w',encoding='utf-8-sig',newline='') as f:
            wr=csv.DictWriter(f,fields,delimiter=';');wr.writeheader()
            for r in rows:wr.writerow({k:json.dumps(v,ensure_ascii=False) if isinstance(v,(dict,list)) else v for k,v in r.items()})
    allzones=[dict(r,site=l['id'],origin=l['origin']) for l in d['locations'] for r in l['rooms']]
    doors=[]
    for l in d['locations']:
        for r in l['rooms']:
            if r['enclosed']:
                doors.append(dict(zone=r['id'],site=l['id'],at=r['door'],side=r['door_side'],z=l['origin'][2],type=r['door_type'],role='primary'))
                doors += [dict(zone=r['id'],site=l['id'],z=l['origin'][2],**e) for e in r.get('extra_doors',[])]
    for key in ('monster_rules','secret_passages','shortcuts','hideouts','furnishings','utility_links','site_entries','loop_audit','zone_evolution','floor_expansion','escape_niches','structural_members','windows','network_devices','threat_nodes','movable_props'):
        export(key+'.csv',d[key])
    export('zones.csv',allzones);export('doors.csv',doors)
    print(json.dumps(dict(revision=5,zones=len(allzones),sites=len(d['locations']),stairs=len(d['stairs']),
        furnishings=len(d['furnishings']),hideouts=Counter(h['kind'] for h in d['hideouts']),
        secrets=Counter(s['site'] for s in d['secret_passages'])),ensure_ascii=False))
