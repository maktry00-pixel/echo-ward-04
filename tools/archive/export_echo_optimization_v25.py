"""Publish only matching, validated runtime and performance evidence."""
from pathlib import Path
import hashlib,json,shutil,datetime
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_optimization_v25';PK=P/'Saved/PackagedWindows'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
LABELS={'v23_court_S01':'Парковка у входа','v23_lab_clutter':'Лаборатория','v23_detail_126':'Комната персонала','v23_court_S06':'Внешний КПП'}
before=read(O/'before_identity.json');old=read(R/'artifacts/echo_real_v23/verification.json');exe=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe')
assert exe==sha(P/'Binaries/Win64/EchoWard.exe')
for n,h in before['data_hashes'].items():
 assert sha(P/'Content/CampaignData'/n)==h and sha(PK/'Windows/EchoWard/Content/CampaignData'/n)==h,n
status={name:read(PK/name) for name in ['verified_world.json','verified_worldvisual_v23.json','verified_storyvisual_v7.json','verified_native_menu_v23.json']}
for name,s in status.items():
 assert s['success'] and (s['build_identity'].split(':')[0].lower() if 'build_identity' in s else s['executable_sha256'].lower())==exe,name
assert status['verified_world.json']['build_identity'].lower().split(':')[1:]==old['build_identity'].lower().split(':')[1:],'Packaged art or campaign payload changed'
w=read(Path(status['verified_world.json']['test_saved'])/'Prototype/book_world_check.json');v=status['verified_worldvisual_v23.json'];s=status['verified_storyvisual_v7.json'];m=status['verified_native_menu_v23.json'];visual=read(Path(v['test_saved'])/'Prototype/book_world_check.json')
assert w['success'] and w['count']==128 and w['rooms']==562 and w['locations']==35
assert visual['success'] and visual['count']==54 and s['checks']==25 and m['checks']==57
assert w['real_props']==old['new_real_props']==16107 and w['quality_props']==old['quality_props']==28850 and w['custom_bounds_failures']==w['unresolved_custom_props']==0
assert w['custom_furniture']==old['active_custom_instances']==81825 and w['custom_model_types']==old['active_model_types']==241
assert w['custom_door_leaves']==old['custom_door_leaves']==2024 and w['quality_trees']==old['quality_trees']==3940 and w['quality_windows']==old['quality_windows']==836
assert w['instances']==read(R/'artifacts/echo_real_v23/native_world_check.json')['instances']
assert read(Path(status['verified_world.json']['test_saved'])/'Prototype/every_door_check.json')['success']
assert read(Path(status['verified_world.json']['test_saved'])/'Prototype/envelope_check.json')['success']
initial=read(P/'Saved/PackageTests/PerformanceV25_baseline/Saved/Prototype/hospital_perf_baseline.json')
bas=read(P/'Saved/PackageTests/PerformanceV25_baseline_recent/Saved/Prototype/hospital_perf_baseline_recent.json');opt=read(P/'Saved/PackageTests/PerformanceV25_optimized/Saved/Prototype/hospital_perf_optimized.json')
assert bas['success'] and opt['success'] and bas['screen_percentage']==opt['screen_percentage']==100
comparison=[]
repeat=read(P/'Saved/PackageTests/PerformanceV25_repeat/Saved/Prototype/hospital_perf_repeat.json')
assert repeat['success'] and repeat['screen_percentage']==100
assert opt['executable_sha256']==repeat['executable_sha256']==exe
assert bas['executable_sha256']!=exe
for logfile in ['echo_v25_control2_build.log','echo_v25_accepted2_build.log']:
 log=(R/'tmp'/logfile).read_text(encoding='utf-8-sig',errors='replace')
 assert all('Compile [x64] '+name in log for name in ['WardBookWorld.cpp','WardHospitalDetails.cpp']),logfile
assert sha(R/'unreal/EchoWard/Source/EchoWard/WardEchoPlacement.cpp')==sha(O/'WardEchoPlacement.cpp.before')
for b,a in zip(bas['views'],opt['views']):
 assert a['view']==b['view'] and a['samples']>100 and b['samples']>100
 # Review found the custom fifth camera inside a wall. Keep raw diagnostics,
 # but exclude that view from comparisons and any performance claim.
 if a['view']=='corridor_horror':continue
 r=next(q for q in repeat['views'] if q['view']==a['view']);assert r['samples']>100
 comparison.append(dict(view=a['view'],before_fps=b['fps'],after_fps=a['fps'],repeat_fps=r['fps'],fps_change_percent=(a['fps']/b['fps']-1)*100,before_p95_ms=b['p95_ms'],after_p95_ms=a['p95_ms'],before_game_ms=b['game_ms'],after_game_ms=a['game_ms'],repeat_game_ms=r['game_ms'],before_process_mb=b['process_physical_mb'],after_process_mb=a['process_physical_mb']))
