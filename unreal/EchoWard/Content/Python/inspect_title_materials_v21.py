import unreal as u,json
from pathlib import Path
r={}
for key in ('4ae6673fcc8b19e7','05227396fb5ca2d8'):
 m=u.load_asset('/Game/Campaign/Materials/MI_'+key)
 r[key]=dict(parent=str(m.get_editor_property('parent')),textures=[str(x) for x in m.get_editor_property('texture_parameter_values')],vectors=[str(x) for x in m.get_editor_property('vector_parameter_values')])
Path(u.Paths.project_saved_dir(),'Prototype/title_materials_v21.json').write_text(json.dumps(r,indent=2))
u.SystemLibrary.quit_editor()
