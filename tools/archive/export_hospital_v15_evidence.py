"""Export only a matching map, executable and fresh native/rendered evidence."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard';PACKAGE=PROJECT/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v15'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source)
exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
assert d['review_revision']==15 and eh==sha(PROJECT/'Binaries/Win64/EchoWard.exe')
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v15.json');story=read(PACKAGE/'verified_story_v7.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for r in (visual,story):assert r['success'] and r['executable_sha256'].lower()==eh and r['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==93 and rendered['success'] and rendered['count']==27 and story['checks']==23 and envelope['success'] and sum(envelope['failed'].values())==0
for name,key in [('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('expansion_acceptance','map_sha256')]:
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh,(name,a.get(key),mh)
images=[]
for c in d['locations']+d['furnishing_captures']:
    p=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    with Image.open(p) as im:assert im.size==(1920,1080)
    shutil.copy2(p,OUT/p.name);images.append(p.name)
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
new=[x for x in images if x.startswith(('world_campus_v15_','world_v15_'))]
for prefix,names in [('contact',images),('expansion_contact',new)]:
    for start in range(0,len(names),20):
        group=names[start:start+20];canvas=Image.new('RGB',(1640,math.ceil(len(group)/4)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for i,name in enumerate(group):
            with Image.open(OUT/name) as src:im=src.convert('RGB');im.thumbnail((400,225))
            x=i%4*410;y=i//4*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name[6:-4],font=font,fill='white')
        canvas.save(OUT/f'{prefix}_{start//20+1}.png')
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
for j,(name,label) in enumerate([('world_v15_bridge_underpass.png','Переходы на втором этаже'),('world_v15_forest_gate.png','Высокий лес за КПП'),('world_v15_psy_window.png','Защищённые окна психиатрического корпуса'),('world_campus_v15_X01.png','Наполнение супермаркета')]):
    with Image.open(OUT/name) as src:im=src.convert('RGB').resize((960,540))
    x=j%2*960;y=j//2*575;preview.paste(im,(x,y));draw.text((x+15,y+547),label,font=font,fill='#cee3df')
preview.save(OUT/'preview.jpg',quality=92)
for status,target,original in [(native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inv=read(OUT/'expansion_inventory.json')
report=dict(success=True,review_revision=15,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=93,story_checks=23,visual_initial_checks=27,world_captures=len(images),envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],expansion_inventory=inv,screenshots=images,limits=['Prototype room kits and procedural trees, not final art.','New guarded windows have real glazing collision and visible external screens; window opening is not a new player interaction.','Original inspection cameras include some obstructed/close-wall views; images do not constitute a manual tour of every room.','Original campus and all1939 room bays are checked by native traversal. Outer forest scenery is additionally rendered; no completed forest campaign is claimed.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');(PACKAGE/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
text=f'''# ЭХО — переходы второго этажа, лес и наполнение v15

Обновлена игра и модуль Unreal Editor. Все562 зоны,35 локаций, размеры комнат,37 лестничных узлов, лифт, прежняя мебель, сюжетные предметы и подземные связи сохранены.

## Переходы, стена и КПП

Семь наземных галерей перенесены на отметку+4,8м — второй этаж, который есть у всех соединяемых корпусов. Шесть боковых ветвей соединяются сDG2/IF2/PS2/RH2/HS2/SV2, продольная галерея соединена со вторым этажом главного корпуса. Укрытия и их реальные входы перенесены вместе с полом; двери на концах переходов работают. Под переходами теперь открытая территория. Добавлены{inv['bridge_supports']} опоры, высокие светопрозрачные боковые панели и регулярные потолочные светильники. Подземные тоннели сохранены.

Стена КПП поднята до5,8м. Посты охраны подняты на6,05м, лестницы40ступеней по15см с проступью45см. Шесть проходов и проездов сохранены открытыми для текущего режима осмотра. На стене есть светильники. Оружие на постах пока декоративное.

## Лес

Добавлено{inv['trees']} отдельных деревьев с настоящими объёмными стволами и округлыми кронами: {inv['inside_trees']} редких посадок11–16м внутри и густой внешний пояс19–30м. Сосны и лиственные деревья различаются формой, высотой и оттенками. Стволы имеют коллизию; кроны не блокируют персонажа. Деревья сохраняют дороги и входы свободными. За прежней территорией добавлен грунт для леса; площадь всех больничных комнат не менялась.

## Двери и окна

Исправлена причина отсутствующих дверей в режиме осмотра: раньше открытая створка полностью скрывалась. Теперь двери имеют видимые распахнутые створки; широкие проёмы получили две створки. Есть смотровое стекло, металлические нажимные панели и защитная нижняя накладка. Закрытая дверь физически перекрывает проём. F, замки, внутренние засовы, открывание пациентами и восстановление состояния из сохранений сохранены.

Добавлено{inv['added_windows']} настоящих оконных проёмов в наружных зданиях, включая боковые фасады. {inv['secure_windows']} окон психиатрических корпусов получили защищённое остекление и наружные металлические решётки/экраны; в данных указан ограничитель8см. Открывание окон игроком в этом проходе не реализовывалось. Использованы официальные планировочные ориентиры NHS, ссылки вSOURCES.md.

## Новое наполнение

Добавлено{inv['room_kits']} полноразмерных комплектов в{inv['rooms_enriched']} закрытых помещениях, в том числе{inv['outside_building_kits']} в зданиях внешней зоны. Это прикроватные тумбы, ширмы, тележки документов, уборочные тележки, места посетителей, бытовые и питьевые станции, шкафчики, терапевтические полки, складские паллеты и торговые холодильники. Кухни, мастерские, склады, супермаркет, аптека и кафе наполнены по назначению. На свободных поверхностях добавлено{inv['surface_details']} комплектов мелких вещей. Все1030 запросов нового комнатного оборудования разместились без уменьшения; перед каждым свободный подход0,8×0,8м.

## Проверено

Независимые проверки сохранности, размеров, входов и маршрутов пройдены. В настоящей сборке —93/93 проверки мира,23/23 сюжета и сохранений,27/27 начальных проверок отрисовки и{len(images)} свежих кадров1920×1080. Персонаж действительно прошёл по поднятой галерее вRH2; проверено свободное пространство для проезда снизу, видимые створки и их реальное изменение положения при закрывании. Проверка оболочек: {sum(envelope['tested'].values())} проб,0 обнаруженных пропусков. Новые кадры и старые сохранённые камеры не заменяют ручной обход всех562 зон.

Новые модели и лес пока прототипные. Готовые фотореалистичные модели и завершённая лесная сюжетная часть не заявляются. Запуск PLAY_UNREAL.cmd; B — яркий осмотр, M — карта.
'''
(OUT/'REPORT.md').write_text(text,encoding='utf-8');print(json.dumps(dict(success=True,map=mh,executable=eh,views=len(images),new_views=len(new)),ensure_ascii=False))
