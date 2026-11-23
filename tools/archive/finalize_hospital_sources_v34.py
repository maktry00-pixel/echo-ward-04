from pathlib import Path
from datetime import datetime,timezone
import json,hashlib,struct,html,csv
from PIL import Image,ImageDraw,ImageFont
R=Path('D:/Projects/ECHO');O=R/'assets/models/hospital_v34';A=R/'artifacts/echo_hospital_sources_v34';X=R/'assets/external/hospital_v34';source=json.loads((X/'SOURCE_MANIFEST.json').read_text());reports=[json.loads((A/n).read_text()) for n in ['prepared_glb.json','prepared_cad.json','original_modules.json']];checks=json.loads((A/'geometry_checks.json').read_text());assert checks['success'] and len(source['assets'])==29 and not source['failed'];models=[]
for i,d in enumerate(reports):
 assert d['success']
 for m in d['models']:
  m['category']='Скачанная GLB-модель' if i==0 else 'CAD: геометрия готова, материалы предварительные' if i==1 else 'Собственный модуль ЭХО';models.append(m)
models.sort(key=lambda m:m['id']);assert len(models)==36 and len({m['id'] for m in models})==36
for m in source['assets']:
 b=Path(m['file']).read_bytes();assert len(b)==m['bytes'] and hashlib.sha256(b).hexdigest()==m['sha256']
for m in models:
 for k in ['model','blend','preview']:assert Path(m[k]).is_file(),(m['id'],k)
 b=Path(m['model']).read_bytes();magic,version,length=struct.unpack_from('<4sII',b);assert magic==b'glTF' and version==2 and length==len(b);size=struct.unpack_from('<I',b,12)[0];j=json.loads(b[20:20+size]);assert j.get('meshes') and j.get('materials');assert all(v>0 for v in m['size_m']);m['sha256']=hashlib.sha256(b).hexdigest();m['gltf_meshes']=len(j['meshes']);m['gltf_images']=len(j.get('images',[]));m['gltf_animations']=len(j.get('animations',[]));m['in_game']=False
