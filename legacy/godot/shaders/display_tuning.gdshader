shader_type canvas_item;
render_mode unshaded;
uniform sampler2D scene : hint_screen_texture, filter_nearest;
uniform float brightness=1.;
uniform float contrast=1.;
uniform bool grain=true;
uniform bool vignette=true;
void fragment(){
    vec3 c=texture(scene,SCREEN_UV).rgb;
    c=(c*brightness-.5)*contrast+.5;
    vec2 p=SCREEN_UV*2.-1.;
    if(vignette)c*=1.-smoothstep(.45,1.4,length(p))*.28;
    if(grain)c+=(fract(sin(dot(FRAGCOORD.xy+floor(TIME*18.),vec2(12.9898,78.233)))*43758.5453)-.5)*.012;
    COLOR=vec4(clamp(c,vec3(0.),vec3(1.)),1.);
}
