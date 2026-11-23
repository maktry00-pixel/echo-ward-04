"""Export a matching tested map/build and the focused finishing-pass captures."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v17'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=P/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source);exe=PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
assert d['review_revision']==17 and eh==sha(P/'Binaries/Win64/EchoWard.exe') and mh==sha(PK/'Windows/EchoWard/Content/CampaignData/hospital_world.json')
native=read(PK/'verified_world.json');visual=read(PK/'verified_worldvisual_v17.json');story=read(PK/'verified_story_v7.json');menu=read(PK/'verified_native_menu_v17.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for v in (visual,story,menu):assert v['success'] and v['executable_sha256'].lower()==eh and v['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(native['test_saved'])/'Prototype/every_door_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==106 and rendered['success'] and rendered['count']==32 and doors['success'] and doors['doors']==1012 and story['checks']==23 and menu['checks']==57 and envelope['success'] and sum(envelope['failed'].values())==0
for name,key in [('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('finish_acceptance','map_sha256')]:
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh
images=visual['screenshots'];menu_images=[]
for name in images:shutil.copy2(Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/name,OUT/name)
for p in (Path(menu['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):
    shutil.copy2(p,OUT/('native_menu_'+p.name));menu_images.append('native_menu_'+p.name)
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17);new=[n for n in images if 'v17' in n]
for prefix,names in [('contact',images),('finish_contact',new),('menu_contact',menu_images)]:
    for start in range(0,len(names),20):
        group=names[start:start+20];canvas=Image.new('RGB',(1640,math.ceil(len(group)/4)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for j,name in enumerate(group):
            with Image.open(OUT/name) as src:im=src.convert('RGB');im.thumbnail((400,225))
            x=j%4*410;y=j//4*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name.removeprefix('world_')[:-4],font=font,fill='white')
        canvas.save(OUT/f'{prefix}_{start//20+1}.png')
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
for j,(name,label) in enumerate([('world_v17_guard_1.png','Площадка и ограждения поста охраны'),('world_campus_v17_room_1F.png','Дополнительное оборудование приёмного корпуса'),('world_campus_v17_room_PS2.png','Обстановка психиатрического корпуса'),('world_campus_v17_room_X01F2.png','Наполнение внешних зданий')]):
    with Image.open(OUT/name) as src:im=src.convert('RGB').resize((960,540))
    x=j%2*960;y=j//2*575;preview.paste(im,(x,y));draw.text((x+15,y+547),label,font=font,fill='#cee3df')
preview.save(OUT/'preview.jpg',quality=92)
for status,target,original in [(native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(native,'every_door_check','every_door_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check'),(menu,'native_menu_check','menu_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inv=read(OUT/'inventory.json')
report=dict(success=True,review_revision=17,build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=106,story_checks=23,native_menu_checks=57,visual_initial_checks=32,world_captures=len(images),menu_captures=len(menu_images),door_check=doors,envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],inventory=inv,screenshots=images,limits=['Prototype furniture and equipment; not final art.','One additional nurse workstation in PS2_02 did not fit; the existing furnishing remains and access is preserved.','Automated full-map collision/door/route checks plus focused images do not constitute a manual check of every detail.','Some inspection viewpoints may be obstructed. Fixed simulation step capture is not an FPS benchmark.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');(PK/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
text=f'''# ЭХО — проверка и дополнительное наполнение v17

Исправлены две ошибки на постах охраны. Сохранение на площадках +6,05 м раньше не проходило проверку допустимого этажа: 82 поверхности площадок и ступеней теперь зарегистрированы по своей реальной высоте. У второго поста грунт лежал на 20 см ниже мощения, и первая ступень фактически имела 35 см подъёма при допустимых 28 см. Добавлены бетонные площадки с верхом на 0 м, первая ступень снова 15 см. Оба поста физически пройдены вверх и вниз без телепорта, сохранение и загрузка на каждой площадке проверены. Добавлено 86 комплектов боковых и торцевых ограждений, центральный проход на лестницу открыт.

Добавлено {inv['room_kits']} полноразмерных комплектов: рабочие места, процедурные тележки, шкафы, архивные ящики, столы для занятий, бельевые корзины, верстаки и кухонные тележки. На реальных столешницах и полках размещено {inv['supported_details']} наборов медицинских, личных, канцелярских, кухонных и ремонтных вещей. Их положение проверено по границам и высоте опор. Старые 562 зоны и 35 локаций, размеры, мебель, лес, переходы, двери, сюжетные предметы и монстры сохранены. В PS2_02 ещё один сестринский стол не поместился: существующее оборудование осталось, проход не сужали.

Независимые проверки размеров, сохранности и маршрутов прошли. В настоящей финальной сборке пройдены 106/106 проверок мира, включая 1939 комнатных секций, 37 лестничных узлов, переходы, тоннели, оба поста и сохранения. Все 1012 дверей снова проверены на закрытую коллизию, открытое положение и восстановление. Сюжет и сохранения: 23/23; главное меню и вход: 57/57; начальные проверки отрисовки: 32/32. Снято {len(images)} кадров 1920×1080: 35 общих локаций и 37 новых ракурсов. Это выборочная визуальная проверка, а не полный ручной обход каждой детали или измерение FPS.

Игра и модуль Unreal Editor пересобраны. Модели пока упрощённые; новые мелкие вещи декоративные, текст записок намеренно не заполнялся. Запуск PLAY_UNREAL.cmd; B — яркий режим, M — карта, F — взаимодействие, K — засов убежища изнутри.
'''
(OUT/'REPORT.md').write_text(text,encoding='utf8');print(json.dumps(dict(success=True,map=mh,executable=eh,views=len(images),new_views=len(new)),ensure_ascii=False))
