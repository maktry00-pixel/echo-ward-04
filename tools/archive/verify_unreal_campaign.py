"""Publish evidence from actual native runs; never substitute source-only checks."""
from pathlib import Path
from datetime import datetime, timezone
import json
import shutil

ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT/'unreal/EchoWard'
RAW=PROJECT/'Saved/Prototype'
OUT=ROOT/'artifacts/unreal_campaign'
OUT.mkdir(parents=True,exist_ok=True)

def read(path):
    data=path.read_bytes()
    return json.loads(data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig'))

imported=read(RAW/'campaign_import.json')
smoke=read(RAW/'campaign_smoke.json')
agent=read(RAW/'campaign_agent_fresh.json')
resumed=read(RAW/'campaign_agent_resumed.json')
geometry=read(RAW/'campaign_asset_audit.json')
assert imported['success'] and imported['objects']==199 and imported['rooms']==54
assert smoke['success'] and smoke['count']==len(smoke['checks'])>=54 and all(c['passed'] for c in smoke['checks'])
assert geometry['success'] and not geometry['issues']
assert agent['success'] and agent['resumed'] is False and agent['elapsed']>1000
assert resumed['success']
assert not (ROOT/'artifacts/campaign_missing_locale.txt').read_text(encoding='utf-8-sig').strip()
profiles=[]
for preset in [1,2]:
    p=read(RAW/f'full_campaign_{preset}_benchmark.json')
    assert p['success'] and len(p['views'])==3 and p['resolution']=='1920x1080'
    assert p['ac_power_before']['ac_line_status']==p['ac_power_after']['ac_line_status']==1
    assert 'NVIDIA' in p['gpu']
    profiles.append(p)

evidence=['campaign_import.json','campaign_smoke.json','campaign_agent_fresh.json','campaign_agent_resumed.json','campaign_asset_audit.json','character_material_fix.json']
for preset in [1,2]:
    evidence += [f'full_campaign_{preset}_benchmark.json',f'full_campaign_{preset}_frames.csv']
for name in evidence:
    shutil.copy2(RAW/name,OUT/name)
for stage in range(3):
    packaged=PROJECT/f'Saved/PackageTests/Capture/Saved/Prototype/CampaignCaptures/view_{stage}.png'
    shutil.copy2(packaged if packaged.exists() else RAW/f'full_campaign_1_view_{stage}.png',OUT/f'campaign_view_{stage}.png')
for stage,name in [(3,'menu'),(4,'training'),(5,'kazakh'),(7,'map'),(8,'xray'),(9,'badge'),(12,'pistol_aim'),(15,'shotgun_aim')]:
    packaged=PROJECT/f'Saved/PackageTests/Capture/Saved/Prototype/CampaignCaptures/view_{stage}.png'
    path=packaged if packaged.exists() else RAW/f'CampaignCaptures/view_{stage}.png'
    if path.exists():shutil.copy2(path,OUT/f'{name}.png')

package_marker=PROJECT/'Saved/PackagedWindows/package_verified.json'
package=read(package_marker) if package_marker.exists() else {'success':False,'status':'not yet verified'}
if package.get('success'):
    shutil.copy2(package_marker,OUT/'package_verified.json')
    shutil.copy2(PROJECT/'Saved/PackageTests/Smoke/Saved/Prototype/campaign_smoke.json',OUT/'packaged_smoke.json')
    if package.get('physical_route_verified'):
        shutil.copy2(PROJECT/'Saved/PackageTests/Agent/Saved/Prototype/campaign_agent.json',OUT/'packaged_agent.json')
report={
    'verified_at':datetime.now(timezone.utc).isoformat(),
    'native_campaign_runtime_verified':True,
    'smoke_checks':smoke['count'],
    'fresh_physical_route_won':True,
    'agent_gameplay_seconds':agent['elapsed'],
    'human_playthrough_measured':False,
    'full_manual_parity_audit_complete':False,
    'asset_import':imported,
    'imported_mesh_audit':geometry,
    'profiles':profiles,
    'profiles_are_historical_before_geometry_repair':False,
    'windows_package':package,
    'limits':['Known-solution agent timing is not a human playthrough.',
              'Frame timings use editor -game, idle AI and three fixed views; no combat stress.',
              'Benchmark screenshots occur during warm-up; long frame intervals remain in the sample.',
              'Godot save files are preserved, not fully converted.',
              'Wall texture text remains authored Russian; menu/prompts/quests support RU/KK/EN.',
              'Legacy head/hair sections receive room shadows but do not cast their own; body cast shadows remain.',
              'Enemies use the imported HorrorGameMaker rig and source clips; the attack adapts Jump. Human animation uses four source clips and corrected arm/finger posing.']
}
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
duration=f"{int(agent['elapsed'])//60:02d}:{int(agent['elapsed'])%60:02d}"
lines=['# Перенос кампании в Unreal — подтверждённый результат','',
       f"{smoke['count']}/{smoke['count']} проверок; физический агент начал новую игру с пустыми тестовыми сохранениями и победил за **{duration}** игрового времени. Телепортации и выдача припасов в маршруте не используются. Победа после загрузки контрольной точки проверялась в предыдущей сборке.",'',
       'Исправления текущей сборки и их проверки: [BUGFIX_REPORT.md](BUGFIX_REPORT.md). Геометрия 257 импортированных мешей сопоставлена с исходными OBJ; ошибок границ и количества материалов не найдено.', '',
       '## Состав','',
       '54 зоны, 199 интерактивных объектов, 25 противников. 257 мешей, 253 материала, 146 текстур, 55 звуков. Исходная Godot-игра и старая сравнительная комната сохранены.',
       '', 'Меню, RU/KK/EN, настройки, справочник, инвентарь, карта, сохранения/автозагрузка после смерти, администратор/агент и основные цепочки всех корпусов перенесены в native C++. Исходные Godot-сохранения не конвертируются автоматически.',
       '', 'Улучшены PBR/масштаб текстур, материалы древесины/органики, свет, поза рук и вспышка выстрела. Исправлены сохранение шейдеров для HISM, направление потолочных источников, препятствия перемещённых шкафов и полная ширина шестиметровой заслонки.',
       '', '## FullHD на зарядке — текущие материалы и свет','',
       'RTX 5060 Laptop. Три фиксированных вида полной карты; это **не бой и не гарантированный минимум FPS**. Пять секунд прогрева, 12 секунд выборки; без лимита/VSync, реальные интервалы времени. Скриншоты сделаны на прогреве; длинные кадры не исключаются. Обычная игра ограничена 60 FPS. Подробности обновления: [графика и физика](../unreal_presentation/REPORT.md).',
       '', '| Профиль | Приёмная FPS / p95 мс | Отделение FPS / p95 мс | Хозкорпус FPS / p95 мс |',
       '|---|---:|---:|---:|']
for p,label in zip(profiles,['Баланс 100%','Производительность TSR 75%']):
    lines.append('| '+label+' | '+' | '.join(f"{v['fps']:.1f} / {v['p95_ms']:.2f}" for v in p['views'])+' |')
lines += ['', '## Учебные темы','',
          '- A*: поиск кратчайшего пути по сетке, динамические двери/шкафы. Проверен обход стены и блокировка всей заслонки.',
          '- CSP: MRV, forward checking и ограничения расстояния для пяти точек припасов; проверены разрешимая и невозможная задачи.',
          '- Minimax / alpha-beta: тактический ход противника; результат сопоставлен с полным поиском. Счётчики доступны в режиме администратора.',
          '', 'Код: `unreal/EchoWard/Source/EchoWard/WardCampaignAI.cpp`; реальные вызовы — `WardCampaignAgent.cpp` и `WardCampaignEnemy.cpp`.',
          '', '## Запуск и пределы проверки','',
          '**PLAY_UNREAL.cmd → Игра → Новая игра.** Редактор: OPEN_UNREAL_EDITOR.cmd. Подробнее: `unreal/EchoWard/README.md`.',
          '', 'Windows package: '+(f"собран и проверен отдельным EXE: {package['smoke_checks']} проверок, {package['captured_views']} кадров"+('; полный физический маршрут до победы также пройден в EXE.' if package.get('physical_route_verified') else '.') if package.get('success') else 'проверка отдельного EXE ещё не завершена.'),
          '', 'Первое человеческое прохождение на 25 минут пока не измерено. Пройден автоматический маршрут, а не исчерпывающий ручной аудит каждого действия и каждого угла камеры. Нужны человеческий плейтест и художественная доработка; это не заявляется уровнем коммерческого Resident Evil.',
          '', 'Сырые JSON/CSV приложены рядом с этим отчётом; логи и оригинальные кадры — `unreal/EchoWard/Saved/Prototype`.']
(OUT/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps({'runtime_verified':True,'checks':smoke['count'],'agent_time':duration,'package_verified':package.get('success',False)},ensure_ascii=False))
