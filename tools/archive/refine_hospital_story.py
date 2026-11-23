"""Hospital v3: preserve the 333 zone IDs/areas, author a traversable story shell.

Geometry, story bindings and verification share this manifest. Story mappings are
production data, not a claim that the 19 chapters have been implemented.
"""
from pathlib import Path
import copy, csv, hashlib, json, math, shutil

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'unreal/EchoWard/Content/CampaignData'
OUT = ROOT / 'artifacts/unreal_hospital_v3'
OUT.mkdir(parents=True, exist_ok=True)
BASE = OUT / 'hospital_v2_baseline.json'
if not BASE.exists():
    shutil.copy2(DATA / 'hospital_world.json', BASE)
world = json.loads(BASE.read_text(encoding='utf-8'))
assert world['map_id'] == 'hospital_campus_v2'
original = {r['id']: copy.deepcopy(r) for l in world['locations'] for r in l['rooms']}
locs = {l['id']: l for l in world['locations']}
rooms = {r['id']: r for l in world['locations'] for r in l['rooms']}
world.update(schema_version=3, map_id='hospital_story_shell_v3', hospital_number=333,
             ward_number='04', start=[-4,-52,.98], story_chapter=0,
             story_status='architecture_and_bindings_only')
world['passages'] = [[48,100,4,230,0], [44,-2,8,2,0]]
for y in (38,158,268):
    world['passages'] += [[-46,y,94,6,0], [52,y,98,6,0]]

