"""Validate native full-map captures including the v26 architectural layer."""
from pathlib import Path
import hashlib,json
from PIL import Image
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';S=P/'Saved/PackageTests/WorldVisualV26/Saved'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
d=read(P/'Content/CampaignData/hospital_world.json');detail=read(P/'Content/CampaignData/echo_detail_v26.json');w=read(S/'Prototype/book_world_check.json')
assert w['success'] and w['count']==58 and w['rooms']==562 and w['locations']==35
assert w['detail_props']==len(detail['props'])==detail['counts']['total'] and w['custom_bounds_failures']==w['unresolved_custom_props']==0
for name in ['hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json']:assert sha(P/'Content/CampaignData'/name)==sha(PK/'Windows/EchoWard/Content/CampaignData'/name)
views=d['locations']+[q for q in d['furnishing_captures'] if 'v19' in q['id']]
for name in ['echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json']:views+=read(P/'Content/CampaignData'/name).get('captures',read(P/'Content/CampaignData'/name).get('review_captures',[]))
names=[]
for view in views:
 name='world_'+view['id']+'.png';f=S/'Prototype/BookWorldCaptures'/name
 with Image.open(f) as im:assert im.size==(1920,1080);im.verify()
 names.append(name)
log=(S/'package_test.log').read_text(encoding='utf8',errors='replace')
assert not any(t in log for t in ['Default Material will be used','Failed to compile Material','GPU Crashed','Out of video memory','is not a valid parent for MaterialInstanceDynamic'])
result=dict(success=True,mode='WorldVisualV26',test_saved=str(S),executable_sha256=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe'),checks=w['count'],detail_props=w['detail_props'],screenshots=names,resolution='1920x1080',scope='All previous 105 views plus new architectural details in bright and horror modes; fixed simulation step, not a performance benchmark.')
(PK/'verified_worldvisual_v26.json').write_text(json.dumps(result,indent=2),encoding='utf8');print(json.dumps(dict(success=True,views=len(names),detail_props=w['detail_props'])))
