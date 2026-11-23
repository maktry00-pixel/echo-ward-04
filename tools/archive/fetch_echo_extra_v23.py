import hashlib,json,urllib.parse,urllib.request
from pathlib import Path
from fetch_echo_real_assets_v23 import OUT,R,get,metadata,download
rows=[];catalog=json.loads((OUT/'polyhaven_catalog.json').read_text(encoding='utf-8-sig'))
for id,slug in [(825,'dining_table'),(826,'coffee_table_round_01'),(827,'side_table_01')]:
 meta=metadata(slug);file=meta['gltf']['2k']['gltf'];folder=OUT/'models'/slug;main=folder/(slug+'_2k.gltf');download((main,file))
 for name,spec in file['include'].items():download((folder/name,spec))
 rows.append(dict(id=id,slug=slug,file=str(main),source='https://polyhaven.com/a/'+slug,license='CC0-1.0',authors=catalog[slug]['authors'],title=catalog[slug]['name'],resolution='2k',publisher_files=file))
for id,slug in [(855,'service_pistol'),(856,'fish_knife')]:
 old=json.loads((R/'assets/models/polyhaven/SOURCE_MANIFEST.json').read_text(encoding='utf8'));a=next(x for x in old if x['id']==slug);f=next((R/'assets/models/polyhaven'/slug).glob('*.gltf'));rows.append(dict(id=id,slug=slug,file=str(f),source=a['source'],license=a['license'],authors=a.get('authors',{'Poly Haven':'Model and textures'}),title=a.get('title',slug),resolution=a['resolution'],publisher_files=a['files'],existing_verified_source=True))
slug='DiffuseTransmissionTeacup';base='https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/main/Models/'+slug+'/';folder=OUT/'models'/slug;folder.mkdir(exist_ok=True);main=folder/(slug+'.gltf')
def fetch(url,path):
 if not path.exists():path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(get(url))
 return path.read_bytes()
doc=json.loads(fetch(base+'glTF/'+slug+'.gltf',main));files=[]
for row in doc.get('images',[])+doc.get('buffers',[]):
 uri=row.get('uri','')
 if not uri or uri.startswith('data:'):continue
 data=fetch(base+'glTF/'+urllib.parse.quote(uri,safe='/'),folder/uri);files.append(dict(file=str(folder/uri),sha256=hashlib.sha256(data).hexdigest()))
for name in ['README.md','LICENSE.md']:fetch(base+name,folder/name)
rows.append(dict(id=854,slug=slug,file=str(main),source='https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/'+slug,license='CC0-1.0',authors={'Polyhaven.com; Eric Chadwick':'Models and textures'},title='Ceramic teacup and saucer',resolution='source',files=files))
(OUT/'EXTRA_MANIFEST.json').write_text(json.dumps(dict(version=23,assets=rows),ensure_ascii=False,indent=2),encoding='utf8');print('EXTRA_SOURCES',len(rows),flush=True)
