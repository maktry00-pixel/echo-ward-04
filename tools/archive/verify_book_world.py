"""Export complete 3D-world evidence only from matching Windows-package tests."""
from pathlib import Path
import json
import shutil
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT/'unreal/EchoWard'
PACKAGE=PROJECT/'Saved/PackagedWindows'
OUT=ROOT/'artifacts/unreal_book_world'
def read(p):return json.loads(p.read_text('utf-8-sig'))
marker=read(PACKAGE/'package_verified.json')
assert marker['success'] and marker['world_id']=='book_atlas_328_3d_v1'
reports=[]
for mode in ['world','worldcapture']:
    status=read(PACKAGE/f'verified_{mode}.json')
    assert status['success'] and status['build_identity']==marker['build_identity']
    saved=Path(status['test_saved'])
    result=read(saved/'Prototype/book_world_check.json')
    assert result['success'] and result['count']>=18 and all(c['passed'] for c in result['checks'])
    assert result['rooms']==328 and result['locations']==13
    reports.append(result)
OUT.mkdir(parents=True,exist_ok=True)
for code in ['1F','B1','DG','2F','IF','PS','3F','RH','HS','SV','ST','RF','B2']:
    source=saved/'Prototype/BookWorldCaptures'/f'world_{code}.png'
    with Image.open(source) as im:assert im.size==(1920,1080)
    shutil.copy2(source,OUT/source.name)
(OUT/'verification.json').write_text(json.dumps(dict(success=True,windows_package=marker,physics=reports[0],render=reports[1]),ensure_ascii=False,indent=2),'utf8')
fps=[x['fps'] for x in reports[1].get('sampled_performance',[])]
performance=f'{min(fps):.1f}–{max(fps):.1f} FPS' if fps else 'не измерено'
(OUT/'REPORT.md').write_text(f'''# ЭХО — полный пустой комплекс в 3D

PLAY_UNREAL.cmd → Игра → Новая игра теперь запускает полный 3D-комплекс из книги.
328 помещений и наружных зон, 13 планов: 1F, B1, DG, 2F, IF, PS, 3F, RH, HS, SV, ST, RF, B2.
Плановые прямоугольники и размеры взяты из book_map.json. Межэтажный шаг — 4,8 м,
высота внутренних помещений — 3,8 м. В основном здании 1F/2F/3F, два подвала и крыша.
Шесть отдельных корпусов связаны крытой галереей; территория открыта между ними.
Расстояния между корпусами и конструкция лестницы выбраны при реализации:
книга задаёт отдельные планы, а не точную трёхмерную геодезическую модель комплекса.

WASD — идти; мышь — смотреть; Shift — бег; M / Tab — навигационный план;
Escape — пауза; F5 — ручное сохранение. Лестница расположена слева от центрального
коридора, у его начала: проём ядра A. По ней можно пройти от B2 до RF и обратно.
Из поперечного коридора 1F доступны DG и IF, галерея дальше ведёт к PS/RH и HS/SV.
Проходы в комнаты открыты. В помещениях только архитектура, свет и обозначения ID;
мебели, интерактивных предметов и противников нет. Главный экран и его фон сохранены.

Новая игра использует этот мир по умолчанию. Старый CampaignSaves не перезаписывается:
новые слоты находятся в BookWorldSaves, имеют world_id=book_atlas_328_3d_v1.
Загрузчик проверяет принадлежность сохранения новой карте. Историческая кампания
доступна техническим флагом -WardLegacyCampaign; исторические проверки запускают её явно.
Это пустая геометрия для осмотра, без сюжетного наполнения большой книги.

Windows-пакет: {reports[0]['count']} проверок на NullRHI и {reports[1]['count']} в реальном рендере.
Коллизии полов, стен и входов проверены для всех 328 помещений. Персонаж физически
прошёл все десять лестничных маршей вверх и вниз и путь по галерее в RH;
телепортации используются только для расстановки начала отдельных тестов/кадров.
Проходы всех комнат к коридорам и шесть связей корпусов проверены capsule sweep
и лучами к полу. Не заявляется ручной обход человеком каждого квадратного метра.
Проверены запись и загрузка нового слота, защита от старого формата, пауза и 2D-план.
Сняты 13 кадров 1920×1080. Короткие неподвижные выборки после перемещения: {performance};
это не измерение всего маршрута и не гарантированный минимальный FPS.

Конструкция использует отдельную Nanite-копию стандартного куба с простой box-коллизией.
Полы и стены объединены в непрерывные поверхности. Пустой комплекс освещается прямыми
источниками; у игровой камеры отключён Lumen GI и выбраны экранные отражения,
чтобы размер комплекса не переполнял surface cache. Камеры титульных меню сохранены.
Подготовка: Content/Python/prepare_book_world.py. Проверки пакета:
Tools/verify_windows_package.ps1 -Mode World / WorldCapture / Validate.

![Первый этаж](world_1F.png)
![Подземный центр](world_B2.png)
![Территория](world_ST.png)
![Крыша](world_RF.png)
''','utf8')
print(json.dumps(dict(success=True,rooms=328,locations=13,checks=reports[0]['count'],fps=performance,report=str(OUT/'REPORT.md')),ensure_ascii=False))
