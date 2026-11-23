"""Remove old background contamination from the 3D facial material, keeping its tone."""
from pathlib import Path
import bpy,json
R=Path('D:/Projects/ECHO');S=R/'artifacts/echo_hero_v38/metahuman_conform_v30';O=R/'artifacts/echo_hero_v38/metahuman_conform_v31';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(S/'Beka_photo_face_replacement_v30.blend'));bpy.context.preferences.filepaths.save_version=0
head=bpy.data.objects['BekaTarget_Head'];mat=head.data.materials[0];nodes=mat.node_tree.nodes;links=mat.node_tree.links
old=next(n for n in nodes if n.type=='TEX_IMAGE' and n.image and 'Beka_SkinDiffuse_NativeUV_4096' in n.image.name)
face_mix=next(n for n in nodes if n.type=='MIX_RGB' and n.blend_type=='MIX')
lum=nodes.new('ShaderNodeRGBToBW');links.new(old.outputs['Color'],lum.inputs[0]);ramp=nodes.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.43;ramp.color_ramp.elements[0].color=(0,0,0,1);ramp.color_ramp.elements[1].position=.64;ramp.color_ramp.elements[1].color=(1,1,1,1);links.new(lum.outputs[0],ramp.inputs[0])
clean=nodes.new('ShaderNodeMixRGB');clean.label='Remove bright neutral background from source atlas only';links.new(ramp.outputs[0],clean.inputs[0]);links.new(old.outputs['Color'],clean.inputs[1]);clean.inputs[2].default_value=(.34,.20,.13,1);links.new(clean.outputs[0],face_mix.inputs[1])
atlas=bpy.data.images.new('Beka_PhotoFaceV31_4096',width=4096,height=4096,alpha=False);dest=nodes.new('ShaderNodeTexImage');dest.image=atlas;nodes.active=dest
head.data.uv_layers.active_index=0;head.data.uv_layers[0].active_render=True;bpy.ops.object.select_all(action='DESELECT');head.select_set(True);bpy.context.view_layer.objects.active=head;sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=1;sc.render.bake.margin=12;sc.render.image_settings.color_mode='RGB';sc.render.image_settings.color_depth='8';bpy.ops.object.bake(type='EMIT');atlas.filepath_raw=str(O/'Beka_PhotoFaceV31_NativeUV_4096.png');atlas.file_format='PNG';atlas.save()
(O/'material_report.json').write_text(json.dumps({'geometry_changed':False,'clothing_changed':False,'base_tone_linear':[.34,.20,.13],'photo_face_preserved':True,'correction':'Bright neutral background contamination in old base atlas is smoothly replaced by its accepted neutral skin tone. The new photo-face layer remains unchanged.','atlas_resolution':[4096,4096],'genuine_new_4k_detail':False},indent=2),encoding='utf8');bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_photo_face_material_v31.blend'));print('PHOTO_FACE_V31_MATERIAL_BAKED')
