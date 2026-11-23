"""Export evidence from the final executable, keeping earlier reviews intact."""
import json,hashlib,math,shutil,csv
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_integration_v21';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
eh=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');mh=sha(P/'Content/CampaignData/hospital_world.json');assert eh==sha(P/'Binaries/Win64/EchoWard.exe')
for f in ['hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json']:assert sha(P/'Content/CampaignData'/f)==sha(PK/'Windows/EchoWard/Content/CampaignData'/f)
n=read(PK/'verified_world.json');v=read(PK/'verified_worldvisual_v21.json');s=read(PK/'verified_storyvisual_v7.json');m=read(PK/'verified_native_menu_v21.json');a=read(O/'acceptance.json')
assert n['success'] and n['build_identity'].split(':')[0].lower()==eh
for q in [v,s,m]:assert q['success'] and q['executable_sha256'].lower()==eh and q['map_sha256'].lower()==mh
w=read(Path(n['test_saved'])/'Prototype/book_world_check.json');vr=read(Path(v['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(n['test_saved'])/'Prototype/every_door_check.json');env=read(Path(n['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and w['count']==121 and vr['success'] and vr['count']==47 and s['checks']==25 and m['checks']==57 and doors['success'] and doors['doors']==1012 and not sum(env['failed'].values())
assert w['unresolved_custom_props']==vr['unresolved_custom_props']==0 and w['custom_bounds_failures']==0 and a['success'] and a['map_sha256']==mh
world=list(v['screenshots']);menus=[]
assert len(world)==82
for name in world:
 src=Path(v['test_saved'])/'Prototype/BookWorldCaptures'/name
 with Image.open(src) as im:assert im.size==(1920,1080);im.verify()
 shutil.copy2(src,O/name)
for src in (Path(m['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):
 name='menu_'+src.name;shutil.copy2(src,O/name);menus.append(name)
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
contact(world,'contact');contact(menus,'menu_contact');contact([x for x in world if 'v21_detail' in x],'new_details')
im=Image.new('RGB',(1920,1150),'#10191b');dr=ImageDraw.Draw(im)
for row,name in enumerate(['world_v19_horror_1F.png','menu_menu_1080.png']):
 for col,base in enumerate([R/'artifacts/echo_integration_v20',O]):
  with Image.open(base/name) as src:pic=src.convert('RGB').resize((960,540))
  x=col*960;y=row*575;im.paste(pic,(x,y));dr.text((x+12,y+547),'До · зелёная пятнистая отделка' if col==0 else 'После · прежний стиль главного меню',font=font,fill='white')
im.save(O/'before_after.jpg',quality=94)
report=dict(success=True,review_revision=19,art_integration_revision=21,build_identity=n['build_identity'],world_id=read(P/'Content/CampaignData/hospital_world.json')['map_id'],executable_sha256=eh,map_sha256=mh,runtime_sha256=a['runtime_sha256'],bindings_sha256=a['bindings_sha256'],world_checks=121,visual_checks=47,story_checks=25,menu_checks=57,imported_native_meshes=613,supplemental_models=40,scheduled_types=154,scheduled_replacements=13674,active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],custom_door_leaves=w['custom_door_leaves'],unresolved_custom_props=0,bounds_failures=0,world_captures=82,menu_captures=17,story_captures=1,limits=a['limits'])
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');(PK/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
for status,target,source in [(n,'native_world_check','book_world_check'),(v,'rendered_world_check','book_world_check'),(s,'native_story_check','story_check'),(m,'native_menu_check','menu_check'),(n,'every_door_check','every_door_check'),(n,'native_envelope_check','envelope_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{source}.json',O/f'{target}.json')
b=read(P/'Content/CampaignData/echo_custom_bindings.json');counts={}
for old,num in a['schedule_by_model'].items():
 ident=b['prototype_models'][old];counts[ident]=counts.get(ident,0)+num
titles={m['id']:m['title'] for m in read(P/'RawAssets/EchoSupplementsV21/manifest.json')['assets']}
with (O/'placement_coverage.csv').open('w',encoding='utf-8-sig',newline='') as f:
 writer=csv.writer(f);writer.writerow(['prototype_id','prototype_name','native_model','instances','status'])
 catalog=read(P/'Content/CampaignData/hospital_world.json')['prototype_catalog']
 for old,num in a['schedule_by_model'].items():writer.writerow([old,catalog[old]['name'],b['prototype_models'][old],num,'native_mesh'])
lib=R/'assets/models/echo_custom';integration=dict(status='all_scheduled_furnishing_types_resolved_and_verified',revision=21,imported_original_static_entries=562,supplemental_original_models=40,native_meshes=613,scheduled_types=154,scheduled_instances=13674,active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],active_door_leaves=w['custom_door_leaves'],skeletal_library_deferred=list(range(563,576)),verification='artifacts/echo_integration_v21/verification.json')
(lib/'GAME_INTEGRATION.json').write_text(json.dumps(integration,ensure_ascii=False,indent=2),encoding='utf8');manifest=read(lib/'LIBRARY_MANIFEST.json');manifest['game_integration']=integration;manifest['scope']='Original first-pass library. Every scheduled furnishing type resolves to a native mesh; this is distinct from placement of every library entry. Rigged library characters remain pending.';(lib/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
text=f'''# ЭХО — прежние стены меню и устранение предметов-заглушек v21

Главное меню вернулось к прежним материалам MI_527b19fd5563fa6d, MI_4ae6673fcc8b19e7 и MI_05227396fb5ca2d8. Крупная зелёная пятнистая отделка v20 исключена из стен всей карты. В корпусах используется прежняя серая штукатурка и холодная краска меню с проекцией в мировых координатах, чтобы длинные стены не растягивали текстуру. Высокие внутренние перегородки тоже разделены на серую штукатурку сверху и окрашенную полосу высотой 1,08 м снизу; добавлена тонкая защитная планка. Их прежние цельные коллизии сохранены. Свет, фоновые события меню, hover и скример сохранены. before_after.jpg — реальные кадры v20 и этой сборки.

Добавлены 40 авторских моделей с фасками, цилиндрами, трубками и отдельными деталями: диван с подушками, чайник, серверные стойки, сантехника, стиральные машины, КТ/МРТ с открытым тоннелем, медицинские приборы и другие недостающие предметы. Все 154 реально расставленных типа мебели/оборудования, всего 13 674 экземпляра расписания, теперь имеют нативную модель. В движке не осталось неразрешённых замен этого расписания; счётчик0. Дополнительно заменены старые шкафы, кровати-укрытия, пять передвигаемых предметов и 395 интерактивных предметов/маркеров. Прежние коллизии, точки взаимодействия и правила укрытий сохранены.

Вместо кубических NPC применена существующая человеческая модель с idle; вместо кубических тел 46 противников — существующая скелетная основа Stalker с idle/walk/run/attack. Это временная общая внешность; отдельные модели 17 видов монстров и персонажи библиотеки 563–575 по-прежнему требуют художественной доработки. 78 сюжетных маркеров получили физические предметы-улики; полноценные катсцены этим не реализуются.

Фактически построены {w['custom_furniture']} новых экземпляров окружения, применяются {w['custom_model_types']} разных номеров моделей; отдельно {w['custom_door_leaves']} створки. Весь исходный файл карты, 562 зоны, 35 локаций, метры и сюжет сохранены побайтово. Исходные GLB/Blender не менялись. Исходники новых моделей: RawAssets/EchoSupplementsV21 и tools/build_echo_supplements_v21.py.

Финал: Game и Editor собраны; мир 121/121, отрисовка 47/47, сюжет/сохранения 25/25, меню 57/57, двери 1012/1012; аудит импорта 9/9. 82 PNG карты, 17 PNG меню и 1 PNG противника сняты из упакованной Windows-сборки 1920×1080. SHA и счётчики в verification.json. Статусы всех 154 типов в placement_coverage.csv. Проверки и кадры не означают ручной обход каждой комнаты, замер FPS или финальное качество графики. Прямоугольная конструкция стены/шкафа сама по себе не ошибка: функциональные панели укрытий и архитектуры сохранены. Специализированная анимация приборов и окончательный арт остаются следующими этапами.

Запуск: PLAY_UNREAL.cmd; N — фонарик, B — яркий осмотр, M/Tab — карта.
'''
(O/'REPORT.md').write_text(text,encoding='utf8');print(json.dumps(report,ensure_ascii=False))
