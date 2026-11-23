"""A measured furnishing pass, protected guard stairs and corrected wayfinding."""
import collections,functools,json,math,random,shutil
from pathlib import Path
from hospital_campus_v14 import Campus
from hospital_clutter_v10 import bounds,rotate
from hospital_item_types_v12 import refresh_navigation
from hospital_realism_v16 import lettering
ROOT=Path(__file__).resolve().parents[1];PATH=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json';OUT=ROOT/'artifacts/unreal_hospital_v17';SOURCE='hospital_finish_v17'

def model(kind):
    p=[]
    def b(x,y,z,w,d,h,m='camp_steel'):p.append([x,y,z,w,d,h,m])
    def cabinet(w,d,h):
        for x in (-w/2+.06,w/2-.06):
            for y in (-d/2+.06,d/2-.06):b(x,y,.06,.08,.08,.12,'camp_rubber')
        b(0,0,h/2+.08,w,d,h-.12,'camp_cream');b(0,0,h+.03,w+.04,d+.04,.06)
        for zz in (.35,.7):
            if zz<h:b(0,d/2+.015,zz,w*.65,.03,.025,'camp_rubber')
    if kind=='nurse_station':
        cabinet(1.15,.62,.77);b(-.25,-.13,.97,.47,.05,.32,'camp_rubber');b(-.25,-.095,.97,.42,.015,.27,'camp_teal');b(-.25,-.13,.81,.12,.15,.10);b(-.2,.15,.82,.5,.16,.035,'camp_rubber')
        b(.35,.12,.87,.16,.16,.16,'camp_cream')
    elif kind=='treatment_trolley':
        for x in (-.37,.37):
            for y in (-.24,.24):b(x,y,.08,.065,.1,.16,'camp_rubber');b(x,y,.58,.035,.035,.95)
        for z in (.25,.9):b(0,0,z,.80,.54,.035);b(0,-.25,z+.035,.8,.02,.08)
        b(0,0,.94,.35,.27,.04,'camp_cream');b(-.25,.05,1.05,.08,.08,.16,'camp_teal')
    elif kind=='linen_hamper':
        for x in (-.25,.25):
            for y in (-.22,.22):b(x,y,.50,.03,.03,.96)
        b(0,0,.5,.48,.43,.72,'camp_cloth');b(0,0,.90,.55,.50,.055,'camp_cream')
        for x in (-.25,.25):b(x,0,.04,.055,.4,.08,'camp_rubber')
    elif kind=='archive_drawers':
        cabinet(.68,.48,1.65)
        for z in (.34,.68,1.02,1.36):b(0,.255,z,.58,.025,.015);b(0,.275,z+.1,.14,.015,.06,'camp_cream')
    elif kind=='clinical_cabinet':
        cabinet(.85,.42,1.8)
        for z in (1.0,1.35,1.7):
            b(0,.23,z,.75,.025,.025)
            for x in (-.25,0,.25):b(x,.24,z-.12,.13,.03,.18,'camp_teal')
        b(0,.24,1.4,.025,.025,.72)
    elif kind=='therapy_table':
        for x in (-.6,.6):
            for y in (-.33,.33):b(x,y,.36,.055,.055,.72,'camp_wood')
        b(0,0,.75,1.4,.82,.06,'camp_wood');b(0,0,.80,.42,.35,.035,'camp_sage')
    elif kind=='workbench':
        for x in (-.61,.61):
            for y in (-.25,.25):b(x,y,.43,.08,.08,.86)
        b(0,0,.9,1.4,.65,.08,'camp_wood');b(0,-.28,1.2,1.35,.045,.55,'camp_sage');b(.42,-.05,1.0,.25,.25,.12,'camp_rubber')
    elif kind=='kitchen_trolley':
        for x in (-.38,.38):
            for y in (-.24,.24):b(x,y,.45,.035,.035,.85);b(x,y,.07,.065,.09,.14,'camp_rubber')
        for z in (.22,.57,.92):b(0,0,z,.85,.55,.035)
        for x in (-.25,0,.25):b(x,0,.65,.18,.2,.12,'camp_cream')
    elif kind=='stock_cabinet':
        cabinet(1.05,.5,1.45)
        for x in (-.28,.28):b(x,.27,.85,.07,.03,.24,'camp_rubber')
    elif kind=='personal_tray':
        b(-.08,0,.025,.2,.23,.05,'camp_ochre');b(-.08,0,.054,.18,.21,.008,'camp_cream')
        b(.12,.02,.045,.07,.07,.09,'camp_cream');b(.164,.02,.045,.025,.02,.05,'camp_cream')
    elif kind=='clinical_tray':
        b(0,0,.012,.28,.22,.024);b(-.07,0,.04,.08,.16,.035,'camp_cream');b(.065,0,.07,.07,.07,.115,'camp_teal');b(.065,0,.135,.06,.06,.015,'camp_cream')
    elif kind=='office_tray':
        b(0,0,.012,.24,.27,.024,'camp_teal');b(0,0,.027,.21,.25,.008,'camp_cream');b(.07,.02,.035,.01,.15,.012,'camp_ink')
    elif kind=='workshop_tray':
        b(0,0,.018,.28,.20,.036,'camp_rubber');b(-.04,0,.048,.19,.025,.025);b(-.11,0,.05,.035,.08,.03);b(.07,.04,.05,.03,.10,.03,'camp_ochre')
    elif kind=='kitchen_tray':
        b(0,0,.013,.28,.22,.026);b(-.06,0,.035,.13,.15,.025,'camp_cream');b(.08,0,.07,.075,.075,.10,'camp_ochre')
    elif kind=='pillow':b(0,0,.05,.43,.28,.10,'camp_cream')
    elif kind=='blanket':b(0,0,.02,.68,.78,.04,'camp_cloth');b(0,.30,.044,.68,.12,.008,'camp_cream')
    else:raise ValueError(kind)
    low=min(a[2]-a[5]/2 for a in p)
    if kind not in ('pillow','blanket'):
        for a in p:a[2]-=low
    return p

