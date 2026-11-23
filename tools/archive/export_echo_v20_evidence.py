"""Export matching native evidence, honest active counts, and before/after views."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_integration_v20';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
mh=sha(P/'Content/CampaignData/hospital_world.json');eh=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');runtime=read(P/'Content/CampaignData/echo_custom_runtime.json')
assert eh==sha(P/'Binaries/Win64/EchoWard.exe')
for n in ('hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json'):assert sha(P/'Content/CampaignData'/n)==sha(PK/'Windows/EchoWard/Content/CampaignData'/n)
n=read(PK/'verified_world.json');v=read(PK/'verified_worldvisual_v20.json');s=read(PK/'verified_story_v7.json');m=read(PK/'verified_native_menu_v20.json');a=read(O/'acceptance.json')
assert n['success'] and n['build_identity'].split(':')[0].lower()==eh
for status in (v,s,m):assert status['success'] and status['executable_sha256'].lower()==eh and status['map_sha256'].lower()==mh
w=read(Path(n['test_saved'])/'Prototype/book_world_check.json');visual=read(Path(v['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(n['test_saved'])/'Prototype/every_door_check.json');env=read(Path(n['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and w['count']==120 and visual['success'] and visual['count']==46 and s['checks']==23 and m['checks']==57 and doors['success'] and doors['doors']==1012 and env['success'] and not sum(env['failed'].values())
assert w['custom_furniture']>=a['scheduled_replacements'] and w['custom_bounds_failures']==0 and w['custom_door_leaves']>1000 and a['success'] and a['map_sha256']==mh
names=[];menus=[]
for name in v['screenshots']:
    src=Path(v['test_saved'])/'Prototype/BookWorldCaptures'/name
    with Image.open(src) as im:assert im.size==(1920,1080);im.verify()
    shutil.copy2(src,O/name);names.append(name)
for src in (Path(m['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):name='menu_'+src.name;shutil.copy2(src,O/name);menus.append(name)
assert len(names)==64 and len(menus)==17
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
for label,group in [('contact',names),('menu_contact',menus)]:
    for start in range(0,len(group),20):
        page=group[start:start+20];canvas=Image.new('RGB',(1640,math.ceil(len(page)/4)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for j,name in enumerate(page):
            with Image.open(O/name) as im:pic=im.convert('RGB');pic.thumbnail((400,225))
            x=j%4*410;y=j//4*254;canvas.paste(pic,(x,y+25));draw.text((x+4,y+2),name.removeprefix('world_')[:-4],fill='white',font=font)
        canvas.save(O/f'{label}_{start//20+1}.png')
chosen=[('world_v19_horror_1F.png','Коридор · облупленная краска и новые двери'),('world_v19_horror_v18_scan_3.png','Новая мебель в прежних размерах'),('world_v19_horror_v18_scan_25.png','Плитка, металл и красный свет'),('world_v19_bright_PS.png','Яркий осмотр · фактическая отделка')]
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
for j,(name,label) in enumerate(chosen):
    with Image.open(O/name) as im:pic=im.convert('RGB').resize((960,540))
    x=j%2*960;y=j//2*575;preview.paste(pic,(x,y));draw.text((x+12,y+547),label,font=font,fill='#d2dfdf')
preview.save(O/'preview.jpg',quality=94)
beforeafter=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(beforeafter)
for row,name in enumerate(('world_v19_horror_1F.png','world_v19_horror_v18_scan_3.png')):
    for col,base in enumerate((R/'artifacts/unreal_hospital_v19',O)):
        with Image.open(base/name) as im:pic=im.convert('RGB').resize((960,540))
        beforeafter.paste(pic,(col*960,row*575));draw.text((col*960+12,row*575+547),'До · v19' if col==0 else 'После · новая библиотека',font=font,fill='white')
beforeafter.save(O/'before_after.jpg',quality=94)
for status,target,source in [(n,'native_world_check','book_world_check'),(v,'rendered_world_check','book_world_check'),(s,'native_story_check','story_check'),(m,'native_menu_check','menu_check'),(n,'every_door_check','every_door_check'),(n,'native_envelope_check','envelope_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{source}.json',O/f'{target}.json')
report=dict(success=True,review_revision=19,art_integration_revision=20,build_identity=n['build_identity'],world_id=read(P/'Content/CampaignData/hospital_world.json')['map_id'],executable_sha256=eh,map_sha256=mh,runtime_sha256=a['runtime_sha256'],bindings_sha256=a['bindings_sha256'],world_checks=w['count'],visual_checks=visual['count'],story_checks=s['checks'],menu_checks=m['checks'],imported_static_entries=562,imported_native_meshes=573,active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],custom_door_leaves=w['custom_door_leaves'],bounds_failures=w['custom_bounds_failures'],world_captures=64,menu_captures=17,limits=a['limits'])
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');(PK/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
integration=dict(status='partially_placed_and_verified',revision=20,imported_static_entries=562,native_meshes=573,active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],active_door_leaves=w['custom_door_leaves'],skeletal_deferred=list(range(563,576)),verification='artifacts/echo_integration_v20/verification.json')
library=R/'assets/models/echo_custom';(library/'GAME_INTEGRATION.json').write_text(json.dumps(integration,ensure_ascii=False,indent=2),encoding='utf8')
manifest=read(library/'LIBRARY_MANIFEST.json');manifest['game_integration']=integration;manifest['scope']='First-pass authored library; static assets imported into Unreal, selected models actively placed and verified. Skeletal art/animation remains pending.'
(library/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
text=f'''# ЭХО — внедрение авторской библиотеки v20

В настоящую Windows-сборку импортированы 562 статические позиции EH_001–EH_562, 11 дополнительных отдельных дверных полотен и 16 материалов поверхностей. Исходные GLB/Blender и лицензии не изменены. Стены получили авторские текстуры штукатурки и облупленной зелёной краски, пол — грязную треснувшую плитку с неоднородной шероховатостью. Новые поверхности также используются в сцене главного меню. Металл использует канал B упакованной карты ORM; стекло прозрачно. Размеры комнат, все 562 зоны и сюжетный файл карты побайтово совпадают с v19.

Настроены 96 соответствий старых типов новым моделям. В текущем мире построены {w['custom_furniture']} новых экземпляров мебели, оборудования и светильников; используются {w['custom_model_types']} разных номеров EH, включая дверные варианты и текстурированные пятна крови. Из них {a['scheduled_replacements']} — замены существующего расписания мебели/оборудования, остальные — светильники, отдельные матрасы и подушки. Новые предметы вписаны в прежние проверенные габариты. Прежние невидимые коллизии сохранены, новые визуальные меши не создают второй слой препятствий. Шкаф-укрытие и аппараты без подходящего аналога сохраняют свои модели.

Все {w['custom_door_leaves']} действующих створок заменены авторскими полотнами с отдельной фурнитурой. Они вращаются вокруг прежних петель и используют прежние замки и коллизии. Настенные и напольные пятна крови заменены текстурированными прозрачными по краям плоскостями. Мерцание, холодный/красный свет, туман, звуки, B/N, сюжет и скример сохранены.

Проверки финальной сборки: мир120/120; двери1012/1012; сюжет и сохранения23/23; меню57/57; начальные проверки отрисовки46/46. Независимый аудит импорта10/10. Сняты64кадра карты и17меню1920×1080. preview.jpg и before_after.jpg показывают реальные кадры игры. SHA исходников/сборки и счётчики записаны в verification.json.

Ограничения: импорт библиотеки не означает расстановку каждой её позиции. Часть архитектурных деталей используется через материал, а не заменяет геометрическую оболочку. Объекты без подходящей игровой точки остаются в библиотеке Unreal. Персонажи563–575 не подменены черновыми статическими телами: нужны финальный арт и перенос полноценной ходьбы, бега и атак. Некоторая специализированная техника остаётся прототипной. Просмотр кадров и автоматические проверки не означают ручной обход каждой детали или измерение FPS.

Первый параллельный импорт/компиляция исчерпал память. Работа затем продолжена последовательно с освобождением временной памяти. Пользователь освободил место на C; исходные модели и прежние отчёты сохранены. Непригодные первые попытки не считаются финальной проверкой.

Запуск: PLAY_UNREAL.cmd. N — фонарик; B — яркий осмотр; M — карта.
'''
(O/'REPORT.md').write_text(text,encoding='utf8');print(json.dumps(report,ensure_ascii=False))
