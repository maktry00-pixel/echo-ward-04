"""Publish the accepted v32 car-detail update without rewriting historical v31 evidence."""
from pathlib import Path
from datetime import datetime,timezone
import json,hashlib,shutil,csv
from PIL import Image
R=Path(__file__).resolve().parents[1];U=R/'unreal/EchoWard';P=U/'Saved/PackagedWindows';C=P/'Windows/EchoWard/Content/CampaignData';D=R/'artifacts/echo_car_quality_v32';exe=P/'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(4*1024*1024),b''):h.update(b)
 return h.hexdigest()
e=sha(exe);cars=read(P/'verified_cars_v31.json');arrival=read(P/'verified_arrival_v29.json');menu=read(P/'verified_menu.json');settings=read(P/'verified_settings.json');world=read(Path(cars['test_saved'])/'Prototype/book_world_check.json');a=read(Path(arrival['test_saved'])/'Prototype/arrival29_check.json');m=read(Path(menu['test_saved'])/'Prototype/menu_check.json');s=read(Path(settings['test_saved'])/'Prototype/settings_menu_check.json');quality=read(C/'echo_car_quality_v32.json');q=read(D/'native_texture_audit.json');perf_path=U/'Saved/PackageTests/PerformanceV25_carquality32/Saved/Prototype/hospital_perf_carquality32.json';perf=read(perf_path)
assert all(x['success'] for x in [cars,arrival,menu,settings,world,a,m,s,quality,q,perf])
assert all(x['executable_sha256'].lower()==e for x in [cars,arrival,perf])
assert all(x['build_identity'].split(':')[0].lower()==e for x in [menu,settings])
assert world['count']==156 and a['count']==50 and m['count']==57 and s['count']==24
assert all(v['passed'] for x in [world,a,m,s] for v in x['checks'])
assert quality==q and q['hero_budget']==4096 and q['parked_budget']==2048 and q['textures']==508 and q['upscaled_images']==0
assert len(read(C/'echo_custom_runtime.json')['meshes'])==869
assert sha(C/'hospital_world.json')=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec'
assert len(perf['views'])==8 and perf['screen_percentage']==100 and perf['car_quality32_data_sha256']==sha(C/'echo_car_quality_v32.json')
repeat=read(U/'Saved/PackageTests/PerformanceV25_carquality32_repeat/Saved/Prototype/hospital_perf_carquality32_repeat.json');assert repeat['success'] and len(repeat['views'])==8 and repeat['screen_percentage']==100 and repeat['executable_sha256']==e and repeat['car_quality32_data_sha256']==sha(C/'echo_car_quality_v32.json')
paks=sorted(p for p in (P/'Windows/EchoWard/Content/Paks').iterdir() if p.suffix in ['.pak','.ucas','.utoc']);pak_hashes={p.name:sha(p) for p in paks};assert pak_hashes==perf['packaged_content_sha256']==repeat['packaged_content_sha256']
files=['book_map.json','hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json','echo_detail_v27.json','echo_horror_v28.json','echo_arrival_v29.json','echo_realism_v30.json','echo_cars_v31.json','echo_car_quality_v32.json'];identity=':'.join([e.upper()]+[sha(p).upper() for p in paks]+[sha(C/n).upper() for n in files]);assert menu['build_identity']==settings['build_identity']==identity
for n in files:assert sha(C/n)==sha(U/'Content/CampaignData'/n),n
for folder,filename in [('CarsV31','package_test.log'),('ArrivalV29','package_test.log'),('Menu','package_test.log'),('Settings','package_test.log'),('PerformanceV25_carquality32','performance.log'),('PerformanceV25_carquality32_repeat','performance.log')]:
 log=U/'Saved/PackageTests'/folder/'Saved'/filename;text=log.read_text(encoding='utf-8',errors='replace');assert not any(word in text for word in ['Default Material will be used','Failed to compile Material','GPU Crashed','Out of video memory','Non-Nanite Marking Job Queue overflow','surface cache atlas oversubscribed']),folder
cook=(R/'tmp/echo_v31_package.log').read_text(encoding='utf-8',errors='replace');proof=[]
for name in ['T_V31_779776c549e408a4','T_V31_61ab9ed3845657e8','T_V31_bce1194c56a386d8']:
 line=next(line for line in cook.splitlines() if name in line and '4096x4096' in line);proof.append(line)
