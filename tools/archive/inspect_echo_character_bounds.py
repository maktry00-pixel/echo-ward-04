import bpy
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1]/'assets/models/echo_custom'
for old in list(bpy.data.objects):bpy.data.objects.remove(old,do_unlink=True)
bpy.ops.import_scene.gltf(filepath=str(root/'EH_563/EH_563.glb'))
bpy.context.scene.frame_set(1);bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
for ob in bpy.context.scene.objects:
    if ob.type!='MESH':continue
    ev=ob.evaluated_get(deps);me=ev.to_mesh();points=[ev.matrix_world@v.co for v in me.vertices]
    print('BOUNDS_DETAIL',ob.name,len(points),[m.type for m in ob.modifiers],[min(v[k] for v in points) for k in range(3)],[max(v[k] for v in points) for k in range(3)],flush=True)
    bad=[(j,tuple(p)) for j,p in enumerate(points) if p.z<-.1 or p.z>2]
    print('BOUND_OUTLIERS',len(bad),bad[:6],flush=True);ev.to_mesh_clear()
