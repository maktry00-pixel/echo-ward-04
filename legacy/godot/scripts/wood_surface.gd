extends RefCounted
## Original continuous grain: furniture texture atlases are unsuitable for planks.
static func make(tint: Color = Color("796447"), grain: Vector3 = Vector3(.14,6,9)) -> ShaderMaterial:
	var mat := ShaderMaterial.new()
	mat.shader = preload("res://shaders/wood_grain.gdshader")
	mat.set_shader_parameter("wood_color",tint)
	mat.set_shader_parameter("grain_scale",grain)
	mat.set_meta("surface_kind","wood")
	return mat
