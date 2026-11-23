"""Native UE vehicle importer. Supplied cars stay separate from prior art assets."""
import unreal as u,json,hashlib,re,traceback
from pathlib import Path
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));D=json.loads((P/'RawAssets/EchoCarsV31/manifest.json').read_text());B='/Game/Campaign/EchoCustom';A=u.AssetToolsHelpers.get_asset_tools();E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;S=u.get_editor_subsystem(u.StaticMeshEditorSubsystem);cmd=u.SystemLibrary.get_command_line()
def opt(n,default):
 m=re.search('-'+n+r'=(\d+)',cmd);return int(m.group(1)) if m else default
start=opt('CarStart',0);end=start+opt('CarCount',10000);ms=opt('CarMatStart',0);me=ms+opt('CarMatCount',10000)
def node(m,cls,**kw):
 n=L.create_material_expression(m,cls)
 for k,v in kw.items():n.set_editor_property(k,v)
 return n
def link(a,o,b,p):assert L.connect_material_expressions(a,o,b,p)
def scalar(m,n,v):return node(m,u.MaterialExpressionScalarParameter,parameter_name=n,default_value=v)
def vec(m,n,v):return node(m,u.MaterialExpressionVectorParameter,parameter_name=n,default_value=u.LinearColor(*v))
def mul(m,a,o,b,p):
 n=node(m,u.MaterialExpressionMultiply);link(a,o,n,'A');link(b,p,n,'B');return n
def master(mode,two):
 name='M_Cars31B_'+mode+('_2S' if two else '');m=u.load_asset(B+'/Materials/'+name)
 if m:return m
 m=A.create_asset(name,B+'/Materials',u.Material,u.MaterialFactoryNew());m.set_editor_property('two_sided',two)
 if mode=='BLEND':m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
 elif mode=='MASK':m.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED)
 if mode=='OPAQUE':
  m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_CLEAR_COAT);m.set_editor_property('use_material_attributes',True);attrs=node(m,u.MaterialExpressionMakeMaterialAttributes);assert L.connect_material_property(attrs,'',u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
  def bind(n,o,p):link(n,o,attrs,p)
  link(scalar(m,'Coat',0),'',attrs,'ClearCoat');link(scalar(m,'CoatRoughness',.14),'',attrs,'ClearCoatRoughness')
 else:
  def bind(n,o,p):assert L.connect_material_property(n,o,getattr(u.MaterialProperty,{'BaseColor':'MP_BASE_COLOR','Normal':'MP_NORMAL','Roughness':'MP_ROUGHNESS','Metallic':'MP_METALLIC','EmissiveColor':'MP_EMISSIVE_COLOR','Opacity':'MP_OPACITY','OpacityMask':'MP_OPACITY_MASK'}[p]))
 def sample(n,default,typ):return node(m,u.MaterialExpressionTextureSampleParameter2D,parameter_name=n,texture=u.load_asset('/Game/Campaign/Textures/'+default),sampler_type=typ)
 color=sample('Albedo','T_White',u.MaterialSamplerType.SAMPLERTYPE_COLOR);vertex=node(m,u.MaterialExpressionVertexColor);white=node(m,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(1,1,1,1));mix=node(m,u.MaterialExpressionLinearInterpolate);link(white,'',mix,'A');link(vertex,'',mix,'B');link(scalar(m,'VertexRGB',0),'',mix,'Alpha');base=mul(m,color,'RGB',vec(m,'Tint',[1,1,1,1]),'RGB');bind(mul(m,base,'',mix,''),'','BaseColor')
 normal=sample('Normal','T_FlatNormal',u.MaterialSamplerType.SAMPLERTYPE_NORMAL);bind(normal,'RGB','Normal')
 for param in ['Roughness','Metallic']:
  tx=sample(param+'Map','T_WhiteMasks',u.MaterialSamplerType.SAMPLERTYPE_MASKS);dot=node(m,u.MaterialExpressionDotProduct);link(tx,'RGB',dot,'A');link(vec(m,param+'Channel',[1,0,0,0]),'RGB',dot,'B');bind(mul(m,dot,'',scalar(m,param,.5 if param=='Roughness' else 0),''),'',param)
 bind(mul(m,color,'RGB',vec(m,'Emission',[0,0,0,1]),'RGB'),'','EmissiveColor')
 if mode!='OPAQUE':
  alpha=sample('Alpha','T_WhiteMasks',u.MaterialSamplerType.SAMPLERTYPE_MASKS);bind(mul(m,alpha,'R',scalar(m,'Opacity',1),''),'','Opacity' if mode=='BLEND' else 'OpacityMask')
 L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);L.recompile_material(m);assert E.save_loaded_asset(m);return m
