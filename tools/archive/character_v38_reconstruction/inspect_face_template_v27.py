from pathlib import Path
import bpy,json
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/metahuman_conform_v23/face_v27'
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(O/'Beka_Face_BeforeV27.fbx'),use_anim=False)
face=max((o for o in bpy.data.objects if o.type=='MESH'),key=lambda o:len(o.data.vertices));r={'vertices':len(face.data.vertices),'uvs':[uv.name for uv in face.data.uv_layers],'materials':[],'matrix':list(map(list,face.matrix_world))}
coords=[face.matrix_world@v.co for v in face.data.vertices]
for mi,mat in enumerate(face.data.materials):
    ids={i for p in face.data.polygons if p.material_index==mi for i in p.vertices}
    r['materials'].append({'index':mi,'name':mat.name,'vertices':len(ids),'bounds_m':[[min(coords[i][k] for i in ids),max(coords[i][k] for i in ids)] for k in range(3)] if ids else []})
(O/'source_template_inspection.json').write_text(json.dumps(r,indent=2),encoding='utf8');print(json.dumps(r,indent=2))
