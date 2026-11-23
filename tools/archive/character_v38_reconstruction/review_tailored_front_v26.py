from pathlib import Path
import bpy,json,math
from mathutils import Vector
O=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/cloth_tailoring_v23/photo_tailored');bpy.ops.wm.open_mainfile(filepath=str(O/'Beka_photo_tailored_v23.blend'))
garment=bpy.data.objects['Beka photo black full zip outfit'];rig=garment.find_armature()
weights=[sum(g.weight for g in v.groups) for v in garment.data.vertices]
report={'vertex_count':len(weights),'unweighted_vertices':sum(w<.01 for w in weights),'non_normalized':sum(abs(w-1)>.02 for w in weights),'groups':len(garment.vertex_groups),'images':[]}
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.render.resolution_x=960;scene.render.resolution_y=1280;scene.render.resolution_percentage=100
if not scene.world:scene.world=bpy.data.worlds.new('Garment review world')
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.16,.18,.20,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.55
scene.view_settings.view_transform='AgX';scene.view_settings.exposure=-1.1
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.025));floor=bpy.context.object;floor.name='Review floor';mat=bpy.data.materials.new('Review grey');mat.use_nodes=True;mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.08,.09,.10,1);mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.9;floor.data.materials.append(mat)
for pos,power,size in [((-2,-3,3.5),550,3),((2,-1,2.3),350,2.5),((0,2,3),650,2)]:
    data=bpy.data.lights.new('Soft studio area','AREA');data.energy=power;data.shape='DISK';data.size=size;ob=bpy.data.objects.new('Soft studio area',data);bpy.context.collection.objects.link(ob);ob.location=pos;ob.rotation_euler=(Vector((0,0,1))-ob.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('Garment review');camera=bpy.data.objects.new('Garment review',data);bpy.context.collection.objects.link(camera);scene.camera=camera;data.sensor_fit='VERTICAL';data.sensor_height=36
views=[('outfit_front',(0,-3.1,.8),(0,0,.77),65),('outfit_three_quarter',(2.0,-2.8,1.2),(0,0,.80),65),('jacket_details',(.20,-1.2,1.38),(0,0,1.23),55),('boots',(.40,-.70,.30),(0,-.04,.10),55)]
for name,pos,target,lens in views[:1]:
    camera.location=pos;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();data.lens=lens;scene.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True);report['images'].append(name+'.png')
(O/'visual_qa.json').write_text(json.dumps(report,indent=2),encoding='utf8');print(json.dumps(report))