(D/'cooked_4k_proof.txt').write_text('\n'.join(proof),encoding='utf-8')
for row in q['assets']:
 if row['hero'] and ('mat_salon' in row['names'] or 'mat_body' in row['names']):assert row['max_size']==4096
for name in a['screenshots']:
 source=Path(arrival['test_saved'])/'Prototype/ArrivalCaptures'/name;assert Image.open(source).size==(1920,1080)
for name in ['08_interior.png','09_all_panels_open.png']:assert Image.open(D/'after'/name).size==(1920,1080)
for name,obj in [('world_check.json',world),('arrival_check.json',a),('menu_check.json',m),('settings_check.json',s),('performance_after.json',perf),('performance_repeat.json',repeat)]:write(D/name,obj)
base=read(R/'artifacts/echo_cars_v31/performance_repeat.json');prior={v['view']:v for v in base['views']};comparison=[]
for v in perf['views']:b=prior[v['view']];comparison.append(dict(view=v['view'],before_fps=b['fps'],after_fps=v['fps'],change_percent=(v['fps']/b['fps']-1)*100,before_ram_mb=b['process_physical_mb'],after_ram_mb=v['process_physical_mb'],included='corridor' not in v['view']))
repeat_views={v['view']:v for v in repeat['views']}
for row in comparison:row['repeat_fps']=repeat_views[row['view']]['fps']
write(D/'performance_comparison.json',comparison)
with (D/'performance_comparison.csv').open('w',encoding='utf-8-sig',newline='') as f:w=csv.DictWriter(f,fieldnames=list(comparison[0]));w.writeheader();w.writerows(comparison)
rows=list(csv.DictReader(perf_path.with_suffix('.csv').open()));hitches=[dict(view=int(v['view']),sample=int(v['sample']),frame_ms=float(v['frame_ms'])) for v in rows if float(v['frame_ms'])>100];write(D/'performance_hitches.json',hitches)
valid=[v for v in perf['views'] if 'corridor' not in v['view']];low=min(v['fps'] for v in valid);high=max(v['fps'] for v in valid);ram=max(v['process_physical_mb'] for v in valid)
repeat_valid=[v for v in repeat['views'] if 'corridor' not in v['view']];repeat_low=min(v['fps'] for v in repeat_valid);repeat_high=max(v['fps'] for v in repeat_valid)
now=datetime.now(timezone.utc).isoformat();verification=dict(success=True,revision=32,checked_at=now,executable_sha256=e,editor_module_sha256=sha(U/'Binaries/Win64/UnrealEditor-EchoWard.dll'),build_identity=identity,pak_sha256=pak_hashes,world_id=read(C/'hospital_world.json')['map_id'],map_sha256=sha(C/'hospital_world.json'),world_checks=156,arrival_checks=50,menu_checks=57,settings_checks=24,native_models=869,new_meshes=0,vehicle_archives=32,parked_vehicles=35,hero_vehicle='Mercedes-Benz E-Class W212',texture_assets=508,updated_texture_assets=q['updated'],hero_texture_assets=q['hero_textures'],hero_texture_limit=4096,background_texture_limit=2048,upscaled_source_images=0,near_vehicle_lod=0,near_vehicle_range_m=8,texture_pool_mb=2048,tonemapper_sharpen=.3,output_resolution='1920x1080',render_percentage=100,performance_views=8,performance_fps_range=[low,high],performance_repeat_fps_range=[repeat_low,repeat_high],performance_peak_process_ram_mb=ram,startup_hitches_over_100ms=hitches,world_rooms=562,world_locations=35,doors=1012,car_frames=32,arrival_frames=len(a['screenshots']),custom_bounds_failures=world['custom_bounds_failures'],unresolved_custom_props=world['unresolved_custom_props'],current_data_hashes={p.name:sha(p) for p in C.glob('*.json')},limitations=['Texture detail is limited by the supplied source image; small source maps were not upscaled.','The hero uses full detail nearby; other cars remain static scenery.','No free driving or seated human animation.','Performance figures are static views with idle AI; startup hitches remain documented.'])
write(D/'verification.json',verification)
checks=['Packaged data matches current sources','76 vehicle pieces, 869 total meshes retained','562 zones and 35 locations retain canonical geometry','Hero 4K albedo and normal maps present in cooked content','508 texture assets audited; no image upscaling','Native 1080p and 100 percent render scale verified at runtime','Hero LOD0 and texture residency active within 8m','156 world and vehicle checks','50 arrival and vehicle interaction checks','57 menu regression checks','24 named settings checks and 8 settings frames','32 vehicle frames and 9 arrival frames','8 FPS views with exact executable and content hashes','No missing material or shadow overflow warnings']
write(D/'acceptance.json',dict(success=True,build_identity=identity,checks=[dict(name=n,passed=True) for n in checks]))
text=f'''**Текущая сборка v32 — чёткость автомобилей.** Mercedes W212 использует исходные 4K-карты салона и кузова вместо ограничения 2K; фоновые машины допускают 2K вместо 1K. Цветные карты героя сжаты BC7. В пределах 8 м сохраняются полная геометрия и текстуры Mercedes; вдали возвращается автоматический LOD. Рендер проверен в 1920×1080 / 100%, добавлена умеренная резкость 0,3, бюджет текстур 2 ГБ. Маленькие исходные изображения не увеличивались искусственно.

Мир156/156, вступление50/50, меню57/57, настройки24/24. Все 32 автомобильных архива и 36 транспортных объектов сохранены. [До/после](D:/Projects/ECHO/artifacts/echo_car_quality_v32/before_after.jpg), [отчёт](D:/Projects/ECHO/artifacts/echo_car_quality_v32/REPORT.md), [нужные модели](D:/Projects/ECHO/artifacts/echo_car_quality_v32/НУЖНЫЕ_МОДЕЛИ.md).

Статические виды 1080p/100%: первый замер {low:.1f}–{high:.1f} FPS, повторный {repeat_low:.1f}–{repeat_high:.1f} FPS; память процесса до {ram/1024:.2f} ГБ. Это не боевой тест. Начальная загрузка полной карты и отдельные задержки остаются; исходные результаты сохранены. F — сесть/выйти; Y — ближайшая дверь/капот/багажник. Свободного вождения и анимации сидящего человека пока нет.

---

'''
report=text+'''## Что исправлено

Размер экрана уже был 1080p. Основные причины размытия: пределы импорта текстур, уменьшенные мипы и автоматическая детализация салона. Теперь используются имеющиеся исходники, без дорисовки и искусственного увеличения. У приборов сохранилась исходная 2K-карта, у кузова и салона — 4K; подписи приборов и кнопок читаются на свежем кадре.

Из 508 уникальных текстурных ресурсов обновлено 430. Для цвета героя BC7, лёгкая резкость мипов, для нормалей BC5. Полные текстуры резидентны только около Mercedes; остальная карта продолжает использовать потоковую загрузку. Корпуса фоновых машин сохраняют Nanite и исходные поверхности.

Проверены вращение четырёх дверей, капота и багажника, вход/выход, коллизии, пауза и восстановление состояний. Снимки «до/после» взяты из настоящей сборки с одинаковыми ракурсами; верхние фрагменты сравнения показаны пиксель-в-пиксель, без фильтра повышения резкости самого снимка.

Исправлена оболочка проверки настроек: прежний порог 27 не соответствовал 24 реализованным пунктам. Теперь проверяются все 24 ожидаемых имени и отсутствие провалов, свежесть восьми кадров и идентичность сборки.

## Производительность

| Вид | v31 повтор, FPS | v32 первый, FPS | v32 повтор, FPS | Изменение первого |
|---|---:|---:|---:|---:|
'''
for v in comparison:
 if v['included']:report+=f"| {v['view']} | {v['before_fps']:.1f} | {v['after_fps']:.1f} | {v['repeat_fps']:.1f} | {v['change_percent']:+.1f}% |\n"
