"""Inspect model geometry and movable hierarchy without executing supplied scripts."""
import bpy,json,sys,traceback
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[1];O=R/'artifacts/echo_cars_v31';d=json.loads((O/'inventory.json').read_text());rows=[]
priority={'.glb':0,'.gltf':1,'.fbx':2,'.blend':3,'.obj':4,'.usdz':5,'.ply':6}
for row in d['vehicles']:
 slug=Path(row['archive']).stem;out=O/(slug+'_scene.json')
 if out.exists():rows.append(json.loads(out.read_text()));continue
 choices=[Path(x['file']) for x in row['models']]
 if not choices:choices=list((R/'assets/models/cars/extracted_v31'/slug).rglob('*.ply'))
 if slug=='mercedes-e-class-w212':choices=sorted(choices,key=lambda p:p.name!='model.fbx')
 else:choices=sorted(choices,key=lambda p:priority.get(p.suffix.lower(),100))
 p=choices[0];bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.use_scripts_auto_execute=False
 try:
  ext=p.suffix.lower()
  if ext in ['.glb','.gltf']:bpy.ops.import_scene.gltf(filepath=str(p))
  elif ext=='.fbx':bpy.ops.import_scene.fbx(filepath=str(p),use_anim=True,ignore_leaf_bones=True)
  elif ext=='.blend':bpy.ops.wm.open_mainfile(filepath=str(p),load_ui=False,use_scripts=False)
  elif ext=='.obj':bpy.ops.wm.obj_import(filepath=str(p))
  elif ext=='.usdz':bpy.ops.wm.usd_import(filepath=str(p))
  elif ext=='.ply':bpy.ops.wm.ply_import(filepath=str(p))
  else:raise ValueError(ext)
  objects=[];points=[];tris=0
  for ob in bpy.context.scene.objects:
   entry=dict(name=ob.name,type=ob.type,parent=ob.parent.name if ob.parent else None,location=list(ob.location),rotation=list(ob.rotation_euler),scale=list(ob.scale),world_matrix=[list(x) for x in ob.matrix_world])
   if ob.type=='MESH':
    ob.data.calc_loop_triangles();n=len(ob.data.loop_triangles);tris+=n;vs=[ob.matrix_world@Vector(v) for v in ob.bound_box];points+=vs
    entry.update(triangles=n,vertices=len(ob.data.vertices),bounds_world=[[min(v[i] for v in vs) for i in range(3)],[max(v[i] for v in vs) for i in range(3)]],materials=[s.material.name if s.material else '' for s in ob.material_slots],vertex_groups=[g.name for g in ob.vertex_groups],modifiers=[(m.name,m.type) for m in ob.modifiers])
   if ob.type=='ARMATURE':entry['bones']=[dict(name=b.name,head=list(b.head_local),tail=list(b.tail_local)) for b in ob.data.bones]
   objects.append(entry)
  mats=[]
  for m in bpy.data.materials:
   images=[]
   if m.use_nodes:
    images=[dict(name=n.image.name,file=n.image.filepath,size=list(n.image.size),packed=bool(n.image.packed_file)) for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image]
   mats.append(dict(name=m.name,color=list(m.diffuse_color),metallic=m.metallic,roughness=m.roughness,images=images))
  actions=[dict(name=a.name,frames=list(a.frame_range)) for a in bpy.data.actions]
  result=dict(slug=slug,file=str(p),success=True,triangles=tris,bounds=[[min(v[i] for v in points) for i in range(3)],[max(v[i] for v in points) for i in range(3)]],objects=objects,materials=mats,actions=actions,unit_scale=bpy.context.scene.unit_settings.scale_length)
  out.write_text(json.dumps(result,ensure_ascii=False,indent=2));rows.append(result);print('CAR_SCENE',slug,'tris',tris,'objects',len(objects),'actions',actions,flush=True)
 except Exception as e:
  result=dict(slug=slug,file=str(p),success=False,error=traceback.format_exc());out.write_text(json.dumps(result));rows.append(result);print('CAR_SCENE_FAIL',slug,repr(e),flush=True)
(O/'scenes.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2))
