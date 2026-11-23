from pathlib import Path
from PIL import Image
import json,collections
R=Path('D:/Projects/ECHO');D=json.loads((R/'unreal/EchoWard/RawAssets/EchoCarsV31/manifest.json').read_text());hero_keys={k for a in D['assets'] if 2000<=a['id']<=2011 for k in a['materials']};rows=[]
for key,s in D['materials'].items():
 for channel in ['albedo','normal','roughness_map','metallic_map','alpha_file']:
  if not s.get(channel):continue
  p=Path(s[channel]);im=Image.open(p);rows.append(dict(key=key,name=s['name'],hero=key in hero_keys,channel=channel,file=str(p),size=list(im.size)))
by=collections.Counter(tuple(v['size']) for v in rows);out=dict(materials=len(D['materials']),hero_materials=len(hero_keys),texture_refs=len(rows),sizes={str(k):n for k,n in by.items()},hero=[v for v in rows if v['hero']],all=rows)
(R/'artifacts/echo_car_quality_v32/source_textures.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));print('Source sizes',dict(by));print('Hero texture references',len(out['hero']));print(json.dumps([{k:v[k] for k in ['name','channel','size','file']} for v in out['hero']],ensure_ascii=False,indent=2))
