import bpy,json
from pathlib import Path
R=Path('D:/Projects/ECHO');m=bpy.data.meshes.new('test');m.from_pydata([(0,0,0),(1,0,0),(0,1,0),(0,0,1)],[],[(0,1,2),(0,1,3)]);m.materials.append(bpy.data.materials.new('a'));m.materials.append(bpy.data.materials.new('b'));m.polygons[1].material_index=1;m.materials.clear();m.materials.append(bpy.data.materials['a']);m.materials.append(bpy.data.materials['b']);print('CLEAR_INDICES',[p.material_index for p in m.polygons],flush=True)
p=R/'unreal/EchoWard/RawAssets/EchoCarsV31/EH_1031.fbx';bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(p));o=next(o for o in bpy.context.scene.objects if o.type=='MESH');print('FBX_MATERIAL_FACES',[(ma.name,sum(1 for x in o.data.polygons if x.material_index==i)) for i,ma in enumerate(o.data.materials)],flush=True)
scene=json.loads((R/'artifacts/echo_cars_v31/volkswagen-crafter_scene.json').read_text());bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=scene['file']);bpy.context.scene.frame_set(0);deps=bpy.context.evaluated_depsgraph_get()
for o in bpy.context.scene.objects:
 if o.type!='MESH':continue
 ev=o.evaluated_get(deps);me=bpy.data.meshes.new_from_object(ev,depsgraph=deps);pts=[o.matrix_world@v.co for v in me.vertices];sz=[round(max(v[i] for v in pts)-min(v[i] for v in pts),2) for i in range(3)];print('CRAFTER',o.name,sz,[(x.type,x.name) for x in o.modifiers],flush=True)
