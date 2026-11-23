"""Export screenshots and checks from the current Windows build."""
from pathlib import Path
import json
import shutil
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'unreal/EchoWard'
OUT = ROOT / 'artifacts/unreal_difficulty_menu'
def read(path):
    return json.loads(path.read_text('utf-8-sig'))
package = read(PROJECT / 'Saved/PackagedWindows/package_verified.json')
status = read(PROJECT / 'Saved/PackagedWindows/verified_difficulty.json')
assert package['success'] and status['success'] and package['build_identity'] == status['build_identity']
saved = Path(status['test_saved'])
checks = read(saved / 'Prototype/difficulty_menu_check.json')
assert checks['success'] and checks['count'] >= 22 and all(c['passed'] for c in checks['checks'])
assert package['difficulty_menu_checks'] == checks['count']
OUT.mkdir(parents=True, exist_ok=True)
for name in ['difficulty_1080','difficulty_hover_1080','difficulty_room_1080','difficulty_kk_1080','difficulty_en_1080']:
    path = saved / 'Prototype/DifficultyCaptures' / (name + '.png')
    with Image.open(path) as picture:
        assert picture.size == (1920,1080)
    shutil.copy2(path, OUT / path.name)
(OUT / 'verification.json').write_text(json.dumps(dict(success=True, checks=checks, windows_package=package), ensure_ascii=False, indent=2), 'utf8')
(OUT / 'REPORT.md').write_text(f'''# ЭХО — выбор сложности

Четыре стеклянные карточки с описаниями внутри, логотип, широкая кнопка «Назад».
Красный цвет появляется только от наведения мыши; фокус и выбранная сложность его не включают.
Жёлтая игровая подсказка удалена из этого экрана.
Живой фон — отдельный ракурс комнаты записей из больничных декораций кампании:
медленное движение камеры, качающаяся лампа, мерцание, красный свет, туман и кровь.
Персонажи и монстры скрыты. При возврате в раздел «Игра» его фоновый зомби сохранён.
Все четыре режима запускают настоящую кампанию, переход и сохранения сохранены.

Обычный fullscreen заменён на EWindowMode::WindowedFullscreen. Старое fullscreen=true
также получает новый режим без рамки. В конфигурации r.FullScreenMode=1 и
PreferredFullscreenMode=1: F11/Alt+Enter не возвращают эксклюзивный режим.
Настройки изображения не запрашивают пересоздание окна при каждом движении ползунка.
Физическое разрешение полноэкранного окна равно рабочему столу; качество рендера
регулируется масштабом рендера. Оконный режим сохраняет выбор размера окна.

В упакованном EXE: {checks['count']} проверок сцены, указателя, RU/KK/EN,
реального borderless/windowed режима в настройках, перехода и запуска четырёх сложностей.
Проверка Print Screen и переключения окон описана отдельно в HANDOFF_NEXT_GPT.md;
автоматические снимки RenderOffscreen сами по себе её не подтверждают.

![Выбор сложности](difficulty_1080.png)

![Наведение](difficulty_hover_1080.png)

Запуск: PLAY_UNREAL.cmd → Игра → Новая игра.
''', 'utf8')
print(json.dumps(dict(success=True, checks=checks['count'], report=str(OUT / 'REPORT.md')), ensure_ascii=False))
