"""Import the licensed real PBR artwork; do not replace source library files."""
import hashlib,json,traceback,re
from pathlib import Path
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));D=json.loads((P/'RawAssets/EchoRealV23/manifest.json').read_text(encoding='utf8'));BASE='/Game/Campaign/EchoCustom';E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools();S=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
cmd=u.SystemLibrary.get_command_line()
def option(name,default):
 match=re.search(r'-'+name+r'=(\d+)',cmd);return int(match.group(1)) if match else default
mat_start=option('R23MatStart',0);mat_end=mat_start+option('R23MatCount',10000);mesh_start=option('R23MeshStart',0);mesh_end=mesh_start+option('R23MeshCount',10000)
def node(m,cls,**props):
 n=L.create_material_expression(m,cls)
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def link(a,out,b,pin):assert L.connect_material_expressions(a,out,b,pin)
def mul(m,a,ao,b,bo):
 n=node(m,u.MaterialExpressionMultiply);link(a,ao,n,'A');link(b,bo,n,'B');return n
def scalar(m,name,value):return node(m,u.MaterialExpressionScalarParameter,parameter_name=name,default_value=value)
def vector(m,name,value):return node(m,u.MaterialExpressionVectorParameter,parameter_name=name,default_value=u.LinearColor(*value))
def prop(a,out,p):assert L.connect_material_property(a,out,p)
def master(mode,two_sided):
 name='M_Real23_'+mode+('_2S' if two_sided else '');path=BASE+'/Materials/'+name;m=u.load_asset(path)
 if m:return m
 m=A.create_asset(name,BASE+'/Materials',u.Material,u.MaterialFactoryNew());m.set_editor_property('two_sided',two_sided)
 if mode=='BLEND':
  m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
 elif mode=='MASK':m.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.4)
 else:m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
 uv=node(m,u.MaterialExpressionTextureCoordinate)
 def sample(name,default,kind):
  trans=vector(m,name+'UV',[1,1,0,0]);offset=vector(m,name+'Offset',[0,0,0,0]);angle=scalar(m,name+'Rotation',0);cu=node(m,u.MaterialExpressionCustom,code='float2 q=UV*Scale.xy;float s=sin(Angle),c=cos(Angle);return float2(q.x*c-q.y*s,q.x*s+q.y*c)+Offset.xy;',output_type=u.CustomMaterialOutputType.CMOT_FLOAT2)
  pins=[]
  for key in ['UV','Scale','Offset','Angle']:x=u.CustomInput();x.set_editor_property('input_name',key);pins.append(x)
  cu.set_editor_property('inputs',pins);link(uv,'',cu,'UV');link(trans,'RGB',cu,'Scale');link(offset,'RGB',cu,'Offset');link(angle,'',cu,'Angle')
  t=node(m,u.MaterialExpressionTextureSampleParameter2D,parameter_name=name,texture=u.load_asset('/Game/Campaign/Textures/'+default),sampler_type=kind);link(cu,'',t,'UVs');return t
 color=sample('Albedo','T_White',u.MaterialSamplerType.SAMPLERTYPE_COLOR);tint=vector(m,'Tint',[1,1,1,1]);prop(mul(m,color,'RGB',tint,'RGB'),'',u.MaterialProperty.MP_BASE_COLOR)
 normal=sample('Normal','T_FlatNormal',u.MaterialSamplerType.SAMPLERTYPE_NORMAL);flat=node(m,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(0,0,1,1));n=node(m,u.MaterialExpressionLinearInterpolate);link(flat,'',n,'A');link(normal,'RGB',n,'B');link(scalar(m,'NormalStrength',1),'',n,'Alpha');prop(n,'',u.MaterialProperty.MP_NORMAL)
 orm=sample('ORM','T_WhiteMasks',u.MaterialSamplerType.SAMPLERTYPE_MASKS)
 prop(mul(m,orm,'G',scalar(m,'Roughness',1),''),'',u.MaterialProperty.MP_ROUGHNESS);prop(mul(m,orm,'B',scalar(m,'Metallic',1),''),'',u.MaterialProperty.MP_METALLIC)
 ao=sample('AO','T_WhiteMasks',u.MaterialSamplerType.SAMPLERTYPE_MASKS);prop(ao,'R',u.MaterialProperty.MP_AMBIENT_OCCLUSION)
 em=sample('Emissive','T_White',u.MaterialSamplerType.SAMPLERTYPE_COLOR);prop(mul(m,em,'RGB',vector(m,'Emission',[0,0,0,1]),'RGB'),'',u.MaterialProperty.MP_EMISSIVE_COLOR)
 if mode in ('BLEND','MASK'):
  am=sample('Alpha','T_WhiteMasks',u.MaterialSamplerType.SAMPLERTYPE_MASKS);opacity=mul(m,am,'R',scalar(m,'Opacity',1),'');prop(opacity,'',u.MaterialProperty.MP_OPACITY if mode=='BLEND' else u.MaterialProperty.MP_OPACITY_MASK)
 L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);L.recompile_material(m);assert E.save_loaded_asset(m);return m
