import bpy,json
from pathlib import Path
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/metahuman_conform_v22/techwear_source'
report={}
for name in ['G','H']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(O/('Techwear_'+name+'.fbx')))
    rows=[]
    for ob in bpy.data.objects:
        if ob.type!='MESH':continue
        sections=[]
        for mi,mat in enumerate(ob.data.materials):
            ids={i for p in ob.data.polygons if p.material_index==mi for i in p.vertices}
            if not ids:continue
            coords=[ob.matrix_world@ob.data.vertices[i].co for i in ids]
            sections.append({'index':mi,'name':mat.name,'verts':len(ids),'bounds':[[round(min(v[k] for v in coords),4),round(max(v[k] for v in coords),4)] for k in range(3)]})
        rows.append({'name':ob.name,'vertices':len(ob.data.vertices),'faces':len(ob.data.polygons),'materials':sections,'uvs':[x.name for x in ob.data.uv_layers]})
    report[name]=rows
    bpy.context.preferences.filepaths.save_version=0;bpy.ops.wm.save_as_mainfile(filepath=str(O/('Techwear_'+name+'_editable.blend')))
(O/'geometry_inspection.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print(json.dumps(report,indent=2))
