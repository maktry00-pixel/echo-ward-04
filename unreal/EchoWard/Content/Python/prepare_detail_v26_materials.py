"""New architecture materials from the already licensed local 4K sources."""
import json, traceback
from pathlib import Path
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));BASE='/Game/Campaign/DetailV26';E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools()
def node(m,cls,**props):
 n=L.create_material_expression(m,cls)
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def save(a):assert E.save_loaded_asset(a)
def custom(m,code,pins,kind):
 n=node(m,u.MaterialExpressionCustom,code=code,output_type=kind);inputs=[]
 for key in pins:p=u.CustomInput();p.set_editor_property('input_name',key);inputs.append(p)
 n.set_editor_property('inputs',inputs)
 for key,(a,out) in pins.items():assert L.connect_material_expressions(a,out,n,key)
 return n
def texture(ident,kind):
 name='T_D26_'+ident;path=BASE+'/'+name;t=u.load_asset(path)
 if not t:
  task=u.AssetImportTask();task.filename=str(P/'RawAssets/Campaign/Textures'/('T_'+ident+'.png'));task.destination_path=BASE;task.destination_name=name;task.automated=True;task.save=True;A.import_asset_tasks([task]);t=u.load_asset(path)
 assert t,ident;t.set_editor_property('srgb',kind=='color');t.set_editor_property('max_texture_size',4096 if kind=='color' else 2048)
 if kind=='normal':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
 elif kind=='mask':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
 save(t);return t
