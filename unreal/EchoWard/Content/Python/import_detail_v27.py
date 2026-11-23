"""Import only fifteen original v27 meshes and their dedicated PBR surfaces."""
from pathlib import Path
import json,traceback
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));D=json.loads((P/'RawAssets/EchoDetailV27/manifest.json').read_text());BASE='/Game/Campaign/EchoCustom';E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools();S=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
def node(m,c,**p):
 n=L.create_material_expression(m,c)
 for k,v in p.items():n.set_editor_property(k,v)
 return n
try:
 m=u.load_asset(BASE+'/Materials/M_Detail27')
 if not m:
  m=A.create_asset('M_Detail27',BASE+'/Materials',u.Material,u.MaterialFactoryNew());m.set_editor_property('tangent_space_normal',False)
  pos=node(m,u.MaterialExpressionWorldPosition);normal=node(m,u.MaterialExpressionVertexNormalWS);vn=node(m,u.MaterialExpressionVertexInterpolator);L.connect_material_expressions(normal,'',vn,'')
  pins={'P':(pos,''),'N':(vn,''),'Tint':(node(m,u.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=u.LinearColor(.6,.6,.6,1)),'RGB')}
  for k,v in [('Roughness',.5),('Metallic',0),('Grime',.12)]:pins[k]=(node(m,u.MaterialExpressionScalarParameter,parameter_name=k,default_value=v),'')
  noise='struct Q{float h(float3 p){p=frac(p*.1031);p+=dot(p,p.yzx+33.33);return frac((p.x+p.y)*p.z);}float n(float3 p){float3 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(lerp(h(i),h(i+float3(1,0,0)),f.x),lerp(h(i+float3(0,1,0)),h(i+float3(1,1,0)),f.x),f.y),lerp(lerp(h(i+float3(0,0,1)),h(i+float3(1,0,1)),f.x),lerp(h(i+float3(0,1,1)),h(i+1),f.x),f.y),f.z);}}q;'
  code=noise+'float grime=q.n(P*.045);float grain=q.h(floor(P*5));float scratch=pow(saturate(sin(P.z*24+P.x*.16)*.5+.5),32)*.025;float3 c=Tint*(.93+.07*grain)*(1-Grime*smoothstep(.45,.85,grime));return float4(c,clamp(Roughness+(grain-.5)*.06+grime*Grime*.18-scratch,.08,.96));'
  cr=node(m,u.MaterialExpressionCustom,code=code,output_type=u.CustomMaterialOutputType.CMOT_FLOAT4);ip=[]
  for k in pins:p=u.CustomInput();p.set_editor_property('input_name',k);ip.append(p)
  cr.set_editor_property('inputs',ip)
  for k,(n,o) in pins.items():L.connect_material_expressions(n,o,cr,k)
  for prop,rgba in [(u.MaterialProperty.MP_BASE_COLOR,(True,True,True,False)),(u.MaterialProperty.MP_ROUGHNESS,(False,False,False,True))]:
   mask=node(m,u.MaterialExpressionComponentMask,r=rgba[0],g=rgba[1],b=rgba[2],a=rgba[3]);L.connect_material_expressions(cr,'',mask,'');L.connect_material_property(mask,'',prop)
  L.connect_material_property(pins['Metallic'][0],'',u.MaterialProperty.MP_METALLIC);L.connect_material_property(vn,'',u.MaterialProperty.MP_NORMAL)
  em=node(m,u.MaterialExpressionVectorParameter,parameter_name='Emission',default_value=u.LinearColor(0,0,0,1));L.connect_material_property(em,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
  for usage in [u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,u.MaterialUsage.MATUSAGE_NANITE]:L.set_base_material_usage(m,usage,True)
  L.recompile_material(m);assert E.save_loaded_asset(m)
 specs={'ivory':((.65,.68,.64),.48,0),'alloy':((.36,.40,.42),.31,.88),'dark':((.013,.02,.022),.48,0),'rubber':((.009,.011,.012),.83,0),'teal':((.035,.15,.17),.43,.12),'paper':((.72,.70,.62),.87,0),'red':((.31,.018,.012),.44,.22),'amber':((.62,.26,.035),.52,0),'screen':((.002,.009,.01),.17,.12),'trace':((.06,.5,.32),.30,0),'linen':((.54,.56,.49),.93,0),'glass':((.29,.39,.40),.21,.08)}
 for key,(rgb,rough,metal) in specs.items():
  name='MI_D27_'+key;mi=u.load_asset(BASE+'/Materials/'+name) or A.create_asset(name,BASE+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(mi,m);L.set_material_instance_vector_parameter_value(mi,'Tint',u.LinearColor(*rgb,1))
  for k,v in [('Roughness',rough),('Metallic',metal),('Grime',.07 if key in ['paper','trace','screen'] else .18)]:L.set_material_instance_scalar_parameter_value(mi,k,v)
  if key=='trace':L.set_material_instance_vector_parameter_value(mi,'Emission',u.LinearColor(.015,.4,.16,1))
  assert E.save_loaded_asset(mi)
 result=json.loads((P/'Content/CampaignData/echo_custom_runtime.json').read_text());imported=[]
 for row in D['assets']:
  name='SM_'+row['name'];path=BASE+'/Meshes/'+name;mesh=u.load_asset(path)
  if not mesh or row['id'] in [900,906,914]:
   opts=u.FbxImportUI()
   for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,original_import_type=u.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,is_obj_import=True).items():opts.set_editor_property(k,v)
   imp=opts.static_mesh_import_data
   for k,v in dict(convert_scene=False,convert_scene_unit=False,combine_meshes=True,transform_vertex_to_absolute=True,auto_generate_collision=False,generate_lightmap_u_vs=False,build_nanite=False).items():imp.set_editor_property(k,v)
   imp.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
   task=u.AssetImportTask();task.filename=row['file'];task.destination_path=BASE+'/Meshes';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;task.options=opts;task.factory=u.FbxFactory();A.import_asset_tasks([task]);mesh=u.load_asset(path)
  assert mesh,name
  for i,slot in enumerate(mesh.static_materials):mat=u.load_asset(BASE+'/Materials/MI_'+str(slot.material_slot_name));assert mat,str(slot.material_slot_name);mesh.set_material(i,mat)
  if row['triangles']>1500 and S.get_lod_count(mesh)<4:
   opt=u.StaticMeshReductionOptions();opt.auto_compute_lod_screen_size=False;opt.reduction_settings=[u.StaticMeshReductionSettings(1.,1.),u.StaticMeshReductionSettings(.5,.32),u.StaticMeshReductionSettings(.2,.12),u.StaticMeshReductionSettings(.06,.035)];assert S.set_lods(mesh,opt)>=4
  assert E.save_loaded_asset(mesh);bb=mesh.get_bounding_box();actual=[[bb.min.x,bb.min.y,bb.min.z],[bb.max.x,bb.max.y,bb.max.z]];assert max(abs(a-b) for x,y in zip(actual,row['bounds_cm']) for a,b in zip(x,y))<.5,(name,actual,row['bounds_cm'])
  result['meshes']=[a for a in result['meshes'] if a['id']!=row['id']];result['meshes'].append(dict(id=row['id'],name=row['name'],asset=mesh.get_path_name(),bounds_cm=actual,materials=len(mesh.static_materials),source_sha256=row['source_sha256'],door_leaf=False,lods=S.get_lod_count(mesh)));imported.append(row['id']);u.log('DETAIL27_MESH '+str(row['id']));u.SystemLibrary.collect_garbage()
 result['version']=27;(P/'Content/CampaignData/echo_custom_runtime.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
 (P/'Saved/Prototype/detail_v27_import.json').write_text(json.dumps(dict(success=True,meshes=imported,materials=len(specs),total=len(result['meshes']))),encoding='utf8');u.log('DETAIL_V27_IMPORT_COMPLETE')
except Exception:
 (P/'Saved/Prototype/detail_v27_import_failed.txt').write_text(traceback.format_exc(),encoding='utf8');raise
finally:u.SystemLibrary.quit_editor()
