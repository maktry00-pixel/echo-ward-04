"""Export the saves screen from the matching tested Windows executable."""
from pathlib import Path
import json,shutil
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'unreal/EchoWard';OUT=ROOT/'artifacts/unreal_save_menu'
def read(p):return json.loads(p.read_text('utf-8-sig'))
package=read(P/'Saved/PackagedWindows/package_verified.json');status=read(P/'Saved/PackagedWindows/verified_saves.json')
assert package['success'] and status['success'] and package['build_identity']==status['build_identity']
saved=Path(status['test_saved']);checks=read(saved/'Prototype/save_menu_check.json')
assert checks['success'] and checks['count']>=21 and package['save_menu_checks']==checks['count'] and all(c['passed'] for c in checks['checks'])
OUT.mkdir(parents=True,exist_ok=True)
for name in ['saves_1080','saves_hover_1080','save_room_1080','saves_ingame_1080','saves_kk_1080','saves_en_1080']:
    source=saved/'Prototype/SaveMenuCaptures'/(name+'.png')
    with Image.open(source) as im:assert im.size==(1920,1080),(name,im.size)
    shutil.copy2(source,OUT/source.name)
result={'success':True,'native_live_3d':True,'monsters_in_save_set':False,'yellow_gameplay_hint':False,'resolution':'1920x1080','checks':checks,'windows_package':package,'human_playtest_complete':False}
(OUT/'verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
(OUT/'REPORT.md').write_text(f'''# ЭХО — экран сохранений

Экран оформлен по образцу: прозрачная стеклянная панель, логотип, Literata, тонкие разделители, значки ручных/автоматических слотов и компактные кнопки. Шесть ручных слотов и три автосохранения сохранены. Пустые/некорректные слоты нельзя загрузить; ручная запись доступна только при активной кампании. Красный цвет появляется только под мышью, не от фокуса клавиатуры. Жёлтая игровая подсказка удалена. Результат записи показан отдельной спокойной строкой.

За интерфейсом — отдельная настоящая 3D-комната из мебели/материалов кампании: кровать, коляска, шкафы, стеллаж, стол, бумаги, кровь на полу и стенах (13 декалей), влажная плитка и приоткрытая дверь. Камера медленно движется, подвесная лампа покачивается, свет мерцает/пульсирует, у пола движется туман. В наборе нет персонажей и монстров; остальные фоновые сцены скрываются. Тихая вентиляция/капли сохраняются. Открывается как из главного меню, так и из паузы. Герой не перемещается в декорацию; сохранение захватывает его настоящее состояние.

Назад из титульного меню ведёт в раздел «Игра», из действующей кампании — в паузу. Загрузка делает плавное затемнение и возвращает камеру персонажу. Основное меню, его скример и зомби в разделе игры сохранены.

Проверено в Windows EXE: **{checks['count']} проверка** записи/загрузки тестовых слотов, правил доступности кнопок, паузы, камеры/лампы/красного света, отсутствия скелетов/монстров в комнате, отсутствия игровой подсказки в дереве интерфейса, hover/focus/leave, возврата из титульного меню и паузы. Тесты используют отдельную папку сохранений. Шесть кадров1920×1080, включая RU/KK/EN. Контрольная сумма EXE и архивов контента — в `verification.json`. Полное человеческое прохождение не заявляется.

![Сохранения](saves_1080.png)

![Комната без интерфейса](save_room_1080.png)

Код: `WardGameMenuUI.cpp::SaveMenu`, `WardSaveMenuScene.cpp`, `WardSaveMenuAssessment.cpp`. Повтор: `Tools/verify_windows_package.ps1 -Mode Saves` и `tools/verify_save_menu.py`. Запуск: `PLAY_UNREAL.cmd`.
''',encoding='utf8')
print(json.dumps({'success':True,'checks':checks['count'],'report':str(OUT/'REPORT.md')},ensure_ascii=False))
