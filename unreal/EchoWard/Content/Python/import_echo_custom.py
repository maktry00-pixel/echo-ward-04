"""Import original Echo static art with resolved PBR slots and metric bounds."""
import json,traceback,os
from pathlib import Path
import unreal as u
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));RAW=P/'RawAssets/EchoCustom';D=json.loads((RAW/'manifest.json').read_text(encoding='utf8'))
BASE='/Game/Campaign/EchoCustom';E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools();rows=[];texture_cache={}
def save(o):
    if not E.save_loaded_asset(o):raise RuntimeError('Save failed '+o.get_path_name())
def node(m,c,**props):
    n=L.create_material_expression(m,c)
    for k,v in props.items():n.set_editor_property(k,v)
    return n
def texture(path,role):
    name='T_'+Path(path).stem[:32]+'_'+role;dest=BASE+'/Textures/'+name
    if dest in texture_cache:return texture_cache[dest]
    t=u.load_asset(dest)
    if not t:
        task=u.AssetImportTask();task.filename=path;task.destination_path=BASE+'/Textures';task.destination_name=name;task.automated=True;task.save=True;A.import_asset_tasks([task]);t=u.load_asset(dest)
        if not t:raise RuntimeError('Texture import '+path)
        t.set_editor_property('srgb',role=='color');t.set_editor_property('max_texture_size',2048)
        if role=='normal':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
        if role=='orm':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
        save(t)
    texture_cache[dest]=t;return t
