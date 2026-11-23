"""Verified CC0 room dressing. Powered by Poly Haven's public API."""
import concurrent.futures
import json
from pathlib import Path
from fetch_detail_assets import read, fetch

ROOT = Path(__file__).resolve().parents[1] / 'assets'
MODELS = ['sofa_03', 'SchoolChair_01', 'painted_wooden_cabinet_02', 'desk_lamp_arm_01']
TEXTURES = ['terrazzo_tiles', 'old_linoleum_flooring_01', 'painted_plaster_wall',
            'stretch_poplin', 'blue_plaster_weathered']

if __name__ == '__main__':
    model_path = ROOT / 'models/polyhaven/SOURCE_MANIFEST.json'
    manifest = {x['id']: x for x in json.loads(model_path.read_text(encoding='utf-8'))}
    texture_manifest, jobs = [], []
    for kind, slugs in [('models', MODELS), ('textures', TEXTURES)]:
        catalog = json.loads(read('https://api.polyhaven.com/assets?t=' + kind))
        for slug in slugs:
            metadata = json.loads(read('https://api.polyhaven.com/files/' + slug))
            folder = ROOT / kind / 'polyhaven' / slug
            entry = {'id': slug, 'title': catalog[slug]['name'], 'authors': catalog[slug]['authors'],
                     'source': 'https://polyhaven.com/a/' + slug, 'license': 'CC0-1.0', 'resolution': '2k'}
            if kind == 'models':
                item = metadata['gltf']['2k']['gltf']
                jobs.append((folder / (slug + '_2k.gltf'), item))
                for name, dependency in item['include'].items():
                    path = (folder / name).resolve()
                    if not path.is_relative_to(folder.resolve()): raise RuntimeError('Unsafe asset path')
                    jobs.append((path, dependency))
                entry['files'] = item
                manifest[slug] = entry
            else:
                entry['files'] = {}
                for channel, key in [('color', 'Diffuse'), ('normal', 'nor_gl'), ('roughness', 'Rough')]:
                    item = metadata[key]['2k']['jpg']
                    jobs.append((folder / (channel + '.jpg'), item))
                    entry['files'][channel + '.jpg'] = item
                texture_manifest.append(entry)
    with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
        list(pool.map(fetch, jobs))
    model_path.write_text(json.dumps(list(manifest.values()), indent=2), encoding='utf-8')
    texture_path = ROOT / 'textures/polyhaven/SOURCE_MANIFEST.json'
    texture_path.write_text(json.dumps(texture_manifest, indent=2), encoding='utf-8')
    print('Verified', len(jobs), 'files:', len(MODELS), 'models and', len(TEXTURES), 'PBR materials')
