"""Publish evidence only for the exact tested packaged EXE and data set."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_real_v23';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
accept=read(O/'acceptance.json');art=read(P/'Content/CampaignData/echo_real_v23.json');raw=read(P/'RawAssets/EchoRealV23/manifest.json');eh=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');mh=sha(P/'Content/CampaignData/hospital_world.json')
reflow=read(O/'support_reflow.json');assert reflow['success'] and not art['unresolved_support']
assert accept['success'] and eh==sha(P/'Binaries/Win64/EchoWard.exe')
for name in ['hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','THIRD_PARTY_ART.md']:assert sha(P/'Content/CampaignData'/name)==sha(PK/'Windows/EchoWard/Content/CampaignData'/name),name
latest=max(p.stat().st_mtime for p in (PK/'Windows/EchoWard/Content/Paks').iterdir() if p.suffix in ('.pak','.ucas','.utoc'))
n=read(PK/'verified_world.json');v=read(PK/'verified_worldvisual_v23.json');s=read(PK/'verified_storyvisual_v7.json');m=read(PK/'verified_native_menu_v23.json')
for filename in ['verified_world.json','verified_worldvisual_v23.json','verified_storyvisual_v7.json','verified_native_menu_v23.json']:assert (PK/filename).stat().st_mtime+2>=latest,filename
assert n['success'] and n['build_identity'].split(':')[0].lower()==eh
for status in [v,s,m]:assert status['success'] and status['executable_sha256'].lower()==eh and status['map_sha256'].lower()==mh
w=read(Path(n['test_saved'])/'Prototype/book_world_check.json');visual=read(Path(v['test_saved'])/'Prototype/book_world_check.json');door=read(Path(n['test_saved'])/'Prototype/every_door_check.json');envelope=read(Path(n['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and w['count']==128 and visual['success'] and visual['count']==54 and s['checks']==25 and m['checks']==57
assert door['success'] and door['doors']==1012 and not sum(envelope['failed'].values()) and w['custom_bounds_failures']==0 and w['unresolved_custom_props']==0
assert w['real_props']==len(art['props']) and w['real_solid_props']==art['counts']['floor_belonging']
world=list(v['screenshots']);menus=[]
for name in world:
 p=Path(v['test_saved'])/'Prototype/BookWorldCaptures'/name
 with Image.open(p) as im:assert im.size==(1920,1080);im.verify()
 shutil.copy2(p,O/name)
for p in (Path(m['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):
 with Image.open(p) as im:assert im.size==(1920,1080);im.verify()
 name='menu_'+p.name;shutil.copy2(p,O/name);menus.append(name)
assert len(menus)==17
shutil.copy2(Path(s['test_saved'])/'Prototype/story_v7.png',O/'story_mob.png')
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
def contact(files,prefix):
 for start in range(0,len(files),20):
  page=files[start:start+20];image=Image.new('RGB',(1640,math.ceil(len(page)/4)*254),'#10191b');draw=ImageDraw.Draw(image)
  for index,name in enumerate(page):
   with Image.open(O/name) as im:pic=im.convert('RGB');pic.thumbnail((400,225))
   x=index%4*410;y=index//4*254;image.paste(pic,(x,y+25));draw.text((x+4,y+2),name.removeprefix('world_')[:-4],fill='white',font=font)
  image.save(O/f'{prefix}_{start//20+1}.png')
contact(world,'contact');contact(menus,'menu_contact');contact([x for x in world if 'v23_' in x],'real_art_contact')
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
for i,(name,label) in enumerate([('world_v23_court_S01.png','Подробный автомобиль и ночной двор'),('world_v23_detail_126.png','Настоящие материалы мебели и вещей'),('world_v23_detail_152.png','Растения с текстурами'),('world_v23_lab_clutter.png','Лабораторное наполнение')]):
 with Image.open(O/name) as src:pic=src.convert('RGB').resize((960,540))
 x=i%2*960;y=i//2*575;preview.paste(pic,(x,y));draw.text((x+12,y+547),label,font=font,fill='white')
preview.save(O/'preview.jpg',quality=94)
compare=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(compare)
for row,(before,after) in enumerate([('world_v22_court_S01.png','world_v23_court_S01.png'),('world_v21_detail_126.png','world_v23_detail_126.png')]):
 for col,path in enumerate([R/'artifacts/echo_quality_v22'/before,O/after]):
  with Image.open(path) as im:pic=im.convert('RGB').resize((960,540))
  x=col*960;y=row*575;compare.paste(pic,(x,y));draw.text((x+12,y+547),'До · v22' if col==0 else 'После · настоящие PBR-модели v23',font=font,fill='white')
compare.save(O/'before_after.jpg',quality=94)
active_ids={p['mesh'] for p in art['props']}|set(art['mesh_aliases'].values());active_sources=sorted({a['source'] for a in raw['assets'] if a['id'] in active_ids})
report=dict(success=True,review_revision=19,art_integration_revision=23,build_identity=n['build_identity'],world_id=read(P/'Content/CampaignData/hospital_world.json')['map_id'],executable_sha256=eh,map_sha256=mh,**accept['data_hashes'],world_checks=w['count'],visual_checks=visual['count'],story_checks=s['checks'],menu_checks=m['checks'],imported_native_meshes=accept['native_meshes'],new_detailed_native_models=len(raw['assets']),active_external_sources=len(active_sources),replacement_aliases=len(art['mesh_aliases']),new_real_props=w['real_props'],new_floor_solids=w['real_solid_props'],new_props_by_role=art['counts'],quality_props=w['quality_props'],quality_trees=w['quality_trees'],quality_windows=w['quality_windows'],active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],custom_door_leaves=w['custom_door_leaves'],unresolved_custom_props=0,bounds_failures=0,world_captures=len(world),menu_captures=len(menus),limits=art['limits'])
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');(PK/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
for status,target,source in [(n,'native_world_check','book_world_check'),(v,'rendered_world_check','book_world_check'),(s,'native_story_check','story_check'),(m,'native_menu_check','menu_check'),(n,'every_door_check','every_door_check'),(n,'native_envelope_check','envelope_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{source}.json',O/f'{target}.json')
lib=R/'assets/models/echo_custom';integration=read(lib/'GAME_INTEGRATION.json');integration.update(revision=23,native_meshes=accept['native_meshes'],external_detailed_models=len(raw['assets']),active_external_sources=len(active_sources),new_real_props=w['real_props'],active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],verification='artifacts/echo_real_v23/verification.json',third_party_credit='unreal/EchoWard/Content/CampaignData/THIRD_PARTY_ART.md');(lib/'GAME_INTEGRATION.json').write_text(json.dumps(integration,ensure_ascii=False,indent=2),encoding='utf8');manifest=read(lib/'LIBRARY_MANIFEST.json');manifest['game_integration']=integration;(lib/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
text=f'''# ЭХО — подробные модели и дополнительное наполнение v23

В настоящую Windows-игру добавлены подробные лицензированные модели с UV, авторскими PBR-текстурами, нормалями, масками шероховатости/металла и отдельной прозрачностью. Импортированы {len(raw['assets'])} моделей и вариантов, 212 материалов, подготовлены уровни детализации для тяжёлой геометрии; в общей библиотеке {accept['native_meshes']} нативный меш. Проверены контрольные суммы {accept['verified_source_files']} исходных файлов. Исходные EH001–575 сохранены. Загрузка и импорт не означают, что каждая новая модель уже встречается в игре: сейчас используется {len(active_sources)} различных внешних источников, а {len(art['mesh_aliases'])} соответствия заменяют прежние типы моделей по всей карте.

Заменены заметные машины, стулья, диваны, металлические столы, тумбы, шкафы, стеллажи, часть тележек, коляски, холодильники, инструменты, настольные лампы, кашпо и растительность. Современные уличные фонари сохранены: опробованная старинная модель не подходила месту. Небольшие предметы сохраняют пропорции и основание на опоре. Защитные габариты старой расстановки сохранены при подгонке замен.

Сверх v22 добавлено {w['real_props']} объектов: {art['counts']['supported_detail']} принадлежностей на измеренных полках и столешницах, {art['counts']['floor_belonging']} напольных вещей с соответствующими коллизиями, {art['counts']['wall_equipment']} настенных приборов и {art['counts']['lawn_plant']} растений на прежних газонах. Наборы зависят от назначения: лабораторная посуда и ленты в клинических зонах, инструмент и приборы в мастерских, продукты в магазинах/кухне, канцелярия и посуда в комнатах персонала, бытовая химия в хозяйственных помещениях. Исправлена опора у {art['counts']['corrected_supports']} существующих элементов слоя v22 с учётом новых поверхностей мебели. Дверные проёмы и центральные проходы сохраняются; новое напольное размещение проверено относительно дверных зон.

На крупных игровых кадрах обнаружены вещи под столами, ошибочно поставленные на внутренние плоскости ящиков. У закрытых столов и тумб опорой теперь служит открытая столешница; footprint проверяется по реальным треугольникам поверхности, дополнительно проверен зазор до полки выше. В дополнительном проходе переставлен {reflow['relocated']} предмет, из них {sum(bool(x.get('replacement_mesh')) for x in reflow['moved'])} заменены подходящими меньшими моделями без растягивания. Неразмещённых предметов нет; доказательства — support_assignments.json и support_reflow.json.

Проверки итоговой сборки: мир {w['count']}/{w['count']}, отрисовка {visual['count']}/{visual['count']}, бой/сохранения {s['checks']}/{s['checks']}, меню {m['checks']}/{m['checks']}, двери 1012/1012, оболочки без обнаруженных ошибок, независимый аудит {len(accept['checks'])}/{len(accept['checks'])}. Сохранены {len(world)} кадров мира, 17 меню и один сюжетный кадр 1920×1080. preview.jpg и before_after.jpg — реальные изображения из сборки. Геометрия канонической карты, 562 зоны, 35 локаций и сюжетные данные побайтово сохранены. Hash EXE: {eh}. Полная идентичность сборки и данных: verification.json.

Источники: [Poly Haven](https://polyhaven.com/) (CC0), [Khronos sample assets](https://github.com/KhronosGroup/glTF-Sample-Assets) (индивидуальные CC0/CC-BY). Авторы, ссылки, тексты лицензий и изменения указаны в Content/CampaignData/THIRD_PARTY_ART.md и assets/external/quality_v23/CREDITS.md; эти сведения включены в пакет. Powered by Poly Haven. В игре нет обращений к внешнему API.

Ограничения: замены перечислены полностью в replacement_coverage.csv. Для части специализированного медицинского оборудования, архитектурных модулей и персонажей пока сохранены текущие модели, поскольку подходящая проверенная замена не найдена. Не утверждается, что абсолютно все предметы проекта заменены. Устройство модели холодильника и отдельные виды мебели художественно адаптированы; это не инженерная копия оборудования. Расширенные glTF эффекты ткани/просвечивания/лака переданы приближённо существующими материалами Unreal. Отражения остаются экранными. Качество фотографического образца не заявляется; FPS и длительная стабильность этого прохода не измерялись. Автоматический аудит не заменяет ручную проверку каждого мелкого предмета.

Запуск: PLAY_UNREAL.cmd. N — фонарик; B — яркий осмотр; M/Tab — карта. Существующие игровые сохранения сохранены.
'''
(O/'REPORT.md').write_text(text,encoding='utf8');print(json.dumps(report,ensure_ascii=False))
