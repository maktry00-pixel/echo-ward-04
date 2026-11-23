"""Run INSIDE Unreal Editor after compiling EchoWardEditor. Creates only /Game/Prototype.

This is a reproducible importer/map builder, not proof of native compilation or playtesting.
On failure Saved/Prototype/import_result.json records the error; bootstrap checks success.
"""
from pathlib import Path
import json
import traceback
import unreal as u

PROJECT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
RAW = PROJECT / "RawAssets"
BASE = "/Game/Prototype"
TOOLS = u.AssetToolsHelpers.get_asset_tools()
LIB = u.EditorAssetLibrary
MAT = u.MaterialEditingLibrary
ACTORS = u.get_editor_subsystem(u.EditorActorSubsystem)
MESHES = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
REPORT = PROJECT / "Saved/Prototype/import_result.json"
CREATED = []


def load(path, cls=None):
    asset = u.load_asset(path)
    if asset is None or (cls and not isinstance(asset, cls)):
        raise RuntimeError("Missing/wrong native asset: " + path)
    return asset


def import_asset(source, folder, name, options=None, factory=None):
    path = f"{BASE}/{folder}/{name}"
    if LIB.does_asset_exist(path):
        return load(path)
    task = u.AssetImportTask()
    for key, value in dict(filename=str(RAW / source), destination_path=f"{BASE}/{folder}",
                           destination_name=name, automated=True, save=True, replace_existing=False).items():
        task.set_editor_property(key, value)
    if options:
        task.set_editor_property("options", options)
    if factory:
        task.set_editor_property("factory", factory)
    TOOLS.import_asset_tasks([task])
    result = list(task.get_editor_property("imported_object_paths"))
    if not result:
        raise RuntimeError("Import returned no objects: " + source)
    CREATED.extend(result)
    return load(path)


def fbx_options(kind):
    opts = u.FbxImportUI()
    opts.set_editor_property("automated_import_should_detect_type", False)
    opts.set_editor_property("mesh_type_to_import", kind)
    opts.set_editor_property("original_import_type", kind)
    opts.set_editor_property("import_materials", False)
    opts.set_editor_property("import_textures", False)
    opts.set_editor_property("override_full_name", True)
    return opts


def static_mesh(record, folder="Meshes"):
    opts = fbx_options(u.FBXImportType.FBXIT_STATIC_MESH)
    opts.set_editor_property("import_as_skeletal", False)
    opts.set_editor_property("import_mesh", True)
    opts.set_editor_property("is_obj_import", True)
    data = opts.get_editor_property("static_mesh_import_data")
    for key, value in dict(convert_scene=False, convert_scene_unit=False, combine_meshes=True,
                           transform_vertex_to_absolute=True, auto_generate_collision=False,
                           generate_lightmap_u_vs=True, build_nanite=False).items():
        data.set_editor_property(key, value)
    data.set_editor_property("normal_import_method", u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    mesh = import_asset(record["source"], folder, record["name"], opts, u.FbxFactory())
    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError("Static mesh expected: " + record["name"])
    MESHES.remove_collisions(mesh)
    if MESHES.add_simple_collisions(mesh, u.ScriptCollisionShapeType.BOX) < 0:
        raise RuntimeError("No simple collision: " + record["name"])
    return mesh


def texture(source, name, kind):
    tex = import_asset(source, "Textures", name)
    if not isinstance(tex, u.Texture2D):
        raise RuntimeError("Texture2D expected: " + source)
    tex.set_editor_property("srgb", kind == "color")
    if kind == "normal":
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("flip_green_channel", True)  # Existing Poly Haven/Rocketbox GL normals.
    elif kind in ("arm", "roughness"):
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_MASKS)
    LIB.save_loaded_asset(tex)
    return tex


def expression(mat, cls, x, y, **values):
    item = MAT.create_material_expression(mat, cls, x, y)
    if not item:
        raise RuntimeError("Cannot create material expression: " + str(cls))
    for key, value in values.items():
        item.set_editor_property(key, value)
    return item


