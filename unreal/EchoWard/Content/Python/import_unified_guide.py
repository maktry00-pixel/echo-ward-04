"""Import the nine unedited native campaign photographs with one texture policy."""
from pathlib import Path
import json
import shutil
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
source = root / 'Saved/GuideMediaSession/Saved/Prototype/GuideMedia'
report = json.loads((source / 'capture.json').read_text('utf-8-sig'))
assert report['success'] and report['count'] == 9
originals = root / 'RawAssets/UnifiedGuide'
originals.mkdir(parents=True, exist_ok=True)
assets = []
for view in report['views']:
    name = view['id']
    image = originals / (name + '.png')
    shutil.copy2(source / image.name, image)
    task = u.AssetImportTask()
    for key, value in dict(filename=str(image), destination_path='/Game/Campaign/Guide',
                           destination_name='T_Guide_' + name, automated=True,
                           save=True, replace_existing=True).items():
        task.set_editor_property(key, value)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = u.load_asset('/Game/Campaign/Guide/T_Guide_' + name)
    if not texture:
        raise RuntimeError('Missing guide photograph: ' + name)
    texture.set_editor_property('lod_group', u.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('mip_gen_settings', u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    u.EditorAssetLibrary.save_loaded_asset(texture)
    assets.append(texture.get_path_name())
shutil.copy2(source / 'capture.json', originals / 'capture.json')
(root / 'Saved/Prototype/unified_guide_import.json').write_text(
    json.dumps(dict(success=True, assets=assets, capture=report), ensure_ascii=False, indent=2), 'utf8')
