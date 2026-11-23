"""Publish only matching v13 data, native tests and actual game captures."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont

ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard'
PACKAGE=PROJECT/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v13'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source)
exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
assert d['review_revision']==13 and eh==read(ROOT/'artifacts/unreal_hospital_v12/verification.json')['executable_sha256']==sha(PROJECT/'Binaries/Win64/EchoWard.exe')
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v13.json');story=read(PACKAGE/'verified_story_v7.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for result in (visual,story):assert result['success'] and result['executable_sha256'].lower()==eh and result['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==89 and rendered['success'] and rendered['count']==23 and story['checks']==23
assert envelope['success'] and sum(envelope['failed'].values())==0
for name,key in (('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('services_acceptance','map_sha256')):
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh,(name,a.get(key),mh)
images=[]
for c in d['locations']+d['furnishing_captures']:
    p=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    with Image.open(p) as im:assert im.size==(1920,1080)
    shutil.copy2(p,OUT/p.name);images.append(p.name)
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
for prefix,names,columns in [('contact',images,4),('services_contact',[x for x in images if x.startswith('world_services_')],3)]:
    for start in range(0,len(names),20):
        group=names[start:start+20];canvas=Image.new('RGB',(columns*410,math.ceil(len(group)/columns)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for i,name in enumerate(group):
            with Image.open(OUT/name) as src:im=src.convert('RGB');im.thumbnail((400,225))
            x=i%columns*410;y=i//columns*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name[6:-4],font=font,fill='white')
        canvas.save(OUT/f'{prefix}_{start//20+1}.png')
for status,target,original in ((native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check')):
    shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inventory=read(OUT/'services_inventory.json')
report=dict(success=True,review_revision=13,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,
    executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=89,story_checks=23,
    visual_initial_checks=23,world_captures=len(images),envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],services_inventory=inventory,screenshots=images,
    implementation='Data programme, room names, full-size equipment and physical Cyrillic sign geometry rendered by the retained Game/Editor.',
    limits=d['hospital_services']['limits']+['Some retained inspection cameras face close walls or furniture; captures do not inspect every room exhaustively.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(PACKAGE/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
summary=f'''# ЭХО - больничные службы и оборудование v13

В предоставленном списке учтены все **{inventory['services']} пункта / 12 направлений**. Адрес каждого пункта указан в requirements.csv. Существующие помещения сохранены, а **{inventory['renamed_rooms']} повторяющееся помещение** получило самостоятельное назначение. Размеры и ID всех 562 зон, 35 локаций, лестниц, лифта, тоннелей и сюжетных мест сохранены.

## Специализация

На верхних этажах появились гематология, онкология, гистология, цитология, банк крови, детские палаты, акушерство и гинекология, ожоговый бокс, нефрология и гемодиализ. Есть социальный работник, длительное лечение и безопасная тихая комната. Хозяйственные помещения различаются: столярная, слесарная и электротехническая мастерские, учебные аудитории, трансформаторная и отопительный узел. Старые функции, которые нужны сюжету, не переименованы.

## Видимые изменения

Добавлено **{inventory['equipment']} полноразмерных экземпляров оборудования / {inventory['equipment_types']} конструкций**: ИВЛ, диализный аппарат, инфузионные стойки, дефибрилляторы, наркозные аппараты, кислородные баллоны, холодильники крови, морозильники образцов и продуктов, детские койки, неонатальная кроватка, гинекологическое кресло, тракционная стойка, брусья и тренировочная лестница. Есть хирургические рукомойники, автоклавы, лабораторные вытяжные шкафы, стиральная и сушильная машины, кухонная плита, верстаки, насос, котёл, трансформатор, посты сестры и педальный тренажёр. Пищевой морозильник стоит в кухонной кладовой B106, отдельно от холодильной машинной морга.

**{inventory['signs']} читаемых табличек** установлены на настоящих стенах со стороны коридора возле дверей. Кириллица отрисована геометрией, поэтому не требует новых внешних текстур или сторонних шрифтов в сборке. Таблички показывают названия из карты. Их {inventory['parts']:,} частей вместе с аппаратами входят в отдельный слой 13.

## Проверка и границы готовности

Независимо проверены размеры, физические границы, отсутствие пересечений, стеновые крепления и сохранность всех прежних предметов. Все запросы оборудования нашли место без уменьшения моделей. NativeWorld: 89/89; сюжет и сохранения: 23/23; начальная отрисовка: 23/23; {len(images)} свежих кадров 1920 x 1080. Проверка оболочек: {sum(envelope['tested'].values()):,} проб, обнаруженных пропусков нет.

Оборудование, интерьеры и мелкие детали пока прототипные; это не новая симуляция приборов и не готовый фотореалистичный арт. Сохранена прежняя универсальная мебель. Кадры не являются полным визуальным обходом каждой комнаты. Все 1711 моделей из каталога v12 по-прежнему не объявляются готовыми.

Запуск: PLAY_UNREAL.cmd. B переключает яркий режим.
'''
(OUT/'REPORT.md').write_text(summary,encoding='utf-8')
print(json.dumps(dict(success=True,equipment=inventory['equipment'],signs=inventory['signs'],views=len(images),map_sha256=mh),ensure_ascii=False))
