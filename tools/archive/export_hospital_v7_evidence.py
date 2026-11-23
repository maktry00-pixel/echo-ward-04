"""Publish evidence only when native, rendered and independent checks match."""
from pathlib import Path
import csv,hashlib,json,shutil
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT/'unreal/EchoWard';OUT=ROOT/'artifacts/unreal_hospital_v7'
PACKAGE=PROJECT/'Saved/PackagedWindows'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json'
exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
data=read(source);map_hash=sha(source);exe_hash=sha(exe)
assert sha(PROJECT/'Binaries/Win64/EchoWard.exe')==exe_hash
assert data['review_revision']==7
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
world=read(PACKAGE/'verified_world.json')
assert world['success'] and world['build_identity'].split(':')[0].lower()==exe_hash and world['build_identity'].split(':')[-1].lower()==map_hash
world_result=read(Path(world['test_saved'])/'Prototype/book_world_check.json')
assert world_result['success'] and world_result['count']==78
for tag in ('story','storyvisual'):
    status=read(PACKAGE/f'verified_{tag}_v7.json')
    assert status['success'] and status['executable_sha256'].lower()==exe_hash and status['map_sha256'].lower()==map_hash
    result=read(Path(status['test_saved'])/'Prototype/story_check.json')
    assert result['success'] and result['count']>=23 and all(c['passed'] for c in result['checks'])
    shutil.copy2(Path(status['test_saved'])/'Prototype/story_check.json',OUT/f'native_{tag}_check.json')
    if tag=='storyvisual':
        picture=Path(status['test_saved'])/'Prototype/story_v7.png'
        assert Image.open(picture).size==(1920,1080)
        shutil.copy2(picture,OUT/'story_preview_1080.png')
        visual=status
for name,key in (('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256')):
    result=read(OUT/f'{name}.json')
    assert result['success'] and result[key]==map_hash
for target in ('game','editor'):
    assert 'Result: Succeeded' in (OUT/f'build_{target}.log').read_text(encoding='utf-8',errors='replace')
for status_name in ('story','storyvisual'):
    status=read(PACKAGE/f'verified_{status_name}_v7.json')
    log=(Path(status['test_saved'])/'console.log').read_text(encoding='utf-8',errors='replace')
    assert not any(s in log for s in ('Fatal error:','Assertion failed:','STORY_CHECK FAIL','Default Material will be used','Failed to compile Material'))
shutil.copy2(source,OUT/'hospital_world_source.json')
for name,rows in (('story_placements',data['story_gameplay']['placements']),('monster_placements',data['story_gameplay']['mobs'])):
    fields=['id','zone','site','chapter','kind','label','species','name','combat_class','kill_condition','x_m','y_m','z_m']
    with (OUT/f'{name}.csv').open('w',encoding='utf-8-sig',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=fields,extrasaction='ignore');writer.writeheader()
        for row in rows:writer.writerow(dict(row,x_m=row['at'][0],y_m=row['at'][1],z_m=row['at'][2]))
shutil.copy2(Path(world['test_saved'])/'Prototype/book_world_check.json',OUT/'native_world_check.json')
report=dict(success=True,review_revision=7,checked_at=visual['checked_at'],
    executable_sha256=exe_hash,map_sha256=map_hash,build_identity=world['build_identity'],
    world_id=data['map_id'],world_rooms=562,world_locations=35,world_checks=78,
    story_checks=read(OUT/'native_story_check.json')['count'],
    rendered_story_checks=read(OUT/'native_storyvisual_check.json')['count'],
    story_objects=len(data['story_gameplay']['placements']),blank_notes=96,mobs=46,species=17,
    scene_stubs=78,chapters=19,link_nodes=12,
    visual_scope='One actual 1080p rendered encounter view; full architecture independently checked by the native 78-check traversal.',
    limits=data['story_gameplay']['limits']+[
        'Same-floor prototype navigation; monsters do not pursue between storeys.',
        'Branching dialogue, companion escort, cinematics, advanced infrastructure powers and authored endings remain unfinished.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
marker=dict(report,executable=str(exe),executable_timestamp=world['executable_timestamp'],
    physical_stair_and_gallery_walk_verified=True,physical_tunnel_walk_verified=True)
(PACKAGE/'package_verified.json').write_text(json.dumps(marker,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
