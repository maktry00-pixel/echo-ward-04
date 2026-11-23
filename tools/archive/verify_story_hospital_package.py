"""Export evidence only from the matching v3 Windows world/capture runs."""
from pathlib import Path
import hashlib,json,shutil,struct
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/'unreal/EchoWard';PACKAGE=P/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v3'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
marker=read(PACKAGE/'package_verified.json')
assert marker['success'] and marker['world_id']=='hospital_story_shell_v3'
source=P/'Content/CampaignData/hospital_world.json'
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
runs=[]
for mode in ('world','worldcapture'):
    status=read(PACKAGE/f'verified_{mode}.json');result=read(Path(status['test_saved'])/'Prototype/book_world_check.json')
    assert status['success'] and status['build_identity']==marker['build_identity']
    assert result['success'] and result['count']>=38 and result['rooms']==334 and result['locations']==15
    assert all(c['passed'] for c in result['checks'])
    runs.append((status,result))
images=[]
for site in read(source)['locations']:
    name='world_'+site['id']+'.png'
    src=Path(runs[1][0]['test_saved'])/'Prototype/BookWorldCaptures'/name
    assert struct.unpack('>II',src.read_bytes()[16:24])==(1920,1080)
    shutil.copy2(src,OUT/name);images.append(name)
report=dict(success=True,world_id=marker['world_id'],checks=runs[1][1]['checks'],
    rooms=334,locations=15,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    executable_sha256=hashlib.sha256(Path(marker['executable']).read_bytes()).hexdigest(),
    build_identity=marker['build_identity'],checked_at=marker['checked_at'],screenshots=images,
    sampled_performance=runs[1][1]['sampled_performance'],
    navigation=read(OUT/'navigation_audit.json'),authoring=read(OUT/'authoring_audit.json'),
    scope='Playable architecture and story bindings. Full chapter scenes, dialogue and monster AI are not implemented.')
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k:report[k] for k in ('success','rooms','locations','executable_sha256')},ensure_ascii=False))
