"""Download verified CC0 close-up assets. Powered by Poly Haven's public API."""
import concurrent.futures
import hashlib
import json
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'assets' / 'models' / 'polyhaven'
ASSETS = ['service_pistol', 'fish_knife', 'circuit_board', 'portable_cassette_player',
          'small_plastic_torch', 'medical_tape', 'painted_wooden_nightstand',
          'vintage_wooden_drawer_01', 'medical_box', 'crowbar_01']
HEADERS = {'User-Agent': 'Mozilla/5.0', 'Referer': 'https://polyhaven.com/'}

def read(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=HEADERS), timeout=60) as response:
        return response.read()

def fetch(job):
    path, item = job
    if path.exists() and hashlib.md5(path.read_bytes()).hexdigest() == item['md5']:
        return
    for suffix in ['', '?download=1', '?verified=2']:
        try:
            data = read(item['url'] + suffix)
        except OSError:
            continue
        if hashlib.md5(data).hexdigest() == item['md5']:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            return
    raise RuntimeError('Hash mismatch: ' + str(path))

if __name__ == '__main__':
    path = ROOT / 'SOURCE_MANIFEST.json'
    manifest = {x['id']: x for x in json.loads(path.read_text(encoding='utf-8'))}
    catalog = json.loads(read('https://api.polyhaven.com/assets?t=models'))
    jobs = []
    for slug in ASSETS:
        item = json.loads(read('https://api.polyhaven.com/files/' + slug))['gltf']['2k']['gltf']
        folder = ROOT / slug
        jobs.append((folder / (slug + '_2k.gltf'), item))
        for name, dependency in item['include'].items():
            target = (folder / name).resolve()
            if not target.is_relative_to(folder.resolve()):
                raise RuntimeError('Unsafe asset path')
            jobs.append((target, dependency))
        manifest[slug] = {'id': slug, 'source': 'https://polyhaven.com/a/' + slug,
                          'license': 'CC0-1.0', 'resolution': '2k', 'files': item,
                          'title': catalog[slug]['name'], 'authors': catalog[slug]['authors']}
    with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
        list(pool.map(fetch, jobs))
    path.write_text(json.dumps(list(manifest.values()), indent=2), encoding='utf-8')
    print('Verified', len(jobs), 'files for', len(ASSETS), '2K assets')
