"""Connected supported galleries, corridor equipment and real hospital signage.

Builds on a frozen v15 map. Door/room dimensions and story positions are retained.
"""
import collections,functools,json,math,shutil
from pathlib import Path
from hospital_campus_v14 import Campus,shape
from hospital_expansion_v15 import profile
from hospital_services_v13 import text_rectangles
from hospital_clutter_v10 import bounds,rotate
from hospital_furnishing_v6 import overlap
from hospital_item_types_v12 import refresh_navigation
ROOT=Path(__file__).resolve().parents[1];PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json';OUT=ROOT/'artifacts/unreal_hospital_v16';SOURCE='hospital_realism_v16'

@functools.lru_cache(None)
def lettering(text):return text_rectangles(text)

def equipment(kind):
    p=[]
    def b(x,y,z,w,d,h,m='camp_steel'):p.append([x,y,z,w,d,h,m])
    if kind=='corridor_bin':
        b(0,0,.30,.34,.30,.60,'camp_sage');b(0,0,.62,.37,.33,.04,'camp_rubber');b(0,.17,.27,.11,.04,.025)
    elif kind=='fire_station':
        b(0,0,1.05,.52,.16,.85,'camp_red');b(0,.09,1.05,.44,.02,.72,'camp_cream')
        for x in (-.15,.15):b(x,.12,1.05,.04,.025,.55,'camp_red')
        b(0,.13,1.05,.34,.025,.04,'camp_red');b(0,.12,.72,.35,.025,.04,'camp_rubber')
    elif kind=='hand_sanitizer':
        b(0,0,1.28,.15,.14,.28,'camp_cream');b(0,.08,1.24,.10,.05,.035,'camp_steel');b(0,.11,1.10,.18,.11,.02)
    elif kind=='notice_board':
        b(0,0,1.7,1.25,.05,.72,'camp_wood')
        for x in (-.42,-.15,.15,.42):b(x,.03,1.72,.21,.008,.31,'camp_cream')
        for x in (-.60,.60):b(x,.04,1.7,.025,.03,.74)
    elif kind=='wall_clock':
        b(0,0,2.65,.30,.06,.30,'camp_rubber');b(0,.035,2.65,.27,.015,.27,'camp_cream')
        b(0,.05,2.70,.012,.008,.10,'camp_ink');b(.045,.05,2.65,.10,.008,.012,'camp_ink')
    elif kind=='phone_station':
        b(0,0,1.35,.24,.08,.35,'camp_cream');b(-.065,.06,1.36,.05,.08,.27,'camp_rubber')
        for x in (0,.05,.10):
            for z in (1.28,1.33,1.38):b(x,.05,z,.02,.02,.02,'camp_ink')
    elif kind=='wheelchair_bay':
        for x in (-.32,.32):
            b(x,0,.32,.04,.52,.62,'camp_rubber');b(x,0,.32,.06,.04,.58)
            b(x,.25,.18,.04,.15,.30,'camp_rubber');b(x,-.24,.67,.04,.04,.72)
        b(0,0,.52,.62,.48,.05,'camp_teal');b(0,-.25,.78,.62,.05,.48,'camp_teal')
        for x in (-.29,.29):b(x,.05,.73,.045,.58,.045)
    elif kind=='crash_cart':
        p=profile('chart_cart')
        b(0,0,1.03,.4,.23,.25,'camp_red');b(0,.13,1.04,.30,.02,.18,'camp_teal')
        b(-.33,-.15,1.4,.025,.025,1.1);b(-.33,-.15,1.95,.35,.025,.025)
    else:return profile(kind) if kind in ('chart_cart','cleaning_cart','mail_cart','visitor_pair','water_dispenser','beverage_station') else shape(kind)
    if kind=='wheelchair_bay':
        low=min(a[2]-a[5]/2 for a in p)
        for a in p:a[2]-=low
    return p

