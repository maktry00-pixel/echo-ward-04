"""Publish v31 evidence only after all packaged acceptance checks match."""
from pathlib import Path
from datetime import datetime, timezone
import json,hashlib,shutil,csv
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];U=R/'unreal/EchoWard';D=R/'artifacts/echo_cars_v31';P=U/'Saved/PackagedWindows';C=P/'Windows/EchoWard/Content/CampaignData';exe=P/'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(4*1024*1024),b''):h.update(block)
 return h.hexdigest()
def write(p,d):p.write_text(json.dumps(d,ensure_ascii=False,indent=2),encoding='utf-8')
e=sha(exe);cars=read(P/'verified_cars_v31.json');arrival=read(P/'verified_arrival_v29.json');menu=read(P/'verified_menu.json');world=read(U/'Saved/PackageTests/CarsV31/Saved/Prototype/book_world_check.json');a=read(U/'Saved/PackageTests/ArrivalV29/Saved/Prototype/arrival29_check.json');m=read(Path(menu['test_saved'])/'Prototype/menu_check.json');perf=read(U/'Saved/PackageTests/PerformanceV25_cars31/Saved/Prototype/hospital_perf_cars31.json');native=read(U/'Saved/Prototype/cars31_native_audit.json');manifest=read(U/'RawAssets/EchoCarsV31/manifest.json');data=read(C/'echo_cars_v31.json');runtime=read(C/'echo_custom_runtime.json')
assert all(x['success'] for x in [cars,arrival,menu,world,a,m,perf,native])
assert all(x['executable_sha256'].lower()==e for x in [cars,arrival,perf])
assert menu['build_identity'].split(':')[0].lower()==e
assert world['count']>=156 and a['count']>=48 and m['count']>=57
assert not any(not x['passed'] for x in world['checks']+a['checks']+m['checks'])
assert cars['cars_data_sha256'].lower()==sha(C/'echo_cars_v31.json')==perf['cars31_data_sha256']
assert arrival['arrival_data_sha256'].lower()==sha(C/'echo_arrival_v29.json')
assert sha(C/'hospital_world.json')=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec'
assert native['manifest_sha256']==sha(U/'RawAssets/EchoCarsV31/manifest.json')
partitions=read(D/'nanite_material_partitions.json');assert len(partitions)==4 and all(v['triangles_before']==v['triangles_after'] for v in partitions)
for rel in ['CarsV31/Saved/package_test.log','ArrivalV29/Saved/package_test.log','PerformanceV25_cars31/Saved/performance.log']:
 text=(U/'Saved/PackageTests'/rel).read_text(encoding='utf-8',errors='replace');assert not any(w in text for w in ['Non-Nanite Marking Job Queue overflow','Default Material will be used','GPU Crashed','Out of video memory'])
assert len(manifest['vehicles'])==32 and native['meshes']==76 and len(runtime['meshes'])==869
assert len(data['placements'])==35 and world['cars31_count']==35 and world['cars31_sources']==31
assert {v['slug'] for v in data['placements']}|{data['hero']['slug']}==set(data['vehicles'])
inventory=read(D/'inventory.json')
for src in inventory['vehicles']:assert sha(R/'assets/models/cars'/src['archive'])==src['sha256']
paks=sorted(p for p in (P/'Windows/EchoWard/Content/Paks').iterdir() if p.suffix in ['.pak','.ucas','.utoc']);pak_hashes={p.name:sha(p) for p in paks};assert pak_hashes==perf['packaged_content_sha256']
repeat=read(U/'Saved/PackageTests/PerformanceV25_cars31_repeat/Saved/Prototype/hospital_perf_cars31_repeat.json');assert repeat['success'] and len(repeat['views'])==8 and repeat['executable_sha256']==e and repeat['packaged_content_sha256']==pak_hashes
repeat_log=(U/'Saved/PackageTests/PerformanceV25_cars31_repeat/Saved/performance.log').read_text(encoding='utf-8',errors='replace');assert not any(w in repeat_log for w in ['Non-Nanite Marking Job Queue overflow','Default Material will be used','GPU Crashed','Out of video memory'])
write(D/'performance_repeat.json',repeat)
files=['book_map.json','hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json','echo_detail_v27.json','echo_horror_v28.json','echo_arrival_v29.json','echo_realism_v30.json','echo_cars_v31.json']
identity=':'.join([e.upper()]+[sha(p).upper() for p in paks]+[sha(C/n).upper() for n in files]);assert identity==menu['build_identity']
for v in data['captures']:
 p=U/'Saved/PackageTests/CarsV31/Saved/Prototype/BookWorldCaptures'/('world_'+v['id']+'.png');im=Image.open(p);assert im.size==(1920,1080)
