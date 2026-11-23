"""Reproducible MIT skeletal nurse + original cloth and audio for the title only."""
from pathlib import Path
import json
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
SRC=ROOT.parents[1]/'assets/models/menu_nurse'
RAW=ROOT/'RawAssets/MenuNurse';BASE='/Game/Campaign/MenuNurse'
LIB=u.EditorAssetLibrary;TOOLS=u.AssetToolsHelpers.get_asset_tools();M=u.MaterialEditingLibrary
def save(a):
    if not LIB.save_loaded_asset(a):raise RuntimeError('Save failed: '+a.get_path_name())
def imp(source,name,opts=None):
    task=u.AssetImportTask()
    for k,v in dict(filename=str(source),destination_path=BASE,destination_name=name,automated=True,save=True,replace_existing=True,replace_existing_settings=True).items():task.set_editor_property(k,v)
    if opts:task.set_editor_property('options',opts);task.set_editor_property('factory',u.FbxFactory())
    TOOLS.import_asset_tasks([task]);a=u.load_asset(BASE+'/'+name)
    if not a:raise RuntimeError('Import failed '+str(source))
    return a
def fbx(kind):
    o=u.FbxImportUI()
    for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=kind,original_import_type=kind,import_materials=False,import_textures=False,override_full_name=True).items():o.set_editor_property(k,v)
    return o
o=fbx(u.FBXImportType.FBXIT_SKELETAL_MESH)
for k,v in dict(import_mesh=True,import_as_skeletal=True,import_animations=False,create_physics_asset=False).items():o.set_editor_property(k,v)
o.get_editor_property('skeletal_mesh_import_data').set_editor_property('import_morph_targets',True)
mesh=imp(SRC/'Medical_Female_01_facial.fbx','SK_MenuNurseFace',o);skeleton=mesh.get_editor_property('skeleton');save(skeleton)
clips={}
for src,name in [('m_idle_breathe_01.max.fbx','A_NurseIdle'),('m_walk_neutral.max.fbx','A_NurseWalk')]:
    o=fbx(u.FBXImportType.FBXIT_ANIMATION)
    for k,v in dict(import_mesh=False,import_as_skeletal=True,import_animations=True,skeleton=skeleton,override_animation_name=name).items():o.set_editor_property(k,v)
    a=imp(ROOT.parents[1]/'assets/models/rocketbox/motion'/src,name,o)
    a.set_editor_property('enable_root_motion',True);a.set_editor_property('force_root_lock',True);save(a);clips[name]=a.get_play_length()
def node(m,cls,**props):
    a=M.create_material_expression(m,cls)
    for k,v in props.items():a.set_editor_property(k,v)
    return a
def material(name):
    a=u.load_asset(BASE+'/'+name) or TOOLS.create_asset(name,BASE,u.Material,u.MaterialFactoryNew());M.delete_all_material_expressions(a);return a
