"""Compile user-authored PDF data into a separate, reversible UE runtime scenario."""
from pathlib import Path
import json,copy,hashlib,re,collections
ROOT=Path(__file__).resolve().parents[1];REF=ROOT/'data/story_reference_v2_2026_10_28';OUT=ROOT/'unreal/EchoWard/Content/CampaignData/echo_scenario_v35.json'
L=json.loads((REF/'literary/story_v2.json').read_text(encoding='utf-8'));S=json.loads((REF/'staging/scenario_data.json').read_text(encoding='utf-8'));W=json.loads((OUT.parent/'hospital_world.json').read_text(encoding='utf-8'));D=copy.deepcopy(W['story_gameplay']);props={p['id']:p for p in D['placements']};zones={r['id']:(site,r) for site in W['locations'] for r in site['rooms']}
N=dict(version=35,literary_version='2.0',source_map_sha256=hashlib.sha256((OUT.parent/'hospital_world.json').read_bytes()).hexdigest(),source_pdf_sha256=hashlib.sha256((REF/'ЭХО_полный_сюжет_и_диалоги_версия_2.pdf').read_bytes()).hexdigest(),source_kind='user_pdf',chapters=[],scenes={},documents=copy.deepcopy(S['notes']),alternatives=L['alternatives'],questions=L['questions'],barks=L['barks'],system_text=L['system_text'],journal=L['journal'],side_stories=L['side_stories'],staging=S,gameplay=D,limits=['Complete authored text and interactive state graph; text presentation, not voiced cinematic production.','Character models reuse the existing rig; choreographed escort, physical boss set pieces and bespoke animations require later production.'])
for q in S['proposed_placements']:
 p=props[q['id']];p.update({k:copy.deepcopy(v) for k,v in q.items() if k not in ('status','source_at','source_chapter')})
 if q['status'].startswith('Пустой контейнер'):p['kind']='empty_container';p['amount']=0
 if p['id'] in S['notes']:p['body']=S['notes'][p['id']]['text'];p['label']=S['notes'][p['id']]['title']
props['PERSON_2']['label']='Виктор Рудин';props['PERSON_3']['label']='Ирина Соловьёва';props['PERSON_4']['label']='Никита Фролов';props['PERSON_6']['label']='Денис Ковалёв';props['PERSON_6']['chapter']=7
props['PERSON_5']['kind']='story_object';props['PERSON_5']['label']='Список выживших';props['PERSON_7']['kind']='story_object';props['PERSON_7']['label']='Следы пребывания Ирины';props['PERSON_7']['chapter']=13
for q in S['new_supply_packs']:
 p=copy.deepcopy(q);D['placements'].append(p);props[p['id']]=p
for q in S['new_objects']:
 p=copy.deepcopy(q);p.update(kind='story_object',label=q['description'].split('.')[0],amount=1)
 if p['id']=='NEW_STROBE_01':p['at'][0]+=.28
 D['placements'].append(p);props[p['id']]=p
# Fit authored anchor heights to actual supporting geometry, preserving the document's intended object.
props['NOTE_01_01']['at'][2]=1.34
props['NEW_BASELINE_001']['at'][2]=-8.28 # 1.30m cryomodule lid +2cm
for q in D['placements']:
 if 'site' not in q:q['site']=zones[q['zone']][0]['id']
 if q['id'] in ('NEW_UT_CAMERA','NEW_UT_LEVER'):q['site']='UT'
# Additional v2 documents use existing zone supports, retaining the exact source text.
for i,(title,place,body) in enumerate(L['new_documents'],1):
 matches=re.findall(r'\b(?:RH00|DG\d+|HS\d+|PS\d+|SV\d+|B1\d+|F\d+|R\d+|E\d+|S\d+|T\d+)\b',place);zone=next((q for q in matches if q in zones),'R14');existing=next((q for q in props.values() if q['zone']==zone and q['kind']=='note'),None)
 at=copy.deepcopy(existing['at'] if existing else [zones[zone][0]['origin'][0]+zones[zone][1]['rect'][0]+2,zones[zone][0]['origin'][1]+zones[zone][1]['rect'][1]+2,zones[zone][0]['origin'][2]+.8]);at[0]+=.32
 ident=f'DOC_V2_{i:02}';p=dict(id=ident,zone=zone,site=zones[zone][0]['id'],at=at,kind='note',label=title,body=body,chapter=existing['chapter'] if existing else 12,amount=1);props[ident]=p;D['placements'].append(p);N['documents'][ident]=dict(title=title,text=body,author='Документ версии 2')
