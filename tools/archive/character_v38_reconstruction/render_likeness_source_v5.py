import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path('D:/Projects/ECHO');OUT=ROOT/'artifacts/echo_hero_v38/likeness_geometry_v5';OUT.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'artifacts/echo_hero_v38/photo_fit_v2/face.fbx'),use_anim=False)
face=next(o for o in bpy.data.objects if o.type=='MESH')
def mat(name,col):
 m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*col,1);p.inputs['Roughness'].default_value=.65;return m
clay=mat('Untextured geometry study',(.45,.40,.36));eye=mat('Sclera',(.70,.68,.62));teeth=mat('Teeth',(.6,.55,.45))
for idx,m in enumerate(list(face.data.materials)):face.data.materials[idx]=eye if idx in [3,4] else teeth if idx==1 else clay
import bmesh
bm=bmesh.new();bm.from_mesh(face.data)
bmesh.ops.delete(bm,geom=[p for p in bm.faces if p.material_index not in [0,1,3,4]],context='FACES')
bm.to_mesh(face.data);bm.free();face.name='Neutral_Source_Head'
iris=mat('Brown iris',(.05,.02,.008));pupil=mat('Pupil',(.001,.001,.001))
for side in [-1,1]:
 for name,y,r,ma in [('iris',-.1107,.0058,iris),('pupil',-.1119,.0024,pupil)]:
  bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,location=(side*.031,y,1.637));o=bpy.context.object;o.name=name;o.scale=(r,.001,r);o.data.materials.append(ma)
world=bpy.data.worlds.new('Plain studio');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.13,.14,.15,1);world.node_tree.nodes['Background'].inputs[1].default_value=.4;bpy.context.scene.world=world
for pos,power,size in [((-1,-1.5,2.4),70,1.3),((1,-.9,1.9),45,1),((0,1,2.3),70,1)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.size=size;o.rotation_euler=(Vector((0,0,1.62))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(0,-1,1.62));cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=.33
sc=bpy.context.scene;sc.camera=cam;sc.render.engine='CYCLES';sc.cycles.samples=20;sc.cycles.use_denoising=True;sc.render.resolution_x=1024;sc.render.resolution_y=1024;sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'neutral_source.blend'))
for name,pos in [('source_front',(0,-1,1.62)),('source_nose_right',(-1,0,1.62)),('source_nose_left',(1,0,1.62))]:
 cam.location=pos;cam.rotation_euler=(Vector((0,0,1.62))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(OUT/(name+'.png'));bpy.ops.render.render(write_still=True)
(OUT/'camera.json').write_text(json.dumps({'orthographic_scale':.33,'resolution':1024,'target_z':1.62,'front_direction':[0,1,0]}),encoding='utf8')
print('NEUTRAL_GEOMETRY_RENDERED')

