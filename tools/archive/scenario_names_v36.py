"""Kazakh naming profile. Stable scene, actor and save IDs are never renamed."""
import copy,re,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PEOPLE=[
(1,'Алексей Соловьёв','Маулеш Бека','Главный герой'),(2,'Ирина Соловьёва','Құрбанәлі Аружан','Сестра главного героя'),(3,'Вера Ланская','Мусалимова Гүлсезім','Врач по связи / сохранённая личность'),(4,'Виктор Рудин','Файзулаев Талғат','Сотрудник безопасности'),(5,'Марина Егорова','Абдухаримов Ұлбала','Старшая медсестра'),(6,'Никита Фролов','Жарылқасын Маржан','Раненая санитарка'),(7,'Борис Астахов','Қайсенов Қасым','Инженер'),(8,'Павел Мельник','Шардарбек Абылай','Кладовщик'),(9,'Денис Ковалёв','Чиликбаев Тохтар','Врач исследовательского центра'),(10,'Артём Беляев','Оразгалиев Саловат','Пациент-свидетель'),(11,'Олег Дёмин','Асубаев Ерболат','Руководитель безопасности'),(12,'Софья Орлова','Аргымбаева Аягөз','Врач эвакуационной группы'),(13,'Семён Левин','Жуманбаев Адылбек','Младший техник'),(14,'Михаил Громов','Имашев Ерик','Пациент 001'),(15,'Рая Каримова','Жанабекова Айсулу','Уборщица'),(16,'Леонид Зуев','Оразданов Жантеке','Пожилой пациент'),(17,'Антон Серов','Кәрім Батырхан','Пациент на каталке'),(18,'Галина','Жадра','Лаборант'),(19,'Глеб Зотов','Сейдалиев Абай','Постовой шлюза'),(20,'Нина Мельник','Назарова Лейла','Жена кладовщика'),(21,'Вадим','Шаяхмет Азиз','Пациент скрытой палаты'),(22,'Лида','Кабылова Карина','Пациентка скрытой палаты'),(23,'Ольга','Абакова Рашида','Пациентка психоневрологии'),(24,'Егор Морозов','Шаймердинов Аргын','Пациент, ищущий дочь'),(25,'Алёна Морозова','Жаксымбетова Акбота','Дочь Аргына'),(26,'Саша','Булдыбаев Мурат','Пациент с переносным комплектом'),(27,'Лазарев','Уразов Руслан','Заражённый пациент с рисунком дочери'),(28,'Андрей Лосев','Идырысов Арсен','Погибший ночной техник')]
EXTRAS=[(29,'Дочь Лазарева','Уразова Аяулым','Дочь Руслана'),(30,'Дежурная карантина','Сәрсенова Әсел','Дежурная внешнего поста'),(31,'Хирургический ассистент','Нұртаев Дәурен','Ассистент хирургической группы'),(32,'Офицер у створки','Төлегенов Нұрсұлтан','Охранник, удерживающий дверь'),(33,'Водитель эвакуационной машины','Әбдірахманов Ержан','Водитель'),(34,'Пилот эвакуационного борта','Жұмағұлов Айдос','Пилот'),(35,'Фельдшер эвакуационной группы','Бекенова Інжу','Фельдшер'),(36,'Сотрудник эвакуационной группы','Сапарбек Данияр','Эвакуатор')]
MONSTERS={
'patients':('Первые пациенты','Елестер',['Сусыз','Үнсіз','Діріл','Қисық','Бозарған','Селтек','Түнек','Көлеңке']),
'stitched':('Сшитый','Жамау',['Жамау']),
'hound':('Ищейка','Ізкесер',['Қансоқпақ','Қараіз','Тісқайрақ','Түнжортар']),
'listener':('Слушатель','Тыңшы',['Тыңшы']),
'carrier':('Носитель','Тасымалдаушы',['Ісік','Сызат','Қуыскеуде','Ірің']),
'keeper':('Смотритель','Қарауыл',['Қарауыл']),
'repeater':('Повторяющий','Жаңғырық',['Сыбыр','Қайтаүн']),
'metatransporter':('Переносчик','Сүйреткіш',['Қылтұзақ','Сүйретпе']),
'impostor':('Подмена','Сыңар',['Сыңар']),
'nest':('Гнездо','Ұя',['Ұя']),
'blind_surgeon':('Слепой хирург','Соқыр оташы',['Соқыр оташы']),
'linked':('Связанные','Байлаулылар',['Жіп','Түйін','Ілмек','Шырмау']),
'armoured':('Панцирный','Сауыт',['Сауыт']),
'collector':('Сборщик','Құрастырушы',['Құрастырушы']),
'conductor':('Проводник','Жетелеуші',['Жетелеуші']),
'pursuer':('Преследователь','Қуғыншы',['Қуғыншы']),
'central_organism':('Центральный организм','Өзек',['Өзек']),
'parasite':('Малый организм','Құрт',['Құрт'])}
MONSTERS['transporter']=MONSTERS.pop('metatransporter')
FORMS={}
FIRST_FORMS={}
def forms(old,values):
 FIRST_FORMS[old.split('|')[0]]=dict(zip(old.split('|'),values.split('|')))
 for a,b in FIRST_FORMS[old.split('|')[0]].items():FORMS[a]=b