# One actor per named character; the original duplicate sister marker becomes evidence.
npc_specs=[('MARINA','Марина Егорова','N05',2),('BORIS','Борис Астахов','B102',5),('PAVEL','Павел Мельник','B102',5),('RAYA','Рая Каримова','N05',2),('LEONID','Леонид Зуев','N05',2),('SEROV','Антон Серов','F212',3),('GALINA','Галина','E14',6),('ZOTOV','Глеб Зотов','B120',9),('ARTYOM','Артём Беляев','F203',8),('SEMYON','Семён Левин','DG18',12),('ORLOVA','Софья Орлова','S08',14),('DEMIN','Олег Дёмин','S16',14),('001','Михаил Громов','E09',15),('NINA','Нина Мельник','RH00',11),('OLGA','Ольга','PS11',11),('EGOR','Егор','PS06',11),('SASHA','Саша','PS20',11)]
used=collections.Counter()
for suffix,name,zone,ch in npc_specs:
 site,r=zones[zone];ox,oy,z=site['origin'];x,y,w,h=r['rect'];n=used[zone];used[zone]+=1
 at=[ox+x+1.6+n*.9,oy+y+1.7,z];ident='NEW_NPC_'+suffix;p=dict(id=ident,kind='person',label=name,zone=zone,site=site['id'],chapter=ch,at=at,amount=1,person_guid=suffix);D['placements'].append(p);props[ident]=p
for p in D['placements']:
 if p['id'].startswith('SIDE_'):
  s=int(p['id'].split('_')[1]);p['chapter']={1:1,2:11,3:11,4:7,5:2,6:5}[s]
for ch in L['main']:
 n=ch['n'];required=[q['id'] for q in S['scenes'] if q['chapter']==n]
 if n==10:required=['SCENE_10_01','SCENE_10_02','SCENE_10_05']
 if n==19:required=['SCENE_19_01','SCENE_19_02','SCENE_19_03','SCENE_19_04','SCENE_19_05']
 N['chapters'].append(dict(id=n,title=ch['title'],required=required,route=S['chapters'][n-1]['route'],goal=S['chapters'][n-1]['goal']))
 D['chapters'][n-1]['required']=required
for q in S['scenes']:
 ident=q['id'];ch=q['chapter'];sn=int(ident.rsplit('_',1)[1]);b=next(q for q in L['scene_binding'] if q['source_id']==ident)
 if 'literary_scene' in b:lit=L['main'][ch-1]['scenes'][b['literary_scene']-1]
 elif ch==10:lit=L['main'][9]['scenes'][min(sn,4)-1]
 else:lit=L['main'][18]['scenes'][min(sn,4)-1]
 text=lit['text'];title=lit['title'];place=lit['place']
 if ident=='SCENE_10_05':text=L['main'][9]['scenes'][2]['text']+'\n\n'+L['main'][9]['scenes'][3]['text'];title='Нижняя заслонка и недавний след'
 if ident in ('SCENE_10_03','SCENE_10_04'):
  alt=L['alternatives'][7 if sn==3 else 8];text=alt['text'];title=alt['title']
 if ident=='SCENE_19_03':text='Подготовленный выход. Дождись подтверждения чистого маршрута. Воздушный путь доступен только при плане эвакуации.';title='Выбранный путь выхода'
 if ident=='SCENE_19_04':text=L['main'][18]['scenes'][2]['text'];title=L['main'][18]['scenes'][2]['title']
 if ident=='SCENE_19_05':text=L['main'][18]['scenes'][3]['text'];title=L['main'][18]['scenes'][3]['title']
 node=dict(id=ident,chapter=ch,title=title,place=place,text=text,stage=q['stage'],play=q['play'],outcome=q['result'],choices=[],requires=[],effects={},skippable=ident in ('SCENE_10_03','SCENE_10_04'))
 N['scenes'][ident]=node;props[ident]['label']=title
