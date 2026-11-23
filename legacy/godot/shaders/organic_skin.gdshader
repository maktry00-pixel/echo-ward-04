shader_type spatial;
render_mode diffuse_burley;
uniform vec4 tissue_color : source_color = vec4(0.21,0.15,0.12,1.0);
uniform float wetness : hint_range(0.0,1.0) = 0.12;
varying vec3 surface_position;
float hash3(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float noise3(vec3 p){
 vec3 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
 return mix(mix(mix(hash3(i),hash3(i+vec3(1,0,0)),f.x),mix(hash3(i+vec3(0,1,0)),hash3(i+vec3(1,1,0)),f.x),f.y),mix(mix(hash3(i+vec3(0,0,1)),hash3(i+vec3(1,0,1)),f.x),mix(hash3(i+vec3(0,1,1)),hash3(i+vec3(1,1,1)),f.x),f.y),f.z);
}
void vertex(){surface_position=VERTEX;}
void fragment(){
 vec3 p=surface_position*14.0;
 float mottling=noise3(p)*0.65+noise3(p*3.1)*0.35;
 float veins=1.0-smoothstep(0.025,0.10,abs(sin(p.y*1.9+noise3(p*.35)*8.0)));
 float pores=noise3(p*11.0);
 float scars=smoothstep(.65,.85,noise3(p*.47+vec3(9.0)));
 ALBEDO=mix(tissue_color.rgb,vec3(.30,.12,.105),scars*.55)*mix(.55,1.4,mottling)*mix(1.0,.55,veins)*mix(.86,1.05,pores);
 NORMAL=normalize(NORMAL+vec3(noise3(p*11.0+vec3(.05,0,0))-pores,noise3(p*11.0+vec3(0,.05,0))-pores,noise3(p*11.0+vec3(0,0,.05))-pores)*.13);
 ROUGHNESS=mix(.88,.44,wetness+scars*.24)*mix(.80,1.0,pores);
 SPECULAR=.25+wetness*.15;
}