def master(world=False,masked=False):
    name='M_Echo'+('WorldV20' if world else 'Masked' if masked else 'PBR');path=BASE+'/Materials/'+name;m=u.load_asset(path)
    if m:return m
    source='/Game/Campaign/Materials/'+('M_CampaignWorldPBR' if world else 'M_CampaignPBR');m=E.duplicate_asset(source,path)
    if world:
        L.delete_all_material_expressions(m)
        m.set_editor_property('tangent_space_normal',False)
        pins={}
        for key,cls,props in [('P',u.MaterialExpressionWorldPosition,{}),('N',u.MaterialExpressionVertexInterpolator,{}),('Tint',u.MaterialExpressionVectorParameter,dict(parameter_name='Tint',default_value=u.LinearColor(1,1,1,1))),('Roughness',u.MaterialExpressionScalarParameter,dict(parameter_name='Roughness',default_value=.9)),('Strength',u.MaterialExpressionScalarParameter,dict(parameter_name='NormalStrength',default_value=.85))]:pins[key]=node(m,cls,**props)
        vn=node(m,u.MaterialExpressionVertexNormalWS);L.connect_material_expressions(vn,'',pins['N'],'')
        for key,default,sampler in [('Albedo','T_White',u.MaterialSamplerType.SAMPLERTYPE_COLOR),('Normal','T_FlatNormal',u.MaterialSamplerType.SAMPLERTYPE_NORMAL),('ORM','T_WhiteMasks',u.MaterialSamplerType.SAMPLERTYPE_MASKS)]:pins[key]=node(m,u.MaterialExpressionTextureObjectParameter,parameter_name=key,texture=u.load_asset('/Game/Campaign/Textures/'+default),sampler_type=sampler)
        common='float3 w=pow(abs(N),4);w/=max(.001,w.x+w.y+w.z);float3 p=P*.0033333;\n'
        code=common+"""
float3 c=Texture2DSample(Albedo,AlbedoSampler,p.yz).rgb*w.x+Texture2DSample(Albedo,AlbedoSampler,p.xz).rgb*w.y+Texture2DSample(Albedo,AlbedoSampler,p.xy).rgb*w.z;
float3 r=Texture2DSample(ORM,ORMSampler,p.yz).rgb*w.x+Texture2DSample(ORM,ORMSampler,p.xz).rgb*w.y+Texture2DSample(ORM,ORMSampler,p.xy).rgb*w.z;
float grain=sin(P.x*.006+sin(P.y*.009))*sin(P.y*.007+P.z*.005);
float wet=smoothstep(.15,.72,grain)*saturate(N.z);
float dirt=.88+.12*sin(P.x*.003+P.y*.004+sin(P.z*.018));
return float4(c*Tint*dirt,clamp(r.g*Roughness-wet*.36,.12,.97));
"""
        normalcode=common+"""
float2 a=Texture2DSample(Normal,NormalSampler,p.yz).rg*2-1;float2 b=Texture2DSample(Normal,NormalSampler,p.xz).rg*2-1;float2 c=Texture2DSample(Normal,NormalSampler,p.xy).rg*2-1;
float3 nx=float3(sqrt(saturate(1-dot(a,a)))*sign(N.x),a.x,a.y);float3 ny=float3(b.x,sqrt(saturate(1-dot(b,b)))*sign(N.y),b.y);float3 nz=float3(c.x,c.y,sqrt(saturate(1-dot(c,c)))*sign(N.z));
return normalize(lerp(normalize(N),normalize(nx*w.x+ny*w.y+nz*w.z),Strength));
"""
        for code,kind in [(code,u.CustomMaterialOutputType.CMOT_FLOAT4),(normalcode,u.CustomMaterialOutputType.CMOT_FLOAT3)]:
            custom=node(m,u.MaterialExpressionCustom,code=code,output_type=kind);inputs=[]
            for key in pins:
                pin=u.CustomInput();pin.set_editor_property('input_name',key);inputs.append(pin)
            custom.set_editor_property('inputs',inputs)
            for key,n in pins.items():L.connect_material_expressions(n,'RGB' if key=='Tint' else '',custom,key)
            if kind==u.CustomMaterialOutputType.CMOT_FLOAT4:
                color=node(m,u.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False);rough=node(m,u.MaterialExpressionComponentMask,r=False,g=False,b=False,a=True)
                L.connect_material_expressions(custom,'',color,'');L.connect_material_expressions(custom,'',rough,'');L.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR);L.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
            else:L.connect_material_property(custom,'',u.MaterialProperty.MP_NORMAL)
        em=node(m,u.MaterialExpressionVectorParameter,parameter_name='Emission',default_value=u.LinearColor(0,0,0,1));L.connect_material_property(em,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        # Add packed metallic channel, absent from the legacy master.
        tex=node(m,u.MaterialExpressionTextureSampleParameter2D,parameter_name='ORMMetal',texture=u.load_asset('/Game/Campaign/Textures/T_WhiteMasks'),sampler_type=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
        coord=node(m,u.MaterialExpressionTextureCoordinate);L.connect_material_expressions(coord,'',tex,'UVs')
        L.connect_material_property(tex,'B',u.MaterialProperty.MP_METALLIC)
        if masked:
            m.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True)
            alpha=node(m,u.MaterialExpressionTextureSampleParameter2D,parameter_name='AlphaColor',texture=u.load_asset('/Game/Campaign/Textures/T_White'))
            L.connect_material_expressions(coord,'',alpha,'UVs');L.connect_material_property(alpha,'A',u.MaterialProperty.MP_OPACITY_MASK)
    L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True);L.recompile_material(m);save(m);return m
def material(key,spec,world=False):
    name='MI_'+key+('_World' if world else '');path=BASE+'/Materials/'+name;m=u.load_asset(path)
    if m and not world:return m
    m=m or A.create_asset(name,BASE+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew());L.set_material_instance_parent(m,master(world,spec['alpha']!='OPAQUE'))
    L.set_material_instance_vector_parameter_value(m,'Tint',u.LinearColor(*spec['color']))
    L.set_material_instance_scalar_parameter_value(m,'Roughness',max(.35,spec['roughness']))
    L.set_material_instance_vector_parameter_value(m,'RoughnessChannel',u.LinearColor(0,1,0,0));L.set_material_instance_scalar_parameter_value(m,'NormalStrength',.85)
    if world:L.set_material_instance_vector_parameter_value(m,'Tiling',u.LinearColor(1/3,1/3,1,0))
    for k,param,role in [('albedo','Albedo','color'),('normal','Normal','normal'),('orm','ORM' if world else 'RoughnessMap','orm')]:
        if k in spec:
            t=texture(spec[k],role);L.set_material_instance_texture_parameter_value(m,param,t)
            if k=='orm' and not world:L.set_material_instance_texture_parameter_value(m,'ORMMetal',t)
            if k=='albedo' and spec['alpha']!='OPAQUE':L.set_material_instance_texture_parameter_value(m,'AlphaColor',t)
    if 'orm' not in spec and not world:
        L.set_material_instance_scalar_parameter_value(m,'Metallic',spec['metallic'])
    L.set_material_instance_vector_parameter_value(m,'Emission',u.LinearColor(*spec['emission'],1));save(m);return m
