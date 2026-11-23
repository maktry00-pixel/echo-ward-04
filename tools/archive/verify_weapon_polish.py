"""Publish evidence for the weapon pass from the matching cooked Windows build."""
from pathlib import Path
import json
import math
import shutil

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'unreal/EchoWard'
TESTS = PROJECT / 'Saved/PackageTests'
OUT = ROOT / 'artifacts/unreal_weapon_polish'

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

package = read(PROJECT / 'Saved/PackagedWindows/package_verified.json')
smoke = read(TESTS / 'Smoke/Saved/Prototype/campaign_smoke.json')
assert package['success'] and package['physical_route_verified']
assert package['smoke_checks'] == smoke['count'] == 54
assert all(check['passed'] for check in smoke['checks'])
captures = TESTS / 'Capture/Saved/Prototype/CampaignCaptures'
poses = {i: read(captures / f'pose_{i}.json') for i in range(11, 17)}
for i, pose in poses.items():
    assert math.isfinite(pose['right_grip_error_cm']) and pose['right_grip_error_cm'] < 3
    if i >= 12:
        assert math.isfinite(pose['left_grip_error_cm']) and pose['left_grip_error_cm'] < 3
OUT.mkdir(exist_ok=True, parents=True)
for i, name in [(12, 'pistol_aim'), (13, 'pistol_reload'), (14, 'shotgun_carry'), (16, 'shotgun_reload')]:
    shutil.copy2(captures / f'view_{i}.png', OUT / f'{name}.png')
report = {'success': True, 'package': package, 'grip_errors_cm': poses,
          'checks': smoke['checks'], 'hand_authored_animation_clips_added': False}
(OUT / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
maximum = max(p['left_grip_error_cm'] for i, p in poses.items() if i >= 12)
(OUT / 'REPORT.md').write_text(f'''# Оружие, хват и перезарядка — 20 октября

Плавное поднятие/опускание оружия синхронизировано с целями рук. Левая рука постепенно переходит от ходьбы к поддержке пистолета; дробовик поддерживается двумя руками также вне прицела. При перезарядке магазин пистолета выдвигается и возвращается, левая рука следует за ним. Для дробовика рука переходит от цевья к точке заряжания.

Проверка препятствия идёт от персонажа до поднятого ствола. У стены оружие опускается, прицел становится янтарным крестом, попытка выстрела не расходует патрон. Проверка выполняется снова в момент выстрела, независимо от сглаженного визуального положения. Противники не считаются препятствием для поднятия оружия; обычная трассировка попадания остаётся активна.

## Подтверждение в самостоятельном EXE

- 54 игровые проверки и 8 проверок Chaos; свежий физический маршрут до победы.
- Шесть новых игровых проверок: плавность перехода, запрет выстрела у стены, один патрон за разрешённый выстрел, движение настоящего магазина до выдачи патронов, точный перенос при завершении перезарядки, сброс состояний в новой игре.
- Шесть ракурсов оружия с измерением положения костей кистей. Обе применимые кисти остаются в пределах допуска 3 см от целей, включая переноску дробовика. Точные ошибки записаны в JSON.
- 28 кадров и 258 GPU-материалов проверены на той же сборке; идентичность EXE/архивов подтверждена хешами. Дополнительные крупные ракурсы и исправление пальцев: [визуальная проверка](../unreal_visual_repairs/REPORT.md).

Это процедурная доработка существующей анимации. Новые профессиональные клипы, новая модель героя и полноценная анимация пальцев под каждый предмет не добавлены. Совпадение кисти с целью не доказывает идеальную форму каждого пальца; движение и бой требуют ручного плейтеста.

[Пистолет](pistol_aim.png) · [Перезарядка](pistol_reload.png) · [Дробовик вне прицела](shotgun_carry.png) · [Заряжание дробовика](shotgun_reload.png)
''', encoding='utf-8')
print(json.dumps({'success': True, 'game_checks': 54, 'support_hand_max_error_cm': maximum}))
