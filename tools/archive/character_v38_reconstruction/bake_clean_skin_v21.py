from pathlib import Path
import bpy,bmesh,json
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/metahuman_conform_v21';O.mkdir(exist_ok=True)
s=(R/'tools/character_v38_reconstruction/resolved_head_v11.py').read_text(encoding='utf8')
s=s[:s.index('skin=projection_material(')]
s=s.replace("R/'artifacts/echo_hero_v38/likeness_head_v6/approved_portrait_eyes_open.png'","R/'artifacts/echo_hero_v38/metahuman_conform_v21/Beka_clean_diffuse_portrait_v21.png'")
# AI edit preserves framing; these remain the original photographed feature projection coordinates.
# Restrict portrait below the head so neck/shoulder skin retains a smooth base instead of baked collar shadows.
s=s.replace('blend.data[v.index].color=(w,w,w,1)', 'w*=float(np.clip((p.z-1.54)/.035,0,1))\n blend.data[v.index].color=(w,w,w,1)')
ns={'__file__':str(Path(__file__)),'O':O};exec(compile(s,'native_clean_diffuse_projection','exec'),ns)
head=ns['head']
bm=bmesh.new();bm.from_mesh(head.data);bmesh.ops.delete(bm,geom=[p for p in bm.faces if p.material_index!=0],context='FACES');bmesh.ops.delete(bm,geom=[v for v in bm.verts if not v.link_faces],context='VERTS');bm.to_mesh(head.data);bm.free();head.data.update()
mat=ns['projection_material']('Beka neutral diffuse skin source',(.46,.285,.19));head.data.materials.clear();head.data.materials.append(mat)
for p in head.data.polygons:p.material_index=0
head.data.uv_layers.active_index=0;head.data.uv_layers[0].active_render=True
bpy.ops.object.select_all(action='DESELECT');head.select_set(True);bpy.context.view_layer.objects.active=head
im=bpy.data.images.new('Beka_SkinDiffuse_NativeUV_4096',width=4096,height=4096,alpha=False);node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=im;mat.node_tree.nodes.active=node
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=1;sc.render.bake.margin=12;bpy.ops.object.bake(type='EMIT');im.filepath_raw=str(O/'Beka_SkinDiffuse_NativeUV_4096.png');im.file_format='PNG';im.save()
(O/'texture_provenance.json').write_text(json.dumps({'method':'Native Blender material projection bake on standard MetaHuman UV; hair/brows removed from source using built-in imagegen, face framing retained','source_portrait':'Beka_clean_diffuse_portrait_v21.png','source_resolution':[1086,1448],'atlas_resolution':[4096,4096],'real_4k_source_detail':False,'geometry_unchanged':True}),encoding='utf8')
print('NATIVE_DIFFUSE_V21_BAKED')
