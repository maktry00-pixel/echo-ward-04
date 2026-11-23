import unreal as u
from pathlib import Path
import json
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
rows=[]
for key in ['527b19fd5563fa6d','668529dd97668acf','05227396fb5ca2d8']:
    m=u.load_asset('/Game/Campaign/Materials/MI_'+key)
    row={'id':key,'parent':m.get_editor_property('parent').get_path_name()}
    for name in ['Albedo','Normal','RoughnessMap']:
        t=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(m,name);row[name]=t.get_path_name() if t else None
    row['tiling']=str(u.MaterialEditingLibrary.get_material_instance_vector_parameter_value(m,'Tiling'));rows.append(row)
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Campaign/Maps/Campaign')
actors=[]
groups=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    for c in a.get_components_by_class(u.HierarchicalInstancedStaticMeshComponent):
        m=c.get_material(0)
        if m and '527b19fd' in m.get_path_name():
            mesh=c.get_editor_property('static_mesh')
            groups.append({'actor':a.get_actor_label(),'material':m.get_path_name(),'mesh':mesh.get_path_name(),'sections':mesh.get_num_sections(0),'instances':c.get_instance_count()})
    if isinstance(a,u.StaticMeshActor):
        mesh=a.static_mesh_component.get_editor_property('static_mesh')
        actors.append({'label':a.get_actor_label(),'mesh':mesh.get_path_name() if mesh else None,'position':str(a.get_actor_location()),'scale':str(a.get_actor_scale3d())})
(root/'Saved/Prototype/surface_bindings.json').write_text(json.dumps({'materials':rows,'standalone_mesh_actors':actors,'floor_groups':groups},indent=2))
