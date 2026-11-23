"""Export only evidence matching the final canonical data and executable."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v18'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=P/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source);exe=PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
assert d['review_revision']==18 and eh==sha(P/'Binaries/Win64/EchoWard.exe') and mh==sha(PK/'Windows/EchoWard/Content/CampaignData/hospital_world.json')
native=read(PK/'verified_world.json');visual=read(PK/'verified_worldvisual_v18.json');story=read(PK/'verified_story_v7.json');menu=read(PK/'verified_native_menu_v18.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for v in (visual,story,menu):assert v['success'] and v['executable_sha256'].lower()==eh and v['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(native['test_saved'])/'Prototype/every_door_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==108 and rendered['success'] and rendered['count']==34 and doors['success'] and doors['doors']==1012 and story['checks']==23 and menu['checks']==57 and envelope['success'] and sum(envelope['failed'].values())==0
for name,key in [('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('realism_acceptance','map_sha256')]:
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh
images=visual['screenshots'];menu_images=[]
for name in images:
    src=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/name
    with Image.open(src) as im:assert im.size==(1920,1080);im.verify()
    shutil.copy2(src,OUT/name)
for p in (Path(menu['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):
    shutil.copy2(p,OUT/('native_menu_'+p.name));menu_images.append('native_menu_'+p.name)
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17);new=[n for n in images if 'v18' in n]
assert len(images)==76 and len(new)==41 and len(menu_images)==17
for prefix,names in [('contact',images),('realism_contact',new),('menu_contact',menu_images)]:
    for start in range(0,len(names),20):
        group=names[start:start+20];canvas=Image.new('RGB',(1640,math.ceil(len(group)/4)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for j,name in enumerate(group):
            with Image.open(OUT/name) as src:im=src.convert('RGB');im.thumbnail((400,225))
            x=j%4*410;y=j//4*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name.removeprefix('world_')[:-4],font=font,fill='white')
        canvas.save(OUT/f'{prefix}_{start//20+1}.png')
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
for j,(name,label) in enumerate([('world_v18_scan_3.png','Текстурированная кровать и металлический стеллаж'),('world_v18_scan_25.png','Стулья и мебель в прежних габаритах'),('world_campus_v18_room_1F.png','Штукатурка, окрашенная стена и покрытие пола'),('world_campus_v18_room_X01F2.png','Материалы и фурнитура внешних зданий')]):
    with Image.open(OUT/name) as src:im=src.convert('RGB').resize((960,540))
    x=j%2*960;y=j//2*575;preview.paste(im,(x,y));draw.text((x+15,y+547),label,font=font,fill='#cee3df')
preview.save(OUT/'preview.jpg',quality=92)
for status,target,original in [(native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(native,'every_door_check','every_door_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check'),(menu,'native_menu_check','menu_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inv=read(OUT/'inventory.json')
report=dict(success=True,review_revision=18,build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=108,story_checks=23,native_menu_checks=57,visual_initial_checks=34,world_captures=len(images),menu_captures=len(menu_images),door_check=doors,envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],inventory=inv,screenshots=images,limits=['Four imported scanned furniture types replace five prototype model IDs; other furniture and equipment remain simplified.','Original collision, bed hiding spaces, doors, story and room metrics retained.','Automated full-map checks and selected images do not constitute a manual inspection of every detail.','Some room viewpoints remain partly occluded; fixed-step captures do not measure gameplay FPS.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');(PK/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
text=f'''# ЭХО — повторная проверка и реалистичные материалы v18

В настоящей игре заменено {inv['scanned_furniture']} предметов: 381 каркас кровати, 846 стульев, 941 металлический стеллаж и 229 прикроватных тумб. Используются ранее импортированные текстурированные модели Poly Haven. Их оси согласованы с планом, габариты вписаны в прежние размеры. Матрасы и предметы на полках сохранены. Невидимые оригинальные части сохраняют проверенную коллизию и пространство для укрытия под койкой; новая модель не расширяет препятствие.

Исправлена ошибка назначения материалов: параметр исходной поверхности в генераторе карты раньше игнорировался. Стены и пол теперь используют существующие фотографические текстуры и нормали в мировом масштабе. При первой съёмке обнаружено устаревшее наследование материала пола; финальная версия явно назначает правильные карты цвета, нормали и шероховатости. Текстура терраццо подтверждена на кадрах. Дерево отделено от эмали и металла; масштаб волокон исправлен после обнаружения слишком частого клетчатого повтора на столешницах.

836 наружных окон получили отливы и обрамления. У 1939 потолочных светильников добавлены корпуса, поперечные планки и подвесы до потолка. На 95 шкафах добавлены щели ящиков, карманы этикеток и металлические накладки. Это {inv['new_parts']} новых декоративных частей. Размеры зданий, 562 зоны, 35 локаций, лестницы, переходы, лес, прежняя мебель и сюжетные предметы сохранены.

Проверено в финальной сборке: мир 108/108, включая все 2397 заменённых предметов и их габариты; двери 1012/1012; сюжет и сохранения 23/23; меню и вход 57/57. Начальные проверки отрисовки 34/34. Все пробы оболочек прошли без пропусков. Независимые проверки карты, мебели, сюжета и нового слоя прошли на том же исходном файле. Снято 76 кадров карты и 17 меню в 1920×1080. Оба поста охраны повторно пройдены вверх и вниз с сохранением/загрузкой на площадках.

Служебные камеры для пяти типов мебели теперь выбираются с проверкой свободного положения и луча обзора: первая версия смотрела в перегородки. Часть старых общих ракурсов всё ещё может быть частично закрыта. При параллельном запуске нескольких тяжёлых проверок Python не хватило памяти; независимые проверки карты и сюжета затем успешно повторены последовательно. Эти проверки не измеряют стабильность длительной игровой сессии или FPS.

Это улучшение текущего прототипа, а не завершённый фотореализм всех зданий и предметов: специализированная техника, матрасы, часть мебели, деревья и отделка пока упрощённые. Новые строительные детали декоративные. Проверки и просмотр выбранных кадров не означают ручной обход каждой детали. Источники перечислены в SOURCES.md.

Запуск: PLAY_UNREAL.cmd. B — яркий осмотр, M — карта, F — взаимодействие, K — засов убежища изнутри. preview.jpg показывает реальные кадры финальной сборки в ярком режиме.
'''
(OUT/'REPORT.md').write_text(text,encoding='utf8');print(json.dumps(dict(success=True,map=mh,executable=eh,views=len(images),new_views=len(new),menu_views=len(menu_images)),ensure_ascii=False))
