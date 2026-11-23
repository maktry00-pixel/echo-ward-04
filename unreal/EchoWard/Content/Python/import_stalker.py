"""Import the CC0 HorrorGameMaker humanoid and its actual skeletal motion."""
from pathlib import Path
import json
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
SOURCE=ROOT.parents[1]/'assets/models/horror_game_maker/Poses'
BASE='/Game/Campaign/Creatures'
LIB=u.EditorAssetLibrary
def save(a):
    if not LIB.save_loaded_asset(a):raise RuntimeError('Cannot save '+a.get_path_name())
def imported(source,name,opts=None,force=False):
    path=BASE+'/'+name
    if LIB.does_asset_exist(path) and not force:return u.load_asset(path)
    task=u.AssetImportTask()
    for k,v in dict(filename=str(source),destination_path=BASE,destination_name=name,automated=True,save=True,replace_existing=True).items():task.set_editor_property(k,v)
    if opts:
        task.set_editor_property('options',opts);task.set_editor_property('factory',u.FbxFactory())
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    a=u.load_asset(path)
    if not a:raise RuntimeError('Import failed '+str(source)+' '+str(task.get_editor_property('imported_object_paths')))
    return a
def options(mesh,skeleton=None):
    o=u.FbxImportUI()
    for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH if mesh else u.FBXImportType.FBXIT_ANIMATION,import_mesh=mesh,import_as_skeletal=True,import_animations=True,import_materials=False,import_textures=False,override_full_name=True,create_physics_asset=mesh).items():o.set_editor_property(k,v)
    o.set_editor_property('original_import_type',u.FBXImportType.FBXIT_SKELETAL_MESH if mesh else u.FBXImportType.FBXIT_ANIMATION)
    anim=o.get_editor_property('anim_sequence_import_data');anim.set_editor_property('use_default_sample_rate',False);anim.set_editor_property('custom_sample_rate',60);anim.set_editor_property('snap_to_closest_frame_boundary',True)
    if skeleton:o.set_editor_property('skeleton',skeleton)
    return o
o=options(True);o.set_editor_property('override_animation_name','A_StalkerIdle')
existing=u.load_asset(BASE+'/SK_Stalker')
mesh=imported(SOURCE/'Idle.fbx','SK_Stalker',o,force=not existing or not existing.get_editor_property('skeleton'))
if not isinstance(mesh,u.SkeletalMesh):raise RuntimeError('No skeletal monster')
skeleton=mesh.get_editor_property('skeleton')
if not skeleton:raise RuntimeError('Imported stalker has no skeleton')
save(skeleton)
if not mesh.get_editor_property('physics_asset'):
    physics=u.SkeletalMeshEditorSubsystem.create_physics_asset(mesh,True,0)
    if not physics:raise RuntimeError('Could not generate monster physics bodies')
    save(physics);save(mesh)
if mesh.get_editor_property('physics_asset'):save(mesh.get_editor_property('physics_asset'))
for source,name in [('Idle','A_StalkerIdle'),('Walk','A_StalkerWalk'),('Run','A_StalkerRun'),('Jump','A_StalkerAttack')]:
    o=options(False,skeleton);o.set_editor_property('override_animation_name',name)
    clip=imported(SOURCE/(source+'.fbx'),name,o)
    clip.set_editor_property('enable_root_motion',True);clip.set_editor_property('force_root_lock',True);save(clip)
textures={}
for suffix,kind in [('AlbedoTransparency','color'),('Normal','normal'),('MetallicSmoothness','packed')]:
    t=imported(SOURCE/'UnityTexture'/('test_StingrayPBS1SG_'+suffix+'.png'),'T_Stalker_'+kind)
    t.set_editor_property('srgb',kind=='color')
    if kind=='normal':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
    elif kind=='packed':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
    textures[kind]=t;save(t)
mat_path=BASE+'/M_Stalker'
mat=u.load_asset(mat_path) if LIB.does_asset_exist(mat_path) else u.AssetToolsHelpers.get_asset_tools().create_asset('M_Stalker',BASE,u.Material,u.MaterialFactoryNew())
M=u.MaterialEditingLibrary;M.delete_all_material_expressions(mat)
for kind,prop in [('color',u.MaterialProperty.MP_BASE_COLOR),('normal',u.MaterialProperty.MP_NORMAL)]:
    n=M.create_material_expression(mat,u.MaterialExpressionTextureSample);n.set_editor_property('texture',textures[kind]);n.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='normal' else u.MaterialSamplerType.SAMPLERTYPE_COLOR);M.connect_material_property(n,'RGB',prop)
r=M.create_material_expression(mat,u.MaterialExpressionConstant);r.set_editor_property('r',.78);M.connect_material_property(r,'',u.MaterialProperty.MP_ROUGHNESS)
s=M.create_material_expression(mat,u.MaterialExpressionConstant);s.set_editor_property('r',.18);M.connect_material_property(s,'',u.MaterialProperty.MP_SPECULAR)
M.set_base_material_usage(mat,u.MaterialUsage.MATUSAGE_SKELETAL_MESH,True);M.recompile_material(mat);save(mat)
slots=list(mesh.get_editor_property('materials'))
for slot in slots:slot.set_editor_property('material_interface',mat)
mesh.set_editor_property('materials',slots);save(mesh)
b=mesh.get_bounds()
report={'success':True,'mesh':mesh.get_path_name(),'bounds_origin':[b.origin.x,b.origin.y,b.origin.z],'bounds_extent':[b.box_extent.x,b.box_extent.y,b.box_extent.z],'physics_asset':str(mesh.get_editor_property('physics_asset')),'animations':{n:u.load_asset(BASE+'/'+n).get_play_length() for n in ['A_StalkerIdle','A_StalkerWalk','A_StalkerRun','A_StalkerAttack']},'license':'CC0','source':'https://opengameart.org/content/3d-horror-game-monster','attack_source':'Jump.fbx used for a lunging attack, not a bespoke melee animation'}
(ROOT/'Saved/Prototype/stalker_import.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('STALKER_IMPORT '+json.dumps(report))
