"""Repair face/hair slot assignment and use the authored alpha channel."""
import unreal as u
from pathlib import Path
import json
for part in ['body','head']:
    surface=u.load_asset('/Game/Prototype/Materials/M_Character_'+part)
    lib=u.MaterialEditingLibrary
    lib.delete_all_material_expressions(surface)
    color=lib.create_material_expression(surface,u.MaterialExpressionTextureSample)
    color.set_editor_property('texture',u.load_asset('/Game/Prototype/Textures/T_Character_'+part+'_color'))
    lib.connect_material_property(color,'RGB',u.MaterialProperty.MP_BASE_COLOR)
    normal=lib.create_material_expression(surface,u.MaterialExpressionTextureSample)
    normal.set_editor_property('texture',u.load_asset('/Game/Prototype/Textures/T_Character_'+part+'_normal'))
    normal.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    flat=lib.create_material_expression(surface,u.MaterialExpressionConstant3Vector)
    flat.set_editor_property('constant',u.LinearColor(0,0,1,1))
    mix=lib.create_material_expression(surface,u.MaterialExpressionLinearInterpolate)
    mix.set_editor_property('const_alpha',.25)
    lib.connect_material_expressions(flat,'',mix,'A');lib.connect_material_expressions(normal,'RGB',mix,'B')
    lib.connect_material_property(mix,'',u.MaterialProperty.MP_NORMAL)
    for prop,value in [(u.MaterialProperty.MP_ROUGHNESS,.95 if part=='head' else .86),(u.MaterialProperty.MP_SPECULAR,0 if part=='head' else .15)]:
        scalar=lib.create_material_expression(surface,u.MaterialExpressionConstant);scalar.set_editor_property('r',value);lib.connect_material_property(scalar,'',prop)
    lib.set_base_material_usage(surface,u.MaterialUsage.MATUSAGE_SKELETAL_MESH,True)
    lib.recompile_material(surface)
    if not u.EditorAssetLibrary.save_loaded_asset(surface):raise RuntimeError('Could not save '+part)
mesh=u.load_asset('/Game/Prototype/Character/SK_Survivor')
mat=u.load_asset('/Game/Prototype/Materials/M_Character_opacity')
tex=u.load_asset('/Game/Prototype/Textures/T_Character_opacity')
u.MaterialEditingLibrary.delete_all_material_expressions(mat)
mat.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED)
mat.set_editor_property('two_sided',True)
mat.set_editor_property('opacity_mask_clip_value',.4)
sample=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionTextureSample)
sample.set_editor_property('texture',tex)
u.MaterialEditingLibrary.connect_material_property(sample,'RGB',u.MaterialProperty.MP_BASE_COLOR)
u.MaterialEditingLibrary.connect_material_property(sample,'A',u.MaterialProperty.MP_OPACITY_MASK)
rough=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant)
rough.set_editor_property('r',.9)
u.MaterialEditingLibrary.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
spec=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant)
spec.set_editor_property('r',0)
u.MaterialEditingLibrary.connect_material_property(spec,'',u.MaterialProperty.MP_SPECULAR)
u.MaterialEditingLibrary.set_base_material_usage(mat,u.MaterialUsage.MATUSAGE_SKELETAL_MESH,True)
u.MaterialEditingLibrary.recompile_material(mat)
if not u.EditorAssetLibrary.save_loaded_asset(mat):raise RuntimeError('Could not save hair material')
rows=[]
slots=list(mesh.get_editor_property('materials'))
for slot in slots:
    name=str(slot.get_editor_property('material_slot_name')).lower()
    part='opacity' if any(s in name for s in ['opacity','hair','lash']) else 'head' if 'head' in name else 'body'
    slot.set_editor_property('material_interface',u.load_asset('/Game/Prototype/Materials/M_Character_'+part))
    rows.append({'slot':name,'assigned':part})
mesh.set_editor_property('materials',slots)
# The legacy low-poly head and overlapping alpha cards create self-shadow
# artifacts under close ceiling sources. Retain the body's cast shadow;
# head surfaces still receive shadows from the room and other actors.
skeletal=u.get_editor_subsystem(u.SkeletalMeshEditorSubsystem)
sections=[]
for section in range(skeletal.get_num_sections(mesh,0)):
    slot_index=skeletal.get_lod_material_slot(mesh,0,section)
    cast_shadow=slot_index==0
    if not skeletal.set_section_cast_shadow(mesh,0,section,cast_shadow):
        raise RuntimeError('Could not set section shadows')
    sections.append({'section':section,'material_slot':slot_index,'cast_shadow':cast_shadow})
if not u.EditorAssetLibrary.save_loaded_asset(mesh):raise RuntimeError('Could not save character slots')
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
(root/'Saved/Prototype/character_material_fix.json').write_text(json.dumps({'success':True,'slots':rows,'sections':sections},indent=2))
u.log('CHARACTER_MATERIALS_FIXED '+str(rows))
