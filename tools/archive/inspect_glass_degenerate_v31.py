import bpy
from pathlib import Path
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath='D:/Projects/ECHO/unreal/EchoWard/RawAssets/EchoCarsV31/EH_1124.fbx');o=next(o for o in bpy.context.scene.objects if o.type=='MESH');m=o.data;m.calc_loop_triangles();used=set(i for t in m.loop_triangles if t.area>1e-6 for i in t.vertices)
print('COUNT',len(m.vertices),len(m.polygons),len(m.loop_triangles),'VALID',len(used));print('BOUND',[(min(v.co[j] for v in m.vertices),max(v.co[j] for v in m.vertices)) for j in range(3)]);print('VALID_BOUND',[(min(m.vertices[i].co[j] for i in used),max(m.vertices[i].co[j] for i in used)) for j in range(3)])
for t in m.loop_triangles:
 if any(m.vertices[i].co.x<-230 or m.vertices[i].co.z>105 for i in t.vertices):print('EXTREME',t.index,t.area,[(i,tuple(m.vertices[i].co)) for i in t.vertices])
