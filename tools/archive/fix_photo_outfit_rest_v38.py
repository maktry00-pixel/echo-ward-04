import bpy,json
from pathlib import Path
from mathutils import Vector
out=Path('D:/Projects/ECHO/assets/models/hero_custom/photo_outfit_v1')
bpy.ops.wm.open_mainfile(filepath=str(out/'beka_photo_outfit_v1.blend'))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
print('Rig root basis',[(p.name,list(p.scale)) for p in rig.pose.bones if p.name in ['root','pelvis','spine_01']])
rig.data.pose_position='REST'
for p in rig.pose.bones:p.matrix_basis.identity()
bpy.context.view_layer.update()
parts=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.hide_render]
for o in parts:o.matrix_parent_inverse=rig.matrix_world.inverted()
bpy.context.view_layer.update()
print('Outfit evaluated bounds',[(o.name,list(o.dimensions)) for o in parts[:3]])
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True)
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(out/'beka_photo_outfit_v1.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y')
bpy.ops.export_scene.gltf(filepath=str(out/'beka_photo_outfit_v1.glb'),use_selection=True,export_format='GLB',export_animations=False,export_apply=False)
bpy.context.scene.cycles.samples=24;bpy.context.scene.render.filepath=str(out/'garment_review.png')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'beka_photo_outfit_v1.blend'))
bpy.ops.render.render(write_still=True)