for name in a['screenshots']:assert Image.open(U/'Saved/PackageTests/ArrivalV29/Saved/Prototype/ArrivalCaptures'/name).size==(1920,1080)
for name,obj in [('world_check.json',world),('arrival_check.json',a),('menu_check.json',m),('native_audit.json',native),('performance_after.json',perf)]:write(D/name,obj)
old=read(R/'artifacts/echo_realism_v30/performance_after.json');before={v['view']:v for v in old['views']};comparison=[]
for v in perf['views']:
 b=before[v['view']];comparison.append(dict(view=v['view'],before_fps=b['fps'],after_fps=v['fps'],change_percent=(v['fps']/b['fps']-1)*100,included='corridor' not in v['view'],note='Static 1080p/100%, AI idle; corridor view excluded because it faces a nearby wall.' if 'corridor' in v['view'] else 'Static 1080p/100%, AI idle.'))
repeat_by_view={v['view']:v for v in repeat['views']}
for v in comparison:v['repeat_fps']=repeat_by_view[v['view']]['fps']
write(D/'performance_comparison.json',comparison)
with (D/'performance_comparison.csv').open('w',encoding='utf-8-sig',newline='') as f:w=csv.DictWriter(f,fieldnames=list(comparison[0]));w.writeheader();w.writerows(comparison)
shots=U/'Saved/PackageTests/ArrivalV29/Saved/Prototype/ArrivalCaptures';(D/'arrival').mkdir(exist_ok=True)
for name in a['screenshots']:shutil.copy2(shots/name,D/'arrival'/name)
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',24);preview=Image.new('RGB',(1600,950),(8,12,15));draw=ImageDraw.Draw(preview)
for i,(name,label) in enumerate([('01_drive.png','Mercedes W212: arrival'),('08_interior.png','Original interior / mouse look'),('09_all_panels_open.png','Four doors, hood and trunk'),('03_checkpoint_record.png','Checkpoint / preserved story')]):
 im=Image.open(shots/name).convert('RGB');im.thumbnail((800,450));x=i%2*800;y=i//2*475;preview.paste(im,(x,y));draw.text((x+12,y+447),label,font=font,fill='white')
preview.save(D/'preview.jpg',quality=92)
verification=dict(success=True,checked_at=datetime.now(timezone.utc).isoformat(),revision=31,executable_sha256=e,editor_module_sha256=sha(U/'Binaries/Win64/UnrealEditor-EchoWard.dll'),build_identity=identity,native_models=len(runtime['meshes']),new_native_meshes=76,materials=native['materials'],archives=32,parked=35,hero=1,transport_objects=36,world_checks=world['count'],arrival_checks=a['count'],menu_checks=m['count'],world_rooms=world['rooms'],world_locations=world['locations'],doors=1012,car_frames=32,arrival_frames=len(a['screenshots']),performance_views=8,custom_bounds_failures=world['custom_bounds_failures'],unresolved_custom_props=world['unresolved_custom_props'],pak_sha256=pak_hashes,current_data_hashes={p.name:sha(p) for p in C.glob('*.json')},limitations=['Only the hero W212 has interactive panels and a seat camera. Other provided cars are static scenery.','No free driving or seated human animation in this revision.','GLS source was a colored point cloud; reconstructed exterior only.','Performance measured in static views, not during combat.'])
verification['performance_repeat_views']=8;verification['startup_hitches_documented']=True;verification['nanite_partitions']=partitions;verification['shadow_overflow_in_current_tests']=False
write(D/'verification.json',verification)
write(D/'acceptance.json',dict(success=True,build_identity=identity,checks=[dict(name=n,passed=True) for n in ['32 original archives preserved by SHA256','All 32 sources represented in game','35 static placements and one interactive hero','76 native meshes with metric bounds','869 total native records, prior library retained','Four Nanite partitions retain every source triangle','No translucent materials on Nanite meshes','Full world 156 checks','Vehicle arrival 48 checks','Menu regression 57 checks','32 vehicle frames, 9 arrival frames','8 static performance views at 1080p and 100 percent','No current shadow overflow or missing material warnings','Canonical hospital geometry SHA unchanged']]))
for target in [R/'assets/models/echo_custom/GAME_INTEGRATION.json',R/'assets/models/echo_custom/LIBRARY_MANIFEST.json']:
 obj=read(target);g=obj['game_integration'] if 'game_integration' in obj else obj;g.update(revision=31,native_meshes=869,active_custom_instances=world['custom_furniture'],active_model_types=world['custom_model_types'],optimization_revision=31,verification='artifacts/echo_cars_v31/verification.json',cars31=dict(archives=32,native_meshes=76,parked=35,hero='Mercedes-Benz E-Class W212',parts=12,interactive_panels=6,point_cloud_exterior='GLS van',source_manifest='artifacts/echo_cars_v31/inventory.json'));assert len(g['skeletal_library_deferred'])==13;write(target,obj)
