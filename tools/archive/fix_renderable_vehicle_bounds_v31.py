import bpy,json
from pathlib import Path
R=Path('D:/Projects/ECHO');p=R/'unreal/EchoWard/RawAssets/EchoCarsV31/manifest.json';d=json.loads(p.read_text());changed=[]
for row in d['assets']:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=row['file']);o=next(o for o in bpy.context.scene.objects if o.type=='MESH');me=o.data;used={i for f in me.polygons for i in f.vertices};assert used;lo=[min(me.vertices[i].co[j] for i in used) for j in range(3)];hi=[max(me.vertices[i].co[j] for i in used) for j in range(3)];new=[[lo[0],-hi[1],lo[2]],[hi[0],-lo[1],hi[2]]]
 if max(abs(a-b) for x,y in zip(new,row['bounds_cm']) for a,b in zip(x,y))>.01:changed.append(dict(id=row['id'],unused_vertices=len(me.vertices)-len(used),before=row['bounds_cm'],after=new));row['bounds_cm']=new
row_by_id={r['id']:r for r in d['assets']}
for v in d['vehicles']:
 rows=[row_by_id[v['body']]]+([row_by_id[v['glass']]] if v['glass'] else []);v['bounds_cm']=[[min(r['bounds_cm'][0][j] for r in rows) for j in range(3)],[max(r['bounds_cm'][1][j] for r in rows) for j in range(3)]]
p.write_text(json.dumps(d,ensure_ascii=False,indent=2));(R/'artifacts/echo_cars_v31/renderable_bounds_corrections.json').write_text(json.dumps(changed,indent=2));print('RENDERABLE_BOUND_CORRECTIONS',[(r['id'],r['unused_vertices']) for r in changed],flush=True)
