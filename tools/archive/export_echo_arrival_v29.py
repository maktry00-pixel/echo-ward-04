"""Publish evidence for the corrected story opening, without reusing stale build results."""
from pathlib import Path
import json,hashlib,datetime,shutil,difflib
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';PK=P/'Saved/PackagedWindows';O=R/'artifacts/echo_arrival_v29'
read=lambda f:json.loads(f.read_text(encoding='utf-8-sig'))
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
exe=sha(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe');editor=P/'Binaries/Win64/UnrealEditor-EchoWard.dll';assert editor.exists();assert editor.stat().st_mtime>=max(f.stat().st_mtime for f in (P/'Source/EchoWard').glob('*') if f.suffix in ['.cpp','.h','.cs']);old=read(O/'before_identity.json');audit=read(O/'acceptance.json');assert audit['success'];assert exe==sha(P/'Binaries/Win64/EchoWard.exe')
states={k:read(PK/n) for k,n in [('world','verified_world.json'),('story','verified_storyvisual_v7.json'),('menu','verified_native_menu_v23.json'),('arrival','verified_arrival_v29.json')]}
for k,s in states.items():assert s['success'] and s.get('executable_sha256',s.get('build_identity','').split(':')[0]).lower()==exe,k
w=read(Path(states['world']['test_saved'])/'Prototype/book_world_check.json');a=read(Path(states['arrival']['test_saved'])/'Prototype/arrival29_check.json');doors=read(Path(states['world']['test_saved'])/'Prototype/every_door_check.json');env=read(Path(states['world']['test_saved'])/'Prototype/envelope_check.json')
assert w['success'] and w['count']==140 and w['rooms']==562 and w['locations']==35 and w['instances']==761983
assert a['success'] and a['count']==26 and states['story']['checks']==25 and states['menu']['checks']==57
assert doors['success'] and doors['doors']==1012 and env['success'] and w['custom_bounds_failures']==w['unresolved_custom_props']==0
current={f.name:sha(f) for f in (P/'Content/CampaignData').glob('*.json')}
for n,h in old['current_data_hashes'].items():assert current[n]==h,n
for n,h in current.items():assert sha(PK/'Windows/EchoWard/Content/CampaignData'/n)==h,n
assert current['echo_arrival_v29.json']==states['arrival']['arrival_data_sha256'].lower()==audit['data_sha256']
for k in ['arrival','menu']:
 log=(Path(states[k]['test_saved'])/'package_test.log').read_text(encoding='utf8',errors='replace')
 assert not any(t in log for t in ['Default Material will be used','Failed to compile Material','GPU Crashed','Out of video memory'])
for k,name in [('world','book_world_check'),('world','every_door_check'),('world','envelope_check'),('story','story_check'),('menu','menu_check'),('arrival','arrival29_check')]:shutil.copy2(Path(states[k]['test_saved'])/f'Prototype/{name}.json',O/f'native_{name}.json')
c=O/'captures';c.mkdir(exist_ok=True)
for n in a['screenshots']:
 src=Path(states['arrival']['test_saved'])/'Prototype/ArrivalCaptures'/n
 assert src.stat().st_mtime>=(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe').stat().st_mtime
 with Image.open(src) as im:assert im.size==(1920,1080);im.verify()
 shutil.copy2(src,c/n)
m=O/'menu';m.mkdir(exist_ok=True)
for f in (Path(states['menu']['test_saved'])/'Prototype/MenuCaptures').glob('*.png'):
 assert f.stat().st_mtime>=(PK/'Windows/EchoWard/Binaries/Win64/EchoWard.exe').stat().st_mtime
 shutil.copy2(f,m/f.name)
shutil.copy2(Path(states['story']['test_saved'])/'Prototype/story_v7.png',O/'story_mob.png')
font=ImageFont.truetype(str(P/'Content/CampaignData/Fonts/NotoSans.ttf'),19)
def sheet(files,name,cols=3):
 files=list(files);out=Image.new('RGB',(cols*640,((len(files)+cols-1)//cols)*390),'#0c1416');draw=ImageDraw.Draw(out)
 for i,f in enumerate(files):
  with Image.open(f) as src:pic=src.convert('RGB').resize((640,360))
  x=i%cols*640;y=i//cols*390;out.paste(pic,(x,y));draw.text((x+8,y+363),{'01_drive':'Приезд к больнице','03_checkpoint_record':'КПП: уведомление о давней аварии','05_doctor_task':'Продолжение первой главы'}.get(f.stem,f.stem),font=font,fill='white')
 out.save(O/name,quality=94)
sheet(sorted(c.glob('*.png')),'contact.jpg');sheet(sorted(m.glob('*.png')),'menu_contact.jpg',4)
sheet([c/'01_drive.png',c/'03_checkpoint_record.png',c/'05_doctor_task.png'],'preview.jpg')
patch=[]
for f in O.glob('*.before'):
 now=P/'Source/EchoWard'/f.name.removesuffix('.before')
 if now.exists():patch+=list(difflib.unified_diff(f.read_text(encoding='utf-8-sig').splitlines(True),now.read_text(encoding='utf-8-sig').splitlines(True),fromfile='v28/'+now.name,tofile='v29/'+now.name))
(O/'source_changes.patch').write_text(''.join(patch),encoding='utf8')
report={k:old[k] for k in ['world_id','map_sha256','art_integration_revision','horror_revision','detail_revision','optimization_revision','native_models','horror_data_sha256','detail27_data_sha256','active_custom_instances','active_model_types']}
report.update(success=True,story_revision=29,editor_module_sha256=sha(editor),checked_at=datetime.datetime.now().astimezone().isoformat(),executable_sha256=exe,build_identity=states['world']['build_identity'],world_checks=140,world_rooms=562,world_locations=35,story_checks=25,menu_checks=57,arrival_checks=26,arrival_captures=len(a['screenshots']),menu_captures=len(list(m.glob('*.png'))),doors=1012,bounds_failures=0,unresolved_custom_props=0,book_world_instances=w['instances'],current_data_hashes=current,arrival_data_sha256=current['echo_arrival_v29.json'],canon='Arrival by car -> checkpoint -> evidence of a laboratory accident long before arrival -> reception -> original story. No new laboratory accident.',previous_world_visual_review={'revision':28,'path':'artifacts/echo_horror_v28/visual_review.json','note':'Historical 156-frame review; architectural data unchanged. Current v29 render checks cover opening and menus; no new whole-map capture or performance benchmark.'})
(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
lines=['# ЭХО — исправленное начало v29','','**Авария в лаборатории произошла давно, до приезда героя.** На КПП он узнаёт о ней из старого уведомления. Новая авария при его появлении не запускается. Дата и причина не придуманы. Давняя авария самого героя остаётся поздним раскрытием сценария.','','В новой игре: восьмисекундный подъезд на машине → F завершает подъезд → F выводит из машины → терминал у шлагбаума справа → запись в журнал и открытие шлагбаума → пешком через тамбур N02 в приёмную → защитная система и сообщение врача. Затем продолжается первая глава. Открытых нападений монстров в первой главе нет.','','Новые сохранения запоминают этап вступления и запись о давней аварии. Старые сохранения продолжаются с прежней позиции; приезд не воспроизводится. Яркий осмотр B и фонарь N сохранены. Начало новой игры запускает мрачный режим.','','Проверки этой сборки: вступление26/26 с7кадрами1080p; мир140/140,все1012дверей/габариты; сюжет и сохранения25/25; меню57/57 и17кадров. Editor и Game собраны. Предыдущая карта562зоны/35локаций и данные оформленияv28 побайтово сохранены.','','Ограничения: это рабочее игровое вступление из существующих моделей. Нет отдельной анимации посадки/выхода водителя, озвучки новых реплик актёрами и полной постановки поздних глав. Вводные реплики подаются текстом, со звуками телефона, двери и связи. Дальнейшие события первой главы и поздние сюжетные сцены всё ещё частично представлены текстовыми/пространственными заготовками. Старые модели персонажей/NPC и деталей местами упрощены. Полный обход каждой комнаты и новый замер FPS не проводились.','','В процессе визуальной проверки уведомление вынесено из-за стенки будки на наружную сторону; стандартный 3D-шрифт с квадратами заменён на русскую табличку с Noto через world-space Slate widget.','','Запуск: D:/Projects/ECHO/PLAY_UNREAL.cmd → Игра → Новая игра. Управление: F — взаимодействие, J — журнал. Источник уточнённого начала: data/hospital_furnishing/arrival_v29_canon.md.']
(O/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf8')
marker=PK/'package_verified.tmp';marker.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');marker.replace(PK/'package_verified.json')
print(json.dumps(dict(success=True,exe=exe,arrival_checks=26,captures=len(a['screenshots']),canon='old_accident_before_arrival')))
