"""A readable dark-blue overcast sky, separate from the title scene."""
from pathlib import Path
import json,unreal as u
E=u.EditorAssetLibrary;L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools();base='/Game/Campaign/EchoCustom/Materials';name='M_EchoSkyV22';m=u.load_asset(base+'/'+name) or A.create_asset(name,base,u.Material,u.MaterialFactoryNew())
L.delete_all_material_expressions(m);m.set_editor_property('two_sided',True);m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
v=L.create_material_expression(m,u.MaterialExpressionCameraVectorWS);bright=L.create_material_expression(m,u.MaterialExpressionScalarParameter);bright.set_editor_property('parameter_name','Bright');bright.set_editor_property('default_value',0)
c=L.create_material_expression(m,u.MaterialExpressionCustom);c.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
c.set_editor_property('code','''struct Q{float h(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}float n(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y);}}q;
float h=saturate(abs(V.z));float2 p=V.xy/(abs(V.z)+.2)*2.5;float cloud=q.n(p)*.52+q.n(p*2.07)*.27+q.n(p*4.13)*.14+q.n(p*8.3)*.07;
float3 night=lerp(float3(.24,.34,.55),float3(.045,.09,.22),pow(h,.55));night*=lerp(.32,1.1,smoothstep(.26,.75,cloud));
return lerp(night,float3(5,6,8)*(0.72+cloud*.5),saturate(Bright));''')
pins=[]
for name,n in [('V',v),('Bright',bright)]:pin=u.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
c.set_editor_property('inputs',pins);L.connect_material_expressions(v,'',c,'V');L.connect_material_expressions(bright,'',c,'Bright');L.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR);L.recompile_material(m);assert E.save_loaded_asset(m)
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));(P/'Saved/Prototype/quality_sky_v22.json').write_text(json.dumps(dict(success=True,asset=m.get_path_name())))
u.SystemLibrary.quit_editor()
