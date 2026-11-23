"""Exercise exported skeletal meshes, including deformation at frame 19."""
import bpy,json,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/models/echo_custom';checks=[]
for i in range(563,576):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(OUT/f'EH_{i:03d}'/f'EH_{i:03d}.glb'))
    rigs=[o for o in bpy.context.scene.objects if o.type=='ARMATURE'];assert rigs,('missing rig',i)
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];skinned=[o for o in meshes if any(m.type=='ARMATURE' for m in o.modifiers)]
    assert skinned,('missing armature modifier',i)
    weighted=0
    for ob in skinned:
        for v in ob.data.vertices:
            weights=[g.weight for g in v.groups if ob.vertex_groups[g.group].name in rigs[0].data.bones]
            if not weights:continue
            assert all(math.isfinite(w) and 0<=w<=1 for w in weights)
            assert .98<=sum(weights)<=1.02,(i,ob.name,sum(weights))
            weighted+=1
    assert weighted>0
    frames=[]
    for frame in [1,19]:
        bpy.context.scene.frame_set(frame);bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
        points=[]
        for ob in skinned:
            evaluated=ob.evaluated_get(deps);mesh=evaluated.to_mesh()
            points.extend(evaluated.matrix_world@mesh.vertices[index].co for index in range(0,len(mesh.vertices),max(1,len(mesh.vertices)//300)))
            evaluated.to_mesh_clear()
        assert points and all(math.isfinite(x) for p in points for x in p)
        low=[min(p[k] for p in points) for k in range(3)];high=[max(p[k] for p in points) for k in range(3)]
        frames.append({'frame':frame,'bounds':[low,high]})
    checks.append({'id':i,'rigs':len(rigs),'bones':sum(len(r.data.bones) for r in rigs),'weighted_vertices':weighted,'frames_evaluated':frames,'passed':True})
    print('RIG_OK',i,weighted,flush=True)
(OUT/'rig_verification.json').write_text(json.dumps({'passed':len(checks),'checks':checks},indent=2))
print('ALL_RIGS_VERIFIED',len(checks),flush=True)
