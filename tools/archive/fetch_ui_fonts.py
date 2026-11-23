"""Fetch redistributable OFL fonts from the official Google Fonts repository."""
from pathlib import Path
import hashlib
import json
import urllib.request
ROOT = Path(__file__).resolve().parents[1] / 'assets/ui/fonts'
ROOT.mkdir(parents=True, exist_ok=True)
items = [('literata', 'Literata%5Bopsz,wght%5D.ttf', 'Literata.ttf'),
         ('notosans', 'NotoSans%5Bwdth,wght%5D.ttf', 'NotoSans.ttf')]
manifest = []
for family, remote, name in items:
    for source, local in [(remote, name), ('OFL.txt', family + '-OFL.txt')]:
        url = f'https://raw.githubusercontent.com/google/fonts/main/ofl/{family}/{source}'
        with urllib.request.urlopen(url, timeout=45) as response:
            data = response.read()
        (ROOT / local).write_bytes(data)
        manifest.append({'file': local, 'url': url, 'sha256': hashlib.sha256(data).hexdigest(), 'license': 'OFL-1.1'})
(ROOT / 'SOURCES.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print('Downloaded Literata and Noto Sans, including original OFL licenses.')
