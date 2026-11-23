"""Publish the screenshot-driven fixes only after cooked runtime validation."""
from pathlib import Path
from collections import Counter
import json
import shutil
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/'unreal/EchoWard'
TEST=P/'Saved/PackageTests'
OUT=ROOT/'artifacts/unreal_visual_repairs'
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
package=read(P/'Saved/PackagedWindows/package_verified.json')
assert package['success'] and package['smoke_checks']==54 and package['physics_checks']==8 and package['physical_route_verified']
assert package['captured_views']==28 and package['gpu_materials_checked']==258
monster=read(P/'Saved/Prototype/stalker_import.json')
assert monster['success'] and monster['physics_asset']!='None'
data=read(P/'Content/CampaignData/campaign.json')
repairs=Counter(r['visual_repair'] for r in data['instances'] if r.get('visual_repair') and not r.get('visible',True))
assert repairs['obsolete_flat_stain']==3 and repairs['door_overlap_window']>=30 and repairs['door_overlap_rail']>0
capture=TEST/'Capture/Saved/Prototype/CampaignCaptures'
hands={i:read(capture/f'pose_{i}.json') for i in range(18,22)}
for pose in hands.values():
    finger=pose['right_finger_joints_weapon_cm']['Bip01 R Finger12']
    assert pose['right_grip_error_cm']<3 and pose['right_palm_down_alignment']>.8
    assert 3<finger[0]<9 and -10<finger[2]<-2
OUT.mkdir(exist_ok=True,parents=True)
for i,name in [(18,'pistol_front'),(19,'pistol_side'),(20,'pistol_aim_down'),(21,'pistol_aim_up'),(22,'doorway'),(23,'floor'),(24,'stalker'),(25,'stalker_walk'),(26,'stalker_lunge'),(27,'stalker_fallen')]:shutil.copy2(capture/f'view_{i}.png',OUT/f'{name}.png')
for source,name in [('045657','before_hand'),('045713','before_aim'),('045739','before_monster')]:
    before=Path('C:/Users/YOUR_WINDOWS_USER/OneDrive/Pictures/Screenshots')/f'Снимок экрана 2026-10-20 {source}.png'
    if before.exists():shutil.copy2(before,OUT/f'{name}.png')
result={'success':True,'windows_package':package,'monster':monster,'hidden_set_dressing':dict(repairs),'hand_pose_evidence':hands,'human_playtest_complete':False}
(OUT/'verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'REPORT.md').write_text(f'''# Исправления по семи скриншотам — 20 октября

1. Убраны декоративные окна с жалюзи, пересекавшие вход в служебное крыло. Перила и настенные полосы разрезаны по границам дверных проёмов; их крепления убраны из проходов. Исправлен повторный поворот створок: экспорт содержал текущее открытое положение, к которому ещё раз добавлялись 90°. Нормализация теперь использует фактическое `is_open`, а не начальное состояние.
2. Кисть пистолета перенесена к настоящей рукояти. Направление пясти теперь идёт вниз по рукояти; пальцы сгибаются вокруг неё. Указательный палец отдельно направлен к спусковой скобе. Проверены крупные ракурсы спереди, сбоку, с прицелом вниз на 50° и вверх на 45°.
3. Убраны три старые овальные накладки на полу. Это были полупрозрачные приплюснутые сферы из Godot, а не отражения; в Unreal они выглядели как непрозрачные лужи.
4. Старое многоногое существо заменено скелетным человекоподобным монстром [HorrorGameMaker / City Building Game Art](https://opengameart.org/content/3d-horror-game-monster), CC0. Подключены исходные Idle/Walk/Run и выпад на основе Jump. Добавлены пространственные шаги, тихий рык поблизости, физическое падение и восстановление живого тела при загрузке. Три размера сохраняют роли врагов; навигация, боевые характеристики и учебные алгоритмы не заменены.

## Подтверждение

54 игровых проверки и 8 проверок физики прошли в самостоятельном EXE. Включены проверки центров всех закрытых створок, удаления мешающего декора, совместимости скелетов всех 25 врагов и ragdoll/восстановления. Получены 28 свежих кадров, проверены 258 GPU-материалов. Автоматический физический маршрут завершился победой за {int(package['agent_seconds'])//60}:{int(package['agent_seconds'])%60:02d}; это время агента, знающего решения.

Крупные ракурсы дополнительно проверяют направление ладони и положение костей указательного пальца относительно геометрии рукояти/спусковой скобы. Старый тест совпадения кисти с заданной точкой был недостаточен и сохранён только как одна из проверок. Измерения приложены в JSON; кадры осмотрены отдельно.

| Проверка | Кадр |
|---|---|
| Пистолет сбоку | [pistol_side.png](pistol_side.png) |
| Прицел вниз | [pistol_aim_down.png](pistol_aim_down.png) |
| Свободный проём | [doorway.png](doorway.png) |
| Пол без овала | [floor.png](floor.png) |
| Новый монстр | [stalker.png](stalker.png) |
| Физическое падение | [stalker_fallen.png](stalker_fallen.png) |

## Пределы

Модель героя и его одежда прежние; пальцы корректируются процедурно, специальных профессиональных клипов пистолета здесь нет. Выпад монстра адаптирует исходный Jump, не выдаётся за отдельную авторскую анимацию удара. Все враги используют одну новую модель с разными размерами, это не несколько разных моделей. Ручное прохождение всей кампании и каждый промежуточный кадр анимации ещё не проверены. Сюжетные сохранения native v2 совместимы; физические позы трупов в слотах не сохраняются.

Запуск: **PLAY_UNREAL.cmd → Игра → Новая игра**. Источник и SHA-256 архивов: `assets/models/horror_game_maker/SOURCE.json`.
''',encoding='utf-8')
print(json.dumps({'success':True,'game_checks':54,'physics_checks':8,'captures':28,'materials':258,'repairs':dict(repairs)},ensure_ascii=False))
