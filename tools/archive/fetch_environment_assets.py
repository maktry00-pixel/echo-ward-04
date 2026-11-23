"""Fetch CC0 props from the official Poly Haven API, verifying every file hash."""
import concurrent.futures
import hashlib
import json
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SLUGS = ['steel_frame_shelves_01', 'painted_wooden_nightstand', 'wheelchair_01',
         'medical_box', 'plastic_jerrycan', 'metal_office_desk', 'old_bed_frame',
         'crowbar_01', 'vintage_wooden_drawer_01']
HEADERS = {'User-Agent': 'Mozilla/5.0', 'Referer': 'https://polyhaven.com/'}

def read(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=HEADERS), timeout=60) as response:
        return response.read()

def fetch(job):
    path, item = job
    if path.exists() and hashlib.md5(path.read_bytes()).hexdigest() == item['md5']:
        return
    data = read(item['url'])
    if hashlib.md5(data).hexdigest() != item['md5']:
        data = read(item['url'] + ('&' if '?' in item['url'] else '?') + 'download=1')
    if hashlib.md5(data).hexdigest() != item['md5']:
        raise RuntimeError('Hash mismatch: ' + str(path))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)

manifest, jobs = [], []
previous_file = ROOT / 'assets' / 'models' / 'polyhaven' / 'SOURCE_MANIFEST.json'
previous = {x['id']: x for x in json.loads(previous_file.read_text(encoding='utf-8'))} if previous_file.exists() else {}
manifest = [item for slug, item in previous.items() if slug not in SLUGS]
for slug in SLUGS:
    metadata = json.loads(read('https://api.polyhaven.com/files/' + slug))
    resolution = previous.get(slug, {}).get('resolution', '1k')
    item = metadata['gltf'][resolution]['gltf']
    folder = ROOT / 'assets' / 'models' / 'polyhaven' / slug
    for name in item['include']:
        if not (folder / name).resolve().is_relative_to(folder.resolve()):
            raise RuntimeError('Invalid asset path')
    jobs.append((folder / (slug + '_' + resolution + '.gltf'), item))
    jobs.extend((folder / name, dependency) for name, dependency in item['include'].items())
    manifest.append({**{k: v for k, v in previous.get(slug, {}).items() if k in ('authors', 'title')}, 'id': slug, 'source': 'https://polyhaven.com/a/' + slug,
                     'license': 'CC0-1.0', 'resolution': resolution, 'files': item})
with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
    list(pool.map(fetch, jobs))
folder = ROOT / 'assets' / 'models' / 'polyhaven'
(folder / 'SOURCE_MANIFEST.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print('Verified', len(jobs), 'files for', len(SLUGS), 'CC0 models')