def material(name, textures=None, color=(.25, .29, .27), roughness=.8, metallic=0., tiling=(1., 1.), masked=False):
    path = f"{BASE}/Materials/{name}"
    mat = u.load_asset(path) or TOOLS.create_asset(name, f"{BASE}/Materials", u.Material, u.MaterialFactoryNew())
    MAT.delete_all_material_expressions(mat)
    if masked:
        mat.set_editor_property("blend_mode", u.BlendMode.BLEND_MASKED)
        mat.set_editor_property("two_sided", True)
    coord = expression(mat, u.MaterialExpressionTextureCoordinate, -750, -100, u_tiling=tiling[0], v_tiling=tiling[1])
    if textures:
        samples = {}
        for i, (kind, tex) in enumerate(textures.items()):
            sample = expression(mat, u.MaterialExpressionTextureSample, -470, i*220, texture=tex)
            if kind == "normal":
                sample.set_editor_property("sampler_type", u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            elif kind in ("arm", "roughness"):
                sample.set_editor_property("sampler_type", u.MaterialSamplerType.SAMPLERTYPE_MASKS)
            MAT.connect_material_expressions(coord, "", sample, "UVs")
            samples[kind] = sample
        for kind, prop in [("color", u.MaterialProperty.MP_BASE_COLOR), ("normal", u.MaterialProperty.MP_NORMAL),
                           ("roughness", u.MaterialProperty.MP_ROUGHNESS)]:
            if kind in samples:
                MAT.connect_material_property(samples[kind], "RGB" if kind != "roughness" else "R", prop)
        if "arm" in samples:
            for channel, prop in [("R", u.MaterialProperty.MP_AMBIENT_OCCLUSION), ("G", u.MaterialProperty.MP_ROUGHNESS), ("B", u.MaterialProperty.MP_METALLIC)]:
                MAT.connect_material_property(samples["arm"], channel, prop)
        elif "roughness" not in samples:
            scalar = expression(mat, u.MaterialExpressionConstant, -200, 300, r=roughness)
            MAT.connect_material_property(scalar, "", u.MaterialProperty.MP_ROUGHNESS)
        if masked and "color" in samples:
            MAT.connect_material_property(samples["color"], "A", u.MaterialProperty.MP_OPACITY_MASK)
    else:
        rgb = expression(mat, u.MaterialExpressionConstant3Vector, -300, 0, constant=u.LinearColor(*color, 1))
        MAT.connect_material_property(rgb, "", u.MaterialProperty.MP_BASE_COLOR)
        for value, prop in [(roughness, u.MaterialProperty.MP_ROUGHNESS), (metallic, u.MaterialProperty.MP_METALLIC)]:
            scalar = expression(mat, u.MaterialExpressionConstant, -300, 200, r=value)
            MAT.connect_material_property(scalar, "", prop)
    MAT.recompile_material(mat)
    LIB.save_loaded_asset(mat)
    return mat


def character():
    opts = fbx_options(u.FBXImportType.FBXIT_SKELETAL_MESH)
    for key, value in dict(import_as_skeletal=True, import_mesh=True, import_animations=False, create_physics_asset=True).items():
        opts.set_editor_property(key, value)
    data = opts.get_editor_property("skeletal_mesh_import_data")
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("normal_import_method", u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    mesh = import_asset("Character/SK_Survivor.fbx", "Character", "SK_Survivor", opts, u.FbxFactory())
    if not isinstance(mesh, u.SkeletalMesh):
        raise RuntimeError("Survivor must have a native skeletal mesh")
    skeleton = mesh.get_editor_property("skeleton")
    if not skeleton or not mesh.get_editor_property("physics_asset"):
        raise RuntimeError("Survivor skeleton/physics asset missing")
    for name in ("A_Idle", "A_Walk", "A_Run"):
        opt = fbx_options(u.FBXImportType.FBXIT_ANIMATION)
        opt.set_editor_property("import_mesh", False)
        opt.set_editor_property("import_as_skeletal", True)
        opt.set_editor_property("import_animations", True)
        opt.set_editor_property("skeleton", skeleton)
        opt.set_editor_property("override_animation_name", name)
        anim = import_asset(f"Character/{name}.fbx", "Character", name, opt, u.FbxFactory())
        if not isinstance(anim, u.AnimSequence):
            raise RuntimeError("Animation missing: " + name)
        anim.set_editor_property("enable_root_motion", True)
        anim.set_editor_property("force_root_lock", True)
        LIB.save_loaded_asset(anim)
    mats = {}
    for part in ("body", "head"):
        textures = {kind: texture(f"Character/m006_{part}_{suffix}.png", f"T_Character_{part}_{kind}", kind)
                    for kind, suffix in [("color", "color"), ("normal", "normal")]}
        mats[part] = material("M_Character_" + part, textures, roughness=.86)
    opacity = texture("Character/m006_opacity_color.png", "T_Character_opacity", "color")
    mats["opacity"] = material("M_Character_opacity", {"color": opacity}, masked=True)
    slots = list(mesh.get_editor_property("materials"))
    for slot in slots:
        name = str(slot.get_editor_property("material_slot_name")).lower()
        part = "opacity" if any(x in name for x in ("opacity", "hair", "lash")) else ("head" if "head" in name else "body")
        slot.set_editor_property("material_interface", mats[part])
    mesh.set_editor_property("materials", slots)
    LIB.save_loaded_asset(mesh)


def spawn(cls, name, pos, rotation=(0, 0, 0)):
    actor = ACTORS.spawn_actor_from_class(cls, u.Vector(*pos), u.Rotator(*rotation))
    if not actor:
        raise RuntimeError("Failed to spawn " + name)
    actor.set_actor_label(name)
    return actor


def prop(name, mesh, pos, yaw=0, mat=None, scale=(1, 1, 1), collision=True, physics=False):
    actor = spawn(u.StaticMeshActor, name, pos, (0, yaw, 0))
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    actor.set_actor_scale3d(u.Vector(*scale))
    if mat:
        comp.set_material(0, mat)
    comp.set_collision_profile_name("PhysicsActor" if physics else "BlockAll")
    if not collision:
        comp.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    if physics:
        comp.set_mobility(u.ComponentMobility.MOVABLE)
        comp.set_simulate_physics(True)
        comp.set_mass_override_in_kg("None", 4., True)
    return actor


def build_map(meshes, mats):
    if not LEVELS.new_level(f"{BASE}/Maps/Ward04"):
        raise RuntimeError("Could not create Ward04 map; close PIE and retry Setup")
    cube = load("/Engine/BasicShapes/Cube", u.StaticMesh)
    def box(name, pos, size, mat):
        return prop(name, cube, pos, mat=mat, scale=tuple(v/100 for v in size))
    box("Ward floor", (0, 0, -12), (1220, 920, 24), mats["floor"])
    box("Passage floor", (0, 785, -12), (300, 670, 24), mats["passage"])
    box("Ward ceiling", (0, 0, 352), (1220, 920, 24), mats["ceiling"])
    box("Passage ceiling", (0, 785, 352), (300, 670, 24), mats["ceiling"])
    for name, pos, size in [
        ("West wall", (-610, 0, 170), (20, 920, 340)),
        ("East wall", (610, 0, 170), (20, 920, 340)),
        ("South wall", (0, -460, 170), (1200, 20, 340)),
        ("North left", (-375, 460, 170), (470, 20, 340)),
        ("North right", (375, 460, 170), (470, 20, 340)),
        ("Door lintel", (0, 460, 310), (280, 20, 60)),
        ("Passage west", (-150, 790, 170), (20, 660, 340)),
        ("Passage east", (150, 790, 170), (20, 660, 340)),
        ("Passage end", (0, 1130, 170), (320, 20, 340))]:
        box(name, pos, size, mats["wall"])
    # Skirting and doorframe: clear, continuous collision geometry around the exit.
    for x in (-592, 592):
        box("Ward skirting", (x, 0, 9), (8, 900, 18), mats["metal"])
    for x in (-143, 143):
        box("Exit frame", (x, 447, 140), (6, 26, 280), mats["metal"])
    box("Exit frame top", (0, 447, 284), (292, 26, 8), mats["metal"])
    for n, pos, yaw in [(1, (-430, -190, 0), 0), (2, (370, -120, 0), 180)]:
        prop(f"Bed frame {n}", meshes["SM_Bed"], pos, yaw)
        box(f"Mattress {n}", (pos[0], pos[1], 50), (190, 86, 18), mats["linen"])
        box(f"Folded blanket {n}", (pos[0]+25, pos[1], 61), (90, 87, 5), mats["blanket"])
        box(f"Pillow {n}", (pos[0]-65, pos[1], 63), (36, 65, 10), mats["linen"])
    prop("Medicine cupboard", meshes["SM_Cabinet"], (540, -350, 0), 180)
    prop("Supply shelves", meshes["SM_Shelf"], (542, 210, 0), 180)
    prop("Desk chair", meshes["SM_Chair"], (-350, 335, 0), -90)
    prop("Reading lamp", meshes["SM_Lamp"], (-430, 175, 78), 0, collision=False)
    # Searchable nightstand, power panel and door are native gameplay actors spawned by GameMode.
    for i in range(6):
        box("Supply pack " + str(i), (545, 182+(i%3)*25, 48+(i//3)*45), (25, 18, 18), mats["paper"])
    for i, pos in enumerate([(-120, 90, 20), (160, 290, 20)]):
        prop("Loose supply box " + str(i), cube, pos, mat=mats["paper"], scale=(.35, .28, .30), physics=True)
    spawn(u.PlayerStart, "Survivor start", (-200, -270, 110), (0, 45, 0))
    for name, pos, color, power in [
        ("Ward practical", (-260, -100, 325), (1., .76, .47), 4200),
        ("Cold ward practical", (250, 220, 325), (.45, .70, .78), 2800),
        ("Passage practical", (0, 810, 325), (.55, .75, .66), 2200)]:
        actor = spawn(u.RectLight, name, pos, (-90, 0, 0))
        light = actor.get_component_by_class(u.RectLightComponent)
        light.set_mobility(u.ComponentMobility.MOVABLE)
        light.set_editor_property("intensity_units", u.LightUnits.LUMENS)
        light.set_intensity(power)
        light.set_light_color(u.LinearColor(*color, 1))
        light.set_attenuation_radius(1100)
        light.set_editor_property("source_width", 90.)
        light.set_editor_property("source_height", 25.)
    lamp = spawn(u.PointLight, "Warm lamp pool", (-430, 175, 130))
    point = lamp.get_component_by_class(u.PointLightComponent)
    point.set_mobility(u.ComponentMobility.MOVABLE)
    point.set_editor_property("intensity_units", u.LightUnits.LUMENS)
    point.set_intensity(320)
    point.set_attenuation_radius(300)
    point.set_light_color(u.LinearColor(1, .53, .25, 1))
    fog = spawn(u.ExponentialHeightFog, "Subtle interior haze", (0, 0, 0))
    fogcomp = fog.get_component_by_class(u.ExponentialHeightFogComponent)
    fogcomp.set_editor_property("fog_density", .006)
    fogcomp.set_volumetric_fog(True)
    volume = spawn(u.PostProcessVolume, "Ward exposure", (0, 0, 0))
    volume.set_editor_property("unbound", True)
    settings = volume.get_editor_property("settings")
    for key, value in dict(override_auto_exposure_min_brightness=True, override_auto_exposure_max_brightness=True,
                           auto_exposure_min_brightness=4.5, auto_exposure_max_brightness=4.5,
                           override_bloom_intensity=True, bloom_intensity=.18,
                           override_vignette_intensity=True, vignette_intensity=.28,
                           override_motion_blur_amount=True, motion_blur_amount=0.).items():
        settings.set_editor_property(key, value)
    volume.set_editor_property("settings", settings)
    if not LEVELS.save_current_level():
        raise RuntimeError("Failed to save map")
    if not LIB.save_directory(BASE, only_if_is_dirty=False, recursive=True):
        raise RuntimeError("Could not save imported assets")
    return len(ACTORS.get_all_level_actors())


def main():
    data = json.loads((RAW / "manifest.json").read_text(encoding="utf-8"))
    if not u.load_class(None, "/Script/EchoWard.EWardCharacter"):
        raise RuntimeError("Compile EchoWardEditor first; native gameplay module is missing")
    mats = {}
    textures_by_prop = {}
    meshes = {}
    for record in data["meshes"]:
        asset = record["asset"]
        if asset not in textures_by_prop:
            textures_by_prop[asset] = {kind: texture(src, "T_"+asset+"_"+kind, kind) for kind, src in record["textures"].items()}
        mat = material("M_"+asset, textures_by_prop[asset])
        mesh = static_mesh(record)
        for index in range(len(mesh.get_editor_property("static_materials"))):
            mesh.set_material(index, mat)
        meshes[record["name"]] = mesh
    for record in data["surfaces"]:
        mats[record["name"]] = {kind: texture(src, "T_"+record["name"]+"_"+kind, kind) for kind, src in record["textures"].items()}
    mats["floor"] = material("M_Floor", mats["terrazzo_tiles"], tiling=(6, 4.5))
    mats["passage"] = material("M_Passage", mats["terrazzo_tiles"], tiling=(1.5, 3.35))
    mats["wall"] = material("M_Wall", mats["blue_plaster_weathered"], tiling=(4.5, 1.7))
    mats["ceiling"] = material("M_Ceiling", mats["painted_plaster_wall"], tiling=(6, 4.5))
    for key, color, rough, metal in [("metal", (.12, .16, .15), .55, .8), ("linen", (.47, .45, .36), .95, 0),
                                    ("blanket", (.12, .20, .18), .98, 0), ("paper", (.34, .29, .20), .9, 0),
                                    ("door", (.16, .22, .19), .7, .3)]:
        mats[key] = material("M_" + key.title(), color=color, roughness=rough, metallic=metal)
    creature_mats = [material(m["name"], color=m["color"], roughness=m["roughness"]) for m in data["monster"]["materials"]]
    for record in data["monster"]["parts"]:
        mesh = static_mesh(record, "Monster")
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            name = str(slot.get_editor_property("material_slot_name"))
            number = int(name.rsplit("_", 1)[1])
            mesh.set_material(index, creature_mats[number])
    for record in data["audio"]:
        sound = import_asset(record["source"], "Audio", record["name"])
        sound.set_editor_property("looping", record["loop"])
        LIB.save_loaded_asset(sound)
    character()
    count = build_map(meshes, mats)
    return dict(success=True, native_import=True, map=f"{BASE}/Maps/Ward04", actors=count,
                created=CREATED, playtested=False, fps_measured=False)


if __name__ == "__main__":
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    try:
        result = main()
        REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        u.log("ECHO_WARD_IMPORT_OK: " + str(REPORT))
    except Exception:
        result = dict(success=False, error=traceback.format_exc(), created=CREATED)
        REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        u.log_error(result["error"])
        raise