forms('Алексей|Алексея|Алексею|Алексеем|Лёша|Лёше|Лёшу|Лёши','Бека|Беки|Беке|Бекой|Бека|Беке|Беку|Беки')
forms('Ирина|Ирины|Ирине|Ирину|Ириной|Ира|Иры|Ире|Иру','Аружан|Аружан|Аружан|Аружан|Аружан|Аружан|Аружан|Аружан|Аружан')
forms('Вера|Веры|Вере|Веру|Верой','Гүлсезім|Гүлсезім|Гүлсезім|Гүлсезім|Гүлсезім')
forms('Виктор|Виктора|Виктору|Виктором','Талғат|Талғата|Талғату|Талғатом')
forms('Марина|Марины|Марине|Марину|Мариной','Ұлбала|Ұлбалы|Ұлбале|Ұлбалу|Ұлбалой')
forms('Никита|Никиты|Никите|Никиту|Никитой','Маржан|Маржан|Маржан|Маржан|Маржан')
forms('Борис|Бориса|Борису|Борисом','Қасым|Қасыма|Қасыму|Қасымом')
forms('Павел|Павла|Павлу|Павлом','Абылай|Абылая|Абылаю|Абылаем')
forms('Денис|Дениса|Денису|Денисом','Тохтар|Тохтара|Тохтару|Тохтаром')
forms('Артём|Артёма|Артёму|Артёмом','Саловат|Саловата|Саловату|Саловатом')
forms('Олег|Олега|Олегу|Олегом','Ерболат|Ерболата|Ерболату|Ерболатом')
forms('Софья|Софьи|Софье|Софью|Софьей','Аягөз|Аягөз|Аягөз|Аягөз|Аягөз')
forms('Семён|Семёна|Семёну|Семёном','Адылбек|Адылбека|Адылбеку|Адылбеком')
forms('Михаил|Михаила|Михаилу|Михаилом','Ерик|Ерика|Ерику|Ериком')
forms('Рая|Раи|Рае|Раю|Раей','Айсулу|Айсулу|Айсулу|Айсулу|Айсулу')
forms('Леонид|Леонида|Леониду|Леонидом','Жантеке|Жантеке|Жантеке|Жантеке')
forms('Антон|Антона|Антону|Антоном','Батырхан|Батырхана|Батырхану|Батырханом')
forms('Галина|Галину|Галины|Галине','Жадра|Жадру|Жадры|Жадре')
forms('Глеб|Глеба|Глебу|Глебом','Абай|Абая|Абаю|Абаем')
forms('Нина|Нины|Нине|Нину|Ниной','Лейла|Лейлы|Лейле|Лейлу|Лейлой')
forms('Вадим|Вадима|Вадиму|Вадимом','Азиз|Азиза|Азизу|Азизом')
forms('Лида|Лиды|Лиде|Лиду|Лидой','Карина|Карины|Карине|Карину|Кариной')
forms('Ольга|Ольги|Ольге|Ольгу|Ольгой','Рашида|Рашиды|Рашиде|Рашиду|Рашидой')
forms('Егор|Егора|Егору|Егором','Аргын|Аргына|Аргыну|Аргыном')
forms('Алёна|Алёны|Алёне|Алёну|Алёной','Акбота|Акботы|Акботе|Акботу|Акботой')
forms('Саша|Саши|Саше|Сашу|Сашей','Мурат|Мурата|Мурату|Муратом')
forms('Андрей|Андрея|Андрею|Андреем','Арсен|Арсена|Арсену|Арсеном')
# Surname-only mentions refer to the same stable character, not a new person.
for old,new in [('Ланская','Мусалимова'),('Рудин','Файзулаев'),('Ковалёв','Чиликбаев'),('Дёмин','Асубаев'),('Орлова','Аргымбаева'),('Громов','Имашев'),('Серов','Кәрім'),('Зотов','Сейдалиев'),('Лазарев','Уразов'),('Соловьёв','Маулеш'),('Фролов','Жарылқасын'),('Астахов','Қайсенов'),('Беляев','Оразгалиев'),('Левин','Жуманбаев'),('Зуев','Оразданов'),('Лосев','Идырысов')]:
 FORMS[old]=new
 if old.endswith('а'):
  for end,ne in [('ой','ой'),('у','у')]:FORMS[old[:-1]+end]=new[:-1]+ne
 else:
  for end in ['а','у','ом','ым']:FORMS[old+end]=new+end if not new.endswith(('ім','сын')) else new
