"""Export only native, matching v26 game evidence and its measured limits."""
from pathlib import Path
import datetime,hashlib,json,shutil
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_detail_v26';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
old=read(O/'before_identity.json');data=read(P/'Content/CampaignData/echo_detail_v26.json');audit=read(O/'acceptance.json');assert audit['success']
exe=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');assert exe==sha(P/'Binaries/Win64/EchoWard.exe')
for n,h in old['unchanged_data_hashes'].items():assert sha(P/'Content/CampaignData'/n)==h and sha(PK/'Windows/EchoWard/Content/CampaignData'/n)==h,n
assert sha(P/'Content/CampaignData/echo_detail_v26.json')==sha(PK/'Windows/EchoWard/Content/CampaignData/echo_detail_v26.json')
statuses={n:read(PK/n) for n in ['verified_world.json','verified_worldvisual_v26.json','verified_storyvisual_v7.json','verified_native_menu_v23.json']}
for name,s in statuses.items():assert s['success'] and s.get('executable_sha256',s.get('build_identity','').split(':')[0]).lower()==exe,name
n=statuses['verified_world.json'];v=statuses['verified_worldvisual_v26.json'];s=statuses['verified_storyvisual_v7.json'];m=statuses['verified_native_menu_v23.json']
w=read(Path(n['test_saved'])/'Prototype/book_world_check.json');visual=read(Path(v['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(n['test_saved'])/'Prototype/every_door_check.json');envelope=read(Path(n['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and w['count']==132 and w['rooms']==562 and w['locations']==35 and w['detail_props']==data['counts']['total']
assert visual['success'] and visual['count']==58 and s['checks']==25 and m['checks']==57
assert w['custom_bounds_failures']==w['unresolved_custom_props']==0 and doors['success'] and doors['doors']==1012 and envelope['success']
assert w['real_props']==16107 and w['quality_props']==28850 and w['quality_trees']==3940 and w['quality_windows']==836
assert w['custom_furniture']==old['active_custom_instances']+w['detail_props']
perf=read(P/'Saved/PackageTests/PerformanceV25_v26/Saved/Prototype/hospital_perf_v26.json');assert perf['success'] and perf['screen_percentage']==100 and perf['detail_instances']==w['detail_props'] and perf['executable_sha256']==exe
assert perf['detail_data_sha256']==sha(P/'Content/CampaignData/echo_detail_v26.json')
previous=read(R/'artifacts/echo_optimization_v25/hospital_perf_repeat.json');comparison=[]
for b,a in zip(previous['views'],perf['views']):
 assert b['view']==a['view']
 if a['view']=='corridor_horror':continue
 comparison.append(dict(view=a['view'],before_fps=b['fps'],after_fps=a['fps'],before_p95_ms=b['p95_ms'],after_p95_ms=a['p95_ms'],before_game_ms=b['game_ms'],after_game_ms=a['game_ms'],before_ram_mb=b['process_physical_mb'],after_ram_mb=a['process_physical_mb']))
assert len(comparison)==4
captures=O/'captures';captures.mkdir(exist_ok=True)
for name in v['screenshots']:shutil.copy2(Path(v['test_saved'])/'Prototype/BookWorldCaptures'/name,captures/name)
menus=O/'menu';menus.mkdir(exist_ok=True)
for f in (Path(m['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):shutil.copy2(f,menus/f.name)
shutil.copy2(Path(s['test_saved'])/'Prototype/story_v7.png',O/'story_mob.png')
for state,target,source in [(n,'native_world_check','book_world_check'),(v,'rendered_world_check','book_world_check'),(s,'native_story_check','story_check'),(m,'native_menu_check','menu_check'),(n,'every_door_check','every_door_check'),(n,'native_envelope_check','envelope_check')]:shutil.copy2(Path(state['test_saved'])/f'Prototype/{source}.json',O/f'{target}.json')
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
def contacts(folder,prefix):
 files=sorted(folder.glob('*.png'))
 for start in range(0,len(files),20):
  page=Image.new('RGB',(1600,((min(20,len(files)-start)+3)//4)*250),'#10191b');draw=ImageDraw.Draw(page)
  for j,path in enumerate(files[start:start+20]):
   with Image.open(path) as im:assert im.size==(1920,1080);pic=im.convert('RGB').resize((400,225))
   x=j%4*400;y=j//4*250;page.paste(pic,(x,y));draw.text((x+4,y+227),path.stem[:42],font=font,fill='white')
  page.save(O/f'{prefix}_{start//20+1}.jpg',quality=93)
contacts(captures,'world_contact');contacts(menus,'menu_contact')
review=O/'new_details';review.mkdir(exist_ok=True)
for f in captures.glob('world_v26_*.png'):shutil.copy2(f,review/f.name)
contacts(review,'new_detail_contact')
names=['world_v23_court_S01.png','world_v23_lab_clutter.png','world_v19_horror_1F.png','world_v23_detail_126.png'];sheet=Image.new('RGB',(1600,len(names)*475),'#10191b');draw=ImageDraw.Draw(sheet)
for i,name in enumerate(names):
 for j,path in enumerate([R/'artifacts/echo_optimization_v25/captures'/name,captures/name]):
  with Image.open(path) as im:pic=im.convert('RGB').resize((800,450))
  sheet.paste(pic,(j*800,i*475));draw.text((j*800+10,i*475+452),('v25' if j==0 else 'v26')+' / '+name,font=font,fill='white')
sheet.save(O/'before_after.jpg',quality=94)
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
for i,(name,label) in enumerate([('world_v23_court_S01.png','Ночные фасады'),('world_v23_lab_clutter.png','Поверхности и интерьер'),('world_v26_medical_power_bright.png','Электрика и фурнитура'),('world_v26_exterior_air_conditioner_bright.png','Наружные технические узлы')]):
 with Image.open(captures/name) as im:pic=im.convert('RGB').resize((960,540))
 x=i%2*960;y=i//2*575;preview.paste(pic,(x,y));draw.text((x+10,y+545),label,font=font,fill='white')
preview.save(O/'preview.jpg',quality=94)
for suffix in ['json','csv']:shutil.copy2(P/f'Saved/PackageTests/PerformanceV25_v26/Saved/Prototype/hospital_perf_v26.{suffix}',O/f'performance.{suffix}')
report=dict(old,success=True,art_integration_revision=26,detail_revision=26,checked_at=datetime.datetime.now().astimezone().isoformat(),build_identity=n['build_identity'],executable_sha256=exe,world_checks=w['count'],visual_checks=visual['count'],story_checks=s['checks'],menu_checks=m['checks'],detail_props=w['detail_props'],detail_categories=data['counts'],detail_data_sha256=sha(P/'Content/CampaignData/echo_detail_v26.json'),active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],world_captures=len(v['screenshots']),menu_captures=len(list(menus.glob('*.png'))),performance=comparison,performance_scope='Four identical stationary 1920x1080 views at 100%; story AI frozen only in benchmark. One v26 run versus earlier v25 run; approximate comparison, not combat or long-duration stress testing.')
shutil.copy2(P/'Saved/Prototype/detail_v26_materials.json',O/'material_preparation.json')
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
lib=R/'assets/models/echo_custom';integration=read(lib/'GAME_INTEGRATION.json');integration.update(revision=26,detail_revision=26,detail_props=w['detail_props'],active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],verification='artifacts/echo_detail_v26/verification.json');(lib/'GAME_INTEGRATION.json').write_text(json.dumps(integration,ensure_ascii=False,indent=2),encoding='utf8');manifest=read(lib/'LIBRARY_MANIFEST.json');manifest['game_integration']=integration;(lib/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
labels={'v23_court_S01':'Парковка','v23_lab_clutter':'Лаборатория','v23_detail_126':'Комната персонала','v23_court_S06':'Внешний КПП'}
lines=['# ЭХО — дополнительные детали и материалы v26','',f'Добавлено {w["detail_props"]:,} нативных 3D-деталей отдельным слоем. Геометрия всех 562 зон и 35 локаций, сюжет, прежние предметы и исходные модели сохранены. Оптимизации v25 продолжают работать. Новые детали — из существующей библиотеки; новых внешних моделей в этом проходе не скачивали.','','Появились выключатели и розетки, медицинские блоки питания, вентиляционные решётки, распределительные коробки, рамки эвакуационных планов, телефоны, манометры, доводчики над дверями, металлические пороги, защитные накладки, оконные ручки и наружные блоки кондиционеров. Габариты проверены, декор не создаёт новых препятствий на прежних маршрутах.','','Новые материалы штукатурки, приглушённой синевато-серой краски, пола и фасада используют существующие лицензированные исходники Poly Haven: цвет 4K, нормали и шероховатость 2K. Есть мировой масштаб текстуры, поверхностные неровности, грязь у основания стен и неоднородная влажность пола. Плиты пола имеют швы и рельеф кромок с шагом 60 см. Для плоской архитектуры выборка текстур выполняется по основной оси. У мелкой новой фурнитуры сокращены динамические тени; тени прежних групп и крупных объектов сохранены. Главное меню сохранено. Это улучшение существующего окружения, не полная замена всей библиотеки и не обещание фотографического качества.','','Источники текстур: assets/textures/polyhaven/HD_SOURCE_MANIFEST.json (CC0). Ресурсы игры локальные.','','| Участок | v25, FPS | v26, FPS | v25, p95 мс | v26, p95 мс |','|---|---:|---:|---:|---:|']
for c in comparison:lines.append(f'| {labels[c["view"]]} | {c["before_fps"]:.1f} | {c["after_fps"]:.1f} | {c["before_p95_ms"]:.1f} | {c["after_p95_ms"]:.1f} |')
lines+=['','1080p, масштаб 100%, короткие неподвижные выборки. Разница включает разброс отдельных запусков ноутбука. Пятый диагностический ракурс в стену исключён. Боевой FPS и длительная стабильность не измерялись.','','Проверки итогового пакета: мир 132/132, отрисовка 58/58, бой/сохранения 25/25, меню 57/57, все 1012 дверей; габариты/отсутствующие модели — 0 ошибок. before_after.jpg и captures/ — реальные изображения. Количество по категориям и контрольная сумма EXE — verification.json.','','Запуск: D:\\Projects\\ECHO\\PLAY_UNREAL.cmd. Все тесты выполнялись в отдельных профилях.']
(O/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf8');marker=PK/'package_verified.tmp';marker.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');marker.replace(PK/'package_verified.json');print(json.dumps(dict(success=True,executable_sha256=exe,detail_props=w['detail_props'],performance=comparison),ensure_ascii=False))
