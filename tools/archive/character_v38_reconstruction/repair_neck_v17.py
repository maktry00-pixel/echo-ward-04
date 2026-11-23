import bpy,bmesh,json
from pathlib import Path
from mathutils import Vector
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/character_candidate_v17';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(R/'artifacts/echo_hero_v38/character_candidate_v16/Beka_character_candidate_v16.blend'))
bpy.context.preferences.filepaths.save_version=0
old=next(o for o in bpy.context.scene.objects if 'likeness_candidate' in o.name);oldtop=max(v.co.z for v in old.data.vertices)
with bpy.data.libraries.load(str(R/'artifacts/echo_hero_v38/likeness_head_v11/Beka_head_likeness_candidate_v11.blend'),link=False) as (a,b):
 b.objects=[x for x in a.objects if x=='Beka_head_likeness_candidate_v11']
head=b.objects[0];bpy.context.collection.objects.link(head);factor=oldtop/max(v.co.z for v in head.data.vertices)
for v in head.data.vertices:v.co*=factor
bpy.data.objects.remove(old,do_unlink=True);head.name='Beka_head_likeness_candidate_v17'
bm=bmesh.new();bm.from_mesh(head.data)
# Only hidden lower neck/shoulder material is cropped. Preserve the jaw and the neck above it.
drop=[v for v in bm.verts if v.co.z<1.475 or (v.co.z<1.490 and abs(v.co.x)>.058)]
bmesh.ops.delete(bm,geom=drop,context='VERTS')
ring={v for e in bm.edges if e.is_boundary for v in e.verts if v.co.z<1.491}
for v in ring:
 v.co.z=1.460;v.co.x=max(-.049,min(.049,v.co.x));v.co.y=max(-.040,min(.055,v.co.y))
bm.to_mesh(head.data);bm.free();head.data.update()
sc=bpy.context.scene;objects=[o for o in sc.objects if o.type=='MESH'];bpy.ops.object.select_all(action='DESELECT')
for ob in objects:ob.select_set(True)
bpy.context.view_layer.objects.active=head
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_character_candidate_v17.blend'))
bpy.ops.export_scene.gltf(filepath=str(O/'Beka_character_candidate_v17.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
sc.cycles.samples=24;sc.render.resolution_x=900;sc.render.resolution_y=1200
for name,pos,z,scale in [('front',(0,-3,1.0),.88,2.02),('face',(0,-1,1.62),1.62,.34),('three_quarter_face',(-.6,-1,1.62),1.62,.34)]:
 cam=sc.camera;cam.data.ortho_scale=scale;cam.location=pos;cam.rotation_euler=(Vector((0,0,z))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True)
(O/'status.json').write_text(json.dumps({'status':'candidate_not_approved','likeness_verified':False,'game_changed':False,'rigged':False,'neck_repair':'Restored uncut jaw and limited cropping to concealed lower neck'}),encoding='utf8')
print('V17_NECK_REPAIR_COMPLETE')