try:
 name='M_HospitalDetail26B';m=u.load_asset(BASE+'/'+name)
 if not m:
  m=A.create_asset(name,BASE,u.Material,u.MaterialFactoryNew());m.set_editor_property('tangent_space_normal',False)
  pos=node(m,u.MaterialExpressionWorldPosition);normal=node(m,u.MaterialExpressionVertexNormalWS);vn=node(m,u.MaterialExpressionVertexInterpolator);L.connect_material_expressions(normal,'',vn,'')
  pins={'P':(pos,''),'N':(vn,'')}
  for key,default in [('Roughness',.8),('NormalStrength',.65),('Wetness',0),('Scale',.5),('Desaturate',.85),('Grime',.18),('Tiles',0)]:pins[key]=(node(m,u.MaterialExpressionScalarParameter,parameter_name=key,default_value=default),'')
  pins['Tint']=(node(m,u.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=u.LinearColor(.55,.58,.58,1)),'RGB')
  for key,default,kind in [('Albedo','T_White',u.MaterialSamplerType.SAMPLERTYPE_COLOR),('Normal','T_FlatNormal',u.MaterialSamplerType.SAMPLERTYPE_NORMAL),('RoughnessMap','T_WhiteMasks',u.MaterialSamplerType.SAMPLERTYPE_MASKS)]:pins[key]=(node(m,u.MaterialExpressionTextureObjectParameter,parameter_name=key,texture=u.load_asset('/Game/Campaign/Textures/'+default),sampler_type=kind),'')
  common='float3 an=abs(N);float3 p=P*.01*Scale;float2 uv=an.x>an.y&&an.x>an.z?p.yz:an.y>an.z?p.xz:p.xy;\n'
  def sample(key):return f'Texture2DSample({key},{key}Sampler,uv)'
  noise='struct Q{float h(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}float n(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y);}}q;\n'
  weather=noise+'float macro=q.n(P.xy*.0027);float up=saturate(N.z);float floorHeight=frac((P.z+960)/480)*480;float foot=(1-smoothstep(0,48,floorHeight))*(1-up);float damp=smoothstep(.56,.78,macro)*Wetness*up;float2 cell=frac(P.xy/60);float2 edge2=min(cell,1-cell);float edge=min(edge2.x,edge2.y);float seam=(1-smoothstep(.004,.012,edge))*Tiles*up;float bevel=(1-smoothstep(.012,.045,edge))*Tiles*up;\n'
  color=common+weather+f'float3 albedo={sample("Albedo")}.rgb;float lum=dot(albedo,float3(.2126,.7152,.0722));float3 c=lerp(albedo,lum.xxx,Desaturate)*Tint;float dirt=(foot*.65+q.n(P.xy*.007+P.zz*.002)*.35)*Grime;c*=1-dirt;c*=1-seam*.67;c=lerp(c,c*.67,damp);float rough=clamp({sample("RoughnessMap")}.r*Roughness,.34,.97);return float4(c,lerp(lerp(rough,.17,damp),.92,seam));'
  cr=custom(m,color,pins,u.CustomMaterialOutputType.CMOT_FLOAT4)
  for prop,channels in [(u.MaterialProperty.MP_BASE_COLOR,dict(r=True,g=True,b=True,a=False)),(u.MaterialProperty.MP_ROUGHNESS,dict(r=False,g=False,b=False,a=True))]:
   mask=node(m,u.MaterialExpressionComponentMask,**channels);L.connect_material_expressions(cr,'',mask,'');L.connect_material_property(mask,'',prop)
  normalcode=common+weather+'''float2 a=Texture2DSample(Normal,NormalSampler,uv).rg*2-1;float nz=sqrt(saturate(1-dot(a,a)));float3 mapped=an.x>an.y&&an.x>an.z?float3(nz*sign(N.x),a.x,a.y):an.y>an.z?float3(a.x,nz*sign(N.y),a.y):float3(a.x,a.y,nz*sign(N.z));float3 result=normalize(lerp(normalize(N),mapped,NormalStrength*(1-damp*.65)));float2 inward=(.5-cell)*2;float2 bevelNormal=(edge2.x<edge2.y?float2(sign(inward.x),0):float2(0,sign(inward.y)))*bevel*.18;return normalize(result+float3(bevelNormal,0));'''
  L.connect_material_property(custom(m,normalcode,pins,u.CustomMaterialOutputType.CMOT_FLOAT3),'',u.MaterialProperty.MP_NORMAL)
  em=node(m,u.MaterialExpressionVectorParameter,parameter_name='Emission',default_value=u.LinearColor(0,0,0,1));L.connect_material_property(em,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
  spec=node(m,u.MaterialExpressionConstant,r=.38);L.connect_material_property(spec,'',u.MaterialProperty.MP_SPECULAR)
  L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);L.recompile_material(m);save(m)
 L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_NANITE,True);L.recompile_material(m);save(m)
 specs={'Plaster':(['a2f08e6cd5305c97','27c2fbe0d6724fbc','3a9f2e212f9d1af2'],[.71,.70,.67],.92,.4,.40,.95,.16,0),
        'Paint':(['a569e90eb94884bd','a4be803bf483bea2','fb2ed1411f678222'],[.25,.33,.36],.81,.62,.52,.88,.21,0),
        'Floor':(['5964baf57bcf1a38','3844fe61a1836855','03112f6ca90acaff'],[.43,.48,.49],.88,.72,.58,.25,.12,.30),
        'Facade':(['a2f08e6cd5305c97','27c2fbe0d6724fbc','3a9f2e212f9d1af2'],[.52,.49,.44],.93,.50,.28,.96,.24,0)}
 for key,(textures,rgb,rough,strength,scale,desat,grime,wet) in specs.items():
  path=BASE+'/MI_'+key;mi=u.load_asset(path) or A.create_asset('MI_'+key,BASE,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(mi,m);L.set_material_instance_vector_parameter_value(mi,'Tint',u.LinearColor(*rgb,1))
  for param,value in [('Roughness',rough),('NormalStrength',strength),('Scale',scale),('Desaturate',desat),('Grime',grime),('Wetness',wet),('Tiles',1 if key=='Floor' else 0)]:L.set_material_instance_scalar_parameter_value(mi,param,value)
  for param,ident,kind in zip(['Albedo','Normal','RoughnessMap'],textures,['color','normal','mask']):L.set_material_instance_texture_parameter_value(mi,param,texture(ident,kind))
  save(mi)
 (P/'Saved/Prototype/detail_v26_materials.json').write_text(json.dumps(dict(success=True,materials=list(specs),albedo_resolution=4096,normal_resolution=2048,source='Existing licensed Poly Haven texture sources; no new downloads.')),encoding='utf8');u.log('DETAIL_V26_MATERIALS_COMPLETE')
except Exception:
 (P/'Saved/Prototype/detail_v26_materials_failed.txt').write_text(traceback.format_exc(),encoding='utf8');raise
