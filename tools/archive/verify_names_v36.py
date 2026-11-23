"""Validate naming-only migration independently from the scenario progression assessment."""
import json,sys,re,hashlib
from pathlib import Path
import struct
def font_has(path,code):
 b=path.read_bytes();u16=lambda at:struct.unpack_from(">H",b,at)[0];u32=lambda at:struct.unpack_from(">I",b,at)[0]
 cmap=None
 for i in range(u16(4)):
  off=12+16*i
  if b[off:off+4]==b"cmap":cmap=u32(off+8);break
 if cmap is None:return False
 for i in range(u16(cmap+2)):
  sub=cmap+u32(cmap+4+8*i+4);fmt=u16(sub)
  if fmt==12:
   for j in range(u32(sub+12)):
    a,z,g=struct.unpack_from(">III",b,sub+16+12*j)
    if a<=code<=z and g+code-a:return True
  if fmt==4 and code<65536:
   n=u16(sub+6)//2;ends=sub+14;starts=ends+2*n+2;deltas=starts+2*n;ranges=deltas+2*n
   for j in range(n):
    if u16(starts+2*j)<=code<=u16(ends+2*j):
     shift=u16(deltas+2*j);rng=u16(ranges+2*j)
     glyph=(code+shift)&65535 if rng==0 else u16(ranges+2*j+rng+2*(code-u16(starts+2*j)))
     if glyph:return True
 return False
from scenario_names_v36 import *
R=ROOT;A=R/'artifacts/echo_names_v36';U=R/'unreal/EchoWard';p=U/'Content/CampaignData/echo_scenario_v35.json';d=json.loads(p.read_text(encoding='utf-8'));old=json.loads((A/'before/echo_scenario_v35.json').read_text(encoding='utf-8-sig'))
checks=[]
def check(name,value):
 checks.append(dict(name=name,passed=bool(value)))
 if not value:print('FAIL',name)
reg=d['name_registry'];check('28_user_spellings_exact',[(q['number'],q['name']) for q in reg['people'][:28]]==[(n,new) for n,_,new,_ in PEOPLE]);check('36_people_unique',len(reg['people'])==len({q['name'] for q in reg['people']})==36)
check('35_individual_monsters_unique',len(reg['monster_individuals'])==len({q['nickname'] for q in reg['monster_individuals']})==35)
check('same_unique_actor_has_same_name_at_every_anchor',all(len({q['name'] for q in d['gameplay']['mobs'] if q['actor_guid']==m['actor_guid']})==1 for m in reg['monster_individuals']))
def structure(v):
 if isinstance(v,dict):return {k:structure(x) for k,x in v.items() if k not in ('name_registry','names_revision','type_nickname')}
 if isinstance(v,list):return [structure(x) for x in v]
 if isinstance(v,str):return '<text>'
 return v
check('no_numeric_geometry_or_gameplay_value_changed',structure(d)==structure(old))
for field in ('placements','mobs'):
 check(field+'_stable_ids',[q['id'] for q in d['gameplay'][field]]==[q['id'] for q in old['gameplay'][field]])
check('scene_ids_and_order_preserved',list(d['scenes'])==list(old['scenes']))
check('required_story_ids_preserved',[[*c['required']] for c in d['gameplay']['chapters']]==[[*c['required']] for c in old['gameplay']['chapters']])
check('npc_labels_match_user_roster',all(q['label']==reg['people'][NPC_NUM[q['id']]-1]['name'] for q in d['gameplay']['placements'] if q['id'] in NPC_NUM))
check('optional_question_speakers_match_live_npcs_or_remote_doctor',all(row[0]=='Гүлсезім' or any(row[0] in q['label'] for q in d['gameplay']['placements'] if q['id'] in NPC_NUM) for row in d['questions']))
texts=[]
def collect(v,key=''):
 if isinstance(v,dict):
  for k,x in v.items():
   if k not in KEEP and k!='name_registry':collect(x,k)
 elif isinstance(v,list):
  for x in v:collect(x,key)
 elif isinstance(v,str):texts.append(v)
collect(d);t='\n'.join(texts)
stems=['Алекс','Соловь','Ирин','Ланск','Рудин','Никит','Фролов','Борис','Астахов','Павел','Павл','Мельник','Денис','Ковал','Артём','Беляев','Олег','Дёмин','Софь','Орлов','Семён','Левин','Михаил','Громов','Карим','Леонид','Зуев','Антон','Серов','Галин','Глеб','Зотов','Нина','Вадим','Лида','Ольг','Егор','Мороз','Алён','Саша','Лазар','Андре','Лосев']
left=sorted(set(re.findall(r'\b(?:'+'|'.join(stems)+r')\w*',t)));check('no_old_human_names_in_display_data',not left)
check('family_cases_disambiguated',rename_text('Алексея Соловьёва / Ирины Соловьёвой / Егора Морозова / Алёны Морозовой')=='Маулеш Беки / Құрбанәлі Аружан / Шаймердинов Аргына / Жаксымбетова Акботы')
chars=set(''.join(q['name'] for q in reg['people'])+''.join(q['nickname'] for q in reg['monster_individuals']))
for fn in ('NotoSans.ttf','Literata.ttf'):
 check(fn+'_all_names_have_glyphs',all(font_has(U/'Content/CampaignData/Fonts'/fn,ord(c)) for c in chars))
check('canonical_world_unchanged',hashlib.sha256((U/'Content/CampaignData/hospital_world.json').read_bytes()).hexdigest()==d['source_map_sha256']=='d3aa691f002641ea65b675c07294f26477b67d33185e0dd8d18b4d9755e873ec')
report=dict(success=all(c['passed'] for c in checks),count=len(checks),checks=checks,remaining_old_names=left);(A/'naming_data_checks.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(A/'РЕЕСТР_ИМЁН.json').write_text(json.dumps(reg,ensure_ascii=False,indent=2),encoding='utf-8')
lines=['# ЭХО — казахские имена и клички, версия 36','','Имена 1–28 записаны точно как прислал пользователь. Родственные связи сохранены, даже если фамилии разные. Язык остального текста игры не менялся.','','## Люди','']
lines += [f"{q['number']}. **{q['name']}** — {q['role']}." for q in reg['people'][:28]]
lines+=['','## Дополнительные имена, предложенные для проверки','','Эти имена закреплены в реестре ролей. Само присвоение имени не создаёт нового 3D-персонажа.','']
lines += [f"{q['number']}. **{q['name']}** — {q['role']}." for q in reg['people'][28:]]
lines+=['','## Клички монстров','','У каждого из 35 постоянных противников своя кличка. Опорные точки одного монстра используют одну кличку.','']
for sp,(old_type,type_name,_) in MONSTERS.items():
 names=[q['nickname'] for q in reg['monster_individuals'] if q['species']==sp]
 if sp=='parasite':names=['Құрт-01, Құрт-02 и далее (появляющиеся малые организмы)']
 lines.append(f"- **{type_name}** ({old_type}): {', '.join(names)}.")
lines+=['','Изменены игровые данные сюжета, подписи NPC, записки, реплики и названия врагов. Для старых строк журнала добавлена замена при отображении. Служебные ID персонажей, событий и сохранений не переименовывались. Исходный сюжетный PDF сохранён без изменений.','']
(A/'ИМЕНА_НА_ПРОВЕРКУ.md').write_text('\n'.join(lines),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False));sys.exit(0 if report['success'] else 1)
