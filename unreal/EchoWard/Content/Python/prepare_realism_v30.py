"""Scanned asphalt with bounded wet patches; no displacement/tessellation."""
import json,hashlib,traceback
from pathlib import Path
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));R=P.parents[1];E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools();BASE='/Game/Campaign/RealismV30'
def node(m,cls,**kw):
 n=L.create_material_expression(m,cls)
 for k,v in kw.items():n.set_editor_property(k,v)
 return n
def custom(m,code,pins,kind):
 n=node(m,u.MaterialExpressionCustom,code=code,output_type=kind);ins=[]
 for k in pins:q=u.CustomInput();q.set_editor_property('input_name',k);ins.append(q)
 n.set_editor_property('inputs',ins)
 for k,(p,out) in pins.items():assert L.connect_material_expressions(p,out,n,k)
 return n
try:
 d=json.loads((R/'assets/external/quality_v30/ROAD_MANIFEST.json').read_text());textures={}
 for key,q in d['files'].items():
  name='T_Road30_'+key;target=BASE+'/'+name;t=u.load_asset(target)
  if not t:
   task=u.AssetImportTask();task.filename=q['file'];task.destination_path=BASE;task.destination_name=name;task.automated=True;task.save=True;A.import_asset_tasks([task]);t=u.load_asset(target)
  assert t;t.set_editor_property('srgb',key=='albedo');t.set_editor_property('max_texture_size',4096 if key=='albedo' else 2048)
  if key=='normal':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
  elif key=='roughness':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
  assert E.save_loaded_asset(t);textures[key]=t
 m=u.load_asset(BASE+'/M_Road30')
 if not m:
  m=A.create_asset('M_Road30',BASE,u.Material,u.MaterialFactoryNew());m.set_editor_property('tangent_space_normal',False)
  pos=node(m,u.MaterialExpressionWorldPosition);pins={'P':(pos,'')}
  for key,kind in [('albedo',u.MaterialSamplerType.SAMPLERTYPE_COLOR),('normal',u.MaterialSamplerType.SAMPLERTYPE_NORMAL),('roughness',u.MaterialSamplerType.SAMPLERTYPE_MASKS)]:pins[key]=(node(m,u.MaterialExpressionTextureObjectParameter,parameter_name=key,texture=textures[key],sampler_type=kind),'')
  common='float2 uv=P.xy*.0025;float macro=Texture2DSample(roughness,roughnessSampler,uv*.071+float2(.13,.41)).r;float wet=smoothstep(.38,.67,macro)*.72;'
  code=common+'float3 c=Texture2DSample(albedo,albedoSampler,uv).rgb;float rough=Texture2DSample(roughness,roughnessSampler,uv).r;return float4(c*lerp(.72,.42,wet),lerp(clamp(rough*.9,.55,.96),.19,wet));'
  cr=custom(m,code,pins,u.CustomMaterialOutputType.CMOT_FLOAT4)
  for prop,mask in [(u.MaterialProperty.MP_BASE_COLOR,dict(r=True,g=True,b=True,a=False)),(u.MaterialProperty.MP_ROUGHNESS,dict(r=False,g=False,b=False,a=True))]:
   n=node(m,u.MaterialExpressionComponentMask,**mask);assert L.connect_material_expressions(cr,'',n,'');assert L.connect_material_property(n,'',prop)
  n=custom(m,common+'float2 xy=Texture2DSample(normal,normalSampler,uv).rg*2-1;xy*=lerp(.65,.16,wet);return normalize(float3(xy,sqrt(saturate(1-dot(xy,xy)))));',pins,u.CustomMaterialOutputType.CMOT_FLOAT3);assert L.connect_material_property(n,'',u.MaterialProperty.MP_NORMAL)
  for usage in [u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,u.MaterialUsage.MATUSAGE_NANITE]:L.set_base_material_usage(m,usage,True)
  L.recompile_material(m);assert E.save_loaded_asset(m)
 mi=u.load_asset(BASE+'/MI_Road30') or A.create_asset('MI_Road30',BASE,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(mi,m);assert E.save_loaded_asset(mi)
 # Actual native asset and LOD audit, not just source-file existence.
 S=u.get_editor_subsystem(u.StaticMeshEditorSubsystem);a=json.loads((P/'RawAssets/EchoRealV30/manifest.json').read_text());checks=[]
 for q in a['assets']:
  sm=u.load_asset('/Game/Campaign/EchoCustom/Meshes/SM_'+q['name']);assert sm;q['native_lods']=S.get_lod_count(sm);assert q['native_lods']>=(4 if q['triangles']>1500 else 1);checks.append({'id':q['id'],'lods':q['native_lods'],'materials':len(sm.static_materials),'triangles_lod0':q['triangles']})
 assert u.load_asset('/Game/Campaign/EchoCustom/Materials/M_Real30B_OPAQUE_Coat').get_editor_property('shading_model')==u.MaterialShadingModel.MSM_CLEAR_COAT
 (P/'Saved/Prototype/realism30_native_audit.json').write_text(json.dumps({'success':True,'models':checks,'road':mi.get_path_name(),'resolution':'Albedo 4K; normal/roughness 2K; streaming mips'},indent=2));u.log('REALISM30_MATERIAL_PASS')
except Exception:
 (P/'Saved/Prototype/realism30_material_failed.txt').write_text(traceback.format_exc());raise
finally:u.SystemLibrary.quit_editor()
