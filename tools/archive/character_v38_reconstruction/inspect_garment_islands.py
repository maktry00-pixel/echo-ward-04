import bpy,json
from pathlib import Path
O=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/cloth_tailoring_v23');P=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/metahuman_conform_v23/fitted_exports/Beka_FittedCloth_0.fbx')
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(P));ob=max((o for o in bpy.data.objects if o.type=='MESH'),key=lambda o:len(o.data.vertices));coords=[ob.matrix_world@v.co for v in ob.data.vertices]
rows={}
for mi,mat in enumerate(ob.data.materials):
    polys=[p for p in ob.data.polygons if p.material_index==mi];adj={}
    for p in polys:
        for a,b in p.edge_keys:adj.setdefault(a,set()).add(b);adj.setdefault(b,set()).add(a)
    left=set(adj);islands=[]
    while left:
        seed=left.pop();group={seed};stack=[seed]
        while stack:
            for v in adj[stack.pop()]:
                if v in left:left.remove(v);group.add(v);stack.append(v)
        islands.append({'count':len(group),'bounds':[[round(min(coords[i][k] for i in group),4),round(max(coords[i][k] for i in group),4)] for k in range(3)]})
    rows[mat.name]=sorted(islands,key=lambda x:x['count'],reverse=True)
(O/'fitted_islands.json').write_text(json.dumps(rows,indent=2),encoding='utf8')
print(json.dumps(rows,indent=2))
