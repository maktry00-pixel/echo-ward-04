"""Export actual GPU captures from the tested Windows build."""
from pathlib import Path
import json
import shutil
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'unreal/EchoWard'
OUT = ROOT / 'artifacts/unreal_about_menu'

def read(path):
    return json.loads(path.read_text('utf-8-sig'))

package = read(PROJECT / 'Saved/PackagedWindows/package_verified.json')
status = read(PROJECT / 'Saved/PackagedWindows/verified_about.json')
assert package['success'] and status['success']
assert package['build_identity'] == status['build_identity']
saved = Path(status['test_saved'])
checks = read(saved / 'Prototype/about_menu_check.json')
assert checks['success'] and checks['count'] >= 18
assert all(c['passed'] for c in checks['checks'])
assert package['about_menu_checks'] == checks['count']
OUT.mkdir(parents=True, exist_ok=True)
for name in ['about_01_1080', 'about_02_1080', 'about_03_1080', 'about_04_1080',
             'about_hover_1080', 'about_room_1080', 'about_kk_1080', 'about_en_1080']:
    source = saved / 'Prototype/AboutCaptures' / (name + '.png')
    with Image.open(source) as im:
        assert im.size == (1920, 1080), (name, im.size)
    shutil.copy2(source, OUT / source.name)
result = dict(success=True, native_live_3d=True, resolution='1920x1080',
              monsters_in_about_set=False, yellow_gameplay_hint=False,
              checks=checks, windows_package=package, human_playtest_complete=False)
(OUT / 'verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), 'utf8')
(OUT / 'REPORT.md').write_text(f'''# ЭХО — раздел «Об игре»

Стеклянный интерфейс по образцу: логотип, широкая кнопка «Назад», четыре вкладки,
крупная Literata, текст слева, большой кадр справа и нижние разделы. Все фотографии — неизменённые кадры1920×1080 одной серии из больничного окружения Unreal.
Жёлтые заголовки заменены холодными белыми/бирюзовыми. Игровая подсказка внизу удалена.
Текущая вкладка отмечена тонким бирюзовым подчёркиванием, красный появляется только под мышью.
Все четыре страницы и темы A*, CSP, Minimax/alpha-beta сохранены. Длинные страницы прокручиваются.

Фон — отдельная пустая консультационная комната из мебели и материалов кампании,
с другим расположением предметов и ракурсом. Камера движется, лампа покачивается,
холодный свет мерцает, красный пульсирует, у пола перемещается туман.
Кровь размещена на стенах и полу (13 декалей). В наборе нет скелетных персонажей;
монстры других фоновых сцен скрыты. Главный скример и остальные меню сохранены.
Открытие работает из титульного меню и паузы; возврат восстанавливает камеру героя.

Проверено в текущем Windows EXE: **{checks['count']} проверок**.
Восемь кадров1920×1080, включая все вкладки, hover, комнату без UI и KK/EN.
Состояния фоновой камеры/лампы/света проверяются во время паузы, hover отдельно от фокуса.
Проверка языков сверяет реальные надписи KK/EN с каталогом. Исправлен общий кэш перевода:
он теперь учитывает язык, поэтому казахские строки не остаются после переключения на английский.
EXE/content identity и подробные результаты находятся в `verification.json`.
Полное человеческое прохождение не заявляется.

![Об игре](about_01_1080.png)

![Пустая комната](about_room_1080.png)

Повтор: `Tools/verify_windows_package.ps1 -Mode About`, затем `tools/verify_about_menu.py`.
Запуск: `PLAY_UNREAL.cmd` → «Об игре».
''', 'utf8')
print(json.dumps(dict(success=True, checks=checks['count'], report=str(OUT / 'REPORT.md')), ensure_ascii=False))
