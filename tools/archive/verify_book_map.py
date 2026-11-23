"""Export screenshots of the complete, presentation-only in-game atlas."""
from pathlib import Path
import hashlib
import json
import shutil
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT/'unreal/EchoWard'
OUT=ROOT/'artifacts/unreal_book_map'
def read(path):return json.loads(path.read_text('utf-8-sig'))
package=read(PROJECT/'Saved/PackagedWindows/package_verified.json')
status=read(PROJECT/'Saved/PackagedWindows/verified_book.json')
assert package['success'] and status['success'] and package['build_identity']==status['build_identity']
saved=Path(status['test_saved'])
checks=read(saved/'Prototype/book_map_check.json')
assert checks['success'] and checks['count']>=27 and all(c['passed'] for c in checks['checks'])
assert package['book_map_checks']==checks['count'] and package['book_map_rooms']==328 and package['book_map_locations']==13
source=PROJECT/'Content/CampaignData/book_map.json'
staged=PROJECT/'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/book_map.json'
assert hashlib.sha256(source.read_bytes()).digest()==hashlib.sha256(staged.read_bytes()).digest()
data=read(staged)
assert len(data['locations'])==13 and sum(len(l['rooms']) for l in data['locations'])==328
assert data['presentation_only'] and all(set(r)=={'id','name','rect'} for l in data['locations'] for r in l['rooms'])
OUT.mkdir(parents=True,exist_ok=True)
names=['book_all_1080']+[f"book_{l['id']}_1080" for l in data['locations']]+['book_en_1080','book_kk_1080','book_720']
for name in names:
    path=saved/'Prototype/BookMapCaptures'/(name+'.png')
    with Image.open(path) as im:
        assert im.size==((1280,720) if name=='book_720' else (1920,1080))
    shutil.copy2(path,OUT/path.name)
assert len({hashlib.sha256((OUT/(f"book_{l['id']}_1080.png")).read_bytes()).hexdigest() for l in data['locations']})==13
(OUT/'verification.json').write_text(json.dumps({'success':True,'checks':checks,'package':package,'book_map_sha256':hashlib.sha256(source.read_bytes()).hexdigest()},ensure_ascii=False,indent=2),'utf8')
(OUT/'REPORT.md').write_text(f'''# ЭХО - полный 2D-план из книги

M / Tab во время игры открывает «Полный план»: 328 помещений, 13 локаций.
Это обзорный план расширения; текущая 3D-кампания из 54 зон остаётся прежней.
Никаких предметов, врагов, заданий, сюжетных событий или мебели на полном плане нет.
Видны контуры комнат, коридоры, вертикальные ядра и ID. Наведение показывает имя и размер.

Локации: 1F, B1, DG, 2F, IF, PS, 3F, RH, HS, SV, ST, RF, B2.
«Все планы» показывает 13 отдельных листов, а не географическую расстановку зданий.
Лист выбирается кнопкой или щелчком по плитке. Колесо масштабирует относительно курсора,
левая кнопка перетаскивает, двойной щелчок / «Сбросить вид» возвращает общий вид листа.
Прямоугольники крыши и двора увеличены в 2 раза согласно примечанию атласа.

«Текущая игра» возвращает прежнюю карту реального прохождения; её допуски/отметки сохранены.
Полный план доступен для просмотра сразу, без получения предмета-карты. Escape закрывает экран.
Данные book_map.json импортированы из исходного atlas_data.json, ID всех 328 комнат сверены
с приложенным ECHO_Master_Game_Bible_Complete (1).pdf. В рантайме PDF не нужен.

Проверено в текущем Windows-пакете: {checks['count']} проверок, 17 снимков, включая все 13 локаций,
RU/KK/EN интерфейс и 720p. Названия комнат из русской книги используют существующий словарь;
полный перевод всех 328 новых названий на EN/KK в эту правку не входит.
Проверки происхождения/геометрии/упаковки не подтверждают строительство или проходимость
будущего 3D-мира. Проверка перетаскивания подтверждает запрос захвата мыши;
ручной тест полного жеста не проводился.

Повтор: Tools/verify_windows_package.ps1 -Mode Book.

![Весь комплекс](book_all_1080.png)
![Первый этаж](book_1F_1080.png)
![Подземный центр](book_B2_1080.png)
''','utf8')
print(json.dumps({'success':True,'checks':checks['count'],'rooms':328,'locations':13,'report':str(OUT/'REPORT.md')},ensure_ascii=False))
