"""Publish naming revision only after fresh checks of the packaged executable."""
from pathlib import Path
import json,hashlib,datetime,shutil
R=Path(__file__).resolve().parents[1];U=R/'unreal/EchoWard';P=U/'Saved/PackagedWindows';A=R/'artifacts/echo_names_v36';A.mkdir(exist_ok=True)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
exe=P/'Windows/EchoWard/Binaries/Win64/EchoWard.exe';editor=U/'Binaries/Win64/UnrealEditor-EchoWard.dll';h=sha(exe)
current=read(U/'Content/CampaignData/echo_scenario_v35.json');old=read(A/'before/package_verified.json');names=read(A/'naming_data_checks.json');assert names['success']
assert sha(U/'Binaries/Win64/EchoWard.exe')==h
src=U/'Source/EchoWard'
assert editor.stat().st_mtime>max((src/f).stat().st_mtime for f in ['WardNames36.cpp','WardCampaign.cpp','WardCampaignData.h','WardNarrative35.cpp','WardNarrativeAssessment35.cpp','WardHospitalGameplay.cpp'])
checks={}
for name,fn in [('world','verified_world.json'),('menu','verified_menu.json'),('arrival','verified_arrival_v29.json')]:
 v=read(P/fn);assert v['success'] and v.get('executable_sha256',v.get('build_identity','').split(':')[0]).lower()==h,(name,'stale executable')
 checks[name]=v
 if name in ('world','menu'):checks[name]['checks']=read(Path(v['test_saved'])/'Prototype'/('book_world_check.json' if name=='world' else 'menu_check.json'))['count']
 (A/(name+'_checks.json')).write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
f=U/'Saved/PackageTests/Scenario35Release/Saved/Prototype/scenario35_check.json';scenario=read(f);assert scenario['success'] and scenario['count']>=44 and f.stat().st_mtime>exe.stat().st_mtime
for fn in ['scenario35_check.json','scenario35_spatial.json','scenario35_choices.png','scenario35_dialogue.png']:shutil.copy2(f.with_name(fn),A/fn)
spatial=read(f.with_name('scenario35_spatial.json'));assert spatial['accessible']==457 and spatial['unresolved']==0
hashes={fn:sha(U/'Content/CampaignData'/fn) for fn in old['current_data_hashes']}
for fn,value in hashes.items():assert sha(P/'Windows/EchoWard/Content/CampaignData'/fn)==value,fn
report={**old,'revision':36,'names_revision':36,'checked_at':datetime.datetime.now(datetime.timezone.utc).isoformat(),'executable_sha256':h,'editor_module_sha256':sha(editor),'current_data_hashes':hashes,'build_identity':checks['world']['build_identity'],'scenario_checks':scenario['count'],'world_checks':checks['world']['checks'],'menu_checks':checks['menu']['checks'],'arrival_checks':checks['arrival']['checks'],'naming_data_checks':names['count'],'named_people':36,'user_named_people':28,'named_permanent_monsters':35,'performance_rebenchmarked':False,'manual_full_playthrough':False}
assert report['current_data_hashes']['hospital_world.json']==current['source_map_sha256']
for p in [A/'verification.json',P/'package_verified.json']:p.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
intro=f'''# v36 — казахские имена, 28.10.2026

Пользователь заменил 28 персонажей по старой нумерации. Написание ровно пользовательское (включая казахские буквы); не исправлять автоматически. Полный реестр artifacts/echo_names_v36/ИМЕНА_НА_ПРОВЕРКУ.md и РЕЕСТР_ИМЁН.json. Новые имена 29–36 предложены для проверки, не означают новых физических NPC. Для 35 существующих уникальных мобов назначены отдельные казахские клички, альтернативные якоря одного actor_guid используют одну. Паразиты Құрт-01 и далее.

Воспроизводимый профиль tools/scenario_names_v36.py вызывается compile_scenario_v35.py перед записью echo_scenario_v35.json (names_revision36). Прочие story-ID/actor_guid/сохранения/координаты не переименованы, 19глав/78ID/103документа/457размещений сохранены. ИсходныйPDF и canonical hospital_world неизменны. WardNames36.cpp кэширует безопасную замену отображаемого текста (включая старые строки журнала и английское Irina), вызов из Tr после локализации; проверяются границы слов. WardNarrative35.cpp обновляет голоса/реплики/эпилог; Маржан заменяет Никиту, роль санитарки. Некоторые местоимения в длинной постановочной прозе требуют последующей литературной редакции; не обещать полную гендерную переработку. Казахские буквы проверены в NotoSans и Literata.

Запускаемая сборка EXE {h}; EditorDLL {report['editor_module_sha256']}. Свежие проверки: сценарий {scenario['count']}, данныеимён {names['count']}, мир {report['world_checks']}, меню {report['menu_checks']}, вступление {report['arrival_checks']}, доступ457/457. Evidence artifacts/echo_names_v36. UI1080 проверен по текущему диалогу. Полная озвучка/постановка3D не добавлялась, ограничения v35 сохраняются. Пользовательские сохранения не тронуты; compatibility layer только отображение. Старые PDF со старыми именами — исторические, не перезаписаны.

Не выполнять prepare_names_v36.py как обязательный шаг обычного компилятора: это генератор совместимостиC++ с исходными backup(before), его новые правки требуют пересборки. verify_names_v36.py — независимая проверка структуры/имён/шрифтов, verify_scenario_v35.ps1 — полная сборка и игровые регрессии. finalize_names_v36.py проверяет актуальные хеши и публикует marker. Финализатор35 не запускать после36, он вернёт старую revision. Реальный root D:/Projects/ECHO, старый junction C не удалять.

---

'''
p=R/'HANDOFF_NEXT_GPT.md';p.write_text(intro+p.read_text(encoding='utf-8-sig'),encoding='utf-8')
print(json.dumps({k:report[k] for k in ['revision','naming_data_checks','scenario_checks','world_checks','menu_checks','arrival_checks','executable_sha256']},ensure_ascii=False))
