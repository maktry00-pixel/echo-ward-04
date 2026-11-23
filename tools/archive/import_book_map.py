"""Import only room geometry from the authored atlas; never import gameplay instructions."""
from pathlib import Path
import hashlib
import json
import re
from collections import Counter

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'tmp/pdfs/atlas_data.json'
BOOK=Path(r'C:/Users/YOUR_WINDOWS_USER/Downloads/ECHO_Master_Game_Bible_Complete (1).pdf')
TARGET=ROOT/'unreal/EchoWard/Content/CampaignData/book_map.json'
LOCATIONS=[
    ('1F','Приёмный корпус','Reception','Қабылдау корпусы'),
    ('B1','Подвал','Basement','Жертөле'),
    ('DG','Диагностика','Diagnostics','Диагностика'),
    ('2F','Лечебный этаж','Treatment floor','Емдеу қабаты'),
    ('IF','Инфекционный корпус','Infectious diseases','Инфекция корпусы'),
    ('PS','Психоневрология','Psychoneurology','Психоневрология'),
    ('3F','Администрация','Administration','Әкімшілік'),
    ('RH','Реабилитация','Rehabilitation','Оңалту'),
    ('HS','Корпус персонала','Staff housing','Персонал корпусы'),
    ('SV','Хозяйственный корпус','Services','Шаруашылық'),
    ('ST','Территория','Grounds','Аумақ'),
    ('RF','Крыша','Roof','Шатыр'),
    ('B2','Центр «Гелиос»','Helios centre','«Гелиос»'),
]

data=json.loads(SOURCE.read_text('utf8'))
rooms=list(data['rooms'].values())
assert len(rooms)==328 and len({r['id'] for r in rooms})==328
# Cross-check every room ID against the attached book, not merely the data filename.
pages=json.loads((ROOT/'tmp/echo_bible_review/pages.json').read_text('utf8'))
book_atlas='\n'.join(pages[219:326])
for r in rooms:
    assert re.search(r'(?<![A-Z0-9])'+re.escape(r['id'])+r'(?![A-Z0-9])',book_atlas),r['id']
out={'schema_version':1,'map_id':'book_atlas_328','source_title':BOOK.name,
     'source_sha256':hashlib.sha256(BOOK.read_bytes()).hexdigest(),
     'presentation_only':True,'locations':[]}
for code,ru,en,kk in LOCATIONS:
    rr=[r for r in rooms if r['level']==code]
    factor=2 if code in ('ST','RF') else 1
    rect=lambda x,y,w,h:[x*factor,y*factor,w*factor,h*factor]
    corridors=[rect(28,0,6,44),rect(0,20,62,4)]
    for wing in sorted({r['wing'] for r in rr}):
        corridors.append(rect(34*(wing%2),24*(wing//2)+8,28,4))
    cores=[{'name':label,'rect':rect(28,y,3,8)} for y,label in [(0,'A'),(12,'L'),(24,'B'),(36,'T')]]
    rows=[]
    for r in rr:
        assert r['w']>0 and r['h']>0
        rows.append({'id':r['id'],'name':r['name'],'rect':rect(r['x'],r['y'],r['w'],r['h'])})
    out['locations'].append({'id':code,'name':ru,'name_en':en,'name_kk':kk,'bounds':rect(0,0,62,44),'rooms':rows,'corridors':corridors,'cores':cores})
assert sum(len(l['rooms']) for l in out['locations'])==328
assert all(set(r)=={'id','name','rect'} for l in out['locations'] for r in l['rooms'])
TARGET.write_text(json.dumps(out,ensure_ascii=False,indent=2),'utf8')
print(json.dumps({'success':True,'rooms':len(rooms),'locations':len(LOCATIONS),'counts':dict(Counter(r['level'] for r in rooms)),'target':str(TARGET)},ensure_ascii=False))
