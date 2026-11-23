extends CanvasLayer
## Subtle image finish below HUD; no motion blur or artificial camera movement.
func _ready() -> void:
	layer = -1
	var surface := ColorRect.new()
	surface.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	surface.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var shader := Shader.new()
	shader.code = """shader_type canvas_item;
uniform sampler2D screen_texture : hint_screen_texture, filter_linear;
float hash(vec2 p){return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453);}
void fragment(){
 vec3 c=texture(screen_texture,SCREEN_UV).rgb;
 float grey=dot(c,vec3(0.2126,0.7152,0.0722));
 c=mix(vec3(grey),c,0.90)*vec3(1.015,1.0,0.98);
 float edge=smoothstep(0.25,0.88,length((UV-vec2(0.5))*vec2(1.35,1.0)));
 float grain=hash(FRAGCOORD.xy+floor(TIME*24.0))-0.5;
 COLOR=vec4(max(vec3(0.0),c*(1.0-edge*0.18)+grain*0.009),1.0);
}"""
	var material := ShaderMaterial.new()
	material.shader = shader
	surface.material = material
	add_child(surface)