assert len(comparison)==4
assert all(max(c['after_fps'],c['repeat_fps'])>=c['before_fps']*.95 for c in comparison),'Repeated performance regression: reject candidate before publication'
scope='Four reviewed full-map 1920x1080 views, 100% rendering, uncapped, six-second warm-up and fourteen-second sample; idle AI. Recent control baseline followed by two final-build runs; initial baseline and rejected candidate also retained. Subject to run-to-run variation; not a full gameplay or combat stress test. Fifth wall-facing diagnostic camera excluded.'
for label in ['baseline','baseline_recent','optimized','repeat']:
 source=P/f'Saved/PackageTests/PerformanceV25_{label}/Saved/Prototype'
 for suffix in ['json','csv']:shutil.copy2(source/f'hospital_perf_{label}.{suffix}',O/f'hospital_perf_{label}.{suffix}')
 for i in range(5):shutil.copy2(source/f'perf_{label}_{i}.png',O/f'perf_{label}_{i}.png')
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),18)
sheet=Image.new('RGB',(1600,len(comparison)*475),'#10191b');d=ImageDraw.Draw(sheet)
for i,c in enumerate(comparison):
 for j,label in enumerate(['baseline_recent','optimized']):
  with Image.open(O/f'perf_{label}_{i}.png') as im:
   assert im.size==(1920,1080);pic=im.convert('RGB').resize((800,450))
  sheet.paste(pic,(j*800,i*475));d.text((j*800+12,i*475+451),f'{label} / {c["view"]}',font=font,fill='white')
