"""Import the missing shaped furnishings without reimporting the main library."""
import json,traceback
from pathlib import Path
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));D=json.loads((P/'RawAssets/EchoSupplementsV21/manifest.json').read_text());BASE='/Game/Campaign/EchoCustom';E=u.EditorAssetLibrary;A=u.AssetToolsHelpers.get_asset_tools()
try:
 result=json.loads((P/'Content/CampaignData/echo_custom_runtime.json').read_text());result['meshes']=[m for m in result['meshes'] if m['id']<600]
 for row in D['assets']:
  name='SM_'+row['name'];path=BASE+'/Meshes/'+name
  mesh=u.load_asset(path)
  if not mesh or any(str(s.material_slot_name)=='defaultMat' for s in mesh.static_materials):
   opts=u.FbxImportUI()
   for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,original_import_type=u.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,is_obj_import=True).items():opts.set_editor_property(k,v)
   imp=opts.static_mesh_import_data
   for k,v in dict(convert_scene=False,convert_scene_unit=False,combine_meshes=True,transform_vertex_to_absolute=True,auto_generate_collision=False,generate_lightmap_u_vs=False,build_nanite=False).items():imp.set_editor_property(k,v)
   imp.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
   task=u.AssetImportTask();task.filename=row['file'];task.destination_path=BASE+'/Meshes';task.destination_name=name;task.replace_existing=True;task.automated=True;task.save=True;task.options=opts;task.factory=u.FbxFactory();A.import_asset_tasks([task]);mesh=u.load_asset(path)
  assert mesh,name
  for j,slot in enumerate(mesh.static_materials):
   mat=u.load_asset(BASE+'/Materials/MI_'+str(slot.material_slot_name));assert mat,str(slot.material_slot_name);mesh.set_material(j,mat)
  assert E.save_loaded_asset(mesh)
  bounds=mesh.get_bounding_box();actual=[[bounds.min.x,bounds.min.y,bounds.min.z],[bounds.max.x,bounds.max.y,bounds.max.z]]
  assert max(abs(a-b) for x,y in zip(actual,row['bounds_cm']) for a,b in zip(x,y))<.3,(name,actual,row['bounds_cm'])
  result['meshes'].append(dict(id=row['id'],name=row['name'],asset=mesh.get_path_name(),bounds_cm=actual,materials=len(mesh.static_materials),source_sha256=row['source_sha256'],door_leaf=False))
  u.log('ECHO_SUPPLEMENT '+name);u.SystemLibrary.collect_garbage()
 result['version']=21
 (P/'Content/CampaignData/echo_custom_runtime.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
 (P/'Saved/Prototype/echo_supplement_import_v21.json').write_text(json.dumps(dict(success=True,meshes=len(D['assets']),native_total=len(result['meshes']))),encoding='utf8')
 u.log('ECHO_SUPPLEMENTS_COMPLETE')
except Exception:
 (P/'Saved/Prototype/echo_supplement_failed_v21.txt').write_text(traceback.format_exc(),encoding='utf8');raise
finally:u.SystemLibrary.quit_editor()