def place(ident, rect, side='south'):
    r=rooms[ident]; r['rect']=rect; r['door_side']=side
    x,y,w,h=rect
    r['door']=[x+w//2, y+h if side=='south' else y]

# No intersecting quarantine gate/warehouse; an actual near-entrance arrival.
place('S01',[8,110,64,34]); place('S05',[8,8,30,30]); place('S15',[44,8,28,28])
# X=144 is a WORLD coordinate in the supplied metric atlas; ST starts at X=-70.
place('S06',[214,60,24,20]); place('S02',[130,166,28,22]); place('S16',[160,142,24,20])
place('S07',[170,166,24,20]);place('S11',[202,142,22,20])
locs['ST']['bounds']=[0,0,240,190]
locs['ST']['corridors']=[[0,0,240,190]]
locs['ST']['capture']=[118,152,.98,-5,90]
rooms['S01']['name']='Парковка посетителей / прибытие'
rooms['S16']['name']='Пост карантина у приёмной'
rooms['R07']['name']='Палата 04'
place('N02',[44,-26,12,12],'north')
place('N01',[36,-14,24,12],'north')
rooms['N01']['extra_doors']=[{'at':[48,-2], 'side':'south','type':'shutter','role':'main_entrance'}]
rooms['N02']['extra_doors']=[{'at':[48,-14], 'side':'south','type':'manual','role':'vestibule'}]
locs['1F']['bounds']=[-8,-26,112,126]
rooms['N12']['name']='Холл доступа к грузовому лифту'
rooms['T12']['name']='Машинная грузового лифта'
place('T12',[42,0,22,18]);place('T13',[40,82,24,18])
rooms['N12']['shaft_ref']=rooms['T12']['shaft_ref']='BED_LIFT'
world['lift']={'shaft':[48,4,4,6], 'stops':[-9.6,-4.8,0,4.8,9.6,14.4],
               'lobby_zone':'N12','machine_zone':'T12','shared_xy':True}

# An old two-storey pavilion. IDs DG13-24 move upstairs, retaining their areas.
dg2=copy.deepcopy(locs['DG']); dg2.update(id='DG2',name='Старый корпус / верхний этаж',
    name_en='Old pavilion / upper floor',name_kk='Ескі корпус / жоғарғы қабат',origin=[-150,0,4.8],style='old_ward')
dg2['rooms']=locs['DG']['rooms'][12:]; locs['DG']['rooms']=locs['DG']['rooms'][:12]
locs['DG']['name']='Старый диагностический корпус'
world['locations'].append(dg2); locs['DG2']=dg2

# One small unregistered patient block, discoverable physically in inspection.
hidden=dict(id='RH00',name='Палата без номера',rect=[24,82,12,12],minimum_area_m2=0,
            enclosed=True,door_side='south',door=[30,94],hidden=True,
            extra_doors=[{'at':[36,92.6],'side':'east','type':'bolt','role':'secret_exit'}])
locs['RH']['rooms'].append(hidden); rooms['RH00']=hidden
locs['RH']['corridors'] += [[24,94,16,4],[36,82,4,16],[36,78,12,4]]

# Basements of all six pavilions share service tunnels at B1. All connections
# are floors/stairs in the same world, without teleporters or loading portals.
ut=dict(id='UT',name='Подземные переходы и подвалы корпусов',name_en='Service tunnels',
        name_kk='Жерасты өткелдері',origin=[0,0,-4.8],style='tunnel',
        bounds=[-158,36,412,294],rooms=[],cores=[],corridors=[[48,36,4,294]],
        capture=[50,122,.98,-4,90])
ut['corridors'] += [[40,214,4,24],[56,214,4,24],[40,214,20,4],[40,234,20,4]]
world['isolation_airlock']={'at':[50,226,-4.8],'door_type':'shutter','side_routes':[40,58],
    'maintenance_hatch':[50,224,-4.8],'chapters':[9,10,19]}
world['locations'].append(ut); locs['UT']=ut
world['stairs']=[dict(id=a,rect=r,z_min=-9.6,z_max=14.4) for a,r in
    [('A',[-8,0,3,8]),('B',[44,88,3,8]),('C',[-8,86,3,8])]]
# A was blocking the public entrance beside the lift. Move its entire stack to
# the west edge, with an asymmetrical evacuation gallery on all six levels.
for key in ('1F','B1','B2','2F','3F'):
    for core in locs[key]['cores']:
        if core['name']=='STAIR A':core['rect']=[-8,0,3,8]
    locs[key]['corridors'].append([-8,0,8,100])
locs['1F']['corridors'].append([-8,-2,60,2])
for ident in ('DG','IF','PS','RH','HS','SV'):
    l=locs[ident]; ox,oy,_=l['origin']
    l['corridors'] += [[-8,86,8,14],[0,96,52,4]]
    l['cores']=[dict(name='STAIR '+ident,rect=[-8,86,3,8])]
    world['stairs'].append(dict(id=ident,rect=[ox-8,oy+86,3,8],z_min=-4.8,z_max=4.8 if ident=='DG' else 0))
    ut['corridors'] += [[ox-8,oy+82,60,18]]
    ut['cores'].append(dict(name='BASEMENT '+ident,rect=[ox-8,oy+86,3,8]))
    # A continuous branch from basement C to the central service spine.
    a,b=sorted((ox-8,50)); ut['corridors'].append([a,oy+96,b-a+4,4])
dg2['corridors'] += [[-8,86,8,14],[0,96,52,4]]
dg2['cores']=[dict(name='STAIR DG',rect=[-8,86,3,8])]
world['infrastructure_zones']=[dict(id='U_'+k,name='Подвал '+k,
    rect=[locs[k]['origin'][0]-8,locs[k]['origin'][1]+82,60,18],z=-4.8)
    for k in ('DG','IF','PS','RH','HS','SV')]

# Explicit chapter anchors. The story remains a blueprint; chapter 0 is free
# architectural inspection, so class C never makes the whole map inaccessible.
anchors={
 1:['S01','S03','N02','N01','R05','F101','R11','R21'],
 2:['F201','F202','F203','F204','F205','F206','F207','F208','R37','R38','N05'],
 3:['F209','F210','F211','F212','F213','F214','F215','F216','R22'],
 4:['N01','R11','S02','S08','S06','S16'],
 5:['R14','B106','B107','B108','B109','B110','B102','B120','N12'],
 6:['E14','E15','E05','E07','E01'],7:['E17','E21','R40'],
 8:['R40','B112','F203','B114','R45'],9:['E10','SV24','N15','B120','E17'],
 10:['E10','SV24','HS11'],11:['B101','B102','B106','B107','B108','B109','B110','B111','B112','B113','N11','R52','B116'],
 12:['DG01','DG08','DG13','DG17','DG18','DG20','DG24'],
 13:['HS09','HS11','HS12','HS17','PS22'],
 14:['S08','S09','S02','S06','S12','SV01','SV14','T04'],
 15:['E12','E04','E09','E02'],16:['IF11','F214','R54','B115','E23','SV18','S04','DG18','R45','B119'],
 17:['SV18','B115','E23','R45','B119','IF11'],
 18:['E11','E06','E24','R54','B115','R06','S11'],19:['E09','E10','T04','S06','N01','R11','S01']}
titles=['Если услышишь мой голос','Отделение не спит','Хирургия','Почти снаружи',
 'Грузовой лифт','Нижняя больница','После аварии','Протокол возврата','Технический выход',
 'Не открывай ей','Убежище','Старый корпус','Та, что осталась','Эвакуация','Первый пациент',
 'Цена лечения','Закрытие комплекса','ЭХО','Последнее посещение']
threats=['indirect','patients','stitched','hound','listener','carrier','keeper','repeater',
 'transporter','impostor','nest','blind_surgeon','linked','armoured','collector','conductor',
 'pursuer','central_organism','branch_finale']
world['chapters']=[dict(id=c,title=titles[c-1],anchors=ids,new_threat=threats[c-1],
    implementation='spatial_bindings',branch='choice_dependent' if c>=16 else 'main') for c,ids in anchors.items()]
chapter_by_zone={ident:[c for c,ids in anchors.items() if ident in ids] for ident in rooms}
side_stories={'HS':'Человек с пропуском','PS':'Последняя дежурная','RH':'Палата без номера',
              'IF':'Тот, кто стучит','SV':'Ночная смена','DG':'Палата без номера','DG2':'Палата без номера'}
profile={
 '1F':('reception',[.70,.79,.75],[.15,.31,.28],[.72,.86,.87],'ventilation_reception','registration'),
 '2F':('ward',[.67,.72,.64],[.26,.36,.28],[.82,.88,.75],'ward_calls','ward'),
 '3F':('archive',[.70,.66,.56],[.28,.23,.18],[.92,.79,.62],'relay_archive','office'),
 'B1':('service',[.48,.50,.47],[.22,.25,.23],[.72,.76,.68],'pipes_pumps','utility'),
 'B2':('lab',[.56,.63,.66],[.15,.24,.30],[.50,.77,.86],'sealed_ventilation','laboratory'),
 'DG':('old_diagnostics',[.55,.49,.40],[.27,.20,.16],[.95,.73,.45],'old_pipes','diagnostic'),
 'DG2':('old_ward',[.49,.45,.37],[.28,.23,.17],[.91,.70,.43],'floor_vibration','old_ward'),
 'IF':('isolation',[.65,.73,.69],[.19,.34,.30],[.57,.86,.75],'negative_pressure','isolation'),
 'PS':('courtyard',[.67,.69,.59],[.29,.35,.25],[.80,.83,.65],'wind_inner_court','therapy'),
 'RH':('rehabilitation',[.63,.70,.71],[.19,.33,.38],[.66,.83,.91],'drain_water','rehab'),
 'HS':('residential',[.71,.62,.50],[.32,.24,.20],[.95,.77,.57],'radio_heating','residential'),
 'SV':('industrial',[.49,.53,.52],[.25,.28,.29],[.79,.84,.83],'transformer','industrial'),
 'UT':('tunnel',[.40,.44,.42],[.19,.24,.22],[.65,.76,.72],'pipe_resonance','service')}
for ident,l in locs.items():
    p=profile.get(ident,('outdoor',[.5]*3,[.2]*3,[.7]*3,'wind','exterior'))
    l['identity']=dict(material=p[0],wall_tint=p[1],lower_tint=p[2],light_tint=p[3],
        soundscape=p[4],furniture_template=p[5],landmark='')

refuges={'1F':'R12','2F':'N05','3F':'F308','B1':'B102','B2':'E16','DG':'DG08','DG2':'DG20',
 'IF':'IF08','PS':'PS22','RH':'RH08','HS':'HS08','SV':'SV23','RF':'T08','ST':'X05'}
landmarks={'1F':'N01','2F':'F203','3F':'R39','B1':'R54','B2':'E09','DG':'DG02','DG2':'DG18',
 'IF':'IF11','PS':'PS20','RH':'RH09','HS':'HS11','SV':'SV01','RF':'T04','ST':'S03'}
# Exactly 80 A + 120 B + 133 C across the preserved 333 IDs, deterministic by
# narrative relevance, with one A refuge per building and all chapters covered.
priority=list(dict.fromkeys(list(refuges.values())+[x for ids in anchors.values() for x in ids]))
priority += sorted(set(original)-set(priority))
a=set(priority[:80]); b=set(priority[80:200])
for ident,l in locs.items():
    l['refuge']=refuges.get(ident,'U_'+ident)
    l['identity']['landmark']=landmarks.get(ident,'SERVICE SPINE')
    for r in l['rooms']:
        rid=r['id']; r['production_class']='A' if rid in a or rid=='RH00' else 'B' if rid in b else 'C'
        r['chapters']=chapter_by_zone.get(rid,[]) or ({'ST':[1,4,14,19],'RF':[14,19],
            'DG':[12],'DG2':[12],'IF':[16],'PS':[2,13],'RH':[11,16],
            'HS':[10,13],'SV':[9,14,17],'B1':[5,11],'B2':[6,15,18],
            '1F':[1,4,19],'2F':[2,3],'3F':[8,16]}[ident])
        r['side_story']=side_stories.get(ident,'Письма домой' if rid in ('B114','R40','B119','R45') else '')
        r['refuge']=rid==refuges.get(ident)
        r['furniture_template']=l['identity']['furniture_template']
        r['states']=[dict(chapter=0,state='open'),dict(chapter=1,state='closed' if r['production_class']=='C' else 'open')]
        if r['production_class']=='C' and chapter_by_zone.get(rid):
            r['states'].append(dict(chapter=min(chapter_by_zone[rid]),state='open'))
        if rid in ('E17','B114','HS12'):r['states'].append(dict(chapter=17,state='sealed'))
        if rid in ('DG23','SV03','E18'):r['states'].append(dict(chapter=17,state='destroyed'))
        r['wall_type']='reinforced' if ident=='B2' else 'glass' if rid in ('F101','F209','F210','N01') else 'main'
        r['door_type']='bolt' if r['refuge'] else 'biometric' if ident=='B2' else 'grille' if rid=='N12' else 'shutter' if rid=='N01' else 'manual' if ident in ('B1','SV') else 'ordinary'

# Author actual architectural partitions in large zones, retaining each zone's
# external footprint. A 2.8m front aisle and 1.6m central aisle serve 10-30m²
# bays; large clinical halls and heavy plant remain appropriately large.
world['partitions']=[]; world['subrooms']=[]; world['windows']=[]; world['route_checks']=[]
def part(site,a,b,kind='partition',height=3.8):
    l=locs[site]; ox,oy,z=l['origin'];
    world['partitions'].append(dict(site=site,a=[ox+a[0],oy+a[1],z],b=[ox+b[0],oy+b[1],z],
        height=height,thickness=.12,type=kind))
def splitline(site,a,b,centres,width=2,kind='partition',height=3.8):
    horizontal=a[1]==b[1]; start=a[0] if horizontal else a[1]; end=b[0] if horizontal else b[1]
    for value in sorted(set(centres)):
        lo=max(start,value-width/2); hi=min(end,value+width/2)
        if lo>start:part(site,(start,a[1]) if horizontal else (a[0],start),(lo,b[1]) if horizontal else (b[0],lo),kind,height)
        start=max(start,hi)
    if end>start:part(site,(start,a[1]) if horizontal else (a[0],start),b,kind,height)

skip_words=('зал','операцион','бассейн','гараж','генератор','дизель','инкубатор','крио','ядро','КТ-','МРТ-','шахта','лифт','вестибюль','тамбур')
for ident,l in locs.items():
    if ident in ('ST','RF','UT'):continue
    for r in l['rooms']:
        x,y,w,h=r['rect']; side=r['door_side']; north=side=='south'
        if not any(k.lower() in r['name'].lower() for k in skip_words) and r['enclosed'] and w>=7 and h>=10:
            # Two columns of bays around a central aisle running away from entry.
            cx=r['door'][0]; aisle={'IF':2.6,'PS':2.4,'HS':2.2,'DG2':2.4}.get(ident,2.2)
            front_depth={'IF':3.4,'PS':3.2,'HS':2.8,'DG2':3.6,'SV':3.4,'RH':3.2}.get(ident,2.8)
            front=y+h-front_depth if north else y+front_depth
            by0,by1=(y+.12,front) if north else (front,y+h-.12)
            bay_depth={'HS':4.5,'IF':5.4,'PS':6.2,'DG2':6.4,'SV':7}.get(ident,5.8)
            rows=max(1,math.ceil((by1-by0)/bay_depth)); depth=(by1-by0)/rows
            for s,(x0,x1) in enumerate(((x+.12,cx-aisle/2),(cx+aisle/2,x+w-.12))):
                if x1-x0<2.25:continue
                # Every individual bay has a 1.2m doorway onto the centre aisle.
                centres=[]
                for j in range(rows):
                    yy=by0+j*depth; centres.append(yy+depth/2)
                    world['subrooms'].append(dict(id=f"{r['id']}.{s+1}{j+1}",zone=r['id'],site=ident,
                        rect=[x0,yy,x1-x0,depth],name='Палата / кабинет',area_m2=round((x1-x0)*depth,2),
                        clear_width_m=round(x1-x0-.02,4),clear_depth_m=round(depth-(.02 if (north and j==0) or (not north and j==rows-1) else .12),4)))
                    if j:part(ident,(x0,yy),(x1,yy),'breakable' if ident=='2F' else 'partition')
                edge=x1 if s==0 else x0
                splitline(ident,(edge,by0),(edge,by1),centres,1.4,'partition')
                part(ident,(x0,front),(x1,front),'partition')
        if r['production_class']=='C' or r['wall_type']=='glass' or r['refuge']:
            world['windows'].append(dict(zone=r['id'],side=r['door_side'],at=r['door'],width=1.0,
                height=1.0,sill=1.1,offset=2.5,site=ident,transparent=True))
        world['windows'].append(dict(zone=r['id'],side='north' if north else 'south',
            at=[x+w//2,y if north else y+h],width=2.0,height=1.0,sill=1.1,offset=0,
            site=ident,transparent=True,role='external_view'))
    # Narrow the existing floor strips with partial walls. Door recesses retain
    # approach space, so no original doorway is covered by the new partitions.
    for cy,depth in ((16,6),(36,10),(62,6)):
        centre=cy+depth/2; clear=4 if cy==36 else 3.5
        for yy in (centre-clear/2,centre+clear/2):
            gaps=[r['door'][0] for r in l['rooms'] if abs(r['door'][1]-(cy if yy<centre else cy+depth))<.1]
            gaps += [1.5,48,102.5]
            # Distinct landmarks change sightlines without blocking the centre.
            splitline(ident,(0,yy),(104,yy),gaps,2.4,'niche',3.8)
    for a0,a1 in ((23,34),(47,60),(70,84)):
        for xx in (47,51):part(ident,(xx,a0),(xx,a1),'niche',3.8)

# Four door pairs per pavilion/floor create local loops through adjacent rooms.
# Extra door geometry is carved from the actual shared wall, not only metadata.
world['shortcuts']=[]
for ident in ('ST','RF'):
    for r in locs[ident]['rooms']:
        if not r['enclosed']:continue
        x,y,w,h=r['rect']
        world['windows'].append(dict(zone=r['id'],side=r['door_side'],at=r['door'],width=1.0,
            height=1.0,sill=1.1,offset=2.5,site=ident,transparent=True))
for ident,l in locs.items():
    if ident in ('ST','RF','UT'):continue
    pairs=[]
    for i,r in enumerate(l['rooms']):
        x,y,w,h=r['rect']
        if r.get('hidden'):continue
        for other in l['rooms'][i+1:]:
            bx,by,bw,bh=other['rect']
            if abs(x+w-bx)<.01 and abs(y-by)<.01 and abs(h-bh)<.01:
                # Front aisle is kept free, so the shortcut lands in circulation.
                yy=y+h-1.4 if r['door_side']=='south' else y+1.4
                pairs.append((r,other,[bx,yy]))
    # One connection per wing, instead of all links clustering in one row.
    selected=[]; seen=set()
    for r,other,point in pairs:
        key=(r['rect'][1],r['door_side'])
        if key in seen:continue
        seen.add(key); selected.append((r,other,point))
    for r,other,point in selected[:4]:
        r.setdefault('extra_doors',[]).append(dict(at=point,side='east',type='bolt',role='shortcut'))
        other.setdefault('extra_doors',[]).append(dict(at=point,side='west',type='bolt',role='shortcut'))
        world['shortcuts'].append(dict(site=ident,a=r['id'],b=other['id'],at=point,kind='bolt'))

# Physical security/utility endpoints and their independent network segments.
power=[('R54','B115'),('B115','R06'),('R06','S11'),('S11','SV18'),('SV18','E23'),('E23','E09')]
world['power_graph']=[dict(source=a,target=b,circuit='main_'+str(i),switchable=True) for i,(a,b) in enumerate(power)]
world['acoustic_graph']=[dict(source=r['id'],target=l['id']+'_vent',kind='ventilation',
    attenuation=.35,open=True) for l in world['locations'] for r in l['rooms'] if r['enclosed']]
world['acoustic_graph'] += [dict(source=k+'_vent',target='UT_pipes',kind='riser',attenuation=.55,open=True)
    for k in ('B1','B2','DG','IF','PS','RH','HS','SV')]
world['network_devices']=[]
for site,l in locs.items():
    for r in l['rooms']:
        if r['refuge'] or r['id'] in {'F101','F203','R40','DG18','B114','E06','HS11','E24','R21'}:
            for kind in ('phone','camera','power_switch'):
                world['network_devices'].append(dict(id=kind+'_'+r['id'],zone=r['id'],site=site,type=kind,
                    segment=site+'_local',upstream='E06' if kind=='camera' else 'R40',
                    sound_bus=site+'_pipes' if site in ('B1','B2','UT') else site+'_vent'))
nodes=['HS09','HS11','IF11','F214','R54','B115','E23','SV18','S04','DG18','R45','B119']
world['threat_nodes']=[dict(id=f'LINK_{i+1:02}',zone=z,chapter=13 if i<2 else 16,
    roles=['linked','conductor','pursuer'],network=next(l['id'] for l in world['locations'] if any(r['id']==z for r in l['rooms'])),
    active_from=13 if i<2 else 16,disable_methods=['power','manual','damage']) for i,z in enumerate(nodes)]
world['monster_rules']=[dict(id=t,first_chapter=i+1,implemented=False,
    infrastructure={'listener':'pipe_resonance','repeater':'phones_and_vents','keeper':'camera_segments',
    'linked':'LINK_nodes','conductor':'LINK_nodes','pursuer':'LINK_nodes'}.get(t,'chapter_geometry')) for i,t in enumerate(threats) if i]
world['story_routes']=[dict(chapter=c,anchors=ids,mode='physical_walk') for c,ids in anchors.items()]
world['helipad_role']='optional_evacuation_ch14_19_requires_external_clearance'
world['visibility_links']=[dict(chapter=4,source_zone='R11',target_zone='S16',
    window=[72.5,0,1.65],target=[102,-28,1.65],purpose='quarantine_post_visible_from_reception')]
world['boundary']={'rect':[-180,-190,460,570],'south_gate':[32,30],
    'south_fence':True,'note':'30m clear opening at X32..62; continuous fence elsewhere'}

# Collision footprints of the existing hard-coded architectural details. They
# let the independent route planner avoid actual lamps, columns and roof plant.
world['collision_features']=[]
def solid(code,rect,z):world['collision_features'].append(dict(id=code,rect=rect,z=z))
for i in range(10):solid('lamp_'+str(i),[19.9,-180+i*16-.1,.2,.2],0)
for x in (40,56):solid('entrance_column_'+str(x),[x-.175,-11-.175,.35,.35],0)
for x in (61,87):solid('ambulance_column_'+str(x),[x-.12,-25-.12,.24,.24],0)
for l in world['locations']:
    ox,oy,z=l['origin']
    if l['id'] not in ('ST','RF','UT'):
        for x in (0,104):
            for y in (36.5,45.5):solid(l['id']+f'_portal_{x}_{y}',[ox+x-.21,oy+y-.21,.42,.42],z)
        for x in (0,104):
            if x==0 and l['id'] in ('1F','B1','B2','2F','3F'):continue
            for y in (19,65):solid(l['id']+f'_end_window_{x}_{y}',[ox+x-.12,oy+y-3,.24,6],z)
    for r in l['rooms']:
        x,y,w,h=r['rect'];cx=ox+x+w/2;cy=oy+y+h/2
        if l['id']=='RF' and not r['enclosed'] and r['id'] not in ('T04','T01','T13'):
            solid('plant_'+r['id'],[cx-2.15,cy-3.25,4.3,6.5],14.4)
            solid('duct_'+r['id'],[cx+2.15,cy-.45,2.7,.9],14.4)
for z in (-4.8,14.4):solid('electrical_'+str(z),[44.1,29.35,.70,1.30],z)
for z in (-9.6,-4.8,0,4.8,9.6,14.4):
    for x in (48,52):solid('lift_wall_'+str(x)+str(z),[x-.08,4,.16,6],z)
    solid('lift_back_'+str(z),[48,3.92,4,.16],z)
    if z!=0:solid('lift_closed_'+str(z),[48.52,9.94,2.96,.12],z)
for stair in world['stairs']:
    x,y,_,_=stair['rect'];z=stair['z_min']
    while z<stair['z_max']-.01:
        for xx in (x,x+3):solid(stair['id']+'_rail_'+str(xx)+str(z),[xx-.04,y,.08,6.8],z)
        z=round(z+4.8,2)
for i,r in enumerate([[-8.11,0,.22,100],[103.89,0,.22,100],[-8,-.11,112,.22],[-8,99.89,112,.22]]):solid('parapet_'+str(i),r,14.4)
for i in range(12):
    angle=i*math.tau/12;solid('padlight_'+str(i),[46+16.5*math.cos(angle)-.15,48+16.5*math.sin(angle)-.15,.3,.3],14.4)
for n in world['threat_nodes']:
    rr=rooms[n['zone']];l=next(l for l in world['locations'] if any(r['id']==n['zone'] for r in l['rooms']))
    ox,oy,z=l['origin'];x,y=rr['door'];solid(n['id'],[ox+x+2.4-.18,oy+y+(-.4 if rr['door_side']=='south' else .4)-.15,.36,.30],z)

# The review extends the existing shell, rather than replacing its metric grid.
from hospital_review_v4 import apply_review
apply_review(world)
from hospital_expansion_v5 import apply_expansion
apply_expansion(world)
allrooms=[r for l in world['locations'] for r in l['rooms']]
assert len(allrooms)==len({r['id'] for r in allrooms})
assert set(original)|{'RH00'} <= {r['id'] for r in allrooms}
for r in allrooms:
    if r['id'] in original:
        assert r['rect'][2]*r['rect'][3]>=original[r['id']]['rect'][2]*original[r['id']]['rect'][3],r['id']
for l in world['locations']:
    for i,a0 in enumerate(l['rooms']):
        x,y,w,h=a0['rect']
        for b0 in l['rooms'][i+1:]:
            bx,by,bw,bh=b0['rect']
            assert min(x+w,bx+bw)<=max(x,bx) or min(y+h,by+bh)<=max(y,by),(a0['id'],b0['id'])
for c,ids in anchors.items():assert set(ids)<=set(rooms),(c,set(ids)-set(rooms))
assert len(a)==80 and len(b)==120
(DATA/'hospital_world.json').write_text(json.dumps(world,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
source_story=Path('C:/Users/YOUR_WINDOWS_USER/Downloads/EHO_story_v2.md')
if source_story.exists():shutil.copy2(source_story,OUT/'EHO_story_v2_source.md')
with (OUT/'zone_story_classes.csv').open('w',encoding='utf-8-sig',newline='') as f:
    writer=csv.writer(f,delimiter=';');writer.writerow(['ID','Корпус','Название','Класс','Главы','История','Убежище','Тип двери','Тип стены','Площадь м2'])
    for l in world['locations']:
        for r in l['rooms']:writer.writerow([r['id'],l['id'],r['name'],r['production_class'],','.join(map(str,r['chapters'])),r['side_story'],r['refuge'],r['door_type'],r['wall_type'],r['rect'][2]*r['rect'][3]])
(OUT/'authoring_audit.json').write_text(json.dumps(dict(rooms=len(allrooms),preserved_ids=333,hidden=1,
    classes_original={c:sum(r['id'] in original and r['production_class']==c for r in allrooms) for c in ('A','B','C')},sites=len(world['locations']),
    subrooms=len(world['subrooms']),partitions=len(world['partitions']),shortcuts=len(world['shortcuts']),
    stairs=len(world['stairs']),power_edges=len(power),network_devices=len(world['network_devices']),threat_nodes=len(nodes),
    source_story_sha256=hashlib.sha256(source_story.read_bytes()).hexdigest() if source_story.exists() else None),ensure_ascii=False,indent=2),encoding='utf-8')
print((OUT/'authoring_audit.json').read_text(encoding='utf-8'))
