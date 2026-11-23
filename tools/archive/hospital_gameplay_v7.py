"""Story v2 spatial prototype: real pickups, blank notes, encounter placements.

The source is a scenario outline, not a finished 210-hour quest script. Scene
markers deliberately identify unfinished acting/dialogue rather than claiming
to implement it. Coordinates are selected on the independently audited floor.
"""
import contextlib,hashlib,io,json,math,re,runpy
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v7';OUT.mkdir(exist_ok=True)

def build_gameplay(d,g):
    source=ROOT/'data/hospital_furnishing/story_v2_source.md';story=source.read_text(encoding='utf-8')
    sites={l['id']:l for l in d['locations']};zones={r['id']:(l,r) for l in d['locations'] for r in l['rooms']}
    mask=g['mask'];res=g['RES'];x0=g['X0'];y0=g['Y0'];zindex=g['ZINDEX']
    reserved=[];placements=[];mobs=[];scenes=[];notes=[];chapters=[];used={}
    def point(zone,large=False):
        l,r=zones[zone];ox,oy,z=l['origin'];x,y,w,h=r['rect'];x+=ox;y+=oy
        # Every marker and pickup is on reachable standing floor, never in a
        # wall, wardrobe, roof void, or outside its registered zone.
        candidates=[];li=zindex[round(z,2)]
        for yy in np.arange(y+.65,y+h-.6,.5):
            for xx in np.arange(x+.65,x+w-.6,.5):
                ix=int((xx-x0)/res);iy=int((yy-y0)/res)
                if not mask[li,iy,ix] or g['parent'][li*g['PLANE']+iy*g['W']+ix]<0:continue
                if large and not mask[li,iy-1:iy+2,ix-1:ix+2].all():continue
                if any(abs(z-q[2])<.1 and math.dist((xx,yy),q[:2])<(1.4 if large else .65) for q in reserved):continue
                # Different entries use different clear bays instead of a heap.
                score=(xx-(x+w*.55))**2+(yy-(y+h*.45))**2
                candidates.append((score,xx,yy,z))
        if not candidates:raise ValueError(('No reachable clear placement',zone,large))
        candidates.sort();_,xx,yy,z=candidates[min(used.get(zone,0)*3,len(candidates)-1)];used[zone]=used.get(zone,0)+1
        at=[round(xx,3),round(yy,3),z];reserved.append(at);return at
    def item(ident,zone,kind,label,chapter=1,amount=1,**kw):
        p=dict(id=ident,zone=zone,site=zones[zone][0]['id'],at=point(zone),kind=kind,label=label,chapter=chapter,amount=amount,**kw);placements.append(p);return p
    # Correct first act locations. The weapon is acquired at the security post,
    # not conjured in inventory; chapter one still has no open monster attacks.
    for ident,zone,kind,label in [
        ('visitor_pass','R05','key','Пропуск посетителя'),('staff_pass','N01','key','Пропуск мужчины'),
        ('first_map','R11','map','План больницы'),('first_pistol','F101','weapon','Служебный пистолет'),
        ('spare_fuse','R06','key','Предохранитель'),('first_shotgun','B114','shotgun','Помповый дробовик'),
        ('service_prybar','SV18','key','Монтажный инструмент'),('medical_clearance','E14','key','Медицинский допуск'),
        ('quarantine_code','S16','key','Код карантинного шлюза'),('evidence_case','E21','key','Носитель доказательств')]:
        item(ident,zone,kind,label,1 if ident!='first_shotgun' else 5)
    part=story.split('## Часть II.')[1].split('## Часть III.')[0]
    sections=re.split(r'^### Глава (\d+)\. (.+)$',part,flags=re.M)
    overrides={1:['S01','N01','R21','R11'],2:['R37','F202','F203','N05'],3:['F209','F211','F212','F216'],
        4:['S16','S06','S02','S08'],5:['B102','B107','B110','B120'],6:['E14','E15','E05','E07'],
        7:['R40','E17','E21','E17'],8:['F203','B112','R40','R45'],9:['E10','B120','SV24','E17'],
        10:['SV24','HS11','SV24','HS11'],11:['B102','B111','B113','R52'],12:['DG17','DG18','DG20','DG24'],
        13:['HS09','HS11','HS17','PS22'],14:['S08','S16','SV01','T04'],15:['E12','E04','E09','E02'],
        16:['IF11','E23','R45','B119'],17:['SV18','B115','E23','IF11'],18:['E11','E06','E24','S11'],
        19:['E09','E10','T04','S01','R11']}
    titles={c['id']:c['title'] for c in d['chapters']}
    for i in range(1,len(sections),3):
        ch=int(sections[i]);body=sections[i+2]
        headings=re.findall(r'^\*\*(\d+)\. (.+?)\*\*',body,flags=re.M)
        if ch==16:headings=[('1','Подготовка лечения.'),('2','Подготовка уничтожения.'),('3','Выход с доказательствами.'),('4','Узлы Проводника.')]
        if not headings:raise ValueError(('Missing chapter scene headings',ch))
        required=[]
        for index,(_,title) in enumerate(headings):
            zone=overrides[ch][index%len(overrides[ch])];sid=f'SCENE_{ch:02}_{index+1:02}'
            row=item(sid,zone,'scene',title.rstrip('.'),ch,scene_stub=True)
            scenes.append(dict(id=sid,chapter=ch,zone=zone,title=title.rstrip('.'),implementation='interactive_spatial_stub'))
            required.append(sid)
            note=item(f'NOTE_{ch:02}_{index+1:02}',zone,'note',f'Записка {ch:02}.{index+1:02}',ch,body='')
            notes.append(dict(id=note['id'],zone=zone,chapter=ch,title=note['label'],body=''))
        chapters.append(dict(id=ch,title=titles[ch],required=required,anchors=list(dict.fromkeys(overrides[ch])),
            progression='interact_scene_stubs_in_order',duration_claim=False))
    # First act props and the six optional side stories are physically present.
    for n in d['threat_nodes']:
        item(n['id'],n['zone'],'node','Отключить узел связи '+n['id'],n['chapter'])
    for j,(zone,label) in enumerate([('S01','Манекен за стеклом'),('N01','Охранник за стойкой'),('F209','Сестра у стекла'),
        ('F202','Раненый свидетель'),('B102','Выжившие в убежище'),('E17','Врач из изолятора'),('HS09','Сестра: личная комната')]):
        item('PERSON_'+str(j+1),zone,'person',label,1 if j<2 else 2 if j==3 else 3 if j==2 else 5)
    side=[('Человек с пропуском',['S03','R05','R40']),('Последняя дежурная',['PS08','PS11','PS22']),
          ('Палата без номера',['RH08','RH13','RH00']),('Письма домой',['HS09','HS17','B119']),
          ('Тот, кто стучит',['F202','DG08','DG24']),('Ночная смена',['SV08','B120','R14'])]
    for sn,(title,areas) in enumerate(side,1):
        for k,zone in enumerate(areas,1):
            item(f'SIDE_{sn}_{k}',zone,'note',title+f' · лист {k}',2,body='',side_story=title)
    # Ammo/healing supports exploration and the new combat loop, including all
    # expanded floors; no infinite respawn or unbounded item stacking.
    for l in d['locations']:
        rooms=[r for r in l['rooms'] if r.get('refuge') or r.get('production_class')=='B']
        if not rooms:rooms=l['rooms'][:1]
        for k,r in enumerate(rooms[:3]):
            item(f'SUPPLY_{l["id"]}_{k}_AMMO',r['id'],'ammo','Патроны',1,12)
            item(f'SUPPLY_{l["id"]}_{k}_MED',r['id'],'medkit','Аптечка',1,1)
            if k==0:item(f'SUPPLY_{l["id"]}_{k}_SHELLS',r['id'],'shells','Патроны дробовика',5,5)
    # Keep the source's kill classes. B opponents can be delayed before the
    # finale; C cannot be killed by ordinary weapon damage.
    for rule in d['monster_rules']:
        ch=rule['first_chapter'];anchors=next(c['anchors'] for c in d['chapters'] if c['id']==ch)
        allowed=[z for z in anchors if not zones[z][1].get('refuge')]
        amount=8 if rule['id']=='patients' else 4 if rule['id'] in ('linked','hound','carrier') else 2
        for j in range(amount):
            zone=allowed[(j+1)%len(allowed)];large=rule['size_class']=='large';at=point(zone,large)
            mobs.append(dict(id=f'MOB_{rule["id"]}_{j+1:02}',zone=zone,site=zones[zone][0]['id'],at=at,
                species=rule['id'],name=rule['name'],chapter=ch,combat_class=rule['combat_class'],kill_condition=rule['kill_condition'],
                radius=.55 if large else .30,health=180 if large else 70,speed=1.15 if large else 1.65,
                damage=24 if large else 12,behaviour='prototype_'+rule['id']))
        rule['implemented']=True;rule['implementation_scope']='prototype_actor_perception_combat_and_class_rules'
    # Compact row runs of the independent half-metre free-floor grid. Real
    # capsule collision remains authoritative; this is a path-planning graph.
    runs=[]
    for zi,z in enumerate(g['LEVELS']):
        for yi,row in enumerate(mask[zi]):
            changes=np.diff(np.r_[False,row,False].astype(np.int8));starts=np.flatnonzero(changes==1);ends=np.flatnonzero(changes==-1)
            runs.extend([[round(z*100),yi,int(a),int(b-a)] for a,b in zip(starts,ends)])
    game=dict(version=7,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),chapters=chapters,
        placements=placements,scenes=scenes,blank_notes=notes,mobs=mobs,side_stories=[dict(title=t,zones=a) for t,a in side],
        navigation=dict(resolution_m=res,x0=x0,y0=y0,rows=runs),
        limits=['Scene markers are playable spatial stubs, not completed dialogue/cinematics or a 210-hour campaign.',
                'Notes are intentionally blank. Mob models and advanced infrastructure powers are prototypes.'])
    d['review_revision']=7;d['story_gameplay']=game
    (OUT/'story_gameplay.json').write_text(json.dumps(game,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(dict(chapters=len(chapters),scene_stubs=len(scenes),blank_notes=sum(p['kind']=='note' for p in placements),
        placements=len(placements),mobs=len(mobs),species=len(d['monster_rules']),nav_runs=len(runs)),ensure_ascii=False))
    return game

if __name__=='__main__':
    with contextlib.redirect_stdout(io.StringIO()):g=runpy.run_path(str(ROOT/'tools/verify_story_layout.py'))
    d=g['D'];build_gameplay(d,g)
    (ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json').write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
