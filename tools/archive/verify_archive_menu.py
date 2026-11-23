"""Publish evidence from the matching packaged Windows executable."""
from pathlib import Path
import json,shutil
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard';OUT=ROOT/'artifacts/unreal_archive_menu'
def read(p):return json.loads(p.read_text('utf-8-sig'))
package=read(PROJECT/'Saved/PackagedWindows/package_verified.json');status=read(PROJECT/'Saved/PackagedWindows/verified_archive.json')
assert package['success'] and status['success'] and package['build_identity']==status['build_identity']
saved=Path(status['test_saved']);checks=read(saved/'Prototype/archive_check.json')
assert checks['success'] and checks['count']>=23 and package['archive_checks']==checks['count'] and all(c['passed'] for c in checks['checks'])
OUT.mkdir(parents=True,exist_ok=True)
for name in ['game_glass_1080','game_glass_focus_1080','archive_corridor_1080','archive_second_room_1080','archive_third_room_1080','archive_zombie_1080','settings_hover_1080']:
    p=saved/'Prototype/ArchiveCaptures'/(name+'.png')
    with Image.open(p) as im:assert im.size==(1920,1080),(name,im.size)
    shutil.copy2(p,OUT/p.name)
result={'success':True,'resolution':'1920x1080','native_live_3d':True,'reference_pixel_identical':False,'checks':checks,'windows_package':package,'zombie':{'author':'Pixelhouse','source':'https://opengameart.org/content/zombie','license':'CC BY 3.0','exact_reference_model':False},'human_playtest_complete':False}
(OUT/'verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
(OUT/'REPORT.md').write_text(f'''# ЭХО — раздел «Игра»

Отдельный живой 3D-фон для раздела «Игра», выбора сложности и сохранений: новое крыло больницы из моделей и материалов кампании. Главная сцена с медсестрой, головой, мячом и прежний вход со скримером сохранены. Новая сцена скрывается в главном меню и во время игры.

Оформление по референсу: логотип, разреженный подзаголовок, широкая кнопка «Назад», прозрачная рамка и три стеклянные строки с рисованными значками. Красное свечение появляется только при наведении мыши и плавно гаснет после её ухода. «Новая игра», «Игра», выбранные вкладки и фокус клавиатуры не создают постоянной красной подсветки. Это правило также применяется к обычным кнопкам настроек, сохранений и других разделов. Жёлтая подсказка в этом разделе удалена. Literata и RU / KK / EN сохранены.

В новой декорации: влажная плитка, грязь, 17 декалей крови, мебель из игры, низкий туман, холодные лампы, красный аварийный свет и редкое мерцание. Камера плавно движется. Зомби проходит через три настоящих проёма; дверь закрывается, он остаётся в комнате **10 секунд**, затем идёт в следующую. Цикл — 72 секунды, повороты сглажены. Тихие шаги, скрип дверей, капли и стуки подчиняются громкости эффектов.

Использована похожая модель [Zombie от Pixelhouse](https://opengameart.org/content/zombie), CC BY 3.0, с оригинальными текстурами и walk-анимацией. Это **не точная модель с присланного фото**. Авторство и изменения включены в `CampaignData/Legal/CREDITS.md`; исходник и авторский readme сохранены в `assets/models/menu_zombie`. Пользовательские изображения в игру не включены.

Проверено в Windows EXE: **{checks['count']} проверок нового раздела**, остановка по реальному таймеру **{checks['room_wait_real_seconds']:.2f} с**, семь кадров1920×1080. Подтверждены нейтральный цвет без мыши, красный hover и гашение после ухода, включая настройки и сохранённый фокус клавиатуры. Проверены камера/скелет при паузе, GPU-материалы, маршрут, сложность, сохранения и восстановление камеры. Дополнительные проверки и SHA256 EXE+контента — в `verification.json`. Полное человеческое прохождение не заявляется.

Потёртости стекла кешируются как текстуры вместо тысяч отдельных Slate-элементов каждый кадр. Ресурсы удерживаются TStrongObjectPtr для сохранности при GC.

![Раздел «Игра»](game_glass_1080.png)

Запуск: `PLAY_UNREAL.cmd`. Повтор: `unreal/EchoWard/Tools/verify_windows_package.ps1 -Mode Archive`, затем `tools/verify_archive_menu.py`. Новый код: `WardGameMenuUI.cpp`, `WardGameMenuScene.cpp`; импорт: `Content/Python/import_game_menu.py`.
''',encoding='utf8')
print(json.dumps({'success':True,'checks':checks['count'],'room_wait_seconds':checks['room_wait_real_seconds'],'report':str(OUT/'REPORT.md')},ensure_ascii=False))
