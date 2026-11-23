"""Native procedural wear/cloth/dust materials, confined to the title set."""
from pathlib import Path
import json
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
BASE='/Game/Campaign/Menu'
L=u.MaterialEditingLibrary
TOOLS=u.AssetToolsHelpers.get_asset_tools()
def node(m,cls,**props):
    n=L.create_material_expression(m,cls)
    for k,v in props.items():n.set_editor_property(k,v)
    return n
def mat(name):
    m=u.load_asset(BASE+'/'+name) or TOOLS.create_asset(name,BASE,u.Material,u.MaterialFactoryNew())
    L.delete_all_material_expressions(m)
    return m
def constant(m,prop,v):L.connect_material_property(node(m,u.MaterialExpressionConstant,r=v),'',prop)
noise='''struct N {
float h(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
float n(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y);}
};N n;
float2 p=UV;
float edge=(1-smoothstep(.31,.5,abs(p.x-.5)))*(1-smoothstep(.32,.5,abs(p.y-.5)));
float a=n.n(p*6), b=n.n(p*31), c=n.n(p*109);
float drips=pow(saturate(n.n(float2(p.x*33,0))-.25)*1.33,3)*smoothstep(.98,.18,p.y);
return saturate(edge*(.055+a*.13+b*.045+drips*.32));'''
m=mat('M_MenuWear');m.set_editor_property('material_domain',u.MaterialDomain.MD_DEFERRED_DECAL);m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
uv=node(m,u.MaterialExpressionTextureCoordinate)
custom=node(m,u.MaterialExpressionCustom,code=noise,output_type=u.CustomMaterialOutputType.CMOT_FLOAT1)
pin=u.CustomInput();pin.set_editor_property('input_name','UV');custom.set_editor_property('inputs',[pin]);L.connect_material_expressions(uv,'',custom,'UV')
L.connect_material_property(custom,'',u.MaterialProperty.MP_OPACITY)
color=node(m,u.MaterialExpressionVectorParameter,parameter_name='DirtTint',default_value=u.LinearColor(.027,.023,.014,1))
L.connect_material_property(color,'RGB',u.MaterialProperty.MP_BASE_COLOR);constant(m,u.MaterialProperty.MP_ROUGHNESS,.93)
L.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
m=mat('M_MenuDust');m.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE);m.set_editor_property('two_sided',True);m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
uv=node(m,u.MaterialExpressionTextureCoordinate)
custom=node(m,u.MaterialExpressionCustom,code='return pow(saturate(1-length(UV-.5)*2),2)*.13;',output_type=u.CustomMaterialOutputType.CMOT_FLOAT1)
pin=u.CustomInput();pin.set_editor_property('input_name','UV');custom.set_editor_property('inputs',[pin]);L.connect_material_expressions(uv,'',custom,'UV');L.connect_material_property(custom,'',u.MaterialProperty.MP_OPACITY)
color=node(m,u.MaterialExpressionConstant3Vector,constant=u.LinearColor(6,7,7,1));L.connect_material_property(color,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
L.set_base_material_usage(m,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True)
L.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
m=mat('M_MenuLinen');m.set_editor_property('two_sided',False)
uv=node(m,u.MaterialExpressionTextureCoordinate)
custom=node(m,u.MaterialExpressionCustom,code='float2 t=UV*180;float aa=saturate(1-max(length(ddx(t)),length(ddy(t))));float weave=(sin(t.x*6.283)*sin(t.y*6.283))*.005*aa;float stain=smoothstep(.6,.95,sin(UV.x*17+sin(UV.y*11))*.5+.5)*.035;return float3(.37,.39,.34)+weave-stain;',output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
pin=u.CustomInput();pin.set_editor_property('input_name','UV');custom.set_editor_property('inputs',[pin]);L.connect_material_expressions(uv,'',custom,'UV');L.connect_material_property(custom,'',u.MaterialProperty.MP_BASE_COLOR);constant(m,u.MaterialProperty.MP_ROUGHNESS,.96)
L.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
(ROOT/'Saved/Prototype/title_materials.json').write_text(json.dumps(dict(success=True,materials=['M_MenuWear','M_MenuDust','M_MenuLinen'])))
