"""Bake a skin-only projection mask in the 3D material; preserve the source photo."""
from pathlib import Path
s=Path(__file__).with_name('bake_clean_skin_v21.py').read_text(encoding='utf8').replace('metahuman_conform_v21','metahuman_conform_v25')
s=s.replace("s=s.replace('blend.data[v.index].color=(w,w,w,1)', 'w*=float(np.clip((p.z-1.54)/.035,0,1))\\n blend.data[v.index].color=(w,w,w,1)')",'').replace('(.46,.285,.19)','(.34,.20,.13)')
needle="head.data.materials.clear();head.data.materials.append(mat)"
replacement="""nodes=mat.node_tree.nodes;links=mat.node_tree.links
photo_node=next(n for n in nodes if n.type=='TEX_IMAGE');attr=next(n for n in nodes if n.type=='VERTEX_COLOR');mix=next(n for n in nodes if n.type=='MIX_RGB')
lum=nodes.new('ShaderNodeRGBToBW');links.new(photo_node.outputs['Color'],lum.inputs[0])
ramp=nodes.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.50;ramp.color_ramp.elements[0].color=(1,1,1,1);ramp.color_ramp.elements[1].position=.72;ramp.color_ramp.elements[1].color=(0,0,0,1);links.new(lum.outputs[0],ramp.inputs[0])
mask=nodes.new('ShaderNodeMath');mask.operation='MULTIPLY';links.new(attr.outputs['Color'],mask.inputs[0]);links.new(ramp.outputs[0],mask.inputs[1]);links.new(mask.outputs[0],mix.inputs[0])
head.data.materials.clear();head.data.materials.append(mat)"""
if needle not in s:raise RuntimeError('Baking material hook missing')
s=s.replace(needle,replacement);exec(compile(s,str(Path(__file__).with_name('bake_clean_skin_v21.py')),'exec'))
