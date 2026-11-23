"""Refresh inspection views only; never move furniture or story objects."""
import json,math
from pathlib import Path
from hospital_furnishing_v6 import Index
ROOT=Path(__file__).resolve().parents[1];path=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(path.read_text(encoding='utf-8'));solid=Index()
for f in d['collision_features']:solid.add(f['z'],f['rect'])
for f in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):solid.add(f['z'],f['rect'])
rooms={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};catalog=d['prototype_catalog']
for c in d['furnishing_captures']:
    if c['id']=='environment_campus':c.update(at=[124,-10,5],target=[170,30,3])
    if c['id']=='environment_garden':c.update(at=[116,188,3],target=[86,208,.2])
    if not c['id'].startswith('environment_furnished_'):continue
    zid=c['id'].removeprefix('environment_furnished_');s,r=rooms[zid];ox,oy,z=s['origin'];options=[]
    room_bays=[b for b in d['subrooms'] if b['zone']==zid] or [dict(id=zid,rect=r['rect'])]
    for b in room_bays:
        x,y,w,h=b['rect'];x+=ox;y+=oy
        objects=[p for p in d['prototype_props']+d['environment_props'] if p.get('subroom')==b['id'] and p['mount']=='floor']
        for p in objects:
            tx,ty,_=p['at'];m=catalog[str(p['model'])];height=min(1.2,max(.45,m['bounds'][5]*.5))
            for xx in (x+.7,x+w/2,x+w-.7):
                for yy in (y+.7,y+h/2,y+h-.7):
                    distance=math.dist((xx,yy),(tx,ty))
                    if not 1.3<distance<5 or solid.hits(z,[xx-.2,yy-.2,.4,.4]):continue
                    score=min(distance,3)*2+len(objects)+(2 if p['model'] in (1,3,31,17,16,13) else 0)
                    options.append((score,[xx,yy,z+1.65],[tx,ty,z+height]))
    assert options,c['id']
    _,c['at'],c['target']=max(options,key=lambda o:o[0]);c['envelope']=False
    print(c['id'],c['at'],c['target'])
path.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
