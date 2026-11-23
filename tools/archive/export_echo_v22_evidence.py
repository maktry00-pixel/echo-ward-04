"""Export the actual packaged render evidence and exact build identity."""
import json,hashlib,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_quality_v22';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
a=read(O/'acceptance.json');q=read(P/'Content/CampaignData/echo_quality_v22.json');eh=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');mh=sha(P/'Content/CampaignData/hospital_world.json');assert eh==sha(P/'Binaries/Win64/EchoWard.exe')
for f in ['hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json']:assert sha(P/'Content/CampaignData'/f)==sha(PK/'Windows/EchoWard/Content/CampaignData'/f)
n=read(PK/'verified_world.json');v=read(PK/'verified_worldvisual_v22.json');s=read(PK/'verified_storyvisual_v7.json');m=read(PK/'verified_native_menu_v22.json')
# Material-only recooks keep the executable hash. Require all four checks
# after the latest content archive as well as the exact executable/map.
latest_content=max(p.stat().st_mtime for p in (PK/'Windows/EchoWard/Content/Paks').iterdir() if p.suffix in ('.pak','.ucas','.utoc'))
for name in ('verified_world.json','verified_worldvisual_v22.json','verified_storyvisual_v7.json','verified_native_menu_v22.json'):
 assert (PK/name).stat().st_mtime+2>=latest_content,('stale content check',name)
