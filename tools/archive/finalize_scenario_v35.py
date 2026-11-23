"""Publish v35 evidence only after fresh packaged scenario/world/menu/arrival checks."""
from pathlib import Path
import json,hashlib,datetime,shutil
R=Path(__file__).resolve().parents[1];U=R/'unreal/EchoWard';P=U/'Saved/PackagedWindows';A=R/'artifacts/echo_scenario_v35';A.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
exe=P/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';editor=U/'Binaries/Win64/UnrealEditor-EchoWard.dll';assert exe.exists() and editor.exists()
h=sha(exe);runtime=U/'Content/CampaignData/echo_scenario_v35.json';data=read(runtime);assert runtime.read_bytes()==(P/'Windows/EchoWard/Content/CampaignData/echo_scenario_v35.json').read_bytes()
checks={}
for name,filename in [('world','verified_world.json'),('menu','verified_menu.json'),('arrival','verified_arrival_v29.json')]:
 v=read(P/filename);recorded=v.get('executable_sha256',v.get('build_identity','').split(':')[0]);assert v['success'] and recorded.lower()==h,(name,'stale executable');checks[name]=v
 if name in ('world','menu'):
  result=read(Path(v['test_saved'])/'Prototype'/('book_world_check.json' if name=='world' else 'menu_check.json'));assert result['success'];checks[name]['checks']=result['count']
scenario=U/'Saved/PackageTests/Scenario35Release/Saved/Prototype/scenario35_check.json';v=read(scenario);assert v['success'] and v['count']>=36 and all(q['passed'] for q in v['checks']);checks['scenario']=v;scenario_count=v['count']
assert scenario.stat().st_mtime>exe.stat().st_mtime
spatial=read(scenario.with_name('scenario35_spatial.json'));assert spatial['accessible']==457 and spatial['unresolved']==0
for name in ['scenario35_choices.png','scenario35_dialogue.png']:
 p=scenario.with_name(name);assert p.exists() and p.stat().st_mtime>exe.stat().st_mtime;shutil.copy2(p,A/name)
for fn in ['scenario35_check.json','scenario35_spatial.json','scenario35_roundtrip.json']:shutil.copy2(scenario.with_name(fn),A/fn)
for name,v in checks.items():(A/(name+'_checks.json')).write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
for log in ['scenario35_build_editor.log','scenario35_build_game.log','scenario35_release.log']:
 text=(R/'tmp'/log).read_text(encoding='utf-8',errors='replace');assert 'Result: Succeeded' in text if 'build' in log else 'SCENARIO35_CHECK FAIL' not in text
