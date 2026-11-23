"""Import an unedited 1080p Unreal capture, not the reference mockup."""
from pathlib import Path
import unreal as u
root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
source = root.parents[1] / 'artifacts/unreal_save_menu/save_room_1080.png'
task = u.AssetImportTask()
for key, value in dict(filename=str(source), destination_path='/Game/Campaign/Menu',
                       destination_name='T_About_ward', automated=True, save=True,
                       replace_existing=True).items():
    task.set_editor_property(key, value)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = u.load_asset('/Game/Campaign/Menu/T_About_ward')
if not texture:
    raise RuntimeError('About photograph import failed')
texture.set_editor_property('lod_group', u.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('mip_gen_settings', u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
u.EditorAssetLibrary.save_loaded_asset(texture)