cache={}
def texture(f,kind,hero=False):
 assert Path(f).is_file(),f
 key=hashlib.sha256((str(f)+kind).encode()).hexdigest()[:16];path=B+'/Textures/T_V31_'+key
 if key in cache:return cache[key]
 t=u.load_asset(path)
 if not t:
  task=u.AssetImportTask();task.filename=f;task.destination_path=B+'/Textures';task.destination_name='T_V31_'+key;task.automated=True;task.save=True;A.import_asset_tasks([task]);t=u.load_asset(path)
 assert t,f;t.set_editor_property('srgb',kind=='color');t.set_editor_property('max_texture_size',4096)
 if kind=='normal':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
 if kind=='mask':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
 E.save_loaded_asset(t);cache[key]=t;return t
try:
 for parent_path in E.list_assets(B+'/Materials',recursive=False):
  if '/M_Cars31B_' in parent_path and 'BLEND' not in parent_path:
   parent=u.load_asset(parent_path)
   if not parent.get_editor_property('used_with_nanite'):
    L.set_base_material_usage(parent,u.MaterialUsage.MATUSAGE_NANITE,True);L.recompile_material(parent);assert E.save_loaded_asset(parent)
 for i,(key,spec) in enumerate(D['materials'].items()):
  if not ms<=i<me:continue
  path=B+'/Materials/MI_'+key
  existing=u.load_asset(path)
  if existing and existing.get_editor_property('parent'):continue
  mode=spec['alpha'];m=existing or A.create_asset('MI_'+key,B+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(m,master(mode,spec['two_sided']))
  L.set_material_instance_vector_parameter_value(m,'Tint',u.LinearColor(*spec['color']));em=spec['emission'] if 'gauges' in spec['name'].lower() else [0,0,0];L.set_material_instance_vector_parameter_value(m,'Emission',u.LinearColor(*em,1))
  for n,v in [('Roughness',1 if spec.get('roughness_map') else spec['roughness']),('Metallic',1 if spec.get('metallic_map') else spec['metallic']),('Opacity',spec['color'][3]),('Coat',spec['clearcoat']),('VertexRGB',1 if spec['vertex_color'] else 0)]:L.set_material_instance_scalar_parameter_value(m,n,v)
  hero=spec['name'].startswith('mat_')
  for n,k,kind in [('Albedo','albedo','color'),('Normal','normal','normal'),('RoughnessMap','roughness_map','mask'),('MetallicMap','metallic_map','mask'),('Alpha','alpha_file','mask')]:
   if spec.get(k):L.set_material_instance_texture_parameter_value(m,n,texture(spec[k],kind,hero))
  E.save_loaded_asset(m)
  if i%20==0:u.log('CARS31_MATERIAL '+str(i));cache.clear();u.SystemLibrary.collect_garbage()
 result=json.loads((P/'Content/CampaignData/echo_custom_runtime.json').read_text());imported=[]
 for i,row in enumerate(D['assets']):
  if not start<=i<end:continue
  name='SM_'+row['name'];path=B+'/Meshes/'+name;mesh=u.load_asset(path)
  previous=next((r for r in result['meshes'] if r['id']==row['id'] and not r.get('door_leaf')),None)
  if mesh and ('-CarClean=1' in cmd or not previous or previous.get('source_sha256')!=row['source_sha256']):assert E.delete_loaded_asset(mesh);mesh=None;u.SystemLibrary.collect_garbage()
  if not mesh:
   opts=u.FbxImportUI()
   for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,original_import_type=u.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,is_obj_import=False).items():opts.set_editor_property(k,v)
   imp=opts.static_mesh_import_data
   for k,v in dict(convert_scene=False,convert_scene_unit=False,import_uniform_scale=.01,combine_meshes=True,transform_vertex_to_absolute=True,auto_generate_collision=False,generate_lightmap_u_vs=False,build_nanite=row['nanite']).items():imp.set_editor_property(k,v)
   imp.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS);imp.set_editor_property('vertex_color_import_option',u.VertexColorImportOption.REPLACE)
   task=u.AssetImportTask();task.filename=row['file'];task.destination_path=B+'/Meshes';task.destination_name=name;task.automated=True;task.save=True;task.options=opts;task.factory=u.FbxFactory();A.import_asset_tasks([task]);mesh=u.load_asset(path)
  assert mesh,name
  build=S.get_lod_build_settings(mesh,0)
  if build.get_editor_property("distance_field_resolution_scale")!=0:
   build.set_editor_property("distance_field_resolution_scale",0.0);S.set_lod_build_settings(mesh,0,build)
  slots=list(mesh.static_materials);changed=False
  for slot in slots:
   mat=u.load_asset(B+'/Materials/MI_'+str(slot.material_slot_name));assert mat,str(slot.material_slot_name)
   if slot.material_interface!=mat:slot.set_editor_property('material_interface',mat);changed=True
  if changed:mesh.set_editor_property('static_materials',slots)
  if not row['nanite'] and row['triangles']>1500 and S.get_lod_count(mesh)<4:
   red=u.StaticMeshReductionOptions();red.auto_compute_lod_screen_size=False;red.reduction_settings=[u.StaticMeshReductionSettings(1.,.4),u.StaticMeshReductionSettings(.55,.12),u.StaticMeshReductionSettings(.22,.045),u.StaticMeshReductionSettings(.06,.012)];assert S.set_lods(mesh,red)>=4
  E.save_loaded_asset(mesh);bb=mesh.get_bounding_box();actual=[[bb.min.x,bb.min.y,bb.min.z],[bb.max.x,bb.max.y,bb.max.z]]
  assert max(abs(a-b) for x,y in zip(actual,row['bounds_cm']) for a,b in zip(x,y))<.5,(name,actual,row['bounds_cm'])
  assert 0<mesh.get_num_sections(0)<=len(mesh.static_materials)<=len(row['materials']) and set(str(v.material_slot_name) for v in mesh.static_materials).issubset(row['materials']) and len(set(str(v.material_slot_name) for v in mesh.static_materials))==len(mesh.static_materials),(name,'sections',mesh.get_num_sections(0),len(row['materials']))
  result['meshes']=[x for x in result['meshes'] if x['id']!=row['id']];result['meshes'].append(dict(id=row['id'],name=row['name'],asset=mesh.get_path_name(),bounds_cm=actual,materials=len(mesh.static_materials),source_sha256=row['source_sha256'],door_leaf=False,source=row['source'],license=row['license'],lods=S.get_lod_count(mesh),nanite=row['nanite']));imported.append(row['id']);result['version']=31;(P/'Content/CampaignData/echo_custom_runtime.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));u.log('CARS31_MESH '+str(row['id']));cache.clear();u.SystemLibrary.collect_garbage()
 result['version']=31;(P/'Content/CampaignData/echo_custom_runtime.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));(P/f'Saved/Prototype/cars31_batch_{start}_{ms}.json').write_text(json.dumps(dict(success=True,imported=imported,materials=len(D['materials']))));u.log('CARS31_IMPORT_COMPLETE')
except Exception:
 (P/'Saved/Prototype/cars31_failed.txt').write_text(traceback.format_exc());raise
finally:u.SystemLibrary.quit_editor()
