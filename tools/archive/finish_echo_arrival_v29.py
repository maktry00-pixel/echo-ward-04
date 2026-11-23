"""Finish readable docs only after successful export of the matching build."""
from pathlib import Path
import json,datetime,shutil
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_arrival_v29'
v=json.loads((O/'verification.json').read_text());assert v['success'] and v['arrival_checks']==26
review={
 'reviewed_at':datetime.datetime.now().astimezone().isoformat(),'executable_sha256':v['executable_sha256'],'arrival_data_sha256':v['arrival_data_sha256'],
 'scope':'Seven current automated 1080p opening captures and 17 current menu captures reviewed as contact sheets; selected full-resolution drive, checkpoint and doctor frames. Not a full manual playthrough of all rooms or chapters.',
 'findings':['Arrival car is visible before the gate and approaches in the direction of its headlights.','The old notice is outside the guard booth and its Russian Cyrillic font is readable. The journal and subtitle explicitly date the accident before arrival.','The doctor text remains visible while the save indicator is shown; automatic save no longer replaces this dialogue.','Menu and existing scare/submenu flow retained, with new arrival after New Game.'],
 'limits':['The hero partly occludes the notice from the capture angle; it can also be read in the journal.','Capture 07_old_lab_remnant is occluded/misframed by the hero and wall; it does not verify the added non-blocking old metal plate.','The reception NPC uses the existing survivor placeholder and resembles the hero. Driver seating/door exit animation and newly voiced dialogue are not implemented.','Later first-chapter events and the remaining chapters retain existing prototype scene markers/text responses.','No new whole-map visual capture, complete manual campaign run, OS focus/PrintScreen test or performance benchmark. Earlier v28 art evidence is historical and its data are unchanged.']}
