"""Package the reviewed PDF atlas and Poppler-rendered plan images."""
from pathlib import Path
import csv
import hashlib
import json
import zipfile

from PIL import Image
from pypdf import PdfReader

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'output/pdf/ECHO_Hospital_2D'
manifest = json.loads((OUT / 'manifest.json').read_text(encoding='utf-8'))
plans = [p for p in manifest['page_index'] if 2 <= p['page'] <= 15]
for p in plans:
    old = OUT / f"plan-{p['page']:02}.png"
    target = OUT / f"{p['page']:02}_{p['key']}.png"
    if old.exists():
        old.replace(target)
    with Image.open(target) as im:
        assert max(im.size) == 4000, (target, im.size)

index = '\n'.join(f"{p['page']:02}_{p['key']}.png - {p['title']}" for p in plans)
readme = f'''ЭХО / полный 2D-атлас текущей карты игры
Дата: 22.10.2026

С ЧЕГО НАЧАТЬ
Откройте PDF: 29 страниц, все 333 зоны.
Страница 1: единицы и условные обозначения.
Страница 2: общий план всего комплекса.
Страницы 3-15: территория, каждый этаж и отдельные корпуса.
Страница 16: лестницы, лифт, высоты и конструктивные размеры.
Страницы 17-29: полный реестр помещений с размерами и площадями.

PNG ДЛЯ ПРОСМОТРА И МАСШТАБИРОВАНИЯ
14 изображений; длинная сторона каждого - 4000 пикселей.
{index}

КАК ЧИТАТЬ РАЗМЕРЫ
Все координаты и размеры - в метрах; площади - в квадратных метрах.
X направлен вправо, Y вниз на плане. Это оси игры, не стороны света.
Размер W x D задаёт границы зоны в текущей модели. Площадь W*D не
исключает стены, оборудование и пересечения открытых зон.
Внутренние размеры «в свету» приблизительно исключают стены 0.16 м.
Указанный масштаб PDF действует при печати 100%, без подгонки к листу.
Для PNG используйте числовые подписи и графическую масштабную линейку.

ПОКРЫТИЯ
Серо-синий: асфальт или открытая кровля.
Светлый голубой: плитка / пол закрытого корпуса.
Зелёный: две плоские зелёные вставки в S05 и S15; в игре это зелёное
покрытие, а не объёмные травинки. Общая площадь вставок 1592.48 м².
Светлая штриховка: на данной высоте нет сгенерированного пола.
Основание ниже первого этажа и наложенные покрытия перечислены в CSV.
Строки покрытий могут перекрываться по высоте и слою; их площади нельзя
просто складывать для получения уникальной площади территории.

ТАБЛИЦЫ CSV
Кодировка UTF-8 с BOM, разделитель «;», десятичный разделитель «.».
01: все 333 зоны, названия, координаты, ширина, глубина, площадь, вход.
02: прямоугольники покрытий, высота верха и слои наложения.
03: стены и наддверные перемычки; перемычка не перекрывает проход внизу.
04: лестницы и их 450 ступеней, лифт, свет и инженерные компоненты.
Слои покрытий: 0 - основание, 10 - пол, 11 - кровельная полоса,
15 - дорожка из плитки, 20 - зелёная вставка.

ИСТОЧНИК И ПРОВЕРКА
План построен из текущих данных игры hospital_campus_v2 и её кода
построения геометрии. Сохранены реальные размеры, не проектный эскиз.
hospital_world_source.json содержит исходные данные карты.
manifest.json содержит оглавление, количества и контрольные суммы.
SHA-256 данных карты: {manifest['source_sha256']}
Карта и игровая сборка при подготовке атласа не изменялись.
'''
(OUT / 'README.txt').write_text(readme, encoding='utf-8-sig')

pdf = OUT / 'ЭХО_точная_2D_карта_333_зоны.pdf'
assert len(PdfReader(pdf).pages) == 29
with (OUT / '01_Все_333_зоны_метры.csv').open(encoding='utf-8-sig', newline='') as f:
    rooms = list(csv.DictReader(f, delimiter=';'))
assert len(rooms) == len({r['ID'] for r in rooms}) == 333
source = ROOT / 'unreal/EchoWard/Content/CampaignData/hospital_world.json'
assert hashlib.sha256(source.read_bytes()).hexdigest() == manifest['source_sha256']
assert (OUT / 'hospital_world_source.json').read_bytes() == source.read_bytes()
archive = OUT.parent / 'ЭХО_2D_карта_полный_архив.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for path in sorted(OUT.iterdir()):
        if path.is_file():
            assert path.stat().st_size > 0, path
            z.write(path, 'ECHO_Hospital_2D/' + path.name)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert len(z.namelist()) == 22, len(z.namelist())
print(json.dumps({'archive': str(archive), 'bytes': archive.stat().st_size,
                  'files': 22, 'pages': 29, 'zones': 333, 'pngs': 14}, ensure_ascii=False))