def scalar(m,prop,v):M.connect_material_property(node(m,u.MaterialExpressionConstant,r=v),'',prop)
def custom(m,code,pins):
    n=node(m,u.MaterialExpressionCustom,code=code,output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
    args=[]
    for key in pins:
        p=u.CustomInput();p.set_editor_property('input_name',key);args.append(p)
    n.set_editor_property('inputs',args)
    for key,(expression,pin) in pins.items():M.connect_material_expressions(expression,pin,n,key)
    M.connect_material_property(n,'',u.MaterialProperty.MP_BASE_COLOR)
    return n
mats={}
noise='float h=frac(sin(dot(floor(UV*97),float2(127.1,311.7)))*43758.5453); float dirt=smoothstep(.15,.85,sin(UV.x*31+sin(UV.y*17))*sin(UV.y*23)*.5+.5);'
for part in ['body','head','opacity']:
    mat=material('M_Nurse_'+part)
    tex=imp(SRC/f'f152_{part}_color.tga','T_Nurse_'+part)
    color=node(mat,u.MaterialExpressionTextureSample,texture=tex)
    uv=node(mat,u.MaterialExpressionTextureCoordinate)
    code=noise+'float l=dot(Color,float3(.3,.59,.11)); return lerp(Color,float3(l,l*.98,l*.86),.78)*lerp(float3(.38,.43,.34),float3(.82,.80,.66),dirt)*(1-h*.08);'
    custom(mat,code,{'UV':(uv,''),'Color':(color,'RGB')})
    if part=='opacity':
        mat.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED);mat.set_editor_property('two_sided',True);M.connect_material_property(color,'A',u.MaterialProperty.MP_OPACITY_MASK)
    else:
        normal=imp(SRC/f'f152_{part}_normal.tga','T_Nurse_'+part+'_normal');normal.set_editor_property('srgb',False);normal.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);normal.set_editor_property('flip_green_channel',True);save(normal)
        n=node(mat,u.MaterialExpressionTextureSample,texture=normal,sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL);M.connect_material_property(n,'RGB',u.MaterialProperty.MP_NORMAL)
    scalar(mat,u.MaterialProperty.MP_ROUGHNESS,.88);scalar(mat,u.MaterialProperty.MP_SPECULAR,.18)
    M.set_base_material_usage(mat,u.MaterialUsage.MATUSAGE_SKELETAL_MESH,True);M.set_base_material_usage(mat,u.MaterialUsage.MATUSAGE_MORPH_TARGETS,True);M.recompile_material(mat);save(mat);mats[part]=mat
slots=list(mesh.get_editor_property('materials'))
for slot in slots:
    n=str(slot.get_editor_property('material_slot_name')).lower();slot.set_editor_property('material_interface',mats['opacity' if 'opacity' in n else 'head' if 'head' in n else 'body'])
mesh.set_editor_property('materials',slots);save(mesh)
cloth=material('M_NurseCloth');uv=node(cloth,u.MaterialExpressionTextureCoordinate)
custom(cloth,noise+'float seam=pow(abs(sin(UV.y*19)),24)*.08; float weave=sin(UV.x*1100)*sin(UV.y*750)*.012; return lerp(float3(.17,.14,.08),float3(.55,.54,.40),dirt)+weave-seam;',{'UV':(uv,'')})
cloth.set_editor_property('two_sided',True);scalar(cloth,u.MaterialProperty.MP_ROUGHNESS,.96);M.recompile_material(cloth);save(cloth)
for name in ['nurse_cap','nurse_wrap']:
    o=fbx(u.FBXImportType.FBXIT_STATIC_MESH)
    for k,v in dict(import_as_skeletal=False,import_mesh=True,is_obj_import=True).items():o.set_editor_property(k,v)
    d=o.get_editor_property('static_mesh_import_data')
    for k,v in dict(convert_scene=False,convert_scene_unit=False,combine_meshes=True,auto_generate_collision=False,build_nanite=False).items():d.set_editor_property(k,v)
    a=imp(RAW/(name+'.obj'),'SM_'+name,o)
    a.set_material(0,cloth);save(a)
for name in ['menu_door','menu_scare','menu_footstep']:
    a=imp(RAW/(name+'.wav'),'A_'+name);a.set_editor_property('looping',False);save(a)
cloth_bounds={}
for name in ['nurse_cap','nurse_wrap']:
    a=u.load_asset(BASE+'/SM_'+name);b=a.get_bounds();cloth_bounds[name]={'origin':[b.origin.x,b.origin.y,b.origin.z],'extent':[b.box_extent.x,b.box_extent.y,b.box_extent.z]}
b=mesh.get_bounds()
report={'cloth_bounds':cloth_bounds,'success':True,'mesh':mesh.get_path_name(),'clips':clips,'bounds_origin':[b.origin.x,b.origin.y,b.origin.z],'bounds_extent':[b.box_extent.x,b.box_extent.y,b.box_extent.z],'morphs':[m.get_name() for m in mesh.get_editor_property('morph_targets')],'slots':[str(s.get_editor_property('material_slot_name')) for s in slots],'source':'https://github.com/microsoft/Microsoft-Rocketbox','license':'MIT, original cloth geometry and audio'}
(ROOT/'Saved/Prototype/menu_nurse_import.json').write_text(json.dumps(report,indent=2));u.log('MENU_NURSE '+json.dumps(report))

