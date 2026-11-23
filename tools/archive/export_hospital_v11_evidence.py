"""Export evidence for the data-only stocked furniture revision."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard';PACKAGE=PROJECT/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v11'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source);exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
baseline=read(ROOT/'artifacts/unreal_hospital_v10/verification.json')
assert d['review_revision']==11 and eh==baseline['executable_sha256']==sha(PROJECT/'Binaries/Win64/EchoWard.exe')
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v11.json');story=read(PACKAGE/'verified_story_v7.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for result in (visual,story):assert result['success'] and result['executable_sha256'].lower()==eh and result['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==89 and rendered['success'] and rendered['count']==23 and story['checks']==23
assert envelope['success'] and sum(envelope['failed'].values())==0
for name,key in (('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('details_acceptance','map_sha256')):
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh
images=[]
for c in d['locations']+d['furnishing_captures']:
    p=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    with Image.open(p) as im:assert im.size==(1920,1080)
    shutil.copy2(p,OUT/p.name);images.append(p.name)
assert len(images)==90
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
for start in range(0,len(images),20):
    names=images[start:start+20];canvas=Image.new('RGB',(1640,math.ceil(len(names)/4)*254),'#10191b');draw=ImageDraw.Draw(canvas)
    for i,name in enumerate(names):
        with Image.open(OUT/name) as source_image:im=source_image.convert('RGB');im.thumbnail((400,225))
        x=i%4*410;y=i//4*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name[6:-4],font=font,fill='white')
    canvas.save(OUT/f'contact_{start//20+1}.png')
for status,target,original in ((native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check')):
    shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inventory=read(OUT/'details_inventory.json')
report=dict(success=True,review_revision=11,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,
    executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=89,story_checks=23,
    visual_initial_checks=23,world_captures=90,envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],microdetails_inventory=inventory,screenshots=images,
    implementation='Data-only layer rendered by the already-built and unchanged v9 Game/Editor.',
    limits=['New belongings and supplies are decorative box prototypes, not collectable inventory or interactive containers.',
            'Some inspection views are close-ups; captures are not an exhaustive visual tour of every room.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(PACKAGE/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'REPORT.md').write_text(f'''# ЭХО — мелкие детали v11

Добавлено ещё **{inventory['items']:,} декоративных деталей**, {inventory['types']} видов. Они состоят из {inventory['parts']:,} простых частей. Прежняя мебель и 15 727 предметов предыдущего этапа сохранены.

## Новые детали

Медицинские: шприцы, пакеты марли, блистеры таблеток, ампулы в держателях, пинцеты, перчатки, мыло и зубные щётки. Канцелярские и бытовые: ручки, степлеры, калькуляторы, бейджи, стикеры, конверты, скотч, чай, салфетки и столовые приборы. Они расставлены на свободных участках существующей мебели, с учётом назначения помещения.

Настенные: 541 решётка вентиляции, 540 розеток, 540 дозаторов, 540 медицинских информационных табличек, потёртости и небольшие трещины. Таблички имеют простую графику; это не новая система читаемых записок. Крепление проверено по реальным стенам в обход дверных проёмов.

Дополнительно: 2 381 штрихкод на старых коробках, упаковки у урн, следы возле мебели, опавшие листья и крепёж фонарных столбов. Все детали пока простые модели без новой игровой интерактивности.

## Проверка

Сохранены все прежние размеры, этажи, мебель, свет, маршруты, укрытия, лестницы, лифт, сюжетные предметы, мобы и сохранения. Новые детали не имеют блокирующей коллизии.

Реальная игра: 89/89 проверок карты и 23/23 сюжетных механик. Данные: карта 15/15, мебель 9/9, сюжет 11/11, новые детали 9/9. Оболочки: 899 569 выборочных проверок без найденных пропусков. Отрисовка: 23 начальные проверки и 90 свежих снимков 1920×1080, включая пять крупных планов новых деталей. Снимки не заменяют личный обход каждой комнаты; некоторые старые ракурсы показывают близкие стены и оборудование.

Изменены данные окружения; используются прежние Game/Editor без изменения C++. Исходный и запускаемый EXE и JSON совпадают.

Запуск: `PLAY_UNREAL.cmd`. **B** — яркость, **M / Tab** — карта.

EXE SHA256: `{eh}`

Карта SHA256: `{mh}`
''',encoding='utf-8')
handoff=ROOT/'HANDOFF_NEXT_GPT.md';old=handoff.read_text(encoding='utf-8-sig')
if not old.startswith('# Завершено: мелкие детали v11'):
    handoff.write_text(f'''# Завершено: мелкие детали v11 — 23 октября 2026

Последний запрос «ещё много вещей и маленьких деталей» выполнен. tools/hospital_microdetails_v11.py добавляет data-only слой microdetails_dressing и environment_dressing.boxes layer11:15082 логических деталей,72328 частей,29видов. Медицинские мелочи(шприцы/марля/блистеры/ампулы/пинцеты/перчатки), канцелярия(ручки/степлеры/бейджи/калькуляторы/стикеры/конверты), бытовые(мыло/щётки/чай/салфетки/приборы),541решётка/540розеток/540дозаторов/540табличек,2381штрихкод,упаковки/потёртости/трещины/листья/крепёж. Это декоративные простые модели, не новые предметы инвентаря или рабочая электрика.

Сохранены вся v10 мебель,15727предметов слоя10,размеры,oldJSONkeys,коллизии,маршруты,сюжет,мобы,lights/materials. Генератор занимает только свободные участки опор с 3D-index. Направление реальных стен берётся по radiator/camera_housing; в трёх точках каждой новой настенной детали проверяется настоящее backing в raster_walls, не воображаемая стена. Все новые части nonblocking. На коробки добавлены полоски-штрихкоды поверх прежних этикеток. Розетки/таблички простые модели без читаемых сюжетных текстов. Новые материалы runtimeMID уже cooked M_HospitalSurface, C++/Game/Editor не менялись и пересборка не нужна. Полный build_hospital_layout.py заканчиваетсяv11+аудиты.

Финал EXE {eh}, JSON {mh}. NativeWorld89/89, Story23/23, VisualV11 initial23/23 +90 свежих1920×1080. Данные15/9/11/9,оболочки899569точек/0пропусков. NativeWorld и Story проверены на финальном JSON, source/stagedEXE+JSON равны. package_verified.json обновлён. Доказательства artifacts/unreal_hospital_v11/REPORT.md, verification.json, details_inventory.json, details_acceptance.json,90PNG/5contact. Перед финальной передачей просмотреть5контактов и ModeValidate. Новые5явныхкамер details_clinical/office/cafe/storage/wall смотрят на мелкие предметы; независимая выборка лучей по raster_walls для этих5не нашла пересечений. Старые85видов остаются, некоторыепоказываюткрупныестены; не заявлять полный визуальный обход.

Проверки Tools/verify_windows_package.ps1 -Mode World; verify_hospital_story_v7.ps1; verify_hospital_visual_v11.ps1; tools/audit_hospital_microdetails_v11.py. tools/export_hospital_v11_evidence.py экспортирует только при совпадении всех SHA/проверок и обновляет package_verified. Старые exporters не запускать наv11. Запуск PLAY_UNREAL.cmd;Bяркость. Технические ограниченияv7/v9/v10 ниже сохраняются.

---

'''+old,encoding='utf-8')
print(json.dumps(dict(success=True,items=inventory['items'],types=inventory['types'],screenshots=len(images),executable_sha256=eh,map_sha256=mh),ensure_ascii=False))
