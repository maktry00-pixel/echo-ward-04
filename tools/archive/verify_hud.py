"""Export actual gameplay HUD captures from the verified Windows EXE."""
from pathlib import Path
import json
import shutil
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'unreal/EchoWard'
OUT = ROOT / 'artifacts/unreal_game_hud'
def read(path): return json.loads(path.read_text('utf-8-sig'))
package = read(PROJECT / 'Saved/PackagedWindows/package_verified.json')
status = read(PROJECT / 'Saved/PackagedWindows/verified_hud.json')
assert package['success'] and status['success'] and package['build_identity'] == status['build_identity']
checks = read(Path(status['test_saved']) / 'Prototype/hud_check.json')
assert checks['success'] and checks['count'] >= 18 and all(c['passed'] for c in checks['checks'])
assert package['hud_checks'] == checks['count']
OUT.mkdir(parents=True, exist_ok=True)
for item in checks['captures']:
    name = item['image']
    path = Path(status['test_saved']) / 'Prototype/HUDCaptures' / (name+'.png')
    expected = (1280,720) if name=='hud_720' else (1920,1200) if name=='hud_1200' else (1920,1080)
    with Image.open(path) as picture: assert picture.size==expected, (name,picture.size)
    assert item['frame']['bounded']
    shutil.copy2(path, OUT / path.name)
(OUT / 'verification.json').write_text(json.dumps(dict(success=True, checks=checks, windows_package=package), ensure_ascii=False, indent=2), 'utf8')
(OUT / 'REPORT.md').write_text(f'''# ЭХО — игровой HUD

Интерфейс по образцу: красная вертикальная линия и задача сверху слева,
шесть сегментов шума справа, здоровье/дыхание снизу слева, рамка с названием,
силуэтом оружия и боеприпасами справа. Белые и бирюзовые значения, красный акцент;
Здоровье всегда красное с плавной пульсацией; при≤25% пульс сильнее и быстрее. Активные сегменты шума красные, тихие серые. Оформление относится к HUD, игровые модели и геометрия карты сохранены.

Подсказка взаимодействия появляется у подходящего предмета, длинный текст переносится.
Подсказки, поиск, субтитры, перезарядка, прицел/блокировка ствола, сохранение,
эвакуация и админ-индикаторы сохранены. Пистолет и дробовик используют свои настоящие боеприпасы.
На паузе HUD скрыт, после возврата появляется. Пользовательские переключатели HUD сохранены.
Размер задаётся минимумом отношения ширины/высоты к1920×1080, углы привязаны к экрану.

{checks['count']} проверок EXE: фактические числа, контекст, настройки, локализация длинной
задачи, оружие, скрытие/возврат и границы текста/панелей на1920×1080,1280×720,1920×1200.
Аудит измеряет фактически нарисованный текст Canvas; в обычной игре JSON не создаётся.
Кадры сняты в Unreal, макет или референс не подставляется в игровой фон.

![HUD](hud_1080.png)

![Взаимодействие](hud_interaction_1080.png)

Запуск: PLAY_UNREAL.cmd → Игра → Новая игра.
''', 'utf8')
print(json.dumps(dict(success=True, checks=checks['count'], report=str(OUT/'REPORT.md')), ensure_ascii=False))