# The central airlock is underground, SV24 remains a remote log.
for ident,at in [('SCENE_10_01',[50,221.5,-4.8]),('SCENE_10_02',[49,222.5,-4.8])]:props[ident].update(at=at,site='UT')
props['SCENE_10_05'].update(at=copy.deepcopy(props['SCENE_10_04']['at']),site='HS',zone='HS11')
props['SCENE_19_04'].update(at=[52,-60,0],zone='S01',site='ST')
# Exact authored alternatives with explicit choices; defaults are visible player choices, never auto-selected.
def choice(node,key,options):
 N['scenes'][node]['choice_key']=key;N['scenes'][node]['choices']=[dict(id=i,label=label,text=N['scenes'][node]['text'] if alt is None else L['alternatives'][alt]['text'],effects=effects) for i,label,alt,effects in options]
choice('SCENE_01_03','POWER_MODE',[('LOCAL','Местная линия',None,{'POWER_LOCAL':True}),('FULL','Общая линия и камеры',0,{'POWER_FULL':True})])
choice('SCENE_02_03','PHONE',[('ANSWER','Ответить на звонок',None,{'VOICE_SAMPLE':True}),('SILENT','Не отвечать',1,{'VOICE_SAMPLE':False})])
choice('SCENE_02_04','RESCUE_ORDER',[('RESCUE','Сначала вывести людей',None,{'NIKITA_ALIVE':True,'SURGERY_EQUIPMENT':False}),('SISTER','Сразу идти к сестре',2,{'NIKITA_ALIVE':False,'SURGERY_EQUIPMENT':True})])
choice('SCENE_03_03','SEROV',[('SAVE','Помочь Серову',None,{'PATIENT_RESCUED':True}),('LEAVE','Уходить без Серова',3,{'PATIENT_RESCUED':False})])
choice('SCENE_05_04','LOG',[('KEEP','Сохранить журнал',None,{'LOG_PRESERVED':True}),('DELETE','Позволить удалить журнал',4,{'LOG_PRESERVED':False})])
choice('SCENE_06_04','ARCHIVE',[('TOGETHER','Смотреть запись вместе',None,{'ARCHIVE_TOGETHER':True}),('ALONE','Посмотреть тайно',5,{'ARCHIVE_TOGETHER':False})])
choice('SCENE_08_04','PACT',[('TOGETHER','Продолжать вместе',None,{'PACT':True}),('SEPARATE','Временно расстаться',6,{'PACT':False})])
choice('SCENE_10_02','AIRLOCK_CHOICE',[('VERIFY','Проверить два независимых признака',None,{'IDENTITY_CHECKED':True,'RECEIVER_DAMAGED':False}),('OPEN','Открыть женщине за стеклом',7,{'RECEIVER_DAMAGED':True,'IMPOSTOR_RELEASED':True}),('RADIO','Сразу идти на рацию',8,{'RECEIVER_DAMAGED':False,'IMPOSTOR_DETOUR':True})])
# Isolating a borrowed voice is explicit, not a random roll, including after restore.
N['scenes']['SCENE_10_01']['effects']={'AIRLOCK_SPLIT':True,'GLASS_IS_IMPOSTOR':True,'RADIO_IS_LIVE_IRINA':True}
choice('SCENE_11_02','PAVEL',[('STAY','Оставить Павла под наблюдением',None,{'PAVEL_RETAINED':True}),('EXILE','Изгнать Павла',9,{'PAVEL_RETAINED':False}),('ACCUSE','Сначала обвинить Марину',10,{'MARINA_TRUST':False,'PAVEL_RETAINED':True})])
choice('SCENE_11_03','REFUGE',[('EVACUATE','Вывести людей, изолировать бытовку',None,{'REFUGE_EVACUATED':True}),('HOLD','Удержать бытовку после отсечек',11,{'REFUGE_EVACUATED':False})])
choice('SCENE_12_02','SEMYON',[('SEPARATE','Разделить линии, помочь Семёну',None,{'SEMYON_HELPED':True}),('CUT','Отключить корпус грубо',12,{'SEMYON_HELPED':False})])
choice('SCENE_13_04','RELATION',[('PACT','Договориться о помощи и согласии',None,{'BROTHER_SISTER_PACT':True}),('ALLIES','Сотрудничать без прощения',13,{'INDEPENDENT_ALLIES':True})])
plans=[('CURE','Подготовить лечение',None,{}),('DESTROY','Подготовить уничтожение центра',16,{}),('EVAC','Подготовить эвакуацию',18,{}),('REPLACE','Добровольное замещение носителя',19,{}),('CONTINUE','Рассмотреть сделку',20,{})]
choice('SCENE_15_04','PLAN',plans)
# Planning does not prematurely play final outcome prose.
for opt in N['scenes']['SCENE_15_04']['choices']:opt['text']=N['scenes']['SCENE_15_04']['text'];opt['effects']={}
choice('SCENE_16_02','BORIS',[('HONEST','Честно объяснить риски',None,{'BORIS_TRUST':True}),('LIE','Скрыть риск отключения питания',14,{'BORIS_TRUST':False})])
choice('SCENE_16_03','TRUTH',[('SEND','Передать доказательства двумя каналами',None,{'TRUTH_SENT':True}),('KEEP','Оставить документы внутри',22,{'TRUTH_SENT':False})])
choice('SCENE_16_04','CONSENT',[('ACCEPT','Подтвердить объяснённые условия',None,{'IRINA_CONSENT':True}),('QUARANTINE','Отказаться от процедуры, готовить карантин',18,{'IRINA_CONSENT':False})])
choice('SCENE_18_04','FINAL_PLAN',plans)
for opt in N['scenes']['SCENE_18_04']['choices']:
 opt['text']='Окончательная операция: '+opt['label']+'.\n\n'+N['scenes']['SCENE_18_04']['text'] if opt['id']=='CURE' else 'Окончательная операция: '+opt['label']+'. Условия операции подтверждены; внешний карантин остаётся действующим.'
 opt['requires']=['PLAN_READY_'+opt['id']]
