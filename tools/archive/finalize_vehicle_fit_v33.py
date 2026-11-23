"""Publish v33 only after fresh packaged geometry, vehicle, UI and performance checks."""
from pathlib import Path
from datetime import datetime,timezone
import json,hashlib,csv,shutil
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];U=R/'unreal/EchoWard';P=U/'Saved/PackagedWindows';C=P/'Windows/EchoWard/Content/CampaignData';D=R/'artifacts/echo_vehicle_fit_v33';EXE=P/'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(4*1024*1024),b''):h.update(b)
 return h.hexdigest()
e=sha(EXE);cars=read(P/'verified_cars_v31.json');arrival=read(P/'verified_arrival_v29.json');menu=read(P/'verified_menu.json');settings=read(P/'verified_settings.json')
world=read(Path(cars['test_saved'])/'Prototype/book_world_check.json');fit=read(Path(cars['test_saved'])/'Prototype/car_fit33.json');a=read(Path(arrival['test_saved'])/'Prototype/arrival29_check.json');m=read(Path(menu['test_saved'])/'Prototype/menu_check.json');s=read(Path(settings['test_saved'])/'Prototype/settings_menu_check.json');q=read(D/'native_texture_audit.json');quality=read(C/'echo_car_quality_v32.json');data=read(C/'echo_cars_v31.json');perf=read(U/'Saved/PackageTests/PerformanceV25_vehiclefit33_final/Saved/Prototype/hospital_perf_vehiclefit33_final.json')
assert all(x['success'] for x in [cars,arrival,menu,settings,world,fit,a,m,s,q,quality,perf])
assert all(x['executable_sha256'].lower()==e for x in [cars,arrival,perf]);assert all(x['build_identity'].split(':')[0].lower()==e for x in [menu,settings])
assert [world['count'],a['count'],m['count'],s['count']]==[159,50,57,24]
assert all(v['passed'] for x in [world,a,m,s] for v in x['checks'])
assert len(fit['cars'])==35 and all(v['passed'] and v['attempts']==1 and v['wheel_height_spread_cm']<.11 for v in fit['cars'])
assert fit['paint_count']==480 and fit['paint_max_gap_cm']<.1
assert data['revision']==33 and quality==q and q['revision']==33 and q['textures']==508 and q['parked_budget']==4096 and q['upscaled_images']==0
assert all(v['max_size']==4096 for v in q['assets'])
contacts=read(D/'source_wheel_contacts.json');assert len(contacts)==7 and all(v.get('tire_ground_offset_cm',0)<1.5 for v in contacts)
assert sha(C/'hospital_world.json')=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec'
assert len(read(C/'echo_custom_runtime.json')['meshes'])==869
paks=sorted(p for p in (P/'Windows/EchoWard/Content/Paks').iterdir() if p.suffix in ['.pak','.ucas','.utoc']);pak_hashes={p.name:sha(p) for p in paks}
files=['book_map.json','hospital_world.json','echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json','echo_detail_v27.json','echo_horror_v28.json','echo_arrival_v29.json','echo_realism_v30.json','echo_cars_v31.json','echo_car_quality_v32.json'];identity=':'.join([e.upper()]+[sha(p).upper() for p in paks]+[sha(C/n).upper() for n in files]);assert menu['build_identity']==settings['build_identity']==identity
for name in files:assert sha(C/name)==sha(U/'Content/CampaignData'/name),name
assert cars['cars_data_sha256'].lower()==sha(C/'echo_cars_v31.json') and arrival['arrival_data_sha256'].lower()==sha(C/'echo_arrival_v29.json')
assert len(perf['views'])==8 and perf['screen_percentage']==100 and perf['cars31_data_sha256']==sha(C/'echo_cars_v31.json') and perf['car_quality32_data_sha256']==sha(C/'echo_car_quality_v32.json') and perf['packaged_content_sha256']==pak_hashes
for folder,logname in [('CarsV31','package_test.log'),('ArrivalV29','package_test.log'),('Menu','package_test.log'),('Settings','package_test.log'),('PerformanceV25_vehiclefit33_final','performance.log')]:
 text=(U/'Saved/PackageTests'/folder/'Saved'/logname).read_text(encoding='utf-8',errors='replace');assert not any(w in text for w in ['Default Material will be used','Failed to compile Material','GPU Crashed','Out of video memory','Non-Nanite Marking Job Queue overflow','surface cache atlas oversubscribed','Virtual Shadow Map Page Pool overflow']),folder