cache={}
def texture(path,kind):
 key=hashlib.sha256((str(path)+kind).encode()).hexdigest()[:16];target=BASE+'/Textures/T_R23_'+key
 if key in cache:return cache[key]
 t=u.load_asset(target)
 if t:cache[key]=t;return t
 if not t:
  task=u.AssetImportTask();task.filename=str(path);task.destination_path=BASE+'/Textures';task.destination_name='T_R23_'+key;task.automated=True;task.save=True;A.import_asset_tasks([task]);t=u.load_asset(target)
 assert t,str(path);t.set_editor_property('srgb',kind=='color');t.set_editor_property('max_texture_size',2048)
 if kind=='normal':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
 elif kind=='mask':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
 assert E.save_loaded_asset(t);cache[key]=t;return t
try:
 for index,(key,spec) in enumerate(D['materials'].items()):
  if not mat_start<=index<mat_end or E.does_asset_exist(BASE+'/Materials/MI_'+key):continue
  mode=spec['alpha'];two=spec['two_sided'] or mode!='OPAQUE';parent=master(mode,two);name='MI_'+key;m=u.load_asset(BASE+'/Materials/'+name) or A.create_asset(name,BASE+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(m,parent)
  L.set_material_instance_vector_parameter_value(m,'Tint',u.LinearColor(*spec['color']));L.set_material_instance_vector_parameter_value(m,'Emission',u.LinearColor(*spec['emission'],1))
  for name,value in [('Roughness',spec['roughness']),('Metallic',spec['metallic']),('NormalStrength',min(1,spec['normal_strength'])),('Opacity',spec['color'][3]),('Coat',spec['clearcoat'])]:L.set_material_instance_scalar_parameter_value(m,name,value)
  for name,k,kind in [('Albedo','albedo','color'),('Normal','normal','normal'),('ORM','orm','mask'),('AO','ao','mask'),('Emissive','emissive','color')]:
   item=spec.get(k)
   if not item or (k=='ao' and item['uv']!=spec['primary_uv']):continue
   t=texture(item['file'],kind);L.set_material_instance_texture_parameter_value(m,name,t);tr=item['transform'];sc=tr.get('scale',[1,1]);off=tr.get('offset',[0,0]);L.set_material_instance_vector_parameter_value(m,name+'UV',u.LinearColor(*sc,0,0));L.set_material_instance_vector_parameter_value(m,name+'Offset',u.LinearColor(*off,0,0));L.set_material_instance_scalar_parameter_value(m,name+'Rotation',tr.get('rotation',0))
  if spec.get('alpha_file'):L.set_material_instance_texture_parameter_value(m,'Alpha',texture(spec['alpha_file'],'mask'))
  assert E.save_loaded_asset(m)
  if index%20==0:u.log('R23_MATERIAL '+str(index));cache.clear();u.SystemLibrary.collect_garbage()
 result=json.loads((P/'Content/CampaignData/echo_custom_runtime.json').read_text(encoding='utf8'));imported=[]
 for row_index,row in enumerate(D['assets']):
  if not mesh_start<=row_index<mesh_end:continue
  name='SM_'+row['name'];path=BASE+'/Meshes/'+name;mesh=u.load_asset(path)
  if not mesh:
   opts=u.FbxImportUI()
   for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,original_import_type=u.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,is_obj_import=True).items():opts.set_editor_property(k,v)
   imp=opts.static_mesh_import_data
   for k,v in dict(convert_scene=False,convert_scene_unit=False,combine_meshes=True,transform_vertex_to_absolute=True,auto_generate_collision=False,generate_lightmap_u_vs=False,build_nanite=False).items():imp.set_editor_property(k,v)
   imp.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
   task=u.AssetImportTask();task.filename=row['file'];task.destination_path=BASE+'/Meshes';task.destination_name=name;task.automated=True;task.save=True;task.options=opts;task.factory=u.FbxFactory();A.import_asset_tasks([task]);mesh=u.load_asset(path)
  assert mesh,name
  for i,slot in enumerate(mesh.static_materials):mat=u.load_asset(BASE+'/Materials/MI_'+str(slot.material_slot_name));assert mat,str(slot.material_slot_name);mesh.set_material(i,mat)
  if row['triangles']>1500 and S.get_lod_count(mesh)<4:
   options=u.StaticMeshReductionOptions();options.auto_compute_lod_screen_size=False;options.reduction_settings=[u.StaticMeshReductionSettings(1.,1.),u.StaticMeshReductionSettings(.48,.30),u.StaticMeshReductionSettings(.18,.11),u.StaticMeshReductionSettings(.04,.035)];assert S.set_lods(mesh,options)>=4
  assert E.save_loaded_asset(mesh);bb=mesh.get_bounding_box();actual=[[bb.min.x,bb.min.y,bb.min.z],[bb.max.x,bb.max.y,bb.max.z]];assert max(abs(a-b) for x,y in zip(actual,row['bounds_cm']) for a,b in zip(x,y))<.5,(name,actual,row['bounds_cm'])
  result['meshes']=[x for x in result['meshes'] if x['id']!=row['id']];result['meshes'].append(dict(id=row['id'],name=row['name'],asset=mesh.get_path_name(),bounds_cm=actual,materials=len(mesh.static_materials),source_sha256=row['source_sha256'],door_leaf=False,source=row['source'],license=row['license'],lods=S.get_lod_count(mesh)));imported.append(dict(id=row['id'],asset=mesh.get_path_name(),lods=S.get_lod_count(mesh),bounds_cm=actual));u.log('R23_MESH '+str(row['id']));cache.clear();u.SystemLibrary.collect_garbage()
 result['version']=23;(P/'Content/CampaignData/echo_custom_runtime.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
 batch=P/f'Saved/Prototype/real_import_batch_{mesh_start}.json';batch.write_text(json.dumps(dict(success=True,meshes=len(imported),details=imported)),encoding='utf8')
 expected={x['id'] for x in D['assets']};complete=expected<={x['id'] for x in result['meshes']} and all(E.does_asset_exist(BASE+'/Materials/MI_'+key) for key in D['materials']);(P/'Saved/Prototype/real_import_v23.json').write_text(json.dumps(dict(success=complete,meshes=len(expected & {x['id'] for x in result['meshes']}),materials=len(D['materials']),total=len(result['meshes']),pending=not complete)),encoding='utf8');u.log('REAL_V23_BATCH_COMPLETE')
except Exception:
 (P/'Saved/Prototype/real_import_failed_v23.txt').write_text(traceback.format_exc(),encoding='utf8');raise
finally:u.SystemLibrary.quit_editor()
