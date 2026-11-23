import bpy,json
from pathlib import Path
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath='D:/Projects/ECHO/artifacts/echo_hero_v38/photo_fit_v2/face.fbx',use_anim=False)
r=next(o for o in bpy.data.objects if o.type=='ARMATURE')
rows={b.name:list(r.matrix_world@b.head_local) for b in r.data.bones if b.name.startswith('FACIAL_') and any(x in b.name.lower() for x in ['eye','nose','lip','chin','ear','jaw','cheek'])}
Path('D:/Projects/ECHO/artifacts/echo_hero_v38/manual_assembly_v1/facial_bones.json').write_text(json.dumps(rows,indent=2),encoding='utf8')
print('FACIAL_BONES_DONE '+str(len(rows)))
