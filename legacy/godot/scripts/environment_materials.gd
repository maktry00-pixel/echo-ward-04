extends RefCounted
## Shared imported CC0 PBR maps, also available in exported builds.

static var textures: Dictionary = {}
static var quality: String = "high"
static var registered: Array[WeakRef] = []
static func set_quality(value:String) -> void:
	if quality==value: return
	quality=value
	var live: Array[WeakRef] = []
	for ref:WeakRef in registered:
		var mat:StandardMaterial3D=ref.get_ref()
		if mat==null: continue
		live.append(ref)
		var id:String=mat.get_meta("pbr_id","")
		if mat.albedo_texture!=null: mat.albedo_texture=_map(id,"color")
		mat.normal_texture=_map(id,"normal")
		mat.roughness_texture=_map(id,"roughness")
	registered=live
static func _map(id:String,channel:String) -> Texture2D:
	var prefix:String="res://assets/textures/polyhaven/%s/"%id
	var hd:String=prefix+channel+"_4k.jpg"
	return _texture(hd if quality=="high" and ResourceLoader.exists(hd) else prefix+channel+".jpg")

static func pbr(id: String, tint: String = "ffffff", tiling: float = 0.5, roughness: float = 1.0) -> StandardMaterial3D:
	var prefix: String = "res://assets/textures/polyhaven/%s/" % id
	var mat := StandardMaterial3D.new()
	mat.set_meta("surface_kind","vinyl" if id=="old_linoleum_flooring_01" else "concrete")
	mat.set_meta("pbr_id",id)
	registered.append(weakref(mat))
	mat.albedo_texture = _map(id,"color")
	mat.albedo_color = Color(tint)
	mat.normal_enabled = true
	mat.normal_texture = _map(id,"normal")
	mat.normal_scale = .45 if id=="painted_plaster_wall" else .65
	mat.roughness_texture = _map(id,"roughness")
	mat.roughness = roughness
	mat.metallic_specular = .3
	if id=="terrazzo_tiles": mat.normal_scale = .2
	mat.uv1_triplanar = true
	mat.uv1_world_triplanar = true
	mat.uv1_scale = Vector3.ONE*tiling
	mat.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
	return mat


static func surface(kind: String, tint: String, scale: float, roughness: float = 0.8) -> StandardMaterial3D:
	var base: String = "Concrete035" if kind == "concrete" else "Tiles074"
	var folder: String = "concrete" if kind == "concrete" else "tiles"
	var prefix: String = "res://assets/textures/%s/%s_1K-JPG_" % [folder, base]
	if kind == "metal":
		prefix = "res://assets/textures/metal/Metal032_2K-JPG_"
	var material := StandardMaterial3D.new()
	material.set_meta("surface_kind",kind)
	material.albedo_color = Color(tint)
	material.albedo_texture = _texture(prefix + "Color.jpg")
	material.normal_enabled = true
	material.normal_texture = _texture(prefix + "NormalGL.jpg")
	material.normal_scale = 0.65
	material.roughness = roughness
	material.roughness_texture = _texture(prefix + "Roughness.jpg")
	if kind == "metal":
		material.metallic = 0.75
	material.uv1_triplanar = true
	material.uv1_world_triplanar = true
	material.uv1_scale = Vector3.ONE * scale
	material.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
	return material


static func _texture(path: String) -> Texture2D:
	if textures.has(path):
		return textures[path]
	var texture: Texture2D = load(path)
	textures[path] = texture
	return texture
