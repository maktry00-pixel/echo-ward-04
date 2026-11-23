"""Fetch individually licensed, high-detail Khronos sample artwork."""
import hashlib,json,urllib.request,urllib.parse
from pathlib import Path
R=Path(__file__).resolve().parents[1];OUT=R/'assets/external/quality_v23';entries=[]
for asset,id,author,lic in [('CarConcept',850,'Eric Chadwick / Darmstadt Graphics Group GmbH','CC-BY-4.0'),('CommercialRefrigerator',851,'Sean Thomas; Eric Chadwick / Darmstadt Graphics Group GmbH','CC-BY-4.0'),('SheenChair',852,'Eric Chadwick / Wayfair, LLC','CC0-1.0'),('ToyCar',853,'Guido Odendahl; Eric Chadwick','CC0-1.0')]:
 base='https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/main/Models/'+asset+'/'
 folder=OUT/'models'/asset;folder.mkdir(parents=True,exist_ok=True)
 def get(url,path):
  if path.exists():return path.read_bytes()
  data=urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'EchoWardAssetIntegration/23'}),timeout=90).read();path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data);return data
 for text in ['README.md','LICENSE.md']:get(base+text,folder/text)
 kind='glTF';file=folder/(asset+'.gltf');raw=get(base+kind+'/'+asset+'.gltf',file);doc=json.loads(raw);files=[]
 for row in doc.get('buffers',[])+doc.get('images',[]):
  uri=row.get('uri','')
  if not uri or uri.startswith('data:'):continue
  target=(folder/uri).resolve();assert target.is_relative_to(folder.resolve())
  data=get(base+kind+'/'+urllib.parse.quote(uri,safe='/'),target);files.append(dict(file=str(target),sha256=hashlib.sha256(data).hexdigest(),bytes=len(data)))
 get(base+'screenshot/'+('screenshot_large.jpg' if asset=='ToyCar' else 'screenshot-large.jpg' if asset=='SheenChair' else 'screenshot_Large.jpg'),folder/'source_preview.jpg')
 entries.append(dict(id=id,slug=asset,file=str(file),source='https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/'+asset,license=lic,authors={author:'Model and textures'},title=asset,resolution='source',files=files,modifications='Static metric conversion; material translation. CarConcept logo/plate graphics omitted; this is not an endorsement.'))
 print(asset,len(files),sum(f['bytes'] for f in files),flush=True)
(OUT/'SAMPLES_MANIFEST.json').write_text(json.dumps(dict(version=23,assets=entries),ensure_ascii=False,indent=2),encoding='utf8')