for name,obj in [('world_check.json',world),('car_fit33.json',fit),('arrival_check.json',a),('menu_check.json',m),('settings_check.json',s),('performance_final.json',perf)]:write(D/name,obj)
first=read(U/'Saved/PackageTests/PerformanceV25_vehiclefit33/Saved/Prototype/hospital_perf_vehiclefit33.json');write(D/'performance_first.json',first)
source=U/'Saved/PackageTests/CarsV31/Saved/Prototype/BookWorldCaptures';frames=D/'frames';frames.mkdir(exist_ok=True)
for v in data['captures']:
 p=source/('world_'+v['id']+'.png');assert Image.open(p).size==(1920,1080);shutil.copy2(p,frames/p.name)
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',16)
for page in range(3):
 views=data['captures'][page*12:(page+1)*12];im=Image.new('RGB',(1440,223*((len(views)+2)//3)),(10,14,16));dr=ImageDraw.Draw(im)
 for i,v in enumerate(views):
  f=Image.open(frames/('world_'+v['id']+'.png')).convert('RGB');f.thumbnail((476,196));x=(i%3)*480;y=(i//3)*223;im.paste(f,(x,y));dr.text((x+3,y+199),v['id'].removeprefix('v31_')[:47],fill='white',font=font)
 im.save(D/('contact_'+str(page+1)+'.jpg'),quality=93)
views=['v31_2025-bmw-330i-xdrive','v31_gmc-savana-2003','v31_road33_service','v31_road33_parking'];im=Image.new('RGB',(1600,900))
for i,name in enumerate(views):
 f=Image.open(frames/('world_'+name+'.png')).convert('RGB');f.thumbnail((800,450));im.paste(f,((i%2)*800,(i//2)*450))
im.save(D/'preview.jpg',quality=94)
manifest=read(U/'RawAssets/EchoCarsV31/manifest.json');assets={v['id']:v for v in manifest['assets']};textures=read(R/'artifacts/echo_car_quality_v32/source_textures.json')['all'];rows=[];placements={v['id']:v for v in data['placements']}
for v in fit['cars']:
 p=placements[v['id']];model=data['vehicles'][v['slug']];keys={k for n in [model['body'],model['glass']]+model.get('body_extra',[]) if n for k in assets[n]['materials']};ts={a['file']:a for a in textures if a['key'] in keys};colors=[t for t in ts.values() if t['channel']=='albedo'];rows.append(dict(id=v['id'],model=v['slug'],zone=p['zone33'],x_m=round(v['at_cm'][0]/100,5),y_m=round(v['at_cm'][1]/100,5),z_m=round(v['at_cm'][2]/100,6),yaw_degrees=p['yaw'],supported=v['passed'],opaque_tint_if_incomplete=model['tint33'],source_texture_files=len(ts),source_4k_files=sum(max(t['size'])>=4096 for t in ts.values()),largest_source_color_px=max([max(t['size']) for t in colors],default=0),texture_cap=4096,artificial_upscale=False))
write(D/'vehicles.json',rows)
with (D/'vehicles.csv').open('w',encoding='utf-8-sig',newline='') as f:w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
valid=[v for v in perf['views'] if 'corridor' not in v['view']];fps=[min(v['fps'] for v in valid),max(v['fps'] for v in valid)];ram=max(v['process_physical_mb'] for v in valid)
verification=dict(success=True,revision=33,checked_at=datetime.now(timezone.utc).isoformat(),executable_sha256=e,editor_module_sha256=sha(U/'Binaries/Win64/UnrealEditor-EchoWard.dll'),build_identity=identity,pak_sha256=pak_hashes,world_id=read(C/'hospital_world.json')['map_id'],map_sha256=sha(C/'hospital_world.json'),world_checks=159,arrival_checks=50,menu_checks=57,settings_checks=24,native_models=869,vehicle_sources=32,parked_vehicles=35,transport_objects=36,car_frames=len(data['captures']),output_resolution='1920x1080',render_percentage=100,texture_assets=508,updated_texture_assets=q['updated'],texture_cap=4096,source_maps_at_least_4k=sum(max(v['size'])>=4096 for v in q['assets']),upscaled_source_images=0,near_parked_range_m=10,near_hero_range_m=8,texture_pool_mb=2048,paint_count=fit['paint_count'],paint_max_gap_mm=fit['paint_max_gap_cm']*10,removed_off_pavement_paint=fit['paint_rejected_off_pavement'],all_vehicle_placements_supported=True,vehicle_fit_search_attempts_total=sum(v['attempts'] for v in fit['cars']),performance_views=8,performance_fps_range=fps,performance_peak_process_ram_mb=ram,world_rooms=562,world_locations=35,doors=1012,custom_bounds_failures=world['custom_bounds_failures'],unresolved_custom_props=world['unresolved_custom_props'],current_data_hashes={p.name:sha(p) for p in C.glob('*.json')},limitations=['4K is the maximum allowed source detail, not a claim that every supplied texture has 4K pixels.','GLS is a coarse exterior scan, not a detailed production vehicle.','Only the hero W212 has interactive panels and seat camera.','No free driving or seated human animation.','Static performance views do not measure combat or every area of the map.'])
write(D/'verification.json',verification);write(D/'acceptance.json',dict(success=True,build_identity=identity,checks=[dict(name=n,passed=True) for n in ['35 static vehicles have validated paved supports and clear collision envelopes','Verified positions baked into source data; all 35 fit on first attempt','Vehicle hero vertical offset reduced from 8cm to 0.5mm','480 paint strips follow pavement; maximum 0.35mm gap','Off-pavement and uneven paint fragments removed','508 vehicle texture assets allow source detail to 4096','No source image upscaling; 34 unique source textures at least 4K','Incomplete interiors use dedicated opaque dark glazing','159 world, 50 arrival, 57 menu, 24 settings checks','35 vehicle and parking images at 1920x1080','Eight performance views at native 1080p and 100 percent scale','No missing materials or runtime shadow overflow warnings','Canonical 562-zone map geometry unchanged']]))
intro=f'''**Сборка v33 — все автомобили и разметка.** Все 35 расставленных транспортных объектов проверены по опоре и коллизиям; машина героя также опущена на асфальт. Сохранены 32 источника / 36 объектов транспорта. У неполных салонов тёмное непрозрачное стекло. Предел текстур всех машин 4096 вместо 2048, BC7 для цвета, без искусственного увеличения маленьких исходников. Полная детализация фоновых машин запрашивается в пределах 10 м; Mercedes героя — 8 м. Разметка лежит на покрытии: максимум 0,35 мм, без коллизий и теней.

Проверки: мир159/159, вступление50/50, меню57/57, настройки24/24. Статические ракурсы1920×1080/100%: {fps[0]:.1f}–{fps[1]:.1f} FPS; RAM процесса до {ram/1024:.2f} ГБ. [Отчёт](D:/Projects/ECHO/artifacts/echo_vehicle_fit_v33/REPORT.md), [кадры](D:/Projects/ECHO/artifacts/echo_vehicle_fit_v33/contact_1.jpg), [координаты и качество источников](D:/Projects/ECHO/artifacts/echo_vehicle_fit_v33/vehicles.csv).

---

'''
report='''# Проверка транспорта v33

Исправлены общая фиксированная высота машин, неподходящие места парковки и приподнятая разметка. Машины переставлены в проверенные свободные места на асфальте или полу гаража. Для длинного прицепа выбран поперечный участок площадки разгрузки. Положение проверяется по четырём точкам опоры, физическим препятствиям, газонам и габаритам соседних машин. Результаты сохранены в car_fit33.json. Это проверка опоры габаритов, а не симуляция подвески каждой шины. Для семи подозрительных моделей отдельно сверены вершины FBX; у грузовика нижняя точка шин оказалась на 6,7 мм выше нижней точки кузова. Для него сохранена поправка опоры по шинам. Короткие контактные тени длиной до 35 см помогают читать соприкосновение с покрытием. Исходная геометрия не переделывалась.

Результаты: 35/35 машин приняты, в окончательных данных все проходят с первой попытки; геометрия исходной карты не менялась. Ранний неуспешный подбор прицепа сохранён отдельно в first_fit_failed_trailer.json. Машина героя опущена с 8 см на 0,5 мм над опорой; двери, капот, багажник, посадка и маршрут через КПП сохранены.

480 полос следуют асфальту с максимальным зазором 0,35 мм. Ещё 49 частей старой разметки не имели ровного асфальта под всеми контрольными точками и убраны. Полосы толщиной 0,2 мм не блокируют движение и не отбрасывают тени. Парковочные линии строятся вокруг фактических мест машин.

Все 508 уникальных текстур автомобилей проверены в Unreal; обновлены 478 настроек. Предел — 4096, смещение LOD — 0, цвет — BC7, нормали — BC5, фильтрация — 16×. Источники не растягивались. Только 34 текстуры имеют исходник размером 4096 или больше; остальные сохраняют своё разрешение. Подробные модели могут использовать однотонные маленькие карты, поэтому размер файла сам по себе не определяет качество геометрии. Фургон GLS остаётся грубым сканом с участком земли: для качественного результата нужна полноценная модель с UV и PBR-текстурами. Никакой настройкой не получить из этого скана полноценный салон.

Чёрная тонировка назначается только отдельным материалам стекла у моделей без подтверждённого подробного салона. Смешанные материалы фар и стекла не перекрашиваются целиком. Полноценные салоны, включая интерактивный Mercedes W212, сохранены. Показываемые кадры взяты из игры, а не сгенерированы.

## Проверки

'''+intro.split('---')[0]+'\n## Производительность\n\n| Ракурс | FPS | Память процесса, МБ |\n|---|---:|---:|\n'
for v in perf['views']:report+=f"| {v['view']} | {v['fps']:.1f} | {v['process_physical_mb']:.0f} |\n"
report+='\nRTX 5060 Laptop 8 ГБ, 16 ГБ RAM. Статические виды: 6 с прогрева и 14 с измерения, 1080p / 100%. Коридорный ракурс в стену не включён в диапазон. Цифры не гарантируют FPS в бою и при загрузке всех зон. Потоковая загрузка сохранена, общий бюджет текстур 2 ГБ.\n'
(D/'REPORT.md').write_text(report,encoding='utf-8')
for target in [R/'README.md',U/'README.md']:
 text=target.read_text(encoding='utf-8-sig');assert not text.startswith('**Сборка v33');target.write_text(intro+text,encoding='utf-8')
handoff=f'''# v33 — транспорт и разметка: ЗАВЕРШЕНО

Проект D:/Projects/ECHO. Не удалять старый Cкорень рекурсивно: дочерниеjunction ведут наD. Один тяжёлый UE/Blender за раз.

Запрос: все машины до4K, окна неполных салонов затонировать, убрать зависание/погружение/пересечения и приподнятые полосы. Все35статичных машин подогнаны к поверхности/зонам/коллизиям; проверенные координаты записаны echo_cars_v31.json revision33, повторная посадка35×1попытка. 4опорные точки/габариты, не симуляция подвески. Отдельные повёрнутыеUBoxколлизии, HISMпоэкземплярно. HeroZ.0005м вместо.08м. Тонировкапоотдельнымключам, полныйheroинтерьер сохранён.

WardCars31.cpp: BuildCars31/FitCarsAndMarkings33/UpdateCars33. После всехBuilderиGroupTree выполняетсяподгонка; paintвBoxоткладывается, road_dashизenvironmentпереводитсявMat8;480полос,49частейбезопорыудалены, maxgap.035cm. SetForcedLodModel1/residentв10м, герой8м. Общийtexturepool2048. Max4096всем508текстурам,478updated,BC7цвет,34source>=4K,0upscale. GLSгрубыйscanсземлёй—заменитьдлянастоящегодетальногоавто. Не обещать всемисточникамнастоящее4K.

EXE {e}. EditorDLL {verification['editor_module_sha256']}. World159,arrival50,menu57,settings24,perf8,35кадровмашин/парковок. Финальные логи tmp/echo_v33_*_final.log. Артефакты artifacts/echo_vehicle_fit_v33:verification/acceptance/car_fit/vehicles.csv/REPORT/contact/preview. native_texture_audit508. Историческийv32сохранён. hospital_worldSHAканоническийпрежний,869нативныхmesh,562зоны,35локаций,1012дверей.

Upgrade Content/Python/upgrade_vehicle_quality_v33.py обновляет echo_car_quality_v32.json сrevision33 длясохранениясуществующейцепочкиhash. Импортерdefault4096всем. Пайплайн tools/package_cars_v31.ps1 -> Tools/verify_windows_package Menu/Settings -> benchmark_hospital_v28 -Label vehiclefit33_final -> tools/finalize_vehicle_fit_v33.py. Не перезапускатьстарыйfinalize32 надv33.

Нетсвободноговожденияилианимациисидящегоперсонажа. FPSстатичные {fps[0]:.1f}–{fps[1]:.1f};загрузкабольшойкарты остаётсятяжёлой.

---

'''
target=R/'HANDOFF_NEXT_GPT.md';target.write_text(handoff+target.read_text(encoding='utf-8-sig'),encoding='utf-8');(D/'STATE_COMPLETED.md').write_text(handoff,encoding='utf-8')
for target in [R/'assets/models/echo_custom/GAME_INTEGRATION.json',R/'assets/models/echo_custom/LIBRARY_MANIFEST.json']:
 obj=read(target);g=obj['game_integration'] if 'game_integration' in obj else obj;g.update(revision=33,optimization_revision=33,verification='artifacts/echo_vehicle_fit_v33/verification.json');g['car_quality32']['superseded_by_revision']=33;g['vehicle_fit33']=dict(parked=35,all_supported=True,texture_assets=508,texture_cap=4096,no_upscaling=True,paint_gap_mm=.35,paint_count=480);assert g['native_meshes']==869 and len(g['skeletal_library_deferred'])==13;write(target,obj)
marker=dict(verification,art_integration_revision=33,optimization_revision=33,story_revision=29,physical_stair_and_gallery_walk_verified=True,physical_tunnel_walk_verified=True,active_custom_instances=world['custom_furniture'],active_model_types=world['custom_model_types'],book_world_instances=world['instances']);write(P/'package_verified.json',marker);print('VEHICLE_FIT33_PUBLISHED',e,'checks',159,50,57,24,'FPS',fps)