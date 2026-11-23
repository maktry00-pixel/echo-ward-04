"""Pixelhouse CC BY 3.0 zombie, its original walk, and menu-only assets."""
from pathlib import Path
import json
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
SRC=ROOT.parents[1]/'assets/models/menu_zombie/pixelhouse'
BASE='/Game/Campaign/GameMenu';T=u.AssetToolsHelpers.get_asset_tools();L=u.MaterialEditingLibrary;LIB=u.EditorAssetLibrary
def save(a):
    if not LIB.save_loaded_asset(a):raise RuntimeError('Cannot save '+a.get_path_name())
def imp(path,name,opts=None):
    t=u.AssetImportTask()
    for k,v in dict(filename=str(path),destination_path=BASE,destination_name=name,automated=True,save=True,replace_existing=True,replace_existing_settings=True).items():t.set_editor_property(k,v)
    if opts:t.set_editor_property('options',opts);t.set_editor_property('factory',u.FbxFactory())
    T.import_asset_tasks([t]);a=u.load_asset(BASE+'/'+name)
    if not a:raise RuntimeError('Missing '+name)
    return a
def fbx(kind):
    o=u.FbxImportUI()
    for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=kind,original_import_type=kind,import_materials=False,import_textures=False,override_full_name=True).items():o.set_editor_property(k,v)
    return o
o=fbx(u.FBXImportType.FBXIT_SKELETAL_MESH)
for k,v in dict(import_mesh=True,import_as_skeletal=True,import_animations=False,create_physics_asset=False).items():o.set_editor_property(k,v)
mesh=imp(SRC/'walk.FBX','SK_MenuZombie',o);sk=mesh.get_editor_property('skeleton');save(sk)
o=fbx(u.FBXImportType.FBXIT_ANIMATION)
for k,v in dict(import_mesh=False,import_as_skeletal=True,import_animations=True,skeleton=sk,override_animation_name='A_MenuZombieWalk').items():o.set_editor_property(k,v)
clip=imp(SRC/'walk.FBX','A_MenuZombieWalk',o);clip.set_editor_property('enable_root_motion',True);clip.set_editor_property('force_root_lock',True);save(clip)
m=u.load_asset(BASE+'/M_MenuZombie') or T.create_asset('M_MenuZombie',BASE,u.Material,u.MaterialFactoryNew());L.delete_all_material_expressions(m)
for file,name,prop in [('difusse.jpg','color',u.MaterialProperty.MP_BASE_COLOR),('normal.JPG','normal',u.MaterialProperty.MP_NORMAL)]:
    tex=imp(SRC/file,'T_MenuZombie_'+name)
    if name=='normal':tex.set_editor_property('srgb',False);tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);save(tex)
    n=L.create_material_expression(m,u.MaterialExpressionTextureSample);n.set_editor_property('texture',tex)
    if name=='normal':n.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    L.connect_material_property(n,'RGB',prop)
for prop,v in [(u.MaterialProperty.MP_ROUGHNESS,.78),(u.MaterialProperty.MP_SPECULAR,.18)]:
    n=L.create_material_expression(m,u.MaterialExpressionConstant);n.set_editor_property('r',v);L.connect_material_property(n,'',prop)
L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_SKELETAL_MESH,True);L.recompile_material(m);save(m)
slots=list(mesh.get_editor_property('materials'))
for slot in slots:slot.set_editor_property('material_interface',m)
mesh.set_editor_property('materials',slots);save(mesh)
# Menu-only original hospital tiles: world-space joints and irregular dampness.
# The campaign's shared floor material is not changed.
floor=u.load_asset(BASE+'/M_ArchiveFloor') or T.create_asset('M_ArchiveFloor',BASE,u.Material,u.MaterialFactoryNew());L.delete_all_material_expressions(floor)
pos=L.create_material_expression(floor,u.MaterialExpressionWorldPosition)
def custom(code,prop,vector=False):
    n=L.create_material_expression(floor,u.MaterialExpressionCustom);n.set_editor_property('code',code);n.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3 if vector else u.CustomMaterialOutputType.CMOT_FLOAT1)
    p=u.CustomInput();p.set_editor_property('input_name','P');n.set_editor_property('inputs',[p]);L.connect_material_expressions(pos,'',n,'P');L.connect_material_property(n,'',prop)
noise='''struct N {float h(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}float n(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y);}};N n;float2 q=P.xy/60,cell=floor(q),uv=frac(q);float edge=min(min(uv.x,1-uv.x),min(uv.y,1-uv.y));float dirt=n.n(q*2.3)*.65+n.n(q*17)*.25+n.n(q*113)*.1;float joint=1-smoothstep(.008,.025+n.n(q*41)*.012,edge);float wet=smoothstep(.30,.72,n.n(q*.65+31));'''
custom(noise+'''float tile=n.h(cell);float3 c=lerp(float3(.105,.145,.143),float3(.19,.23,.215),tile)*(.72+dirt*.4);c*=1-smoothstep(.06,.0,edge)*.23;c=lerp(c,float3(.022,.028,.025),joint);return c*(1-wet*.18);''',u.MaterialProperty.MP_BASE_COLOR,True)
custom(noise+'return lerp(lerp(.60,.20,wet)+dirt*.07,.92,joint);',u.MaterialProperty.MP_ROUGHNESS)
n=L.create_material_expression(floor,u.MaterialExpressionConstant);n.set_editor_property('r',.38);L.connect_material_property(n,'',u.MaterialProperty.MP_SPECULAR);L.recompile_material(floor);save(floor)
b=mesh.get_bounds()
r={'success':True,'source':'https://opengameart.org/content/zombie','author':'Pixelhouse','license':'CC BY 3.0','clip_seconds':clip.get_play_length(),'bounds_origin':[b.origin.x,b.origin.y,b.origin.z],'bounds_extent':[b.box_extent.x,b.box_extent.y,b.box_extent.z]}
(ROOT/'Saved/Prototype/game_menu_import.json').write_text(json.dumps(r,indent=2));u.log('GAME_MENU_IMPORT '+json.dumps(r))
