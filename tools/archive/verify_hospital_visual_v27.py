"""Validate native full-map captures and the v27 themed equipment layer."""
from pathlib import Path
import hashlib,json
from PIL import Image
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';S=P/'Saved/PackageTests/WorldVisualV27/Saved'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
d=read(P/'Content/CampaignData/hospital_world.json');a=read(P/'Content/CampaignData/echo_detail_v27.json');w=read(S/'Prototype/book_world_check.json')
assert w['success'] and w['count']==62 and w['rooms']==562 and w['locations']==35
assert w['detail27_props']==len(a['props']) and w['custom_bounds_failures']==w['unresolved_custom_props']==0
names=['hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json','echo_detail_v27.json']
for n in names:assert sha(P/'Content/CampaignData'/n)==sha(PK/'Windows/EchoWard/Content/CampaignData'/n),n
views=d['locations']+[q for q in d['furnishing_captures'] if 'v19' in q['id']]
for n in names[2:]:
 layer=read(P/'Content/CampaignData'/n);views+=layer.get('captures',layer.get('review_captures',[]))
screens=[]
for v in views:
 n='world_'+v['id']+'.png'
 with Image.open(S/'Prototype/BookWorldCaptures'/n) as im:assert im.size==(1920,1080);im.verify()
 screens.append(n)
log=(S/'package_test.log').read_text(encoding='utf8',errors='replace')
assert not any(t in log for t in ['Default Material will be used','Failed to compile Material','GPU Crashed','Out of video memory','is not a valid parent for MaterialInstanceDynamic'])
result=dict(success=True,test_saved=str(S),executable_sha256=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe'),checks=w['count'],detail27_props=w['detail27_props'],data_sha256=sha(P/'Content/CampaignData/echo_detail_v27.json'),screenshots=screens,resolution='1920x1080')
(PK/'verified_worldvisual_v27.json').write_text(json.dumps(result,indent=2),encoding='utf8');print(json.dumps(dict(success=True,views=len(screens),detail27_props=w['detail27_props'])))
