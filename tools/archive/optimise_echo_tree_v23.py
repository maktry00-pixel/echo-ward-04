"""Run with Blender. Keep the source intact; export a reduced UV mesh cache."""
import bpy,json,numpy as np
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/'assets/external/quality_v23/models/jacaranda_tree/jacaranda_tree_2k.gltf';O=R/'unreal/EchoWard/RawAssets/EchoRealV23/geometry';O.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(P))
parts=[]
for obj in [x for x in bpy.context.scene.objects if x.type=='MESH']:
 mod=obj.modifiers.new('EchoRealtimeReduction','DECIMATE');mod.ratio=.065;mod.use_collapse_triangulate=True
 ev=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=ev.to_mesh();mesh.calc_loop_triangles()
 verts=np.empty(len(mesh.vertices)*3,dtype=np.float32);mesh.vertices.foreach_get('co',verts);verts=verts.reshape(-1,3)
 loops=np.empty(len(mesh.loop_triangles)*3,dtype=np.int32);mesh.loop_triangles.foreach_get('loops',loops)
 tri=np.empty(len(mesh.loop_triangles)*3,dtype=np.int32);mesh.loop_triangles.foreach_get('vertices',tri)
 normals=np.empty(len(mesh.corner_normals)*3,dtype=np.float32);mesh.corner_normals.foreach_get('vector',normals);normals=normals.reshape(-1,3)[loops]
 uv=np.empty(len(mesh.loops)*2,dtype=np.float32);mesh.uv_layers.active.data.foreach_get('uv',uv);uv=uv.reshape(-1,2)[loops];uv[:,1]=1-uv[:,1]
 mi=np.empty(len(mesh.loop_triangles),dtype=np.int32);mesh.loop_triangles.foreach_get('material_index',mi)
 M=np.array(obj.matrix_world);pos=(np.c_[verts[tri],np.ones(len(tri))]@M.T)[:,:3]*100
 normals=normals@np.linalg.inv(M[:3,:3]);normals/=np.maximum(np.linalg.norm(normals,axis=1,keepdims=True),1e-9)
 # Blender world -> counter-reflected legacy OBJ. The importer flips Y back.
 pos[:,1]*=-1;normals[:,1]*=-1
 parts.append(dict(pos=pos.astype('f4'),normal=normals.astype('f4'),uv=uv.astype('f4'),material=mi,material_names=[x.name for x in mesh.materials]))
 assert len(parts)==1,'Unexpected source hierarchy'
 np.savez_compressed(O/'jacaranda_tree.npz',**parts[0]);print('TREE_REDUCED',len(mi),flush=True);ev.to_mesh_clear()
(O/'jacaranda_tree.json').write_text(json.dumps(dict(source=str(P),original_triangles=3863832,triangles=len(parts[0]['material']),ratio=.065,uv_preserved=True,modification='Decimate collapse, source unchanged')),encoding='utf8')
