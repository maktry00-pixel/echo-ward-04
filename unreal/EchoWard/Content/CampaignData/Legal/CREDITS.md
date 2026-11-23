# Ресурсы игры «Эхо»

## Шрифты и фотографии справочника — 18 октября 2026

- **Literata**: The Literata Project Authors, SIL Open Font License1.1. Источник: https://github.com/google/fonts/tree/main/ofl/literata . Локальная лицензия: `ui/fonts/literata-OFL.txt`.
- **Noto Sans**: Noto Project Authors, SIL Open Font License1.1. Источник: https://github.com/google/fonts/tree/main/ofl/notosans . Локальная лицензия: `ui/fonts/notosans-OFL.txt`.
- Исходные файлы шрифтов не изменены. URL и SHA256 всех четырёх файлов: `ui/fonts/SOURCES.json`; воспроизводимая загрузка: `tools/fetch_ui_fonts.py`.
- Восемь JPG в `ui/guide/` — собственные1280×720 снимки игровой сцены с имеющимися лицензированными моделями/материалами. Съёмка: `tools/capture_guide_photos.gd`. Это не чужие игровые скриншоты. Два пользовательских макета использованы как визуальные ориентиры и не упакованы как игровые ресурсы.

## Текстуры 4K и интерфейс, 18 октября 2026

