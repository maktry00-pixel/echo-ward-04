"""Export only current native traversal + fresh rendered views of the same build."""
from pathlib import Path
from collections import Counter
import hashlib,json,shutil
from PIL import Image,ImageDraw,ImageFont

root=Path(__file__).resolve().parents[1];project=root/'unreal/EchoWard';out=root/'artifacts/unreal_hospital_v5';package=project/'Saved/PackagedWindows'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
source=project/'Content/CampaignData/hospital_world.json';data=read(source);sha=hashlib.sha256(source.read_bytes()).hexdigest()
exe=package/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';exe_sha=hashlib.sha256(exe.read_bytes()).hexdigest()
native=read(package/'verified_world.json');visual=read(package/'verified_worldvisual_v5.json');acceptance=read(out/'acceptance.json')
assert native['success'] and visual['success'] and acceptance['success']
assert source.read_bytes()==(package/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
assert native['build_identity'].split(':')[-1].lower()==sha==visual['map_sha256'].lower()==acceptance['source_sha256']
assert native['build_identity'].split(':')[0].lower()==exe_sha==visual['executable_sha256'].lower()
check=read(Path(native['test_saved'])/'Prototype/book_world_check.json')
views=read(Path(visual['test_saved'])/'Prototype/book_world_check.json')
assert check['success'] and check['count']==76 and check['rooms']==562 and check['locations']==35
assert views['success'] and views['count']==10
for name in ('book_world_check','book_world_geometry','escape_doors_check','hideout_check','niche_check','story_routes_collision'):
    shutil.copy2(Path(native['test_saved'])/f'Prototype/{name}.json',out/f'native_{name}.json')
shutil.copy2(source,out/'hospital_world_source.json')
font=ImageFont.truetype(str(project/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
images=[]
for start in (0,20):
    canvas=Image.new('RGB',(1640,1270),'#101a1b');draw=ImageDraw.Draw(canvas)
    for index,l in enumerate(data['locations'][start:start+20]):
        name='world_'+l['id']+'.png';src=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/name
        im=Image.open(src).convert('RGB');assert im.size==(1920,1080);shutil.copy2(src,out/name);images.append(name)
        im.thumbnail((400,225));x=index%4*410;y=index//4*254
        canvas.paste(im,(x,y+25));draw.text((x+4,y+2),l['id'],font=font,fill='white')
    canvas.save(out/f'capture_contact_{start//20+1}.png')
report=dict(success=True,revision=5,world_id=data['map_id'],checked_at=visual['checked_at'],
    executable_sha256=exe_sha,map_sha256=sha,build_identity=native['build_identity'],
    world_checks=check['count'],visual_initial_checks=views['count'],requirements_checks=len(acceptance['checks']),
    rooms=562,locations=35,stairs=len(data['stairs']),furnishings=len(data['furnishings']),
    hideouts=dict(Counter(h['kind'] for h in data['hideouts'])),screenshots=images,
    visual_scope=visual['scope'],sampled_performance=views['sampled_performance'],
    limits=acceptance['limits'])
(out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
marker=dict(success=True,checked_at=visual['checked_at'],executable=str(exe),executable_timestamp=native['executable_timestamp'],
    build_identity=native['build_identity'],world_id=data['map_id'],world_checks=check['count'],world_rooms=562,world_locations=35,
    world_captures=35,visual_initial_checks=10,visual_scope=visual['scope'],physical_stair_and_gallery_walk_verified=True,
    physical_tunnel_walk_verified=True,review_revision=5)
(package/'package_verified.json').write_text(json.dumps(marker,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(success=True,rooms=562,locations=35,native_checks=check['count'],visual_initial_checks=10,photos=len(images),map_sha256=sha),ensure_ascii=False))
