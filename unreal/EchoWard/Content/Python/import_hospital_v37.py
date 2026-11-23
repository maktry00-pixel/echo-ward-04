"""Import selected source-library models into an isolated v37 namespace."""
from pathlib import Path
import json,traceback
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));D=json.loads((P/'RawAssets/HospitalV37/manifest.json').read_text());BASE='/Game/Campaign/HospitalV37';E=u.EditorAssetLibrary;A=u.AssetToolsHelpers.get_asset_tools();S=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
try:
 result=[]
 for row in D['assets']:
  name='SM_H37_'+str(row['id']);path=BASE+'/Meshes/'+name;mesh=u.load_asset(path)
  if not mesh:
   opt=u.FbxImportUI()
   for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,original_import_type=u.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,is_obj_import=True).items():opt.set_editor_property(k,v)
   imp=opt.static_mesh_import_data
   for k,v in dict(convert_scene=False,convert_scene_unit=False,combine_meshes=True,transform_vertex_to_absolute=True,auto_generate_collision=False,generate_lightmap_u_vs=False,build_nanite=False).items():imp.set_editor_property(k,v)
   imp.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
   t=u.AssetImportTask();t.filename=row['file'];t.destination_path=BASE+'/Meshes';t.destination_name=name;t.automated=True;t.replace_existing=False;t.save=True;t.options=opt;t.factory=u.FbxFactory();A.import_asset_tasks([t]);mesh=u.load_asset(path)
  assert mesh,name
  for i,slot in enumerate(mesh.static_materials):
   key=str(slot.material_slot_name);key=key[key.find('D27_'):] if 'D27_' in key else 'D27_ivory'
   mat=u.load_asset('/Game/Campaign/EchoCustom/Materials/MI_'+key);assert mat,key;mesh.set_material(i,mat)
  if S.get_lod_count(mesh)<4:
   opt=u.StaticMeshReductionOptions();opt.auto_compute_lod_screen_size=False;opt.reduction_settings=[u.StaticMeshReductionSettings(1.,1.),u.StaticMeshReductionSettings(.55,.34),u.StaticMeshReductionSettings(.22,.12),u.StaticMeshReductionSettings(.07,.035)];assert S.set_lods(mesh,opt)>=4
  assert E.save_loaded_asset(mesh)
  bb=mesh.get_bounding_box();actual=[[bb.min.x,bb.min.y,bb.min.z],[bb.max.x,bb.max.y,bb.max.z]]
  expected=[row['bounds_cm'][0].copy(),row['bounds_cm'][1].copy()];expected[0][1]=-row['bounds_cm'][1][1];expected[1][1]=-row['bounds_cm'][0][1]
  assert max(abs(a-b) for x,y in zip(actual,expected) for a,b in zip(x,y))<.5,(name,actual,expected)
  result.append(dict(**row,asset=mesh.get_path_name(),actual_bounds_cm=actual,lods=S.get_lod_count(mesh),material_slots=len(mesh.static_materials)))
  u.log('HOSPITAL37_IMPORTED '+str(row['id']));u.SystemLibrary.collect_garbage()
 (P/'Content/CampaignData/hospital_assets_v37.json').write_text(json.dumps(dict(version=37,success=True,assets=result),ensure_ascii=False,indent=2),encoding='utf-8')
 (P/'Saved/Prototype/hospital37_import.json').write_text(json.dumps(dict(success=True,meshes=len(result),assets=result),ensure_ascii=False,indent=2),encoding='utf-8')
except Exception:
 (P/'Saved/Prototype/hospital37_import_failed.txt').write_text(traceback.format_exc(),encoding='utf-8');raise
finally:u.SystemLibrary.quit_editor()
