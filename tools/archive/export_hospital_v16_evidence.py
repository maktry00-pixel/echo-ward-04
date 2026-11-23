"""Package evidence only when the final map/executable and native checks agree."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard';PACKAGE=PROJECT/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v16'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source);exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
assert d['review_revision']==16 and eh==sha(PROJECT/'Binaries/Win64/EchoWard.exe') and mh==sha(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json')
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v16.json');story=read(PACKAGE/'verified_story_v7.json');menu=read(PACKAGE/'verified_native_menu_v16.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for v in (visual,story,menu):assert v['success'] and v['executable_sha256'].lower()==eh and v['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(native['test_saved'])/'Prototype/every_door_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==96 and rendered['success'] and rendered['count']==30 and doors['success'] and doors['doors']==1012 and story['checks']==23 and menu['checks']==57 and envelope['success'] and sum(envelope['failed'].values())==0
for name,key in [('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('realism_acceptance','map_sha256')]:
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh
images=[]
for c in d['locations']+d['furnishing_captures']:
    p=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    if not p.exists():p=Path(visual['additional_capture_saved'])/'Prototype/BookWorldCaptures'/p.name
    with Image.open(p) as im:assert im.size==(1920,1080)
    shutil.copy2(p,OUT/p.name);images.append(p.name)
menu_names=[]
for p in (Path(menu['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):
    shutil.copy2(p,OUT/('native_menu_'+p.name));menu_names.append('native_menu_'+p.name)
assert 'native_menu_menu_1080.png' in menu_names and 'native_menu_door_open_1080.png' in menu_names
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
new=[x for x in images if x.startswith(('world_campus_v16_','world_v16_'))]
for prefix,names in [('contact',images),('realism_contact',new),('menu_contact',menu_names)]:
    for start in range(0,len(names),20):
        group=names[start:start+20];canvas=Image.new('RGB',(1640,math.ceil(len(group)/4)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for i,name in enumerate(group):
            with Image.open(OUT/name) as src:im=src.convert('RGB');im.thumbnail((400,225))
            x=i%4*410;y=i//4*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name.removeprefix('world_')[:-4],font=font,fill='white')
        canvas.save(OUT/f'{prefix}_{start//20+1}.png')
preview=Image.new('RGB',(1920,1725),'#10191b');draw=ImageDraw.Draw(preview)
for j,(name,label) in enumerate([('native_menu_menu_1080.png','Главное меню без деревьев'),('world_v16_return_0.png','Обратный переход к задним корпусам'),('world_v16_spine_supports.png','Опоры под переходами'),('world_v16_perimeter_mesh.png','Сетка и свет на высокой стене'),('world_v16_plate_R07.png','Таблички с назначением комнат'),('world_campus_v16_corridor_1F.png','Обстановка коридоров')]):
    with Image.open(OUT/name) as src:im=src.convert('RGB').resize((960,540))
    x=j%2*960;y=j//2*575;preview.paste(im,(x,y));draw.text((x+15,y+547),label,font=font,fill='#cee3df')
preview.save(OUT/'preview.jpg',quality=92)
for status,target,original in [(native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(native,'every_door_check','every_door_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check'),(menu,'native_menu_check','menu_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inv=read(OUT/'inventory.json')
report=dict(success=True,review_revision=16,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=96,story_checks=23,native_menu_checks=57,visual_initial_checks=30,world_captures=len(images),menu_captures=len(menu_names),door_check=doors,envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],inventory=inv,screenshots=images,capture_method=visual['scope'],limits=['Prototype furniture and forest models, not final art.','Every door has an automated open/close/closed-collision/state-restore check; no claim of a manual tour of every detail.','Some retained camera viewpoints are obstructed or close to walls.','Doors retain authored story locks; a locked story door does not imply a broken door.','The first bulk screenshot process stopped after 165 images without a final result; identical-geometry continuation completed the remaining views. No sustained performance claim is made.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');(PACKAGE/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
text=f'''# ЭХО — коридоры, опоры, двери и меню v16

Обновлена настоящая игра и исходники Unreal Editor. Все562 зоны,35 локаций, размеры комнат, прежнее наполнение и сюжетные места сохранены.

Деревья проникали в меню потому, что сцены меню наX/Y300м пересеклись с новым лесом. Весь набор меню перенесён наX/Y3000м; старые относительные движения камер, скример, медсестра, мяч и переход в игру сохранены. Проверка проведена с полной картой и лесом, без отключения больничного мира.

Северный конец продольной галереи теперь соединён обратной ветвью сHS2 иSV2. Он образует петлю и путь к задним лестницам. Установлены{inv['support_frames']} новых рамных опор с двумя стойками и поперечной балкой под настилом; максимальный пролёт12,75м. После первого теста перенесена одна опора, мешавшая проезду. {len(d['realism_upgrade']['converted_endpoint_niches'])} прежних карманов на стыках превратились в полноценные проходы: их узкие косяки убраны. Вдоль обратного маршрута добавлено{sum(n['id'].startswith('V16_NICHE') for n in d['escape_niches'])} новых ниш.

В коридоры добавлен{inv['corridor_floor_items']} полноразмерный предмет: места ожидания, урны с педалью, тележки документов и уборки, питьевые станции, коляски, реанимационные и почтовые тележки. Все размещены вне защищённых подходов к дверям, укрытиям и прежних маршрутов. Добавлено{inv['wall_equipment']} настенных комплектов: пожарные посты, телефоны, часы, доски объявлений и дозаторы. Колёса новых колясок опущены на настоящий пол после проверки размеров.

{inv['signs']} читаемых русских табличек и указателей: номера и назначения всех541закрытого помещения, корпуса, общественные здания, лифты, лестницы и выходы. На стене КПП —{inv['mesh_panels']} секций металлической сетки, проходы ворот оставлены свободными; добавлено{inv['floodlights']} прожекторов. Официальные ориентиры NHS перечислены вSOURCES.md.

Все1012дверей автоматически проверены на закрытую коллизию, открытое положение всех18216видимых деталей и восстановление состояния. Сюжетные запоры и засовы сохранены. Итог настоящей сборки:96/96 проверок мира,23/23сюжета и сохранений,57/57главного меню и входа в игру,30/30начальных проверок отрисовки; {len(images)} кадров карты и{len(menu_names)} меню в1920×1080. Пробы оболочек: {sum(envelope['tested'].values())}, обнаруженных пропусков0. Это автоматические проверки и просмотр выбранных кадров, а не ручное утверждение о каждой детали562зон.

Мебель, деревья и оборудование пока прототипные. Старые служебные камеры иногда смотрят в близкую стену; яркий осмотр даёт пересвет на части наружных поверхностей. ЗапускPLAY_UNREAL.cmd, B — яркий осмотр, M — карта, F — взаимодействие с дверью, K — засов убежища изнутри.
'''
text+='\nПервый массовый проход съёмки остановился после165кадров без итогового файла. Оставшиеся ракурсы и все35локаций сняты отдельным проходом: изменялся только список служебных камер, геометрия совпадает с основной картой. Исходный JSON восстановлен. Длительная стабильность массового захвата и игровой FPS этим не подтверждаются.\n'
(OUT/'REPORT.md').write_text(text,encoding='utf8');print(json.dumps(dict(success=True,map=mh,executable=eh,views=len(images),new_views=len(new),menu_views=len(menu_names)),ensure_ascii=False))
