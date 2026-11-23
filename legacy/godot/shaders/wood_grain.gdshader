shader_type spatial;
uniform vec4 wood_color : source_color = vec4(.47,.39,.28,1.0);
uniform vec3 grain_scale = vec3(.14,6.0,9.0);
varying vec3 wood_position;
void vertex() { wood_position = VERTEX; }
void fragment() {
	vec3 p = wood_position * grain_scale;
	float warp = sin(p.x*2.3+p.z*.8)*.8 + sin(p.x*7.0)*.12;
	float grain = sin((p.y+p.z+warp)*35.0)*.5+.5;
	float fine = sin((p.y+p.z+warp)*133.0)*.5+.5;
	float weather = sin(p.x*5.1+p.y*1.7)*sin(p.z*2.7)*.5+.5;
	ALBEDO = wood_color.rgb * (.67+.22*grain+.08*fine+.16*weather);
	ROUGHNESS = .78+.14*(1.0-fine);
	SPECULAR = .20;
}
