"""Publish only SHA-matched data, runtime and rendered verification for v12."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard'
PACKAGE=PROJECT/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v12'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source)
exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
assert d['review_revision']==12 and eh==read(ROOT/'artifacts/unreal_hospital_v11/verification.json')['executable_sha256']==sha(PROJECT/'Binaries/Win64/EchoWard.exe')
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v12.json');story=read(PACKAGE/'verified_story_v7.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for result in (visual,story):assert result['success'] and result['executable_sha256'].lower()==eh and result['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==89 and rendered['success'] and rendered['count']==23 and story['checks']==23
assert envelope['success'] and sum(envelope['failed'].values())==0
for name,key in (('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('catalog_acceptance','map_sha256')):
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh,(name,a[key],mh)
images=[]
for c in d['locations']+d['furnishing_captures']:
    p=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    with Image.open(p) as im:assert im.size==(1920,1080)
    shutil.copy2(p,OUT/p.name);images.append(p.name)
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
for prefix,names,columns in [('contact',images,4),('catalog_contact',[x for x in images if x.startswith('world_catalog_')],3)]:
    for start in range(0,len(names),20):
        group=names[start:start+20];canvas=Image.new('RGB',(columns*410,math.ceil(len(group)/columns)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for i,name in enumerate(group):
            with Image.open(OUT/name) as src:im=src.convert('RGB');im.thumbnail((400,225))
            x=i%columns*410;y=i//columns*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name[6:-4],font=font,fill='white')
        canvas.save(OUT/f'{prefix}_{start//20+1}.png')
for status,target,original in ((native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check')):
    shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inventory=read(OUT/'catalog_inventory.json')
report=dict(success=True,review_revision=12,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,
    executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=89,story_checks=23,
    visual_initial_checks=23,world_captures=len(images),envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],catalog_inventory=inventory,screenshots=images,
    implementation='Additional authored catalogue data rendered by the retained Windows Game/Editor.',limits=d['item_types_dressing']['limits'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(PACKAGE/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'REPORT.md').write_text(f'''# ЭХО - предметное наполнение v12

По двум предоставленным спискам добавлено **{inventory['new_items']} экземпляра предметов** в **{inventory['zones']} зонах**. Они состоят из {inventory['new_parts']} простых видимых частей.

В каталог перенесены все **1711 наименований / 50 категорий** с прежними T-ID. Это не 1711 уникальных готовых моделей: новые размещения используют {inventory['geometry_families']} семейств простой геометрии. {inventory['new_types']} наименований имеют новые экземпляры; 159 связаны с прежними семействами мебели. Статусы остальных честно указаны в item_types.csv: крупные аппараты и архитектурные детали требуют следующего этапа, а часть предметов не нашла подходящей свободной опоры.

## Расстановка

Медицинские и лабораторные приборы отделены от продуктов, личных вещей и инструмента. Есть микроскопы, пробирки, расходники, посуда, одежда, документы, настольные игры, средства гигиены и ассортимент магазинов. Диагностические принадлежности дополнительно привязаны к своей специальности. Музыкальные инструменты стоят в музыкальных комнатах, принадлежности протезирования - в ортезных/протезных.

В комнатах занятий появились мольберты и шахматы, в музыкальных - гитары и ксилофоны, в прачечных - швейные машины. Для крупных настольных композиций добавлены отдельные складные столы, чтобы не уменьшать предметы ради заполненных полок. Всего {inventory['new_floor_accents']} новых напольных объектов с физической коллизией, совпадающей с видимой геометрией.

Все старые размеры, корпуса, мебель, свет, материалы, игровые предметы, мобы и правила сохранены. Проходы к дверям, укрытиям и лестницам зарезервированы. Небольшие новые предметы декоративные; новые записки не получили выдуманных сюжетных текстов. Это пока простые модели, а не готовый фотореалистичный арт или новые работающие приборы.

## Проверка

Пройдены независимые проверки карты, мебели, сюжета и нового слоя. Проверены реальные размеры опор, отсутствие пересечений с прежними и новыми предметами, соответствие назначениям помещений. В игре: 89 проверок мира, 23 сюжетных, 23 начальных проверки с отрисовкой и {len(images)} свежих кадров 1920 x 1080. Проверка оболочек: {sum(envelope['tested'].values()):,} проб, обнаруженных пропусков нет. Снимки не являются исчерпывающим визуальным обходом каждого помещения.

Запуск: PLAY_UNREAL.cmd. Клавиша B переключает яркий режим для изучения карты.
''',encoding='utf-8')
print(json.dumps(dict(success=True,items=inventory['new_items'],views=len(images),map_sha256=mh),ensure_ascii=False))
