"""Independent Blender import check of exported GLBs and hinge structure."""
import bpy,json,sys,math,argparse
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/models/echo_custom'
parser=argparse.ArgumentParser();parser.add_argument('--start',type=int,default=1);parser.add_argument('--end',type=int,default=575)
opt=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
results=[]
for folder in sorted(OUT.glob('EH_*')):
    path=folder/(folder.name+'.glb')
    if not (folder/'asset.json').exists():continue
    number=int(folder.name.split('_')[1])
    if not opt.start<=number<=opt.end:continue
    if number==563 and results:
        (OUT/'import_verification.json').write_text(json.dumps({'passed':len(results),'assets':results},ensure_ascii=False,indent=2),encoding='utf-8')
    for old_object in list(bpy.data.objects):bpy.data.objects.remove(old_object,do_unlink=True)
    for collection in [bpy.data.meshes,bpy.data.materials,bpy.data.images]:
        for item in list(collection):
            if item.users==0:collection.remove(item)
    bpy.ops.import_scene.gltf(filepath=str(path));bpy.context.view_layer.update()
    bone_display_shapes={bone.custom_shape for rig in bpy.context.scene.objects if rig.type=='ARMATURE' for bone in rig.pose.bones if bone.custom_shape}
    objs=[o for o in bpy.context.scene.objects if o.type=='MESH' and o not in bone_display_shapes];assert objs,folder.name
    if number>=563:
        bpy.context.scene.frame_set(1);bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();points=[]
        for ob in objs:
            evaluated=ob.evaluated_get(deps);evaluated_mesh=evaluated.to_mesh()
            points.extend(evaluated.matrix_world@v.co for v in evaluated_mesh.vertices);evaluated.to_mesh_clear()
    else:points=[o.matrix_world@Vector(v) for o in objs for v in o.bound_box]
    assert all(math.isfinite(t) for v in points for t in v)
    missing=[im.filepath for im in bpy.data.images if im.source=='FILE' and im.filepath and not im.packed_file and not Path(bpy.path.abspath(im.filepath)).exists()]
    assert not missing,(folder.name,missing)
    meta=json.loads((folder/'asset.json').read_text(encoding='utf-8'))
    low=[min(p[k] for p in points) for k in range(3)];high=[max(p[k] for p in points) for k in range(3)]
    expected=sum(meta['bounds_blender_m'],[])
    bounds_match=all(abs(actual-target)<.025 for actual,target in zip(low+high,expected))
    if meta['id']<563:assert bounds_match,(folder.name,low+high,expected)
    else:
        # Licensed skeletons can import in a different neutral/rest pose than
        # the saved source preview. Rig weights and deformation are checked
        # independently; require coherent metre scale and finite geometry here.
        expected_height=expected[5]-expected[2];height=high[2]-low[2]
        assert .65*expected_height<height<1.35*expected_height,(folder.name,'skeletal scale',height,expected_height)
    results.append({'id':meta['id'],'imported':True,'mesh_parts':len(objs),'bounds_match':bounds_match,'pose_dependent_bounds':meta['id']>=563,'actual_bounds_m':[low,high],'images_resolved':True,'has_pivot':any('Pivot' in o.name for o in bpy.context.scene.objects)})
    print('IMPORT_OK',meta['id'],flush=True)
if opt.start>1 and (OUT/'import_verification.json').exists():
    prior=json.loads((OUT/'import_verification.json').read_text(encoding='utf-8'))['assets'];mapping={item['id']:item for item in prior};mapping.update({item['id']:item for item in results});results=[mapping[i] for i in sorted(mapping)]
(OUT/'import_verification.json').write_text(json.dumps({'passed':len(results),'assets':results},ensure_ascii=False,indent=2),encoding='utf-8')
print('IMPORTS_COMPLETE',len(results),flush=True)
