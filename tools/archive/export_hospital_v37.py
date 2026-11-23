"""Export selected licensed source-library furniture. Originals stay unchanged."""
import bpy,json,hashlib
from pathlib import Path
R=Path('D:/Projects/ECHO');O=R/'unreal/EchoWard/RawAssets/HospitalV37';O.mkdir(parents=True,exist_ok=True)
lib=json.loads((R/'assets/models/hospital_v34/LIBRARY_MANIFEST.json').read_text());ids=[3300,3302,3303,3304,3305,3306,3309,3311,3312,3313,3317,3321,3322,3324,3345];out=[]
for row in lib['models']:
 if row['id'] not in ids:continue
 folder=R/'assets/models/hospital_v34'/row['slug'];bpy.ops.wm.open_mainfile(filepath=str(folder/'model.blend'))
 mats={}
 for ob in bpy.data.objects:
  if ob.type!='MESH':continue
  for i,slot in enumerate(ob.material_slots):
   m=slot.material;name=(m.name if m else '').lower();rgb=list(m.diffuse_color[:3]) if m else [.5,.5,.5]
   kind='alloy' if any(s in name for s in ('metal','steel','chrome','alumin','frame','rail')) else 'linen' if any(s in name for s in ('fabric','linen','cloth','mattress','curtain')) else 'rubber' if any(s in name for s in ('rubber','wheel','tire')) else 'dark' if max(rgb)<.12 else 'teal' if rgb[2]>rgb[0]*1.3 and rgb[1]>rgb[0]*1.2 else 'ivory'
   if kind not in mats:
    n=bpy.data.materials.new('D27_'+kind);n.diffuse_color=(.55,.56,.54,1);mats[kind]=n
   ob.material_slots[i].material=mats[kind]
  ob.select_set(True)
 meshes=[o for o in bpy.context.selected_objects if o.type=='MESH'];bpy.context.view_layer.objects.active=meshes[0];bpy.ops.object.join();ob=bpy.context.active_object
 # All existing dimensions are metres. Keep imported coordinate axes explicit.
 p=O/f'H37_{row["id"]}.obj';bpy.ops.wm.obj_export(filepath=str(p),export_selected_objects=True,apply_modifiers=True,global_scale=100,forward_axis='Y',up_axis='Z',export_materials=True,export_triangulated_mesh=True)
 from mathutils import Vector
 vertices=[list(map(float,line.split()[1:4])) for line in p.read_text(encoding='utf-8').splitlines() if line.startswith('v ')];bounds=[[min(c[i] for c in vertices) for i in range(3)],[max(c[i] for c in vertices) for i in range(3)]]
 out.append(dict(id=row['id'],slug=row['slug'],title=row['title'],file=str(p),bounds_cm=bounds,materials=sorted(mats),source=row['source'],license=row['license'],source_sha256=row.get('source_sha256',hashlib.sha256((folder/'model.glb').read_bytes()).hexdigest())))
 print('H37_EXPORT',row['id'],flush=True)
(O/'manifest.json').write_text(json.dumps(dict(version=37,assets=out),ensure_ascii=False,indent=2),encoding='utf-8')

