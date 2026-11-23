"""Publish only matching native runtime, render and performance evidence."""
from pathlib import Path
import json,hashlib,datetime,shutil,difflib
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';C=P/'Content/CampaignData';O=R/'artifacts/echo_realism_v30'
read=lambda f:json.loads(f.read_text(encoding='utf-8-sig'));sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
old=read(O/'before_identity.json');audit=read(O/'acceptance.json');a=read(C/'echo_realism_v30.json');exe=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');editor=P/'Binaries/Win64/UnrealEditor-EchoWard.dll'
assert audit['success'] and audit['executable_sha256']==exe
assert sha(P/'Binaries/Win64/EchoWard.exe')==exe
assert editor.stat().st_mtime>=max(q.stat().st_mtime for q in (P/'Source/EchoWard').iterdir() if q.suffix in ['.h','.cpp','.cs'])
states={k:read(PK/n) for k,n in [('world','verified_world.json'),('review','verified_realism_v30.json'),('arrival','verified_arrival_v29.json')]}
for key,state in states.items():assert state['success'] and state.get('executable_sha256',state.get('build_identity','').split(':')[0]).lower()==exe,key
w=read(Path(states['world']['test_saved'])/'Prototype/book_world_check.json');rw=read(Path(states['review']['test_saved'])/'Prototype/book_world_check.json');arr=read(Path(states['arrival']['test_saved'])/'Prototype/arrival29_check.json');doors=read(Path(states['world']['test_saved'])/'Prototype/every_door_check.json');envelope=read(Path(states['world']['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and rw['success'] and w['count']==rw['count']==144 and w['rooms']==562 and w['locations']==35
assert w['realism30_props']==rw['realism30_props']==len(a['props'])==54
assert w['custom_bounds_failures']==w['unresolved_custom_props']==0 and doors['success'] and doors['doors']==1012 and envelope['success']
assert arr['success'] and arr['count']==27 and states['review']['data_sha256'].lower()==sha(C/'echo_realism_v30.json')
perfbase=P/'Saved/PackageTests/PerformanceV25_v30_before/Saved/Prototype';perfafter=P/'Saved/PackageTests/PerformanceV25_v30_after/Saved/Prototype'
before=read(perfbase/'hospital_perf_v30_before.json');after=read(perfafter/'hospital_perf_v30_after.json');assert before['success'] and after['success'] and after['executable_sha256']==exe and before['executable_sha256']==old['executable_sha256']
assert after['realism_data_sha256']==sha(C/'echo_realism_v30.json')
for n,h in after['packaged_content_sha256'].items():assert sha(PK/'Windows/EchoWard/Content/Paks'/n)==h,n
assert before['screen_percentage']==after['screen_percentage']==100 and before['gpu']==after['gpu'] and len(before['views'])==len(after['views'])==8
assert [x['view'] for x in before['views']]==[x['view'] for x in after['views']]
comparison=[]
for b,n in zip(before['views'],after['views']):
 comparison.append(dict(view=b['view'],before_fps=b['fps'],after_fps=n['fps'],change_percent=(n['fps']/b['fps']-1)*100,before_p95_ms=b['p95_ms'],after_p95_ms=n['p95_ms'],included=b['view']!='corridor_horror'))
(O/'performance_comparison.json').write_text(json.dumps(dict(success=True,scope=after['scope'],resolution='1920x1080 / 100%',gpu=after['gpu'],results=comparison,excluded_reason='corridor_horror faces a nearby wall and is not representative',maximum_process_ram_mb=max(q['process_physical_mb'] for q in after['views'])),indent=2))
for key,state in states.items():
 log=(Path(state['test_saved'])/'package_test.log').read_text(encoding='utf8',errors='replace')
 assert not any(x in log for x in ['Default Material will be used','Failed to compile Material','GPU Crashed','Out of video memory'])
for kind,file in [('world','book_world_check'),('world','every_door_check'),('world','envelope_check'),('arrival','arrival29_check')]:shutil.copy2(Path(states[kind]['test_saved'])/('Prototype/'+file+'.json'),O/('native_'+file+'.json'))
for label,src in [('before',perfbase),('after',perfafter)]:
 for ext in ['json','csv']:shutil.copy2(src/('hospital_perf_v30_'+label+'.'+ext),O/('performance_'+label+'.'+ext))
folder=O/'captures';folder.mkdir(exist_ok=True);files=[]
def capture(src,dest):
 assert src.stat().st_mtime>=(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe').stat().st_mtime
 with Image.open(src) as im:assert im.size==(1920,1080);im.verify()
 shutil.copy2(src,dest);return dest
for v in a['captures']:files.append(capture(Path(states['review']['test_saved'])/('Prototype/BookWorldCaptures/world_'+v['id']+'.png'),folder/(v['id']+'.png')))
arrivalfolder=O/'arrival';arrivalfolder.mkdir(exist_ok=True)
for name in arr['screenshots']:capture(Path(states['arrival']['test_saved'])/'Prototype/ArrivalCaptures'/name,arrivalfolder/name)
font=ImageFont.truetype(str(C/'Fonts/NotoSans.ttf'),18)
def sheet(paths,name,cols=2):
 out=Image.new('RGB',(640*cols,390*((len(paths)+cols-1)//cols)),'#0c1417');d=ImageDraw.Draw(out)
 for i,f in enumerate(paths):
  with Image.open(f) as raw:pic=raw.convert('RGB').resize((640,360))
  x=i%cols*640;y=i//cols*390;out.paste(pic,(x,y));d.text((x+9,y+365),f.stem,font=font,fill='white')
 out.save(O/name,quality=94)
sheet(files,'contact.jpg');sheet([perfbase/'perf_v30_before_7.png',perfafter/'perf_v30_after_7.png'],'before_after.jpg')
sheet([arrivalfolder/'01_drive.png',perfafter/'perf_v30_after_7.png',folder/'v30_maintenance_cart.png',folder/'v30_industrial_pendant.png'],'preview.jpg')
patch=[]
for f in O.glob('*.before'):
 now=P/'Source/EchoWard'/f.name.removesuffix('.before')
 if now.exists():patch+=list(difflib.unified_diff(f.read_text(encoding='utf-8-sig').splitlines(True),now.read_text(encoding='utf-8-sig').splitlines(True),fromfile='v29/'+now.name,tofile='v30/'+now.name))
(O/'source_changes.patch').write_text(''.join(patch),encoding='utf8')
current={f.name:sha(f) for f in C.glob('*.json')}
for name,h in current.items():assert sha(PK/'Windows/EchoWard/Content/CampaignData'/name)==h,name
report={k:old[k] for k in ['world_id','map_sha256','horror_revision','detail_revision','horror_data_sha256','detail27_data_sha256']}
report.update(success=True,checked_at=datetime.datetime.now().astimezone().isoformat(),executable_sha256=exe,editor_module_sha256=sha(editor),build_identity=states['world']['build_identity'],art_integration_revision=30,optimization_revision=30,story_revision=29,native_models=audit['native_models'],new_native_models=7,realism30_props=54,active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],book_world_instances=w['instances'],world_checks=144,world_rooms=562,world_locations=35,doors=1012,arrival_checks=27,realism_render_checks=144,current_realism_captures=len(files),current_arrival_captures=len(arr['screenshots']),current_performance_views=8,bounds_failures=0,unresolved_custom_props=0,current_data_hashes=current,arrival_data_sha256=current['echo_arrival_v29.json'],realism_data_sha256=current['echo_realism_v30.json'],performance_comparison='artifacts/echo_realism_v30/performance_comparison.json',prior_menu_and_story_tests={'revision':29,'note':'Historical accepted v29 menu/story checks; no new menu/story script changes in v30. Current arrival and full-map route/save tests rerun.'})
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
names={'v23_court_S01':'Парковка, яркий осмотр','v23_lab_clutter':'Лаборатория, яркий осмотр','v23_detail_126':'Рабочий пост, яркий осмотр','v23_court_S06':'Ворота, яркий осмотр','v28_v23_lab_clutter_horror':'Лаборатория, хоррор','v28_v23_detail_126_horror':'Рабочий пост, хоррор','v28_v23_court_S01_horror':'Парковка, хоррор'}
lines=['# ЭХО — качество и оптимизация v30','','В игру внедрены семь новых моделей из открытых источников: четырёхдверный седан Innerscene; мусорный мешок, защищённый настенный светильник, промышленный подвес, сварочная тележка, служебная роллета и секция сетки Poly Haven. Исходники и прежние модели сохранены. Все новые источники CC0, URL/авторы/хэши находятся в assets/external/quality_v30.','','Парковочные автомобили и машина вступления используют новый седан. Для 6 парковочных машин расширены только целевые габариты модели внутри существующих мест, чтобы сохранить полный размер вместо уменьшения до старого купе; их центры и планировка сохранены. Его реальный размер после удаления преувеличенной антенны — 4,840 × 2,181 × 1,447 м. 131554 треугольника в исходном LOD0 против 213347 у прежнего CarConcept. Десять секций материалов, четыре нативных LOD; стекло и резина отделены, автомобильный лак использует Clear Coat. Исправлена обратная ось модели, чтобы машина ехала передом. Посадка водителя и открывание автомобильной двери остаются прежними прототипами.','','Добавлены **54** измеренных объекта: 8 сварочных тележек, 16 мешков, 18 настенных/КПП светильников, 8 промышленных подвесов, 2 закрытые служебные роллеты и 2 секции сетки. Роллеты — отделка глухой стены, не замена действующих дверей. При тестировании три предмета перекрывали укрытия: расстановка исправлена, теперь учитывает объём укрытия, подход к нему, ниши и перегородки.','','Асфальт заменён на скан Asphalt 02: цвет 4K, нормали и шероховатость 2K; разные влажные и сухие участки, без тесселяции. Дорожная краска стала толщиной 2 мм вместо 2 см. Ночной холодный и аварийный красный свет, туман, кровь, режим осмотра B и фонарь N сохранены.','','Оптимизация: нативные LOD на шести новых моделях выше 1500 треугольников; роллете из 552 треугольников они не нужны. Мелкие предметы исчезают на 12–35 м по размеру, крупные сохраняют прежнюю дистанцию, парковочные машины видны до 140 м. Предметы меньше 20 см имеют отдельные группы без собственных динамических теней. Переимпорт машины сначала создал 64 секции вместо 10; это выявила проверка готового рендера. Ресурс пересоздан из сохранённого исходника, обычный повтор импорта теперь переиспользует проверенный mesh. Разрешение и освещение при замере не снижались.','','## Производительность','','RTX 5060 Laptop, 1920×1080, масштаб 100%, без ограничения FPS. Восемь одинаковых ракурсов до/после, прогрев 6 секунд и выборка 14 секунд. Сравниваются целые сборки с изменёнными моделями и материалами, а не отдельное изолированное изменение. AI неподвижен; это не замер боя или всей кампании.','','| Сцена | До, FPS | После, FPS | Изменение | P95 после, мс |','|---|---:|---:|---:|---:|']
for q in comparison:
 if q['included']:lines.append(f"| {names[q['view']]} | {q['before_fps']:.1f} | {q['after_fps']:.1f} | {q['change_percent']:+.1f}% | {q['after_p95_ms']:.1f} |")
lines+=['','Восьмой диагностический ракурс смотрит в стену и исключён из выводов. Полные данные и CSV сохранены рядом с отчётом.','','## Проверки','','Game и Editor собраны; cook/stage/archive успешны. На окончательном EXE: карта **144/144**, все **1012** дверей и проверки оболочки, укрытий, маршрутов и сохранений; вступление **27/27** и 7 кадров; визуальная проверка оформления **144/144** и 6 кадров 1080p. Семь моделей загружаются из нативных ресурсов, у машины проверены реальные четыре LOD и не больше 10 секций материалов. Проверено 41 скачивание Poly Haven по MD5/SHA256 и исходный GLB по SHA256. Старые 786 записей нативной библиотеки сохранены; теперь их 793, включая отдельные записи дверных полотен. Полная карта по-прежнему строится при запуске: в этих тестах это занимало порядка полутора–двух минут, ускорение этого этапа не заявляется. Полный ручной обход каждой из 562 зон и всех поздних сюжетных глав не проводился. Меню и дальнейшие сюжетные сценарии в этом проходе не менялись; их предыдущие проверки v29 — исторические.','','## Источники','','- [Седан Innerscene, CC0](https://www.innerscene.com/tools/library/3d-parts/graphite-four-door-sedan-eed96d12).','- [Poly Haven: CC0](https://polyhaven.com/license); ссылки на каждую модель и Asphalt 02 записаны в SOURCE_MANIFEST.json / ROAD_MANIFEST.json / ALPHA_MANIFEST.json.','','Это проход по машинам, покрытиям и инженерным деталям. Остальная старая мебель, персонажи и часть геометрии ещё упрощены: вся карта не стала фотореалистичной после этой замены. Нет новой актёрской озвучки или полной постановки поздних глав.','','Запуск: D:/Projects/ECHO/PLAY_UNREAL.cmd. Для обновлённого въезда выбери новую игру. Старые сохранения сохраняют своё место.','',f'EXE SHA256: {exe}']
(O/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf8')
marker=PK/'package_verified.tmp';marker.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');marker.replace(PK/'package_verified.json')
print(json.dumps(dict(success=True,exe=exe,props=54,models=793,world_checks=144,arrival_checks=27,render_checks=144,views=len(files))))
