"""Export evidence only for the exact packaged build which passed both runs."""
import json, shutil, hashlib, struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
project=ROOT/'unreal/EchoWard'; package=project/'Saved/PackagedWindows'
out=ROOT/'artifacts/unreal_hospital';out.mkdir(parents=True,exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
marker=read(package/'package_verified.json')
assert marker['success'] and marker['world_id']=='hospital_campus_v2'
runs=[]
for name in ('world','worldcapture'):
    status=read(package/f'verified_{name}.json')
    assert status['success'] and status['build_identity']==marker['build_identity']
    result=read(Path(status['test_saved'])/'Prototype/book_world_check.json')
    assert result['success'] and result['count']>=28 and result['rooms']==333
    assert all(c['passed'] for c in result['checks'])
    runs.append((status,result))
source=project/'Content/CampaignData/hospital_world.json'
staged=package/'Windows/EchoWard/Content/CampaignData/hospital_world.json'
assert source.read_bytes()==staged.read_bytes()
images=[]
for site in read(source)['locations']:
    name='world_'+site['id']+'.png'
    src=Path(runs[1][0]['test_saved'])/'Prototype/BookWorldCaptures'/name
    assert struct.unpack('>II',src.read_bytes()[16:24])==(1920,1080)
    shutil.copy2(src,out/name);images.append(name)
report={'success':True,'world_id':marker['world_id'],'checks':runs[1][1]['checks'],
        'area_audit':read(out/'area_audit.json'),'screenshots':images,
        'executable_sha256':hashlib.sha256(Path(marker['executable']).read_bytes()).hexdigest(),
        'sampled_performance':runs[1][1]['sampled_performance'],'checked_at':marker['checked_at']}
(out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'success':True,'checks':len(report['checks']),'captures':len(images),'exe_sha256':report['executable_sha256']},ensure_ascii=False))
