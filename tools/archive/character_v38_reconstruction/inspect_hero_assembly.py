import bpy,json
from pathlib import Path
from mathutils import Vector
bpy.ops.wm.open_mainfile(filepath='D:/Projects/ECHO/assets/models/hero_custom/photo_outfit_v1/beka_photo_outfit_v1.blend')
before=set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath='D:/Projects/ECHO/artifacts/echo_hero_v38/photo_fit_v2/face.fbx',use_anim=False)
rows=[]
for o in bpy.data.objects:
 if o.type not in ['MESH','ARMATURE']:continue
 row={'name':o.name,'type':o.type,'new_face_import':o not in before,'scale':list(o.scale)}
 if o.type=='MESH':
  vs=[o.matrix_world@v.co for v in o.data.vertices]
  row.update(vertices=len(vs),bounds=[[min(v[i] for v in vs) for i in range(3)],[max(v[i] for v in vs) for i in range(3)]],materials=[m.name for m in o.data.materials if m],modifiers=[m.type for m in o.modifiers])
 else:row['bones']={b.name:list(o.matrix_world@b.head_local) for b in o.data.bones if b.name in ['head','FACIAL_C_FacialRoot','neck_01','upperarm_l','lowerarm_l','hand_l']}
 rows.append(row)
out=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/manual_assembly_v1');out.mkdir(exist_ok=True)
(out/'source_inspection.json').write_text(json.dumps(rows,indent=2),encoding='utf8')
print('ECHO_INSPECT_DONE')
