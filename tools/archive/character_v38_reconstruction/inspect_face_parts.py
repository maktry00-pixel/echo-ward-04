import bpy,json
from pathlib import Path
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath='D:/Projects/ECHO/artifacts/echo_hero_v38/photo_fit_v2/face.fbx',use_anim=False)
o=next(o for o in bpy.data.objects if o.type=='MESH')
rows=[]
for idx,mat in enumerate(o.data.materials):
 ids={i for p in o.data.polygons if p.material_index==idx for i in p.vertices}
 if not ids:continue
 vs=[o.matrix_world@o.data.vertices[i].co for i in ids]
 rows.append(dict(index=idx,name=mat.name,vertices=len(ids),faces=sum(p.material_index==idx for p in o.data.polygons),bounds=[[min(v[i] for v in vs) for i in range(3)],[max(v[i] for v in vs) for i in range(3)]]))
print('FACEPARTS '+json.dumps(rows))