# Final operations share spatial checkpoints but never the CURE dialogue of another ending.
N['final_texts']={name:L['alternatives'][i]['text'] for name,i in [('CURE_LOSS',15),('DESTROY',16),('DESTROY_STAY',17),('EVAC',18),('REPLACE',19),('CONTINUE',20),('OUTBREAK',21),('NO_TRUTH',22)]}
choice('SCENE_19_01','FINAL_OPERATION',[('EXECUTE','Выполнить подтверждённый план',None,{}),('STAY','Остаться у ручного контура',17,{'HERO_STAYS':True})])
choice('SCENE_19_02','BOUNDARY',[('CHECK','Повторная независимая проверка',None,{'RECHECKED':True}),('BREACH','Открыть наружный коридор без подтверждения',21,{'OUTBREAK':True,'RECHECKED':False})])
# Interactive prerequisites are authored items, not a decorative count of rooms visited.
N['scenes']['SCENE_01_03']['requires']=['visitor_pass','first_pistol','spare_fuse']
N['scenes']['SCENE_05_03']['requires']=['NEW_LIFT_BELT']
N['scenes']['SCENE_15_01']['requires']=['NEW_BASELINE_001']
N['scenes']['SCENE_01_04']['effects']={'RUDIN_ALIVE':False,'CH1_DONE':True}
N['scenes']['SCENE_04_02']['effects']={'OUTSIDE_DENIED':True}
N['scenes']['SCENE_06_03']['effects']={'PROFILE_17_KNOWN':True}
N['scenes']['SCENE_08_04']['effects']={'FULL_PROTOCOL_FRAGMENT':True}
N['scenes']['SCENE_07_04']['effects']={'KOVALEV_RESCUED':True}
N['scenes']['SCENE_14_02']['effects']={'ORLOVA_ALLY':True}
N['scenes']['SCENE_14_04']['effects']={'EVAC_LIST_CHECKED':True}
N['scenes']['SCENE_16_01']['effects']={'COMPARISON_READY':True}
N['scenes']['SCENE_16_02']['effects']={'MANUAL_POWER_READY':True}
N['scenes']['SCENE_17_03']['effects']={'CONTAINMENT_READY':True}
N['side_rewards']={'1':'RUDIN_TRUTH','2':'LAST_NURSE','3':'NINA_FOUND','4':'LETTERS_SAVED','5':'LAZAREV_DRAWING','6':'NIGHT_SHIFT_TRUTH'}
# Keep legacy mob records as anchor points but assign persistent individual identities.
unique={'stitched','listener','keeper','impostor','nest','blind_surgeon','armoured','collector','pursuer','conductor','central_organism'}
for m in D['mobs']:m['actor_guid']=m['species'] if m['species'] in unique else m['id']
# Keep delayed branch consequences at their actual later scene, avoiding early spoilers.
N['reactions']=[]
def delayed(alt,node,marker,flag=None,choice_key=None,choice_value=None,until=None,replace=False):
 text=L['alternatives'][alt]['text'];head,sep,tail=text.partition(marker)
 if not sep:return
 for scene in N['scenes'].values():
  for opt in scene['choices']:
   if opt['text']==text:opt['text']=head.rstrip()
 if until:tail=tail.partition(until)[0]
 N['reactions'].append(dict(scene=node,flag=flag,choice_key=choice_key,choice_value=choice_value,text=tail.strip(),replace=replace))
