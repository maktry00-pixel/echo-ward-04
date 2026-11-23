from pathlib import Path
import bpy,bmesh,json
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/metahuman_conform_v18';O.mkdir(exist_ok=True)
s=(R/'tools/character_v38_reconstruction/resolved_head_v11.py').read_text(encoding='utf8')
s=s[:s.index("skin=projection_material(")]
ns={'__file__':str(Path(__file__))};exec(compile(s,'source_preserved_topology','exec'),ns)
base=ns['head'];projectmat=ns['projection_material'];parts={};report={'source':'Photo-fitted MetaHuman geometry with standard UV preserved','game_changed':False,'likeness_approved':False,'parts':{}}
for name,mi in [('Head',0),('Teeth',1),('LeftEye',3),('RightEye',4)]:
 me=base.data.copy();ob=bpy.data.objects.new('BekaTarget_'+name,me);bpy.context.collection.objects.link(ob)
 bm=bmesh.new();bm.from_mesh(me);bmesh.ops.delete(bm,geom=[p for p in bm.faces if p.material_index!=mi],context='FACES');bmesh.ops.delete(bm,geom=[v for v in bm.verts if not v.link_faces],context='VERTS');bm.to_mesh(me);bm.free();me.update()
 parts[name]=ob;report['parts'][name]={'vertices':len(me.vertices),'polygons':len(me.polygons),'uv_layers':list(me.uv_layers.keys())}
bpy.data.objects.remove(base,do_unlink=True)
head=parts['Head'];head.data.materials.clear();head.data.materials.append(projectmat('Photo color on native MetaHuman UV',(.48,.25,.15)))
for p in head.data.polygons:p.material_index=0
for layer in head.data.uv_layers:layer.active_render=False
head.data.uv_layers.active_index=0;head.data.uv_layers[0].active_render=True
bpy.ops.object.select_all(action='DESELECT');head.select_set(True);bpy.context.view_layer.objects.active=head
atlas=bpy.data.images.new('Beka_FaceColor_nativeUV_4096',width=4096,height=4096,alpha=False)
mat=head.data.materials[0];ti=mat.node_tree.nodes.new('ShaderNodeTexImage');ti.image=atlas;mat.node_tree.nodes.active=ti
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=1;sc.render.bake.margin=12;bpy.ops.object.bake(type='EMIT')
atlas.filepath_raw=str(O/'Beka_FaceColor_nativeUV_4096.png');atlas.file_format='PNG';atlas.save();atlas.pack()
# Texture resolution is the output atlas, not an assertion of genuine new source detail.
report.update(albedo_atlas_resolution=[4096,4096],source_portrait_resolution=[1086,1448],native_4k_photo_detail=False)
for name,ob in parts.items():
 for layer in list(ob.data.uv_layers)[1:]:ob.data.uv_layers.remove(layer)
 ob.data.uv_layers.active_index=0;ob.data.uv_layers[0].active_render=True
 ob.data.materials.clear();clay=bpy.data.materials.new('Target '+name);ob.data.materials.append(clay)
 for p in ob.data.polygons:p.material_index=0
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
 bpy.ops.export_scene.fbx(filepath=str(O/('BekaTarget_'+name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',path_mode='AUTO')
report['head_expected_vertex_count_matched']=len(head.data.vertices)==24414
bpy.ops.wm.save_as_mainfile(filepath=str(O/'BekaTarget_native_topology.blend'))
(O/'template_report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('METAHUMAN_TEMPLATE_AND_NATIVE_UV_COLOR_READY',json.dumps(report),flush=True)
