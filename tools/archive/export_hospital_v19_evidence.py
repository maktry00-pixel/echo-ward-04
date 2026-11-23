"""Publish evidence only from the same final native executable and map."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageStat
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v19'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=P/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source);exe=PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
assert d['review_revision']==19 and eh==sha(P/'Binaries/Win64/EchoWard.exe') and mh==sha(PK/'Windows/EchoWard/Content/CampaignData/hospital_world.json')
native=read(PK/'verified_world.json');visual=read(PK/'verified_worldvisual_v19.json');story=read(PK/'verified_story_v7.json');menu=read(PK/'verified_native_menu_v19.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for v in (visual,story,menu):assert v['success'] and v['executable_sha256'].lower()==eh and v['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(native['test_saved'])/'Prototype/every_door_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==117 and rendered['success'] and rendered['count']==43 and doors['success'] and doors['doors']==1012 and story['checks']==23 and menu['checks']==57 and envelope['success'] and sum(envelope['failed'].values())==0
audit=read(OUT/'atmosphere_acceptance.json');assert audit['success'] and audit['map_sha256']==mh and len(audit['checks'])==8
images=visual['screenshots'];menu_images=[]
for name in images:
    src=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/name
    with Image.open(src) as im:assert im.size==(1920,1080);im.verify()
    shutil.copy2(src,OUT/name)
for p in (Path(menu['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):
    shutil.copy2(p,OUT/('native_menu_'+p.name));menu_images.append('native_menu_'+p.name)
assert len(images)==64 and len(menu_images)==17
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
new=[n for n in images if 'v19' in n]
for prefix,names in [('contact',images),('atmosphere_contact',new),('menu_contact',menu_images)]:
    for start in range(0,len(names),20):
        group=names[start:start+20];canvas=Image.new('RGB',(1640,math.ceil(len(group)/4)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for j,name in enumerate(group):
            with Image.open(OUT/name) as src:im=src.convert('RGB');im.thumbnail((400,225))
            x=j%4*410;y=j//4*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name.removeprefix('world_')[:-4],font=font,fill='white')
        canvas.save(OUT/f'{prefix}_{start//20+1}.png')
regions=('1F','B1','B2','DG','IF','PS','RH','HS','SV','UT','ST','RF');pairs=[]
for start in range(0,len(regions),4):
    canvas=Image.new('RGB',(1600,4*475),'#10191b');draw=ImageDraw.Draw(canvas)
    for row,sid in enumerate(regions[start:start+4]):
        means=[]
        for col,mode in enumerate(('horror','bright')):
            with Image.open(OUT/f'world_v19_{mode}_{sid}.png') as src:
                im=src.convert('RGB');means.append(round(ImageStat.Stat(im.convert('L')).mean[0],2));im.thumbnail((800,450))
            canvas.paste(im,(col*800,row*475+25));draw.text((col*800+10,row*475+2),sid+' / '+mode,font=font,fill='white')
        pairs.append(dict(region=sid,horror_mean=means[0],bright_mean=means[1],note='Image statistics are diagnostics, not proof of path visibility.'))
    canvas.save(OUT/f'comparison_{start//4+1}.jpg',quality=93)
(OUT/'comparison_metrics.json').write_text(json.dumps(pairs,indent=2),encoding='utf8')
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
for j,(name,label) in enumerate([('world_v19_horror_1F.png','Приёмный корпус · холодный свет и фонарик'),('world_v19_horror_v18_scan_25.png','Дверной проём · красный аварийный свет'),('world_v19_horror_v18_scan_3.png','Интерьер · локальный свет и знакомая мебель'),('world_v19_horror_ST.png','Территория · лунный свет')]):
    with Image.open(OUT/name) as src:im=src.convert('RGB').resize((960,540))
    x=j%2*960;y=j//2*575;preview.paste(im,(x,y));draw.text((x+15,y+547),label,font=font,fill='#cee3df')
preview.save(OUT/'preview.jpg',quality=93)
for status,target,original in [(native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(native,'every_door_check','every_door_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check'),(menu,'native_menu_check','menu_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inv=read(OUT/'inventory.json')
limits=['Existing prototype geometry and simplified specialist equipment remain.','Normal gameplay uses the torch; dark corners deliberately remain dark.','Selected frames and automated checks do not constitute a manual tour of every detail or an FPS/stability benchmark.','Audio PCM, scheduling and existing cues verified with nosound native tests; no listening test through the users speakers.']
report=dict(success=True,review_revision=19,build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=117,story_checks=23,native_menu_checks=57,visual_initial_checks=43,atmosphere_checks=8,world_captures=len(images),menu_captures=len(menu_images),door_check=doors,envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],inventory=inv,screenshots=images,limits=limits)
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');(PK/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
report_text='''# ЭХО — хоррор-атмосфера v19

Обычная игра теперь начинает с мрачного освещения. Старый яркий режим переводится в хоррор один раз при первом запуске версии 19; последующие осознанные настройки игрока сохраняются. B включает яркий осмотр, N включает и выключает фонарик. Исправлен прежний конфликт B: эта клавиша больше не переключает фонарь одновременно и не открывает пустую панель меню поверх игры. Фонарь располагается у переднего плеча героя, а не за его головой; в новой игре он включён. Взаимодействие F и батарея вспышки T сохранены.

Ограниченные пятна холодного света, неработающие светильники, редкие короткие провалы одной электрической цепи, красные аварийные лампы, сдержанное зерно и виньетка. Вместо равномерного залива — локальные источники. У пола пять переиспользуемых объёмов тумана с медленным движением; в ярком осмотре они скрыты. Наружные участки освещены холодным лунным светом. Две ранние настройки были пересняты: первая получилась слишком чёрной, а затем проверка клавиш выявила лишнюю панель интерфейса. В финальных кадрах эти дефекты проверены повторно.

Для 35 локаций и галерей заданы отдельные профили света и звуков. Вентиляция и тихий фон преобладают над музыкой; раз в 24–42 секунды звучат трубы, скрипы, металл, радио или редкий шёпот. Источники расположены в существующих помещениях в 4,5–18 м от игрока. Эти атмосферные звуки не выдают игрока монстрам и не порождают новых противников. Прежние сюжет, существа, скример главного меню, сохранения, мебель, метры и архитектура сохранены.

Финальная нативная проверка мира: 117/117, в том числе реальные клавиши B/N и отсутствие лишнего меню; сюжет и сохранения: 23/23; меню и вход: 57/57; начальные проверки отрисовки: 43/43. Проверены все 1012 дверей и оболочки. Независимые 8 проверок подтверждают сохранность предыдущей карты, 36 профилей, границы эффектов и реальные PCM-файлы звуков. 64 кадра карты и 17 кадров меню сняты в 1920×1080, включая 12 пар одной игровой камеры в тёмном и ярком режимах. Финальные SHA перечислены в verification.json.

Это атмосферный проход по существующему прототипу. Оборудование и часть моделей всё ещё упрощены; тёмные углы оставлены намеренно. Съёмка с фиксированным шагом не измеряет игровой FPS. Автоматические проверки и выборочный просмотр не означают ручной обход каждой детали. Проверки звука выполнены по PCM и планировщику с отключённым аудиовыводом, без прослушивания через колонки пользователя.

Запуск: PLAY_UNREAL.cmd. N — фонарик; B — яркий осмотр; M — карта. preview.jpg и comparison_1–3.jpg — реальные кадры сборки.
'''
(OUT/'REPORT.md').write_text(report_text,encoding='utf8');print(json.dumps(dict(success=True,map=mh,executable=eh,views=len(images),menu_views=len(menu_images)),ensure_ascii=False))