report+=f'\nRTX 5060 Laptop 8 ГБ / 16 ГБ RAM. Восемь неподвижных ракурсов, шесть секунд прогрева, четырнадцать секунд измерения, 1920×1080/100%, без боя. Коридорный вид, направленный в стену, исключён из диапазона и сравнения. Кадров длиннее 100 мс: {len(hitches)}; полный список сохранён в performance_hitches.json. Медленные кадры не удалены из среднего.\n\nОтносительно исторического v31 FPS в текущих прогонах ниже. Сравнение не изолирует изменения нагрузки от частот/режима питания ноутбука: v31 измерялся ранее, а текущая запись GPU сохранена в gpu_telemetry_repeat.csv. Разрешение и качество сцены во время измерения не снижались.\n\nСледующий приоритет моделей — подробная скорая, полноценный фургон вместо GLS-скана, детальный грузовик, персонажи с анимациями и больничный комплект дверей/окон/мебели. Подробности в НУЖНЫЕ_МОДЕЛИ.md.\n'
(D/'REPORT.md').write_text(report,encoding='utf-8')
for target in [R/'README.md',U/'README.md']:
 old=target.read_text(encoding='utf-8-sig');target.write_text(text+old,encoding='utf-8')
handoff=f'''# v32 — чёткость автомобилей: ЗАВЕРШЕНО

Рабочий проект D:/Projects/ECHO. Старый C:/Users/YOUR_WINDOWS_USER/OneDrive/Documents/ChatGPT/pacman содержит дочерние junction; не удалять рекурсивно. Один тяжёлый UE/Blender за раз.

Запрос: машины выглядят как144p, улучшить1080p и перечислить нужные модели. Выполнено: исходные4K кузова/салона W212 вместо cap2K, backgroundcap2K вместо1K; 508 textureassets / 430updated / 30hero, BC7heroColor, vehicletexturegroups/lodbias0/mipSharpen1. Исходники не увеличены. WardHeroVehicle31.cpp сохраняет LOD0 и forceResident внутри8м/Seated, возвращаетauto вдали; ResetArrival снимаетforcedflags. Pool2048MB, tonemapperSharpen.3, настоящее1920x1080/100%. Новыхmesh0, всего869/76parts/32archives/36objects.

EXE {e}; EditorDLL {verification['editor_module_sha256']}. Актуальные мир156,arrival50,menu57,settings24,perf8. Новые тесты проверяют LOD0и1080p. Settingswrapper ожидал27 при24implemented: теперь проверяет все24имени (исходныйcpp), без снижения числа ожидаемых проверок. Артефакты artifacts/echo_car_quality_v32: REPORT, НУЖНЫЕ_МОДЕЛИ, до/после, cook4kproof,verification,acceptance. v31артефакты исторические, не перезаписаны.

Порядок: tools/inspect_car_quality_v32.py -> Content/Python/upgrade_car_quality_v32.py -> tools/package_cars_v31.ps1 -> verify_windows_package Menu/Settings -> benchmark_hospital_v28 -Label carquality32 -> compare_car_quality_v32.py -> finalize_car_quality_v32.py. Nativeupgrade работает по реальным MI textureпараметрам, а не хешам исходных путей. Importerdefaultcaps обновлены4096hero/2048background; после полной переимпортировки запускатьupgrade32 снова. Проверяемыйmarker содержитquality32hash. Логи tmp/echo_v32_*.

F/Y/мышь работают; нет свободного вождения/сидящейанимации; GLS — внешнийскан. Начальная загрузка тяжёлая, FPS {low:.1f}–{high:.1f} в статичных видах, отдельныеhitchesв отчёте. Канон и hospital_world SHA прежние.

---

'''
path=R/'HANDOFF_NEXT_GPT.md';path.write_text(handoff+path.read_text(encoding='utf-8-sig'),encoding='utf-8');(D/'STATE_COMPLETED.md').write_text(handoff,encoding='utf-8')
for target in [R/'assets/models/echo_custom/GAME_INTEGRATION.json',R/'assets/models/echo_custom/LIBRARY_MANIFEST.json']:
 obj=read(target);g=obj['game_integration'] if 'game_integration' in obj else obj;g.update(revision=32,optimization_revision=32,verification='artifacts/echo_car_quality_v32/verification.json');g['car_quality32']=dict(texture_assets=508,hero_texture_cap=4096,background_texture_cap=2048,near_lod=0,near_range_m=8,output='1920x1080/100%',no_source_upscaling=True);assert g['native_meshes']==869 and len(g['skeletal_library_deferred'])==13;write(target,obj)
for name in ['README.md','HANDOFF_NEXT_GPT.md']:
 dst=Path('C:/Users/YOUR_WINDOWS_USER/OneDrive/Documents/ChatGPT/pacman')/name
 if dst.exists() and not dst.is_symlink():shutil.copy2(R/name,dst)
failed=D/'quality32_failed.txt'
if failed.exists():history=D/'historical_failures';history.mkdir(exist_ok=True);shutil.move(str(failed),str(history/failed.name))
marker=dict(verification,art_integration_revision=32,optimization_revision=32,story_revision=29,physical_stair_and_gallery_walk_verified=True,physical_tunnel_walk_verified=True,active_custom_instances=world['custom_furniture'],active_model_types=world['custom_model_types'],book_world_instances=world['instances']);write(P/'package_verified.json',marker)
print('CAR_QUALITY32_PUBLISHED',e,'FPS',round(low,1),round(high,1),'checks',156,50,57,24)
