"""Audit native vehicle assets and correct shared glTF ORM channel routing."""
import unreal as u,json,traceback,hashlib
from pathlib import Path
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));D=json.loads((P/'RawAssets/EchoCarsV31/manifest.json').read_text());B='/Game/Campaign/EchoCustom';E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;S=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
try:
 corrected=[]
 for path in E.list_assets(B+'/Materials',recursive=False):
  if '/M_Cars31B_' in path and 'BLEND' not in path:
   parent=u.load_asset(path)
   if not parent.get_editor_property('used_with_nanite'):
    L.set_base_material_usage(parent,u.MaterialUsage.MATUSAGE_NANITE,True);L.recompile_material(parent);assert E.save_loaded_asset(parent)
 for key,spec in D['materials'].items():
  m=u.load_asset(B+'/Materials/MI_'+key);assert m and m.get_editor_property('parent'),key
  if key=='VC31_8aa67043e05b0da7':
   parent=u.load_asset(B+'/Materials/M_Cars31B_OPAQUE');assert parent and spec['alpha']=='OPAQUE';L.set_material_instance_parent(m,parent);L.set_material_instance_vector_parameter_value(m,'Tint',u.LinearColor(*spec['color']));L.set_material_instance_scalar_parameter_value(m,'Roughness',spec['roughness']);L.set_material_instance_scalar_parameter_value(m,'Opacity',1);assert E.save_loaded_asset(m)
  if spec.get('roughness_map') and spec['roughness_map']==spec.get('metallic_map'):
   L.set_material_instance_vector_parameter_value(m,'RoughnessChannel',u.LinearColor(0,1,0,0));L.set_material_instance_vector_parameter_value(m,'MetallicChannel',u.LinearColor(0,0,1,0));assert E.save_loaded_asset(m);corrected.append(key)
 rows=[]
 for row in D['assets']:
  mesh=u.load_asset(B+'/Meshes/SM_'+row['name']);assert mesh,row['name']
  assert not row['nanite'] or all(D['materials'][k]['alpha']!='BLEND' for k in row['materials']),row['name']
  ns=mesh.get_editor_property('nanite_settings')
  if ns.enabled!=row['nanite']:ns.set_editor_property('enabled',row['nanite']);mesh.set_editor_property('nanite_settings',ns);assert E.save_loaded_asset(mesh)
  box=mesh.get_bounding_box();actual=[[box.min.x,box.min.y,box.min.z],[box.max.x,box.max.y,box.max.z]];assert max(abs(a-b) for x,y in zip(actual,row['bounds_cm']) for a,b in zip(x,y))<.5,row['name']
  lods=S.get_lod_count(mesh);assert lods>=4 if row['triangles']>1500 and not row['nanite'] else lods>=1,row['name'];assert 0<mesh.get_num_sections(0)<=len(mesh.static_materials)<=len(row['materials']) and set(str(v.material_slot_name) for v in mesh.static_materials).issubset(row['materials']) and len(set(str(v.material_slot_name) for v in mesh.static_materials))==len(mesh.static_materials),row['name'];assert all(v.material_interface and v.material_interface.get_editor_property('parent') for v in mesh.static_materials),row['name']
  rows.append(dict(id=row['id'],asset=mesh.get_path_name(),lods=lods,render_lods=mesh.get_num_lods(),materials=len(mesh.static_materials),sections=mesh.get_num_sections(0),bounds_cm=actual,nanite=bool(mesh.get_editor_property('nanite_settings').enabled)))
  u.SystemLibrary.collect_garbage()
 runtime_path=P/'Content/CampaignData/echo_custom_runtime.json';runtime=json.loads(runtime_path.read_text());actual_by_id={r['id']:r for r in rows}
 for record in runtime['meshes']:
  if record['id'] in actual_by_id:record['nanite']=actual_by_id[record['id']]['nanite'];record['render_sections']=actual_by_id[record['id']]['sections']
 runtime_path.write_text(json.dumps(runtime,ensure_ascii=False,indent=2))
 result=dict(success=True,vehicles=len(D['vehicles']),meshes=len(rows),materials=len(D['materials']),orm_channel_corrections=corrected,assets=rows,manifest_sha256=hashlib.sha256((P/'RawAssets/EchoCarsV31/manifest.json').read_bytes()).hexdigest());(P/'Saved/Prototype/cars31_native_audit.json').write_text(json.dumps(result,indent=2));u.log('CARS31_AUDIT_PASS '+str(len(rows)))
except Exception:
 (P/'Saved/Prototype/cars31_audit_failed.txt').write_text(traceback.format_exc());raise
finally:u.SystemLibrary.quit_editor()