try:
    test=os.environ.get('ECHO_IMPORT_PROBE')=='1'
    selected=[a for a in D['assets'] if not test or a['id'] in (1,25,26,34,63,175)]
    keys={k for a in selected for k in a['materials']};mats={k:material(k,D['materials'][k]) for k in sorted(keys)}
    surfaces={}
    for ident in (25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40):
        a=next(a for a in D['assets'] if a['id']==ident);key=a['materials'][0];surfaces[str(ident)]=material(key,D['materials'][key],True).get_path_name()
    for index,row in enumerate(selected):
        name='SM_'+row['name'];path=BASE+'/Meshes/'+name;mesh=u.load_asset(path)
        if not mesh:
            opts=u.FbxImportUI()
            for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,original_import_type=u.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,is_obj_import=True).items():opts.set_editor_property(k,v)
            imp=opts.static_mesh_import_data
            for k,v in dict(convert_scene=False,convert_scene_unit=False,combine_meshes=True,transform_vertex_to_absolute=True,auto_generate_collision=False,generate_lightmap_u_vs=False,build_nanite=False).items():imp.set_editor_property(k,v)
            imp.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
            task=u.AssetImportTask();task.filename=row['file'];task.destination_path=BASE+'/Meshes';task.destination_name=name;task.automated=True;task.save=True;task.options=opts;task.factory=u.FbxFactory();A.import_asset_tasks([task]);mesh=u.load_asset(path)
            if not mesh:raise RuntimeError('Mesh import '+name)
        for j,slot in enumerate(mesh.static_materials):
            key=str(slot.material_slot_name)
            if key not in mats:raise RuntimeError(f'Unknown material slot {name} {key}')
            mesh.set_material(j,mats[key])
        save(mesh);bounds=mesh.get_bounding_box();actual=[[bounds.min.x,bounds.min.y,bounds.min.z],[bounds.max.x,bounds.max.y,bounds.max.z]]
        if any(abs(a-b)>.3 for x,y in zip(actual,row['bounds_cm']) for a,b in zip(x,y)):raise RuntimeError(f'Bounds mismatch {name} {actual} != {row["bounds_cm"]}')
        rows.append(dict(id=row['id'],name=row['name'],asset=mesh.get_path_name(),bounds_cm=actual,materials=len(mesh.static_materials),source_sha256=row['source_sha256'],door_leaf=row['door_leaf']))
        u.log(f'ECHO_IMPORT {index+1}/{len(selected)} {name}')
        if index%30==29:u.SystemLibrary.collect_garbage()
    result=dict(success=True,version=20,meshes=rows,surfaces=surfaces,materials=len(mats),skeletal_deferred=D['skeletal_deferred'],probe=test)
    (P/'Saved/Prototype/echo_custom_import.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
    if not test:(P/'Content/CampaignData/echo_custom_runtime.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
    u.log('ECHO_IMPORT_COMPLETE '+str(len(rows)))
except Exception:
    (P/'Saved/Prototype/echo_custom_import_failed.txt').write_text(traceback.format_exc(),encoding='utf8');u.log_error(traceback.format_exc());raise
finally:u.SystemLibrary.quit_editor()

