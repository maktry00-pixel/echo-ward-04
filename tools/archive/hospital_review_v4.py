"""Corrections from the supplied two-page review; no invented missing v4 brief.

All coordinates are metres. The chapter/monster contracts are production data;
the architectural interactions are implemented separately by WardHospitalStory.
"""
from pathlib import Path
import csv
import json
import math

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'artifacts/unreal_hospital_v4'


def apply_review(d):
    sites = {l['id']: l for l in d['locations']}
    zones = {r['id']: (l, r) for l in d['locations'] for r in l['rooms']}
    d['review_revision'] = 4
    d['review_source'] = 'ЭХО — оценка карты v3.pdf (2 pages)'
    d['review_scope'] = 'architecture, refuge interactions and chapter production data; no new monster AI'
    d['structural_members'] = []
    # DG2 is a staggered upper wing, not a cantilever held up by thin air.
    # An accessible ground-floor undercroft and six column/beam frames support it.
    d['passages'] += [[-139, 46, 76, 16, 0], [-139, 68, 76, 16, 0]]
    for y in (48, 58, 72, 80):
        for x in (-136, -66):
            item = dict(id=f'DG_COLUMN_{x}_{y}',kind='column',at=[x,y,2.22],size=[.36,.36,4.44])
            d['structural_members'].append(item)
            d['collision_features'].append(dict(id=item['id'],rect=[x-.18,y-.18,.36,.36],z=0))
        d['structural_members'].append(dict(id=f'DG_BEAM_{y}',kind='beam',at=[-101,y,4.52],size=[70.36,.36,.56]))
    d['structural_support'] = dict(site='DG2',lower_site='DG',type='column_beam_undercroft',
        upper_z=4.8,ground_z=0,stair='DG',footprint=[-139,46,76,38],columns=8,
        note='The upper wing is offset in plan; the new undercroft supports it without moving any zone.')
    sites['DG']['bounds']=[-8,0,112,100]
    # E01 was an access-control room with an ambiguous name, never the lift shaft.
    zones['E01'][1]['name']='Досмотр перед шлюзом нижнего уровня'
    zones['E01'][1]['lift_access']=[50,12,-9.6]
    zones['E01'][1]['shaft_ref']='BED_LIFT'
    d['lift']['landings']=[dict(site=k,shaft=[48,4,4,6],z=z,door=[50,10,z],lobby=[50,12,z])
        for k,z in zip(('B2','B1','1F','2F','3F','RF'),d['lift']['stops'])]
    d['utility_links']=[]
    for key in ('DG','IF','PS','RH','HS','SV'):
        ox,oy,_=sites[key]['origin']
        d['utility_links'].append(dict(id='U_LINK_'+key,source='U_SPINE',target='U_'+key,
            width_m=4,points=[[50,oy+98,-4.8],[ox-4.5,oy+98,-4.8],[ox-4.5,oy+93.4,-4.8]],stair=key))
    d['utility_links'].append(dict(id='U_SV_TO_SV24',source='U_SV',target='SV24',width_m=3.4,
        points=[[145.5,323.4,-4.8],[145.5,323.4,0],[145.5,328,0],[198.5,328,0],
                [198.5,295,0],[230,295,0],[230,299.4,0]],stair='SV'))

    # At least two real portals for each enclosed A zone. Shared-wall exits use
    # the already empty front aisle; detached entrance suites receive two doors.
    for l,r in zones.values():
        if r['production_class']!='A' or not r['enclosed'] or r.get('extra_doors'):continue
        x,y,w,h=r['rect']; yy=y+h-1.4 if r['door_side']=='south' else y+1.4
        neighbour=None
        for q in l['rooms']:
            bx,by,bw,bh=q['rect']
            if q is r or q.get('hidden') or abs(by-y)>.01 or abs(bh-h)>.01:continue
            if abs(x+w-bx)<.01:neighbour=(q,[bx,yy],'east','west');break
            if abs(bx+bw-x)<.01:neighbour=(q,[x,yy],'west','east');break
        if neighbour:
            q,at,side,opposite=neighbour
            r.setdefault('extra_doors',[]).append(dict(at=at,side=side,type='ordinary',role='escape_exit'))
            q.setdefault('extra_doors',[]).append(dict(at=at,side=opposite,type='ordinary',role='escape_exit'))
            d['shortcuts'].append(dict(site=l['id'],a=r['id'],b=q['id'],at=at,kind='escape_exit'))
        else:
            at=[r['door'][0]+(3 if r['door'][0]+3<x+w-1 else -3),r['door'][1]]
            r.setdefault('extra_doors',[]).append(dict(at=at,side=r['door_side'],type='ordinary',role='escape_exit'))

    # E20 had both a legacy E19 shortcut and the required E21 escape exit.
    # Keep the A-room escape exit while restoring the B limit of two exits.
    for rid,at in (('E19',[22,69.4]),('E20',[22,69.4])):
        r=zones[rid][1]
        r['extra_doors']=[e for e in r.get('extra_doors',[]) if e['at']!=at]
    d['shortcuts']=[s for s in d['shortcuts'] if {s['a'],s['b']}!={'E19','E20'}]

    # Hiding recesses are part of the architecture, not a furniture pass. Put
    # them against the outer bay wall, away from every doorway/centre aisle.
    d['hideouts']=[]
    for l,r in zones.values():
        if not r['enclosed'] or r['production_class']=='C':continue
        x,y,w,h=r['rect']; ox,oy,z=l['origin']; south=r['door_side']=='south'
        bays=[b for b in d['subrooms'] if b['zone']==r['id']]
        candidates=[]
        for b in bays:
            bx,by,bw,bh=b['rect']
            # Use the outer side of a bay, never its central-aisle opening.
            xx=bx+.28 if bx<r['door'][0] else bx+bw-1.48
            candidates.append([ox+xx,oy+by+.12,z])
        if not candidates:
            candidates=[[ox+x+.4,oy+y+(.4 if south else h-2.2),z],
                        [ox+x+w-1.8,oy+y+(.4 if south else h-2.2),z]]
        count=2 if r['production_class']=='A' else 1
        for i,at in enumerate(candidates[:count]):
            # 1.20m outer width minus two 0.15m cheeks = 0.90m opening.
            item=dict(id=f'HIDE_{r["id"]}_{i+1}',zone=r['id'],site=l['id'],at=at,
                size=[1.2,1.0,2.1],opening_m=.9,small_pass=True,large_pass=False,
                chapters=r['chapters'],kind='architectural_recess')
            d['hideouts'].append(item)
            for suffix,rect in (('back',[at[0],at[1],1.2,.15]),('left',[at[0],at[1],.15,1.0]),
                                ('right',[at[0]+1.05,at[1],.15,1.0])):
                d['collision_features'].append(dict(id=item['id']+'_'+suffix,rect=rect,z=z))

    # About 15% of C zones have a physical maintenance door. It stays closed in
    # story mode and can be opened from inside, without unsealing the front door.
    c_rooms=[(l,r) for l,r in zones.values() if r['production_class']=='C' and r['enclosed']]
    d['secret_passages']=[]
    for l,r in sorted(c_rooms,key=lambda p:p[1]['id'])[:20]:
        if not r.get('extra_doors'):
            x,y,w,h=r['rect'];at=[r['door'][0]-3 if r['door'][0]-3>x+1 else r['door'][0]+3,r['door'][1]]
            r['extra_doors']=[dict(at=at,side=r['door_side'],type='manual',role='secret_maintenance')]
        e=r['extra_doors'][0];e['role']='secret_maintenance';e['inside_only']=True
        d['secret_passages'].append(dict(zone=r['id'],site=l['id'],at=e['at'],side=e['side'],inside_only=True))

    # Distinct B2 visits. Access state is independent of contamination/damage.
    d['zone_evolution']=[]
    for l,r in zones.values():
        if l['id']!='B2':continue
        r['environment_states']=[dict(chapter=0,state='clean'),dict(chapter=6,state='infected'),
            dict(chapter=15,state='damaged'),dict(chapter=18,state='collapsed')]
        d['zone_evolution'].append(dict(zone=r['id'],site='B2',states=r['environment_states']))
    d['environment_profiles']={
        'clean':dict(wall=[.56,.63,.66],lower=[.15,.24,.30],light=1.0),
        'infected':dict(wall=[.38,.49,.35],lower=[.20,.28,.12],light=.8),
        'damaged':dict(wall=[.39,.32,.29],lower=[.26,.12,.10],light=.65),
        'collapsed':dict(wall=[.24,.25,.27],lower=[.19,.08,.08],light=.45)}
    # Expand chapter spatial sequences. Hours are not inferred from anchor count.
    extensions={6:['N12','B120','E01','E02','E03','E04','E05','E06','E07','E08','E14','E15','E16'],
        7:['E16','E17','E18','E19','E20','E21','E22','E23','B120','B112','R40'],
        10:['E10','E16','E17','B120','SV24','SV23','SV20','SV14','SV07','HS24','HS17','HS12','HS11'],
        15:['B120','E01','E02','E03','E04','E05','E08','E09','E10','E11','E12','E16']}
    for c in d['chapters']:
        ids=list(dict.fromkeys(extensions.get(c['id'],[])+c['anchors']))
        c['route_zones']=ids;c['duration_status']='requires_playtesting_not_hours_per_zone'
        for rid in ids:
            r=zones[rid][1]
            r['chapters']=sorted(set(r['chapters']+[c['id']]))
            if r['production_class']=='C':r['states'].append(dict(chapter=c['id'],state='open'))
    for l,r in zones.values():
        # Repeated reviews may author chapter states out of order.
        r['states']=sorted(r['states'],key=lambda x:x['chapter'])
    # The half-size DG ground floor still needs a third independent room/hall loop.
    r,q=zones['DG04'][1],zones['DG05'][1];at=[65,14.6]
    r.setdefault('extra_doors',[]).append(dict(at=at,side='east',type='ordinary',role='loop'))
    q.setdefault('extra_doors',[]).append(dict(at=at,side='west',type='ordinary',role='loop'))
    d['shortcuts'].append(dict(site='DG',a='DG04',b='DG05',at=at,kind='loop'))
    # Corridor recesses, including the long galleries and underground branches.
    # A separate 2x2m floor pocket avoids cluttering the centre of the corridor.
    d['escape_niches']=[]
    rects=[]
    for l in d['locations']:
        if l['id'] in ('ST','RF'):continue
        ox,oy,z=l['origin']
        rects += [(l['id'],[ox+x,oy+y,w,h],z) for x,y,w,h in l['corridors']]
    rects += [('UT' if z<0 else '1F',[x,y,w,h],z) for x,y,w,h,z in d['passages']]
    occupied={z:[] for z in (-9.6,-4.8,0,4.8,9.6,14.4)}
    for l,r in zones.values():
        ox,oy,z=l['origin'];x,y,w,h=r['rect'];occupied[z].append([ox+x,oy+y,w,h])
    for _,rect,z in rects:occupied[z].append(rect)
    for s in d['stairs']:
        for z in occupied:
            if s['z_min']<=z<=s['z_max']:occupied[z].append(s['rect'])
    targets={z:[] for z in occupied}
    for l,r in zones.values():
        if r['enclosed']:
            ox,oy,z=l['origin'];targets[z].append([ox+r['door'][0],oy+r['door'][1]])
    def overlap(a,b):
        return min(a[0]+a[2],b[0]+b[2])>max(a[0],b[0])+.01 and min(a[1]+a[3],b[1]+b[3])>max(a[1],b[1])+.01
    missed=[]
    for site,(x,y,w,h),z in rects:
        horizontal=w>=h;length=w if horizontal else h
        for t in range(4,max(5,int(length)),8):
            px,py=(x+t,y+h/2) if horizontal else (x+w/2,y+t)
            if any(p['z']==z and overlap([px-.35,py-.35,.7,.7],p['rect']) for p in d['collision_features']):continue
            if any(math.dist((px,py),q)<=10 for q in targets[z]):continue
            found=False
            for shift in (0,-2,2,-4,4):
                cx,cy=round(px+(shift if horizontal else 0)),round(py+(0 if horizontal else shift))
                options=([([cx-1,y-2,2,2],[cx,y],'south'),([cx-1,y+h,2,2],[cx,y+h],'north')]
                    if horizontal else [([x-2,cy-1,2,2],[x,cy],'east'),([x+w,cy-1,2,2],[x+w,cy],'west')])
                for rect,entry,side in options:
                    if horizontal and not x+.75<=entry[0]<=x+w-.75:continue
                    if not horizontal and not y+.75<=entry[1]<=y+h-.75:continue
                    if any(overlap(rect,r) for r in occupied[z]):continue
                    approach=[min(px,entry[0])-.35,min(py,entry[1])-.35,abs(px-entry[0])+.7,abs(py-entry[1])+.7]
                    if any(s['z_min']<=z<=s['z_max'] and overlap(approach,s['rect']) for s in d['stairs']):continue
                    item=dict(id=f'NICHE_{len(d["escape_niches"])+1:03}',site=site,rect=rect,z=z,entry=[*entry,z],
                        side=side,opening_m=.9,large_pass=False,corridor_point=[px,py,z])
                    d['escape_niches'].append(item);occupied[z].append(rect);d['passages'].append([*rect,z]);targets[z].append(entry)
                    # The thin narrowing walls must not seal the path to the pocket.
                    parts=[]
                    for p in d['partitions']:
                        ax,ay,pz=p['a'];bx,by,_=p['b']
                        if pz!=z:parts.append(p);continue
                        cut=None
                        if not horizontal and ax==bx and min(px,entry[0])<=ax<=max(px,entry[0]) and ay<cy+.8 and by>cy-.8:cut=(max(ay,cy-.8),min(by,cy+.8))
                        if horizontal and ay==by and min(py,entry[1])<=ay<=max(py,entry[1]) and ax<cx+.8 and bx>cx-.8:cut=(max(ax,cx-.8),min(bx,cx+.8))
                        if cut:
                            for lo,hi in ((ay if ax==bx else ax,cut[0]),(cut[1],by if ax==bx else bx)):
                                if hi<=lo:continue
                                q=dict(p);q['a']=[ax,lo,pz] if ax==bx else [lo,ay,pz];q['b']=[bx,hi,pz] if ax==bx else [hi,by,pz];parts.append(q)
                        else:parts.append(p)
                    d['partitions']=parts;found=True;break
                if found:break
            if not found and not any(math.dist((px,py),q)<=15 for q in targets[z]):missed.append([site,px,py,z])
    for l,r in zones.values():
        ox,oy,z=l['origin']
        targets[z] += [[ox+e['at'][0],oy+e['at'][1]] for e in r.get('extra_doors',[])]
    still_missing=[]
    for site,px,py,z in missed:
        if any(math.dist((px,py),q)<=14 for q in targets[z]):continue
        options=[]
        for ss,(x,y,w,h),zz in rects:
            if zz!=z or not(x<=px<=x+w and y<=py<=y+h):continue
            if w>=h and h>=6:
                options += [([round(px)-1,y+.2,2,2],[round(px),y+2.2],'south'),
                            ([round(px)-1,y+h-2.2,2,2],[round(px),y+h-2.2],'north')]
            elif h>w and w>=6:
                options += [([x+.2,round(py)-1,2,2],[x+2.2,round(py)],'east'),
                            ([x+w-2.2,round(py)-1,2,2],[x+w-2.2,round(py)],'west')]
        blockers=[]
        for l,r in zones.values():
            ox,oy,zz=l['origin'];rx,ry,rw,rh=r['rect']
            if zz==z:blockers.append([ox+rx,oy+ry,rw,rh])
        blockers += [n['rect'] for n in d['escape_niches'] if n['z']==z]
        blockers += [s['rect'] for s in d['stairs'] if s['z_min']<=z<=s['z_max']]
        found=False
        for rect,entry,side in options:
            if any(overlap(rect,b) for b in blockers):continue
            x,y,w,h=rect
            item=dict(id=f'NICHE_{len(d["escape_niches"])+1:03}',site=site,rect=rect,z=z,
                entry=[*entry,z],side=side,opening_m=.9,large_pass=False,internal=True,corridor_point=[px,py,z])
            d['escape_niches'].append(item);targets[z].append(entry)
            if side in ('north','south'):
                back=y+(.075 if side=='south' else 1.925)
                boxes=[([x+1,back,z+1.3],[2,.15,2.6]),([x+.075,y+1,z+1.3],[.15,2,2.6]),([x+1.925,y+1,z+1.3],[.15,2,2.6])]
            else:
                back=x+(.075 if side=='east' else 1.925)
                boxes=[([back,y+1,z+1.3],[.15,2,2.6]),([x+1,y+.075,z+1.3],[2,.15,2.6]),([x+1,y+1.925,z+1.3],[2,.15,2.6])]
            for i,(at,size) in enumerate(boxes):
                d['structural_members'].append(dict(id=item['id']+'_wall_'+str(i),kind='niche_wall',at=at,size=size))
                d['collision_features'].append(dict(id=item['id']+'_wall_'+str(i),rect=[at[0]-size[0]/2,at[1]-size[1]/2,*size[:2]],z=z))
            parts=[]
            for p in d['partitions']:
                ax,ay,pz=p['a'];bx,by,_=p['b'];cut=None
                if pz==z:
                    if ax==bx and x-.2<=ax<=x+2.2 and ay<y+2.2 and by>y-.2:cut=(max(ay,y-.2),min(by,y+2.2))
                    if ay==by and y-.2<=ay<=y+2.2 and ax<x+2.2 and bx>x-.2:cut=(max(ax,x-.2),min(bx,x+2.2))
                    if not cut and ax==bx and min(px,entry[0])<=ax<=max(px,entry[0]) and ay<entry[1]<by:cut=(max(ay,entry[1]-.8),min(by,entry[1]+.8))
                    if not cut and ay==by and min(py,entry[1])<=ay<=max(py,entry[1]) and ax<entry[0]<bx:cut=(max(ax,entry[0]-.8),min(bx,entry[0]+.8))
                if cut:
                    for lo,hi in ((ay if ax==bx else ax,cut[0]),(cut[1],by if ax==bx else bx)):
                        if hi<=lo:continue
                        q=dict(p);q['a']=[ax,lo,pz] if ax==bx else [lo,ay,pz];q['b']=[bx,hi,pz] if ax==bx else [hi,by,pz];parts.append(q)
                else:parts.append(p)
            d['partitions']=parts;found=True;break
        if not found:still_missing.append([site,px,py,z])
    still_missing=[p for p in still_missing if not any(math.dist(p[1:3],q)<=15 for q in targets[p[3]])]
    d['corridor_escape_audit']=dict(sample_spacing_m=8,target_max_m=15,unresolved_samples=still_missing,
        note='Centreline planning samples; native capsule sweeps verify each new niche entry. Not a whole-surface geodesic proof.')
    for n in d['escape_niches']:
        x,y,z=n['entry'];ns=n['side'] in ('north','south')
        for sign in (-1,1):
            rect=[x+sign*.725-.275,y-.11,.55,.22] if ns else [x-.11,y+sign*.725-.275,.22,.55]
            d['collision_features'].append(dict(id=n['id']+'_jamb_'+str(sign),rect=rect,z=z))
    d['story_routes']=[dict(chapter=c['id'],anchors=c['anchors'],zones=c['route_zones'],
        mode='physical_walk',required_infrastructure=['U_SPINE','U_SV','SV'] if c['id'] in (9,10) else []) for c in d['chapters']]

    classes={
        'patients':('Первые пациенты','A','small'),'hound':('Ищейка','A','small'),
        'carrier':('Носитель','A','small'),'repeater':('Повторяющий','A','small'),
        'transporter':('Переносчик','A','medium'),'collector':('Сборщик','A','medium'),
        'stitched':('Сшитый','B','large'),'blind_surgeon':('Слепой хирург','B','large'),
        'linked':('Связанные','B','medium'),'armoured':('Панцирный','B','large'),
        'nest':('Гнездо','B','large'),'conductor':('Проводник','B','medium'),
        'central_organism':('Центральный организм','B','large'),
        'listener':('Слушатель','C','medium'),'keeper':('Смотритель','C','medium'),
        'impostor':('Подмена','C','medium'),'pursuer':('Преследователь','C','large')}
    old={m['id']:m for m in d['monster_rules']}
    d['monster_rules']=[]
    for key,(name,combat,size) in classes.items():
        m=old.get(key,dict(id=key,first_chapter=1,implemented=False))
        m.update(name=name,combat_class=combat,size_class=size,classification_status='review_proposal',
            kill_condition='immediate' if combat=='A' else 'finale_only' if combat=='B' else 'never',
            niche_pass=size=='small',barricade_break=size=='large',
            distraction_sources=['tray','bell'] if key in ('listener','repeater') else [],
            counterplay=['escape','bolt','power_off'] if combat=='C' else ['escape','cover','combat'])
        if key=='pursuer':m['finale_variant']=dict(combat_class='B',enabled=False,reason='branch dependent; not confirmed')
        d['monster_rules'].append(m)
    d['movable_prop_rules']=[dict(type=t,pushable=True,blocks=['small','medium'],fails_against=['large'],noise=True,
        runtime_status='interaction_prototype_no_monster_ai') for t in ('wheelchair','gurney','screen')]
    d['distraction_rules']=[dict(type=t,attracts=['listener','repeater'],noise_radius_m=18) for t in ('tray','bell')]
    d['movable_props']=[dict(id='PROP_'+t,type=t,at=[-130+i*5,54,0],site='DG',
        size=size,pushable=t!='bell',noise_radius_m=18 if t in ('tray','bell') else 9,
        visual_status='architectural_prototype') for i,(t,size) in enumerate((
            ('wheelchair',[.8,1.05,1.1]),('gurney',[.85,2.0,.85]),('screen',[.35,1.4,1.9]),
            ('tray',[.5,.35,.08]),('bell',[.3,.3,.3])))]
    for n in d['hideouts']:n['chapters']=zones[n['zone']][1]['chapters']

    from hospital_escape_refinement import tighten_escape_geometry
    tighten_escape_geometry(d)

    # Explicit registers remove the ambiguity of a PDF containing only rooms.
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'review_contract.json').write_text(json.dumps({k:d[k] for k in ('review_revision','structural_support',
        'utility_links','monster_rules','zone_evolution','movable_prop_rules','distraction_rules')},ensure_ascii=False,indent=2),encoding='utf-8')
    def export(name,rows):
        if not rows:return
        fields=list(dict.fromkeys(k for r in rows for k in r))
        with (OUT/name).open('w',encoding='utf-8-sig',newline='') as f:
            w=csv.DictWriter(f,fields,delimiter=';');w.writeheader()
            for r in rows:w.writerow({k:json.dumps(v,ensure_ascii=False) if isinstance(v,(dict,list)) else v for k,v in r.items()})
    doors=[]
    for l,r in zones.values():
        if r['enclosed']:
            doors.append(dict(zone=r['id'],site=l['id'],at=r['door'],type=r['door_type'],role='primary',z=l['origin'][2]))
            doors += [dict(zone=r['id'],site=l['id'],z=l['origin'][2],**e) for e in r.get('extra_doors',[])]
    for name,rows in [('doors.csv',doors),('windows.csv',d['windows']),('network_devices.csv',d['network_devices']),
        ('threat_nodes.csv',d['threat_nodes']),('hideouts.csv',d['hideouts']),('zone_evolution.csv',d['zone_evolution']),
        ('monster_rules.csv',d['monster_rules']),('utility_links.csv',d['utility_links']),('structural_members.csv',d['structural_members']),
        ('corridor_niches.csv',d['escape_niches']),('chapter_routes.csv',d['story_routes']),('movable_props.csv',d['movable_props'])]:export(name,rows)