delayed(0,'SCENE_07_01','\n\nВ главе 7',choice_key='POWER_MODE',choice_value='FULL',until='\n\nВ главе 18')
N['reactions'].append(dict(scene='SCENE_18_01',choice_key='POWER_MODE',choice_value='FULL',text=L['alternatives'][0]['text'].partition('\n\nВ главе 18')[2].strip(),replace=False))
delayed(1,'SCENE_08_02','\n\nВ главе 8',choice_key='PHONE',choice_value='SILENT',replace=True)
delayed(2,'SCENE_03_01','\n\nВ хирургии',choice_key='RESCUE_ORDER',choice_value='SISTER',until='\n\nПозднее Марина')
N['reactions'].append(dict(scene='SCENE_05_01',choice_key='RESCUE_ORDER',choice_value='SISTER',text=L['alternatives'][2]['text'].partition('\n\nПозднее Марина')[2].partition('\n\nВ бытовке')[0].strip(),replace=False))
delayed(3,'SCENE_16_01','\n\nВ главе 16',choice_key='SEROV',choice_value='LEAVE',replace=True)
delayed(4,'SCENE_16_03','\n\nВ архиве допуска',choice_key='LOG',choice_value='DELETE',until='\n\nПозднее Зотов')
N['reactions'].append(dict(scene='SCENE_09_02',choice_key='LOG',choice_value='DELETE',text=L['alternatives'][4]['text'].partition('\n\nПозднее Зотов')[2].strip(),replace=False))
delayed(5,'SCENE_08_04','\n\nВ R45',choice_key='ARCHIVE',choice_value='ALONE')
delayed(6,'SCENE_09_01','\n\nДо главы 9',choice_key='PACT',choice_value='SEPARATE')
delayed(13,'SCENE_19_04','\n\nПосле чистого выхода',choice_key='RELATION',choice_value='ALLIES',replace=True)
delayed(14,'SCENE_17_02','\n\nВ главе 17',choice_key='BORIS',choice_value='LIE',until='\n\nПосле выхода',replace=True)
N['final_scenes']={}
for variant,text in N['final_texts'].items():
 if variant=='NO_TRUTH':continue
 blocks=text.split('\n\n');groups=['']*5;total=sum(len(b) for b in blocks);cumulative=0
 for block in blocks:
  index=min(4,int(cumulative/max(total,1)*5));groups[index]+=(('\n\n' if groups[index] else '')+block);cumulative+=len(block)
 N['final_scenes'][variant]=groups

D['blank_notes']=[];D['version']=35
assert len(N['documents'])==103
assert len(N['scenes'])==78 and sum(len(c['required']) for c in N['chapters'])==76
for p in D['placements']:assert p['zone'] in zones,p['id']
from scenario_names_v36 import apply_names
N=apply_names(N)
OUT.write_text(json.dumps(N,ensure_ascii=False,separators=(',',':')),encoding='utf-8')
report=dict(success=True,version=35,source_pdf_sha256=N['source_pdf_sha256'],source_map_sha256=N['source_map_sha256'],chapters=19,scene_ids=78,main_required_scenes=76,choice_scenes=sum(bool(q['choices']) for q in N['scenes'].values()),alternatives=23,documents=len(N['documents']),placements=len(D['placements']),new_npcs=len(npc_specs),runtime_file=str(OUT))
art=ROOT/'artifacts/echo_scenario_v35';art.mkdir(exist_ok=True);(art/'data_compile.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(report,ensure_ascii=False))