def apply(d):
    c=Campus(d);c.layer=17;c.source=SOURCE;c.prefix='FIN17';rng=random.Random(1704);requests=[];surfaces=[];bed_items=[];kits=[]
    # Guard decks and every physical tread are legitimate save surfaces.
    d['raised_access']=[]
    for post in d['campus_plan']['sentry_platforms']:
        x,y,_=post['at'];z=post['floor_z'];d['raised_access'].append(dict(rect=[x-1.9,y-1.4,3.8,2.8],z=z,kind='guard_deck'))
        # Campus soil is 20cm below the street paving. A 15cm first tread
        # otherwise becomes a 35cm step, which the character cannot climb.
        c.emit('guard_ground_landing','ST',[[0,0,-.10,1.8,2.0,.20,'camp_concrete']],[x,-27.75,0],strict=False,partial=True,extra=dict(mount='guard_landing'))
        for j in range(40):
            yy=-46+.45*(39-j)+.225;h=.15*(j+1)
            d['raised_access'].append(dict(rect=[x-.6,yy-.225,1.2,.45],z=h,kind='guard_stair'))
            # Closely spaced balusters prevent a capsule slipping off the flight.
            parts=[]
            for side in (-1,1):
                parts.extend([[side*.67,0,h+.50,.04,.04,1.0,'camp_steel'],[side*.67,0,h+1.0,.07,.46,.08,'camp_steel']])
            c.emit('guard_stair_rail','ST',parts,[x,yy,0],solid=True,strict=False,partial=True,extra=dict(mount='guard_rail'))
        p=[[0,0,z+.55,3.95,.07,1.1,'camp_steel']]
        c.emit('guard_back_rail','ST',p,[x,y-1.47,0],strict=False,partial=True,extra=dict(mount='guard_rail'))
        for side in (-1,1):c.emit('guard_front_rail','ST',[[0,0,z+.55,1.28,.07,1.1,'camp_steel']],[x+side*1.33,y+1.47,0],strict=False,partial=True,extra=dict(mount='guard_rail'))
    # A different room function gets a different full-sized item. Do not force
    # an extra object into a room if its real footprint/access cannot fit.
    for zid,(s,r) in sorted(c.zones.items()):
        if not r['enclosed']:continue
        name=r['name'].lower();sid=s['id']
        if zid.startswith(('X01','X03','X04')) or sid.startswith(('X01','X03','X04')):kind='stock_cabinet'
        elif any(k in name for k in ('кухн','кафе','буфет','столов','посуд')) or sid.startswith('X05'):kind='kitchen_trolley'
        elif any(k in name for k in ('бель','прачеч','душ','сануз','убор')):kind='linen_hamper'
        elif sid.startswith('SV') or any(k in name for k in ('мастер','ремонт','вент','электр','технич')):kind='workbench'
        elif sid.startswith(('PS','RH')) and any(k in name for k in ('терап','сенсор','учеб','игр','лфк','трен')):kind='therapy_table'
        elif any(k in name for k in ('пост','регист','сестры','дежур','кабинет')):kind='nurse_station'
        elif any(k in name for k in ('лаборат','процед','осмотр','операц','реаним','аптек')) or sid.startswith('X02'):kind='clinical_cabinet'
        elif any(k in name for k in ('архив','хранен','склад','бухгал','диспет')):kind='archive_drawers'
        else:kind='treatment_trolley'
        item=c.fit(kind,zid,parts=model(kind));requests.append(dict(zone=zid,kind=kind,item=item['id'] if item else None))
        if item:
            kits.append(item)
            # The support is a real horizontal top face of the rendered kit.
            bs=[b for b in c.e['boxes'][-item['parts']:] if b['size'][0]>=.30 and b['size'][1]>=.24 and .4<=b['at'][2]+b['size'][2]/2-s['origin'][2]<=1.9]
            if bs:
                b=max(bs,key=lambda b:b['at'][2]+b['size'][2]/2);surfaces.append(dict(parent=item['id'],zone=zid,at=b['at'],size=b['size'],site=sid))
    print('Room additions',len(kits),'of',len(requests),flush=True)
    # Stock existing cabinets and tables too. Each composition rests on a
    # real, sufficiently broad box face; no unsupported hovering small items.
    eligible={i['id']:i for i in d['campus_upgrade']['items']+d['realism_upgrade']['items'] if i['kind'] in ('bedside_cabinet','chart_cart','mail_cart','beverage_station','guard_desk','therapy_shelf','market_crates','archive_drawers')}
    byparent=collections.defaultdict(list)
    for b in c.e['boxes']:
        if b.get('item_id') in eligible and b['size'][0]>=.30 and b['size'][1]>=.24:byparent[b['item_id']].append(b)
    for ident,bs in byparent.items():
        i=eligible[ident];base=i['at'][2];choices=[b for b in bs if .4<=b['at'][2]+b['size'][2]/2-base<=1.9]
        if choices:
            b=max(choices,key=lambda b:b['at'][2]+b['size'][2]/2);surfaces.append(dict(parent=ident,zone=i['zone'],at=b['at'],size=b['size'],site=c.zones[i['zone']][0]['id'] if i['zone'] in c.zones else 'ST'))
    stock=[]
    for q in surfaces:
        name=c.zones[q['zone']][1]['name'].lower() if q['zone'] in c.zones else '';sid=q['site']
        kind='workshop_tray' if sid.startswith('SV') else 'kitchen_tray' if sid.startswith(('HS','X01','X03','X05')) or any(k in name for k in ('кухн','буфет','столов')) else 'personal_tray' if 'палат' in name or sid.startswith(('PS','RH')) else 'office_tray' if any(k in name for k in ('архив','кабинет','пост','регист')) else 'clinical_tray'
        p=model(kind);width=max(a[0]+a[3]/2 for a in p)-min(a[0]-a[3]/2 for a in p);depth=max(a[1]+a[4]/2 for a in p)-min(a[1]-a[4]/2 for a in p)
        if width>q['size'][0]-.02 or depth>q['size'][1]-.02:continue
        for dx,dy in ((0,0),(-.18,0),(.18,0),(0,-.12),(0,.12)):
            if abs(dx)+width/2>q['size'][0]/2-.01 or abs(dy)+depth/2>q['size'][1]/2-.01:continue
            at=[q['at'][0]+dx,q['at'][1]+dy,q['at'][2]+q['size'][2]/2+.004]
            item=c.emit(kind,q['zone'],p,at,solid=False,extra=dict(mount='supported_detail',parent=q['parent'],support=dict(at=q['at'],size=q['size'])))
            if item:stock.append(item);break
    for prop in d['prototype_props']+d['environment_props']:
        if prop['model'] not in (1,2) or prop['mount']!='floor':continue
        for kind,ly in (('pillow',-.63),('blanket',.12)):
            off=rotate([0,ly,.854],prop['turn']);at=[prop['at'][i]+off[i] for i in range(3)]
            item=c.emit(kind,prop['zone'],model(kind),at,prop['turn'],False,extra=dict(mount='bed_linen',parent=prop['id']))
            if item:
                bed_items.append(item)
                if kind=='pillow':
                    for b in c.e['boxes'][-item['parts']:]:b['mesh']='sphere'
    # Compact, clear samples of each department and public building.
    seen=set()
    for item in kits:
        sid=c.zones[item['zone']][0]['id']
        if sid in seen:continue
        seen.add(sid);c.capture(item,'v17_room_'+sid)
    for j,x in enumerate((70,225)):
        d['furnishing_captures'].append(dict(id=f'v17_guard_{j}',at=[x+4,-39,3],target=[x,-46,6.2],envelope=True))
    d['furnishing_captures'].append(dict(id='v17_detail',at=[9.2,10,1.6],target=[10,8,1.0]))
    d['review_revision']=17
    d['finish_upgrade']=dict(version=17,items=c.items,requests=requests,room_kits=[i['id'] for i in kits],supported_details=[i['id'] for i in stock],bed_linen=[i['id'] for i in bed_items],guard_access_regions=len(d['raised_access']),limits=['Added full-sized room kits only when a clear real footprint and front access fit.','Small objects are decorative; story notes and pickups are unchanged.'])
    stats=dict(new_items=len(c.items),new_parts=sum(i['parts'] for i in c.items),room_kits=len(kits),supported_details=len(stock),bed_linen=len(bed_items),guard_rail_groups=sum(i['mount']=='guard_rail' for i in c.items),room_requests=len(requests),unfitted=sum(q['item'] is None for q in requests),new_views=sum('v17' in q['id'] for q in d['furnishing_captures']))
    (OUT/'inventory.json').write_text(json.dumps(stats,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(stats),flush=True)

if __name__=='__main__':
    OUT.mkdir(parents=True,exist_ok=True);base=OUT/'baseline_v16.json'
    if not base.exists():
        assert json.loads(PATH.read_text(encoding='utf8'))['review_revision']==16;shutil.copy2(PATH,base)
    d=json.loads(base.read_text(encoding='utf8'));apply(d);PATH.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf8');refresh_navigation();print('v17 map built',flush=True)
