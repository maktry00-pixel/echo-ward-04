"""Export the training screen from its matching tested Windows executable."""
from pathlib import Path
import json
import shutil
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'unreal/EchoWard'
OUT = ROOT / 'artifacts/unreal_training_menu'

def read(path):
    return json.loads(path.read_text('utf-8-sig'))

package = read(PROJECT / 'Saved/PackagedWindows/package_verified.json')
status = read(PROJECT / 'Saved/PackagedWindows/verified_training.json')
assert package['success'] and status['success']
assert package['build_identity'] == status['build_identity']
saved = Path(status['test_saved'])
checks = read(saved / 'Prototype/training_menu_check.json')
assert checks['success'] and checks['count'] >= 25
assert all(c['passed'] for c in checks['checks'])
assert package['training_menu_checks'] == checks['count']
OUT.mkdir(parents=True, exist_ok=True)
names = [f'training_{i:02}_1080' for i in range(1, 9)]
names += ['training_hover_1080', 'training_room_1080', 'training_kk_1080', 'training_en_1080']
for name in names:
    source = saved / 'Prototype/TrainingCaptures' / (name + '.png')
    with Image.open(source) as im:
        assert im.size == (1920, 1080), (name, im.size)
    shutil.copy2(source, OUT / source.name)
result = dict(success=True, native_live_3d=True, resolution='1920x1080',
              monsters_in_training_set=False, yellow_gameplay_hint=False,
              checks=checks, windows_package=package, human_playtest_complete=False)
(OUT / 'verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), 'utf8')
(OUT / 'REPORT.md').write_text(f'''# ЭХО — обучение

Стеклянный экран по образцу: логотип, широкая «Назад», восемь вкладок01–08,
крупная Literata и большой кадр справа. Текст белый/бирюзовый, жёлтые заголовки
и нижняя игровая подсказка удалены. Красный появляется исключительно под мышью;
текущая страница отмечена тонким бирюзовым подчёркиванием.

Все восемь тем, подробности и подсказки клавиш сохранены. Клавиша карты в обучении
показана как M (Unreal поддерживает также Tab). Длинные страницы прокручиваются.
Все фото — неизменённые native Unreal кадры1920×1080 из одной холодной больничной серии.
Фотографии/логотип удерживаются сильными ссылками и заполняют широкие рамки.

За интерфейсом — отдельный пустой проход у палат с поперечным коридором и окном поста.
Использованы мебель и материалы кампании: кровать, коляска, шкаф, стеллаж, лампа и бумаги.
Камера медленно движется, приоткрытая дверь качается, холодный свет мерцает,
красная лампа пульсирует, четыре объёма тумана движутся у пола.
На стенах и полу24 кровавых декали, включая поперечный проход настроек. В наборе нет персонажей/монстров;
главный скример и остальные фоновые сцены сохраняют собственное поведение.

Работает из титульного меню и паузы, возврат восстанавливает камеру героя.
Код интерфейса разделяется с «Об игре» через IllustratedGuide, сохраняя его раскладку.

В текущем Windows EXE прошли **{checks['count']} проверки**: все восемь страниц,
управление, видимость подписей клавиш внутри scroll clip и отсутствие старой подсказки, hover/focus/leave, камера/дверь/свет во время
паузы, отсутствие скелетных персонажей и видимых монстров, возврат title/pause/resume,
реальные надписи KK/EN. Двенадцать кадров1920×1080.
EXE/content identity — в `verification.json`. Полное человеческое прохождение не заявляется.

![Первые шаги](training_01_1080.png)

![Пустой проход](training_room_1080.png)

Повтор: `Tools/verify_windows_package.ps1 -Mode Training`, затем `tools/verify_training_menu.py`.
Запуск: `PLAY_UNREAL.cmd` → «Обучение».
''', 'utf8')
print(json.dumps(dict(success=True, checks=checks['count'], report=str(OUT / 'REPORT.md')), ensure_ascii=False))