(O/'visual_review.json').write_text(json.dumps(review,ensure_ascii=False,indent=2),encoding='utf8')
intro='''**Сюжетное начало v29 готово:** герой приезжает на машине, выходит у КПП и узнаёт из старого уведомления, что авария лаборатории произошла **давно, до его приезда**. Затем проходит через тамбур в приёмную; включается защитная система и поступает сообщение врача. Новая лабораторная авария при появлении героя не запускается. F — взаимодействие/выход из машины, J — журнал. Для вступления выбери «Новую игру»; старые сохранения продолжаются с прежнего места.

Сборка игры и редактора готова. Проверки этой сборки: вступление **26/26**, мир **140/140** и **1012 дверей**, сюжет/сохранения **25/25**, меню **57/57**. Русская табличка читается; автосохранение сохраняет значок и не перекрывает реплики; системный замок N02 не снимается обычным F изнутри, сюжетный маршрут главы 4 открывает вход. Карта и прежнее оформление сохранены. Дальнейшие сцены пока частично прототипные; вводные новые реплики подаются текстом. [Реальные кадры](artifacts/echo_arrival_v29/preview.jpg), [отчёт](artifacts/echo_arrival_v29/REPORT.md).

'''
f=R/'README.md';s=f.read_text(encoding='utf-8-sig');s=s.replace('# Эхо — Отделение 04\n\n','# Эхо — Отделение 04\n\n'+intro,1);f.write_text(s,encoding='utf8')
ue='''## Сюжетное вступление v29 — завершено

Канон: давняя лабораторная авария предшествует приезду. Car → КПП/старое уведомление → N02/N01 → защитная система/врач → существующая глава 1. Новое событие аварии не запускается. Источник уточнения: data/hospital_furnishing/arrival_v29_canon.md. Дата/причина аварии не придуманы; поздняя тайна аварии героя не раскрывается.

WardHospitalArrival29.cpp/Assessment29.cpp, Content/CampaignData/echo_arrival_v29.json. StartArrival вызывается из Commandnew после прежнего меню-перехода; NewGame/Restore сами его не запускают. Состояния -1legacy,0drive,1parked,2КПП,3вход,4связь,5normal. Snapshot arrival29 хранит этап/таймеры; старые saves без поля пропускают вступление. Машина640 и terminal149 из прежней библиотеки. Русский стенд использует world-space UWidgetComponent со Slate/Noto, добавлена зависимость UMG. После входа N02north получает системную блокировку до маршрута главы4; состояние двери и тип восстанавливаются при загрузке. Автоматический save toast не перебивает активную Notice, значок остаётся.

Tools/verify_hospital_arrival_v29.ps1 -Pictures:26checks/7PNG. Дополнительно World140/1012doors, Story25, Menu57/17PNG на одном EXE; Game и Editor собраны. tools/audit_echo_arrival_v29.py и export_echo_arrival_v29.py сохраняют доказательства/идентичность сборки. [Отчёт и ограничения](../../artifacts/echo_arrival_v29/REPORT.md). Карта562/35 и все данные оформленияv28 прежние. Полный156-кадровый обзор и FPS этой версии заново не измерялись: в отчёте v28 они исторические. Оставшиеся сцены частично прототипные, нет готовой анимации посадки/выхода и новой актёрской озвучки.

---

'''
f=P/'README.md';f.write_text(ue+f.read_text(encoding='utf-8-sig'),encoding='utf8')
f=R/'HANDOFF_NEXT_GPT.md';s=f.read_text(encoding='utf-8-sig');history='# v28 — хоррор-оформление карты ЗАВЕРШЕНО'+s.split('# v28 — хоррор-оформление карты ЗАВЕРШЕНО',1)[1]
head=f'''# v29 — исправленное начало сюжета ЗАВЕРШЕНО

Последний запрос выполнен. Пользователь поправил: лабораторная авария произошла ДАВНО, до приезда героя. Начало: машина → КПП, где он узнаёт о старой аварии → тамбур/приёмная → оригинальный сюжет v2. Новой аварии при его появлении нет. Не путать с поздней тайной аварии самого героя. Канон data/hospital_furnishing/arrival_v29_canon.md; исходный story_v2_source.md сохранён. Дата/причина не назначены.

D:/Projects/ECHO. Итоговый EXE {v['executable_sha256']}; ArrivalData {v['arrival_data_sha256']}; EditorDLL {v['editor_module_sha256']}. Последняя цепь59179: GameBuild28сек/EditorBuild16сек → stage/archive71сек с reuse cooked content (кодовое исправление) → Arrival26/7PNG → World140/1012doors → Story25 → Menu57/17PNG. Финальные логи tmp/echo_v29_*_seal.log. Полный предшествующий cook с UMG успешен package_accepted.log, финальные архитектурные/UMG assets прежние. Export/ModeValidate и identity прошли. Старые package_verified_v28_before_arrival.json и artifacts.before_identity сохранены; текущий package_verified.json v29. Не запускать старые экспортёры поверх текущей сборки.

WardHospitalArrival29.cpp/Assessment29.cpp + echo_arrival_v29.json. -1 legacy/nointro;0drive8сек(кинематическая машина640/фары/camera);1parkedFexit;2КПП(F/LOS);3walkentry;4security/doctor5сек;5normalChapter1Step2. StartArrival только Commandnew после прежней TitleScene cinematic, не общий NewGame/Load. Старые saves без arrival29 не повторяют вступление. Новые сохраняют этап/clocks/flags; сидя в машине Save запрещён, пешком работает. Первый этап записывает сестринское сообщение в J, КПП — старое уведомление о лаборатории. Никаких новых монстров/взрывов/blackout; монстры со2главы. Нужна «Новая игра», чтобы увидеть вступление.

Терминал(63.3,-56.8,1.1), точка use(63.3,-58.4,.98) на наружной стороне настоящей будки у medical_main периметра. Старый S03 другой удалённый КПП сохранён. Шлагбаум визуально поворачивается; проход проверен sweeps автомобиля по1м и непрерывным capsule-sweep маршрута машина→КПП→тамбур→вестибюль. Notice — UWidgetComponent/SetSlateWidget/Noto (UMG в Build.cs), правильная кириллица. Первый TextRender давал квадраты и был заменён. Не использовать ранние final/accepted картинки с квадратами; текущие скопированы в artifacts/echo_arrival_v29/captures.

После входа N02north под системным замком: P.Type=shutter не даёт обычному UseArchitecture открывать изнутри. В главе4 открывается маршрут, тип возвращается ordinary. ApplyArrivalState вызывается после изменения главы и после Restore; legacy восстанавливает ordinary без изменения savedOpen/Locked. Тест реального UseArchitecture изнутри плюс освобождения Ch4 прошёл. Это не заявка на полную готовность всех карантинных маршрутов/ветвлений. AutoSave больше не перекрывает активную Notice; SaveIcon остаётся. Native26 проверяет видимость реплики врача после автоматического Save. Manual save в тесте отложен после кадра05.

Карта562зоны/35локаций, oldJSON и материалыv28 побайтово прежние. Native786, BookWorldcustom112123/types266/instances761983,910traces28,15models27/23918props27 сохранены. Отдельные сюжетные акторы не входят в BookWorldсчётчики. Никаких новых внешних моделей/скачиваний/агентов/облачных операций. Метаданные art library остаются28: здесь сюжет29, художественные ресурсы прежние.

artifacts/echo_arrival_v29: REPORT/verification/visual_review, preview/contact/menu_contact,7PNG/17menu, исходные before8files и source_changes.patch, nativechecks,acceptance10/10. Визуально проверены финальные контакты и full-res01drive/03notice/05doctor. Ограничения: hero частично закрывает текст таблички, запись доступна в J;07old_lab_remnant плохо направлен/перекрыт, не доказывает видимость добавленной неколлизионной пластины; NPC в приёмной — прежний survivor placeholder похожий на героя. Дальнейшие события первой главы/19глав сохраняют частично текстовые/пространственные заготовки. Новые реплики — текст+старые телефон/дверь/связь, не готовая актёрская озвучка; нет настоящей анимации посадки/выхода/колёсного рига. Не заявлять полностью готовыми весь сюжет, все ветки или фотореалистичные катсцены. Новый FPS/полный worldvisual156 не измерялись; evidence28 историческое.

Обязательных работ по текущему вступлению не осталось. Ссылки запуска PLAY_UNREAL.cmd. README/HANDOFF tinyкопии синхронизированы в старый Cконтекст, дочерние junctionнаD не удалять. Googleархив старой задачи всё ещё НЕподтверждён: не удалять Dархивы/миграционные папки. Временные ранние прогоны прерывались намеренно для UI и замка; один wrapper ложнопадал на null LASTEXITCODE -ne0 (PSscripts проверять через ErrorActionStop, LASTEXITCODE только дляnativecmd). Это не текущие ошибки. Ориентироваться на финальный seal pipeline/report.

---

'''
f.write_text(head+history,encoding='utf8')
legacy=Path('C:/Users/YOUR_WINDOWS_USER/OneDrive/Documents/ChatGPT/pacman')
for n in ['README.md','HANDOFF_NEXT_GPT.md']:shutil.copy2(R/n,legacy/n)
print('V29_COMPLETE_DOCS_AND_VISUAL_REVIEW_SAVED')
