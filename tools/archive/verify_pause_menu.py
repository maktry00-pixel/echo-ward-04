"""Export the tested pause screen from the current Windows package."""
from pathlib import Path
import json
import shutil
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'unreal/EchoWard'
OUT = ROOT / 'artifacts/unreal_pause_menu'
def read(path):
    return json.loads(path.read_text('utf-8-sig'))
package = read(PROJECT / 'Saved/PackagedWindows/package_verified.json')
status = read(PROJECT / 'Saved/PackagedWindows/verified_pause.json')
assert package['success'] and status['success'] and package['build_identity'] == status['build_identity']
saved = Path(status['test_saved'])
checks = read(saved / 'Prototype/pause_menu_check.json')
assert checks['success'] and checks['count'] >= 27 and all(c['passed'] for c in checks['checks'])
assert package['pause_menu_checks'] == checks['count']
OUT.mkdir(parents=True, exist_ok=True)
for name in ['pause_1080','pause_hover_1080','pause_nav_hover_1080','pause_room_1080','pause_kk_1080','pause_en_1080','pause_admin_1080']:
    path = saved / 'Prototype/PauseCaptures' / (name + '.png')
    with Image.open(path) as picture:
        assert picture.size == (1920,1080)
    shutil.copy2(path, OUT / path.name)
(OUT / 'verification.json').write_text(json.dumps(dict(success=True, checks=checks, windows_package=package), ensure_ascii=False, indent=2), 'utf8')
(OUT / 'REPORT.md').write_text(f'''# ЭХО — живое меню паузы

Пять стеклянных кнопок в одну строку, логотип, крупный заголовок «Останься в тишине.»
с красным акцентом и широкая «Вернуться в игру» со значком воспроизведения.
Игровая жёлтая подсказка и старый непрозрачный фон удалены.
Красный у кнопок появляется только от мыши; фокус не включает подсветку.
Кнопки «Игра», «Карты», «Онлайн» убраны именно из паузы; добавлена «Главное меню».
Она возвращает исходную титульную сцену и её фоновый цикл без сброса состояния кампании.

За интерфейсом — другой ракурс пустой консультационной комнаты, собранной из
материалов и мебели кампании: кровь на стенах/полу, движущийся туман, камера,
качающаяся лампа, мерцание и красный пульс. Герой и монстры скрыты.
Положение героя, здоровье, патроны, враги и время прохождения не меняются на паузе.
Возврат из разделов сохраняет эту сцену, «Вернуться»/Escape восстанавливают игру.
Администратор сохраняет свои кнопки и летающую камеру. Активный агент ждёт на паузе.
Главное меню с медсестрой/скримером сохраняет прежний сценарий.

Пакетный EXE прошёл {checks['count']} проверок: камера/лампа/свет/кровь,
заморозка мира, единая строка меню, hover/focus/leave, RU/KK/EN, переходы разделов,
восстановление камеры/здоровья/патронов и режим администратора.
Контрольные суммы EXE и контента — в verification.json. Полное ручное прохождение не заявляется.

![Пауза](pause_1080.png)

![Наведение](pause_hover_1080.png)

Запуск: PLAY_UNREAL.cmd → Игра → Новая игра → Escape.
Повтор: Tools/verify_windows_package.ps1 -Mode Pause.
''', 'utf8')
print(json.dumps(dict(success=True, checks=checks['count'], report=str(OUT / 'REPORT.md')), ensure_ascii=False))
