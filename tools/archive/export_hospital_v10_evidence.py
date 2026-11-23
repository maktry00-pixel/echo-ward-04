"""Export evidence for the data-only stocked furniture revision."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard';PACKAGE=PROJECT/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v10'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source);exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
baseline=read(ROOT/'artifacts/unreal_hospital_v9/verification.json')
assert d['review_revision']==10 and eh==baseline['executable_sha256']==sha(PROJECT/'Binaries/Win64/EchoWard.exe')
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v10.json');story=read(PACKAGE/'verified_story_v7.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for result in (visual,story):assert result['success'] and result['executable_sha256'].lower()==eh and result['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==89 and rendered['success'] and rendered['count']==23 and story['checks']==23
assert envelope['success'] and sum(envelope['failed'].values())==0
for name,key in (('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('clutter_acceptance','map_sha256')):
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh
images=[]
for c in d['locations']+d['furnishing_captures']:
    p=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    with Image.open(p) as im:assert im.size==(1920,1080)
    shutil.copy2(p,OUT/p.name);images.append(p.name)
assert len(images)==85
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
for start in range(0,len(images),20):
    names=images[start:start+20];canvas=Image.new('RGB',(1640,math.ceil(len(names)/4)*254),'#10191b');draw=ImageDraw.Draw(canvas)
    for i,name in enumerate(names):
        with Image.open(OUT/name) as source_image:im=source_image.convert('RGB');im.thumbnail((400,225))
        x=i%4*410;y=i//4*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name[6:-4],font=font,fill='white')
    canvas.save(OUT/f'contact_{start//20+1}.png')
for status,target,original in ((native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check')):
    shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inventory=read(OUT/'clutter_inventory.json')
report=dict(success=True,review_revision=10,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,
    executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=89,story_checks=23,
    visual_initial_checks=23,world_captures=85,envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],clutter_inventory=inventory,screenshots=images,
    implementation='Data-only layer rendered by the already-built and unchanged v9 Game/Editor.',
    limits=['New belongings and supplies are decorative box prototypes, not collectable inventory or interactive containers.',
            'Some inspection views are close-ups; captures are not an exhaustive visual tour of every room.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(PACKAGE/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'REPORT.md').write_text(f'''# ЭХО — дополнительные вещи v10

Добавлено **{inventory['items']:,} декоративных предметов**, состоящих из {inventory['parts']:,} простых частей. Вещи расставлены на {inventory['stocked_furniture']:,} существующих столах, полках, койках, тумбах, тележках и палетах.

Есть кружки, истории болезни, папки, книги, флаконы лекарств и реактивов, медицинские лотки, полотенца, бельё, коробки с припасами и продуктами, посылки, инструментальные кейсы, бумаги на полу и дорожные решётки. Это декоративные прототипы; новые контейнеры и предметы инвентаря не добавлялись.

Сохранены вся прежняя мебель, габариты карты, освещение, растения, луна, камеры, все проходы, укрытия, лифт, лестницы, оружие, сюжетные предметы и мобы. Новые детали не создают блокирующей коллизии.

Проверки: реальные игровые 89/89, сюжетные 23/23; независимые карта 15/15, мебель 9/9, сюжет 11/11, вещи 8/8. Отрисовка: 23 начальные проверки и 85 свежих кадров 1920×1080. Проверка оболочек: 899569 проб, без найденных пропусков. Не все старые обзорные ракурсы показывают комнаты целиком.

Изменены только данные окружения. Прежние проверенные Game/Editor используются без пересборки C++; запускаемый и исходный EXE и JSON совпадают.

Запуск: `PLAY_UNREAL.cmd`. **B** — яркий режим, **M / Tab** — план.

EXE SHA256: `{eh}`

Карта SHA256: `{mh}`
''',encoding='utf-8')
handoff=ROOT/'HANDOFF_NEXT_GPT.md';old=handoff.read_text(encoding='utf-8-sig')
if not old.startswith('# Завершено: дополнительные вещи v10'):
    handoff.write_text(f'''# Завершено: дополнительные вещи v10 — 23 октября 2026

Пользователь попросил «ещё много вещей». Добавлен отдельный data-only слой clutter_dressing и environment_dressing.boxes layer10: {inventory['items']} логических декоративных предметов / {inventory['parts']} частей на {inventory['stocked_furniture']} опорах. 15 типов: кружки, истории болезни, папки, флаконы лекарств, кейсы, коробки, полотенца, книги, продукты, бельё, лотки, посылки, реактивы, бумаги на полу, решётки дороги. Это декор, а не новые подбираемые предметы/интерактивные контейнеры.

tools/hospital_clutter_v10.py использует фактическую геометрию предыдущей мебели и 3D-index, ставит предметы в пределах полки/стола, не пересекает старые объекты, проверяет контакты по опорам; все детали nonblocking. Никаких изменений старой геометрии, collision_features, маршрутов, lights, существующей мебели, сюжета и мобов. Объекты с kind/zone/item_id/layer10; счётчик EnvironmentBoxes автоматически учитывает их. Новые материалы используют уже cooked M_HospitalSurface. C++ не менялся, source/staged EXE и Editor от v9 сохранены; пересборка/recook не нужны. Полный build_hospital_layout.py завершает v10 и повторяет аудиты. audit_hospital_clutter_v10.py имеет8контрактов сохранения/опор/сборки. Не запускать прежние exporters v9/v8 наv10.

Финал EXE {eh}, JSON {mh}. NativeWorld89/89, Story23/23, VisualV10 initial23/23 +85 свежих1920×1080. Независимые карта15/мебель9/сюжет11/вещи8,899569проб оболочек без пропусков. Исходный и staged EXE+JSON равны, package_verified.json обновлён. Доказательства artifacts/unreal_hospital_v10/REPORT.md, verification.json, clutter_inventory.json, clutter_acceptance.json,85PNG/5contact. Перед финальным сообщением просмотреть5контактов и выполнить ModeValidate. tools/export_hospital_v10_evidence.py проверяет все актуальные SHA и создаёт эти артефакты. Запуск PLAY_UNREAL.cmd; B яркость. Новые85видов — прежние80 плюс5clutter_furnished_*. Камеры старых видов местами показывают близкие стены; не утверждать полный визуальный обход. Ограничения сюжета v7 и камер v9 ниже сохраняются.

---

'''+old,encoding='utf-8')
print(json.dumps(dict(success=True,items=inventory['items'],screenshots=len(images),executable_sha256=eh,map_sha256=mh),ensure_ascii=False))
