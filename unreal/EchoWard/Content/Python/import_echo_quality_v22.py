"""Native quality materials and original meshes; preserve all source assets."""
import json,traceback
from pathlib import Path
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));D=json.loads((P/'RawAssets/EchoQualityV22/manifest.json').read_text());BASE='/Game/Campaign/EchoCustom';E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools()
def node(m,c,**props):
 n=L.create_material_expression(m,c)
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def save(m):assert E.save_loaded_asset(m)
def custom(m,code,pins,kind):
 c=node(m,u.MaterialExpressionCustom,code=code,output_type=kind);inputs=[]
 for key in pins:pin=u.CustomInput();pin.set_editor_property('input_name',key);inputs.append(pin)
 c.set_editor_property('inputs',inputs)
 for key,(n,out) in pins.items():assert L.connect_material_expressions(n,out,c,key)
 return c
def master(name,road=False,sky=False):
 path=BASE+'/Materials/'+name;m=u.load_asset(path)
 if m:return m
 m=A.create_asset(name,BASE+'/Materials',u.Material,u.MaterialFactoryNew());m.set_editor_property('tangent_space_normal',False)
 pos=node(m,u.MaterialExpressionWorldPosition);normal=node(m,u.MaterialExpressionVertexNormalWS);vn=node(m,u.MaterialExpressionVertexInterpolator);L.connect_material_expressions(normal,'',vn,'')
 tint=node(m,u.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=u.LinearColor(.6,.62,.59,1));rough=node(m,u.MaterialExpressionScalarParameter,parameter_name='Roughness',default_value=.7)
 pins={'P':(pos,''),'N':(vn,''),'Tint':(tint,'RGB'),'Roughness':(rough,'')}
 noise='''struct Q{float h(float3 p){p=frac(p*.1031);p+=dot(p,p.yzx+33.33);return frac((p.x+p.y)*p.z);}float n(float3 p){float3 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(lerp(h(i),h(i+float3(1,0,0)),f.x),lerp(h(i+float3(0,1,0)),h(i+float3(1,1,0)),f.x),f.y),lerp(lerp(h(i+float3(0,0,1)),h(i+float3(1,0,1)),f.x),lerp(h(i+float3(0,1,1)),h(i+1),f.x),f.y),f.z);}}q;\n'''
 if road:
  code=noise+'''float broad=q.n(float3(P.xy*.0018,2))*.65+q.n(float3(P.xy*.0041,3))*.35;float wet=smoothstep(.47,.65,broad);float gravel=q.n(P*.7);float cracks=pow(saturate(1-abs(q.n(P*.026)*2-1)),26);float3 c=Tint*(.7+gravel*.6)*(1-cracks*.27);return float4(c*lerp(1,.56,wet),lerp(clamp(Roughness+gravel*.12,.4,.95),.075,wet));'''
  normalcode=noise+'''float wet=smoothstep(.47,.65,q.n(float3(P.xy*.0018,2))*.65+q.n(float3(P.xy*.0041,3))*.35);float h=q.n(P*.7);float dx=q.n((P+float3(.6,0,0))*.7)-h,dy=q.n((P+float3(0,.6,0))*.7)-h;return normalize(float3(-dx,-dy,1)*float3(lerp(.48,.018,wet),lerp(.48,.018,wet),1));'''
 else:
  code=noise+'''float fine=q.n(P*1.8);float dirt=q.n(P*.008);float3 c=Tint*(.92+.08*fine)*lerp(.88,1,smoothstep(.2,.7,dirt));return float4(c,clamp(Roughness+(fine-.5)*.08,.08,.98));'''
  normalcode='return normalize(N);'
 cr=custom(m,code,pins,u.CustomMaterialOutputType.CMOT_FLOAT4)
 for prop,channels in [(u.MaterialProperty.MP_BASE_COLOR,dict(r=True,g=True,b=True,a=False)),(u.MaterialProperty.MP_ROUGHNESS,dict(r=False,g=False,b=False,a=True))]:
  mask=node(m,u.MaterialExpressionComponentMask,**channels);L.connect_material_expressions(cr,'',mask,'');L.connect_material_property(mask,'',prop)
 L.connect_material_property(custom(m,normalcode,pins,u.CustomMaterialOutputType.CMOT_FLOAT3),'',u.MaterialProperty.MP_NORMAL)
 for param,prop,default in [('Metallic',u.MaterialProperty.MP_METALLIC,0),('Specular',u.MaterialProperty.MP_SPECULAR,.5)]:n=node(m,u.MaterialExpressionScalarParameter,parameter_name=param,default_value=default);L.connect_material_property(n,'',prop)
 em=node(m,u.MaterialExpressionVectorParameter,parameter_name='Emission',default_value=u.LinearColor(0,0,0,1));L.connect_material_property(em,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
 L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_NANITE,True);L.recompile_material(m);save(m);return m
