"""Append new verified sources without touching OBJ files already imported."""
import json
from pathlib import Path
R=Path(__file__).resolve().parents[1];script=R/'tools/prepare_echo_real_v23.py';scope={'__file__':str(script)}
exec(script.read_text(encoding='utf8').split('for source in sources:export(source)')[0],scope)
out=scope['OUT'];current=json.loads((out/'manifest.json').read_text(encoding='utf8'));scope['assets']=current['assets'];scope['materials']=current['materials'];ids={x['id'] for x in current['assets']}
for a in json.loads((R/'assets/external/quality_v23/EXTRA_MANIFEST.json').read_text(encoding='utf8'))['assets']:
 if a['id'] not in ids:scope['export'](a)
(out/'manifest.json').write_text(json.dumps(dict(version=23,units='cm',assets=scope['assets'],materials=scope['materials']),ensure_ascii=False,indent=2,default=lambda x:x.item()),encoding='utf8');print('TOTAL',len(scope['assets']),len(scope['materials']))