Четыре материала Poly Haven CC0-1.0 обновлены с сохранением исходного набора2K: [Terrazzo Tiles](https://polyhaven.com/a/terrazzo_tiles), [Old Linoleum Flooring 01](https://polyhaven.com/a/old_linoleum_flooring_01), [Painted Plaster Wall](https://polyhaven.com/a/painted_plaster_wall), [Blue Plaster Weathered](https://polyhaven.com/a/blue_plaster_weathered). Скачаны оригинальные4K JPEG diffuse/normalGL/roughness, не увеличенные2K. URL, MD5 и размеры файлов: `textures/polyhaven/HD_SOURCE_MANIFEST.json`; авторы материалов — в прежнем SOURCE_MANIFEST.json. Загрузка и проверка: `tools/fetch_hd_textures.py`.

Оформление UI, иконка записи, тексты меню и RU/EN/KK каталог созданы в исходниках проекта с помощью AI. Фон меню — существующая3D-сцена игры с указанными здесь моделями и материалами. Новых внешних моделей в этой итерации нет. Английская синтетическая озвучка прежних кассет не заменена казахской или русской; переводятся субтитры.

## Глава изолятора

Планировка, реквизит пациента17, генератор с трубами/приборами/маховиком, дополнительные детали преследователя, механизм выбора питания, история и шейдерная отделка созданы в исходниках с помощью AI. Расстановка использует уже указанные ниже CC0-модели Poly Haven: тумбы, мебель, Circuit Board, Portable Cassette Player, Service Pistol и отделочные PBR. Новых внешних моделей не скачано.

Форма приклада/цевья/рукояти дробовика переработана исходной геометрией с фасками и сужением. Движение руки при ноже и поза работы — процедурные дополнения к указанным ниже родным клипам Rocketbox. Звуки restraint_drag, isolation_pipe и isolation_relay синтезируются в sound_bank.gd. Они не являются записью профессионального актёра/оркестра. Папка доказательств в финале и падающий светильник — оригинальные модели проекта.

## Персонажи и анимации

Kenney — Animated Characters 1 / Animated Characters Survivors. Лицензия CC0.

- Автор и исходный набор: https://opengameart.org/content/animated-characters-1
- Сайт автора: https://kenney.nl/assets/animated-characters-survivors
- Текст лицензии из архива: `models/characters/KENNEY_LICENSE.txt`.
- Предыдущий герой использовал characterMedium, idle, run и survivorMaleB. zombieC сохранён в старом прототипе. Текущий герой использует Microsoft Rocketbox; враги используют оригинальную процедурную анатомию из `scripts/aberration_visual.gd` и `scripts/horror_creature.gd`.
- В игре изменены пропорции, оттенок заражённых, скорость и смешивание анимаций.

## Материалы окружения

Дополнительно: ambientCG Metal032, карты цвета, нормалей OpenGL и шероховатости, 2K JPG; CC0. Используются на оборудовании нового крыла. Текущий пистолет сохраняет собственные карты Poly Haven. Источник: https://ambientcg.com/view?id=Metal032

ambientCG — Concrete035 и Tiles074, карты цвета, нормалей OpenGL и шероховатости, 1K JPG. Лицензия CC0.

- https://ambientcg.com/view?id=Concrete035
- https://ambientcg.com/view?id=Tiles074
- https://docs.ambientcg.com/license/

Процедурная модель существа, планировка, оставшаяся мебель из геометрии, модели/этикетки пропуска и батареи, освещение, интерфейс и звуковые синтезаторы созданы в исходниках проекта с помощью AI. Кассеты теперь используют модель Poly Haven. Эффекты генерируются локально; основной музыкальный трек указан ниже. Игра использует собственное название и окружение; материалы Resident Evil не включены.

Неудачные попытки скачать другие публичные наборы не являются используемыми ресурсами и не входят в эти титры.

## Оружие

Текущий пистолет: Service Pistol — Mateusz Sadek, Poly Haven, CC0, исходные карты PBR 2K. Используется деревянный вариант с отдельными затвором, курком, спуском и магазином; исходная раскладка запасных частей собрана в одно оружие. https://polyhaven.com/a/service_pistol . Нож: Fish Knife, тот же автор и лицензия: https://polyhaven.com/a/fish_knife .

Архивный пистолет Kenney Blaster Kit 2.1 — модель `blaster-a.glb`, переименована в `models/weapons/service_pistol.glb`; в актуальном игровом оружии не используется. Лицензия CC0; оригинальная лицензия: `models/weapons/KENNEY_LICENSE.txt`. Источник: https://kenney.nl/assets/blaster-kit

## Текущий взрослый герой

Microsoft Rocketbox Avatar Library — Male_Adult_04, модель, цвет и нормали тела и головы, прозрачность волос, четыре исходных движения: m_idle_breathe_01, m_walk_neutral, m_run_neutral, m_crouch_idle. MIT, Copyright (c) 2020 Microsoft. Источник: https://github.com/microsoft/Microsoft-Rocketbox . Полная лицензия включена: `models/rocketbox/LICENSE.md`.

Интеграция: `scripts/rocket_survivor.gd`. Удалены горизонтальный перенос корня и вспомогательные треки, сохранены длины конечностей, установлено отсечение альфа-канала волос. Совместимые Biped-клипы используют исходные ориентации костей; их импортированная rest-поза совпадает с первым кадром, поэтому повторная rest-коррекция ошибочно отменяет позу. При тихой ходьбе используется ходьба с понижением таза и двухзвенной коррекцией ног, удерживающей исходные траектории стоп. Поза рук и оружие синхронизированы с физическим обновлением. Перезарядка и обыск теперь используют процедурную двухручную IK; реакции на удар и падение также созданы в проекте. Отдельный съёмный магазин не анимирован, полноценного ragdoll нет.

## Предыдущая основа героя и анимации

Quaternius Universal Base Characters, бесплатная Standard-версия: Superhero_Male_FullBody, кожа, глаза, брови, нормали и шероховатость. CC0. Источник: https://quaternius.com/packs/universalbasecharacters.html и https://quaternius.itch.io/universal-base-characters . Лицензия архива: `models/survivor/QUATERNIUS_LICENSE.txt`.

Производная одежда: треугольники разделены на кожу, куртку, брюки и обувь; изменены контуры куртки и брюк, пересчитаны нормали, сохранены веса и скелет. Скрипт: `tools/build_survivor_outfit.py`. Пояс, карманы и волосы сделаны геометрией и следуют костям. Это упрощённая одежда, не готовый фотореалистичный костюм.

Quaternius Universal Animation Library, Standard, файл UAL1_Standard.glb: idle, walk, jog, sprint, crouch, pistol aim/reload. CC0. Источник: https://quaternius.com/packs/universalanimationlibrary.html . Лицензия: `models/survivor/ANIMATION_LICENSE.txt`. Перенос движений учитывает разные исходные положения костей; анимация и ручная поза рук обновляются в одном физическом шаге. Эти архивные клипы перезарядки не подключены к текущему Rocketbox: руки актуального героя используют процедурную IK. Процедурные враги и материал тканей существа `shaders/organic_skin.gdshader` созданы в проекте с помощью AI.

## Музыка

«Lost in a bad place (horror ambience loop)» — congusbongus, 2022, CC0. Используется `audio/lost.ogg`, включён циклический режим и динамическая громкость. Источник и лицензия: https://opengameart.org/content/lost-in-a-bad-place-horror-ambience-loop . Остальные звуки и слой погони синтезируются исходниками игры.

## Мебель и предметы Poly Haven, 17 октября 2026

19 моделей в проекте, CC0 1.0. Четырнадцать моделей ближнего просмотра имеют карты 2K; прочее окружение — 1K. Геометрия мебели нормализована по границам модели; столкновения заданы отдельно под игровой масштаб. Мелкие предметы сохраняют исходные пропорции и UV. У тумб и комодов двигается отдельный импортированный ящик. Повторное применение мебели после обыска не выдаёт предметы заново. Официальная лицензия: https://polyhaven.com/license .

- Steel Frame Shelves 01: James Ray Cock; https://polyhaven.com/a/steel_frame_shelves_01
- Painted Wooden Nightstand: Kirill Sannikov; https://polyhaven.com/a/painted_wooden_nightstand
- Wheelchair 01: Garreth Dean; https://polyhaven.com/a/wheelchair_01
- Medical Box: Ulan Cabanilla; https://polyhaven.com/a/medical_box
- Plastic Jerrycan: Ulan Cabanilla; https://polyhaven.com/a/plastic_jerrycan
- Metal Office Desk: Ulan Cabanilla; https://polyhaven.com/a/metal_office_desk
- Old Bed Frame: Luca B; https://polyhaven.com/a/old_bed_frame
- Crowbar 01: Alexander Otterbeck; https://polyhaven.com/a/crowbar_01
- Vintage Wooden Drawer 01: James Ray Cock; https://polyhaven.com/a/vintage_wooden_drawer_01
- Service Pistol: Mateusz Sadek; https://polyhaven.com/a/service_pistol
- Fish Knife: Mateusz Sadek; https://polyhaven.com/a/fish_knife
- Circuit Board: Benny Weimer; https://polyhaven.com/a/circuit_board
- Portable Cassette Player: Mateusz Sadek; https://polyhaven.com/a/portable_cassette_player
- Small Plastic Torch: Abiyyu Panggalih (Abiyyu_xyz); https://polyhaven.com/a/small_plastic_torch
- Medical Tape: Miroslav Turura; https://polyhaven.com/a/medical_tape

Файлы: `models/polyhaven/`. Ссылки, авторы и проверенные MD5 каждого скачанного файла: `models/polyhaven/SOURCE_MANIFEST.json`. Скрипты воспроизведения: `tools/fetch_environment_assets.py` и `tools/fetch_detail_assets.py` и `tools/fetch_room_assets.py`. **Powered by Poly Haven** — загрузчики используют официальный публичный API. Модели существ остаются оригинальными процедурными моделями проекта; модели мебели не заменяют существ и героя.

## Оригинальные дополнения выживания, 18 октября 2026

Вымышленный помповый дробовик, патроны, баррикады, дополнительная анатомия/движения существ и шейдеры органической кожи/древесины созданы в исходниках проекта с помощью AI. Это не импортированные реалистичные модели. Существующие металлические PBR-карты используют источники, перечисленные в этом файле. Новых внешних моделей в этой итерации не загружено.

`audio/stories/shift.wav`, `evidence.wav`, `ceiling.wav`, `farewell.wav`: оригинальные тексты команды/AI, английская синтетическая речь Microsoft Zira Desktop через установленный Windows System.Speech, созданная локально. Точные тексты и метод: `audio/shift_recordings.json`. Русские субтитры в `scripts/survival_upgrade.gd`. Это синтез речи, не голос актёра и не клон чьего-либо голоса. Звуки дробовика, помпы, заряжания и скрежета генерируются в `scripts/sound_bank.gd`.

## Комнаты и освещение, 17 октября 2026

Новые модели CC0 с PBR 2K:

- Sofa 03 — Fran Calvente; https://polyhaven.com/a/sofa_03
- School Chair 01 — Ethan Place; https://polyhaven.com/a/SchoolChair_01
- Painted Wooden Cabinet 02 — Kirill Sannikov; https://polyhaven.com/a/painted_wooden_cabinet_02
- Desk Lamp Arm 01 — Kuutti Siitonen (модель и текстуры), Yann Kervran (риг); https://polyhaven.com/a/desk_lamp_arm_01

Новые материалы CC0, 2K, цвет/нормали/шероховатость: Terrazzo Tiles, Old Linoleum Flooring 01, Painted Plaster Wall, Blue Plaster Weathered и Stretch Poplin. Точные авторы и MD5: `textures/polyhaven/SOURCE_MANIFEST.json`, лицензия рядом. Для больничной ткани используются нормали переплетения и шероховатость Stretch Poplin; исходный ярко-зелёный цвет не используется. Ссылки:

- https://polyhaven.com/a/terrazzo_tiles
- https://polyhaven.com/a/old_linoleum_flooring_01
- https://polyhaven.com/a/painted_plaster_wall
- https://polyhaven.com/a/blue_plaster_weathered
- https://polyhaven.com/a/stretch_poplin

Закруглённые матрасы/подушки, складки покрывал и занавесок, окна с жалюзи и схема освещения созданы в исходниках проекта с помощью AI. Подвижные тележки удалены из сцен; `medical_cart.gd` и часть скачанных принадлежностей сохранены как архив и не означают, что тележки используются в актуальной игре.


## Menu nurse / медсестра главного меню (2026-10-20)

Microsoft Rocketbox: Medical_Female_01 mesh and textures, existing neutral walk / breathe motion. Copyright (c) 2020 Microsoft; MIT License. https://github.com/microsoft/Microsoft-Rocketbox/tree/master/Assets/Avatars/Professions/Medical_Female_01

Echo Ward adaptations: original procedural cap and bandage geometry, grime materials, additive horror pose, entrance staging, original 48 kHz door / jumpscare / footstep synthesis. Fish Knife: Mateusz Sadek / Poly Haven, CC0, already included above. The user supplied reference image is not distributed in the game.


## Menu ambient story / фоновая история меню (2026-10-20)

Head and hand: the existing HorrorGameMaker CC0 skeletal mesh and PBR textures; rendered as cropped geometry in a title-only material. The gameplay creature asset is unchanged. Original Echo Ward contributions: organic forearm OBJ, ball/grime/blood procedural materials, door/prop choreography, quiet rolling and dragging audio synthesis. No user reference image is distributed.

## Archive game submenu zombie (2026-10-20)

Zombie model, original diffuse/normal maps and walk animation by **Pixelhouse team**. Source: https://opengameart.org/content/zombie — **Creative Commons Attribution 3.0**, https://creativecommons.org/licenses/by/3.0/ . Author: http://www.pixelhouse.com.ar . Adaptations: Unreal import, root locking, runtime scale and placement. Similar licensed alternative to the supplied reference; the reference image is not distributed. Original Echo Ward additions: archive-wing arrangement using existing campaign furniture, tile/grime materials, glass menu rendering and three-room choreography.
