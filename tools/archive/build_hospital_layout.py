"""Author a hospital blockout; retain every atlas ID and its minimum usable area."""
import copy, json, math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'unreal/EchoWard/Content/CampaignData'
source = json.loads((DATA / 'book_map.json').read_text(encoding='utf-8'))
world = copy.deepcopy(source)
world.update(schema_version=2, map_id='hospital_campus_v2', presentation_only=False)
origins = {'1F':[0,0,0], 'B1':[0,0,-9.6/2], 'B2':[0,0,-9.6],
           '2F':[0,0,4.8], '3F':[0,0,9.6], 'RF':[0,0,14.4], 'ST':[-70,-190,0],
           'DG':[-150,0,0], 'IF':[150,0,0], 'PS':[-150,120,0],
           'RH':[150,120,0], 'HS':[-150,230,0], 'SV':[150,230,0]}
styles = {'1F':'reception','2F':'wards','3F':'administration','B1':'service',
          'B2':'laboratory','DG':'imaging','IF':'isolation','PS':'courtyard',
          'RH':'rehabilitation','HS':'residential','SV':'industrial','ST':'campus','RF':'roof'}
for location in world['locations']:
    ident = location['id']; location['origin'] = origins[ident]; location['style'] = styles[ident]
    location['corridors']=[]; location['cores']=[]
    for r in location['rooms']:
        r['minimum_area_m2'] = r['rect'][2]*r['rect'][3]
        r['enclosed'] = ident not in ('ST','RF')
    if ident not in ('ST','RF'):
        # Two department streets, a wide public cross-hall and a separate vertical core.
        count=len(location['rooms'])//8
        depths={'1F':[16,12,16,14],'2F':[12,14,14,16],'3F':[10,12,16,12],
                'B1':[16,14,12,16],'B2':[16,16,16,16], 'DG':[16,12,16,12],
                'IF':[12,16,12,16],'PS':[12,10,14,10], 'RH':[16,12,16,14],
                'HS':[10,12,10,12],'SV':[16,16,16,16]}[ident]
        for row in range(4):
            for side in range(2):
                batch=location['rooms'][(row*2+side)*count:(row*2+side+1)*count]
                widths=[max(7, math.ceil(r['rect'][2]*1.22)) for r in batch]
                if ident in ('DG','RH','SV','B2'): widths=[w+2 for w in widths]
                x=44-sum(widths) if side==0 else 52
                depth=depths[row]; y=[16-depth,22,62-depth,68][row]
                for r,w in zip(batch,widths):
                    r['rect']=[x,y,w,depth]; r['door_side']='south' if row%2==0 else 'north'
                    r['door']=[x+w//2, y+depth if row%2==0 else y]
                    x+=w
        location['bounds']=[-8,0,112,100]
        location['corridors']=[[44,0,8,100],[0,16,104,6],[0,36,104,10],[0,62,104,6]]
        if ident in ('1F','B1','B2','2F','3F'):
            location['cores']=[{'name':'STAIR A','rect':[44,0,3,8]},
                               {'name':'STAIR B','rect':[44,88,3,8]},
                               {'name':'STAIR C','rect':[-8,86,3,8]},
                               {'name':'BED LIFT','rect':[48,4,4,6]}]
            location['corridors'] += [[-8,86,8,14],[0,96,52,4]]
        location['capture']=[48,30,0.98,-8,90]
    elif ident=='RF':
        location['bounds']=[-8,0,112,100]
        slots=[(x,0,20,18) for x in (0,20,64,84)]+[(x,82,20,18) for x in (0,20,64,84)]
        slots += [(0,y,18,20) for y in (20,40,60)]+[(86,y,18,20) for y in (20,40,60)]+[(64,20,20,18)]
        for r in location['rooms']:
            if r['id']=='T04': r.update(rect=[28,30,36,36],name='Вертолётная площадка',feature='helipad')
            else:
                r['rect']=list(slots.pop(0)); r['feature']='plant'
                if r['id'] in ('T07','T08','T10','T12','T16'): r.update(enclosed=True,feature='roof_store')
            north=r['rect'][1]==82
            r['door_side']='north' if north else 'south'
            r['door']=[r['rect'][0]+r['rect'][2]//2,r['rect'][1] if north else r['rect'][1]+r['rect'][3]]
        location['corridors']=[[-8,0,112,100]]
        location['capture']=[46,38,0.98,-18,90]
    else:
        location['bounds']=[0,0,240,170]
        rects=[[8,8,64,34],[80,110,28,22],[102,8,24,20],[180,110,32,24],
               [8,110,30,30],[210,60,24,20],[150,142,24,20],[10,68,68,20],
               [180,80,32,24],[100,48,12,52],[180,142,22,20],[180,8,32,24],
               [8,146,30,20],[42,146,22,20],[44,110,28,28],[130,8,24,20]]
        enclosed={'S03','S11','S12','S13','S14','S16'}
        for r,rect in zip(location['rooms'],rects):
            r['rect']=rect; r['enclosed']=r['id'] in enclosed; r['door_side']='south';r['door']=[rect[0]+rect[2]//2,rect[1]+rect[3]]
        for number,name,rect in [('X01','Супермаркет',[8,46,42,18]),('X02','Аптека',[54,46,24,18]),
                                  ('X03','Продуктовый магазин',[8,92,30,14]),('X04','Склад снабжения',[180,36,32,36]),
                                  ('X05','Кафе для посетителей',[42,92,32,14])]:
            location['rooms'].append(dict(id=number,name=name,rect=rect,minimum_area_m2=0,enclosed=True,
                                         door_side='south',door=[rect[0]+rect[2]//2,rect[1]+rect[3]]))
        location['corridors']=[[0,0,240,170]]
        location['capture']=[86,74,0.98,-6,-140]

# Verify the authored plan independently of the game generator.
baseline={r['id']:r['rect'][2]*r['rect'][3] for l in source['locations'] for r in l['rooms']}
rooms=[r for l in world['locations'] for r in l['rooms']]
assert len(rooms)==333 and len({r['id'] for r in rooms})==333
for r in rooms:
    assert r['rect'][2]*r['rect'][3]>=baseline.get(r['id'],0),r['id']
for l in world['locations']:
    if l['id'] in ('ST','RF'): continue
    for i,a in enumerate(l['rooms']):
        x,y,w,h=a['rect']
        for b in l['rooms'][i+1:]:
            bx,by,bw,bh=b['rect']
            assert min(x+w,bx+bw)<=max(x,bx) or min(y+h,by+bh)<=max(y,by), (a['id'],b['id'])
(DATA/'hospital_world.json').write_text(json.dumps(world,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
report={'original_rooms':len(baseline),'rooms':len(rooms),'shrunk_rooms':[],
        'original_room_area_m2':sum(baseline.values()),'new_room_area_m2':sum(r['rect'][2]*r['rect'][3] for r in rooms),
        'dimensions':{l['id']:l['bounds'] for l in world['locations']}}
out=ROOT/'artifacts/unreal_hospital';out.mkdir(parents=True,exist_ok=True)
(out/'area_audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))

# The retained v2 grid is the area/ID baseline; normal regeneration also applies
# the approved story-shell corrections and verifies all of its routes.
if __name__ == '__main__':
    import runpy
    runpy.run_path(str(ROOT/'tools/refine_hospital_story.py'),run_name='__main__')
    # Fit numbered set dressing against a fresh empty-shell navigation export.
    runpy.run_path(str(ROOT/'tools/verify_story_layout.py'),run_name='__main__')
    from hospital_furnishing_v6 import apply_furnishing
    current=json.loads((DATA/'hospital_world.json').read_text(encoding='utf-8'))
    assert apply_furnishing(current),'The furnishing schedule must fit completely'
    (DATA/'hospital_world.json').write_text(json.dumps(current,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_shell_v8.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_environment_v9.py'),run_name='__main__')
    # Refresh mob navigation for the added solid floor kits, retaining every
    # existing story pickup and encounter position.
    import contextlib,io,numpy as np
    with contextlib.redirect_stdout(io.StringIO()):env_nav=runpy.run_path(str(ROOT/'tools/verify_story_layout.py'))
    env_data=env_nav['D'];runs=[]
    for zi,z in enumerate(env_nav['LEVELS']):
        for yi,row in enumerate(env_nav['mask'][zi]):
            edges=np.diff(np.r_[False,row,False].astype(np.int8));starts=np.flatnonzero(edges==1);ends=np.flatnonzero(edges==-1)
            runs.extend([[round(z*100),yi,int(a),int(b-a)] for a,b in zip(starts,ends)])
    env_data['story_gameplay']['navigation']['rows']=runs
    (DATA/'hospital_world.json').write_text(json.dumps(env_data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_environment_v9.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_clutter_v10.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_clutter_v10.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_microdetails_v11.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_microdetails_v11.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_item_types_v12.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_item_types_v12.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_services_v13.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_services_v13.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_campus_v14.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_campus_v14.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_expansion_v15.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_expansion_v15.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_realism_v16.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_realism_v16.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_finish_v17.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_finish_v17.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_visual_realism_v18.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_v5.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_furnishing_v6.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_gameplay_v7.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_visual_realism_v18.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/hospital_horror_v19.py'),run_name='__main__')
    runpy.run_path(str(ROOT/'tools/audit_hospital_horror_v19.py'),run_name='__main__')
