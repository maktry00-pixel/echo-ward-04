"""Import the complete campaign snapshot as native UE assets and a separate map.

Run through full UnrealEditor-Cmd -ExecutePythonScript. Does not overwrite Ward04.
Meshes/materials are shared; the native scene actor batches small spatial clusters.
"""
from pathlib import Path
import json
import shutil
import traceback
import struct
import zlib
import runpy
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
RAW = ROOT / "RawAssets/Campaign"
runpy.run_path(str(ROOT/'Content/Python/repair_set_dressing.py'))
if (ROOT.parents[1]/'assets/models/horror_game_maker/Poses/Idle.fbx').exists():runpy.run_path(str(ROOT/'Content/Python/import_stalker.py'))
if (ROOT/'RawAssets/Menu/menu_vent.wav').exists():runpy.run_path(str(ROOT/'Content/Python/import_title_media.py'))
if (ROOT/'Content/Python/polish_title_materials.py').exists():runpy.run_path(str(ROOT/'Content/Python/polish_title_materials.py'))
if (ROOT/'RawAssets/MenuNurse/nurse_cap.obj').exists():runpy.run_path(str(ROOT/'Content/Python/import_menu_nurse.py'))
if (ROOT/'RawAssets/MenuEvents/forearm.obj').exists():runpy.run_path(str(ROOT/'Content/Python/import_menu_events.py'))
if (ROOT.parents[1]/'assets/models/menu_zombie/pixelhouse/walk.FBX').exists():runpy.run_path(str(ROOT/'Content/Python/import_game_menu.py'))
DATA = json.loads((RAW / "campaign.json").read_text(encoding="utf-8"))
BASE = "/Game/Campaign"
LIB = u.EditorAssetLibrary
MAT = u.MaterialEditingLibrary
TOOLS = u.AssetToolsHelpers.get_asset_tools()
ACTORS = u.get_editor_subsystem(u.EditorActorSubsystem)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
REPORT = ROOT / "Saved/Prototype/campaign_import.json"


def load(path):
    result = u.load_asset(path)
    if not result:
        raise RuntimeError("Missing asset: " + path)
    return result


def imported(source, folder, name, options=None, force=False):
    path = f"{BASE}/{folder}/{name}"
    if LIB.does_asset_exist(path) and name != "SM_campaign_sign_quad" and not force:
        return load(path)
    task = u.AssetImportTask()
    for key, value in dict(filename=str(source), destination_path=f"{BASE}/{folder}",
                           destination_name=name, automated=True, save=True, replace_existing=True).items():
        task.set_editor_property(key, value)
    if options:
        task.set_editor_property("options", options)
        task.set_editor_property("factory", u.FbxFactory())
    TOOLS.import_asset_tasks([task])
    return load(path)


def native_obj(row):
    # Legacy OBJ/FbxFactory always reflects Y, even when convert_scene is off.
    # Counter-reflect source geometry only; actor transforms/colliders are already UE.
    source=RAW/row['file']
    if row['id']=='campaign_sign_quad':return source
    folder=RAW/'UEImportMeshes';folder.mkdir(exist_ok=True)
    target=folder/source.name
    lines=['mtllib '+source.stem+'.mtl']
    for line in source.read_text().splitlines():
        parts=line.split()
        if parts and parts[0] in ['v','vn']:
            parts[2]=str(-float(parts[2]));line=' '.join(parts)
        elif parts and parts[0]=='f':line='f '+' '.join(reversed(parts[1:]))
        elif line.startswith('o '):line='g '+line[2:]
        lines.append(line)
    target.write_text('\n'.join(lines)+'\n')
    (folder/(source.stem+'.mtl')).write_text(''.join(f'newmtl slot_{i}\nKd 0.5 0.5 0.5\n\n' for i in range(row.get('surfaces',1))))
    return target


def expr(mat, cls, **props):
    node = MAT.create_material_expression(mat, cls)
    for key, val in props.items():
        node.set_editor_property(key, val)
    return node


def save_asset(asset):
    if not LIB.save_loaded_asset(asset):
        raise RuntimeError("Could not save asset (close running game/editor): "+asset.get_path_name())


