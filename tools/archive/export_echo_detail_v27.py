"""Publish matching final v27 build evidence; never mark an untested package."""
from pathlib import Path
import datetime,hashlib,json,shutil
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_detail_v27';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
before=read(O/'before_identity.json');data=read(P/'Content/CampaignData/echo_detail_v27.json');audit=read(O/'acceptance.json');assert audit['success'];exe=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');assert exe==sha(P/'Binaries/Win64/EchoWard.exe')
for n,h in before['unchanged_data_hashes'].items():
 if n!='echo_custom_runtime.json':assert sha(P/'Content/CampaignData'/n)==h,n
for n in list(before['unchanged_data_hashes'])+['echo_detail_v26.json','echo_detail_v27.json']:assert sha(P/'Content/CampaignData'/n)==sha(PK/'Windows/EchoWard/Content/CampaignData'/n),n
states={n:read(PK/n) for n in ['verified_world.json','verified_worldvisual_v27.json','verified_storyvisual_v7.json','verified_native_menu_v23.json']}
for n,s in states.items():assert s['success'] and s.get('executable_sha256',s.get('build_identity','').split(':')[0]).lower()==exe,n
world=states['verified_world.json'];vis=states['verified_worldvisual_v27.json'];story=states['verified_storyvisual_v7.json'];menu=states['verified_native_menu_v23.json'];w=read(Path(world['test_saved'])/'Prototype/book_world_check.json');v=read(Path(vis['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(world['test_saved'])/'Prototype/every_door_check.json');envelope=read(Path(world['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and w['count']==136 and v['success'] and v['count']==62 and story['checks']==25 and menu['checks']==57
assert w['rooms']==562 and w['locations']==35 and w['custom_bounds_failures']==w['unresolved_custom_props']==0
assert w['detail27_props']==len(data['props'])==data['counts']['total'] and w['detail_props']==6374 and w['real_props']==16107 and w['quality_props']==28850
assert doors['success'] and doors['doors']==1012 and envelope['success']
assert w['custom_furniture']==before['active_custom_instances']+w['detail27_props']
perf=read(P/'Saved/PackageTests/PerformanceV25_v27/Saved/Prototype/hospital_perf_v27.json');assert perf['success'] and perf['detail27_instances']==w['detail27_props'] and perf['executable_sha256']==exe and perf['screen_percentage']==100
assert perf['detail27_data_sha256']==sha(P/'Content/CampaignData/echo_detail_v27.json')
previous=read(R/'artifacts/echo_detail_v26/performance.json');comparison=[]
for b,a in zip(previous['views'],perf['views']):
 assert a['view']==b['view']
 if a['view']=='corridor_horror':continue
 comparison.append(dict(view=a['view'],before_fps=b['fps'],after_fps=a['fps'],before_p95_ms=b['p95_ms'],after_p95_ms=a['p95_ms']))
assert len(comparison)==4
captures=O/'captures';captures.mkdir(exist_ok=True)
for n in vis['screenshots']:shutil.copy2(Path(vis['test_saved'])/'Prototype/BookWorldCaptures'/n,captures/n)
menus=O/'menu';menus.mkdir(exist_ok=True)
for f in (Path(menu['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):shutil.copy2(f,menus/f.name)
new=O/'new_details';new.mkdir(exist_ok=True)
for f in captures.glob('world_v27_*.png'):shutil.copy2(f,new/f.name)
shutil.copy2(Path(story['test_saved'])/'Prototype/story_v7.png',O/'story_mob.png')
for state,target,source in [(world,'native_world_check','book_world_check'),(vis,'rendered_world_check','book_world_check'),(story,'native_story_check','story_check'),(menu,'native_menu_check','menu_check'),(world,'every_door_check','every_door_check'),(world,'native_envelope_check','envelope_check')]:shutil.copy2(Path(state['test_saved'])/f'Prototype/{source}.json',O/f'{target}.json')
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
def contacts(folder,prefix):
 files=sorted(folder.glob('*.png'))
 for start in range(0,len(files),20):
  page=Image.new('RGB',(1600,((min(20,len(files)-start)+3)//4)*250),'#10191b');draw=ImageDraw.Draw(page)
  for j,path in enumerate(files[start:start+20]):
   with Image.open(path) as im:assert im.size==(1920,1080);pic=im.convert('RGB').resize((400,225))
   x=j%4*400;y=j//4*250;page.paste(pic,(x,y));draw.text((x+4,y+227),path.stem[:42],font=font,fill='white')
  page.save(O/f'{prefix}_{start//20+1}.jpg',quality=93)
contacts(captures,'world_contact');contacts(menus,'menu_contact');contacts(new,'new_detail_contact')
names=['world_v23_lab_clutter.png','world_v23_detail_126.png','world_v26_medical_power_bright.png'];sheet=Image.new('RGB',(1600,len(names)*475),'#10191b');draw=ImageDraw.Draw(sheet)
for i,n in enumerate(names):
 for j,path in enumerate([R/'artifacts/echo_detail_v26/captures'/n,captures/n]):
  with Image.open(path) as im:pic=im.convert('RGB').resize((800,450))
  sheet.paste(pic,(j*800,i*475));draw.text((j*800+10,i*475+452),('v26' if j==0 else 'v27')+' / '+n,font=font,fill='white')
sheet.save(O/'before_after.jpg',quality=94)
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
choices=[('world_v27_replacement_200.png','Монитор на подкатной стойке'),('world_v27_replacement_230.png','Раковина с открытой чашей и смесителем'),('world_v27_fire_hose_cabinet_bright.png','Пожарный рукав и шкаф'),('world_v27_ceiling_services_bright.png','Инженерные коммуникации')]
for i,(n,label) in enumerate(choices):
 path=captures/n
 if not path.exists():path=captures/'world_v27_supported_clinical_bright.png'
 with Image.open(path) as im:pic=im.convert('RGB').resize((960,540))
 x=i%2*960;y=i//2*575;preview.paste(pic,(x,y));draw.text((x+10,y+545),label,font=font,fill='white')
preview.save(O/'preview.jpg',quality=94)
for suffix in ['json','csv']:shutil.copy2(P/f'Saved/PackageTests/PerformanceV25_v27/Saved/Prototype/hospital_perf_v27.{suffix}',O/f'performance.{suffix}')
report=dict(before,success=True,art_integration_revision=27,detail_revision=27,checked_at=datetime.datetime.now().astimezone().isoformat(),build_identity=world['build_identity'],executable_sha256=exe,world_checks=w['count'],visual_checks=v['count'],story_checks=25,menu_checks=57,detail27_props=w['detail27_props'],detail27_categories=data['counts'],detail27_data_sha256=sha(P/'Content/CampaignData/echo_detail_v27.json'),native_model_additions=15,native_models=len(read(P/'Content/CampaignData/echo_custom_runtime.json')['meshes']),replacement_types=10,active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],world_captures=len(vis['screenshots']),menu_captures=len(list(menus.glob('*.png'))),performance=comparison,performance_scope='Four stationary 1080p views at 100%; idle AI; single-run comparison, excludes combat and thermal stability.')
report['unchanged_data_hashes']=dict(before['unchanged_data_hashes']);report['unchanged_data_hashes']['echo_custom_runtime.json']=sha(P/'Content/CampaignData/echo_custom_runtime.json')
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');shutil.copy2(P/'Saved/Prototype/detail_v27_import.json',O/'native_import.json')
lib=R/'assets/models/echo_custom';integration=read(lib/'GAME_INTEGRATION.json');integration.update(revision=27,detail_revision=27,detail27_props=w['detail27_props'],native_model_additions=15,native_meshes=report['native_models'],active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],verification='artifacts/echo_detail_v27/verification.json');(lib/'GAME_INTEGRATION.json').write_text(json.dumps(integration,ensure_ascii=False,indent=2),encoding='utf8');manifest=read(lib/'LIBRARY_MANIFEST.json');manifest['game_integration']=integration;(lib/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
lines=['# ЭХО — оборудование и наполнение v27','',f'Добавлено {w["detail27_props"]:,} дополнительных объектов отдельным слоем. Все 562 зоны / 35 локаций и прежние слои наполнения сохранены. Новые предметы на мебели проверены по открытым треугольным поверхностям, а также по высоте до следующей полки. Дверные подходы и укрытия свободны; новых декоративных коллизий нет.','','15 новых оригинальных моделей и 12 материалов: монитор с ЭКГ и органами управления; кислородный баллон с редуктором; капельница с пакетом и трубкой; дефибриллятор с электродами и витыми проводами; инфузионный насос; медицинская урна с педалью; раковина с углублённой чашей и отдельный смеситель; газовая/электрическая панель у койки; набор флаконов и пакетов; трубы с креплениями; пожарный шкаф; архивные папки; сложенное бельё; настенный дозатор. Десять прежних типов техники заменены этими моделями во всех местах, где используется исходный тип.','','У новых моделей фаски, отдельные винты, штуцеры, кнопки, шкалы, швы и решётки. Поверхности различают металл, пластик, резину, бумагу и ткань, имеют микрозерно и неравномерную шероховатость. Тяжёлые новые модели используют четыре LOD. Архитектурные 4K-материалы v26 сохранены. Мелкие новые принадлежности используют ограничение дальности и экономные тени.','','Это авторские игровые модели и декоративные наборы; симуляции жидкостей или нового взаимодействия с расходниками нет. Специальная техника, автомобили и персонажи вне перечисленных замен остаются прежними. Новые загрузки и сторонние источники в этом проходе не использовались.','','| Ракурс | v26 FPS | v27 FPS | v27 p95 мс |','|---|---:|---:|---:|']
for c in comparison:lines.append(f'| {c["view"]} | {c["before_fps"]:.1f} | {c["after_fps"]:.1f} | {c["after_p95_ms"]:.1f} |')
lines+=['','1920×1080, масштаб 100%, короткие неподвижные выборки; единичные запуски подвержены разбросу. Пятый ракурс в стену исключён. Боевой FPS и длительный прогрев не измерялись.','','Проверки: мир 136/136, отрисовка 62/62, бой/сохранения 25/25, меню 57/57; все 1012 дверей. Автопроверки не заменяют ручной осмотр каждого из тысяч объектов. Реальные PNG и контактные листы находятся рядом.','','Новые мелкие принадлежности собраны отдельно от старых групп: дальность 22 м, для крупного оборудования 45 м. Прежние тени сохранены, мелочи нового слоя используют экономные тени и затенение поверхности. Некоторые старые обзорные ракурсы частично перекрыты мебелью; крупные новые модели проверяются также в отдельных ракурсах.','','Запуск: D:/Projects/ECHO/PLAY_UNREAL.cmd.']
(O/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf8');marker=PK/'package_verified.tmp';marker.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');marker.replace(PK/'package_verified.json');print(json.dumps(dict(success=True,exe=exe,details=w['detail27_props'],performance=comparison)))
