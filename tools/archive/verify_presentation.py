"""Publish measured presentation/physics evidence from the final Windows build."""
from pathlib import Path
import json,shutil
from datetime import datetime,timezone
R=Path(__file__).resolve().parents[1]
P=R/'unreal/EchoWard'
RAW=P/'Saved/Prototype'
TEST=P/'Saved/PackageTests'
OUT=R/'artifacts/unreal_presentation'
OUT.mkdir(exist_ok=True,parents=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
upgrade=read(RAW/'presentation_upgrade.json')
package=read(P/'Saved/PackagedWindows/package_verified.json')
physics=read(TEST/'Physics/Saved/Prototype/physics_assessment.json')
shader=read(TEST/'Capture/Saved/Prototype/shader_validation.json')
smoke=read(TEST/'Smoke/Saved/Prototype/campaign_smoke.json')
agent=read(TEST/'Agent/Saved/Prototype/campaign_agent.json')
assert upgrade['success'] and package['success'] and package['physical_route_verified'] and package['physics_checks']==8
assert physics['success'] and len(physics['checks'])==8 and all(c['passed'] for c in physics['checks'])
assert shader['success'] and shader['materials_checked']==258 and not shader['failed']
assert smoke['success'] and smoke['count']==54 and agent['success'] and not agent['resumed']
profiles=[read(RAW/f'full_campaign_{i}_benchmark.json') for i in [1,2]]
for v in profiles:
    assert v['success'] and v['resolution']=='1920x1080' and v['ac_power_before']['ac_line_status']==v['ac_power_after']['ac_line_status']==1
    assert len(v['views'])==3 and datetime.fromisoformat(v['started_at']).timestamp()>(RAW/'presentation_upgrade.json').stat().st_mtime
before=R/'artifacts/unreal_campaign/campaign_view_0.png'
if before.exists() and not (OUT/'before.png').exists():shutil.copy2(before,OUT/'before.png')
for i,name in [(0,'after'),(1,'ward'),(12,'hero'),(15,'shotgun'),(17,'shot')]:
    shutil.copy2(TEST/f'Capture/Saved/Prototype/CampaignCaptures/view_{i}.png',OUT/f'{name}.png')
for src,name in [(RAW/'presentation_upgrade.json','surface_upgrade.json'),(TEST/'Physics/Saved/Prototype/physics_assessment.json','physics.json'),(TEST/'Capture/Saved/Prototype/shader_validation.json','gpu_materials.json'),(P/'Saved/PackagedWindows/package_verified.json','package.json'),(TEST/'Agent/Saved/Prototype/campaign_agent.json','physical_route.json')]:shutil.copy2(src,OUT/name)
for i in [1,2]:
    for suffix in ['benchmark.json','frames.csv']:shutil.copy2(RAW/f'full_campaign_{i}_{suffix}',OUT/f'profile_{i}_{suffix}')
result={'checked_at':datetime.now(timezone.utc).isoformat(),'surface_upgrade':upgrade,'game_checks':54,'physics_checks':8,'gpu_materials':shader,'windows_build':package,'profiles':profiles,'human_playthrough_timed':False,'commercial_resident_evil_parity':False}
(OUT/'verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
lines=['# Графика и физика — проверенная Windows-сборка','',
'172 экземпляра материалов обновлены с сохранением исходных текстур. Подключено AO упакованных карт мебели, переработаны нормали/шероховатость, добавлены отдельные влажные участки пола. Для 4K-терраццо настроена явная UV-развёртка. Пересобраны нормали общего куба, исправив освещение пола и стен. Для повторяемой геометрии помещения включён Nanite, чтобы она участвовала в Surface Cache Lumen; поддержка инстансированных мешей описана в [документации Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine). Свет потолочных источников стал мягче, уменьшен равномерный заполняющий свет, настроены цвет и объёмный туман. Убраны полосатые альфа-пряди: используется текстурированная основа причёски.', '',
'Бутылки массой 0,45 кг летят по траектории Chaos и разбиваются от контакта. Гильзы и осколки имеют массу, трение, отскок и CCD. Шаг физики — до 120 Гц, максимум восемь подшагов. Осколки перегородок сохраняют материал исходного объекта. У выстрела есть возвратная отдача оружия, гильза, вспышка, физические сколы и след попадания. Косметические тела не мешают персонажу и камере; одновременно допускается до 64 тел и 48 следов.', '',
'Дополнение 20 октября: плавный прицел, поддержка дробовика вне прицела, движение магазина и опорной руки, проверка препятствия до ствола. Подтверждение поз: [оружие и перезарядка](../unreal_weapon_polish/REPORT.md).', '',
'## Проверки','',
'- 54 проверки игровых механизмов, дверей, нового монстра и сохранений.',
'- 8 проверок настоящей симуляции: масса/импульс, контактное разрушение бутылки, остановка на полу, CCD у стены при 35 м/с, отсутствие блокировки персонажа/камеры, создание следа, ограничение числа тел и сброс.',
'- 258 ресурсов материалов имеют пригодные для рендера GPU-шейдеры; ошибок компиляции в итоговой проверке нет.',
f"- 28 кадров из самостоятельного EXE. Физический агент прошёл новую игру до победы за {int(agent['elapsed'])//60}:{int(agent['elapsed'])%60:02d}; агент знает решения.", '',
'## Full HD, питание от сети','',
'RTX 5060 Laptop. Три неподвижных вида карты, без боевой нагрузки и без лимита FPS. Пять секунд прогрева, затем 12 секунд измерения. Скриншот делается на прогреве; длинные кадры из выборки не удаляются. Это средние FPS по ракурсам, не гарантированный минимум во время боя.', '',
'| Профиль | Приёмная FPS / p95 мс | Отделение FPS / p95 мс | Хозкорпус FPS / p95 мс |','|---|---:|---:|---:|']
for v,label in zip(profiles,['Баланс, 100%','Производительность, TSR 75%']):lines.append('| '+label+' | '+' | '.join(f"{w['fps']:.1f} / {w['p95_ms']:.2f}" for w in v['views'])+' |')
lines+=['','## Пределы результата','',
'Это улучшение текущей игры, а не графический паритет с коммерческой Resident Evil. Монстры заменены скелетной моделью HorrorGameMaker с исходной ходьбой/бегом и физическим падением. Для выпада адаптирован Jump, это не новая профессиональная анимация удара. Герой всё ещё использует процедурную коррекцию рук. Физика мебели целиком не переведена в динамическую симуляцию. Косметические гильзы/сколы не сохраняются в игровых слотах; сюжетные сохранения совместимы с native v2. Тень секций старой модели головы остаётся отключённой; тело отбрасывает тень. Исправления по пользовательским кадрам: [визуальная проверка](../unreal_visual_repairs/REPORT.md).', '',
'Запуск из корня: **PLAY_UNREAL.cmd → Игра → Новая игра**. F4 — профиль графики, F3 — FPS. Проверенная папка для передачи: `unreal/EchoWard/Saved/PackagedWindows/Windows`.', '',
'[Текущий кадр](after.png) · [Герой](hero.png) · [Выстрел](shot.png). Предыдущий кадр сохранён как `before.png`; сравнение относится к текущему проекту, не к чужой игре.']
(OUT/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps({'success':True,'game_checks':54,'physics_checks':8,'gpu_materials':258,'captured_views':28},ensure_ascii=False))
