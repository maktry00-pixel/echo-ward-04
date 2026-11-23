"""Reproducible surface/light pass. Uses existing licensed PBR maps at source resolution."""
import unreal as u
from pathlib import Path
import json

ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
DATA=json.loads((ROOT/'RawAssets/Campaign/campaign.json').read_text(encoding='utf-8'))
BASE='/Game/Campaign/Materials/'
LIB=u.MaterialEditingLibrary
TOOLS=u.AssetToolsHelpers.get_asset_tools()

def node(mat,cls,**props):
    n=LIB.create_material_expression(mat,cls)
    for k,v in props.items():n.set_editor_property(k,v)
    return n

def save(asset):
    if not u.EditorAssetLibrary.save_loaded_asset(asset):raise RuntimeError('Cannot save '+asset.get_path_name())

def material(name):
    m=u.load_asset(BASE+name) or TOOLS.create_asset(name,BASE.rstrip('/'),u.Material,u.MaterialFactoryNew())
    LIB.delete_all_material_expressions(m)
    return m

def constant(mat,prop,value):
    n=node(mat,u.MaterialExpressionConstant,r=value);LIB.connect_material_property(n,'',prop)

def surface(name,world):
    m=material(name);m.set_editor_property('tangent_space_normal',not world)
    n={
        'P':(node(m,u.MaterialExpressionWorldPosition),''),
        'UV':(node(m,u.MaterialExpressionTextureCoordinate),''),
        'N':(node(m,u.MaterialExpressionVertexInterpolator),''),
        'Tint':(node(m,u.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=u.LinearColor(1,1,1,1)),'RGB'),
        'Tiling':(node(m,u.MaterialExpressionVectorParameter,parameter_name='Tiling',default_value=u.LinearColor(1,1,1,1)),'RGB'),
        'Channel':(node(m,u.MaterialExpressionVectorParameter,parameter_name='RoughnessChannel',default_value=u.LinearColor(1,0,0,0)),'RGB'),
    }
    normal=node(m,u.MaterialExpressionVertexNormalWS);LIB.connect_material_expressions(normal,'',n['N'][0],'')
    for param,default in [('Roughness',.8),('Strength',.5),('Wetness',0),('UsePackedAO',0)]:
        n[param]=(node(m,u.MaterialExpressionScalarParameter,parameter_name='NormalStrength' if param=='Strength' else param,default_value=default),'')
    for param,tex,sampler in [('Albedo','T_White',u.MaterialSamplerType.SAMPLERTYPE_COLOR),('Normal','T_FlatNormal',u.MaterialSamplerType.SAMPLERTYPE_NORMAL),('RoughnessMap','T_WhiteMasks',u.MaterialSamplerType.SAMPLERTYPE_MASKS)]:
        n[param]=(node(m,u.MaterialExpressionTextureObjectParameter,parameter_name=param,texture=u.load_asset('/Game/Campaign/Textures/'+tex),sampler_type=sampler),'')
    common='float3 w=pow(abs(N),4);w/=max(.001,w.x+w.y+w.z);float3 p=P*.01*Tiling.r;float2 uv=UV*Tiling.rg;\n'
    def sample(tex):
        return f'(Texture2DSample({tex},{tex}Sampler,p.yz)*w.x+Texture2DSample({tex},{tex}Sampler,p.xz)*w.y+Texture2DSample({tex},{tex}Sampler,p.xy)*w.z)' if world else f'Texture2DSample({tex},{tex}Sampler,uv)'
    macro='float macro=sin(P.x*.004+sin(P.y*.002))*sin(P.y*.003+P.z*.001);float wet=smoothstep(.57,.86,macro*.5+.5)*Wetness*step(.8,N.z);\n'
    color=common+macro+f'float3 c={sample("Albedo")}.rgb*Tint*lerp(.94,1.04,macro*.5+.5);float rough=dot({sample("RoughnessMap")}.rgb,Channel)*Roughness;return float4(lerp(c,c*.64,wet),lerp(clamp(rough,.28,.98),.18,wet));'
    if world:
        normal_code=common+macro+'''
float2 a=Texture2DSample(Normal,NormalSampler,p.yz).rg*2-1;
float2 b=Texture2DSample(Normal,NormalSampler,p.xz).rg*2-1;
float2 c=Texture2DSample(Normal,NormalSampler,p.xy).rg*2-1;
float3 nx=float3(sqrt(saturate(1-dot(a,a)))*sign(N.x),a.x,a.y);
float3 ny=float3(b.x,sqrt(saturate(1-dot(b,b)))*sign(N.y),b.y);
float3 nz=float3(c.x,c.y,sqrt(saturate(1-dot(c,c)))*sign(N.z));
return normalize(lerp(normalize(N),normalize(nx*w.x+ny*w.y+nz*w.z),Strength*(1-wet*.7)));
'''
    else:
        normal_code=common+'float2 xy=Texture2DSample(Normal,NormalSampler,uv).rg*2-1;return normalize(float3(xy*Strength,sqrt(saturate(1-dot(xy,xy)))));'
    ao=common+f'return lerp(1,clamp({sample("RoughnessMap")}.r,.25,1),UsePackedAO);'
    def custom(code,kind):
        c=node(m,u.MaterialExpressionCustom,code=code,output_type=kind);pins=[]
        for key in n:
            pin=u.CustomInput();pin.set_editor_property('input_name',key);pins.append(pin)
        c.set_editor_property('inputs',pins)
        for key,(value,out) in n.items():
            if not LIB.connect_material_expressions(value,out,c,key):raise RuntimeError('Input '+key)
        return c
    cr=custom(color,u.CustomMaterialOutputType.CMOT_FLOAT4)
    for prop,channels in [(u.MaterialProperty.MP_BASE_COLOR,dict(r=True,g=True,b=True,a=False)),(u.MaterialProperty.MP_ROUGHNESS,dict(r=False,g=False,b=False,a=True))]:
        mask=node(m,u.MaterialExpressionComponentMask,**channels);LIB.connect_material_expressions(cr,'',mask,'');LIB.connect_material_property(mask,'',prop)
    LIB.connect_material_property(custom(normal_code,u.CustomMaterialOutputType.CMOT_FLOAT3),'',u.MaterialProperty.MP_NORMAL)
    LIB.connect_material_property(custom(ao,u.CustomMaterialOutputType.CMOT_FLOAT1),'',u.MaterialProperty.MP_AMBIENT_OCCLUSION)
    metal=node(m,u.MaterialExpressionScalarParameter,parameter_name='Metallic',default_value=0.)
    emission=node(m,u.MaterialExpressionVectorParameter,parameter_name='Emission',default_value=u.LinearColor(0,0,0,0))
    LIB.connect_material_property(metal,'',u.MaterialProperty.MP_METALLIC);LIB.connect_material_property(emission,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
    constant(m,u.MaterialProperty.MP_SPECULAR,.28)
    LIB.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);LIB.recompile_material(m);save(m)

