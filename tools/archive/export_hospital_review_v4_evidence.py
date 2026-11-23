"""Export only evidence matching the current executable, archives and manifest."""
from pathlib import Path
import csv,hashlib,json,shutil,struct,zipfile
from PIL import Image,ImageDraw,ImageFont

root=Path(__file__).resolve().parents[1]
p=root/'unreal/EchoWard';package=p/'Saved/PackagedWindows';out=root/'artifacts/unreal_hospital_v4'
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
marker=read(package/'package_verified.json');source=p/'Content/CampaignData/hospital_world.json';d=read(source)
assert marker['success'] and d['review_revision']==4
assert source.read_bytes()==(package/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
assert marker['build_identity'].split(':')[-1].lower()==hashlib.sha256(source.read_bytes()).hexdigest()
assert marker['build_identity'].split(':')[0].lower()==hashlib.sha256(Path(marker['executable']).read_bytes()).hexdigest()
runs={}
for mode in ('world','worldcapture'):
    status=read(package/f'verified_{mode}.json');result=read(Path(status['test_saved'])/'Prototype/book_world_check.json')
    assert status['success'] and status['build_identity']==marker['build_identity']
    assert result['success'] and result['count']>=46 and all(c['passed'] for c in result['checks'])
    runs[mode]=dict(status=status,result=result)
    for name in ('escape_doors_check','hideout_check','niche_check','story_routes_collision'):
        src=Path(status['test_saved'])/f'Prototype/{name}.json'
        if src.exists():shutil.copy2(src,out/f'{mode}_{name}.json')
images=[]
contact=Image.new('RGB',(1600,1620),'#10191b');draw=ImageDraw.Draw(contact)
font=ImageFont.truetype(str(p/'Content/CampaignData/Fonts/NotoSans.ttf'),18)
for i,site in enumerate(d['locations']):
    name='world_'+site['id']+'.png';src=Path(runs['worldcapture']['status']['test_saved'])/'Prototype/BookWorldCaptures'/name
    assert struct.unpack('>II',src.read_bytes()[16:24])==(1920,1080)
    shutil.copy2(src,out/name);images.append(name)
    im=Image.open(src).convert('RGB');im.thumbnail((520,292));x=(i%3)*533;y=(i//3)*324
    contact.paste(im,(x,y+26));draw.text((x+4,y+2),site['id'],fill='white',font=font)
contact.save(out/'capture_contact.png')
acceptance=read(out/'acceptance.json');assert acceptance['success'] and acceptance['map_sha256']==hashlib.sha256(source.read_bytes()).hexdigest()
distance=read(out/'escape_distance_audit.json');assert distance['success'] and distance['source_sha256']==acceptance['map_sha256']
report=dict(success=True,revision=4,world_id=d['map_id'],checked_at=marker['checked_at'],
    executable_sha256=marker['build_identity'].split(':')[0],map_sha256=acceptance['map_sha256'],
    build_identity=marker['build_identity'],world_checks=runs['world']['result']['count'],
    capture_checks=runs['worldcapture']['result']['count'],requirements_checks=len(acceptance['checks']),
    screenshots=images,counts=acceptance['counts'],corridor_distance=dict(grid_m=.5,maximum_m=max(s['maximum_upper_m'] for s in distance['per_site']),samples=sum(s['samples'] for s in distance['per_site']),failed_samples=distance['failed_samples']),sampled_performance=runs['worldcapture']['result']['sampled_performance'],
    scope='Architecture, interactions and chapter/monster production data. No new monster AI or complete story campaign.')
(out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
shutil.copy2(source,out/'hospital_world_source.json')
shutil.copy2(root/'artifacts/unreal_hospital_v3/zone_story_classes.csv',out/'zone_story_classes.csv')
shutil.copy2(root/'artifacts/unreal_hospital_v3/navigation_audit.json',out/'navigation_audit.json')
shutil.copy2(root/'artifacts/unreal_hospital_v3/authoring_audit.json',out/'authoring_audit.json')
# A portable review pack. Never present an earlier test marker as final evidence.
dest=root/'output/reviews';dest.mkdir(parents=True,exist_ok=True)
archive=dest/'ЭХО_исправления_по_оценке_v3.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for f in sorted(out.iterdir()):
        if f.is_file() and f.name!='verification_before_pull_guard.json':z.write(f,f.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print(json.dumps(dict(success=True,world_checks=report['world_checks'],capture_checks=report['capture_checks'],
    archive=str(archive),archive_bytes=archive.stat().st_size,exe=report['executable_sha256']),ensure_ascii=False,indent=2))
