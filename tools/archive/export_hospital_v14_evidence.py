"""Export only a matching map, executable and fresh native/rendered evidence."""
import hashlib,json,math,shutil
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];PROJECT=ROOT/'unreal/EchoWard';PACKAGE=PROJECT/'Saved/PackagedWindows';OUT=ROOT/'artifacts/unreal_hospital_v14'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
source=PROJECT/'Content/CampaignData/hospital_world.json';d=read(source);mh=sha(source)
exe=PACKAGE/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';eh=sha(exe)
assert d['review_revision']==14 and eh==sha(PROJECT/'Binaries/Win64/EchoWard.exe')==read(ROOT/'artifacts/unreal_hospital_v13/verification.json')['executable_sha256']
assert source.read_bytes()==(PACKAGE/'Windows/EchoWard/Content/CampaignData/hospital_world.json').read_bytes()
native=read(PACKAGE/'verified_world.json');visual=read(PACKAGE/'verified_worldvisual_v14.json');story=read(PACKAGE/'verified_story_v7.json')
assert native['success'] and native['build_identity'].split(':')[0].lower()==eh and native['build_identity'].split(':')[-1].lower()==mh
for r in (visual,story):assert r['success'] and r['executable_sha256'].lower()==eh and r['map_sha256'].lower()==mh
world=read(Path(native['test_saved'])/'Prototype/book_world_check.json');rendered=read(Path(visual['test_saved'])/'Prototype/book_world_check.json');envelope=read(Path(native['test_saved'])/'Prototype/envelope_check.json')
assert world['success'] and world['count']==89 and rendered['success'] and rendered['count']==23 and story['checks']==23 and envelope['success'] and sum(envelope['failed'].values())==0
for name,key in [('acceptance','source_sha256'),('furnishing_acceptance','map_sha256'),('story_acceptance','map_sha256'),('campus_acceptance','map_sha256')]:
    a=read(OUT/f'{name}.json');assert a['success'] and a[key]==mh,(name,a.get(key),mh)
images=[]
for c in d['locations']+d['furnishing_captures']:
    p=Path(visual['test_saved'])/'Prototype/BookWorldCaptures'/('world_'+c['id']+'.png')
    with Image.open(p) as im:assert im.size==(1920,1080)
    shutil.copy2(p,OUT/p.name);images.append(p.name)