FORMS.update({'Егорова':'Абдухаримов','Каримова':'Жанабекова','Морозова':'Жаксымбетова','Мельник':'Шардарбек','Соловьёва':'Құрбанәлі','Соловьёвой':'Құрбанәлі','Соловьёву':'Маулешу','Павлович':'аға'})
# Full names must be replaced before individual tokens; listed spellings are exact user input.
FULL={old:new for _,old,new,_ in PEOPLE}
FULL.update({'Егор Морозов':'Шаймердинов Аргын','Леонид Павлович':'Жантеке аға'})
# Resolve full-name cases before ambiguous surname-only forms (father/daughter, brother/sister).
SURNAME_FORMS={
'Соловьёв':['Соловьёв','Соловьёва','Соловьёву','Соловьёвым','Соловьёве'],
'Соловьёва':['Соловьёва','Соловьёвой','Соловьёву'],
'Егорова':['Егорова','Егоровой','Егорову'],
'Каримова':['Каримова','Каримовой','Каримову'],
'Морозов':['Морозов','Морозова','Морозову','Морозовым','Морозове'],
'Морозова':['Морозова','Морозовой','Морозову'],
'Мельник':['Мельник','Мельника','Мельнику','Мельником'],
'Ланская':['Ланская','Ланской','Ланскую'],
'Орлова':['Орлова','Орловой','Орлову']}
for _,old,new,_ in PEOPLE:
 parts=old.split()
 if len(parts)!=2:continue
 first,last=parts;new_last,new_first=new.split();variants=SURNAME_FORMS.get(last,[last,last+'а',last+'у',last+'ом',last+'ым',last+'е'])
 for old_first,new_form in FIRST_FORMS.get(first,{first:new_first}).items():
  for old_last in variants:FULL[old_first+' '+old_last]=new_last+' '+new_form
 FULL[old]=new
FULL['Дочь Лазарева']='Аяулым'
FORMS.update({'Ланской':'Мусалимовой','Ланскую':'Мусалимову','Лазаревой':'Уразова'})
FULL_RE=re.compile(r'(?<!\w)('+ '|'.join(re.escape(v) for v in sorted(FULL,key=len,reverse=True))+r')(?!\w)')
FORMS.update({'Irina':'Аружан','Alexey':'Бека','Aleksey':'Бека','Viktor':'Талғат','Victor':'Талғат','Nikita':'Маржан','Vera':'Гүлсезім','Rudin':'Файзулаев','Lanskaya':'Мусалимова','Solovyov':'Маулеш','Solovyova':'Құрбанәлі'})
WORD_RE=re.compile(r'(?<!\w)('+ '|'.join(re.escape(v) for v in sorted(FORMS,key=len,reverse=True))+r')(?!\w)')
MOB_WORDS={v[0]:v[1] for v in MONSTERS.values() if v[0] not in ('Носитель','Малый организм')}
MOB_WORDS.update({'Сшитого':'Жамау','Сшитому':'Жамау','Сшитым':'Жамау','Ищейки':'Ізкесер','Ищейку':'Ізкесер','Ищейкой':'Ізкесер','Ищеек':'Ізкесер','Слушателя':'Тыңшы','Слушателю':'Тыңшы','Смотрителя':'Қарауыл','Смотрителю':'Қарауыл','Повторяющего':'Жаңғырық','Переносчика':'Сүйреткіш','Подмены':'Сыңар','Подмену':'Сыңар','Подменой':'Сыңар','Гнезда':'Ұя','Гнезду':'Ұя','Слепого хирурга':'Соқыр оташы','Слепому хирургу':'Соқыр оташы','Панцирного':'Сауыт','Сборщика':'Құрастырушы','Проводника':'Жетелеуші','Преследователя':'Қуғыншы'})
MOB_WORDS.update({'Носитель':'Тасымалдаушы','Носителя':'Тасымалдаушы','Носителю':'Тасымалдаушы','Носителем':'Тасымалдаушы','Переносчике':'Сүйреткіш','Повторяющему':'Жаңғырық','Повторяющим':'Жаңғырық','Подмене':'Сыңар','Связанный':'Байлаулы','Связанных':'Байлаулылар'})
MOB_RE=re.compile(r'(?<!\w)('+ '|'.join(re.escape(v) for v in sorted(MOB_WORDS,key=len,reverse=True))+r')(?!\w)')
def rename_text(text):
 text=FULL_RE.sub(lambda m:FULL[m[0]],text);text=WORD_RE.sub(lambda m:FORMS[m[0]],text);text=MOB_RE.sub(lambda m:MOB_WORDS[m[0]],text)
 text=text.replace('Дочь Уразова:','Аяулым:').replace('Дежурная:','Әсел:').replace('Санитар, раненый свидетель','Санитарка, раненая свидетельница')
 return text
