"""Publish matching native horror presentation evidence; marker is written last."""
from pathlib import Path
import datetime,hashlib,json,shutil
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_horror_v28';O.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
old=read(O/'before_identity.json');data=read(P/'Content/CampaignData/echo_horror_v28.json');audit=read(O/'acceptance.json');assert audit['success'];exe=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');assert exe==sha(P/'Binaries/Win64/EchoWard.exe')
for n,h in old['unchanged_data_hashes'].items():assert sha(P/'Content/CampaignData'/n)==h,n
for n in list(old['unchanged_data_hashes'])+['echo_detail_v26.json','echo_detail_v27.json','echo_horror_v28.json']:assert sha(P/'Content/CampaignData'/n)==sha(PK/'Windows/EchoWard/Content/CampaignData'/n),n
states={n:read(PK/n) for n in ['verified_world.json','verified_worldvisual_v28.json','verified_storyvisual_v7.json','verified_native_menu_v23.json']}
for n,s in states.items():assert s['success'] and s.get('executable_sha256',s.get('build_identity','').split(':')[0]).lower()==exe,n
world=states['verified_world.json'];vis=states['verified_worldvisual_v28.json'];story=states['verified_storyvisual_v7.json'];menu=states['verified_native_menu_v23.json'];w=read(Path(world['test_saved'])/'Prototype/book_world_check.json');v=read(Path(vis['test_saved'])/'Prototype/book_world_check.json');doors=read(Path(world['test_saved'])/'Prototype/every_door_check.json');env=read(Path(world['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and w['count']==140 and v['success'] and v['count']==66 and story['checks']==25 and menu['checks']==57
assert w['rooms']==562 and w['locations']==35 and w['custom_bounds_failures']==w['unresolved_custom_props']==0
assert w['horror_traces28']==len(data['traces'])==910 and w['horror_markers28']==len(data['emergency_markers'])==6
assert w['detail27_props']==23918 and w['detail_props']==6374 and w['real_props']==16107 and w['quality_props']==28850
assert w['custom_furniture']==old['active_custom_instances']+w['horror_markers28'] and doors['success'] and doors['doors']==1012 and env['success']
perf=read(P/'Saved/PackageTests/PerformanceV25_v28/Saved/Prototype/hospital_perf_v28.json');assert perf['success'] and len(perf['views'])==8 and perf['horror_traces28']==910 and perf['executable_sha256']==exe and perf['screen_percentage']==100 and perf['horror28_data_sha256']==sha(P/'Content/CampaignData/echo_horror_v28.json')
prev=read(R/'artifacts/echo_detail_v27/performance.json');comparison=[]
for before,after in zip(prev['views'][:4],perf['views'][:4]):
 assert before['view']==after['view'];comparison.append(dict(view=after['view'],before_fps=before['fps'],after_fps=after['fps'],p95_ms=after['p95_ms']))
dark=perf['views'][5:];assert len(dark)==3
captures=O/'captures';captures.mkdir(exist_ok=True)
for n in vis['screenshots']:shutil.copy2(Path(vis['test_saved'])/'Prototype/BookWorldCaptures'/n,captures/n)
menus=O/'menu';menus.mkdir(exist_ok=True)
for f in (Path(menu['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):shutil.copy2(f,menus/f.name)
shutil.copy2(Path(story['test_saved'])/'Prototype/story_v7.png',O/'story_mob.png')
for state,target,source in [(world,'native_world_check','book_world_check'),(vis,'rendered_world_check','book_world_check'),(story,'native_story_check','story_check'),(menu,'native_menu_check','menu_check'),(world,'every_door_check','every_door_check'),(world,'native_envelope_check','envelope_check')]:shutil.copy2(Path(state['test_saved'])/f'Prototype/{source}.json',O/f'{target}.json')
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),17)
def contacts(files,prefix):
 files=sorted(files)
 for start in range(0,len(files),20):
  page=Image.new('RGB',(1600,((min(20,len(files)-start)+3)//4)*250),'#10191b');draw=ImageDraw.Draw(page)
  for j,path in enumerate(files[start:start+20]):
   with Image.open(path) as im:assert im.size==(1920,1080);pic=im.convert('RGB').resize((400,225))
   x=j%4*400;y=j//4*250;page.paste(pic,(x,y));draw.text((x+4,y+227),path.stem[:44],font=font,fill='white')
  page.save(O/f'{prefix}_{start//20+1}.jpg',quality=93)
contacts(captures.glob('*.png'),'world_contact');contacts(captures.glob('world_v28_*.png'),'horror_contact');contacts(menus.glob('*.png'),'menu_contact')
names=['world_v19_horror_1F.png','world_v23_lab_clutter.png'];sheet=Image.new('RGB',(1600,len(names)*475),'#10191b');draw=ImageDraw.Draw(sheet)
for i,n in enumerate(names):
 for j,path in enumerate([R/'artifacts/echo_detail_v27/captures'/n,captures/n]):
  with Image.open(path) as im:pic=im.convert('RGB').resize((800,450))
  sheet.paste(pic,(j*800,i*475));draw.text((j*800+10,i*475+452),('v27' if j==0 else 'v28')+' / '+n,font=font,fill='white')
sheet.save(O/'before_after.jpg',quality=94)
preview=Image.new('RGB',(1920,1150),'#10191b');draw=ImageDraw.Draw(preview)
for i,(n,label) in enumerate([('world_v19_horror_1F.png','Коридор: свет и тёмные участки'),('world_v28_v23_lab_clutter_horror.png','Лаборатория: красные отблески и потёртости'),('world_v28_v27_replacement_200_horror.png','Палата: оборудование в холодном свете'),('world_v28_v23_court_S01_horror.png','Территория больницы ночью')]):
 with Image.open(captures/n) as im:pic=im.convert('RGB').resize((960,540))
 x=i%2*960;y=i//2*575;preview.paste(pic,(x,y));draw.text((x+10,y+545),label,font=font,fill='white')
preview.save(O/'preview.jpg',quality=94)
for suffix in ['json','csv']:shutil.copy2(P/f'Saved/PackageTests/PerformanceV25_v28/Saved/Prototype/hospital_perf_v28.{suffix}',O/f'performance.{suffix}')
shutil.copy2(P/'Saved/Prototype/horror_v28_materials.json',O/'material_preparation.json')
report=dict(old,success=True,art_integration_revision=28,horror_revision=28,checked_at=datetime.datetime.now().astimezone().isoformat(),build_identity=world['build_identity'],executable_sha256=exe,world_checks=140,visual_checks=66,story_checks=25,menu_checks=57,horror_counts=data['counts'],horror_data_sha256=sha(P/'Content/CampaignData/echo_horror_v28.json'),active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],world_captures=len(vis['screenshots']),menu_captures=len(list(menus.glob('*.png'))),comparison_bright_views=comparison,measured_horror_views=dark,performance_scope='Single short 1080p/100% run, idle AI; three actual horror scenes and four comparable inspection views. One wall-facing diagnostic view excluded. No combat/long duration measurement.')
report['performance']=[v for v in perf['views'] if v['view']!='corridor_horror']
runtime=read(P/'Content/CampaignData/echo_custom_runtime.json')
report['imported_native_meshes']=report['native_models']=len(runtime['meshes'])
assert report['native_models']==786
report['all_instances']=w['instances'];assert report['all_instances']==761983
report['current_data_hashes']={n.name:sha(n) for n in (P/'Content/CampaignData').glob('*.json')}
for n in ['echo_custom_runtime.json','echo_custom_bindings.json','echo_quality_v22.json','echo_real_v23.json','echo_detail_v26.json','echo_detail_v27.json','echo_horror_v28.json']:report[n]=report['current_data_hashes'][n]
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
lib=R/'assets/models/echo_custom';integration=read(lib/'GAME_INTEGRATION.json');integration.update(revision=28,horror_revision=28,horror_traces=910,horror_emergency_markers=6,active_custom_instances=w['custom_furniture'],active_model_types=w['custom_model_types'],verification='artifacts/echo_horror_v28/verification.json');(lib/'GAME_INTEGRATION.json').write_text(json.dumps(integration,ensure_ascii=False,indent=2),encoding='utf8');manifest=read(lib/'LIBRARY_MANIFEST.json');manifest['game_integration']=integration;(lib/'LIBRARY_MANIFEST.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf8')
lines=['# ЭХО — хоррор-оформление карты v28','','Обновлены материалы штукатурки, старой серо-синей краски, плитки и фасадов: сколы, потемнения, влажность и различия плит. Цветные 4K-текстуры и нормали/шероховатость 2K из прежних лицензированных источников сохранены; новые внешние ресурсы не скачивались.','','Добавлены 910 локальных неколлизионных следов: 518 потёртостей/царапин, 166 влажных пятен, 212 потёков, 11 пятен крови и 3 следа руки. Появились шесть дополнительных аварийных указателей в существующей архитектуре. Все предыдущие предметы, размеры 562 зон и маршруты 35 локаций сохранены.','','Холодные локальные источники теперь создают отдельные освещённые участки; красные аварийные отблески сильнее, тёплый свет снаружи приглушён. Небольшой рассеянный источник рядом с игроком помогает видеть ближайшие детали в тени. Снижено затемнение краёв, сохраняются управляемые зерно, туман и редкие сбои ламп. Яркий осмотр по B и фонарь по N работают отдельно. Звуковые события и громкость комнатных профилей сохранены.','','Проверки итогового пакета: мир 140/140, отрисовка 66/66, сюжет/бой/сохранения 25/25, меню 57/57; все 1012 дверей, габариты и отсутствующие модели — без ошибок. Игра и редактор собраны.','','| Сцена хоррор-режима | FPS | p95, мс | RAM, МБ |','|---|---:|---:|---:|']
for p in dark:lines.append(f'| {p["view"]} | {p["fps"]:.1f} | {p["p95_ms"]:.1f} | {p["process_physical_mb"]:.0f} |')
lines+=['','Короткие неподвижные замеры 1920×1080, 100% масштаба, без боевого ИИ; не измерение всего прохождения. Сопоставимые прежние ракурсы и полные сырые данные — performance.json/csv. Дополнительные материалы и свет имеют стоимость по FPS; результаты разных запусков зависят также от состояния ноутбука.','','Кадры настоящие. Ряд старых и новых обзорных камер частично закрыт стенами/мебелью; выбранные крупные планы и общий вид не заменяют ручной осмотр всех комнат. Старые упрощённые модели вне v27-замен остаются в игре. Подробнее — visual_review.json.','','Запуск: D:/Projects/ECHO/PLAY_UNREAL.cmd.']
(O/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf8');marker=PK/'package_verified.tmp';marker.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');marker.replace(PK/'package_verified.json');print(json.dumps(dict(success=True,exe=exe,traces=910,horror_fps=[p['fps'] for p in dark],world_views=len(vis['screenshots']))))
