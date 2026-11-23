"""Use packed metallic correctly; glass remains transparent in native assets."""
import json,unreal as u
from pathlib import Path
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));D=json.loads((P/'RawAssets/EchoCustom/manifest.json').read_text());E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;BASE='/Game/Campaign/EchoCustom/Materials'
def node(m,c,**props):
    n=L.create_material_expression(m,c)
    for k,v in props.items():n.set_editor_property(k,v)
    return n
def parent(masked=False,glass=False):
    name='M_EchoGlassV2' if glass else 'M_EchoMaskedV2' if masked else 'M_EchoPBRV2';path=BASE+'/'+name;m=u.load_asset(path)
    if m:return m
    m=E.duplicate_asset('/Game/Campaign/Materials/'+('M_CampaignGlass' if glass else 'M_CampaignPBR'),path)
    if not glass:
        uv=node(m,u.MaterialExpressionTextureCoordinate);tex=node(m,u.MaterialExpressionTextureSampleParameter2D,parameter_name='ORMMetal',texture=u.load_asset('/Game/Campaign/Textures/T_WhiteMasks'),sampler_type=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
        amount=node(m,u.MaterialExpressionScalarParameter,parameter_name='Metallic',default_value=0.);mul=node(m,u.MaterialExpressionMultiply)
        L.connect_material_expressions(uv,'',tex,'UVs');L.connect_material_expressions(tex,'B',mul,'A');L.connect_material_expressions(amount,'',mul,'B');L.connect_material_property(mul,'',u.MaterialProperty.MP_METALLIC)
        if masked:
            m.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True)
            a=node(m,u.MaterialExpressionTextureSampleParameter2D,parameter_name='AlphaColor',texture=u.load_asset('/Game/Campaign/Textures/T_White'))
            L.connect_material_expressions(uv,'',a,'UVs');L.connect_material_property(a,'A',u.MaterialProperty.MP_OPACITY_MASK)
    L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);L.recompile_material(m);E.save_loaded_asset(m);return m
for key,spec in D['materials'].items():
    m=u.load_asset(BASE+'/MI_'+key);assert m,key
    glass=spec['name'] in ('glass','water');L.set_material_instance_parent(m,parent(spec['alpha']!='OPAQUE',glass))
    L.set_material_instance_scalar_parameter_value(m,'Metallic',spec['metallic'])
    if glass:
        c=spec['color'][:3];L.set_material_instance_vector_parameter_value(m,'Tint',u.LinearColor(*c,.18 if spec['name']=='glass' else .4))
        L.set_material_instance_scalar_parameter_value(m,'Roughness',.2)
    assert E.save_loaded_asset(m)
(P/'Saved/Prototype/echo_material_finish.json').write_text(json.dumps(dict(success=True,materials=len(D['materials']))))
u.log('ECHO_MATERIAL_FINISH');u.SystemLibrary.quit_editor()
