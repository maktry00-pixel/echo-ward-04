"""The glTF JPG package loses alpha: fetch the official separate masks."""
import json
from pathlib import Path
from fetch_echo_real_assets_v23 import OUT,download
import concurrent.futures
manifest=json.loads((OUT/'SOURCE_MANIFEST.json').read_text(encoding='utf8'));jobs=[];out={}
for a in manifest['assets']:
 p=OUT/('files_'+a['slug']+'.json')
 if not p.exists():continue
 d=json.loads(p.read_text(encoding='utf8'));masks=[]
 for name,spec in d.items():
  if not name.lower().endswith('alpha'):continue
  item=spec.get(a['resolution'],{}).get('png')
  if not item:continue
  path=OUT/'models'/a['slug']/'masks'/(name+'.png');jobs.append((path,item));masks.append(dict(channel=name,file=str(path),publisher=item))
 if masks:out[a['slug']]=masks
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:list(pool.map(download,jobs))
(OUT/'ALPHA_MANIFEST.json').write_text(json.dumps(out,indent=2),encoding='utf8');print('VERIFIED_ALPHA_MASKS',len(jobs),flush=True)
