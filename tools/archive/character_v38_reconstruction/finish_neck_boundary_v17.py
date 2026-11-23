import bpy,bmesh
from pathlib import Path
from mathutils import Vector
O=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/character_candidate_v17')
bpy.ops.wm.open_mainfile(filepath=str(O/'Beka_character_candidate_v17.blend'));bpy.context.preferences.filepaths.save_version=0
head=bpy.data.objects['Beka_head_likeness_candidate_v17'];bm=bmesh.new();bm.from_mesh(head.data)
ring={v for e in bm.edges if e.is_boundary for v in e.verts if v.co.z<1.491}
for v in ring:
 v.co.z=1.460;v.co.x=max(-.049,min(.049,v.co.x));v.co.y=max(-.040,min(.055,v.co.y))
bm.to_mesh(head.data);bm.free();head.data.update();bpy.ops.object.select_all(action='DESELECT')
for ob in bpy.context.scene.objects:
 if ob.type=='MESH':ob.select_set(True)
bpy.context.view_layer.objects.active=head
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_character_candidate_v17.blend'))
bpy.ops.export_scene.gltf(filepath=str(O/'Beka_character_candidate_v17.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
sc=bpy.context.scene;sc.cycles.samples=16
for name,pos in [('face',(0,-1,1.62)),('three_quarter_face',(-.6,-1,1.62))]:
 cam=sc.camera;cam.location=pos;cam.rotation_euler=(Vector((0,0,1.62))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=.34;sc.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True)
print('LOWER_NECK_BOUNDARY_CONCEALED',len(ring))
