"""Export native in-game inventory, documents and map captures from a matching package."""
from pathlib import Path
import json
import shutil
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'unreal/EchoWard'
OUT = ROOT / 'artifacts/unreal_field_menu'
def read(path):
    return json.loads(path.read_text('utf-8-sig'))

package = read(PROJECT / 'Saved/PackagedWindows/package_verified.json')
status = read(PROJECT / 'Saved/PackagedWindows/verified_field.json')
assert package['success'] and status['success'] and package['build_identity'] == status['build_identity']
saved = Path(status['test_saved'])
checks = read(saved / 'Prototype/field_menu_check.json')
assert checks['success'] and checks['count'] >= 22 and all(c['passed'] for c in checks['checks'])
assert package['field_menu_checks'] == checks['count']
OUT.mkdir(parents=True, exist_ok=True)
for name in ['inventory_1080','inventory_hover_1080','inventory_en_1080','inventory_kk_1080','inventory_720',
             'documents_empty_1080','documents_1080','map_locked_1080',
             'map_ward_1080','map_basement_1080','map_archive_1080']:
    path = saved / 'Prototype/FieldCaptures' / (name + '.png')
    with Image.open(path) as picture:
        assert picture.size == ((1280,720) if name.endswith('_720') else (1920,1080))
        if name == 'inventory_1080':
            # A native pistol must actually be rendered in the right preview.
            crop = picture.convert('RGB').crop((1000,280,1750,650))
            visible = sum(max(pixel) > 35 for pixel in crop.getdata())
            assert visible > 1500, 'Missing or unlit native item preview'
        if name == 'documents_1080':
            r, g, b = picture.convert('RGB').getpixel((1100,700))
            assert 60 < r < 220 and r > b + 15, 'Missing aged paper or wrong captured screen'
    shutil.copy2(path, OUT / path.name)
assert (OUT / 'map_basement_1080.png').read_bytes() != (OUT / 'map_archive_1080.png').read_bytes()
(OUT / 'verification.json').write_text(json.dumps(dict(success=True, checks=checks, windows_package=package), ensure_ascii=False, indent=2), 'utf8')
(OUT / 'REPORT.md').write_text(f'''# ЭХО — игровые экраны

I — инвентарь, J — документы, M / Tab — карта. Повторное нажатие или Escape
закрывает экран; вкладки позволяют переключаться, пока игра на паузе.
По умолчанию карта теперь открывает полный 2D-план из книги; прежние листы прохождения
доступны через «Текущая игра». Проверка ниже относится к сохранённым игровым листам.

Тёмная стилистика референсов: тонкие рамки, приглушённый текст, красное наведение,
сетка 4 × 3, модели предметов из игры, крупное превью, старая бумага и реальный план.
Сетка содержит 12 доступных мест; ограничения количества патронов и припасов сохранены.
Документы содержат только уже найденные записи. Пропуск можно осмотреть и перевернуть.

«Использовать» лечит, меняет оружие, заряжает вспышку или открывает осмотр.
«Объединить» + подходящие патроны заряжает оружие без создания новых боеприпасов.
Несовместимое сочетание ничего не расходует. Выброшенные припасы остаются у ног
и сохраняются в мире. Сюжетные предметы выбросить нельзя.

Карта открывается после получения плана. Вкладки показывают отделение / текущий
сектор, подвал и архив, используя настоящие координаты помещений и дверей.
Игрок, текущая комната, задача, доступные события и закрытые двери отмечены на плане.
Масштаб — колесом мыши, перемещение — перетаскиванием.

Проверено в Windows-пакете: {checks['count']} функциональных проверок и 11 снимков.
Интерфейс и описания переведены на RU / KK / EN. В 720p кнопки остаются в одну строку.
Полное прохождение кампании не повторялось.
Повтор: Tools/verify_windows_package.ps1 -Mode Field.

![Инвентарь](inventory_1080.png)
![Документы](documents_1080.png)
![Карта](map_ward_1080.png)
''', 'utf8')
print(json.dumps(dict(success=True, checks=checks['count'], report=str(OUT / 'REPORT.md')), ensure_ascii=False))
