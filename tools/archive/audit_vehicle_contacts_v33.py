import bpy,json,numpy as np
from pathlib import Path
r=Path('D:/Projects/ECHO');u=r/'unreal/EchoWard';m=json.loads((u/'RawAssets/EchoCarsV31/manifest.json').read_text());results=[]
for row in m['assets']:
 if row['id'] not in [1001,1008,1017,1018,1029,1000,1030]:continue
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=row['file'],use_anim=False,ignore_leaf_bones=True)
 points=[];tires=[];material_names={k:m['materials'][k]['name'] for k in row['materials']}
 for ob in bpy.context.scene.objects:
  if ob.type!='MESH':continue
  mesh=ob.data;co=np.empty(len(mesh.vertices)*3,dtype=np.float64);mesh.vertices.foreach_get('co',co);co=co.reshape(-1,3);mat=np.array(ob.matrix_world);co=co@mat[:3,:3].T+mat[:3,3]
  used=np.unique(np.array([v for f in mesh.polygons for v in f.vertices]));points.append(co[used])
  slots=[i for i,v in enumerate(mesh.materials) if v and any(t in material_names.get(v.name,v.name).lower() for t in ['tyre','tire','tyer'])]
  ix=np.unique(np.array([v for f in mesh.polygons if f.material_index in slots for v in f.vertices],dtype=np.int64))
  if len(ix):tires.append(co[ix])
 a=np.concatenate(points);lo=a.min(0);hi=a.max(0);quad=[]
 for x,y in [(-1,-1),(-1,1),(1,-1),(1,1)]:
  q=a[((a[:,0]-(lo[0]+hi[0])/2)*x>=0)&((a[:,1]-(lo[1]+hi[1])/2)*y>=0)];quad.append(q[:,2].min())
 result=dict(id=row['id'],slug=row['slug'],bounds_world=[lo.tolist(),hi.tolist()],declared_bounds=row['bounds_cm'],quadrant_min_z=quad)
 if tires:
  t=np.concatenate(tires);result['tire_bounds']=[t.min(0).tolist(),t.max(0).tolist()];result['tire_ground_offset']=float(t[:,2].min()-lo[2]);result['tire_ground_offset_cm']=float(t[:,2].min()-lo[2])*(row['bounds_cm'][1][2]-row['bounds_cm'][0][2])/(hi[2]-lo[2])
 results.append(result);print('SOURCE_CONTACT',json.dumps(result),flush=True)
(r/'artifacts/echo_vehicle_fit_v33/source_wheel_contacts.json').write_text(json.dumps(results,indent=2));bpy.ops.wm.quit_blender()