assert next(m for m in models if m['slug']=='hospital_door')['gltf_animations']>0
manifest=dict(success=True,revision=34,stage='source_library_prepared_not_game_integration',checked_at=datetime.now(timezone.utc).isoformat(),downloaded=29,downloaded_glb=19,downloaded_step=10,original_modules=7,prepared_models=36,models=models,geometry_checks=checks['checks'],source_manifest=str(X/'SOURCE_MANIFEST.json'),texture_manifest=str(X/'TEXTURE_MANIFEST.json'),game_build_remains=33,limits=['CAD models currently use a neutral preview material and need material assignment for the game.','Clean downloaded props need condition and material matching for the horror scene.','Source preparation does not add an object to the packaged game.','The whole 60-item request list is not claimed complete.'])
(O/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2));(A/'verification.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2))
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',18)
for page in range(3):
 batch=models[page*12:(page+1)*12];im=Image.new('RGB',(1600,((len(batch)+3)//4)*320),(16,18,21));dr=ImageDraw.Draw(im)
 for i,m in enumerate(batch):
  f=Image.open(m['preview']).convert('RGB');f.thumbnail((390,280));x=i%4*400;y=i//4*320;im.paste(f,(x,y));dr.text((x+6,y+282),str(m['id'])+' '+m['title'][:34],font=font,fill='white')
 im.save(A/('catalog_'+str(page+1)+'.jpg'),quality=94)
selected=['manual-wheelchair-e5061094','patient-bedside-cabinet-19c0b99c','wall_door_opening','hospital_door'];im=Image.new('RGB',(1200,600),(15,17,20))
for i,slug in enumerate(selected):
 m=next(m for m in models if m['slug']==slug);f=Image.open(m['preview']).convert('RGB');f.thumbnail((300,560));im.paste(f,(i*300,0))
im.save(A/'preview.jpg',quality=94)
lines=['# ЭХО — подготовленные модели v34','','**29 моделей скачаны, 7 модулей созданы. Всего 36 подготовленных ассетов.** Это библиотека исходников и редактируемых моделей. В игровую сборку v33 они пока не внесены; готовность всех 60 пунктов не заявляется.','','19 источников получены в GLB; ещё 10 — STEP CAD, преобразованы Gmsh/OpenCASCADE в меши. У CAD-копий пока нейтральный материал для просмотра: краску, стекло и фактуры нужно назначать перед расстановкой. Чистую больничную мебель предстоит согласовать по износу с хоррор-сценой.','','Собственные модули: прямая стена, угол, стена с дверным проёмом, стена с оконным проёмом, плиточный пол, потолок с отсутствующими секциями и дверь. У двери есть отдельный узел петли и анимация 0–70°. Лучами в Blender проверены проёмы и освобождение прохода после открытия.','','Каждая папка содержит model.glb, model.blend, preview.png и asset.json. Метрический масштаб проверен. SHA256 всех 29 скачанных исходников совпадает с опубликованным издателем. Цветные фактуры штукатурки — 4K; normal/roughness — 2K. Для остальных моделей 4K не обещается: размеры карт определяются источником.','','## Каталог','','| ID | Модель | Подготовка |','|---|---|---|']
for m in models:lines.append(f"| {m['id']} | [{m['title']}]({m['model'].replace(chr(92),'/')}) | {m['category']} |")
lines+=['','## Источники','','Скачанные модели: [Innerscene Free Parts Library](https://www.innerscene.com/blog/free-parts-library), индивидуальные страницы и CC0-1.0 сохранены в SOURCE_MANIFEST.json и provenance.json. Исходная геометрия этих моделей принадлежит указанному издателю, не создавалась для ЭХО.','','Фактуры: [Plastered Wall 02](https://polyhaven.com/a/plastered_wall_02), Charlotte Baglioni; [Painted Plaster Wall](https://polyhaven.com/a/painted_plaster_wall), Amal Kumar. [Лицензия Poly Haven](https://polyhaven.com/license) — CC0. Серые и синевато-серые варианты штукатурки получены из разрешённой фактуры; не являются фотосканами нашей больницы.','','Платные модели не покупались, новые аккаунты не создавались. Существующие игровые ассеты и сохранения не заменялись.']
(O/'README.md').write_text('\n'.join(lines),encoding='utf-8');(A/'REPORT.md').write_text('\n'.join(lines),encoding='utf-8')
rows=[]
for m in models:
 folder=Path(m['model']).parent;relative=folder.relative_to(O).as_posix();rows.append(f'<article><img loading="lazy" src="{relative}/preview.png"><h3>{m["id"]}: {html.escape(m["title"])}</h3><p>{html.escape(m["category"])}</p><a href="{relative}/model.glb">GLB</a> · <a href="{relative}/model.blend">Blender</a></article>')
page='<!doctype html><html lang="ru"><meta charset="utf-8"><title>ЭХО — модели v34</title><style>body{background:#11161a;color:#e0e7e9;font:16px Arial;margin:28px}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:18px}article{background:#1b2329;padding:12px;border:1px solid #41505a;border-radius:8px}img{width:100%;aspect-ratio:1;object-fit:contain}h3{font-size:17px}p{color:#aebbc3}a{color:#8ed4e1}</style><h1>ЭХО — подготовленные модели v34</h1><p>29 скачанных источников + 7 собственных модулей. Пока не внесены в игровую сборку. У CAD-моделей материалы предварительные.</p><main>'+''.join(rows)+'</main></html>'
(O/'index.html').write_text(page,encoding='utf-8')
status='''# v34 — подготовка библиотеки моделей, игра остаётся v33

Пользователь разрешил самому скачивать/создавать модели вместо запроса всех файлов у него. Подготовлены 29 CC0-источников Innerscene:19GLB+10STEP; SHA256+размер сверены с издателем. Источники assets/external/hospital_v34 (source_page.html/provenance/SOURCE_MANIFEST); 19GLB нормализованы, 10STEP тесселированы Gmsh4.15.2/OpenCASCADE и сохранены в GLB/BLEND. CAD копии с нейтральным материалом, требуют художественного прохода перед интеграцией. 7 собственных модулей с разрешёнными PH-фактурами: 3340–3346. Материалы стен 4Kdiff/2Knor/rough. Дверь действительно открывается70°, проёмы проверены лучами, geometry_checks4/4. Не все60 пунктов закрыты. Нельзя выдавать подготовленные файлы за внедрённые в игру или фотореалистичный финальный арт.

Готовые к следующему этапу исходники assets/models/hospital_v34/{slug}/{model.glb,model.blend,asset.json,preview.png}; LIBRARY_MANIFEST.json, README.md, index.html. Артефакты artifacts/echo_hospital_sources_v34/{verification.json,REPORT.md,catalog_1..3.jpg,preview.jpg}. Всего36 файлов моделей в GLB/BLEND. Исходники автора сохранены, свой стиль — серый верх/выцветший сине-серый низ, без сплошной зелёной плесени. Скрипты tools/{fetch_hospital_sources_v34,convert_hospital_cad_v34,prepare_hospital_sources_v34,prepare_hospital_cad_v34,create_hospital_modules_v34,verify_hospital_sources_v34,finalize_hospital_sources_v34}.py.

Нативный runtime, пакет, source C++ и сохранения не менялись: действующая игровая сборка v33,869native,EXE b313c0cc7a647e2f477064dad34dd3a69225964e5ce35948b75dfdc2117bfdc8. Следующий шаг — выбор прошедших визуальную проверку моделей, материалы старой больницы, UE-импорт в отдельный namespace, замены с проверкой габаритов/взаимодействий, затем пакет и проверки. Не ломать старые count/assert только ради нового ассета.

HTTP403 на поисковом queryURL обойдён штатным публичным индексом blog/free-parts-library (не авторизацией). HTTP429 устранён последовательной загрузкой с паузами6с/35с. Доступа к аккаунтам не требовалось. Gmsh dev tool в .tools/python_gmsh; DLL из официального wheel с SHA256проверкой. В игру библиотека Gmsh не включается. Один тяжёлый Blender/UE за раз, явный workdirD, старыйCкорень с junction не удалять.

---

'''
p=R/'HANDOFF_NEXT_GPT.md';old=p.read_text(encoding='utf-8-sig');p.write_text(status+old,encoding='utf-8');(A/'STATE_PREPARED.md').write_text(status,encoding='utf-8');print('HOSPITAL_SOURCE_LIBRARY_PREPARED',len(models),'sources',29,'original',7,'game remains33')