font=ImageFont.truetype(str(PROJECT/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
for prefix,names,cols in [('contact',images,4),('campus_contact',[x for x in images if x.startswith('world_campus_')],4)]:
    for start in range(0,len(names),20):
        group=names[start:start+20];canvas=Image.new('RGB',(cols*410,math.ceil(len(group)/cols)*254),'#10191b');draw=ImageDraw.Draw(canvas)
        for i,name in enumerate(group):
            with Image.open(OUT/name) as src:im=src.convert('RGB');im.thumbnail((400,225))
            x=i%cols*410;y=i//cols*254;canvas.paste(im,(x,y+25));draw.text((x+4,y+2),name[6:-4],font=font,fill='white')
        canvas.save(OUT/f'{prefix}_{start//20+1}.png')
preview=Image.new('RGB',(1920,1150),'#10191b');pd=ImageDraw.Draw(preview)
for j,(name,label) in enumerate([('perimeter','КПП и внутренний периметр'),('RH17','Реабилитация'),('hall_DG25','Транспортный узел диагностики'),('SV03','Хозяйственный корпус')]):
    with Image.open(OUT/('world_campus_'+name+'.png')) as src:im=src.convert('RGB').resize((960,540))
    x=j%2*960;y=j//2*575;preview.paste(im,(x,y));pd.text((x+15,y+547),label,font=font,fill='#cee3df')
preview.save(OUT/'preview.jpg',quality=92)
for status,target,original in [(native,'native_world_check','book_world_check'),(native,'native_envelope_check','envelope_check'),(story,'native_story_check','story_check'),(visual,'rendered_world_check','book_world_check')]:shutil.copy2(Path(status['test_saved'])/f'Prototype/{original}.json',OUT/f'{target}.json')
shutil.copy2(source,OUT/'hospital_world_source.json');inv=read(OUT/'campus_inventory.json')
report=dict(success=True,review_revision=14,checked_at=visual['checked_at'],build_identity=native['build_identity'],executable_sha256=eh,map_sha256=mh,executable=str(exe),executable_timestamp=native['executable_timestamp'],world_id=d['map_id'],world_rooms=562,world_locations=35,world_checks=89,story_checks=23,visual_initial_checks=23,world_captures=len(images),envelope_probes=envelope['tested'],envelope_failures=envelope['failed'],campus_inventory=inv,screenshots=images,limits=d['campus_plan']['limits']+['Inspection captures cover retained views and new selected features, not a manual tour of every room.'])
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(PACKAGE/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
text=f'''# ЭХО — больничный комплекс v14

В игре сохранены 562 зоны, 35 локаций, все реальные размеры, 37 лестничных узлов, лифт, крыша, подвалы, старое оборудование и сюжетные места. Последний пользовательский список совпадает с предыдущим: все 123 требования сохраняют адреса. Добавлен отдельный слой территории и специализации: **{inv['items']} объектов / {inv['parts']} частей**, из них **{inv['new_floor_kits']} полноразмерных комплектов в {inv['new_zones']} зонах**.

## Территория и связи

Клиническая территория X=−178…278м, Y=−50…366м ограждена физической стеной высотой 2,9м. Шесть проходов/проездов оставлены открытыми для текущего свободного осмотра; отдельный проход связывает торговую галерею и парковку без длинного обхода стены. У главного и хозяйственного въездов стоят два новых открытых КПП с рабочим местом охраны, оружейными стойками и ящиками. На стене два поста с крышей и лестницами: ступени 15см, проступь45см, ширина1,2м. Оружие на постах декоративное; сюжетное оружие и баланс не изменены. Супермаркет, аптека, продуктовый магазин и кафе находятся целиком снаружи клинического периметра.

Сохранены шесть настоящих крытых галерей к корпусам и все прежние подземные связи с изломами. Территория разделена на семь участков: торговая зона, приём/скорая, два лечебных дворика, двор персонала, хозяйственный двор и южная служебная зона. Добавлены раздельные группы скамеек, клумб, велопарковок, мусорных контейнеров, машин, бордюров и навес. Дороги и проверенные пешеходные маршруты не заставлены.

## Корпуса и большие залы

Психоневрология: сенсорные панели, терапевтические скамьи и тележки занятий. Реабилитация: велотренажёры, брусья, балансировочные элементы и учебная кухня. Персонал: бытовые кухни, личные шкафчики, доски объявлений, прачечные и мастерские. Хозяйственный блок: компрессоры, насосные рамы, воздуховоды, верстаки и сортировка белья. Цветные полосы помогают отличать направления на этажах.

DG25 и DG26 сохраняют каждый1216м². DG25 — транспортный узел диагностики: диспетчерская секция, каталки, электротягач, зарядка и санитарная подготовка. DG26 — склад списанной техники: старые сканеры, клетки деталей, паллетные стеллажи и ремонтные места. В каждом12 новых островков оборудования; средний проход и старые маршруты сохранены.

## Размеры и укрытия

Все новые комнатные комплекты расставлены по физическим границам без уменьшения; перед каждым свободный подход0,8×0,8м. ВR01 прежний полноразмерный терминал переставлен и повёрнут: перед экраном теперь есть отдельное свободное место0,8×0,8м, двери и входы не заставлены. ЕгоID и модель сохранены; остальная10977 единица мебели не сдвигалась. Повторно проверены двери, капсула персонажа и подходы ко всем внутренним отсекам. Необязательные новые комплекты, которые не поместились, перечислены вcampus_inventory.json; исходное наполнение этих комнат сохранено.

Укрытия сохраняют реальное различие: ниша разрывает видимость и отсекает крупные капсулы; шкаф и место под койкой управляютсяF; убежище закрывается внутренним засовом. Слушатель и Слепой хирург ориентируются на звук, а Ищейка может идти по крови при здоровье<65. Мебель не стирает память погони и не даёт полной неуязвимости. Новых универсальных иммунитетов не добавлено.

## Проверено

Независимые проверки сохранности, соответствия мебели и навигации пройдены. В настоящей сборке:89/89 проверок мира,23/23 сюжета/сохранений,23/23 начальных проверок отрисовки и{len(images)} свежих кадров1920×1080. Проверка оболочек: {sum(envelope['tested'].values())} проб,0 пропусков. Кадры — выборочный осмотр, не ручной обход всех562 зон.

Это полноценное дополнительное наполнение прототипа; окончательные модели, текстуры и постановка сцен остаются этапом художественной доводки. Интернет использован для планировочных примеров, ссылки —SOURCES.md. Новых внешних моделей в этой версии нет.

Служебные ракурсы PS20, RH03, SV10 и DG25_overview смотрят в близкую стену; они не подтверждают видимость оборудования. Другие проверенные ракурсы и отдельные испытания ходьбы сохранены.

Запуск: PLAY_UNREAL.cmd. B — яркий осмотр; M — карта.
'''
(OUT/'REPORT.md').write_text(text,encoding='utf-8')
print(json.dumps(dict(success=True,items=inv['items'],views=len(images),map_sha256=mh),ensure_ascii=False))
