import bpy,json
from pathlib import Path
from mathutils import Vector
out=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/photo_fit_v2')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(out/'body.fbx'),use_anim=False)
r=[]
for o in bpy.context.scene.objects:
 row={'name':o.name,'type':o.type,'dimensions':list(o.dimensions),'scale':list(o.scale),'rotation':list(o.rotation_euler)}
 if o.type=='MESH':
  coords=[o.matrix_world@v.co for v in o.data.vertices]
  row['bounds']=[[min(v[i] for v in coords) for i in range(3)],[max(v[i] for v in coords) for i in range(3)]]
  row['vertices']=len(coords)
 if o.type=='ARMATURE':row['bones']={b.name:list(o.matrix_world@b.head_local) for b in o.data.bones if b.name in ['pelvis','spine_01','spine_02','spine_03','spine_04','spine_05','neck_01','clavicle_l','upperarm_l','lowerarm_l','hand_l','thigh_l','calf_l','foot_l','ball_l','head']}
 r.append(row)
(out/'body_blender_inspection.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
print(json.dumps(r))
