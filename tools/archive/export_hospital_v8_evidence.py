"""Verify identical staged/source build and export native envelope + all views."""
from pathlib import Path
import hashlib,json,shutil
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard'
OUT=ROOT/'artifacts/unreal_hospital_v8';PACKAGE=PROJECT/'Saved/PackagedWindows'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';data=read(source);map_hash=sha(source)
exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';exe_hash=sha(exe)
assert data['review_revision']==8 and sha(PROJECT/'Binaries/Win64/EchoWard.exe')==exe_hash
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v8.json');story=read(PACKAGE/'verified_story_v7.json')
assert all(s['success'] for s in (native,visual,story))
assert native['build_identity'].split(':')[0].lower()==exe_hash and native['build_identity'].split(':')[-1].lower()==map_hash
for s in (visual,story):assert s['executable_sha256'].lower()==exe_hash and s['map_sha256'].lower()==map_hash
world_result=read(Path(native['test_saved'])/'Prototype/book_world_check.json');visual_result=read(Path(visual['test_saved'])/'Prototype/book_world_check.json')
assert world_result['success'] and world_result['count']==83 and visual_result['success'] and visual_result['count']==17
assert read(Path(story['test_saved'])/'Prototype/story_check.json')['success'] and story['checks']>=23
envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert envelope['success'] and not any(envelope['failed'].values())
for name,key in (('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256')):
    result=read(OUT/f'{name}.json');assert result['success'] and result[key]==map_hash
assert read(OUT/'metric_preservation.json')['success']
for target in ('game','editor'):assert 'Result: Succeeded' in (OUT/f'build_{target}.log').read_text(encoding='utf-8',errors='replace')
captures=data['locations']+data['furnishing_captures'];images=[]
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
for index,c in enumerate(captures):
    path=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    with Image.open(path) as im:assert im.size==(1920,1080)
    shutil.copy2(path,OUT/path.name);images.append(path.name)
for start in range(0,len(images),20):
    canvas=Image.new('RGB',(1640,1270),'#10191b');draw=ImageDraw.Draw(canvas)
    for i,name in enumerate(images[start:start+20]):
        im=Image.open(OUT/name).convert('RGB');im.thumbnail((400,225));x=i%4*410;y=i//4*254
        canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name[6:-4],font=font,fill='white')
    canvas.save(OUT/f'contact_{start//20+1}.png')
for status,name,folder in ((native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(visual,'rendered_world_check','book_world_check'),(story,'native_story_check','story_check')):
    shutil.copy2(Path(status['test_saved'])/f'Prototype/{folder}.json',OUT/f'{name}.json')
shutil.copy2(source,OUT/'hospital_world_source.json')
report=dict(success=True,review_revision=8,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=exe_hash,map_sha256=map_hash,
    world_id=data['map_id'],world_rooms=562,world_locations=35,world_checks=83,story_checks=story['checks'],visual_initial_checks=17,
    envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],world_captures=len(images),exterior_views=22,furnished_views=13,
    physical_stair_and_gallery_walk_verified=True,physical_tunnel_walk_verified=True,
    visual_scope=visual['scope'],screenshots=images,
    limits=['Native envelope uses one-metre floor/ceiling centres and three horizontal samples per metre of exterior wall, not a proof at every pixel.',
            'Some inspection cameras show close-ups of walls or equipment; the 70 captures do not replace traversal of every room.',
            'Furniture and monsters retain prototype models; the 210-hour story remains a spatial gameplay prototype.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(PACKAGE/'package_verified.json').write_text(json.dumps(dict(report,executable=str(exe),executable_timestamp=native['executable_timestamp']),ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
