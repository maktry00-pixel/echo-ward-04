import bpy,json
from pathlib import Path
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/cloth_tailoring_v23'
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(O/'Beka_BaseTechwear_G.fbx'))
report=[]
for ob in bpy.data.objects:
    if ob.type!='MESH':continue
    parts=[]
    for mi,mat in enumerate(ob.data.materials):
        ids={i for p in ob.data.polygons if p.material_index==mi for i in p.vertices}
        if not ids:continue
        coords=[ob.matrix_world@ob.data.vertices[i].co for i in ids]
        parts.append({'index':mi,'name':mat.name,'verts':len(ids),'bounds':[[round(min(v[k] for v in coords),4),round(max(v[k] for v in coords),4)] for k in range(3)]})
    report.append({'name':ob.name,'vertices':len(ob.data.vertices),'polygons':len(ob.data.polygons),'sections':parts,'vertex_groups':len(ob.vertex_groups),'armature':ob.find_armature().name if ob.find_armature() else None})
bpy.context.preferences.filepaths.save_version=0;bpy.ops.wm.save_as_mainfile(filepath=str(O/'Actual_Techwear_G_editable.blend'))
(O/'actual_geometry_inspection.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print(json.dumps(report,indent=2))
