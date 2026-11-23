from pathlib import Path
import json
from collections import Counter
R=Path(__file__).resolve().parents[1]
D=json.loads((R/'unreal/EchoWard/Content/CampaignData/campaign.json').read_text(encoding='utf-8-sig'))
meshes={m['id']:m for m in D['meshes']}
for word in ['shadow','puddle','vent','grate','shutter']:
    rows=[r for r in D['instances'] if word in r['path'].lower()]
    print(word,len(rows))
    for r in rows[:8]:print(r['path'],r.get('owner'),r['mesh'],r['transform'])
print('objects')
for o in D['objects']:
    if o.get('id') in ['quarantine_gate','main_exit','weapon']:print(json.dumps(o,ensure_ascii=False)[:1800])
for owner in ['equipped_pistol']:
    print(owner)
    for r in D['instances']:
        if r.get('owner')!=owner:continue
        m=meshes[r['mesh']];lo=[1e10]*3;hi=[-1e10]*3
        for l in (R/'unreal/EchoWard/RawAssets/Campaign'/m['file']).read_text().splitlines():
            if l.startswith('v '):
                v=list(map(float,l.split()[1:4]));lo=[min(a,b) for a,b in zip(lo,v)];hi=[max(a,b) for a,b in zip(hi,v)]
        print(r['path'].split('/')[-1],r['mesh'],r['transform'],'bounds',lo,hi)
