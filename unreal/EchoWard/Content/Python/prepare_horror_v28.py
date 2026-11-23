"""New architecture materials from the already licensed local 4K sources."""
import json, traceback
from pathlib import Path
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));BASE='/Game/Campaign/HorrorV28';E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools()
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
 t=u.load_asset('/Game/Campaign/DetailV26/T_D26_'+ident);assert t,ident;return t
try:
 name='M_HospitalPatina28B';m=u.load_asset(BASE+'/'+name)
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
  weather=noise+'float macro=q.n(P.xy*.0027);float up=saturate(N.z);float floorHeight=frac((P.z+960)/480)*480;float foot=(1-smoothstep(0,48,floorHeight))*(1-up);float damp=smoothstep(.56,.78,macro)*Wetness*up;float2 cell=frac(P.xy/60);float2 edge2=min(cell,1-cell);float edge=min(edge2.x,edge2.y);float seam=(1-smoothstep(.004,.012,edge))*Tiles*up;float bevel=(1-smoothstep(.012,.045,edge))*Tiles*up;float2 face=uv/max(Scale,.001);float2 shifted=float2(face.x*.8-face.y*.6,face.x*.6+face.y*.8);float flake=q.n(shifted*3.4)*.5+q.n(face*9.7+4.1)*.3+q.n(shifted*37.3)*.2;float chip=smoothstep(.69,.84,flake)*(1-up);float tileShade=.88+q.h(floor(P.xy/60))*.23;float drip=smoothstep(.61,.85,q.n(float2(face.x*14,face.y*.27)))*(1-up);\n'
  color=common+weather+f'float3 albedo={sample("Albedo")}.rgb;float lum=dot(albedo,float3(.2126,.7152,.0722));float3 c=lerp(albedo,lum.xxx,Desaturate)*Tint;float dirt=(foot*.65+q.n(P.xy*.007+P.zz*.002)*.35)*Grime;c*=1-dirt-drip*Grime*.35;c=lerp(c,c*.55+float3(.095,.08,.06),chip*.7);c*=lerp(1,tileShade,Tiles*up);c*=1-seam*.70;c=lerp(c,c*.67,damp);float rough=clamp({sample("RoughnessMap")}.r*Roughness,.34,.97);return float4(c,lerp(lerp(rough,.17,damp),.92,seam));'
  cr=custom(m,color,pins,u.CustomMaterialOutputType.CMOT_FLOAT4)
  for prop,channels in [(u.MaterialProperty.MP_BASE_COLOR,dict(r=True,g=True,b=True,a=False)),(u.MaterialProperty.MP_ROUGHNESS,dict(r=False,g=False,b=False,a=True))]:
   mask=node(m,u.MaterialExpressionComponentMask,**channels);L.connect_material_expressions(cr,'',mask,'');L.connect_material_property(mask,'',prop)
  normalcode=common+weather+'''float2 a=Texture2DSample(Normal,NormalSampler,uv).rg*2-1;float nz=sqrt(saturate(1-dot(a,a)));float3 mapped=an.x>an.y&&an.x>an.z?float3(nz*sign(N.x),a.x,a.y):an.y>an.z?float3(a.x,nz*sign(N.y),a.y):float3(a.x,a.y,nz*sign(N.z));float3 result=normalize(lerp(normalize(N),mapped,NormalStrength*(1-damp*.65)));float2 inward=(.5-cell)*2;float2 bevelNormal=(edge2.x<edge2.y?float2(sign(inward.x),0):float2(0,sign(inward.y)))*bevel*.18;return normalize(result+float3(bevelNormal,0));'''
  L.connect_material_property(custom(m,normalcode,pins,u.CustomMaterialOutputType.CMOT_FLOAT3),'',u.MaterialProperty.MP_NORMAL)
  em=node(m,u.MaterialExpressionVectorParameter,parameter_name='Emission',default_value=u.LinearColor(0,0,0,1));L.connect_material_property(em,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
  spec=node(m,u.MaterialExpressionConstant,r=.38);L.connect_material_property(spec,'',u.MaterialProperty.MP_SPECULAR)
  L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);L.recompile_material(m);save(m)
 L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_NANITE,True);L.recompile_material(m);save(m)
 specs={'Plaster':(['a2f08e6cd5305c97','27c2fbe0d6724fbc','3a9f2e212f9d1af2'],[.62,.61,.58],.94,.48,.40,.95,.30,0),
        'Paint':(['a569e90eb94884bd','a4be803bf483bea2','fb2ed1411f678222'],[.23,.29,.31],.84,.68,.52,.94,.37,0),
        'Floor':(['5964baf57bcf1a38','3844fe61a1836855','03112f6ca90acaff'],[.36,.40,.41],.85,.70,.58,.65,.25,.58),
        'Facade':(['a2f08e6cd5305c97','27c2fbe0d6724fbc','3a9f2e212f9d1af2'],[.46,.45,.42],.94,.52,.28,.96,.32,0)}
 for key,(textures,rgb,rough,strength,scale,desat,grime,wet) in specs.items():
  path=BASE+'/MI_'+key;mi=u.load_asset(path) or A.create_asset('MI_'+key,BASE,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(mi,m);L.set_material_instance_vector_parameter_value(mi,'Tint',u.LinearColor(*rgb,1))
  for param,value in [('Roughness',rough),('NormalStrength',strength),('Scale',scale),('Desaturate',desat),('Grime',grime),('Wetness',wet),('Tiles',1 if key=='Floor' else 0)]:L.set_material_instance_scalar_parameter_value(mi,param,value)
  for param,ident,kind in zip(['Albedo','Normal','RoughnessMap'],textures,['color','normal','mask']):L.set_material_instance_texture_parameter_value(mi,param,texture(ident,kind))
  save(mi)
 sm=u.load_asset(BASE+'/M_HorrorTrace28B')
 if not sm:
  sm=A.create_asset('M_HorrorTrace28B',BASE,u.Material,u.MaterialFactoryNew());sm.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED);sm.set_editor_property('two_sided',True);sm.set_editor_property('opacity_mask_clip_value',.33)
  uv=node(sm,u.MaterialExpressionTextureCoordinate);typ=node(sm,u.MaterialExpressionScalarParameter,parameter_name='Kind',default_value=0)
  noise="""struct Q{float h(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}float n(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y);}}v;"""
  code=noise+"""float2 q=UV*2-1;float2 drift=float2(v.n(UV*9+2),v.n(UV*11+14))-.5;float r=length(q+drift*.35+float2(.09,-.03));float body=1-smoothstep(.50,.80,r+(v.n(UV*17)-.5)*.22);float droplets=0;for(int i=0;i<7;i++){float2 c=float2(v.h(float2(i,7)),v.h(float2(i,19)))*1.6-.8;float rr=.022+v.h(float2(i,39))*.06;droplets=max(droplets,1-smoothstep(rr*.7,rr,length(q-c)));}float holes=smoothstep(.18,.34,v.n(UV*53));float mask=max(body*holes,droplets);if(Kind>1.5&&Kind<2.5){mask=0;for(int i=0;i<5;i++){float y=-.65+i*.29+(v.n(float2(UV.x*8,i))-.5)*.16;float width=.018+v.h(float2(i,4))*.05;float strokeCoverage=1-smoothstep(width*.5,width,abs(q.y-y));mask=max(mask,strokeCoverage*saturate(1-abs(q.x)*1.05));}mask*=smoothstep(.18,.45,v.n(UV*37));}if(Kind>2.5){float palm=1-smoothstep(.27,.35,length((q-float2(0,-.18))*float2(1,1.15)));float fingers=0;for(int i=0;i<4;i++){float2 d=(q-float2(-.29+i*.18,.32+sin(i*.8)*.08))*float2(1,.34);fingers=max(fingers,1-smoothstep(.065,.09,length(d)));}mask=max(palm,fingers)*holes;}return saturate(mask);"""
  c=custom(sm,code,{'UV':(uv,''),'Kind':(typ,'')},u.CustomMaterialOutputType.CMOT_FLOAT1);L.connect_material_property(c,'',u.MaterialProperty.MP_OPACITY_MASK)
  tint=node(sm,u.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=u.LinearColor(.045,.003,.001,1))
  color=custom(sm,noise+'float n=v.n(UV*22);float fine=v.n(UV*91);return Tint*(.28+.5*n+.22*fine);',{'UV':(uv,''),'Tint':(tint,'RGB')},u.CustomMaterialOutputType.CMOT_FLOAT3);L.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
  spec=node(sm,u.MaterialExpressionConstant,r=.17);L.connect_material_property(spec,'',u.MaterialProperty.MP_SPECULAR)
  ro=node(sm,u.MaterialExpressionScalarParameter,parameter_name='Roughness',default_value=.4);L.connect_material_property(ro,'',u.MaterialProperty.MP_ROUGHNESS)
  L.set_base_material_usage(sm,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);L.set_base_material_usage(sm,u.MaterialUsage.MATUSAGE_NANITE,True);L.recompile_material(sm);save(sm)
 for key,rgb,rough,kind in [('Blood',(.032,.0015,.0007),.62,0),('Damp',(.024,.031,.032),.49,0),('Water',(.045,.055,.061),.13,0),('Drag',(.026,.017,.012),.79,2),('Hand',(.055,.003,.001),.54,3)]:
  mi=u.load_asset(BASE+'/MI_'+key) or A.create_asset('MI_'+key,BASE,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(mi,sm);L.set_material_instance_vector_parameter_value(mi,'Tint',u.LinearColor(*rgb,1));L.set_material_instance_scalar_parameter_value(mi,'Roughness',rough);L.set_material_instance_scalar_parameter_value(mi,'Kind',kind);save(mi)
 plane=u.load_asset(BASE+'/SM_TracePlane') or E.duplicate_asset('/Engine/BasicShapes/Plane',BASE+'/SM_TracePlane');assert plane;plane.set_material(0,u.load_asset(BASE+'/MI_Damp'));save(plane)
 (P/'Saved/Prototype/horror_v28_materials.json').write_text(json.dumps(dict(success=True,materials=list(specs),architecture_parent=m.get_name(),trace_parent=sm.get_name(),albedo_resolution=4096,normal_resolution=2048,source='Existing licensed Poly Haven texture sources; no new downloads.')),encoding='utf8');u.log('HORROR_V28_MATERIALS_COMPLETE')
except Exception:
 (P/'Saved/Prototype/horror_v28_materials_failed.txt').write_text(traceback.format_exc(),encoding='utf8');raise

finally:u.SystemLibrary.quit_editor()
