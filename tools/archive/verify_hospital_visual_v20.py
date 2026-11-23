"""Validate a focused native capture against the complete unchanged game map."""
import hashlib,json
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';SAVED=P/'Saved/PackageTests/WorldVisualV20/Saved'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
d=read(P/'Content/CampaignData/hospital_world.json');r=read(SAVED/'Prototype/book_world_check.json')
assert r['success'] and r['count']==46 and r['rooms']==562 and r['locations']==35
assert sha(P/'Content/CampaignData/hospital_world.json')==sha(PK/'Windows/EchoWard/Content/CampaignData/hospital_world.json')
assert r['custom_furniture']>3000 and r['custom_door_leaves']>1000 and r['custom_bounds_failures']==0
for name in ('echo_custom_runtime.json','echo_custom_bindings.json'):
    assert sha(P/'Content/CampaignData'/name)==sha(PK/'Windows/EchoWard/Content/CampaignData'/name)
names=[]
for q in d['locations']+[q for q in d['furnishing_captures'] if 'v19' in q['id']]:
    name='world_'+q['id']+'.png';p=SAVED/'Prototype/BookWorldCaptures'/name
    with Image.open(p) as im:assert im.size==(1920,1080);im.verify()
    names.append(name)
log=(SAVED/'package_test.log').read_text(encoding='utf8',errors='replace')
assert not any(s in log for s in ('Default Material will be used','Failed to compile Material','pending kill, unreachable or pending destroy'))
result=dict(success=True,mode='WorldVisualV20',test_saved=str(SAVED),executable_sha256=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe'),map_sha256=sha(P/'Content/CampaignData/hospital_world.json'),initial_checks=r['count'],resolution='1920x1080',screenshots=names,scope='All 35 location cameras and 29 horror/bright comparison views in the canonical map. Camera-prefix filtering only; map metadata and geometry unchanged. Fixed simulation step, not a performance benchmark.')
(PK/'verified_worldvisual_v20.json').write_text(json.dumps(result,indent=2),encoding='utf8');print(json.dumps(dict(success=True,views=len(names))))