def apply(d):
    # Return bridge connects the old unsupported terminal to BOTH rear buildings.
    added=[[52,326,90,4,4.8],[-38,326,86,4,4.8]]
    d['passages']+=added;d['campus_upgrade']['gallery_rects']+=added
    # Old endpoint recesses now form part of a full-width connection. Their
    # 90cm jambs must not remain in the middle of the new 4m gallery.
    converted=[n for n in d['escape_niches'] if abs(n['entry'][2]-4.8)<.01 and any(overlap(n['rect'],p[:4]) for p in added)]
    converted_ids={n['id'] for n in converted}
    d['escape_niches']=[n for n in d['escape_niches'] if n['id'] not in converted_ids]
    d['collision_features']=[f for f in d['collision_features'] if f['id'].rsplit('_jamb_',1)[0] not in converted_ids]
    for sid,x,side in [('SV2',142,'west'),('HS2',-38,'east')]:
        d['site_entries'].append(dict(id='V16_REAR_'+sid,site=sid,at=[x,328,4.8],side=side,width=2.4))
        d['campus_plan']['connections'].append(dict(from_site='2F',to_site=sid,passage=added[0 if sid=='SV2' else 1],kind='rear_return_gallery'))
    for x,y,w,h,z in added:
        for xx in range(math.ceil(x)+6,math.floor(x+w)-3,10):
            ident=f'V16_NICHE_{xx}';rect=[xx-1,y-2,2,2]
            d['escape_niches'].append(dict(id=ident,site='2F',rect=rect,z=z,entry=[xx,y,z],side='south',opening_m=.9,large_pass=False,corridor_point=[xx,y+1.5,z]))
            d['passages'].append([*rect,z])
            for side,rx in ((-1,xx-1),(1,xx+.45)):
                d['collision_features'].append(dict(id=ident+f'_jamb_{side}',rect=[rx,y-.11,.55,.22],z=z,source=SOURCE))
    c=Campus(d);c.layer=16;c.source=SOURCE;c.prefix='REAL16';e=c.e
    # Portal frames carry the deck, with <=12m between supports. Ground routes
    # remain clear between the paired legs; the cross-beam is above head height.
    supports=[]
    for bi,(x,y,w,h,z) in enumerate(d['campus_upgrade']['gallery_rects']):
        horizontal=w>h;length=w if horizontal else h;stations=[]
        for n in range(math.ceil(length/12)+1):
            ideal=min(length-.8,max(.8,n*length/math.ceil(length/12)));choice=None
            for delta in (0,.5,-.5,1,-1,1.5,-1.5,2,-2,3,-3):
                t=max(.6,min(length-.6,ideal+delta));legs=[]
                for sign in (-1,1):
                    xx=x+t if horizontal else x+w/2+sign*(w/2+.65);yy=y+h/2+sign*(h/2+.65) if horizontal else y+t
                    rr=[xx-.20,yy-.20,.40,.40]
                    if overlap(rr,[45,109,10,4]):break  # real vehicle underpass
                    if c.floor.hits(0,rr,.05) or c.reserved.hits(0,rr,.08):break
                    if any(overlap(rr,[s['origin'][0]+r['rect'][0],s['origin'][1]+r['rect'][1],*r['rect'][2:]]) for s in d['locations'] if s['origin'][2]==0 for r in s['rooms'] if r['enclosed']):break
                    legs.append([xx,yy,0])
                if len(legs)==2:choice=(t,legs);break
            if not choice:continue
            t,legs=choice;parts=[]
            for xx,yy,_ in legs:parts.append([xx,yy,2.13,.4,.4,4.66,'camp_concrete']);parts.append([xx,yy,-.05,.80,.80,.3,'camp_concrete'])
            centre=[x+t,y+h/2] if horizontal else [x+w/2,y+t]
            parts.append([*centre,4.50,.45 if horizontal else w+1.7,h+1.7 if horizontal else .45,.40,'camp_steel'])
            # Absolute coordinates avoid silently translating the two legs.
            item=c.emit('gallery_support_frame','ST',parts,[0,0,0],strict=False,partial=True,extra=dict(mount='structure',gallery=bi,station=t,legs=legs))
            supports.append(item['id']);stations.append(t)
        if not stations:raise RuntimeError(('unsupported_gallery',bi))
    # Ceiling fixtures on the new rear loop, with real light anchors.
    for x,y,w,h,z in added:
        for xx in range(math.ceil(x)+5,math.floor(x+w)-2,10):
            c.emit('rear_gallery_lamp','ST',[[0,0,3.6,.95,.22,.08,'lamp']],[xx,y+h/2,z],solid=False,strict=False,extra=dict(mount='ceiling'))
            e['white_lights'].append([xx,y+h/2,z+3.1])
    # Fence topper is on solid perimeter runs only, never across a gate.
    segments=d['campus_plan']['clinical_boundary']['segments'];mesh_panels=0
    for a,b in segments:
        length=math.dist(a,b);along=a[1]==b[1];n=math.ceil(length/3)
        for k in range(n):
            xx=a[0]+(b[0]-a[0])*(k+.5)/n;yy=a[1]+(b[1]-a[1])*(k+.5)/n;span=length/n
            p=[[0,0,6.30,span,.035,1.0,'camp_ink']]
            # A sparse welded mesh with visible metal wires, 15cm grid.
            p=[]
            for zz in (5.82,6.12,6.42,6.72):p.append([0,0,zz,span,.025,.025,'camp_steel'])
            for j in range(math.ceil(span/.15)+1):p.append([-span/2+j*span/math.ceil(span/.15),0,6.27,.018,.025,.9,'camp_steel'])
            for dx in (-span/2,span/2):p.append([dx,0,6.30,.055,.07,1.10,'camp_steel'])
            c.emit('perimeter_mesh','ST',p,[xx,yy,0],0 if along else 1,solid=False,strict=False,extra=dict(mount='fence_top'));mesh_panels+=1
    # Double-sided readable Cyrillic plates, sized as actual wall signage.
    signs=[]
    def plate(text,at,turn=0,w=1.05,h=.34,kind='room_plate',zone='ST',colour='camp_teal'):
        p=[[0,0,0,w,.04,h,colour]];scale=min((w-.06)/256,(h-.04)/76)
        for px,py,pw,ph in lettering(text):p.append([(px+pw/2-128)*scale,.026,(38-py-ph/2)*scale,pw*scale,.008,ph*scale,'camp_cream'])
        item=c.emit(kind,zone,p,at,turn,solid=False,strict=False,extra=dict(mount='wall_sign',text=text))
        signs.append(item['id']);return item
    for zid,(s,r) in c.zones.items():
        if not r['enclosed']:continue
        ox,oy,z=s['origin'];x,y,w,h=r['rect'];dx,dy=r['door'];side=r['door_side'];ns=side in ('north','south')
        normal={'north':(0,-1),'south':(0,1),'west':(-1,0),'east':(1,0)}[side];turn={'north':2,'south':0,'west':1,'east':3}[side]
        choices=[]
        for sign in (1,-1):
            xx=ox+dx+(1.70*sign if ns else 0)+normal[0]*.17;yy=oy+dy+(0 if ns else 1.70*sign)+normal[1]*.17
            along=xx-ox if ns else yy-oy
            if (x+.55<=along<=x+w-.55) if ns else (y+.55<=along<=y+h-.55):choices.append([xx,yy,z+1.70])
        if choices:
            name=r['name'].split('/')[0].strip();name=name if len(name)<41 else name[:38]+'…'
            plate(zid+' · '+name,choices[0],turn,zone=zid)
    print('Room plates',len(signs),flush=True)
    # Corridor objects are placed off the protected travel/door/escape routes.
    floor_items=[];wall_items=[];attempts=0
    for s in d['locations']:
        if s['id'] in ('ST','RF','UT'):continue
        ox,oy,z=s['origin'];sid=s['id'];placed_here=[]
        for ci,rect in enumerate(s['corridors']):
            x,y,w,h=rect;horizontal=w>=h;length=max(w,h)
            if length<9:continue
            for seq,t in enumerate(range(4,math.floor(length-2),11)):
                kinds=['visitor_pair','corridor_bin','chart_cart','water_dispenser','cleaning_cart','wheelchair_bay','crash_cart','mail_cart']
                kind=kinds[(seq+ci+len(floor_items))%len(kinds)];p=equipment(kind)
                bb=[min(a[j]-a[j+3]/2 for a in p) for j in (0,1)]+[max(a[j]+a[j+3]/2 for a in p) for j in (0,1)]
                found=None
                # Both edge strips; retry inward if an authored partition intervenes.
                for edge in (0,1):
                    turn=(0 if edge==0 else 2) if horizontal else (3 if edge==0 else 1)
                    fw,fd=bb[2]-bb[0],bb[3]-bb[1]
                    if turn%2:fw,fd=fd,fw
                    for inset in (.22,.55,.9,1.25,1.6):
                        xx=ox+x+t if horizontal else ox+x+(fw/2+inset if edge==0 else w-fw/2-inset)
                        yy=oy+y+(fd/2+inset if edge==0 else h-fd/2-inset) if horizontal else oy+y+t
                        rr=[xx-fw/2,yy-fd/2,fw,fd];attempts+=1
                        if rr[0]<ox+x+.05 or rr[1]<oy+y+.05 or rr[0]+fw>ox+x+w-.05 or rr[1]+fd>oy+y+h-.05:continue
                        if c.floor.hits(z,rr,.12) or c.reserved.hits(z,rr,.16):continue
                        if any(r['enclosed'] and overlap(rr,[ox+r['rect'][0],oy+r['rect'][1],*r['rect'][2:]]) for r in s['rooms']):continue
                        item=c.emit(kind,sid,p,[xx,yy,z],turn,extra=dict(mount='corridor_floor',site=sid,corridor=ci))
                        if item:found=item;break
                    if found:break
                if found:floor_items.append(found['id']);placed_here.append(found)
        # Site directories at the circulation spine, plus clocks and emergency stations.
        # Hanging signs are above doors and player headroom, on both faces.
        dirs=s['corridors'][:4]
        for ci,(x,y,w,h) in enumerate(dirs):
            if max(w,h)<10:continue
            xx=ox+x+w/2;yy=oy+y+h/2
            txt=s['name'].split('/')[0].strip()[:30]+' · '+('ЛИФТ / ЛЕСТНИЦА' if ci%2==0 else 'ВЫХОД / ПОСТ СЕСТРЫ')
            for turn in (0,2):plate(txt,[xx,yy,z+2.95],turn,w=1.8,h=.34,kind='direction_board',zone=sid)
        # Use actual raster wall planes beside the clear corridor, not room centres.
        walls=[q for q in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf8')) if abs(q['z']-z)<.01]
        for wi,kind in enumerate(('fire_station','notice_board','phone_station','hand_sanitizer','wall_clock')):
            placed=0
            for q in walls:
                wx,wy,ww,wh=q['rect'];ns=ww>wh;length=ww if ns else wh
                if length<1.4:continue
                # Sample each face; accept only a free corridor location in THIS site.
                for side in (-1,1):
                    xx=wx+ww/2+(0 if ns else side*.22);yy=wy+wh/2+(side*.22 if ns else 0)
                    if not any(ox+x+.2<xx<ox+x+w-.2 and oy+y+.2<yy<oy+y+h-.2 for x,y,w,h in s['corridors']):continue
                    if any(r['enclosed'] and overlap([xx-.35,yy-.35,.7,.7],[ox+r['rect'][0],oy+r['rect'][1],*r['rect'][2:]]) for r in s['rooms']):continue
                    # Keep wall equipment outside every door's swing/approach corridor.
                    if c.reserved.hits(z,[xx-.4,yy-.4,.8,.8],.05):continue
                    turn=(0 if side>0 else 2) if ns else (3 if side>0 else 1)
                    item=c.emit(kind,sid,equipment(kind),[xx,yy,z],turn,solid=False,extra=dict(mount='corridor_wall',site=sid))
                    if item:
                        wall_items.append(item['id']);placed+=1
                        if kind=='fire_station':plate('ПОЖАРНЫЙ ПОСТ',[xx,yy,z+2.0],turn,w=.55,h=.20,kind='fire_plate',zone=sid,colour='camp_red')
                        break
                if placed>=2:break
        if placed_here:c.capture(placed_here[0],'v16_corridor_'+sid)
    # Facade names and external floodlights make buildings and gates readable.
    for sid in ('DG','IF','PS','RH','HS','SV','1F'):
        s=c.sites[sid];ox,oy,z=s['origin'];plate(s['name'].split('/')[0][:42],[ox+54,oy-.18,z+3.2],2,w=3.6,h=.55,kind='building_name',zone=sid)
    for zid in ('X01','X02','X03','X04','X05'):
        s,r=c.zones[zid];ox,oy,z=s['origin'];xx,yy=r['door'];plate(r['name'],[ox+xx,oy+yy+.20,z+3.25],0,w=3.2,h=.45,kind='building_name',zone=zid)
    lamps=0
    for x in range(-165,278,24):
        for y in (-49.6,365.6):
            c.emit('security_floodlight','ST',[[0,0,5.1,.08,.08,.50,'camp_steel'],[0,.25,5.30,.55,.45,.12,'camp_steel'],[0,.25,5.23,.47,.36,.025,'lamp']],[x,y,0],solid=False,strict=False,extra=dict(mount='wall_light'))
            e['white_lights'].append([x,y,4.9]);lamps+=1
    for ci,(x,y,w,h,z) in enumerate(added):
        d['furnishing_captures'].append(dict(id=f'v16_return_{ci}',at=[50 if ci==0 else -10,328,6.5],target=[130 if ci==0 else -38,328,6.4],envelope=True))
    d['furnishing_captures']+= [dict(id='v16_spine_supports',at=[61,310,1.8],target=[50,315,3.1],envelope=True),dict(id='v16_perimeter_mesh',at=[70,-60,2.0],target=[70,-50,6.3],envelope=True)]
    for zid in ('R07','PS09','X01'):
        s,r=c.zones[zid];ox,oy,z=s['origin'];dx,dy=r['door'];side=r['door_side'];sgn=-1 if side=='north' else 1
        d['furnishing_captures'].append(dict(id='v16_plate_'+zid,at=[ox+dx,oy+dy+sgn*2.8,z+1.7],target=[ox+dx+1.4,oy+dy,z+1.7]))
    d['review_revision']=16
    d['realism_upgrade']=dict(version=16,items=c.items,support_frames=supports,rear_connections=added,converted_endpoint_niches=converted,wall_signs=signs,corridor_items=floor_items,wall_equipment=wall_items,mesh_panels=mesh_panels,floodlights=lamps,menu_origin_cm=[300000,300000,2000],corridor_clearance='Original protected door, route and hiding access is retained; endpoint recesses became full-width passages and new recesses line the return route.')
    stats=dict(new_items=len(c.items),new_parts=sum(i['parts'] for i in c.items),signs=len(signs),corridor_floor_items=len(floor_items),wall_equipment=len(wall_items),support_frames=len(supports),mesh_panels=mesh_panels,floodlights=lamps,new_views=sum(q['id'].startswith(('campus_v16','v16')) for q in d['furnishing_captures']))
    (OUT/'inventory.json').write_text(json.dumps(stats,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(stats),flush=True)
    return d

if __name__=='__main__':
    OUT.mkdir(parents=True,exist_ok=True);base=OUT/'baseline_v15.json'
    if not base.exists():
        assert json.loads(PATH.read_text(encoding='utf8'))['review_revision']==15
        shutil.copy2(PATH,base)
    d=apply(json.loads(base.read_text(encoding='utf8')));PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf8');refresh_navigation();print('v16 map built',flush=True)
