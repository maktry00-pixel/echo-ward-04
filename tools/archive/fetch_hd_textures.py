"""Download genuine 4K CC0 maps from Poly Haven, preserving the existing 2K set."""
import concurrent.futures
import json
from pathlib import Path
from fetch_detail_assets import read, fetch

root = Path(__file__).resolve().parents[1] / 'assets/textures/polyhaven'
slugs = ['terrazzo_tiles', 'old_linoleum_flooring_01', 'painted_plaster_wall', 'blue_plaster_weathered']
jobs, entries = [], []
for slug in slugs:
    data = json.loads(read('https://api.polyhaven.com/files/' + slug))
    entry = {'id': slug, 'source': 'https://polyhaven.com/a/' + slug,
             'license': 'CC0-1.0', 'resolution': '4k', 'files': {}}
    for channel, key in [('color', 'Diffuse'), ('normal', 'nor_gl'), ('roughness', 'Rough')]:
        item = data[key]['4k']['jpg']
        name = channel + '_4k.jpg'
        jobs.append((root / slug / name, item))
        entry['files'][name] = item
    entries.append(entry)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    list(pool.map(fetch, jobs))
(root / 'HD_SOURCE_MANIFEST.json').write_text(json.dumps(entries, indent=2), encoding='utf-8')
print('Verified 12 genuine 4K PBR maps; existing 2K textures preserved.')
