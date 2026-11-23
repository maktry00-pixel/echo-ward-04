"""Import original foley and native gameplay captures; no reference image in the build."""
from pathlib import Path
import json
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
SOURCE=ROOT/'RawAssets/Menu'
tools=u.AssetToolsHelpers.get_asset_tools()
assets=[]
for p in sorted(SOURCE.iterdir()):
    if p.suffix not in ['.wav','.png']:continue
    audio=p.suffix=='.wav'
    dest='/Game/Campaign/Audio' if audio else '/Game/Campaign/Menu'
    name=('A_' if audio else 'T_Menu_')+p.stem
    task=u.AssetImportTask()
    for k,v in dict(filename=str(p),destination_path=dest,destination_name=name,automated=True,save=True,replace_existing=True).items():task.set_editor_property(k,v)
    tools.import_asset_tasks([task])
    a=u.load_asset(dest+'/'+name)
    if not a:raise RuntimeError('Missing imported menu asset '+name)
    if audio:
        a.set_editor_property('looping',p.stem in ['menu_vent','menu_electric'])
        a.set_editor_property('volume',1.0)
    else:
        a.set_editor_property('lod_group',u.TextureGroup.TEXTUREGROUP_UI)
        a.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_EDITOR_ICON)
        a.set_editor_property('mip_gen_settings',u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    if not u.EditorAssetLibrary.save_loaded_asset(a):raise RuntimeError('Save failed '+name)
    assets.append(a.get_path_name())
(ROOT/'Saved/Prototype/title_media_import.json').write_text(json.dumps(dict(success=True,assets=assets),indent=2))
