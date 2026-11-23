"""Title-only cropped 3D monster, rolling ball, blood decals and original forearm."""
from pathlib import Path
import json
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));RAW=ROOT/'RawAssets/MenuEvents';BASE='/Game/Campaign/MenuEvents'
T=u.AssetToolsHelpers.get_asset_tools();L=u.MaterialEditingLibrary;LIB=u.EditorAssetLibrary
def save(a):
    if not LIB.save_loaded_asset(a):raise RuntimeError('Save failed '+a.get_path_name())
def node(m,cls,**props):
    a=L.create_material_expression(m,cls)
    for k,v in props.items():a.set_editor_property(k,v)
    return a
def material(name):
    m=u.load_asset(BASE+'/'+name) or T.create_asset(name,BASE,u.Material,u.MaterialFactoryNew());L.delete_all_material_expressions(m);return m
def scalar(m,prop,value):L.connect_material_property(node(m,u.MaterialExpressionConstant,r=value),'',prop)
def custom(m,code,pins,prop,vector=False):
    n=node(m,u.MaterialExpressionCustom,code=code,output_type=u.CustomMaterialOutputType.CMOT_FLOAT3 if vector else u.CustomMaterialOutputType.CMOT_FLOAT1)
    inputs=[]
    for name in pins:
        p=u.CustomInput();p.set_editor_property('input_name',name);inputs.append(p)
    n.set_editor_property('inputs',inputs)
    for name,(a,pin) in pins.items():L.connect_material_expressions(a,pin,n,name)
    L.connect_material_property(n,'',prop);return n
def finish(m,skinned=False):
    if skinned:L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_SKELETAL_MESH,True)
    L.recompile_material(m);save(m)
m=material('M_MenuZombieCrop');m.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.5)
for kind,prop in [('color',u.MaterialProperty.MP_BASE_COLOR),('normal',u.MaterialProperty.MP_NORMAL)]:
    n=node(m,u.MaterialExpressionTextureSample,texture=u.load_asset('/Game/Campaign/Creatures/T_Stalker_'+kind))
    if kind=='normal':n.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    L.connect_material_property(n,'RGB',prop)