current={p.name:sha(p) for p in (U/'Content/CampaignData').glob('*.json') if p.name in read(A/'before/package_verified.json')['current_data_hashes'] or p.name=='echo_scenario_v35.json'}
assert current['hospital_world.json']==data['source_map_sha256']=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec'
for name,hashvalue in current.items():assert sha(P/'Windows/EchoWard/Content/CampaignData'/name)==hashvalue,name
pak={p.name:sha(p) for p in (P/'Windows/EchoWard/Content/Paks').iterdir() if p.suffix in ('.pak','.utoc','.ucas')}
report=dict(success=True,revision=35,checked_at=datetime.datetime.now(datetime.timezone.utc).isoformat(),executable_sha256=h,editor_module_sha256=sha(editor),pak_sha256=pak,world_id='hospital_story_shell_v3',map_sha256=current['hospital_world.json'],current_data_hashes=current,story_revision=35,art_integration_revision=33,optimization_revision=33,world_rooms=562,world_locations=35,native_models=869,parked_vehicles=35,transport_objects=36,scenario_chapters=19,scenario_scene_bindings=78,scenario_required_events=76,scenario_documents=103,scenario_alternatives=23,scenario_interactive_choice_scenes=19,scenario_placements=457,scenario_unique_mob_actors=35,scenario_mob_anchors=46,scenario_npc_additions=17,scenario_checks=scenario_count,spatial_accessible=spatial['accessible'],spatial_unresolved=0,world_checks=checks['world'].get('checks'),menu_checks=checks['menu'].get('checks'),arrival_checks=checks['arrival'].get('checks'),build_identity=checks['world'].get('build_identity'),manual_full_playthrough=False,performance_rebenchmarked=False,limitations=data['limits']+['Full physical escort, unique voiced NPCs, facial animation, dragging/cure cinematics and every bespoke boss mechanism are not implemented by the text-state integration.','Source stage directions and 164 furniture annotations are retained as production specifications, not certified physical implementation.','Historical saves have no past branch choices; a new playthrough is recommended for the new narrative.'])
# Avoid borrowing unrelated counts or performance results from older binaries.
(A/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(P/'package_verified.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
intro=f'''# v35 - игровой сценарный слой PDF, 28.10.2026

Проект D:/Projects/ECHO. Пользователь разрешил внедрение полного PDF версии2. Присланные authoring-скрипты не исполнялись; собственный компилятор tools/compile_scenario_v35.py читает выбранные JSON/тексты. SourcePDF SHA {data['source_pdf_sha256']}. Canonical hospital_world SHA неизменён d3aa...873ec, архитектура19; арт и машины33, библиотека34 по-прежнему не импортирована в UE.

Запускаемая игра35: runtime echo_scenario_v35.json, WardNarrative35.cpp/Assessment35/UI35; hooks в HospitalGameplay, Arrival29, CampaignCommands/UI, FieldMenu, HeroPause. 19глав/78старыхID/76обязательныхсобытий;103текстовыхдокумента/23альтернативы/19выборныхсцен/457размещений (+62). 35уникальныхмобов/46опор;17дополнительныхименныхNPC с общей моделью. Вселитературныетексты сохранены; частьпозднихреакций перенесена в нужныеглавы. Кассета поднята на криомодуль, записка над коллизией HERO; настенныевзаимодействияпроверяюткорректнуювысоту.

EXE {h}; EditorDLL {sha(editor)}. Проверки сценария {scenario_count}, физический доступ457/457; World/Menu/Arrival подтверждены свежимиEXEхешами в artifacts/echo_scenario_v35. UI1920x1080 проверен. НоваябенчмаркинговаясессияFPS не проводилась. Source-map geometry неизменна;12runtimeкарантинныхобъёмов в главныхлестничныхузлах освобождаются по главам. Сохранения включаютвнутреннийscenario35:deepcopy,страница,выбор,факты,NPCпозиции; legacyсохраняетпрогресс безавтовыбораконцовки, новыйпроходлучшеначатьотдельнойновойигрой.

ВАЖНО: это пространственный текстовый прототип всей сюжетной цепи, НЕ полностью снятая 3D-постановка PDF. Полная озвучка/лицеваяанимация/эскортNPC/утаскиваниеРудина/медицинскиеанимации/всеуникальныебоссовые механизмы не реализованы. 164сценическиханнотации и бестиарий сохраненыкакспецификация, нельзя выдаватьихза готовуюфизику. Некоторые сочетаниявыборов требуютредакции следующихканоническихреплик. Не говоритьпользователю «всёизPDFготово» безэтогоуточнения. Подробности artifacts/echo_scenario_v35/IMPLEMENTATION.md.

tools/verify_scenario_v35.ps1 собираетEditor/Game, копируетвpackage/stage, запускаетWardScenarioCheckсrender,World,Menu,Arrival. Потом tools/finalize_scenario_v35.py публикуетmarker. WardStoryCheck исторический, не проверяетновыйсценарий. Наобычномстарте scenario35 включён; вWardArrivalCheck/StoryCheck историческиотключён длястаройконтрактнойцепи; новыйAssessor проверяет фактическийcar-exit/full-intro и текущуюUI отдельно. СтарыеreadonlyPDFатласы описываюткарту33,новыйслойсюжета отдельный.

ОдинтяжёлыйUE/Blender за раз. Старый Cкорень сjunction не удалять. Резервныеисходники+EXE33 в artifacts/echo_scenario_v35/before. ТестыиспользуютотдельныйUserDir; пользовательскиесохранения не трогались.

---

'''
handoff=R/'HANDOFF_NEXT_GPT.md';old=handoff.read_text(encoding='utf-8-sig');handoff.write_text(intro+old,encoding='utf-8')
print(json.dumps(dict(success=True,revision=35,executable_sha256=h,scenario_checks=scenario_count,world_checks=report['world_checks'],menu_checks=report['menu_checks'],arrival_checks=report['arrival_checks'],spatial_accessible=457),ensure_ascii=False))