def master(translucent=False):
    name = "M_CampaignGlass" if translucent else "M_CampaignPBR"
    path = f"{BASE}/Materials/{name}"
    if LIB.does_asset_exist(path):
        return load(path)
    mat = TOOLS.create_asset(name, BASE + "/Materials", u.Material, u.MaterialFactoryNew())
    if translucent:
        mat.set_editor_property("blend_mode", u.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property("two_sided", True)
    coord = expr(mat, u.MaterialExpressionTextureCoordinate)
    tiling = expr(mat, u.MaterialExpressionVectorParameter, parameter_name="Tiling", default_value=u.LinearColor(1, 1, 0, 0))
    multiply_uv = expr(mat, u.MaterialExpressionMultiply)
    MAT.connect_material_expressions(coord, "", multiply_uv, "A")
    MAT.connect_material_expressions(tiling, "RG", multiply_uv, "B")
    tint = expr(mat, u.MaterialExpressionVectorParameter, parameter_name="Tint", default_value=u.LinearColor(1, 1, 1, 1))
    albedo = expr(mat, u.MaterialExpressionTextureSampleParameter2D, parameter_name="Albedo", texture=load(BASE + "/Textures/T_White"))
    MAT.connect_material_expressions(multiply_uv, "", albedo, "UVs")
    mul = expr(mat, u.MaterialExpressionMultiply)
    MAT.connect_material_expressions(albedo, "RGB", mul, "A")
    MAT.connect_material_expressions(tint, "RGB", mul, "B")
    MAT.connect_material_property(mul, "", u.MaterialProperty.MP_BASE_COLOR)
    if translucent:
        MAT.connect_material_property(tint, "A", u.MaterialProperty.MP_OPACITY)
    normal = expr(mat, u.MaterialExpressionTextureSampleParameter2D, parameter_name="Normal", texture=load(BASE + "/Textures/T_FlatNormal"), sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    MAT.connect_material_expressions(multiply_uv, "", normal, "UVs")
    flat = expr(mat, u.MaterialExpressionConstant3Vector, constant=u.LinearColor(0, 0, 1, 1))
    strength = expr(mat, u.MaterialExpressionScalarParameter, parameter_name="NormalStrength", default_value=1.0)
    lerp = expr(mat, u.MaterialExpressionLinearInterpolate)
    for node, output, pin in [(flat, "", "A"), (normal, "RGB", "B"), (strength, "", "Alpha")]:
        MAT.connect_material_expressions(node, output, lerp, pin)
    MAT.connect_material_property(lerp, "", u.MaterialProperty.MP_NORMAL)
    roughtex = expr(mat, u.MaterialExpressionTextureSampleParameter2D, parameter_name="RoughnessMap", texture=load(BASE + "/Textures/T_WhiteMasks"), sampler_type=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
    MAT.connect_material_expressions(multiply_uv, "", roughtex, "UVs")
    channel = expr(mat, u.MaterialExpressionVectorParameter, parameter_name="RoughnessChannel", default_value=u.LinearColor(1, 0, 0, 0))
    dot = expr(mat, u.MaterialExpressionDotProduct)
    MAT.connect_material_expressions(roughtex, "RGB", dot, "A")
    MAT.connect_material_expressions(channel, "RGB", dot, "B")
    scalar = expr(mat, u.MaterialExpressionScalarParameter, parameter_name="Roughness", default_value=0.8)
    mulr = expr(mat, u.MaterialExpressionMultiply)
    MAT.connect_material_expressions(dot, "", mulr, "A")
    MAT.connect_material_expressions(scalar, "", mulr, "B")
    MAT.connect_material_property(mulr, "", u.MaterialProperty.MP_ROUGHNESS)
    metal = expr(mat, u.MaterialExpressionScalarParameter, parameter_name="Metallic", default_value=0.0)
    MAT.connect_material_property(metal, "", u.MaterialProperty.MP_METALLIC)
    emission = expr(mat, u.MaterialExpressionVectorParameter, parameter_name="Emission", default_value=u.LinearColor(0, 0, 0, 0))
    MAT.connect_material_property(emission, "RGB", u.MaterialProperty.MP_EMISSIVE_COLOR)
    MAT.recompile_material(mat)
    save_asset(mat)
    return mat

def sign_master():
    path=BASE+"/Materials/M_CampaignSign"
    mat=u.load_asset(path)
    if mat:
        return mat
    mat=TOOLS.create_asset("M_CampaignSign",BASE+"/Materials",u.Material,u.MaterialFactoryNew())
    mat.set_editor_property("blend_mode",u.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided",True)
    mat.set_editor_property("shading_model",u.MaterialShadingModel.MSM_UNLIT)
    tex=expr(mat,u.MaterialExpressionTextureSampleParameter2D,parameter_name="Albedo",texture=load(BASE+"/Textures/T_White"))
    MAT.connect_material_property(tex,"RGB",u.MaterialProperty.MP_EMISSIVE_COLOR)
    MAT.connect_material_property(tex,"A",u.MaterialProperty.MP_OPACITY_MASK)
    MAT.recompile_material(mat)
    save_asset(mat)
    return mat

def world_master():
    """World-space PBR keeps the source's texel scale on long walls and floors."""
    path=BASE+"/Materials/M_CampaignWorldPBR"
    mat=u.load_asset(path)
    if mat:
        return mat
    mat=TOOLS.create_asset("M_CampaignWorldPBR",BASE+"/Materials",u.Material,u.MaterialFactoryNew())
    mat.set_editor_property("tangent_space_normal",False)
    nodes={
        "P":(expr(mat,u.MaterialExpressionWorldPosition),""),
        "N":(expr(mat,u.MaterialExpressionVertexInterpolator),""),
        "Tint":(expr(mat,u.MaterialExpressionVectorParameter,parameter_name="Tint",default_value=u.LinearColor(1,1,1,1)),"RGB"),
        "Tiling":(expr(mat,u.MaterialExpressionVectorParameter,parameter_name="Tiling",default_value=u.LinearColor(1,1,1,1)),"RGB"),
        "Channel":(expr(mat,u.MaterialExpressionVectorParameter,parameter_name="RoughnessChannel",default_value=u.LinearColor(1,0,0,0)),"RGB"),
        "Roughness":(expr(mat,u.MaterialExpressionScalarParameter,parameter_name="Roughness",default_value=.8),""),
        "Strength":(expr(mat,u.MaterialExpressionScalarParameter,parameter_name="NormalStrength",default_value=.65),""),
    }
    vnormal=expr(mat,u.MaterialExpressionVertexNormalWS)
    MAT.connect_material_expressions(vnormal,"",nodes["N"][0],"")
    for name,default,sampler in [("Albedo","T_White",u.MaterialSamplerType.SAMPLERTYPE_COLOR),
                                 ("Normal","T_FlatNormal",u.MaterialSamplerType.SAMPLERTYPE_NORMAL),
                                 ("RoughnessMap","T_WhiteMasks",u.MaterialSamplerType.SAMPLERTYPE_MASKS)]:
        nodes[name]=(expr(mat,u.MaterialExpressionTextureObjectParameter,parameter_name=name,texture=load(BASE+"/Textures/"+default),sampler_type=sampler),"")
    common="float3 w=pow(abs(N),4);w/=max(.001,w.x+w.y+w.z);float3 p=P*.01*Tiling.r;\n"
    color_code=common+"""
float3 c=Texture2DSample(Albedo,AlbedoSampler,p.yz).rgb*w.x+Texture2DSample(Albedo,AlbedoSampler,p.xz).rgb*w.y+Texture2DSample(Albedo,AlbedoSampler,p.xy).rgb*w.z;
float3 r=Texture2DSample(RoughnessMap,RoughnessMapSampler,p.yz).rgb*w.x+Texture2DSample(RoughnessMap,RoughnessMapSampler,p.xz).rgb*w.y+Texture2DSample(RoughnessMap,RoughnessMapSampler,p.xy).rgb*w.z;
return float4(c*Tint,dot(r,Channel)*Roughness);
"""
    normal_code=common+"""
float2 a=Texture2DSample(Normal,NormalSampler,p.yz).rg*2-1;
float2 b=Texture2DSample(Normal,NormalSampler,p.xz).rg*2-1;
float2 c=Texture2DSample(Normal,NormalSampler,p.xy).rg*2-1;
float3 nx=float3(sqrt(saturate(1-dot(a,a)))*sign(N.x),a.x,a.y);
float3 ny=float3(b.x,sqrt(saturate(1-dot(b,b)))*sign(N.y),b.y);
float3 nz=float3(c.x,c.y,sqrt(saturate(1-dot(c,c)))*sign(N.z));
return normalize(lerp(normalize(N),normalize(nx*w.x+ny*w.y+nz*w.z),Strength));
"""
    customs=[]
    for code,kind in [(color_code,u.CustomMaterialOutputType.CMOT_FLOAT4),(normal_code,u.CustomMaterialOutputType.CMOT_FLOAT3)]:
        custom=expr(mat,u.MaterialExpressionCustom,output_type=kind,code=code)
        inputs=[]
        for name in nodes:
            pin=u.CustomInput();pin.set_editor_property("input_name",name);inputs.append(pin)
        custom.set_editor_property("inputs",inputs)
        for name,(node,output) in nodes.items():
            if not MAT.connect_material_expressions(node,output,custom,name):
                raise RuntimeError("World PBR input failed: "+name)
        customs.append(custom)
    color=expr(mat,u.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False)
    rough=expr(mat,u.MaterialExpressionComponentMask,r=False,g=False,b=False,a=True)
    MAT.connect_material_expressions(customs[0],"",color,"")
    MAT.connect_material_expressions(customs[0],"",rough,"")
    MAT.connect_material_property(color,"",u.MaterialProperty.MP_BASE_COLOR)
    MAT.connect_material_property(rough,"",u.MaterialProperty.MP_ROUGHNESS)
    MAT.connect_material_property(customs[1],"",u.MaterialProperty.MP_NORMAL)
    metal=expr(mat,u.MaterialExpressionScalarParameter,parameter_name="Metallic",default_value=0.)
    emission=expr(mat,u.MaterialExpressionVectorParameter,parameter_name="Emission",default_value=u.LinearColor(0,0,0,0))
    MAT.connect_material_property(metal,"",u.MaterialProperty.MP_METALLIC)
    MAT.connect_material_property(emission,"RGB",u.MaterialProperty.MP_EMISSIVE_COLOR)
    MAT.recompile_material(mat);save_asset(mat)
    return mat
def main():
    # Tiny defaults are authored bytes, not replacement surface textures.
    for name, color, normal, masks in [("T_White", (255, 255, 255), False, False),
                                       ("T_FlatNormal", (128, 128, 255), True, False),
                                       ("T_WhiteMasks", (255, 255, 255), False, True)]:
        file = RAW / (name + ".png")
        def chunk(kind, payload):
            return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xffffffff)
        file.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 4, 4, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress((b"\0" + bytes(color)*4)*4)) + chunk(b"IEND", b""))
        tex = imported(file, "Textures", name)
        tex.set_editor_property("srgb", not (normal or masks))
        if normal:
            tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
        elif masks:
            tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_MASKS)
        save_asset(tex)
    normal_ids = {m["normal"] for m in DATA["materials"] if m["normal"]}
    rough_ids = {m["roughness_texture"] for m in DATA["materials"] if m["roughness_texture"]}
    textures = {}
    for row in DATA["textures"]:
        tex = imported(RAW / row["file"], "Textures", "T_" + row["id"])
        normal, masks = row["id"] in normal_ids, row["id"] in rough_ids
        tex.set_editor_property("srgb", not (normal or masks))
        if normal:
            tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("flip_green_channel", True)
        elif masks:
            tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_MASKS)
        save_asset(tex)
        textures[row["id"]] = tex
    parents = [master(False), master(True)]
    signs = sign_master()
    world_pbr = world_master()
    texture_sources={row["id"]:row.get("source","") for row in DATA["textures"]}
    organic_path = BASE + "/Materials/M_CampaignOrganic"
    organic = u.load_asset(organic_path)
    if not organic:
        organic = TOOLS.create_asset("M_CampaignOrganic", BASE + "/Materials", u.Material, u.MaterialFactoryNew())
    if organic:
        MAT.delete_all_material_expressions(organic)
        pos = expr(organic, u.MaterialExpressionPreSkinnedPosition)
        local = expr(organic, u.MaterialExpressionVertexInterpolator)
        if not MAT.connect_material_expressions(pos,"",local,""):
            raise RuntimeError("Organic local coordinates connection failed")
        tint = expr(organic, u.MaterialExpressionVectorParameter, parameter_name="Tint", default_value=u.LinearColor(.21,.15,.12,1))
        wet = expr(organic, u.MaterialExpressionScalarParameter, parameter_name="Wetness", default_value=.12)
        wood = expr(organic, u.MaterialExpressionScalarParameter, parameter_name="Wood", default_value=0.)
        grain = expr(organic, u.MaterialExpressionVectorParameter, parameter_name="Grain", default_value=u.LinearColor(.14,6,9,1))
        custom = expr(organic, u.MaterialExpressionCustom, output_type=u.CustomMaterialOutputType.CMOT_FLOAT4)
        inputs = []
        for name in ["P", "Tint", "Wetness", "Wood", "Grain"]:
            pin = u.CustomInput()
            pin.set_editor_property("input_name", name)
            inputs.append(pin)
        custom.set_editor_property("inputs", inputs)
        custom.set_editor_property("code", """
struct Tissue {
float hash(float3 p) { return frac(sin(dot(p,float3(127.1,311.7,74.7)))*43758.5453); }
float noise(float3 p) {
 float3 i=floor(p),f=frac(p); f=f*f*(3-2*f);
 return lerp(lerp(lerp(hash(i),hash(i+float3(1,0,0)),f.x),lerp(hash(i+float3(0,1,0)),hash(i+float3(1,1,0)),f.x),f.y),
 lerp(lerp(hash(i+float3(0,0,1)),hash(i+float3(1,0,1)),f.x),lerp(hash(i+float3(0,1,1)),hash(i+1),f.x),f.y),f.z);
}} t;
float3 p=P*.14;
if(Wood>.5) {
 float3 w=P*.01*Grain;
 float g=sin(w.y*22+t.noise(w*.7)*9+t.noise(w*3)*2)*.5+.5;
 float fleck=t.noise(w*8);
 float3 c=Tint.rgb*lerp(.52,1.22,g)*lerp(.82,1.05,fleck);
 return float4(c,lerp(.57,.84,fleck));
}
float mottling=t.noise(p)*.65+t.noise(p*3.1)*.35;
float veins=1-smoothstep(.025,.10,abs(sin(p.y*1.9+t.noise(p*.35)*8)));
float pores=t.noise(p*11);
float scars=smoothstep(.65,.85,t.noise(p*.47+9));
float3 c=lerp(Tint.rgb,float3(.30,.12,.105),scars*.55)*lerp(.55,1.4,mottling)*lerp(1,.55,veins)*lerp(.86,1.05,pores);
return float4(c,saturate(lerp(.88,.44,Wetness+scars*.24)*lerp(.8,1,pores)));
""")
        for node, output, pin in [(local,"","P"),(tint,"RGB","Tint"),(wet,"","Wetness"),(wood,"","Wood"),(grain,"RGB","Grain")]:
            MAT.connect_material_expressions(node,output,custom,pin)
        color = expr(organic,u.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False)
        rough = expr(organic,u.MaterialExpressionComponentMask,r=False,g=False,b=False,a=True)
        if not MAT.connect_material_expressions(custom,"",color,"") or not MAT.connect_material_expressions(custom,"",rough,""):
            raise RuntimeError("Organic shader output connection failed")
        MAT.connect_material_property(color,"",u.MaterialProperty.MP_BASE_COLOR)
        MAT.connect_material_property(rough,"",u.MaterialProperty.MP_ROUGHNESS)
        MAT.recompile_material(organic)
        save_asset(organic)
    # Explicitly compile HISM variants before saving. Runtime -game cannot repair
    # missing usage flags, and silently substitutes the default material.
    for parent in parents+[signs,world_pbr,organic]:
        MAT.set_base_material_usage(parent,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True)
        MAT.recompile_material(parent)
        save_asset(parent)
    for row in DATA["materials"]:
        name = "MI_" + row["id"]
        path = BASE + "/Materials/" + name
        mi = u.load_asset(path) or TOOLS.create_asset(name, BASE + "/Materials", u.MaterialInstanceConstant, u.MaterialInstanceConstantFactoryNew())
        environment=texture_sources.get(row.get("albedo"),"").startswith("res://assets/textures/")
        MAT.set_material_instance_parent(mi, signs if row.get("sign") else organic if row.get("organic") or row.get("wood") else world_pbr if environment and not row["masked"] else parents[int(row["masked"])])
        if row.get("wood"):
            MAT.set_material_instance_scalar_parameter_value(mi,"Wood",1.)
            MAT.set_material_instance_vector_parameter_value(mi,"Grain",u.LinearColor(*row["grain"],1))
        if row.get("organic"):
            MAT.set_material_instance_scalar_parameter_value(mi, "Wetness", row.get("wetness",.12))
        MAT.set_material_instance_vector_parameter_value(mi, "Tint", u.LinearColor(*row["color"]))
        MAT.set_material_instance_vector_parameter_value(mi, "Tiling", u.LinearColor(*row["uv"], 0, 0))
        channel = [0., 0., 0., 0.]
        channel[min(2, max(0, row["roughness_channel"]))] = 1.
        MAT.set_material_instance_vector_parameter_value(mi, "RoughnessChannel", u.LinearColor(*channel))
        for parameter, value in [("Roughness", row["roughness"]), ("Metallic", row["metallic"]), ("NormalStrength", row["normal_scale"])]:
            MAT.set_material_instance_scalar_parameter_value(mi, parameter, value)
        for parameter, key in [("Albedo", "albedo"), ("Normal", "normal"), ("RoughnessMap", "roughness_texture")]:
            if row[key]:
                MAT.set_material_instance_texture_parameter_value(mi, parameter, textures[row[key]])
        MAT.set_material_instance_vector_parameter_value(mi, "Emission", u.LinearColor(*(v*row["emission_energy"] for v in row["emission"][:3]), 1))
        MAT.set_material_usage_override(mi,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True,True)
        MAT.update_material_instance(mi)
        if not MAT.has_material_usage(mi,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES):
            raise RuntimeError("Missing instanced mesh material shader: "+name)
        save_asset(mi)
    geometry_stamp=ROOT/'Saved/Prototype/geometry_import_v2.json'
    rebuild_geometry=not geometry_stamp.exists()
    for i, row in enumerate(DATA["meshes"]):
        opts = u.FbxImportUI()
        for key, value in dict(automated_import_should_detect_type=False, mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,
                               original_import_type=u.FBXImportType.FBXIT_STATIC_MESH, import_as_skeletal=False,
                               import_mesh=True, import_materials=False, import_textures=False, is_obj_import=True).items():
            opts.set_editor_property(key, value)
        imp = opts.get_editor_property("static_mesh_import_data")
        for key, value in dict(convert_scene=False, convert_scene_unit=False, combine_meshes=True, transform_vertex_to_absolute=True,
                               auto_generate_collision=False, generate_lightmap_u_vs=False, build_nanite=False).items():
            imp.set_editor_property(key, value)
        imp.set_editor_property("normal_import_method", u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        imported(native_obj(row), "Meshes", "SM_" + row["id"], opts,force=rebuild_geometry)
        u.log(f"CAMPAIGN_MESH {i+1}/{len(DATA['meshes'])}")
    game_root = ROOT.parents[1]
    runpy.run_path(str(ROOT/'Content/Python/fix_campaign_character.py'))
    crouch=game_root/"assets/models/rocketbox/motion/m_crouch_idle.max.fbx"
    if crouch.is_file():
        opts=u.FbxImportUI()
        for key,value in dict(automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION,
                              original_import_type=u.FBXImportType.FBXIT_ANIMATION,import_mesh=False,import_as_skeletal=True,
                              import_animations=True,import_materials=False,import_textures=False,
                              skeleton=load("/Game/Prototype/Character/SK_Survivor").get_editor_property("skeleton"),override_animation_name="A_Crouch").items():
            opts.set_editor_property(key,value)
        anim=imported(crouch,"Character","A_Crouch",opts)
        anim.set_editor_property("force_root_lock",True)
        save_asset(anim)
    for row in DATA.get("audio", []):
        sound = imported(RAW / row["file"], "Audio", "A_" + row["id"])
        sound.set_editor_property("looping", row["loop"])
        save_asset(sound)
    for photo in (game_root / "assets/ui/guide").glob("*.jpg"):
        image=imported(photo, "Guide", "T_Guide_" + photo.stem)
        image.set_editor_property("lod_group",u.TextureGroup.TEXTUREGROUP_UI)
        image.set_editor_property("never_stream",True)
        image.set_editor_property("mip_gen_settings",u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        save_asset(image)
    target = ROOT / "Content/CampaignData"
    target.mkdir(parents=True, exist_ok=True)
    shutil.copy2(RAW / "campaign.json", target / "campaign.json")
    font_root = target / "Fonts"
    font_root.mkdir(exist_ok=True)
    for font in (game_root / "assets/ui/fonts").glob("*.ttf"):
        shutil.copy2(font, font_root / font.name)
    legal_root=target/"Legal"
    legal_root.mkdir(exist_ok=True)
    for source,name in [(game_root/"assets/CREDITS.md","CREDITS.md"),
                        (game_root/"assets/models/rocketbox/LICENSE.md","Rocketbox-MIT.md"),
                        (game_root/"assets/ui/fonts/notosans-OFL.txt","NotoSans-OFL.txt"),
                        (game_root/"assets/ui/fonts/literata-OFL.txt","Literata-OFL.txt"),
                        (game_root/"assets/models/polyhaven/LICENSE.txt","PolyHaven-models-CC0.txt"),
                        (game_root/"assets/textures/polyhaven/LICENSE.txt","PolyHaven-textures-CC0.txt")]:
        shutil.copy2(source,legal_root/name)
    scene_class = u.load_class(None, "/Script/EchoWard.EWardCampaignScene")
    if not scene_class:
        raise RuntimeError("Compile campaign scene class first")
    map_path = BASE + "/Maps/Campaign"
    if LIB.does_asset_exist(map_path):
        if not LEVELS.load_level(map_path):
            raise RuntimeError("Could not load generated Campaign map")
        for actor in ACTORS.get_all_level_actors():
            if actor.get_class() == scene_class or isinstance(actor, (u.PlayerStart, u.Light, u.SkyLight, u.ExponentialHeightFog, u.PostProcessVolume)):
                ACTORS.destroy_actor(actor)
    elif not LEVELS.new_level(map_path):
        raise RuntimeError("Could not create Campaign map")
    scene = ACTORS.spawn_actor_from_class(scene_class, u.Vector())
    scene.set_actor_label("Complete campaign / native clustered geometry")
    scene.rebuild_scene()
    mode_class = u.load_class(None, "/Script/EchoWard.EWardCampaignMode")
    if not mode_class:
        raise RuntimeError("Native campaign gameplay module is missing")
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", mode_class)
    start = DATA["start"]
    player = ACTORS.spawn_actor_from_class(u.PlayerStart, u.Vector(start[0], start[1], start[2]+105))
    player.set_actor_label("Campaign survivor start")
    # Keep only real practical lights; source-only helpers and unpowered fixtures
    # remain represented in JSON for the gameplay power-state controller.
    light_count = 0
    for row in DATA["lights"]:
        if not row["visible"] or row["energy"] <= 0 or row["type"] not in ["OmniLight3D", "SpotLight3D"]:
            continue
        pos = row["transform"]["position"]
        ceiling = row["shadow"] and pos[2] % 500 > 265
        actor = ACTORS.spawn_actor_from_class(u.RectLight if ceiling else u.PointLight, u.Vector(pos[0],pos[1],pos[2]-26 if ceiling else pos[2]),u.Rotator(pitch=-90.,yaw=0.,roll=0.) if ceiling else u.Rotator())
        if ceiling and actor.get_actor_forward_vector().z > -.9:
            raise RuntimeError("Ceiling light must point down: " + str(actor.get_actor_rotation()))
        actor.set_actor_label(row["path"])
        light = actor.get_component_by_class(u.RectLightComponent if ceiling else u.PointLightComponent)
        light.set_mobility(u.ComponentMobility.MOVABLE)
        light.set_editor_property("intensity_units", u.LightUnits.LUMENS)
        light.set_intensity(min(6500., row["energy"]*(2400. if ceiling else 1600.)))
        if ceiling:
            light.set_editor_property("source_width",72.)
            light.set_editor_property("source_height",24.)
        light.set_light_color(u.LinearColor(*row["color"]))
        light.set_attenuation_radius(min(1800., row["range"]*100))
        light.set_cast_shadows(row["shadow"])
        light.set_editor_property("max_draw_distance", 2800.)
        light.set_editor_property("max_distance_fade_range", 400.)
        light_count += 1
    sky = ACTORS.spawn_actor_from_class(u.SkyLight, u.Vector())
    sky.set_actor_label("Soft ambient fill / source ward ambience")
    skylight = sky.get_component_by_class(u.SkyLightComponent)
    skylight.set_mobility(u.ComponentMobility.MOVABLE)
    skylight.set_editor_property("source_type",u.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    skylight.set_editor_property("cubemap",load("/Engine/EngineResources/DefaultTextureCube"))
    skylight.set_intensity(.12)
    skylight.set_light_color(u.LinearColor(.48,.62,.52,1))
    skylight.set_cast_shadows(False)
    fog = ACTORS.spawn_actor_from_class(u.ExponentialHeightFog, u.Vector())
    fog_component = fog.get_component_by_class(u.ExponentialHeightFogComponent)
    fog_component.set_editor_property("fog_density", .004)
    fog_component.set_volumetric_fog(True)
    pp = ACTORS.spawn_actor_from_class(u.PostProcessVolume, u.Vector())
    pp.set_editor_property("unbound", True)
    settings = pp.get_editor_property("settings")
    settings.set_editor_property("ambient_cubemap",load("/Engine/MapTemplates/Sky/DaylightAmbientCubemap"))
    for key, value in dict(override_auto_exposure_min_brightness=True, override_auto_exposure_max_brightness=True,
                           auto_exposure_min_brightness=4.8, auto_exposure_max_brightness=4.8,
                           override_ambient_cubemap_intensity=True,ambient_cubemap_intensity=.65,
                           override_ambient_cubemap_tint=True,ambient_cubemap_tint=u.LinearColor(.28,.36,.30,1),
                           override_bloom_intensity=True, bloom_intensity=.20,
                           override_motion_blur_amount=True, motion_blur_amount=0.).items():
        settings.set_editor_property(key, value)
    pp.set_editor_property("settings", settings)
    runpy.run_path(str(ROOT/'Content/Python/upgrade_presentation.py'),init_globals={'UPDATE_CURRENT_LEVEL':True})
    if not LEVELS.save_current_level():
        raise RuntimeError("Campaign map could not be saved")
    if not LIB.save_directory(BASE, only_if_is_dirty=True, recursive=True):
        raise RuntimeError("Some campaign assets could not be saved")
    geometry_stamp.write_text(json.dumps({'conversion_version':2,'meshes':len(DATA['meshes'])}))
    return dict(success=True, meshes=len(DATA["meshes"]), materials=len(DATA["materials"]), textures=len(DATA["textures"]),
                objects=len(DATA["objects"]), rooms=len(DATA["rooms"]), lights=light_count,
                visual_instances=scene.get_editor_property("visual_instances"),
                collision_shapes=scene.get_editor_property("collision_shapes"),
                native_gameplay_port_complete=False)


try:
    result = main()
    REPORT.write_text(json.dumps(result, indent=2), encoding="utf-8")
    u.log("CAMPAIGN_IMPORT_OK " + str(REPORT))
except Exception:
    REPORT.write_text(json.dumps(dict(success=False, error=traceback.format_exc()), indent=2), encoding="utf-8")
    raise
