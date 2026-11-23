"""Native, world-scaled hospital surfaces and a subdued overcast sky."""
import unreal as u
L=u.MaterialEditingLibrary; A=u.AssetToolsHelpers.get_asset_tools(); E=u.EditorAssetLibrary
BASE='/Game/Campaign/BookWorld'
def node(m,c,**props):
    n=L.create_material_expression(m,c)
    for k,v in props.items():n.set_editor_property(k,v)
    return n
def material(name):
    m=u.load_asset(BASE+'/'+name) or A.create_asset(name,BASE,u.Material,u.MaterialFactoryNew())
    L.delete_all_material_expressions(m)
    return m
def custom(m,code,pins,prop):
    n=node(m,u.MaterialExpressionCustom,code=code,output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs=[]
    for name in pins:
        p=u.CustomInput();p.set_editor_property('input_name',name);inputs.append(p)
    n.set_editor_property('inputs',inputs)
    for k,p in pins.items():L.connect_material_expressions(p,'',n,k)
    L.connect_material_property(n,'',prop)
def finish(m):
    L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True)
    L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_NANITE,True)
    L.recompile_material(m)
    if not E.save_loaded_asset(m):raise RuntimeError('Could not save '+m.get_name())
m=material('M_HospitalSurface')
pos=node(m,u.MaterialExpressionWorldPosition)
normal=node(m,u.MaterialExpressionVertexNormalWS)
tint=node(m,u.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=u.LinearColor(.45,.48,.45,1))
kind=node(m,u.MaterialExpressionScalarParameter,parameter_name='Surface',default_value=0.)
wear=node(m,u.MaterialExpressionScalarParameter,parameter_name='Weathering',default_value=.28)
custom(m,'''
float3 c=Tint;
float2 uv=abs(N.z)>.5?P.xy:(abs(N.x)>.5?P.yz:P.xz);
float grain=frac(sin(dot(floor(uv*1.4),float2(127.1,311.7)))*43758.5453);
float dirt=0.;float weight=.57;
for(int i=0;i<3;i++){
 float2 p=uv/(160./exp2(i));float2 b=floor(p),f=frac(p);f=f*f*(3.-2.*f);
 float4 n=frac(sin(float4(dot(b,float2(127.1,311.7)),dot(b+float2(1,0),float2(127.1,311.7)),dot(b+float2(0,1),float2(127.1,311.7)),dot(b+1.,float2(127.1,311.7))))*43758.5453);
 dirt+=lerp(lerp(n.x,n.y,f.x),lerp(n.z,n.w,f.x),f.y)*weight;weight*=.5;
}
c*=.97+grain*.035-Wear*smoothstep(.4,.78,dirt)*.33;
if(abs(N.z)<.5){
 float base=1.-smoothstep(3.,45.,fmod(abs(P.z)+.01,480.));
 c*=1.-Wear*base*(.18+dirt*.32);
 float chip=smoothstep(.90,.985,grain)*smoothstep(.60,.78,dirt);
 c=lerp(c,float3(.19,.18,.16),chip*Wear*.5);
}
if(Kind>.5 && Kind<1.5){
 float2 q=frac(P.xy/60.);q=min(q,1-q);
 float aa=max(length(ddx(P.xy/60.)),length(ddy(P.xy/60.)));
 float grout=1-smoothstep(.008,.014+aa,min(q.x,q.y));c*=lerp(1.,.52,grout);
}
if(Kind>1.5 && Kind<2.5){
 float2 q=frac(P.xy/90.);q=min(q,1-q);
 float aa=max(length(ddx(P.xy/90.)),length(ddy(P.xy/90.)));
 c*=lerp(1.,.48,1-smoothstep(.007,.014+aa,min(q.x,q.y)));
}
if(Kind>2.5){
 float2 p=abs(N.x)>.5?P.yz:P.xz;
 float row=floor(p.y/24.);float2 q=frac(float2(p.x/48.+fmod(row,2.)*.5,p.y/24.));q=min(q,1-q);
 c*=lerp(1.,.5,1-smoothstep(.018,.035,min(q.x,q.y)));
}
return c;
''',{'P':pos,'N':normal,'Tint':tint,'Kind':kind,'Wear':wear},u.MaterialProperty.MP_BASE_COLOR)
rough=node(m,u.MaterialExpressionScalarParameter,parameter_name='Roughness',default_value=.8)
wet=node(m,u.MaterialExpressionScalarParameter,parameter_name='Wetness',default_value=0.)
custom(m,'''
float mottling=sin(P.x*.009+sin(P.y*.014))*sin(P.y*.011+P.x*.003);
float r=clamp(Rough+(.5+sin(P.x*.7)*sin(P.y*.93))*.025-Wet*smoothstep(-.45,.7,mottling),.18,.96);
return float3(r,r,r);
''',{'P':pos,'Rough':rough,'Wet':wet},u.MaterialProperty.MP_ROUGHNESS)
metal=node(m,u.MaterialExpressionScalarParameter,parameter_name='Metallic',default_value=0.)
L.connect_material_property(metal,'',u.MaterialProperty.MP_METALLIC)
emission=node(m,u.MaterialExpressionVectorParameter,parameter_name='Emission',default_value=u.LinearColor(.008,.011,.012,1))
L.connect_material_property(emission,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)
m=material('M_HospitalSky');m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT);m.set_editor_property('two_sided',True)
view=node(m,u.MaterialExpressionCameraVectorWS)
bright=node(m,u.MaterialExpressionScalarParameter,parameter_name='Bright',default_value=1.)
custom(m,'''
float h=saturate(abs(V.z));
float3 c=lerp(float3(.065,.085,.115),float3(.012,.022,.047),pow(h,.55));
float cloud=sin(V.x*13+sin(V.y*9))*sin(V.y*17+V.z*12);
return lerp(c+cloud*.007,float3(5.,6.,8.)+cloud*.7,saturate(Bright));
''',{'V':view,'Bright':bright},u.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)
u.log('HOSPITAL_MATERIALS_READY world_scale_cm=60 sky=overcast')

# Small real vision panels. Translucency uses ordinary cube instances, avoiding
# the unsupported Nanite translucent-material path used by opaque architecture.
m=material('M_HospitalGlass')
m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property('two_sided',True)
color=node(m,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(.12,.24,.26,1))
alpha=node(m,u.MaterialExpressionConstant,r=.16)
rough=node(m,u.MaterialExpressionConstant,r=.18)
L.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
L.connect_material_property(alpha,'',u.MaterialProperty.MP_OPACITY)
L.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True)
L.recompile_material(m)
if not E.save_loaded_asset(m):raise RuntimeError('Could not save hospital glazing')

sky_asset=BASE+'/SM_HospitalSky'
if not u.load_asset(sky_asset):
    sky=u.EditorAssetLibrary.duplicate_asset('/Engine/BasicShapes/Sphere',sky_asset)
    if not sky or not E.save_loaded_asset(sky):raise RuntimeError('Could not save sky sphere')