def effects():
    for name,color,rough,metal in [('M_FXBrass',(.48,.27,.065),.28,.9),('M_FXChip',(.12,.105,.085),.88,0),('M_FXBottle',(.018,.055,.025),.19,0)]:
        m=material(name);rgb=node(m,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(*color,1));LIB.connect_material_property(rgb,'',u.MaterialProperty.MP_BASE_COLOR)
        constant(m,u.MaterialProperty.MP_ROUGHNESS,rough);constant(m,u.MaterialProperty.MP_METALLIC,metal);LIB.recompile_material(m);save(m)
    m=material('M_BulletMark');m.set_editor_property('material_domain',u.MaterialDomain.MD_DEFERRED_DECAL);m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
    rgb=node(m,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(.012,.009,.006,1));LIB.connect_material_property(rgb,'',u.MaterialProperty.MP_BASE_COLOR)
    uv=node(m,u.MaterialExpressionTextureCoordinate)
    mask=node(m,u.MaterialExpressionCustom,code='float2 p=UV-.5;float a=atan2(p.y,p.x);return (1-smoothstep(.16+.025*sin(a*11),.46,length(p)))*.85;',output_type=u.CustomMaterialOutputType.CMOT_FLOAT1)
    pin=u.CustomInput();pin.set_editor_property('input_name','UV');mask.set_editor_property('inputs',[pin]);LIB.connect_material_expressions(uv,'',mask,'UV');LIB.connect_material_property(mask,'',u.MaterialProperty.MP_OPACITY);constant(m,u.MaterialProperty.MP_ROUGHNESS,.95);LIB.recompile_material(m);save(m)

