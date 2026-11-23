"""Publish captures and evidence from the matching tested Windows build."""
from pathlib import Path
import hashlib
import json
import shutil
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'unreal/EchoWard'
OUT = ROOT / 'artifacts/unreal_settings_menu'
def read(p):
    return json.loads(p.read_text('utf-8-sig'))
package = read(PROJECT / 'Saved/PackagedWindows/package_verified.json')
status = read(PROJECT / 'Saved/PackagedWindows/verified_settings.json')
assert package['success'] and status['success'] and package['build_identity'] == status['build_identity']
saved = Path(status['test_saved'])
checks = read(saved / 'Prototype/settings_menu_check.json')
assert checks['success'] and checks['count'] >= 24 and all(c['passed'] for c in checks['checks'])
assert package['settings_menu_checks'] == checks['count']
OUT.mkdir(parents=True, exist_ok=True)
for name in ['settings_graphics_1080', 'settings_audio_1080', 'settings_controls_1080',
             'settings_game_1080', 'settings_hover_1080', 'settings_room_1080',
             'settings_kk_1080', 'settings_en_1080']:
    p = saved / 'Prototype/SettingsCaptures' / (name + '.png')
    with Image.open(p) as image:
        assert image.size == (1920, 1080), (name, image.size)
    shutil.copy2(p, OUT / p.name)

source = PROJECT / 'RawAssets/UnifiedGuide'
capture = read(source / 'capture.json')
assert capture['success'] and capture['count'] == 9
photos = []
for view in capture['views']:
    p = source / (view['id'] + '.png')
    with Image.open(p) as image:
        assert image.size == (1920, 1080), (p, image.size)
    assert view['camera_error_cm'] < 2 and view['fov'] == 72
    asset = PROJECT / 'Content/Campaign/Guide' / ('T_Guide_' + view['id'] + '.uasset')
    assert asset.exists()
    photos.append(dict(id=view['id'], sha256=hashlib.sha256(p.read_bytes()).hexdigest(), asset=str(asset)))
assert len({p['sha256'] for p in photos}) == 9
result = dict(success=True, native_live_3d=True, resolution='1920x1080', checks=checks,
              windows_package=package, uniform_guide_photos=photos, capture=capture,
              human_playtest_complete=False)
(OUT / 'verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), 'utf8')
(OUT / 'REPORT.md').write_text(f'''# ЭХО — настройки и единая серия фотографий

Компактный экран по образцу: четыре вкладки «Графика / Звук / Управление / Игра»,
параметр слева, выбор со стрелками или настоящий ползунок справа. Жёлтая игровая
подсказка убрана. Кнопки краснеют только под мышью; текущая вкладка отмечена нейтральной линией.
Сохраняются настройки экрана, света, тумана, звука, мыши, HUD, языков и администратор0890.
Добавлены разрешение, профиль качества, масштаб рендера, motion blur и сила зерна.
Применяются настоящие параметры Unreal, включая TSR; наличие DLSS/FSR не имитируется.
FOV55–100, изменения ползунков видны сразу, запись выполняется после отпускания/выхода.

Фон без персонажей — другой ракурс поперечного прохода из больничного набора,
с отдельной мебелью/светом/кровью. Камера движется, красный свет пульсирует,
у пола есть туман, дверь в соседнем проходе покачивается. Обучение сохраняет свой ракурс.

Девять старых фотографий заменены настоящими кадрами больничных декораций Unreal,
собранных из мебели и материалов кампании: коридоры, палаты, пост, шкафы и оборудование.
У всех1920×1080, FOV72 и одинаковый цвет/экспозиция. HUD, персонажи и подсказки скрыты.
Одни и те же текстуры используются в «Обучении», «Об игре» и справочнике управления.
Исходные PNG в RawAssets/UnifiedGuide, подтверждённые ракурсы — capture.json.
Пиксели фотографий не дорисованы и не заменены референсом.

В Windows EXE прошли **{checks['count']} проверок настроек**: стрелки разрешения,
профиль TSR75%, смена реального AA, значения/границы ползунков, grain/motion blur,
запись/сброс, четыре раздела, управление/администратор, KK/EN, hover/focus/leave,
живая камера/свет, отсутствие монстров, возврат из title и pause.
Контрольные суммы EXE/контента и фотографий — в verification.json.
Полное человеческое прохождение не заявляется.

![Настройки](settings_graphics_1080.png)

![Пустой коридор](settings_room_1080.png)

Запуск: PLAY_UNREAL.cmd → «Настройки». Повтор: Tools/verify_windows_package.ps1 -Mode Settings.
''', 'utf8')
print(json.dumps(dict(success=True, checks=checks['count'], photos=len(photos), report=str(OUT / 'REPORT.md')), ensure_ascii=False))