# These are identifiers/provenance/contracts, never display strings.
KEEP={'id','source_id','actor_guid','person_guid','species','kind','site','zone','source_subroom','subroom','source','source_file','source_kind','source_pdf_sha256','source_map_sha256','source_sha256','choice_key','requires','effects','source_reference','mount','status','role','type','implementation','behaviour','combat_class','kill_condition','side_rewards','final_plan'}
def walk(value,key=''):
 if isinstance(value,str):return value if key in KEEP else rename_text(value)
 if isinstance(value,list):return [walk(v,key) for v in value]
 if isinstance(value,dict):return {k:(v if k in KEEP else walk(v,k)) for k,v in value.items()}
 return value
NPC_NUM={'PERSON_2':4,'PERSON_3':2,'PERSON_4':6,'PERSON_6':9,'NEW_NPC_MARINA':5,'NEW_NPC_BORIS':7,'NEW_NPC_PAVEL':8,'NEW_NPC_RAYA':15,'NEW_NPC_LEONID':16,'NEW_NPC_SEROV':17,'NEW_NPC_GALINA':18,'NEW_NPC_ZOTOV':19,'NEW_NPC_ARTYOM':10,'NEW_NPC_SEMYON':13,'NEW_NPC_ORLOVA':12,'NEW_NPC_DEMIN':11,'NEW_NPC_001':14,'NEW_NPC_NINA':20,'NEW_NPC_OLGA':23,'NEW_NPC_EGOR':24,'NEW_NPC_SASHA':26}
def apply_names(data):
 result=walk(copy.deepcopy(data));result['names_revision']=36
 humans=[dict(number=n,old_name=o,name=name,given_name=name.split()[-1],role=role,origin='user' if n<=28 else 'proposed') for n,o,name,role in PEOPLE+EXTRAS]
 result['name_registry']=dict(language='kk',people=humans,monster_types=[dict(species=k,old_name=v[0],nickname=v[1]) for k,v in MONSTERS.items()],extra_names_are_proposals=True)
 bynum={q['number']:q for q in humans}
 for q in result['gameplay']['placements']:
  if q['id'] in NPC_NUM:q['label']=bynum[NPC_NUM[q['id']]]['name']
 # Each ordinary enemy gets its own nickname; alternate anchors of one unique actor share it.
 individuals={};counts={}
 for q in result['gameplay']['mobs']:
  species=q['species'];guid=q['actor_guid'];kind=MONSTERS[species]
  if guid not in individuals:
   index=counts.get(species,0);counts[species]=index+1;individuals[guid]=dict(actor_guid=guid,species=species,nickname=kind[2][index],type_nickname=kind[1],anchor_ids=[])
  individuals[guid]['anchor_ids'].append(q['id']);q['name']=individuals[guid]['nickname'];q['type_nickname']=kind[1]
 result['name_registry']['monster_individuals']=list(individuals.values())
 for row in result.get('bestiary',[]):
  if isinstance(row,dict) and row.get('species') in MONSTERS:row['name']=MONSTERS[row['species']][1]

 # Preserve old surname matches used by optional NPC questions while updating speakers.
 for row in result['questions']:
  for person in humans:
   if row[0] in (person['given_name'],person['name'].split()[0]):row[0]=person['given_name']
 return result
if __name__=='__main__':
 p=ROOT/'unreal/EchoWard/Content/CampaignData/echo_scenario_v35.json';d=apply_names(json.loads(p.read_text(encoding='utf-8')));p.write_text(json.dumps(d,ensure_ascii=False,separators=(',',':')),encoding='utf-8');print('names36 applied')