pre=node(m,u.MaterialExpressionPreSkinnedPosition)
pos=node(m,u.MaterialExpressionVertexInterpolator)
if not L.connect_material_expressions(pre,'',pos,'VS'):raise RuntimeError('Cannot interpolate pre-skinned position')
params={key:node(m,u.MaterialExpressionScalarParameter,parameter_name=key,default_value=value) for key,value in [('CutZ',155),('CutX',62),('HandSign',1),('HandMode',0)]}
custom(m,'return lerp(step(CutZ,P.z),step(CutX,P.x*HandSign),HandMode);',{'P':(pos,''),**{key:(n,'') for key,n in params.items()}},u.MaterialProperty.MP_OPACITY_MASK)
scalar(m,u.MaterialProperty.MP_ROUGHNESS,.84);scalar(m,u.MaterialProperty.MP_SPECULAR,.16);finish(m,True)
noise='''struct N {float h(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}float n(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y);}};N n;'''
m=material('M_MenuBall');uv=node(m,u.MaterialExpressionTextureCoordinate)
custom(m,noise+'''float panels=step(.5,frac(UV.x*3));float seam=1-smoothstep(.014,.03,min(frac(UV.x*6),1-frac(UV.x*6)));float dirt=n.n(UV*21)*.6+n.n(UV*73)*.4;return lerp(float3(.20,.012,.007),float3(.42,.36,.23),panels)*(.48+dirt*.52)*(1-seam*.75);''',{'UV':(uv,'')},u.MaterialProperty.MP_BASE_COLOR,True)
scalar(m,u.MaterialProperty.MP_ROUGHNESS,.64);scalar(m,u.MaterialProperty.MP_SPECULAR,.25);finish(m)
m=material('M_MenuArm');uv=node(m,u.MaterialExpressionTextureCoordinate)
custom(m,noise+'''float dirt=n.n(UV*13)*.65+n.n(UV*71)*.35;float veins=pow(saturate(1-abs(sin(UV.x*38+sin(UV.y*13)*.8))*9),2);return lerp(float3(.065,.07,.045),float3(.22,.20,.13),dirt)-veins*float3(.035,.027,.008);''',{'UV':(uv,'')},u.MaterialProperty.MP_BASE_COLOR,True)
scalar(m,u.MaterialProperty.MP_ROUGHNESS,.76);scalar(m,u.MaterialProperty.MP_SPECULAR,.18);finish(m)
m=material('M_MenuBlood');m.set_editor_property('material_domain',u.MaterialDomain.MD_DEFERRED_DECAL);m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
uv=node(m,u.MaterialExpressionTextureCoordinate);seed=node(m,u.MaterialExpressionScalarParameter,parameter_name='Seed',default_value=4);wall=node(m,u.MaterialExpressionScalarParameter,parameter_name='Wall',default_value=0)
code=noise+'''float2 p=UV;float a=n.n(p*9+Seed),b=n.n(p*47-Seed),c=n.n(p*133+Seed);float2 q=(p-.5)*float2(1.1,1.65);q.x+=sin(p.y*13+Seed)*.07;float edge=length(q)+a*.18+b*.045;float smear=1-smoothstep(.28,.39,edge);float drips=pow(saturate(n.n(float2(p.x*37+Seed,0))-.52)*2.08,2)*smoothstep(.98,.30,p.y);float drops=0;for(int i=0;i<11;i++){float2 center=float2(n.h(float2(i+Seed,3)),n.h(float2(7,i+Seed)));float r=.006+.018*n.h(float2(i,Seed));drops=max(drops,1-smoothstep(r,r+.008,length(p-center)));}float edgeFade=smoothstep(0,.07,p.x)*smoothstep(0,.07,p.y)*smoothstep(0,.07,1-p.x)*smoothstep(0,.07,1-p.y);return saturate((smear*(.63+b*.26)+Wall*drips*.55+drops*.72)*edgeFade)*(1-step(.95,c)*.35);'''
custom(m,code,{'UV':(uv,''),'Seed':(seed,''),'Wall':(wall,'')},u.MaterialProperty.MP_OPACITY)
L.connect_material_property(node(m,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(.19,.0025,.004,1)),'',u.MaterialProperty.MP_BASE_COLOR);scalar(m,u.MaterialProperty.MP_ROUGHNESS,.35);scalar(m,u.MaterialProperty.MP_SPECULAR,.22);finish(m)
def imp(path,name,opts=None):
    t=u.AssetImportTask()
    for k,v in dict(filename=str(path),destination_path=BASE,destination_name=name,automated=True,save=True,replace_existing=True,replace_existing_settings=True).items():t.set_editor_property(k,v)
    if opts:t.set_editor_property('options',opts);t.set_editor_property('factory',u.FbxFactory())
    T.import_asset_tasks([t]);a=u.load_asset(BASE+'/'+name)
    if not a:raise RuntimeError('Missing '+name)
    return a
o=u.FbxImportUI()
for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,original_import_type=u.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,is_obj_import=True,import_materials=False,import_textures=False,override_full_name=True).items():o.set_editor_property(k,v)
d=o.get_editor_property('static_mesh_import_data')
for k,v in dict(convert_scene=False,convert_scene_unit=False,combine_meshes=True,auto_generate_collision=False,build_nanite=False).items():d.set_editor_property(k,v)
a=imp(RAW/'forearm.obj','SM_MenuForearm',o);a.set_material(0,u.load_asset(BASE+'/M_MenuArm'));save(a)
for name in ['menu_roll','menu_drag']:
    a=imp(RAW/(name+'.wav'),'A_'+name);a.set_editor_property('looping',False);save(a)
(ROOT/'Saved/Prototype/menu_events_import.json').write_text(json.dumps({'success':True,'materials':['M_MenuZombieCrop','M_MenuBall','M_MenuArm','M_MenuBlood'],'geometry':'SM_MenuForearm','audio':['menu_roll','menu_drag']},indent=2))