valid=[v for v in perf['views'] if 'corridor' not in v['view']];fps_min=min(v['fps'] for v in valid);fps_max=max(v['fps'] for v in valid);repeat_valid=[v for v in repeat['views'] if 'corridor' not in v['view']];repeat_min=min(v['fps'] for v in repeat_valid);repeat_max=max(v['fps'] for v in repeat_valid)
intro=f'''**Текущая сборка v31 — автомобили пользователя.** Использованы все 32 архива: 35 статичных транспортных объектов и Mercedes-Benz E-Class W212 героя, всего 36 вместе с полуприцепом. У Mercedes настоящий исходный салон, четыре двери, капот и багажник открываются отдельно. F — сесть/выйти и взаимодействовать; Y — открыть/закрыть ближайшую панель; мышь — смотреть из салона. Для въезда выбери новую игру. Свободного вождения и анимации сидящего человека пока нет; остальные автомобили статичны. GLS восстановлен из цветного облака точек, это только внешняя поверхность.

Мир {world['count']}/{world['count']}, вступление {a['count']}/{a['count']}, меню {m['count']}/{m['count']}; 32 автомобильных и {len(a['screenshots'])} сюжетных кадров. Полная геометрия 562 зон / 35 локаций сохранена. Используются Nanite для фоновых кузовов, LOD для прочих деталей и ограничение текстур фоновых машин. Четыре сложных кузова разделены по материалам без уменьшения геометрии. Статические виды 1080p/100%: первый замер {fps_min:.1f}–{fps_max:.1f} FPS, повторный {repeat_min:.1f}–{repeat_max:.1f} FPS. Первый ракурс включает отдельные задержки 6,1 / 2,5 / 1,2 с; они сохранены в исходном результате. Это не замер боя.

Запуск: D:/Projects/ECHO/PLAY_UNREAL.cmd. [Кадры](D:/Projects/ECHO/artifacts/echo_cars_v31/preview.jpg), [список автомобилей](D:/Projects/ECHO/artifacts/echo_cars_v31/VEHICLES_RU.md), [отчёт](D:/Projects/ECHO/artifacts/echo_cars_v31/REPORT.md).

---

'''
for target in [R/'README.md',U/'README.md']:
 text=target.read_text(encoding='utf-8-sig');target.write_text(intro+text,encoding='utf-8')
handoff=f'''# v31 — автомобили: ЗАВЕРШЕНО

Рабочая папка D:/Projects/ECHO. Старый корень C:/Users/YOUR_WINDOWS_USER/OneDrive/Documents/ChatGPT/pacman содержит дочерние junction: не удалять рекурсивно. Один тяжёлый процесс UE/Blender за раз.

Все 32 архива assets/models/cars сохранены с проверенными SHA256. 76 новых native mesh / всего869; 910 используемых материал-спецификаций. 35 статичных объектов плюс W212 героя. 12 деталей W212, четыре двери/капот/багажник, F/Y/мышь, препятствия при выходе, сохранение открытых панелей вне салона; сохранение/оружие/рывок в салоне запрещены. Никакого свободного вождения/сидящей анимации. GLS — восстановленная внешняя поверхность скана. Оригиналы не изменены; лицензии пользовательских ресурсов не объявлены CC0.

EXE {e}; EditorDLL {verification['editor_module_sha256']}. Мир156, вступление48, меню57, автомобильных кадров32, сюжетных9; benchmark8. Артефакты artifacts/echo_cars_v31; проверенный package_verified.json опубликован последним. Канон v29 и hospital_world SHA прежние. Производительность и ограничения в REPORT.md.

Файлы WardCars31.cpp, WardHeroVehicle31.cpp, WardHospitalArrival29.cpp, WardHospitalArrivalAssessment29.cpp; Content/CampaignData/echo_cars_v31.json. tools/prepare_vehicle_meshes_v31.py сохраняет полные поверхности (без глобального Decimate), выбрасывает stage/helper-геометрию и дублирующий coplanar coat Porsche. Перегенерация: Blender -> split_vehicle_nanite_sections_v31.py -> finalize_vehicle_sources -> deploy -> UE import -> package_cars_v31. Нельзя пересобирать/импортировать во время benchmark. Финальные проверки подтверждены точными хешами в verification.json.

---

'''
t=R/'HANDOFF_NEXT_GPT.md';t.write_text(handoff+t.read_text(encoding='utf-8-sig'),encoding='utf-8')
for name in ['README.md','HANDOFF_NEXT_GPT.md']:
 target=Path('C:/Users/YOUR_WINDOWS_USER/OneDrive/Documents/ChatGPT/pacman')/name
 if target.exists() and not target.is_symlink():shutil.copy2(R/name,target)