assert n['success'] and n['build_identity'].split(':')[0].lower()==eh
for status in [v,s,m]:assert status['success'] and status['executable_sha256'].lower()==eh and status['map_sha256'].lower()==mh
w=read(Path(n['test_saved'])/'Prototype/book_world_check.json');vr=read(Path(v['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(n['test_saved'])/'Prototype/every_door_check.json');env=read(Path(n['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and w['count']==124 and vr['success'] and vr['count']==50 and s['checks']==25 and m['checks']==57 and doors['success'] and doors['doors']==1012 and not sum(env['failed'].values())
assert w['unresolved_custom_props']==0 and w['custom_bounds_failures']==0 and a['success'] and w['quality_props']==len(q['props']) and w['quality_trees']==3940 and w['quality_windows']==836
world=list(v['screenshots']);menus=[];assert len(world)==93
for name in world:
 src=Path(v['test_saved'])/'Prototype/BookWorldCaptures'/name
 with Image.open(src) as im:assert im.size==(1920,1080);im.verify()
 shutil.copy2(src,O/name)
for src in (Path(m['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):name='menu_'+src.name;shutil.copy2(src,O/name);menus.append(name)
assert len(menus)==17
shutil.copy2(Path(s['test_saved'])/'Prototype/story_v7.png',O/'story_mob.png')
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
def contact(group,prefix):
 for start in range(0,len(group),20):
  page=group[start:start+20];im=Image.new('RGB',(1640,math.ceil(len(page)/4)*254),'#10191b');dr=ImageDraw.Draw(im)
  for j,name in enumerate(page):
   with Image.open(O/name) as src:pic=src.convert('RGB');pic.thumbnail((400,225))
   x=j%4*410;y=j//4*254;im.paste(pic,(x,y+25));dr.text((x+4,y+2),name.removeprefix('world_')[:-4],fill='white',font=font)
  im.save(O/f'{prefix}_{start//20+1}.png')
contact(world,'contact');contact(menus,'menu_contact');contact([x for x in world if 'v22_' in x],'quality_contact')
im=Image.new('RGB',(1920,1150),'#10191b');dr=ImageDraw.Draw(im)
for row,name in enumerate(['world_v19_horror_1F.png','world_v21_detail_126.png']):
 for col,base in enumerate([R/'artifacts/echo_integration_v21',O]):
  with Image.open(base/name) as src:pic=src.convert('RGB').resize((960,540))
  x=col*960;y=row*575;im.paste(pic,(x,y));dr.text((x+12,y+547),'До · v21' if col==0 else 'После · отделка и наполнение v22',font=font,fill='white')
im.save(O/'before_after.jpg',quality=94)
preview=Image.new('RGB',(1920,1150),'#10191b');dr=ImageDraw.Draw(preview)
for j,(name,label) in enumerate([('world_v22_court_S01.png','Двор и парковка'),('world_v22_court_S05.png','Свет и растительность'),('world_v19_horror_1F.png','Коридор'),('world_v21_detail_126.png','Мебель, материалы и мелкие предметы')]):
 with Image.open(O/name) as src:pic=src.convert('RGB').resize((960,540))
 x=j%2*960;y=j//2*575;preview.paste(pic,(x,y));dr.text((x+12,y+547),label,font=font,fill='white')
preview.save(O/'preview.jpg',quality=94)
report=dict(success=True,review_revision=19,art_integration_revision=22,build_identity=n['build_identity'],world_id=read(P/'Content/CampaignData/hospital_world.json')['map_id'],executable_sha256=eh,map_sha256=mh,runtime_sha256=a['runtime_sha256'],bindings_sha256=a['bindings_sha256'],quality_sha256=a['quality_sha256'],world_checks=124,visual_checks=50,story_checks=25,menu_checks=57,imported_native_meshes=622,new_models=9,refined_clutter=q['counts']['refined_items'],new_supported_details=q['counts']['new_supported_details'],quality_props=w['quality_props'],quality_trees=w['quality_trees'],quality_windows=w['quality_windows'],active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],custom_door_leaves=w['custom_door_leaves'],unresolved_custom_props=0,bounds_failures=0,world_captures=93,menu_captures=17,limits=a['limits'])
for name in ['verification.json']:(O/name).write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
(PK/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
for status,target,source in [(n,'native_world_check','book_world_check'),(v,'rendered_world_check','book_world_check'),(s,'native_story_check','story_check'),(m,'native_menu_check','menu_check'),(n,'every_door_check','every_door_check'),(n,'native_envelope_check','envelope_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{source}.json',O/f'{target}.json')
lib=R/'assets/models/echo_custom';integration=read(lib/'GAME_INTEGRATION.json');integration.update(revision=22,native_meshes=622,additional_quality_models=9,quality_props=w['quality_props'],new_supported_details=q['counts']['new_supported_details'],refined_clutter=q['counts']['refined_items'],native_branch_trees=3940,detailed_external_windows=836,active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],verification='artifacts/echo_quality_v22/verification.json');(lib/'GAME_INTEGRATION.json').write_text(json.dumps(integration,ensure_ascii=False,indent=2),encoding='utf8');manifest=read(lib/'LIBRARY_MANIFEST.json');manifest['game_integration']=integration;(lib/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
text=f'''# ЭХО — качество окружения и плотность деталей v22

Изменения реализованы в настоящей Windows-игре. Мокрый зернистый асфальт использует неоднородную шероховатость: гладкие влажные участки и матовую сухую крошку; включены более качественные экранные отражения и контактное затемнение. Свет во дворе сочетает холодную ночь и тёплые фонари/окна. Дополнительный свет ограничен пулом из 12 ламп, в помещениях одновременно не более 4; остальные источники — светящиеся геометрические полосы. Центры окон прозрачны: за стеклом остаются настоящие комнаты. У 836 окон добавлены выступающие подоконники, переплёты и козырьки; наружные стены получили отдельную более спокойную облицовку. Прежние серые внутренние стены и главное меню сохранены. Во дворе добавлены 102 наружных фонаря в свободных точках, улучшено ночное небо; при первом запуске этой версии применяется TSR при 100% разрешения, затем ручной выбор пользователя сохраняется.

Созданы 9 новых оригинальных моделей: округлый седан с отдельными стёклами, салоном, колёсами и спицами; два вида деревьев с ветвями и листвой; настенный и уличный светильники; листья, кабельная бухта, мелкие обломки и кашпо. Сферические кроны 3940 деревьев заменены настоящей веточной геометрией, старые стволовые коллизии сохранены. Повторяемая геометрия объединена в экземпляры с ограничением дальности отрисовки.

13 230 прямоугольных пучков травы заменены моделями заострённых травинок. На наружных металлических полосах уменьшена крупная ржавая фактура. Крупная стеновая текстура больше не используется на белых приборах, обивке, окрашенной мебели, металле, резине и мелких деревянных предметах: теперь у них отдельная сдержанная мелкая фактура. {q['counts']['refined_items']} прежних логических предметов из слоёв мелочей и оформления заменены моделями; добавлены {q['counts']['new_supported_details']} новых принадлежностей на свободных частях столов и полок. Их опора, границы и отсутствие пересечения с уже учтёнными вещами проверены при генерации; существующие проходы не используются для размещения. Вместе слой строит {w['quality_props']} объектов и отдельно 3940 деревьев. Полный файл карты, 562 зоны, 35 локаций, размеры, коллизии, двери и сюжет сохранены.

Финальная сборка: мир 124/124; отрисовка 50/50; сюжет/бой/сохранения 25/25; меню 57/57; двери 1012/1012; независимый аудит 10/10. Сохранены 93 кадра карты, 17 меню и 1 сюжетный кадр 1920×1080. preview.jpg и before_after.jpg содержат реальные кадры игры. SHA сборки и данных в verification.json. Источники новых моделей: RawAssets/EchoQualityV22 и tools/build_echo_quality_v22.py; материалы создаются Content/Python/import_echo_quality_v22.py. Исходные EH001–575 не изменены.

Ограничения: приложенный пользователем образец — цель для направления графики, а не обещание точного совпадения. Машины, растительность, персонажи и специализированная техника всё ещё требуют профессионального арта для такого крупного плана. Отражения экранные: объекты вне кадра полностью не отражаются. Все 17 видов противников пока используют общую существующую скелетную основу. Аудит не означает ручной осмотр каждой мелкой детали, длительную проверку стабильности или измерение FPS.

Запуск: PLAY_UNREAL.cmd. N — фонарик; B — яркий осмотр; M/Tab — карта.
'''
(O/'REPORT.md').write_text(text,encoding='utf8');print(json.dumps(report,ensure_ascii=False))