try:
 clean=master('M_EchoFineSurfaceV22');road=master('M_EchoWetRoadV22',True)
 specs={'CarPaint':((.025,.035,.048),.19,.75),'Chrome':((.48,.52,.55),.2,1),'Bark':((.11,.065,.032),.91,0),'Needles':((.028,.065,.032),.83,0),'Leaves':((.08,.15,.044),.8,0),'Stone':((.37,.38,.35),.82,0),'Metal':((.09,.105,.11),.4,.65),'Plastic':((.7,.71,.68),.55,0),'Tail':((.32,.003,.001),.21,0),'Headlamp':((.65,.43,.19),.2,0),'Road':((.075,.08,.085),.74,0),'Facade':((.46,.45,.41),.83,0),'DadoTile':((.045,.095,.105),.35,0),'WindowWarm':((.26,.15,.055),.5,0)}
 for name,(rgb,ro,metal) in specs.items():
  key='V22_'+name;path=BASE+'/Materials/MI_'+key;m=u.load_asset(path) or A.create_asset('MI_'+key,BASE+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(m,road if name=='Road' else clean);L.set_material_instance_vector_parameter_value(m,'Tint',u.LinearColor(*rgb,1));L.set_material_instance_scalar_parameter_value(m,'Roughness',ro);L.set_material_instance_scalar_parameter_value(m,'Metallic',metal)
  if name in ('Headlamp','Tail','WindowWarm'):L.set_material_instance_vector_parameter_value(m,'Emission',u.LinearColor(*({'Headlamp':(5.5,3.2,1.1),'Tail':(2.5,.005,.001),'WindowWarm':(1.15,.6,.2)}[name]),1))
  save(m)
 glass=u.load_asset(BASE+'/Materials/MI_V22_CarGlass') or A.create_asset('MI_V22_CarGlass',BASE+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(glass,u.load_asset(BASE+'/Materials/M_EchoGlassV2'));L.set_material_instance_vector_parameter_value(glass,'Tint',u.LinearColor(.025,.045,.055,.65));L.set_material_instance_scalar_parameter_value(glass,'Roughness',.14);save(glass)
 # The former white/steel/black textures were giant wall patterns on every
 # kettle, chair and machine. Replace those slots with restrained fine finishes.
 old=json.loads((P/'RawAssets/EchoCustom/manifest.json').read_text())
 finishes={'white':(.63,.65,.61),'steel':(.33,.36,.36),'black':(.026,.03,.032),'rubber':(.012,.015,.016),'wood':(.19,.105,.047),'fabric_dirty':(.40,.385,.33),'fabric_blue':(.07,.14,.17),'025':(.55,.56,.52),'026':(.11,.19,.20),'028':(.58,.59,.55),'031':(.12,.105,.085),'032':(.36,.39,.40)}
 for key,spec in old['materials'].items():
  if spec['name'] not in finishes:continue
  m=u.load_asset(BASE+'/Materials/MI_'+key);L.set_material_instance_parent(m,clean)
  rgb=finishes[spec['name']]
  L.set_material_instance_vector_parameter_value(m,'Tint',u.LinearColor(*rgb,1));L.set_material_instance_scalar_parameter_value(m,'Metallic',.72 if spec['name'] in ('steel','032') else .45 if spec['name']=='031' else 0);L.set_material_instance_scalar_parameter_value(m,'Roughness',.4 if spec['name'] in ('steel','032') else .62 if spec['name'] in ('white','028') else .8);save(m)
 result=json.loads((P/'Content/CampaignData/echo_custom_runtime.json').read_text());result['meshes']=[m for m in result['meshes'] if m['id']<640]
 for row in D['assets']:
  name='SM_'+row['name'];path=BASE+'/Meshes/'+name;mesh=u.load_asset(path)
  if not mesh:
   opts=u.FbxImportUI()
   for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,original_import_type=u.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,is_obj_import=True).items():opts.set_editor_property(k,v)
   imp=opts.static_mesh_import_data
   for k,v in dict(convert_scene=False,convert_scene_unit=False,combine_meshes=True,transform_vertex_to_absolute=True,auto_generate_collision=False,generate_lightmap_u_vs=False,build_nanite=False).items():imp.set_editor_property(k,v)
   imp.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
   task=u.AssetImportTask();task.filename=row['file'];task.destination_path=BASE+'/Meshes';task.destination_name=name;task.automated=True;task.save=True;task.options=opts;task.factory=u.FbxFactory();A.import_asset_tasks([task]);mesh=u.load_asset(path)
  assert mesh,name
  for j,slot in enumerate(mesh.static_materials):mat=u.load_asset(BASE+'/Materials/MI_'+str(slot.material_slot_name));assert mat,str(slot.material_slot_name);mesh.set_material(j,mat)
  save(mesh);bb=mesh.get_bounding_box();actual=[[bb.min.x,bb.min.y,bb.min.z],[bb.max.x,bb.max.y,bb.max.z]];assert max(abs(a-b) for x,y in zip(actual,row['bounds_cm']) for a,b in zip(x,y))<.3,(name,actual,row['bounds_cm'])
  result['meshes'].append(dict(id=row['id'],name=row['name'],asset=mesh.get_path_name(),bounds_cm=actual,materials=len(mesh.static_materials),source_sha256=row['source_sha256'],door_leaf=False));u.SystemLibrary.collect_garbage()
 result['version']=22;(P/'Content/CampaignData/echo_custom_runtime.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8');(P/'Saved/Prototype/quality_import_v22.json').write_text(json.dumps(dict(success=True,meshes=9,total=len(result['meshes']),materials=len(specs)+1)),encoding='utf8');u.log('QUALITY_V22_IMPORT_COMPLETE')
except Exception:
 (P/'Saved/Prototype/quality_import_failed_v22.txt').write_text(traceback.format_exc(),encoding='utf8');raise
finally:u.SystemLibrary.quit_editor()