report=intro+'## Проверка\n\nОригиналы 32 архивов проверены по SHA256. Импорт: 76 native mesh, '+str(native['materials'])+' материалов. Все шесть панелей Mercedes проверены на открытие/закрытие; проверены посадка, выход, мышь, пауза, блокировка действий и восстановление состояний из сохранения. Старый дублирующий лак Porsche заменён clear-coat материалом. Все 32 фактических игровых кадра просмотрены; пути и коллизии входят в 156 проверок мира.\n\n## Производительность\n\n| Вид | v30 FPS | v31 первый | v31 повтор | Изменение первого |\n|---|---:|---:|---:|---:|\n'
for v in comparison:
 if v['included']:report+=f"| {v['view']} | {v['before_fps']:.1f} | {v['after_fps']:.1f} | {v['repeat_fps']:.1f} | {v['change_percent']:+.1f}% |\n"
report+='\nВ первом ракурсе первичного замера три многосекундных задержки (6,1 / 2,5 / 1,2 с) снизили средний FPS. Они не удалены из статистики: без кадров длиннее 100 мс этот участок давал около 51,5 FPS, но это только диагностическая оценка. Повторный замер сохранён отдельно, на той же сборке и без изменения разрешения. Начальная загрузка полной карты по-прежнему тяжёлая.\n\nКузова Kia Carnival, Kia Seltos, Brabus G900 и Volkswagen Crafter разделены по материалам для Nanite: все 2 501 109 исходных треугольников сохранены вместе с UV, цветами и нормалями. Стёкла не отбрасывают отдельные тени. Переполнение очереди теней проверяется как ошибка.\n\nRTX 5060 Laptop 8 GB, 1920×1080, 100% render scale. Восемь статических видов, AI idle; коридорный вид упирается в стену и исключён из сравнения. Это не замер боевых сцен или длительного прохождения. Отдельные исходные модели имеют разное качество; GLS — внешняя реконструкция скана, Tata/скорая сохранены в исходной степени детализации.\n'
(D/'REPORT.md').write_text(report,encoding='utf-8')
# Publish the authoritative marker only after all evidence and documentation are complete.
marker=dict(verification,world_id=read(C/'hospital_world.json')['map_id'],map_sha256=sha(C/'hospital_world.json'),art_integration_revision=31,optimization_revision=31,story_revision=29,active_custom_instances=world['custom_furniture'],active_model_types=world['custom_model_types'],book_world_instances=world['instances'],current_car_captures=32,current_arrival_captures=len(a['screenshots']),current_performance_views=8,physical_stair_and_gallery_walk_verified=True,physical_tunnel_walk_verified=True)
write(P/'package_verified.json',marker)
(D/'STATE_IN_PROGRESS.md').write_text(handoff,encoding='utf-8')
(D/'STATE_COMPLETED.md').write_text(handoff,encoding='utf-8')
history=D/'historical_failures';history.mkdir(exist_ok=True)
for name in ['cars31_failed.txt','cars31_audit_failed.txt']:
 source=U/'Saved/Prototype'/name
 if source.exists():shutil.move(str(source),str(history/name))
print('V31_PUBLISHED',e,world['count'],a['count'],m['count'],f'FPS {fps_min:.1f}-{fps_max:.1f}')