def main():
    surface('M_CampaignPBR',False);surface('M_CampaignWorldPBR',True);effects()
    cube=u.load_asset('/Game/Campaign/Meshes/SM_d1884d819fca686b')
    mesh_tools=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
    build=mesh_tools.get_lod_build_settings(cube,0)
    for key,value in dict(recompute_normals=True,recompute_tangents=True,use_mikk_t_space=True,use_high_precision_tangent_basis=True,use_full_precision_u_vs=True).items():build.set_editor_property(key,value)
    mesh_tools.set_lod_build_settings(cube,0,build);save(cube)
    # The repeated room shell uses HISM. Nanite enables its Lumen surface cache.
    nanite=mesh_tools.get_nanite_settings(cube);nanite.set_editor_property('enabled',True);nanite.set_editor_property('keep_percent_triangles',1.);nanite.set_editor_property('trim_relative_error',0.)
    mesh_tools.set_nanite_settings(cube,nanite,True);save(cube)
    for name in ['M_CampaignPBR','M_CampaignWorldPBR','M_CampaignOrganic']:
        parent=u.load_asset(BASE+name);LIB.set_base_material_usage(parent,u.MaterialUsage.MATUSAGE_NANITE,True);LIB.recompile_material(parent);save(parent)
    sources={t['id']:t['source'] for t in DATA['textures']}
    changed=0
    for row in DATA['materials']:
        if row.get('sign') or row.get('wood') or row.get('organic') or row['masked']:continue
        mi=u.load_asset(BASE+'MI_'+row['id'])
        src=sources.get(row.get('albedo'),'')
        packed='_arm_' in sources.get(row.get('roughness_texture'),'')
        LIB.set_material_instance_scalar_parameter_value(mi,'UsePackedAO',1 if packed else 0)
        wet=.7 if 'terrazzo_tiles' in src else .3 if '/concrete/' in src else 0
        LIB.set_material_instance_scalar_parameter_value(mi,'Wetness',wet)
        LIB.set_material_instance_scalar_parameter_value(mi,'NormalStrength',max(.42,min(.85,row['normal_scale'])) if row.get('normal') else 0)
        if 'terrazzo_tiles' in src:LIB.set_material_instance_scalar_parameter_value(mi,'Roughness',.76)
        if 'terrazzo_tiles' in src:
            # The exported four-metre floor cubes have a 1/3 x 1/2 top UV island.
            # Explicit UV mapping avoids the legacy instance vertex interpolator.
            LIB.set_material_instance_parent(mi,u.load_asset(BASE+'M_CampaignPBR'))
            LIB.set_material_instance_vector_parameter_value(mi,'Tiling',u.LinearColor(6,4,0,0))
        LIB.update_material_instance(mi);save(mi);changed+=1
    levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
    if not globals().get('UPDATE_CURRENT_LEVEL',False):levels.load_level('/Game/Campaign/Maps/Campaign')
    actors=u.get_editor_subsystem(u.EditorActorSubsystem)
    for a in actors.get_all_level_actors():
        for c in a.get_components_by_class(u.StaticMeshComponent):
            for mat in c.get_materials():
                if not mat:continue
                while isinstance(mat,u.MaterialInstance):mat=mat.get_editor_property('parent')
                if mat.get_editor_property('blend_mode') not in [u.BlendMode.BLEND_OPAQUE,u.BlendMode.BLEND_MASKED]:c.set_editor_property('disallow_nanite',True)
        if isinstance(a,u.RectLight):
            light=a.get_component_by_class(u.RectLightComponent)
            # Derive from exported source rather than multiply, so reruns are stable.
            source=next((r for r in DATA['lights'] if r['path']==a.get_actor_label()),None)
            if source:light.set_intensity(min(3900.,source['energy']*1800.))
            light.set_editor_property('source_width',100.);light.set_editor_property('source_height',35.)
            light.set_editor_property('use_temperature',True);light.set_temperature(4100.)
        elif isinstance(a,u.PostProcessVolume):
            s=a.get_editor_property('settings')
            for k,v in dict(override_auto_exposure_min_brightness=True,override_auto_exposure_max_brightness=True,auto_exposure_min_brightness=4.4,auto_exposure_max_brightness=4.4,override_ambient_cubemap_intensity=True,ambient_cubemap_intensity=.36,override_bloom_intensity=True,bloom_intensity=.13,override_vignette_intensity=True,vignette_intensity=.25,override_film_grain_intensity=True,film_grain_intensity=.035,override_color_saturation=True,color_saturation=u.Vector4(.91,.93,.94,1),override_color_contrast=True,color_contrast=u.Vector4(1.04,1.04,1.04,1)).items():s.set_editor_property(k,v)
            a.set_editor_property('settings',s)
        elif isinstance(a,u.ExponentialHeightFog):
            fog=a.get_component_by_class(u.ExponentialHeightFogComponent);fog.set_editor_property('fog_density',.006);fog.set_editor_property('volumetric_fog_scattering_distribution',.35)
    if not levels.save_current_level():raise RuntimeError('Cannot save upgraded map')
    return {'success':True,'surface_instances':changed,'source_textures_preserved':True,'packed_occlusion':True,'physical_effect_materials':4,'primitive_normals_rebuilt':True,'room_shell_nanite':True}

try:
    report=main()
except Exception:
    import traceback
    report={'success':False,'error':traceback.format_exc()}
    (ROOT/'Saved/Prototype/presentation_upgrade.json').write_text(json.dumps(report,indent=2))
    raise
(ROOT/'Saved/Prototype/presentation_upgrade.json').write_text(json.dumps(report,indent=2))
u.log('PRESENTATION_UPGRADE '+str(report))
