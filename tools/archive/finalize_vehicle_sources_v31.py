from pathlib import Path
from PIL import Image
import json
p=Path('unreal/EchoWard/RawAssets/EchoCarsV31/manifest.json');d=json.loads(p.read_text());fixed=[]
for spec in d['materials'].values():
 for k in ['albedo','normal','roughness_map','metallic_map','alpha_file']:
  if not spec.get(k):continue
  f=Path(spec[k])
  with Image.open(f) as im:
   expected={'.png':'PNG','.jpg':'JPEG','.jpeg':'JPEG','.tga':'TGA','.dds':'DDS','.bmp':'BMP','.webp':'WEBP'}.get(f.suffix.lower())
   if im.format!=expected:
    q=f.with_name(f.stem+'_ue.png')
    if not q.exists():im.save(q,'PNG')
    spec[k]=str(q.resolve());fixed.append(f.name)
for row in d['assets']:
 if row['nanite'] and (len(row['materials'])>64 or any(d['materials'][k]['alpha']=='BLEND' for k in row['materials'])):row['nanite']=False
p.write_text(json.dumps(d,ensure_ascii=False,indent=2));print('Reformatted textures',len(set(fixed)))
