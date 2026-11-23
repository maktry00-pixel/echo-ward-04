import bpy,math,json
from pathlib import Path
from mathutils import Vector
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/character_candidate_v16';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(R/'artifacts/echo_hero_v38/character_candidate_v15/Beka_character_candidate_v15.blend'))
bpy.context.preferences.filepaths.save_version=0
pants=bpy.data.objects['Beka_Trousers'];uv=pants.data.uv_layers.active;fixed=0
for p in pants.data.polygons:
 a,b,c=[uv.data[i].uv.copy() for i in list(p.loop_indices)[:3]]
 area=abs((b-a).x*(c-a).y-(b-a).y*(c-a).x)
 if area<1e-12 or (abs(p.normal.z)>.85 and (p.center.z>.925 or p.center.z<.14)):
  for li in p.loop_indices:
   co=pants.data.vertices[pants.data.loops[li].vertex_index].co;uv.data[li].uv=(.025+(co.x+.20)/.40*.95,.961+(co.y+.12)/.24*.035)
  fixed+=1
print('PLANAR_CAP_UV_FACES',fixed,flush=True)
m=pants.data.materials[0];n=m.node_tree.nodes;l=m.node_tree.links;bs=n.get('Principled BSDF');mix=next(x for x in n if x.bl_idname=='ShaderNodeMixRGB');bump=next(x for x in n if x.bl_idname=='ShaderNodeBump');l.new(mix.outputs[0],bs.inputs['Base Color']);l.new(bump.outputs[0],bs.inputs['Normal'])
al=bpy.data.images.new('Trouser_albedo_v16',width=2048,height=2048,alpha=False);no=bpy.data.images.new('Trouser_normal_v16',width=2048,height=2048,alpha=False);no.colorspace_settings.name='Non-Color'
bpy.ops.object.select_all(action='DESELECT');pants.select_set(True);bpy.context.view_layer.objects.active=pants;sc=bpy.context.scene;sc.cycles.samples=1
out=n.get('Material Output');em=n.new('ShaderNodeEmission');l.new(mix.outputs[0],em.inputs[0]);l.new(em.outputs[0],out.inputs[0]);ti=n.new('ShaderNodeTexImage');ti.image=al;n.active=ti;bpy.ops.object.bake(type='EMIT');al.filepath_raw=str(O/'Trouser_albedo_2048.png');al.file_format='PNG';al.save();al.pack()
l.new(bs.outputs[0],out.inputs[0]);n.remove(em);ni=n.new('ShaderNodeTexImage');ni.image=no;n.active=ni;bpy.ops.object.bake(type='NORMAL');no.filepath_raw=str(O/'Trouser_normal_2048.png');no.file_format='PNG';no.save();no.pack()
u=n.new('ShaderNodeUVMap');u.uv_map='TrouserCylindricalUV';ai=n.new('ShaderNodeTexImage');ai.image=al;l.new(u.outputs[0],ai.inputs[0]);l.new(ai.outputs[0],bs.inputs['Base Color']);l.new(u.outputs[0],ni.inputs[0]);nm=n.new('ShaderNodeNormalMap');l.new(ni.outputs[0],nm.inputs['Color']);l.new(nm.outputs[0],bs.inputs['Normal'])
objects=[o for o in sc.objects if o.type=='MESH'];bpy.ops.object.select_all(action='DESELECT')
for ob in objects:ob.select_set(True)
bpy.context.view_layer.objects.active=pants
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_character_candidate_v16.blend'))
bpy.ops.export_scene.gltf(filepath=str(O/'Beka_character_candidate_v16.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
sc.cycles.samples=24;sc.render.resolution_x=900;sc.render.resolution_y=1200
for name,pos,z,scale in [('front',(0,-3,1.0),.88,2.02),('back',(0,3,1.0),.88,2.02),('face',(0,-1,1.62),1.62,.34)]:
 cam=sc.camera;cam.data.ortho_scale=scale;cam.location=pos;cam.rotation_euler=(Vector((0,0,z))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True)
(O/'status.json').write_text(json.dumps({'status':'candidate_not_approved','likeness_verified':False,'game_changed':False,'rigged':False,'planar_cap_uv_faces_fixed':fixed}),encoding='utf8')
print('V16_CAP_UV_REPAIRED')
