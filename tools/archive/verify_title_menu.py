"""Publish menu evidence from the exact verified Windows executable."""
from pathlib import Path
import json,shutil
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT/'unreal/EchoWard'
OUT=ROOT/'artifacts/unreal_title_menu'
def read(p):return json.loads(p.read_text('utf-8-sig'))
package=read(PROJECT/'Saved/PackagedWindows/package_verified.json')
menu_status=read(PROJECT/'Saved/PackagedWindows/verified_menu.json')
assert package['success'] and menu_status['success']
assert package['build_identity']==menu_status['build_identity']
assert package.get('menu_checks',0)>=56
saved=Path(menu_status['test_saved'])
checks=read(saved/'Prototype/menu_check.json')
assert package['menu_checks']==checks['count']
assert checks['success'] and checks['count']>=56 and all(c['passed'] for c in checks['checks'])
OUT.mkdir(parents=True,exist_ok=True)
for name in ['menu_1080','corridor_1080','game_1080','gallery_1080','menu_kk_1080','menu_en_1080','menu_hover_1080','menu_card_hover_1080','door_open_1080','scare_1080','nurse_1080','zombie_head_1080','ball_roll_1080','ball_wait_1080','hand_reach_1080','entrance_idle_1080']:
    p=saved/'Prototype/MenuCaptures'/(name+'.png')
    with Image.open(p) as im:assert im.size==(1920,1080),(name,im.size)
    shutil.copy2(p,OUT/p.name)
result=dict(success=True,resolution='1920x1080',native_3d_background=True,player_character_in_menu=False,zombie_nurse_3d=True,idle_story_loop_seconds=60,ball_wait_scene_seconds=5,entrance_hold_scene_seconds=3,
            automatic_checks=checks,windows_package=package,human_playtest_complete=False,
            trailer='Design placeholder; no completed trailer is claimed.',
            sounds='Original procedural ventilation, dripping water, UI click, door scraping and jumpscare (PCM peak 0.94); respects effects volume. Existing game radio/whisper/knock.',
            source_assets='Head/hand: existing HorrorGameMaker CC0; forearm geometry and foley original; procedural blood decals title-only. Existing licensed campaign PBR assets, original title-set props and procedural wear/linen/dust. Transparent wordmark generated with built-in ImageGen; LOGO_PROMPT.md records the prompt.')
(OUT/'verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'REPORT.md').write_text('''# ЭХО — живое главное меню

Настоящий коридор Unreal Engine без игрового персонажа. Фоновая история повторяется раз в минуту: зомби выглядывает из открытой палаты, дверь закрывается, затем выкатывается мяч. Мяч стоит5с, появляется объёмная кисть с длинным предплечьем и утягивает его обратно. После закрытия двери выходит медсестра, выполняет прежний маршрут и возвращается. В конце приоткрывается одна створка главного входа, остаётся так3с и закрывается. Нажатие «Игра» прерывает фоновые события и запускает прежний скример. Голова и кисть используют геометрию уже лицензированного CC0-монстра, с маской отсечения; полноценный монстр в меню не отображается. Основа — Medical_Female_01 из Microsoft Rocketbox (MIT), адаптированная под хоррор: загрязнённая форма, оригинальные объёмные бинты и колпак, анимация тела/головы и сканированный нож Poly Haven (CC0). Это адаптация по мотивам референса, не точная копия его модели. Камера медленно движется, красная лампа мигает, у пола движется локальный туман, коляска покачивается. Добавлены каталка, медицинская тележка, приоткрытая боковая дверь с красным светом и отражением, потёртости, микропыль и лёгкая глубина резкости. Применены модели и материалы существующей игры; референс пользователя в сборку не включён.

Слева семь пунктов меню со шрифтом с засечками и тонкими светлыми вертикальными штрихами. Красная полоса, свечение и пульс появляются только под мышью; после ухода курсора плавно гаснут. «Игра» не выделена постоянно, фокус клавиатуры красный цвет не включает. То же правило действует в стеклянном разделе игры и обычных кнопках настроек, сохранений и других страниц. Кровь —11 декалей только в главной декорации меню; мигание сохранено. RU / ҚАЗ / EN убрано из угла, язык меняется в настройках. Потёртый логотип появляется с коротким искажением. При наведении текст сдвигается на5px, у карточек усиливаются рамки/zoom. ТРЕЙЛЕР / СКРИНШОТЫ подписаны под рамками. Галерея показывает кадры игры; трейлер — материал в подготовке.

Вентиляция, электрическое жужжание, капли, тихий hover и более тяжёлый щелчок выбора созданы процедурно для проекта. Редкие помехи, стуки и шёпот используют существующие звуки игры. Начало меню тихое. При выборе «Игра» интерфейс исчезает, камера приближается к настоящему проёму, обе створки открываются со скрежетом. После входа и короткого затемнения медсестра резко появляется перед камерой с оригинальным звуком. Вся сцена занимает около 7,6 с; только после неё открывается раздел игры. Повторные клики во время сцены блокируются. Громкость скримера подчиняется звуковым настройкам; системная громкость Windows не изменяется. Сохранены выбор новой игры / последнего сохранения / списка сохранений и выбор сложности только после новой игры.

Проверено в Windows EXE:56 автоматических проверок главного меню, включая наведение мыши, гашение свечения после ухода и отсутствие красного выделения от фокуса клавиатуры. Также проверены пауза мира, камера, скрытие персонажа, новая игра/последнее сохранение, возврат, галерея, заглушка трейлера, GPU-шейдеры, скелет/анимация медсестры, створки, вход камеры и однократность скримера. Кадры RU / ҚАЗ / EN —1920×1080. Пройден минутный цикл, вращение/остановка мяча, порядок сцен, повтор и сохранность картинок после ожидания/GC. Полное человеческое прохождение не заявляется.

![Главное меню](menu_1080.png)

Логотип создан встроенным ImageGen: потёртые кириллические буквы «ЭХО», красная линия ЭКГ, прозрачный фон. Полный запрос — [LOGO_PROMPT.md](LOGO_PROMPT.md); исходный PNG — `unreal/EchoWard/RawAssets/Menu/logo_echo.png`.

Запуск из корня проекта: `PLAY_UNREAL.cmd`.

Повтор проверки: `unreal/EchoWard/Tools/title_assessment.ps1 -Packaged`. Подробные результаты и идентификатор EXE — `verification.json`.
''',encoding='utf-8')
print(json.dumps(dict(success=True,automatic_checks=checks['count'],resolution='1920x1080',report=str(OUT/'REPORT.md')),ensure_ascii=False))
