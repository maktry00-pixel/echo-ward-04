import unreal as u,json
from pathlib import Path
p=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
m=u.load_asset('/Game/Campaign/HorrorV28/M_HorrorTrace28B');assert m
info=dict(material_methods=[n for n in dir(m) if 'editor' in n.lower() or 'express' in n.lower()],editing_methods=[n for n in dir(u.MaterialEditingLibrary) if 'express' in n.lower()],object_methods=[n for n in dir(u) if 'object' in n.lower() and ('iter' in n.lower() or 'get_' in n.lower())]);fixed=0
try:
 nodes=[]
 if hasattr(u,'ObjectIterator'):nodes=list(u.ObjectIterator(u.MaterialExpressionCustom))
 elif hasattr(u,'get_objects_of_class'):nodes=u.get_objects_of_class(u.MaterialExpressionCustom)
 for n in nodes:
  if n.get_path_name().startswith(m.get_path_name()+':') or n.get_outer()==m:
   code=n.get_editor_property('code');new=code.replace('float line=','float strokeCoverage=').replace('mask,line*','mask,strokeCoverage*')
   if new!=code:n.set_editor_property('code',new);fixed+=1
 if fixed:u.MaterialEditingLibrary.recompile_material(m);assert u.EditorAssetLibrary.save_loaded_asset(m)
 info.update(fixed=fixed,nodes=len(nodes))
finally:
 (p/'Saved/Prototype/trace28_repair.json').write_text(json.dumps(info,indent=2),encoding='utf8');u.SystemLibrary.quit_editor()
