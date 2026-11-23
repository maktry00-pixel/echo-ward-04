import bpy,json
from pathlib import Path
O=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/cloth_tailoring_v23/photo_tailored')
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(O/'Beka_photo_tailored_v23.fbx'),use_anim=False)
ob=max((o for o in bpy.data.objects if o.type=='MESH'),key=lambda o:len(o.data.vertices));rig=ob.find_armature();weights=[sum(g.weight for g in v.groups) for v in ob.data.vertices]
r={'fbx_roundtrip':True,'vertices':len(ob.data.vertices),'triangles':sum(len(p.vertices)-2 for p in ob.data.polygons),'unweighted':sum(w<.01 for w in weights),'non_normalized':sum(abs(w-1)>.02 for w in weights),'bones':len(rig.data.bones),'root_bones':[b.name for b in rig.data.bones if not b.parent],'uv_layers':[uv.name for uv in ob.data.uv_layers],'materials':[m.name for m in ob.data.materials],'bounds_m':[[min((ob.matrix_world@v.co)[i] for v in ob.data.vertices),max((ob.matrix_world@v.co)[i] for v in ob.data.vertices)] for i in range(3)]}
original_objects=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath='D:/Projects/ECHO/artifacts/echo_hero_v38/metahuman_conform_v23/fitted_exports/Beka_Body.fbx',use_anim=False)
reference=next(o for o in bpy.data.objects if o not in original_objects and o.type=='ARMATURE')
r['reference_roots']=[b.name for b in reference.data.bones if not b.parent];r['reference_bones']=len(reference.data.bones);r['armature_node']=rig.name
r['max_rest_bone_error_m']=max(((rig.matrix_world@b.head_local)-(reference.matrix_world@reference.data.bones[b.name].head_local)).length for b in rig.data.bones)
r['passed']=r['unweighted']==0 and r['non_normalized']==0 and r['root_bones']==r['reference_roots'] and r['bones']==r['reference_bones'] and r['max_rest_bone_error_m']<1e-5 and rig.name=='root' and len(r['uv_layers'])>0
(O/'fbx_roundtrip_v26.json').write_text(json.dumps(r,indent=2),encoding='utf8');print(json.dumps(r));assert r['passed']
