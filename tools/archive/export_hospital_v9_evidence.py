"""Verify identical staged/source build and export native envelope + all views."""
from pathlib import Path
import hashlib,json,shutil
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard'
OUT=ROOT/'artifacts/unreal_hospital_v9';PACKAGE=PROJECT/'Saved/PackagedWindows'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';data=read(source);map_hash=sha(source)
exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';exe_hash=sha(exe)
assert data['review_revision']==9 and sha(PROJECT/'Binaries/Win64/EchoWard.exe')==exe_hash
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v9.json');story=read(PACKAGE/'verified_story_v7.json')
assert all(s['success'] for s in (native,visual,story))
assert native['build_identity'].split(':')[0].lower()==exe_hash and native['build_identity'].split(':')[-1].lower()==map_hash
for s in (visual,story):assert s['executable_sha256'].lower()==exe_hash and s['map_sha256'].lower()==map_hash
world_result=read(Path(native['test_saved'])/'Prototype/book_world_check.json');visual_result=read(Path(visual['test_saved'])/'Prototype/book_world_check.json')
assert world_result['success'] and world_result['count']==89 and visual_result['success'] and visual_result['count']==23
assert read(Path(story['test_saved'])/'Prototype/story_check.json')['success'] and story['checks']>=23
envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert envelope['success'] and not any(envelope['failed'].values())
for name,key in (('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('environment_acceptance','map_sha256')):
    result=read(OUT/f'{name}.json');assert result['success'] and result[key]==map_hash
assert read(OUT/'environment_acceptance.json')['checks']['all_562_zone_coordinates_areas_and_37_stairs_unchanged']
for target in ('game','editor'):assert 'Result: Succeeded' in (OUT/f'build_{target}.log').read_text(encoding='utf-8',errors='replace')
captures=data['locations']+data['furnishing_captures'];images=[]
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
for index,c in enumerate(captures):
    path=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    with Image.open(path) as im:assert im.size==(1920,1080)
    shutil.copy2(path,OUT/path.name);images.append(path.name)
for start in range(0,len(images),20):
    canvas=Image.new('RGB',(1640,1270),'#10191b');draw=ImageDraw.Draw(canvas)
    for i,name in enumerate(images[start:start+20]):
        im=Image.open(OUT/name).convert('RGB');im.thumbnail((400,225));x=i%4*410;y=i//4*254
        canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name[6:-4],font=font,fill='white')
    canvas.save(OUT/f'contact_{start//20+1}.png')
for status,name,folder in ((native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(visual,'rendered_world_check','book_world_check'),(story,'native_story_check','story_check')):
    shutil.copy2(Path(status['test_saved'])/f'Prototype/{folder}.json',OUT/f'{name}.json')
shutil.copy2(source,OUT/'hospital_world_source.json')
report=dict(success=True,review_revision=9,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=exe_hash,map_sha256=map_hash,
    world_id=data['map_id'],world_rooms=562,world_locations=35,world_checks=89,story_checks=story['checks'],visual_initial_checks=23,
    envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],environment_inventory=read(OUT/'environment_inventory.json'),world_captures=len(images),exterior_views=27,furnished_views=18,
    physical_stair_and_gallery_walk_verified=True,physical_tunnel_walk_verified=True,
    visual_scope=visual['scope'],screenshots=images,
    limits=['Native envelope uses one-metre floor/ceiling centres and three horizontal samples per metre of exterior wall, not a proof at every pixel.',
            'Some inspection cameras show close-ups of walls or equipment; the 80 captures do not replace traversal of every room.',
            'Furniture and monsters retain prototype models; the 210-hour story remains a spatial gameplay prototype.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(PACKAGE/'package_verified.json').write_text(json.dumps(dict(report,executable=str(exe),executable_timestamp=native['executable_timestamp']),ensure_ascii=False,indent=2),encoding='utf-8')
inventory=report['environment_inventory']
description=f'''# ЭХО — наполнение окружения v9

Обновлена игра и проект Unreal Engine. Сохранены все 562 зоны, 35 локаций, размеры помещений и 37 лестничных узлов.

## Добавлено

- Ещё {inventory['added_floor_props']} предметов мебели и оборудования: койки, тумбы, столы, стулья, шкафы, стеллажи, тележки, санитарное и хозяйственное оборудование. Предметы подбираются по назначению помещения.
- Все прежние 10 978 предметов из пронумерованного плана оставлены на своих местах.
- Потолочные светильники во всех 1 939 внутренних секциях. В 1 925 секциях есть напольная мебель; в оставшихся 14 сохранён проход и установлено освещение. Во всех 562 именованных зонах и 17 технических подвалах остаётся обстановка исходного плана.
- 541 комплект настенных деталей: камера с объективом и индикатором, радиатор, выключатель, красный аварийный маяк, огнетушитель и указатель выхода. Камеры и новые настенные выключатели пока декоративные; существующие интерактивные системы сохранены.
- {inventory['lawn_area_m2']:,} м² газонов, простая трава, кусты и деревья. Газоны исключают здания, существующие служебные площадки, дороги и лестницы.
- Асфальт, новая дорожная разметка и тротуары; 131 дополнительный уличный фонарь.
- Видимая луна внутри неба, пульсация и краткие пропадания красного освещения, настенные и напольные пятна. Сохранена окраска стен по отделениям и потёртости материалов.

## Проверено на этой сборке

- 89/89 проверок реальной игровой сцены: оболочки, проходы, все лестницы, лифт, тоннели, двери, укрытия, сохранения и новые элементы окружения.
- 23/23 проверок сюжетного прототипа: оружие, пустые записки, мобы и сохранение состояния.
- Независимые проверки данных: карта 15/15, прежняя мебель 9/9, сюжет 11/11, новое окружение 12/12.
- 899 569 выборочных проверок коллизии оболочек — без найденных пропусков.
- 23 начальные проверки с отрисовкой и 80 свежих снимков 1920×1080. Некоторые старые обзорные камеры по-прежнему показывают близкие стены или оборудование; снимки не заменяют личный обход каждой комнаты.
- Собраны Game и Editor; исходный и запускаемый EXE и JSON совпадают.

## Запуск

`PLAY_UNREAL.cmd` в корне проекта. **B** переключает яркий режим и более тёмное освещение. **M / Tab** открывает полный план.

Модели мебели, растительности и монстров остаются простыми прототипами. Сюжет по-прежнему содержит заготовки сцен и пустые записки; финальная постановка и художественные модели ещё не выполнены.

EXE SHA256: `{exe_hash}`

Карта SHA256: `{map_hash}`
'''
(OUT/'REPORT.md').write_text(description,encoding='utf-8')
handoff=ROOT/'HANDOFF_NEXT_GPT.md'
prefix=f'''# Завершено: окружение и наполнение v9 — 23 октября 2026

Последний запрос: повторно проверить и добавить всю мебель, траву, асфальт, луну, освещение, камеры, окраску стен и красный свет; качество допустимо прототипное. Работа выполнена в реальном Windows EXE и UE Editor. Ничего не запрашивает разрешения и не ожидает продолжения.

Новый tools/hospital_environment_v9.py добавляет отдельный слой environment_props (2696 напольных предметов) и environment_dressing (29326 простых частей, 541 комплект настенных деталей, 1939 потолочных светильников, 131 уличный фонарь, 81136м² газонов, 13230 частей травы, 79 кустов и 26 деревьев). Все 10978 старых prototype_props, 562 комнаты/35locations/37stairs и сюжетные позиции сохранены. 1925 из1939 внутренних секций имеют напольную мебель;14 оставлены свободными ради прохода, но освещены. Все именованные зоны уже сохраняют исходную обстановку PDF. Камеры, новые выключатели, радиаторы, огнетушители — статические прототипы, не новый симулятор видеонаблюдения/электрики.

Генератор резервирует старые реальные маршруты, дверные подходы и центры комнат, лестницы/лифт, все укрытия и ниши,395 сюжетных предметов/46 мобов. Новая напольная мебель имеет collision_features с теми же границами, что рендер. Газоны не пересекают protected_surface_rects и roads. Нельзя повторно бездумно запускать apply_furnishingv6: это исторический базовый слой. Полный build_hospital_layout.py теперь заканчивается v9 и обновлением story_gameplay.navigation.rows по новому mask, без сдвига сюжетных объектов. Отдельный refresh_hospital_environment_views.py меняет только10 дополнительных ракурсов.

WardHospitalEnvironment.cpp строит окружение, четыре переиспользуемых локальных красных point lights, visibleMoon (уже cooked SM_HospitalSky, position100000,-120000,110000cm scale46, собственный emissive MID без коллизии). Новые белые плафоны используют Palette4 для устойчивого свечения при SetBright. WardHospitalDetails::Update пульсирует EmergencyMaterial и четыре красных источника, UpdateLighting выбирает ближайшие красные маяки на текущем этаже. WardHospitalFurnishing рисует environment_props вместе с каталогом, сохраняя счётчик PrototypeProps10978; EnvironmentProps2696 отдельно.

Финальные SHA EXE `{exe_hash}`, JSON `{map_hash}`. Game+Editor Succeeded. NativeWorld89/89 (83 старых+6 окружения), Story23/23, renderedPreview23/23+80 свежих1920×1080 (35 игровых+13 старых furnishing+22 старых envelope+5 новых внешних+5 новых интерьерных). Независимые аудиты15/9/11/12 проходят и привязаны к finalJSON SHA; source/stagedEXE+JSON идентичны. AuditEnvelope899569 проб,0пропусков. Все4контактныхлиста надо просмотреть перед окончательной передачей пользователю; старые кадры всё ещё местами показывают стены, новые5интерьеров имеют явный target и внутрикомнатную экспозицию4.8. Технические ограничения сюжета v7 ниже сохраняются.

Доказательства: artifacts/unreal_hospital_v9/REPORT.md, verification.json, environment_inventory.json, environment_acceptance.json,80PNG/4contacts. tools/export_hospital_v9_evidence.py проверяет одинаковый EXE/JSON и все native/rendered/story hashes, обновляет package_verified.json. Запуск PLAY_UNREAL.cmd. Проверки Tools/verify_windows_package.ps1 -Mode World; verify_hospital_story_v7.ps1 (версия набора, проверяет текущий EXE); verify_hospital_visual_v9.ps1. Старые exporters v8/v7 не запускать наv9. Сборки require_escalated для UBA, использован tmp/uba_v9. Для PowerShell скриптов проверок не цеплять второй через $LASTEXITCODE: скрипты используют Start-Process и могут не выставить эту переменную; запускать отдельно.

---

'''
if not handoff.read_text(encoding='utf-8-sig').startswith('# Завершено: окружение и наполнение v9'):
    handoff.write_text(prefix+handoff.read_text(encoding='utf-8-sig'),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