sheet.save(O/'before_after.png')
captures=O/'captures';captures.mkdir(exist_ok=True)
for n in v['screenshots']:shutil.copy2(Path(v['test_saved'])/'Prototype/BookWorldCaptures'/n,captures/n)
menus=O/'menu';menus.mkdir(exist_ok=True)
for f in (Path(m['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):shutil.copy2(f,menus/f.name)
for folder,prefix in [(captures,'world'),(menus,'menu')]:
 files=sorted(folder.glob('*.png'))
 for start in range(0,len(files),20):
  contact=Image.new('RGB',(1280,1000),'#10191b');draw=ImageDraw.Draw(contact)
  for j,f in enumerate(files[start:start+20]):
   with Image.open(f) as im:pic=im.convert('RGB').resize((320,180))
   x=(j%4)*320;y=(j//4)*200;contact.paste(pic,(x,y));draw.text((x+3,y+180),f.stem[:35],font=font,fill='white')
  contact.save(O/f'{prefix}_contact_{start//20+1}.jpg')
audit=read(R/'artifacts/echo_real_v23/acceptance.json');assert audit['success'] and len(audit['checks'])==16
shutil.copy2(R/'artifacts/echo_real_v23/acceptance.json',O/'art_audit.json')
report=dict(old,success=True,optimization_revision=25,checked_at=datetime.datetime.now().astimezone().isoformat(),executable_sha256=exe,build_identity=status['verified_world.json']['build_identity'],before_game_identical_after_move=before['success'],unchanged_data_hashes=before['data_hashes'],performance=comparison,performance_scope=scope,gpu=opt['gpu'],world_checks=w['count'],visual_checks=visual['count'],story_checks=s['checks'],menu_checks=m['checks'],bounds_failures=0,unresolved_custom_props=0)
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
for info,target,source in [(status['verified_world.json'],'native_world_check','book_world_check'),(v,'rendered_world_check','book_world_check'),(s,'native_story_check','story_check'),(m,'native_menu_check','menu_check'),(status['verified_world.json'],'every_door_check','every_door_check'),(status['verified_world.json'],'native_envelope_check','envelope_check')]:
 shutil.copy2(Path(info['test_saved'])/f'Prototype/{source}.json',O/f'{target}.json')
shutil.copy2(Path(s['test_saved'])/'Prototype/story_v7.png',O/'story_mob.png')
lib=R/'assets/models/echo_custom';integration=read(lib/'GAME_INTEGRATION.json');integration.update(optimization_revision=25,verification='artifacts/echo_optimization_v25/verification.json');(lib/'GAME_INTEGRATION.json').write_text(json.dumps(integration,ensure_ascii=False,indent=2),encoding='utf8');manifest=read(lib/'LIBRARY_MANIFEST.json');manifest['game_integration']=integration;(lib/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
lines=['# ЭХО — проверка переноса и оптимизация v25','','Файлы игры после переноса совпадали с v23 по SHA256. Каноническая карта, все 562 зоны / 35 локаций, сюжетные данные, модели и расстановка побайтово сохранены. При оптимизации разрешение и число предметов не снижались.','','Изменения: табло лифта обновляется при изменении текста вместо пересоздания графического объекта каждый кадр; выбор ближайших ламп ограничен 4,17 раза/сек в обоих режимах; заведомо недоступные по расстоянию источники исключаются до сортировки. Анимация света и тумана продолжает обновляться каждый кадр. Объединение групп одинаковых моделей было испытано и отменено: надёжное преимущество не подтвердилось. Ранние замеры лаборатории были ниже первого исходного запуска; свежий контроль показал такой же разброс и у исходного кода. Исходная группировка сохранена. Инициализировано запасное смещение окон для отсутствующей записи корпуса.','','Измерение: 1920×1080, 100%, без ограничения FPS, четыре проверенных неподвижных вида, 6 сек прогрева / 14 сек выборки, ноутбук подключён к питанию. Сюжетный ИИ остановлен только в замере. В основной таблице — контрольный запуск исходного кода непосредственно перед двумя запусками итогового. Первый исходный замер также сохранён отдельно; на результат влияет разброс нагрузки ноутбука. Это короткий сравнительный тест, а не гарантия FPS всей игры. Пятый диагностический ракурс оказался направлен в стену: сырые данные сохранены, из сравнения он исключён.','','| Участок | До, FPS | После, FPS | До, p95 мс | После, p95 мс |','|---|---:|---:|---:|---:|']
for c in comparison:lines.append(f'| {LABELS[c["view"]]} | {c["before_fps"]:.1f} | {c["after_fps"]:.1f} | {c["before_p95_ms"]:.1f} | {c["after_p95_ms"]:.1f} |')
lines+=['','Самый первый исходный замер (FPS): '+', '.join(f'{c["view"]}: {c["fps"]:.1f}' for c in initial['views'] if c['view']!='corridor_horror')+'. Контроль повторён после прогрева ноутбука для более близких условий.']
lines+=['','Повторный запуск той же итоговой сборки (FPS): '+', '.join(f'{c["view"]}: {c["repeat_fps"]:.1f}' for c in comparison)+'. Все замеры сохранены; в основной таблице — свежий контроль и первый итоговый запуск. Отклонённый эксперимент и ошибочный контроль с пропущенной компиляцией вынесены отдельно и не используются для вывода. Стабильный общий прирост FPS не подтверждён. Убраны лишние обновления текста и пересчёты освещения без снижения настроек качества; для существенного роста FPS могут потребоваться более глубокие изменения рендера.']
lines+=['','| Участок | Игровой поток до, мс | После, мс | Повтор, мс |','|---|---:|---:|---:|']
for c in comparison:lines.append(f'| {LABELS[c["view"]]} | {c["before_game_ms"]:.2f} | {c["after_game_ms"]:.2f} | {c["repeat_game_ms"]:.2f} |')
lines+=['','Проверки итоговой сборки: мир 128/128, отрисовка 54/54, бой/сохранения 25/25, меню 57/57, двери 1012/1012. Предметов слоя v23 по-прежнему 16 107, слоя v22 — 28 850. Не обнаружены отсутствующие модели или выход за защитные габариты. Свежие 105 кадров мира и 17 меню сохранены. EXE SHA256: '+exe+'.','','Запуск: PLAY_UNREAL.cmd из D:\\Projects\\ECHO. Настройки и сохранения пользователя не перезаписывались; замеры запускались с отдельными профилями.']
(O/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf8')
marker=PK/'package_verified.tmp';marker.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');marker.replace(PK/'package_verified.json')
print(json.dumps(dict(success=True,executable_sha256=exe,performance=comparison),ensure_ascii=False))
